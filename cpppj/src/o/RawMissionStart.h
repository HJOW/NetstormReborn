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
    std::int32_t denySalvage{},denyAscend{},allowAnyCapture{}; // 00595078/0059507c/005453e4의 atol DWORD다.
    std::uint32_t ordinaryGeysers{}; // 005950ac은 moreGeysers의 논리 부정이다.
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
    // 004c2b20 전체 순서이며 실제 효과가 실패하면 이후 상태/started 쓰기를 실행하지 않는다.
    void Start() const;
    // 004c3100: 현재 tellAtIndex 항목의 첫 글자 뒤를 atol로 읽는다. '@' 여부를 추가로 검사하지 않는다.
    void RefreshTellAt() const;
private:
    MissionFrameGateState& gate_; // 프레임 진행과 같은 장기 상태다.
    MissionStartState& state_; // base Mission/추가 Tutorial 필드다.
    MissionStartGlobals& globals_; // 매 원본 위치에서 다시 읽는 전역이다.
    MissionStartHooks hooks_; // 현재 설정/세계/Player/UI의 실제 효과다.
};
}
