/*
 * C functions for use in tests.pyx
 */

/*****************************************************************************
 *       Copyright (C) 2010-2016 Jeroen Demeyer <J.Demeyer@UGent.be>
 *
 * cysignals is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cysignals is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with cysignals.  If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************************************/

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#if HAVE_WINDOWS_H
#include <windows.h>
#endif


static int on_alt_stack(void)
{
#if HAVE_SIGALTSTACK
    stack_t oss;
    sigaltstack(NULL, &oss);
    return oss.ss_flags & SS_ONSTACK;
#else
    return 0;
#endif
}


/* Wait ``ms`` milliseconds */
static void ms_sleep(long ms)
{
#if HAVE_UNISTD_H
    usleep(1000 * ms);
#else
    Sleep(ms);
#endif
}

#ifdef POSIX
/* Signal process ``killpid`` with signal ``signum`` after ``ms``
 * milliseconds.  Wait ``interval`` milliseconds, then signal again.
 * Repeat this until ``n`` signals have been sent.
 *
 * This works as follows:
 *  - create a first child process in a new process group
 *  - the main process waits until the first child process terminates
 *  - this child process creates a second child process
 *  - the second child process kills the first child process
 *  - the main process sees that the first child process is killed
 *    and continues running Python code
 *  - the second child process does the actual waiting and signalling
 */
static void signal_pid_after_delay(int signum, pid_t killpid, long ms, long interval, int n)
{
    /* Flush all buffers before forking (otherwise we end up with two
     * copies of each buffer). */
    fflush(stdout);
    fflush(stderr);

    pid_t child1 = fork();
    if (child1 == -1) {perror("fork"); exit(1);}

    if (!child1)
    {
        /* This is child process 1 */
        child1 = getpid();

        /* New process group to prevent us getting the signals. */
        setpgid(0,0);

        /* Unblock SIGINT (to fix a warning when testing sig_block()) */
        cysigs.block_sigint = 0;

        /* Make sure SIGTERM simply terminates the process */
        signal(SIGTERM, SIG_DFL);

        pid_t child2 = fork();
        if (child2 == -1) exit(1);

        if (!child2)
        {
            /* This is child process 2 */
            kill(child1, SIGTERM);

            /* Signal Python after delay */
            ms_sleep(ms);
            for (;;)
            {
                kill(killpid, signum);
                if (--n == 0) exit(0);
                ms_sleep(interval);
            }
        }

        /* Wait to be killed by child process 2... */
        /* We use a 2-second timeout in case there is trouble. */
        ms_sleep(2000);
        exit(2);  /* This should NOT be reached */
    }

    /* Main Python process, continue when child 1 finishes */
    int wait_status;
    waitpid(child1, &wait_status, 0);
}

/* Signal the Python process */
#define signal_after_delay(signum, ms) signal_pid_after_delay(signum, getpid(), ms, 0, 1)

/* The same as above, but sending ``n`` signals */
#define signals_after_delay(signum, ms, interval, n) signal_pid_after_delay(signum, getpid(), ms, interval, n)
#else

/* Struct to pass data to thread */
typedef struct {
    int signum;
    long ms;
} sad_data, *psad_data;

/* Signal process  with signal ``signum`` after ``ms``
 * milliseconds.
 */
DWORD WINAPI thread_signal_after_delay(LPVOID lpParam) {
    psad_data param = (psad_data) lpParam;
    ms_sleep(param->ms);
    fprintf(stdout, "call raise(%i), ms:%i\n", param->signum, param->ms);
    raise(param->signum);
    fprintf(stdout, "after raise call\n");
//    Sleep(2000);
    return 0;
}

//DWORD WINAPI thread_signal_after_delay(void* data) {
//    Sleep(200);
//    printf("T1\n");
//    raise(SIGILL);
////    raise(SIGINT);
//    printf("Bob\n");
////    raise(SIGINT);
////    raise(SIGINT);
////    raise(SIGINT);
////    raise(SIGINT);
//    printf("Still here\n");
//    return 0;
//}

void signal_after_delay(int signum, long ms)
{
    psad_data param = (psad_data) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                sizeof(sad_data));
    param->signum = signum;
    param->ms = ms;

//    HANDLE thread = CreateThread(NULL, 0, thread_signal_after_delay, NULL, 0, NULL);
    HANDLE thread = CreateThread(NULL, 0, thread_signal_after_delay, param, 0, NULL);
    if(thread)
    {
//        raise(SIGINT);
        /* Wait 2 seconds for the signal raise */
        int ret = WaitForSingleObject(thread, 20000);
        switch(ret){
            case WAIT_ABANDONED: fprintf(stderr, "WAIT_ABANDONED\n");
                break;
            case WAIT_OBJECT_0: fprintf(stderr, "WAIT_OBJECT_0\n");
                break;
            case WAIT_TIMEOUT: fprintf(stderr, "WAIT_TIMEOUT\n");
                break;
            case WAIT_FAILED: fprintf(stderr, "WAIT_FAILED\n");
                break;
            default: fprintf(stderr, "event '%i'\n", ret);
        }
        fprintf(stderr, "raise(%i) has not killed the main thread\n", signum);
    }
    else
    {
        fprintf(stderr, "CreateThread failed\n");
    }
    fprintf(stdout, "OMG ! This sould not be reached.");
    exit(2); /* This should NOT be reached */
}

void signals_after_delay(int signum, long ms, long interval, int n) {}
#endif
