// 실제원문해석/OnExit저장/HTML부모소멸관측과같은필드/효과를비교한다.
#include "RawSceneSupport.h"
#include "client/DialogExitCommand.h"
#include <bit>
#include <stdexcept>
#include <utility>
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::uint32_t kStringToken=0x12000000U; // 새독립원문고리의첫실제할당주소와같은명시토큰이다.
// 원문16진수캡처바이트를첫NUL의해석과분리하여읽는다.
std::string DecodeText(std::string_view hex) {
    if (hex=="-") return {};
    std::string result;
    for (std::size_t index=0;index<hex.size();index+=2) result.push_back(static_cast<char>(std::stoul(std::string(hex.substr(index,2)),nullptr,16))); // 입력바이트만복원한다.
    return result;
}
// 해석된필드의C문자열바이트를원문효과로그와같은16진수로직렬화한다.
std::string TextHex(std::string_view value) { return Hex({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()}); }
// 새모듈이재사용하는숫자CRT내부관측만빼고외부효과를원래순서로비교한다.
std::string ExternalTrace(std::string_view source) {
    std::string result;
    for (const auto& item:Split(source,';')) if (!item.starts_with("Digit,") && !item.starts_with("Atol,")) { // 기대숫자/인자/예약값은계산하거나바꾸지않는다.
        if (!result.empty()) result+=';';result+=item;
    }
    return result;
}
}
// regex캡처뒤실제원문204입력×두정밀도에서같았던필드와소멸예약순서를대조한다.
TEST_CASE(DialogExitCommand_Native204) {
    std::size_t count{}; // 실제fixture비교행수다.
    for (const auto& row:LoadFixture(NETSTORM_DIALOG_EXIT_COMMAND_FIXTURE).rows) { // 기대원문은실행중재작성하지않는다.
        CHECK(row.size()==8);if (row.size()!=8) return;
        const auto command=DecodeText(row[0]),argument=DecodeText(row[1]);const bool configZero=Number(row[2])!=0;
        DialogExitCommandState state;DialogExitResolveHooks hooks;std::string trace;std::uint32_t ring{},queued{}; // 실제외부입력과관찰효과다.
        const auto event=[&](std::string text) { if (!trace.empty()) trace+=';';trace+=text; }; // 같은외부순서만기록한다.
        hooks.commands.lookup=[&](std::string_view name) {
            event("Lookup,"+TextHex(name));
            if (name=="QuitApp") return 0x02000001U;if (name=="Tell") return 0x02000095U;
            if (name=="Config") return configZero ? 0U : 0x0200006dU;if (name=="Pause") return 0x020000cfU;return 0xffffffffU;
        };
        hooks.commands.unknown=[&](std::string_view name) { event("Diagnostic,"+TextHex(name)); };
        hooks.storeText=[&](std::string_view text) { event("String,"+TextHex(text));++ring;return kStringToken; };
        ResolveDialogExitCommand(state,command,argument,hooks);
        CHECK(state.command==Number(row[3]) && state.parameter==Number(row[4]) && ring==Number(row[5]));
        DisposeDialogExitCommand(state,[&](auto code,auto first,auto second) { ++queued;event("Queue,"+std::to_string(code)+','+std::to_string(first)+','+std::to_string(second)); });
        CHECK(!state.command && !state.parameter && queued==Number(row[6]));CHECK(trace==ExternalTrace(row[7]));++count;
    }
    CHECK(count==204);
}
// 실제80칸고리/같은입력큐를공유하고반납callback의재진입과중복호출을한번으로제한한다.
TEST_CASE(DialogExitCommand_CurrentRingQueueAndSingleDisposal) {
    ScriptStringRing strings;InputQueue queue;DialogExitCommandState state;std::uint32_t lookups{};
    const auto hooks=MakeDialogExitResolveHooks(strings,{[&](std::string_view name) { ++lookups;CHECK(name=="Tell");return 0x02000095U; },{}});
    ResolveDialogExitCommand(state,"Tell","Blank",hooks);const auto first=state.parameter;CHECK(strings.Resolve(first)=="Blank" && strings.Index()==1 && queue.Empty());
    ResolveDialogExitCommand(state,"Tell","Next",hooks);CHECK(lookups==2 && strings.Index()==2 && strings.Resolve(state.parameter)=="Next" && queue.Empty());
    const auto current=state.parameter;std::uint32_t callbacks{};
    const std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> reserved=[&](auto code,auto a,auto b) {
        ++callbacks;CHECK(!state.command && !state.parameter);DisposeDialogExitCommand(state,queue);queue.PushCommand(code,std::bit_cast<std::int32_t>(a),std::bit_cast<std::int32_t>(b));
    };
    DisposeDialogExitCommand(state,reserved);DisposeDialogExitCommand(state,reserved);CHECK(callbacks==1 && strings.Index()==2);
    const auto event=queue.Pop(false);CHECK(event.code==0x02000095U && std::bit_cast<std::uint32_t>(event.x)==current && std::bit_cast<std::uint32_t>(event.y)==current && queue.Empty());
    CHECK(strings.Resolve(first)=="Blank" && strings.Resolve(current)=="Next");
}
// 미선택장치와지역버퍼범위를암묵성공으로대체하지않으며code0도실제문자열해석은수행한다.
TEST_CASE(DialogExitCommand_MissingAndPhysicalBoundaries) {
    DialogExitCommandState state;DialogExitResolveHooks hooks;bool missing{};
    try { ResolveDialogExitCommand(state,"Tell","Blank",hooks); } catch (const std::invalid_argument&) { missing=true; }CHECK(missing);
    hooks.commands.lookup=[](std::string_view) { return 0x02000095U; };missing=false;
    try { ResolveDialogExitCommand(state,"Tell","Blank",hooks); } catch (const std::logic_error&) { missing=true; }CHECK(missing);
    hooks.storeText=[](std::string_view) { return 1234U; };ResolveDialogExitCommand(state,"Tell","Blank",hooks);missing=false;
    try { DisposeDialogExitCommand(state,std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)>{}); } catch (const std::logic_error&) { missing=true; }CHECK(missing && state.command==0x02000095U);
    for (const auto& input:{std::pair<std::string,std::string>{std::string(80,'C'),"0"},{"Tell",std::string(256,'A')}}) { // 원문의한바이트밖NUL쓰기를호스트에재현하지않는다.
        bool oversized{};try { ResolveDialogExitCommand(state,input.first,input.second,hooks); } catch (const std::length_error&) { oversized=true; }CHECK(oversized);
    }
}
