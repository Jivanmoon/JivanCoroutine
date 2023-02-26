#include "../include/all.h"

#include <unistd.h>

jivan::Logger::ptr g_logger = JIVAN_LOG_ROOT(); // 默认INFO级别

int main(int argc, char *argv[]) {
    jivan::EnvMgr::GetInstance()->init(argc, argv);
    jivan::Config::LoadFromConfDir(jivan::EnvMgr::GetInstance()->getConfigPath());

    JIVAN_LOG_FATAL(g_logger) << "fatal msg";
    JIVAN_LOG_ERROR(g_logger) << "err msg";
    JIVAN_LOG_INFO(g_logger) << "info msg";
    JIVAN_LOG_DEBUG(g_logger) << "debug msg";

    JIVAN_LOG_FMT_FATAL(g_logger, "fatal %s:%d", __FILE__, __LINE__);
    JIVAN_LOG_FMT_ERROR(g_logger, "err %s:%d", __FILE__, __LINE__);
    JIVAN_LOG_FMT_INFO(g_logger, "info %s:%d", __FILE__, __LINE__);
    JIVAN_LOG_FMT_DEBUG(g_logger, "debug %s:%d", __FILE__, __LINE__);

    sleep(1);
    jivan::SetThreadName("brand_new_thread");

    g_logger->setLevel(jivan::LogLevel::WARN);
    JIVAN_LOG_FATAL(g_logger) << "fatal msg";
    JIVAN_LOG_ERROR(g_logger) << "err msg";
    JIVAN_LOG_INFO(g_logger) << "info msg"; // 不打印
    JIVAN_LOG_DEBUG(g_logger) << "debug msg"; // 不打印


    jivan::FileLogAppender::ptr fileAppender(new jivan::FileLogAppender("./log.txt"));
    g_logger->addAppender(fileAppender);
    JIVAN_LOG_FATAL(g_logger) << "fatal msg";
    JIVAN_LOG_ERROR(g_logger) << "err msg";
    JIVAN_LOG_INFO(g_logger) << "info msg"; // 不打印
    JIVAN_LOG_DEBUG(g_logger) << "debug msg"; // 不打印

    jivan::Logger::ptr test_logger = JIVAN_LOG_NAME("test_logger");
    jivan::StdoutLogAppender::ptr appender(new jivan::StdoutLogAppender);
    jivan::LogFormatter::ptr formatter(new jivan::LogFormatter("%d:%rms%T%p%T%c%T%f:%l %m%n")); // 时间：启动毫秒数 级别 日志名称 文件名：行号 消息 换行
    appender->setFormatter(formatter);
    test_logger->addAppender(appender);
    test_logger->setLevel(jivan::LogLevel::WARN);

    JIVAN_LOG_ERROR(test_logger) << "err msg";
    JIVAN_LOG_INFO(test_logger) << "info msg"; // 不打印

    // 输出全部日志器的配置
    g_logger->setLevel(jivan::LogLevel::INFO);
    JIVAN_LOG_INFO(g_logger) << "logger config:\n" << jivan::LoggerMgr::GetInstance()->toYamlString();

    return 0;
}