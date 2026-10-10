// Squid.cpp 004afa70 / CD 004ac4d0: 생산 칸 등에 넣는 contained Form을 생성한다.
#pragma once
#include "o/SquidFactory.h"
#include <functional>

namespace netstorm::o {
// 원본 contained/bridge 타입 전역이다. 외부 효과가 바꾼 값은 사용 시점에 다시 읽는다.
struct ContainedCreateState {
    std::uint32_t containedType{6},bridgeType{82};
};
// 생성자는 SID를 반환하며 표면 알림과 Form +0x50 부착은 명시 가상 경계다.
struct ContainedCreateHooks {
    std::function<Sid(std::uint32_t,std::uint32_t)> create;
    std::function<void(Sid)> notifySurface;
    std::function<void(Sid,Sid,std::uint32_t)> attach;
};
// 새 Form의 타입 WORD·패턴 WORD·플래그 WORD·판본별 품질 BYTE 쓰기 순서를 복원한다.
class RawContainedCreate {
public:
    // 상태와 풀은 객체보다 오래 살아야 한다. 가상 경계 세 가지는 모두 필수다.
    RawContainedCreate(SidPool& pool,const ContainedCreateState& state,ContainedCreateHooks hooks);
    // attachFlags 0x20은 예측 생성, 0x10은 클라이언트 생성이며 0x20이 우선한다.
    // flags가 DWORD 전체로 0일 때만 0x20을 기본값으로 사용한다. raw에는 하위 WORD/BYTE만 쓴다.
    Sid Create(Sid parent,std::uint32_t type,std::uint32_t argument,std::uint32_t flags,
        std::uint32_t attachFlags,std::uint32_t quality) const;
    // 어댑터가 동일한 풀에 연결되는지 확인한다.
    const SidPool& Pool() const;
private:
    SidPool& pool_;
    const ContainedCreateState& state_;
    ContainedCreateHooks hooks_;
};
// 실제 SquidFactory의 타입 6 contained 전용 생성자를 연결한다. 가상 부착은 별도 복원 경계다.
ContainedCreateHooks MakeContainedCreateHooks(const SidPool& pool,SquidFactory& factory,ContainedCreateHooks hooks);
}
