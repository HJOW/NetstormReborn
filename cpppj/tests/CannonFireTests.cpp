// 초기화 이후 실제 공통 Fire와 두 파생의 전체 이미지/슬롯/효과를 원문 기대값과 대조한다.
#include "RawSceneSupport.h"
#include "o/RawCannonFire.h"
#include <cstdio>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원본 결과를 계산하지 않고 보존된 독립 행만 읽는다.
const std::vector<std::vector<std::string>>& Rows() { static const auto rows=LoadFixture(NETSTORM_CANNON_FIRE_FIXTURE).rows;return rows; }
// 실제 슬롯과 원본 코드 입력·외부 효과를 공유하는 독립 입력 장면이다.
struct CannonScene {
    SidPool pool{OriginalEdition::Patch1078,kCapacity,false}; // 실제50BYTE 슬롯의 단일 풀이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재 표적 HP 플래그다.
    std::vector<PriestPlainCanonType> frames=std::vector<PriestPlainCanonType>(188); // 실제 문자별 구간을 만들 코드 입력이다.
    CannonFireState state; // 현재 mutable 게임 시각/가상 getter/소리 차이다.
    GunAnimationFields fields; // 실제 원본64BYTE 이미지다.
    const std::vector<std::string>* row{}; // 실행 중인 명시 입력 행이다.
    std::vector<std::string> events; // 실제 효과 인수·순서를 관찰한다.
    // 원본 부모50/51·표적60/61·GunForm70까지 실제 번호를 할당한다.
    CannonScene() { for (std::uint16_t id=5;id<=70;++id) CHECK(pool.Allocate(2)==Sid{id}); }
    // free 입력도 다음 독립 행에서 재설치할 때만 쓰는 검사 전용 접근이다.
    std::span<std::uint8_t> Input(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // 현재 독립 입력의 숫자를 원본 정수 폭으로 읽는다.
    std::uint32_t N(std::size_t index) const { return Number((*row)[index]); }
    // 원문과 같은 패턴·슬롯·문자 코드 입력을 만들며 기대 출력은 읽지 않는다.
    void Prepare(const std::vector<std::string>& value) {
        CHECK(value.size()==15);row=&value;events.clear();state=CannonFireState{};state.now=10;state.growlOffset=std::stoi(value[8]);state.roarOffset=std::stoi(value[9]);const bool sun=N(0)==0;
        if (sun) { state.kind=CannonFireKind::Sun;state.type=128;state.displayType=80;state.effectType=11;state.delay=.07f; }
        types[158].flags1=N(5) ? 0x10U : 0U;std::vector<FrameCode> codes;
        for (unsigned i=0;i<32;++i) { // 연속·단일·교차·불균등 문자 입력을 원문과 똑같이 설치한다.
            const auto letter=N(6)==0 ? 65U+i/8 : N(6)==1 ? 65U : N(6)==2 ? 65U+i%2 : i==0 ? 65U : i<6 ? 66U : i<19 ? 67U : 68U;codes.push_back({static_cast<std::uint8_t>(letter),0,0,0});
        }
        frames[state.type].frames=RiftTypeFrames(std::move(codes));
        for (std::size_t i=0;i<fields.bytes.size();++i) fields.bytes[i]=static_cast<std::uint8_t>(i*13+7); // ctor의 미초기화 padding도 보존한다.
        Put(fields.bytes,0,sun ? 0x50b318U : 0x50b370U);Put(fields.bytes,4,81);Put(fields.bytes,8,50);Put(fields.bytes,12,std::bit_cast<std::uint32_t>(60.0f));Put(fields.bytes,16,std::bit_cast<std::uint32_t>(60.0f));Put(fields.bytes,20,std::bit_cast<std::uint32_t>(59.5f));Put(fields.bytes,24,std::bit_cast<std::uint32_t>(59.5f));Put(fields.bytes,32,sun ? 0x50b310U : 0x50b368U);fields.bytes[40]=0x20;Put(fields.bytes,42,70,2);Put(fields.bytes,44,60,2);const auto time=std::bit_cast<std::uint64_t>(N(3) ? 10.0 : 10.25);Put(fields.bytes,48,static_cast<std::uint32_t>(time));Put(fields.bytes,52,static_cast<std::uint32_t>(time>>32));Put(fields.bytes,56,0);
        for (const auto [sid,type]:{std::array<std::uint16_t,2>{50,static_cast<std::uint16_t>(state.type)},{51,static_cast<std::uint16_t>(state.type)},{60,158},{61,158},{70,34}}) { // 네 원문 관찰 슬롯과 GunForm을 준비한다.
            auto raw=Input(Sid{sid});std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x1131a000);raw[10]=static_cast<std::uint8_t>(type);raw[11]=static_cast<std::uint8_t>(sid==60 ? N(4) : 0U);raw[12]=0xa5;raw[34]=1;Put(raw,14,std::bit_cast<std::uint32_t>(sid<60 ? 50.5f : 60.0f));Put(raw,18,std::bit_cast<std::uint32_t>(sid<60 ? 50.5f : 60.0f));Put(raw,36,N(2));
        }
    }
    // 원문과 같은 선언된 외부 효과 뒤 현재 필드/전역 변경만 공급한다.
    void Mutate(std::string_view where) {
        const auto mut=N(10);
        if (where=="Center" && mut==1) Put(Input(Sid{50}),36,2);
        if (where=="Center" && mut==2) Put(fields.bytes,44,61,2);
        if (where=="Unpop" && mut==3) { state.now=100;const auto bits=std::bit_cast<std::uint64_t>(777.0);Put(fields.bytes,48,static_cast<std::uint32_t>(bits));Put(fields.bytes,52,static_cast<std::uint32_t>(bits>>32)); }
        if (where=="Repop" && mut==4) Input(Sid{60})[11]=2;
        if (where=="Point" && mut==5) { Put(fields.bytes,8,51);Put(fields.bytes,44,61,2);Put(fields.bytes,12,std::bit_cast<std::uint32_t>(77.25f));Put(fields.bytes,16,std::bit_cast<std::uint32_t>(78.5f));state.speed=37;state.displayType=state.kind==CannonFireKind::Sun ? 82U : 133U; }
        if (where=="Missile" && mut==6) Put(fields.bytes,42,71,2);
        if (where=="Sound" && mut==7) { state.roarOffset=state.growlOffset;Put(fields.bytes,8,51);state.now=100; }
    }
    // 실제 Missile 경계 인수만 직렬화하며 생성/비행 완료로 간주하지 않는다.
    void Projectile(const GunProjectileRequest& request) {
        events.push_back("A:128");const std::array<std::uint32_t,12> args{request.firing.value,request.target.value,std::bit_cast<std::uint32_t>(request.speed),request.displayType,request.effectType,std::bit_cast<std::uint32_t>(request.targetPosition[0]),std::bit_cast<std::uint32_t>(request.targetPosition[1]),std::bit_cast<std::uint32_t>(request.targetCenter[0]),std::bit_cast<std::uint32_t>(request.targetCenter[1]),request.ricochet,std::bit_cast<std::uint32_t>(request.firingPosition[0]),std::bit_cast<std::uint32_t>(request.firingPosition[1])};std::string text="J:";
        for (const auto value:args) { if (text.size()>2) text+=',';text+=std::to_string(value); } // 실제12개 인수를 원문 순서로 보존한다.
        events.push_back(text);Mutate("Missile");
    }
    // 실제 공통 공간/중심/소리/완료 효과를 같은 원문 형식으로 관찰한다.
    GunAnimationHooks Hooks() {
        GunAnimationHooks hooks;
        hooks.center=[this](Sid sid) { events.push_back("C:"+std::to_string(sid.value));Mutate("Center");return std::array<float,2>{60,50}; };
        hooks.unpop=[this](Sid sid,std::uint32_t flags) { events.push_back("U:"+std::to_string(sid.value)+':'+std::to_string(flags));Mutate("Unpop"); };
        hooks.repop=[this](Sid sid,std::uint32_t flags) { events.push_back("P:"+std::to_string(sid.value)+':'+std::to_string(flags));Mutate("Repop"); };
        hooks.targetPoint=[this](Sid sid,Sid target) { events.push_back("T:"+std::to_string(sid.value)+':'+std::to_string(target.value));Mutate("Point");return std::array<float,2>{52.25f,52.75f}; };
        hooks.createProjectile=[this](const GunProjectileRequest& request) { Projectile(request); };
        hooks.sound=[this](Sid,const std::string& name) { events.push_back("S:"+name);Mutate("Sound"); };
        hooks.notify=[this](Sid,std::uint32_t event,std::uint32_t info,std::uint32_t value) { events.push_back("M:"+std::to_string(event)+':'+std::to_string(info)+':'+std::to_string(value)); };
        hooks.kill=[this](Sid form,std::uint32_t flags) { events.push_back("K:"+std::to_string(form.value)+':'+std::to_string(flags)); };return hooks;
    }
    // 네 실제 원문 몸체 중 현재 독립 입력을 실행하고 전체 raw/효과를 반환한다.
    std::vector<std::string> Run() {
        RawCannonFire logic(pool,types,frames,state,Hooks());std::string result="return";
        switch (N(1)) { case 0:logic.Frame(fields);break;case 1:logic.Initialize(fields,Sid{70},Sid{60});events.push_back("B:35:50:0:16");Put(fields.bytes,4,81);Put(fields.bytes,8,50);break;case 2:result=std::to_string(logic.Step(fields,std::stoi((*row)[7])));break;case 3:result=logic.Launch(fields) ? "1" : "0";break;default:throw std::logic_error("캐논 원문 fixture 몸체 오류"); }
        std::string actions,slots;
        for (const auto& event:events) { if (!actions.empty()) actions+=';';actions+=event; } // 효과 순서와 인수를 그대로 비교한다.
        for (const auto sid:{50U,51U,60U,61U}) slots+=Hex(pool.Slot(Sid{static_cast<std::uint16_t>(sid)})); // 원문 네 슬롯200BYTE를 모두 비교한다.
        return {result,actions,Hex(fields.bytes),slots};
    }
};
// 원본 실제 입력의 지정 범위를 재생하고 최초 차이만 식별한다.
void Replay(unsigned profile,unsigned kindLow,unsigned kindHigh) {
    CannonScene scene;std::size_t count=0;unsigned failures=0;CHECK(Rows().size()==5534);
    for (const auto& row:Rows()) { // C++ 계산으로 기대값을 만들지 않고 저장된 원문 행을 읽는다.
        if (Number(row[0])!=profile || Number(row[1])<kindLow || Number(row[1])>kindHigh) continue;scene.Prepare(row);const auto actual=scene.Run(),expected=std::vector<std::string>(row.begin()+11,row.end());const bool same=actual==expected;CHECK(same);
        if (!same) { for (std::size_t i=0;i<actual.size();++i) if (actual[i]!=expected[i]) std::printf("캐논 행%zu profile%s kind%s field%zu 기대%s / 실제%s\n",count,row[0].c_str(),row[1].c_str(),i,expected[i].c_str(),actual[i].c_str());if (++failures==3) break; } // 최초 세 차이만 출력한다.
        ++count;
    }
    CHECK(count>0 && failures==0);
}
}
// 초기화 이후 썬 캐논의 전체 프레임/실패/재진입을 실제 공통 몸체와 대조한다.
TEST_CASE(CannonFire_NativeSunFramesAndReentry) { Replay(0,0,0); }
// 썬더 캐논의 현재 문자 구간·포효·발사 지연과 재진입을 원문과 대조한다.
TEST_CASE(CannonFire_NativeThunderFramesAndReentry) { Replay(1,0,0); }
// 두 실제 생성자/launch와 썬더 가상 단계의 별도 인수·소리 중첩을 대조한다.
TEST_CASE(CannonFire_NativeConstructorsStepsAndLaunch) { Replay(0,1,3);Replay(1,1,3); }
// 살아 있는 HP 표적에서 실제 Missile 경계가 누락되면 성공으로 우회하지 않는다.
TEST_CASE(CannonFire_MissingMissileRejected) {
    CannonScene scene;const auto found=std::find_if(Rows().begin(),Rows().end(),[](const auto& row) { return Number(row[1])==3 && Number(row[4])==0 && Number(row[5])==1; });CHECK(found!=Rows().end());if (found==Rows().end()) return;scene.Prepare(*found);auto hooks=scene.Hooks();hooks.createProjectile={};RawCannonFire logic(scene.pool,scene.types,scene.frames,scene.state,std::move(hooks));CHECK(Throws([&] { logic.Launch(scene.fields); }));CHECK(scene.events.empty());
}

// 실제 GunProcess 통지/패킷·Kernel/Host kill이 자기 삭제 뒤 재접근 없이 완료되어야 한다.
TEST_CASE(CannonFire_ActualHostNotifyAndLifetime) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,true);std::vector<RiftTypeRecord> types(188);types[131].group=1;std::vector<PriestPlainCanonType> frames(188);frames[131].frames=RiftTypeFrames({FrameCode{'A',0,0,0}});
    for (std::uint16_t id=5;id<=60;++id) CHECK(pool.Allocate(2)==Sid{id}); // 실제 원본 번호까지 같은 풀을 확보한다.
    auto parent=pool.AllocatedBytes(kSource);std::fill(parent.begin(),parent.end(),std::uint8_t{});parent[10]=131;auto target=pool.AllocatedBytes(Sid{60});target[10]=158;target[11]=2;
    SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);Kernel kernel;SquidProcessState hostState;hostState.now=10;GunProcessState gunState;gunState.now=10;gunState.boss=false;std::array<std::uint8_t,64> packet{};std::size_t packets=0;ProcessFormTransmission transmit;
    transmit.writeData=[&](SquidProcess& process,bool changed) { auto* gun=dynamic_cast<GunProcess*>(&process);if (!gun) throw std::logic_error("캐논 완료 검사 실제 writer 타입 오류");return static_cast<std::int32_t>(gun->WritePacket(packet,changed)); };
    transmit.transmit=[&](std::int32_t) { ++packets; };SquidProcessHost host(pool,types,kernel,destroy,hostState,{}, {},transmit);RawGunProcess gunLogic(pool,types,gunState,{});auto* gun=CreateGunProcess(host,gunLogic,kSource);CHECK(gun!=nullptr);if (!gun) return;Put(gun->Fields().bytes,28,6);
    CannonFireState state;state.now=10;GunAnimationHooks hooks;hooks.unpop=[](Sid,std::uint32_t) {};hooks.repop=[](Sid,std::uint32_t) {};RawCannonFire logic(pool,types,frames,state,MakeCannonFireHostHooks(host,std::move(hooks)));auto* fire=CreateCannonFire(host,logic,gun->Form(),Sid{60});CHECK(fire!=nullptr);if (!fire) return;const auto form=fire->Form();CHECK(kernel.Size()==2);fire->RunFrame();CHECK(gun->Fields().Stage()==7 && host.FromForm(form)==nullptr && kernel.Size()==1 && (pool.Slot(form)[11]&3));CHECK(packets>0 && destroy.PreDepth()==0 && destroy.PostDepth()==0);host.Kill(*gun,0x40);CHECK(kernel.Size()==0);
}
