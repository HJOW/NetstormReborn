// 원문 언어 요청/현재 Config·큐와 기존 Chat 재부착을 창/월드/MP 없이 검사한다.
#include "TestSupport.h"
#include "client/LanguageRequest.h"
#include "client/ClientMain.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/GameMenuCommands.h"
#include "client/ScriptCommand.h"
#include "client/ChatWindow.h"
#include <bit>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
using namespace netstorm::client;
namespace { //원시 fixture의 실제 사건/trace만 읽는다.
const auto kLanguageRequestRoot=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path(); //보호 원본과 검사 출력의 루트다.
std::string LanguageRequestHex(std::string_view text) { //문자열과 큐를 같은 원시 hex로 기록한다.
    constexpr std::string_view kDigits="0123456789abcdef";std::string result; //각BYTE의 두 자리를 고정한다.
    for (const unsigned char byte:text) { result+=kDigits[byte>>4];result+=kDigits[byte&15U]; } //인코딩/대소문자/빈칸을 바꾸지 않는다.
    return result;
}
std::string LanguageRequestEvent(InputEvent event) { //실제 세DWORD를 little endian BYTE로 기록한다.
    std::string bytes;
    for (auto value:{event.code,std::bit_cast<std::uint32_t>(event.x),std::bit_cast<std::uint32_t>(event.y)}) for (std::uint32_t byte=0;byte<4;++byte) bytes+=static_cast<char>(value>>(8*byte)); //원문12BYTE 레이아웃이다.
    return LanguageRequestHex(bytes);
}
}
// 설정 callback 효과 뒤의 현재 명령/카운터와 실제 큐 writer 관측337개를 대조한다.
TEST_CASE(LanguageRequest_IndependentNative337) {
    std::ifstream file(kLanguageRequestRoot/"cpppj/tests/fixtures/language-request-x86.tsv");CHECK(file.good());std::string line;std::size_t count{};
    while (std::getline(file,line)) { //기대값은 제품 모델이 아닌 최초 원문 관측에서 읽는다.
        if (line.empty() || line.front()=='#') continue;std::istringstream row(line);std::uint32_t requested{},before{},refresh{},mutation{},currentAfter{},refreshAfter{};std::string event,wanted;row>>requested>>before>>refresh>>mutation>>currentAfter>>refreshAfter>>event>>wanted;CHECK(!row.fail());auto current=std::bit_cast<std::int32_t>(before);std::uint32_t changed=kLanguageChangedCommand;std::string trace;InputQueue queue;
        LanguageRequestHooks hooks;hooks.set=[&](auto key,auto name) { CHECK(key=="currentLanguage");trace="S,"+LanguageRequestHex(name)+','+std::to_string(refresh);if (mutation&1) { current=std::bit_cast<std::int32_t>(requested);refresh=100;changed=0xa2001234U; } };
        hooks.changed=[&] { return changed; };hooks.queue=[&](auto code,auto first,auto second) { trace+=";Q,"+std::to_string(code)+','+std::to_string(first)+','+std::to_string(second)+','+std::to_string(refresh);if (mutation&2) refresh=0xffffffffU;queue.PushCommand(code,std::bit_cast<std::int32_t>(first),std::bit_cast<std::int32_t>(second)); };
        RequestOriginalLanguage(requested,current,refresh,hooks);CHECK(std::bit_cast<std::uint32_t>(current)==currentAfter && refresh==refreshAfter && (trace.empty() ? "-" : trace)==wanted);const auto actual=queue.Empty() ? std::string("-") : LanguageRequestEvent(queue.Pop(false));CHECK(actual==event && queue.Empty());++count;
    }
    CHECK(count==337);
}
// 선택되지 않은 장치를 요구하지 않고 예외 때 앞선 효과를 취소하거나 뒤 카운터를 증가시키지 않는다.
TEST_CASE(LanguageRequest_NaturalReturnAndPartialFailures) {
    std::int32_t current=3;std::uint32_t refresh=7;RequestOriginalLanguage(3,current,refresh,{});CHECK(refresh==7);
    LanguageRequestHooks hooks;hooks.set=[&](auto,auto) { current=6;throw std::runtime_error("set"); };bool failed{};
    try { RequestOriginalLanguage(6,current,refresh,hooks); }catch (const std::runtime_error&) { failed=true; }CHECK(failed && current==6 && refresh==7);
    current=0;hooks.set=[&](auto,auto) { current=3; };hooks.changed=[] { return kLanguageChangedCommand; };hooks.queue=[&](auto,auto,auto) { refresh=91;throw std::runtime_error("queue"); };failed=false;
    try { RequestOriginalLanguage(3,current,refresh,hooks); }catch (const std::runtime_error&) { failed=true; }CHECK(failed && current==3 && refresh==91);
}
// 이름→실제 Script 큐→입력 부모→현재 Client/Config/번역·공유 카운터를 하나로 연결한다.
TEST_CASE(LanguageRequest_ActualClientNameQueueAndConfig) {
    CHECK(GameMenuScriptCode("SetLanguage")==kSetLanguageCommand && GameMenuScriptCode("lAnGuAgEcHaNgEd")==kLanguageChangedCommand && GameMenuCommandValue(0x542988U)==kSetLanguageCommand && GameMenuCommandValue(0x54298cU)==kLanguageChangedCommand);
    ClientOptions options;options.gameDirectory=kLanguageRequestRoot/"originals";options.noAudio=true;Client client(options);auto& config=client.Configuration();config.Configuration().AppendArguments("currentLanguage=english;LanguageSpec=\"d/language.{LOCAL.1}\"",{});client.InterpretLanguageOptions();client.InitializeNativeTranslations();auto& refresh=client.LocalStateGlobals()->MetaRefresh();const auto* address=&refresh;refresh=0xffffffffU;
    ScriptStringRing strings;QueueScriptCommand("SetLanguage,3",client.Input(),strings,{GameMenuScriptCode,{}});const auto event=client.Input().Pop(false);CHECK(DispatchSetLanguage(event,[&](auto requested) { client.SetLanguage(requested); }));CHECK(client.LanguageNumber()==3 && config.PathSpec("currentLanguage")=="german" && client.Translate("Loading")=="Ladevorgang" && refresh==0 && &client.LocalStateGlobals()->MetaRefresh()==address);
    const auto notification=client.Input().Pop(false);CHECK(notification.code==kLanguageChangedCommand && notification.x==0 && notification.y==0 && client.Input().Empty());client.SetLanguage(3);CHECK(refresh==0 && client.Input().Empty());client.SetLanguage(6);CHECK(client.Translate("Loading")=="los" && refresh==1 && client.Input().Pop(false).code==kLanguageChangedCommand);
    std::size_t calls{};CHECK(DispatchSetLanguage({kSetLanguageCommand|InputCode::kRelease,1,0},{}) && !DispatchSetLanguage({kLanguageChangedCommand,0,0},{}));CHECK(DispatchSetLanguage({kSetLanguageCommand|InputCode::kControl,-7,2},[&](auto value) { CHECK(value==2);++calls; }) && calls==1);
}
// 기존 Chat 부모의 알림은 release도 처리하고 Push 뒤 현재 좌표를 읽는다. 전체 GUI/MP 검사가 아니다.
TEST_CASE(LanguageRequest_ChangedRoutesOnceAndReattachesChat) {
    HudChatNode root;root.x=10;root.y=20;ChatWindowState state;state.current=1;std::size_t pushes{},attaches{},bases{};ChatWindowHooks hooks;hooks.chat=[&](auto token) { return token==1 ? &root : nullptr; };hooks.base=[&](auto token)->GumpCreateNode* { return token==1 ? &root : nullptr; };
    hooks.global=[](auto address)->std::uint32_t { switch (address) { case 0x542b24:return 0x02000115;case 0x5427a8:return 0x0200002c;case 0x5429f0:return 0x020000bf;case 0x5427d8:return 0x02000038;case 0x542794:return 0x02000027;case 0x542af8:return 0x02000101;case 0x54298c:return kLanguageChangedCommand;default:throw std::logic_error("언어 알림의 예상 밖 현재 전역"); } };
    hooks.push=[&](auto token) { CHECK(token==1);++pushes;root.x+=3;root.y+=5; };hooks.attach=[&](auto token,auto x,auto y,auto parent,auto mode) { CHECK(token==1 && x==root.x && y==root.y && parent==0 && mode==0);++attaches; };hooks.baseHandle=[&](auto,auto,auto,auto) { ++bases;return 0U; };ChatWindow chat(state,std::move(hooks));
    for (auto code:{kLanguageChangedCommand,kLanguageChangedCommand|InputCode::kRelease}) { //공통 Gump의 선행 전달 여부를 같은 실제 사건으로 구별한다.
        const InputEvent event{code,11,12};CHECK(DispatchLanguageChanged(event,false,[&](const auto& routed) { CHECK(routed.code==code && routed.x==11 && routed.y==12);CHECK(chat.Handle(1,routed.code,11,12)==0); }));CHECK(DispatchLanguageChanged(event,true,{}));
    }
    CHECK(pushes==2 && attaches==2 && bases==2 && root.x==16 && root.y==30);CHECK(!DispatchLanguageChanged({kSetLanguageCommand,0,0},false,{}));
}
