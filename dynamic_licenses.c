#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <pthread.h>

#include "slurm/slurm.h"
#include "src/common/list.h"
#include "src/common/log.h"
#include "src/common/macros.h"
#include "src/common/xmalloc.h"
/*
#include "src/common/xstring.h"
*/
#include "src/slurmctld/licenses.h"


#include "dynamic_licenses.h"

#define DYN_LIC_BUF_SIZE 1024

static int cancel_flag=0;
static volatile int be_active_flag=0;

extern int _license_find_rec(void *x, void *key);

static void handler(int signal, siginfo_t *info, void *signal_body)
{
    be_active_flag=1;
    return;
}


void stop_dynamic_licenses_agent()
{
    cancel_flag=1;
    return;
}

void *dynamic_licenses_agent(void *args)
{
    int fds[2];

    static char buf[DYN_LIC_BUF_SIZE];

    FILE *f;

    timer_t timer_id;
    struct sigevent sev;
    sigset_t sig_set;
    siginfo_t sig_info;
    struct itimerspec its;
    struct sigaction sig_action;

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = &timer_id;

    sigemptyset(&sig_set);
    sigaddset(&sig_set, SIGRTMIN);

    sig_action.sa_handler=NULL;
    sig_action.sa_sigaction=handler;
    /*
    memcpy(&sig_action.sa_mask,&sig_set,sizeof(sigset_t));
    */
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags=SA_SIGINFO|SA_RESTART;
    sig_action.sa_restorer=NULL;

    sigaction(SIGRTMIN,&sig_action,NULL);

    /*
    pthread_sigmask(SIG_BLOCK, &sig_set , NULL);
    */

#if 0
    if(timer_create(CLOCK_MONOTONIC,NULL,&timer_id))
    {
        error("dynamic_licenses: can't create timer '%m'",strerror(errno));
        return NULL;
    }

    its.it_value.tv_sec = 60;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 60;
    its.it_interval.tv_nsec = 0;


    if (timer_settime(timer_id, 0, &its, NULL) == -1)
    {
        error("dynamic_licenses: set timer '%m'",strerror(errno));
        return NULL;
    }
#endif

    while(!cancel_flag)
    {
        pid_t pid;
        int symb;

        pipe(fds);

        pid=fork();
        if(pid==0)
        {
            dup2(fds[1],1);
            close(fds[0]);
            close(fds[1]);
            execlp("/path/to/lmutil","lmutil","lmstat","-a","-c","1055@server.in.some.place",NULL);
            error("dynamic_licenses: lmutil run error because '%m'",strerror(errno));
            return NULL;
        }

        if(pid<0)
        {
            error("dynamic_licenses: Can't create process");
            close(fds[0]);
            close(fds[1]);
            return NULL;
        }

        close(fds[1]);

        f=fdopen(fds[0],"r");
        if(f==NULL)
        {
            error("dynamic_licenses: Can't attach pipe to FILE*");
            close(fds[1]);
            return NULL;
        }

        while(fgets(buf,DYN_LIC_BUF_SIZE,f)!=NULL)
        {
            char *where=NULL;
            where=strstr(buf,"Users of ");
            if(where!=NULL)
            {

                char *string_after_license_name=NULL;
                char *dynamic_license_name=NULL;
                char *string_pointer=NULL;
                unsigned long int available_licenses=0;
                unsigned long int used_licenses=0;


                /*
                 * Buildig license name from string
                 */
                licenses_t *license_entry=NULL;

                where+=strlen("Users of ");

                for
                (
                    string_after_license_name=where;
                    *string_after_license_name!=':';
                    string_after_license_name++
                )
                {
                    /* nop */;
                }
                *string_after_license_name='\0';
                string_after_license_name++;

                dynamic_license_name=xmalloc(
                                         strlen(where) +
                                         8 /* as result of  strlen("dynamic_") */ +
                                         1 /* for end of line */
                                     );
                strcpy(dynamic_license_name,"dynamic_");
                strcat(dynamic_license_name,where);


                for
                (
                    string_pointer=string_after_license_name;
                    ( *string_pointer!='\0' ) &&  !isdigit(*string_pointer);
                    *string_pointer++
                )
                {
                    /* nop */
                }
                available_licenses=strtoul(string_pointer,&string_after_license_name,10);

                for
                (
                    string_pointer=string_after_license_name;
                    ( *string_pointer!='\0' ) &&  !isdigit(*string_pointer);
                    *string_pointer++
                )
                {
                    /* nop */
                }
                used_licenses=strtoul(string_pointer,&string_after_license_name,10);


                /*
                 * Building license entry
                 */
                license_entry = xmalloc(sizeof(licenses_t));
                license_entry->name=dynamic_license_name;
                license_entry->remote=0;
                license_entry->total=available_licenses;
                license_entry->used=used_licenses;

                {
                    licenses_t *match=NULL;

                    pthread_mutex_lock(&license_mutex);

                    if (license_list!=NULL)
                    {
                        match = list_find_first(license_list, _license_find_rec, license_entry->name);
                        if(match!=NULL)
                        {
                            match->total=license_entry->total;
                            match->used=license_entry->used;
                            xfree(dynamic_license_name);
                            xfree(license_entry);
                        }
                        else
                        {
                            list_push(license_list, license_entry);
                        }
                    }


                    pthread_mutex_unlock(&license_mutex);
                }

            }
        } /* End while over FILE* */

        fclose(f);
        waitpid(pid,NULL,0);

        verbose("Iteration before sigwaitinfo");

        /*
        if(!be_active_flag)
        {
        	pause();
        }
        else
        {
        	be_active_flag=0;
        }
            */
        sleep(60);

        verbose("Iteration");

    }
    return NULL;
} /* End thread function */

