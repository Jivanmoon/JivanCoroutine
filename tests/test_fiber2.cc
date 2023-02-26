#include "../include/all.h"
#include <string>
#include <vector>

jivan::Logger::ptr g_logger = JIVAN_LOG_ROOT();

void run_in_fiber2() {
    JIVAN_LOG_INFO(g_logger) << "run_in_fiber2 begin";
    JIVAN_LOG_INFO(g_logger) << "run_in_fiber2 end";
}

void run_in_fiber() {
    JIVAN_LOG_INFO(g_logger) << "run_in_fiber begin";

    /**
     * 非对称协程，子协程不能创建并运行新的子协程，下面的操作是有问题的，
     * 子协程再创建子协程，原来的主协程就跑飞了
     */
    jivan::Fiber::ptr fiber(new jivan::Fiber(run_in_fiber2, 0, false));
    fiber->resume();

    JIVAN_LOG_INFO(g_logger) << "run_in_fiber end";
}

int main(int argc, char *argv[]) {
    jivan::EnvMgr::GetInstance()->init(argc, argv);
    jivan::Config::LoadFromConfDir(jivan::EnvMgr::GetInstance()->getConfigPath());

    JIVAN_LOG_INFO(g_logger) << "main begin";

    jivan::Fiber::GetThis();

    jivan::Fiber::ptr fiber(new jivan::Fiber(run_in_fiber, 0, false));
    fiber->resume();

    JIVAN_LOG_INFO(g_logger) << "main end";
    return 0;
}