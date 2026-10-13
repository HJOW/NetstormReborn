// 독립 PE 관찰과 실제 Host의 완료 통지·Form 삭제를 검증한다.
#include "RawSceneSupport.h"
#include "o/RawGunAnimation.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 실제 Missile ctor 진입 정지를 C++ 관찰 경계에서 구별한다.
struct MissilePrefix {};
// 실제 PE가 저장한 관찰만 읽으며 기대값을 복원 코드에서 계산하지 않는다.
const std::vector<std::vector<std::string>>& Rows() { static const auto rows=LoadFixture(NETSTORM_GUNANIMATION_FIXTURE).rows;return rows; }
// 검사용 raw 장면과 명시 경계 효과를 보관한다.
struct AnimationScene {
    // pool: native 입력 번호와 같은 실제 SID 저장소다.
    SidPool pool;
    // types: 부모/표적의 group·HP 플래그다.
    std::vector<RiftTypeRecord> types;
    // state: 현재 native 입력의 게임 시각/타입 번호다.
    GunAnimationState state;
    // fields: 원본 0x40 필드와 padding 입력이다.
    GunAnimationFields fields;
    // events: 원본 효과의 순서와 인자를 관찰한다.
    std::vector<std::string> events;
    // point: 원본 중심 외부 경계에 주입하는 독립 좌표다.
    std::array<float,2> point{};
    // patch: 현재 입력의 판본이다.
    bool patch;
    // native 입력 번호 5~100을 실제 풀에서 확보한다.
    explicit AnimationScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),patch(edition==OriginalEdition::Patch1078) {
        // 다음 입력의 raw 슬롯을 안정적으로 확보한다.
        for (std::uint16_t id=5;id<=100;++id) CHECK(pool.Allocate(2)==Sid{id});
    }
    // native의 free/dead 입력도 다음 독립 행에서 다시 준비하는 검사 전용 raw 접근이다.
    std::span<std::uint8_t> FixtureSlot(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // 관찰 행의 입력 칸으로 초기 필드를 준비한다.
    void Prepare(const std::vector<std::string>& row) {
        CHECK(row.size()==13);events.clear();state.now=10;const auto kind=Number(row[1]),index=kind==3 ? 1U : kind;
        // 미초기화 원본 padding을 별도 패턴으로 보존한다.
        for (std::size_t i=0;i<fields.bytes.size();++i) fields.bytes[i]=static_cast<std::uint8_t>(i*13+7);
        const std::array<std::uint32_t,3> outer=patch ? std::array<std::uint32_t,3>{0x50b260,0x50b318,0x50b2c8} : std::array<std::uint32_t,3>{0x505520,0x505480,0x505660};
        const std::array<std::uint32_t,3> inner=patch ? std::array<std::uint32_t,3>{0x50b258,0x50b310,0x50b2c4} : std::array<std::uint32_t,3>{0x505518,0x505478,0x505658};
        Put(fields.bytes,0,outer[index]);Put(fields.bytes,4,81);Put(fields.bytes,8,50);Put(fields.bytes,12,0x42700000);Put(fields.bytes,16,0x42700000);Put(fields.bytes,20,0x426e0000);Put(fields.bytes,24,0x426e0000);
        Put(fields.bytes,32,inner[index]);fields.bytes[40]=static_cast<std::uint8_t>(Number(row[8]));Put(fields.bytes,42,70,2);Put(fields.bytes,44,60,2);
        const auto time=std::bit_cast<std::uint64_t>(Number(row[7]) ? 10.0 : 10.25);Put(fields.bytes,48,static_cast<std::uint32_t>(time));Put(fields.bytes,52,static_cast<std::uint32_t>(time>>32));Put(fields.bytes,56,Number(row[4]));
        auto parent=pool.AllocatedBytes(Sid{50});std::fill(parent.begin(),parent.end(),std::uint8_t{});Put(parent,0,0x1131a000);parent[10]=128;Put(parent,14,0x424a0000);Put(parent,18,0x424a0000);parent[patch ? 34 : 32]=1;Put(parent,patch ? 36U : 34U,Number(row[2]),patch ? 4U : 1U);
        auto target=FixtureSlot(Sid{60});std::fill(target.begin(),target.end(),std::uint8_t{});target[10]=71;target[11]=static_cast<std::uint8_t>(Number(row[5]));Put(target,14,0x42700000);Put(target,18,0x42700000);types[71].flags1=Number(row[6]) ? 0x10U : 0U;
        point=std::array<std::array<float,2>,4>{{{{50,40}},{{60,50}},{{50,60}},{{40,50}}}}[Number(row[3])%4];
    }
    // 원본과 같은 명시 월드 경계의 인자/순서를 기록한다.
    GunAnimationHooks Hooks() {
        GunAnimationHooks hooks;
        hooks.center=[this](Sid sid) { events.push_back("C:"+std::to_string(sid.value));return sid.value==50 ? std::array<float,2>{50,50} : point; };
        hooks.targetPoint=[this](Sid firing,Sid target) { events.push_back("T:"+std::to_string(firing.value)+':'+std::to_string(target.value));return std::array<float,2>{52.25f,52.75f}; };
        hooks.unpop=[this](Sid sid,std::uint32_t flags) { events.push_back("U:"+std::to_string(sid.value)+':'+std::to_string(flags)); };
        hooks.repop=[this](Sid sid,std::uint32_t flags) { events.push_back("P:"+std::to_string(sid.value)+':'+std::to_string(flags)); };
        hooks.randomInteger=[this](std::int32_t low,std::int32_t high) { events.push_back("N:"+std::to_string(low)+':'+std::to_string(high));return 3000; };
        hooks.sound=[this](Sid,const std::string& name) { events.push_back("F:3");events.push_back("S:"+name); };
        hooks.notify=[this](Sid,std::uint32_t event,std::uint32_t info,std::uint32_t value) { events.push_back("M:"+std::to_string(event)+':'+std::to_string(info)+':'+std::to_string(value)); };
        hooks.kill=[this](Sid sid,std::uint32_t flags) { events.push_back("K:"+std::to_string(sid.value)+':'+std::to_string(flags)); };
        hooks.createProjectile=[this](const GunProjectileRequest& request) {
            events.push_back(patch ? "A:128" : "A:104");
            const std::array<std::uint32_t,12> args{request.firing.value,request.target.value,std::bit_cast<std::uint32_t>(request.speed),request.displayType,request.effectType,
                std::bit_cast<std::uint32_t>(request.targetPosition[0]),std::bit_cast<std::uint32_t>(request.targetPosition[1]),std::bit_cast<std::uint32_t>(request.targetCenter[0]),std::bit_cast<std::uint32_t>(request.targetCenter[1]),request.ricochet,std::bit_cast<std::uint32_t>(request.firingPosition[0]),std::bit_cast<std::uint32_t>(request.firingPosition[1])};
            std::string text="J:";
            // 실제 ctor가 받는 인자 12개를 원본 순서로 기록한다.
            for (const auto arg:args) { if (text.size()>2) text+=',';text+=std::to_string(arg); }
            events.push_back(text);throw MissilePrefix{};
        };
        return hooks;
    }
    // 같은 입력의 프레임 또는 launch 접두만 실행한다.
    std::vector<std::string> Run(const std::vector<std::string>& row) {
        RawGunAnimation logic(pool,types,state,Hooks());bool prefix=false;
        try { if (Number(row[1])==3) logic.Launch(fields);else logic.Frame(fields,static_cast<GunAnimationKind>(Number(row[1]))); }
        catch (const MissilePrefix&) { prefix=true; }
        std::string actions;
        // native 효과 순서를 그대로 직렬화한다.
        for (const auto& event:events) { if (!actions.empty()) actions+=';';actions+=event; }
        return {prefix ? "prefix" : "return",actions,Hex(fields.bytes),std::to_string(Get(pool.Slot(Sid{50}),patch ? 36U : 34U,patch ? 4U : 1U))};
    }
};
// 정상 반환과 Missile ctor 접두 관찰을 서로 구별해 독립 비교한다.
void Replay(const char* edition) {
    AnimationScene scene(std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;int failures=0;
    // 저장된 PE 모든 입력을 같은 복원 몸체로 소비한다.
    for (const auto& row:Rows()) {
        if (row[0]!=edition) continue;scene.Prepare(row);const auto output=scene.Run(row);const auto expected=std::vector<std::string>(row.begin()+9,row.end());const bool same=output==expected;CHECK(same);
        if (!same) {
            // 첫 차이들을 원본 기대값과 함께 출력하며 기대값을 덮지 않는다.
            for (std::size_t i=0;i<output.size();++i) if (output[i]!=expected[i]) std::printf("  %s Animation 행%zu/%s/%s 결과%zu 기대%s / 실제%s\n",edition,count,row[1].c_str(),row[2].c_str(),i,expected[i].c_str(),output[i].c_str());
            if (++failures>=5) break;
        }
        ++count;
    }
    CHECK(failures==0 && count==Rows().size()/2);
}
}
// 10.78의 실제 프레임/완료/launch descriptor를 대조한다.
TEST_CASE(GunAnimation_PatchNativeFramesAndLaunchPrefix) { Replay("originals"); }
// CD의 실제 BYTE 프레임과 짧은 virtual thunk를 대조한다.
TEST_CASE(GunAnimation_CdNativeFramesAndLaunchPrefix) { Replay("originalCD"); }
// 실제 투사체 효과가 없으면 valid launch를 성공이나 완료로 우회하지 않는다.
TEST_CASE(GunAnimation_MissingMissileIsExplicit) {
    AnimationScene scene(OriginalEdition::Patch1078);const auto& row=Rows().front();scene.Prepare(row);scene.types[71].flags1=0x10;
    RawGunAnimation logic(scene.pool,scene.types,scene.state,{});CHECK(Throws([&] { logic.Launch(scene.fields); }));
    scene.pool.AllocatedBytes(Sid{60})[11]=2;CHECK(!logic.Launch(scene.fields));
}
// 실제 Kernel factory가 현재 GunProcess에 완료를 통지하고 animation Form을 지운다.
TEST_CASE(GunAnimation_ActualHostNotifyAndLifetime) {
    // 두 판본에서 actual host와 원본 필드 폭의 차이를 모두 방문한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,kCapacity,true);std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);types[128].group=1;const bool patch=edition==OriginalEdition::Patch1078;
        Sid parent;
        // native와 같은 부모/표적 SID를 실제 풀에 확보한다.
        for (std::uint16_t id=5;id<=60;++id) { const auto sid=pool.Allocate(2);if (id==50) parent=sid; }
        auto raw=pool.AllocatedBytes(parent);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=128;Put(raw,patch ? 36U : 34U,0,patch ? 4U : 1U);
        SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);Kernel kernel;SquidProcessState hostState;hostState.now=10;GunProcessState gunState;gunState.now=10;gunState.boss=false;
        std::array<std::uint8_t,64> packet{};std::vector<std::vector<std::uint8_t>> packets;ProcessFormTransmission transmit;
        transmit.writeData=[&](SquidProcess& process,bool parentChanged) {
            if (auto* gun=dynamic_cast<GunProcess*>(&process)) return static_cast<std::int32_t>(gun->WritePacket(packet,parentChanged));
            if (auto* animation=dynamic_cast<GunAnimation*>(&process)) return static_cast<std::int32_t>(animation->WritePacket(packet,parentChanged));
            throw std::logic_error("GunAnimation test writer 타입 오류");
        };
        transmit.transmit=[&](std::int32_t size) { packets.emplace_back(packet.begin(),packet.begin()+size); };
        SquidProcessHost host(pool,types,kernel,destroy,hostState,{}, {},transmit);RawGunProcess gunLogic(pool,types,gunState,{});auto* gun=CreateGunProcess(host,gunLogic,parent);CHECK(gun!=nullptr);
        GunAnimationState animationState;animationState.now=10;GunAnimationHooks hooks;hooks.center=[](Sid sid) { return sid.value==50 ? std::array<float,2>{50,50} : std::array<float,2>{50,60}; };
        RawGunAnimation animationLogic(pool,types,animationState,MakeGunAnimationHostHooks(host,hooks));Put(gun->Fields().bytes,28,3);
        auto* animation=CreateGunAnimation(host,animationLogic,2,gun->Form(),Sid{60});CHECK(animation!=nullptr);if (!animation) throw std::logic_error("GunAnimation actual factory 실패");const auto form=animation->Form();
        CHECK(kernel.Size()==2 && animation->Fields().GunForm()==gun->Form() && animation->Fields().Parent()==parent && animation->Fields().Time()==10);
        animation->RunFrame();CHECK(gun->Fields().Stage()==4 && host.FromForm(form)==nullptr && kernel.Size()==1 && (pool.Slot(form)[11]&3));
        CHECK(packets.size()>=1 && destroy.PreDepth()==0 && destroy.PostDepth()==0);host.Kill(*gun,0x40);CHECK(kernel.Size()==0);
    }
}
