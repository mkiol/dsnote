#ifndef SINGLETON_H
#define SINGLETON_H

#include <utility>

template <typename T>
class singleton {
   protected:
    singleton() = default;
    singleton(const singleton&) = delete;
    singleton& operator=(const singleton&) = delete;
    singleton& operator=(singleton&&) = delete;
    virtual ~singleton() = default;

   public:
    template <typename... Args>
    static T* instance(Args... args) {
        static T inst{std::forward<Args>(args)...};
        return &inst;
    }
};

#endif // SINGLETON_H
