// 실제 사제 Pop/Activate의 원본 관찰과 회복/낙하/프레임/공간의 명시 분배 연결을 검사한다.
#include "RawSceneSupport.h"
#include "o/RawPriestPostPopTail.h"
#include "o/RawPriestFall.h"
#include "o/RawCarrierCheck.h"
#include "o/RawFrameAdvance.h"
#include "o/SquidFrameBinding.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원본 관찰의 실제 SID와 판본마다 중복 없는 입력 행 수다.
constexpr Sid kObject{50};
constexpr std::size_t kRows=1280;
// CTest는 원본 PE/Python 없이 저장된 독립 관찰을 읽는다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto data=LoadFixture(NETSTORM_PRIESTPOP_FIXTURE);return data.rows;
}
// fixture와 같은 입력/경계를 가지며 일반 Pop은 실제 모듈로 실행하는 장면이다.
struct PriestPopScene {
    SidPool pool;GameRandom random;ScrambledSpStore store;SquidPostPopState book;SquidRewardState hpMode;
    std::vector<RiftTypeRecord> types;std::vector<RiftTypeFrames> frames;std::vector<std::uint8_t> spots;
    PriestPostPopState priests;SquidHash hash;bool patch;std::vector<std::string> events;
    // 원본 client SID와 사제 타입/HP/비용 입력을 준비한다.
    explicit PriestPopScene(OriginalEdition edition):pool(edition,kCapacity,false),store(random,[] { return 1U; }),
        types(edition==OriginalEdition::Patch1078 ? 188 : 171),frames(types.size(),RiftTypeFrames({})),spots(65536),patch(edition==OriginalEdition::Patch1078) {
        // 번호 50까지 실제 client 할당 정책을 유지한다.
        for (std::uint16_t sid=5;sid<=kObject.value;++sid) CHECK(pool.Allocate(2)==Sid{sid});
        types[kPriestType].maxHitPoints=200;types[kPriestType].cost=123;types[kPriestType].flags1=0x69012;
        types[kPriestType].flags2=0x210000;types[kPriestType].footX=types[kPriestType].footY=1;
    }
    // 각 입력은 새로운 void 사제와 빈 공간/장부/목록에서 시작한다.
    void Prepare(const std::vector<std::string>& row) {
        CHECK(row.size()==13);events.clear();hash.Reset();std::fill(spots.begin(),spots.end(),std::uint8_t{});
        spots[21*256+20]=spots[22*256+21]=static_cast<std::uint8_t>(Number(row[4]));
        book={};book.suppressed=true;priests.priests={{60,50,70,80},0};
        // 원본 소유자 표의 stale 초기값도 전체 관찰에 포함한다.
        for (std::uint32_t i=0;i<priests.ownerState.size();++i) priests.ownerState[i]=0xabc000+i;
        frames[kPriestType]=RiftTypeFrames({{static_cast<std::uint8_t>(Number(row[5])),80,1,0}});
        auto raw=pool.AllocatedBytes(kObject);std::fill(raw.begin(),raw.end(),std::uint8_t{});
        Put(raw,0,patch ? kPatchPriestVtable : kCdPriestVtable);raw[10]=kPriestType;raw[11]=4;
        raw[patch ? 34 : 32]=1;raw[patch ? 40 : 35]=static_cast<std::uint8_t>(Number(row[1]));Put(raw,12,1,2);Put(raw,26,Number(row[3]),patch ? 4 : 2);
    }
    // 외부 효과의 원본 순서만 합친다.
    std::string Events() const {
        std::string value;
        // 발생 순서대로 구분자를 넣는다.
        for (const auto& event:events) { if (!value.empty()) value+=';';value+=event; }
        return value.empty() ? "-" : value;
    }
};
// 원본의 전체 Pop/Activate/사제 wrapper 관찰을 실제 C++ 공간/후처리에 대조한다.
void Replay(const char* edition) {
    PriestPopScene scene(std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;
    CHECK(Fixture().size()==kRows*3);
    // 원본에서 얻은 목표 flags/슬롯/지면/목록을 계산하지 않고 각 입력을 실제 모듈에 연결한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;scene.Prepare(row);
        SquidUnpop unpop(scene.pool,scene.hash,scene.spots);SquidPostPop base(scene.pool,scene.types,scene.book);
        SquidReward hp(scene.pool,scene.types,scene.book,scene.hpMode,scene.patch ? &scene.store : nullptr);
        RawPriestState state(scene.pool,hp,scene.types,scene.spots);
        RawPriestPostPop prefix(scene.pool,hp,scene.priests,{
            // 접두가 실제 목록 삽입을 마친 시점의 검색 호출을 관찰한다.
            [&](Sid sid,std::uint32_t event) { scene.events.push_back("F:"+std::to_string(sid.value)+':'+std::to_string(event)+':'+std::to_string(scene.priests.priests.count));return Number(row[6])!=0; },
            // 원본 new 실패 입력은 예약/HP 조회의 분기를 보존한다.
            [&] { scene.events.push_back("A:40");return Number(row[7])!=0; },
            // 생성 내부는 원본 실행기와 같은 경계다. payload 비트를 관찰한다.
            [&](Sid sid,std::uint32_t event,float payload) { scene.events.push_back("R:"+std::to_string(sid.value)+':'+std::to_string(event)+':'+std::to_string(std::bit_cast<std::uint32_t>(payload))); }});
        CarrierPostPopState mode;auto effects=MakeCarrierPostPopHooks(scene.pool,base,[](Sid) { CHECK(false); },[](float,float,std::uint8_t) { CHECK(false); });
        // Carrier의 실제 flags를 기록하고 base 깊이 감소는 실제 공통 몸체로 실행한다.
        effects.base=[&](Sid sid,std::uint32_t flags) { scene.events.push_back("C:"+std::to_string(sid.value)+':'+std::to_string(flags));base.PostPopBase(sid,flags); };
        RawCarrierPostPop carrier(scene.pool,scene.types,mode,std::move(effects));
        RawPriestPostPopTail tail(scene.pool,prefix,carrier,state,scene.frames,scene.spots,{
            [&](Sid sid) { scene.events.push_back("D:"+std::to_string(sid.value)); },
            [&](Sid sid) { scene.events.push_back("S:"+std::to_string(sid.value)); },
            [&](Sid sid) { scene.events.push_back("X:"+std::to_string(sid.value)); }});
        base.SetPriestPostPop(&tail);SquidPop pop(scene.pool,scene.hash,scene.spots,nullptr,&base);
        CHECK(pop.Pop(kObject,scene.types[kPriestType],1,1,20.75f,21.9f,Number(row[2]))==RawPopResult::Registered);
        std::string list=std::to_string(scene.priests.priests.count)+':',owners;
        // 활성 개수 밖의 원본 stale 목록도 비교한다.
        for (const auto value:scene.priests.priests.entries) { if (list.back()!=':') list+=',';list+=std::to_string(value); }
        // 칸 0을 포함한 모든 소유자 상태를 비교한다.
        for (const auto value:scene.priests.ownerState) { if (!owners.empty()) owners+=',';owners+=std::to_string(value); }
        const bool same=scene.Events()==row[8] && Hex(scene.pool.Slot(kObject))==row[9] && scene.hash.Bucket(1,20.75f,21.9f)==Number(row[10])
            && list==row[11] && owners==row[12] && scene.book.depth==0;
        CHECK(same);++count;
        if (!same) { std::printf("%s 사제 Pop 행 %zu: 기대 %s / 실제 %s\n",edition,count,row[8].c_str(),scene.Events().c_str());break; }
    }
    CHECK(count==kRows);
}
// 현재 raw 프레임을 읽는 실제 표시 계산기의 콘솔 출력 대상이다.
struct Sink final:SquidDisplaySink {
    std::size_t changes{};
    // 프레임마다 old/new 영역을 실제 계산한다.
    bool Suppressed() const override { return false; }
    // 전달된 영역 수를 관찰하며 좌표를 변경하지 않는다.
    void Invalidate(SquidDisplayRect,std::uint32_t) override { ++changes; }
};
}
// 패치의 공통 Pop/firstPop/Activate→사제 wrapper 관찰을 재생한다.
TEST_CASE(PriestPop_ReplaysOriginals) { Replay("originals"); }
// CD의 BYTE/WORD 필드와 flags 정규화 관찰을 재생한다.
TEST_CASE(PriestPop_ReplaysCd) { Replay("originalCD"); }
// 추가 10.37은 독립 PE의 같은 연결 관찰을 재생한다.
TEST_CASE(PriestPop_ReplaysExtra1037) { Replay("original1037"); }

// 최초 Pop→실제 회복/낙하 예약→중첩 재등록→프레임 단계 이동→착지를 같은 장부에서 확인한다.
TEST_CASE(PriestPop_ComposesFallFrameSpaceAndRegularLifetime) {
    // 실제 사제 가상 표와 두 raw 배치의 슬롯 폭을 모두 사용한다. 생성자 입력은 장면이 공급한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        PriestPopScene scene(edition);scene.Prepare(Fixture().front());scene.book.suppressed=false;scene.book.localOwner=1;
        scene.frames[kPriestType]=RiftTypeFrames({{65,80,1,0},{74,80,1,0},{74,80,2,0}});
        std::vector<SquidDisplayShape> shapes(scene.types.size());shapes[kPriestType]={3,true,
            {{12,20,3,18,1,1},{16,21,4,19,5,5},{18,22,5,20,5,5},{6,3,1,0,1,1},{7,3,1,0,1,1},{8,3,1,0,1,1}}};
        Sink sink;SquidDisplay display(edition,scene.types,shapes,sink,{0,0,65536,{0,0,640,480}});
        SquidUnpop unpop(scene.pool,scene.hash,scene.spots,&display);SquidPostPop base(scene.pool,scene.types,scene.book);
        SquidPop pop(scene.pool,scene.hash,scene.spots,&display,&base);RawSquidDestroy destroy(scene.pool,unpop,scene.types);
        Kernel kernel;SquidProcessState clock;clock.now=12.5;std::unique_ptr<RawPriestFall> fall;
        SquidProcessHost host(scene.pool,scene.types,kernel,destroy,clock,[&](Sid sid,std::uint32_t event,std::uint32_t count,float payload) {
            return MakePriestFallHandler(scene.pool,*fall)(sid,event,count,payload);
        });
        SquidReward hp(scene.pool,scene.types,scene.book,scene.hpMode,scene.patch ? &scene.store : nullptr);
        RawPriestState state(scene.pool,hp,scene.types,scene.spots);RawPriestPostPop prefix(scene.pool,hp,scene.priests,MakePriestPostPopProcessHooks(host));
        PriestFallState authority{true};std::vector<PriestFallFrames> choices(scene.types.size());choices[kPriestType]={1,0};
        std::vector<std::uint16_t> surfaces(65536);SquidFrame setter(scene.pool,scene.types,MakeSquidFrameHooks(scene.pool,scene.types,display,unpop,pop));
        RawFrameAdvance advance(scene.pool,scene.frames,setter);int shields=0,sounds=0,clears=0;
        // 현재 프레임과 좌표의 실제 공통 Pop으로 낙하 재등록을 수행한다.
        const auto repop=[&](Sid sid,std::uint32_t flags) {
            const auto raw=scene.pool.Slot(sid);const auto frame=Get(raw,scene.patch ? 36 : 34,scene.patch ? 4 : 1);
            const auto size=display.FrameSize(edition,raw,static_cast<int>(frame));
            CHECK(pop.Pop(sid,scene.types[kPriestType],size[0],size[1],std::bit_cast<float>(Get(raw,14)),std::bit_cast<float>(Get(raw,18)),flags)==RawPopResult::Registered);
        };
        PriestFallHooks hooks{[&](Sid) { ++shields; },[] { return true; },
            [&](Sid sid,std::uint32_t event,float payload) { host.AddSharedRegular(sid,event,payload); },
            [&](Sid sid,std::uint32_t flags) { unpop.Unpop(sid,scene.types[kPriestType],flags); },repop,
            [&](Sid,float,float) { ++sounds; },{},{},[&](Sid) { ++clears; }};
        fall=std::make_unique<RawPriestFall>(scene.pool,state,scene.frames,choices,surfaces,authority,MakePriestFallFrameHooks(setter,advance,std::move(hooks)));
        RawCarrierCheck check(scene.pool,scene.types,scene.spots,MakePriestFallWalker(scene.pool,*fall));CarrierPostPopState mode;
        RawCarrierPostPop carrier(scene.pool,scene.types,mode,MakeCarrierPostPopHooks(scene.pool,base,
            [&](Sid sid) { static_cast<void>(check.Check(sid)); },[](float,float,std::uint8_t) { CHECK(false); }));
        RawPriestPostPopTail tail(scene.pool,prefix,carrier,state,scene.frames,scene.spots,
            {[&](Sid sid) { fall->Begin(sid); },[&](Sid) { ++shields; },[&](Sid) { ++clears; }});
        base.SetPriestPostPop(&tail);scene.pool.AllocatedBytes(kObject)[scene.patch ? 40 : 35]=32;
        CHECK(pop.Pop(kObject,scene.types[kPriestType],1,1,20.75f,21.9f)==RawPopResult::Registered);
        CHECK(kernel.Size()==2 && host.FindEvent(kObject,kPriestRegenEvent) && host.FindEvent(kObject,kPriestFallEvent));
        CHECK(scene.book.depth==0 && scene.book.totalCost==123 && scene.book.globalCounts[kPriestType]==1 && scene.priests.priests.count==1 && sounds==1);
        host.Kill(*host.FindEvent(kObject,kPriestRegenEvent),0x10);kernel.RunFrame();auto* process=host.FindEvent(kObject,kPriestFallEvent);
        CHECK(process && process->Count()==1 && Get(scene.pool.Slot(kObject),scene.patch ? 36 : 34,scene.patch ? 4 : 1)==1);
        clock.now=process->Time();kernel.RunFrame();CHECK(scene.hash.Bucket(1,20.75f,21.9f)==0 && scene.hash.Bucket(3,20.75f,21.9f)==kObject.value);
        CHECK(scene.book.depth==0 && scene.book.globalCounts[kPriestType]==1 && sounds==1 && sink.changes>0);
        scene.pool.AllocatedBytes(kObject)[scene.patch ? 40 : 35]&=static_cast<std::uint8_t>(~32U);Put(scene.pool.AllocatedBytes(kObject),26,150,scene.patch ? 4 : 2);
        std::fill(scene.spots.begin(),scene.spots.end(),std::uint8_t{6});surfaces[22*256+21]=65535;clock.now=process->Time();kernel.RunFrame();
        CHECK(kernel.Size()==0 && !host.FindEvent(kObject,kPriestFallEvent) && clears>0 && shields>0 && scene.book.depth==0);
        CHECK(scene.book.totalCost==123 && scene.book.globalCounts[kPriestType]==1 && Get(scene.pool.Slot(kObject),scene.patch ? 36 : 34,scene.patch ? 4 : 1)==0);
    }
}

// 전체 후처리가 없거나 다른 풀/잘못된 접두 계약이면 공간 쓰기 전에 거부한다.
TEST_CASE(PriestPop_RequiresExplicitWholeHandlerBeforeMutation) {
    PriestPopScene scene(OriginalEdition::Patch1078);scene.Prepare(Fixture().front());SquidUnpop unpop(scene.pool,scene.hash,scene.spots);
    SquidPostPop base(scene.pool,scene.types,scene.book);SquidPop pop(scene.pool,scene.hash,scene.spots,nullptr,&base);
    SquidReward hp(scene.pool,scene.types,scene.book,scene.hpMode,&scene.store);RawPriestState state(scene.pool,hp,scene.types,scene.spots);
    RawPriestPostPop prefix(scene.pool,hp,scene.priests,{[](Sid,std::uint32_t) { return true; },[] { return false; },[](Sid,std::uint32_t,float) { CHECK(false); }});
    CarrierPostPopState mode;RawCarrierPostPop carrier(scene.pool,scene.types,mode,MakeCarrierPostPopHooks(scene.pool,base,[](Sid) {},[](float,float,std::uint8_t) {}));
    RawPriestPostPopTail tail(scene.pool,prefix,carrier,state,scene.frames,scene.spots,{[](Sid) {},[](Sid) {},[](Sid) {}});
    const auto before=Hex(scene.pool.Slot(kObject));
    CHECK(Throws([&] { pop.Pop(kObject,scene.types[kPriestType],1,1,20.75f,21.9f); }) && Hex(scene.pool.Slot(kObject))==before);
    SidPool other(OriginalEdition::Patch1078,kCapacity,false);SquidPostPop otherBase(other,scene.types,scene.book);
    CHECK(Throws([&] { otherBase.SetPriestPostPop(&tail); }));base.SetPriestPostPop(&tail);CHECK(base.HandlesPriest(kObject));
    scene.priests.priests.count=5;
    CHECK(Throws([&] { pop.Pop(kObject,scene.types[kPriestType],1,1,20.75f,21.9f); }) && Hex(scene.pool.Slot(kObject))==before && scene.hash.Bucket(1,20.75f,21.9f)==0);
    scene.priests.priests.count=0;base.SetPriestPostPop(nullptr);CHECK(!base.HandlesPriest(kObject) && !SquidPop::Supports(OriginalEdition::Patch1078,kPatchPriestVtable,0));
}
