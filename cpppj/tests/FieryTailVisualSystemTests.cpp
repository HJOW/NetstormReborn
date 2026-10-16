// 실제 Host/Kernel에 건설과 Fiery Tail을 함께 붙여 공통 목록/수명/출력 요청을 검사한다.
#include "ConstructionSupport.h"
#include "o/ConstructionVisualSystem.h"
#include "o/FieryTailVisualSystem.h"
#include "client/ConstructionVisualRenderer.h"

using namespace netstorm::test::construction;
namespace {
// 인접 DWORD는 실제 게임 heap의 관찰값이 아닌 이 통합 검사의 명시 입력이다.
constexpr std::uint32_t kExplicitAdjacent=0xabcdef01;
// 원본 몸체에 공급할 합성 HP 보유 부모 타입이다. 실제 미션/SHP 대조를 대신하지 않는다.
std::vector<RiftTypeRecord> VisualTypes() {
    std::vector<RiftTypeRecord> result(188);result[100].flags1=0x10;result[100].maxHitPoints=100;
    result[100].footX=result[100].footY=1;return result;
}
// 독립 fixture의 바이트만 읽으며 기대 영역을 재계산하지 않는다.
std::vector<std::uint8_t> Payload(std::string_view text) {
    if (text.size()%2) throw std::invalid_argument("표시 삭제 fixture hex 폭 오류");std::vector<std::uint8_t> bytes;
    // 두16진 문자마다 입력 바이트 하나를 복원한다.
    for (std::size_t index=0;index<text.size();index+=2) bytes.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(index,2)),nullptr,16)));
    return bytes;
}
// 같은 실제 raw 풀/공통 삭제/Kernel/Host를 사용하며 장치 요청만 관찰한다.
struct VisualScene {
    SidPool pool{OriginalEdition::Patch1078,32768,true}; // 두 효과와 부모/Form이 함께 사용하는 슬롯 풀이다.
    SquidHash hash; // 실제 공통 삭제가 참조하는 공간 해시다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); // 같은 원본 spot 지도다.
    std::vector<RiftTypeRecord> types{VisualTypes()}; // 명시 합성 부모/프로세스 자료다.
    SquidUnpop unpop{pool,hash,spots}; // 부모의 공통 공간 해제 몸체다.
    RawSquidDestroy destroy{pool,unpop,types}; // Form/부모의 실제 삭제 순서를 처리한다.
    Kernel kernel; // 두 효과가 등록 순서대로 실행되는 실제 Kernel이다.
    SquidProcessState clock; // 공통 Host의 현재 시각/권한이다.
    SquidProcessHost host; // 두 효과에 client Form을 부착하는 실제 호스트다.
    GameRandom random; // 건설/Fiery Tail이 공유하는 게임 난수다.
    ConstructionVisualState visual; // 공통16/11 배율과 건설의 이미 있는 색 표다.
    ConstructionFrameState constructionClock; // 같은 명시 시각을 공급하는 건설 프레임 전역이다.
    FieryTailFrameState fieryClock; // Fiery Tail 프레임/카메라 전역이다.
    std::array<std::uint32_t,12> colors{0,1,2,3,4,5,6,7,8,9,10,kExplicitAdjacent}; // 색수11 뒤 DWORD까지 명시 제공한다.
    ConstructionFramePalette constructionPalette{71,11,colors}; // 기존 건설 표시의 색 입력이다.
    FieryTailLifetimeState fieryLifetime{77}; // 이미 존재하는 공유 표 토큰이다.
    FieryTailFramePalette fieryPalette{11,colors}; // 실제 배열 길이와 색수는 구별한다.
    std::vector<FieryTailDirtyRect> dirty; // 실제 프레임/삭제 통지가 요청한 영역이다.
    std::vector<FieryTailLine> lines; // 실제 Draw의 전체 선 요청이다.
    std::vector<FieryTailRelease> releases; // 입자와 객체의 반납 순서다.
    std::vector<std::uint32_t> allocations; // 공통 표시 목록의 실제32비트 확보 바이트 수다.
    std::vector<char> draws; // 종류를 분리하지 않은 실제 출력 순서를 관찰한다.
    bool failParticles{},probeCreation{}; // 명시 확보 실패와 생성 재진입 입력이다.
    std::size_t probes{}; // 생성 중 프레임/Draw/수집을 호출한 횟수다.
    Sid parent; // 두 종류 효과를 함께 부착할 실제 부모다.
    std::unique_ptr<RenderProcessSystem> rendering; // 각 효과보다 오래 사는 공통 표시 저장소다.
    std::unique_ptr<ConstructionVisualSystem> construction; // 같은 공통 저장소를 사용하는 건설 표시다.
    std::unique_ptr<FieryTailVisualSystem> fiery; // Kernel의 process11 수명 관리자다.

    // 부모/공통 목록을 준비한 뒤 두 효과의 실제 Host 어댑터를 만든다.
    VisualScene():host(pool,types,kernel,destroy,clock,{},AssetHooks()) {
        parent=pool.Allocate(0);auto raw=pool.AllocatedBytes(parent);Put(raw,0,0x501dd0);raw[10]=100;raw[11]=0;
        raw[34]=1;raw[35]=255;raw[40]=1;Put(raw,14,Bits(20.5F));Put(raw,18,Bits(21.25F));Put(raw,26,50);
        visual.palette=71;constructionClock.now=fieryClock.now=2.0;
        RenderRegistryHooks registry;registry.reserve=[this](std::uint32_t bytes) { allocations.push_back(bytes);return true; };registry.releaseArray=[] {};
        rendering=std::make_unique<RenderProcessSystem>(pool,host,visual,std::move(registry));
        construction=std::make_unique<ConstructionVisualSystem>(*rendering,constructionClock,constructionPalette,random,ConstructionHooks(),ConstructionFrameHooks(),[](ConstructionRelease) {});
        fiery=std::make_unique<FieryTailVisualSystem>(*rendering,random,fieryLifetime,fieryClock,fieryPalette,FieryHooks());
    }
    // 공간/화면의 합성 부모 경계만 완료하고 Form 삭제는 실제 Host가 처리한다.
    SquidDestroyHooks AssetHooks() {
        SquidDestroyHooks result;result.selected=[] { return Sid{}; };
        result.emit=[this](const SquidDestroyEvent& event) {
            if (event.effect==SquidDestroyEffect::PreDestroy) destroy.CompletePreDestroy();
            if (event.effect==SquidDestroyEffect::PostDestroy) destroy.CompletePostDestroy();
        };return result;
    }
    // 실제 생성/Seed 몸체에 작은 명시 캡처를 공급한다. 공유 색 표71은 이미 있으므로 생성 경계는 사용하지 않는다.
    ConstructionVisualHooks ConstructionHooks() {
        ConstructionVisualHooks result;result.reserve=[](std::uint32_t) { return true; };result.createPalette=[] { return 71U; };
        result.gradient=[](std::uint32_t,std::array<double,3>,std::array<double,3>,double) {};result.buildPalette=[](std::uint32_t) {};
        result.releaseParticles=[] {};result.capture=[](Sid) { return ConstructionVisualImage{5,7,2,3,std::vector<std::uint8_t>(35,17)}; };return result;
    }
    // HP/장치만 명시 입력/관찰이며 건설 프레임·삭제 몸체는 실제 모듈이다.
    netstorm::o::ConstructionFrameHooks ConstructionFrameHooks() {
        netstorm::o::ConstructionFrameHooks result;result.maxHitPoints=[](Sid) { return 100; };
        result.invalidate=[](ConstructionFrameRect) {};result.draw=[this](const ConstructionFrameDraw&) { draws.push_back('C'); };return result;
    }
    // 새 효과를 Frame/Draw 없이 등록하는 빈 성공 경계는 사용하지 않는다.
    FieryTailSystemHooks FieryHooks() {
        FieryTailSystemHooks result;
        result.lifetime.reserve=[this](std::uint32_t bytes) {
            CHECK(bytes==2880);
            if (probeCreation) {
                ++probes;const auto before=random.State();kernel.RunFrame();rendering->Draw();std::array<RenderCollectionEntry,2> work{};
                CHECK(random.State()==before && rendering->Collect({0,0,256,256},work,2)==0 && lines.empty());
                CHECK(Throws([&] { fiery->Add(parent); }));
            }
            return !failParticles;
        };
        result.frame.invalidate=[this](FieryTailDirtyRect rect) { dirty.push_back(rect); };
        result.frame.line=[this](FieryTailLine line) { lines.push_back(line);draws.push_back('F'); };
        result.endpoint=[this](Sid sid) { CHECK(sid==parent);return std::array<std::int32_t,2>{326,230}; };
        result.release=[this](FieryTailRelease released) { releases.push_back(released); };return result;
    }
};
}

// 두 효과가 하나의 번호/등록 배열·실제 Kernel·공통 수집/깊이 정렬·슬롯 재사용을 공유한다.
TEST_CASE(FieryTailVisualSystem_SharedConstructionRegistryAndKernel) {
    VisualScene scene;auto* first=scene.construction->Add(scene.parent,false);auto* tail=scene.fiery->Add(scene.parent);auto* second=scene.construction->Add(scene.parent,true);
    const auto firstToken=first->Registration().token,tailToken=tail->Fields().registration.token;
    CHECK(firstToken==1 && tailToken==2 && second->Registration().token==3);
    CHECK(scene.rendering->Size()==3 && scene.kernel.Size()==3 && scene.construction->Size()==2 && scene.fiery->Size()==1);
    CHECK(scene.host.FromForm(tail->Form())==tail && scene.pool.Slot(tail->Form())[10]==11 && tail->Fields().registration.form==tail->Form().value);
    CHECK(tail->Fields().particles.size()==120 && tail->Fields().total==0 && !tail->Fields().first && tail->Fields().registration.zOrder==-25);
    scene.kernel.RunFrame();CHECK(tail->Fields().first && tail->Fields().total==10);
    std::array<RenderCollectionEntry,3> work{};const RenderWorldRect view{0,0,256,256};
    const auto result=netstorm::client::DrawVisibleScene(*scene.rendering,{{},view,view,0,0},work,{}, {},{});
    CHECK(result.count==3 && work[2].token==tailToken && work[2].kind==3 && work[2].depth==-25);
    CHECK(scene.lines.size()==10 && scene.draws.size()==12 && scene.draws[0]=='C' && scene.draws[1]=='C');
    // 최초 최대 수명이 읽는 인접 DWORD도 테스트의 명시 입력과 같아야 한다.
    for (const auto& line:scene.lines) CHECK(line.color==kExplicitAdjacent && line.x2==326 && line.y2==230);
    const auto oldForm=tail->Form();scene.host.Kill(*first,0x10);auto* replacement=scene.fiery->Add(scene.parent);
    CHECK(replacement->Fields().registration.slot==0 && replacement->Fields().registration.token!=firstToken && replacement->Fields().registration.token!=tailToken);
    scene.constructionClock.now=scene.fieryClock.now=2.1;scene.kernel.RunFrame();CHECK(tail->Fields().total>10 && replacement->Fields().total==10);
    scene.fiery.reset();CHECK(scene.kernel.Size()==1 && scene.rendering->Size()==1 && (scene.pool.Slot(oldForm)[11]&1));
    scene.construction.reset();CHECK(scene.kernel.Size()==0 && scene.rendering->Size()==0 && Get(scene.pool.Slot(scene.parent),6,2)==0);
    CHECK(scene.allocations==std::vector<std::uint32_t>{160} && scene.fieryLifetime.palette==77);
    CHECK(scene.releases==std::vector<FieryTailRelease>({FieryTailRelease::Particles,FieryTailRelease::Object,FieryTailRelease::Particles,FieryTailRelease::Object}));
}
// 실제 부착 뒤 확보 실패·생성 중 재진입·dead 부모 실패가 등록/Form/Kernel 잔재를 남기지 않는다.
TEST_CASE(FieryTailVisualSystem_FailureAndCreationReentry) {
    VisualScene scene;scene.failParticles=scene.probeCreation=true;CHECK(Throws([&] { scene.fiery->Add(scene.parent); }));
    CHECK(scene.probes==1 && scene.kernel.Size()==0 && scene.rendering->Size()==0 && scene.fiery->Size()==0 && scene.rendering->Slots()[0]==0);
    scene.failParticles=false;auto* alive=scene.fiery->Add(scene.parent);CHECK(scene.probes==2 && alive->Fields().registration.slot==0 && alive->Fields().total==0);
    scene.probeCreation=false;scene.host.Kill(*alive,0x10);scene.pool.AllocatedBytes(scene.parent)[11]|=2;
    CHECK(Throws([&] { scene.fiery->Add(scene.parent); }));CHECK(scene.kernel.Size()==0 && scene.rendering->Size()==0 && scene.fiery->Size()==0);
}
// 부모의 실제 종속 삭제가 두 종류의 등록과 Form을 모두 정리하고 Fiery Tail dirty 통지를 보낸다.
TEST_CASE(FieryTailVisualSystem_ParentDeletionInvalidatesAndUnregisters) {
    VisualScene scene;scene.construction->Add(scene.parent,false);auto* tail=scene.fiery->Add(scene.parent);const auto form=tail->Form();scene.kernel.RunFrame();
    scene.fieryClock.cameraX=.25F;scene.fieryClock.cameraY=-.5F;const auto expected=RenderProcessRemovalBounds(tail->Fields().registration,.25F,-.5F);
    scene.dirty.clear();scene.pool.AllocatedBytes(scene.parent)[11]|=4;scene.destroy.Destroy(scene.parent,0,scene.host.Hooks());
    CHECK(scene.kernel.Size()==0 && scene.rendering->Size()==0 && scene.construction->Size()==0 && scene.fiery->Size()==0 && (scene.pool.Slot(form)[11]&1));
    CHECK(scene.dirty.size()==1);if (!scene.dirty.empty()) CHECK((std::array<std::int32_t,4>{scene.dirty[0].left,scene.dirty[0].top,scene.dirty[0].right,scene.dirty[0].bottom}==expected));
}
// 공통 목록의 오래된 수집 토큰이 재사용 슬롯의 새 효과를 잘못 그리지 않으며 두 배율을 섞지 않는다.
TEST_CASE(FieryTailVisualSystem_StaleTokenAndSharedScale) {
    VisualScene scene;auto* tail=scene.fiery->Add(scene.parent);scene.kernel.RunFrame();const auto token=tail->Fields().registration.token;
    scene.host.Kill(*tail,0x10);scene.fiery->Add(scene.parent);scene.rendering->DrawToken(token);CHECK(scene.lines.empty());
    scene.fieryClock.pixelScaleX=17;CHECK(Throws([&] { scene.kernel.RunFrame(); }));scene.fieryClock.pixelScaleX=16;
}
// 실제00487960/CRT의48개 기본 정밀도 결과를 입력/출력 DWORD 전체로 대조한다.
TEST_CASE(RenderProcessSystem_RemovalBoundsNative027f) {
    std::ifstream input(NETSTORM_RENDER_REMOVAL_FIXTURE);if (!input) throw std::runtime_error("표시 삭제 영역 fixture 없음");
    std::string line;std::size_t count=0,extended=0;
    // 비교037f 행도 보존하며 기본53비트만 제품 대조 수에 가산한다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.front()=='#') continue;
        const auto row=Split(line,'\t');if (row.size()!=4) throw std::runtime_error("표시 삭제 영역 fixture 열 수 오류");
        if (std::stoul(row[1])==0x037f) { ++extended;continue; }CHECK(std::stoul(row[1])==0x027f);
        const auto payload=Payload(row[2]),expected=Payload(row[3]);CHECK(payload.size()==28 && expected.size()==20);
        RenderProcessFields fields;fields.left=Float(Get(payload,0));fields.top=Float(Get(payload,4));fields.right=Float(Get(payload,8));fields.bottom=Float(Get(payload,12));
        const auto actual=RenderProcessRemovalBounds(fields,Float(Get(payload,16)),Float(Get(payload,20)));
        // 원본 dirty의 flags0과 네 signed DWORD를 그대로 비교한다.
        for (std::size_t index=0;index<actual.size();++index) CHECK(static_cast<std::uint32_t>(actual[index])==Get(expected,index*4));
        CHECK(Get(expected,16)==0);++count;
    }
    CHECK(count==48 && extended==48);
}
