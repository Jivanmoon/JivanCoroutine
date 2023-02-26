#ifndef __JIVAN_SINGLETON_H__
#define __JIVAN_SINGLETON_H__

#include <memory>

namespace jivan {


/**
 * @brief 单例模式封装类
 * @details T 类型
 */
    template<class T>
    class Singleton {
    public:
        /**
         * @brief 返回单例裸指针
         */
        static T *GetInstance() {
            static T v;
            return &v;
        }
    };

/**
 * @brief 单例模式智能指针封装类
 * @details T 类型
 */
    template<class T>
    class SingletonPtr {
    public:
        /**
         * @brief 返回单例智能指针
         */
        static std::shared_ptr<T> GetInstance() {
            static std::shared_ptr<T> v(new T);
            return v;
        }
    };

}

#endif
