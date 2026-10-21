// AI 생산 소비·비용·대기·권한별 실제 배치 계약의 합성 검사다. 중단 지시 해제 전에는 작성·컴파일만 한다.
#include "o/RawAiDeployment.h"
#include "RawSceneSupport.h"
#include <limits>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 실제 내용물 생성과 명시 월드 효과를 한 풀/본체에 연결한다.
struct DeploymentInput {
    RawAiBrainMemory memory{0x45000000,[](auto raw) { std::fill(raw.begin(),raw.end(),std::uint8_t{}); }}; //명시0 본체 입력이다.
    std::uint32_t token{memory.Allocate(RawAiBrainConstructor::kBytes)},type{133}; //실제 소유 본체와 현재 타입 입력이다.
    SidPool pool{OriginalEdition::Patch1078,32768,true};std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); //같은 PATCH 풀/TYPE다.
    ContainedCreateState contentState;std::unique_ptr<RawContainedCreate> create; //원본 필드 쓰기와 실제 할당을 재사용한다.
    Sid parent{},product{};std::vector<Sid> born; //명시 기술부모/생산 입력과 실제 새 내용물이다.
    bool authority{true},move{},reserve{true},demo{};double now{16777216.25}; //현재 권한/이동 플래그/할당 결과/미션/시각 입력이다.
    AiDeploymentHooks hooks;std::vector<std::string> trace,assertions; //선택 효과와 실제 호출 순서다.
    std::vector<ConstructionConfirmRequest> requests;std::vector<ConstructionLocalRequest> locals;std::vector<std::pair<Sid,float>> timers;std::vector<std::uint32_t> owners; //실제 전달 인자의 관찰값이다.
    // 생산 항목과 기존 생성기를 같은 실제 저장소에 준비한다.
    DeploymentInput() {
        parent=New();product=New();types[type].cost=123.75F;types[type].flags2=0x4a00;Field(0x260,2);Field(0x2d8,1000);Field(0x224,std::bit_cast<std::uint32_t>(2.5F));
        ContainedCreateHooks contents;contents.create=[this](auto number,auto flags) { CHECK(number==6 && flags==2);const auto form=New();pool.AllocatedBytes(form)[10]=6;born.push_back(form);return form; };contents.notifySurface=[](Sid) { throw std::logic_error("검사 외 표면 통지"); };contents.attach=[this](Sid form,Sid owner,std::uint32_t flags) { CHECK(owner==parent && flags==0x10);Put(pool.AllocatedBytes(form),4,Get(pool.Slot(parent),6,2),2);Put(pool.AllocatedBytes(parent),6,form.value,2);trace.push_back("create"); };create=std::make_unique<RawContainedCreate>(pool,contentState,std::move(contents));
        hooks.product=[this](auto brain,auto number) { CHECK(brain==token && number==type);trace.push_back("product");return product; };hooks.technology=[this](auto owner) { owners.push_back(owner);trace.push_back("technology");return parent; };
        hooks.destroy=[this](Sid form,auto flags) { CHECK(form==product && flags==0);product={};trace.push_back("destroy"); };hooks.reserveRegular=[this] { trace.push_back("reserve");return reserve; };hooks.addRegular=[this](Sid form,float rate) { timers.emplace_back(form,rate);trace.push_back("timer"); };
        hooks.request=[this](const auto& request) { CHECK(move);requests.push_back(request);trace.push_back("server"); };hooks.localPlace=[this](auto request) { CHECK(move);locals.push_back(request);trace.push_back("local"); };hooks.demo=[this] { trace.push_back("demo");return demo; };hooks.assertion=[this](auto text) { assertions.emplace_back(text);trace.push_back("assert"); };
    }
    // 실제 할당 블록을 명시0 입력으로 준비한다.
    Sid New() { const auto sid=pool.Allocate(2);auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});return sid; }
    // 현재 본체 DWORD를 읽는다.
    std::uint32_t Field(std::size_t at) const { return Get(memory.Bytes(token),at); }
    // 현재 본체의 해당 DWORD만 쓴다.
    void Field(std::size_t at,std::uint32_t value) { Put(memory.Bytes(token),at,value); }
    // 현재 QWORD 기한을 읽는다.
    double Time(std::size_t at) const { return std::bit_cast<double>((std::uint64_t(Field(at+4))<<32)|Field(at)); }
    // 같은 본체/실제 내용물/현재 전역을 조립한다.
    RawAiDeployment Make() { return RawAiDeployment(memory,pool,types,*create,AiDeploymentGlobals{authority,move,now},hooks); }
};

// 기존 생산 소비→대기 항목/Regular→비용→10인자 서버 요청→이동 해제/종류별 기한 순서다.
TEST_CASE(AiDeployment_ServerProductAndCooldownOrder) {
    DeploymentInput input;input.Make().Place(input.token,input.type,40.25F,41.5F,3,7);CHECK(input.trace==std::vector<std::string>({"product","destroy","technology","create","reserve","timer","demo","server"}));CHECK(input.born.size()==1 && input.timers.size()==1 && input.requests.size()==1 && input.Field(0x2d8)==877 && !input.move);if (!input.born.empty()) CHECK(Get(input.pool.Slot(input.born[0]),20,2)==0x108 && Get(input.pool.Slot(input.born[0]),18,2)==133);if (!input.timers.empty()) CHECK(input.timers[0]==std::pair(input.born[0],2.5F));
    if (!input.requests.empty()) { const auto& request=input.requests[0];CHECK(request.type==133 && request.x==40.25F && request.y==41.5F && request.player==2 && request.argument==3 && request.direction==7 && !request.flags && !request.timeLow && !request.timeHigh && !request.quality); }CHECK(input.Time(0x5b8)==input.now+20 && input.Time(0x5c0)==input.now+20);
}
// new 실패는 대기 조각/비용/배치 전체를 되돌리거나 중단하지 않는다.
TEST_CASE(AiDeployment_RegularAllocationFailureStillPlaces) {
    DeploymentInput input;input.reserve=false;input.hooks.addRegular={};input.Make().Place(input.token,input.type,10,11,0,0);CHECK(input.born.size()==1 && input.timers.empty() && input.requests.size()==1 && input.Field(0x2d8)==877 && !input.move);
}
// 생산이 없으면 일반 타입은 무료 배치하며 시설 flags4200만 별도 비용을 지불한다.
TEST_CASE(AiDeployment_MissingProductTypeBranches) {
    DeploymentInput plain;plain.product={};plain.types[plain.type].flags2=0;plain.hooks.reserveRegular={};plain.hooks.demo={};plain.Make().Place(plain.token,plain.type,10,11,0,0);CHECK(plain.Field(0x2d8)==1000 && plain.requests.size()==1 && plain.born.empty() && plain.Time(0x5b8)==0 && plain.Time(0x5c0)==0);
    DeploymentInput facility;facility.product={};facility.types[facility.type].flags2=0x4200;facility.Make().Place(facility.token,facility.type,10,11,0,0);CHECK(facility.Field(0x2d8)==877 && facility.born.empty() && facility.Time(0x5b8)==0 && facility.Time(0x5c0)==facility.now+20);
}
// 원본 진단이 반환하면 현재 flags2를 다시 읽어 비용 여부를 결정한다.
TEST_CASE(AiDeployment_MissingProductAssertionRechecksType) {
    DeploymentInput input;input.product={};input.types[input.type].flags2=0x70000;input.hooks.assertion=[&](auto text) { input.assertions.emplace_back(text);input.types[input.type].flags2=0x4200; };input.Make().Place(input.token,input.type,10,11,0,0);CHECK(input.assertions.size()==1 && input.Field(0x2d8)==877 && input.requests.size()==1);
}
// 비권한 경로는 별도 예측 조각을 만들고 기존 로컬5인자 배치에 그대로 전달한다.
TEST_CASE(AiDeployment_ClientPredictionUsesSecondContainedForm) {
    DeploymentInput input;input.authority=false;input.hooks.request={};input.Make().Place(input.token,input.type,17.25F,29.5F,255,0x123);CHECK(input.requests.empty() && input.locals.size()==1 && input.born.size()==2 && input.timers.size()==1 && !input.move);if (!input.locals.empty()) { const auto& request=input.locals[0];CHECK(request.preview==input.born[1] && request.x==17.25F && request.y==29.5F && request.argument==255 && request.direction==0x123 && Get(input.pool.Slot(request.preview),20,2)==0x108); }
}
// signed 비용 절삭/감김/데모 재조회와 물리 TYPE/본체 경계를 확인한다.
TEST_CASE(AiDeployment_CostTruncationAndDemoCurrentWallet) {
    DeploymentInput input;auto deployment=input.Make();input.Field(0x2d8,123);CHECK(deployment.CanAfford(input.token,input.type));input.Field(0x2d8,122);CHECK(!deployment.CanAfford(input.token,input.type));input.types[input.type].cost=-1.9F;deployment.Charge(input.token,input.type);CHECK(input.Field(0x2d8)==123);input.types[input.type].cost=std::numeric_limits<float>::quiet_NaN();deployment.Charge(input.token,input.type);CHECK(input.Field(0x2d8)==123);input.types[input.type].cost=1.0F;input.Field(0x2d8,0x80000000U);deployment.Charge(input.token,input.type);CHECK(input.Field(0x2d8)==0x7fffffffU);
    input.hooks.demo=[&] { input.Field(0x2d8,1191);return true; };input.Make().Charge(input.token,input.type);CHECK(input.Field(0x2d8)==1191);CHECK(Throws([&] { input.Make().CanAfford(input.token,188); }));const auto small=input.memory.Allocate(4);CHECK(Throws([&] { input.Make().Charge(small,input.type); }));
}
// 선택된 누락 하위는 앞선 실제 쓰기를 유지한 채 실패하며 원본 이동 플래그를 임의 복구하지 않는다.
TEST_CASE(AiDeployment_SelectedMissingEffectsPreservePrefix) {
    DeploymentInput input;input.hooks.request={};CHECK(Throws([&] { input.Make().Place(input.token,input.type,10,11,0,0); }));CHECK(input.move && input.born.size()==1 && input.Field(0x2d8)==877 && input.Time(0x5b8)==0);
    DeploymentInput missing;missing.hooks.product={};CHECK(Throws([&] { missing.Make().Place(missing.token,missing.type,10,11,0,0); }));CHECK(!missing.move && missing.born.empty() && missing.Field(0x2d8)==1000);
}
// 예약 뒤 현재 rate/비용/owner·실제 요청 뒤 현재 시각을 각각 다시 읽는다.
TEST_CASE(AiDeployment_CurrentInputsAfterChildEffects) {
    DeploymentInput input;input.hooks.reserveRegular=[&] { input.Field(0x224,std::bit_cast<std::uint32_t>(5.5F));input.Field(0x260,3);return true; };input.hooks.addRegular=[&](Sid form,float duration) { input.timers.emplace_back(form,duration);input.types[input.type].cost=12.5F; };input.hooks.request=[&](const auto& request) { CHECK(input.move);input.requests.push_back(request);input.now+=1; };input.Make().Place(input.token,input.type,10,11,0,0);CHECK(input.owners==std::vector<std::uint32_t>({2}) && input.timers[0].second==5.5F && input.requests[0].player==3 && input.Field(0x2d8)==988 && input.Time(0x5b8)==input.now+20 && !input.move);
}
}
