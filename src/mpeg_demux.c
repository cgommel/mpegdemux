/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:     mpeg_demux.c                                               *
 * Created:       2003-02-02 by Hampa Hug <hampa@hampa.ch>                   *
 * Last modified: 2003-03-04 by Hampa Hug <hampa@hampa.ch>                   *
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

/* $Id: mpeg_demux.c,v 1.6 2003/03/05 07:43:58 hampa Exp $ */


#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "message.h"
#include "mpeg_parse.h"
#include "mpeg_demux.h"
#include "mpegdemux.h"


static FILE *fp[256];


static
char *mpeg_demux_get_name (const char *base, unsigned sid)
{
  unsigned n;
  unsigned dig;
  char     *ret;

  if (base == NULL) {
    base = "stream_##.dat";
  }

  n = 0;
  while (base[n] != 0) {
    n += 1;
  }

  n += 1;

  ret = (char *) malloc (n);
  if (ret == NULL) {
    return (NULL);
  }

  while (n > 0) {
    n -= 1;
    ret[n] = base[n];

    if (ret[n] == '#') {
      dig = sid % 16;
      sid = sid / 16;
      if (dig < 10) {
        ret[n] = '0' + dig;
      }
      else {
        ret[n] = 'a' + dig - 10;
      }
    }
  }

  return (ret);
}

static
int mpeg_demux_copy (mpeg_demux_t *mpeg, FILE *fp, unsigned n)
{
  unsigned char buf[4096];
  unsigned      i;

  while (n > 0) {
    if (n > 4096) {
      i = 4096;
    }
    else {
      i = n;
    }

    if (mpegd_read (mpeg, buf, i)) {
      return (1);
    }

    fwrite (buf, 1, i, fp);

    n -= i;
  }

  return (0);
}

static
int mpeg_demux_copy_spu (mpeg_demux_t *mpeg, FILE *fp, unsigned cnt)
{
  static int         first = 1;
  static unsigned    spucnt = 0;
  static int         half = 0;
  unsigned           i, n;
  unsigned char      buf[8];
  unsigned long long pts;

  if (first) {
    fwrite ("SPU ", 1, 4, fp);
    first = 0;
  }

  if (half) {
    mpegd_read (mpeg, buf, 1);
    fwrite (buf, 1, 1, fp);
    spucnt = (spucnt << 8) + buf[0];
    half = 0;

    spucnt -= 2;
    cnt -= 1;
  }

  while (cnt > 0) {
    if (spucnt == 0) {
      pts = mpeg->packet.pts;
      for (i = 0; i < 8; i++) {
        buf[7 - i] = pts & 0xff;
        pts = pts >> 8;
      }
      fwrite (buf, 1, 8, fp);

      if (cnt == 1) {
        mpegd_read (mpeg, buf, 1);
        fwrite (buf, 1, 1, fp);
        spucnt = buf[0];
        half = 1;
        return (0);
      }

      mpegd_read (mpeg, buf, 2);
      fwrite (buf, 1, 2, fp);

      spucnt = (buf[0] << 8) + buf[1];
      if (spucnt < 2) {
        return (1);
      }

      spucnt -= 2;
      cnt -= 2;
    }

    n = (cnt < spucnt) ? cnt : spucnt;

    mpeg_demux_copy (mpeg, fp, n);
    cnt -= n;
    spucnt -= n;
  }

  return (0);
}

static
int mpeg_demux_system_header (mpeg_demux_t *mpeg)
{
  return (0);
}

static
int mpeg_demux_packet (mpeg_demux_t *mpeg)
{
  unsigned sid, ssid;
  unsigned cnt;
  int      r;

  sid = mpeg->packet.sid;
  ssid = mpeg->packet.ssid;

  cnt = mpeg->packet.offset;

  /* select substream in private stream 1 (AC3 audio) */
  if (sid == 0xbd) {
    cnt += 1;

    if (par_dvdac3) {
      cnt += 4;
    }
  }

  if (mpeg_stream_mark (sid, ssid)) {
    return (0);
  }

  if (fp[sid] == NULL) {
    char *name;

    name = mpeg_demux_get_name (par_demux_name, sid);
    fp[sid] = fopen (name, "wb");
    if (fp[sid] == NULL) {
      prt_err ("can't open stream file (%s)\n", name);
      par_stream[sid] |= PAR_STREAM_EXCLUDE;
      free (name);
      return (0);
    }
    free (name);
  }

  if (cnt > 0) {
    mpegd_skip (mpeg, cnt);
  }

  cnt = mpeg->packet.size - cnt;

  if ((sid == 0xbd) && par_dvdsub) {
    r = mpeg_demux_copy_spu (mpeg, fp[sid], cnt);
  }
  else {
    r = mpeg_demux_copy (mpeg, fp[sid], cnt);
  }

  return (r);
}

static
int mpeg_demux_pack (mpeg_demux_t *mpeg)
{
  return (0);
}

static
int mpeg_demux_end (mpeg_demux_t *mpeg)
{
  return (0);
}

int mpeg_demux (FILE *inp, FILE *out)
{
  unsigned     i;
  int          r;
  mpeg_demux_t *mpeg;

  for (i = 0; i < 256; i++) {
    fp[i] = NULL;
  }

  mpeg = mpegd_open_fp (NULL, inp, 0);
  if (mpeg == NULL) {
    return (1);
  }

  mpeg->mpeg_system_header = &mpeg_demux_system_header;
  mpeg->mpeg_pack = &mpeg_demux_pack;
  mpeg->mpeg_packet = &mpeg_demux_packet;
  mpeg->mpeg_end = &mpeg_demux_end;

  r = mpegd_parse (mpeg);

  mpegd_close (mpeg);

  for (i = 0; i < 256; i++) {
    if (fp[i] != NULL) {
      fclose (fp[i]);
    }
  }

  return (r);
}
