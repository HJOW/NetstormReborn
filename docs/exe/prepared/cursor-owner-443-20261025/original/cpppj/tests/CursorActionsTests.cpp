// 원문 경계 입력과 실제 세 부모의 전체 전역/효과 순서를 재생한다.
#include "RawSceneSupport.h"
#include "client/RawCursorActions.h"
#include <filesystem>
using namespace netstorm::test::rawscene;
using namespace netstorm::client;
namespace {
// 고정 fixture의14개 독립 입력 열이다.
enum Column:std::size_t { Operation,Depth,Argument,Direction,Gump,Group,Selected,Special,Activate,Create,Sound,Remove,Select,Range };
// 원본 DWORD 목록만 읽으며 기대 출력을 호스트 수식으로 생성하지 않는다.
std::vector<std::uint32_t> Numbers(std::string_view value) {
    std::vector<std::uint32_t> values;
    // 모든 열과 부호 비트를 원문 십진수 순서대로 보존한다.
    for (const auto& field:Split(value,',')) values.push_back(Number(field));return values;
}
// 원본 실행기와 같은 명시 효과 입력만 갖는 현재 커서다.
struct ActionScene {
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재 TYPE 논리 포인터의 실제 표다.
    CursorDropState drop; // 같은 깊이/방향/Gump/선택 타입이다.
    CursorLifecycleState life; // 같은 논리 TYPE/첫 미리보기 SID다.
    std::uint32_t special{},rangeType{51}; // 원본이 매 접근 읽는 특별 타입과 프로세스다.
    std::vector<std::uint32_t> input; // 한 독립 입력의14DWORD다.
    std::string trace; // 명시 경계의 실제 호출 순서다.
    // 각 경계의 전체 DWORD를 원문과 같은 문자열에 기록한다.
    void Event(std::string_view letter,std::initializer_list<std::uint32_t> values={}) {
        if (!trace.empty()) trace+=';';trace+=letter;
        // 부호가 있는 값도 저장된 DWORD 십진수를 유지한다.
        for (const auto value:values) trace+=':'+std::to_string(value);
    }
    // 이전 관측 결과를 다음 독립 입력에 넘기지 않는다.
    void Prepare(std::vector<std::uint32_t> values) {
        input=std::move(values);drop={};life={};trace.clear();types[100].group=std::bit_cast<std::int32_t>(input[Group]);types[101].group=1;
        drop.lockDepth=input[Depth];drop.direction=input[Direction];drop.legalGump=input[Gump];drop.type=input[Selected];drop.cursorGump=2;life.abstractType=100;life.previews[0]=Sid{100};special=input[Special];
    }
    // 재진입 변경만 같은 실제 저장소에 적용하고 부모 계산은 C++에 맡긴다.
    CursorActionHooks Hooks() {
        CursorActionHooks hooks;
        hooks.activate=[this](std::uint32_t force) { Event("A",{force});if (input[Activate]==1) drop.lockDepth=3;if (input[Activate]==2) drop.direction=0xfffffff9U;if (input[Activate]==3) { life.abstractType=101;drop.type=special; } };
        hooks.createPreviews=[this] { Event("B");if (input[Create]==1) drop.legalGump=9;if (input[Create]==2) drop.lockDepth=5;if (input[Create]==3) { life.previews[0]=Sid{101};types[100].group=1; } };
        hooks.selectCursor=[this](std::uint32_t cursor) { Event("K",{cursor});if (input[Select]) life.abstractType=101; };
        hooks.sound=[this](std::string_view text) { CHECK(text=="rotatepiece.wav");Event("S:rotatepiece.wav",{0,0,0,0,0});if (input[Sound]==1) drop.lockDepth=1;if (input[Sound]==2) life.abstractType=101; };
        hooks.removeRange=[this](Sid parent,std::uint32_t type) { Event("D",{parent.value,type,0,1});if (input[Remove]==1) life.previews[0]=Sid{102};if (input[Remove]==2) { life.abstractType=100;types[100].group=2; } };
        hooks.createRange=[this](Sid parent,std::uint32_t form) { Event("G",{parent.value,form});return input[Range] ? 1U : 0U; };return hooks;
    }
    // 관측된 전역과 같은 순서로 현재 DWORD를 읽는다.
    std::vector<std::uint32_t> Values() const {
        return {drop.lockDepth,drop.direction,drop.legalGump,drop.type,life.previews[0].value,drop.cursorGump,life.abstractType,std::bit_cast<std::uint32_t>(types[100].group),std::bit_cast<std::uint32_t>(types[101].group)};
    }
};
}
// 기대값은 보호 PE의 실제 두 정밀도 반환에서만 읽는다.
TEST_CASE(CursorActions_NativeWholeBodiesAndCurrentState) {
    std::size_t rows{};ActionScene scene;
    // 원본 전체 고유 입력289행을 독립 초기화하여 비교한다.
    for (const auto& fields:LoadFixture(NETSTORM_CURSOR_ACTIONS_FIXTURE).rows) {
        CHECK(fields.size()==3);if (fields.size()!=3) continue;scene.Prepare(Numbers(fields[0]));CHECK(scene.input.size()==14);
        RawCursorActions actions(scene.types,scene.drop,scene.life,scene.special,scene.rangeType,scene.Hooks());
        if (scene.input[Operation]==0) actions.Lock(scene.input[Argument]);else if (scene.input[Operation]==1) actions.Unlock();else actions.Rotate(std::bit_cast<std::int32_t>(scene.input[Argument]));
        CHECK((scene.trace.empty() ? "-" : scene.trace)==fields[1]);CHECK(scene.Values()==Numbers(fields[2]));++rows;
    }
    CHECK(rows==289);
}
