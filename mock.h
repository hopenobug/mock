
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include "frida/frida-gum.h"

// frida-gum 下载地址 https://github.com/frida/frida/releases/tag/16.1.3
// MOCK_VIRTUAL 在 linux和mac上测试正常， 在windows上无法工作， 原因在于windows上无法通过函数指针获取正确的虚函数表偏移， 需要自己手动指定偏移

#define _COMBINE_(X, Y) X##Y  // helper macro
#define COMBINE(X, Y) _COMBINE_(X,Y)
#define MOCK(target, alter) auto && COMBINE(mock, __LINE__) = mock::Mock(target, alter)
#define MOCK_RETURN(target, alter) auto && COMBINE(mock, __LINE__) = mock::MockReturn(target, alter)
#define MOCK_VIRTUAL(target, obj, alter) auto && COMBINE(mock, __LINE__) = mock::MockVirtual(target, obj, alter)


namespace mock {

template<typename Dst, typename Src>
Dst toAnyType(Src src) {
    union {
        Dst dst;
        Src src;
    } Func{};
    Func.src = src;
    return Func.dst;
}

template<typename Dst>
Dst toAnyType(Dst src) {
    return src;
}

template<typename T>
uint64_t toUint64(T src) {
    return toAnyType<uint64_t>(src);
}

template<typename T>
void * toVoidPtr(T src) {
    return toAnyType<void *>(src);
}

class Checker {
public:
    virtual void Check() = 0;
};

template<typename T>
T getVirtualFunctionByIndex(const void *obj, uint64_t index) {
    void **vt = *(void ***)obj;
    return reinterpret_cast<T>(vt[index]);
}

// 在windows上无法正常获取虚函数
template<typename T>
void *getVirtualFunction(T func, const void *obj) {
    static const uint64_t funcOffset = toUint64(&Checker::Check);
    uint64_t index = (toUint64(func) - funcOffset) / sizeof(void *);
    return getVirtualFunctionByIndex<void *>(obj, index);
}

/**
 * 一个对象管理一个mock，析构时自动回滚
 * @tparam RET
 * @tparam ARGS
 */
template<typename RET, typename ...ARGS>
class MockHandler : public std::enable_shared_from_this<MockHandler<RET, ARGS...>> {
public:
 
 
    /**
     * 这个函数作为回调函数传给frida
     * @param args
     * @return
     */
    static RET _invoke(ARGS... args) {
        auto * context = gum_interceptor_get_current_invocation();
        auto * handler = reinterpret_cast<MockHandler<RET, ARGS...> *>(
                gum_invocation_context_get_replacement_data(context));
        return handler->mAlterFun(std::forward<ARGS>(args)...);
    }
 
 
    MockHandler(void * target, const std::function<RET(ARGS...)> & fun) : mAlterFun(fun), mTarget(target) {
		Mock();
    }

    RET CallOriginal(ARGS... args) {
        auto f = reinterpret_cast<RET (*) (ARGS...)>(mOriginalTarget);
        return f(std::forward<ARGS>(args)...);
    }
 
	void Reset() {
		if (mInterceptor == nullptr || !mMocked) {
            return;
        }
        gum_interceptor_begin_transaction(mInterceptor);
        gum_interceptor_revert(mInterceptor, mTarget);
        gum_interceptor_end_transaction(mInterceptor);
 
 
        g_object_unref(mInterceptor);
		mMocked = false;
	}

	void Mock() {
		if (!mMocked) {
            gpointer original = nullptr;
			gum_init_embedded();
			mInterceptor = gum_interceptor_obtain();
			gum_interceptor_begin_transaction(mInterceptor);
			gum_interceptor_replace(mInterceptor, mTarget, toVoidPtr(_invoke), toVoidPtr(this), &original);
			gum_interceptor_end_transaction(mInterceptor);
			mMocked = true;
            mOriginalTarget = original;
		}
	}

    bool GetMocked() {
        return mMocked;
    }
 
    /**
     * 析构时会回滚已经替换掉的函数，实现测试用例隔离
     */
    ~MockHandler() {
        Reset();
    }
 
 
    MockHandler(const MockHandler &) = delete;
 
 
    MockHandler & operator=(const MockHandler &) = delete;
 
 
    MockHandler(MockHandler &&) = delete;
 
 
    MockHandler & operator=(MockHandler &&) = delete;
 
 
private:
    GumInterceptor * mInterceptor{};
 
    //目标函数地址
    void * mTarget{};

    void * mOriginalTarget{};
 
 
    //用于替换的函数
    std::function<RET(ARGS...)> mAlterFun;

	bool mMocked{false};
};
 
 
/**
 * 使用alter替换掉target函数指针，alter可以是lambda，也可以是函数指针
 * @tparam RET
 * @tparam CLS
 * @tparam ARG
 * @tparam T
 * @param target
 * @param alter
 * @return MockHandler
 */
template<typename RET, class CLS, typename ...ARG, typename T>
std::shared_ptr<MockHandler<RET, CLS *, ARG...>> Mock(RET(CLS::* target)(ARG...), T alter) {
    return std::make_shared<MockHandler<RET, CLS *, ARG...>>(toVoidPtr(target), alter);;
}
 
 
template<typename RET, class CLS, typename ...ARG, typename T>
std::shared_ptr<MockHandler<RET, CLS *, ARG...>> Mock(RET(CLS::* target)(ARG...) const, T alter) {
    return std::make_shared<MockHandler<RET, CLS *, ARG...>>(toVoidPtr(target), alter);
}

template<typename RET, typename ...ARG, typename T>
std::shared_ptr<MockHandler<RET, ARG...>> Mock(RET(* target)(ARG...), T alter) {
    return std::make_shared<MockHandler<RET, ARG...>>(toVoidPtr(target), alter);
}

template<typename RET, class CLS, typename ...ARG, typename T>
std::shared_ptr<MockHandler<RET, CLS *, ARG...>> MockVirtual(RET(CLS::* target)(ARG...), const CLS *obj, T alter) {
    return std::make_shared<MockHandler<RET, CLS *, ARG...>>(getVirtualFunction(target, obj), alter);
} 

template<typename RET, class CLS, typename ...ARG, typename T>
std::shared_ptr<MockHandler<RET, CLS *, ARG...>> MockVirtual(RET(CLS::* target)(ARG...) const, const CLS *obj, T alter) {
    return std::make_shared<MockHandler<RET, CLS *, ARG...>>(getVirtualFunction(target, obj), alter);
}

/**
 * mock返回值，mock的简便用法
 * @tparam RET
 * @tparam CLS
 * @tparam ARG
 * @param target
 * @param ret
 * @return MockHandler
 */
template<typename RET, class CLS, typename ...ARG>
std::shared_ptr<MockHandler<RET, CLS *, ARG...>> MockReturn(RET(CLS::* target)(ARG...), RET ret) {
    return std::make_shared<MockHandler<RET, CLS *, ARG...>>(toVoidPtr(target), [=](CLS *obj, ARG... arg) {
        return ret;
    });
}
 
 
template<typename RET, class CLS, typename ...ARG>
std::shared_ptr<MockHandler<RET, CLS *, ARG...>> MockReturn(RET(CLS::* target)(ARG...) const, RET ret) {
    return std::make_shared<MockHandler<RET, CLS *, ARG...>>(toVoidPtr(target), [=](CLS *obj, ARG&&... arg) {
        return ret;
    });
}
 
 
template<typename RET, typename ...ARG>
std::shared_ptr<MockHandler<RET, ARG...>> MockReturn(RET(target)(ARG...), RET ret) {
    return std::make_shared<MockHandler<RET, ARG...>>(toVoidPtr(target), [=](ARG&&... arg) {
        return ret;
    });
}

template<typename RET, typename ...ARG>
RET CallOriginal(ARG... args) {
    auto * context = gum_interceptor_get_current_invocation();
    auto * handler = reinterpret_cast<MockHandler<RET, ARG...> *>(
            gum_invocation_context_get_replacement_data(context));
    return handler->CallOriginal(std::forward<ARG>(args)...);
}

template<typename ...ARG>
void CallOriginalVoid(ARG... args) {
    auto * context = gum_interceptor_get_current_invocation();
    auto * handler = reinterpret_cast<MockHandler<void, ARG...> *>(
            gum_invocation_context_get_replacement_data(context));
    handler->CallOriginal(std::forward<ARG>(args)...);
}

}
