// PATCH10.78의 buried 전용00462170을 같은 현재 게임 전역과 공통 장부에 연결한다.
#pragma once
#include "o/SidPool.h"
#include "o/RawGraph.h"
#include <functional>

namespace netstorm::o {
// 실제 난수 선택/개별 할당/Regular 생성/공통 후처리의 외부 몸체다.
struct BuriedPostPopHooks {
    std::function<std::uint32_t(std::uint32_t)> chooseType; // 00461f00(5)의 실제 보상 타입 선택이며 선택된 경우만 필요하다.
    std::function<bool()> reserveRegular; // 두 new(0x28)의 성공 여부를 각각 조회한다.
    std::function<void(Sid,std::uint32_t,float)> addRegular; // 사건0/0초와 사건1/0.01초를 같은 SID에 생성한다.
    std::function<void(Sid,std::uint32_t,const RawGraphPop*)> validateBase; // Pop 공간 쓰기 전 같은 공통 장부를 검증한다.
    std::function<void(Sid,std::uint32_t)> base; // 원본004b0d30 비가상 몸체이며 깊이를 한 번 감소시킨다.
};
// 현재 boss/로딩 저장소는 이 객체보다 오래 살아야 하며 값 사본으로 고정하지 않는다.
class RawBuriedPostPop {
public:
    // 이 단위는 PATCH만 복원했다. CD00416ac0은 별도 원문 대조 전 지원하지 않는다.
    RawBuriedPostPop(SidPool& pool,const bool& boss,const std::uint32_t& loading,BuriedPostPopHooks hooks);
    bool Handles(Sid sid) const; // 실제 buried 생성자의 가상 표만 허용한다.
    void Validate(Sid sid,std::uint32_t flags,const RawGraphPop* pop=nullptr) const; // 같은 공통 공간/장부 경계를 사전 검사한다.
    void PostPop(Sid sid,std::uint32_t flags) const; // 현재 분기에 필요한 실제 효과를 원문 순서로 실행한다.
    const SidPool& Pool() const; // 파생 등록 시 동일 월드인지 확인한다.
private:
    SidPool& pool_; // 실제 QA WORD를 포함한 단일 원본 객체 풀이다.
    const bool& boss_; // DAT00540bc4의 현재 권한 조건이며 server/전투 모드와 구별한다.
    const std::uint32_t& loading_; // DAT005c89b8의 현재 중첩 로딩 깊이다.
    BuriedPostPopHooks hooks_; // 원본문체의 외부 호출 경계이며 선택 위치에서 필수 확인한다.
};
}
