// 원본 낙하 요청/0x25b 관찰과 실제 공유 Form·Kernel·보호막·회복 분배 결합을 검사한다.
#include "RawSceneSupport.h"
#include "o/RawPriestFall.h"
#include "o/RawPriestRegen.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 판본마다 중복 없는 요청/이벤트 입력 수다. 두 정밀도는 원본 생성기가 따로 대조한다.
constexpr std::size_t kRows=456;
// C++ 일반 검사에서는 PE나 Python을 실행하지 않고 저장한 원본 관찰을 읽는다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto fixture=LoadFixture(NETSTORM_PRIESTFALL_FIXTURE);return fixture.rows;
}
// 동일 월드의 raw 풀·상태·프레임·지도와 명시 외부 효과 입력을 소유한다.
struct FallScene {
    SidPool pool;
    GameRandom random;
    ScrambledSpStore store;
    SquidPostPopState book;
    SquidRewardState hpMode;
    std::vector<RiftTypeRecord> types;
    std::vector<RiftTypeFrames> frames;
    std::vector<PriestFallFrames> choices;
    std::vector<std::uint8_t> spots;
    std::vector<std::uint16_t> surfaces;
    std::unique_ptr<SquidReward> hp;
    PriestFallState mode;
    std::vector<std::string> events;
    bool patch{},begin{},allocated{};
    std::uint32_t change{};
    // 예약 번호 5부터 원본 관찰 SID 50까지 실제 풀로 확보한다.
    explicit FallScene(OriginalEdition edition):pool(edition,kCapacity,false),store(random,[] { return 1U; }),
        types(edition==OriginalEdition::Patch1078 ? 188 : 171),frames(types.size(),RiftTypeFrames({})),
        choices(types.size()),spots(65536),surfaces(65536),patch(edition==OriginalEdition::Patch1078) {
        // 서버/클라이언트 번호 정책을 재사용한다.
        for (std::uint16_t id=5;id<=kSource.value;++id) CHECK(pool.Allocate(2)==Sid{id});
    }
    // 입력 한 행을 복구한다. 기대값이나 분기 결과는 계산하지 않는다.
    void Prepare(const std::vector<std::string>& row) {
        CHECK(row.size()==16);events.clear();begin=row[1]=="Begin";mode.authority=Number(row[2])!=0;
        allocated=Number(row[3])!=0;change=Number(row[9]);types[kPriestType].maxHitPoints=200;
        types[kPriestType].flags1=0x69012;types[kPriestType].flags2=0x210000;
        hp=std::make_unique<SquidReward>(pool,types,book,hpMode,patch ? &store : nullptr);
        frames[kPriestType]=RiftTypeFrames({{static_cast<std::uint8_t>(Number(row[7])),80,1,0},{74,80,1,0}});
        choices[kPriestType]={3,4};std::fill(spots.begin(),spots.end(),static_cast<std::uint8_t>(Number(row[6])));
        std::fill(surfaces.begin(),surfaces.end(),static_cast<std::uint16_t>(Number(row[8])));
        auto raw=pool.AllocatedBytes(kSource);std::fill(raw.begin(),raw.end(),std::uint8_t{});
        Put(raw,0,patch ? kPatchPriestVtable : kCdPriestVtable);raw[10]=kPriestType;raw[patch ? 34 : 32]=1;
        raw[patch ? 40 : 35]=static_cast<std::uint8_t>(Number(row[4]));Put(raw,26,Number(row[5]),patch ? 4 : 2);
        Put(raw,14,Number(row[10]));Put(raw,18,Number(row[11]));
    }
    // 원본 실행기에 공급한 외부 효과의 입력 변이만 수행한다.
    void Change(std::string_view stage) {
        auto raw=pool.AllocatedBytes(kSource);
        if (begin) {
            if ((stage=="shield" && change==1) || (stage=="new" && change==2)) mode.authority=!mode.authority;
            if (stage=="repop" && change==3) { Put(raw,14,std::bit_cast<std::uint32_t>(30.75f));Put(raw,18,std::bit_cast<std::uint32_t>(31.9f)); }
            return;
        }
        if (stage=="shield" && change==1) Put(raw,patch ? 36 : 34,1,patch ? 4 : 1);
        if ((stage=="set" || stage=="advance") && (change==2 || change==4)) {
            Put(raw,14,std::bit_cast<std::uint32_t>(30.75f));Put(raw,18,std::bit_cast<std::uint32_t>(31.9f));
        }
        if (stage=="land" && (change==3 || change==4)) { raw[patch ? 40 : 35]=0;Put(raw,26,150,patch ? 4 : 2); }
    }
    // 원본의 명시 경계 인자를 기록한다. 실제 낙하 조건/반환은 생산 코드가 결정한다.
    PriestFallHooks Hooks() {
        return {[this](Sid sid) { events.push_back("S:"+std::to_string(sid.value));Change("shield"); },
            [this] { events.push_back("A:40");Change("new");return allocated; },
            [this](Sid sid,std::uint32_t event,float payload) {
                events.push_back("R:"+std::to_string(sid.value)+':'+std::to_string(event)+':'+std::to_string(std::bit_cast<std::uint32_t>(payload)));
            },[this](Sid sid,std::uint32_t flags) { events.push_back("U:"+std::to_string(sid.value)+':'+std::to_string(flags)); },
            [this](Sid sid,std::uint32_t flags) { events.push_back("P:"+std::to_string(sid.value)+':'+std::to_string(flags));Change("repop"); },
            [this](Sid sid,float x,float y) {
                events.push_back("W:"+std::to_string(sid.value)+':'+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y)));
            },[this](Sid sid,std::int32_t frame,std::uint32_t flags) {
                events.push_back("F:"+std::to_string(sid.value)+':'+std::to_string(frame)+':'+std::to_string(flags));Change(frame==4 ? "land" : "set");
            },[this](Sid sid,std::uint32_t steps,std::uint32_t flags) {
                events.push_back("N:"+std::to_string(sid.value)+':'+std::to_string(steps)+':'+std::to_string(flags));Change("advance");
            },[this](Sid sid) { events.push_back("X:"+std::to_string(sid.value)); }};
    }
    // 호출 순서를 원본 fixture와 같은 문자열로 합친다.
    std::string Events() const {
        std::string text;
        // 모든 효과를 발생한 순서대로 보존한다.
        for (const auto& event:events) { if (!text.empty()) text+=';';text+=event; }
        return text.empty() ? "-" : text;
    }
};
// 같은 판본의 독립 기대값과 모든 사건·raw 슬롯·반환 비트·현재 권한을 대조한다.
void Replay(const char* edition) {
    FallScene scene(std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);
    std::size_t count=0;CHECK(Fixture().size()==kRows*3);
    // 기대값을 계산하지 않고 입력별 생산 모듈을 실행한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;scene.Prepare(row);
        RawPriestState state(scene.pool,*scene.hp,scene.types,scene.spots);
        RawPriestFall fall(scene.pool,state,scene.frames,scene.choices,scene.surfaces,scene.mode,scene.Hooks());
        std::string result="-";
        if (scene.begin) fall.Begin(kSource);
        else result=std::to_string(std::bit_cast<std::uint32_t>(fall.Handle(kSource,kPriestFallEvent,0xabcdef01,std::bit_cast<float>(0x7fc12345U))));
        const bool same=scene.Events()==row[12] && Hex(scene.pool.Slot(kSource))==row[13] && result==row[14] && scene.mode.authority==(Number(row[15])!=0);
        CHECK(same);++count;
        if (!same) { std::printf("%s 낙하 행 %zu: 기대 %s / 실제 %s\n",edition,count,row[12].c_str(),scene.Events().c_str());break; }
    }
    CHECK(count==kRows && !scene.store.Initialized());
}
}
// 패치 낙하 요청/실제 공유 생성/이벤트 처리 관찰을 재생한다.
TEST_CASE(PriestFall_ReplaysOriginals) { Replay("originals"); }
// CD의 inline 지도·프레임 BYTE와 보호막 clear 경계를 재생한다.
TEST_CASE(PriestFall_ReplaysCd) { Replay("originalCD"); }
// 추가 10.37 실행 파일의 독립 결과도 재생한다.
TEST_CASE(PriestFall_ReplaysExtra1037) { Replay("original1037"); }

// 실제 공유 타입/체인/검색·Kernel 재예약/삭제와 실제 보호막 생성/소유자/조회 효과를 합성한다.
TEST_CASE(PriestFall_ComposesSharedProcessAndShieldLifetime) {
    // 두 raw 배치의 실제 풀을 각각 사용한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        FallScene scene(edition);scene.Prepare(Fixture().front());scene.begin=false;scene.change=0;
        scene.types[167].footX=scene.types[167].footY=1;
        std::fill(scene.spots.begin(),scene.spots.end(),std::uint8_t{6});
        auto raw=scene.pool.AllocatedBytes(kSource);Put(raw,14,std::bit_cast<std::uint32_t>(20.75f));Put(raw,18,std::bit_cast<std::uint32_t>(21.9f));
        SquidHash hash;SquidUnpop unpop(scene.pool,hash,scene.spots);RawSquidDestroy destroy(scene.pool,unpop,scene.types);
        Kernel kernel;SquidProcessState processMode;processMode.now=12.5;
        std::unique_ptr<RawPriestFall> fall;
        SquidProcessHost host(scene.pool,scene.types,kernel,destroy,processMode,
            [&](Sid sid,std::uint32_t event,std::uint32_t count,float payload) { return MakePriestFallHandler(scene.pool,*fall)(sid,event,count,payload); });
        SquidFactory factory(scene.pool,scene.types);SquidOwnerMode ownerMode;SquidOwner owner(scene.pool,scene.types,scene.book,ownerMode);
        PriestForcefieldState fieldMode;RawPriestForcefield lookup(scene.pool,hash,scene.types,fieldMode);
        PriestShieldState shieldMode{1,1,0};Sid shieldSid;int creates=0,clears=0;
        auto effects=MakePriestShieldCreationHooks(scene.pool,factory,owner,{{},{},[&](Sid sid,float x,float y,std::uint32_t flags) {
            CHECK(flags==0);shieldSid=sid;++creates;auto born=scene.pool.AllocatedBytes(sid);
            // 보호막 Pop 경계가 실제 finder 입력을 등록한다.
            Put(born,14,std::bit_cast<std::uint32_t>(x));Put(born,18,std::bit_cast<std::uint32_t>(y));born[11]=0;
            auto& bucket=hash.Bucket(0,x,y);Put(born,4,bucket,2);bucket=sid.value;
        },[] { return false; },[](std::string_view) { return 0U; },[](Sid,std::uint32_t,std::uint32_t,std::uint32_t) {},
          [](const PriestShieldNotice&) {},[](std::string_view) { return std::string{}; },[](std::string_view) {}});
        RawPriestShield shield(scene.pool,lookup,shieldMode,effects);RawPriestState state(scene.pool,*scene.hp,scene.types,scene.spots);
        auto hooks=MakePriestFallHooks(host,shield,lookup,[&](Sid sid,std::uint32_t flags) {
            CHECK(sid==shieldSid && flags==0);++clears;hash.Bucket(0,20.75f,21.9f)=0;
            scene.pool.AllocatedBytes(sid)[11]=4;scene.pool.Release(sid);
        },scene.Hooks());
        hooks.setFrame=[&](Sid sid,std::int32_t frame,std::uint32_t flags) {
            CHECK(flags==0);auto current=scene.pool.AllocatedBytes(sid);
            // 미복원 공간/프레임 효과의 입력이다. 도착 때 J 방향을 풀고 현재 HP를 회복한다.
            Put(current,scene.patch ? 36 : 34,frame==3 ? 1 : 0,scene.patch ? 4 : 1);
            if (frame==4) { current[scene.patch ? 40 : 35]=0;Put(current,26,150,scene.patch ? 4 : 2); }
        };
        fall=std::make_unique<RawPriestFall>(scene.pool,state,scene.frames,scene.choices,scene.surfaces,scene.mode,std::move(hooks));
        fall->Begin(kSource);auto* process=host.FindEvent(kSource,kPriestFallEvent);
        CHECK(process && kernel.Size()==1 && scene.pool.Slot(process->Form())[10]==61 && creates==1 && lookup.Find(kSource)==shieldSid);
        CHECK(process->Time()==12.5 && process->Count()==0 && process->Payload()==0.01f);
        // 낙하 요청은 기존 이벤트를 검색하지 않는다. 가장 새 공유 form을 지워도 이전 예약이 남는다.
        processMode.now=13.0;fall->Begin(kSource);auto* newest=host.FindEvent(kSource,kPriestFallEvent);
        CHECK(newest && newest!=process && newest->Time()==13.0 && kernel.Size()==2 && creates==1);
        host.Kill(*newest,0x10);CHECK(kernel.Size()==1 && host.FindEvent(kSource,kPriestFallEvent)==process);
        processMode.now=12.5;
        kernel.RunFrame();process=host.FindEventMasked(kSource,0xffff,kPriestFallEvent);
        CHECK(process && process->Count()==1 && process->Payload()==0.1f && process->Time()==12.5+static_cast<double>(0.1f));
        processMode.now=process->Time()-0.000001;kernel.RunFrame();CHECK(process->Count()==1 && creates==1);
        processMode.now=process->Time();std::fill(scene.surfaces.begin(),scene.surfaces.end(),std::uint16_t{65535});kernel.RunFrame();
        CHECK(kernel.Size()==0 && !host.FindEvent(kSource,kPriestFallEvent) && clears==1 && lookup.Find(kSource)==Sid{});
    }
}

// 기존 회복 분배와 fallback을 합성하며 사제의 미복원 사건/잘못된 풀/미연결 훅을 거부한다.
TEST_CASE(PriestFall_ComposesRegenDispatcherAndRejectsInvalidBindings) {
    FallScene scene(OriginalEdition::Patch1078);scene.Prepare(Fixture().front());
    RawPriestState state(scene.pool,*scene.hp,scene.types,scene.spots);
    RawPriestFall fall(scene.pool,state,scene.frames,scene.choices,scene.surfaces,scene.mode,scene.Hooks());
    PriestHitPointMode hpMode;PriestRegenState regenMode;
    RawPriestRegen regen(scene.pool,*scene.hp,state,hpMode,{[](Sid,std::uint32_t) {},[](Sid,std::uint32_t) {}},scene.random,regenMode,
        {[](Sid) {},[](Sid) {},[](std::uint32_t) { return 0U; },[](Sid,std::uint32_t) { return true; },[](Sid) {},[] {},[] { return 0.0; }});
    auto handler=MakePriestRegenHandler(scene.pool,regen,MakePriestFallHandler(scene.pool,fall));
    CHECK(handler(kSource,kPriestFallEvent,42,-7.0f)==0.1f);
    CHECK(handler(kSource,kPriestRegenEvent,0,2.0f)==2.0f);
    CHECK(Throws([&] { handler(kSource,0x25c,0,1.0f); }));
    CHECK(Throws([&] { fall.Handle(kSource,0x25a,0,1.0f); }));
    CHECK(Throws([&] { RawPriestFall invalid(scene.pool,state,{},scene.choices,scene.surfaces,scene.mode,scene.Hooks()); }));
    CHECK(Throws([&] { RawPriestFall invalid(scene.pool,state,scene.frames,scene.choices,scene.surfaces,scene.mode,{}); }));
    SidPool other(OriginalEdition::Patch1078,kCapacity,false);
    CHECK(Throws([&] { MakePriestFallHandler(other,fall); }));
}
