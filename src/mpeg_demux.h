/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:   src/mpeg_demux.h                                             *
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


#ifndef MPEGDEMUX_MPEG_DEMUX_H
#define MPEGDEMUX_MPEG_DEMUX_H 1


#include "config.h"


int mpeg_demux (FILE *inp, FILE *out);


#endif
