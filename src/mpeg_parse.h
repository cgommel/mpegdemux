/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:     mpeg_parse.h                                               *
 * Created:       2003-02-01 by Hampa Hug <hampa@hampa.ch>                   *
 * Last modified: 2003-03-07 by Hampa Hug <hampa@hampa.ch>                   *
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

/* $Id: mpeg_parse.h,v 1.10 2003/03/07 08:16:10 hampa Exp $ */


#ifndef MPEG_PARSE_H
#define MPEG_PARSE_H 1


#include "config.h"

#include <stdio.h>


#define MPEG_DEMUX_BUFFER 256

#define MPEG_END_CODE      0x01b9
#define MPEG_PACK_START    0x01ba
#define MPEG_SYSTEM_HEADER 0x01bb
#define MPEG_PACKET_START  0x0001


typedef struct {
  unsigned long      packet_cnt;
  unsigned long long size;
} mpeg_stream_info_t;

typedef struct {
  unsigned size;
  int      fixed;
  int      csps;
} mpeg_shdr_t;

typedef struct {
  unsigned           type;
  unsigned           sid;
  unsigned           ssid;
  unsigned           size;
  unsigned           offset;
  unsigned long long pts;
} mpeg_packet_t;

typedef struct {
  unsigned           size;
  unsigned           type;
  unsigned long long scr;
  unsigned long      mux_rate;
  unsigned           stuff;
} mpeg_pack_t;

typedef struct mpeg_demux_t {
  int                close;
  int                free;

  FILE               *fp;

  unsigned long long ofs;

  unsigned           buf_i;
  unsigned           buf_n;
  unsigned char      buf[MPEG_DEMUX_BUFFER];

  mpeg_shdr_t        shdr;
  mpeg_packet_t      packet;
  mpeg_pack_t        pack;

  unsigned long      shdr_cnt;
  unsigned long      pack_cnt;
  unsigned long      packet_cnt;
  unsigned long      end_cnt;
  unsigned long      skip_cnt;
  mpeg_stream_info_t streams[256];
  mpeg_stream_info_t substreams[256];

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
void mpegd_reset_stats (mpeg_demux_t *mpeg);
unsigned long mpegd_get_bits (mpeg_demux_t *mpeg, unsigned i, unsigned n);
int mpegd_skip (mpeg_demux_t *mpeg, unsigned n);
int mpegd_read (mpeg_demux_t *mpeg, void *buf, unsigned n);
int mpegd_set_offset (mpeg_demux_t *mpeg, unsigned long long ofs);
int mpegd_parse (mpeg_demux_t *mpeg);


#endif
