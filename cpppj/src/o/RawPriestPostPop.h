// 사제 postPop에서 carrier 호출 전의 목록·회복 예약·소유자 상태 초기화를 복원한다.
#pragma once
#include "o/RawPriestOwner.h"
#include "o/SquidProcess.h"
#include "o/SquidReward.h"

namespace netstorm::o {
// 회복 Regular의 실제 이벤트 번호다(004950f0 / CD 0040c110).
inline constexpr std::uint32_t kPriestRegenEvent=0x25a;
// 005412d0/CD0051cbbc 사제 TYPE DWORD의 원본 초기값이다. 독립 구성의 기본 참조로만 쓰며 실제 월드는 현재 DWORD를 따로 빌린다.
// (2026-11-04 단위862: 회복 분자의 고정 TYPE158을 현재 전역 읽기로 바꾸며 추가했다.)
inline constexpr std::uint32_t kPriestPostPopInitialType=kPriestType;
// 사제 후처리 Pop 처리 중 조회·갱신하는 상태 묶음이다.
struct PriestPostPopState {
    SquidPostPopList priests; // 005954c4 / CD 00549188: 중복 없이 등록하는 사제 목록이다.
    std::array<std::uint32_t,kPlayerCount+1> ownerState{}; // 00595428 / CD 005119a0의 독립 raw 검사 호환 저장소다.
    std::array<std::uint32_t,kPlayerCount+1>* sharedOwnerState{}; // 실제 Session은 첫 소비자 구성 전에 장기9DWORD를 고정 연결한다. 사제 SID/count는 별도 sharedPriests로 빌린다.
    std::array<std::uint32_t,kPlayerCount+1>& OwnerState() { return sharedOwnerState ? *sharedOwnerState : ownerState; } // 실제 writer가 그 시점의 동일 공유 표에 직접 쓴다.
    const std::array<std::uint32_t,kPlayerCount+1>& OwnerState() const { return sharedOwnerState ? *sharedOwnerState : ownerState; } // outcome/AI/메뉴가 같은 현재9DWORD를 사본 없이 읽는다.
    SquidPostPopList* sharedPriests{}; //실제 Session은 장기16칸 SID/count를 직접 빌린다.
    SquidPostPopList& Priests() { return sharedPriests ? *sharedPriests : priests; } //Pop/삭제가 같은 현재 사제 목록에 직접 쓴다.
    const SquidPostPopList& Priests() const { return sharedPriests ? *sharedPriests : priests; } //AI/메뉴/outcome 조회가 같은 현재 목록을 읽는다.
};
// PATCH00492240의 진단→유효owner1..8 한DWORD0쓰기다. Pool/목록/Session 없이 같은 실제 표에 사용할 수 있다.
void ResetPriestOwnerState(std::array<std::uint32_t,kPlayerCount+1>& states,std::uint32_t player,
    const std::function<void(std::uint32_t)>& assertion={});
// 사제 후처리 Pop 외부 동작과 저장소 조회를 연결하는 콜백 묶음이다.
// PATCH는 실제 몸체가 도달한 장치만 그 시점에 요구하고, CD는 기존 구성 시 사전 계약을 유지한다(단위862).
struct PriestPostPopHooks {
    std::function<bool(Sid,std::uint32_t)> findRegular; // 실제 종속 체인에서 이미 예약된 회복을 찾는다.
    std::function<bool()> reserveRegular; // 원본 new(0x28)의 성공 여부. 실패하면 HP 조회/생성을 생략한다.
    std::function<void(Sid,std::uint32_t,float)> addRegular; // 확보 성공 뒤 payload를 계산하여 Regular에 연결한다.
};
// 사제 목록/회복 예약 접두를 실행한다. 전체 postPop 조합에서도 이 접두를 재사용한다.
class RawPriestPostPop {
public:
    // 풀과 최대 HP 모듈은 같은 월드를 참조해야 하며, 상태·훅·사제 TYPE DWORD의 참조 대상은 이 객체보다 오래 살아야 한다.
    // priestType은 005412d0/CD0051cbbc의 현재 DWORD이며 회복 분자 HP를 조회할 때마다 다시 읽는다.
    // (2026-11-04 단위862: PATCH 구성 시 훅 사전 요구를 제거하고 현재 사제 TYPE 참조를 추가했다.)
    RawPriestPostPop(SidPool& pool,const SquidReward& hp,PriestPostPopState& state,PriestPostPopHooks hooks,
        const std::uint32_t& priestType=kPriestPostPopInitialType);
    // 004950f0 / CD 0040c110의 carrier 진입 전까지만 실행한다. 전체 가상 postPop으로 등록하지 않는다.
    // extra & 9면 아무 효과도 없고, flags & 1일 때만 목록/회복을 예약한다.
    // PATCH는 사전 Validate/할당 확인 없이 현재 물리 슬롯과 signed 목록을 읽는다(단위862). CD는 기존 사전 계약을 유지한다.
    void Prefix(Sid sid,std::uint32_t flags) const;
    // 명시 사전 검사: 공간 쓰기 전에 가상 표/타입과 목록 개수를 확인한다. 효과는 발생시키지 않는다.
    // PATCH 실제 가상 진입은 이 검사를 거치지 않으며 독립 Validate/Activate 계약으로만 남는다.
    void Validate(Sid sid) const;
    // 전체 postPop 조합이 같은 풀의 접두를 연결하는지 확인한다.
    const SidPool& Pool() const;
private:
    // PATCH004950f0 접두 몸체: 도달 순서대로 extra→목록→검색→확보→현재 사제 TYPE HP→객체 최대HP→예약→소유자 상태를 실행한다.
    // (2026-11-04 단위862 추가)
    void PrefixPatch(Sid sid,std::uint32_t flags) const;
    // 확보 성공 뒤 현재 사제 TYPE HP와 객체 최대 HP로 원본 x87 순서의 회복 payload를 계산한다. 두 판본이 공유한다.
    // (2026-11-04 단위862: 고정 TYPE158 대신 현재 DWORD를 읽도록 분리했다.)
    float RegenPayload(Sid sid) const;
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    SidPool& pool_;
    // hp_: 객체의 HP를 조회·변경하는 처리기 또는 HP 값.
    const SquidReward& hp_;
    // state_: 처리 중 조회·갱신하는 상태 (PriestPostPopState).
    PriestPostPopState& state_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (PriestPostPopHooks).
    PriestPostPopHooks hooks_;
    // priestType_: 005412d0/CD0051cbbc 현재 사제 TYPE DWORD의 비소유 참조. 사본을 만들지 않는다(단위862).
    const std::uint32_t& priestType_;
};
// 기존 ProcessHost의 실제 찾기/부착에 연결한다. 호스트 메모리 부족은 C++ 예외로 전파한다.
// 원본 new 실패 분기를 재현할 때에는 반환 훅의 reserveRegular를 별도로 주입한다.
PriestPostPopHooks MakePriestPostPopProcessHooks(SquidProcessHost& host);
}
