/* ====================================================================
 * Copyright (c) 1995-1998 The Apache Group.  All rights reserved.
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
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
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

/*
 * Routines in http_main.c which other code --- in particular modules ---
 * may want to call.  Right now, that's limited to timeout handling.
 * There are two functions which modules can call to trigger a timeout
 * (with the per-virtual-server timeout duration); these are hard_timeout
 * and soft_timeout.
 *
 * The difference between the two is what happens when the timeout
 * expires (or earlier than that, if the client connection aborts) ---
 * a soft_timeout just puts the connection to the client in an
 * "aborted" state, which will cause http_protocol.c to stop trying to
 * talk to the client, but otherwise allows the code to continue normally.
 * hard_timeout(), by contrast, logs the request, and then aborts it
 * completely --- longjmp()ing out to the accept() loop in http_main.
 * Any resources tied into the request's resource pool will be cleaned up;
 * everything that isn't will leak.
 *
 * soft_timeout() is recommended as a general rule, because it gives your
 * code a chance to clean up.  However, hard_timeout() may be the most
 * convenient way of dealing with timeouts waiting for some external
 * resource other than the client, if you can live with the restrictions.
 *
 * (When a hard timeout is in scope, critical sections can be guarded
 * with block_alarms() and unblock_alarms() --- these are declared in
 * alloc.c because they are most often used in conjunction with
 * routines to allocate something or other, to make sure that the
 * cleanup does get registered before any alarm is allowed to happen
 * which might require it to be cleaned up; they * are, however,
 * implemented in http_main.c).
 *
 * kill_timeout() will disarm either variety of timeout.
 *
 * reset_timeout() resets the timeout in progress.
 */

void start_shutdown(void);
void start_restart(int);
API_EXPORT(void) hard_timeout(char *, request_rec *);
void keepalive_timeout(char *, request_rec *);
API_EXPORT(void) soft_timeout(char *, request_rec *);
API_EXPORT(void) kill_timeout(request_rec *);
API_EXPORT(void) reset_timeout(request_rec *);

API_EXPORT(void) child_terminate(request_rec *r);
API_EXPORT(void) sync_scoreboard_image(void);
int update_child_status(int child_num, int status, request_rec *r);
void time_process_request(int child_num, int status);
unsigned int set_callback_and_alarm(void (*fn) (int), int x);
int check_alarm(void);

#ifndef NO_OTHER_CHILD
/*
 * register an other_child -- a child which the main loop keeps track of
 * and knows it is different than the rest of the scoreboard.
 *
 * pid is the pid of the child.
 *
 * maintenance is a function that is invoked with a reason, the data
 * pointer passed here, and when appropriate a status result from waitpid().
 *
 * write_fd is an fd that is probed for writing by select() if it is ever
 * unwritable, then maintenance is invoked with reason OC_REASON_UNWRITABLE.
 * This is useful for log pipe children, to know when they've blocked.  To
 * disable this feature, use -1 for write_fd.
 */
API_EXPORT(void) register_other_child(int pid,
       void (*maintenance) (int reason, void *data, int status), void *data,
				      int write_fd);
#define OC_REASON_DEATH		0	/* child has died, caller must call
					 * unregister still */
#define OC_REASON_UNWRITABLE	1	/* write_fd is unwritable */
#define OC_REASON_RESTART	2	/* a restart is occuring, perform
					 * any necessary cleanup (including
					 * sending a special signal to child)
					 */
#define OC_REASON_UNREGISTER	3	/* unregister has been called, do
					 * whatever is necessary (including
					 * kill the child) */
#define OC_REASON_LOST		4	/* somehow the child exited without
					 * us knowing ... buggy os? */

/*
 * unregister an other_child.  Note that the data pointer is used here, and
 * is assumed to be unique per other_child.  This is because the pid and
 * write_fd are possibly killed off separately.
 */
API_EXPORT(void) unregister_other_child(void *data);

#endif
