/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:     mpeg_scan.c                                                *
 * Created:       2003-02-07 by Hampa Hug <hampa@hampa.ch>                   *
 * Copyright:     (C) 2003-2007 Hampa Hug <hampa@hampa.ch>                   *
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

/* $Id$ */


#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "message.h"
#include "mpeg_parse.h"
#include "mpeg_scan.h"
#include "mpegdemux.h"


static unsigned long long pts1[256];
static unsigned long long pts2[256];


static
int mpeg_scan_system_header (mpeg_demux_t *mpeg)
{
	return (0);
}

static
int mpeg_scan_packet (mpeg_demux_t *mpeg)
{
	FILE               *fp;
	int                skip;
	unsigned           sid, ssid;
	unsigned long long ofs;

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

	skip = 0;

	if (sid == 0xbd) {
		if (mpeg->substreams[ssid].packet_cnt > 1) {
			if (!par_first_pts) {
				return (0);
			}

			if (!mpeg->packet.have_pts) {
				return (0);
			}

			if (mpeg->packet.pts >= pts2[ssid]) {
				return (0);
			}
		}

		if (mpeg->packet.pts < pts2[ssid]) {
			pts2[ssid] = mpeg->packet.pts;
		}
	}
	else {
		if (mpeg->streams[sid].packet_cnt > 1) {
			if (!par_first_pts) {
				return (0);
			}

			if (!mpeg->packet.have_pts) {
				return (0);
			}

			if (mpeg->packet.pts >= pts1[sid]) {
				return (0);
			}
		}

		if (mpeg->packet.pts < pts1[sid]) {
			pts1[sid] = mpeg->packet.pts;
		}
	}

	fprintf (fp, "%08llx: sid=%02x", ofs, sid);

	if (sid == 0xbd) {
		fprintf (fp, "[%02x]", ssid);
	}
	else {
		fputs ("    ", fp);
	}

	if (mpeg->packet.type == 1) {
		fputs (" MPEG1", fp);
	}
	else if (mpeg->packet.type == 2) {
		fputs (" MPEG2", fp);
	}
	else {
		fputs (" UNKWN", fp);
	}

	if (mpeg->packet.have_pts) {
		fprintf (fp, " pts=%llu[%.4f]",
			mpeg->packet.pts, (double) mpeg->packet.pts / 90000.0
		);
	}

	fputs ("\n", fp);

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
	unsigned     i;
	mpeg_demux_t *mpeg;

	for (i = 0; i < 256; i++) {
		pts1[i] = ~(unsigned long long) 0;
		pts2[i] = ~(unsigned long long) 0;
	}

	mpeg = mpegd_open_fp (NULL, inp, 0);
	if (mpeg == NULL) {
		return (1);
	}

	mpeg->ext = out;

	mpeg->mpeg_system_header = &mpeg_scan_system_header;
	mpeg->mpeg_pack = &mpeg_scan_pack;
	mpeg->mpeg_packet = &mpeg_scan_packet;
	mpeg->mpeg_packet_check = &mpeg_packet_check;
	mpeg->mpeg_end = &mpeg_scan_end;

	r = mpegd_parse (mpeg);

	mpeg_print_stats (mpeg, out);

	mpegd_close (mpeg);

	return (r);
}
