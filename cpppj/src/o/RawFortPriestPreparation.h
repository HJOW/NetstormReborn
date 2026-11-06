// PATCH004923e0의 기본 기술/지역 사제 준비와00492120/004921a0의 현재 목록 순서를 보존한다.
#pragma once
#include "o/RawPriestPostPop.h"
#include <functional>

namespace netstorm::o { // 일반 Fort 설치의 현재 전역과 실제 사제 목록만 빌린다.
struct FortPriestPreparationGlobals {
    const bool& missionPost;const std::uint32_t& editor; // 원문의 두 자연 반환/지역 처리 관문이다.
    const std::uint32_t& subscriber;const std::uint32_t& templateOwner;const std::uint32_t& playerOwner; // 색상 기술·고정 Template 선택·소유자 재조회는 별도 전역이다.
};
struct FortPriestPreparationHooks {
    std::function<std::uint32_t(std::uint32_t)> baseType,themeType; // 앞 세 TYPE와 subscriber%3의 테마 TYPE를 원문 시점에 읽는다.
    std::function<std::uint32_t()> technology; // 각 기술마다0048fc60의 현재 부모를 다시 읽는다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t)> contains; // 같은 부모의 mask4/type/index0 첫 내용물이다.
    std::function<void(std::uint32_t,std::uint32_t)> createTechnology; // 실제004afa70(parent,type,0,4,0x10,0)이다.
    std::function<bool(std::uint32_t)> templatePresent;std::function<std::span<std::uint8_t>(std::uint32_t)> state; // 같은 현재 헤더와 State+17 BYTE를 읽는다.
    std::function<void(std::uint32_t,std::span<std::uint8_t,120>)> territory; // 같은 실제 manager의20×6BYTE 기록을 읽는다.
    std::function<std::int32_t(std::uint32_t,std::uint32_t)> island; // 현재 playerOwner와 Territory 인덱스로0046f380을 호출한다.
    std::function<std::int32_t(float,float)> region; // 각 목록 항목의 raw 좌표를0046f530에 전달한다.
    std::function<void(std::uint32_t,std::uint32_t)> setOwner; // 실제 가상74이며 목록/상태 부수 효과도 보존한다.
    std::function<void(std::int32_t)> populate; // 실제 ChunkMap bounds→0040d900→0040e2b0의 사제 초기 배치다.
    std::function<void(std::uint32_t)> tellPriests; // 남은 수가 양수이면 번역/형식화한 global.priests를 쓴다.
};
class RawFortPriestPreparation {
public:
    RawFortPriestPreparation(const SidPool& pool,const PriestPostPopState& priests,FortPriestPreparationGlobals globals,FortPriestPreparationHooks hooks); // 같은 실제 목록/풀/현재 전역을 더 긴 소유자에서 빌린다.
    void Run() const; // 기술4종 뒤 현재 관문을 다시 읽고 지역20개를 원문 순서로 처리한다.
    void Collect(std::int32_t region,std::span<std::uint32_t> entries,std::uint32_t& count) const; //00492120의 현재 목록 재조회/유한 용량 계약이다.
    std::uint32_t Marked(std::span<const std::uint32_t> entries) const; //004921a0은 첫 raw+12 bit80만 검사한다. owner/생존 필터를 추가하지 않는다.
private:
    std::span<const std::uint8_t> Raw(std::uint32_t sid) const; // 현재 DWORD SID가 실제 Pool의 물리 범위 안인지 접근 때만 검사한다.
    const SidPool& pool_;const PriestPostPopState& priests_; // 같은 raw Pool과 등록/삭제가 공유하는 사제 목록이다.
    FortPriestPreparationGlobals globals_;FortPriestPreparationHooks hooks_; // 모드 사본이나 기본 성공 장치를 만들지 않는다.
};
}
