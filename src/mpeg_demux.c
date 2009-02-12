/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:   src/mpeg_demux.c                                             *
 * Created:     2003-02-02 by Hampa Hug <hampa@hampa.ch>                     *
 * Copyright:   (C) 2003-2009 Hampa Hug <hampa@hampa.ch>                     *
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


#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "message.h"
#include "buffer.h"
#include "mpeg_parse.h"
#include "mpeg_demux.h"
#include "mpegdemux.h"


static FILE *fp[512];

static mpeg_buffer_t packet = { NULL, 0, 0 };


static
int mpeg_demux_copy_spu (mpeg_demux_t *mpeg, FILE *fp, unsigned cnt)
{
	static unsigned    spucnt = 0;
	static int         half = 0;
	unsigned           i, n;
	unsigned char      buf[8];
	unsigned long long pts;

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

		mpeg_copy (mpeg, fp, n);
		cnt -= n;
		spucnt -= n;
	}

	return (0);
}

static
FILE *mpeg_demux_open (mpeg_demux_t *mpeg, unsigned sid, unsigned ssid)
{
	FILE     *fp;
	char     *name;
	unsigned seq;

	if (par_demux_name == NULL) {
		fp = (FILE *) mpeg->ext;
	}
	else {
		seq = (sid == 0xbd) ? ((sid << 8) + ssid) : sid;

		name = mpeg_get_name (par_demux_name, seq);

		fp = fopen (name, "wb");
		if (fp == NULL) {
			prt_err ("can't open stream file (%s)\n", name);

			if (sid == 0xbd) {
				par_substream[ssid] &= ~PAR_STREAM_SELECT;
			}
			else {
				par_stream[sid] &= ~PAR_STREAM_SELECT;
			}

			free (name);

			return (NULL);
		}

		free (name);
	}

	if ((sid == 0xbd) && par_dvdsub) {
		fwrite ("SPU ", 1, 4, fp);
	}

	return (fp);
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
	unsigned fpi;
	unsigned cnt;
	int      r;

	sid = mpeg->packet.sid;
	ssid = mpeg->packet.ssid;

	if (mpeg_stream_excl (sid, ssid)) {
		return (0);
	}

	cnt = mpeg->packet.offset;

	fpi = sid;

	/* select substream in private stream 1 (AC3 audio) */
	if (sid == 0xbd) {
		fpi = 256 + ssid;
		cnt += 1;

		if (par_dvdac3) {
			cnt += 3;
		}
	}

	if (cnt > mpeg->packet.size) {
		prt_msg ("demux: AC3 packet too small (sid=%02x size=%u)\n",
			sid, mpeg->packet.size
		);

		return (1);
	}

	if (fp[fpi] == NULL) {
		fp[fpi] = mpeg_demux_open (mpeg, sid, ssid);
		if (fp[fpi] == NULL) {
			return (1);
		}
	}

	if (cnt > 0) {
		mpegd_skip (mpeg, cnt);
	}

	cnt = mpeg->packet.size - cnt;

	if ((sid == 0xbd) && par_dvdsub) {
		return (mpeg_demux_copy_spu (mpeg, fp[fpi], cnt));
	}

	r = 0;

	if (mpeg_buf_read (&packet, mpeg, cnt)) {
		prt_msg ("demux: incomplete packet (sid=%02x size=%u/%u)\n",
			sid, packet.cnt, cnt
		);

		if (par_drop) {
			mpeg_buf_clear (&packet);
			return (1);
		}

		r = 1;
	}

	if (mpeg_buf_write_clear (&packet, fp[fpi])) {
		r = 1;
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

	for (i = 0; i < 512; i++) {
		fp[i] = NULL;
	}

	mpeg = mpegd_open_fp (NULL, inp, 0);
	if (mpeg == NULL) {
		return (1);
	}

	mpeg->mpeg_system_header = &mpeg_demux_system_header;
	mpeg->mpeg_pack = &mpeg_demux_pack;
	mpeg->mpeg_packet = &mpeg_demux_packet;
	mpeg->mpeg_packet_check = &mpeg_packet_check;
	mpeg->mpeg_end = &mpeg_demux_end;

	mpeg->ext = out;

	r = mpegd_parse (mpeg);

	mpegd_close (mpeg);

	for (i = 0; i < 512; i++) {
		if ((fp[i] != NULL) && (fp[i] != out)) {
			fclose (fp[i]);
		}
	}

	return (r);
}
