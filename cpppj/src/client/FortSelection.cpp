// 입력 명령에서 실제 Editor 설치를 앞당기지 않고 원본 pending/next 예약을 보존한다.
#include "client/FortSelection.h"
#include "o/OriginalText.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 선택된 효과가 없으면 앞선 원본 쓰기를 유지한 채 해당 경계에서 실패한다.
template<class Function> const Function& Need(const Function& function) {
    if (!function) throw std::logic_error("Fort 선택의 실제 효과 미연결");return function;
}
// 원본 State의 필요한16BYTE 범위를 확인한다. 미정 바이트를 생성하거나 초기화하지 않는다.
std::span<std::uint8_t> Core(const FortSelectionHooks& hooks,std::uint32_t slot) {
    auto bytes=Need(hooks.core)(slot);if (bytes.size()<16) throw std::out_of_range("Fort 선택의 현재 State 크기 부족");return bytes;
}
// CRT _ftol은 signed64 절삭의 하위 DWORD다. 무효 변환의 indefinite 하위 DWORD도0이다.
std::uint32_t TimeInteger(double value) {
    if (!std::isfinite(value) || value>=9223372036854775808.0 || value< -9223372036854775808.0) return 0;
    return static_cast<std::uint32_t>(static_cast<std::int64_t>(value));
}
}
// 진단 후의 복사를 보존하며 원본 C 문자열의 NUL 뒤 바이트는 이름에 포함하지 않는다.
void SetPendingFortName(std::string& destination,std::string_view source,const std::function<void()>& assertion) {
    source=source.substr(0,source.find('\0'));if (source.find('.')!=source.npos) Need(assertion)();destination=source;
}
// 현재 NUL/점 검사 뒤 같은 주소에서 전방 BYTE 복사를 수행한다. 목적/원본 겹침도 별도 문자열로 바꾸지 않는다.
void SetPendingFortName(std::span<std::uint8_t> destination,std::span<const std::uint8_t> source,const std::function<void()>& assertion) {
    std::size_t end{};for (;;++end) { // 원문 strchr은 첫 점 또는 NUL까지만 읽는다.
        if (end>=source.size()) throw std::out_of_range("Fort pending 이름 검사 NUL 범위");if (!source[end]) break;if (source[end]=='.') { Need(assertion)();break; }
    }
    for (std::size_t index=0;;++index) { // 진단이 바꾼 현재 source에서 한BYTE씩 읽고 같은 목적 버퍼에 쓴다.
        if (index>=source.size() || index>=destination.size()) throw std::out_of_range("Fort pending 이름 복사 물리 범위");const auto byte=source[index];destination[index]=byte;if (!byte) return;
    }
}
// 큐의 선착순 관문 없이 원본 nextState DWORD만 덮어쓴다.
void RequestFortRead(std::uint32_t& nextState) { nextState=6; }
// 파일 읽기/각 State 조회/통지 사이에 templateOwner가 바뀔 수 있으므로 슬롯을 미리 고정하지 않는다.
void RequestFortEdit(FortSelectionGlobals globals,const FortSelectionHooks& hooks) {
    globals.nextState=1;
    if (!globals.inBattle || globals.currentState!=14) return;
    const auto path=Need(hooks.currentFortPath)(); // 경로 조회가 끝난 뒤 현재 슬롯으로 읽는다.
    Need(hooks.loadFort)(globals.templateOwner,path);
    Core(hooks,globals.templateOwner)[0]&=0xfdU;
    const auto stamp=TimeInteger(Need(hooks.gameSeconds)()); // 첫 State 수정 뒤 시각을 읽고 State를 다시 조회한다.
    auto bytes=Core(hooks,globals.templateOwner);
    for (std::size_t byte=0;byte<4;++byte) bytes[12+byte]=static_cast<std::uint8_t>(stamp>>(8*byte)); // +c DWORD만 little endian으로 기록한다.
    if (Need(hooks.ownedIslands)(globals.owner)<1) {
        Core(hooks,globals.templateOwner)[0]&=0xfbU;Need(hooks.notify)(globals.templateOwner);
    }
}
// 일반 입력 부모의 release 관문 뒤 상위 플래그를 제거한다. Control/Shift는 허용한다.
bool DispatchFortSelectionCommand(const InputEvent& event,FortSelectionGlobals globals,const FortSelectionHooks& hooks) {
    if (event.code&InputCode::kRelease) return false;
    const auto command=event.code&0x03ffffffU; // Guide/Gump가 이미 본 원래 사건의 명령 부분이다.
    if (command==kAskEditFortCommand) {
        globals.mode=2;const auto path=Need(hooks.lastEditPath)();
        Need(hooks.pending)(path);Need(hooks.refresh)(2);Need(hooks.tell)("LoadEditable");return true;
    }
    if (command==kReadFortCommand) {
        Need(hooks.pending)(Need(hooks.resolveString)(std::bit_cast<std::uint32_t>(event.y)));
        if (!globals.mode) RequestFortRead(globals.nextState);else RequestFortEdit(globals,hooks);return true;
    }
    return false;
}
// 맵 행과 일반 스크립트가 동일한 원본 명령 번호를 사용하도록 한다.
std::uint32_t FortSelectionScriptCode(std::string_view name) {
    const auto lower=o::AsciiLower(name); // 원본 명령 표의 stricmp 비교다.
    if (lower=="askeditfort") return kAskEditFortCommand;
    if (lower=="readfort") return kReadFortCommand;
    return 0xffffffffU;
}
// 이미 복원한 parser를 공유하므로 숫자 시작/음수/빈 인자의 수명을 새 규칙으로 바꾸지 않는다.
bool QueueFortSelectionAction(std::string_view name,std::string_view argument,InputQueue& queue,ScriptStringRing& strings) {
    if (FortSelectionScriptCode(name)==0xffffffffU) return false;
    QueueScriptCommand(std::string(name)+','+std::string(argument),queue,strings,{FortSelectionScriptCode,{}});return true;
}
}
