/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:     message.c                                                  *
 * Created:       2003-02-02 by Hampa Hug <hampa@hampa.ch>                   *
 * Last modified: 2003-03-02 by Hampa Hug <hampa@hampa.ch>                   *
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

/* $Id: message.c,v 1.2 2003/03/02 11:19:49 hampa Exp $ */


#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "message.h"


static unsigned msg_level = MSG_DEBUG;


void msg_set_level (unsigned level)
{
  msg_level = level;
}

unsigned msg_get_level (void)
{
  return msg_level;
}

void prt_msg_va (unsigned level, const char *msg, va_list va)
{
  if (level <= msg_level) {
    vfprintf (stderr, msg, va);
    fflush (stderr);
  }
}

void prt_msg (unsigned level, const char *msg, ...)
{
  va_list va;

  if (level <= msg_level) {
    va_start (va, msg);
    prt_msg_va (level, msg, va);
    va_end (va);
  }
}

void prt_err (const char *msg, ...)
{
  va_list va;

  if (MSG_ERR <= msg_level) {
    va_start (va, msg);
    prt_msg_va (MSG_ERR, msg, va);
    va_end (va);
  }
}
