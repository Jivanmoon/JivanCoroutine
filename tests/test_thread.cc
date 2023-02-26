#include "../include/all.h"

jivan::Logger::ptr g_logger = JIVAN_LOG_ROOT();

int count = 0;
jivan::Mutex s_mutex;

void func1(void *arg) {
    JIVAN_LOG_INFO(g_logger) << "name:" << jivan::Thread::GetName()
        << ", this.name:" << jivan::Thread::GetThis()->getName()
        << ", thread name:" << jivan::GetThreadName()
        << ", id:" << jivan::GetThreadId()
        << ", this.id:" << jivan::Thread::GetThis()->getId()
        << "\n" << "arg: " << *(int*)arg;
    for(int i = 0; i < 10000; i++) {
        jivan::Mutex::Lock lock(s_mutex);
        ++count;
    }
}

int main(int argc, char *argv[]) {
    jivan::EnvMgr::GetInstance()->init(argc, argv);
    jivan::Config::LoadFromConfDir(jivan::EnvMgr::GetInstance()->getConfigPath());

    std::vector<jivan::Thread::ptr> thrs;
    int arg = 123456;
    for(int i = 0; i < 3; i++) {
        // 带参数的线程用std::bind进行参数绑定
        jivan::Thread::ptr thr(new jivan::Thread(std::bind(func1, &arg), "thread_" + std::to_string(i)));
        thrs.push_back(thr);
    }

    for(int i = 0; i < 3; i++) {
        thrs[i]->join();
    }
    
    JIVAN_LOG_INFO(g_logger) << "count = " << count;
    return 0;
}

