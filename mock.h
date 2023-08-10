
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include "frida/frida-gum.h"

// frida-gum 下载地址 https://github.com/frida/frida/releases/tag/16.1.3
 
#define _COMBINE_(X, Y) X##Y  // helper macro
#define COMBINE(X, Y) _COMBINE_(X,Y)
#define MOCK(target, alter) auto && COMBINE(mock, __LINE__) = mock::Mock(target, alter)
#define MOCK_RETURN(target, alter) auto && COMBINE(mock, __LINE__) = mock::MockReturn(target, alter)
 
 
namespace {
/**
 * 将任意对象转换成void *
 * 用于函数指针的转换
 * @tparam T
 * @param t
 * @return 转换成void * 的函数指针
 */
template<typename T>
void * toVoidPtr(T src) {
    union {
        void * ptr;
        T src;
    } Func{};
    Func.src = src;
    return Func.ptr;
}
}
 
 
namespace mock {
 
 
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
		if (mInterceptor == nullptr || !mocked) {
            return;
        }
        gum_interceptor_begin_transaction(mInterceptor);
        gum_interceptor_revert(mInterceptor, mTarget);
        gum_interceptor_end_transaction(mInterceptor);
 
 
        g_object_unref(mInterceptor);
		mocked = false;
	}

	void Mock() {
		if (!mocked) {
            gpointer original = nullptr;
			gum_init_embedded();
			mInterceptor = gum_interceptor_obtain();
			gum_interceptor_begin_transaction(mInterceptor);
			gum_interceptor_replace(mInterceptor, mTarget, toVoidPtr(_invoke), toVoidPtr(this), &original);
			gum_interceptor_end_transaction(mInterceptor);
			mocked = true;
            mOriginalTarget = original;
		}
	}

    bool GetMocked() {
        return mocked;
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

	bool mocked{false};
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
auto Mock(RET(CLS::* target)(ARG...), T alter) {
    return std::make_shared<MockHandler<RET, CLS *, ARG...>>(toVoidPtr(target), alter);;
}
 
 
template<typename RET, class CLS, typename ...ARG, typename T>
auto Mock(RET(CLS::* target)(ARG...) const, T alter) {
    return std::make_shared<MockHandler<RET, CLS *, ARG...>>(toVoidPtr(target), alter);
}
 
 
template<typename RET, typename ...ARG, typename T>
auto Mock(RET(* target)(ARG...), T alter) {
    return std::make_shared<MockHandler<RET, ARG...>>(toVoidPtr(target), alter);;
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
auto MockReturn(RET(CLS::* target)(ARG...), RET ret) {
    return std::make_shared<MockHandler<RET, CLS *, ARG...>>(toVoidPtr(target), [=](auto && ...) {
        return ret;
    });
}
 
 
template<typename RET, class CLS, typename ...ARG>
auto MockReturn(RET(CLS::* target)(ARG...) const, RET ret) {
    return std::make_shared<MockHandler<RET, CLS *, ARG...>>(toVoidPtr(target), [=](auto && ...) {
        return ret;
    });
}
 
 
template<typename RET, typename ...ARG>
auto MockReturn(RET(target)(ARG...), RET ret) {
    return std::make_unique<MockHandler<RET, ARG...>>(toVoidPtr(target), [=](auto && ...) {
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