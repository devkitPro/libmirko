/*
 * Modplayer driver - gp_modplayer.c
 *
 * Copyright (C) 2003,2004 Mirko Roller <mirko@mirkoroller.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Changelog:
 *
 *  21 Aug 2004 - Mirko Roller <mirko@mirkoroller.de>
 *   first release, this player was partly rewritten to fit my needs.
 *   I dont know who wrote it first, i found it in some very old demo source.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gp32.h>
#include "modtables.h"

#define FALSE 0
#define TRUE !FALSE
#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

// As a sample is being mixed into the buffer it's position pointer is updated.
// The position pointer is a 32-bit integer that is used to store a fixed-point
// number. The following constant specifies how many of the bits should be used
// for the fractional part.
#define FRAC_BITS 10

#define NUMCHANNELS 2
#define PLAYBACK_FREQ 22050
//#define OVERSAMPLE


// This #define is used to convert an Amiga period number to a frequency.
// The freqency returned is the frequency that the sample should be played at.
//  3579545.25f / 428 = 8363.423 Hz for Middle C (PAL)
#define Period2Freq(period) (3579545.25f / (period))

// size should either be either 800 (NTSC) or 960 (PAL) - ints that is
// so 1024 was picked as a safe value for the arrays.
// these end up in the BSS section anyway so it shouldnt matter too much
#ifdef GP32
//#define SAMPLEBUFFER2  0x0C7B3000    // 2kb  MODPLAYER_LEFT_CHANNEL
//#define SAMPLEBUFFER3  0x0C7B3800    // 2kb  MODPLAYER_RIGHT_CHANNEL
static short *left  = (unsigned short*) SAMPLEBUFFER2;
static short *right = (unsigned short*) SAMPLEBUFFER3;
#else
static short   left[  1024] __attribute__((aligned (64))); // 2kb
static short  right[  1024] __attribute__((aligned (64))); // 2kb
#endif

// The NoteData structure stores the information for a single note and/or effect.
typedef struct {
 int sample_num;         // The sample number (ie "instrument") to play, 1->31
 int period_index;       // This effectively stores the frequency to play the
                         // sample, although it's actually an index into
                         // PeriodTable.
 int effect;             // Contains the effect code
 int effect_parms;       // Used to store control parameters for the
                         // various effects
} NoteData;

// RowData stores all the information for a single row. If there are 8 tracks
// in this mod then this structure will be filled with 8 elements of NoteData,
// one for each track.
typedef struct {
  int numnotes;
  NoteData *note;
} RowData;

// Pattern contains all the information for a single pattern.
// It is filled with 64 elements of type RowData.
typedef struct {
  int numrows;
  RowData *row;
} Pattern;

// The Sample structure is used to store all the information for a single
// sample, or "instrument". Most of the member's should be self-explanatory.
// The "data" member is an array of bytes containing the raw sample data
// in 8-bit signed format.
typedef struct {
 char szName[100];
 int nLength, nFineTune, nVolume, nLoopStart, nLoopLength, nLoopEnd;
 int data_length;
 char *data;
} Sample;


// TrackData is used to store ongoing information about a particular track.
typedef struct {
 int sample;         // The current sample being played (0 for none)
 int pos;            // The current playback position in the sample,
                     // stored in fixed-point format
 int period_index;   // Which note to play, stored as an index into the
                     // PeriodTable array
 int period;         // The period number that period_index corresponds to
                     // (needed for various effects)
 float freq;         // This is the actual frequency used to do the mixing.
                     // It's a combination of the value calculated from the
                     // period member and an "adjustment" made by various effects.
 int volume;         // Volume that this track is to be mixed at
 int mixvol;         // This is the actual volume used to do the mixing.
                     // It's a combination of the volume member and an
                     // "adjustment" made by various effects.
 int porta;          // Used by the porta effect, this stores the note we're
                     // porta-ing (?) to
 int portasp;        // The speed at which to porta
 int vibspe;         // Vibrato speed
 int vibdep;         // Vibrato depth
 int tremspe;        // Tremolo speed
 int tremdep;        // Tremolo depth
 int panval;         // Pan value....this player doesn't actually do panning,
                     // so this member is ignored (but included for future use)
 int sinepos;        // These next two values are pointers to the sine table.
                     // They're used to do
 int sineneg;        // various effects.
} TrackData;


//////////////////////////////////////////////////////////////////////
// Function Prototypes
//////////////////////////////////////////////////////////////////////
void ModPlayer_Load(unsigned char *data);
void ModPlay_Init(char *data);
void ModPlay_Tick();
static void SetMasterVolume(int volume);
static int Stop();
static int Pause();
static int Play();

static int ReadModWord(unsigned char *data, int index);
static void DoTremalo(int track);
static void DoVibrato(int track);
static void DoPorta(int track);
static void SlideVolume(int track, int amount);
static int VolumeTable[65][256];
static void UpdateEffects();
static int MixSubChunk(short * left, short * right, int numsamples);
static void UpdateRow();
static int MixChunk();

//////////////////////////////////////////////////////////////////////
// Global local variables
//////////////////////////////////////////////////////////////////////

// The following variables contain the music data, ie they don't change value until you load a new file
static char     m_szName[200];
static int      m_nSongLength;

static int      m_nOrders_num;
static int     *m_nOrders;
static int      m_Patterns_num;
static Pattern *m_Patterns;
static int      m_Samples_num;
static Sample  *m_Samples;

// The following variables are maintained and updated by the tracker during playback
static int m_nSpeed;             // Speed of mod being played
static int m_nOrder;             // Current order being played
static int m_nRow;               // Current row being played
static int m_nTick;              // Current tick number (there are "m_nSpeed"
                                 // ticks between each row)
static int BPM_RATE;             // Adjusted master BPM for refresh rate
static int m_nBPM;               // Beats-per-minute...controls length of
                                 // each tick
static int m_nSamplesLeft;       // Number of samples left to mix for the
                                 // current tick
static int m_nNumTracks;         // Number of tracks in this mod

static int        m_TrackDat_num;
static TrackData *m_TrackDat;    // Stores info for each track being played
static RowData   *m_CurrentRow;  // Pointer to the current row being played
static int        m_bPlaying;    // Set to true when a mod is being played

static int numbersamples_tick;   // Number of samples to send to IOP per tick
static int PAL = TRUE;
//extern int PAL;                  // true if PAL, false if NTSC

//////////////////////////////////////////////////////////////////////
// These are the public functions
//////////////////////////////////////////////////////////////////////

int gp_startmod(unsigned char *data){
    ModPlayer_Load(data);
    Play(); // Setup playing
	return 0;
}

int gp_rendermod(unsigned short *buffer) {
//int gp_rendermod() {
        int i;
        if (m_bPlaying == TRUE) {
           MixChunk();
           unsigned short *uleft  =  (unsigned short *)&left[0];
           unsigned short *uright = (unsigned short *)&right[0];
           for (i=0;i<numbersamples_tick*2;i+=2) buffer[i]=  *uleft++;
           for (i=1;i<numbersamples_tick*2;i+=2) buffer[i]= *uright++;
           return  numbersamples_tick*4; // returns in bytes, 3840 bytes
        }
        //else { Play(); return 0; } // setup to play and start playing
        else return 0;
}

//////////////////////////////////////////////////////////////////////
// Functions - Local and not public
//////////////////////////////////////////////////////////////////////

//  This is the initialiser and module loader
//  This is a general call, which loads the module from the
//  given address into the modplayer
//
//  It basically loads into an internal format, so once this function
//  has returned the buffer at 'data' will not be needed again.
void ModPlayer_Load(unsigned char *data)
{
  int i, numpatterns, row, note;
  int index = 0;
  int numsamples;
  char modname[21];

  if (PAL)
  {
    numbersamples_tick = 960/2;  // /2 = 1920 bytes, normal = 3840 Bytes
    BPM_RATE = 125;
  }
  else
  {
    numbersamples_tick = 800;
    BPM_RATE = 140;
  }

  //  Set default settings
  numsamples = 32;
  m_nNumTracks = 4;
  //  Check for diff types of mod
  if ( (data[1080]=='M') && (data[1081]=='.') && (data[1082]=='K') && (data[1083]=='.'));
  else if ( (data[1080]=='F') && (data[1081]=='L') && (data[1082]=='T') && (data[1083]=='4'));
  else if ( (data[1080]=='F') && (data[1081]=='L') && (data[1082]=='T') && (data[1083]=='8'))
    m_nNumTracks = 8;
  else if ( (data[1080]=='6') && (data[1081]=='C') && (data[1082]=='H') && (data[1083]=='N'))
    m_nNumTracks = 6;
  else if ( (data[1080]=='8') && (data[1081]=='C') && (data[1082]=='H') && (data[1083]=='N'))
    m_nNumTracks = 8;
  else
    numsamples = 16;

  //  Setup the trackdata structure
  m_TrackDat_num = m_nNumTracks;
  m_TrackDat = (TrackData *)malloc(m_TrackDat_num * sizeof(TrackData));

  // Get the name
  memcpy(modname, &data[index], 20);
  modname[20] = 0;
  strcpy(m_szName,modname);
  index += 20;

  // Read in all the instrument headers - mod files have 31, sample #0 is ignored
  m_Samples_num = numsamples;
  m_Samples = (Sample *)malloc(m_Samples_num *sizeof(Sample));
  for (i=1; i<numsamples; i++)
  {
   // Read the sample name
   char samplename[23];
   memcpy(samplename, &data[index], 22);
   samplename[22] = 0;
   strcpy(m_Samples[i].szName,samplename);
   index += 22;

   // Read remaining info about sample
   m_Samples[i].nLength = ReadModWord(data, index);index+=2;
   m_Samples[i].nFineTune = (int)(unsigned char)*(data+index); index++;
   if (m_Samples[i].nFineTune > 7)
     m_Samples[i].nFineTune -= 16;
     m_Samples[i].nVolume = (int)(unsigned char)*(data+index); index++;
     m_Samples[i].nLoopStart = ReadModWord(data, index);index+=2;
     m_Samples[i].nLoopLength = ReadModWord(data, index);index+=2;
     m_Samples[i].nLoopEnd = m_Samples[i].nLoopStart + m_Samples[i].nLoopLength;

     // Fix loop end in case it goes too far
     if (m_Samples[i].nLoopEnd > m_Samples[i].nLength)
       m_Samples[i].nLoopEnd = m_Samples[i].nLength;
  }

  // Read in song data
  m_nSongLength = (int)(unsigned char)*(data+index); index++;
  index++; // Skip over this byte, it's no longer used

  numpatterns = 0;
  m_nOrders_num = 128;
  m_nOrders = (int *)malloc(m_nOrders_num * sizeof(int));
  for (i=0; i<128; i++)
  {
    m_nOrders[i] = (int)(unsigned char)*(data+index); index++;
    if (m_nOrders[i] > numpatterns)
      numpatterns = m_nOrders[i];
  }
  numpatterns++;
  index += 4; // skip over the identifier

  // Load in the pattern data
  m_Patterns_num = numpatterns;
  m_Patterns = (Pattern *)malloc(m_Patterns_num * sizeof(Pattern));
  for (i=0; i<numpatterns; i++)
  {
    // Set the number of rows for this pattern, for mods it's always 64
    m_Patterns[i].numrows = 64;
    m_Patterns[i].row = (RowData *)malloc(m_Patterns[i].numrows * sizeof(RowData));

    // Loop through each row
    for (row=0; row<64; row++)
    {
      // Set the number of notes for this pattern
      m_Patterns[i].row[row].numnotes = m_nNumTracks;
      m_Patterns[i].row[row].note = (NoteData *)malloc(m_Patterns[i].row[row].numnotes * sizeof(NoteData));

      // Loop through each note
      for (note=0; note<m_nNumTracks; note++)
      {
        int b0,b1,b2,b3,period;
        // Get the 4 bytes for this note
        b0 = (int)(unsigned char)*(data+index);
        b1 = (int)(unsigned char)*(data+index+1);
        b2 = (int)(unsigned char)*(data+index+2);
        b3 = (int)(unsigned char)*(data+index+3); index+=4;

        // Parse them
        period = ((b0 & 0x0F) << 8) | b1 ;
        if (period)
          //m_Patterns[i].row[row].note[note].period_index = (int)((log(856) - log(period)) / log(1.007246412224) + 8);// ??
          m_Patterns[i].row[row].note[note].period_index = Period_Log_Lookup[period];
        else
          m_Patterns[i].row[row].note[note].period_index = -1;

        m_Patterns[i].row[row].note[note].sample_num = (b0 & 0xF0) | (b2 >> 4);
        m_Patterns[i].row[row].note[note].effect = b2 & 0x0F;
        m_Patterns[i].row[row].note[note].effect_parms = b3;
      }
    }
  }

  // Load in the sample data
  for (i=1; i<numsamples; i++)
  {
    int length;
    m_Samples[i].data_length = m_Samples[i].nLength;
    m_Samples[i].data = (char *)malloc(m_Samples[i].data_length + 1);

    if (m_Samples[i].nLength)
    {
      memcpy(&m_Samples[i].data[0], &data[index], m_Samples[i].nLength);
    }

    #ifdef GP32
    { // signed to unsigned
      int p=0;
      for (p=0;p<m_Samples[i].nLength;p++)
        m_Samples[i].data[p] = (m_Samples[i].data[p])+128;
    }
    #endif

    index += m_Samples[i].nLength;

    // Duplicate the last byte, we'll need an extra one in order to safely anti-alias
    length = m_Samples[i].nLength;
    if (length > 0)
    {
      m_Samples[i].data[length] = m_Samples[i].data[length-1];

      if (m_Samples[i].nLoopLength > 2)
        m_Samples[i].data[m_Samples[i].nLoopEnd] = m_Samples[i].data[m_Samples[i].nLoopStart];
    }
  }
  //  Set volume to full ready to play
  SetMasterVolume(64);
  m_bPlaying = FALSE;
}

// This function initialises for playing, and starts
int Play()
{
  int track;
  // See if I'm already playing
  if (m_bPlaying)
    return TRUE;

  // Reset all track data
  for (track=0; track<m_nNumTracks; track++)
  {
    m_TrackDat[track].sample = 0;
    m_TrackDat[track].pos = 0;
    m_TrackDat[track].period_index = 0;
    m_TrackDat[track].period = 0;
    m_TrackDat[track].volume = 0;
    m_TrackDat[track].mixvol = 0;
    m_TrackDat[track].porta = 0;
    m_TrackDat[track].portasp = 0;
    m_TrackDat[track].vibspe = 0;
    m_TrackDat[track].vibdep = 0;
    m_TrackDat[track].tremspe = 0;
    m_TrackDat[track].tremdep = 0;
    m_TrackDat[track].panval = 0;
    m_TrackDat[track].freq = 0;
    m_TrackDat[track].sinepos = 0;
    m_TrackDat[track].sineneg = 0;
  }

  // Get ready to play
  m_nSpeed = 6;
  m_nBPM = BPM_RATE;
  m_nSamplesLeft = 0;
  m_nOrder = 0;
  m_nRow = 0;
  m_CurrentRow = &m_Patterns[m_nOrders[m_nOrder]].row[m_nRow];
  m_nTick = 0;

  {
    m_bPlaying = TRUE;
    return TRUE;
  }
  // Oops...something went wrong.
  return FALSE;
}

int Pause() {
  return TRUE;
}

int Stop() {
  // Close it
  m_bPlaying = FALSE;
  return TRUE;
}

// This function mixes an entire chunk of sound which is then
// to be sent to the sound driver, in this case the IOP module.
int MixChunk()
{
//  int i,j, thissample;

  // Calculate the number of samples per beat
  //  48000 / (125 * 2 / 5) = 48000/ 50 = 960
  int samples_per_beat = PLAYBACK_FREQ / (m_nBPM * 2 / 5);

  // size should either be either 800 (NTSC) or 960 (PAL) - ints that is
  int numsamples =  numbersamples_tick;

  int thiscount;
  // Keep looping until we've filled the buffer
  int tickdata = 0;
  int samples_to_mix = numsamples;

  while (samples_to_mix)
  {
    // Only move on to the next tick if we finished mixing the last
    if (!m_nSamplesLeft)
    {
      // If we're on tick 0 then update the row
      if (m_nTick==0)
      {
        // Get this row
        m_CurrentRow = &m_Patterns[m_nOrders[m_nOrder]].row[m_nRow];

        // Set up for next row (effect might change these values later)
        m_nRow++;
        if (m_nRow>=64)
        {
          m_nRow=0;
          m_nOrder++;
          if (m_nOrder>=m_nSongLength)
            m_nOrder=0;
        }
        // Now update this row
        UpdateRow();
      }

      // Otherwise, all we gotta do is update the effects
      else
      {
        UpdateEffects();
      }

      // Move on to next tick
      m_nTick++;
      if (m_nTick>=m_nSpeed)
        m_nTick=0;

      // Set the number of samples to mix in this chunk
      m_nSamplesLeft = samples_per_beat;
    }

    // Ok, so we know that we gotta mix 'm_nSamplesLeft' samples into
    // this buffer, see how much room we actually got
    thiscount = m_nSamplesLeft;
    if (thiscount > samples_to_mix)
      thiscount = samples_to_mix;

    // Make a note that we've added this amount
    m_nSamplesLeft -= thiscount;
    samples_to_mix -= thiscount;

    // Now mix it!
    MixSubChunk(&left[tickdata], &right[tickdata], thiscount);
    tickdata += thiscount;
  }
  return TRUE;
}

// This function is called whenever a new row is encountered.
// It loops through each track, check's it's appropriate NoteData structure
// and updates the track accordingly.
void UpdateRow()
{
  int neworder = m_nOrder;
  int newrow = m_nRow;
  int track;

  // Loop through each track
  for (track=0; track<m_nNumTracks; track++)
  {
    // Get note data
    NoteData *note = &m_CurrentRow->note[track];

    // Make a copy of each value in the NoteData structure so they'll
    // be easier to work with (less typing)
    int sample = note->sample_num;
    int period = note->period_index;
    int effect = note->effect;
    int eparm  = note->effect_parms;
    int eparmx = eparm >> 4;                  // effect parameter x
    int eparmy = eparm & 0xF;                 // effect parameter y

    // Are we changing the sample being played?
    if (sample>0)
    {
      m_TrackDat[track].sample = sample;
      m_TrackDat[track].volume = m_Samples[sample].nVolume;
      m_TrackDat[track].mixvol = m_TrackDat[track].volume;
      if ((effect!=3) && (effect!=5))
        m_TrackDat[track].pos = 0;
    }

    // Are we changing the frequency being played?
    if (period >= 0)
    {
      // Remember the note
      m_TrackDat[track].period_index = note->period_index;

      // If not a porta effect, then set the channels frequency to the
      // looked up amiga value + or - any finetune
      if ((effect!=3) && (effect!=5))
      {
        int notenum = m_TrackDat[track].period_index + m_Samples[m_TrackDat[track].sample].nFineTune;
        if (notenum < 0) notenum = 0;
        if (notenum > NUMNOTES-1) notenum=NUMNOTES-1;
        m_TrackDat[track].period = PeriodTable[notenum];
      }

      // If there is no sample number or effect then we reset the position
      if ((sample==0) && (effect==0))
        m_TrackDat[track].pos = 0;

      // Now reset a few things
      m_TrackDat[track].vibspe  = 0;
      m_TrackDat[track].vibdep  = 0;
      m_TrackDat[track].tremspe = 0;
      m_TrackDat[track].tremdep = 0;
      m_TrackDat[track].sinepos = 0;
      m_TrackDat[track].sineneg = 0;
    }

    // Process any effects - need to include 1, 2, 3, 4 and A
    switch (note->effect)
    {
      // Arpeggio
      case 0x00:  break; // tick effect

      // Porta Up
      case 0x01:  break; // tick effect

      // Porta Down
      case 0x02:  break; // tick effect

      // Porta to Note (3) and Porta + Vol Slide (5)
      case 0x03:
      case 0x05:
        m_TrackDat[track].porta = PeriodTable[m_TrackDat[track].period_index+m_Samples[sample].nFineTune];
        if (eparm>0 && effect == 0x3)
        m_TrackDat[track].portasp = eparm;
        break;

      // Vibrato
      case 0x04:
        if (eparmx > 0) m_TrackDat[track].vibspe = eparmx;
        if (eparmy > 0) m_TrackDat[track].vibdep = eparmy;
        break;

      // Vibrato + Vol Slide
      case 0x06:  break;  // tick effect

      // Tremolo
      case 0x07:
        if (eparmx > 0) m_TrackDat[track].tremspe = eparmx;
        if (eparmy > 0) m_TrackDat[track].tremdep = eparmy;
        break;

      // Pan - not supported in the mixing yet
      case 0x08:
        if (eparm==0xa4)
          m_TrackDat[track].panval = 7;
        else
          m_TrackDat[track].panval = (eparm >> 3) - 1;
        if (m_TrackDat[track].panval < 0)
          m_TrackDat[track].panval = 0;
        break;

      // Sample offset
      case 0x09:
        m_TrackDat[track].pos = note->effect_parms << (FRAC_BITS + 8);
        break;

      // Volume Slide
      case 0x0A:  break; // tick effect

      // Jump To Pattern
      case 0x0B:
        neworder = note->effect_parms;
        if (neworder>=m_nSongLength)
          neworder = 0;
        newrow = 0;

      // Set Volume
      case 0x0C:
        m_TrackDat[track].volume = note->effect_parms;
        SlideVolume(track, 0);
        m_TrackDat[track].mixvol = m_TrackDat[track].volume;
        break;

      // Break from current pattern
      case 0x0D:
        newrow = eparmx*10+eparmy;
        if (newrow > 63)
          newrow =0;
        neworder = m_nOrder+1;
        if (neworder>=m_nSongLength)
          neworder=0;
        break;

      // Extended effects
      case 0x0E:
        switch (eparmx)
        {
          // Set filter
          case 0x00:  break; // not supported

          // Fine porta up
          case 0x01:
            m_TrackDat[track].period -= eparmy;
            break;

          // Fine porta down
          case 0x02:
            m_TrackDat[track].period += eparmy;
            break;

          // Glissando
          case 0x03:  break; // not supported


          // Set vibrato waveform
          case 0x04:  break; // not supported

          // Set finetune
          case 0x05:
            m_Samples[sample].nFineTune = eparmy;
            if (m_Samples[sample].nFineTune > 7)
              m_Samples[sample].nFineTune -= 16;
            break;

          // Pattern loop
          case 0x6: break; // not supported

          // Set tremolo waveform
          case 0x07:  break; // not supported

          // Pos panning - not supported in the mixing yet
          case 0x08:
            m_TrackDat[track].panval = eparmy;
            break;

          // Retrig Note
          case 0x09:  break; // tick effect

          // Fine volside up
          case 0x0A:
            SlideVolume(track, eparmy);
            m_TrackDat[track].mixvol = m_TrackDat[track].volume;
            break;

          // Fine volside down
          case 0xB:
            SlideVolume(track, -eparmy);
            m_TrackDat[track].mixvol = m_TrackDat[track].volume;
            break;

          // Cut note
          case 0x0C:  break; // tick effect

          // Delay note
          case 0x0D:  break; // not supported

          // Pattern delay
          case 0x0E:  break; // not supported

          // Invert loop
          case 0x0F:  break; // not supported
        }
        break;

      // Set Speed
      case 0x0F:
        if (eparm < 0x20)
          m_nSpeed = note->effect_parms;
        else
          m_nBPM = note->effect_parms;
        break;

      default:
        break;
    }

    // If we have something playing then set the frequency
    if (m_TrackDat[track].period > 0)
      m_TrackDat[track].freq = Period2Freq(m_TrackDat[track].period);
  }

  // Update our row and orders
  m_nRow = newrow;
  m_nOrder = neworder;
}

int MixSubChunk(short * left, short * right, int numsamples)
{
  int i,track;
  // Set up a mixing buffer and clear it
  for (i=0; i<numsamples; i++)
    left[i] = right[i] = 0;

  // Loop through each channel and process note data
  for (track=0; track<m_nNumTracks; track++)
  {
    // Make sure I'm actually playing something
    if (m_TrackDat[track].sample <= 0)
      continue;

    // Make sure this sample actually contains sound data
    if (!m_Samples[m_TrackDat[track].sample].data_length)
      continue;
    {
    // Set up for the mix loop
    short * mixed = ((track&3)==0)||((track&3)==3) ? left : right;
    unsigned char * sample = (unsigned char *)&m_Samples[m_TrackDat[track].sample].data[0];
    int nLength = m_Samples[m_TrackDat[track].sample].nLength << FRAC_BITS;
    int nLoopLength = m_Samples[m_TrackDat[track].sample].nLoopLength << FRAC_BITS;
    int nLoopEnd = m_Samples[m_TrackDat[track].sample].nLoopEnd << FRAC_BITS;
    int finetune = m_Samples[m_TrackDat[track].sample].nFineTune;
    int notenum = m_TrackDat[track].period_index+finetune;
    float freq = m_TrackDat[track].freq;
    int pos = m_TrackDat[track].pos;
    int deltapos = (int)(freq * (1 << FRAC_BITS) / PLAYBACK_FREQ);
    int * VolumeTablePtr = VolumeTable[m_TrackDat[track].mixvol];
    int mixpos = 0;
    int samples_to_mix = numsamples;
    notenum = notenum < 0 ? 0 : notenum >= NUMNOTES ? NUMNOTES-1 : notenum;

    while (samples_to_mix)
    {
      int thiscount;

      // If I'm a looping sample then I need to check if it's time to
      // loop back. I also need to figure out how many samples I can mix
      // before I need to loop again
      if (nLoopEnd > (2 << FRAC_BITS))
      {
        if (pos >= nLoopEnd)
          pos -= nLoopLength;
        thiscount = min(samples_to_mix, (nLoopEnd-pos-1) / deltapos + 1);
        // above returns the smaller parameter

        samples_to_mix -= thiscount;
      }

      // If I'm not a looping sample then mix until I'm done playing
      // the entire sample
      else
      {
        // If we've already reached the end of the sample then forget it
        if (pos >= nLength)
          thiscount = 0;
        else
          thiscount = min(numsamples, (nLength-pos-1) / deltapos + 1);
        samples_to_mix = 0;
      }

      // Inner Loop start
      for (i=0; i<thiscount; i++)
      {
        // Mix this sample in and update our position
#ifdef OVERSAMPLE
        //  This smooths the sound a bit by oversampling, but
        //  uses up more cpu
        int sample1 = VolumeTablePtr[sample[pos >> FRAC_BITS]];
        int sample2 = VolumeTablePtr[sample[(pos >> FRAC_BITS)+1]];
        int frac1 = pos & ((1 << FRAC_BITS) - 1);
        int frac2 = (1 << FRAC_BITS) - frac1;
        mixed[mixpos] += ((sample1 * frac2) + (sample2 * frac1)) >> FRAC_BITS;
        mixpos++;
#else
        //  This is normal plain mixing
        mixed[mixpos] += VolumeTablePtr[sample[pos >> FRAC_BITS]];
        mixpos++;
#endif
        pos += deltapos;
      }
      // Inner Loop end
    }
    // Save current position
    m_TrackDat[track].pos = pos;
    }
  }

  return TRUE;
}

void UpdateEffects()
{
  int track;
  // Loop through each channel
  for (track=0; track<m_nNumTracks; track++)
  {
    // Get note data
    NoteData * note = &m_CurrentRow->note[track];

    // Parse it
    int effect = note->effect;                // grab the effect number
    int eparm  = note->effect_parms;          // grab the effect parameter
    int eparmx = eparm >> 4;                  // grab the effect parameter x
    int eparmy = eparm & 0xF;                 // grab the effect parameter y

    // Process it
    switch (effect)
    {
      // Arpeggio
      case 0x00:
        if (eparm > 0)
        {
          int notenum,period;
          switch (m_nTick%3)
          {
          case 0:
            period = m_TrackDat[track].period;
            break;
          case 1:
            notenum = m_TrackDat[track].period_index + 8*eparmx + m_Samples[m_TrackDat[track].sample].nFineTune;
            period = PeriodTable[notenum];
            break;
          case 2:
            notenum = m_TrackDat[track].period_index + 8*eparmy + m_Samples[m_TrackDat[track].sample].nFineTune;
            period = PeriodTable[notenum];
            break;
          default:
            period = 0;
            break;
          }
          m_TrackDat[track].freq = Period2Freq(period);
        }
        break;


      // Porta up
      case 0x01:
        m_TrackDat[track].period -= eparm;           // subtract freq
        if (m_TrackDat[track].period < 54)
          m_TrackDat[track].period = 54;  // stop at C-5
        m_TrackDat[track].freq = Period2Freq(m_TrackDat[track].period);
        break;

      // Porta down
      case 0x02:
        m_TrackDat[track].period += eparm;           // add freq
        m_TrackDat[track].freq = Period2Freq(m_TrackDat[track].period);
        break;

      // Porta to note
      case 0x03:
        DoPorta(track);
        break;

      // Vibrato
      case 0x04:
        DoVibrato(track);
        break;

      // Porta + Vol Slide
      case 0x05:
        DoPorta(track);
        SlideVolume(track, eparmx - eparmy);
        m_TrackDat[track].mixvol = m_TrackDat[track].volume;
        break;

      // Vibrato + Vol Slide
      case 0x06:
        DoVibrato(track);
        SlideVolume(track, eparmx - eparmy);
        m_TrackDat[track].mixvol = m_TrackDat[track].volume;
        break;

      // Tremolo
      case 0x07:
        DoTremalo(track);
        break;

      // Pan
      case 0x08:  break; // note effect

      // Sample offset
      case 0x09:  break; // note effect

      // Volume slide
      case 0x0A:
        SlideVolume(track, eparmx - eparmy);
        m_TrackDat[track].mixvol = m_TrackDat[track].volume;
        break;

      // Jump To Pattern
      case 0x0B:  break; // note effect

      // Set Volume
      case 0x0C:  break; // note effect

      // Pattern Break
      case 0x0D:  break; // note effect

      // Extended effects
      case 0x0E:
        switch(eparmx)
        {
          // Retrig note
          case 0x9: break; // not supported

          // Cut note
          case 0xC:
            if (m_nTick==eparmy)
            {
              m_TrackDat[track].volume = 0;
              m_TrackDat[track].mixvol = m_TrackDat[track].volume;
            }
            break;

          // Delay note
          case 0xD: break; // not supported

          // All other Exy effects are note effects
        }

      // Set Speed
      case 0x0F:  break; // note effect
    }
  }
}

void SlideVolume(int track, int amount)
{
  amount += m_TrackDat[track].volume;
  m_TrackDat[track].volume = amount < 0 ? 0 : amount > 64 ? 64 : amount;
}

void DoPorta(int track)
{
  if (m_TrackDat[track].period < m_TrackDat[track].porta)
  {
    m_TrackDat[track].period += m_TrackDat[track].portasp;
    if (m_TrackDat[track].period > m_TrackDat[track].porta)
      m_TrackDat[track].period = m_TrackDat[track].porta;
  }
  else if (m_TrackDat[track].period > m_TrackDat[track].porta)
  {
    m_TrackDat[track].period -= m_TrackDat[track].portasp;
    if (m_TrackDat[track].period < m_TrackDat[track].porta)
      m_TrackDat[track].period = m_TrackDat[track].porta;
  }
  m_TrackDat[track].freq = Period2Freq(m_TrackDat[track].period);
}

void DoVibrato(int track)
{
  int vib = m_TrackDat[track].vibdep*sintab[m_TrackDat[track].sinepos] >> 7; // div 128
  if (m_TrackDat[track].sineneg == 0)
    m_TrackDat[track].freq = Period2Freq(m_TrackDat[track].period + vib);
  else
    m_TrackDat[track].freq = Period2Freq(m_TrackDat[track].period - vib);
  m_TrackDat[track].sinepos += m_TrackDat[track].vibspe;
  if (m_TrackDat[track].sinepos > 31)
  {
    m_TrackDat[track].sinepos -=32;
    m_TrackDat[track].sineneg = ~m_TrackDat[track].sineneg;   // flip pos/neg flag
  }
}

void DoTremalo(int track)
{
  int vib = m_TrackDat[track].tremdep*sintab[m_TrackDat[track].sinepos] >> 6; // div64
  if (m_TrackDat[track].sineneg == 0)
  {
    if (m_TrackDat[track].volume+vib > 64)
      vib = 64-m_TrackDat[track].volume;
    m_TrackDat[track].mixvol = m_TrackDat[track].volume+vib;
  }
  else
  {
    if (m_TrackDat[track].volume-vib < 0)
      vib = m_TrackDat[track].volume;
    m_TrackDat[track].mixvol = m_TrackDat[track].volume+vib;
  }

  m_TrackDat[track].sinepos += m_TrackDat[track].tremspe;
  if (m_TrackDat[track].sinepos > 31)
  {
    m_TrackDat[track].sinepos -=32;
    m_TrackDat[track].sineneg = ~m_TrackDat[track].sineneg;     // flip pos/neg flag
  }
}

// Sets the master volume for the mod being played.
// The value should be from 0 (silence) to 64 (max volume)
void SetMasterVolume(int volume)
{
  int i,j;
  for (i=0; i<65; i++)
    for (j=0; j<256; j++)
      VolumeTable[i][j] = volume * i * (int)(char)j / 64;
}

// 16-bit word values in a mod are stored in the Motorola
// Most-Significant-Byte-First format.
// They're also stored at half their actual value, thus doubling their range.
// This function accepts a pointer to such a word and returns it's integer value
// NOTE: relic from pc testing.
int ReadModWord(unsigned char * data, int index)
{
  int byte1 = (int)(unsigned char)*(data+index);
  int byte2 = (int)(unsigned char)*(data+index+1);
  return ((byte1 * 256) + byte2) * 2;
}
