/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:     mpeg_list.c                                                *
 * Created:       2003-02-02 by Hampa Hug <hampa@hampa.ch>                   *
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

/* $Id: mpeg_list.c,v 1.3 2003/02/03 16:16:31 hampa Exp $ */


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

  if (par_list_first && (mpeg->shdr_cnt > 1)) {
    return (0);
  }

  fp = (FILE *) mpeg->ext;

  fprintf (fp, "%08llx: system header[%lu]: size=%u fixed=%d csps=%d\n",
    mpeg->ofs, mpeg->shdr_cnt - 1,
    mpeg->sh_size, mpeg->sh_fixed, mpeg->sh_csps
  );

  return (0);
}

static
int mpeg_list_packet (mpeg_demux_t *mpeg)
{
  FILE     *fp;
  unsigned id;

  id = mpeg->packet_stm_id;

  if (par_stream[id] & PAR_STREAM_EXCLUDE) {
    return (0);
  }

  if (par_list_first && (mpeg->streams[id].packet_cnt > 1)) {
    return (0);
  }

  fp = (FILE *) mpeg->ext;

  fprintf (fp, "%08llx: packet[%lu]: stream id=%02x size=%u pts=%llu[%.4f]\n",
    mpeg->ofs, mpeg->streams[id].packet_cnt - 1,
    mpeg->packet_stm_id, mpeg->packet_size,
    mpeg->packet_pts, (double) mpeg->packet_pts / 90000.0
  );

  return (0);
}

static
int mpeg_list_pack (mpeg_demux_t *mpeg)
{
  FILE *fp;

  if (par_list_first && (mpeg->pack_cnt > 1)) {
    return (0);
  }

  fp = (FILE *) mpeg->ext;

  fprintf (fp, "%08llx: pack[%lu]: scr=%llu[%.4fs] mux=%lu[%.2f bytes/s]\n",
    mpeg->ofs, mpeg->pack_cnt - 1,
    mpeg->pack_scr, (double) mpeg->pack_scr / 90000.0,
    mpeg->pack_mux_rate, 50.0 * mpeg->pack_mux_rate
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
    "Skipped:        %lu bytes\n",
    mpeg->shdr_cnt, mpeg->pack_cnt, mpeg->packet_cnt, mpeg->skip_cnt
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
