// 원문 일반 입력과 HTML 메타의 동일 고리/명령 큐를 실제 링크 소비 경계에 대조한다.
#include "TestSupport.h"
#include "client/UrlInput.h"
#include "client/GameMenuCommands.h"
#include "client/ScriptCommand.h"
#include "client/DialogExitCommand.h"
#include <bit>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace netstorm::client; // 같은 제품 입력/이름/문자열 고리를 사용한다.

// 두번째 DWORD의 전체비트와뗌·수정키·모드독립을 원문96입력에 직접 대조한다.
TEST_CASE(UrlInput_Native96WholeSecondParameterAndRelease) {
    std::ifstream input(NETSTORM_URL_INPUT_FIXTURE);CHECK(static_cast<bool>(input));std::string line;std::size_t count{};
    std::getline(input,line);
    while (std::getline(input,line)) { // 각 원문 관측 행에서 앞네열만 입력으로 사용한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();std::istringstream fields(line);std::vector<std::string> row;std::string field;
        while (std::getline(fields,field,'\t')) row.push_back(field); // 빈 문자열을 임의 숫자로 바꾸지 않는다.
        CHECK(row.size()==7);if (row.size()!=7) return;
        const InputEvent event{static_cast<std::uint32_t>(std::stoul(row[0])),std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(std::stoul(row[1]))),std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(std::stoul(row[2])))};
        std::string trace;std::uint32_t resolved{};const auto mode=std::stoul(row[3]);CHECK(mode<4); // 원문의Editor/missionPost 입력은이소비자가조회하지않는다.
        CHECK(DispatchUrlInput(event,[&](auto token) { ++resolved;trace="url:"+std::to_string(token);return std::string_view("observed"); },[](auto text) { CHECK(text=="observed"); }));
        CHECK((trace.empty() ? "-" : trace)==row[4]);CHECK(resolved==((event.code&InputCode::kRelease) ? 0U:1U));CHECK(row[5]=="1" && row[6]=="0");++count;
    }
    CHECK(count==96);
}

// 실제 이름/메타 resolver/문자열 고리/큐를 통과하며 미등록 오자와 도달한 장치 실패를 구별한다.
TEST_CASE(UrlInput_MetadataNameQueueAndCurrentStringLifetime) {
    CHECK(GameMenuScriptCode("uRl")==kUrlCommand && GameMenuCommandValue(0x542864)==kUrlCommand && IsGameMenuCommand(kUrlCommand));CHECK(GameMenuScriptCode(" URL")==0xffffffffU);
    InputQueue queue;ScriptStringRing strings;DialogExitCommandState meta;ResolveDialogExitCommand(meta,"URL","file:///C:/Netstorm/image1.gif",MakeDialogExitResolveHooks(strings,{GameMenuScriptCode,{}}));DisposeDialogExitCommand(meta,queue);
    const auto event=queue.Pop(false);std::string activated;
    CHECK(DispatchUrlInput(event,[&](auto token) { return strings.Resolve(token); },[&](auto text) { activated=text; }));CHECK(activated=="file:///C:/Netstorm/image1.gif" && meta.command==0 && event.x==event.y);
    CHECK(DispatchUrlInput({kUrlCommand|InputCode::kRelease,0,0},{},{}));CHECK(!DispatchUrlInput({0x02000089U,0,0},{},{}));
    bool failed{};try { static_cast<void>(DispatchUrlInput({kUrlCommand,0,0},{},{})); }catch (const std::logic_error&) { failed=true; }CHECK(failed);
    const auto oldToken=strings.Store("old");for (std::size_t index=0;index<80;++index) strings.Store("replacement"); // 현재고리가반납한토큰을새URL로잘못해석하지않아야한다.
    failed=false;try { static_cast<void>(DispatchUrlInput({kUrlCommand,0,std::bit_cast<std::int32_t>(oldToken)},[&](auto token) { return strings.Resolve(token); },[&](auto) { activated="invalid"; })); }catch (const std::out_of_range&) { failed=true; }CHECK(failed && activated!="invalid");
}
