// 실제 세 PE의 전체 Damageable/Carrier 반환과 해방 좌표·WORD·호출 순서를 대조한다.
#include "RawSceneSupport.h"
#include "o/RawDamageableRelease.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 독립 입력 752개를 두 전체 호출 경로에서 관찰한 판본별 행 수다.
constexpr std::size_t kRowsPerEdition=1504;
// 부모/종속 입력과 생성 경계가 반환하는 새 객체의 관찰 번호다.
constexpr std::array<Sid,5> kIds{{{50},{60},{61},{62},{63}}};
constexpr std::array<Sid,4> kNewIds{{{100},{101},{102},{103}}};
// 실행기 안의 합성 가상 표 기록값이며 호스트 주소로 역참조하지 않는다.
constexpr std::uint32_t kReleaseVtable=0x20000100;
// 원본 PE나 Python 실행 없이 저장된 기계어 출력을 읽는다.
const auto& Fixture() { static const auto data=LoadFixture(NETSTORM_DAMAGEABLERELEASE_FIXTURE);return data.rows; }
struct ReleaseScene {
    SidPool pool;
    ContainedFinderState finderState;
    DamageablePreDestroyState preState;
    DamageableReleaseState state;
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536);
    std::vector<RiftTypeRecord> types;
    std::array<std::span<std::uint8_t>,5> slots;
    std::array<std::span<std::uint8_t>,4> newSlots;
    std::string events;
    std::uint32_t change{},born{};
    bool changed{},patch{};
    // 준비 때 할당 span을 보관하여 합성 free 후보를 재설정할 수 있게 한다.
    explicit ReleaseScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),patch(edition==OriginalEdition::Patch1078) {
        // 실행기와 같은 번호 범위만 확보한다.
        for (std::uint16_t id=5;id<=103;++id) CHECK(pool.Allocate(2)==Sid{id});
        // 후보 free 비트를 넣기 전에 입력 span을 저장한다.
        for (std::size_t i=0;i<slots.size();++i) slots[i]=pool.AllocatedBytes(kIds[i]);
        // 미생성 슬롯의 free 비트 입력도 직접 초기화할 수 있게 저장한다.
        for (std::size_t i=0;i<newSlots.size();++i) newSlots[i]=pool.AllocatedBytes(kNewIds[i]);
        finderState.checkingDead=true;
    }
    // 외부 효과를 독립 관찰 형식으로 기록한다.
    void Record(const std::string& event) { if (!events.empty()) events+=';';events+=event; }
    // 좌표는 raw float 비트를 그대로 쓴다.
    void Coordinates(float x,float y) { Put(slots[0],14,std::bit_cast<std::uint32_t>(x));Put(slots[0],18,std::bit_cast<std::uint32_t>(y)); }
    // 첫 생성 경계에서 독립 실행기와 같은 입력 변경만 공급한다.
    void Mutation() {
        if (!change || changed) return;
        changed=true;
        if (change==1) {
            Put(slots[1],12,0xfedc,2);Put(slots[1],4,0,2);Coordinates(23.25f,24.75f);finderState.boss=false;
            Put(slots[2],18,158,2);Put(slots[2],20,32,2);state.allowedGenus=0xffffffff;
        } else {
            state.battle=false;state.allowedGenus=0;finderState.containedType=46;slots[3][10]=46;
            state.bridgeType=90;spots[21*256+20]=16;
        }
    }
    // 실제 하위 함수 대체만 제공하며 좌표/타입 분기와 WORD 복사는 구현에 맡긴다.
    DamageableReleaseHooks Hooks() {
        return {[this](float x,float y,std::uint32_t kind,std::uint16_t word) {
                Record("P:"+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y))+':'+std::to_string(kind)+':'+std::to_string(word));Mutation(); },
            [this](std::uint32_t kind,std::uint32_t flags) {
                CHECK(flags==0 && born<newSlots.size());const auto index=born++;auto raw=newSlots[index];
                std::fill(raw.begin(),raw.end(),std::uint8_t{0xcd});Put(raw,0,kReleaseVtable);raw[10]=static_cast<std::uint8_t>(kind);raw[11]=0;
                const Sid sid=kNewIds[index];Record("N:"+std::to_string(kind)+':'+std::to_string(flags)+':'+std::to_string(sid.value));Mutation();return sid; },
            [this](Sid sid,std::uint32_t owner) { CHECK(owner==0);Record("O:"+std::to_string(sid.value)+':'+std::to_string(owner)); },
            [this](Sid sid,float x,float y,std::uint32_t flags) {
                CHECK(flags==0);Record("L:"+std::to_string(sid.value)+':'+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y))+':'+std::to_string(flags)); },
            [this](Sid sid) { Record("S:"+std::to_string(sid.value)); }};
    }
    // 이미 대조한 효과/소리/공통 경계이며 해방은 실제 몸체에 연결한다.
    DamageablePreDestroyHooks PreHooks(const RawDamageableRelease& release) {
        return MakeDamageableReleaseHooks(pool,release,{
            [this](Sid sid) { Record("C:"+std::to_string(sid.value)); },[this](Sid sid) { Record("X:"+std::to_string(sid.value)); },
            [this](const DamageableSoundEvent& event) {
                if (event.sound==DamageableSound::PriestFree) Record("F");
                else Record(std::string(event.sound==DamageableSound::Collapse ? "A0" : "A1")+':'+std::to_string(std::bit_cast<std::uint32_t>(event.x))+':'+std::to_string(std::bit_cast<std::uint32_t>(event.y))); },
            {},[](Sid,std::uint32_t) {},[this](Sid sid,std::uint32_t flags) { Record("B:"+std::to_string(sid.value)+':'+std::to_string(flags)); }});
    }
    // 입력 칸만 사용하여 현재 타입 표·공간·raw를 준비한다.
    void Prepare(const std::vector<std::string>& row) {
        CHECK(row.size()==25);events.clear();born=0;changed=false;change=Number(row[19]);
        finderState.boss=Number(row[4])!=0;finderState.containedType=Number(row[5]);preState.priestType=Number(row[6]);
        state.battle=Number(row[17])!=0;state.allowedGenus=Number(row[18]);state.bridgeType=82;
        std::fill(spots.begin(),spots.end(),std::uint8_t{});
        // 각 map 표시를 실행기의 셀에 기록한다.
        for (const auto& mark:Split(row[10],';')) { const auto value=Split(mark,',');spots.at(Number(value[1])*256+Number(value[0]))=static_cast<std::uint8_t>(Number(value[2])); }
        // 미지정 raw 바이트도 독립 실행기의 채움 값과 일치시킨다.
        for (auto raw:slots) { std::fill(raw.begin(),raw.end(),std::uint8_t{0xab});raw[10]=158;raw[11]=0;raw[patch ? 40 : 35]=0;Put(raw,4,0); }
        slots[0][10]=160;slots[0][11]=static_cast<std::uint8_t>(Number(row[2]));slots[0][patch ? 40 : 35]=static_cast<std::uint8_t>(Number(row[3]));
        Put(slots[0],14,Number(row[8]));Put(slots[0],18,Number(row[9]));Put(slots[0],6,Number(row[11]),2);
        // next/kind/flags와 후보 수명 비트를 추가 필터 없이 쓴다.
        for (const auto& node:Split(row[12],';')) {
            const auto value=Split(node,',');auto raw=slots.at(Number(value[0])-59);
            raw[10]=static_cast<std::uint8_t>(Number(value[1]));raw[11]=static_cast<std::uint8_t>(Number(value[2]));
            Put(raw,4,Number(value[3]),2);Put(raw,18,Number(value[4]),2);Put(raw,20,Number(value[5]),2);
        }
        // 생성 대상의 WORD 복사에서 상위 비트가 남는지 관찰할 입력이다.
        for (std::size_t i=1;i<slots.size();++i) Put(slots[i],12,0x8100+kIds[i].value,2);
        // 실제 생성 여부와 관계없이 네 개의 슬롯을 모두 비교한다.
        for (auto raw:newSlots) std::fill(raw.begin(),raw.end(),std::uint8_t{0xcd});
        types[160].flags2=Number(row[16]);types[160].footX=static_cast<int>(Number(row[14]));types[160].footY=static_cast<int>(Number(row[15]));
        types[158].flags2=0x200000;types[82].flags2=2;types[90].flags2=0x40000000;
    }
    // 부모와 후보 전체 바이트를 관찰한다.
    std::string Slots() const {
        std::string result;
        for (auto sid:kIds) { if (!result.empty()) result+='|';result+=Hex(pool.Slot(sid)); }
        return result;
    }
    // 생성 전의 미지정 슬롯을 포함한 새 슬롯 전체 바이트를 관찰한다.
    std::string NewSlots() const {
        std::string result;
        for (auto sid:kNewIds) { if (!result.empty()) result+='|';result+=Hex(pool.Slot(sid)); }
        return result;
    }
    // 원본의 현재 boss/contained/priest 전역이다.
    std::string Globals() const { return std::to_string(finderState.boss ? 1 : 0)+','+std::to_string(finderState.containedType)+','+std::to_string(preState.priestType); }
    // 해방 중 바뀔 수 있는 현재 전투/허용 genus/다리 타입 전역이다.
    std::string ReleaseGlobals() const { return std::to_string(state.battle ? 1 : 0)+','+std::to_string(state.allowedGenus)+','+std::to_string(state.bridgeType); }
};
// 실제 Damageable/Carrier 전체 출력과 새 자산 WORD까지 재생한다.
void Replay(std::string_view edition) {
    ReleaseScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;
    CHECK(Fixture().size()==3*kRowsPerEdition);
    // 구현에서 기대 분기를 만들지 않고 각 원본 관찰과 비교한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;
        scene.Prepare(row);RawDamageableRelease release(scene.pool,scene.types,scene.spots,scene.finderState,scene.state,scene.Hooks());
        RawDamageablePreDestroy damageable(scene.pool,scene.spots,scene.finderState,scene.preState,scene.PreHooks(release));
        if (row[1]=="D") damageable.PreDestroy(kSource,Number(row[7]));
        else { RawCarrierPreDestroy carrier(scene.pool,scene.finderState,MakeDamageablePreDestroyHooks(scene.pool,damageable,{{},{},[&] { scene.Record("T"); }}));carrier.PreDestroy(kSource,Number(row[7])); }
        const bool same=scene.Globals()==row[20] && scene.events==row[21] && scene.Slots()==row[22] && scene.ReleaseGlobals()==row[23] && scene.NewSlots()==row[24];
        CHECK(same);
        if (!same) { std::printf("Damageable release %s 행 %zu: 기대 %s / 실제 %s\n",std::string(edition).c_str(),count,row[21].c_str(),scene.events.c_str());break; }
        ++count;
    }
    CHECK(count==kRowsPerEdition);
}
}
// 10.78 실제 중심·스냅·WORD 쓰기와 전체 반환이다.
TEST_CASE(damageable_release_patch_x86) { Replay("originals"); }
// CD 별도 중심/공간 조회 기계어를 대조한다.
TEST_CASE(damageable_release_cd_x86) { Replay("originalCD"); }
// 추가 10.37 PE의 별도 관찰을 대조한다.
TEST_CASE(damageable_release_1037_x86) { Replay("original1037"); }
// 자료/하위 경계 누락·잘못된 부모와 서로 다른 풀 연결을 검사한다.
TEST_CASE(damageable_release_guards_and_binding) {
    // 판본별 타입 표 길이와 같은 풀 연결 계약을 검사한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        ReleaseScene scene(edition);scene.Prepare(Fixture().front());auto hooks=scene.Hooks();hooks.create={};
        CHECK(Throws([&] { RawDamageableRelease missing(scene.pool,scene.types,scene.spots,scene.finderState,scene.state,hooks); }));
        CHECK(Throws([&] { RawDamageableRelease shortMap(scene.pool,scene.types,std::span(scene.spots).first(1),scene.finderState,scene.state,scene.Hooks()); }));
        CHECK(Throws([&] { RawDamageableRelease shortTypes(scene.pool,std::span(scene.types).first(1),scene.spots,scene.finderState,scene.state,scene.Hooks()); }));
        RawDamageableRelease release(scene.pool,scene.types,scene.spots,scene.finderState,scene.state,scene.Hooks());
        CHECK(Throws([&] { release.Release({0}); }));scene.slots[0][11]=1;CHECK(Throws([&] { release.Release(kSource); }));scene.slots[0][11]=2;
        Put(scene.slots[0],14,0x7fc00000);CHECK(Throws([&] { release.Release(kSource); }));CHECK(scene.events.empty());
        SidPool other(edition,kCapacity,false);CHECK(Throws([&] { MakeDamageableReleaseHooks(other,release,{}); }));
    }
}
