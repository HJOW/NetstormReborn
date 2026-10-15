// 사제 보호막의 조회→생성/소유자/Pop→소리→로컬 안내 요청을 복원한다.
#pragma once
#include "o/RawPriestForcefield.h"
#include "o/SquidFactory.h"
#include "o/SquidOwner.h"
#include <string>
#include <string_view>

namespace netstorm::o {
// 사제 보호막 처리 중 조회·갱신하는 상태 묶음이다.
struct PriestShieldState {
    std::uint32_t localPlayer{},loadingDepth{}; // 로컬 플레이어 DWORD와 요새 로딩 중첩 전역이다.
    double noticeClock{}; // 원본은 +0/-0일 때만 로컬 알림을 요청하며 NaN은 제외한다.
};
// 사제 보호막 통지 동작을 처리하는 자료형이다.
struct PriestShieldNotice {
    // file: 읽기·재생에 사용할 파일 이름 또는 열린 파일 자료.
    std::string_view file{"ourPriestImmobile.wav"};
    std::array<std::uint32_t,5> arguments{0,0,0,1,0}; // 전역 소리 함수의 원본 인자를 보존한다.
};
// 사제 보호막 외부 동작과 저장소 조회를 연결하는 콜백 묶음이다.
struct PriestShieldHooks {
    // 자산 생성은 flags=2인 로컬 풀 할당이다. factory 어댑터로 실제 생성자를 연결한다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create;
    // setOwner: 대상의 소유 플레이어를 변경하는 콜백.
    std::function<void(Sid,std::uint32_t)> setOwner;
    // 보호막 가상 +0x90의 전체 공간 효과는 필수 외부 경계다.
    std::function<void(Sid,float,float,std::uint32_t)> pop;
    // 원본 new(0x28) 실패이면 소리 조회/프로세스 생성만 생략한다.
    std::function<bool()> reserveSound;
    // 파일명 하나의 cdecl 조회다. 스택의 0/1은 다음 소리 프로세스 생성 인자다.
    std::function<std::uint32_t(std::string_view)> loadSound;
    // attachSound: 대상에 연결된 위치에서 소리를 재생하는 콜백.
    std::function<void(Sid,std::uint32_t,std::uint32_t,std::uint32_t)> attachSound;
    // noticeSound: 통지에 대응하는 소리를 재생하는 콜백.
    std::function<void(const PriestShieldNotice&)> noticeSound;
    // 패치만 PriestImmobile을 포맷하여 Tell 섹션 이름으로 전달한다. 원본은 Xlat 번역을 호출하지 않는다. CD에는 두 경계가 없다.
    std::function<std::string(std::string_view)> translate;
    // tell: 미션 스크립트의 안내 섹션을 요청하는 콜백.
    std::function<void(std::string_view)> tell;
};
// 사제 보호막의 조회→생성/소유자/Pop→소리→로컬 안내 요청을 복원한다.
class RawPriestShield {
public:
    // 조회·타입 전역은 같은 풀을 참조하며 외부 상태/효과 대상은 더 오래 살아야 한다.
    RawPriestShield(SidPool& pool,const RawPriestForcefield& lookup,const PriestShieldState& state,PriestShieldHooks hooks);
    // 00493d30 / CD 0040bfb0: 기존 보호막이 있으면 모든 생성/소리/안내를 생략한다.
    void Ensure(Sid priest) const;
    // 후속 사제 postPop에 같은 풀의 실제 보호막 생성 몸체를 연결한다.
    const SidPool& Pool() const;
private:
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    SidPool& pool_;
    // lookup_: 조건에 맞는 현재 객체를 찾는 조회 처리기.
    const RawPriestForcefield& lookup_;
    // state_: 처리 중 조회·갱신하는 상태 (const PriestShieldState).
    const PriestShieldState& state_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (PriestShieldHooks).
    PriestShieldHooks hooks_;
};
// 생성·소유자 지정만 실제 모듈로 바꾸며 Pop/소리/안내의 미복원 효과는 유지한다.
PriestShieldHooks MakePriestShieldCreationHooks(const SidPool& pool,SquidFactory& factory,SquidOwner& owner,PriestShieldHooks hooks);
}
