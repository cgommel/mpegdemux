/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:     mpegdemux.c                                                *
 * Created:       2003-02-01 by Hampa Hug <hampa@hampa.ch>                   *
 * Last modified: 2003-02-04 by Hampa Hug <hampa@hampa.ch>                   *
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

/* $Id: mpegdemux.c,v 1.6 2003/02/04 22:16:17 hampa Exp $ */


#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "message.h"
#include "mpeg_parse.h"
#include "mpeg_list.h"
#include "mpeg_remux.h"
#include "mpeg_demux.h"
#include "mpegdemux.h"


static int    par_list = 1;
static int    par_remux = 0;
static int    par_demux = 0;

static FILE   *par_inp = NULL;
static FILE   *par_out = NULL;

unsigned char par_stream[256];
unsigned char par_substream[256];
int           par_one_shdr = 0;
int           par_one_pack = 0;
unsigned char par_first = 0;
int           par_dvdac3 = 0;

char          *par_demux_name = NULL;


static
void prt_help (void)
{
  fputs (
    "usage: mpegdemux [options]\n"
    "  -l, --list               List packets [default]\n"
    "  -r, --remux              Copy modified input to output\n"
    "  -d, --demux              Demux streams\n"
    "  -s, --stream id          Select streams [none]\n"
    "  -p, --substream id       Select substreams [none]\n"
    "  -b, --base-name name     Set the base name for demuxed streams\n"
    "  -h, --one-system-header  Repeat system headers [no]\n"
    "  -k, --one-pack           Repeat packs [no]\n"
    "  -a, --ac3                Assume DVD AC3 headers\n",
    stdout
  );
}

static
void prt_version (void)
{
  fputs (
    "mpegdemux version " MPEGDEMUX_VERSION_STR
    " (compiled " __DATE__ " " __TIME__ ")\n\n"
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

int mpeg_stream_mark (unsigned char sid, unsigned char ssid)
{
  int r;

  if (sid == 0xbd) {
    r = (par_substream[ssid] & PAR_STREAM_SEEN) != 0;

    par_stream[sid] |= PAR_STREAM_SEEN;
    par_substream[ssid] |= PAR_STREAM_SEEN;

    if (par_stream[sid] & PAR_STREAM_EXCLUDE) {
      return (1);
    }
    if (par_substream[ssid] & PAR_STREAM_EXCLUDE) {
      return (1);
    }

    if (par_first) {
      return (r);
    }

    return (0);
  }

  r = (par_stream[sid] & PAR_STREAM_SEEN) != 0;

  par_stream[sid] |= PAR_STREAM_SEEN;

  if (par_stream[sid] & PAR_STREAM_EXCLUDE) {
    return (1);
  }

  if (par_first) {
    return (r);
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
    if (str_isarg2 (argv[argi], "-l", "--list")) {
      par_list = 1;
      par_remux = 0;
      par_demux = 0;
    }
    else if (str_isarg2 (argv[argi], "-r", "--remux")) {
      par_list = 0;
      par_remux = 1;
      par_demux = 0;
    }
    else if (str_isarg2 (argv[argi], "-d", "--demux")) {
      par_list = 0;
      par_remux = 0;
      par_demux = 1;
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
    else if (str_isarg2 (argv[argi], "-a", "--ac3")) {
      par_dvdac3 = 1;
    }
    else if (str_isarg2 (argv[argi], "-h", "--one-system-header")) {
      par_one_shdr = 1;
    }
    else if (str_isarg2 (argv[argi], "-k", "--one-packs")) {
      par_one_pack = 1;
    }
    else if (str_isarg2 (argv[argi], "-f", "--first")) {
      par_first = 1;
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

  if (par_list) {
    r = mpeg_list (par_inp, par_out);
  }
  else if (par_remux) {
    r = mpeg_remux (par_inp, par_out);
  }
  else if (par_demux) {
    r = mpeg_demux (par_inp, par_out);
  }
  else {
    r = 1;
  }

  if (r) {
    return (1);
  }

  return (0);
}
