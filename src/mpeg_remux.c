/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:     mpeg_remux.c                                               *
 * Created:       2003-02-02 by Hampa Hug <hampa@hampa.ch>                   *
 * Last modified: 2003-03-05 by Hampa Hug <hampa@hampa.ch>                   *
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

/* $Id: mpeg_remux.c,v 1.6 2003/03/05 10:35:17 hampa Exp $ */


#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "message.h"
#include "mpeg_parse.h"
#include "mpeg_remux.h"
#include "mpegdemux.h"


#define PACK_BUF_SIZE 16384


static unsigned char pack_buf[PACK_BUF_SIZE];
static unsigned      pack_cnt = 0;


static
int mpeg_remux_copy (mpeg_demux_t *mpeg, unsigned n)
{
  unsigned char buf[4096];
  FILE          *fp;
  unsigned      i;

  fp = (FILE *) mpeg->ext;

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
int mpeg_remux_pack_write (mpeg_demux_t *mpeg)
{
  FILE *fp;

  if (pack_cnt > 0) {
    fp = (FILE *) mpeg->ext;
    fwrite (pack_buf, 1, pack_cnt, fp);
    pack_cnt = 0;
  }

  return (0);
}

static
int mpeg_remux_system_header (mpeg_demux_t *mpeg)
{
  if (mpeg->shdr_cnt > 1) {
    if (par_first || par_one_shdr) {
      return (0);
    }
  }

  return (mpeg_remux_copy (mpeg, mpeg->shdr.size));
}

static
int mpeg_remux_packet (mpeg_demux_t *mpeg)
{
  unsigned sid, ssid;

  sid = mpeg->packet.sid;
  ssid = mpeg->packet.ssid;

  if (mpeg_stream_mark (sid, ssid)) {
    return (0);
  }

  if (mpeg_remux_pack_write (mpeg)) {
    return (1);
  }

  return (mpeg_remux_copy (mpeg, mpeg->packet.size));
}

static
int mpeg_remux_pack (mpeg_demux_t *mpeg)
{
  int r;

  if (mpeg->pack_cnt > 1) {
    if (par_first || par_one_pack) {
      return (0);
    }
  }

  /*
    If we don't allow empty packs then copy the pack to the buffer,
    possibly overwriting an old (empty) pack. Otherwise copy the
    pack immediately to the output.
  */

  if (!par_empty_pack) {
    if (mpeg->pack.size > PACK_BUF_SIZE) {
      return (1);
    }

    pack_cnt = mpeg->pack.size;
    r = mpegd_read (mpeg, pack_buf, pack_cnt);
  }
  else {
    r = mpeg_remux_copy (mpeg, mpeg->pack.size);
  }

  return (r);
}

static
int mpeg_remux_end (mpeg_demux_t *mpeg)
{
  if (par_one_end) {
    return (0);
  }

  return (mpeg_remux_copy (mpeg, 4));
}

int mpeg_remux (FILE *inp, FILE *out)
{
  int          r;
  mpeg_demux_t *mpeg;

  mpeg = mpegd_open_fp (NULL, inp, 0);
  if (mpeg == NULL) {
    return (1);
  }

  mpeg->ext = out;

  mpeg->mpeg_system_header = &mpeg_remux_system_header;
  mpeg->mpeg_pack = &mpeg_remux_pack;
  mpeg->mpeg_packet = &mpeg_remux_packet;
  mpeg->mpeg_end = &mpeg_remux_end;

  r = mpegd_parse (mpeg);

  if (par_one_end) {
    unsigned char buf[4];

    buf[0] = (MPEG_END_CODE >> 24) & 0xff;
    buf[1] = (MPEG_END_CODE >> 16) & 0xff;
    buf[2] = (MPEG_END_CODE >> 8) & 0xff;
    buf[3] = MPEG_END_CODE & 0xff;

    fwrite (buf, 1, 4, out);
  }

  mpegd_close (mpeg);

  return (r);
}
