/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:     mpegdemux.h                                                *
 * Created:       2003-02-01 by Hampa Hug <hampa@hampa.ch>                   *
 * Last modified: 2004-10-12 by Hampa Hug <hampa@hampa.ch>                   *
 * Copyright:     (C) 2003-2004 Hampa Hug <hampa@hampa.ch>                   *
 *****************************************************************************/

/*****************************************************************************
 * This program is free software. You can redistribute it and / or modify it *
 * under the terms of the GNU General Public License version 2 as  published *
 * by the Free Software Foundation.                                          *
 *                                                                           *
 * This program is distributed in the hope  that  it  will  be  useful,  but *
 * WITHOUT  ANY   WARRANTY,   without   even   the   implied   warranty   of *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU  General *
 * Public License for more details.                                          *
 *****************************************************************************/

/* $Id$ */


#ifndef MPEGDEMUX_H
#define MPEGDEMUX_H 1


#include "config.h"


#define PAR_STREAM_SELECT  0x01
#define PAR_STREAM_INVALID 0x02

#define PAR_MODE_SCAN  0
#define PAR_MODE_LIST  1
#define PAR_MODE_REMUX 2
#define PAR_MODE_DEMUX 3


extern unsigned char par_stream[256];
extern unsigned char par_substream[256];
extern unsigned char par_invalid[256];
extern int           par_no_shdr;
extern int           par_no_pack;
extern int           par_no_packet;
extern int           par_no_end;
extern int           par_empty_pack;
extern int           par_split;
extern int           par_drop;
extern int           par_scan;
extern int           par_first_pts;
extern int           par_dvdac3;
extern int           par_dvdsub;
extern char          *par_demux_name;


char *mpeg_get_name (const char *base, unsigned sid);
int mpeg_stream_excl (unsigned char sid, unsigned char ssid);
int mpeg_packet_check (mpeg_demux_t *mpeg);
void mpeg_print_stats (mpeg_demux_t *mpeg, FILE *fp);
int mpeg_copy (mpeg_demux_t *mpeg, FILE *fp, unsigned n);


#endif
