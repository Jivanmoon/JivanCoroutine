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

    JIVAN_LOG_INFO(g_logger) << "before run_in_fiber yield";
    jivan::Fiber::GetThis()->yield();
    JIVAN_LOG_INFO(g_logger) << "after run_in_fiber yield";

    JIVAN_LOG_INFO(g_logger) << "run_in_fiber end";
    // fiber结束之后会自动返回主协程运行
}

void test_fiber() {
    JIVAN_LOG_INFO(g_logger) << "test_fiber begin";

    // 初始化线程主协程
    jivan::Fiber::GetThis();

    jivan::Fiber::ptr fiber(new jivan::Fiber(run_in_fiber, 0, false));
    JIVAN_LOG_INFO(g_logger) << "use_count:" << fiber.use_count(); // 1

    JIVAN_LOG_INFO(g_logger) << "before test_fiber resume";
    fiber->resume();
    JIVAN_LOG_INFO(g_logger) << "after test_fiber resume";

    /** 
     * 关于fiber智能指针的引用计数为3的说明：
     * 一份在当前函数的fiber指针，一份在MainFunc的cur指针
     * 还有一份在在run_in_fiber的GetThis()结果的临时变量里
     */
    JIVAN_LOG_INFO(g_logger) << "use_count:" << fiber.use_count(); // 3

    JIVAN_LOG_INFO(g_logger) << "fiber status: " << fiber->getState(); // READY

    JIVAN_LOG_INFO(g_logger) << "before test_fiber resume again";
    fiber->resume();
    JIVAN_LOG_INFO(g_logger) << "after test_fiber resume again";

    JIVAN_LOG_INFO(g_logger) << "use_count:" << fiber.use_count(); // 1
    JIVAN_LOG_INFO(g_logger) << "fiber status: " << fiber->getState(); // TERM

    fiber->reset(run_in_fiber2); // 上一个协程结束之后，复用其栈空间再创建一个新协程
    fiber->resume();

    JIVAN_LOG_INFO(g_logger) << "use_count:" << fiber.use_count(); // 1
    JIVAN_LOG_INFO(g_logger) << "test_fiber end";
}

int main(int argc, char *argv[]) {
    jivan::EnvMgr::GetInstance()->init(argc, argv);
    jivan::Config::LoadFromConfDir(jivan::EnvMgr::GetInstance()->getConfigPath());

    jivan::SetThreadName("main_thread");
    JIVAN_LOG_INFO(g_logger) << "main begin";

    std::vector<jivan::Thread::ptr> thrs;
    for (int i = 0; i < 1; i++) {
        thrs.push_back(jivan::Thread::ptr(
            new jivan::Thread(&test_fiber, "thread_" + std::to_string(i))));
    }

    for (auto i : thrs) {
        i->join();
    }

    JIVAN_LOG_INFO(g_logger) << "main end";
    return 0;
}