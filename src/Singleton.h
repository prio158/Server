//
// Created by 陈子锐 on 2024/2/4.
//

#ifndef SERVER_SINGLETON_H
#define SERVER_SINGLETON_H

#include <memory>

namespace Server {

    template<class T, class X = void, int N = 0>
    class Singleton {
    public:
        static T *GetInstance() {
            static T v;
            return &v;
        }

    };

    template<class T, class X = void, int N = 0>
    class SingletonPtr {
    public:
        static std::shared_ptr<T> GetInstance() {
            static std::shared_ptr<T> v(new T);
            return v;
        }
    };
}

#endif //SERVER_SINGLETON_H
