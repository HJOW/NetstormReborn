// 실제 키/전투 실행 없이 원문 선택 키 부모와 같은 Guide·일반 큐·View 메뉴 관문을 대조한다.
#include "RawSceneSupport.h"
#include "client/SelectionKeyInput.h"
#include "client/GameMenuCommands.h"
#include "client/GameMenuActions.h"
#include "o/RawTutorialOne.h"
#include <bit>
#include <filesystem>
#include <map>
#include <stdexcept>
using namespace netstorm::client;
using namespace netstorm::test::rawscene;

namespace { //Native fixture의 독립 인자와 실제 경계 기록만 사용한다.
std::vector<std::uint32_t> SelectionKeyNumbers(std::string_view text) { std::vector<std::uint32_t> out;for (const auto& value:Split(text,',')) out.push_back(Number(value));return out; } //입력의 전체 DWORD 비트를 그대로 읽는다.
void SelectionKeyEvent(std::string& trace,std::string_view name,std::initializer_list<std::uint32_t> args={}) { if (!trace.empty()) trace+=';';trace+=name;for (auto arg:args) trace+=':'+std::to_string(arg); } //원문 경계 순서/인자 형식이다.
}
//정지/Shift/Control/Alt와 대소문자·F4/F5의 모든 원문480행을 방문한다.
TEST_CASE(SelectionKeyInput_IndependentNativeParent) {
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();const auto fixture=LoadFixture((root/"cpppj/tests/fixtures/selection-key-input-x86.tsv").string().c_str());CHECK(!fixture.rows.empty());std::size_t count{};
    for (const auto& fields:fixture.rows) { //기대 분기/인자를 제품 계산으로 다시 만들지 않는다.
        CHECK(fields.size()==2);if (fields.size()!=2) continue;const auto row=SelectionKeyNumbers(fields[0]);CHECK(row.size()==3);std::string trace;SelectionKeyInputHooks hooks;
        hooks.paused=[&] { SelectionKeyEvent(trace,"P",{row[1]});return row[1]!=0; };hooks.command=[](auto address) { return GameMenuCommandValue(address).value(); };hooks.localOwner=[&] { return row[2]; };hooks.queue=[&](auto code,auto first,auto second) { SelectionKeyEvent(trace,"Q",{code,first,second}); };hooks.next=[&](auto add) { SelectionKeyEvent(trace,"N",{add}); };hooks.priest=[&] { SelectionKeyEvent(trace,"R"); };
        CHECK(DispatchSelectionKeyInput({row[0],37,-19},hooks));if (trace!=fields[1]) std::fprintf(stderr,"선택 키 원문 입력 %s / 기대 %s / 실제 %s\n",fields[0].c_str(),fields[1].c_str(),trace.c_str());CHECK(trace==fields[1]);++count;
    }
    CHECK(count==480 && count==fixture.rows.size()); //같은 일부 키만 재생해 통과한 것으로 세지 않는다.
}
//뗌/명령 사건은 키로 오인하지 않으며 정지나 F4 수정키 거부는 실제 선택/카메라를 요구하지 않는다.
TEST_CASE(SelectionKeyInput_LazyGatesAndExactCodes) {
    SelectionKeyInputHooks hooks;CHECK(!DispatchSelectionKeyInput({InputCode::kRelease|'N',0,0},hooks));CHECK(!DispatchSelectionKeyInput({InputCode::kCommand|'H',0,0},hooks));CHECK(!DispatchSelectionKeyInput({InputCode::kLeftButton|'P',0,0},hooks));CHECK(!DispatchSelectionKeyInput({'V',0,0},hooks));
    hooks.paused=[] { return true; };CHECK(DispatchSelectionKeyInput({'N',0,0},hooks));CHECK(DispatchSelectionKeyInput({'R',0,0},hooks));CHECK(DispatchSelectionKeyInput({0x740000,0,0},hooks));hooks.paused=[] { return false; };CHECK(DispatchSelectionKeyInput({0x730000|InputCode::kControl,0,0},hooks));CHECK(DispatchSelectionKeyInput({0x730000|InputCode::kShift,0,0},hooks));
}
//원시 F4를 Guide Home으로 합성하지 않고 실제 큐의 다음 사건에서 한 번 판정하며 기존 ViewHome 관문도 유지한다.
TEST_CASE(SelectionKeyInput_QueuedHomeGuideAndMenuGates) {
    InputQueue queue;std::uint32_t owner=1;SelectionKeyInputHooks keys;keys.paused=[] { return false; };keys.command=[](auto address) { return GameMenuCommandValue(address).value(); };keys.localOwner=[&] { return owner; };keys.queue=[&](auto code,auto first,auto second) { queue.PushCommand(code,std::bit_cast<std::int32_t>(first),std::bit_cast<std::int32_t>(second)); };
    std::int32_t stage='A';std::uint32_t shown=1,home=0x020000ae,reclaim=0x020000ff,advances{};std::vector<std::uint8_t> players(9*180);SquidPostPopList geysers;TutorialOneHooks guideHooks;
    guideHooks.builtForGenus=[](auto) { return 0; };guideHooks.graphFor=[](auto) { return 0U; };guideHooks.money=[](auto) { return 0.0F; };guideHooks.advance=[&] { ++advances;++stage; };guideHooks.spawnGeysers=[] {};RawTutorialOne guide({stage,owner,shown,home,reclaim},players,geysers,guideHooks);
    std::map<std::uint32_t,std::uint32_t> globals{{0x595344,0},{0x594fc8,0},{0x540c70,1}};std::uint32_t hide{},redraw{};std::vector<std::uint32_t> centered,priests;std::array<std::uint8_t,180> player{};Put(player,0x7c,77);GameMenuActionHooks actions;
    actions.command=[](auto address) { return GameMenuCommandValue(static_cast<std::uint32_t>(address)).value(); };actions.player=[&](auto local) -> std::span<const std::uint8_t> { CHECK(local==1);return player; };actions.centerObject=[&](auto sid) { centered.push_back(sid); };actions.viewPriest=[&](auto local) { priests.push_back(local); };GameMenuActions menu({[&](auto address) { return globals.at(address); },hide,redraw},actions);
    CHECK(guide.Handle(0x730000)==0 && !advances && stage=='A');CHECK(DispatchSelectionKeyInput({0x730000,0,0},keys));CHECK(!advances);auto event=queue.Pop(false);CHECK(event.code==home && !event.x && !event.y);CHECK(guide.Handle(event.code)==0 && advances==1 && stage=='B');CHECK(menu.Handle(event.code,0,0)==0U && centered.empty());
    globals[0x595344]=1;globals[0x594fc8]=1;CHECK(DispatchSelectionKeyInput({'H',0,0},keys));event=queue.Pop(false);CHECK(menu.Handle(event.code,0,0)==0U && centered.empty());globals[0x594fc8]=0;CHECK(DispatchSelectionKeyInput({'h',0,0},keys));event=queue.Pop(false);CHECK(menu.Handle(event.code,0,0)==0U && centered==std::vector<std::uint32_t>{77} && advances==1);
    owner=8;CHECK(DispatchSelectionKeyInput({0x740000|InputCode::kShift,0,0},keys));event=queue.Pop(false);CHECK(event.code==0x020000b8 && event.x==8 && event.y==8);CHECK(menu.Handle(event.code,static_cast<std::uint32_t>(event.x),static_cast<std::uint32_t>(event.y))==0U && priests==std::vector<std::uint32_t>{8});CHECK(queue.Empty());
}
//실제 큐 포화는 명령 유실을 원문처럼 맡기고 이미 조회한 전역이나 누락된 장치의 효과를 바꾸지 않는다.
TEST_CASE(SelectionKeyInput_QueueCapacityAndMissingBoundary) {
    InputQueue queue;for (std::size_t index=0;index<InputQueue::kCapacity-1;++index) queue.Push('V',0,0); //원문39칸 가득 찬 상태를 만든다.
    SelectionKeyInputHooks hooks;hooks.paused=[] { return false; };hooks.command=[](auto address) { return GameMenuCommandValue(address).value(); };hooks.queue=[&](auto code,auto,auto) { queue.PushCommand(code,0,0); };CHECK(DispatchSelectionKeyInput({'H',0,0},hooks));std::size_t count{};while (!queue.Empty()) { CHECK(queue.Pop(false).code=='V');++count; }CHECK(count==39); //Home이 앞 사건을 밀어내거나 직접 카메라로 우회하지 않는다.
    hooks.queue={};std::uint32_t reads{};hooks.command=[&](auto) { ++reads;return 0x020000aeU; };bool failed{};try { static_cast<void>(DispatchSelectionKeyInput({'H',0,0},hooks)); }catch (const std::logic_error&) { failed=true; }CHECK(failed && reads==1);
}
