/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:     mpegdemux.h                                                *
 * Created:       2003-02-01 by Hampa Hug <hampa@hampa.ch>                   *
 * Last modified: 2003-03-02 by Hampa Hug <hampa@hampa.ch>                   *
 * Copyright:     (C) 2003 by Hampa Hug <hampa@hampa.ch>                     *
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

/* $Id: mpegdemux.h,v 1.6 2003/03/02 11:19:49 hampa Exp $ */


#ifndef MPEGDEMUX_H
#define MPEGDEMUX_H 1


#include "config.h"


#define PAR_STREAM_EXCLUDE 1
#define PAR_STREAM_SEEN    2


extern unsigned char par_stream[256];
extern unsigned char par_substream[256];
extern int           par_one_shdr;
extern int           par_one_pack;
extern int           par_one_end;
extern unsigned char par_first;
extern int           par_dvdac3;
extern char          *par_demux_name;


int mpeg_stream_mark (unsigned char sid, unsigned char ssid);


#endif
