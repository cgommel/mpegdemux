/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:   src/getopt.h                                                 *
 * Created:     2010-05-30 by Hampa Hug <hampa@hampa.ch>                     *
 * Copyright:   (C) 2010 Hampa Hug <hampa@hampa.ch>                          *
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


#ifndef MPEGDEMUX_GETOPT_H
#define MPEGDEMUX_GETOPT_H 1


#define GETOPT_DONE    -1
#define GETOPT_UNKNOWN -2
#define GETOPT_MISSING -3


typedef struct {
	short          name1;
	unsigned short argcnt;
	const char     *name2;
	const char     *argdesc;
	const char     *optdesc;
} mpegd_option_t;


void mpegd_getopt_help (const char *tag, const char *usage, mpegd_option_t *opt);

int mpegd_getopt (int argc, char **argv, char ***arg, mpegd_option_t *opt);


#endif
