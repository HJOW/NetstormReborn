// 실제두PE SunArcher 회전/각상태발사/helper/ctor와생성인자관찰을독립대조한다.
#include "RawSceneSupport.h"
#include "o/RawArcherAnimation.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 복원계산으로기대값을만들지않고저장된원본출력만읽는다.
const std::vector<std::vector<std::string>>& ArcherRows() { static const auto rows=LoadFixture(NETSTORM_GUNARCHER_FIXTURE).rows;return rows; }
// 별도launch의생성접두정지경계를C++에서도같은시점에관찰한다.
struct ArcherPrefixStop {};
// 원본현재타입/각상태와외부관찰경계를공유하는독립입력장면이다.
struct ArcherScene {
    // pool/types/data: 현재원본번호와방향수/물리프레임의합성입력이다.
    SidPool pool;
    std::vector<RiftTypeRecord> types;
    std::vector<ArcherTypeData> data;
    // state/fields/output: 원본시각/delta 및88바이트객체/float반환버퍼다.
    ArcherAnimationState state;
    ArcherAnimationFields fields;
    std::array<std::uint8_t,4> output{};
    // row/events: 현재독립입력과실제같은경계인자/순서다.
    const std::vector<std::string>* row{};
    std::vector<std::string> events;
    // patch: 판본별frame/raw폭을선택한다.
    bool patch;
    // 원본 SID50/60/70을 실제로 확보한다.
    explicit ArcherScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),data(types.size()),patch(edition==OriginalEdition::Patch1078) {
        // 각독립행은동일한원본번호를사용한다.
        for (std::uint16_t id=5;id<=70;++id) CHECK(pool.Allocate(2)==Sid{id});
    }
    // free표적의다음행재초기화에서만사용하는검사전용쓰기다.
    std::span<std::uint8_t> InputSlot(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // 원본 입력 float/double의 raw 비트를 그대로 읽는다.
    float F(std::size_t index) const { return std::bit_cast<float>(Number((*row)[index])); }
    // 현재입력의raw상태를작성하고기대값은읽지않는다.
    void Prepare(const std::vector<std::string>& value) {
        CHECK(value.size()==23);row=&value;events.clear();output.fill(0);state.now=10;state.delta=std::bit_cast<double>(std::stoull(value[3]));data[71]={static_cast<std::int32_t>(Number(value[15])),static_cast<std::int32_t>(Number(value[16]))};types[158].flags1=Number(value[17]) ? 0x10U : 0U;const auto kind=Number(value[1]);const bool fire=kind==1 || kind>=3;
        // 생성자가쓰지않는padding도같은입력으로남긴다.
        for (std::size_t i=0;i<fields.bytes.size();++i) fields.bytes[i]=static_cast<std::uint8_t>(i*13+7);
        Put(fields.bytes,0,fire ? (patch ? 0x50b1c0U : 0x5054d0U) : (patch ? 0x50b450U : 0x505570U));Put(fields.bytes,4,81);Put(fields.bytes,8,50);Put(fields.bytes,12,std::bit_cast<std::uint32_t>(60.0f));Put(fields.bytes,16,std::bit_cast<std::uint32_t>(60.0f));Put(fields.bytes,20,std::bit_cast<std::uint32_t>(59.5f));Put(fields.bytes,24,std::bit_cast<std::uint32_t>(59.5f));Put(fields.bytes,32,fire ? (patch ? 0x50b1bcU : 0x5054ccU) : (patch ? 0x50b448U : 0x50556cU));fields.bytes[40]=0x20;Put(fields.bytes,42,70,2);Put(fields.bytes,44,Number(value[17]) || fire ? 60U : 0U,2);const auto time=std::bit_cast<std::uint64_t>(Number(value[11]) ? 10.0 : 10.25);Put(fields.bytes,48,static_cast<std::uint32_t>(time));Put(fields.bytes,52,static_cast<std::uint32_t>(time>>32));Put(fields.bytes,56,0);Put(fields.bytes,64,Number(value[4]));Put(fields.bytes,68,Number(value[5]));Put(fields.bytes,72,Number(value[6]));Put(fields.bytes,76,Number(value[8]));Put(fields.bytes,80,Number(value[7]));
        // 각판본의원본세슬롯은표적free/dead입력도전부재구성한다.
        for (const auto [sid,type]:{std::array<std::uint16_t,2>{50,71},{60,158},{70,34}}) {
            auto raw=InputSlot(Sid{sid});std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x1131a000);Put(raw,6,sid==50 ? 81U : 0U,2);raw[10]=static_cast<std::uint8_t>(type);raw[11]=static_cast<std::uint8_t>(sid==60 ? Number(value[9]) : 0U);raw[12]=0xa5;raw[patch ? 34U : 32U]=1;Put(raw,14,std::bit_cast<std::uint32_t>(sid==50 ? 50.5f : 60.0f));Put(raw,18,std::bit_cast<std::uint32_t>(sid==50 ? 50.5f : 60.0f));Put(raw,patch ? 36U : 34U,Number(value[2]),patch ? 4U : 1U);
        }
        if (kind==0 && !Number(value[17])) pool.AllocatedBytes(kSource)[patch ? 30U : 28U]=static_cast<std::uint8_t>(Number(value[12]));
    }
    // 실제Missile 호출인자를관찰하며그생성완료로세지않는다.
    void Projectile(const GunProjectileRequest& request) {
        events.push_back(std::string("A:")+(patch ? "128" : "104"));const std::array<std::uint32_t,12> args{request.firing.value,request.target.value,std::bit_cast<std::uint32_t>(request.speed),request.displayType,request.effectType,std::bit_cast<std::uint32_t>(request.targetPosition[0]),std::bit_cast<std::uint32_t>(request.targetPosition[1]),std::bit_cast<std::uint32_t>(request.targetCenter[0]),std::bit_cast<std::uint32_t>(request.targetCenter[1]),request.ricochet,std::bit_cast<std::uint32_t>(request.firingPosition[0]),std::bit_cast<std::uint32_t>(request.firingPosition[1])};std::string descriptor="J:";
        // 호출자가이미만든원본field 인자를순서대로직렬화한다.
        for (std::size_t i=0;i<args.size();++i) { if (i) descriptor+=',';descriptor+=std::to_string(args[i]); }
        events.push_back(descriptor);if (Number((*row)[1])==6) throw ArcherPrefixStop{};
    }
    // 중심/가상공간/소리/완료/생성은원본과같은명시관찰경계다.
    ArcherAnimationHooks Hooks() {
        ArcherAnimationHooks hooks;
        hooks.common.center=[this](Sid sid) { events.push_back("C:"+std::to_string(sid.value));return sid==kSource ? std::array<float,2>{50,50} : std::array<float,2>{F(13),F(14)}; };
        hooks.common.unpop=[this](Sid sid,std::uint32_t flags) { events.push_back("U:"+std::to_string(sid.value)+':'+std::to_string(flags)); };
        hooks.common.repop=[this](Sid sid,std::uint32_t flags) { events.push_back("P:"+std::to_string(sid.value)+':'+std::to_string(flags)); };
        hooks.common.sound=[this](Sid,const std::string& name) { events.push_back("S:"+name); };
        hooks.common.randomInteger=[this](std::int32_t low,std::int32_t high) { events.push_back("N:"+std::to_string(low)+':'+std::to_string(high));events.push_back("F:3");return 3000; };
        hooks.common.notify=[this](Sid,std::uint32_t event,std::uint32_t info,std::uint32_t value) { events.push_back("M:"+std::to_string(event)+':'+std::to_string(info)+':'+std::to_string(value)); };
        hooks.common.kill=[this](Sid form,std::uint32_t flags) { events.push_back("K:"+std::to_string(form.value)+':'+std::to_string(flags)); };
        hooks.common.targetPoint=[this](Sid firing,Sid target) { events.push_back("T:"+std::to_string(firing.value)+':'+std::to_string(target.value));return std::array<float,2>{52.25f,52.75f}; };
        hooks.common.createProjectile=[this](const GunProjectileRequest& request) { Projectile(request); };
        hooks.aimDirection=[this](Sid sid) { return pool.Slot(sid)[patch ? 30U : 28U]; };
        hooks.notifySurface=[this](Sid sid) { events.push_back("Q:"+std::to_string(sid.value)); };return hooks;
    }
    // 각 독립 몸체와 동일 field/기하/경계를 실행한다.
    std::vector<std::string> Run() {
        RawArcherAnimation logic(pool,types,data,state,Hooks());std::string result="return";
        try {
            switch (Number((*row)[1])) {
            case 0:logic.Rotate(fields);break;
            case 1:logic.Fire(fields);break;
            case 2:result=std::to_string(logic.NearestDirection(F(4),pool.Edition()));break;
            case 3:Put(output,0,std::bit_cast<std::uint32_t>(logic.InitialAngle(static_cast<std::int32_t>(Number((*row)[2])))));break;
            case 4:result=std::to_string(logic.FireFrame(F(4),Number((*row)[8])!=0));break;
            case 5:logic.Initialize(fields,true,Sid{70},Sid{60});events.push_back("B:35:50:0:16");Put(fields.bytes,4,81);Put(fields.bytes,8,50);logic.InitializeFire(fields);break;
            case 6:result=logic.Launch(fields) ? "1" : "0";break;
            default:throw std::logic_error("Archer fixture body 오류");
            }
        } catch (const ArcherPrefixStop&) { result="prefix"; }
        std::string actions;
        // 최종기대값과같은경계순서만이어붙인다.
        for (const auto& event:events) { if (!actions.empty()) actions+=';';actions+=event; }
        return {result,actions,Hex(fields.bytes),Hex(pool.Slot(kSource)),Hex(output)};
    }
};
// 현재판본의고유정상/생성접두행을한번씩실제복원몸체로재생한다.
void ReplayArcher(std::string_view edition) {
    ArcherScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;int failures=0;
    // 저장된독립PE관찰이외에기대값을계산하지않는다.
    for (const auto& row:ArcherRows()) {
        if (row[0]!=edition) continue;scene.Prepare(row);const auto output=scene.Run();const auto expected=std::vector<std::string>(row.begin()+18,row.end());const bool same=output==expected;CHECK(same);
        if (!same) {
            // 최초차이는native값과C++값을함께보여준다.
            for (std::size_t i=0;i<output.size();++i) if (output[i]!=expected[i]) std::printf("  %s Archer 행%zu/body%s 결과%zu 기대%s / 실제%s\n",row[0].c_str(),count,row[1].c_str(),i,expected[i].c_str(),output[i].c_str());
            if (++failures>=5) break;
        }
        ++count;
    }
    CHECK(failures==0 && count==ArcherRows().size()/2);
}
}
// 패치실제회전/발사/기하/fmod/helper/ctor를독립raw와대조한다.
TEST_CASE(ArcherAnimation_PatchNativeRotateFireAndCtor) { ReplayArcher("originals"); }
// CD10.72의실제각상태/물리BYTE frame 및다른코드몸체를대조한다.
TEST_CASE(ArcherAnimation_CdNativeRotateFireAndCtor) { ReplayArcher("originalCD"); }
// 실제sunDisc 생성경계가없는live HP표적을묵시성공으로바꾸지않는다.
TEST_CASE(ArcherAnimation_MissingRealMissileRejected) {
    ArcherScene scene(OriginalEdition::Patch1078);auto row=ArcherRows().front();row[1]="6";row[17]="1";scene.Prepare(row);auto hooks=scene.Hooks();hooks.common.createProjectile={};RawArcherAnimation logic(scene.pool,scene.types,scene.data,scene.state,std::move(hooks));CHECK(Throws([&] { logic.Launch(scene.fields); }));CHECK(scene.events.empty());
}
// 실제 두 판본 Host/Kernel이 원본 회전·발사 완료로 Gun 단계를 바꾸고 Form을 삭제한다.
TEST_CASE(ArcherAnimation_ActualHostNotifyAndLifetime) {
    // 서버 송신과 분리한 실제 풀에서 두 판본의 Form/부모 연결과 Kill을 방문한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,kCapacity,false);std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);std::vector<ArcherTypeData> data(types.size());data[71]={20,40};const bool patch=edition==OriginalEdition::Patch1078;
        // 실제 동일 SID50/60까지 풀을 확보한다.
        for (std::uint16_t id=5;id<=60;++id) CHECK(pool.Allocate(2)==Sid{id});
        auto parent=pool.AllocatedBytes(kSource);std::fill(parent.begin(),parent.end(),std::uint8_t{});parent[10]=71;parent[12]=0xa5;Put(parent,patch ? 36U : 34U,0,patch ? 4U : 1U);auto target=pool.AllocatedBytes(Sid{60});target[10]=158;types[158].flags1=0x10;
        SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);Kernel kernel;SquidProcessState hostState;hostState.now=10;GunProcessState gunState;gunState.now=10;gunState.boss=false;SquidProcessHost host(pool,types,kernel,destroy,hostState);RawGunProcess gunLogic(pool,types,gunState,{});auto* gun=CreateGunProcess(host,gunLogic,kSource);CHECK(gun!=nullptr);if (!gun) throw std::logic_error("Archer 실제 Gun factory 실패");
        ArcherAnimationState state;state.now=10;state.delta=0;ArcherAnimationHooks hooks;std::uint32_t surfaceCalls=0;hooks.common.center=[](Sid sid) { return sid==kSource ? std::array<float,2>{50,50} : std::array<float,2>{50,40}; };hooks.notifySurface=[&surfaceCalls](Sid) { ++surfaceCalls; };RawArcherAnimation logic(pool,types,data,state,MakeArcherAnimationHostHooks(host,std::move(hooks)));Put(gun->Fields().bytes,28,3);
        auto* rotate=CreateArcherAnimation(host,logic,2,gun->Form(),Sid{60});CHECK(rotate!=nullptr);if (!rotate) throw std::logic_error("Archer 실제 회전 factory 실패");const auto rotateForm=rotate->Form();CHECK(rotate->Fields().GunForm()==gun->Form() && rotate->Fields().Parent()==kSource && rotate->Fields().Time()==10 && kernel.Size()==2);rotate->RunFrame();CHECK(gun->Fields().Stage()==4 && host.FromForm(rotateForm)==nullptr && kernel.Size()==1 && (pool.Slot(rotateForm)[11]&3));
        Put(gun->Fields().bytes,28,6);auto* fire=CreateArcherAnimation(host,logic,5,gun->Form(),Sid{60});CHECK(fire!=nullptr);if (!fire) throw std::logic_error("Archer 실제 발사 factory 실패");const auto fireForm=fire->Form();CHECK(surfaceCalls==1 && !(pool.Slot(kSource)[12]&1) && Get(fire->Fields().bytes,64)==std::bit_cast<std::uint32_t>(360.0f));Put(fire->Fields().bytes,64,std::bit_cast<std::uint32_t>(270.0f));Put(fire->Fields().bytes,68,0);Put(fire->Fields().bytes,80,1);fire->RunFrame();CHECK(gun->Fields().Stage()==7 && host.FromForm(fireForm)==nullptr && kernel.Size()==1 && (pool.Slot(fireForm)[11]&3));CHECK(destroy.PreDepth()==0 && destroy.PostDepth()==0);host.Kill(*gun,0x40);CHECK(kernel.Size()==0);
    }
}
