// 원본 _rand의 게임 난수와 별개인 TLS 스트림을 복원한다.
#pragma once
#include <cstdint>

namespace netstorm::o {
// 실제004e64ab의32비트 상태와15비트 반환이다.0은 재시드하지 않는다.
class CrtRandom {
public:
    // 원본 새 TLS의 초기값은1이며 독립 대조에는 명시 상태를 전달한다.
    explicit CrtRandom(std::uint32_t state=1);
    // 상태×214013+2531011의 low32를 저장하고 상위15비트를 반환한다.
    std::uint32_t Next();
    // 독립 원문 대조/재현에서 실제 현재 DWORD를 읽는다.
    std::uint32_t State() const;
    // 원본 srand의 TLS 상태 쓰기와 같은 명시 재시드다.
    void SetState(std::uint32_t state);
private:
    std::uint32_t state_; // 같은 스레드의 다음 CRT 난수가 소비할32비트 상태다.
};
// 게임/미션마다 초기화하지 않는 원본 TLS 수명이다. 같은 스레드의 모든 CRT 소비자가 공유한다.
CrtRandom& ThreadCrtRandom();
}
