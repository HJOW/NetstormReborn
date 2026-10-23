#include "client/DialogExitCommand.h"
#include "o/RawTextToken.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
constexpr std::size_t kCommandBuffer=80,kArgumentBuffer=256; // 원문regex캡처의지역명령/인자물리버퍼다.
// 원문C문자열의첫NUL뒤는버리고한바이트넘치는원본NUL쓰기는호스트에서명시거부한다.
std::string_view Captured(std::string_view source,std::size_t capacity) {
    source=source.substr(0,source.find('\0'));
    if (source.size()>=capacity) throw std::length_error("OnExit 메타캡처의원본지역버퍼NUL범위초과");
    return source;
}
}
// 원문은메타를읽을때문자열고리를즉시진행한다.소멸시다시해석하면고리수명이달라진다.
void ResolveDialogExitCommand(DialogExitCommandState& state,std::string_view command,std::string_view argument,const DialogExitResolveHooks& hooks) {
    command=Captured(command,kCommandBuffer);argument=Captured(argument,kArgumentBuffer);
    if (!hooks.commands.lookup) throw std::invalid_argument("OnExit 현재명령표조회누락");
    const auto code=hooks.commands.lookup(command); // native4c85da의현재명령표결과다.
    if (!code) {
        if (!hooks.commands.unknown) throw std::logic_error("OnExit code0의원본진단미연결");
        hooks.commands.unknown(command);
    }
    std::uint32_t parameter{}; // native4c863c가보관할숫자또는현재고리토큰이다.
    if (!argument.empty() && argument.front()>='0' && argument.front()<='9') parameter=std::bit_cast<std::uint32_t>(o::ConfigParseLong(argument));
    else {
        if (!hooks.storeText) throw std::logic_error("OnExit 현재문자열고리미연결");
        parameter=hooks.storeText(argument);
    }
    state={code,parameter}; // native4c8a58/5e처럼모든조회/변환이끝난뒤두필드를교체한다.
}
// 실제HTML부모는한번만소멸한다.호스트페이지반납의재진입에도같은예약을반복하지않는다.
void DisposeDialogExitCommand(DialogExitCommandState& state,const std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)>& queue) {
    if (state.command && !queue) throw std::logic_error("OnExit 실제입력큐미연결");
    const auto current=std::exchange(state,DialogExitCommandState{}); // callback이페이지수명을다시정리하기전에먼저반납한다.
    if (current.command) queue(current.command,current.parameter,current.parameter);
}
// 조회/진단의정책은현재호출자가공급하고기존문자열고리만그대로빌린다.
DialogExitResolveHooks MakeDialogExitResolveHooks(ScriptStringRing& strings,ScriptCommandHooks commands) {
    return {std::move(commands),[&strings](std::string_view text) { return strings.Store(text); }};
}
// 원문52f90의DWORD두인자를같은기존호스트입력큐에비트보존하여예약한다.
void DisposeDialogExitCommand(DialogExitCommandState& state,InputQueue& queue) {
    DisposeDialogExitCommand(state,[&queue](auto command,auto first,auto second) {
        queue.PushCommand(command,std::bit_cast<std::int32_t>(first),std::bit_cast<std::int32_t>(second));
    });
}
}
