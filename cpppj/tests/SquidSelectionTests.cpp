// 원본83VT/전체extra BYTE/사정거리 종류/491생성 입구를 독립 PE 자료와 대조한다.
#include "RawSceneSupport.h"
#include "o/RawSquidSelection.h"
#include <bit>
#include <stdexcept>
using namespace netstorm::test::rawscene;

namespace {
// unsigned 십진수의 원문 DWORD만 읽으며 기대 분기를 계산하지 않는다.
std::vector<std::uint32_t> SquidSelectionNumbers(std::string_view text) {
    std::vector<std::uint32_t> result; // 입력/현재 필드의 원본 비트다.
    // signed였던 group도 상위 비트를 그대로 읽는다.
    for (const auto& field:Split(text,',')) result.push_back(Number(field));return result;
}
// 원본이 상태 필터 없이 쓰는 물리 슬롯의 명시 입력과 표시 효과다.
struct SquidSelectionScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // 같은50바이트 물리 범위다.
    std::vector<std::uint32_t> row; // 10개 독립 DWORD이며 기대값과 분리한다.
    std::string trace; // 실제 표시/할당/생성/진단 호출 순서다.
    // free/dead 입력을 생성 성공으로 바꾸지 않고 명시 raw 바이트로 주입한다.
    std::span<std::uint8_t> Raw() { const auto raw=pool.Slot(Sid{50});return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // 원문 boundary 인자를 동일한 unsigned 표현으로 기록한다.
    void Event(std::string_view name,std::initializer_list<std::uint32_t> values={}) {
        if (!trace.empty()) trace+=';';trace+=name;
        // 전체 DWORD를 항목 순서대로 보관한다.
        for (const auto value:values) trace+=':'+std::to_string(value);
    }
    // 경계가 수정하는 현재 입력만 공급한다. 제품의 bit 변경/종류/Regular 관문은 실행하지 않는다.
    void Refresh(Sid sid) {
        CHECK(sid.value==50);const auto raw=Raw();Event("R",{sid.value,raw[40],Get(raw,0)});
        if (row[4]==1) raw[40]=0xa5;else if (row[4]==2) { raw[40]=0x12;Put(raw,0,0xdeadbeef); }else if (row[4]==3) raw[40]=0xff;
    }
    // Native와 같은 물리 raw입력만 설치하고 이전 효과를 지운다.
    void Prepare(std::vector<std::uint32_t> values) {
        row=std::move(values);CHECK(row.size()==10);trace.clear();auto raw=Raw();std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,row[1]);raw[10]=static_cast<std::uint8_t>(row[7]);raw[11]=0xff;raw[40]=static_cast<std::uint8_t>(row[2]);
    }
};
}
// 기대 extra/VT/TYPE와 외부 효과는 두 정밀도의 실제 원본 반환에서만 읽는다.
TEST_CASE(SquidSelection_NativeVtablesBytesAndPreview) {
    SquidSelectionScene scene;std::size_t count{}; // 같은 물리 저장소를 각 입력으로 독립 초기화한다.
    // 모든 새 원문 행을 순서대로 재생하며 기대값을 코드로 생성하지 않는다.
    for (const auto& fields:LoadFixture(NETSTORM_SQUID_SELECTION_FIXTURE).rows) {
        CHECK(fields.size()==4);if (fields.size()!=4) continue;scene.Prepare(SquidSelectionNumbers(fields[0]));std::uint32_t result{};
        if (scene.row[0]==0) { RawSquidSelection selection(scene.pool,[&](Sid sid) { scene.Refresh(sid); });selection.Mark(Sid{50},scene.row[3]); }
        else if (scene.row[0]==1) { RiftTypeRecord type;type.group=std::bit_cast<std::int32_t>(scene.row[5]);type.flags2=scene.row[6];result=SelectionUsesRange(type) ? 1U : 0U; }
        else StartSelectionContentPreview(scene.pool,Sid{50},scene.row[8],[&](Sid sid,std::uint32_t event,float payload) { scene.Event("A",{40});if (scene.row[9]) scene.Event("C",{event,sid.value,std::bit_cast<std::uint32_t>(payload)}); },[&] { scene.Event("!");scene.trace+=":!s.isForm():144";if (scene.row[4]==4) scene.Raw()[10]=158; });
        CHECK(result==Number(fields[1]));CHECK((scene.trace.empty() ? "-" : scene.trace)==fields[2]);const auto raw=scene.Raw();CHECK((std::vector<std::uint32_t>{Get(raw,0),raw[40],raw[10]}==SquidSelectionNumbers(fields[3])));++count;
    }
    CHECK(count==5600);
}
// 기본+7c는 표시를 요구하지 않고 파생+7c의 미연결 표시는 원본 bit쓰기 전에 거부한다.
TEST_CASE(SquidSelection_LazyDisplayAndUnknownVtable) {
    SquidSelectionScene scene;scene.Prepare({0,0x501dd0,0xa1,1,0,0,0,158,0,1});RawSquidSelection basic(scene.pool,{});basic.Mark(Sid{50},1);CHECK(scene.Raw()[40]==0xa5);
    Put(scene.Raw(),0,0x501ea0);scene.Raw()[40]=0xa1;bool rejected{};try { basic.Mark(Sid{50},1); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected && scene.Raw()[40]==0xa1);
    Put(scene.Raw(),0,0xdeadbeef);rejected=false;try { basic.Mark(Sid{50},1); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected && scene.Raw()[40]==0xa1);
    SidPool cd(OriginalEdition::Cd1072,15000,true);rejected=false;try { RawSquidSelection invalid(cd,{}); }catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
}
