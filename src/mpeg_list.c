/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:     mpeg_list.c                                                *
 * Created:       2003-02-02 by Hampa Hug <hampa@hampa.ch>                   *
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

/* $Id: mpeg_list.c,v 1.9 2003/03/02 11:19:49 hampa Exp $ */


#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "message.h"
#include "mpeg_parse.h"
#include "mpeg_list.h"
#include "mpegdemux.h"


static
int mpeg_list_system_header (mpeg_demux_t *mpeg)
{
  FILE *fp;

  if (mpeg->shdr_cnt > 1) {
    if (par_first || par_one_shdr) {
      return (0);
    }
  }

  fp = (FILE *) mpeg->ext;

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

  sid = mpeg->packet.sid;
  ssid = mpeg->packet.ssid;

  if (mpeg_stream_mark (sid, ssid)) {
    return (0);
  }

  fp = (FILE *) mpeg->ext;

  fprintf (fp, "%08llx: packet[%lu]: sid=%02x ssid=%02x size=%u type=%u pts=%llu[%.4f]\n",
    mpeg->ofs, mpeg->streams[sid].packet_cnt - 1,
    sid, ssid, mpeg->packet.size,
    mpeg->packet.type,
    mpeg->packet.pts, (double) mpeg->packet.pts / 90000.0
  );

  return (0);
}

static
int mpeg_list_pack (mpeg_demux_t *mpeg)
{
  FILE *fp;

  if (mpeg->pack_cnt > 1) {
    if (par_first || par_one_pack) {
      return (0);
    }
  }

  fp = (FILE *) mpeg->ext;

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

  fp = (FILE *) mpeg->ext;

  fprintf (fp, "%08llx: end\n", mpeg->ofs);

  return (0);
}

int mpeg_list (FILE *inp, FILE *out)
{
  unsigned     i;
  int          r;
  mpeg_demux_t *mpeg;

  mpeg = mpegd_open_fp (NULL, inp, 0);
  if (mpeg == NULL) {
    return (1);
  }

  mpeg->ext = out;

  mpeg->mpeg_system_header = &mpeg_list_system_header;
  mpeg->mpeg_pack = &mpeg_list_pack;
  mpeg->mpeg_packet = &mpeg_list_packet;
  mpeg->mpeg_end = &mpeg_list_end;

  r = mpegd_parse (mpeg);

  mpegd_close (mpeg);

  fprintf (out,
    "\n"
    "System headers: %lu\n"
    "Packs:          %lu\n"
    "Packets:        %lu\n"
    "Skipped:        %lu bytes\n"
    "Next:           %08lx\n",
    mpeg->shdr_cnt, mpeg->pack_cnt, mpeg->packet_cnt, mpeg->skip_cnt,
    mpegd_get_bits (mpeg, 0, 32)
  );

  for (i = 0; i < 256; i++) {
    if (mpeg->streams[i].packet_cnt > 0) {
      fprintf (out, "Stream %02x:      %lu packets / %llu bytes\n",
        i, mpeg->streams[i].packet_cnt, mpeg->streams[i].size
      );
    }
  }

  return (r);
}
