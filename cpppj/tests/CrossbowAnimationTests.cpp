// 실제10.78 크로스보우의 전체 이미지/부모·표적/효과 순서를 독립 원본 행과 대조한다.
#include "RawSceneSupport.h"
#include "o/RawCrossbowAnimation.h"
#include <cstdio>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원문 출력은 저장된 fixture만 사용하며 C++ 구현으로 기대값을 만들지 않는다.
const std::vector<std::vector<std::string>>& Rows() { static const auto rows=LoadFixture(NETSTORM_CROSSBOW_ANIMATION_FIXTURE).rows;return rows; }
// 동일 현재 풀·TYPE와 선언한 외부 경계를 가진 독립 입력 장면이다.
struct CrossbowScene {
    SidPool pool{OriginalEdition::Patch1078,kCapacity,false}; // 실제50BYTE 슬롯과 같은 SID 저장소다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 현재 표적 HP 플래그다.
    std::vector<ArcherTypeData> data=std::vector<ArcherTypeData>(188); // 방향/물리 프레임 보충 자료다.
    ArcherAnimationState rotateState; // 공유 회전의 현재 시계와 부모140이다.
    CrossbowAnimationState state; // 발사의 현재 시계/속도/표시 TYPE다.
    ArcherAnimationFields fields; // 회전88BYTE와 발사 앞64BYTE를 같은 입력으로 보존한다.
    const std::vector<std::string>* row{}; // 현재 독립 입력 행이다.
    std::vector<std::string> events; // 실제 경계 순서와 인수만 관찰한다.
    // 실제 부모50/51·표적60/61·GunForm70을 먼저 할당한다.
    CrossbowScene() { for (std::uint16_t id=5;id<=70;++id) CHECK(pool.Allocate(2)==Sid{id}); }
    // free 입력도 다음 행에서 재구성하기 위한 검사 전용 쓰기다.
    std::span<std::uint8_t> Input(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // 현재 독립 입력의 원본 정수/float 비트를 읽는다.
    std::uint32_t N(std::size_t index) const { return Number((*row)[index]); }
    // 프레임 실행 전 원본과 같은 패턴·raw 슬롯을 작성한다.
    void Prepare(const std::vector<std::string>& value) {
        CHECK(value.size()==16);row=&value;events.clear();state=CrossbowAnimationState{};state.now=10;rotateState=ArcherAnimationState{};rotateState.type=140;rotateState.now=10;data[140]={static_cast<std::int32_t>(N(5)),static_cast<std::int32_t>(N(6))};types[158].flags1=N(4) ? 0x10U : 0U;
        for (std::size_t i=0;i<fields.bytes.size();++i) fields.bytes[i]=static_cast<std::uint8_t>(i*13+7); // ctor의 미초기화 padding도 대조한다.
        const bool fire=N(0)==1 || N(0)==2 || N(0)==4;Put(fields.bytes,0,fire ? 0x50b120U : 0x50b450U);Put(fields.bytes,4,81);Put(fields.bytes,8,50);Put(fields.bytes,12,std::bit_cast<std::uint32_t>(60.0f));Put(fields.bytes,16,std::bit_cast<std::uint32_t>(60.0f));Put(fields.bytes,20,std::bit_cast<std::uint32_t>(59.5f));Put(fields.bytes,24,std::bit_cast<std::uint32_t>(59.5f));Put(fields.bytes,32,fire ? 0x50b11cU : 0x50b448U);fields.bytes[40]=0x20;Put(fields.bytes,42,70,2);Put(fields.bytes,44,N(10),2);const auto time=std::bit_cast<std::uint64_t>(N(2) ? 10.0 : 10.25);Put(fields.bytes,48,static_cast<std::uint32_t>(time));Put(fields.bytes,52,static_cast<std::uint32_t>(time>>32));Put(fields.bytes,56,0);
        for (const auto [sid,type]:{std::array<std::uint16_t,2>{50,140},{51,140},{60,158},{61,158},{70,34}}) { // 원문과 같은 네 관찰 슬롯과 Gun을 준비한다.
            auto raw=Input(Sid{sid});std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x1131a000);raw[10]=static_cast<std::uint8_t>(type);raw[11]=static_cast<std::uint8_t>(sid==60 ? N(3) : 0U);raw[12]=0xa5;raw[34]=1;raw[30]=static_cast<std::uint8_t>(N(9));Put(raw,14,std::bit_cast<std::uint32_t>(sid<60 ? 50.5f : 60.0f));Put(raw,18,std::bit_cast<std::uint32_t>(sid<60 ? 50.5f : 60.0f));Put(raw,36,N(1));
        }
    }
    // 원본 실행과 동일하게 선언한 외부 재진입만 현재 입력에 적용한다.
    void Mutate(std::string_view where) {
        const auto mutation=N(11);
        if (where=="Center" && mutation==1) Put(Input(Sid{50}),36,7);
        if (where=="Center" && mutation==2) Put(fields.bytes,44,61,2);
        if (where=="Unpop" && mutation==3) { state.now=100;const auto bits=std::bit_cast<std::uint64_t>(777.0);Put(fields.bytes,48,static_cast<std::uint32_t>(bits));Put(fields.bytes,52,static_cast<std::uint32_t>(bits>>32)); }
        if (where=="Repop" && mutation==4) Input(Sid{60})[11]=2;
        if (where=="Point" && mutation==5) { Put(fields.bytes,8,51);Put(fields.bytes,44,61,2);Put(fields.bytes,12,std::bit_cast<std::uint32_t>(77.25f));Put(fields.bytes,16,std::bit_cast<std::uint32_t>(78.5f));state.speed=37;state.displayType=82; }
        if (where=="Missile" && mutation==6) Put(fields.bytes,42,71,2);
    }
    // 실제 생성 descriptor의 인수만 직렬화하고 투사체 실행으로 간주하지 않는다.
    void Projectile(const GunProjectileRequest& request) {
        events.push_back("A:128");const std::array<std::uint32_t,12> args{request.firing.value,request.target.value,std::bit_cast<std::uint32_t>(request.speed),request.displayType,request.effectType,std::bit_cast<std::uint32_t>(request.targetPosition[0]),std::bit_cast<std::uint32_t>(request.targetPosition[1]),std::bit_cast<std::uint32_t>(request.targetCenter[0]),std::bit_cast<std::uint32_t>(request.targetCenter[1]),request.ricochet,std::bit_cast<std::uint32_t>(request.firingPosition[0]),std::bit_cast<std::uint32_t>(request.firingPosition[1])};std::string text="J:";
        for (const auto value:args) { if (text.size()>2) text+=',';text+=std::to_string(value); } // 원문 호출의12인수를 순서대로 관찰한다.
        events.push_back(text);Mutate("Missile");
    }
    // 실제 공통 경계의 인수와 순서를 원문과 같은 형태로 관찰한다.
    GunAnimationHooks Hooks() {
        GunAnimationHooks hooks;
        hooks.center=[this](Sid sid) { events.push_back("C:"+std::to_string(sid.value));const auto point=sid==Sid{50} ? std::array<float,2>{50,50} : std::array<float,2>{std::bit_cast<float>(N(7)),std::bit_cast<float>(N(8))};Mutate("Center");return point; };
        hooks.unpop=[this](Sid sid,std::uint32_t flags) { events.push_back("U:"+std::to_string(sid.value)+':'+std::to_string(flags));Mutate("Unpop"); };
        hooks.repop=[this](Sid sid,std::uint32_t flags) { events.push_back("P:"+std::to_string(sid.value)+':'+std::to_string(flags));Mutate("Repop"); };
        hooks.targetPoint=[this](Sid source,Sid target) { events.push_back("T:"+std::to_string(source.value)+':'+std::to_string(target.value));Mutate("Point");return std::array<float,2>{52.25f,52.75f}; };
        hooks.createProjectile=[this](const GunProjectileRequest& request) { Projectile(request); };
        hooks.notify=[this](Sid,std::uint32_t event,std::uint32_t info,std::uint32_t value) { events.push_back("M:"+std::to_string(event)+':'+std::to_string(info)+':'+std::to_string(value)); };
        hooks.kill=[this](Sid form,std::uint32_t flags) { events.push_back("K:"+std::to_string(form.value)+':'+std::to_string(flags)); };
        hooks.randomInteger=[this](std::int32_t low,std::int32_t high) { events.push_back("N:"+std::to_string(low)+':'+std::to_string(high));events.push_back("F:3");return 3000; };
        hooks.sound=[this](Sid,const std::string& name) { events.push_back("S:"+name); };return hooks;
    }
    // 발사 이미지는64BYTE만 복사하되 외부 효과가 같은 필드를 즉시 바꿀 수 있게 한다.
    std::vector<std::string> Run() {
        auto hooks=Hooks();ArcherAnimationHooks rotateHooks;rotateHooks.common=hooks;rotateHooks.aimDirection=[this](Sid sid) { return pool.Slot(sid)[30]; };RawArcherAnimation rotate(pool,types,data,rotateState,std::move(rotateHooks));std::string result="return";
        if (N(0)==0) rotate.Rotate(fields);
        else if (N(0)==3) { rotate.Initialize(fields,false,Sid{70},Sid{static_cast<std::uint16_t>(N(10))});events.push_back("B:36:50:0:16");Put(fields.bytes,4,81);Put(fields.bytes,8,50); }
        else {
            GunAnimationFields image;std::copy_n(fields.bytes.begin(),64,image.bytes.begin());
            // 동일 별칭을 가리키는 callback을 사용해 원본 외부 재진입 쓰기를 즉시 반영한다.
            auto active=Hooks();active.center=[this,&image,base=active.center](Sid sid) { std::copy_n(image.bytes.begin(),64,fields.bytes.begin());const auto value=base(sid);std::copy_n(fields.bytes.begin(),64,image.bytes.begin());return value; };
            active.unpop=[this,&image,base=active.unpop](Sid sid,std::uint32_t flags) { std::copy_n(image.bytes.begin(),64,fields.bytes.begin());base(sid,flags);std::copy_n(fields.bytes.begin(),64,image.bytes.begin()); };
            active.repop=[this,&image,base=active.repop](Sid sid,std::uint32_t flags) { std::copy_n(image.bytes.begin(),64,fields.bytes.begin());base(sid,flags);std::copy_n(fields.bytes.begin(),64,image.bytes.begin()); };
            active.targetPoint=[this,&image,base=active.targetPoint](Sid sid,Sid target) { std::copy_n(image.bytes.begin(),64,fields.bytes.begin());const auto value=base(sid,target);std::copy_n(fields.bytes.begin(),64,image.bytes.begin());return value; };
            active.createProjectile=[this,&image,base=active.createProjectile](const GunProjectileRequest& request) { std::copy_n(image.bytes.begin(),64,fields.bytes.begin());base(request);std::copy_n(fields.bytes.begin(),64,image.bytes.begin()); };
            RawCrossbowAnimation current(pool,types,data,state,std::move(active));
            if (N(0)==1) current.Frame(image);
            else if (N(0)==2) { current.Initialize(image,Sid{70},Sid{static_cast<std::uint16_t>(N(10))});events.push_back("B:35:50:0:16");Put(image.bytes,4,81);Put(image.bytes,8,50); }
            else result=current.Launch(image) ? "1" : "0";
            std::copy_n(image.bytes.begin(),64,fields.bytes.begin());
        }
        std::string actions,slots;
        for (const auto& event:events) { if (!actions.empty()) actions+=';';actions+=event; } // 실제 경계 순서만 합친다.
        for (const auto sid:{50U,51U,60U,61U}) slots+=Hex(pool.Slot(Sid{static_cast<std::uint16_t>(sid)})); // 네 원본 슬롯200BYTE를 전부 대조한다.
        return {result,actions,Hex(std::span<const std::uint8_t>(fields.bytes).first(N(0)==0 || N(0)==3 ? 88U : 64U)),slots};
    }
};
// 지정한 원본 몸체 집합을 독립 기대값과 대조하고 최초 차이의 행을 표시한다.
void Replay(std::uint32_t low,std::uint32_t high) {
    CrossbowScene scene;std::size_t count=0;unsigned failures=0;CHECK(Rows().size()==2638);
    for (const auto& row:Rows()) { // 저장된 실제 원본 입력만 재생한다.
        if (Number(row[0])<low || Number(row[0])>high) continue;scene.Prepare(row);const auto actual=scene.Run();const auto expected=std::vector<std::string>(row.begin()+12,row.end());const bool same=actual==expected;CHECK(same);
        if (!same) { for (std::size_t i=0;i<actual.size();++i) if (actual[i]!=expected[i]) std::printf("크로스보우 행%zu kind%s field%zu 기대%s / 실제%s\n",count,row[0].c_str(),i,expected[i].c_str(),actual[i].c_str());if (++failures==3) break; } // 최초 세 차이만 출력한다.
        ++count;
    }
    CHECK(count>0 && failures==0);
}
}
// 다른 부모 TYPE140의 실제 공유 회전을 원본812입력으로 대조한다.
TEST_CASE(CrossbowAnimation_NativeSharedRotation) { Replay(0,0); }
// 내림 프레임·due 전 추적 금지·생존 snapshot·외부 재진입을 원문1816입력으로 대조한다.
TEST_CASE(CrossbowAnimation_NativeFireAndReentry) { Replay(1,1); }
// 실제 두 생성자와 launch의 HP/free/dead/arrow 인자를 독립 대조한다.
TEST_CASE(CrossbowAnimation_NativeConstructorsAndLaunch) { Replay(2,4); }
// 실제 투사체 생성 경계를 빠뜨린 살아 있는 HP 표적은 성공으로 처리하지 않는다.
TEST_CASE(CrossbowAnimation_MissingMissileRejected) {
    CrossbowScene scene;const auto found=std::find_if(Rows().begin(),Rows().end(),[](const auto& row) { return Number(row[0])==4 && Number(row[3])==0 && Number(row[4])==1; });CHECK(found!=Rows().end());if (found==Rows().end()) return;scene.Prepare(*found);auto hooks=scene.Hooks();hooks.createProjectile={};RawCrossbowAnimation logic(scene.pool,scene.types,scene.data,scene.state,std::move(hooks));GunAnimationFields image;std::copy_n(scene.fields.bytes.begin(),64,image.bytes.begin());CHECK(Throws([&] { logic.Launch(image); }));CHECK(scene.events.empty());
}
