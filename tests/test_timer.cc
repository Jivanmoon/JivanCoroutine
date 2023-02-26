#include "../include/all.h"

static jivan::Logger::ptr g_logger = JIVAN_LOG_ROOT();

static int timeout = 1000;
static jivan::Timer::ptr s_timer;

void timer_callback() {
    JIVAN_LOG_INFO(g_logger) << "timer callback, timeout = " << timeout;
    timeout += 1000;
    if(timeout <= 5000) {
//        s_timer->reset(timeout, true);
    } else {
        s_timer->cancel();
    }
}

void test_timer(std::weak_ptr<int> &winfo) {
    jivan::IOManager iom;

    // 循环定时器
    s_timer = iom.addTimer(1000, timer_callback, true);

    //条件定时器
    iom.addConditionTimer(6000, [](){
        JIVAN_LOG_INFO(g_logger) << "6000ms timeout";
    }, winfo);

    // 单次定时器
    iom.addTimer(500, []{
        JIVAN_LOG_INFO(g_logger) << "500ms timeout";
    });
    iom.addTimer(5000, []{
        JIVAN_LOG_INFO(g_logger) << "5000ms timeout";
    });
}

int main(int argc, char *argv[]) {
    jivan::EnvMgr::GetInstance()->init(argc, argv);
    jivan::Config::LoadFromConfDir(jivan::EnvMgr::GetInstance()->getConfigPath());

    std::shared_ptr<int> tinfo(new int(0));
    std::weak_ptr<int> winfo(tinfo);
    test_timer(winfo);

    JIVAN_LOG_INFO(g_logger) << "end";

    return 0;
}