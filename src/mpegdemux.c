/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:     mpegdemux.c                                                *
 * Created:       2003-02-01 by Hampa Hug <hampa@hampa.ch>                   *
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

/* $Id: mpegdemux.c,v 1.3 2003/02/04 02:48:11 hampa Exp $ */


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
unsigned char par_rep_sh = 0;
unsigned char par_list_first = 0;

char          *par_demux_name = NULL;


static
void prt_help (void)
{
  fputs (
    "usage: mpegdemux [options]\n"
    "  -l, --list            List packets [default]\n"
    "  -r, --remux           Copy modified input to output\n"
    "  -d, --demux           Demux streams\n"
    "  -s, --stream id       Include a stream [all]\n"
    "  -x, --exclude id      Exclude a stream [none]\n"
    "  -i, --invert          Invert exclude mask\n"
    "  -h, --system-headers  Repeat system headers [no]\n"
    "  -b, --base-name name  Set the base name for demuxed streams\n",
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

int main (int argc, char **argv)
{
  int argi;
  int r;

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
      unsigned id;

      argi += 1;
      if (argi >= argc) {
        prt_err ("%s: missing stream id\n", argv[0]);
        return (1);
      }

      id = strtoul (argv[argi], NULL, 0) & 0xff;
      par_stream[id] &= ~PAR_STREAM_EXCLUDE;
    }
    else if (str_isarg2 (argv[argi], "-x", "--exclude")) {
      unsigned id;

      argi += 1;
      if (argi >= argc) {
        prt_err ("%s: missing stream id\n", argv[0]);
        return (1);
      }

      id = strtoul (argv[argi], NULL, 0) & 0xff;
      par_stream[id] |= PAR_STREAM_EXCLUDE;
    }
    else if (str_isarg2 (argv[argi], "-i", "--invert")) {
      unsigned i;

      for (i = 0; i < 256; i++) {
        if (par_stream[i] == 0) {
          par_stream[i] ^= PAR_STREAM_EXCLUDE;
        }
      }
    }
    else if (str_isarg2 (argv[argi], "-h", "--system-headers")) {
      par_rep_sh = 1;
    }
    else if (str_isarg2 (argv[argi], "-f", "--first")) {
      par_list_first = 1;
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
