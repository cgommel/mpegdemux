/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:     mpeg_scan.c                                                *
 * Created:       2003-02-07 by Hampa Hug <hampa@hampa.ch>                   *
 * Last modified: 2003-07-28 by Hampa Hug <hampa@hampa.ch>                   *
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

/* $Id: mpeg_scan.c,v 1.7 2003/07/28 06:11:50 hampa Exp $ */


#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "message.h"
#include "mpeg_parse.h"
#include "mpeg_scan.h"
#include "mpegdemux.h"


static
int mpeg_scan_system_header (mpeg_demux_t *mpeg)
{
  return (0);
}

static
int mpeg_scan_packet (mpeg_demux_t *mpeg)
{
  FILE               *fp;
  unsigned           sid, ssid;
  unsigned long long ofs;
  char               *type;

  sid = mpeg->packet.sid;
  ssid = mpeg->packet.ssid;

  if (mpeg_stream_excl (sid, ssid)) {
    return (0);
  }

  fp = (FILE *) mpeg->ext;

  ofs = mpeg->ofs;

  if (mpegd_set_offset (mpeg, ofs + mpeg->packet.size)) {
    fprintf (fp, "%08llx: sid=%02x ssid=%02x incomplete packet\n",
      ofs, sid, ssid
    );
  }

  if (sid == 0xbd) {
    if (mpeg->substreams[ssid].packet_cnt > 1) {
      return (0);
    }
  }
  else {
    if (mpeg->streams[sid].packet_cnt > 1) {
      return (0);
    }
  }

  if (mpeg->packet.type == 1) {
    type = "MPEG1";
  }
  else if (mpeg->packet.type == 2) {
    type = "MPEG2";
  }
  else {
    type = "UNKWN";
  }

  fprintf (fp, "%08llx: sid=%02x ssid=%02x %s pts=%llu[%.4f]\n",
    ofs, sid, ssid,
    type,
    mpeg->packet.pts, (double) mpeg->packet.pts / 90000.0
  );

  fflush (fp);

  return (0);
}

static
int mpeg_scan_pack (mpeg_demux_t *mpeg)
{
  return (0);
}

static
int mpeg_scan_end (mpeg_demux_t *mpeg)
{
  FILE *fp;

  fp = (FILE *) mpeg->ext;

  if (!par_no_end) {
    fprintf (fp, "%08llx: end code\n", mpeg->ofs);
  }

  return (0);
}

int mpeg_scan (FILE *inp, FILE *out)
{
  int          r;
  mpeg_demux_t *mpeg;

  mpeg = mpegd_open_fp (NULL, inp, 0);
  if (mpeg == NULL) {
    return (1);
  }

  mpeg->ext = out;

  mpeg->mpeg_system_header = &mpeg_scan_system_header;
  mpeg->mpeg_pack = &mpeg_scan_pack;
  mpeg->mpeg_packet = &mpeg_scan_packet;
  mpeg->mpeg_end = &mpeg_scan_end;

  r = mpegd_parse (mpeg);

  mpeg_print_stats (mpeg, out);

  mpegd_close (mpeg);

  return (r);
}
