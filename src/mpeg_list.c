/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:     mpeg_list.c                                                *
 * Created:       2003-02-02 by Hampa Hug <hampa@hampa.ch>                   *
 * Last modified: 2003-07-27 by Hampa Hug <hampa@hampa.ch>                   *
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

/* $Id: mpeg_list.c,v 1.13 2003/07/28 06:12:20 hampa Exp $ */


#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "message.h"
#include "mpeg_parse.h"
#include "mpeg_list.h"
#include "mpegdemux.h"


static unsigned long long skip_ofs = 0;
static unsigned long      skip_cnt = 0;


static
void mpeg_list_print_skip (FILE *fp)
{
  if (skip_cnt > 0) {
    fprintf (fp, "%08llx: skip %lu\n", skip_ofs, skip_cnt);

    skip_cnt = 0;
  }
}

static
int mpeg_list_skip (mpeg_demux_t *mpeg)
{
  if (skip_cnt == 0) {
    skip_ofs = mpeg->ofs;
  }

  skip_cnt += 1;

  return (0);
}

static
int mpeg_list_system_header (mpeg_demux_t *mpeg)
{
  FILE *fp;

  if (par_no_shdr) {
    return (0);
  }

  fp = (FILE *) mpeg->ext;

  mpeg_list_print_skip (fp);

  fprintf (fp, "%08llx: system header[%lu]: size=%u fixed=%d csps=%d\n",
    mpeg->ofs, mpeg->shdr_cnt - 1,
    mpeg->shdr.size, mpeg->shdr.fixed, mpeg->shdr.csps
  );

  return (0);
}

static
int mpeg_list_packet (mpeg_demux_t *mpeg)
{
  FILE     *fp;
  unsigned sid, ssid;
  char     *type;

  if (par_no_packet) {
    return (0);
  }

  sid = mpeg->packet.sid;
  ssid = mpeg->packet.ssid;

  if (mpeg_stream_excl (sid, ssid)) {
    return (0);
  }

  fp = (FILE *) mpeg->ext;

  mpeg_list_print_skip (fp);

  if (mpeg->packet.type == 1) {
    type = "MPEG1";
  }
  else if (mpeg->packet.type == 2) {
    type = "MPEG2";
  }
  else {
    type = "UNKWN";
  }

  fprintf (fp,
    "%08llx: packet[%lu]: %s sid=%02x ssid=%02x size=%u",
    mpeg->ofs,
    mpeg->streams[sid].packet_cnt - 1,
    type,
    sid, ssid, mpeg->packet.size
  );

  if (mpeg->packet.have_pts || mpeg->packet.have_dts) {
    fprintf (fp,
      " pts=%llu[%.4f] dts=%llu[%.4f]",
      mpeg->packet.pts, (double) mpeg->packet.pts / 90000.0,
      mpeg->packet.dts, (double) mpeg->packet.dts / 90000.0
    );
  }

  fputs ("\n", fp);

  return (0);
}

static
int mpeg_list_pack (mpeg_demux_t *mpeg)
{
  FILE *fp;

  if (par_no_pack) {
    return (0);
  }

  fp = (FILE *) mpeg->ext;

  mpeg_list_print_skip (fp);

  fprintf (fp, "%08llx: pack[%lu]: type=%u scr=%llu[%.4f] mux=%lu[%.2f] stuff=%u\n",
    mpeg->ofs, mpeg->pack_cnt - 1,
    mpeg->pack.type,
    mpeg->pack.scr, (double) mpeg->pack.scr / 90000.0,
    mpeg->pack.mux_rate, 50.0 * mpeg->pack.mux_rate,
    mpeg->pack.stuff
  );

  fflush (fp);

  return (0);
}

static
int mpeg_list_end (mpeg_demux_t *mpeg)
{
  FILE *fp;

  if (par_no_end) {
    return (0);
  }

  fp = (FILE *) mpeg->ext;

  mpeg_list_print_skip (fp);

  fprintf (fp, "%08llx: end\n", mpeg->ofs);

  return (0);
}

int mpeg_list (FILE *inp, FILE *out)
{
  int          r;
  mpeg_demux_t *mpeg;

  mpeg = mpegd_open_fp (NULL, inp, 0);
  if (mpeg == NULL) {
    return (1);
  }

  skip_cnt = 0;
  skip_ofs = 0;

  mpeg->ext = out;

  mpeg->mpeg_skip = &mpeg_list_skip;
  mpeg->mpeg_system_header = &mpeg_list_system_header;
  mpeg->mpeg_pack = &mpeg_list_pack;
  mpeg->mpeg_packet = &mpeg_list_packet;
  mpeg->mpeg_end = &mpeg_list_end;

  r = mpegd_parse (mpeg);

  mpeg_list_print_skip (out);

  mpeg_print_stats (mpeg, out);

  mpegd_close (mpeg);

  return (r);
}
