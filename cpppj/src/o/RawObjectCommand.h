// 공통 객체 명령004ae010과 Form 부모004ac9b0의 분기 및 가상 인자를 보존한다.
#pragma once
#include "o/SidPool.h"
#include <functional>

namespace netstorm::o {
// 각 분기가 소비하는 현재 전역을 조회한다. 메뉴를 연 시점의 사본으로 고정하지 않는다.
struct ObjectCommandState {
    std::function<std::uint32_t()> localOwner; // 가상+74에 전달할 현재00540c70 DWORD다.
    std::function<bool()> editor,mission; // 현재005c85a4/00594fa4이며 Hurt의 단락 평가 순서를 유지한다.
    std::uint32_t ownerCommand{0x030000afU},hurtCommand{0x030000e2U}; // 원본5429b0/542a7c의 명령 토큰이다.
    std::uint32_t destroyCommand{0x03000015U},describeCommand{0x0300006eU}; // Form 부모54274c/5428b0의 명령 토큰이다.
};
// 원본 가상 슬롯의 실제 효과를 같은 월드에 연결한다. 호출되지 않는 경계는 요구하지 않는다.
struct ObjectCommandHooks {
    std::function<void(Sid,std::uint32_t)> owner; // 가상+74이며 현재 로컬 소유자 전체 DWORD를 전달한다.
    std::function<void(Sid,std::int32_t,std::uint32_t,Sid)> hit; // 가상+98의 피해/소유자0/공격자0이다.
    std::function<void(Sid,std::uint32_t)> destroy; // Form 가상+10의 flags0 삭제다.
    std::function<void(Sid)> describe; // Form 가상+70의 진단 출력이며 일반 메뉴 열기로 대체하지 않는다.
};
// 10.78의 객체 명령 처리기다. CD10.72의 WORD HP/무조건 Hurt는 별도 원본 자료에만 남긴다.
class RawObjectCommand {
public:
    // 풀과 현재 상태는 처리기보다 오래 살아야 한다. 생성할 때 외부 효과를 호출하지 않는다.
    RawObjectCommand(const SidPool& pool,const ObjectCommandState& state,ObjectCommandHooks hooks);
    // 상위6비트를 제거한 명령을 처리하고 처리1/미처리0을 반환한다. 소유자/생존 필터를 추가하지 않는다.
    std::uint32_t Command(Sid parent,std::uint32_t code) const;
private:
    const SidPool& pool_; // 현재 HP DWORD를 읽는 같은 물리 슬롯 저장소다.
    const ObjectCommandState& state_; // 실제 모드/로컬 소유자와 명령 토큰의 현재 조회다.
    ObjectCommandHooks hooks_; // 같은 월드의 가상 소유자/피해/삭제와 별도 진단 경계다.
};
}
