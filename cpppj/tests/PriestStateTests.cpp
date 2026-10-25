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
// PriestState 검사에서 같은 풀·상태·처리기를 조립해 입력과 결과를 대조하는 자료형이다.
struct StateScene {
    // pool: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀. 입력 구성 또는 실행 결과 대조에 사용한다.
    SidPool pool;
    // random: 원본 FUN_004558c0: 상태 0이면 0x0bad0bad로 바꾼 뒤 상태*0x10003+3의 상위 16비트를 limit로 나눈 나머지를 쓴다 (GameRandom).
    GameRandom random;
    // store: 패치판이 플레이어별 SP를 32개의 9칸 DWORD 배열에 흩뿌려 보관하는 방식이다 (ScrambledSpStore).
    ScrambledSpStore store;
    // book: 등록 목록과 개수의 장부. 입력 구성 또는 실행 결과 대조에 사용한다.
    SquidPostPopState book;
    // hpMode: 보상 계산이 읽거나 갱신하는 판본별 전역과 AI 지갑이다. 패치 주소/CD 주소는 괄호에 둔다 (SquidRewardState).
    SquidRewardState hpMode;
    // types: 타입 번호로 속성·플래그를 조회하는 타입 표. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<RiftTypeRecord> types;
    // spots: 칸별 점유·통행 비트를 읽는 spot 지도. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536);
    // hp: 객체의 HP를 조회·변경하는 처리기 또는 HP 값. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::unique_ptr<SquidReward> hp;
    // state: 검사 입력과 실행 뒤 결과를 보관할 상태.
    std::unique_ptr<RawPriestState> state;
    // mode: 동작 분기를 선택하는 모드. 입력 구성 또는 실행 결과 대조에 사용한다.
    PriestHitPointMode mode;
    // patch: 10.78 패치판의 상태 또는 패치판 검사용 자료. 입력 구성 또는 실행 결과 대조에 사용한다.
    bool patch;
    // events: 발생한 이벤트 또는 이벤트 처리기. 입력 구성 또는 실행 결과 대조에 사용한다.
    // after: 후속. 입력 구성 또는 실행 결과 대조에 사용한다.
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

// 복사된 HP 모드도 원본 boss 주소를 다시 읽고 권한 없는 절반 HP 전환과 실제 공간 전환을 구분한다.
TEST_CASE(PriestState_SharedAuthoritySurvivesModeCopyAndChangesWithoutPrepare) {
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) { // 두 판본의 기존 HP 저장 폭을 유지한다.
        StateScene scene(edition);std::vector<std::string> row(18,"0");row[2]="200";row[5]="200";row[7]="4";row[8]=std::to_string(std::bit_cast<std::uint32_t>(30.0F));row[9]=std::to_string(std::bit_cast<std::uint32_t>(31.0F));row[11]="1";row[12]="1";row[13]="1";row[14]="-";scene.Prepare(row);
        bool authority=false;scene.mode.sharedAuthority=&authority;const auto copied=scene.mode;std::size_t unpops{},repops{};
        PriestHitPointHooks hooks{[&](Sid sid,std::uint32_t flags) { ++unpops;scene.Record('U',sid,flags); },[&](Sid sid,std::uint32_t flags) { ++repops;scene.Record('P',sid,flags); }};
        scene.state->SetHitPoints(kSource,1,copied,hooks);CHECK(scene.state->CurrentHitPoints(kSource)==100 && unpops==0 && repops==0);
        authority=true;scene.mode.authority=false;scene.state->SetHitPoints(kSource,1,copied,hooks);CHECK(scene.state->CurrentHitPoints(kSource)==1 && unpops==1 && repops==1 && copied.sharedAuthority==&authority);
    }
}

// 실제 세 PE의 HP setter 관찰 3,936행을 반대인 값형 배열과 현재 Player 기록으로 재생한다.
TEST_CASE(PriestState_SharedPlayerRecordsReplaysAllNativeSetterRows) {
    // 10.37은 기존 비교 경로와 같은 CD 배치를 쓰고 독립 PE 기대값은 그대로 유지한다.
    for (const auto edition:{std::string_view{"originals"},std::string_view{"originalCD"},std::string_view{"original1037"}}) {
        StateScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);const std::size_t stride=scene.patch ? 0xb4U : 0xacU,field=scene.patch ? 0x7cU : 0x74U;std::vector<std::uint8_t> players(9*stride);std::size_t count{};
        // 기대 HP·전체 raw·공간 호출 순서는 저장된 원본 관찰에서만 읽는다.
        for (const auto& row:Fixture()) {
            if (row[0]!="Set" || row[1]!=edition) continue;scene.Prepare(row);std::fill(players.begin(),players.end(),std::uint8_t{0xa5});const auto owner=Number(row[13]);Put(players,owner*stride+field,Number(row[12]));scene.mode.recoveryAllowed.fill(Number(row[12])==0);scene.mode.playerRecords=players;const auto before=players;
            scene.state->SetHitPoints(kSource,std::stoi(row[10]),scene.mode,{[&](Sid sid,std::uint32_t flags) { scene.Record('U',sid,flags); },[&](Sid sid,std::uint32_t flags) { scene.Record('R',sid,flags); }});CHECK((scene.events.empty() ? "-" : scene.events)==row[16] && Hex(scene.pool.Slot(kSource))==row[17] && players==before);++count;
        }
        CHECK(count==1312 && !scene.store.Initialized());
    }
}

// 중립·첫·마지막 owner와 하위 WORD가 0인 신전 DWORD도 원문대로 현재값을 읽는다.
TEST_CASE(PriestState_SharedPlayerRecoveryUsesCurrentWholeDwordAndCopiedView) {
    // 각 판본의 끝 기록까지 실제 원문 폭으로 준비한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        // 원문은 owner0도 허용하며 owner8 뒤를 읽지 않는다.
        for (const auto owner:{0U,1U,8U}) {
            StateScene scene(edition);std::vector<std::string> row(18,"0");row[2]="200";row[5]="20";row[7]="6";row[8]=std::to_string(std::bit_cast<std::uint32_t>(20.F));row[9]=std::to_string(std::bit_cast<std::uint32_t>(21.F));row[11]="1";row[13]=std::to_string(owner);row[14]="-";scene.Prepare(row);const std::size_t stride=scene.patch ? 0xb4U : 0xacU,field=scene.patch ? 0x7cU : 0x74U;std::vector<std::uint8_t> players(9*stride,0xa5);Put(players,owner*stride+field,0);scene.mode.playerRecords=players;auto copied=scene.mode;copied.recoveryAllowed.fill(true);std::size_t spaces{};PriestHitPointHooks hooks{[&](Sid,std::uint32_t) { ++spaces; },[&](Sid,std::uint32_t) { ++spaces; }};
            scene.state->SetHitPoints(kSource,150,copied,hooks);CHECK(scene.state->CurrentHitPoints(kSource)==20 && spaces==0);Put(players,owner*stride+field,0x10000);copied.recoveryAllowed.fill(false);const auto before=players;scene.state->SetHitPoints(kSource,150,copied,hooks);CHECK(scene.state->CurrentHitPoints(kSource)==150 && spaces==2 && players==before && copied.playerRecords.data()==players.data());scene.state->SetHitPoints(kSource,20,copied,hooks);Put(players,owner*stride+field,0);copied.recoveryAllowed.fill(true);scene.state->SetHitPoints(kSource,150,copied,hooks);CHECK(scene.state->CurrentHitPoints(kSource)==20 && spaces==4);
        }
    }
}

// 권한 없는 전환·저HP로의 전환·전환 없음은 Player 허용 필드를 조회하지 않는다.
TEST_CASE(PriestState_SharedPlayerRecoveryPreservesUnusedGuardsAndRejectsBadWidth) {
    // 양 판본에서 필요한 경로만 잘못된 기록 폭을 거부하며 실패 전 HP를 보존한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        StateScene scene(edition);std::vector<std::string> row(18,"0");row[2]="200";row[5]="200";row[7]="6";row[8]=std::to_string(std::bit_cast<std::uint32_t>(20.F));row[9]=std::to_string(std::bit_cast<std::uint32_t>(21.F));row[11]="1";row[13]="1";row[14]="-";scene.Prepare(row);std::array<std::uint8_t,1> wrong{};scene.mode.playerRecords=wrong;PriestHitPointHooks hooks{[](Sid,std::uint32_t) {},[](Sid,std::uint32_t) {}};scene.state->SetHitPoints(kSource,150,scene.mode,{});scene.state->SetHitPoints(kSource,20,scene.mode,hooks);const auto before=Hex(scene.pool.Slot(kSource));CHECK(Throws([&] { scene.state->SetHitPoints(kSource,150,scene.mode,hooks); }) && Hex(scene.pool.Slot(kSource))==before);scene.mode.authority=false;scene.state->SetHitPoints(kSource,150,scene.mode,{});CHECK(scene.state->CurrentHitPoints(kSource)==99);scene.mode.playerRecords={};scene.mode.authority=true;scene.mode.recoveryAllowed[1]=true;scene.state->SetHitPoints(kSource,150,scene.mode,hooks);CHECK(scene.state->CurrentHitPoints(kSource)==150 && Throws([&] { scene.mode.RecoveryAllowed(9,edition); }));
    }
}

// 원문은 Unpop 전에 허용을 결정하므로 Unpop이 신전 필드를 지워도 진행 중인 Repop은 수행한다.
TEST_CASE(PriestState_SharedPlayerRecoveryReadsBeforeUnpopAndAgainOnNextCall) {
    // 두 저장 폭 모두 현재 회복은 허용하고 다음 회복부터 새 0을 반영한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        StateScene scene(edition);std::vector<std::string> row(18,"0");row[2]="200";row[5]="20";row[7]="6";row[8]=std::to_string(std::bit_cast<std::uint32_t>(20.F));row[9]=std::to_string(std::bit_cast<std::uint32_t>(21.F));row[11]="1";row[13]="1";row[14]="-";scene.Prepare(row);const std::size_t stride=scene.patch ? 0xb4U : 0xacU,field=scene.patch ? 0x7cU : 0x74U;std::vector<std::uint8_t> players(9*stride);Put(players,stride+field,0x80000000U);scene.mode.playerRecords=players;std::size_t unpops{},repops{};PriestHitPointHooks hooks{[&](Sid,std::uint32_t) { ++unpops;Put(players,stride+field,0); },[&](Sid,std::uint32_t) { ++repops; }};scene.state->SetHitPoints(kSource,150,scene.mode,hooks);CHECK(scene.state->CurrentHitPoints(kSource)==150 && unpops==1 && repops==1 && Get(players,stride+field)==0);scene.state->SetHitPoints(kSource,20,scene.mode,hooks);scene.state->SetHitPoints(kSource,150,scene.mode,hooks);CHECK(scene.state->CurrentHitPoints(kSource)==20 && unpops==2 && repops==2);
    }
}
