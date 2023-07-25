#include <stdio.h>
#include <sample-encode-video.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <comm_def.h>
#include <lux_base.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include<sys/prctl.h>

int save_sampile(void)
{
    printf("**************** start save media sample! ****************\n");

  	int KeepAlive = 0;
    int ping_success = 0;
    //char a = 'V';
    char buf[1024] = {0};
    int offline_time = 0;
    char tmpCmd[128] = {0};
    FILE *fp = NULL;
    FILE *fp_watchdog = NULL;
    int index = 0;
    int index_old = 0;

    while(1)
    {
        memset(buf, 0x0, sizeof(buf));
        KeepAlive = 0;
        LUX_BASE_System_CMD("/bin/ps > /tmp/sample_pid");

        fp = fopen("/tmp/sample_pid", "r");
        if(fp == NULL)
        {
            printf("open file /tmp/sample_pid fail");
            return -1;
        }

        while(fgets(buf, sizeof(buf), fp) != NULL)
        {
            if(strstr(buf, "media_sample") != NULL && strstr(buf, "save") == NULL)
            {
                KeepAlive = 1;
                break;
            }
        }

        fclose(fp);

        fp_watchdog = fopen("/dev/watchdog", "w");
        if(fp_watchdog == NULL)
        {
            printf("**************** reboot system! ****************");
            return 0;
        }

        fprintf(fp_watchdog, "a");

        fclose(fp_watchdog);

        if(index - index_old >= 30)
        {
            index_old = index;
            fp = NULL;
            fp = fopen("/system/etc/first_set_wifi", "r");
            if(fp != NULL)
            {
                fclose(fp);
                fp = NULL;
                memset(buf, 0x0, sizeof(buf));
                ping_success = 0;

                system("/bin/ping -c 1 -w 1 www.tuya.com > /tmp/result_ping &");

                fp = fopen("/tmp/result_ping", "r");
                if(fp != NULL)
                {
                    printf("open file /tmp/result_ping fail");
                    //return -1;


                    while(fgets(buf, sizeof(buf), fp) != NULL)
                    {
                        if(strstr(buf, "1 packets received") != NULL)
                        {
                            printf("\r\n[%s]%d: buf = %s\n", __func__, __LINE__, buf);
                            ping_success = 1;
                            offline_time = 0;
                            break;
                        }
                    }

                    fclose(fp);
                }

                if(ping_success == 0)
                {

                    sprintf(tmpCmd, "killall -9 udhcpc");
                    //LUX_BASE_System_CMD(tmpCmd);
                    sprintf(tmpCmd, "/sbin/udhcpc -i wlan0 -q &");
                    //system(tmpCmd);

                    offline_time++;
                    printf("\r\n[%s]%d: offline_time = %d\n", __func__, __LINE__, offline_time);
                }
            }
        }
        index++;

        if(KeepAlive != 1)
        {
            printf("**************** reboot system! ****************\n");
            LUX_BASE_System_CMD("/bin/echo 1 > "UNNORMAL_REBOOT_FILE_NAME);
            LUX_BASE_System_CMD("/usr/bin/free > /system/etc/result_free");
            LUX_BASE_System_CMD("/bin/mpstat > /system/etc/result_mpstat");
            return 0;
        }

        fp_watchdog = fopen("/dev/watchdog", "w");
        if(fp_watchdog == NULL)
        {
            printf("**************** reboot system! ****************");
            return 0;
        }

        fprintf(fp_watchdog, "a");

        fclose(fp_watchdog);

        sleep(1);
    }

    return 0;
}


void signalHandler( int signum )
{
    char cThreadName[32] = {0};

    prctl(PR_GET_NAME, (unsigned long)cThreadName);

    switch(signum)
    {
        case SIGABRT:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGABRT(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;
        case SIGALRM:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGALRM(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;
        case SIGBUS:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGBUS(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;
        case SIGCHLD:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGCHLD(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;
        case SIGCONT:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGCONT(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;
        case SIGEMT:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGEMT(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;
        case SIGFPE:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGFPE(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;
        case SIGHUP:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGHUP(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;
        case SIGILL:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGILL(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;
        case SIGINT:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGINT(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;
        case SIGIO:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGIO(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;
        /*case SIGIOT:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGIOT(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;*/
        case SIGKILL:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGKILL(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;
        case SIGPIPE:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGPIPE(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;
        /*case SIGPOLL:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGPOLL(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;*/
        case SIGPROF:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGPROF(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;
        case SIGPWR:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGPWR(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;
        case SIGQUIT:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGQUIT(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;
        case SIGSEGV:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGSEGV(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;
        case SIGSTOP:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGSTOP(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;
        case SIGSYS:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGSYS(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;
        case SIGTERM:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGTERM(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;
        case SIGTRAP:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGTRAP(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;
        case SIGTSTP:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGTSTP(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;
        case SIGTTIN:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGTTIN(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;
        case SIGTTOU:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGTTOU(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;
        case SIGURG:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGURG(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;
        case SIGVTALRM:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGVTALRM(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;
        case SIGWINCH:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGWINCH(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;
        case SIGXCPU:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGXCPU(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;
        case SIGXFSZ:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = SIGXFSZ(%d) thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, signum, syscall(__NR_gettid), cThreadName);
          break;
        default:
          printf("\r\n ++++++++++++++++++ [%s]%d: signal = unkown signal thread = %ld name = %s++++++++++++++++++\n",
            __func__, __LINE__, syscall(__NR_gettid), cThreadName);
          break;
    }

    return;

}

int main(int argc, char *argv[])
{

    signal(SIGPIPE, signalHandler);

    if(argc >=2 && strcmp(argv[1], "save") == 0)
        save_sampile();
    else
    {
        printf("**************** start media_sample program! ****************\n");
        LUSHARE_test_sample_Encode_video(argc, argv[1]);
    }


    return 0;

}
