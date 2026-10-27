// 실제10.78 메뉴가읽는명령전역의초기ID와스크립트이름을현재입력큐에연결한다.
#pragma once
#include "client/InputEvent.h"
#include <cstdint>
#include <optional>
#include <string_view>
namespace netstorm::client {
std::optional<std::uint32_t> GameMenuCommandValue(std::uint32_t address); // 확인한원본전역주소만현재제품의동일명령ID로해석한다.
std::uint32_t GameMenuScriptCode(std::string_view name); // 원본Tell하위의실제명령이름만조회하고미등록은ffffffff다.
bool IsGameMenuCommand(std::uint32_t code); // 상위입력표시를제거한실제메뉴/하위명령인지검사한다.
bool QueuePlayerListKey(const InputEvent& event,InputQueue& queue); // 원본004d95ce의 F9/Shift+F9를 Player List/정지 명령으로 예약하며 뗌과 Ctrl은 거부한다.
}
