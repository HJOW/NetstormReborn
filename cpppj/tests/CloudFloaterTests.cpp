// 클라우드 플로터의 최초 Regular/공통 후처리 순서와 실제 event3 반환을 원본88행에 대조한다.
#include "RawSceneSupport.h"
#include "o/RawBalloonBody.h"
#include <filesystem>
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원본 입력과 실제 공통 몸체를 연결하되 큰 외부 효과의 순서만 기록한다.
struct CloudScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false};std::vector<RiftTypeRecord> types{188};std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); // 같은 월드 입력이다.
    CarrierPostPopState state;CarrierPreDestroyState preState;bool boss{true},allocated{true};std::string trace; // 공통 관문/명시 할당/관찰이다.
    std::unique_ptr<RawCarrierPostPop> post;std::unique_ptr<RawCarrierPreDestroy> pre;std::unique_ptr<RawBalloonBody> body; // 위 참조보다 먼저 해제되는 제품 몸체다.
    // 실제 슬롯과 현재 TYPE의 기본 벌룬 후처리를 준비한다.
    CloudScene() {
        for (;;) { const auto sid=pool.Allocate(2);if (sid.value==50) break;if (!sid.value || sid.value>50) throw std::logic_error("클라우드 검사 SID 준비 실패"); } // 부모50까지 실제 슬롯을 확보한다.
        auto raw=pool.AllocatedBytes(Sid{50});std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x504198);raw[10]=146;state.loadingDepth=0;
        CarrierPostPopHooks p;p.validateBase=[](Sid,std::uint32_t) {};p.base=[this](Sid,std::uint32_t flags) { Event("Balloon,"+std::to_string(flags)); };p.carrierCheck=[](Sid) {};p.claimGround=[](float,float,std::uint8_t) {};
        post=std::make_unique<RawCarrierPostPop>(pool,types,state,p);CarrierPreDestroyHooks q;q.validateDamageable=[](Sid,std::uint32_t) {};q.damageablePre=[](Sid,std::uint32_t) {};q.dependentFound=[] {};pre=std::make_unique<RawCarrierPreDestroy>(pool,preState,q);
        body=std::make_unique<RawBalloonBody>(pool,types,spots,boss,*post,*pre,Hooks());
    }
    void Event(std::string value) { if (!trace.empty()) trace+=';';trace+=value; } // 명시 효과의 순서다.
    BalloonBodyHooks Hooks() {
        BalloonBodyHooks h;h.frames=[](std::uint32_t) { return 4; };h.check=[](Sid) {};
        h.allocateRegular=[this] { Event("Allocate,40");return allocated; };h.constructRegular=[this](Sid sid,std::uint32_t event,float value) { Event("Regular,"+std::to_string(event)+','+std::to_string(sid.value)+','+std::to_string(std::bit_cast<std::uint32_t>(value))); };return h;
    } // 기존 그림자 동작은 기존 전체 fixture가 따로 검증한다.
};
// 원본과 같은 명시 입력을 매번 새 월드에 재생한다.
void CompareCloud(char kind,std::size_t expected) {
    const auto fixture=LoadFixture(NETSTORM_CLOUD_FLOATER_FIXTURE);std::size_t seen{};
    for (std::size_t i=1;i<fixture.rows.size();++i) { // 해당 함수의 원본 입력만 비교한다.
        const auto& row=fixture.rows[i];if (row[0]!=std::string(1,kind)) continue;++seen;const auto in=Split(row[1],',');CloudScene scene;std::uint32_t result=0xa5a5a5a5;
        if (kind=='P') { scene.allocated=Number(in[1])!=0;scene.body->PostPop(Sid{50},Number(in[0])); }
        else {
            const auto value=scene.body->CloudRegular(Sid{50},Number(in[0]),Number(in[1]),std::bit_cast<float>(Number(in[2])),
                [&](Sid,std::int32_t delta,std::uint32_t flags) { scene.Event("Advance,"+std::to_string(delta)+','+std::to_string(flags)); },
                [&](Sid,std::uint32_t event,std::uint32_t count,float payload) { scene.Event("Carrier,"+std::to_string(event)+','+std::to_string(count)+','+std::to_string(std::bit_cast<std::uint32_t>(payload)));return -3.5f; });result=std::bit_cast<std::uint32_t>(value);
        }
        CHECK((scene.trace.empty() ? "-" : scene.trace)==row[2]);CHECK(result==Number(row[3]));CHECK(scene.body->Handles(Sid{50}));
    }
    CHECK(seen==expected);
}
}
TEST_CASE(CloudFloater_NativePostPop24) { CompareCloud('P',24); } // 서버 제한 없이 최초 비트와 NULL 할당을 보존한다.
TEST_CASE(CloudFloater_NativeRegular64) { CompareCloud('R',64); } // 정확한event3/현재payload 반환과 기본 위임이다.
// 미선택 하위는 요구하지 않으며 실제 선택된 누락만 앞선 효과 뒤 거부한다.
TEST_CASE(CloudFloater_SelectedEffectsRequired) {
    CloudScene scene;auto h=scene.Hooks();h.allocateRegular={};RawBalloonBody body(scene.pool,scene.types,scene.spots,scene.boss,*scene.post,*scene.pre,h);
    body.PostPop(Sid{50},0);CHECK(scene.trace=="Balloon,0");CHECK(Throws([&] { body.PostPop(Sid{50},1); }));
    CHECK(Throws([&] { static_cast<void>(body.CloudRegular(Sid{50},3,0,0.1f,{},{})); }));
    Put(scene.pool.AllocatedBytes(Sid{50}),0,0x5040c0);CHECK(body.Handles(Sid{50}));CHECK(Throws([&] { static_cast<void>(body.CloudRegular(Sid{50},3,0,0.1f,{},{})); }));
}
