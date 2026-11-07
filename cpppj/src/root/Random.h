// R의 실제 process 전역 난수 상태를 Client CRT TLS와 분리해 공유한다.
#pragma once
#include "o/CrtRandom.h"

namespace netstorm::root { //0048bac0/0048bad0과004cda10의 현재 한 DWORD다.
class Random final { // 서버 재초기화/CCh 교체로 자동 재시드하지 않는 프로세스 owner다.
public:
    Random()=default; // PE004cda10의 초기1만 구성하며 난수를 소비하지 않는다.
    Random(const Random&)=delete;Random& operator=(const Random&)=delete; // 같은 현재 스트림을 다른 seed로 복제하지 않는다.
    std::uint32_t Next() { return o::CrtRandom::Borrow(state_).Next(); } // 같은 LCG/15비트 계산만 재사용하며 ThreadCrtRandom을 호출하지 않는다.
    void Seed(std::uint32_t value) { state_=value; } //0048bac0처럼0도 그대로 현재 전역에 쓴다.
    const std::uint32_t& State() const { return state_; } // 실제 현재 상태를 읽고 스트림은 전진시키지 않는다.
private:
    std::uint32_t state_{1}; // 원문 data초기1인 현재004cda10이다.
};
}
