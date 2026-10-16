//실제 생성 함수의 전체 부모/새 슬롯과 호출 순서를 독립 원본578개와 대조한다.
#include "RawSceneSupport.h"
#include "o/RawFenceCreate.h"
#include "o/RawFenceRebuild.h"
#include "o/SquidFactory.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::uint32_t kCreateVtable=0x124f0000; //격리 원본 실행기의 명시 가상 경계 주소 기록값이다.
//실제 TYPE 구간 계산기로 A가변/J8/K8의 입력 표를 만든다.
RiftTypeFrames FenceCreateCodes(std::uint32_t first) {
    std::vector<FrameCode> codes; //입력 프레임 코드이며 기대 결과는 계산하지 않는다.
    for (std::uint32_t index=0;index<first+16;++index) codes.push_back({static_cast<std::uint8_t>(index<first ? 'A' : index<first+8 ? 'J' : 'K'),'A',static_cast<std::uint8_t>(index),0}); //원본+154/+158 첫 프레임과 대응한다.
    return RiftTypeFrames(std::move(codes));
}
//같은 Pool/현재 중심과 명시 하위 효과 입력만 조립한다.
struct FenceCreateScene {
    SidPool pool{OriginalEdition::Patch1078,kCapacity,true}; //고정 서버/예측 영역과 높은 끝점SID를 포함한다.
    SquidHash hash; //중심 조회기가 공유하는 실제 월드의 해시다.
    std::vector<RiftTypeRecord> types{188}; //현재1×1 발자국을 공급하는 실제TYPE 표다.
    std::vector<GunTargetType> gunTypes{188}; //같은 실제 중심 조회기의 부가 표다.
    std::vector<SquidDisplayShape> shapes{188}; //이 관찰에서는 중심만 사용하므로 미사용 SHP 경계다.
    std::vector<RiftTypeFrames> frames{188,FenceCreateCodes(8)}; //실제 포스필드J/K 첫 번호 표다.
    GunTargetState targetState; //현재 중심/표적 조회 상태다.
    RawGunTarget target{pool,hash,types,gunTypes,shapes,targetState}; //제품의 실제004adea0 복원 계산이다.
    SquidFactory fixtureFactory{pool,types}; //높은 입력SID의 Take에만 사용하는 실제 입력 설치기다.
    std::uint32_t markerType{90},shieldType{170}; //원본의 서로 다른 현재 생성 타입 전역이다.
    char kind{}; //M 표식/S 포스필드/R 재생성 제어다.
    std::vector<std::uint32_t> input; //원본 행의 입력값들이다.
    std::vector<Sid> parents; //원본이 기록한 부모/이웃의 순서다.
    std::size_t created{}; //명시 create 경계가 반환한 새 객체 수다.
    std::uint32_t other{51},mutation{},blockAt{},direction{}; //현재 입력 끝점/외부 변경/차단/방향이다.
    std::string trace; //실제로 호출한 명시 효과의 순서다.
    std::unique_ptr<RawFenceMarkerCreate> markers; //같은 현재 풀/중심/생성 소비자의 실제 표식 생성기다.
    std::unique_ptr<RawFenceShieldCreate> shields; //같은 현재TYPE/생성 소비자의 실제 화면 생성기다.
    std::unique_ptr<RawFenceRebuild> rebuild; //같은 현재 풀에서 실제 재생성 제어를 실행한다.
    //원본과 같은 입력 기록만 설치하고 생성 판단/순서는 제품 코드에 맡긴다.
    FenceCreateScene(char scope,const std::string& values):kind(scope) {
        for (const auto& value:Split(values,',')) input.push_back(Number(value)); //관찰 입력의 정수 비트를 읽는다.
        if (kind=='M') { other=input[7];mutation=input[6]; } else if (kind=='S') { blockAt=input[8];mutation=input[9]; } else mutation=input[4];
        const auto lastClient=static_cast<std::uint16_t>(other<pool.Layout().serverFirst ? std::max(other,159U) : 159U); //명시 생성이 반환할60~159와 높은 클라이언트 끝점을 입력으로 확보한다.
        for (std::uint16_t sid=5;sid<=lastClient;++sid) CHECK(pool.Allocate(2)==Sid{sid}); //실제 allocator 자체는 이번 원본 관찰의 외부 경계다.
        if (other>=pool.Layout().serverFirst) fixtureFactory.Take(136,Sid{static_cast<std::uint16_t>(other)}); //지정 Take는 원본 계약대로 서버 영역 끝점에만 사용한다.
        for (const auto type:{90U,136U,170U,171U}) types[type].footX=types[type].footY=1; //현재 중심을 계산하는 실제 발자국이다.
        frames[171]=FenceCreateCodes(13);
        if (kind=='R') for (std::uint16_t sid=50;sid<=54;++sid) parents.push_back(Sid{sid}); //재생성은 네 실제 이웃 모두를 결과에 포함한다.
        else parents={kSource,Sid{static_cast<std::uint16_t>(other)}};
        for (const auto sid:parents) { //부모의 모든 보존 바이트를 원본 입력과 동일하게 준비한다.
            auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,0,kCreateVtable);raw[10]=136;raw[11]=0;raw[40]=0;raw[34]=1;
            if (kind=='R') { Put(raw,14,std::bit_cast<std::uint32_t>(float(sid.value)+.25f));Put(raw,18,std::bit_cast<std::uint32_t>(float(sid.value)+.5f));raw[40]=static_cast<std::uint8_t>(sid==kSource ? input[0] : input[3]>>((sid.value-51)*8)); }
            else { Put(raw,14,input[sid==kSource ? 0 : 2]);Put(raw,18,input[sid==kSource ? 1 : 3]);raw[34]=static_cast<std::uint8_t>(input[sid==kSource ? 4 : 5]); }
        }
        markers=std::make_unique<RawFenceMarkerCreate>(pool,target,markerType,CreateHooks());shields=std::make_unique<RawFenceShieldCreate>(pool,frames,shieldType,CreateHooks());
        rebuild=std::make_unique<RawFenceRebuild>(pool,RebuildHooks());
    }
    void Event(std::string value) { if (!trace.empty()) trace+=';';trace+=value; } //명시 경계를 실제 호출 순서로 기록한다.
    //새 객체/표시의 큰 몸체는 원본 관찰과 동일한 입력/현재 변경 경계다.
    FenceCreateHooks CreateHooks() {
        FenceCreateHooks hooks;
        hooks.create=[this](std::uint32_t type,std::uint32_t flags) {
            CHECK(flags==2);const auto sid=Sid{static_cast<std::uint16_t>(60+created)};++created;Event("Create,"+std::to_string(type)+','+std::to_string(flags)+','+std::to_string(sid.value)); //원본 관찰과 같은 순서로 미리 확보한 명시 경계 슬롯을 반환한다.
            auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{0xa5});Put(raw,0,kCreateVtable);raw[10]=static_cast<std::uint8_t>(kind=='S' && mutation==1 ? 171 : type);raw[11]=0;raw[34]=0;raw[40]=static_cast<std::uint8_t>(kind=='S' && blockAt==created ? 32 : 0);Put(raw,36,17);
            if (kind=='S' && mutation==3 && created==1) shieldType=171;return sid;
        };
        hooks.notifySurface=[this](Sid sid) { Event("Notify,"+std::to_string(sid.value));if (kind=='M' && mutation==1) pool.AllocatedBytes(kSource)[34]=7; };
        hooks.setOwner=[this](Sid sid,std::uint32_t owner) { Event("Owner,"+std::to_string(sid.value)+','+std::to_string(owner));pool.AllocatedBytes(sid)[34]=static_cast<std::uint8_t>(owner);if (kind=='M' && mutation==2) pool.AllocatedBytes(kSource)[34]=6;if (kind=='S' && mutation==2) Put(pool.AllocatedBytes(kSource),14,std::bit_cast<std::uint32_t>(199.0f)); };
        hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) {
            CHECK(flags==64);Event("Pop,"+std::to_string(sid.value)+','+std::to_string(std::bit_cast<std::uint32_t>(x))+','+std::to_string(std::bit_cast<std::uint32_t>(y))+','+std::to_string(flags));Put(pool.AllocatedBytes(sid),14,std::bit_cast<std::uint32_t>(x));Put(pool.AllocatedBytes(sid),18,std::bit_cast<std::uint32_t>(y));
            if (kind=='M' && mutation==3) { Put(pool.AllocatedBytes(kSource),14,std::bit_cast<std::uint32_t>(199.0f));Put(pool.AllocatedBytes(Sid{static_cast<std::uint16_t>(other)}),18,std::bit_cast<std::uint32_t>(200.0f)); }
        };return hooks;
    }
    //재생성 관찰의 이웃/거리/하위 생성은 선언한 외부 입력이며 실제 Session은 제품 모듈을 연결한다.
    FenceRebuildHooks RebuildHooks() {
        FenceRebuildHooks hooks;
        hooks.neighbor=[this](Sid sid,std::uint32_t value) { CHECK(sid==kSource);direction=value;Event("Neighbor,"+std::to_string(value));if (mutation==1 && value==0) Put(pool.AllocatedBytes(kSource),14,std::bit_cast<std::uint32_t>(120.0f));return Sid{static_cast<std::uint16_t>(input[1]&(1U<<(value/2)) ? 51+value/2 : 0)}; };
        hooks.withinRange=[this](Sid sid,std::array<float,4> points) { CHECK(sid==kSource);Event(PointsTrace("Range",points));if (mutation==2) pool.AllocatedBytes(Sid{static_cast<std::uint16_t>(51+direction/2)})[40]=0x21;return (input[2]&(1U<<(direction/2)))!=0; };
        hooks.markers=[this](Sid sid,Sid otherSid) { CHECK(sid==kSource);Event("Markers,"+std::to_string(otherSid.value));if (mutation==3) { pool.AllocatedBytes(kSource)[34]=5;Put(pool.AllocatedBytes(otherSid),18,std::bit_cast<std::uint32_t>(125.0f)); } };
        hooks.shields=[this](std::array<float,4> points,std::uint32_t a,std::uint32_t b,std::int32_t first,std::int32_t second) { Event(PointsTrace("Shields",points)+",1,"+std::to_string(a)+','+std::to_string(b)+','+std::to_string(first)+','+std::to_string(second));if (mutation==4) pool.AllocatedBytes(kSource)[40]=9; };
        return hooks;
    }
    //네float 인수의 원본 비트를 순서대로 기록한다.
    static std::string PointsTrace(std::string prefix,std::array<float,4> points) { for (const auto value:points) prefix+=','+std::to_string(std::bit_cast<std::uint32_t>(value));return prefix; }
    void Run() { if (kind=='M') markers->Create(kSource,Sid{static_cast<std::uint16_t>(other)});else if (kind=='S') shields->Create({std::bit_cast<float>(input[0]),std::bit_cast<float>(input[1]),std::bit_cast<float>(input[2]),std::bit_cast<float>(input[3])},input[4],input[5],std::bit_cast<std::int32_t>(input[6]),std::bit_cast<std::int32_t>(input[7]));else rebuild->Rebuild(kSource); } //해당 실제 부모 함수만 실행한다.
    std::string Parents() const { std::string value;for (const auto sid:parents) value+=Hex(pool.Slot(sid));return value; } //원본이 기록한 전체 부모 바이트다.
    std::string Created() const { std::string value;for (std::size_t index=0;index<created;++index) value+=Hex(pool.Slot(Sid{static_cast<std::uint16_t>(60+index)}));return value.empty() ? "-" : value; } //선택한 새 객체들의 전체50BYTE다.
};
//별도 범위마다 원본 결과를 전수 비교해 한 실패가 다른 생성 경로를 감추지 않게 한다.
void CompareFenceCreate(char kind,std::size_t expected) {
    const auto fixture=LoadFixture(NETSTORM_FENCE_CREATE_FIXTURE);std::size_t count{}; //독립 원본 입력의 비교 수다.
    for (const auto& row:fixture.rows) { //기대값은 실제 원본에서만 읽는다.
        CHECK(row.size()==5);if (row[0]!=std::string(1,kind)) continue;FenceCreateScene scene(kind,row[1]);scene.Run();const auto trace=scene.trace.empty() ? "-" : scene.trace;
        const bool same=scene.Parents()==row[2] && scene.Created()==row[3] && trace==row[4];CHECK(same);++count;
        if (!same) { std::fprintf(stderr,"펜스 생성 불일치 %c 입력%s 생성%zu\n실제%s\n기대%s\n",kind,row[1].c_str(),scene.created,trace.c_str(),row[4].c_str());break; }
    }
    CHECK(count==expected);
}
}
TEST_CASE(FenceCreate_NativeMarkers106) { CompareFenceCreate('M',106); } //현재 중심/4칸 구간/high8·low8 태그/후속 owner를 대조한다.
TEST_CASE(FenceCreate_NativeShields306) { CompareFenceCreate('S',306); } //좌표/J·K 직접Frame/100상한/차단/signed SID owner를 대조한다.
TEST_CASE(FenceCreate_NativeRebuild166) { CompareFenceCreate('R',166); } //진입 관문/거리 뒤 대상extra/생성 뒤 현재 인수다.
//선택되지 않은 소비자는 필요 없고 선택 실패는 앞선 새 객체/QA/Frame 쓰기를 보존한다.
TEST_CASE(FenceCreate_RequiredBoundariesAndPartialWrites) {
    FenceCreateScene markers('M',"1112080384,1112145920,1114308608,1112145920,1,2,0,51");auto hooks=markers.CreateHooks();hooks.notifySurface={};RawFenceMarkerCreate missing(markers.pool,markers.target,markers.markerType,std::move(hooks));bool failed{};
    try { missing.Create(kSource,Sid{51}); } catch (const std::logic_error&) { failed=true; }CHECK(failed && markers.created==1 && Get(markers.pool.Slot(Sid{60}),12,2)==50);
    FenceCreateScene blocked('S',"1112080384,1112145920,1114308608,1112145920,1,2,50,51,1,0");hooks=blocked.CreateHooks();hooks.setOwner={};hooks.pop={};RawFenceShieldCreate early(blocked.pool,blocked.frames,blocked.shieldType,std::move(hooks));early.Create({50.25f,50.5f,58.75f,50.5f},1,2,50,51);CHECK(blocked.created==1 && Get(blocked.pool.Slot(Sid{60}),36)==17);
    FenceCreateScene rebuild('R',"1,15,15,0,0");RawFenceRebuild noEffects(rebuild.pool,{});noEffects.Rebuild(kSource);CHECK(rebuild.trace.empty());rebuild.pool.AllocatedBytes(kSource)[40]=0;failed=false;try { noEffects.Rebuild(kSource); } catch (const std::logic_error&) { failed=true; }CHECK(failed);
}
