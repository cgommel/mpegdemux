/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:     mpeg_parse.h                                               *
 * Created:       2003-02-01 by Hampa Hug <hampa@hampa.ch>                   *
 * Last modified: 2003-02-02 by Hampa Hug <hampa@hampa.ch>                   *
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

/* $Id: mpeg_parse.h,v 1.1 2003/02/02 20:26:13 hampa Exp $ */


#ifndef MPEG_PARSE_H
#define MPEG_PARSE_H 1


#include <stdio.h>


#define MPEG_DEMUX_BUFFER 256

#define MPEG_END_CODE      0x01b9
#define MPEG_PACK_START    0x01ba
#define MPEG_SYSTEM_HEADER 0x01bb
#define MPEG_PACKET_START  0x0001


typedef struct mpeg_demux_t {
  int                close;
  int                free;

  FILE               *fp;

  unsigned long long ofs;

  unsigned           buf_i;
  unsigned           buf_n;
  unsigned char      buf[MPEG_DEMUX_BUFFER];

  unsigned long long pack_scr;
  unsigned long      pack_mux_rate;

  unsigned           sh_size;
  int                sh_fixed;
  int                sh_csps;

  unsigned           packet_stm_id;
  unsigned           packet_size;
  unsigned           packet_offset;
  unsigned long long packet_pts;

  void               *ext;

  int (*mpeg_skip) (struct mpeg_demux_t *mpeg);
  int (*mpeg_pack) (struct mpeg_demux_t *mpeg);
  int (*mpeg_system_header) (struct mpeg_demux_t *mpeg);
  int (*mpeg_packet) (struct mpeg_demux_t *mpeg);
  int (*mpeg_end) (struct mpeg_demux_t *mpeg);
} mpeg_demux_t;


mpeg_demux_t *mpegd_open_fp (mpeg_demux_t *mpeg, FILE *fp, int close);
mpeg_demux_t *mpegd_open (mpeg_demux_t *mpeg, const char *fname);
void mpegd_close (mpeg_demux_t *mpeg);
unsigned long mpegd_get_bits (mpeg_demux_t *mpeg, unsigned i, unsigned n);
int mpegd_skip (mpeg_demux_t *mpeg, unsigned n);
int mpegd_read (mpeg_demux_t *mpeg, void *buf, unsigned n);
int mpegd_parse (mpeg_demux_t *mpeg);


#endif
