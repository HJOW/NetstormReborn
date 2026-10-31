// PATCH004c2b20의 공통 시작 순서를 같은 미션 상태/현재 전역과 실제 효과 경계로 복원한다.
#pragma once
#include "o/RawMissionFrameGate.h"
#include "o/RiftType.h"
#include <string>
#include <string_view>

namespace netstorm::o {
// 기존 frameGate와 중복되지 않는 원본 Tutorial/base Mission 필드다.
struct MissionStartState {
    std::uint32_t flags{},sequence{},tellAtIndex{}; // +54/+58/+a0이다. Start는 tellAtIndex를 초기화하지 않는다.
    std::uint32_t fieldC0{},fieldC4{}; // +c0/+c4의 실제0쓰기다. 아직 의미를 추정해 이름을 붙이지 않는다.
};
// Start가 읽고 쓰는 현재 전역이다. 실제 Player/battleOwner의 소유 저장소와 함께 연결한다.
struct MissionStartGlobals {
    std::uint32_t localOwner{1},playerKnown{},editor{}; // 00540c70/00595344/005c85a4다.
    std::int32_t denySalvage{},denyAscend{},allowAnyCapture{}; // 독립 primitive fixture의 00595078/0059507c/005453e4 atol DWORD fallback이다.
    std::uint32_t ordinaryGeysers{}; // 005950ac은 moreGeysers의 논리 부정이다.
    std::int32_t* sharedAllowAnyCapture{};std::uint32_t* sharedOrdinaryGeysers{}; // Start의 실제 출력을 장기 owner에 쓴다.
    std::int32_t* sharedDenySalvage{};std::int32_t* sharedDenyAscend{}; // World 없는 Guide 소멸도 Client의 같은00595078/7c에 쓰도록 빌린다. 근거: docs/exe/cpp-guide-release-20261101.md.
    std::int32_t& DenySalvage() { return sharedDenySalvage ? *sharedDenySalvage : denySalvage; } // 실제 owner가 연결되면 회수 금지 전체 DWORD를 그 주소에 쓴다.
    const std::int32_t& DenySalvage() const { return sharedDenySalvage ? *sharedDenySalvage : denySalvage; } // 미연결 독립 검사만 기존 값형 fallback을 읽는다.
    std::int32_t& DenyAscend() { return sharedDenyAscend ? *sharedDenyAscend : denyAscend; } // Start와 메뉴가 같은 현재 승급 금지 DWORD를 직접 공유한다.
    const std::int32_t& DenyAscend() const { return sharedDenyAscend ? *sharedDenyAscend : denyAscend; } // 현재 signed DWORD의 음수·비영 값을 정규화하지 않는다.
    std::int32_t& AllowAnyCapture() { return sharedAllowAnyCapture ? *sharedAllowAnyCapture : allowAnyCapture; } // atol의 전체 DWORD를 보존한다.
    const std::int32_t& AllowAnyCapture() const { return sharedAllowAnyCapture ? *sharedAllowAnyCapture : allowAnyCapture; } // 현재 출력 DWORD를 const로도 읽는다.
    std::uint32_t& OrdinaryGeysers() { return sharedOrdinaryGeysers ? *sharedOrdinaryGeysers : ordinaryGeysers; } // moreGeysers의 논리 부정을 저장한다.
    const std::uint32_t& OrdinaryGeysers() const { return sharedOrdinaryGeysers ? *sharedOrdinaryGeysers : ordinaryGeysers; } // 현재 부정 값을 별도 사본으로 만들지 않는다.
};
// 실제 Session이 소유한 현재 전역을 원본 읽기 위치에서 조회한다. 미연결 fixture만 기존 Globals 값을 사용한다.
struct MissionStartReads {
    std::function<std::uint32_t()> localOwner; // 00540c70의 현재 PlayerWorld.battleOwner이며 효과 뒤 다시 읽는다.
    std::function<std::uint32_t()> playerKnown; // 00595344의 현재 실제 PlayerKnown DWORD다.
    std::function<std::uint32_t()> editor; // 005c85a4의 현재 실제 CampaignWorld editor DWORD다.
};
// 설정/세계/UI는 도달한 원본 위치에서 실제 몸체로 연결한다. 누락 효과는 성공으로 처리하지 않는다.
struct MissionStartHooks {
    std::function<std::string(std::string_view)> get; // 같은 현재 mission.<key>를 조회한다.
    std::function<std::string(std::string_view)> sectionNames; // 같은 Config의00440640(prefix)다.
    std::function<void(std::string_view,std::string_view)> set; // 같은 Config의004404e0(key,value)다.
    std::function<double()> gameSeconds; // 실제 double 게임 시각이며 demo/start 위치마다 다시 읽는다.
    std::function<void(std::uint32_t,float)> setSp; // 0040edb0의 실제 분산SP 직접 지정이다.
    std::function<void()> refreshSp; // 0043dad0의 현재 Gump 재구성이다.
    std::function<void(std::uint32_t)> clearTechnology; // 004903b0의 현재 기술 contained 제거다.
    std::function<void(std::uint32_t,std::string_view)> addTechnology; // 004915c0의 실제 기술 생성 사슬이다.
    std::function<bool(std::uint32_t)> hasPriest; // 004921f0의 실제 현재 사제 SID가 비제로인지다.
    std::function<void(std::uint32_t)> createAi; // 004c2d40이며600바이트 설정→실제생성 사슬이어야 한다.
    std::function<void()> refreshCommandGump; // 0042e440:00548740 Gump에00542914/0/0을 전달한다.
    std::function<void()> randomGeysers; // 00486440의 현재 세계 전체 랜덤 간헐천 생성이다.
    std::function<void()> resetCamera; // 004340d0의 실제 카메라 초기화다.
};
// 기존 frameGate의 start/demo/tellAt/started를 직접 참조한다. 별도 성공/시작 상태를 만들지 않는다.
class RawMissionStart {
public:
    // 현재 미션 상태와 필수 설정 조회를 빌린다. 실제 효과는 선택된 경계에서 확인한다.
    RawMissionStart(OriginalEdition edition,MissionFrameGateState& gate,MissionStartState& state,MissionStartGlobals& globals,MissionStartHooks hooks);
    // 세 현재 getter를 모두 연결한다. 값형 fallback을 갱신하거나 getter 반환을 저장하지 않는다.
    void BindCurrentInputs(MissionStartReads reads);
    // 004c2b20 전체 순서이며 실제 효과가 실패하면 이후 상태/started 쓰기를 실행하지 않는다.
    void Start() const;
    // 004c3100: 현재 tellAtIndex 항목의 첫 글자 뒤를 atol로 읽는다. '@' 여부를 추가로 검사하지 않는다.
    void RefreshTellAt() const;
private:
    // 원본이 현재 전역을 읽는 위치에서만 getter 또는 기존 값형 fixture를 선택한다.
    std::uint32_t LocalOwner() const;
    // SP 쓰기를 선택하는 현재 PlayerKnown만 읽는다. 로컬 owner를 앞당겨 읽지 않는다.
    std::uint32_t PlayerKnown() const;
    // myStartMoney 조회 뒤의 현재 편집 모드를 읽는다.
    std::uint32_t Editor() const;
    MissionFrameGateState& gate_; // 프레임 진행과 같은 장기 상태다.
    MissionStartState& state_; // base Mission/추가 Tutorial 필드다.
    MissionStartGlobals& globals_; // 매 원본 위치에서 다시 읽는 전역이다.
    MissionStartHooks hooks_; // 현재 설정/세계/Player/UI의 실제 효과다.
    MissionStartReads reads_; // 실제 Session의 현재 getter이며 미연결 fixture의 Globals와 구분한다.
};
}
