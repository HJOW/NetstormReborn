// 실제 원본 0x25a 관찰·HP 변경·원본 생성자/ProcessForm/Kernel 합성을 검사한다.
#include "RawSceneSupport.h"
#include "o/RawPriestRegen.h"
#include "o/SquidFactory.h"
#include <limits>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원본/Python/Ghidra 없이 읽는 독립 기계어 관찰이다.
const auto& Fixture() { static const auto data=LoadFixture(NETSTORM_PRIESTREGEN_FIXTURE);return data.rows; }
struct RegenScene {
    SidPool pool;
    GameRandom random;
    ScrambledSpStore store;
    SquidPostPopState book;
    SquidRewardState hpMode;
    std::vector<RiftTypeRecord> types;
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536);
    std::unique_ptr<SquidReward> hp;
    std::unique_ptr<RawPriestState> priest;
    std::unique_ptr<RawPriestRegen> regen;
    PriestHitPointMode mode;
    PriestRegenState state;
    bool patch,occupied{},auditChange{};
    std::string events,after;
    std::uint8_t afterOwner{};
    double measurement{};
    // 관찰의 client SID를 실제 풀에서 할당한다.
    explicit RegenScene(OriginalEdition edition):pool(edition,kCapacity,false),store(random,[] { return 1U; }),
        types(edition==OriginalEdition::Patch1078 ? 188 : 171),patch(edition==OriginalEdition::Patch1078) {
        // 각 관찰이 같은 SID와 stale 슬롯 내용으로 시작한다.
        for (std::uint16_t id=5;id<=kSource.value;++id) CHECK(pool.Allocate(2)==Sid{id});
    }
    // 외부 호출 순서에 구분자를 붙인다.
    void Event(std::string value) { if (!events.empty()) events+=';';events+=value; }
    // 기록 대체는 호출 시점의 현재 raw 값을 관찰한다.
    void Record(char marker,Sid sid) {
        const auto raw=pool.Slot(sid);
        Event(std::string(1,marker)+':'+std::to_string(sid.value)+':'+std::to_string(priest->CurrentHitPoints(sid))+':'+
            std::to_string(raw[patch ? 34 : 32])+':'+std::to_string(Get(raw,14))+':'+std::to_string(Get(raw,18)));
        if (marker=='V' && after!="-") {
            auto target=pool.AllocatedBytes(sid);Put(target,26,static_cast<std::uint32_t>(std::stoi(after)),patch ? 4 : 2);
            target[patch ? 34 : 32]=afterOwner;
        }
    }
    // StateOracle와 같은 U/R 호출 기록이며 여기서 외부 raw 변화는 V에만 공급한다.
    PriestHitPointHooks Space() {
        // 각 가상 공간 진입 시 HP/좌표를 읽는다.
        const auto record=[this](char marker,Sid sid,std::uint32_t flags) {
            const auto raw=pool.Slot(sid);
            Event(std::string(1,marker)+':'+std::to_string(sid.value)+':'+std::to_string(flags)+':'+
                std::to_string(priest->CurrentHitPoints(sid))+':'+std::to_string(Get(raw,14))+':'+std::to_string(Get(raw,18)));
        };
        return {[record](Sid sid,std::uint32_t flags) { record('U',sid,flags); },
            [record](Sid sid,std::uint32_t flags) { record('R',sid,flags); }};
    }
    // 실제 공간/표시가 아직 없는 경계를 독립 실행기의 입력과 같은 방식으로 공급한다.
    PriestRegenHooks Hooks() {
        return {[this](Sid sid) { Record('V',sid); },[this](Sid sid) { Record('D',sid); },
            [this](std::uint32_t type) { Event("F:"+std::to_string(type));return 0x12345678U; },
            [this](Sid sid,std::uint32_t shape) { Event("O:"+std::to_string(sid.value)+':'+std::to_string(shape)+":0:0");return occupied; },
            [this](Sid sid) { Record('N',sid); },
            [this] { Event("A");if (auditChange) { state.patch.observedSequence=state.patch.sequence;state.patch.remaining=1; } },
            [this] { Event("M");return measurement; }};
    }
    // fixture는 입력만 공급한다. HP 계산/분기 기대값은 준비 코드에 없다.
    void Prepare(const std::vector<std::string>& row) {
        CHECK(row.size()==31);events.clear();auto& type=types[kPriestType];
        type.maxHitPoints=std::stoi(row[1]);type.flags1=0x69012;type.flags2=0x210000;hpMode.priestQuarterHp=Number(row[2])!=0;
        hp=std::make_unique<SquidReward>(pool,types,book,hpMode,patch ? &store : nullptr);
        auto raw=pool.AllocatedBytes(kSource);std::fill(raw.begin(),raw.end(),std::uint8_t{0xab});
        Put(raw,0,patch ? kPatchPriestVtable : kCdPriestVtable);raw[10]=kPriestType;raw[11]=0;
        Put(raw,26,Number(row[3]),patch ? 4 : 2);raw[patch ? 34 : 32]=static_cast<std::uint8_t>(Number(row[6]));
        raw[patch ? 40 : 35]=static_cast<std::uint8_t>(Number(row[7]));Put(raw,14,Number(row[8]));Put(raw,18,Number(row[9]));
        priest=std::make_unique<RawPriestState>(pool,*hp,types,spots);mode.authority=Number(row[4])!=0;
        mode.recoveryAllowed.fill(false);mode.recoveryAllowed[Number(row[6])]=Number(row[5])!=0;
        state.neutralBlocked=Number(row[10])!=0;state.modeBlocked=Number(row[11])!=0;occupied=Number(row[12])!=0;
        random.SetState(Number(row[15]));state.patch={0,std::stoi(row[16]),std::stoi(row[17]),Number(row[18]),Number(row[19]),Number(row[20]),static_cast<std::int16_t>(std::stoi(row[21]))};
        measurement=std::stod(row[22]);after=row[23];afterOwner=static_cast<std::uint8_t>(Number(row[24]));auditChange=Number(row[25])!=0;
        regen=std::make_unique<RawPriestRegen>(pool,*hp,*priest,mode,Space(),random,state,Hooks());
    }
    // 추적 sequence는 signed 판단 뒤 raw DWORD의 출력 비트로 비교한다.
    std::string Tracking() const {
        const auto& t=state.patch;
        return std::to_string(t.sample)+','+std::to_string(static_cast<std::uint32_t>(t.sequence))+','+
            std::to_string(static_cast<std::uint32_t>(t.observedSequence))+','+std::to_string(t.changedSample)+','+
            std::to_string(t.remaining)+','+std::to_string(t.gate)+','+std::to_string(t.sentinel);
    }
};
// 해당 판본의 반환 float 비트/사건/전역 난수/추적/raw 슬롯 전체를 재생한다.
void Replay(std::string_view edition) {
    RegenScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;
    // 입력마다 원본 관찰과 같은 독립 초기 상태를 만든다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;
        scene.Prepare(row);const auto before=netstorm::test::FailureCount();
        const auto result=scene.regen->Handle(kSource,kPriestRegenEvent,Number(row[13]),std::bit_cast<float>(Number(row[14])));
        CHECK(std::bit_cast<std::uint32_t>(result)==Number(row[26]));CHECK((scene.events.empty() ? "-" : scene.events)==row[27]);
        CHECK((scene.patch ? scene.Tracking() : "-")==row[28]);CHECK(scene.random.State()==Number(row[29]));
        CHECK(Hex(scene.pool.Slot(kSource))==row[30]);
        if (netstorm::test::FailureCount()!=before) {
            std::printf("회복 %s 행 %zu: 기대 %s / 실제 %s\n",std::string(edition).c_str(),count,row[27].c_str(),scene.events.c_str());break;
        }
        ++count;
    }
    CHECK(count==1513 && !scene.store.Initialized());
}
}
// 패치 전용 난수/추적과 실제 HP setter를 원본 전체 회복 분기에 대조한다.
TEST_CASE(priest_regen_patch_x86) { Replay("originals"); }
// CD signed WORD 및 중립 helper의 다른 명령 구현을 대조한다.
TEST_CASE(priest_regen_cd_x86) { Replay("originalCD"); }
// 추가 10.37 관찰을 독립 PE 근거로 재생한다.
TEST_CASE(priest_regen_1037_x86) { Replay("original1037"); }

// 빠진 외부 효과/풀/미지원 사건과 잘못된 vtable은 회복 부작용 전에 거부한다.
TEST_CASE(priest_regen_binding_and_dispatch_guards) {
    RegenScene scene(OriginalEdition::Patch1078);scene.Prepare(Fixture().front());
    auto hooks=scene.Hooks();hooks.patchAudit={};
    CHECK(Throws([&] { RawPriestRegen invalid(scene.pool,*scene.hp,*scene.priest,scene.mode,scene.Space(),scene.random,scene.state,hooks); }));
    SidPool other(OriginalEdition::Patch1078,kCapacity,false);
    CHECK(Throws([&] { MakePriestRegenHandler(other,*scene.regen); }));
    const auto before=Hex(scene.pool.Slot(kSource));const auto rng=scene.random.State();const auto tracking=scene.Tracking();
    auto handler=MakePriestRegenHandler(scene.pool,*scene.regen);
    CHECK(Throws([&] { handler(kSource,0x25b,0,1); }));
    CHECK(Hex(scene.pool.Slot(kSource))==before && scene.random.State()==rng && scene.Tracking()==tracking && scene.events.empty());
    auto fallback=MakePriestRegenHandler(scene.pool,*scene.regen,[](Sid,std::uint32_t,std::uint32_t,float) { return 3.0f; });
    CHECK(fallback(kSource,0x25b,0,1)==3.0f);
    Put(scene.pool.AllocatedBytes(kSource),0,kVtable);const auto wrong=Hex(scene.pool.Slot(kSource));
    CHECK(Throws([&] { scene.regen->Handle(kSource,kPriestRegenEvent,0,1); }));CHECK(Hex(scene.pool.Slot(kSource))==wrong && scene.random.State()==rng);
    CHECK(handler(kSource,0x25b,0,2)==2);
}

// 실제 생성자와 Regular/Kernel을 연결해 HP 회복과 절반 경계의 재등록 호출을 확인한다.
TEST_CASE(priest_regen_binds_factory_postpop_process_and_kernel) {
    // 두 판본의 server/client 생성자를 각각 확인한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) for (bool server:{false,true}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,32768,server);
        std::vector<RiftTypeRecord> types(patch ? 188 : 171);auto& type=types[kPriestType];
        type.constructorAddress=TypeConstructorAddress(edition,kPriestType);type.flags1=0x69012;type.flags2=0x210000;type.maxHitPoints=200;
        SquidFactory factory(pool,types);const auto sid=factory.Create(kPriestType,server ? 0U : 2U);
        auto raw=pool.AllocatedBytes(sid);raw[11]=0;raw[patch ? 34 : 32]=1;Put(raw,26,80,patch ? 4 : 2);
        Put(raw,14,std::bit_cast<std::uint32_t>(20.75f));Put(raw,18,std::bit_cast<std::uint32_t>(21.9f));
        GameRandom random;ScrambledSpStore store(random,[] { return 1U; });SquidPostPopState book;SquidRewardState hpMode;
        SquidReward hp(pool,types,book,hpMode,patch ? &store : nullptr);std::vector<std::uint8_t> spots(65536,6);
        RawPriestState priest(pool,hp,types,spots);PriestHitPointMode mode;mode.recoveryAllowed.fill(true);PriestRegenState state;
        int unpops=0,repops=0,refreshes=0,damageables=0,audits=0;
        RawPriestRegen regen(pool,hp,priest,mode,{[&](Sid s,std::uint32_t flags) { CHECK(s==sid && flags==0);++unpops; },
            [&](Sid s,std::uint32_t flags) { CHECK(s==sid && flags==0);++repops; }},random,state,
            {[&](Sid s) { CHECK(s==sid);++refreshes; },[&](Sid s) { CHECK(s==sid);++damageables; },
            [](std::uint32_t) { return 1U; },[](Sid,std::uint32_t) { return false; },[](Sid) {},[&] { ++audits; },[] { return 0.0; }});
        SquidHash hash;SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);
        Kernel kernel;SquidProcessState processState;processState.now=11.5;
        SquidProcessHost host(pool,types,kernel,destroy,processState,MakePriestRegenHandler(pool,regen));
        PriestPostPopState list;list.priests.entries.resize(2);RawPriestPostPop prefix(pool,hp,list,MakePriestPostPopProcessHooks(host));
        const auto free=pool.FreeCount();prefix.Prefix(sid,1);auto* regular=host.FindEvent(sid,kPriestRegenEvent);
        CHECK(regular && regular->Payload()==8.25f && regular->Time()==11.5 && kernel.Size()==1);
        kernel.RunFrame();CHECK(priest.CurrentHitPoints(sid)==113 && regular->Count()==1 && regular->Time()==19.75);
        CHECK(unpops==1 && repops==1 && refreshes==1 && damageables==1);kernel.RunFrame();CHECK(regular->Count()==1);
        // 원본 예약 시각에 맞춰 최대 HP까지 진행하고 가득 찬 뒤에도 예약이 계속되는지 확인한다.
        for (int tick=0;tick<4;++tick) { processState.now=regular->Time();kernel.RunFrame(); }
        CHECK(priest.CurrentHitPoints(sid)==200 && regular->Count()==5 && regular->Payload()==8.25f);
        CHECK(refreshes==4 && damageables==4 && unpops==1 && repops==1 && audits==(patch ? 5 : 0));
        host.Kill(*regular,0x10);CHECK(kernel.Size()==0 && pool.FreeCount()==free && !store.Initialized());
    }
}
