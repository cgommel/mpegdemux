/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:     mpeg_parse.c                                               *
 * Created:       2003-02-01 by Hampa Hug <hampa@hampa.ch>                   *
 * Last modified: 2003-02-05 by Hampa Hug <hampa@hampa.ch>                   *
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

/* $Id: mpeg_parse.c,v 1.9 2003/02/05 03:00:54 hampa Exp $ */


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
    r = fread (mpeg->buf + mpeg->buf_n, 1, n, mpeg->fp);
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
  unsigned char *buf;

  if (mpegd_need_bits (mpeg, i + n)) {
    return (0);
  }

  buf = mpeg->buf + mpeg->buf_i;

  r = 0;

  /* aligned bytes */
  if (((i | n) & 7) == 0) {
    i = i / 8;
    n = n / 8;
    while (n > 0) {
      r = (r << 8) | buf[i];
      i += 1;
      n -= 1;
    }
    return (r);
  }


  while (n > 0) {
    b_n = 8 - (i & 7);
    if (b_n > n) {
      b_n = n;
    }

    b_i = 8 - (i & 7) - b_n;

    m = (1 << b_n) - 1;
    v = (buf[i >> 3] >> b_i) & m;

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

int mpegd_set_offset (mpeg_demux_t *mpeg, unsigned long long ofs)
{
  if (ofs == mpeg->ofs) {
    return (0);
  }

  if (ofs > mpeg->ofs) {
    return (mpegd_skip (mpeg, (unsigned long) (ofs - mpeg->ofs)));
  }

  return (1);
}

static
int mpegd_seek_header (mpeg_demux_t *mpeg)
{
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

  return (0);
}

static
int mpegd_parse_system_header (mpeg_demux_t *mpeg)
{
  unsigned long long ofs;

  mpeg->shdr.size = mpegd_get_bits (mpeg, 32, 16) + 6;

  mpeg->shdr.fixed = mpegd_get_bits (mpeg, 78, 1);
  mpeg->shdr.csps = mpegd_get_bits (mpeg, 79, 1);

  mpeg->shdr_cnt += 1;

  ofs = mpeg->ofs + mpeg->shdr.size;

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
  unsigned           sid, ssid;
  unsigned long long pts;
  unsigned long long ofs;

  mpeg->packet.type = 0;

  sid = mpegd_get_bits (mpeg, 24, 8);
  ssid = 0;

  mpeg->packet.size = mpegd_get_bits (mpeg, 32, 16) + 6;

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

      mpeg->packet.type = 1;

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

      mpeg->packet.type = 2;

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

  if (sid == 0xbd) {
    ssid = mpegd_get_bits (mpeg, i, 8);
  }

  mpeg->packet.sid = sid;
  mpeg->packet.ssid = ssid;
  mpeg->packet.pts = pts;
  mpeg->packet.offset = i / 8;

  mpeg->packet_cnt += 1;
  mpeg->streams[sid].packet_cnt += 1;
  mpeg->streams[sid].size += mpeg->packet.size - mpeg->packet.offset;

  ofs = mpeg->ofs + mpeg->packet.size;

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
    mpeg->pack.type = 1;
    mpeg->pack.scr = mpegd_get_bits (mpeg, 36, 3);
    mpeg->pack.scr = (mpeg->pack.scr << 15) | mpegd_get_bits (mpeg, 40, 15);
    mpeg->pack.scr = (mpeg->pack.scr << 15) | mpegd_get_bits (mpeg, 56, 15);
    mpeg->pack.mux_rate = mpegd_get_bits (mpeg, 73, 22);
    mpeg->pack.stuff = 0;
    mpeg->pack.size = 12;
  }
  else if (mpegd_get_bits (mpeg, 32, 2) == 0x01) {
    mpeg->pack.type = 2;
    mpeg->pack.scr = mpegd_get_bits (mpeg, 34, 3);
    mpeg->pack.scr = (mpeg->pack.scr << 15) | mpegd_get_bits (mpeg, 38, 15);
    mpeg->pack.scr = (mpeg->pack.scr << 15) | mpegd_get_bits (mpeg, 54, 15);
    mpeg->pack.mux_rate = mpegd_get_bits (mpeg, 80, 22);
    mpeg->pack.stuff = mpegd_get_bits (mpeg, 109, 3);
    mpeg->pack.size = 14 + mpeg->pack.stuff;
  }
  else {
    mpeg->pack.type = 0;
    mpeg->pack.scr = 0;
    mpeg->pack.mux_rate = 0;
    mpeg->pack.size = 4;
  }

  ofs = mpeg->ofs + mpeg->pack.size;

  mpeg->pack_cnt += 1;

  if (mpeg->mpeg_pack != NULL) {
    if (mpeg->mpeg_pack (mpeg)) {
      return (1);
    }
  }

  mpegd_set_offset (mpeg, ofs);

  mpegd_seek_header (mpeg);

  if (mpegd_get_bits (mpeg, 0, 32) == MPEG_SYSTEM_HEADER) {
    if (mpegd_parse_system_header (mpeg)) {
      return (1);
    }

    mpegd_seek_header (mpeg);
  }

  while (mpegd_get_bits (mpeg, 0, 24) == MPEG_PACKET_START) {
    sid = mpegd_get_bits (mpeg, 24, 8);

    if ((sid == 0xba) || (sid == 0xb9)) {
      break;
    }
    else {
      mpegd_parse_packet (mpeg);
    }

    mpegd_seek_header (mpeg);
  }

  return (0);
}

int mpegd_parse (mpeg_demux_t *mpeg)
{
  unsigned long long ofs;

  mpegd_seek_header (mpeg);

  while (mpegd_get_bits (mpeg, 0, 32) == MPEG_PACK_START) {
    if (mpegd_parse_pack (mpeg)) {
      return (1);
    }

    if (mpegd_seek_header (mpeg)) {
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
