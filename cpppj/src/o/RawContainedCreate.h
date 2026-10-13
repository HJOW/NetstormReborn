// Squid.cpp 004afa70 / CD 004ac4d0: 생산 칸 등에 넣는 contained Form을 생성한다.
#pragma once
#include "o/SquidFactory.h"
#include <functional>

namespace netstorm::o {
// 원본 contained/bridge 타입 전역이다. 외부 효과가 바꾼 값은 사용 시점에 다시 읽는다.
struct ContainedCreateState {
    // containedType: 내용물 타입.
    // bridgeType: 다리 자산을 타입 표에서 찾는 번호.
    std::uint32_t containedType{6},bridgeType{82};
};
// 생성자는 SID를 반환하며 표면 알림과 Form +0x50 부착은 명시 가상 경계다.
struct ContainedCreateHooks {
    // create: 타입과 생성 인자를 받아 새 대상을 만드는 콜백.
    std::function<Sid(std::uint32_t,std::uint32_t)> create;
    // notifySurface: 표면 객체에 변경을 통지하는 콜백.
    std::function<void(Sid)> notifySurface;
    // attach: 대상을 부모 또는 처리 목록에 부착하는 콜백.
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
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    SidPool& pool_;
    // state_: 원본 contained/bridge 타입 전역이다. 외부 효과가 바꾼 값은 사용 시점에 다시 읽는다 (ContainedCreateState).
    const ContainedCreateState& state_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (ContainedCreateHooks).
    ContainedCreateHooks hooks_;
};
// 실제 SquidFactory의 타입 6 contained 전용 생성자를 연결한다. 가상 부착은 별도 복원 경계다.
ContainedCreateHooks MakeContainedCreateHooks(const SidPool& pool,SquidFactory& factory,ContainedCreateHooks hooks);
}
