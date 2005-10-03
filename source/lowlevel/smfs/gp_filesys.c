/*
 * smc wrapper - gp_filesys.c
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
 *   first release
 *
 */


#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "fileio.h"
#include "smf_fat.h"

#define F_HANDLE long

/*typedef struct {
   char name[16];
} sDIR_ENTRY;
*/
static
char smcInit_OK=0;

static
char smc_init() {
     if (smcInit_OK == 0) {
        int err;
        err=smInit();  // 0=OK
        if (err == 0 )  smcInit_OK=1;
     }
return  smcInit_OK;
}

//  DIR dir_struct;
//  dir_files=sm_dir("dev0:\\mp3\\",&dir_struct);
int smc_dir(char *dirname, DIR *dir_list )   {
        int err;
        unsigned int size;
        int ent_start=0;
        unsigned int entry=0;
        unsigned int count=0;
        sDIR_ENTRY dent;

        if (!(smc_init())) return 0;

        err=smGetListNumDir(dirname,&entry); // 0=OK
        if (err >0 ) return 0;

        if (entry > 128) entry=128;
        while (ent_start != entry) {
          smReadDir(dirname, ent_start, 1, &dent, &count);ent_start++;
          strcpy(dir_list->name[ent_start-1],dent.name);

          {char buffer[32+16];sprintf (buffer,"%s%s",dirname,dent.name);
           smGetSizeFile(buffer,&size);
           dir_list->size[ent_start-1]=size;
          }
        }
return entry;
}

int smc_read(char *filename,void *dest,int offset,int size) {

        int oflags=1; // OPEN_R
        F_HANDLE handle;
        unsigned int read_count;

        if (!(smc_init())) return 0;

        if ( smOpenFile(filename, oflags, &handle) != 0) return 0;

         if (offset > 0) {
            int sm_offset = -1;
            int mode = 1;  // FROM_BEGIN
            smSeekFile((F_HANDLE)handle, mode, offset, &sm_offset);
         }

        smReadFile((F_HANDLE)handle, dest, size, &read_count);
        smCloseFile(handle);
return read_count;
}

int smc_write(char *filename,void *dest,int size) {

	F_HANDLE handle;
	int fcreate_mode=1;  // ALWAYS_CREATE

	if (!(smc_init())) return 0;

	smCreateFile(filename, fcreate_mode, &handle);
	smWriteFile(handle, dest, size);
	smCloseFile(handle);

	return 0;	
}

int smc_createdir(char *dirname) {
        if (!(smc_init())) return 0;
        smCreateDir(dirname, 0);
        return 1;
}


// If you want to add new functions, do not base them on the Samsung smc lib.
// I write a new SMC lib soon.

GPFILE *smc_fopen(const char *path, const char *mode) {
   unsigned int  size;
   int  err=0;
   char buffer[8];
   GPFILE *gpfile;

   if (!(smc_init())) return 0;
   gpfile = malloc (sizeof(GPFILE));

   if (mode[0]==114) {   // "r"
         err=smc_read((char*)path,buffer,0,1);
         if (err ==1) { // ok
             smGetSizeFile(path,&size);
             strcpy(gpfile->path,path);
             gpfile->pos =0;
             gpfile->size=size;
             gpfile->mode=114;
             return gpfile;
         }
   }
   if (mode[0]==119) {   // "w"
             strcpy(gpfile->path,path);
             gpfile->pos =0;
             gpfile->size=0;
             gpfile->mode=119;
             gpfile->wbuffer=NULL;
             return gpfile;
   }
   free (gpfile);
   return NULL;
}

size_t smc_fread(void *ptr, size_t size, size_t nmemb, GPFILE *stream){
   int err=0;
   char *buffer;
   buffer = ptr;

   if (!(smc_init())) return 0;
   if ( (stream->mode) == 114) {
     if (stream) {
        err=smc_read(stream->path,buffer,stream->pos,size*nmemb);
        stream->pos = (stream->pos) + size*nmemb;
     }
   }
   return err;
}

size_t smc_fwrite(void *ptr, size_t size, size_t nmemb, GPFILE *stream){
   char *buffer;
   buffer=ptr;
   size = size*nmemb;

   if (!(smc_init())) return 0;
   if ( (stream->mode) == 119) {
      (stream->wbuffer) = realloc((stream->wbuffer),((stream->size)+size) );
      memcpy((stream->wbuffer)+stream->size, buffer, size);
      stream->size = (stream->size)+size;
      return size;
   }
   return 0;
}


int smc_fclose(GPFILE *stream) {
   if ( (stream->mode) == 119) {
      smc_write(stream->path,stream->wbuffer,stream->size);
      free (stream->wbuffer);
   }
   if (stream) free (stream);
   return 0;
}

int smc_fseek(GPFILE *stream, long offset, int whence){
   if (stream) {
      if (whence == SEEK_SET) stream->pos = offset;
      if (whence == SEEK_END) stream->pos = stream->size;
      if (whence == SEEK_CUR) stream->pos = (stream->pos) + offset;
    }
    return 0;
}

long smc_ftell(GPFILE *stream){
   if (stream) return stream->pos;
   else return 0;
}

void smc_rewind(GPFILE *stream) {
   if (stream) stream->pos=0;
}

int smc_filesize(GPFILE *stream) {
   if (stream) return stream->size;
	return -1;
}

