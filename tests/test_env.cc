#include "../include/all.h"

jivan::Logger::ptr g_logger = JIVAN_LOG_ROOT();

jivan::Env *g_env = jivan::EnvMgr::GetInstance();

int main(int argc, char *argv[]) {
    g_env->addHelp("h", "print this help message");

    bool is_print_help = false;
    if(!g_env->init(argc, argv)) {
        is_print_help = true;
    }
    if(g_env->has("h")) {
        is_print_help = true;
    }

    if(is_print_help) {
        g_env->printHelp();
        return false;
    }

    JIVAN_LOG_INFO(g_logger)<< "exe: " << g_env->getExe();
    JIVAN_LOG_INFO(g_logger) <<"cwd: " << g_env->getCwd();
    JIVAN_LOG_INFO(g_logger) << "absoluth path of tests: " << g_env->getAbsolutePath("tests");
    JIVAN_LOG_INFO(g_logger) << "conf path:" << g_env->getConfigPath();

    g_env->add("key1", "value1");
    JIVAN_LOG_INFO(g_logger) << "key1: " << g_env->get("key1");

    g_env->setEnv("key2", "value2");
    JIVAN_LOG_INFO(g_logger) << "key2: " << g_env->getEnv("key2");

    JIVAN_LOG_INFO(g_logger) << g_env->getEnv("PATH");

    return 0;
}