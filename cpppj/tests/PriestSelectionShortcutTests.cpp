// 실제 사제 행동 없이 P/R 입력 부모의 원문 부분 분기·장기 카운터·일반 큐를 대조한다.
#include "RawSceneSupport.h"
#include "client/PriestSelectionShortcut.h"
#include "client/ClientUserSelectionGlobals.h"
#include "client/GameMenuCommands.h"
#include <bit>
#include <filesystem>
#include <stdexcept>
using namespace netstorm::client;
using namespace netstorm::test::rawscene;

namespace { //참여 SID는 명시 입력이며 실제 포획/희생·생산·전투를 실행하지 않는다.
std::vector<std::uint32_t> PriestKeyNumbers(std::string_view text) { std::vector<std::uint32_t> result;for (const auto& part:Split(text,',')) result.push_back(Number(part));return result; } //각 TSV DWORD를 부호 변환 없이 원시 비트로 복원한다.
struct PriestKeyScene { //현재 세 DWORD와 원문 경계의 호출 기록을 독립 설치한다.
    std::vector<std::uint32_t> row;std::uint32_t selected{},count{},owner{};std::string trace; //이번 입력·현재 전역·경계 순서다.
    void Event(std::string_view name,std::initializer_list<std::uint32_t> args) { if (!trace.empty()) trace+=';';trace+=name;for (auto arg:args) trace+=':'+std::to_string(arg); } //같은 실제 경계의 전체 DWORD 인자를 순서대로 남긴다.
    void Prepare(std::vector<std::uint32_t> input) { row=std::move(input);selected=row[0];count=row[1];owner=row[2];trace.clear(); } //기대 분기를 미리 계산하지 않는다.
    PriestSelectionShortcutHooks Hooks() { //독립 원문 도구와 동일한 명시 하위 효과만 제공한다.
        PriestSelectionShortcutHooks hooks;hooks.localOwner=[this] { return owner; };hooks.viewCommand=[this] { return row[6]; };
        hooks.find=[this](auto player) { Event("F",{player});if (row[4]==1) selected=51;if (row[4]==2) owner=8;if (row[4]==3) count=0xffffffffU;if (row[4]==4) count=0;return row[3]; };
        hooks.select=[this](auto sid,auto force,auto activate) { Event("S",{sid,force,activate});if (row[5]==1) selected=sid;if (row[5]==2) count=0xffffffffU;if (row[5]==3) count=77; };hooks.queue=[this](auto code,auto first,auto second) { Event("Q",{code,first,second}); };return hooks;
    }
};
}
//조회 전 기본 선택 캡처·후속 현재 카운터/owner 읽기를 포함해 원문220개 입력을 대조한다.
TEST_CASE(PriestSelectionShortcut_IndependentNativeBranch) {
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();const auto fixture=LoadFixture((root/"cpppj/tests/fixtures/priest-selection-shortcut-x86.tsv").string().c_str());CHECK(!fixture.rows.empty());PriestKeyScene scene;std::size_t count{};
    for (const auto& fields:fixture.rows) { //기대 fixture를 제품 계산으로 재작성하지 않는다.
        CHECK(fields.size()==3);if (fields.size()!=3) continue;scene.Prepare(PriestKeyNumbers(fields[0]));CHECK(scene.row.size()==7);SelectPriestByKey(scene.selected,scene.count,scene.Hooks());const auto expected=PriestKeyNumbers(fields[2]);const std::vector<std::uint32_t> current{scene.selected,scene.count,scene.owner};
        if (scene.trace!=fields[1] || current!=expected) std::fprintf(stderr,"사제 키 원문 입력 %s / 기대 %s / 실제 %s\n",fields[0].c_str(),fields[1].c_str(),scene.trace.c_str());CHECK(scene.trace==fields[1]);CHECK(current==expected);++count;
    }
    CHECK(count==220 && count==fixture.rows.size()); //모든 독립 부분 분기 입력을 방문해야 한다.
}
//반복 키는 실제 일반 큐의 View 명령이며 누적 DWORD는 World reset/선택 정리의 초기화 대상이 아니다.
TEST_CASE(PriestSelectionShortcut_CurrentOwnerQueueAndCounterLifetime) {
    ClientUserSelectionGlobals globals;CHECK(globals.priestSelections==0);globals.WriteDword(ClientUserSelectionGlobals::Field::PriestSelections,0xffffffffU);globals.ResetForGeneralWorld();CHECK(globals.ReadDword(ClientUserSelectionGlobals::Field::PriestSelections)==0xffffffffU);InputQueue queue;std::uint32_t owner=1,selects{};
    PriestSelectionShortcutHooks hooks;hooks.localOwner=[&] { return owner; };hooks.viewCommand=[] { return GameMenuCommandValue(0x005429d4U).value(); };hooks.find=[](auto player) { CHECK(player==1 || player==8);return 50U; };hooks.select=[&](auto sid,auto force,auto activate) { CHECK(sid==50 && !force && activate==1);globals.selected=sid;++selects; };hooks.queue=[&](auto code,auto first,auto second) { queue.PushCommand(code,std::bit_cast<std::int32_t>(first),std::bit_cast<std::int32_t>(second)); };
    SelectPriestByKey(globals.selected,globals.priestSelections,hooks);CHECK(globals.selected==50 && globals.priestSelections==1 && selects==1 && queue.Empty());owner=8;SelectPriestByKey(globals.selected,globals.priestSelections,hooks);const auto command=queue.Pop(false);CHECK(command.code==0x020000b8U && command.x==8 && command.y==8 && globals.priestSelections==1 && selects==1 && queue.Empty());
    globals.ResetForGeneralWorld();CHECK(globals.selected==0 && globals.priestSelections==1);SelectPriestByKey(globals.selected,globals.priestSelections,hooks);CHECK(globals.selected==50 && globals.priestSelections==1 && selects==2);
}
//선택 거부/누락이 있어도 이미 기록한 변경 분기의 count0과 콜백의 현재 DWORD 감김은 보존한다.
TEST_CASE(PriestSelectionShortcut_PartialFailureAndCurrentWrap) {
    PriestKeyScene scene;scene.Prepare({50,7,1,51,0,0,0x020000b8});auto hooks=scene.Hooks();hooks.select={};bool failed{};try { SelectPriestByKey(scene.selected,scene.count,hooks); }catch (const std::logic_error&) { failed=true; }CHECK(failed && scene.selected==50 && scene.count==0 && scene.trace=="F:1");
    scene.Prepare({50,0,1,50,0,2,0x020000b8});SelectPriestByKey(scene.selected,scene.count,scene.Hooks());CHECK(scene.count==0 && scene.trace=="F:1;S:50:0:1");
}
