// 세 PE의 사제 상태/HP setter 관찰과 분기별 읽기·쓰기 보호를 검사한다.
#include "RawSceneSupport.h"
#include "o/RawPriestState.h"
#include "o/RawPriestOwner.h"
#include <limits>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원본 없는 CTest에서 저장된 독립 기계어 관찰만 읽는다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto fixture=LoadFixture(NETSTORM_PRIESTSTATE_FIXTURE);return fixture.rows;
}
struct StateScene {
    SidPool pool;
    GameRandom random;
    ScrambledSpStore store;
    SquidPostPopState book;
    SquidRewardState hpMode;
    std::vector<RiftTypeRecord> types;
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536);
    std::unique_ptr<SquidReward> hp;
    std::unique_ptr<RawPriestState> state;
    PriestHitPointMode mode;
    bool patch;
    std::string events,after;
    // client SID 50을 확보하며 HP 조회만으로 SP 저장소가 초기화되지 않는지도 확인한다.
    explicit StateScene(OriginalEdition edition):pool(edition,kCapacity,false),store(random,[] { return 1U; }),
        types(edition==OriginalEdition::Patch1078 ? 188 : 171),patch(edition==OriginalEdition::Patch1078) {
        // 지정된 원본 관찰 번호까지 실제 풀에 할당한다.
        for (std::uint16_t id=5;id<=kSource.value;++id) CHECK(pool.Allocate(2)==Sid{id});
    }
    // 입력만 공급한다. 원본 반환값/HP 변경 결과를 준비 코드에서 계산하지 않는다.
    void Prepare(const std::vector<std::string>& row) {
        CHECK(row.size()==18);events.clear();after=row[14];
        auto& type=types[kPriestType];type.maxHitPoints=std::stoi(row[2]);type.flags1=0x69012;type.flags2=Number(row[4]);
        hpMode.priestQuarterHp=Number(row[3])!=0;
        hp=std::make_unique<SquidReward>(pool,types,book,hpMode,patch ? &store : nullptr);
        const auto spot=Number(row[7]);
        // 체커보드는 두 좌표의 절삭/거의 올림 혼동을 드러내는 입력이다.
        for (int y=0;y<256;++y) {
            // 원본 실행기와 같은 y/x 입력 순서다.
            for (int x=0;x<256;++x) spots[static_cast<std::size_t>(y*256+x)]=static_cast<std::uint8_t>(spot^((x+y)%2 ? 6U : 0U));
        }
        auto raw=pool.AllocatedBytes(kSource);std::fill(raw.begin(),raw.end(),std::uint8_t{0xab});
        Put(raw,0,patch ? kPatchPriestVtable : kCdPriestVtable);raw[10]=kPriestType;raw[11]=0;
        raw[patch ? 40 : 35]=static_cast<std::uint8_t>(Number(row[6]));raw[patch ? 34 : 32]=static_cast<std::uint8_t>(Number(row[13]));
        Put(raw,14,Number(row[8]));Put(raw,18,Number(row[9]));Put(raw,26,Number(row[5]),patch ? 4 : 2);
        mode.authority=Number(row[11])!=0;mode.recoveryAllowed.fill(false);mode.recoveryAllowed[Number(row[13])]=Number(row[12])!=0;
        state=std::make_unique<RawPriestState>(pool,*hp,types,spots);
    }
    // 원본의 두 가상 공간 대체처럼 호출 시점의 HP/좌표를 기록하고 Unpop 변경 입력을 적용한다.
    void Record(char marker,Sid sid,std::uint32_t flags) {
        if (!events.empty()) events+=';';
        const auto raw=pool.Slot(sid);
        events+=std::string(1,marker)+':'+std::to_string(sid.value)+':'+std::to_string(flags)+':'+
            std::to_string(state->CurrentHitPoints(sid))+':'+std::to_string(Get(raw,14))+':'+std::to_string(Get(raw,18));
        if (marker=='U' && after!="-") {
            auto target=pool.AllocatedBytes(sid);Put(target,26,static_cast<std::uint32_t>(std::stoi(after)),patch ? 4 : 2);
            Put(target,14,std::bit_cast<std::uint32_t>(30.75f));Put(target,18,std::bit_cast<std::uint32_t>(31.9f));
        }
    }
};
// 실제 전체 몸체의 반환/사건/슬롯을 판본별로 재생한다.
void Replay(std::string_view edition) {
    StateScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);
    std::size_t count=0;
    // 각 입력은 이전 HP 변경의 영향을 받지 않는다.
    for (const auto& row:Fixture()) {
        if (row[1]!=edition) continue;
        scene.Prepare(row);const auto before=netstorm::test::FailureCount();
        if (row[0]=="Set") scene.state->SetHitPoints(kSource,std::stoi(row[10]),scene.mode,{
            [&](Sid sid,std::uint32_t flags) { scene.Record('U',sid,flags); },
            [&](Sid sid,std::uint32_t flags) { scene.Record('R',sid,flags); }});
        else CHECK((row[0]=="Ground" ? scene.state->GroundImmobile(kSource) : scene.state->Immobile(kSource))==(row[15]=="1"));
        CHECK((scene.events.empty() ? "-" : scene.events)==row[16]);CHECK(Hex(scene.pool.Slot(kSource))==row[17]);
        if (netstorm::test::FailureCount()!=before) {
            std::printf("사제 상태 %s 관찰 %zu: %s\n",std::string(edition).c_str(),count,row[0].c_str());break;
        }
        ++count;
    }
    CHECK(count==6752);CHECK(!scene.store.Initialized());
}
}
// 패치 DWORD HP·전체 조회/setter 몸체를 독립 관찰과 비교한다.
TEST_CASE(priest_state_patch_x86) { Replay("originals"); }
// CD signed WORD 및 같은 setter의 다른 어셈블리 구현을 비교한다.
TEST_CASE(priest_state_cd_x86) { Replay("originalCD"); }
// 추가 10.37도 별도 PE SHA의 관찰을 그대로 재생한다.
TEST_CASE(priest_state_1037_x86) { Replay("original1037"); }

// 강제 비트/비사제 genus/저HP는 나쁜 좌표를 조회하지 않는다. 필요한 지면 경로만 거부한다.
TEST_CASE(priest_state_short_circuits_unused_coordinates_and_preserves_raw) {
    // 두 raw HP 폭에 동일한 분기별 읽기 계약을 적용한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        StateScene scene(edition);auto row=Fixture().front();row[2]="200";row[3]="0";row[4]="2162688";row[5]="20";row[6]="0";
        row[8]=std::to_string(std::bit_cast<std::uint32_t>(std::numeric_limits<float>::quiet_NaN()));scene.Prepare(row);
        const auto before=Hex(scene.pool.Slot(kSource));CHECK(scene.state->GroundImmobile(kSource));CHECK(Hex(scene.pool.Slot(kSource))==before);
        row[5]="200";row[6]="32";scene.Prepare(row);CHECK(scene.state->Immobile(kSource));
        row[6]="0";scene.Prepare(row);CHECK(Throws([&] { scene.state->GroundImmobile(kSource); }));
        row[4]="0";scene.Prepare(row);CHECK(!scene.state->GroundImmobile(kSource));
        CHECK(Throws([&] { scene.state->Immobile(Sid{}); }));
        CHECK(Throws([&] { RawPriestState invalid(scene.pool,*scene.hp,scene.types,{}); }));
    }
}

// 전환에 필요한 효과/소유자/가상 표가 빠지면 HP 쓰기 전에 거부하며 전환 없는 변경은 효과를 요구하지 않는다.
TEST_CASE(priest_state_setter_rejects_invalid_bindings_before_hp_write) {
    StateScene scene(OriginalEdition::Patch1078);auto row=Fixture().front();row[2]="200";row[3]="0";row[4]="2162688";row[5]="150";
    scene.Prepare(row);scene.mode.authority=true;const auto before=Hex(scene.pool.Slot(kSource));
    CHECK(Throws([&] { scene.state->SetHitPoints(kSource,20,scene.mode,{}); }));CHECK(Hex(scene.pool.Slot(kSource))==before);
    scene.state->SetHitPoints(kSource,140,scene.mode,{});CHECK(scene.state->CurrentHitPoints(kSource)==140);
    auto raw=scene.pool.AllocatedBytes(kSource);Put(raw,26,20);raw[34]=255;const auto invalidOwner=Hex(raw);
    CHECK(Throws([&] { scene.state->SetHitPoints(kSource,150,scene.mode,{}); }));CHECK(Hex(raw)==invalidOwner);
    raw[34]=1;Put(raw,0,kVtable);const auto invalidVtable=Hex(raw);
    CHECK(Throws([&] { scene.state->SetHitPoints(kSource,30,scene.mode,{}); }));CHECK(Hex(raw)==invalidVtable);
}
