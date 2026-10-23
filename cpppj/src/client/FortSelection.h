// 원본 10.78의 편집 맵 선택과 다음 State 예약을 같은 현재 전역에 적용한다.
#pragma once
#include "client/InputEvent.h"
#include "client/ScriptCommand.h"
#include <functional>
#include <span>
#include <string>
#include <string_view>

namespace netstorm::client {
inline constexpr std::uint32_t kAskEditFortCommand=0x020000c9U; // 원본 명령 표의 AskEditFort다. 인자를 사용하지 않는다.
inline constexpr std::uint32_t kReadFortCommand=0x020000caU; // 원본 ReadFort다. 맵 이름은 사건의 y 문자열 토큰이다.
// 콜백 복귀마다 원본이 다시 읽는 실제 전역을 빌린다. 다음 상태의 부가 인자는 바꾸지 않는다.
struct FortSelectionGlobals {
    std::uint32_t& mode; // 005cae84: AskEditFort는2를 쓰고 ReadFort는 pending 복귀 뒤 읽는다.
    std::uint32_t& nextState; // 005c8878: 기존 요청 여부와 무관하게1 또는6으로 덮어쓴다.
    const std::uint32_t& inBattle; // 00594fbc: 전투 중 기존 Fort의 상태 보존 관문이다.
    const std::uint32_t& currentState; // 005c8758: 확정된 현재 상태14일 때만 보존한다.
    const std::uint32_t& templateOwner; // 00540cac: 각 파일/State/Notify 경계 직전에 다시 읽는 슬롯이다.
    const std::uint32_t& owner; // 00540c70: 현재 플레이어의 섬 수 조회에 사용한다.
};
// 선택된 원본 경계만 제공한다. 현재 월드 설치/제목/dirty 변경은 이 명령의 효과가 아니다.
struct FortSelectionHooks {
    std::function<std::string_view(std::uint32_t)> resolveString; // 사건 y의 현재 원본 문자열 고리 항목을 빌린다.
    std::function<std::string()> lastEditPath; // PathSpec(lastEditFort,0,0,0)의 실제 현재 설정 조회다.
    std::function<void(std::string_view)> pending; // 00460bb0: current Fort와 별개 pending 이름만 복사한다.
    std::function<void(std::uint32_t)> refresh; // 004cafd0: 같은 맵 목록의 모드2 갱신이다.
    std::function<void(std::string_view)> tell; // 004cf960: 실제 LoadEditable 안내를 연다.
    std::function<std::string()> currentFortPath; // 00460680의 현재 Fort 파일 경로다.
    std::function<void(std::uint32_t,std::string_view)> loadFort; // 004bf550: 지정 슬롯의 실제 파일 읽기다.
    std::function<std::span<std::uint8_t>(std::uint32_t)> core; // 004bd550: 현재 슬롯의 State 바이트를 반환한다.
    std::function<double()> gameSeconds; // 원본0055b4d0의 현재 double 게임 시각이다.
    std::function<std::int32_t(std::uint32_t)> ownedIslands; // 0046f430: 현재 owner의 실제 섬 수다.
    std::function<void(std::uint32_t)> notify; // 004bd360: 마지막 현재 슬롯의 실제 변경 통지다.
};
// 00460bb0. 첫 NUL 앞의 점을 진단하고, 진단이 복귀하면 원본처럼 그대로 복사한다.
void SetPendingFortName(std::string& destination,std::string_view source,const std::function<void()>& assertion={});
// 004b3550. 다른 필드와 기존 요청을 검사하지 않고 같은 next DWORD만6으로 바꾼다.
void RequestFortRead(std::uint32_t& nextState);
// 004b3560. 먼저 next1을 쓴 뒤 선택된 전투 Fort 보존 효과를 원본 순서로 수행한다.
void RequestFortEdit(FortSelectionGlobals globals,const FortSelectionHooks& hooks);
// 004d7408/004d745f. Guide/Gump에서 소비하지 않은 사건만 전달한다. 처리한 명령은 true다.
bool DispatchFortSelectionCommand(const InputEvent& event,FortSelectionGlobals globals,const FortSelectionHooks& hooks);
// 원본 표의 두 이름만 대소문자 없이 조회한다. 없는 이름은0xffffffff다.
std::uint32_t FortSelectionScriptCode(std::string_view name);
// 공통 원본 인자 parser/80칸 문자열 고리/40칸 입력 큐로 선택 명령을 보낸다.
bool QueueFortSelectionAction(std::string_view name,std::string_view argument,InputQueue& queue,ScriptStringRing& strings);
}
