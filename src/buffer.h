/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:     buffer.h                                                   *
 * Created:       2003-04-08 by Hampa Hug <hampa@hampa.ch>                   *
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

/* $Id: buffer.h,v 1.1 2003/04/08 19:01:57 hampa Exp $ */


#ifndef MPEGDEMUX_BUFFER_H
#define MPEGDEMUX_BUFFER_H 1


#include "config.h"
#include "mpeg_parse.h"


typedef struct {
  unsigned char *buf;
  unsigned      cnt;
  unsigned      max;
} mpeg_buffer_t;



void mpeg_buf_init (mpeg_buffer_t *buf);
void mpeg_buf_free (mpeg_buffer_t *buf);
void mpeg_buf_clear (mpeg_buffer_t *buf);
int mpeg_buf_set_max (mpeg_buffer_t *buf, unsigned max);
int mpeg_buf_set_cnt (mpeg_buffer_t *buf, unsigned cnt);
int mpeg_buf_read (mpeg_buffer_t *buf, mpeg_demux_t *mpeg, unsigned cnt);
int mpeg_buf_write (mpeg_buffer_t *buf, FILE *fp);
int mpeg_buf_write_clear (mpeg_buffer_t *buf, FILE *fp);


#endif
