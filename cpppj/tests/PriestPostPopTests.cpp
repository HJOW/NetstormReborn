// 사제 postPop prefix를 세 실제 PE 관찰과 실제 최대 HP/ProcessHost 합성으로 검사한다.
#include "RawSceneSupport.h"
#include "o/RawPriestPostPop.h"
#include "o/SquidFactory.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 판본마다 두 x87 정밀도로 얻은 독립 관찰 수다.
constexpr std::size_t kRowsPerEdition=2392;
// 새 fixture만 읽으며 실행 때 원본 PE와 Python은 필요 없다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto fixture=LoadFixture(NETSTORM_PRIESTPOSTPOP_FIXTURE);return fixture.rows;
}
struct PrefixScene {
    SidPool pool;
    GameRandom rng;
    ScrambledSpStore store;
    SquidPostPopState bookkeeping;
    SquidRewardState hpState;
    PriestPostPopState state;
    std::vector<RiftTypeRecord> types;
    std::unique_ptr<SquidReward> hp;
    std::vector<std::string> events;
    std::vector<std::uint32_t> storage;
    bool found{},allocated{},patch{};
    // 콘솔 전용 입력 SID와 읽기만 하는 HP 모듈을 준비한다.
    explicit PrefixScene(OriginalEdition edition):pool(edition,kCapacity,false),store(rng,[] { return 1U; }),
        types(edition==OriginalEdition::Patch1078 ? 188 : 171),patch(edition==OriginalEdition::Patch1078) {
        // 실제 client 영역에서 관찰 SID까지 할당한다.
        for (std::uint16_t id=5;id<=kSource.value;++id) CHECK(pool.Allocate(2)==Sid{id});
    }
    // 입력만 공급하고 기대 출력이나 원본 분기는 계산하지 않는다.
    void Prepare(const std::vector<std::string>& row) {
        CHECK(row.size()==19);events.clear();
        found=Number(row[10])!=0;allocated=Number(row[11])!=0;hpState.priestQuarterHp=Number(row[9])!=0;
        types[kPriestType].maxHitPoints=std::stoi(row[8]);types[kPriestType].flags1=0x00069012;types[kPriestType].flags2=0x00210000;
        hp=std::make_unique<SquidReward>(pool,types,bookkeeping,hpState,patch ? &store : nullptr);
        storage.clear();
        // 미사용 저장소도 비교할 수 있도록 입력 전체를 따로 보관한다.
        for (const auto& value:Split(row[14],',')) storage.push_back(Number(value));
        const auto capacity=Number(row[12]);CHECK(capacity<=storage.size());
        state.priests.entries.assign(storage.begin(),storage.begin()+capacity);state.priests.count=Number(row[13]);
        // 원본 실행기에 공급한 9칸의 비영 상태다.
        for (std::size_t i=0;i<state.ownerState.size();++i) state.ownerState[i]=0xabc000+static_cast<std::uint32_t>(i);
        auto raw=pool.AllocatedBytes(kSource);std::fill(raw.begin(),raw.end(),std::uint8_t{});
        Put(raw,0,patch ? kPatchPriestVtable : kCdPriestVtable);raw[10]=kPriestType;raw[11]=static_cast<std::uint8_t>(Number(row[4]));
        raw[patch ? 34 : 32]=static_cast<std::uint8_t>(Number(row[6]));raw[patch ? 40 : 35]=static_cast<std::uint8_t>(Number(row[5]));
        Put(raw,12,Number(row[7]),2);Put(raw,14,std::bit_cast<std::uint32_t>(20.75f));Put(raw,18,std::bit_cast<std::uint32_t>(21.9f));
        Put(raw,26,42,patch ? 4 : 2);
    }
    // 검색 시점의 목록 개수와 확보/생성 인자만 기록한다. 최대 HP는 실제 복원 함수를 사용한다.
    PriestPostPopHooks Hooks() {
        return {
            [this](Sid sid,std::uint32_t event) {
                events.push_back("F:"+std::to_string(sid.value)+':'+std::to_string(event)+':'+std::to_string(state.priests.count));return found;
            },
            [this] { events.push_back("A:40");return allocated; },
            [this](Sid sid,std::uint32_t event,float payload) {
                events.push_back("R:"+std::to_string(sid.value)+':'+std::to_string(event)+':'+std::to_string(std::bit_cast<std::uint32_t>(payload)));
            }
        };
    }
    // 용량 뒤의 입력 저장소는 C++에서 접근하지 않으므로 원래 값 그대로 비교한다.
    std::string List() {
        std::copy(state.priests.entries.begin(),state.priests.entries.end(),storage.begin());
        std::string result=std::to_string(state.priests.count)+':';
        // 전체 저장소를 원본 관찰과 같은 순서로 기록한다.
        for (std::size_t i=0;i<storage.size();++i) { if (i) result+=',';result+=std::to_string(storage[i]); }
        return result;
    }
    // 0과 범위 밖 소유자 처리에서 엉뚱한 칸이 변경되지 않았는지도 확인한다.
    std::string Owners() const {
        std::string result;
        // 칸 0부터 전 소유자까지 읽는다.
        for (auto value:state.ownerState) { if (!result.empty()) result+=',';result+=std::to_string(value); }
        return result;
    }
    // 호출 순서를 원본 대체 경계의 기록 형식으로 만든다.
    std::string Events() const {
        std::string result;
        // 사건 사이에만 구분자를 넣는다.
        for (const auto& value:events) { if (!result.empty()) result+=';';result+=value; }
        return result.empty() ? "-" : result;
    }
};
// 각 판본은 자신의 독립 원본 출력과 비교한다. float payload는 비트 단위로 검사한다.
void Replay(const char* edition) {
    PrefixScene scene(std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);
    CHECK(Fixture().size()==3*kRowsPerEdition);std::size_t count=0;int failures=0;
    // 입력마다 새 HP 타입 표를 연결하되 실제 prefix 몸체는 동일하게 사용한다.
    for (const auto& row:Fixture()) {
        if (row[1]!=edition) continue;
        scene.Prepare(row);RawPriestPostPop prefix(scene.pool,*scene.hp,scene.state,scene.Hooks());prefix.Prefix(kSource,Number(row[3]));
        const bool same=scene.List()==row[15] && scene.Owners()==row[16] && scene.Events()==row[17] && Hex(scene.pool.Slot(kSource))==row[18];
        CHECK(same);
        if (!same) {
            std::printf("  %s postPop 행 %zu: 기대 %s / 실제 %s\n",edition,count,row[17].c_str(),scene.Events().c_str());
            if (++failures>=6) break;
        }
        ++count;
    }
    CHECK(failures==0 && count==kRowsPerEdition && !scene.store.Initialized());
}
}
// 10.78의 목록/할당 분기와 signed HP 계산을 독립 관찰과 대조한다.
TEST_CASE(priest_postpop_patch_prefix_x86) { Replay("originals"); }
// CD의 inline 목록과 소유자 쓰기 helper도 실제 관찰과 대조한다.
TEST_CASE(priest_postpop_cd_prefix_x86) { Replay("originalCD"); }
// 추가 10.37 PE의 관찰은 별도 판본으로 재생한다.
TEST_CASE(priest_postpop_1037_prefix_x86) { Replay("original1037"); }

// raw 타입/가상 표/목록 오류와 누락된 효과를 효과 전에 거부한다.
TEST_CASE(priest_postpop_prefix_rejects_invalid_bindings_before_effects) {
    // 두 판본의 같은 보호 정책을 확인한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        PrefixScene scene(edition);scene.Prepare(Fixture().front());
        CHECK(Throws([&] { RawPriestPostPop invalid(scene.pool,*scene.hp,scene.state,{}); }));
        auto hooks=scene.Hooks();hooks.reserveRegular={};
        CHECK(Throws([&] { RawPriestPostPop invalid(scene.pool,*scene.hp,scene.state,hooks); }));
        SidPool other(edition,kCapacity,false);
        CHECK(Throws([&] { RawPriestPostPop invalid(other,*scene.hp,scene.state,scene.Hooks()); }));
        RawPriestPostPop prefix(scene.pool,*scene.hp,scene.state,scene.Hooks());
        // 실패 입력은 raw 슬롯·목록·소유자 칸과 호출 기록을 바꾸지 않아야 한다.
        const auto reject=[&](Sid sid) {
            const auto before=Hex(scene.pool.Slot(kSource)),list=scene.List(),owners=scene.Owners();
            CHECK(Throws([&] { prefix.Prefix(sid,1); }));
            CHECK(Hex(scene.pool.Slot(kSource))==before && scene.List()==list && scene.Owners()==owners && scene.events.empty());
        };
        reject(Sid{0});reject(Sid{51});scene.pool.AllocatedBytes(kSource)[10]=82;reject(kSource);
        scene.pool.AllocatedBytes(kSource)[10]=kPriestType;Put(scene.pool.AllocatedBytes(kSource),0,kVtable);reject(kSource);
        Put(scene.pool.AllocatedBytes(kSource),0,scene.patch ? kPatchPriestVtable : kCdPriestVtable);
        scene.state.priests.count=5;reject(kSource);
    }
}

// 확보 실패에서는 HP assert도 읽지 않지만 목록과 소유자 상태 쓰기는 계속 진행한다.
TEST_CASE(priest_postpop_allocation_failure_skips_hp_lookup_and_creation) {
    PrefixScene scene(OriginalEdition::Patch1078);auto row=Fixture().front();
    row[3]="1";row[5]="0";row[6]="1";row[7]="43905";row[10]="0";row[11]="0";row[12]="4";row[13]="0";
    scene.Prepare(row);scene.hpState.debugAsserts=true;scene.types[kPriestType].flags1=0;
    scene.hp=std::make_unique<SquidReward>(scene.pool,scene.types,scene.bookkeeping,scene.hpState,&scene.store);
    CHECK(Throws([&] { scene.hp->TypeHitPoints(kPriestType); }));
    RawPriestPostPop prefix(scene.pool,*scene.hp,scene.state,scene.Hooks());prefix.Prefix(kSource,1);
    CHECK(scene.state.priests.count==1 && scene.state.priests.entries[0]==kSource.value && scene.state.ownerState[1]==0);
    CHECK(scene.Events()=="F:50:602:1;A:40");
    // 이미 있으면 확보/HP 조회도 하지 않는다.
    scene.events.clear();scene.found=true;prefix.Prefix(kSource,1);CHECK(scene.Events()=="F:50:602:1" && scene.state.priests.count==1);
}

// 실제 사제 생성자→공통/사제 소유자→prefix→ProcessForm/Kernel의 부착·중복 방지·재예약을 검사한다.
TEST_CASE(priest_postpop_prefix_binds_real_process_host_and_owner) {
    // 실제 두 슬롯 배치와 server/client 생성 경로를 각각 합성한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) for (const bool server:{false,true}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,32768,server);
        std::vector<RiftTypeRecord> types(patch ? 188 : 171);auto& type=types[kPriestType];
        type.constructorAddress=TypeConstructorAddress(edition,kPriestType);type.flags1=0x00069012;type.flags2=0x00210000;type.maxHitPoints=200;
        SquidFactory factory(pool,types);const auto priest=factory.Create(kPriestType,server ? 0U : 2U);
        SquidPostPopState bookkeeping;SquidOwnerMode mode;mode.battle=true;PriestOwnerState ownerState{1,1,1};
        SquidOwner owner(pool,types,bookkeeping,mode);
        RawPriestOwner setOwner(pool,mode,ownerState,{[&](Sid sid,std::uint32_t player) { owner.Set(sid,player); },[](Sid) {}});
        setOwner.Set(priest,1);
        // 공간 Pop이 아직 사제 전용 효과를 지원하지 않으므로 여기서는 위치/void 준비만 명시적으로 한다.
        pool.AllocatedBytes(priest)[11]=0;GameRandom rng;ScrambledSpStore store(rng,[] { return 1U; });SquidRewardState hpState;
        SquidReward hp(pool,types,bookkeeping,hpState,patch ? &store : nullptr);
        SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);
        Kernel kernel;SquidProcessState processState;processState.now=11.5;std::vector<float> payloads;
        SquidProcessHost host(pool,types,kernel,destroy,processState,
            [&](Sid sid,std::uint32_t event,std::uint32_t count,float payload) {
                CHECK(sid==priest && event==kPriestRegenEvent && count==0);payloads.push_back(payload);return 2.0f;
            });
        PriestPostPopState state;state.priests.entries.resize(2);state.ownerState.fill(17);
        RawPriestPostPop prefix(pool,hp,state,MakePriestPostPopProcessHooks(host));const auto beforeFree=pool.FreeCount();
        prefix.Prefix(priest,1);auto* regen=host.FindEvent(priest,kPriestRegenEvent);CHECK(regen!=nullptr);
        CHECK(state.priests.count==1 && state.priests.entries[0]==priest.value && state.ownerState[1]==0 && state.ownerState[0]==17);
        CHECK(kernel.Size()==1 && pool.FreeCount()==beforeFree-1 && regen->Parent()==priest);
        CHECK(regen->Payload()==8.25f && regen->Time()==11.5 && regen->Count()==0);
        CHECK(pool.Slot(regen->Form())[10]==46 && (pool.Slot(regen->Form())[11]&8)!=0);
        CHECK(Get(pool.Slot(priest),6,2)==regen->Form().value && Get(pool.Slot(regen->Form()),16,2)==priest.value);
        // 중복 Pop은 추가 form을 만들지 않고 로컬 상태만 다시 초기화한다.
        state.ownerState[1]=99;prefix.Prefix(priest,0x801);
        CHECK(host.FindEvent(priest,kPriestRegenEvent)==regen && kernel.Size()==1 && state.priests.count==1 && state.ownerState[1]==0);
        kernel.RunFrame();CHECK(payloads==std::vector<float>({8.25f}) && regen->Count()==1 && regen->Time()==13.5);
        kernel.RunFrame();CHECK(payloads.size()==1);
        // 원본 HP 모드가 바뀌어도 기존 예약은 덮지 않으며, 삭제 뒤 재생성할 때 새 값이 반영된다.
        hpState.priestQuarterHp=true;prefix.Prefix(priest,1);CHECK(regen->Payload()==2.0f);
        host.Kill(*regen,0x10);CHECK(kernel.Size()==0 && host.FindEvent(priest,kPriestRegenEvent)==nullptr && Get(pool.Slot(priest),6,2)==0);
        prefix.Prefix(priest,0x40);CHECK(kernel.Size()==0);
        prefix.Prefix(priest,1);regen=host.FindEvent(priest,kPriestRegenEvent);
        CHECK(regen && regen->Payload()==8.0f && kernel.Size()==1 && state.priests.count==1);
        host.Kill(*regen,0x10);CHECK(kernel.Size()==0 && pool.FreeCount()==beforeFree && !store.Initialized());
    }
}
