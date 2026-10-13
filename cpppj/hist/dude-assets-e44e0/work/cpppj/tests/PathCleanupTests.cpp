// 실제 PE cleanup 이후 몸체와 기존 공간 접두·같은 Kernel 예약 소비자를 연결해 대조한다.
#include "RawSceneSupport.h"
#include "o/RawPathCleanup.h"
#include "o/RawCarrierPreDestroy.h"
#include "o/RawPlayerPlacementAnchor.h"
#include "o/RawPathProcess.h"
#include "o/RawSquidDestroy.h"
#include "o/RawPriestPostPopTail.h"
#include "o/RawCarrierCheck.h"
#include "o/SquidFactory.h"
#include "o/SquidPop.h"
#include "o/SquidUnpop.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원본 격리 입력 좌표이며 범위/분기를 여기서 예측하지 않는다.
constexpr std::array<RawPathPoint,3> kCleanupCoordinates{{{40.75F,41.25F},{40.00004959106445F,41.00004959106445F},{40.000003814697266F,41.000003814697266F}}};
// 동일 raw/hash 입력과 실제 contained/Graph 소비자를 한 수명 안에 둔다.
struct CleanupScene {
    OriginalEdition edition; // 원본 raw 슬롯/타입 폭을 선택한다.
    SidPool pool; // 부모/보조/nugget/신전/표면의 유일한 슬롯 저장소다.
    SquidHash hash; // 네 실제 일반 finder와 Graph 표면 조회의 같은 지도다.
    std::vector<RiftTypeRecord> types; // 합성 등록 입력의 현재 genus/발자국/표면 플래그다.
    RawPathRuntime runtime; // 실제 helper 필드 외 다른 경로 바이트를 보존한다.
    PathCleanupState state; // callback 뒤 현재 권한을 다시 읽는다.
    ContainedFinderState containedState; // 실제 기존 contained 커서의 현재 타입/권한이다.
    SquidPostPopState book; // Graph 모듈 연결에 필요한 공유 장부이며 목록 조회를 실행하지 않는다.
    SquidPostPopList additional; // Graph 모듈의 별도 추가 목록이며 비워 둔다.
    PlayerPlacementAnchorState anchorState; // 실제 GraphFor의 활성/무효 번호다.
    std::array<std::span<std::uint8_t>,12> slots; // free 입력도 입력 준비 때 확보한 span으로만 작성한다.
    std::array<std::uint32_t,8> input; // profile,boss,dead,priest,reserve,checking,mutation,coordinate의 독립 입력이다.
    std::vector<std::string> effects; // 공간 접두 이후 요청만 native와 같은 순서로 기록한다.
    std::unique_ptr<RawCarrierPreDestroy> carrier; // 이미 복원된 contained mask/kind 조회를 재사용한다.
    std::unique_ptr<RawPlayerPlacementAnchor> anchor; // 이미 복원된 실제 GraphFor를 재사용한다.
    std::unique_ptr<RawPathCleanup> cleanup; // 새 cleanup 이후 몸체다.

    // native와 같은 슬롯/타입/버킷/종속 입력만 만든다. 기대 출력은 계산하지 않는다.
    CleanupScene(OriginalEdition selected,std::array<std::uint32_t,8> values):edition(selected),pool(selected,24000,true),types(selected==OriginalEdition::Patch1078 ? 188 : 171),runtime(selected,true,0xaa),input(values) {
        // 5..11을 실제 client free list에서 확보한 뒤 입력 바이트를0으로 채운다.
        for (std::uint16_t sid=5;sid<=11;++sid) { CHECK(pool.Allocate(2)==Sid{sid});slots[sid]=pool.AllocatedBytes(Sid{sid});std::fill(slots[sid].begin(),slots[sid].end(),std::uint8_t{}); }
        const auto profile=input[0],coordinate=input[7];const auto point=kCleanupCoordinates.at(coordinate);const bool helper=profile==1 || profile==2 || profile==3 || profile==4 || profile==5 || profile==6 || profile==7 || profile==14 || profile==15 || profile==16;
        const bool near=profile==1 || profile==6 || profile==7 || profile==16;
        if (helper) Add(Sid{6},128,profile==7 || profile==5 ? 1 : 0,(profile==3 ? 0 : 1)|(profile==6 ? 8 : 0),near ? point : RawPathPoint{80.25F,81.25F},1,profile==4 ? 0 : 0x404200,1,7);
        const bool place=profile==8 || profile==9 || profile==10 || profile==11 || profile==14 || profile==16 || profile==17;
        if (place) Add(Sid{7},profile==8 || profile==14 || profile==17 ? 126 : 85,profile==10 ? 2 : 0,profile==11 ? 1 : 0,point,1,profile==8 || profile==14 || profile==17 ? 0 : 0x200,profile==9 ? 2 : 1,profile==17 ? 3 : 7);
        if (profile==12 || profile==13 || profile==14 || profile==15 || profile==16) Add(Sid{9},165,0,profile==13 || profile==15 ? 8 : 0,point,1,0,1,7);
        Add(Sid{11},97,0,0,point,0,2,1,7);if (profile==17) types[126].flags1=0x800;
        hash.Cell(0,coordinate==0 ? 41 : 40,coordinate==0 ? 42 : 41)=11;
        auto parent=slots[5];Put(parent,0,kVtable);parent[10]=input[3] ? 158 : 133;parent[11]=input[2] ? 2 : 0;Put(parent,14,std::bit_cast<std::uint32_t>(point.x));Put(parent,18,std::bit_cast<std::uint32_t>(point.y));parent[edition==OriginalEdition::Patch1078 ? 34 : 32]=1;Put(parent,6,10,2);
        slots[10][10]=54;Put(slots[10],4,place ? 8 : 0,2);Put(slots[8],0,kVtable);slots[8][10]=6;Put(slots[8],18,154,2);Put(slots[8],20,32,2);
        runtime.SetAttachment(Sid{10},Sid{5});runtime.SetCommandFields(helper ? 6 : 0,0x87654321,0xabcdef01,{17,19});state.boss=input[1]!=0;state.checking=input[5]!=0;
        CarrierPreDestroyHooks carrierHooks;
        carrierHooks.validateDamageable=[](Sid,std::uint32_t) { throw std::logic_error("cleanup 독립 조회는 Damageable Validate를 실행하지 않는다"); };
        carrierHooks.damageablePre=[](Sid,std::uint32_t) { throw std::logic_error("cleanup 독립 조회는 Damageable PreDestroy를 실행하지 않는다"); };
        carrierHooks.dependentFound=[] { throw std::logic_error("cleanup 독립 조회는 Carrier 전역 후처리를 실행하지 않는다"); };
        carrier=std::make_unique<RawCarrierPreDestroy>(pool,containedState,std::move(carrierHooks));anchorState.graphReady=1;
        anchor=std::make_unique<RawPlayerPlacementAnchor>(pool,types,hash.Entries(0),book,additional,containedState,anchorState);
        cleanup=std::make_unique<RawPathCleanup>(pool,hash,types,state,Hooks());
    }
    // 실제 네 버킷 입력에 head 삽입하고 원본 raw next/발자국/현재 타입 속성을 작성한다.
    void Add(Sid sid,std::uint32_t type,std::uint32_t rawState,std::uint32_t extra,RawPathPoint point,int level,std::uint32_t genus,std::uint32_t owner,std::uint32_t graph) {
        auto raw=slots[sid.value];Put(raw,0,kVtable);raw[10]=static_cast<std::uint8_t>(type);raw[11]=static_cast<std::uint8_t>(rawState);raw[edition==OriginalEdition::Patch1078 ? 40 : 35]=static_cast<std::uint8_t>(extra);Put(raw,14,std::bit_cast<std::uint32_t>(point.x));Put(raw,18,std::bit_cast<std::uint32_t>(point.y));
        auto& head=hash.Bucket(level,point.x,point.y);Put(raw,4,head,2);head=sid.value;raw[edition==OriginalEdition::Patch1078 ? 34 : 32]=static_cast<std::uint8_t>(owner);raw[edition==OriginalEdition::Patch1078 ? 30 : 28]=static_cast<std::uint8_t>(graph);
        types[type].flags2=genus;types[type].flags1=type==97 ? 0x800 : 0;types[type].footX=types[type].footY=1;
    }
    // 실제 기존 조회와 명시 효과 요청을 연결하며 callback 뒤 권한 변이를 입력으로 공급한다.
    PathCleanupHooks Hooks() {
        PathCleanupHooks hooks;
        hooks.reserveShared=[this] { effects.push_back("N:40");return input[4]!=0; };
        hooks.addShared=[this](Sid sid,std::uint32_t event,float payload) { CHECK(sid==Sid{6} && event==0x1a2c && payload==1);effects.push_back("S:"+std::to_string(event)+','+std::to_string(sid.value)+','+std::to_string(std::bit_cast<std::uint32_t>(payload)));if (input[6]==1) state.boss=input[1]==0; };
        hooks.contained=[this](Sid sid,std::uint32_t mask,std::uint32_t kind,std::uint32_t index) { containedState.boss=state.boss;containedState.checkingDead=state.checking;return carrier->FindContained(sid,mask,kind,index); };
        hooks.graphFor=[this](Sid sid) { return anchor->GraphFor(sid.value); };
        hooks.ritual=[this](Sid temple,Sid parent,std::uint32_t flags) { CHECK(temple==Sid{9} && parent==Sid{5} && flags==0);effects.push_back("R:9,5,0");if (input[6]==2) state.boss=!state.boss; };
        hooks.refund=[this](std::uint32_t type,std::uint32_t owner) { effects.push_back("F:"+std::to_string(type)+','+std::to_string(owner)); };
        hooks.destroy=[this](Sid sid,std::uint32_t flags) { CHECK((sid==Sid{6} || sid==Sid{8}) && flags==0);effects.push_back("D:"+std::to_string(sid.value)+",0");slots[sid.value][11]|=2; };
        return hooks;
    }
    // 현재 요청 순서만 합쳐 원본 관찰과 비교한다.
    std::string Effects() const {
        std::string result;
        // 등록 순서를 그대로 유지한다.
        for (const auto& effect:effects) { if (!result.empty()) result+=';';result+=effect; }
        return result.empty() ? "-" : result;
    }
    // 가상 표 주입 포인터를 제외한 현재 일곱 슬롯의 모든 byte를 비교한다.
    void CheckSlots(const std::string& expected) const {
        const auto images=Split(expected,';');CHECK(images.size()==7);
        // 각 raw 폭과 미사용 필드도 보존되어야 한다.
        for (std::uint16_t sid=5;sid<=11;++sid) { const auto actual=Hex(pool.Slot(Sid{sid}));CHECK(actual.size()==images[sid-5].size() && actual.substr(8)==images[sid-5].substr(8)); }
    }
};
}

// helper 예약·실패·현재 nugget/Graph·의식 요청·환불/삭제 및 실패 뒤 raw 결과를 두 PE와 대조한다.
TEST_CASE(rawPathCleanup_native_body_and_queries) {
    const auto fixture=LoadFixture(NETSTORM_PATHCLEANUP_FIXTURE);std::size_t count=0,queries=0;
    // 각 고유 입력마다 같은 풀/hash를 새로 만든다. 기대 분기나 효과를 다시 계산하지 않는다.
    for (const auto& row:fixture.rows) {
        CHECK(row.size()==7);const auto values=Split(row[1],',');const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
        if (values[0]=="Q") {
            CHECK(values.size()==5);const auto profile=Number(values[1]),coordinate=Number(values[2]),radius=Number(values[3]),type=Number(values[4]);CleanupScene scene(edition,{profile,1,0,1,1,0,0,coordinate});const auto point=kCleanupCoordinates.at(coordinate);
            const auto found=scene.cleanup->ReturnPlaceAt(point.x,point.y,static_cast<std::int32_t>(radius)),nearby=scene.cleanup->NearbyType(point.x,point.y,2,type);
            CHECK("Q:"+std::to_string(found.value)+','+std::to_string(nearby.value)==row[3]);scene.CheckSlots(row[6]);++queries;continue;
        }
        CHECK(values.size()==8);std::array<std::uint32_t,8> input{};
        // native 입력 필드를 원본 DWORD 값 그대로 읽는다.
        for (std::size_t i=0;i<input.size();++i) input[i]=Number(values[i]);CleanupScene scene(edition,input);
        const std::vector<std::uint8_t> before(scene.runtime.Bytes().begin(),scene.runtime.Bytes().end());
        if (row[3].ends_with(";A") || row[3]=="A") { CHECK(Throws([&] { scene.cleanup->AfterSpatial(scene.runtime,Sid{5}); }));scene.effects.push_back("A"); }
        else scene.cleanup->AfterSpatial(scene.runtime,Sid{5});
        CHECK(scene.Effects()==row[3] && scene.runtime.Helper()==Number(row[4]) && scene.state.boss==(Number(row[5])!=0));scene.CheckSlots(row[6]);const auto after=scene.runtime.Bytes();const auto offset=RawPathRuntime::Layout(edition).helper;
        CHECK(std::equal(before.begin(),before.begin()+offset,after.begin()) && std::equal(before.begin()+offset+4,before.end(),after.begin()+offset+4));++count;
    }
    CHECK(count>500 && queries>250);
}

// 기존 Process 공간 접두 다음 실제 같은 Host/Kernel SharedRegular가 예약되는 소비자 사슬을 실행한다.
TEST_CASE(rawPathCleanup_real_process_spatial_prefix_and_shared) {
    // 두 판본의 client 경로 Form과 server SharedRegular를 같은 저장소에 둔다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        CleanupScene scene(edition,{1,1,0,1,1,0,0,0});std::vector<std::uint8_t> spots(65536);spots[42*256+41]=6;scene.types[158].footX=scene.types[158].footY=1;scene.types[158].flags1=0x10;scene.types[158].flags2=0x210000;scene.types[158].maxHitPoints=200;scene.types[158].constructorAddress=TypeConstructorAddress(edition,158);
        // 이미 확보된 부모 슬롯에 실제 사제 생성자만 적용하고 cleanup에 들어오는 기존 등록 상태를 준비한다.
        SquidFactory factory(scene.pool,scene.types);CHECK(factory.Construct(158,Sid{5})==Sid{5});Put(scene.slots[5],26,200,edition==OriginalEdition::Patch1078 ? 4 : 2);scene.slots[5][edition==OriginalEdition::Patch1078 ? 40 : 35]=0x80;
        SquidPostPop post(scene.pool,scene.types,scene.book);SquidPop pop(scene.pool,scene.hash,spots,nullptr,&post);SquidUnpop unpop(scene.pool,scene.hash,spots);RawSquidDestroy destroy(scene.pool,unpop,scene.types);Kernel kernel(32);SquidProcessState clock;clock.now=91;
        SquidDestroyHooks unused;unused.selected=[] { return Sid{}; };unused.emit=[](const SquidDestroyEvent&) { throw std::logic_error("공간 접두/예약 검사는 가상 자산 삭제를 실행하지 않는다"); };
        SquidProcessHost host(scene.pool,scene.types,kernel,destroy,clock,{},unused);RawPathProcessState pathState{clock};std::vector<RiftTypeFrames> frames(scene.types.size(),RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(scene.types.size());frames[158]=RiftTypeFrames({{'A','P',1,0}});
        // 기존 전체 사제 postPop을 직접 합성한다. 이동 가능한 지면 분기는 실제 보호막 조회로 종료한다.
        GameRandom random;ScrambledSpStore store(random,[] { return 1U; });SquidRewardState rewardState;SquidReward reward(scene.pool,scene.types,scene.book,rewardState,edition==OriginalEdition::Patch1078 ? &store : nullptr);PriestPostPopState priestBook;RawPriestPostPop prefix(scene.pool,reward,priestBook,MakePriestPostPopProcessHooks(host));RawPriestState priestState(scene.pool,reward,scene.types,spots);CarrierPostPopState carrierState;
        RawCarrierCheck check(scene.pool,scene.types,spots,[](Sid)->bool { throw std::logic_error("사제 genus는 Carrier walker 낙하 대상이 아니다"); });
        RawCarrierPostPop carrier(scene.pool,scene.types,carrierState,MakeCarrierPostPopHooks(scene.pool,post,[&](Sid sid) { static_cast<void>(check.Check(sid)); },[](float,float,std::uint8_t) { throw std::logic_error("비로딩 경로 정리는 지면 소유자를 바꾸지 않는다"); }));
        PriestForcefieldState shieldState;RawPriestForcefield shieldLookup(scene.pool,scene.hash,scene.types,shieldState);std::uint32_t shieldQueries=0;PriestPostPopTailHooks tailHooks;
        tailHooks.fall=[](Sid) { throw std::logic_error("fullHP/지면 사제의 cleanup은 낙하를 요청하지 않는다"); };
        tailHooks.ensureShield=[](Sid) { throw std::logic_error("이동 가능한 사제의 cleanup은 보호막을 생성하지 않는다"); };
        tailHooks.clearShield=[&](Sid sid) { CHECK(sid==Sid{5});CHECK(shieldLookup.Find(sid)==Sid{});++shieldQueries; };
        RawPriestPostPopTail tail(scene.pool,prefix,carrier,priestState,frames,spots,std::move(tailHooks));post.SetPriestPostPop(&tail);
        Put(scene.slots[5],6,0,2);scene.slots[5][11]=4;CHECK(pop.Pop(Sid{5},scene.types[158],1,1,40.75F,41.25F,0)==RawPopResult::Registered);
        std::uint32_t spatial=0,shared=0;auto cleanupHooks=scene.Hooks();
        cleanupHooks.addShared=[&](Sid sid,std::uint32_t event,float payload) { CHECK(sid==Sid{6} && event==0x1a2c && payload==1 && spatial==2);auto* added=host.AddSharedRegular(sid,event,payload);CHECK(added && added->Parent()==sid && added->Event()==event && added->Time()==clock.now);++shared; };
        cleanupHooks.ritual=[](Sid,Sid,std::uint32_t) { throw std::logic_error("예약 소비자 profile에는 신전 의식이 없다"); };
        cleanupHooks.refund=[](std::uint32_t,std::uint32_t) { throw std::logic_error("인접 helper 예약 뒤에는 환불하지 않는다"); };
        cleanupHooks.destroy=[](Sid,std::uint32_t) { throw std::logic_error("이 예약 profile에는 nugget/helper 삭제가 없다"); };
        RawPathCleanup cleanup(scene.pool,scene.hash,scene.types,scene.state,std::move(cleanupHooks));RawPathProcessHooks hooks;
        hooks.unpop=[&](Sid sid,std::uint32_t flags) { CHECK(sid==Sid{5} && flags==0 && spatial==0);unpop.Unpop(sid,scene.types[158],flags);++spatial; };
        hooks.repop=[&](Sid sid,std::uint32_t flags) { CHECK(sid==Sid{5} && flags==0 && spatial==1);CHECK(pop.Pop(sid,scene.types[158],1,1,40.75F,41.25F,flags)==RawPopResult::Registered);++spatial; };
        hooks.pop=[](Sid,RawPathPoint,std::uint32_t) { throw std::logic_error("삭제 접두는 Pop 가상 함수를 직접 부르지 않는다"); };
        hooks.halt=[](Sid)->std::uint32_t { throw std::logic_error("삭제 접두는 Halt를 실행하지 않는다"); };
        hooks.arrival=[](Sid,std::uint32_t,std::uint32_t)->std::uint32_t { throw std::logic_error("삭제 접두는 Arrival을 실행하지 않는다"); };
        hooks.soundId=[](Sid)->std::uint32_t { throw std::logic_error("삭제 접두는 발소리 번호를 읽지 않는다"); };
        hooks.stepSound=[](Sid,float,float,std::uint32_t) { throw std::logic_error("삭제 접두는 발소리를 실행하지 않는다"); };
        hooks.clearBlocked=[](Sid,RawPathPoint,std::uint32_t,std::uint32_t) { throw std::logic_error("삭제 접두는 도착 겹침을 처리하지 않는다"); };
        hooks.cleanup=[&](RawPathRuntime& runtime,Sid parent,std::uint32_t flags) { CHECK(parent==Sid{5} && flags==0 && spatial==2);cleanup.AfterSpatial(runtime,parent); };
        auto next=std::make_unique<RawPathProcess>(scene.pool,scene.hash,scene.types,frames,shapes,spots,pathState,std::move(hooks),true);next->Runtime().SetCommandFields(6,0x12345678,0x87654321,{17,19});auto* process=RawPathProcess::Attach(host,std::move(next),Sid{5},0x10);CHECK(process && &process->Pool()==&cleanup.Pool());
        process->OnFormDestroy(0);CHECK(spatial==2 && shared==1 && process->Runtime().Helper()==0 && kernel.Size()==2);CHECK(host.FindEvent(Sid{6},0x1a2c)!=nullptr);CHECK(shieldQueries==2 && scene.book.depth==0 && host.FindEvent(Sid{5},kPriestRegenEvent)==nullptr);kernel.Clear();
    }
}
