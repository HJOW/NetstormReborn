// 실제 Host/Kernel·공통 목록의 냉각 연기 생성/출력·자기 삭제/생성 실패 정리를 검증한다.
#include "ConstructionSupport.h"
#include "o/SmokeVisualSystem.h"

using namespace netstorm::test::construction;
namespace { // 장치 출력만 관찰하고 실제 Form/Kernel/등록 수명을 사용하는 조립 장면이다.
// 부모129의 원문 HP/group과 그 외 기본 프로세스 자료다.
std::vector<RiftTypeRecord> SmokeVisualTypes() { std::vector<RiftTypeRecord> result(188);result[129].flags1=0x10;result[129].group=1;result[129].maxHitPoints=600;result[129].footX=result[129].footY=3;return result; }
// 단일 실제 풀/Host와 같은 공통 renderer를 가진 연기 조립 장면이다.
struct SmokeVisualScene {
    SidPool pool{OriginalEdition::Patch1078,32768,true}; // 같은 부모/프로세스 저장소다.
    SquidHash hash; // 실제 공통 공간 해제가 읽는 해시다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); // 같은 공간 지도다.
    std::vector<RiftTypeRecord> types{SmokeVisualTypes()}; // 부모/내부 프로세스의 실제 분류 입력이다.
    SquidUnpop unpop{pool,hash,spots}; // 실제 Form의 공간 해제다.
    RawSquidDestroy destroy{pool,unpop,types}; // 실제 공통 삭제 깊이다.
    Kernel kernel; // 실제 등록 순서의 프레임/자기 삭제를 실행한다.
    SquidProcessState clock; // 같은 Host의 현재 시계/권한이다.
    SquidProcessHost host; // 실제 client Form 부착을 수행한다.
    GameRandom random; // 생성 재진입과 프레임이 사용하는 같은 난수다.
    ConstructionVisualState visual; // 공통 수집/표시 배율이다.
    FieryTailFrameState frame; // 현재 원본 시각/카메라다.
    SmokeLifetimeState lifetime; // 이 세계의 첫/재사용 색 객체다.
    std::array<std::uint32_t,13> colors{}; // 마지막 한 값은 이 검사의 명시 인접 메모리다.
    FieryTailFramePalette palette{12,colors}; // 색수와 읽을 수 있는 길이를 구별한다.
    std::vector<FieryTailDirtyRect> dirty; // 원문 Frame/삭제의 실제 요청이다.
    std::vector<FieryTailLine> lines; // 원문 Draw의2픽셀 요청이다.
    std::vector<SmokeRelease> released; // 실제 입자→객체 반납 순서다.
    std::size_t palettes{},probes{}; // 첫 색 생성과 생성 중 수집 관찰 횟수다.
    bool probe{},fail{}; // 생성 재진입/실제 입자 확보 실패의 명시 입력이다.
    Sid parent; // 연기를 부착할 실제 서버 부모다.
    std::unique_ptr<RenderProcessSystem> rendering; // 효과보다 오래 사는 원본 공통 목록이다.
    std::unique_ptr<SmokeVisualSystem> smoke; // 실제21 생성/Frame/수명 관리자다.
    // 부모를 만든 뒤 같은 실제 Host/등록 시스템을 조립한다.
    SmokeVisualScene():host(pool,types,kernel,destroy,clock,{},AssetHooks()) {
        parent=pool.Allocate(0);auto raw=pool.AllocatedBytes(parent);Put(raw,0,0x501dd0);raw[10]=129;raw[11]=0;raw[34]=1;raw[35]=7;Put(raw,14,Bits(20.5F));Put(raw,18,Bits(21.25F));Put(raw,26,600);frame.now=clock.now=2;
        RenderRegistryHooks hooks;hooks.reserve=[](std::uint32_t) { return true; };hooks.releaseArray=[] {};rendering=std::make_unique<RenderProcessSystem>(pool,host,visual,std::move(hooks));smoke=std::make_unique<SmokeVisualSystem>(*rendering,random,lifetime,frame,palette,Hooks());
    }
    // 합성 부모의 공통 전후처리만 완료하며 Form의 실제 삭제/부착은 그대로 실행한다.
    SquidDestroyHooks AssetHooks() {
        SquidDestroyHooks hooks;hooks.selected=[] { return Sid{}; };hooks.emit=[this](const SquidDestroyEvent& event) { if (event.effect==SquidDestroyEffect::PreDestroy) destroy.CompletePreDestroy();if (event.effect==SquidDestroyEffect::PostDestroy) destroy.CompletePostDestroy(); };return hooks;
    }
    // 생성/프레임은 실제 몸체이며 출력과 실제 vector 확보의 실패만 경계로 둔다.
    SmokeSystemHooks Hooks() {
        SmokeSystemHooks hooks;hooks.lifetime.reserve=[this](std::uint32_t bytes) {
            CHECK(bytes==4800 || bytes==8);
            if (probe && bytes==4800) { ++probes;const auto before=random.State();kernel.RunFrame();rendering->Draw();std::array<RenderCollectionEntry,2> work{};CHECK(random.State()==before && rendering->Collect({0,0,256,256},work,2)==0 && lines.empty());CHECK(Throws([&] { smoke->AddCooling(parent,{0,0}); })); }
            return !(fail && bytes==4800);
        };
        hooks.lifetime.createPalette=[this] { ++palettes;return 7U; };hooks.lifetime.buildPalette=[this](std::uint32_t token) { CHECK(token==7);const auto native=BuildSmokePalette();std::copy(native.begin(),native.end(),colors.begin());colors.back()=0xcafef00d; };
        hooks.frame.invalidate=[this](FieryTailDirtyRect rect) { dirty.push_back(rect); };hooks.frame.line=[this](FieryTailLine line) { lines.push_back(line); };hooks.release=[this](SmokeRelease release) { released.push_back(release); };return hooks;
    }
};
}
TEST_CASE(SmokeVisualSystem_ActualHostFramesDrawAndSelfDeletion) { // 같은 실제 객체의 수명과 원문10회/자기 종료를 확인한다.
    SmokeVisualScene scene;auto* process=scene.smoke->AddCooling(scene.parent,{16,-11});CHECK(process!=nullptr);if (!process) return;const auto form=process->Form();const auto token=process->Fields().registration.token;
    CHECK(scene.kernel.Size()==1 && scene.rendering->Size()==1 && scene.host.FromForm(form)==process && scene.pool.Slot(form)[10]==21);CHECK(process->Fields().capacity==200 && process->Fields().total==0 && process->Fields().remaining==10 && process->Fields().period==3 && process->Fields().registration.zOrder==6);CHECK(scene.palettes==1);
    scene.kernel.RunFrame();CHECK(process->Fields().first && process->Fields().total==0);
    // 원본은 한 호출마다 유한 방출 수를 하나씩 소비한다.
    for (int i=0;i<10;++i) { scene.frame.now+=.025;scene.clock.now=scene.frame.now;scene.kernel.RunFrame(); }
    CHECK(process->Fields().remaining==0 && process->Fields().total>0);scene.rendering->Draw();CHECK(!scene.lines.empty() && !scene.dirty.empty());
    // 원문은 이번에 수명이 끝난 입자도 이동/dirty 후 다음 프레임에서 자기 삭제한다.
    scene.frame.now+=100;scene.clock.now=scene.frame.now;scene.kernel.RunFrame();CHECK(scene.smoke->Size()==1);scene.kernel.RunFrame();CHECK(scene.smoke->Size()==0 && scene.kernel.Size()==0 && scene.rendering->Size()==0 && scene.host.FromForm(form)==nullptr && scene.rendering->Lookup(token)==nullptr);CHECK(scene.released==std::vector<SmokeRelease>({SmokeRelease::Particles,SmokeRelease::Object}));CHECK(scene.destroy.PreDepth()==0 && scene.destroy.PostDepth()==0);
}
TEST_CASE(SmokeVisualSystem_CreationVisibilityFailureAndPaletteReuse) { // 생성 중 출력/중첩을 차단하고 실패 뒤 같은 슬롯을 재사용한다.
    SmokeVisualScene scene;scene.probe=true;scene.fail=true;CHECK(Throws([&] { scene.smoke->AddCooling(scene.parent,{16,-11}); }));CHECK(scene.probes==1 && scene.smoke->Size()==0 && scene.kernel.Size()==0 && scene.rendering->Size()==0 && scene.palettes==0);CHECK(scene.released==std::vector<SmokeRelease>({SmokeRelease::Object}));
    scene.probe=false;scene.fail=false;auto* first=scene.smoke->AddCooling(scene.parent,{16,-11});auto* second=scene.smoke->AddCooling(scene.parent,{16,-11});CHECK(first && second);CHECK(scene.palettes==1 && scene.smoke->Size()==2 && scene.rendering->Size()==2 && scene.kernel.Size()==2);scene.host.Kill(*first,0x10);scene.host.Kill(*second,0x10);CHECK(scene.smoke->Size()==0 && scene.kernel.Size()==0 && scene.rendering->Size()==0);
}
