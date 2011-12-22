#include <time/time.h>
#include <sys/reent.h>
#include <ppc/atomic.h>
#include <assert.h>
#include <xenon_uart/xenon_uart.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <time/time.h>
#include <xenon_smc/xenon_smc.h>

static unsigned int spinlock = 0;
static int lockcount = 0;

// miss in gligli libxenon
int stat(const char * __restrict path, struct stat * __restrict buf) {
    int fd = -1;
    fd = open(path, O_RDONLY);

    if (fd) {
        return fstat(fd, buf);
    }
    return ENOENT; // file doesn't exist
}

void __malloc_lock(struct _reent *_r) {
    //putch('m');
    assert(lockcount >= 0);

    if (!lockcount) {
        lock(&spinlock);
    }

    lockcount++;
}

void __malloc_unlock(struct _reent *_r) {
    //putch('u');
    assert(lockcount > 0);

    lockcount--;

    if (!lockcount) {
        unlock(&spinlock);
    }
}

void usleep(int i) {
    udelay(i);
}



#if 0
static unsigned int p_spinlock = 0;
static int p_lockcount = 0;

void __print_lock() {
    //putch('m');
    assert(p_lockcount >= 0);

    if (!p_lockcount) {
        lock(&p_spinlock);
    }

    p_lockcount++;
}

void __print_unlock() {
    //putch('u');
    assert(p_lockcount > 0);

    p_lockcount--;

    if (!p_lockcount) {
        unlock(&p_spinlock);
    }
}

int printf(const char *fmt, ...) {
    __print_lock();
    char printf_buf[1024];
    va_list args;
    int printed;

    va_start(args, fmt);
    printed = vsprintf(printf_buf, fmt, args);
    va_end(args);

    //wait a few
    //usleep(50000);
    puts(printf_buf);

    
    __print_unlock();
    return printed;
}
#endif