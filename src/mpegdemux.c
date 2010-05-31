/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:   src/mpegdemux.c                                              *
 * Created:     2003-02-01 by Hampa Hug <hampa@hampa.ch>                     *
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
#include <string.h>
#include <stdarg.h>

#include "getopt.h"
#include "message.h"
#include "mpeg_parse.h"
#include "mpeg_list.h"
#include "mpeg_demux.h"
#include "mpeg_remux.h"
#include "mpeg_scan.h"
#include "mpegdemux.h"


static unsigned par_mode = PAR_MODE_SCAN;

static FILE     *par_inp = NULL;
static FILE     *par_out = NULL;

unsigned char   par_stream[256];
unsigned char   par_substream[256];

unsigned char   par_stream_map[256];
unsigned char   par_substream_map[256];

int             par_no_shdr = 0;
int             par_no_pack = 0;
int             par_no_packet = 0;
int             par_no_end = 0;
int             par_empty_pack = 0;
int             par_remux_skipped = 0;
int             par_split = 0;
int             par_drop = 1;
int             par_scan = 0;
int             par_first_pts = 0;
int             par_dvdac3 = 0;
int             par_dvdsub = 0;
char            *par_demux_name = NULL;
unsigned        par_packet_max = 0;


static mpegd_option_t opts[] = {
	{ '?', 0, "help", NULL, "Print usage information" },
	{ 'a', 0, "ac3", NULL, "Assume DVD AC3 headers in private streams" },
	{ 'b', 1, "base-name", "name", "Set the base name for demuxed streams" },
	{ 'c', 0, "scan", NULL, "Scan the stream [default]" },
	{ 'd', 0, "demux", NULL, "Demultiplex streams" },
	{ 'D', 0, "no-drop", NULL, "Don't drop incomplete packets" },
	{ 'e', 0, "no-end", NULL, "Don't list end codes [no]" },
	{ 'E', 0, "empty-packs", NULL, "Remux empty packs [no]" },
	{ 'F', 0, "first-pts", NULL, "Print packet with lowest PTS [no]" },
	{ 'h', 0, "no-system-headers", NULL, "Don't list system headers" },
	{ 'i', 1, "invalid", "id", "Select invalid streams [none]" },
	{ 'k', 0, "no-packs", NULL, "Don't list packs" },
	{ 'K', 0, "remux-skipped", NULL, "Copy skipped bytes when remuxing [no]" },
	{ 'l', 0, "list", NULL, "List the stream contents" },
	{ 'm', 1, "packet-max-size", "int", "Set the maximum packet size [0]" },
	{ 'p', 1, "substream", "id", "Select substreams [none]" },
	{ 'P', 2, "substream-map", "id1 id2", "Remap substream id1 to id2" },
	{ 'r', 0, "remux", NULL, "Copy modified input to output" },
	{ 's', 1, "stream", "id", "Select streams [none]" },
	{ 'S', 2, "stream-map", "id1 id2", "Remap stream id1 to id2" },
	{ 't', 0, "no-packets", NULL, "Don't list packets" },
	{ 'u', 0, "spu", NULL, "Assume DVD subtitles in private streams" },
	{ 'V', 0, "version", NULL, "Print version information" },
	{ 'x', 0, "split", NULL, "Split sequences while remuxing [no]" },
	{  -1, 0, NULL, NULL, NULL }
};


static
void print_help (void)
{
	mpegd_getopt_help (
		"mpegdemux: demultiplex MPEG1/2 system streams",
		"usage: mpegdemux [options] [input [output]]",
		opts
	);

	fflush (stdout);
}

static
void print_version (void)
{
	fputs (
		"mpegdemux version " MPEGDEMUX_VERSION_STR
		"\n\n"
		"Copyright (C) 2003-2010 Hampa Hug <hampa@hampa.ch>\n",
		stdout
	);
}

static
char *str_clone (const char *str)
{
	char *ret;

	ret = malloc (strlen (str) + 1);
	if (ret == NULL) {
		return (NULL);
	}

	strcpy (ret, str);

	return (ret);
}

static
const char *str_skip_white (const char *str)
{
	while ((*str == ' ') || (*str == '\t')) {
		str += 1;
	}

	return (str);
}

static
int str_get_streams (const char *str, unsigned char stm[256], unsigned msk)
{
	unsigned i;
	int      incl;
	char     *tmp;
	unsigned stm1, stm2;

	incl = 1;

	while (*str != 0) {
		str = str_skip_white (str);

		if (*str == '+') {
			str += 1;
			incl = 1;
		}
		else if (*str == '-') {
			str += 1;
			incl = 0;
		}
		else {
			incl = 1;
		}

		if (strncmp (str, "all", 3) == 0) {
			str += 3;
			stm1 = 0;
			stm2 = 255;
		}
		else if (strncmp (str, "none", 4) == 0) {
			str += 4;
			stm1 = 0;
			stm2 = 255;
			incl = !incl;
		}
		else {
			stm1 = (unsigned) strtoul (str, &tmp, 0);
			if (tmp == str) {
				return (1);
			}

			str = tmp;

			if (*str == '-') {
				str += 1;
				stm2 = (unsigned) strtoul (str, &tmp, 0);
				if (tmp == str) {
					return (1);
				}
				str = tmp;
			}
			else {
				stm2 = stm1;
			}
		}

		if (incl) {
			for (i = stm1; i <= stm2; i++) {
				stm[i] |= msk;
			}
		}
		else {
			for (i = stm1; i <= stm2; i++) {
				stm[i] &= ~msk;
			}
		}

		str = str_skip_white (str);

		if (*str == '/') {
			str += 1;
		}
	}

	return (0);
}

char *mpeg_get_name (const char *base, unsigned sid)
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

int mpeg_stream_excl (unsigned char sid, unsigned char ssid)
{
	if ((par_stream[sid] & PAR_STREAM_SELECT) == 0) {
		return (1);
	}

	if (sid == 0xbd) {
		if ((par_substream[ssid] & PAR_STREAM_SELECT) == 0) {
			return (1);
		}
	}

	return (0);
}

/* check if packet is valid. returns 0 if it is. */
int mpeg_packet_check (mpeg_demux_t *mpeg)
{
	if ((par_packet_max > 0) && (mpeg->packet.size > par_packet_max)) {
		return (1);
	}

	if (par_stream[mpeg->packet.sid] & PAR_STREAM_INVALID) {
		return (1);
	}

	return (0);
}

void mpeg_print_stats (mpeg_demux_t *mpeg, FILE *fp)
{
	unsigned i;

	fprintf (fp,
		"System headers: %lu\n"
		"Packs:          %lu\n"
		"Packets:        %lu\n"
		"End codes:      %lu\n"
		"Skipped:        %lu bytes\n",
		mpeg->shdr_cnt, mpeg->pack_cnt, mpeg->packet_cnt, mpeg->end_cnt,
		mpeg->skip_cnt
	);

	for (i = 0; i < 256; i++) {
		if (mpeg->streams[i].packet_cnt > 0) {
			fprintf (fp,
			"Stream %02x:      "
			"%lu packets / %" PRIuMAX " bytes\n",
				i, mpeg->streams[i].packet_cnt,
				(uintmax_t) mpeg->streams[i].size
			);
		}
	}

	for (i = 0; i < 256; i++) {
		if (mpeg->substreams[i].packet_cnt > 0) {
			fprintf (fp,
				"Substream %02x:   "
				"%lu packets / %" PRIuMAX " bytes\n",
				i, mpeg->substreams[i].packet_cnt,
				(uintmax_t) mpeg->substreams[i].size
			);
		}
	}

	fflush (fp);
}

int mpeg_copy (mpeg_demux_t *mpeg, FILE *fp, unsigned n)
{
	unsigned char buf[4096];
	unsigned      i, j;

	while (n > 0) {
		i = (n < 4096) ? n : 4096;

		j = mpegd_read (mpeg, buf, i);

		if (j > 0) {
			if (fwrite (buf, 1, j, fp) != j) {
				return (1);
			}
		}

		if (i != j) {
			return (1);
		}

		n -= i;
	}

	return (0);
}

int main (int argc, char **argv)
{
	unsigned i;
	int      r;
	unsigned id1, id2;
	char     **optarg;

	for (i = 0; i < 256; i++) {
		par_stream[i] = 0;
		par_substream[i] = 0;
		par_stream_map[i] = i;
		par_substream_map[i] = i;
	}

	while (1) {
		r = mpegd_getopt (argc, argv, &optarg, opts);

		if (r == GETOPT_DONE) {
			break;
		}

		if (r < 0) {
			return (1);
		}

		switch (r) {
		case '?':
			print_help();
			return (0);

		case 'a':
			par_dvdac3 = 1;
			break;

		case 'b':
			if (par_demux_name != NULL) {
				free (par_demux_name);
			}
			par_demux_name = str_clone (optarg[0]);
			break;

		case 'c':
			par_mode = PAR_MODE_SCAN;

			for (i = 0; i < 256; i++) {
				par_stream[i] |= PAR_STREAM_SELECT;
				par_substream[i] |= PAR_STREAM_SELECT;
			}
			break;

		case 'd':
			par_mode = PAR_MODE_DEMUX;
			break;

		case 'D':
			par_drop = 0;
			break;

		case 'e':
			par_no_end = 1;
			break;

		case 'E':
			par_empty_pack = 1;
			break;

		case 'F':
			par_first_pts = 1;
			break;

		case 'h':
			par_no_shdr = 1;
			break;

		case 'i':
			if (strcmp (optarg[0], "-") == 0) {
				for (i = 0; i < 256; i++) {
					if (par_stream[i] & PAR_STREAM_SELECT) {
						par_stream[i] &= ~PAR_STREAM_INVALID;
					}
					else {
						par_stream[i] |= PAR_STREAM_INVALID;
					}
				}
			}
			else {
				if (str_get_streams (optarg[0], par_stream, PAR_STREAM_INVALID)) {
					prt_err ("%s: bad stream id (%s)\n", argv[0], optarg[0]);
					return (1);
				}
			}
			break;

		case 'k':
			par_no_pack = 1;
			break;

		case 'K':
			par_remux_skipped = 1;
			break;

		case 'l':
			par_mode = PAR_MODE_LIST;
			break;

		case 'm':
			par_packet_max = (unsigned) strtoul (optarg[0], NULL, 0);
			break;

		case 'p':
			if (str_get_streams (optarg[0], par_substream, PAR_STREAM_SELECT)) {
				prt_err ("%s: bad substream id (%s)\n", argv[0], optarg[0]);
				return (1);
			}
			break;

		case 'P':
			id1 = (unsigned) strtoul (optarg[0], NULL, 0);
			id2 = (unsigned) strtoul (optarg[1], NULL, 0);
			par_substream_map[id1 & 0xff] = id2 & 0xff;
			break;

		case 'r':
			par_mode = PAR_MODE_REMUX;
			break;

		case 's':
			if (str_get_streams (optarg[0], par_stream, PAR_STREAM_SELECT)) {
				prt_err ("%s: bad stream id (%s)\n", argv[0], optarg[0]);
				return (1);
			}
			break;

		case 'S':
			id1 = (unsigned) strtoul (optarg[0], NULL, 0);
			id2 = (unsigned) strtoul (optarg[1], NULL, 0);
			par_stream_map[id1 & 0xff] = id2 & 0xff;
			break;

		case 't':
			par_no_packet = 1;
			break;

		case 'u':
			par_dvdsub = 1;
			break;

		case 'V':
			print_version();
			return (0);

		case 'x':
			par_split = 1;
			break;

		case 0:
			if (par_inp == NULL) {
				if (strcmp (optarg[0], "-") == 0) {
					par_inp = stdin;
				}
				else {
					par_inp = fopen (optarg[0], "rb");
				}
				if (par_inp == NULL) {
					prt_err (
						"%s: can't open input file (%s)\n",
						argv[0], optarg[0]
					);
					return (1);
				}
			}
			else if (par_out == NULL) {
				if (strcmp (optarg[0], "-") == 0) {
					par_out = stdout;
				}
				else {
					par_out = fopen (optarg[0], "wb");
				}
				if (par_out == NULL) {
					prt_err (
						"%s: can't open output file (%s)\n",
						argv[0], optarg[0]
					);
					return (1);
				}
			}
			else {
				prt_err ("%s: too many files (%s)\n",
					argv[0], optarg[0]
				);
				return (1);
			}
			break;

		default:
			return (1);
		}
	}

	if (par_inp == NULL) {
		par_inp = stdin;
	}

	if (par_out == NULL) {
		par_out = stdout;
	}

	switch (par_mode) {
	case PAR_MODE_SCAN:
		r = mpeg_scan (par_inp, par_out);
		break;

	case PAR_MODE_LIST:
		r = mpeg_list (par_inp, par_out);
		break;

	case PAR_MODE_REMUX:
		r = mpeg_remux (par_inp, par_out);
		break;

	case PAR_MODE_DEMUX:
		r = mpeg_demux (par_inp, par_out);
		break;

	default:
		r = 1;
		break;
	}

	if (r) {
		return (1);
	}

	return (0);
}
