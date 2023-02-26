#include "../include/hook.h"
#include "../include/config.h"
#include "../include/log.h"
#include "../include/fiber.h"
#include "../include/iomanager.h"
#include "../include/fd_manager.h"
#include "../include/macro.h"
#include <dlfcn.h>

jivan::Logger::ptr g_logger = JIVAN_LOG_NAME("system");
namespace jivan {

    static jivan::ConfigVar<int>::ptr g_tcp_connect_timeout =
            jivan::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

    static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX) \
    XX(sleep) \
    XX(usleep) \
    XX(nanosleep) \
    XX(socket) \
    XX(connect) \
    XX(accept) \
    XX(read) \
    XX(readv) \
    XX(recv) \
    XX(recvfrom) \
    XX(recvmsg) \
    XX(write) \
    XX(writev) \
    XX(send) \
    XX(sendto) \
    XX(sendmsg) \
    XX(close) \
    XX(fcntl) \
    XX(ioctl) \
    XX(getsockopt) \
    XX(setsockopt)

    extern "C" {
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX) ;
#undef XX
    }

    void hook_init() {
        static bool is_inited = false;
        if (is_inited) {
            return;
        }
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
        HOOK_FUN(XX);
#undef XX
    }

#undef HOOK_FUN

    static uint64_t s_connect_timeout = -1;

    struct _HookIniter {
        _HookIniter() {
            hook_init();
            s_connect_timeout = g_tcp_connect_timeout->getValue();

            g_tcp_connect_timeout->addListener([](const int &old_value, const int &new_value) {
                JIVAN_LOG_INFO(g_logger) << "tcp connect timeout changed from "
                                         << old_value << " to " << new_value;
                s_connect_timeout = new_value;
            });
        }
    };

    static _HookIniter s_hook_initer;

    bool is_hook_enable() {
        return t_hook_enable;
    }

    void set_hook_enable(bool flag) {
        t_hook_enable = flag;
    }

}

struct timer_info {
    int cancelled = 0;
};

template<typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char *hook_fun_name,
                     uint32_t event, int timeout_so, Args &&... args) {
    if (!jivan::t_hook_enable) {
        return fun(fd, std::forward<Args>(args)...);
    }

    jivan::FdCtx::ptr ctx = jivan::FdMgr::GetInstance()->get(fd);
    if (!ctx) {
        return fun(fd, std::forward<Args>(args)...);
    }

    if (ctx->isClose()) {
        errno = EBADF;
        return -1;
    }

    if (!ctx->isSocket() || ctx->getUserNonblock()) {
        return fun(fd, std::forward<Args>(args)...);
    }

    uint64_t to = ctx->getTimeout(timeout_so);
    std::shared_ptr<timer_info> tinfo(new timer_info);

    retry:
    ssize_t n = fun(fd, std::forward<Args>(args)...);
    while (n == -1 && errno == EINTR) {
        n = fun(fd, std::forward<Args>(args)...);
    }
    if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        jivan::IOManager *iom = jivan::IOManager::GetThis();
        jivan::Timer::ptr timer;
        std::weak_ptr<timer_info> winfo(tinfo);

        if (to != (uint64_t) -1) {
            timer = iom->addConditionTimer(to, [winfo, fd, iom, event]() {
                auto t = winfo.lock();
                if (!t || t->cancelled) {
                    return;
                }
                t->cancelled = ETIMEDOUT;
                iom->cancelEvent(fd, (jivan::IOManager::Event) (event));
            }, winfo);
        }

        int rt = iom->addEvent(fd, (jivan::IOManager::Event) (event));
        if (JIVAN_UNLIKELY(rt)) {
            JIVAN_LOG_ERROR(g_logger) << hook_fun_name << " addEvent("
                                      << fd << ", " << event << ")";
            if (timer) {
                timer->cancel();
            }
            return -1;
        } else {
            jivan::Fiber::GetThis()->yield();
            if (timer) {
                timer->cancel();
            }
            if (tinfo->cancelled) {
                errno = tinfo->cancelled;
                return -1;
            }
            goto retry;
        }
    }

    return n;
}

unsigned int sleep(unsigned int seconds) {
    if (!jivan::t_hook_enable) {
        return sleep_f(seconds);
    }

    jivan::Fiber::ptr fiber = jivan::Fiber::GetThis();
    jivan::IOManager *iom = jivan::IOManager::GetThis();
    iom->addTimer(seconds * 1000,
                  std::bind((void (jivan::Scheduler::*)(jivan::Fiber::ptr, int)) &jivan::IOManager::schedule, iom,
                            fiber, -1));
    jivan::Fiber::GetThis()->yield();
    return 0;
}

int usleep(useconds_t usec) {
    if (!jivan::t_hook_enable) {
        return usleep_f(usec);
    }
    jivan::Fiber::ptr fiber = jivan::Fiber::GetThis();
    jivan::IOManager *iom = jivan::IOManager::GetThis();
    iom->addTimer(usec / 1000, std::bind((void (jivan::Scheduler::*)
            (jivan::Fiber::ptr, int thread)) &jivan::IOManager::schedule, iom, fiber, -1));
    jivan::Fiber::GetThis()->yield();
    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
    if (!jivan::t_hook_enable) {
        return nanosleep_f(req, rem);
    }

    int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
    jivan::Fiber::ptr fiber = jivan::Fiber::GetThis();
    jivan::IOManager *iom = jivan::IOManager::GetThis();
    iom->addTimer(timeout_ms, std::bind((void (jivan::Scheduler::*)
            (jivan::Fiber::ptr, int thread)) &jivan::IOManager::schedule, iom, fiber, -1));
    jivan::Fiber::GetThis()->yield();
    return 0;
}

int socket(int domain, int type, int protocol) {
    if (!jivan::t_hook_enable) {
        return socket_f(domain, type, protocol);
    }
    int fd = socket_f(domain, type, protocol);
    if (fd == -1) {
        return fd;
    }
    jivan::FdMgr::GetInstance()->get(fd, true);
    return fd;
}

int connect_with_timeout(int fd, const struct sockaddr *addr, socklen_t addrlen, uint64_t timeout_ms) {
    if (!jivan::t_hook_enable) {
        return connect_f(fd, addr, addrlen);
    }
    jivan::FdCtx::ptr ctx = jivan::FdMgr::GetInstance()->get(fd);
    if (!ctx || ctx->isClose()) {
        errno = EBADF;
        return -1;
    }

    //判断传入的fd是否为套接字，如果不为套接字，则调用系统的connect函数并返回。
    if (!ctx->isSocket()) {
        return connect_f(fd, addr, addrlen);
    }

    //判断fd是否被显式设置为了非阻塞模式，如果是则调用系统的connect函数并返回。
    if (ctx->getUserNonblock()) {
        return connect_f(fd, addr, addrlen);
    }

    //调用系统的connect函数，由于套接字是非阻塞的，这里会直接返回EINPROGRESS错误。
    int n = connect_f(fd, addr, addrlen);
    if (n == 0) {
        return 0;
    } else if (n != -1 || errno != EINPROGRESS) {
        return n;
    }

    //如果超时参数有效，则添加一个条件定时器，在定时时间到后通过t->cancelled设置超时标志并触发一次WRITE事件。
    jivan::IOManager *iom = jivan::IOManager::GetThis();
    jivan::Timer::ptr timer;
    std::shared_ptr<timer_info> tinfo(new timer_info);
    std::weak_ptr<timer_info> winfo(tinfo);

    if (timeout_ms != (uint64_t) -1) {
        timer = iom->addConditionTimer(timeout_ms, [winfo, fd, iom]() {
            auto t = winfo.lock();
            if (!t || t->cancelled) {
                return;
            }
            t->cancelled = ETIMEDOUT;
            iom->cancelEvent(fd, jivan::IOManager::WRITE);
        }, winfo);
    }

    //添加WRITE事件并yield，等待WRITE事件触发再往下执行。
    int rt = iom->addEvent(fd, jivan::IOManager::WRITE);
    if (JIVAN_UNLIKELY(rt)) {
        if (timer) {
            timer->cancel();
        }
        JIVAN_LOG_ERROR(g_logger) << "connect addEvent(" << fd << ", WRITE) error";
    } else {
        /**
         * 等待超时或套接字可写，如果先超时，则条件变量winfo仍然有效，通过winfo来设置超时标志并触发WRITE事件，协程从yield点返回，
         * 返回之后通过超时标志设置errno并返回-1；如果在未超时之前套接字就可写了，那么直接取消定时器并返回成功。
         */
        jivan::Fiber::GetThis()->yield();
        if (timer) {
            timer->cancel();
        }
        if (tinfo->cancelled) {
            errno = tinfo->cancelled;
            return -1;
        }
    }

    int error = 0;
    socklen_t len = sizeof(int);
    if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)) {
        return -1;
    }
    if (!error) {
        return 0;
    } else {
        errno = error;
        return -1;
    }
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return connect_with_timeout(sockfd, addr, addrlen, jivan::s_connect_timeout);
}

int accept(int s, struct sockaddr *addr, socklen_t *addrlen) {
    int fd = do_io(s, accept_f, "accept", jivan::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
    if (fd >= 0) {
        jivan::FdMgr::GetInstance()->get(fd, true);
    }
    return fd;
}

ssize_t read(int fd, void *buf, size_t count) {
    return do_io(fd, read_f, "read", jivan::IOManager::READ, SO_RCVTIMEO, buf, count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, readv_f, "readv", jivan::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    return do_io(sockfd, recv_f, "recv", jivan::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    return do_io(sockfd, recvfrom_f, "recvfrom", jivan::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr,
                 addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    return do_io(sockfd, recvmsg_f, "recvmsg", jivan::IOManager::READ, SO_RCVTIMEO, msg, flags);
}

ssize_t write(int fd, const void *buf, size_t count) {
    return do_io(fd, write_f, "write", jivan::IOManager::WRITE, SO_SNDTIMEO, buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, writev_f, "writev", jivan::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int s, const void *msg, size_t len, int flags) {
    return do_io(s, send_f, "send", jivan::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags);
}

ssize_t sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen) {
    return do_io(s, sendto_f, "sendto", jivan::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags, to, tolen);
}

ssize_t sendmsg(int s, const struct msghdr *msg, int flags) {
    return do_io(s, sendmsg_f, "sendmsg", jivan::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
}

int close(int fd) {
    if (!jivan::t_hook_enable) {
        return close_f(fd);
    }

    jivan::FdCtx::ptr ctx = jivan::FdMgr::GetInstance()->get(fd);
    if (ctx) {
        auto iom = jivan::IOManager::GetThis();
        if (iom) {
            iom->cancelAll(fd);
        }
        jivan::FdMgr::GetInstance()->del(fd);
    }
    return close_f(fd);
}

int fcntl(int fd, int cmd, ... /* arg */ ) {
    va_list va;
    va_start(va, cmd);
    switch (cmd) {
        case F_SETFL: {
            int arg = va_arg(va, int);
            va_end(va);
            jivan::FdCtx::ptr ctx = jivan::FdMgr::GetInstance()->get(fd);
            if (!ctx || ctx->isClose() || !ctx->isSocket()) {
                return fcntl_f(fd, cmd, arg);
            }
            ctx->setUserNonblock(arg & O_NONBLOCK);
            if (ctx->getSysNonblock()) {
                arg |= O_NONBLOCK;
            } else {
                arg &= ~O_NONBLOCK;
            }
            return fcntl_f(fd, cmd, arg);
        }
            break;
        case F_GETFL: {
            va_end(va);
            int arg = fcntl_f(fd, cmd);
            jivan::FdCtx::ptr ctx = jivan::FdMgr::GetInstance()->get(fd);
            if (!ctx || ctx->isClose() || !ctx->isSocket()) {
                return arg;
            }
            if (ctx->getUserNonblock()) {
                return arg | O_NONBLOCK;
            } else {
                return arg & ~O_NONBLOCK;
            }
        }
            break;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
#ifdef F_SETPIPE_SZ
        case F_SETPIPE_SZ:
#endif
        {
            int arg = va_arg(va, int);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
            break;
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
#ifdef F_GETPIPE_SZ
        case F_GETPIPE_SZ:
#endif
        {
            va_end(va);
            return fcntl_f(fd, cmd);
        }
            break;
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK: {
            struct flock *arg = va_arg(va, struct flock*);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
            break;
        case F_GETOWN_EX:
        case F_SETOWN_EX: {
            struct f_owner_exlock *arg = va_arg(va, struct f_owner_exlock*);
            va_end(va);
            return fcntl_f(fd, cmd, arg);
        }
            break;
        default:
            va_end(va);
            return fcntl_f(fd, cmd);
    }
}

int ioctl(int d, unsigned long int request, ...) {
    va_list va;
    va_start(va, request);
    void *arg = va_arg(va, void*);
    va_end(va);

    if (FIONBIO == request) {
        bool user_nonblock = !!*(int *) arg;
        jivan::FdCtx::ptr ctx = jivan::FdMgr::GetInstance()->get(d);
        if (!ctx || ctx->isClose() || !ctx->isSocket()) {
            return ioctl_f(d, request, arg);
        }
        ctx->setUserNonblock(user_nonblock);
    }
    return ioctl_f(d, request, arg);
}

int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    if (!jivan::t_hook_enable) {
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
    if (level == SOL_SOCKET) {
        if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
            jivan::FdCtx::ptr ctx = jivan::FdMgr::GetInstance()->get(sockfd);
            if (ctx) {
                const timeval *v = (const timeval *) optval;
                ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
            }
        }
    }
    return setsockopt_f(sockfd, level, optname, optval, optlen);
}

