/*
 * Portions Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Portions Copyright (c) 1996, Jason Downs.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * From: OpenBSD: logutmp.c,v 1.1 1996/06/18 10:09:23 downsj Exp
 * From: OpenBSD: logutmp.c,v 1.2 1998/07/13 02:11:17 millert Exp
 */
char logutmp_rcsid[] =
  "$Id: logutmp.c,v 1.4 1999/07/16 01:12:54 dholland Exp $";

#include <sys/types.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <utmp.h>
#include <stdio.h>
#include <string.h>
#ifndef __linux__
#include <ttyent.h>
#endif
#include "extern.h"

typedef struct utmp UTMP;

#ifndef __linux__
static int fd = -1;
static int topslot = -1;
#endif

/*
 * Special versions of login()/logout() which hold the utmp file open,
 * for use with ftpd.
 */

void
login(const UTMP *ut)
{
#ifndef __linux__
	UTMP ubuf;

	/*
	 * First, loop through /etc/ttys, if needed, to initialize the
	 * top of the tty slots, since ftpd has no tty.
	 */
	if (topslot < 0) {
		topslot = 0;
		while (getttyent() != (struct ttyent *)NULL)
			topslot++;
	}
	if ((topslot < 0) || ((fd < 0)
	    && (fd = open(_PATH_UTMP, O_RDWR|O_CREAT, 0644)) < 0))
	    	return;

	/*
	 * Now find a slot that's not in use...
	 */
	(void)lseek(fd, (off_t)(topslot * sizeof(UTMP)), SEEK_SET);

	while (1) {
		if (read(fd, &ubuf, sizeof(UTMP)) == sizeof(UTMP)) {
			if (!ubuf.ut_name[0]) {
				(void)lseek(fd, -(off_t)sizeof(UTMP), SEEK_CUR);
				break;
			}
			topslot++;
		} else {
			(void)lseek(fd, (off_t)(topslot * sizeof(UTMP)), SEEK_SET);
			break;
		}
	}

	(void)write(fd, ut, sizeof(UTMP));
#else
	(void)ut;
#endif
}

int
logout(register const char *line)
{
#ifndef __linux__
	UTMP ut;
	int rval;

	rval = 0;
	if (fd < 0)
		return(rval);

	(void)lseek(fd, 0, SEEK_SET);

	while (read(fd, &ut, sizeof(UTMP)) == sizeof(UTMP)) {
		if (!ut.ut_name[0]
		    || strncmp(ut.ut_line, line, UT_LINESIZE))
			continue;
		bzero(ut.ut_name, UT_NAMESIZE);
		bzero(ut.ut_host, UT_HOSTSIZE);
		(void)time(&ut.ut_time);
		(void)lseek(fd, -(off_t)sizeof(UTMP), SEEK_CUR);
		(void)write(fd, &ut, sizeof(UTMP));
		rval = 1;
	}
	return(rval);
#else
	(void)line;
	return 1;
#endif
}
