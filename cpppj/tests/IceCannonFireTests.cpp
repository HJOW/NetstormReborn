// 보호PE에서 얻은 독립 아이스80BYTE 입력/출력을 같은 실제 공통 Fire와 대조한다.
#include "RawSceneSupport.h"
#include "o/RawIceCannonFire.h"
#include <cstdio>
using namespace netstorm::o; // 실제 원문 런타임 형식을 검사한다.
using namespace netstorm::test::rawscene; // 기존 독립 fixture 판독기만 재사용한다.
namespace { // 이 자료의 명시 입력과 관찰을 다른 검사에서 분리한다.
struct IceScene { // 같은 실제 풀/타입/필드와 선언한 외부 효과의 독립 장면이다.
    SidPool pool{OriginalEdition::Patch1078,kCapacity,false}; // 현재50BYTE 슬롯이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재 표적 HP 입력이다.
    std::vector<PriestPlainCanonType> frames=std::vector<PriestPlainCanonType>(188); // 실제 문자 구간 입력이다.
    CannonFireState state; // 현재 게임 시각과 가상 getter 입력이다.
    IceCannonFireFields fields; // 공통64BYTE와 전용16BYTE/padding이다.
    const std::vector<std::string>* row{}; // 현재 명시 입력 행이다.
    std::vector<std::string> events; // 실제 호출 순서와 인수를 기록한다.
    IceScene() { for (std::uint16_t id=5;id<=70;++id) CHECK(pool.Allocate(2)==Sid{id}); } // 실제 원문 번호까지 할당한다.
    std::uint32_t N(std::size_t index) const { return Number((*row)[index]); } // 저장된 정수 비트 입력만 읽는다.
    std::span<std::uint8_t> Input(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // free/dead 입력도 다음 독립 행에만 다시 설치한다.
    void Prepare(const std::vector<std::string>& value) { // 출력 자료를 읽지 않고 같은 raw 입력을 구성한다.
        CHECK(value.size()==15);row=&value;events.clear();state=CannonFireState{};state.type=129;state.displayType=130;state.delay=.08F;state.now=10;types[158].flags1=N(4) ? 0x10U : 0U;
        std::vector<FrameCode> codes;for (unsigned i=0;i<32;++i) { const auto letter=N(5)==0 ? 65U+i/8 : N(5)==1 ? 65U : N(5)==2 ? 65U+i%2 : i==0 ? 65U : i<6 ? 66U : i<19 ? 67U : 68U;codes.push_back({static_cast<std::uint8_t>(letter),0,0,0}); } // 실제 문자 구간의 명시 입력이다.
        frames[129].frames=RiftTypeFrames(std::move(codes));
        for (std::size_t i=0;i<64;++i) fields.common.bytes[i]=static_cast<std::uint8_t>(i*13+7); // 생성자가 건드리지 않는 접두 padding도 보존한다.
        for (std::size_t i=0;i<16;++i) fields.extra[i]=static_cast<std::uint8_t>((i+64)*13+7); // 전용+44 padding까지 원문과 같은 패턴이다.
        auto& raw=fields.common.bytes;Put(raw,0,0x50b3f0);Put(raw,4,81);Put(raw,8,50);Put(raw,12,std::bit_cast<std::uint32_t>(60.F));Put(raw,16,std::bit_cast<std::uint32_t>(60.F));Put(raw,20,std::bit_cast<std::uint32_t>(59.5F));Put(raw,24,std::bit_cast<std::uint32_t>(59.5F));Put(raw,32,0x50b3ec);raw[40]=0x20;Put(raw,42,70,2);Put(raw,44,60,2);const auto time=std::bit_cast<std::uint64_t>(N(2) ? 10.0 : 10.25);Put(raw,48,static_cast<std::uint32_t>(time));Put(raw,52,static_cast<std::uint32_t>(time>>32));Put(raw,56,0);Put(fields.extra,0,N(7));const auto deadline=std::bit_cast<std::uint64_t>(N(8)==0 ? 9.0 : N(8)==1 ? 10.0 : 10.25);Put(fields.extra,8,static_cast<std::uint32_t>(deadline));Put(fields.extra,12,static_cast<std::uint32_t>(deadline>>32));
        for (const auto [sid,type]:{std::array<std::uint16_t,2>{50,129},{51,129},{60,158},{61,158},{70,34}}) { auto bytes=Input(Sid{sid});std::fill(bytes.begin(),bytes.end(),std::uint8_t{});Put(bytes,0,0x1131a000);bytes[10]=static_cast<std::uint8_t>(type);bytes[11]=static_cast<std::uint8_t>(sid==60 ? N(3) : 0U);bytes[12]=0xa5;bytes[34]=1;Put(bytes,14,std::bit_cast<std::uint32_t>(sid<60 ? 50.5F : 60.F));Put(bytes,18,std::bit_cast<std::uint32_t>(sid<60 ? 50.5F : 60.F));Put(bytes,36,N(1)); } // 두 부모와 두 표적을 매행 독립 설치한다.
    }
    void Mutate(std::string_view where) { // 원문 관찰기와 같은 외부 재진입 입력만 공급한다.
        const auto mut=N(10);auto& raw=fields.common.bytes;
        if (where=="Center" && mut==1) Put(Input(Sid{50}),36,2);
        if (where=="Center" && mut==2) Put(raw,44,61,2);
        if (where=="Unpop" && mut==3) { state.now=100;const auto bits=std::bit_cast<std::uint64_t>(777.0);Put(raw,48,static_cast<std::uint32_t>(bits));Put(raw,52,static_cast<std::uint32_t>(bits>>32)); }
        if (where=="Repop" && mut==4) Input(Sid{60})[11]=2;
        if (where=="Point" && mut==5) { Put(raw,8,51);Put(raw,44,61,2);Put(raw,12,std::bit_cast<std::uint32_t>(77.25F));Put(raw,16,std::bit_cast<std::uint32_t>(78.5F));state.speed=37;state.displayType=133; }
        if (where=="Missile" && mut==6) Put(raw,42,71,2);
    }
    void Projectile(const GunProjectileRequest& request) { // 실제12개 인수를 원문 순서로 관찰한다.
        events.push_back("A:128"); // 실제 Missile factory의128BYTE 확보도 선언된 외부 경계 관찰에 포함한다.
        const std::array<std::uint32_t,12> args{request.firing.value,request.target.value,std::bit_cast<std::uint32_t>(request.speed),request.displayType,request.effectType,std::bit_cast<std::uint32_t>(request.targetPosition[0]),std::bit_cast<std::uint32_t>(request.targetPosition[1]),std::bit_cast<std::uint32_t>(request.targetCenter[0]),std::bit_cast<std::uint32_t>(request.targetCenter[1]),request.ricochet,std::bit_cast<std::uint32_t>(request.firingPosition[0]),std::bit_cast<std::uint32_t>(request.firingPosition[1])};std::string text="J:";
        for (const auto value:args) { if (text.size()>2) text+=',';text+=std::to_string(value); } // 배열의 모든 인수와 순서를 보존한다.
        events.push_back(text);Mutate("Missile");
    }
    GunAnimationHooks Common() { // 공간/중심/투사체/완료만 선언된 외부 효과다.
        GunAnimationHooks hooks;hooks.center=[this](Sid sid) { events.push_back("C:"+std::to_string(sid.value));Mutate("Center");return std::array<float,2>{60,50}; };
        hooks.unpop=[this](Sid sid,std::uint32_t flags) { events.push_back("U:"+std::to_string(sid.value)+':'+std::to_string(flags));Mutate("Unpop"); };
        hooks.repop=[this](Sid sid,std::uint32_t flags) { events.push_back("P:"+std::to_string(sid.value)+':'+std::to_string(flags));Mutate("Repop"); };
        hooks.targetPoint=[this](Sid sid,Sid target) { events.push_back("T:"+std::to_string(sid.value)+':'+std::to_string(target.value));Mutate("Point");return std::array<float,2>{52.25F,52.75F}; };
        hooks.createProjectile=[this](const auto& request) { Projectile(request); };hooks.notify=[this](Sid,std::uint32_t event,std::uint32_t info,std::uint32_t value) { events.push_back("M:"+std::to_string(event)+':'+std::to_string(info)+':'+std::to_string(value)); };hooks.kill=[this](Sid form,std::uint32_t flags) { events.push_back("K:"+std::to_string(form.value)+':'+std::to_string(flags)); };return hooks;
    }
    IceCannonFireHooks Ice() { // 선택된 냉각/문맥/표적 결과를 명시 입력으로만 제공한다.
        IceCannonFireHooks hooks;hooks.cooling=[this](Sid sid) { events.push_back("E:"+std::to_string(sid.value));if (N(10)==7) { state.now=100;Put(fields.common.bytes,8,51); } };
        hooks.context=[this](Sid sid) { events.push_back("X:"+std::to_string(sid.value));if (N(10)==8) { Put(fields.common.bytes,8,51);Put(fields.common.bytes,44,61,2); }return std::array<std::uint32_t,8>{201,202,203,204,205,206,207,208}; };
        hooks.mayTarget=[this](const auto& context,Sid target) { std::string text="A:";for (auto value:context) { if (text.size()>2) text+=',';text+=std::to_string(value); }events.push_back(text+','+std::to_string(target.value));if (N(10)==9) Put(fields.extra,0,0);return N(9)!=0; };return hooks; // 현재 target 재조회와 결과 뒤의 active 재조회를 관찰한다.
    }
    std::vector<std::string> Run() { // 네 실제 원문 진입과80BYTE/슬롯/효과를 비교할 관찰을 반환한다.
        RawIceCannonFire logic(pool,types,frames,state,Common(),Ice());std::string result="return";
        switch (N(0)) { case 0:logic.Frame(fields);break;case 1:logic.Initialize(fields,Sid{70},Sid{60});events.push_back("B:35:50:0:16");Put(fields.common.bytes,4,81);Put(fields.common.bytes,8,50);break;case 2:result=std::to_string(logic.Step(fields,std::stoi((*row)[6])));break;case 3:result=logic.Launch(fields) ? "1" : "0";break;default:throw std::logic_error("아이스 자료의 몸체 입력 오류"); }
        std::string actions,slots;for (const auto& event:events) { if (!actions.empty()) actions+=';';actions+=event; } // 효과 순서와 인수를 그대로 유지한다.
        for (const auto sid:{50U,51U,60U,61U}) slots+=Hex(pool.Slot(Sid{static_cast<std::uint16_t>(sid)})); // 네 슬롯200BYTE 전체다.
        return {result,actions,Hex(fields.common.bytes)+Hex(fields.extra),slots};
    }
};
void Replay(unsigned kind) { // 첫 실제 차이에서 입력 행/필드를 진단하고 기존 기대값은 바꾸지 않는다.
    const auto rows=LoadFixture(NETSTORM_ICE_CANNON_FIRE_FIXTURE).rows;IceScene scene;std::size_t count{};
    for (std::size_t i=0;i<rows.size();++i) { if (Number(rows[i][0])!=kind) continue;scene.Prepare(rows[i]);const auto actual=scene.Run();++count;for (std::size_t column=0;column<4;++column) { if (actual[column]!=rows[i][11+column]) { std::fprintf(stderr,"아이스 원문 차이 row=%zu column=%zu actual=%s expected=%s\n",i,column,actual[column].c_str(),rows[i][11+column].c_str());CHECK(false);return; } } } // 저장된 원문 결과와만 대조한다.
    CHECK(count>0);
}
}
TEST_CASE(IceCannonFire_ReplaysNativeCommonFrame) { Replay(0); } // due/생존/프레임/냉각/재진입 전체를 대조한다.
TEST_CASE(IceCannonFire_ReplaysNativeConstructorPadding) { Replay(1); } //80BYTE의 미초기화 padding 보존을 대조한다.
TEST_CASE(IceCannonFire_ReplaysNativeStepCurrentFields) { Replay(2); } // 별도 인수와 냉각 경계/현재 표적/active 재조회를 대조한다.
TEST_CASE(IceCannonFire_ReplaysNativeLaunchGetters) { Replay(3); } // 실제 아이스130/속도/표적 생존과 getter 재조회를 대조한다.

TEST_CASE(IceCannonFire_ActualHostNotifyAndSelfDeletion) { // 실제 GunProcess/Kernel의 완료 통지와 자기 삭제 뒤 재접근이 없는지 검사한다.
    SidPool pool(OriginalEdition::Patch1078,kCapacity,true);std::vector<RiftTypeRecord> types(188);types[129].group=1;std::vector<PriestPlainCanonType> frames(188);frames[129].frames=RiftTypeFrames({FrameCode{'A',0,0,0},FrameCode{'A',0,0,0}});
    for (std::uint16_t id=5;id<=60;++id) CHECK(pool.Allocate(2)==Sid{id}); // 원문과 같은 실제 SID까지 할당한다.
    auto parent=pool.AllocatedBytes(kSource);std::fill(parent.begin(),parent.end(),std::uint8_t{});parent[10]=129;auto target=pool.AllocatedBytes(Sid{60});target[10]=158;target[11]=2;
    SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);Kernel kernel;SquidProcessState hostState;hostState.now=10;GunProcessState gunState;gunState.now=10;gunState.boss=false;std::array<std::uint8_t,64> packet{};std::size_t packets{};ProcessFormTransmission transmit;
    transmit.writeData=[&](SquidProcess& process,bool changed) { auto* gun=dynamic_cast<GunProcess*>(&process);if (!gun) throw std::logic_error("아이스 완료 검사의 실제 writer 타입");return static_cast<std::int32_t>(gun->WritePacket(packet,changed)); };
    transmit.transmit=[&](std::int32_t) { ++packets; };SquidProcessHost host(pool,types,kernel,destroy,hostState,{}, {},transmit);RawGunProcess gunLogic(pool,types,gunState,{});auto* gun=CreateGunProcess(host,gunLogic,kSource);CHECK(gun!=nullptr);if (!gun) return;Put(gun->Fields().bytes,28,6);
    CannonFireState state;state.now=10;state.type=129;state.displayType=130;state.delay=.08F;GunAnimationHooks common;common.unpop=[](Sid,std::uint32_t) {};common.repop=[](Sid,std::uint32_t) {}; // 공간은 명시 외부 경계이며 실제 Host/Kernel을 대신하지 않는다.
    IceCannonFireHooks hooks;hooks.context=[](Sid) { return std::array<std::uint32_t,8>{}; };hooks.mayTarget=[](const auto&,Sid) { return false; }; // 명시한 표적 실패 뒤의 실제 완료/전송/반납을 관찰한다.
    RawIceCannonFire logic(pool,types,frames,state,MakeIceCannonFireHostHooks(host,std::move(common)),std::move(hooks));auto* fire=CreateIceCannonFire(host,logic,gun->Form(),Sid{60});CHECK(fire!=nullptr);if (!fire) return;const auto form=fire->Form();CHECK(kernel.Size()==2);fire->RunFrame();
    CHECK(gun->Fields().Stage()==7 && host.FromForm(form)==nullptr && kernel.Size()==1 && (pool.Slot(form)[11]&3));CHECK(packets>0 && destroy.PreDepth()==0 && destroy.PostDepth()==0);host.Kill(*gun,0x40);CHECK(kernel.Size()==0);
}
