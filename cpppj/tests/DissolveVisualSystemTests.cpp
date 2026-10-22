// 분해가 대상 삭제 후 같은 실제 Kernel/Host/표시 목록에서 소진되는지 검사한다.
#include "RawSceneSupport.h"
#include "o/DissolveVisualSystem.h"
#include "o/DependFormHost.h"
#include "o/SquidFormDispatch.h"
#include "o/SquidFactory.h"

using namespace netstorm::test::rawscene;
namespace {
// 캡처/Pop의 장치 경계는 명시 입력이며 실제 미션/SHP 대조를 대신하지 않는다.
struct DissolveScene {
    SidPool pool{OriginalEdition::Patch1078,32768}; //객체/Form/감시 쌍이 공유하는 실제 풀이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); //원본 타입 범위의 명시 합성 메타다.
    SquidFactory factory{pool,types}; //같은110 더미의 실제 생성자다.
    SquidHash hash; //같은 공통 공간 해제기가 참조한다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); //공통 삭제의 spot 저장소다.
    SquidUnpop unpop{pool,hash,spots}; //현재 객체의 원본 공간 해제다.
    RawSquidDestroy destroy{pool,unpop,types}; //서로 종속된 Form의 실제 공통 삭제다.
    SquidFormDispatch dispatch; //Process/Depend/자산의 같은 가상 분배다.
    Kernel kernel; //별도 효과 루프 없이 실제 프로세스를 실행한다.
    SquidProcessState clock; //동일 Host 시각/권한이다.
    SquidProcessHost host; //client18 Form의 실제 소유자다.
    DependFormHost depends; //Form과 dummy의 실제 상호 감시 쌍이다.
    ConstructionVisualState visual; //단일 표시 목록의16·11 배율이다.
    RenderProcessSystem rendering; //다른 표시와 공유 가능한 실제 registry다.
    GameRandom random{0x12345678}; //같은 게임 난수를 분해 준비가 소비한다.
    ShrapnelFrameState frame; //현재 Kernel 시각/카메라다.
    std::vector<ShrapnelRelease> releases; //원본 저장소의 실제 반납 순서다.
    std::vector<ShrapnelPixel> pixels; //실제 Draw가 요청한 현재 픽셀이다.
    std::vector<FieryTailDirtyRect> dirty; //Frame/삭제의 원본 변경 영역이다.
    Sid source{},first{},second{},dummy{}; //표적/두 폴/새 더미의 실제 번호다.
    bool transparent{},failObject{},failCapture{}; //명시 경계 실패와 new0의 입력이다.
    std::unique_ptr<DissolveVisualSystem> effect; //Host/registry보다 먼저 소멸한다.

    // 실제 Form 분배와 의존 통지를 묶고 세 합성 부모를 배치한다.
    DissolveScene():dispatch(pool,destroy,Assets(),[this](Sid sid,std::uint32_t flags,const SquidDestroyHooks& hooks) { destroy.Destroy(sid,flags,hooks);return true; }),
        host(pool,types,kernel,destroy,clock,{},dispatch.Hooks()),
        depends(pool,destroy,[this](Sid watcher,Sid watched,Sid form) { if (pool.Slot(watcher)[10]==110) dispatch.Destroy(watcher,0);else host.NotifyDependency(watcher,watched,form); },dispatch.Hooks()),
        rendering(pool,host,visual,{[](std::uint32_t) { return true; },[] {},{}}) {
        dispatch.Bind(host);dispatch.Bind(depends);source=Create(133,2.125F,3.25F);first=Create(145,0,0);second=Create(145,10,14);frame.now=1;
        effect=std::make_unique<DissolveVisualSystem>(rendering,random,frame,Hooks());
    }
    // 파생 자산 효과의 완료 경계이며 Form의 실제 수명은 분배기가 처리한다.
    SquidDestroyHooks Assets() {
        SquidDestroyHooks hooks;hooks.selected=[] { return Sid{}; };hooks.emit=[this](const SquidDestroyEvent& event) {
            if (event.effect==SquidDestroyEffect::PreDestroy) destroy.CompletePreDestroy();
            if (event.effect==SquidDestroyEffect::PostDestroy) destroy.CompletePostDestroy();
        };return hooks;
    }
    // 실제 생성자가 쓴 가상 표/타입을 보존하고 명시 좌표만 공급한다.
    Sid Create(std::uint32_t type,float x,float y) {
        const auto sid=factory.Create(type,2);auto raw=pool.AllocatedBytes(sid);Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y));return sid;
    }
    // 준비/프레임은 원본 모듈이며 캡처만3×4의 독립 입력이다.
    DissolveSystemHooks Hooks() {
        DissolveSystemHooks hooks;hooks.lifetime.reserve=[this](std::uint32_t bytes) { CHECK(bytes==120);return !failObject; };
        hooks.lifetime.create=[this](std::uint32_t type,std::uint32_t flags) { CHECK(type==110 && flags==2);return dummy=factory.Create(type,flags); };
        hooks.lifetime.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { CHECK(sid==dummy && flags==16);auto raw=pool.AllocatedBytes(sid);Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y)); };
        hooks.lifetime.depend=[this](Sid form,Sid parent) { depends.Create(form,parent); };
        hooks.lifetime.capture=[this](Sid sid) {
            CHECK(sid==source && kernel.Size()==1 && rendering.Size()==1);if (failCapture) throw std::runtime_error("명시 캡처 실패");
            kernel.RunFrame();rendering.Draw();CHECK(pixels.empty());return ConstructionVisualImage{3,4,1,3,std::vector<std::uint8_t>(12,transparent ? 255 : 71)};
        };
        hooks.frame.reserve=[](std::uint32_t) { return true; };hooks.frame.release=[](bool) {};
        hooks.frame.invalidate=[this](FieryTailDirtyRect area) { dirty.push_back(area); };hooks.frame.pixel=[this](ShrapnelPixel pixel) { pixels.push_back(pixel); };
        hooks.release=[this](ShrapnelRelease value) { releases.push_back(value); };hooks.destroyUnpaired=[this](Sid sid) { dispatch.Destroy(sid,16); };return hooks;
    }
};
}
// 대상이 먼저 삭제돼도 저장한 bitmap과 더미의 분해18이 유지되고 자연 소진 때 모든 종속이 반납된다.
TEST_CASE(DissolveVisualSystem_ActualKernelSourceDeletionAndNaturalExpiry) {
    for (const auto orientation:{0,1}) { //두 폴의 가로/세로 식을 같은 실제 Host에서 실행한다.
        DissolveScene scene;auto* process=scene.effect->Add(scene.source,scene.first,scene.second,orientation);CHECK(process);
        const auto form=process->Form();const auto token=process->Fields().common.registration.token;const auto dummy=scene.dummy;
        CHECK(scene.pool.Slot(form)[10]==18 && process->Parent()==dummy && scene.depends.Has(form,dummy));CHECK(scene.effect->Size()==1 && scene.kernel.Size()==1);
        CHECK(scene.dispatch.Destroy(scene.source,16));CHECK(scene.effect->Size()==1 && scene.kernel.Size()==1 && scene.rendering.Lookup(token));
        scene.kernel.RunFrame();scene.rendering.Draw();CHECK(process->Fields().clockStarted && process->Fields().common.delta==0);CHECK(scene.pixels.size()==12 && scene.dirty.size()==2);
        scene.frame.now=10;scene.kernel.RunFrame();scene.frame.now=11;scene.kernel.RunFrame();
        CHECK(scene.effect->Size()==0 && scene.kernel.Size()==0 && !scene.rendering.Lookup(token));CHECK((scene.pool.Slot(form)[11]&3)==3 && (scene.pool.Slot(dummy)[11]&3)==3);
        CHECK((scene.releases==std::vector<ShrapnelRelease>{ShrapnelRelease::Source,ShrapnelRelease::Particles,ShrapnelRelease::Object}));CHECK(scene.destroy.PreDepth()==0 && scene.destroy.PostDepth()==0);
    }
}
// 투명 이미지의 new0 반납·확보 실패·생성 중 캡처 예외도 실제 Form/목록을 남기지 않는다.
TEST_CASE(DissolveVisualSystem_TransparentAllocationAndCreationFailure) {
    DissolveScene transparent;transparent.transparent=true;transparent.effect->Add(transparent.source,transparent.first,transparent.second,1);transparent.kernel.RunFrame();
    CHECK(transparent.effect->Size()==0 && transparent.kernel.Size()==0 && transparent.pixels.empty());CHECK(transparent.releases.size()==3);
    DissolveScene failure;failure.failObject=true;CHECK(!failure.effect->Add(failure.source,failure.first,failure.second,0));CHECK(failure.rendering.Size()==0 && failure.kernel.Size()==0 && !failure.dummy.value);
    failure.failObject=false;failure.failCapture=true;CHECK(Throws([&] { failure.effect->Add(failure.source,failure.first,failure.second,0); }));CHECK(failure.rendering.Size()==0 && failure.kernel.Size()==0 && (failure.pool.Slot(failure.dummy)[11]&3)==3);CHECK(failure.destroy.PreDepth()==0 && failure.destroy.PostDepth()==0);
}
