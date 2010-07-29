/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:   src/mpeg_ints.h                                              *
 * Created:     2010-07-29 by Hampa Hug <hampa@hampa.ch>                     *
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


#ifndef MPEG_INTS_H
#define MPEG_INTS_H 1


#include "config.h"


#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#else
typedef unsigned long uintmax_t;
typedef unsigned long uint64_t;
#define PRIuMAX "lu"
#define PRIxMAX "lx"
#endif


#endif
