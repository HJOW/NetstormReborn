// 원본 BaseProcess.cpp의 실행 인터페이스. 가상 함수 슬롯 +0x18은 RunFrame에 대응한다.
#pragma once

namespace netstorm::o {
class BaseProcess {
public:
    // Kernel의 제거 함수가 원본 가상 소멸자 슬롯 0을 호출하던 동작을 재현한다.
    virtual ~BaseProcess() = default;
    // 원본 기반 클래스의 실행 메서드는 빈 함수다. 파생 프로세스가 재정의한다.
    virtual void RunFrame();
};
}
