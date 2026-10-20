// 원본 활성 미션의 F8 예약·Guide First 사건과 현재 안내 재표시를 연결한다.
#pragma once
#include "client/InputEvent.h"
#include "client/ScriptCommand.h"
#include "o/RawMissionStart.h"

namespace netstorm::client {
inline constexpr std::uint32_t kMissionFirstCommand=0x02000091U; // 원본00542938의 현재 미션 첫 단계/안내 재표시 명령이다.
inline constexpr std::uint32_t kMissionReloadCommand=0x0200008dU; // 원본00542928의 현재 미션 재생성 명령이며 데모 타이머가 일반 큐에 넣는다.
inline constexpr std::uint32_t kMissionAbortCommand=0x0200008cU; // 원본00542924의 확인 창/종료 예약 명령이다. 분기는 사건의 세 번째 DWORD를 읽는다.
inline constexpr std::uint32_t kLeaveBattleCommand=0x020000c5U; // 원본00542a08의 실제 전투 떠나기 명령이다. Abort 비제로에서 인자2/2를 예약한다.
inline constexpr std::uint32_t kMissionBeginCommand=0x0200008bU; // 원본00542920의 문자열 인자 미션 시작 명령이다.
inline constexpr std::uint32_t kMissionForkCommand=0x0200008eU; // 원본0054292c의 fork%d 분기 시작 명령이다.
inline constexpr std::uint32_t kMissionEndCommand=0x0200008fU; // 원본00542930의 현재 미션 가상 종료/해제 명령이다.
// 복원한 여섯 원본 제어 이름을 대소문자와 무관하게 조회한다. 그 외에는 원본 미등록값ffffffff다.
std::uint32_t MissionControlScriptCode(std::string_view name);
// 페이지 버튼/명령의 여섯 제어 이름만 같은 원본 토큰/80칸 문자열/40칸 입력 큐에 예약한다.
// 지원하지 않는 이름은 상태와 고리를 바꾸지 않고 false이며 인자는 기존 QueueScriptCommand 규칙을 따른다.
bool QueueMissionControlAction(std::string_view name,std::string_view argument,InputQueue& queue,ScriptStringRing& strings);
// 00463e40의 Abort 분기다. 확인0은 실제 ABORT Tell, 그 외는 LeaveBattle/2/2 예약이며 항상 소비한다.
bool DispatchMissionAbort(const InputEvent& event,InputQueue& queue,const std::function<void(std::string_view)>& tell);
// 원본 Reload가 해제 전에 캡처하는 미션 이름과 bool로 정규화한 두 필드다.
struct MissionReloadRequest {
    std::string name; // 현재 Guide 이름의 소유 복사본이며 기존 Script/월드 해제 뒤에도 유효하다.
    std::uint32_t mode{},demo{}; // 원본+64/+68의 비제로 판정을 각각0/1 DWORD로 보존한다.
};
// 00463cf0의 해제 전 캡처다. 전체 Guide 해제/공장 단계는 실제 Client 수명 경계가 수행한다.
MissionReloadRequest CaptureMissionReload(std::string_view name,std::uint32_t mode,std::uint32_t demo);
// 004c3290의 demo 문자열 분기다. 빈 문자열은 기존 직접 지정값을 유지하고 그 외에는 실제 atol DWORD로 덮어쓴다.
void ApplyMissionDemo(std::string_view text,std::uint32_t& demo);
// 현재 안내 몸체의 실제 Tell·팝업·가상 정지 경계다. 현재 상태는 별도로 복제하지 않는다.
struct MissionStageEffects {
    std::function<std::uint32_t(std::string_view)> tell; // 00482de0의 실제 반환 DWORD이며 비제로일 때만 후속 효과를 실행한다.
    std::function<void()> closeTransient; // 00466710의 실제 팝업 Kill 요청이다.
    std::function<void()> pause; // 현재 미션의 가상+28이며 같은 정지 깊이를 사용한다.
};
// 004c2a90: 현재 demo/단계로 섹션을 선택하고 Tell→Kill→정지 후 마지막 반환 DWORD를 쓴다.
void TellMissionStage(const o::MissionFrameGateState& gate,std::uint32_t& stageShown,const MissionStageEffects& effects);
// 004c2a70: 순번0에서는 flags bit1만 요청하고 그 외에는 현재 안내를 다시 표시한다.
void MissionFirst(o::MissionStartState& state,const std::function<void()>& tellStage);
// 00463e40의 First 분기다. 상위 플래그 제거 후 일치하면 현재 미션이 없어도1을 반환한다.
bool DispatchMissionFirst(const InputEvent& event,const std::function<void()>& currentFirst);
// 원본 key-down 활성 미션의 F8 분기를 같은40칸 큐에 연결한다. Ctrl/Shift·뗌은 예약하지 않는다.
// 도전/편집/디버그 모드와 다른 키의 전체 분배는 이 함수의 복원 범위 밖이다.
bool QueueMissionFirstKey(const InputEvent& event,InputQueue& queue,bool activeMission);
}
