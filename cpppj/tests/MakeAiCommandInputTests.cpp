// 원문 MakeAi 선택입력/이름/FIFO와 지연관문을 실제 제품 공개 경계로 대조한다.
#include "RawSceneSupport.h"
#include "client/MakeAiCommandInput.h"
#include "client/GameMenuCommands.h"
#include "client/ScriptCommand.h"
#include <bit>
#include <filesystem>
using namespace netstorm::client; //같은실제명령값과입력큐를공유한다.
using namespace netstorm::test::rawscene; //최초fixture의숫자와BYTE해독만공유한다.

namespace { //검사자료는실제소스의고정fixture에서만읽는다.
FixtureData MakeInputRows(std::string_view name) { return LoadFixture((std::filesystem::path(__FILE__).parent_path()/"fixtures"/name).string().c_str()); } //실행CWD와무관하다.
std::string MakeInputText(std::string_view hex) { if (hex=="-") return {};std::string value;for (std::size_t i=0;i<hex.size();i+=2) value+=static_cast<char>(std::stoul(std::string(hex.substr(i,2)),nullptr,16));return value; } //원문BYTE/문자열고리를UTF8치환하지않는다.
}

TEST_CASE(MakeAiCommandInput_Native64ModeMaskReleaseAndDeferredReads) { //원문Editor OR missionPost만관문으로쓰며정지/World관문을추가하지않는다.
    std::size_t count{};
    for (const auto& row:MakeInputRows("make-ai-input-x86.tsv").rows) { //실제PE기계어의선택된call과전달전체DWORD를읽는다.
        CHECK(row.size()==6);if (row.size()!=6) continue;MakeAiCommandHooks h;h.editor=[&] { return Number(row[2]); };h.missionPost=[&] { return Number(row[3]); };std::string trace;h.makeAi=[&](auto owner) { trace="Make:"+std::to_string(owner); };
        CHECK(DispatchMakeAiCommandInput({Number(row[0]),37,std::bit_cast<std::int32_t>(Number(row[1]))},h));CHECK((trace.empty()?"-":trace)==row[4]);++count;
    }
    CHECK(count==64 && !DispatchMakeAiCommandInput({'M',0,0},{}));CHECK(DispatchMakeAiCommandInput({kMakeAiCommand|InputCode::kRelease,0,0},{}));
    MakeAiCommandHooks h;h.editor=[] { return 1U; };h.missionPost=[]()->std::uint32_t { throw std::logic_error("Editor비영뒤mission조회금지"); };std::size_t calls{};h.makeAi=[&](auto owner) { CHECK(owner==0xffffffffU);++calls;throw std::runtime_error("명시현재MakeAi실패"); };CHECK(Throws([&] { DispatchMakeAiCommandInput({kMakeAiCommand,9,-1},h); }) && calls==1);
}

TEST_CASE(MakeAiCommandInput_Native12NamesAndActualQueue) { //실제이름표/ASCII대소문자/숫자와문자열고리가같은일반큐에전달된다.
    std::size_t count{};
    for (const auto& row:MakeInputRows("make-ai-names-x86.tsv").rows) { //미등록이름도실제파서의진단/고리소비를유지한다.
        CHECK(row.size()==9);if (row.size()!=9) continue;InputQueue queue;ScriptStringRing ring;std::size_t diagnostics{};
        QueueScriptCommand(MakeInputText(row[1]),queue,ring,{[](auto name) { CHECK(GameMenuScriptCode(name)==HtmlScriptCommandValue(name));return GameMenuScriptCode(name); },[&](auto) { ++diagnostics; }});CHECK(ring.Index()==Number(row[7]));
        if (row[6]=="-") CHECK(queue.Empty() && diagnostics);else { const auto expected=Split(row[6],',');const auto event=queue.Pop(false);CHECK(event.code==Number(expected[0]) && event.x==event.y && queue.Empty() && IsGameMenuCommand(event.code));if (expected[1].starts_with('n')) CHECK(std::bit_cast<std::uint32_t>(event.x)==Number(expected[1].substr(1)));else CHECK(ring.Resolve(std::bit_cast<std::uint32_t>(event.x))==MakeInputText(expected[1].substr(1))); }++count;
    }
    CHECK(count==12 && GameMenuCommandValue(0x5429d0U)==kMakeAiCommand);
    CHECK(MakeAiCommandScriptCode(std::string_view("MakeAi\0ignored",14))==kMakeAiCommand && GameMenuScriptCode(std::string_view("MakeAi\0ignored",14))==kMakeAiCommand); //원문 __stricmp의 첫NUL 계약을 직접 공개 이름 조회에서도 보존한다.

}
