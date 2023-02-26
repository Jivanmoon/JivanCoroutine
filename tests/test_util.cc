#include "../include/all.h"

jivan::Logger::ptr g_logger = JIVAN_LOG_ROOT();

void test2() {
    std::cout << jivan::BacktraceToString() << std::endl;
}
void test1() {
    test2();
}

void test_backtrace() {
    test1();
}

int main() {
    JIVAN_LOG_INFO(g_logger) << jivan::GetCurrentMS();
    JIVAN_LOG_INFO(g_logger) << jivan::GetCurrentUS();
    JIVAN_LOG_INFO(g_logger) << jivan::ToUpper("hello");
    JIVAN_LOG_INFO(g_logger) << jivan::ToLower("HELLO");
    JIVAN_LOG_INFO(g_logger) << jivan::Time2Str();
    JIVAN_LOG_INFO(g_logger) << jivan::Str2Time("1970-01-01 00:00:00"); // -28800

    std::vector<std::string> files;
    jivan::FSUtil::ListAllFile(files, "./code", ".cpp");
    for (auto &i : files) {
        JIVAN_LOG_INFO(g_logger) << i;
    }

    // todo, more...

    test_backtrace();

    JIVAN_ASSERT2(false, "assert");
    return 0;
}