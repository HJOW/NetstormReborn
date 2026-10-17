// 실제 PE Highlight/ResetMouse 반환의 전체 전역과 명시 경계 인자를 재생한다.
#include "RawSceneSupport.h"
#include "client/RawUserHighlight.h"
#include <bit>
#include <stdexcept>
using namespace netstorm::test::rawscene;
using namespace netstorm::client;

namespace {
// fixture의 전체 DWORD 목록을 원본 십진수 순서대로 읽는다. 기대 분기를 계산하지 않는다.
std::vector<std::uint32_t> HighlightNumbers(std::string_view value) {
    std::vector<std::uint32_t> values; // 입력/출력 열의 원문 비트를 보관한다.
    // 필드 순서를 바꾸거나 signed 입력의 상위 비트를 버리지 않는다.
    for (const auto& field:Split(value,',')) values.push_back(Number(field));return values;
}
// 각 독립 입력에서 같은 현재 전역을 빌리는 실제 강조 모듈을 준비한다.
struct HighlightScene {
    std::uint32_t mode{},selected{},mouseType{}; //005cae90/005caea0/005caea4의 서로 다른 저장소다.
    std::int32_t x{},y{}; //005cae94/98의 전체 signed 정수 마우스 좌표다.
    std::vector<std::uint32_t> input; //operation/oldMode/newMode/selected/type/x/y/mutation/projectX/projectY다.
    std::string trace; //실제 경계의 인자와 호출 때의 TYPE 초기화 결과다.
    // 같은 저장소의 참조만 반환한다. 선택이나 화면 좌표를 별도로 복사하지 않는다.
    UserHighlightState State() { return {mode,selected,mouseType,x,y}; }
    // 전체 DWORD 인자와 실제 호출 순서를 원본 관찰의 TSV 셀 표현으로 보존한다.
    void Event(std::string_view letter,std::initializer_list<std::uint32_t> values) {
        if (!trace.empty()) trace+=';';trace+=letter;
        // 각 효과의 인자를 원문 unsigned 십진수로 기록한다.
        for (const auto value:values) trace+=':'+std::to_string(value);
    }
    // 명시 효과가 바꾸는 입력만 공급하며 원본 부모의 다음 읽기는 모듈이 실행한다.
    void Mutate(std::uint32_t selectedBoundary) {
        if (input[7]!=selectedBoundary) return;mode=7;selected=202;mouseType=77;x=-33;y=65;
    }
    // 실제 커서/역투영/이동 경계는 관찰과 같은 인자·반환·현재 상태 변경만 수행한다.
    UserHighlightHooks Hooks() {
        UserHighlightHooks hooks; // mode와 selected의 선택 판단은 실제 RawUserHighlight가 담당한다.
        hooks.selectCursor=[this](std::uint32_t cursor) { Event("K",{cursor,mouseType});Mutate(1); };
        hooks.project=[this](std::int32_t mouseX,std::int32_t mouseY) { Event("P",{std::bit_cast<std::uint32_t>(mouseX),std::bit_cast<std::uint32_t>(mouseY),mouseType});Mutate(2);return std::array{input[8],input[9]}; };
        hooks.move=[this](std::uint32_t sid,std::uint32_t pointX,std::uint32_t pointY,std::uint32_t force) { Event("M",{sid,pointX,pointY,force});Mutate(3); };
        return hooks;
    }
    // 현재 전역의 실제 다섯 DWORD를 원본 관찰과 같은 순서로 읽는다.
    std::vector<std::uint32_t> Values() const { return {mode,selected,mouseType,std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y)}; }
    // 이전 행의 callback 변경을 다음 독립 입력으로 넘기지 않는다.
    void Prepare(std::vector<std::uint32_t> values) {
        input=std::move(values);mode=input[1];selected=input[3];mouseType=input[4];x=std::bit_cast<std::int32_t>(input[5]);y=std::bit_cast<std::int32_t>(input[6]);trace.clear();
    }
};
}

// 기대 전역·순서·비트는 두 정밀도로 실행한 보호 PE의 정상 반환에서만 읽는다.
TEST_CASE(UserHighlight_NativeCurrentStateAndSelectedSnapshot) {
    HighlightScene scene;std::size_t count{}; // 현재 한 행의 상태와 독립 원본 입력 수다.
    // 같은 모드/모든DWORD/Reset/경계변경 입력을 처음부터 끝까지 재생한다.
    for (const auto& fields:LoadFixture(NETSTORM_USER_HIGHLIGHT_FIXTURE).rows) {
        CHECK(fields.size()==3);if (fields.size()!=3) continue;scene.Prepare(HighlightNumbers(fields[0]));CHECK(scene.input.size()==10);
        RawUserHighlight highlight(scene.State(),scene.Hooks()); // 같은 장기 전역을 직접 참조하는 실제 제품 모듈이다.
        if (scene.input[0]==0) highlight.ResetMouse();else highlight.Highlight(scene.input[2]);
        CHECK((scene.trace.empty() ? "-" : scene.trace)==fields[1]);CHECK(scene.Values()==HighlightNumbers(fields[2]));++count;
    }
    CHECK(count==178);
}

// 같은 모드는 미연결 효과를 호출하지 않으며 Reset은 선택/강조/마우스 좌표를 바꾸지 않는다.
TEST_CASE(UserHighlight_NoChangeAndResetKeepSelection) {
    std::uint32_t mode=5,selected=0xffffffffU,mouseType=71;std::int32_t x=-33,y=65; //각 원본 전역의 독립 현재 입력이다.
    RawUserHighlight highlight({mode,selected,mouseType,x,y},{});highlight.Highlight(5);
    CHECK(mode==5 && selected==0xffffffffU && mouseType==71 && x==-33 && y==65);
    highlight.ResetMouse();CHECK(mode==5 && selected==0xffffffffU && mouseType==0 && x==-33 && y==65);
}

// 선택된 실제 경계 누락은 원본의 mode/TYPE 선행 쓰기 뒤 해당 위치에서 거부된다.
TEST_CASE(UserHighlight_MissingSelectedEffectsPreserveWrittenPrefix) {
    std::uint32_t mode=9,selected=73,mouseType=81;std::int32_t x=-1,y=2;bool rejected{}; //세 실제 호출이 공유할 현재 저장소와 관찰이다.
    RawUserHighlight missingCursor({mode,selected,mouseType,x,y},{});
    try { missingCursor.Highlight(0); }catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected && mode==0 && mouseType==0 && selected==73 && x==-1 && y==2);
    rejected=false;mouseType=81;
    try { missingCursor.Highlight(5); }catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected && mode==5 && mouseType==0 && selected==73);
    UserHighlightHooks hooks;std::size_t projections{}; //역투영까지 실제 실행한 뒤 누락된 이동 경계 위치를 구분한다.
    hooks.project=[&](std::int32_t actualX,std::int32_t actualY) { CHECK(actualX==-1 && actualY==2 && mouseType==0);++projections;selected=99;mode=6;return std::array<std::uint32_t,2>{0x80000000U,0x7fc01234U}; };
    RawUserHighlight missingMove({mode,selected,mouseType,x,y},std::move(hooks));mode=4;mouseType=81;rejected=false;
    try { missingMove.Highlight(5); }catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected && projections==1 && mode==6 && mouseType==0 && selected==99);
}
