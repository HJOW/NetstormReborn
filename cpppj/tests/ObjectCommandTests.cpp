// 원본과 독립 관측한 공통 객체 명령의 분기/가상 인자/미처리 반환을 대조한다.
#include "RawSceneSupport.h"
#include "o/RawObjectCommand.h"
#include <filesystem>
using namespace netstorm::test::rawscene;

// 원본10.78의160입력을 읽기만 하며 CD의 차이는 같은 원문에 보존한다.
TEST_CASE(ObjectCommand_Native160Inputs) {
    SidPool pool{OriginalEdition::Patch1078,24000,true};const Sid parent{50};
    const std::array<std::uint32_t,5> commands{0x030000afU,0x030000e2U,0x03000015U,0x0300006eU,0x030003feU}; // 원본 데이터 표와 미등록 명령이다.
    std::size_t count{};
    const auto path=(std::filesystem::path(__FILE__).parent_path()/"fixtures/object-command-x86.tsv").string();
    // 기대 분기를 재계산하지 않고 원본 반환과 호출 인자를 비교한다.
    for (const auto& row:LoadFixture(path.c_str()).rows) {
        CHECK(row.size()==4);if (row.size()!=4 || row[0]!="originals") continue;
        const auto input=Split(row[1],',');CHECK(input.size()==6);if (input.size()!=6) continue;
        const auto slot=pool.Slot(parent);const std::span raw{const_cast<std::uint8_t*>(slot.data()),slot.size()}; // 검사 입력만 물리 슬롯에 설치한다.
        std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,26,Number(input[4]));
        std::string trace; // 이번 제품 호출이 만든 가상 효과 순서다.
        ObjectCommandState state;state.localOwner=[&] { return Number(input[5]); };state.editor=[&] { return Number(input[2])!=0; };state.mission=[&] { return Number(input[3])!=0; };
        ObjectCommandHooks hooks;
        hooks.owner=[&](Sid sid,std::uint32_t owner) { CHECK(sid==parent);trace="owner:"+std::to_string(owner); };
        hooks.hit=[&](Sid sid,std::int32_t amount,std::uint32_t owner,Sid attacker) { CHECK(sid==parent);trace="hit:"+std::to_string(std::bit_cast<std::uint32_t>(amount))+':'+std::to_string(owner)+':'+std::to_string(attacker.value); };
        hooks.destroy=[&](Sid sid,std::uint32_t flags) { CHECK(sid==parent);trace="destroy:"+std::to_string(flags); };
        hooks.describe=[&](Sid sid) { CHECK(sid==parent);trace="describe"; };
        RawObjectCommand command(pool,state,std::move(hooks));CHECK(command.Command(parent,commands.at(Number(input[0]))|Number(input[1]))==Number(row[2]));
        CHECK((trace.empty() ? "-" : trace)==row[3]);CHECK(Get(raw,26)==Number(input[4]));++count;
    }
    CHECK(count==160);
}

// 제한된 Hurt/미처리는 외부 효과를 요구하지 않고, 같은 객체의 현재 모드를 매번 읽는다.
TEST_CASE(ObjectCommand_CurrentModeAndLazyBoundaries) {
    SidPool pool{OriginalEdition::Patch1078,24000,true};bool editor{},mission{};std::size_t missionReads{};
    ObjectCommandState state;state.editor=[&] { return editor; };state.mission=[&] { ++missionReads;return mission; };
    RawObjectCommand command(pool,state,{});CHECK(command.Command(Sid{},0xffffffffU)==0);CHECK(command.Command(Sid{},0x030000e2U)==1);CHECK(missionReads==1);
    editor=true;bool rejected{};try { command.Command(Sid{},0x030000e2U); }catch (const std::exception&) { rejected=true; }CHECK(rejected && missionReads==1);
    editor=false;mission=true;rejected=false;try { command.Command(Sid{},0x030000e2U); }catch (const std::exception&) { rejected=true; }CHECK(rejected && missionReads==2);
}
