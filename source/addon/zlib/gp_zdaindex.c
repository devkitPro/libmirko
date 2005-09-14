/*
 * ZDA Container driver - gp_zdaindex.c
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
#include <zda.h>

int gp_zda_headersize(char *zdadata) {
//    int i;
    ZDAHEAD *head;
    head = (ZDAHEAD*)zdadata;

    // check for "ZDA"
    if ((head->name[0]==0x5a) && (head->name[1]==0x44) && (head->name[2]==0x41) &&  (head->name[3]==0x00) )
      return head->size;
    return 0; // Head incorrect
}

int gp_zda_csize(char *zdadata, char *filename) {
    int i;
    ZDAHEAD *head;
    head = (ZDAHEAD*)zdadata;

    // check for "ZDA"
    if ((head->name[0]==0x5a) && (head->name[1]==0x44) && (head->name[2]==0x41) &&  (head->name[3]==0x00) ) {
         ZDAINDEX *index;
         for (i=0;i<head->records;i++) {
           index = (ZDAINDEX*)(zdadata+12+(i*52)); // pointer to first index
           if (!strcmp (filename,index->filename) ) return index->size_compressed;
         }
    }
    return 0; // Head incorrect
}

int gp_zda_usize(char *zdadata, char *filename) {
    int i;
    ZDAHEAD *head;
    head = (ZDAHEAD*)zdadata;

    // check for "ZDA"
    if ((head->name[0]==0x5a) && (head->name[1]==0x44) && (head->name[2]==0x41) &&  (head->name[3]==0x00) ) {
         ZDAINDEX *index;
         for (i=0;i<head->records;i++) {
           index = (ZDAINDEX*)(zdadata+12+(i*52)); // pointer to first index
           if (!strcmp (filename,index->filename) ) return index->size_uncompressed;
         }
    }
    return 0; // Head incorrect
}

int gp_zda_offset(char *zdadata, char *filename) {
    int i;
    ZDAHEAD *head;
    head = (ZDAHEAD*)zdadata;

    // check for "ZDA"
    if ((head->name[0]==0x5a) && (head->name[1]==0x44) && (head->name[2]==0x41) &&  (head->name[3]==0x00) ) {
         ZDAINDEX *index;
         for (i=0;i<head->records;i++) {
           index = (ZDAINDEX*)(zdadata+12+(i*52)); // pointer to first index
           if (!strcmp (filename,index->filename) )  return ((index->start_offset)+(head->size));
         }
    }
    return 0; // Head incorrect
}

