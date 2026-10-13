// 독립 두 PE의 Missile 생성/이동/충돌 필드를 외부 경계까지 대조한다.
#include "RawSceneSupport.h"
#include "o/RawMissile.h"
#include "o/RawGunHit.h"
#include "o/SquidOwner.h"
#include "o/SquidPop.h"
#include "o/SquidDestroyLifecycle.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 실제 PE 관찰을 저장한 fixture를 읽는다. 기대값을 C++에서 계산하지 않는다.
const std::vector<std::vector<std::string>>& Rows() { static const auto rows=LoadFixture(NETSTORM_GUNMISSILE_FIXTURE).rows;return rows; }
// 숫자 입력·raw 필드·관찰 사건을 같은 풀에 준비한다.
struct MissileScene {
    // pool: native의 표시/발사/표적 SID와 같은 저장소다.
    SidPool pool;
    // types/missileTypes: 실제 입력의 타입 hotspot과 명시 사운드/방향 수다.
    std::vector<RiftTypeRecord> types;
    std::vector<MissileTypeData> missileTypes;
    // state: 같은 현재 게임 시각과 원본 숫자 전역이다.
    MissileState state;
    // fields: 원본 판본별 Missile raw 이미지다.
    MissileFields fields;
    // events: 모든 명시 외부 경계의 순서/인자다.
    std::vector<std::string> events;
    // input: 현재 독립 입력 행이다.
    const std::vector<std::string>* input{};
    // patch: 원본 판본별 필드 폭을 구별한다.
    bool patch;
    // 실제 raw 입력 SID를 미리 확보한다.
    explicit MissileScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),missileTypes(types.size()),patch(edition==OriginalEdition::Patch1078) {
        // 실제 입력 번호 5~100의 슬롯을 유지한다.
        for (std::uint16_t id=5;id<=100;++id) CHECK(pool.Allocate(2)==Sid{id});
    }
    // 실제 free/dead native 입력을 다음 독립 행에서 다시 구성하는 검사 전용 raw 접근이다.
    std::span<std::uint8_t> FixtureSlot(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // 독립 float 입력 비트를 그대로 읽는다.
    float F(std::size_t index) const { return std::bit_cast<float>(Number(input->at(index))); }
    // 고유 입력의 타입·자산·padding을 준비한다.
    void Prepare(const std::vector<std::string>& row) {
        CHECK(row.size()==17);input=&row;events.clear();state.now=Number(row[1])==0 ? 10.25 : 10+Number(row[10])*0.5;
        // 생성자 padding과 판본 밖의 메모리는 원본 실행기와 같은 패턴이다.
        for (std::size_t i=0;i<fields.bytes.size();++i) fields.bytes[i]=static_cast<std::uint8_t>(i*13+7);
        // native 세 자산의 같은 raw 필드를 구성한다.
        for (const auto [sid,type]:{std::array<std::uint16_t,2>{50,128},{60,71},{90,80}}) {
            auto raw=FixtureSlot(Sid{sid});std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x11319000);raw[10]=static_cast<std::uint8_t>(type);raw[11]=static_cast<std::uint8_t>(sid==60 ? Number(row[11]) : 0);raw[patch ? 34U : 32U]=1;Put(raw,14,0x42480000);Put(raw,18,0x42480000);
        }
        types[128].group=1;types[128].hotspotX=std::stoi(row[6]);types[128].hotspotY=std::stoi(row[7]);missileTypes[128].range=40;
        if (Number(row[1])==0) return;
        Put(fields.bytes,0,patch ? 0x50d630U : 0x501ca0U);Put(fields.bytes,4,81);Put(fields.bytes,8,90);Put(fields.bytes,12,0);Put(fields.bytes,16,0);Put(fields.bytes,20,50);Put(fields.bytes,24,60);Put(fields.bytes,28,Number(row[13]));
        Put(fields.bytes,patch ? 48U : 32U,0x41a00000);Put(fields.bytes,patch ? 52U : 36U,0);Put(fields.bytes,patch ? 56U : 40U,Number(row[2]));Put(fields.bytes,patch ? 60U : 44U,Number(row[3]));Put(fields.bytes,patch ? 64U : 48U,Number(row[8]));Put(fields.bytes,patch ? 68U : 52U,Number(row[9]));Put(fields.bytes,patch ? 72U : 56U,Number(row[8]));Put(fields.bytes,patch ? 76U : 60U,Number(row[9]));
        // 실제 ctor가 아닌 독립 frame 입력의 box/time/이전 좌표를 쓴다.
        for (std::size_t offset=patch ? 88U : 64U;offset<(patch ? 104U : 80U);offset+=4) Put(fields.bytes,offset,0xc61c4000);
        const auto end=std::bit_cast<std::uint64_t>(12.0),start=std::bit_cast<std::uint64_t>(10.0);Put(fields.bytes,patch ? 104U : 80U,static_cast<std::uint32_t>(end));Put(fields.bytes,patch ? 108U : 84U,static_cast<std::uint32_t>(end>>32));Put(fields.bytes,patch ? 112U : 88U,static_cast<std::uint32_t>(start));Put(fields.bytes,patch ? 116U : 92U,static_cast<std::uint32_t>(start>>32));Put(fields.bytes,patch ? 120U : 96U,0x42480000);Put(fields.bytes,patch ? 124U : 100U,Number(row[12]) ? std::bit_cast<std::uint32_t>(89.0f) : 0xc61c4000);
        if (patch) { Put(fields.bytes,32,0);Put(fields.bytes,36,0);Put(fields.bytes,40,0);Put(fields.bytes,44,0); }
    }
    // 원본 외부 자료/자산/Pop/effect/삭제 경계를 같은 인자로 관찰한다.
    MissileHooks Hooks() {
        MissileHooks hooks;
        hooks.muzzle=[this](Sid) { events.push_back("M:50");return std::array<float,2>{F(2),F(3)}; };
        hooks.hotspot=[this](Sid) { if (patch) events.push_back("H:50");return std::array<std::int32_t,2>{std::stoi(input->at(4)),std::stoi(input->at(5))}; };
        hooks.targetDimensions=[this](Sid) { events.push_back("D:60");return std::array<std::int32_t,2>{2,3}; };
        hooks.rayEnd=[this](std::array<float,2> from,std::array<float,2> to,Sid firing) { events.push_back("R:"+std::to_string(std::bit_cast<std::uint32_t>(from[0]))+','+std::to_string(std::bit_cast<std::uint32_t>(from[1]))+','+std::to_string(std::bit_cast<std::uint32_t>(to[0]))+','+std::to_string(std::bit_cast<std::uint32_t>(to[1]))+','+std::to_string(firing.value));return std::array<float,2>{F(8),F(9)}; };
        hooks.create=[this](std::uint32_t type,std::uint32_t flags) { events.push_back("A:"+std::to_string(type)+':'+std::to_string(flags));return Sid{90}; };
        hooks.owner=[this](Sid sid,std::uint8_t owner) { pool.AllocatedBytes(sid)[patch ? 34U : 32U]=owner;events.push_back("O:90:"+std::to_string(owner)); };
        hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { Put(pool.AllocatedBytes(sid),14,std::bit_cast<std::uint32_t>(x));Put(pool.AllocatedBytes(sid),18,std::bit_cast<std::uint32_t>(y));events.push_back("P:90:"+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y))+':'+std::to_string(flags)); };
        hooks.unpop=[this](Sid,std::uint32_t flags) { events.push_back("U:90:"+std::to_string(flags)); };
        hooks.flightEffect=[this](std::uint32_t type,Sid asset) { events.push_back("E:"+std::to_string(type)+':'+std::to_string(asset.value)+":0"); };
        hooks.center=[this](Sid) { events.push_back("C:50");return std::array<float,2>{50,50}; };
        hooks.targetBounds=[this](Sid,std::array<float,2> point) { events.push_back("T:"+std::to_string(std::bit_cast<std::uint32_t>(point[0]))+':'+std::to_string(std::bit_cast<std::uint32_t>(point[1])));return std::array<float,4>{49,89,51,91}; };
        hooks.hit=[this](Sid firing,Sid target) { events.push_back("G:"+std::to_string(firing.value)+':'+std::to_string(target.value)); };
        hooks.hitEffect=[this](float x,float y) { events.push_back("H:"+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y))); };
        hooks.refresh=[this](Sid) { events.push_back("Q:50"); };
        hooks.impact=[this](float x,float y,std::uint32_t mode,float payload) { events.push_back("I:"+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y))+':'+std::to_string(mode)+':'+std::to_string(std::bit_cast<std::uint32_t>(payload))); };
        hooks.impactSound=[this](float x,float y,std::uint32_t token) { events.push_back("S:"+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y))+':'+std::to_string(token)+":0:0"); };
        hooks.destroy=[this](Sid,std::uint32_t flags) { events.push_back("X:90:"+std::to_string(flags)); };
        return hooks;
    }
    // 같은 실제 숫자 몸체를 실행하고 관찰 결과를 직렬화한다.
    std::vector<std::string> Run() {
        RawMissile logic(pool,types,missileTypes,state,Hooks());
        if (Number(input->at(1))==0) { GunProjectileRequest request;request.firing=Sid{50};request.target=Sid{60};request.targetPosition={100,90};request.targetCenter=Number(input->at(13)) ? std::array<float,2>{100,90} : std::array<float,2>{-10000,-10000};const auto asset=logic.Construct(fields,request);events.push_back("B:28:90:0:16");Put(fields.bytes,4,81);Put(fields.bytes,8,asset.value); }
        else logic.Frame(fields);
        std::string actions;
        // native 효과 순서를 그대로 이어 붙인다.
        for (const auto& event:events) { if (!actions.empty()) actions+=';';actions+=event; }
        return {actions,Hex(std::span(fields.bytes).first(patch ? 128U : 104U)),Hex(pool.Slot(Sid{90}))};
    }
};
// 판본별 실제 숫자 생성/이동 몸체의 독립 관찰을 대조한다.
void Replay(const char* edition) {
    MissileScene scene(std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;int failures=0;
    // 관찰 행마다 전체 raw 필드/자산/사건 순서를 비교한다.
    for (const auto& row:Rows()) {
        if (row[0]!=edition) continue;scene.Prepare(row);const auto output=scene.Run();const auto expected=std::vector<std::string>(row.begin()+14,row.end());const bool same=output==expected;CHECK(same);
        if (!same) {
            // 최초 차이를 원본 기대값과 함께 출력한다.
            for (std::size_t i=0;i<output.size();++i) if (output[i]!=expected[i]) std::printf("  %s Missile 행%zu/body%s 결과%zu 기대%s / 실제%s\n",edition,count,row[1].c_str(),i,expected[i].c_str(),output[i].c_str());
            if (++failures>=5) break;
        }
        ++count;
    }
    CHECK(failures==0 && count==Rows().size()/2);
}
}
// 패치의 실제 0x80 필드/생성/프레임 숫자를 대조한다.
TEST_CASE(Missile_PatchNativeCtorAndFrames) { Replay("originals"); }
// CD의 실제 0x68 필드/생성/프레임 숫자를 대조한다.
TEST_CASE(Missile_CdNativeCtorAndFrames) { Replay("originalCD"); }
// 실제 Pop/비행효과가 없으면 숫자나 자산을 바꾸기 전에 생성 실패를 드러낸다.
TEST_CASE(Missile_MissingRealFlightEffectRejected) {
    MissileScene scene(OriginalEdition::Patch1078);scene.Prepare(Rows().front());auto hooks=scene.Hooks();hooks.flightEffect={};RawMissile logic(scene.pool,scene.types,scene.missileTypes,scene.state,hooks);const auto before=scene.fields.bytes;
    GunProjectileRequest request;request.firing=Sid{50};request.target=Sid{60};request.targetPosition={100,90};request.targetCenter={100,90};CHECK(Throws([&] { logic.Construct(scene.fields,request); }));CHECK(scene.fields.bytes==before && scene.events.empty());
}
// 실제 자산 factory·소유자·공간 Pop/Unpop·Kernel 부착·공통 삭제를 같은 풀에서 검증한다.
// effectType0은 원본 생성자의 자연스러운 무효과 분기이며 타입11 비행 효과 완료로 세지 않는다.
TEST_CASE(Missile_ActualFactorySpaceKernelAndDelete) {
    // 두 판본의 자산/ProcessForm 필드와 동일한 실제 수명을 모두 방문한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,kCapacity,false);std::vector<RiftTypeRecord> types(patch ? 188 : 171);types[128].group=1;
        types[80].footX=types[80].footY=1;types[128].footX=types[128].footY=1;types[71].footX=types[71].footY=1;
        SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidPostPopState bookkeeping;bookkeeping.suppressed=true;SquidOwnerMode ownerMode;SquidOwner owner(pool,types,bookkeeping,ownerMode);
        SquidPostPop postPop(pool,types,bookkeeping);SquidPop pop(pool,hash,spots,nullptr,&postPop);SquidUnpop unpop(pool,hash,spots);SquidFactory factory(pool,types,false,&unpop);RawSquidDestroy destroy(pool,unpop,types);Kernel kernel;SquidProcessState clock;clock.now=10;
        const auto firing=factory.Create(128,2),target=factory.Create(71,2);owner.Set(firing,1);owner.Set(target,2);Put(pool.AllocatedBytes(firing),14,0x42480000);Put(pool.AllocatedBytes(firing),18,0x42480000);Put(pool.AllocatedBytes(target),14,0x42480000);Put(pool.AllocatedBytes(target),18,0x42700000);
        SquidDeletionState deletionState;deletionState.unitLostSuppressed1=true;SquidProcessHost* hostPointer=nullptr;SquidDeletionHooks deletion;
        deletion.forward=[&](const SquidDestroyEvent& event) { if (hostPointer) hostPointer->Hooks().emit(event);else throw std::logic_error("Missile 수명 검사 Host 미설정"); };
        SquidDestroyLifecycle lifecycle(pool,types,bookkeeping,deletionState,destroy,deletion);SquidProcessHost host(pool,types,kernel,destroy,clock,{},lifecycle.Hooks());hostPointer=&host;
        MissileState state;state.now=10;std::vector<MissileTypeData> missileTypes(types.size());missileTypes[128].range=10;std::vector<GunHitType> hitTypes(types.size());std::uint32_t refreshed=0,moves=0;
        GunHitHooks gunHooks;gunHooks.notifySurface=[&](Sid sid) { CHECK(sid==firing);++refreshed; };gunHooks.damage=[](Sid,std::int32_t,std::uint8_t,Sid) { throw std::logic_error("범위 종료 검사에 직접 피해가 발생함"); };RawGunHit gunHit(pool,types,hitTypes,gunHooks);
        MissileHooks hooks;
        hooks.muzzle=[&](Sid sid) { const auto raw=pool.Slot(sid);return std::array<float,2>{std::bit_cast<float>(Get(raw,14)),std::bit_cast<float>(Get(raw,18))}; };
        hooks.hotspot=[](Sid) { return std::array<std::int32_t,2>{0,0}; };
        hooks.targetDimensions=[](Sid) { return std::array<std::int32_t,2>{1,1}; };
        hooks.rayEnd=[](std::array<float,2> from,std::array<float,2> to,Sid) { return std::array<float,2>{from[0],to[1]}; };
        hooks.owner=[&](Sid sid,std::uint8_t player) { owner.Set(sid,player); };
        hooks.unpop=[&](Sid sid,std::uint32_t flags) { unpop.Unpop(sid,types[pool.Slot(sid)[10]],flags); };
        hooks.pop=[&](Sid sid,float x,float y,std::uint32_t flags) { CHECK(pop.Pop(sid,types[pool.Slot(sid)[10]],1,1,x,y,flags)==RawPopResult::Registered);++moves; };
        hooks.center=[](Sid) { return std::array<float,2>{49.5f,49.5f}; };
        hooks.targetBounds=[](Sid,std::array<float,2> point) { return std::array<float,4>{point[0]-1,point[1]-1,point[0],point[1]}; };
        hooks.hitEffect=[](float,float) { throw std::logic_error("범위 종료 검사에 충돌 효과가 발생함"); };
        hooks.destroy=[&](Sid sid,std::uint32_t flags) { destroy.Destroy(sid,flags,host.Hooks()); };
        hooks=MakeMissileFactoryHooks(pool,factory,MakeMissileGunHitHooks(gunHit,state,std::move(hooks)));RawMissile logic(pool,types,missileTypes,state,std::move(hooks));
        GunProjectileRequest request;request.firing=firing;request.target=target;request.targetPosition={50,60};request.targetCenter={-10000,-10000};request.effectType=0;
        auto* missile=CreateMissile(host,logic,request);CHECK(missile!=nullptr);if (!missile) throw std::logic_error("Missile 실제 factory 부착 실패");const auto asset=missile->Parent(),form=missile->Form();
        CHECK(kernel.Size()==1 && host.FromForm(form)==missile && missile->Fields().Firing()==firing && missile->Fields().Target()==target);CHECK(pool.Slot(asset)[patch ? 34U : 32U]==1 && !(pool.Slot(asset)[11]&7));CHECK(Get(pool.Slot(asset),6,2)==form.value && moves==1);
        state.now=20;clock.now=20;kernel.RunFrame();CHECK(kernel.Size()==0 && (pool.Slot(asset)[11]&3) && (pool.Slot(form)[11]&3));CHECK(refreshed==1 && moves==2 && destroy.PreDepth()==0 && destroy.PostDepth()==0);
    }
}
