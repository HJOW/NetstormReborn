// 실제 Host/Kernel과 공통 표시 목록에서 빔의 생성·소리·소진/부모 삭제를 검사한다.
#include "ConstructionSupport.h"
#include "o/LightningVisualSystem.h"
#include "o/FieryTailVisualSystem.h"

using namespace netstorm::test::construction;
namespace {
// 이 통합 검사의 명시 HP 보유 부모 타입이다. 실제 TYPE/SHP 원문 대조와 구별한다.
std::vector<RiftTypeRecord> Types() {
    std::vector<RiftTypeRecord> types(188);types[100].flags1=0x10;types[100].maxHitPoints=100;types[100].footX=types[100].footY=1;return types;
}
// 같은 raw 풀/공통 삭제/Kernel/Host에서 다른 표시와 빔을 함께 사용한다.
struct LightningScene {
    SidPool pool{OriginalEdition::Patch1078,32768,true}; // 부모/표적/Form의 단일 슬롯 풀이다.
    SquidHash hash; // 실제 공통 공간 해제에 사용하는 같은 해시다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); // 원본 spot 지도다.
    std::vector<RiftTypeRecord> types{Types()}; // 이 검사의 명시 부모 자료다.
    SquidUnpop unpop{pool,hash,spots}; // 실제 부모 공간 해제 몸체다.
    RawSquidDestroy destroy{pool,unpop,types}; // Form/부모 공통 삭제 몸체다.
    Kernel kernel; // 빔과 다른 효과를 등록 순서로 진행하는 실제 Kernel이다.
    SquidProcessState clock; // Host의 현재 권한/시각 전역이다.
    SquidProcessHost host; // client Form 생성/부모 연결/삭제의 실제 호스트다.
    GameRandom random{0x12345678}; // 두 표시와 게임이 공유하는 난수 상태다.
    ConstructionVisualState visual; // 공통 표시 수집의16/11 배율이다.
    LightningFrameState frame; // 두 표시가 공유하는 현재 시간/카메라다.
    LightningLifetimeState beamLifetime{88}; // 이미 존재하는 빔 공유 색 객체다.
    FieryTailLifetimeState fieryLifetime{77}; // 다른 효과와 구별한 공유 색 객체다.
    std::array<std::uint32_t,12> colors{1,2,3,4,5,6,7,8,9,10,11,12}; // Fiery Draw의 인접 DWORD도 명시 입력이다.
    FieryTailFramePalette fieryPalette{11,colors}; // 다른 효과의 읽을 수 있는 색 메모리다.
    std::vector<LightningDirtyRect> dirty; // 실제 일반 프레임/삭제 통지의 요청이다.
    std::vector<LightningLine> lines; // 빔 Draw가 출력한 실제 선이다.
    std::vector<std::string> sounds; // 최초 파일명 소리를 관찰한다.
    std::vector<LightningRelease> releases; // 실제 경로/입자/객체 반납 순서다.
    bool probe{}; // 생성 중 재진입을 공급할지 정한다.
    std::uint32_t failBytes{}; // 명시 확보 실패 요청 크기다.0이면 실패하지 않는다.
    std::size_t probes{}; // 생성 중 시도한 프레임/Draw/수집 수다.
    Sid parent,target; // 같은 풀에 있는 실제 부모/표적이다.
    std::unique_ptr<RenderProcessSystem> rendering; // 효과보다 오래 살아 있는 공통 저장소다.
    std::unique_ptr<FieryTailVisualSystem> fiery; // 같은 목록에 함께 넣는 실제 process11이다.
    std::unique_ptr<LightningVisualSystem> beam; // 실제 process19 소유 관리자다.

    // 부모/표적과 실제 공통 Host를 준비한 뒤 같은 표시 목록을 두 관리자에게 공급한다.
    LightningScene():host(pool,types,kernel,destroy,clock,{},AssetHooks()) {
        parent=Asset(20.5F,21.25F);target=Asset(40.125F,35.375F);frame.now=2;
        RenderRegistryHooks registry;registry.reserve=[](std::uint32_t) { return true; };registry.releaseArray=[] {};
        rendering=std::make_unique<RenderProcessSystem>(pool,host,visual,std::move(registry));
        fiery=std::make_unique<FieryTailVisualSystem>(*rendering,random,fieryLifetime,frame,fieryPalette,FieryHooks());
        beam=std::make_unique<LightningVisualSystem>(*rendering,random,beamLifetime,frame,BeamHooks());
    }
    // 실제 raw 부모/표적의 공간/피해 자료만 구성한다.
    Sid Asset(float x,float y) {
        const auto sid=pool.Allocate(0);auto raw=pool.AllocatedBytes(sid);Put(raw,0,0x501dd0);raw[10]=100;raw[11]=0;raw[34]=1;raw[35]=255;raw[40]=1;Put(raw,14,Bits(x));Put(raw,18,Bits(y));Put(raw,26,50);return sid;
    }
    // 합성 부모의 자산 경계만 명시 완료하고 실제 Form 삭제는 Host가 처리한다.
    SquidDestroyHooks AssetHooks() {
        SquidDestroyHooks hooks;hooks.selected=[] { return Sid{}; };hooks.emit=[this](const SquidDestroyEvent& event) {
            if (event.effect==SquidDestroyEffect::PreDestroy) destroy.CompletePreDestroy();if (event.effect==SquidDestroyEffect::PostDestroy) destroy.CompletePostDestroy();
        };return hooks;
    }
    // 원본 Fiery 몸체도 같은 저장소에 등록하며 장치 요청만 관찰한다.
    FieryTailSystemHooks FieryHooks() {
        FieryTailSystemHooks hooks;hooks.lifetime.reserve=[](std::uint32_t bytes) { CHECK(bytes==2880);return true; };
        hooks.frame.invalidate=[this](FieryTailDirtyRect rect) { dirty.push_back(rect); };hooks.frame.line=[](FieryTailLine) {};
        hooks.endpoint=[](Sid) { return std::array<std::int32_t,2>{326,230}; };hooks.release=[](FieryTailRelease) {};return hooks;
    }
    // 실제 프레임/소진 삭제와 원본 확보량만 사용하며 빈 빔으로 대체하지 않는다.
    LightningSystemHooks BeamHooks() {
        LightningSystemHooks hooks;hooks.lifetime.reserve=[this](std::uint32_t bytes) {
            CHECK(bytes==2880 || bytes==64);
            if (probe) {
                ++probes;const auto before=random.State();kernel.RunFrame();rendering->Draw();std::array<RenderCollectionEntry,2> work{};
                CHECK(before==random.State() && rendering->Collect({0,0,256,256},work,2)==0 && lines.empty());CHECK(Throws([&] { beam->Add(parent,target,std::nullopt); }));
            }
            return bytes!=failBytes;
        };
        hooks.frame.invalidate=[this](LightningDirtyRect rect) { dirty.push_back(rect); };hooks.frame.line=[this](LightningLine line) { lines.push_back(line); };
        hooks.frame.sound=[this](float x,float y,const std::string& name) { CHECK(x==20.5F && y==21.25F);sounds.push_back(name); };
        hooks.center=[this](Sid sid) { const auto raw=pool.Slot(sid);return std::array<float,2>{static_cast<float>(static_cast<double>(Float(Get(raw,14)))-.5),static_cast<float>(static_cast<double>(Float(Get(raw,18)))-.5)}; };
        hooks.release=[this](LightningRelease released) { releases.push_back(released); };return hooks;
    }
};
}
// Seed/파일명 소리·실제 선 출력과 자기 소진 삭제가 같은 Kernel/공통 등록에 연결된다.
TEST_CASE(LightningVisualSystem_NaturalLifetimeAndSound) {
    LightningScene scene;auto* beam=scene.beam->Add(scene.parent,scene.target,"shortZap.wav");const auto form=beam->Form();const auto token=beam->Fields().registration.token;
    CHECK(scene.kernel.Size()==1 && scene.rendering->Size()==1 && scene.pool.Slot(form)[10]==19 && scene.host.FromForm(form)==beam);
    CHECK(beam->Fields().particles.values.size()==120 && beam->Fields().particles.total==0 && beam->Fields().pathAllocated && beam->Fields().palette==88 && beam->Fields().registration.zOrder==-25);
    scene.kernel.RunFrame();CHECK(beam->Fields().first && beam->Fields().particles.total==1 && scene.sounds==std::vector<std::string>{"shortZap.wav"});scene.rendering->Draw();CHECK(scene.lines.empty());
    scene.frame.now=2.025;scene.kernel.RunFrame();scene.rendering->Draw();CHECK(!scene.lines.empty());
    // 모든 빔 선은 원본 리터럴 중심/보조 색을 사용한다.
    for (const auto& line:scene.lines) CHECK(line.color==195 || line.color==85);
    scene.frame.now=2.6;scene.kernel.RunFrame();CHECK(scene.kernel.Size()==0 && scene.rendering->Size()==0 && scene.beam->Size()==0 && (scene.pool.Slot(form)[11]&1));
    CHECK(scene.sounds.size()==1 && scene.beamLifetime.palette==88 && !scene.dirty.empty());
    CHECK(scene.releases==std::vector<LightningRelease>({LightningRelease::Path,LightningRelease::Particles,LightningRelease::Object}));scene.lines.clear();scene.rendering->DrawToken(token);CHECK(scene.lines.empty());
}
// 120입자/경로 확보 실패와 생성 중 재진입이 Form/등록/Kernel 잔재를 남기지 않는다.
TEST_CASE(LightningVisualSystem_FailureAndCreationReentry) {
    LightningScene scene;scene.probe=true;scene.failBytes=2880;CHECK(Throws([&] { scene.beam->Add(scene.parent,scene.target,std::nullopt); }));
    CHECK(scene.probes==1 && scene.kernel.Size()==0 && scene.rendering->Size()==0 && scene.beam->Size()==0);
    scene.failBytes=64;scene.releases.clear();CHECK(Throws([&] { scene.beam->Add(scene.parent,scene.target,std::nullopt); }));
    CHECK(scene.probes==3 && scene.kernel.Size()==0 && scene.rendering->Size()==0 && scene.beam->Size()==0);
    CHECK(scene.releases==std::vector<LightningRelease>({LightningRelease::Particles,LightningRelease::Object}));
    scene.failBytes=0;auto* beam=scene.beam->Add(scene.parent,scene.target,std::nullopt);CHECK(scene.probes==5 && beam->Fields().registration.slot==0);scene.probe=false;
    scene.kernel.RunFrame();CHECK(scene.sounds.empty());scene.host.Kill(*beam,0x10);CHECK(scene.kernel.Size()==0 && scene.rendering->Size()==0);
}
// 다른 process11과 같은 목록/토큰/수집을 공유하고 부모 종속 삭제가 두 표시를 실제 정리한다.
TEST_CASE(LightningVisualSystem_SharedRegistryAndParentDeletion) {
    LightningScene scene;auto* tail=scene.fiery->Add(scene.parent);auto* beam=scene.beam->Add(scene.parent,scene.target,std::nullopt);
    CHECK(tail->Fields().registration.token==1 && beam->Fields().registration.token==2 && scene.kernel.Size()==2 && scene.rendering->Size()==2);scene.kernel.RunFrame();
    std::array<RenderCollectionEntry,2> entries{};CHECK(scene.rendering->Collect({0,0,256,256},entries,2)==2);
    const auto form=beam->Form();scene.dirty.clear();scene.pool.AllocatedBytes(scene.parent)[11]|=4;scene.destroy.Destroy(scene.parent,0,scene.host.Hooks());
    CHECK(scene.kernel.Size()==0 && scene.rendering->Size()==0 && scene.fiery->Size()==0 && scene.beam->Size()==0 && (scene.pool.Slot(form)[11]&1));CHECK(scene.dirty.size()==2);
    CHECK(scene.beamLifetime.palette==88 && scene.fieryLifetime.palette==77);
}
