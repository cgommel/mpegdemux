/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:     mpeg_remux.c                                               *
 * Created:       2003-02-02 by Hampa Hug <hampa@hampa.ch>                   *
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

/* $Id: mpeg_remux.c,v 1.1 2003/02/02 20:26:13 hampa Exp $ */


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "message.h"
#include "mpeg_parse.h"
#include "mpeg_remux.h"
#include "mpegdemux.h"


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
int mpeg_remux_system_header (mpeg_demux_t *mpeg)
{
  static unsigned long sh_cnt = 0;

  sh_cnt += 1;

  if ((par_rep_sh == 0) && (sh_cnt > 1)) {
    return (0);
  }

  return (mpeg_remux_copy (mpeg, mpeg->sh_size + 6));
}

static
int mpeg_remux_packet (mpeg_demux_t *mpeg)
{
  if (par_stream[mpeg->packet_stm_id] & PAR_STREAM_EXCLUDE) {
    return (0);
  }

  return (mpeg_remux_copy (mpeg, mpeg->packet_size + 6));
}

static
int mpeg_remux_pack (mpeg_demux_t *mpeg)
{
  return (mpeg_remux_copy (mpeg, 12));
}

static
int mpeg_remux_end (mpeg_demux_t *mpeg)
{
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

  mpegd_close (mpeg);

  return (r);
}
