/*****************************************************************************
 * mpegdemux                                                                 *
 *****************************************************************************/

/*****************************************************************************
 * File name:     message.h                                                  *
 * Created:       2003-02-02 by Hampa Hug <hampa@hampa.ch>                   *
 * Last modified: 2003-02-02 by Hampa Hug <hampa@hampa.ch>                   *
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

/* $Id: message.h,v 1.1 2003/02/02 20:26:13 hampa Exp $ */


#ifndef MPEGDEMUX_MESSAGE_H
#define MPEGDEMUX_MESSAGE_H 1


#define MSG_ERR   0
#define MSG_MSG   1
#define MSG_INFO  2
#define MSG_DEBUG 3


void msg_set_level (unsigned level);
unsigned msg_get_level (void);

void prt_msg (unsigned level, const char *msg, ...);
void prt_err (const char *msg, ...);


#endif
