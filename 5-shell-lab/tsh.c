/*
 * tsh - A tiny shell program with job control
 *
 * <Put your name and login ID here>
 */
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* Misc manifest constants */
#define MAXLINE 1024   /* max line size */
#define MAXARGS 128    /* max args on a command line */
#define MAXJOBS 16     /* max jobs at any point in time */
#define MAXJID 1 << 16 /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/*
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;   /* defined in libc */
char prompt[] = "tsh> "; /* command line prompt (DO NOT CHANGE) */
int verbose = 0;         /* if true, print additional output */
int nextjid = 1;         /* next job ID to allocate */
char sbuf[MAXLINE];      /* for composing sprintf messages */

struct job_t {             /* The job struct */
    pid_t pid;             /* job PID */
    int jid;               /* job ID [1, 2, ...] */
    int state;             /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE]; /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */

/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv);
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs);
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid);
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid);
int pid2jid(pid_t pid);
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/*
 * main - The shell's main routine
 */
int main(int argc, char **argv) {
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h': /* print help message */
            usage();
            break;
        case 'v': /* emit additional diagnostic info */
            verbose = 1;
            break;
        case 'p':            /* don't print a prompt */
            emit_prompt = 0; /* handy for automatic testing */
            break;
        default:
            usage();
        }
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT, sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler); /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler); /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler);

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

        /* Read command line */
        if (emit_prompt) {
            printf("%s", prompt);
            fflush(stdout);
        }
        if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
            app_error("fgets error");
        if (feof(stdin)) { /* End of file (ctrl-d) */
            fflush(stdout);
            exit(0);
        }

        /* Evaluate the command line */
        eval(cmdline);
        fflush(stdout);
        fflush(stdout);
    }

    exit(0); /* control never reaches here */
}

/*
 * eval - Evaluate the command line that the user has just typed in
 *
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.
 */
void eval(char *cmdline) {
    char *argv[MAXARGS];

    // 解析命令行, 返回后 argv 中包含指向所有参数的字符串指针:
    int need_bg = parseline(cmdline, argv);

    // 如果是内置命令则立即执行:
    if (builtin_cmd(argv)) {
        // 并且直接返回:
        return;
    }

    // 否则可能是可执行文件的路径.

    // 首先阻塞掉 SIGCHLD 信号, 确保 "创建子进程" 和 "将子进程所代表的作业添加进作业列表" 这两个动作的原子性:
    sigset_t set;
    sigset_t old_set;
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);

    if (sigprocmask(SIG_BLOCK, &set, &old_set) < 0) {
        printf("error in `eval`: %s\n", strerror(errno));
        exit(1);
    }

    // 调用 fork 创建新的子进程:
    pid_t pid;
    if ((pid = fork()) == 0) {
        // 子进程继承了父进程的信号, 因此首先应该复原被阻塞的 SIGCHLD 信号:
        if (sigprocmask(SIG_BLOCK, &old_set, NULL) < 0) {
            printf("error in `eval`: %s\n", strerror(errno));
            exit(1);
        }

        // 将子进程解耦为独立进程组:
        if (setpgid(0, 0) < 0) {
            printf("error in `eval`: %s\n", strerror(errno));
            exit(1);
        }

        // 子进程负责调用 execve 运行可执行文件:
        if (execve(argv[0], argv, environ) < 0) {
            // 如果 execve 函数返回, 说明没有找到可执行文件:
            printf("%s: Command not found\n", argv[0]);
            exit(0);
        }
    }

    // 添加进作业列表:
    addjob(jobs, pid, need_bg ? BG : FG, cmdline);

    // 取消阻塞 SIGCHLD 信号:
    if (sigprocmask(SIG_BLOCK, &old_set, NULL) < 0) {
        printf("error in `eval`: %s\n", strerror(errno));
        exit(1);
    }

    // 前台作业:
    if (!need_bg) {
        // 等待其终止:
        waitfg(pid);
    }

    // 后台作业:
    else {
        printf("[%d] (%d) %s", pid2jid(pid), pid, getjobpid(jobs, pid)->cmdline);
    }
}

/*
 * parseline - Parse the command line and build the argv array.
 *
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.
 */
int parseline(const char *cmdline, char **argv) {
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf) - 1] = ' ';   /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
        buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'') {
        buf++;
        delim = strchr(buf, '\'');
    } else {
        delim = strchr(buf, ' ');
    }

    while (delim) {
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while (*buf && (*buf == ' ')) /* ignore spaces */
            buf++;

        if (*buf == '\'') {
            buf++;
            delim = strchr(buf, '\'');
        } else {
            delim = strchr(buf, ' ');
        }
    }
    argv[argc] = NULL;

    if (argc == 0) /* ignore blank line */
        return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc - 1] == '&')) != 0) {
        argv[--argc] = NULL;
    }
    return bg;
}

/*
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.
 */
int builtin_cmd(char **argv) {
    // 内置命令 `quit`:
    if (strcmp(argv[0], "quit") == 0) {
        exit(0);
    }

    // 内置命令 `jobs`:
    if (strcmp(argv[0], "jobs") == 0) {
        listjobs(jobs);
        return 1;
    }

    // 内置命令 `fg` 和 `bg`:
    if (strcmp(argv[0], "fg") == 0 || strcmp(argv[0], "bg") == 0) {
        do_bgfg(argv);
        return 1;
    }

    // 如果不是内置命令则返回 false:
    return 0;
}

/*
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv) {
    // 如果没有参数:
    if (!argv[1]) {
        printf("%s command requires PID or %%jobid argument\n", argv[0]);
        return;
    }

    // 区分 JID / PID:
    int is_jid = argv[1][0] == '%';

    // 合理性检查, 确保第一个参数仅由数字构成:
    int i;
    size_t arg_1_string_length = strlen(argv[1]);
    for (i = is_jid; i < arg_1_string_length; i++) {
        if (!isdigit(argv[1][i])) {
            printf("%s: argument must be a PID or %%jobid\n", argv[0]);
            return;
        }
    }

    // 如果给出的是 JID 则解析 JID:
    int jid = is_jid ? atoi(argv[1] + 1) : 0;

    // 如果没有找到该 JID 对应的作业:
    if (is_jid && !getjobjid(jobs, jid)) {
        printf("%%%d: No such job\n", jid);
        return;
    }

    // 如果给出的是 JID 则通过 JID 获取 PID, 否则直接解析 PID:
    pid_t pid = is_jid ? getjobjid(jobs, jid)->pid : atoi(argv[1]);

    // 如果没有找到该 PID 对应的作业:
    if (!is_jid && !getjobpid(jobs, pid)) {
        printf("(%d): No such process\n", pid);
        return;
    }

    // 内置命令 `fg`:
    if (strcmp(argv[0], "fg") == 0) {
        // 发送唤醒信号给整个进程组:
        if (kill(-pid, SIGCONT) < 0) {
            printf("error in `do_bgfg`: %s\n", strerror(errno));
            exit(1);
        }

        // 立即更改作业状态为前台作业:
        getjobpid(jobs, pid)->state = FG;

        // 等待前台作业终止或停止:
        waitfg(pid);

        return;
    }

    // 内置命令 `bg`:
    if (strcmp(argv[0], "bg") == 0) {
        // 发送唤醒信号给整个进程组:
        if (kill(-pid, SIGCONT) < 0) {
            printf("error in `do_bgfg`: %s\n", strerror(errno));
            exit(1);
        }

        // 立即更改作业状态为后台作业:
        getjobpid(jobs, pid)->state = BG;

        // 打印提示信息:
        printf("[%d] (%d) %s", pid2jid(pid), pid, getjobpid(jobs, pid)->cmdline);

        return;
    }

    exit(1);
}

/*
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid) {
    // 只允许 `sigsuspend` 函数接收下面四种信号:
    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGQUIT);
    sigdelset(&set, SIGINT);
    sigdelset(&set, SIGTSTP);
    sigdelset(&set, SIGCHLD);

    struct job_t *job_ptr;

    while (1) {
        // 等待上面四种信号:
        sigsuspend(&set); // 这里不需要检查错误, 因为 `sigsuspend` 函数的预期行为就是无限期挂起当前进程,
                          // 也就是说报错的定义就是进程被唤醒

        // 注意: 到这里为止, 对应的信号处理程序是已经运行完成了的.

        // 如果依然能够找到前台作业并且该作业仍然在执行:
        if ((job_ptr = getjobpid(jobs, pid)) && job_ptr->state == FG) {
            continue;
        }

        // 否则说明前台作业要么已经终止 (并被信号处理程序回收), 要么暂时停止:
        return;
    }
}

/*****************
 * Signal handlers
 *****************/

/*
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.
 */
void sigchld_handler(int sig) {
    int status;
    pid_t pid;

    // 合理性检查:
    if (sig != SIGCHLD) {
        return;
    }

    // 循环检查, 直至没有子进程需要回收:
    while (1) {
        // 尝试回收一个子进程:
        pid = waitpid((pid_t)-1, &status, WUNTRACED | WNOHANG);

        // 如果发生错误:
        if (pid == (pid_t)-1) {
            // 如果没有子进程:
            if (errno == ECHILD) {
                // 正常情况, 直接返回:
                return;
            }

            // 否则报错:
            printf("error in `sigchld_handler`: %s\n", strerror(errno));
            exit(1);
        }

        // 如果没有需要回收的子进程:
        if (pid == (pid_t)0) {
            return;
        }

        // 否则成功回收到一个子进程, 下面需要检查其终止/停止原因.

        // 如果是正常运行结束:
        if (WIFEXITED(status)) {
            // 从作业列表中删除该作业:
            deletejob(jobs, pid);
        }

        // 如果是由于信号而终止:
        else if (WIFSIGNALED(status)) {
            // 打印提示信息:
            printf("Job [%d] (%d) terminated by signal %d\n", pid2jid(pid), pid, WTERMSIG(status));

            // 从作业列表中删除该作业:
            deletejob(jobs, pid);
        }

        // 如果是由于信号而停止:
        else if (WIFSTOPPED(status)) {
            // 如果是主进程接收的 SIGTSTP 信号:
            if (getjobpid(jobs, pid)->state == ST) {
                // 该做的工作已经做了, 直接下一个:
                continue;
            }

            // 否则是来自于子进程自身或是其他地方的信号, 需要替它们进行更改作业状态:
            getjobpid(jobs, pid)->state = ST;

            // 打印提示信息:
            printf("Job [%d] (%d) stopped by signal %d\n", pid2jid(pid), pid, WSTOPSIG(status));
        }

        // 否则报错:
        else {
            printf("未知执行状态\n");
            exit(1);
        }
    }
}

/*
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.
 */
void sigint_handler(int sig) {
    // 合理性检查:
    if (sig != SIGINT) {
        return;
    }

    pid_t pid;

    // 如果没有前台作业:
    if ((pid = fgpid(jobs)) == 0) {
        return;
    }

    // 否则将信号发送给整个进程组:
    if (kill(-pid, SIGINT) < 0) {
        printf("error in `sigint_handler`: %s\n", strerror(errno));
        exit(1);
    }
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.
 */
void sigtstp_handler(int sig) {
    // 合理性检查:
    if (sig != SIGTSTP) {
        return;
    }

    pid_t pid;

    // 如果没有前台作业:
    if ((pid = fgpid(jobs)) == 0) {
        return;
    }

    // 否则将信号发送给整个进程组:
    if (kill(-pid, SIGTSTP) < 0) {
        printf("error in `sigtstp_handler`: %s\n", strerror(errno));
        exit(1);
    }

    // 更改作业状态 (因为是信号的发送者负责更改作业状态):
    getjobpid(jobs, pid)->state = ST;

    // 打印提示信息:
    printf("Job [%d] (%d) stopped by signal %d\n", pid2jid(pid), pid, SIGTSTP);
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
        clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs) {
    int i, max = 0;

    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].jid > max)
            max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline) {
    int i;

    if (pid < 1)
        return 0;

    for (i = 0; i < MAXJOBS; i++) {
        if (jobs[i].pid == 0) {
            jobs[i].pid = pid;
            jobs[i].state = state;
            jobs[i].jid = nextjid++;
            if (nextjid > MAXJOBS)
                nextjid = 1;
            strcpy(jobs[i].cmdline, cmdline);
            if (verbose) {
                printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            return 1;
        }
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid) {
    int i;

    if (pid < 1)
        return 0;

    for (i = 0; i < MAXJOBS; i++) {
        if (jobs[i].pid == pid) {
            clearjob(&jobs[i]);
            nextjid = maxjid(jobs) + 1;
            return 1;
        }
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].state == FG)
            return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid) {
    int i;

    if (pid < 1)
        return NULL;
    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].pid == pid)
            return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid) {
    int i;

    if (jid < 1)
        return NULL;
    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].jid == jid)
            return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) {
    int i;

    if (pid < 1)
        return 0;
    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].pid == pid) {
            return jobs[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++) {
        if (jobs[i].pid != 0) {
            printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
            switch (jobs[i].state) {
            case BG:
                printf("Running ");
                break;
            case FG:
                printf("Foreground ");
                break;
            case ST:
                printf("Stopped ");
                break;
            default:
                printf("listjobs: Internal error: job[%d].state=%d ", i, jobs[i].state);
            }
            printf("%s", jobs[i].cmdline);
        }
    }
}
/******************************
 * end job list helper routines
 ******************************/

/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) {
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg) {
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg) {
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) {
    struct sigaction action, old_action;

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
        unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) {
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}
