// 창/전투 실행 없이 원문 다중 선택 관측과 같은 실제 Client 목록/선택 몸체를 대조한다.
#include "RawSceneSupport.h"
#include "client/ClientUserSelectionGlobals.h"
#include "client/RawUserSelection.h"
#include <bit>
#include <filesystem>
#include <memory>
#include <sstream>
#include <stdexcept>
using namespace netstorm::client;
using namespace netstorm::test::rawscene;

namespace { //선택 대상은 독립 raw 입력이며 실제 공중 유닛 생산/행동을 실행하지 않는다.
std::vector<std::uint32_t> MultipleNumbers(std::string_view text) { //독립 TSV의 unsigned 원문 비트를 그대로 복원한다.
    std::vector<std::uint32_t> result;for (const auto& value:Split(text,',')) result.push_back(Number(value));return result; //모든 원소를 순서대로 읽으며 음수 비트를 보존한다.
}
struct MultipleScene { //같은 Client 선택 전역/30슬롯 배열과 물리 풀을 각 입력에서 재사용한다.
    SidPool pool{OriginalEdition::Patch1078,24000,true};std::array<RiftTypeRecord,188> types{}; //실제50BYTE 슬롯과188TYPE 경계다.
    ClientUserSelectionGlobals owner;std::vector<std::uint32_t> row;std::string trace; //현재 입력/전역과 실제 경계 호출 순서다.
    std::size_t moves{},refreshes{}; //최초 장치 효과만 바꾸는 명시 입력의 카운터다.
    std::span<std::uint8_t> Raw(std::uint32_t sid) { auto bytes=pool.Slot(Sid{static_cast<std::uint16_t>(sid)});return {const_cast<std::uint8_t*>(bytes.data()),bytes.size()}; } //독립 물리 입력만 쓰며 원본 파일은 읽기 참조다.
    void Event(std::string_view name,std::initializer_list<std::uint32_t> values={}) { //실제 원문 trace 형식으로 현재 인자를 기록한다.
        if (!trace.empty()) trace+=';';trace+=name;for (auto value:values) trace+=':'+std::to_string(value); //각 DWORD의 부호 비트도 보존한다.
    }
    void Prepare(std::vector<std::uint32_t> input) { //이전 관측과 무관하게 같은13DWORD 입력을 구성한다.
        row=std::move(input);trace.clear();moves=refreshes=0;owner.ResetForGeneralWorld();owner.selected=row[5];owner.mode=5;owner.enabled=row[7];owner.multipleCount=std::bit_cast<std::int32_t>(row[6]);owner.multiple[0]=52;owner.multiple[1]=53;owner.multiple[2]=52;
        if (row[9]==2) { owner.multiple[0]=0;owner.multiple[1]=24000; }
        for (std::uint32_t sid=50;sid<55;++sid) { //다섯 독립 선택 대상만 초기화해 표시/이동 장치를 가장하지 않는다.
            auto raw=Raw(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=100;raw[34]=1;raw[40]=static_cast<std::uint8_t>(sid==50 ? row[4] : 4U);Put(raw,14,0x41280000U);Put(raw,18,0xc0500000U);if (sid==50) raw[11]=static_cast<std::uint8_t>(row[3]);if (row[9]==1 && (sid==52 || sid==53)) raw[11]=sid==52 ? 1 : 2;
        }
        types[100].flags2=0x10000U;
    }
    UserSelectionHooks Hooks() { //원문 oracle의 명시 경계만 공급하고 토글/목록/이동 부모는 제품에서 실행한다.
        UserSelectionHooks hooks;hooks.paused=[] { return 0U; };hooks.enabled=[this] { return owner.enabled; };hooks.blocked=[this] { Event("B");if (row[10]==8) { owner.multipleCount=1;owner.multiple[0]=53; }return row[8]; };
        hooks.selectNotice=[this](Sid sid,auto on) { Event("N",{sid.value,on});if (row[10]==1) owner.selected=51;if (row[10]==2) owner.multipleCount=1;auto& extra=Raw(sid.value)[40];extra=static_cast<std::uint8_t>((extra&~4U)|((on&1U)<<2)); }; //기본41c0b0의 명시 가상 경계이며 단독 기존 Mark 검증과 구별한다.
        hooks.refresh=[this](Sid sid) { Event("R",{sid.value});if (row[10]==3 && ++refreshes==1) { owner.multiple[1]=54;owner.multipleCount=3; } };
        hooks.appendMultiple=[this](auto sid) { Event("A",{sid});owner.AppendMultiple(sid); };hooks.removeMultiple=[this](auto sid) { Event("D",{sid});owner.RemoveMultiple(sid); };hooks.writeEnabled=[this](auto value) { owner.enabled=value; };
        hooks.sound=[this](auto x,auto y,auto name,auto flags) { CHECK(name=="select.wav");Event("S",{x,y,flags});if (row[10]==4) owner.selected=52; };hooks.highlight=[this](auto mode) { Event("H",{mode});owner.mode=mode; };
        hooks.localOwner=[] { return 1U; };hooks.editor=[] { return 0U; };hooks.dais=[] { return 165U; };hooks.dump=[](Sid) {};hooks.rangeType=[](auto) { return 0U; };hooks.content=[](Sid) { return Sid{}; };hooks.moving=[](Sid) { return 0U; };hooks.movementPreview=[](Sid) {};return hooks;
    }
    void Move(std::uint32_t sid,std::uint32_t x,std::uint32_t y) { //실제 이동 전송 대신 명시 관측/콜백 뒤 현재값 변경만 수행한다.
        Event("M",{sid,x,y});if (++moves==1) { if (row[10]==5) owner.multipleCount=1;if (row[10]==6) owner.multiple[1]=54;if (row[10]==7) owner.enabled=7; }
    }
    std::vector<std::uint32_t> Current() { //선택/강조/enabled/count·다섯 선택 비트·전체30슬롯을 실제 물리에서 읽는다.
        std::vector<std::uint32_t> result{owner.selected,owner.mode,owner.enabled,std::bit_cast<std::uint32_t>(owner.multipleCount)};for (std::uint32_t sid=50;sid<55;++sid) result.push_back(Raw(sid)[40]); //현재 가상 통지가 수정한 표시 비트를 읽는다.
        result.insert(result.end(),owner.multiple.begin(),owner.multiple.end());return result;
    }
};
}
//원문 토글/다중 이동·기본 선택 비트/실제 목록 helper의 모든 독립 기대 행을 재생한다.
TEST_CASE(MultipleSelection_IndependentNative) {
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();const auto fixture=LoadFixture((root/"cpppj/tests/fixtures/multiple-selection-x86.tsv").string().c_str());CHECK(!fixture.rows.empty());MultipleScene scene;std::size_t count{};
    for (const auto& fields:fixture.rows) { //제품 계산으로 fixture 기대값을 생성하지 않는다.
        CHECK(fields.size()==3);if (fields.size()!=3) continue;scene.Prepare(MultipleNumbers(fields[0]));RawUserSelection selection(scene.pool,scene.types,scene.owner.SelectionState(),scene.Hooks());
        if (!scene.row[0]) selection.ToggleMultiple(scene.row[1],scene.row[2]);else selection.MoveMultiple(scene.row[11],scene.row[12],[&](auto sid,auto x,auto y) { scene.Move(sid,x,y); });
        const auto trace=scene.trace.empty() ? "-" : scene.trace;const auto expected=MultipleNumbers(fields[2]);if (trace!=fields[1] || scene.Current()!=expected) std::fprintf(stderr,"다중 선택 원문 입력 %s / 기대 %s / 실제 %s\n",fields[0].c_str(),fields[1].c_str(),trace.c_str());CHECK(trace==fields[1]);CHECK(scene.Current()==expected);++count;
    }
    CHECK(count==601 && count==fixture.rows.size()); //독립 원문 도구의 최종601개 입력을 모두 방문해야 한다.
}
//실제 단일 선택→Shift 추가/제외→주선택 승격과 고정 용량·중복·안정된 제거를 확인한다.
TEST_CASE(MultipleSelection_ExtendPromotionAndFixedCapacity) {
    MultipleScene scene;scene.Prepare({0,50,0xffffffffU,0,0,0,0,1,0,0,0,0,0});scene.Raw(51)[40]=0;RawUserSelection selection(scene.pool,scene.types,scene.owner.SelectionState(),scene.Hooks());
    selection.ExtendSelection(50,0);CHECK(scene.owner.selected==50 && scene.owner.mode==5 && (scene.Raw(50)[40]&4));selection.ExtendSelection(51,0);CHECK(scene.owner.multipleCount==1 && scene.owner.multiple[0]==51 && (scene.Raw(51)[40]&4));selection.ExtendSelection(51,0);CHECK(scene.owner.multipleCount==0 && !(scene.Raw(51)[40]&4));selection.ExtendSelection(51,0);selection.ToggleMultiple(50,0xffffffffU);CHECK(scene.owner.selected==51 && !scene.owner.multipleCount && !(scene.Raw(50)[40]&4) && (scene.Raw(51)[40]&4));
    scene.owner.ResetForGeneralWorld();for (std::uint32_t sid=1;sid<=31;++sid) scene.owner.AppendMultiple(sid); //원문 용량30 밖의 추가는 확대하지 않는다.
    CHECK(scene.owner.multipleCount==30 && scene.owner.multiple.size()==30 && scene.owner.multiple[29]==30);scene.owner.AppendMultiple(15);CHECK(scene.owner.multipleCount==30);scene.owner.RemoveMultiple(15);CHECK(scene.owner.multipleCount==29 && scene.owner.multiple[14]==16 && scene.owner.multiple[28]==30 && scene.owner.multiple[29]==30);
}
//다중 이동 중 단일 선택 해제가 차단되고 실패 시 enabled0/기존 목록을 보존하는지 확인한다.
TEST_CASE(MultipleSelection_MovementLockAndPartialFailure) {
    MultipleScene scene;scene.Prepare({1,50,0,0,4,50,1,1,0,0,0,0x41280000,0xc0500000});RawUserSelection selection(scene.pool,scene.types,scene.owner.SelectionState(),scene.Hooks());bool failed{};
    try { selection.MoveMultiple(scene.row[11],scene.row[12],[&](auto sid,auto,auto) { CHECK(sid==52 && !scene.owner.enabled);CHECK(selection.Select(0,0,1)==0 && scene.owner.selected==50 && scene.owner.multipleCount==1);throw std::runtime_error("이동 전송 실패 명시 입력"); }); }catch (const std::runtime_error&) { failed=true; }CHECK(failed && !scene.owner.enabled && scene.owner.selected==50 && scene.owner.multipleCount==1 && (scene.Raw(52)[40]&4));
    selection.MoveMultiple(scene.row[11],scene.row[12],[&](auto sid,auto,auto) { CHECK(sid==52 && !scene.owner.enabled); });CHECK(scene.owner.enabled==1 && !scene.owner.multipleCount && scene.owner.selected==50 && !(scene.Raw(52)[40]&4));
}
