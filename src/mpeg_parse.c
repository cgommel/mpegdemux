/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:     mpeg_parse.c                                               *
 * Created:       2003-02-01 by Hampa Hug <hampa@hampa.ch>                   *
 * Last modified: 2003-02-03 by Hampa Hug <hampa@hampa.ch>                   *
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

/* $Id: mpeg_parse.c,v 1.4 2003/02/03 20:58:34 hampa Exp $ */


#include <stdlib.h>

#include "message.h"
#include "mpeg_parse.h"


mpeg_demux_t *mpegd_open_fp (mpeg_demux_t *mpeg, FILE *fp, int close)
{
  if (mpeg == NULL) {
    mpeg = (mpeg_demux_t *) malloc (sizeof (mpeg_demux_t));
    if (mpeg == NULL) {
      return (NULL);
    }
    mpeg->free = 1;
  }
  else {
    mpeg->free = 0;
  }

  mpeg->fp = fp;
  mpeg->close = close;

  mpeg->ofs = 0;

  mpeg->buf_i = 0;
  mpeg->buf_n = 0;

  mpeg->ext = NULL;

  mpeg->mpeg_skip = NULL;
  mpeg->mpeg_system_header = NULL;
  mpeg->mpeg_packet = NULL;
  mpeg->mpeg_pack = NULL;
  mpeg->mpeg_end = NULL;

  mpegd_reset_stats (mpeg);

  return (mpeg);
}

mpeg_demux_t *mpegd_open (mpeg_demux_t *mpeg, const char *fname)
{
  FILE *fp;

  fp = fopen (fname, "rb");
  if (fp == NULL) {
    return (NULL);
  }

  mpeg = mpegd_open_fp (mpeg, fp, 1);

  return (mpeg);
}

void mpegd_close (mpeg_demux_t *mpeg)
{
  if (mpeg->close) {
    fclose (mpeg->fp);
  }

  if (mpeg->free) {
    free (mpeg);
  }
}

void mpegd_reset_stats (mpeg_demux_t *mpeg)
{
  unsigned i;

  mpeg->shdr_cnt = 0;
  mpeg->pack_cnt = 0;
  mpeg->packet_cnt = 0;
  mpeg->skip_cnt = 0;

  for (i = 0; i < 256; i++) {
    mpeg->streams[i].packet_cnt = 0;
    mpeg->streams[i].size = 0;
  }
}

static
int mpegd_buffer_fill (mpeg_demux_t *mpeg)
{
  unsigned i, n;
  size_t   r;

  if ((mpeg->buf_i > 0) && (mpeg->buf_n > 0)) {
    for (i = 0; i < mpeg->buf_n; i++) {
      mpeg->buf[i] = mpeg->buf[mpeg->buf_i + i];
    }
  }

  mpeg->buf_i = 0;

  n = MPEG_DEMUX_BUFFER - mpeg->buf_n;

  if (n > 0) {
    r = fread (mpeg->buf + mpeg->buf_i, 1, n, mpeg->fp);
    if (r < 0) {
      return (1);
    }

    mpeg->buf_n += (unsigned) r;
  }

  return (0);
}

static
int mpegd_need_bits (mpeg_demux_t *mpeg, unsigned n)
{
  n = (n + 7) / 8;

  if (n > mpeg->buf_n) {
    mpegd_buffer_fill (mpeg);
  }

  if (n > mpeg->buf_n) {
    return (1);
  }

  return (0);
}

unsigned long mpegd_get_bits (mpeg_demux_t *mpeg, unsigned i, unsigned n)
{
  unsigned long r;
  unsigned long v, m;
  unsigned      b_i, b_n;

  if (mpegd_need_bits (mpeg, i + n)) {
    return (0);
  }

  i += 8 * mpeg->buf_i;
  r = 0;

  while (n > 0) {
    b_n = 8 - (i & 7);
    if (b_n > n) {
      b_n = n;
    }

    b_i = 8 - (i & 7) - b_n;

    m = (1 << b_n) - 1;
    v = (mpeg->buf[i >> 3] >> b_i) & m;

    r = (r << b_n) | v;

    i += b_n;
    n -= b_n;
  }

  return (r);
}

int mpegd_skip (mpeg_demux_t *mpeg, unsigned n)
{
  size_t r;

  mpeg->ofs += n;

  if (n <= mpeg->buf_n) {
    mpeg->buf_i += n;
    mpeg->buf_n -= n;
    return (0);
  }

  n -= mpeg->buf_n;
  mpeg->buf_i = 0;
  mpeg->buf_n = 0;

  while (n > 0) {
    if (n <= MPEG_DEMUX_BUFFER) {
      r = fread (mpeg->buf, 1, n, mpeg->fp);
    }
    else {
      r = fread (mpeg->buf, 1, MPEG_DEMUX_BUFFER, mpeg->fp);
    }

    if (r <= 0) {
      return (1);
    }

    n -= (unsigned) r;
  }

  return (0);
}

void mpegd_set_offset (mpeg_demux_t *mpeg, unsigned long long ofs)
{
  if (ofs > mpeg->ofs) {
    mpegd_skip (mpeg, (unsigned long) (ofs - mpeg->ofs));
  }
}

int mpegd_read (mpeg_demux_t *mpeg, void *buf, unsigned n)
{
  unsigned      i, j;
  unsigned char *tmp;
  size_t        r;

  mpeg->ofs += n;

  tmp = (unsigned char *) buf;

  if (n <= mpeg->buf_n) {
    j = n;
  }
  else {
    j = mpeg->buf_n;
  }

  if (j > 0) {
    for (i = 0; i < j; i++) {
      tmp[i] = mpeg->buf[mpeg->buf_i + i];
    }

    mpeg->buf_i += j;
    mpeg->buf_n -= j;
    n -= j;
  }

  if (n > 0) {
    r = fread (tmp + j, 1, n , mpeg->fp);
    if (r != n) {
      return (1);
    }
  }

  return (0);
}

static
int mpegd_seek_header (mpeg_demux_t *mpeg, int force)
{
  while (mpegd_get_bits (mpeg, 0, 24) == 0) {
    mpeg->skip_cnt += 1;

    if (mpeg->mpeg_skip != NULL) {
      if (mpeg->mpeg_skip (mpeg)) {
        return (1);
      }
    }

    if (mpegd_skip (mpeg, 1)) {
      return (1);
    }
  }

  if (force) {
    while (mpegd_get_bits (mpeg, 0, 24) != 1) {
      mpeg->skip_cnt += 1;

      if (mpeg->mpeg_skip != NULL) {
        if (mpeg->mpeg_skip (mpeg)) {
          return (1);
        }
      }

      if (mpegd_skip (mpeg, 1)) {
        return (1);
      }
    }
  }

  return (0);
}

static
int mpegd_parse_system_header (mpeg_demux_t *mpeg)
{
  unsigned long long ofs;

  mpeg->sh_size = mpegd_get_bits (mpeg, 32, 16);

  mpeg->sh_fixed = mpegd_get_bits (mpeg, 78, 1);
  mpeg->sh_csps = mpegd_get_bits (mpeg, 79, 1);

  mpeg->shdr_cnt += 1;

  ofs = mpeg->ofs + 6 + mpeg->sh_size;

  if (mpeg->mpeg_system_header != NULL) {
    if (mpeg->mpeg_system_header (mpeg)) {
      return (1);
    }
  }

  mpegd_set_offset (mpeg, ofs);

  return (0);
}

static
int mpegd_parse_packet (mpeg_demux_t *mpeg)
{
  unsigned           i;
  unsigned           sid;
  unsigned long long pts;
  unsigned long long ofs;

  mpeg->packet_type = 0;

  sid = mpegd_get_bits (mpeg, 24, 8);
  mpeg->packet_size = mpegd_get_bits (mpeg, 32, 16);

  pts = 0;

  i = 48;
  if (((sid >= 0xc0) && (sid < 0xf0)) || (sid == 0xbd)) {
    while (mpegd_get_bits (mpeg, i, 8) == 0xff) {
      if (i > (48 + 16 * 8)) {
        break;
      }
      i += 8;
    }

    if (mpegd_get_bits (mpeg, i, 2) == 0x01) {
      unsigned val;

      mpeg->packet_type = 1;

      i += 16;

      val = mpegd_get_bits (mpeg, i, 8);

      if ((val & 0xf0) == 0x20) {
        pts = mpegd_get_bits (mpeg, i + 4, 3);
        pts = (pts << 15) | mpegd_get_bits (mpeg, i + 8, 15);
        pts = (pts << 15) | mpegd_get_bits (mpeg, i + 24, 15);
        i += 40;
      }
      else if ((val & 0xf0) == 0x30) {
        pts = mpegd_get_bits (mpeg, i + 4, 3);
        pts = (pts << 15) | mpegd_get_bits (mpeg, i + 8, 15);
        pts = (pts << 15) | mpegd_get_bits (mpeg, i + 24, 15);
        i += 80;
      }
      else if (val == 0x0f) {
        i += 8;
      }
    }
    else if (mpegd_get_bits (mpeg, i, 2) == 0x02) {
      int      pts_dts_flag;
      unsigned cnt;

      mpeg->packet_type = 2;

      pts_dts_flag = mpegd_get_bits (mpeg, i + 8, 2);
      cnt = mpegd_get_bits (mpeg, i + 16, 8);

      if (pts_dts_flag & 2) {
        if (mpegd_get_bits (mpeg, i + 24, 4) == 0x02) {
          pts = mpegd_get_bits (mpeg, i + 28, 3);
          pts = (pts << 15) | mpegd_get_bits (mpeg, i + 32, 15);
          pts = (pts << 15) | mpegd_get_bits (mpeg, i + 48, 15);
        }
      }

      i += 8 * (cnt + 3);
    }
  }

  mpeg->packet_stm_id = sid;
  mpeg->packet_pts = pts;
  mpeg->packet_offset = i / 8;

  mpeg->packet_cnt += 1;
  mpeg->streams[sid].packet_cnt += 1;
  mpeg->streams[sid].size += mpeg->packet_size + 6 - mpeg->packet_offset;

  ofs = mpeg->ofs + 6 + mpeg->packet_size;

  if (mpeg->mpeg_packet != NULL) {
    if (mpeg->mpeg_packet (mpeg)) {
      return (1);
    }
  }

  mpegd_set_offset (mpeg, ofs);

  return (0);
}

static
int mpegd_parse_pack (mpeg_demux_t *mpeg)
{
  unsigned           sid;
  unsigned long long ofs;

  if (mpegd_get_bits (mpeg, 32, 4) == 0x02) {
    mpeg->pack_type = 1;
    mpeg->pack_scr = mpegd_get_bits (mpeg, 36, 3);
    mpeg->pack_scr = (mpeg->pack_scr << 15) | mpegd_get_bits (mpeg, 40, 15);
    mpeg->pack_scr = (mpeg->pack_scr << 15) | mpegd_get_bits (mpeg, 56, 15);

    mpeg->pack_mux_rate = mpegd_get_bits (mpeg, 73, 22);

    ofs = mpeg->ofs + 12;
  }
  else if (mpegd_get_bits (mpeg, 32, 2) == 0x01) {
    unsigned stuff;

    mpeg->pack_type = 2;
    mpeg->pack_scr = mpegd_get_bits (mpeg, 34, 3);
    mpeg->pack_scr = (mpeg->pack_scr << 15) | mpegd_get_bits (mpeg, 38, 15);
    mpeg->pack_scr = (mpeg->pack_scr << 15) | mpegd_get_bits (mpeg, 54, 15);

    mpeg->pack_mux_rate = mpegd_get_bits (mpeg, 80, 22);

    stuff = mpegd_get_bits (mpeg, 109, 3);

    ofs = mpeg->ofs + 14 + stuff;
  }
  else {
    mpeg->pack_type = 0;
    mpeg->pack_scr = 0;
    mpeg->pack_mux_rate = 0;
    ofs = mpeg->ofs + 4;
  }

  mpeg->pack_cnt += 1;

  if (mpeg->mpeg_pack != NULL) {
    if (mpeg->mpeg_pack (mpeg)) {
      return (1);
    }
  }

  mpegd_set_offset (mpeg, ofs);

  mpegd_seek_header (mpeg, 1);

  if (mpegd_get_bits (mpeg, 0, 32) == MPEG_SYSTEM_HEADER) {
    if (mpegd_parse_system_header (mpeg)) {
      return (1);
    }
  }

  mpegd_seek_header (mpeg, 1);

  while (mpegd_get_bits (mpeg, 0, 24) == MPEG_PACKET_START) {
    sid = mpegd_get_bits (mpeg, 24, 8);

    if (sid == 0xba) {
      break;
    }
    else {
      mpegd_parse_packet (mpeg);
    }

    mpegd_seek_header (mpeg, 1);
  }

  return (0);
}

int mpegd_parse (mpeg_demux_t *mpeg)
{
  unsigned long long ofs;

  mpegd_seek_header (mpeg, 1);
  while (mpegd_get_bits (mpeg, 0, 32) == MPEG_PACK_START) {
    if (mpegd_parse_pack (mpeg)) {
      return (1);
    }

    if (mpegd_seek_header (mpeg, 1)) {
      return (1);
    }
  }

  if (mpegd_get_bits (mpeg, 0, 32) == MPEG_END_CODE) {
    ofs = mpeg->ofs + 4;

    if (mpeg->mpeg_end != NULL) {
      if (mpeg->mpeg_end (mpeg)) {
        return (1);
      }
    }

    mpegd_set_offset (mpeg, ofs);
  }

  return (0);
}
