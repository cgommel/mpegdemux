/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:     mpegdemux.c                                                *
 * Created:       2003-02-01 by Hampa Hug <hampa@hampa.ch>                   *
 * Last modified: 2003-04-08 by Hampa Hug <hampa@hampa.ch>                   *
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

/* $Id: mpegdemux.c,v 1.17 2003/04/08 19:19:30 hampa Exp $ */


#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

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
int             par_no_shdr = 0;
int             par_no_pack = 0;
int             par_no_packet = 0;
int             par_no_end = 0;
int             par_empty_pack = 0;
int             par_drop = 1;
int             par_scan = 0;
int             par_dvdac3 = 0;
int             par_dvdsub = 0;
char            *par_demux_name = NULL;


static
void prt_help (void)
{
  fputs (
    "usage: mpegdemux [options] [input [output]]\n"
    "  -c, --scan               Scan the stream [default]\n"
    "  -l, --list               List the stream contents\n"
    "  -r, --remux              Copy modified input to output\n"
    "  -d, --demux              Demultiplex streams\n"
    "  -s, --stream id          Select streams [none]\n"
    "  -p, --substream id       Select substreams [none]\n"
    "  -b, --base-name name     Set the base name for demuxed streams\n"
    "  -h, --no-system-headers  Don't list system headers\n"
    "  -k, --no-packs           Don't list packs\n"
    "  -t, --no-packets         Don't list packets\n"
    "  -e, --no-end             Don't list end codes [no]\n"
    "  -i, --no-drop            Don't drop incomplete packets\n"
    "  -P, --empty-packs        Remux empty packs [no]\n"
    "  -a, --ac3                Assume DVD AC3 headers in private streams\n"
    "  -u, --spu                Assume DVD subtitles in private streams\n",
    stdout
  );
}

static
void prt_version (void)
{
  fputs (
    "mpegdemux version " MPEGDEMUX_VERSION_STR
    " (compiled " MPEGDEMUX_CFG_DATE " " MPEGDEMUX_CFG_TIME ")\n\n"
    "Copyright (C) 2003 Hampa Hug <hampa@hampa.ch>\n",
    stdout
  );
}

char *str_clone (const char *str)
{
  char *ret;

  ret = (char *) malloc (strlen (str) + 1);
  if (ret == NULL) {
    return (NULL);
  }

  strcpy (ret, str);

  return (ret);
}

int str_isarg1 (const char *str, const char *arg)
{
  if (strcmp (str, arg) == 0) {
    return (1);
  }

  return (0);
}

int str_isarg2 (const char *str, const char *arg1, const char *arg2)
{
  if (strcmp (str, arg1) == 0) {
    return (1);
  }

  if (strcmp (str, arg2) == 0) {
    return (1);
  }

  return (0);
}

const char *str_skip_white (const char *str)
{
  while ((*str == ' ') || (*str == '\t')) {
    str += 1;
  }

  return (str);
}

int str_get_streams (const char *str, unsigned char stm[256])
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
        stm[i] &= ~PAR_STREAM_EXCLUDE;
      }
    }
    else {
      for (i = stm1; i <= stm2; i++) {
        stm[i] |= PAR_STREAM_EXCLUDE;
      }
    }

    str = str_skip_white (str);

    if (*str == '/') {
      str += 1;
    }
  }

  return (0);
}

int mpeg_stream_excl (unsigned char sid, unsigned char ssid)
{
  if (par_stream[sid] & PAR_STREAM_EXCLUDE) {
    return (1);
  }

  if (sid == 0xbd) {
    if (par_substream[ssid] & PAR_STREAM_EXCLUDE) {
      return (1);
    }
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
      fprintf (fp, "Stream %02x:      %lu packets / %llu bytes\n",
        i, mpeg->streams[i].packet_cnt, mpeg->streams[i].size
      );
    }
  }

  for (i = 0; i < 256; i++) {
    if (mpeg->substreams[i].packet_cnt > 0) {
      fprintf (fp, "Substream %02x:   %lu packets / %llu bytes\n",
        i, mpeg->substreams[i].packet_cnt, mpeg->substreams[i].size
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
  int      argi;
  unsigned i;
  int      r;

  if (argc == 2) {
    if (str_isarg1 (argv[1], "--version")) {
      prt_version();
      return (0);
    }
    else if (str_isarg1 (argv[1], "--help")) {
      prt_help();
      return (0);
    }
  }

  for (i = 0; i < 256; i++) {
    par_stream[i] = PAR_STREAM_EXCLUDE;
    par_substream[i] = PAR_STREAM_EXCLUDE;
  }

  argi = 1;
  while (argi < argc) {
    if (str_isarg2 (argv[argi], "-c", "--scan")) {
      unsigned i;

      par_mode = PAR_MODE_SCAN;

      for (i = 0; i < 256; i++) {
        par_stream[i] &= ~PAR_STREAM_EXCLUDE;
        par_substream[i] &= ~PAR_STREAM_EXCLUDE;
      }
    }
    else if (str_isarg2 (argv[argi], "-l", "--list")) {
      par_mode = PAR_MODE_LIST;
    }
    else if (str_isarg2 (argv[argi], "-r", "--remux")) {
      par_mode = PAR_MODE_REMUX;
    }
    else if (str_isarg2 (argv[argi], "-d", "--demux")) {
      par_mode = PAR_MODE_DEMUX;
    }
    else if (str_isarg2 (argv[argi], "-s", "--stream")) {
      argi += 1;
      if (argi >= argc) {
        prt_err ("%s: missing stream id\n", argv[0]);
        return (1);
      }

      if (str_get_streams (argv[argi], par_stream)) {
        prt_err ("%s: bad stream id (%s)\n", argv[0], argv[argi]);
        return (1);
      }
    }
    else if (str_isarg2 (argv[argi], "-p", "--substream")) {
      argi += 1;
      if (argi >= argc) {
        prt_err ("%s: missing substream id\n", argv[0]);
        return (1);
      }

      if (str_get_streams (argv[argi], par_substream)) {
        prt_err ("%s: bad substream id (%s)\n", argv[0], argv[argi]);
        return (1);
      }
    }
    else if (str_isarg2 (argv[argi], "-b", "--base-name")) {
      argi += 1;
      if (argi >= argc) {
        prt_err ("%s: missing base name\n");
        return (1);
      }

      if (par_demux_name != NULL) {
        free (par_demux_name);
      }

      par_demux_name = str_clone (argv[argi]);
    }
    else if (str_isarg2 (argv[argi], "-h", "--no-system-headers")) {
      par_no_shdr = 1;
    }
    else if (str_isarg2 (argv[argi], "-k", "--no-packs")) {
      par_no_pack = 1;
    }
    else if (str_isarg2 (argv[argi], "-t", "--no-packets")) {
      par_no_packet = 1;
    }
    else if (str_isarg2 (argv[argi], "-e", "--no-end")) {
      par_no_end = 1;
    }
    else if (str_isarg2 (argv[argi], "-P", "--empty-packs")) {
      par_empty_pack = 1;
    }
    else if (str_isarg2 (argv[argi], "-i", "--no-drop")) {
      par_drop = 0;
    }
    else if (str_isarg2 (argv[argi], "-a", "--ac3")) {
      par_dvdac3 = 1;
    }
    else if (str_isarg2 (argv[argi], "-u", "--spu")) {
      par_dvdsub = 1;
    }
    else if ((argv[argi][0] != '-') || (argv[argi][1] == 0)) {
      if (par_inp == NULL) {
        if (strcmp (argv[argi], "-") == 0) {
          par_inp = stdin;
        }
        else {
          par_inp = fopen (argv[argi], "rb");
        }
        if (par_inp == NULL) {
          prt_err ("%s: can't open input file (%s)\n", argv[0], argv[argi]);
          return (1);
        }
      }
      else if (par_out == NULL) {
        if (strcmp (argv[argi], "-") == 0) {
          par_out = stdout;
        }
        else {
          par_out = fopen (argv[argi], "wb");
        }
        if (par_out == NULL) {
          prt_err ("%s: can't open output file (%s)\n", argv[0], argv[argi]);
          return (1);
        }
      }
      else {
        prt_err ("%s: too many files (%s)\n", argv[0], argv[argi]);
        return (1);
      }
    }
    else {
      prt_err ("%s: unknown parameter (%s)\n", argv[0], argv[argi]);
      return (1);
    }

    argi += 1;
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
