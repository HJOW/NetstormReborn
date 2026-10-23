// 원문004b743b부터 공통 State 확정 직전까지의 기존 Editor UI 순서만 조립한다.
#pragma once
#include "client/ClientLocalStateGlobals.h"
#include "client/InputEvent.h"
#include <functional>
#include <string_view>

namespace netstorm::client {
// 실제 Client/Gump/음향/palette/섬 목록의 도달한 소비자만 요구한다.
struct ExistingEditorAfterCheckpointEffects {
    std::function<void(std::uint32_t)> hud; //같은 GumpInput에 현재명령/1/0/includeHidden1을 보낸다. Broadcast로 대체하지 않는다.
    std::function<double()> wallSeconds; //Client.WallSeconds의 매호출 timeGetTime 경계다.
    std::function<void(std::string_view)> requestMusic; //ClientAudio.RequestMusic의 같은곡/결과곡 보호 관문을 유지한다.
    std::function<void()> fortPalette; //실제 PathSpec("fortPal",0,0,0)→004b6100 전체 부모다. arg2=0 장치만으로 성공시키지 않는다.
    std::function<std::uint32_t()> islandCount; //현재 RawIslandRuntime.Count이며 벡터/미니맵 수로 치환하지 않는다.
    std::function<void()> battleOver; //앞선 실제 하위가 current를6으로 바꾼 경우의 미복원 분기 인계다. 복귀해도 이 한정 경로는 거부한다.
};
// 외곽의 ESI1/EBP0와 앞선 checkpoint 정상복귀를 전제한다. State commit/Poll/loaded 쓰기는 하지 않는다.
void ApplyExistingEditorAfterCheckpoint(ClientLocalStateGlobals& globals,InputQueue& input,const ExistingEditorAfterCheckpointEffects& effects);
}
