// 독립 원본 후처리/Regular 관찰과 실제 보호막 공간·form·Kernel 삭제 수명을 검사한다.
#include "RawSceneSupport.h"
#include "o/RawForcefieldLifecycle.h"
#include "o/RawPriestShield.h"
#include "o/SquidDestroyLifecycle.h"
#include "o/SquidFrameBinding.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원본 실행기와 같은 목표 SID와 판본별 입력 수다.
constexpr Sid kShield{50};
constexpr std::size_t kCases=1208;
// 일반 CTest는 PE/Python을 실행하지 않고 저장한 독립 입력을 읽는다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto data=LoadFixture(NETSTORM_FORCEFIELDPOSTPOP_FIXTURE);return data.rows;
}
// 실제 일반 finder와 동일한 합성 raw/해시를 준비하는 장면이다.
struct ForcefieldScene {
    SidPool pool;SquidHash hash;std::vector<RiftTypeRecord> types;bool patch;std::vector<std::string> events;
    // 목표/후보 SID를 실제 client 할당 정책으로 확보한다.
    explicit ForcefieldScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),patch(edition==OriginalEdition::Patch1078) {
        // 입력 후보 61까지 실제 할당을 유지한다.
        for (std::uint16_t sid=5;sid<=61;++sid) CHECK(pool.Allocate(2)==Sid{sid});
    }
    // 원본은 원래 vtable과 전체 stale 바이트를 그대로 관찰한다.
    void Prepare(const std::vector<std::string>& row) {
        hash.Reset();events.clear();std::fill(types.begin(),types.end(),RiftTypeRecord{});
        auto raw=pool.AllocatedBytes(kShield);std::fill(raw.begin(),raw.end(),std::uint8_t{});
        Put(raw,0,patch ? kPatchForcefieldVtable : kCdForcefieldVtable);raw[10]=kForcefieldType;raw[patch ? 34 : 32]=1;
        raw[patch ? 40 : 35]=static_cast<std::uint8_t>(Number(row[12]));Put(raw,14,Number(row[8]));Put(raw,18,Number(row[9]));
        types[kForcefieldType].footX=types[kForcefieldType].footY=1;
        const int layout=static_cast<int>(Number(row[11]));if (!layout) return;
        const float x=std::bit_cast<float>(Number(row[8])),y=std::bit_cast<float>(Number(row[9]));
        // 장면은 후보 입력만 만들고 존재 판단은 실제 finder에 맡긴다.
        const auto add=[&](Sid sid,std::uint8_t type,std::uint8_t state,std::uint8_t extra,float ax,float ay,int width,std::uint8_t owner,int level) {
            auto bytes=pool.AllocatedBytes(sid);std::fill(bytes.begin(),bytes.end(),std::uint8_t{});Put(bytes,0,kVtable);
            bytes[10]=type;bytes[11]=state;bytes[patch ? 40 : 35]=extra;bytes[patch ? 34 : 32]=owner;
            Put(bytes,14,std::bit_cast<std::uint32_t>(ax));Put(bytes,18,std::bit_cast<std::uint32_t>(ay));types[type].footX=width;types[type].footY=1;
            auto& head=hash.Bucket(level,ax,ay);Put(bytes,4,head,2);head=sid.value;
        };
        const float ax=static_cast<float>(std::min(255.5,static_cast<double>(x)+(layout==7 ? 1 : layout==8 ? 2 : 0)));
        add(Sid{60},static_cast<std::uint8_t>(layout==2 || layout==9 ? 159 : 158),static_cast<std::uint8_t>(layout==4 ? 2 : layout==5 ? 4 : 0),
            static_cast<std::uint8_t>(layout==3 ? 8 : 0),ax,y,layout==7 ? 2 : 1,static_cast<std::uint8_t>(layout==6 ? 8 : 1),1);
        if (layout==9) add(Sid{61},158,0,0,x,y,1,2,3);
    }
    // 외부 효과의 실제 순서만 문자열로 합친다.
    std::string Events() const {
        std::string text;
        // 같은 호출 순서를 보존한다.
        for (const auto& event:events) { if (!text.empty()) text+=';';text+=event; }
        return text.empty() ? "-" : text;
    }
};
// 원본 fixture를 실제 예약 접두/공통 base/Regular/finder에 대조한다.
void Replay(const char* edition) {
    ForcefieldScene scene(std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;
    CHECK(Fixture().size()==kCases*3);
    // 원본의 출력값을 다시 계산하지 않고 실제 C++ 모듈에 입력을 공급한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==16);scene.Prepare(row);std::string result="-";
        if (row[1]=="Prefix") {
            std::size_t allocations=0;
            RawForcefieldPostPop prefix(scene.pool,{[&] { scene.events.push_back("A:40");return Number(row[allocations++ ? 4 : 3])!=0; },
                [&](Sid sid,std::uint32_t event,float payload) { scene.events.push_back("R:"+std::to_string(sid.value)+':'+std::to_string(event)+':'+std::to_string(std::bit_cast<std::uint32_t>(payload))); }});
            prefix.Prefix(kShield,Number(row[2]));SquidPostPopState book;book.suppressed=true;book.depth=1;
            SquidPostPop base(scene.pool,scene.types,book);scene.events.push_back("B:50:"+row[2]);base.PostPopBase(kShield,Number(row[2]));CHECK(book.depth==0);
        } else {
            ForcefieldRegularState state{Number(row[10]),false};
            RawForcefieldRegular regular(scene.pool,scene.hash,scene.types,state,{
                [&](Sid sid,std::int32_t delta,std::uint32_t flags) { scene.events.push_back("A:"+std::to_string(sid.value)+':'+std::to_string(delta)+':'+std::to_string(flags)); },
                [&](Sid sid,std::uint32_t flags) { scene.events.push_back("D:"+std::to_string(sid.value)+':'+std::to_string(flags)); }});
            const auto returned=regular.Handle(kShield,Number(row[5]),Number(row[6]),std::bit_cast<float>(Number(row[7])));
            char bits[9]{};std::snprintf(bits,sizeof(bits),"%08x",std::bit_cast<std::uint32_t>(returned));result=bits;
        }
        const bool same=scene.Events()==row[13] && result==row[14] && Hex(scene.pool.Slot(kShield))==row[15];CHECK(same);++count;
        if (!same) { std::printf("%s 보호막 행 %zu: 기대 %s/%s 실제 %s/%s\n",edition,count,row[13].c_str(),row[14].c_str(),scene.Events().c_str(),result.c_str());break; }
    }
    CHECK(count==kCases);
}
// 실제 표시 계산기의 콘솔 대상이다. 프레임 변화의 무효화를 관찰한다.
struct ForcefieldSink final:SquidDisplaySink {
    std::size_t changes{};
    // 창 없이 표시 계산을 수행한다.
    bool Suppressed() const override { return false; }
    // 원본 old/new 영역 갱신을 관찰한다.
    void Invalidate(SquidDisplayRect,std::uint32_t) override { ++changes; }
};
}
// 패치판의 실제 후처리/Regular/타입 finder 관찰을 재생한다.
TEST_CASE(ForcefieldLifecycle_ReplaysOriginals) { Replay("originals"); }
// CD의 BYTE 프레임/소유자 배치를 같은 실제 모듈에 대조한다.
TEST_CASE(ForcefieldLifecycle_ReplaysCd) { Replay("originalCD"); }
// 추가 10.37 PE도 독립 관찰로 대조한다.
TEST_CASE(ForcefieldLifecycle_ReplaysExtra1037) { Replay("original1037"); }

// 실제 생성/소유자/Pop→예약/프레임 진행→사제 제거→자기 삭제/종속 form 반납을 검사한다.
TEST_CASE(ForcefieldLifecycle_ComposesSpaceProcessesAndSelfDeletion) {
    // 두 raw 배치에서 원본 생성자/가상 표와 실제 공간/프로세스 모듈을 사용한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,kCapacity,false);SquidHash hash;
        std::vector<RiftTypeRecord> types(patch ? 188 : 171);types[kForcefieldType].constructorAddress=TypeConstructorAddress(edition,kForcefieldType);
        types[kForcefieldType].maxHitPoints=100;types[kForcefieldType].cost=37;types[kForcefieldType].footX=types[kForcefieldType].footY=1;
        types[kPriestType].footX=types[kPriestType].footY=1;types[kPriestType].flags2=0x210000;std::vector<std::uint8_t> spots(65536);
        const Sid priest=pool.Allocate(2);auto raw=pool.AllocatedBytes(priest);std::fill(raw.begin(),raw.end(),std::uint8_t{});
        Put(raw,0,patch ? kPatchPriestVtable : kCdPriestVtable);raw[10]=kPriestType;raw[patch ? 34 : 32]=1;
        Put(raw,14,std::bit_cast<std::uint32_t>(20.75f));Put(raw,18,std::bit_cast<std::uint32_t>(21.9f));raw[patch ? 33 : 31]=1;hash.Bucket(1,20.75f,21.9f)=priest.value;
        std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));frames[kForcefieldType]=RiftTypeFrames({{65,80,1,0},{65,80,2,0}});
        // 두 SHP는 실제 로딩 완료 계약으로 공급한다. 두 번째 필드는 그림자 여부가 아닌 loaded다.
        std::vector<SquidDisplayShape> shapes(types.size());shapes[kForcefieldType]={2,true,{{12,20,3,18,1,1},{16,21,4,19,1,1}}};
        shapes[kPriestType]={1,true,{{12,20,3,18,1,1}}};
        ForcefieldSink sink;SquidDisplay display(edition,types,shapes,sink,{0,0,65536,{0,0,640,480}});
        SquidUnpop unpop(pool,hash,spots,&display);SquidPostPopState book;book.localOwner=1;SquidPostPop base(pool,types,book);SquidPop pop(pool,hash,spots,&display,&base);
        RawSquidDestroy destroy(pool,unpop,types);SquidDeletionState deletion;deletion.unitLostSuppressed1=true;
        SquidDestroyLifecycle lifecycle(pool,types,book,deletion,destroy,{[](const SquidDeletionEvent&) {},[](const SquidDestroyEvent&) { CHECK(false); }});
        Kernel kernel;SquidProcessState clock;clock.now=12.5;std::unique_ptr<RawForcefieldRegular> regular;
        SquidProcessHost host(pool,types,kernel,destroy,clock,[&](Sid sid,std::uint32_t event,std::uint32_t count,float payload) { return regular->Handle(sid,event,count,payload); },lifecycle.Hooks());
        RawForcefieldPostPop prefix(pool,MakeForcefieldPostPopProcessHooks(pool,host,[] { return true; }));base.SetForcefieldPostPop(&prefix);
        SquidFrame setter(pool,types,MakeSquidFrameHooks(pool,types,display,unpop,pop));RawFrameAdvance advance(pool,frames,setter);ForcefieldRegularState state;
        regular=std::make_unique<RawForcefieldRegular>(pool,hash,types,state,MakeForcefieldFrameHooks(pool,advance,{{},[&](Sid sid,std::uint32_t flags) { destroy.Destroy(sid,flags,host.Hooks()); }}));
        SquidFactory factory(pool,types);SquidOwnerMode ownerMode;SquidOwner owner(pool,types,book,ownerMode);
        PriestForcefieldState fieldState;RawPriestForcefield lookup(pool,hash,types,fieldState);PriestShieldState notice{0,1,1};Sid shield;
        auto hooks=MakePriestShieldCreationHooks(pool,factory,owner,{{},{},[&](Sid sid,float x,float y,std::uint32_t flags) {
            shield=sid;CHECK(pop.Pop(sid,types[kForcefieldType],1,1,x,y,flags)==RawPopResult::Registered);
        },[] { return false; },[](std::string_view) { CHECK(false);return 0U; },[](Sid,std::uint32_t,std::uint32_t,std::uint32_t) { CHECK(false); },
            [](const PriestShieldNotice&) { CHECK(false); },[](std::string_view) { CHECK(false);return std::string{}; },[](std::string_view) { CHECK(false); }});
        RawPriestShield create(pool,lookup,notice,std::move(hooks));const auto before=pool.FreeCount();create.Ensure(priest);
        CHECK(lookup.Find(priest)==shield && kernel.Size()==2 && book.depth==0 && book.globalCounts[kForcefieldType]==1 && book.totalCost==37);
        CHECK(host.FindEvent(shield,1) && host.FindEvent(shield,2));const auto allocated=pool.FreeCount();create.Ensure(priest);CHECK(pool.FreeCount()==allocated && kernel.Size()==2);
        kernel.RunFrame();CHECK(kernel.Size()==2 && sink.changes>0 && Get(pool.Slot(shield),patch ? 36 : 34,patch ? 4 : 1)==1);
        const double due=host.FindEvent(shield,2)->Time();unpop.Unpop(priest,types[kPriestType],0);
        clock.now=due;kernel.RunFrame();CHECK(kernel.Size()==0 && pool.Slot(shield)[11]&1 && hash.Bucket(0,20.75f,21.9f)==0);
        CHECK(pool.FreeCount()==before && book.globalCounts[kForcefieldType]==0 && book.totalCost==0 && destroy.PreDepth()==0 && destroy.PostDepth()==0);
        CHECK(lookup.Find(priest)==Sid{});create.Ensure(priest);CHECK(kernel.Size()==2 && lookup.Find(priest)==shield);
        // 외부 삭제도 같은 실제 종속 form/Kernel 정리를 사용한다.
        destroy.Destroy(shield,0,host.Hooks());CHECK(kernel.Size()==0 && pool.FreeCount()==before && book.totalCost==0);
    }
}

// 미연결 보호막/다른 풀과 잘못된 raw를 공간/예약 효과 전에 거부한다.
TEST_CASE(ForcefieldLifecycle_RequiresBindingAndDiagnosesInvalidEvents) {
    ForcefieldScene scene(OriginalEdition::Patch1078);scene.Prepare(Fixture().front());std::vector<std::uint8_t> spots(65536);scene.pool.AllocatedBytes(kShield)[11]=4;
    SquidPostPopState book;SquidPostPop base(scene.pool,scene.types,book);SquidPop pop(scene.pool,scene.hash,spots,nullptr,&base);int calls=0;
    const auto before=Hex(scene.pool.Slot(kShield));CHECK(Throws([&] { pop.Pop(kShield,scene.types[kForcefieldType],1,1,20.75f,21.9f); }) && before==Hex(scene.pool.Slot(kShield)));
    RawForcefieldPostPop prefix(scene.pool,{[&] { ++calls;return false; },[](Sid,std::uint32_t,float) { CHECK(false); }});
    SidPool other(OriginalEdition::Patch1078,kCapacity,false);SquidPostPop otherBase(other,scene.types,book);CHECK(Throws([&] { otherBase.SetForcefieldPostPop(&prefix); }));
    base.SetForcefieldPostPop(&prefix);CHECK(pop.Pop(kShield,scene.types[kForcefieldType],1,1,20.75f,21.9f)==RawPopResult::Registered && calls==2 && book.depth==0);
    ForcefieldRegularState state{158,true};RawForcefieldRegular regular(scene.pool,scene.hash,scene.types,state,{[](Sid,std::int32_t,std::uint32_t) { CHECK(false); },[](Sid,std::uint32_t) { CHECK(false); }});
    CHECK(Throws([&] { regular.Handle(kShield,99,0,0); }));state.debug=false;CHECK(regular.Handle(kShield,99,0,0)==0);
    base.SetForcefieldPostPop(nullptr);CHECK(!base.HandlesForcefield(kShield) && !SquidPop::Supports(OriginalEdition::Patch1078,kPatchForcefieldVtable,0));
}
