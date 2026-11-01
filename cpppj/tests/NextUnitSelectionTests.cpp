// 창/유닛 행동 없이 원문 다음 이동 유닛 부모와 실제 선택 접합을 확인한다.
#include "RawSceneSupport.h"
#include "client/NextUnitSelection.h"
#include "client/ClientUserSelectionGlobals.h"
#include "client/RawUserSelection.h"
#include <filesystem>
#include <stdexcept>
using namespace netstorm::client;
using namespace netstorm::test::rawscene;

namespace { //독립 입력 타입만 쓰며 실제 공중3종 생산/생성/행동은 실행하지 않는다.
std::vector<std::uint32_t> NextNumbers(std::string_view text) { std::vector<std::uint32_t> result;for (const auto& part:Split(text,',')) result.push_back(Number(part));return result; } //모든 TSV DWORD를 원문 비트로 순서대로 읽는다.
struct NextScene { //원문의8개 입력 슬롯과188TYPE·현재 selected/local을 각 입력에서 독립 설치한다.
    SidPool pool{OriginalEdition::Patch1078,24000,true};std::array<RiftTypeRecord,188> types{}; //실제 지원 풀의8번 이후는 전부free로 남겨 원문의8칸 끝과 같은 무후보 경계를 사용한다.
    std::vector<std::uint32_t> row;std::uint32_t selected{},local{};std::string trace; //현재 입력·공유 DWORD와 실제 경계 기록이다.
    std::span<std::uint8_t> Raw(std::uint32_t sid) { const auto bytes=pool.Slot(Sid{static_cast<std::uint16_t>(sid)});return {const_cast<std::uint8_t*>(bytes.data()),bytes.size()}; } //독립 실제 슬롯 바이트를 설치한다.
    void Event(std::string_view name,std::initializer_list<std::uint32_t> values) { if (!trace.empty()) trace+=';';trace+=name;for (auto value:values) trace+=':'+std::to_string(value); } //경계의 실제 순서와 모든 DWORD 인자를 보존한다.
    void Prepare(std::vector<std::uint32_t> input) { //기대값을 계산하지 않고 독립 초기 상태만 설치한다.
        row=std::move(input);selected=row[0];local=row[2];trace.clear();for (std::uint32_t sid=0;sid<8;++sid) { auto raw=Raw(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=static_cast<std::uint8_t>(100+sid);raw[11]=static_cast<std::uint8_t>(row[6+sid*3]);raw[34]=static_cast<std::uint8_t>(row[7+sid*3]);types[100+sid].flags2=row[8+sid*3]; } //모든 물리 슬롯의 state/owner/TYPE flags를 원시 입력대로 저장한다.
    }
    NextUnitSelectionHooks Hooks() { //원문 oracle와 같은 명시 경계만 공급한다. 검색 부모는 제품이다.
        NextUnitSelectionHooks hooks;hooks.localOwner=[this] { return local; };hooks.paused=[this] { Event("P",{row[3]});return row[3]; };
        hooks.select=[this](auto sid,auto force,auto activate) { Event("S",{sid,force,activate});if (row[4]) selected=std::array<std::uint32_t,4>{sid,0,5,4}[row[4]-1]; };
        hooks.center=[this](auto sid) { Event("C",{sid});if (row[5]==1) selected=0;if (row[5]==2) Raw(sid)[11]=1;if (row[5]==3) types[100+sid].flags2=0;if (row[5]==4) selected=6;if (row[5]==5) local=2; };hooks.multiple=[this](auto sid,auto on) { Event("T",{sid,on}); };return hooks;
    }
    std::vector<std::uint32_t> Current() { std::vector<std::uint32_t> result{selected,local};for (std::uint32_t sid=0;sid<8;++sid) { result.push_back(Raw(sid)[11]);result.push_back(types[100+sid].flags2); }return result; } //반환 뒤 변경된 모든 물리 상태와 TYPE를 읽는다.
};
}
//첫 선택/가운데/정지/후속 선택의 순서와 순환/free/dead/전체owner 비교를 실제 원문 기대값에 대조한다.
TEST_CASE(NextUnitSelection_IndependentNative) {
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();const auto fixture=LoadFixture((root/"cpppj/tests/fixtures/next-unit-selection-x86.tsv").string().c_str());CHECK(!fixture.rows.empty());NextScene scene;std::size_t count{};
    for (const auto& fields:fixture.rows) { //기존 fixture나 제품 계산으로 기대값을 재작성하지 않는다.
        CHECK(fields.size()==3);if (fields.size()!=3) continue;scene.Prepare(NextNumbers(fields[0]));CHECK(scene.row.size()==30);SelectNextMovingUnit(scene.pool,scene.types,scene.selected,scene.row[1],scene.Hooks());const auto expected=NextNumbers(fields[2]);const auto trace=scene.trace.empty() ? "-" : scene.trace;
        if (trace!=fields[1] || scene.Current()!=expected) std::fprintf(stderr,"다음 유닛 원문 입력 %s / 기대 %s / 실제 %s\n",fields[0].c_str(),fields[1].c_str(),trace.c_str());CHECK(trace==fields[1]);CHECK(scene.Current()==expected);++count;
    }
    CHECK(count==fixture.rows.size() && count==410); //독립 원문 입력 전체를 빠짐없이 재생한다.
}
//실제 Select와 ToggleMultiple을 연결하고 정지 중 첫 선택 거부 뒤에도 가운데 이동을 수행함을 확인한다.
TEST_CASE(NextUnitSelection_ActualSelectionAndPauseOrder) {
    NextScene scene;std::vector<std::uint32_t> input(30);input[2]=1;for (std::uint32_t sid=0;sid<8;++sid) { input[6+sid*3]=sid<2 ? 1 : 0;input[7+sid*3]=1;input[8+sid*3]=0x10000; }scene.Prepare(input);ClientUserSelectionGlobals globals;globals.selected=2;bool paused{};std::vector<std::uint32_t> centered;
    UserSelectionHooks hooks;hooks.paused=[&] { return paused ? 1U : 0U; };hooks.enabled=[&] { return globals.enabled; };hooks.blocked=[] { return 0U; };hooks.localOwner=[] { return 1U; };hooks.editor=[] { return 0U; };hooks.dais=[] { return 165U; };hooks.dump=[](Sid) {};hooks.rangeType=[](auto) { return 0U; };hooks.content=[](Sid) { return Sid{}; };hooks.moving=[](Sid) { return 0U; };hooks.movementPreview=[](Sid) {};hooks.highlight=[&](auto mode) { globals.mode=mode; };hooks.sound=[](auto,auto,auto,auto) {};
    hooks.selectNotice=[&](Sid sid,auto value) { auto& extra=scene.Raw(sid.value)[40];extra=static_cast<std::uint8_t>((extra&~4U)|((value&1U)<<2)); };hooks.refresh=[](Sid) {};hooks.appendMultiple=[&](auto sid) { globals.AppendMultiple(sid); };hooks.removeMultiple=[&](auto sid) { globals.RemoveMultiple(sid); };hooks.writeEnabled=[&](auto value) { globals.enabled=value; };RawUserSelection selection(scene.pool,scene.types,globals.SelectionState(),hooks);
    NextUnitSelectionHooks next;next.localOwner=[] { return 1U; };next.paused=hooks.paused;next.select=[&](auto sid,auto force,auto activate) { static_cast<void>(selection.Select(sid,force,activate)); };next.center=[&](auto sid) { centered.push_back(sid); };next.multiple=[&](auto sid,auto value) { selection.ToggleMultiple(sid,value); };
    SelectNextMovingUnit(scene.pool,scene.types,globals.selected,0,next);CHECK(globals.selected==3 && centered==std::vector<std::uint32_t>{3} && (scene.Raw(3)[40]&4));SelectNextMovingUnit(scene.pool,scene.types,globals.selected,0x20000000,next);CHECK(globals.selected==4 && globals.multipleCount==0 && (scene.Raw(4)[40]&4));
    paused=true;SelectNextMovingUnit(scene.pool,scene.types,globals.selected,0,next);CHECK(globals.selected==4 && centered==std::vector<std::uint32_t>({3,4,5}));
}
//누락된 경계는 도달한 위치에서 실패하고 앞서 수행한 선택/가운데 효과를 보존한다.
TEST_CASE(NextUnitSelection_PartialFailure) {
    NextScene scene;std::vector<std::uint32_t> row(30);row[2]=1;row[4]=1;row[7+2*3]=1;row[8+2*3]=0x10000;scene.Prepare(row);auto hooks=scene.Hooks();hooks.paused={};bool failed{};
    try { SelectNextMovingUnit(scene.pool,scene.types,scene.selected,0,hooks); }catch (const std::logic_error&) { failed=true; }CHECK(failed && scene.selected==2 && scene.trace=="S:2:0:1;C:2");
}
