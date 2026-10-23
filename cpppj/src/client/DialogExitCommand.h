// 원본HTML부모의OnExit를메타해석시고정하고실제반납시동일입력큐에한번예약한다.
#pragma once
#include "client/ScriptCommand.h"
#include <functional>
#include <string_view>

namespace netstorm::client {
// HTML부모+21e0/+21e4의해석된명령과인자다.문자열인자는기존80칸고리의토큰이다.
struct DialogExitCommandState {
    std::uint32_t command{},parameter{}; // command0은소멸시예약하지않으며ffffffff는별도거부하지않는다.
};
// 메타는명령전체토큰을다시자르지않고이미얻은regex캡처원문을조회한다.
struct DialogExitResolveHooks {
    ScriptCommandHooks commands; // 현재d52c0조회와code0의진단이며code-1에추가진단하지않는다.
    std::function<std::uint32_t(std::string_view)> storeText; // 현재c7910의80칸문자열고리에즉시소유복사한다.
};
// 원본c83b0의lookup→진단0→첫BYTEisdigit→atol/문자열고리→두필드저장순서다.
// 같은페이지에서여러번부르면마지막해석결과가남는다.따옴표/공백/쉼표를제거하지않는다.
void ResolveDialogExitCommand(DialogExitCommandState& state,std::string_view command,std::string_view argument,const DialogExitResolveHooks& hooks);
// 실제페이지가소멸하는한수명에서만부른다.먼저보유값을비운뒤queue(code,param,param)한다.
void DisposeDialogExitCommand(DialogExitCommandState& state,const std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)>& queue);
// 이미검증된실제ScriptStringRing을빌리며수명을이helper의반환값보다길게유지해야한다.
DialogExitResolveHooks MakeDialogExitResolveHooks(ScriptStringRing& strings,ScriptCommandHooks commands);
// 같은40칸InputQueue의실제예약입구이며별도페이지전환큐를만들지않는다.
void DisposeDialogExitCommand(DialogExitCommandState& state,InputQueue& queue);
}
