#include <memory>
#include <utility> // for std::pair
#include "../include/log.h"
#include "../include/config.h"
#include "../include/env.h"

namespace jivan {

    const char *LogLevel::ToString(LogLevel::Level level) {
        switch (level) {
#define XX(name) case LogLevel::name: return #name;
            XX(FATAL);
            XX(ALERT);
            XX(CRIT);
            XX(ERROR);
            XX(WARN);
            XX(NOTICE);
            XX(INFO);
            XX(DEBUG);
#undef XX
            default:
                return "NOTSET";
        }
        return "NOTSET";
    }

    LogLevel::Level LogLevel::FromString(const std::string &str) {
#define XX(level, v) if(str == #v) { return LogLevel::level; }
        XX(FATAL, fatal);
        XX(ALERT, alert);
        XX(CRIT, crit);
        XX(ERROR, error);
        XX(WARN, warn);
        XX(NOTICE, notice);
        XX(INFO, info);
        XX(DEBUG, debug);

        XX(FATAL, FATAL);
        XX(ALERT, ALERT);
        XX(CRIT, CRIT);
        XX(ERROR, ERROR);
        XX(WARN, WARN);
        XX(NOTICE, NOTICE);
        XX(INFO, INFO);
        XX(DEBUG, DEBUG);
#undef XX

        return LogLevel::NOTSET;
    }

    LogEvent::LogEvent(const std::string &logger_name, LogLevel::Level level, const char *file, int32_t line,
                       int64_t elapse, uint32_t thread_id, uint64_t fiber_id, time_t time,
                       const std::string &thread_name)
            : m_level(level), m_file(file), m_line(line), m_elapse(elapse), m_threadId(thread_id), m_fiberId(fiber_id),
              m_time(time), m_threadName(thread_name), m_loggerName(logger_name) {
    }

    void LogEvent::printf(const char *fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        vprintf(fmt, ap);
        va_end(ap);
    }

    void LogEvent::vprintf(const char *fmt, va_list ap) {
        char *buf = nullptr;
        int len = vasprintf(&buf, fmt, ap);
        if (len != -1) {
            m_ss << std::string(buf, len);
            free(buf);
        }
    }

    class MessageFormatItem : public LogFormatter::FormatItem {
    public:
        MessageFormatItem(const std::string &str) {}

        void format(std::ostream &os, LogEvent::ptr event) override {
            os << event->getContent();
        }
    };

    class LevelFormatItem : public LogFormatter::FormatItem {
    public:
        LevelFormatItem(const std::string &str) {}

        void format(std::ostream &os, LogEvent::ptr event) override {
            os << LogLevel::ToString(event->getLevel());
        }
    };

    class ElapseFormatItem : public LogFormatter::FormatItem {
    public:
        ElapseFormatItem(const std::string &str) {}

        void format(std::ostream &os, LogEvent::ptr event) override {
            os << event->getElapse();
        }
    };

    class LoggerNameFormatItem : public LogFormatter::FormatItem {
    public:
        LoggerNameFormatItem(const std::string &str) {}

        void format(std::ostream &os, LogEvent::ptr event) override {
            os << event->getLoggerName();
        }
    };

    class ThreadIdFormatItem : public LogFormatter::FormatItem {
    public:
        ThreadIdFormatItem(const std::string &str) {}

        void format(std::ostream &os, LogEvent::ptr event) override {
            os << event->getThreadId();
        }
    };

    class FiberIdFormatItem : public LogFormatter::FormatItem {
    public:
        FiberIdFormatItem(const std::string &str) {}

        void format(std::ostream &os, LogEvent::ptr event) override {
            os << event->getFiberId();
        }
    };

    class ThreadNameFormatItem : public LogFormatter::FormatItem {
    public:
        ThreadNameFormatItem(const std::string &str) {}

        void format(std::ostream &os, LogEvent::ptr event) override {
            os << event->getThreadName();
        }
    };

    class DateTimeFormatItem : public LogFormatter::FormatItem {
    public:
        DateTimeFormatItem(const std::string &format = "%Y-%m-%d %H:%M:%S")
                : m_format(format) {
            if (m_format.empty()) {
                m_format = "%Y-%m-%d %H:%M:%S";
            }
        }

        void format(std::ostream &os, LogEvent::ptr event) override {
            struct tm tm;
            time_t time = event->getTime();
            localtime_r(&time, &tm);
            char buf[64];
            strftime(buf, sizeof(buf), m_format.c_str(), &tm);
            os << buf;
        }

    private:
        std::string m_format;
    };

    class FileNameFormatItem : public LogFormatter::FormatItem {
    public:
        FileNameFormatItem(const std::string &str) {}

        void format(std::ostream &os, LogEvent::ptr event) override {
            os << event->getFile();
        }
    };

    class LineFormatItem : public LogFormatter::FormatItem {
    public:
        LineFormatItem(const std::string &str) {}

        void format(std::ostream &os, LogEvent::ptr event) override {
            os << event->getLine();
        }
    };

    class NewLineFormatItem : public LogFormatter::FormatItem {
    public:
        NewLineFormatItem(const std::string &str) {}

        void format(std::ostream &os, LogEvent::ptr event) override {
            os << std::endl;
        }
    };

    class StringFormatItem : public LogFormatter::FormatItem {
    public:
        StringFormatItem(const std::string &str)
                : m_string(str) {}

        void format(std::ostream &os, LogEvent::ptr event) override {
            os << m_string;
        }

    private:
        std::string m_string;
    };

    class TabFormatItem : public LogFormatter::FormatItem {
    public:
        TabFormatItem(const std::string &str) {}

        void format(std::ostream &os, LogEvent::ptr event) override {
            os << "\t";
        }
    };

    class PercentSignFormatItem : public LogFormatter::FormatItem {
    public:
        PercentSignFormatItem(const std::string &str) {}

        void format(std::ostream &os, LogEvent::ptr event) override {
            os << "%";
        }
    };

    LogFormatter::LogFormatter(const std::string &pattern)
            : m_pattern(pattern) {
        init();
    }

/**
 * ?????????????????????????????????pattern?????????????????????????????????
 *
 * ???????????????????????????????????????????????????????????????????????????????????????????????????????????????
 *
 * ????????????????????????????????????????????????????????????????????????????????????
 *
 * ??????????????????%%d????????????????????????????????????????????????????????????%%d{%%Y-%%m-%%d %%H:%%M:%%S}?????????????????????????????????
 *
 * ????????????????????????????????????????????????????????????????????????pattern????????????????????????
 *
 * @see LogFormatter::LogFormatter
 */
    void LogFormatter::init() {
        // ???????????????????????????pattern???
        // ??????pattern??????????????????????????????????????????????????????0?????????pattern????????????????????????1?????????pattern????????????
        // ??????????????????????????????dataformat??????
        std::vector<std::pair<int, std::string>> patterns;
        // ???????????????????????????
        std::string tmp;
        // ???????????????????????????????????????%d??????????????????????????????????????????????????????????????????????????????????????????
        std::string dateformat;
        // ??????????????????
        bool error = false;

        // ?????????????????????????????????????????????true
        bool parsing_string = true;
        // ?????????????????????????????????%????????????????????????
        // bool parsing_pattern = false;

        size_t i = 0;
        while (i < m_pattern.size()) {
            std::string c = std::string(1, m_pattern[i]);
            if (c == "%") {
                if (parsing_string) {
                    if (!tmp.empty()) {
                        patterns.push_back(std::make_pair(0, tmp));
                    }
                    tmp.clear();
                    parsing_string = false; // ??????????????????????????????%?????????????????????????????????
                    // parsing_pattern = true;
                    i++;
                    continue;
                } else {
                    patterns.push_back(std::make_pair(1, c));
                    parsing_string = true; // ??????????????????????????????%????????????????????????%??????
                    // parsing_pattern = false;
                    i++;
                    continue;
                }
            } else { // not %
                if (parsing_string) { // ????????????????????????????????????%?????????????????????????????????????????????????????????patterns
                    tmp += c;
                    i++;
                    continue;
                } else { // ??????????????????????????????patterns?????????????????????????????????????????????????????????%d????????????
                    patterns.push_back(std::make_pair(1, c));
                    parsing_string = true;
                    // parsing_pattern = false;

                    // ????????????%d????????????????????????%d???????????????????????????????????????????????????????????????????????????????????????dateformat
                    if (c != "d") {
                        i++;
                        continue;
                    }
                    i++;
                    if (i < m_pattern.size() && m_pattern[i] != '{') {
                        continue;
                    }
                    i++;
                    while (i < m_pattern.size() && m_pattern[i] != '}') {
                        dateformat.push_back(m_pattern[i]);
                        i++;
                    }
                    if (m_pattern[i] != '}') {
                        // %d?????????????????????????????????????????????
                        std::cout << "[ERROR] LogFormatter::init() " << "pattern: [" << m_pattern << "] '{' not closed"
                                  << std::endl;
                        error = true;
                        break;
                    }
                    i++;
                    continue;
                }
            }
        } // end while(i < m_pattern.size())

        if (error) {
            m_error = true;
            return;
        }

        // ????????????????????????????????????????????????????????????
        if (!tmp.empty()) {
            patterns.push_back(std::make_pair(0, tmp));
            tmp.clear();
        }

        // for debug
        // std::cout << "patterns:" << std::endl;
        // for(auto &v : patterns) {
        //     std::cout << "type = " << v.first << ", value = " << v.second << std::endl;
        // }
        // std::cout << "dataformat = " << dateformat << std::endl;

        static std::map<std::string, std::function<FormatItem::ptr(const std::string &str)> > s_format_items = {
#define XX(str, C)  {#str, [](const std::string& fmt) { return FormatItem::ptr(new C(fmt));} }

                XX(m, MessageFormatItem),           // m:??????
                XX(p, LevelFormatItem),             // p:????????????
                XX(c, LoggerNameFormatItem),        // c:???????????????
//        XX(d, DateTimeFormatItem),          // d:????????????
                XX(r, ElapseFormatItem),            // r:???????????????
                XX(f, FileNameFormatItem),          // f:?????????
                XX(l, LineFormatItem),              // l:??????
                XX(t, ThreadIdFormatItem),          // t:?????????
                XX(F, FiberIdFormatItem),           // F:?????????
                XX(N, ThreadNameFormatItem),        // N:????????????
                XX(%, PercentSignFormatItem),       // %:?????????
                XX(T, TabFormatItem),               // T:?????????
                XX(n, NewLineFormatItem),           // n:?????????
#undef XX
        };

        for (auto &v: patterns) {
            if (v.first == 0) {
                m_items.push_back(FormatItem::ptr(new StringFormatItem(v.second)));
            } else if (v.second == "d") {
                m_items.push_back(FormatItem::ptr(new DateTimeFormatItem(dateformat)));
            } else {
                auto it = s_format_items.find(v.second);
                if (it == s_format_items.end()) {
                    std::cout << "[ERROR] LogFormatter::init() " << "pattern: [" << m_pattern << "] " <<
                              "unknown format item: " << v.second << std::endl;
                    error = true;
                    break;
                } else {
                    m_items.push_back(it->second(v.second));
                }
            }
        }

        if (error) {
            m_error = true;
            return;
        }
    }

    std::string LogFormatter::format(LogEvent::ptr event) {
        std::stringstream ss;
        for (auto &i: m_items) {
            i->format(ss, event);
        }
        return ss.str();
    }

    std::ostream &LogFormatter::format(std::ostream &os, LogEvent::ptr event) {
        for (auto &i: m_items) {
            i->format(os, event);
        }
        return os;
    }

    LogAppender::LogAppender(LogFormatter::ptr default_formatter)
            : m_defaultFormatter(default_formatter) {
    }

    void LogAppender::setFormatter(LogFormatter::ptr val) {
        MutexType::Lock lock(m_mutex);
        m_formatter = val;
    }

    LogFormatter::ptr LogAppender::getFormatter() {
        MutexType::Lock lock(m_mutex);
        return m_formatter ? m_formatter : m_defaultFormatter;
    }

    StdoutLogAppender::StdoutLogAppender()
            : LogAppender(std::make_shared<LogFormatter>()) {
    }

    void StdoutLogAppender::log(LogEvent::ptr event) {
        MutexType::Lock lock(m_mutex);
        if (m_formatter) {
            m_formatter->format(std::cout, event);
        } else {
            m_defaultFormatter->format(std::cout, event);
        }
    }

    std::string StdoutLogAppender::toYamlString() {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        node["type"] = "StdoutLogAppender";
        node["pattern"] = m_formatter ? m_formatter->getPattern() : m_defaultFormatter->getPattern();
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    FileLogAppender::FileLogAppender(const std::string &file)
            : LogAppender(LogFormatter::ptr(new LogFormatter)) {
        m_filename = file;
        reopen();
        if (m_reopenError) {
            std::cout << "reopen file " << m_filename << " error" << std::endl;
        }
    }

/**
 * ???????????????????????????????????????????????????3??????????????????????????????????????????
 */
    void FileLogAppender::log(LogEvent::ptr event) {
        uint64_t now = event->getTime();
        if (now >= (m_lastTime + 3)) {
            reopen();
            if (m_reopenError) {
                std::cout << "reopen file " << m_filename << " error" << std::endl;
            }
            m_lastTime = now;
        }
        if (m_reopenError) {
            return;
        }
        MutexType::Lock lock(m_mutex);
        if (m_formatter) {
            if (!m_formatter->format(m_filestream, event)) {
                std::cout << "[ERROR] FileLogAppender::log() format error" << std::endl;
            }
        } else {
            if (!m_defaultFormatter->format(m_filestream, event)) {
                std::cout << "[ERROR] FileLogAppender::log() format error" << std::endl;
            }
        }

    }

    bool FileLogAppender::reopen() {
        MutexType::Lock lock(m_mutex);
        if (m_filestream) {
            m_filestream.close();
        }
        m_filestream.open(m_filename, std::ios::app);
        m_reopenError = !m_filestream;
        return !m_reopenError;
    }

    std::string FileLogAppender::toYamlString() {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        node["type"] = "FileLogAppender";
        node["file"] = m_filename;
        node["pattern"] = m_formatter ? m_formatter->getPattern() : m_defaultFormatter->getPattern();
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    Logger::Logger(const std::string &name)
            : m_name(name), m_level(LogLevel::INFO), m_createTime(GetElapsedMS()) {
    }

    void Logger::addAppender(LogAppender::ptr appender) {
        MutexType::Lock lock(m_mutex);
        m_appenders.push_back(appender);
    }

    void Logger::delAppender(LogAppender::ptr appender) {
        MutexType::Lock lock(m_mutex);
        for (auto it = m_appenders.begin(); it != m_appenders.end(); it++) {
            if (*it == appender) {
                m_appenders.erase(it);
                break;
            }
        }
    }

    void Logger::clearAppenders() {
        MutexType::Lock lock(m_mutex);
        m_appenders.clear();
    }

/**
 * ??????Logger?????????appenders?????????????????????
 * Logger??????????????????appender?????????????????????
 */
    void Logger::log(LogEvent::ptr event) {
        if (event->getLevel() <= m_level) {
            for (auto &i: m_appenders) {
                i->log(event);
            }
        }
    }

    std::string Logger::toYamlString() {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        node["name"] = m_name;
        node["level"] = LogLevel::ToString(m_level);
        for (auto &i: m_appenders) {
            node["appenders"].push_back(YAML::Load(i->toYamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    LogEventWrap::LogEventWrap(Logger::ptr logger, LogEvent::ptr event)
            : m_logger(logger), m_event(event) {
    }

/**
 * @note LogEventWrap?????????????????????
 */
    LogEventWrap::~LogEventWrap() {
        m_logger->log(m_event);
    }

    LoggerManager::LoggerManager() {
        m_root.reset(new Logger("root"));
        m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
        m_loggers[m_root->getName()] = m_root;
        init();
    }

/**
 * ???????????????????????????????????????????????????????????????????????????????????????Logger?????????Appender??????
 * ??????????????????Appender
 */
    Logger::ptr LoggerManager::getLogger(const std::string &name) {
        MutexType::Lock lock(m_mutex);
        auto it = m_loggers.find(name);
        if (it != m_loggers.end()) {
            return it->second;
        }

        Logger::ptr logger(new Logger(name));
        m_loggers[name] = logger;
        return logger;
    }

/**
 * @todo ???????????????????????????????????????
 */
    void LoggerManager::init() {
    }

    std::string LoggerManager::toYamlString() {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        for (auto &i: m_loggers) {
            node.push_back(YAML::Load(i.second->toYamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

///////////////////////////////////////////////////////////////////////////////
// ????????????????????????????????????
/**
 * @brief ????????????????????????????????????
 */
    struct LogAppenderDefine {
        int type = 0; // 1 File, 2 Stdout
        std::string pattern;
        std::string file;

        bool operator==(const LogAppenderDefine &oth) const {
            return type == oth.type && pattern == oth.pattern && file == oth.file;
        }
    };

/**
 * @brief ??????????????????????????????
 */
    struct LogDefine {
        std::string name;
        LogLevel::Level level = LogLevel::NOTSET;
        std::vector<LogAppenderDefine> appenders;

        bool operator==(const LogDefine &oth) const {
            return name == oth.name && level == oth.level && appenders == appenders;
        }

        bool operator<(const LogDefine &oth) const {
            return name < oth.name;
        }

        bool isValid() const {
            return !name.empty();
        }
    };

    template<>
    class LexicalCast<std::string, LogDefine> {
    public:
        LogDefine operator()(const std::string &v) {
            YAML::Node n = YAML::Load(v);
            LogDefine ld;
            if (!n["name"].IsDefined()) {
                std::cout << "log config error: name is null, " << n << std::endl;
                throw std::logic_error("log config name is null");
            }
            ld.name = n["name"].as<std::string>();
            ld.level = LogLevel::FromString(n["level"].IsDefined() ? n["level"].as<std::string>() : "");

            if (n["appenders"].IsDefined()) {
                for (size_t i = 0; i < n["appenders"].size(); i++) {
                    auto a = n["appenders"][i];
                    if (!a["type"].IsDefined()) {
                        std::cout << "log appender config error: appender type is null, " << a << std::endl;
                        continue;
                    }
                    std::string type = a["type"].as<std::string>();
                    LogAppenderDefine lad;
                    if (type == "FileLogAppender") {
                        lad.type = 1;
                        if (!a["file"].IsDefined()) {
                            std::cout << "log appender config error: file appender file is null, " << a << std::endl;
                            continue;
                        }
                        lad.file = a["file"].as<std::string>();
                        if (a["pattern"].IsDefined()) {
                            lad.pattern = a["pattern"].as<std::string>();
                        }
                    } else if (type == "StdoutLogAppender") {
                        lad.type = 2;
                        if (a["pattern"].IsDefined()) {
                            lad.pattern = a["pattern"].as<std::string>();
                        }
                    } else {
                        std::cout << "log appender config error: appender type is invalid, " << a << std::endl;
                        continue;
                    }
                    ld.appenders.push_back(lad);
                }
            } // end for
            return ld;
        }
    };

    template<>
    class LexicalCast<LogDefine, std::string> {
    public:
        std::string operator()(const LogDefine &i) {
            YAML::Node n;
            n["name"] = i.name;
            n["level"] = LogLevel::ToString(i.level);
            for (auto &a: i.appenders) {
                YAML::Node na;
                if (a.type == 1) {
                    na["type"] = "FileLogAppender";
                    na["file"] = a.file;
                } else if (a.type == 2) {
                    na["type"] = "StdoutLogAppender";
                }
                if (!a.pattern.empty()) {
                    na["pattern"] = a.pattern;
                }
                n["appenders"].push_back(na);
            }
            std::stringstream ss;
            ss << n;
            return ss.str();
        }
    };

    jivan::ConfigVar<std::set<LogDefine>>::ptr g_log_defines =
            jivan::Config::Lookup("logs", std::set<LogDefine>(), "logs config");

    struct LogIniter {
        LogIniter() {
            g_log_defines->addListener([](const std::set<LogDefine> &old_value, const std::set<LogDefine> &new_value) {
                JIVAN_LOG_INFO(JIVAN_LOG_ROOT()) << "on log config changed";
                for (auto &i: new_value) {
                    auto it = old_value.find(i);
                    jivan::Logger::ptr logger;
                    if (it == old_value.end()) {
                        // ??????logger
                        logger = JIVAN_LOG_NAME(i.name);
                    } else {
                        if (!(i == *it)) {
                            // ?????????logger
                            logger = JIVAN_LOG_NAME(i.name);
                        } else {
                            continue;
                        }
                    }
                    logger->setLevel(i.level);
                    logger->clearAppenders();
                    for (auto &a: i.appenders) {
                        jivan::LogAppender::ptr ap;
                        if (a.type == 1) {
                            ap.reset(new FileLogAppender(a.file));
                        } else if (a.type == 2) {
                            // ?????????daemon???????????????????????????????????????appender
                            if (!jivan::EnvMgr::GetInstance()->has("d")) {
                                ap.reset(new StdoutLogAppender);
                            } else {
                                continue;
                            }
                        }
                        if (!a.pattern.empty()) {
                            ap->setFormatter(LogFormatter::ptr(new LogFormatter(a.pattern)));
                        } else {
                            ap->setFormatter(LogFormatter::ptr(new LogFormatter));
                        }
                        logger->addAppender(ap);
                    }
                }

                // ???????????????????????????????????????????????????????????????????????????logger??????????????????????????????logger???????????????
                for (auto &i: old_value) {
                    auto it = new_value.find(i);
                    if (it == new_value.end()) {
                        auto logger = JIVAN_LOG_NAME(i.name);
                        logger->setLevel(LogLevel::NOTSET);
                        logger->clearAppenders();
                    }
                }
            });
        }
    };

//???main?????????????????????????????????????????????
//???????????????????????????log????????????????????????Config
    static LogIniter __log_init;

} // end namespace code