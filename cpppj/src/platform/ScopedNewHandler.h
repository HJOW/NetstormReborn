// 실제 C++ new_handler 등록을 정상/예외 복귀에서 이전 프로세스 전역 값으로 돌려놓는다.
#pragma once
#include <new>

namespace netstorm::platform { // OS 메모리 고갈을 강제하지 않고 표준 런타임의 등록 수명만 감싼다.
class ScopedNewHandler final {
public:
    explicit ScopedNewHandler(std::new_handler handler) noexcept:previous_(std::set_new_handler(handler)) {} // 지정된 현재 handler를 설치하고 이전 handler를 소유한다.
    ~ScopedNewHandler() { std::set_new_handler(previous_); } // 부분 초기화·정상 반환·예외 모두 같은 이전 등록을 복원한다.
    ScopedNewHandler(const ScopedNewHandler&)=delete;ScopedNewHandler& operator=(const ScopedNewHandler&)=delete; // 한 번의 등록 수명을 복제하지 않는다.
    ScopedNewHandler(ScopedNewHandler&&)=delete;ScopedNewHandler& operator=(ScopedNewHandler&&)=delete; // 복원 순서가 다른 범위로 이동하지 않게 한다.
private:
    std::new_handler previous_; // 현재 범위에 진입하기 직전의 실제 프로세스 전역 등록값이다.
};
}
