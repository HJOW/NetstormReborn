// 실제 두 PE 충돌 효과/anim Handle/Kernel/Regular 상태를 복원 소비자와 대조한다.
#include "RawSceneSupport.h"
#include "o/RawGunImpact.h"
#include "o/SquidDestroyLifecycle.h"
#include "o/SquidFrameBinding.h"
#include "client/SquidRenderer.h"
#include <cstdio>
#include <filesystem>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 기대값은 저장된 실제 원본 출력만 읽는다.
const std::vector<std::vector<std::string>>& ImpactRows() { static const auto rows=LoadFixture(NETSTORM_GUNIMPACT_FIXTURE).rows;return rows; }
// 실제 Kernel 개수 검사에만 소유권 슬롯을 제공하는 정지 프로세스이며 게임 진행으로 세지 않는다.
class ImpactCountProcess final : public BaseProcess {
public:
    // count 검사에서는 원본처럼 프레임을 실행하지 않고 등록 상태만 읽는다.
    void RunFrame() override {}
};
// 실제 표시 계산의 사각형/flags 요청만 수집하며 창이나 그래픽 장치를 만들지 않는다.
class ImpactDisplaySink final : public SquidDisplaySink {
public:
    // 실제 부분 변경 요청을 계산하도록 억제 전역을 false로 공급한다.
    bool Suppressed() const override { return false; }
    // UI 경계의 호출 사실과 실제 사각형을 보존한다.
    void Invalidate(SquidDisplayRect rect,std::uint32_t flags) override { rectangles.push_back(rect);updates.push_back(flags); }
    std::vector<SquidDisplayRect> rectangles; // 실제 SHP 프레임/좌표로 계산한 화면 영역이다.
    std::vector<std::uint32_t> updates; // 각 영역의 원본 표시 갱신 flags다.
};
// A..P 구간의 존재/빈 구간 및 비구간 padding을 원본 입력과 같은80프레임에 만든다.
std::vector<RiftTypeFrames> ImpactFrames(std::size_t count) {
    std::vector<RiftTypeFrames> result(count,RiftTypeFrames({}));std::vector<FrameCode> codes(80,FrameCode{255,0,0,0});
    // 각 실제 입력 글자는 고유 시작 위치와0..3개 길이를 갖는다.
    for (std::uint8_t event=0;event<16;++event) for (std::uint8_t frame=0;frame<event%4;++frame) codes[event*5+frame]=FrameCode{static_cast<std::uint8_t>('A'+event),0,frame,0};
    result[160]=RiftTypeFrames(std::move(codes));return result;
}
// 실제 Regular 생성/부착의 공개 상태를 원본의 관찰된 의미 필드와 대조한다.
std::string RegularState(OriginalEdition edition,std::uint32_t event,float payload) {
    SidPool pool(edition,kCapacity,false);std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);
    // Native attach 관찰 경계가 제공한 Form71/부모50을 같은 실제 풀에서 확보한다.
    for (std::uint16_t id=5;id<=70;++id) CHECK(pool.Allocate(2)==Sid{id});
    auto raw=pool.AllocatedBytes(kSource);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=160;
    SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);Kernel kernel(edition==OriginalEdition::Patch1078 ? kProcessCapacity : kCdProcessCapacity);SquidProcessState state;state.now=10;SquidProcessHost host(pool,types,kernel,destroy,state);const auto* regular=host.AddRegular(kSource,event,payload);CHECK(regular!=nullptr);if (!regular) throw std::logic_error("충돌 검사 실제 Regular 생성 실패");
    return std::to_string(regular->Form().value)+','+std::to_string(regular->Parent().value)+','+std::to_string(std::bit_cast<std::uint64_t>(regular->Time()))+','+std::to_string(regular->Event())+','+std::to_string(std::bit_cast<std::uint32_t>(regular->Payload()))+','+std::to_string(regular->Count());
}
// 독립 입력 raw/프레임 표와 원본의 동일한 공간/생성 관찰 경계를 소유한다.
struct ImpactScene {
    SidPool pool; // 실제 현재 판본의 raw slot과 예약 상태다.
    std::vector<RiftTypeRecord> types; // frame 지정기의 합성 원본 타입 입력이다.
    std::vector<RiftTypeFrames> frames; // 실제 A..P의 첫 frame/개수 입력이다.
    GameRandom random; // 실제 공유 난수 상태다.
    GunImpactState state; // 현재 억제 double/DWORD 입력이다.
    std::vector<std::string> events; // 원본 호출 시점의 인자/순서다.
    SquidFrame setter; // 실제 해시 단계/프레임 쓰기 몸체다.
    const std::vector<std::string>* row{}; // 현재 입력 행만 참조하며 기대 출력은 읽지 않는다.
    std::string process; // 실제 별도 Host가 반환한 Regular 의미 상태다.
    std::array<std::uint8_t,8> point{}; // 직접 Clamp 수신자의 raw 좌표다.
    bool patch; // 현재 frame DWORD/CD BYTE 및 한도를 선택한다.
    // 원본 SID50을 실제 할당해 쓰기 계약을 유지한다.
    explicit ImpactScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),frames(ImpactFrames(types.size())),setter(pool,types,FrameHooks()),patch(edition==OriginalEdition::Patch1078) {
        // 반복 입력에서도 할당 상태를 바꾸지 않고 현재 원본 slot을 재작성한다.
        for (std::uint16_t id=5;id<=50;++id) CHECK(pool.Allocate(2)==Sid{id});
    }
    // 현재 raw frame을 원본 폭으로 읽는다.
    std::uint32_t Frame() const { return Get(pool.Slot(kSource),patch ? 36U : 34U,patch ? 4U : 1U); }
    // 실제 지정기의 표시/공간 관찰 시점과 인자를 원본과 동일하게 기록한다.
    SquidFrameHooks FrameHooks() {
        SquidFrameHooks hooks;hooks.frameSize=[](Sid,std::int32_t) { return std::array<float,2>{1,1}; };hooks.update=[this](Sid,std::uint32_t) { events.push_back("D:50:"+std::to_string(Frame())); };hooks.unpop=[this](Sid,std::uint32_t flags) { events.push_back("U:50:"+std::to_string(flags)+':'+std::to_string(Frame())); };hooks.pop=[this](Sid,float x,float y,std::uint32_t flags) { Pop(x,y,flags); };return hooks;
    }
    // 현재 Pop 경계에서는 원본처럼 virtual 몸체를 대체하며 frame을 호출 시점에 읽는다.
    void Pop(float x,float y,std::uint32_t flags) { events.push_back("P:50:"+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y))+':'+std::to_string(flags)+':'+std::to_string(Frame())); }
    // 두 판본 실제 Kernel 한도와 배열 끝 입력을 그대로 공급한다.
    std::size_t ProcessCount() const { const std::array<std::size_t,5> counts{0,2,patch ? 30000U : 3000U,patch ? 30001U : 3001U,patch ? 39999U : 3999U};return counts.at(Number((*row)[12])); }
    // 현재 float 입력의 실제 비트를 읽는다.
    float F(std::size_t index) const { return std::bit_cast<float>(Number((*row)[index])); }
    // 매 행은 동일 원본 padding/raw/전역 입력을 만든다.
    void Prepare(const std::vector<std::string>& value) {
        CHECK(value.size()==19);row=&value;events.clear();process="-";random.SetState(Number(value[11]));state.suppressedTime=std::bit_cast<double>(std::stoull(value[9]));state.suppressed=Number(value[10]);Put(point,0,Number(value[2]));Put(point,4,Number(value[3]));auto raw=pool.AllocatedBytes(kSource);
        // 관찰되지 않은 padding은 원본과 동일한 패턴으로 남긴다.
        for (std::size_t i=0;i<raw.size();++i) raw[i]=static_cast<std::uint8_t>(i*7+3);
        Put(raw,0,patch ? 0x509758U : 0x503b30U);raw[10]=160;raw[11]=0;Put(raw,14,std::bit_cast<std::uint32_t>(20.75f));Put(raw,18,std::bit_cast<std::uint32_t>(21.9f));raw[patch ? 33U : 31U]=static_cast<std::uint8_t>(Number(value[8]));raw[patch ? 40U : 35U]=0;Put(raw,patch ? 36U : 34U,Number(value[7]),patch ? 4U : 1U);
    }
    // 원본과 같은 실제 생성 인자/Pop/Regular 의미 필드 및 엄격한 삭제/Dude 경계다.
    GunImpactHooks Hooks() {
        GunImpactHooks hooks;hooks.processCount=[this] { const auto count=ProcessCount();events.push_back("Q:"+std::to_string(count));return count; };hooks.create=[this](std::uint32_t type,std::uint32_t flags) { events.push_back("C:"+std::to_string(type)+':'+std::to_string(flags));return kSource; };hooks.pop=[this](Sid,float x,float y,std::uint32_t flags) { Pop(x,y,flags); };hooks.regular=[this](Sid sid,std::uint32_t event,float payload) { events.push_back("A:40");events.push_back("B:46:"+std::to_string(sid.value)+":0:80");process=RegularState(pool.Edition(),event,payload); };hooks.destroy=[this](Sid sid,std::uint32_t flags) { events.push_back("D:"+std::to_string(sid.value)+':'+std::to_string(flags)); };hooks.notifyDudes=[this](float x,float y) { events.push_back("Qd:"+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y))); };return hooks;
    }
    // 실제 생산 코드만 실행하고 원본 결과 칸을 기대값으로 넘기지 않는다.
    std::vector<std::string> Run() {
        RawGunImpact logic(pool,frames,setter,random,state,Hooks());std::string result="-";const auto kind=Number((*row)[1]);
        switch (kind) {
        case 0:logic.Visual(F(2),F(3),Number((*row)[4]),F(6));break;
        case 1:result=std::to_string(std::bit_cast<std::uint32_t>(logic.Handle(kSource,Number((*row)[4]),Number((*row)[5]),F(6))));break;
        case 2:{ const auto clamped=logic.Clamp(F(2),F(3));Put(point,0,std::bit_cast<std::uint32_t>(clamped[0]));Put(point,4,std::bit_cast<std::uint32_t>(clamped[1]));break; }
        case 3:{ types[160].constructorAddress=TypeConstructorAddress(pool.Edition(),160);SquidFactory factory(pool,types);factory.Construct(160,kSource);break; }
        case 4:{ Kernel kernel(patch ? kProcessCapacity : kCdProcessCapacity);
            // 실제 같은 개수의 객체를 등록한 Kernel.Size를 원본 배열 순회와 대조한다.
            for (std::size_t i=0;i<ProcessCount();++i) CHECK(kernel.Add(std::make_unique<ImpactCountProcess>())!=0);
            result=std::to_string(kernel.Size());break;
        }
        case 5:logic.Ground(F(2),F(3));break;
        default:throw std::logic_error("충돌 fixture 몸체 오류");
        }
        std::string actions;
        // 실제 단일 호출 순서를 원본과 같은 문자열로 묶는다.
        for (const auto& event:events) { if (!actions.empty()) actions+=';';actions+=event; }
        return {result,actions,Hex(pool.Slot(kSource)),process,std::to_string(random.State()),Hex(point)};
    }
};
// 현재 판본의 실제 PE 반환/프레임/raw/난수/Regular 상태를 전수 재생한다.
void ReplayImpact(std::string_view edition) {
    ImpactScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;int failures=0;
    // 원본 고유 입력을 각 한 번만 읽는다.
    for (const auto& row:ImpactRows()) {
        if (row[0]!=edition) continue;scene.Prepare(row);const auto output=scene.Run();const auto expected=std::vector<std::string>(row.begin()+13,row.end());const bool same=output==expected;CHECK(same);
        if (!same) {
            // 최초 차이의 원본 관찰값과 실제 C++ 값을 함께 남긴다.
            for (std::size_t i=0;i<output.size();++i) if (output[i]!=expected[i]) std::printf("%s Impact 행%zu/body%s 결과%zu 기대%s / 실제%s\n",row[0].c_str(),count,row[1].c_str(),i,expected[i].c_str(),output[i].c_str());
            if (++failures>=5) break;
        }
        ++count;
    }
    CHECK(failures==0 && count==ImpactRows().size()/2);
}
}
// 10.78의 실제 count40000·producer30001·프레임 DWORD·Regular 상태를 대조한다.
TEST_CASE(GunImpact_PatchNativeAnimationAndProducer) { ReplayImpact("originals"); }
// CD10.72의 count4000·producer3001·BYTE frame·다른 생성 몸체를 대조한다.
TEST_CASE(GunImpact_CdNativeAnimationAndProducer) { ReplayImpact("originalCD"); }
// 실제 하위 생성/프레임 종료/Dude 반응을 누락된 성공으로 바꾸지 않는다.
TEST_CASE(GunImpact_MissingCreationDeleteAndDudesRejected) {
    ImpactScene scene(OriginalEdition::Patch1078);scene.Prepare(ImpactRows().front());RawGunImpact missing(scene.pool,scene.frames,scene.setter,scene.random,scene.state,{});CHECK(Throws([&] { missing.Visual(50,50,3,0.02f); }));CHECK(Throws([&] { missing.Handle(kSource,0,0,0.02f); }));CHECK(Throws([&] { missing.Ground(50,50); }));CHECK(scene.events.empty() && scene.random.State()==0);
}
// 보호 원본의88프레임/실제 SHP와 같은 Factory/Pop/Regular/장부/종속 삭제 사슬을 두 판본에서 소비한다.
TEST_CASE(GunImpact_ActualAssetsFactoryPopRegularAndDeletion) {
    // 실제 자산 로더만 사용하며 게임/GUI/DirectDraw를 구동하지 않는다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/(edition==OriginalEdition::Patch1078 ? "originals" : "originalCD");BaseFileSystem files(root);files.RegisterArchive(root/"netstorm.tarc");netstorm::client::GameAssets game(files,edition);const auto types=game.TypeTable().Types();auto shapes=netstorm::client::SquidRenderer::Shapes(game,edition);std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));frames[160]=game.Find("anim").definition.FrameTable();const auto run=frames[160].Run('D');CHECK(frames[160].Codes().size()==88 && run.first==66 && run.count==22);
        SidPool pool(edition,kCapacity,true);SquidHash hash;std::vector<std::uint8_t> spots(65536);ImpactDisplaySink sink;SquidDisplay display(edition,types,shapes,sink,{0,0,65536,{0,0,640,480}});SquidPostPopState books;books.localOwner=1;SquidPostPop post(pool,types,books);SquidUnpop unpop(pool,hash,spots,&display);SquidPop pop(pool,hash,spots,&display,&post);SquidFrame setter(pool,types,MakeSquidFrameHooks(pool,types,display,unpop,pop));SquidFactory factory(pool,types,false,&unpop);RawSquidDestroy destroy(pool,unpop,types);SquidDeletionState deletionState;std::vector<SquidDeletionEvent> deletionEvents;SquidDeletionHooks deletionHooks;deletionHooks.emit=[&](const SquidDeletionEvent& event) { deletionEvents.push_back(event); };SquidDestroyLifecycle lifecycle(pool,types,books,deletionState,destroy,std::move(deletionHooks));Kernel kernel(edition==OriginalEdition::Patch1078 ? kProcessCapacity : kCdProcessCapacity);SquidProcessState clock;clock.now=10;SquidProcessHost host(pool,types,kernel,destroy,clock,{},lifecycle.Hooks());GunImpactHooks hooks;Sid created;
        hooks.pop=[&](Sid sid,float x,float y,std::uint32_t flags) { created=sid;const auto frame=static_cast<std::int32_t>(Get(pool.Slot(sid),edition==OriginalEdition::Patch1078 ? 36U : 34U,edition==OriginalEdition::Patch1078 ? 4U : 1U));const auto size=display.FrameSize(edition,pool.Slot(sid),frame);CHECK(pop.Pop(sid,types[pool.Slot(sid)[10]],size[0],size[1],x,y,flags)==RawPopResult::Registered); };
        hooks.destroy=[&](Sid sid,std::uint32_t flags) { destroy.Destroy(sid,flags,host.Hooks()); };
        GunImpactState state;GameRandom random;RawGunImpact impact(pool,frames,setter,random,state,MakeGunImpactRuntimeHooks(pool,factory,kernel,host,std::move(hooks)));host.SetRegularHandler(impact.Handler());impact.Visual(50.5f,50.5f,3,0.02f);CHECK(created.value && kernel.Size()==1 && books.globalCounts[160]==1 && books.depth==0);auto* regular=host.FindEvent(created,3);CHECK(regular!=nullptr);if (!regular) throw std::logic_error("실제 anim Regular 조회 실패");const auto form=regular->Form();CHECK(regular->Parent()==created && regular->Time()==10 && regular->Count()==0);
        // 실제 현재 구간의22프레임을 같은 Kernel/표시/공간에서 진행한다.
        for (std::int32_t index=0;index<run.count;++index) { clock.now=regular->Time();kernel.RunFrame();CHECK(regular->Count()==static_cast<std::uint32_t>(index+1));CHECK(Get(pool.Slot(created),edition==OriginalEdition::Patch1078 ? 36U : 34U,edition==OriginalEdition::Patch1078 ? 4U : 1U)==static_cast<std::uint32_t>(run.first+index)); }
        clock.now=regular->Time();kernel.RunFrame();CHECK(kernel.Size()==0 && host.FromForm(form)==nullptr && (pool.Slot(created)[11]&3) && (pool.Slot(form)[11]&3));CHECK(books.globalCounts[160]==0 && books.totalCost==0 && books.depth==0 && destroy.PreDepth()==0 && destroy.PostDepth()==0 && !sink.rectangles.empty());
        // 마지막 삭제 뒤 실제 해시에 부모나 종속 Form이 남지 않는다.
        for (int level=0;level<4;++level) for (const auto sid:hash.Entries(level)) CHECK(sid!=created.value && sid!=form.value);
    }
}
