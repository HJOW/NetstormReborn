// 실제 Damageable의 효과/소리 접두·contained 순회와 Carrier 연결을 대조한다.
#include "RawSceneSupport.h"
#include "o/RawDamageablePreDestroy.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 각 PE에서 두 x87 정밀도로 검증한 관찰 행 수와 전체 raw 비교 슬롯이다.
constexpr std::size_t kRowsPerEdition=1632;
constexpr std::array<Sid,5> kIds{{{50},{60},{61},{62},{63}}};
// 원본/Python 없이 독립 관찰을 읽는다.
const auto& Fixture() { static const auto data=LoadFixture(NETSTORM_DAMAGEABLEPREDESTROY_FIXTURE);return data.rows; }
struct DamageableScene {
    SidPool pool;
    ContainedFinderState finderState;
    DamageablePreDestroyState state;
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536);
    std::array<std::span<std::uint8_t>,5> slots;
    std::string events;
    std::uint32_t style{},initialBoss{},freeCount{};
    bool patch;
    int validations{};
    // 합성 free 후보를 쓸 수 있도록 준비 단계의 할당 범위를 저장한다.
    explicit DamageableScene(OriginalEdition edition):pool(edition,kCapacity,false),patch(edition==OriginalEdition::Patch1078) {
        // 원래 client 할당으로 비교 대상 번호를 확보한다.
        for (std::uint16_t id=5;id<=63;++id) CHECK(pool.Allocate(2)==Sid{id});
        // 이후 free 비트 입력에서도 쓰기 API의 재검사를 하지 않는다.
        for (std::size_t i=0;i<kIds.size();++i) slots[i]=pool.AllocatedBytes(kIds[i]);
        finderState.checkingDead=true;
    }
    // effect 뒤의 좌표를 비트 그대로 입력한다.
    void Coordinates(float x,float y) { Put(slots[0],14,std::bit_cast<std::uint32_t>(x));Put(slots[0],18,std::bit_cast<std::uint32_t>(y)); }
    // 실행기의 경계별 raw/전역 변경만 공급하며 기대 결과는 계산하지 않는다.
    void Mutation(std::string_view marker) {
        if (style==1) {
            if (marker=="C") { Coordinates(21.00001f,22.00001f);slots[0][patch ? 40 : 35]=9; }
            if (marker=="A0") { Coordinates(22.75f,23.9f);finderState.boss=initialBoss==0; }
            if (marker=="X") Coordinates(23,24);
            if (marker=="A1") Coordinates(24.00001f,25.00001f);
        }
        if (style==2 && marker=="F" && freeCount==1) {
            finderState.boss=initialBoss==0;finderState.containedType=46;state.priestType=159;
            Put(slots[0],6,0,2);Put(slots[1],4,0,2);slots[2][10]=46;Put(slots[2],18,159,2);
        }
        if (style==3 && marker=="B") {
            finderState.boss=true;finderState.containedType=6;Put(slots[0],6,63,2);slots[4][10]=6;Put(slots[4],20,1,2);
        }
    }
    // 외부 호출 순서를 원본 관찰과 같은 문자열로 저장한다.
    void Record(const std::string& event) { if (!events.empty()) events+=';';events+=event; }
    // 실제 PE에서 대체한 함수/구간만 C++에도 동일한 외부 효과로 공급한다.
    DamageablePreDestroyHooks Hooks() {
        return {[this](Sid sid) { Record("C:"+std::to_string(sid.value));Mutation("C"); },
            [this](Sid sid) { Record("X:"+std::to_string(sid.value));Mutation("X"); },
            [this](const DamageableSoundEvent& event) {
                if (event.sound==DamageableSound::PriestFree) { CHECK(event.x==0 && event.y==0);Record("F");++freeCount;Mutation("F"); }
                else {
                    const auto marker=event.sound==DamageableSound::Collapse ? "A0" : "A1";
                    Record(std::string(marker)+':'+std::to_string(std::bit_cast<std::uint32_t>(event.x))+':'+std::to_string(std::bit_cast<std::uint32_t>(event.y)));Mutation(marker);
                }
            },[this](Sid sid) { Record("R:"+std::to_string(sid.value)); },
            [this](Sid,std::uint32_t) { ++validations; },
            [this](Sid sid,std::uint32_t flags) { Record("B:"+std::to_string(sid.value)+':'+std::to_string(flags));Mutation("B"); }};
    }
    // 입력 슬롯/spot/전역은 행의 입력 칸에서만 구성한다.
    void Prepare(const std::vector<std::string>& row) {
        CHECK(row.size()==17);events.clear();validations=0;freeCount=0;style=Number(row[13]);initialBoss=Number(row[4]);
        finderState.boss=initialBoss!=0;finderState.containedType=Number(row[5]);state.priestType=Number(row[6]);
        std::fill(spots.begin(),spots.end(),std::uint8_t{});
        // 지정 map 셀만 표시하므로 좌표 경계의 선택이 실제 출력에 영향을 준다.
        for (const auto& mark:Split(row[10],';')) { const auto values=Split(mark,',');spots.at(Number(values[1])*256+Number(values[0]))=static_cast<std::uint8_t>(Number(values[2])); }
        // 주변/미지정 raw도 전체 비교한다.
        for (auto raw:slots) { std::fill(raw.begin(),raw.end(),std::uint8_t{0xab});raw[10]=158;raw[11]=0;raw[patch ? 40 : 35]=0;Put(raw,4,0); }
        slots[0][11]=static_cast<std::uint8_t>(Number(row[2]));slots[0][patch ? 40 : 35]=static_cast<std::uint8_t>(Number(row[3]));
        Put(slots[0],14,Number(row[8]));Put(slots[0],18,Number(row[9]));Put(slots[0],6,Number(row[11]),2);
        // 번호·타입·상태·next·kind·mask를 입력 그대로 쓴다.
        for (const auto& node:Split(row[12],';')) {
            if (node.empty()) continue;
            const auto values=Split(node,',');auto raw=slots.at(Number(values[0])-60+1);
            raw[10]=static_cast<std::uint8_t>(Number(values[1]));raw[11]=static_cast<std::uint8_t>(Number(values[2]));
            Put(raw,4,Number(values[3]),2);Put(raw,18,Number(values[4]),2);Put(raw,20,Number(values[5]),2);
        }
    }
    // 입력 5개 raw를 실행기의 순서로 직렬화한다.
    std::string Slots() const {
        std::string result;
        // 후보의 미지정 바이트까지 모두 비교한다.
        for (auto sid:kIds) { if (!result.empty()) result+='|';result+=Hex(pool.Slot(sid)); }
        return result;
    }
    // 현재 권한/타입/사제 kind 전역의 전체 DWORD 관찰이다.
    std::string Globals() const { return std::to_string(finderState.boss ? 1 : 0)+','+std::to_string(finderState.containedType)+','+std::to_string(state.priestType); }
};
// 직접 Damageable와 실제 Carrier 두 경로를 각각 실제 PE의 출력과 대조한다.
void Replay(std::string_view edition) {
    DamageableScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;
    CHECK(Fixture().size()==3*kRowsPerEdition);
    // 후보 상태/바뀐 next/현재 전역을 구현에서 추정하지 않고 독립 행을 재생한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;
        scene.Prepare(row);RawDamageablePreDestroy damageable(scene.pool,scene.spots,scene.finderState,scene.state,scene.Hooks());
        if (row[1]=="D") damageable.PreDestroy(kSource,Number(row[7]));
        else {
            RawCarrierPreDestroy carrier(scene.pool,scene.finderState,MakeDamageablePreDestroyHooks(scene.pool,damageable,{{},{},[&] { scene.Record("T"); }}));
            carrier.PreDestroy(kSource,Number(row[7]));
        }
        const bool same=scene.Globals()==row[14] && scene.events==row[15] && scene.Slots()==row[16];
        CHECK(same && scene.validations==(row[1]=="D" ? 1 : 2));
        if (!same) { std::printf("Damageable %s 행 %zu: 기대 %s / 실제 %s\n",std::string(edition).c_str(),count,row[15].c_str(),scene.events.c_str());break; }
        ++count;
    }
    CHECK(count==kRowsPerEdition);
}
}
// 실제 10.78의 +0.9999/x87·yesIKnow=1·현재 전역 읽기를 대조한다.
TEST_CASE(damageable_predestroy_patch_x86) { Replay("originals"); }
// CD 별도 prologue·CRT·inline 조건의 전체 출력이다.
TEST_CASE(damageable_predestroy_cd_x86) { Replay("originalCD"); }
// 추가 10.37 PE 관찰을 별도로 재생한다.
TEST_CASE(damageable_predestroy_1037_x86) { Replay("original1037"); }

// 미연결 공간/효과·잘못된 자산·비유한 ordinary 좌표는 효과 전에 거부한다.
TEST_CASE(damageable_predestroy_guards_and_abstract_coordinates) {
    // 판본별 extra와 같은 풀 연결 계약을 검사한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        DamageableScene scene(edition);scene.Prepare(Fixture().front());auto hooks=scene.Hooks();hooks.sound={};
        CHECK(Throws([&] { RawDamageablePreDestroy missing(scene.pool,scene.spots,scene.finderState,scene.state,hooks); }));
        CHECK(Throws([&] { RawDamageablePreDestroy shortMap(scene.pool,std::span(scene.spots).first(1),scene.finderState,scene.state,scene.Hooks()); }));
        RawDamageablePreDestroy damageable(scene.pool,scene.spots,scene.finderState,scene.state,scene.Hooks());
        const auto reject=[&](Sid sid) { const auto before=scene.Slots();CHECK(Throws([&] { damageable.PreDestroy(sid,0x300000); }));CHECK(scene.events.empty() && scene.Slots()==before); };
        reject({0});reject({1});reject({24000});scene.slots[0][11]=1;reject(kSource);scene.slots[0][11]=8;reject(kSource);scene.slots[0][11]=2;
        scene.slots[0][10]=6;reject(kSource);scene.slots[0][10]=255;reject(kSource);scene.slots[0][10]=158;
        Put(scene.slots[0],14,0x7fc00000);reject(kSource);scene.Coordinates(256,21);reject(kSource);
        // 추상/매장은 좌표를 읽지 않으므로 NaN도 공통 body까지 전달한다.
        Put(scene.slots[0],14,0x7fc00000);scene.slots[0][scene.patch ? 40 : 35]=9;damageable.PreDestroy(kSource,0x300000);CHECK(scene.events=="B:50:3145728");
        scene.Prepare(Fixture().front());hooks=scene.Hooks();hooks.validateBase=[](Sid,std::uint32_t) { throw std::logic_error("공통 효과 거부"); };
        RawDamageablePreDestroy blocked(scene.pool,scene.spots,scene.finderState,scene.state,hooks);reject({0});
        CHECK(Throws([&] { blocked.PreDestroy(kSource,1); }));CHECK(scene.events.empty());
        SidPool other(edition,kCapacity,false);CHECK(Throws([&] { MakeDamageablePreDestroyHooks(other,damageable,{}); }));
    }
}
