// 제품 작성 전에 동결한 원본150행과 실제 요청/이력/공용 팝업 조합을 대조한다. 현재 실행은 이월한다.
#include "GumpRawSupport.h"
#include "client/QuickHelpControl.h"
#include "client/QuickHelpRequest.h"
using namespace netstorm::client;
using namespace netstorm::test::gumpraw;
namespace {
constexpr std::uint32_t kTopic=306208768,kOther=306212864; // 독립 원문이 문자열 입력으로 사용한 격리 주소다.
// 문자열의 실제 바이트만 직렬화한다. 입력/기대값 계산에는 관여하지 않는다.
std::string Text(std::string_view value) { return Hex({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()}); }
// fixture의16진수 문자열을 NUL 포함 원문 그대로 복구한다.
std::string String(const std::string& value) { const auto bytes=Bytes(value);return {bytes.begin(),bytes.end()}; }
// 전체 trace에서 실제 callback 순서를 기록한다.
void Event(std::string& trace,std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
// 실제 이력의 현재 머리부터 읽는다. 빈 문자열 노드도 s로 남긴다.
std::string History(const QuickHelpHistory& history) {
    std::string result;const auto topics=history.Topics();
    // 제품이 보유한 문자열을 역순으로 읽으며 새 기대 이력을 만들지 않는다.
    for (auto i=topics.size();i>0;--i) { if (!result.empty()) result+='/';result+='s'+Text(topics[i-1]); }
    return result.empty() ? "-" : result;
}
// 원본 사건의 세 DWORD 비트를 signed 좌표 칸에 보존한다.
InputEvent Input(std::span<const std::uint32_t> values) { return {values[0],std::bit_cast<std::int32_t>(values[1]),std::bit_cast<std::int32_t>(values[2])}; }
// GUI/창/링크 효과는 native와 같은 명시 입력이며 실제 분배·이력은 제품이 수행한다.
struct EventScene {
    QuickHelpWindowNode window;std::array<GumpCreateNode,3> buttons;QuickHelpHistory history;QuickHelpPopupState popup; // 원본 노드/현재 이력/공용 표식이다.
    std::vector<std::uint32_t> row;std::string trace; // 독립 사건 입력과 관측 효과다.
    // 실제 버튼 serial/현재 페이지 높이/이력만 공급한다.
    EventScene(const std::vector<std::uint32_t>& values,bool zero):row(values) {
        window.width=450;window.height=350;window.cancelButton=1;window.learnButton=2;window.backButton=3;window.learnType=zero ? 0U : 71U;
        // serial과 ID가 다름을 유지해 잘못된 비교를 검출한다.
        for (std::size_t i=0;i<buttons.size();++i) { buttons[i].serial=static_cast<std::uint32_t>(81+i);buttons[i].id=static_cast<std::uint32_t>(901+i); }
        if (row[6]>0) history.Push("firstHelp");if (row[6]>1) history.Push("secondHelp");popup.Mark(row[7],"before");
    }
    // 유효한 문자열 입력만 해석하며 원본 포인터를 호스트에서 역참조하지 않는다.
    static std::string Resolve(std::uint32_t token) { CHECK(token==kTopic || token==kOther || token==256);return token==kTopic ? "themeHelp" : token==kOther ? "otherHelp" : "lowHelp"; }
    // 실제 요청/링크/학습·장치의 경계 호출 순서를 native와 같은 형식으로 기록한다.
    QuickHelpEventHooks Hooks() {
        QuickHelpEventHooks h;h.local=[this](InputEvent) { Event(trace,"Local");return std::array<float,2>{Float(row[3]),Float(row[4])}; };
        h.node=[this](auto token) { CHECK(token>=1 && token<=3);return &buttons.at(token-1); };
        h.typeName=[](auto type) { return "Unit"+std::to_string(type); };h.translate=[](std::string_view value) { CHECK(value=="Type");return std::string(value); };h.text=Resolve;
        h.request=[this](auto type,std::string_view topic) { Event(trace,"Request,"+std::to_string(type)+','+Text(topic)); };
        h.pageHeight=[this] { return Float(row[5]); };h.scroll=[this](auto delta) { Event(trace,"Scroll,"+std::to_string(std::bit_cast<std::uint32_t>(delta))); };
        h.link=[this](std::string_view topic) { Event(trace,"Link,"+Text(topic)); };
        h.kill=[this] { Event(trace,"Kill");if (row[8]) popup.Mark(0x2000abc,"changed"); };
        h.learn=[this](auto type) { Event(trace,"Learn,"+std::to_string(type)); };
        h.queue=[this](auto code,std::string_view first,std::string_view second) { CHECK(first.data()==second.data());Event(trace,"Queue,"+std::to_string(code)+','+Text(first)+','+Text(second)); };
        h.base=[this](InputEvent) { Event(trace,"Base");return 7U; };return h;
    }
};
// 실제 열기 관문에 원본과 같은 현재 등록/모달·생성·부착 경계만 공급한다.
std::string Open(const std::vector<std::uint32_t>& row) {
    GumpCreateNode window;window.depth=120;window.baseWidth=450;std::string trace;const QuickHelpOpenState state{13,14,0,16,800};QuickHelpOpenHooks h;
    h.closePopups=[&] { Event(trace,"ClosePopups"); };h.lookup=[&](auto id) { return id==13 ? row[0] : 2U; };
    h.noModal=[&] { return row[1]==0; };h.currentModal=[&] { return row[1]; };h.construct=[&](auto type) { Event(trace,"Construct,"+std::to_string(type));return 1U; };
    h.node=[&](auto token) { CHECK(token==1);return &window; };
    h.attach=[&](auto token,float x,float y,auto parent,auto mode) { CHECK(token==1);Event(trace,"Attach,"+std::to_string(Bits(x))+','+std::to_string(Bits(y))+','+std::to_string(parent)+','+std::to_string(mode)); };
    h.addDepth=[&](auto token,auto delta) { CHECK(token==1);window.depth+=delta; };
    h.modal=[&](auto token,auto mode) { CHECK(token==1);Event(trace,"Modal,"+std::to_string(mode)); };
    h.handle=[&](auto token,auto code,auto first,auto second) { CHECK(token==1 && second==kTopic);Event(trace,"Handle,"+std::to_string(code)+','+std::to_string(first)+','+Text("themeHelp"));return 1U; };
    const auto result=OpenQuickHelpWindow(state,h,kTopic,static_cast<std::int32_t>(row[2]),71);return std::to_string(result)+'|'+std::to_string(window.depth)+'|'+(trace.empty() ? "-" : trace);
}
}
// 전체150행의 창 반환·깊이·이력·첫NUL·현재 popup 바이트와 호출 순서를 대조한다.
TEST_CASE(QuickHelpControl_Native150) {
    std::size_t count{}; // 실제 대조한 원문 행 수다.
    // 각 행은 독립 저장소로 실행하고 원본 기대값을 변경하지 않는다.
    for (const auto& row:netstorm::test::rawscene::LoadFixture(NETSTORM_QUICK_HELP_CONTROL_FIXTURE).rows) {
        CHECK(row.size()==3);if (row.size()!=3) return;std::string result;
        if (row[0]=="O") result=Open(Inputs(row[1]));
        else if (row[0]=="H") {
            const auto input=Split(row[1],',');QuickHelpHistory history;
            // native에 공급한 중복/NUL 포함 문자열을 같은 순서로 준다.
            for (const auto& name:Split(String(input[0]),';')) history.Push(name);
            std::string returns;
            // Back 반환은 실제 제품 이력에서 읽으며 제거 결과를 별도 계산하지 않는다.
            for (std::uint32_t i=0;i<Number(input[1]);++i) { const auto value=history.Back();if (i) returns+=',';returns+=value ? 's'+Text(*value) : "n"; }
            result=History(history)+'|'+returns;
        } else if (row[0]=="P") {
            const auto input=Split(row[1],',');QuickHelpPopupState popup;popup.Mark(0,"before");const auto topic=String(input[1]);
            popup.Mark(0x20000a0,Number(input[0]) ? std::optional<std::string_view>(topic) : std::nullopt);result=std::to_string(popup.command)+'|'+Hex({reinterpret_cast<const std::uint8_t*>(popup.argument.data()),popup.argument.size()});
        } else {
            CHECK(row[0]=="E" || row[0]=="Z");EventScene scene(Inputs(row[1]),row[0]=="Z");const auto value=DispatchQuickHelpWindow(scene.window,Input(scene.row),scene.history,scene.popup,scene.Hooks());result=std::to_string(value)+'|'+History(scene.history)+'|'+(scene.trace.empty() ? "-" : scene.trace);
        }
        if (result!=row[2]) std::printf("Quick Help 제어 원문 차이 %zu %s\n%s\n",count,row[1].c_str(),result.c_str());CHECK(result==row[2]);++count;
    }
    CHECK(count==150);
}
// 실제 요청의 접미/조회→같은 이력→현재 flags→Push/부착과 Back의 현재 popup 예약을 연결한다.
TEST_CASE(QuickHelpControl_ActualRequestHistoryAndPopup) {
    QuickHelpWindowNode window;window.width=450;window.height=350;window.backButton=1;QuickHelpHistory history;QuickHelpPopupState popup;GumpCreateNode back;back.serial=83;std::vector<std::string> shown;std::uint32_t killed{},pushed{},attached{};InputQueue queue;
    QuickHelpRequestHooks requestHooks;requestHooks.lookup=[](std::string_view name) { CHECK(name=="theme" || name=="sunCannon");return name=="theme" ? 0 : 71; };
    requestHooks.print=[](std::string_view pattern,std::string_view value) { CHECK(pattern=="%s");return std::string(value); };
    requestHooks.history=[&](std::string_view topic) { history.Push(topic); };requestHooks.show=[&](auto type,std::string_view topic) { CHECK(type==0 || type==71);shown.emplace_back(topic); };
    requestHooks.push=[&] { ++pushed;window.flags|=8;window.x=51;window.y=73; };requestHooks.attach=[&](float x,float y,auto parent,auto mode) { ++attached;CHECK(x==51 && y==73 && parent==0 && mode==0); };
    QuickHelpRequest request(window,std::move(requestHooks));QuickHelpEventHooks h;h.local=[](InputEvent) { return std::array<float,2>{1,1}; };h.node=[&](auto token) { CHECK(token==1);return &back; };
    h.text=[](auto token) { CHECK(token==kTopic);return std::string("themeHelp"); };h.typeName=[](auto type) { CHECK(type==71);return std::string("sunCannon"); };h.translate=[](std::string_view text) { return std::string(text); };h.request=[&](auto type,std::string_view topic) { request.Open(type,topic); };
    h.link=[&](std::string_view topic) { request.Open(0,topic); };h.kill=[&] { ++killed;popup.Mark(0x2000abc,"afterKill"); };
    h.queue=[&](auto code,std::string_view first,std::string_view second) { CHECK(first=="afterKill" && first.data()==second.data());queue.Push(code,1,1); };
    popup.Mark(0x20000a0);CHECK(DispatchQuickHelpWindow(window,{0x2000017,0,static_cast<std::int32_t>(kTopic)},history,popup,h)==1);
    CHECK(DispatchQuickHelpWindow(window,{0x2000017,0,71},history,popup,h)==1);CHECK(history.Topics().size()==2 && shown==std::vector<std::string>({"themeHelp","sunCannonType"}));CHECK(pushed==1 && attached==1);
    CHECK(DispatchQuickHelpWindow(window,{0x200002c,83,0},history,popup,h)==1);CHECK(history.Topics().size()==1 && shown.back()=="themeHelp" && killed==0);
    CHECK(DispatchQuickHelpWindow(window,{0x200002c,83,0},history,popup,h)==1);CHECK(history.Topics().empty() && killed==1);const auto event=queue.Pop(false);CHECK(event.code==0x20000a0 && event.x==event.y && queue.Empty());
}
