// Carrier의 실제 가상c8인4287d0과 복귀 위치 거의 올림41d7a0을 복원한다.
#pragma once
#include "o/SidPool.h"
#include <functional>
namespace netstorm::o {
// 낙하 시작이 호출하는 현재 조회/이동 삭제/선택/해방/예약의 실제 경계다.
struct CarrierFallStartHooks {
    std::function<bool(Sid,std::uint32_t)> hasRegular; // 4afb40(parent,5105)의 존재다.
    std::function<std::uint32_t(std::uint32_t)> movementType; // 488fe0(type,0,1)은 TYPE 인자만 사용한다.
    std::function<void(Sid,std::uint32_t)> removeMovement; // 4b1a90(parent,type,0,1)의 실제 삭제다.
    std::function<void()> clearSelection; // 4d5c90(0,0,1)의 같은 현재 UI 몸체다.
    std::function<Sid(Sid,std::uint32_t)> findKind; // 4b26f0(parent,현재 사제,0,0)이다.
    std::function<void(Sid)> releasePriest; // 426c80의 실제 사제 해방이다.
    std::function<void(Sid,std::uint32_t,float)> task; // 마지막426db0(parent,5105,0)이다.
};
// 시작과 종료를 같은 Player 복귀 기록/권한/사제 종류에서 실행한다.
class RawCarrierFallStart {
public:
    RawCarrierFallStart(const SidPool& pool,std::span<std::uint8_t> players,const bool& boss,const std::uint32_t& priestType,CarrierFallStartHooks hooks); // 현재9 Player 기록도 허용하며 잘못된 이전 owner는 쓰기 전에 거부한다.
    bool Begin(Sid parent) const; // 기존 낙하 중이면 즉시 참, 아니면 전체 시작 순서 뒤 참이다.
    const SidPool& Pool() const; // 다른 낙하/생성 모듈의 풀 혼합을 검사한다.
private:
    const SidPool& pool_; // 부모와 포획 사제 Form의 동일 물리 풀이다.
    std::span<std::uint8_t> players_; // 이전 소유자의 a8/ac에 복귀 좌표를 기록한다.
    const bool& boss_; // 각 권한 분기에서 현재 값을 다시 읽는다.
    const std::uint32_t& priestType_; // 선택/이동 삭제 뒤 현재 사제 종류를 읽는다.
    CarrierFallStartHooks hooks_; // 실제 큰 하위 경계다.
};
}
