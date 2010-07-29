/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:   src/mpeg_remux.c                                             *
 * Created:     2003-02-02 by Hampa Hug <hampa@hampa.ch>                     *
 * Copyright:   (C) 2003-2010 Hampa Hug <hampa@hampa.ch>                     *
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
#include <string.h>

#include "message.h"
#include "buffer.h"
#include "mpeg_ints.h"
#include "mpeg_parse.h"
#include "mpeg_remux.h"
#include "mpegdemux.h"


#define mpeg_ext_fp(mpeg) ((FILE *)(mpeg)->ext)


static mpeg_buffer_t shdr = { NULL, 0, 0 };
static mpeg_buffer_t pack = { NULL, 0, 0 };
static mpeg_buffer_t packet = { NULL, 0, 0 };

static unsigned sequence = 0;


static
int mpeg_remux_next_fp (mpeg_demux_t *mpeg)
{
	char *fname;
	FILE *fp;

	fp = (FILE *) mpeg->ext;
	if (fp != NULL) {
		fclose (fp);
		mpeg->ext = NULL;
	}

	fname = mpeg_get_name (par_demux_name, sequence);
	if (fname == NULL) {
		return (1);
	}

	sequence += 1;

	fp = fopen (fname, "wb");

	free (fname);

	if (fp == NULL) {
		return (1);
	}

	mpeg->ext = fp;

	return (0);
}

static
int mpeg_remux_skip (mpeg_demux_t *mpeg)
{
	if (par_remux_skipped == 0) {
		return (0);
	}

	if (mpeg_copy (mpeg, (FILE *) mpeg->ext, 1)) {
		return (1);
	}

	return (0);
}

static
int mpeg_remux_system_header (mpeg_demux_t *mpeg)
{
	if (par_no_shdr && (mpeg->shdr_cnt > 1)) {
		return (0);
	}

	if (mpeg_buf_write_clear (&pack, mpeg_ext_fp (mpeg))) {
		return (1);
	}

	if (mpeg_buf_read (&shdr, mpeg, mpeg->shdr.size)) {
		return (1);
	}

	if (mpeg_buf_write_clear (&shdr, mpeg_ext_fp (mpeg))) {
		return (1);
	}

	return (0);
}

static
int mpeg_remux_packet (mpeg_demux_t *mpeg)
{
	int      r;
	unsigned sid, ssid;

	sid = mpeg->packet.sid;
	ssid = mpeg->packet.ssid;

	if (mpeg_stream_excl (sid, ssid)) {
		return (0);
	}

	r = 0;

	if (mpeg_buf_read (&packet, mpeg, mpeg->packet.size)) {
		prt_msg ("remux: incomplete packet (sid=%02x size=%u/%u)\n",
			sid, packet.cnt, mpeg->packet.size
		);

		if (par_drop) {
			mpeg_buf_clear (&packet);
			return (1);
		}

		r = 1;
	}

	if (packet.cnt >= 4) {
		packet.buf[3] = par_stream_map[sid];

		if ((sid == 0xbd) && (packet.cnt > mpeg->packet.offset)) {
			packet.buf[mpeg->packet.offset] = par_substream_map[ssid];
		}
	}

	if (mpeg_buf_write_clear (&pack, mpeg_ext_fp (mpeg))) {
		return (1);
	}

	if (mpeg_buf_write_clear (&packet, mpeg_ext_fp (mpeg))) {
		return (1);
	}

	return (r);
}

static
int mpeg_remux_pack (mpeg_demux_t *mpeg)
{
	if (mpeg_buf_read (&pack, mpeg, mpeg->pack.size)) {
		return (1);
	}

	if (par_empty_pack) {
		if (mpeg_buf_write_clear (&pack, mpeg_ext_fp (mpeg))) {
			return (1);
		}
	}

	return (0);
}

static
int mpeg_remux_end (mpeg_demux_t *mpeg)
{
	if (par_no_end) {
		return (0);
	}

	if (mpeg_copy (mpeg, (FILE *) mpeg->ext, 4)) {
		return (1);
	}

	if (par_split) {
		if (mpeg_remux_next_fp (mpeg)) {
			return (1);
		}
	}

	return (0);
}

int mpeg_remux (FILE *inp, FILE *out)
{
	int          r;
	mpeg_demux_t *mpeg;

	mpeg = mpegd_open_fp (NULL, inp, 0);
	if (mpeg == NULL) {
		return (1);
	}

	if (par_split) {
		mpeg->ext = NULL;
		sequence = 0;

		if (mpeg_remux_next_fp (mpeg)) {
			return (1);
		}
	}
	else {
		mpeg->ext = out;
	}

	mpeg->mpeg_skip = mpeg_remux_skip;
	mpeg->mpeg_system_header = mpeg_remux_system_header;
	mpeg->mpeg_pack = mpeg_remux_pack;
	mpeg->mpeg_packet = mpeg_remux_packet;
	mpeg->mpeg_packet_check = mpeg_packet_check;
	mpeg->mpeg_end = mpeg_remux_end;

	mpeg_buf_init (&shdr);
	mpeg_buf_init (&pack);
	mpeg_buf_init (&packet);

	r = mpegd_parse (mpeg);

	if (par_no_end) {
		unsigned char buf[4];

		buf[0] = (MPEG_END_CODE >> 24) & 0xff;
		buf[1] = (MPEG_END_CODE >> 16) & 0xff;
		buf[2] = (MPEG_END_CODE >> 8) & 0xff;
		buf[3] = MPEG_END_CODE & 0xff;

		if (fwrite (buf, 1, 4, (FILE *) mpeg->ext) != 4) {
			r = 1;
		}
	}

	if (par_split) {
		fclose ((FILE *) mpeg->ext);
		mpeg->ext = NULL;
	}

	mpegd_close (mpeg);

	mpeg_buf_free (&shdr);
	mpeg_buf_free (&pack);
	mpeg_buf_free (&packet);

	return (r);
}
