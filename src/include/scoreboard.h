/* ====================================================================
 * Copyright (c) 1995-1997 The Apache Group.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Group" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission.
 *
 * 5. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Group and was originally based
 * on public domain software written at the National Center for
 * Supercomputing Applications, University of Illinois, Urbana-Champaign.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#ifndef WIN32
#include <sys/times.h>
#endif

/* Scoreboard info on a process is, for now, kept very brief --- 
 * just status value and pid (the latter so that the caretaker process
 * can properly update the scoreboard when a process dies).  We may want
 * to eventually add a separate set of long_score structures which would
 * give, for each process, the number of requests serviced, and info on
 * the current, or most recent, request.
 *
 * Status values:
 */

#define SERVER_UNKNOWN (-1)	/* should never be in this state */
#define SERVER_DEAD 0
#define SERVER_READY 1          /* Waiting for connection (or accept() lock) */
#define SERVER_STARTING 3       /* Server Starting up */
#define SERVER_BUSY_READ 2      /* Reading a client request */
#define SERVER_BUSY_WRITE 4     /* Processing a client request */
#define SERVER_BUSY_KEEPALIVE 5 /* Waiting for more requests via keepalive */
#define SERVER_BUSY_LOG 6       /* Logging the request */
#define SERVER_BUSY_DNS 7       /* Looking up a hostname */
#define SERVER_GRACEFUL 8	/* server is gracefully finishing request */

/* A "virtual time" is simply a counter that indicates that a child is
 * making progress.  The parent checks up on each child, and when they have
 * made progress it resets the last_rtime element.  But when the child hasn't
 * made progress in a time that's roughly timeout_len seconds long, it is
 * sent a SIGALRM.
 *
 * vtime is an optimization that is used only when the scoreboard is in
 * shared memory (it's not easy/feasible to do it in a scoreboard file).
 * The essential observation is that timeouts rarely occur, the vast majority
 * of hits finish before any timeout happens.  So it really sucks to have to
 * ask the operating system to set up and destroy alarms many times during
 * a request.
 */
typedef unsigned vtime_t;

typedef struct {
    union {
	pid_t pid;		/* if it's not DEAD then this is the pid */
	int free_list;		/* otherwise this is scratch space */
    } x;
#ifdef OPTIMIZE_TIMEOUTS
    vtime_t last_vtime;		/* the last vtime the parent has seen */
    time_t last_rtime;		/* time(0) of the last change */
    vtime_t cur_vtime;		/* the child's current vtime */
    unsigned short timeout_len;	/* length of the timeout */
#endif
    signed char status;
#if defined(STATUS)
    unsigned long access_count;
    unsigned long bytes_served;
    unsigned long my_access_count;
    unsigned long my_bytes_served;
    unsigned long conn_bytes;
    unsigned short conn_count;
#if defined(NO_GETTIMEOFDAY)
    clock_t start_time;
    clock_t stop_time;
#else
    struct timeval start_time;
    struct timeval stop_time;
#endif
#ifndef NO_TIMES
    struct tms times;
#endif
    time_t last_used;
    char client[32];	/* Keep 'em small... */
    char request[64];	/* We just want an idea... */
    char vhost[32];     /* What virtual host is being accessed? */
#endif
} short_score;

typedef struct
    {
    int exit_generation;	/* Set by the main process if a graceful
				   restart is required */
    } global_score;

typedef struct
    {
    short_score servers[HARD_SERVER_LIMIT];
    global_score global;
    } scoreboard;

#define SCOREBOARD_SIZE		sizeof(scoreboard)

API_EXPORT(void) sync_scoreboard_image(void);
API_EXPORT(short_score) get_scoreboard_info(int x);
API_EXPORT(int) exists_scoreboard_image (void);

/* for time_process_request() in http_main.c */
#define START_PREQUEST 1
#define STOP_PREQUEST  2
