// 호스트 통합이다. 개별 몸체의 원본 근거는 surface-graph-integration 및 각 raw 모듈 문서를 따른다.
#include "client/RawSurfaceWorld.h"
#include "o/RawBridgeEvents.h"
#include "o/RawIslandPostPop.h"
#include "o/RawSurfaceLifecycle.h"
#include "o/RawSquidDestroyDispatch.h"
#include "o/SquidDestroyLifecycle.h"
#include "o/SquidFactory.h"
#include "o/SquidFrameBinding.h"
#include "o/SquidOwner.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// 지원하는 표면 월드는 원본 signed SID 한계 안에서 ProcessForm까지 수용한다.
constexpr std::uint32_t kCapacity=32768;
// 해시/spot 지도의 한 변과 free/dead/void/contained 상태 마스크다.
constexpr std::size_t kCells=256*256;
constexpr std::uint8_t kInactive=15;
// 정렬되지 않은 원본 필드를 little endian으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 지정 폭만 읽어 CD의 인접 extra 바이트를 섞지 않는다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(bytes[offset+i])<<(8*i);
    return value;
}
// 초기 저장 프레임을 일반 Pop 전에 쓰며 표시/공간은 Pop에 맡긴다.
void Write(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width) {
    // 필드 밖의 바이트는 보존한다.
    for (std::size_t i=0;i<width;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
}
struct RawSurfaceWorld::Impl final:o::SquidDisplaySink {
    o::OriginalEdition edition;std::vector<o::RiftTypeRecord> types;std::vector<o::RiftTypeFrames> frames;
    std::vector<o::SquidDisplayShape> shapes;std::vector<std::vector<o::FrameCode>> codes;
    o::SidPool pool;o::SquidHash hash;std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(kCells);
    o::BridgeConnectState links;std::uint32_t terrainType;o::SquidPostPopState bookkeeping;
    o::SquidDeletionState deletion;o::BridgeDecayMode mode;o::BridgeEventState eventState;
    o::IslandPostPopState loadState{1,false};o::SquidOwnerMode ownerMode;
    o::SquidProcessState processState;o::FrameTime time{};bool changed{true},loaded{};
    SurfaceDisplayHooks displayHooks;std::size_t effectRequests{},walkerFalls{},deletionRequests{};
    std::unique_ptr<o::RawGraph> graph;std::unique_ptr<o::SquidDisplay> display;
    std::unique_ptr<o::SquidPostPop> postPop;std::unique_ptr<o::SquidUnpop> unpop;
    std::unique_ptr<o::SquidPop> pop;std::unique_ptr<o::SquidFactory> factory;
    std::unique_ptr<o::SquidFrame> frame;std::unique_ptr<o::SquidOwner> owner;
    std::unique_ptr<o::RawSquidNeighbors> neighbors;std::unique_ptr<o::RawBridgeConnect> connect;
    std::unique_ptr<o::RawIslandPostPop> surface;std::unique_ptr<o::RawSquidDestroy> baseDestroy;
    std::unique_ptr<o::RawSquidDestroyDispatch> virtualDestroy;std::unique_ptr<o::SquidDestroyLifecycle> common;
    std::unique_ptr<o::RawSurfaceLifecycle> lifecycle;std::unique_ptr<o::RawBridgeEvents> events;
    std::unique_ptr<o::SquidProcessHost> host;
    o::Kernel kernel; // 모듈/host보다 먼저 해제되어 예약 프로세스의 참조 수명을 보장한다.
    // 순환하는 콜백은 실제 호출 전에 모든 모듈을 구성한다. 생성 중에는 Pop하지 않는다.
    Impl(o::OriginalEdition selected,std::span<const o::RiftTypeRecord> inputTypes,std::span<const o::RiftTypeFrames> inputFrames,
        std::span<const o::SquidDisplayShape> inputShapes,o::BridgeConnectState state,std::uint32_t terrain)
        :edition(selected),types(inputTypes.begin(),inputTypes.end()),frames(inputFrames.begin(),inputFrames.end()),
         shapes(inputShapes.begin(),inputShapes.end()),codes(types.size()),pool(edition,kCapacity,true),links(state),terrainType(terrain) {
        if (frames.size()!=types.size() || shapes.size()!=types.size()) throw std::invalid_argument("raw 표면 월드 자산 표 크기 오류");
        // Graph와 이웃 탐색기는 같은 원본 코드 배열을 사용한다.
        for (std::size_t i=0;i<frames.size();++i) codes[i].assign(frames[i].Codes().begin(),frames[i].Codes().end());
        if (terrainType>=types.size() || links.bridgeType>=types.size() || links.noIslandType>=types.size())
            throw std::out_of_range("raw 표면 월드 타입 번호 오류");
        bookkeeping.localOwner=1;ownerMode.battle=true;eventState.bridgeType=links.bridgeType;
        graph=std::make_unique<o::RawGraph>(pool,hash,spots,types,codes);
        display=std::make_unique<o::SquidDisplay>(edition,types,shapes,*this,o::SquidDisplayView{0,0,65536,{0,0,640,480}});
        postPop=std::make_unique<o::SquidPostPop>(pool,types,bookkeeping,graph.get(),[this](o::Sid sid) { connect->Connect(sid); });
        postPop->SetIslandPrefix([this](o::Sid sid,std::uint32_t flags) { connect->IslandPostPopPrefix(sid,flags); });
        postPop->SetSurfacePrefix([this](o::Sid sid,std::uint32_t flags) { surface->Prefix(sid,flags); });
        unpop=std::make_unique<o::SquidUnpop>(pool,hash,spots,display.get());pop=std::make_unique<o::SquidPop>(pool,hash,spots,display.get(),postPop.get());
        factory=std::make_unique<o::SquidFactory>(pool,types,false,unpop.get());
        frame=std::make_unique<o::SquidFrame>(pool,types,o::MakeSquidFrameHooks(pool,types,*display,*unpop,*pop));
        owner=std::make_unique<o::SquidOwner>(pool,types,bookkeeping,ownerMode);
        neighbors=std::make_unique<o::RawSquidNeighbors>(pool,hash,spots,types,frames);
        o::BridgeConnectHooks objects;
        // 생성/소유자/프레임 변경은 표시 억제 상태와 무관하게 논리 변경을 알린다.
        objects.create=[this](std::uint32_t type,std::uint32_t flags) { changed=true;return factory->Create(type,flags); };
        objects.setOwner=o::MakeOwnerDispatch(pool,links,[this](o::Sid sid,std::uint32_t player) { owner->Set(sid,player);changed=true; });
        objects.pop=[this](o::Sid sid,float x,float y,std::uint32_t flags) { Pop(sid,x,y,flags); };
        objects.setFrame=[this](o::Sid sid,std::int32_t number,std::uint32_t flags) { frame->Set(sid,number,flags);changed=true; };
        objects.notifySurface=[](o::Sid) {}; // 원본 다리 단어의 무상태 진단 경계다.
        connect=std::make_unique<o::RawBridgeConnect>(pool,*neighbors,links,objects);
        surface=std::make_unique<o::RawIslandPostPop>(pool,hash.Entries(0),types,frames,links,loadState,o::IslandPostPopHooks{
            objects,[this](o::Sid sid) { connect->Connect(sid); },
            [this](float x,float y,std::uint32_t type) { return connect->FindTypeAt(x,y,type); },
            [this](o::Sid sid) { changed=true;display->Update(pool.Slot(sid)); },
            [](bool,bool,bool) { throw std::logic_error("raw 표면 월드의 불완전한 받침 묶음"); }});
        baseDestroy=std::make_unique<o::RawSquidDestroy>(pool,*unpop,types);
        virtualDestroy=std::make_unique<o::RawSquidDestroyDispatch>(*baseDestroy,*neighbors,mode,graph.get());
        common=std::make_unique<o::SquidDestroyLifecycle>(pool,types,bookkeeping,deletion,*baseDestroy,o::SquidDeletionHooks{
            [this](const o::SquidDeletionEvent&) { ++deletionRequests; },
            [](const o::SquidDestroyEvent& event) {
                if (event.effect!=o::SquidDestroyEffect::Transmit) throw std::logic_error("raw 표면 월드의 미연결 삭제 효과");
            }},graph.get());
        lifecycle=std::make_unique<o::RawSurfaceLifecycle>(pool,*neighbors,links.connectorType,common->Hooks(),o::SurfaceLifecycleHooks{
            [this](o::Sid sid,float x,float y) { o::ScheduleBridgeFall(*host,sid,x,y); },
            [this](o::Sid sid,std::uint32_t flags) { Destroy(sid,flags); },
            [this](o::Sid) { ++walkerFalls; },
            [this](const o::BridgeLifecycleEvent&) { ++effectRequests; }});
        o::BridgeEventHooks eventHooks;
        // 실제 사건의 삭제/재등록도 같은 가상 경로와 SHP 공급을 사용한다.
        eventHooks.notifySurface=objects.notifySurface;eventHooks.create=[this](std::uint32_t type) { changed=true;return factory->Create(type); };
        eventHooks.destroy=[this](o::Sid sid,std::uint32_t flags) { Destroy(sid,flags); };
        eventHooks.setOwner=[setOwner=objects.setOwner](o::Sid sid,std::uint8_t player) { setOwner(sid,player); };eventHooks.pop=objects.pop;
        events=std::make_unique<o::RawBridgeEvents>(pool,eventState,o::MakeBridgeNeighborHooks(*neighbors,std::move(eventHooks)));
        host=std::make_unique<o::SquidProcessHost>(pool,types,kernel,*baseDestroy,processState,o::MakeBridgeRegularHandler(pool,*events),lifecycle->Hooks());
    }
    // 표시 억제는 현재 GUI 장치에서 조회한다. 콘솔에서는 변경 요청을 보존한다.
    bool Suppressed() const override { return displayHooks.suppressed && displayHooks.suppressed(); }
    // 장치가 교체되어도 현재 콜백으로만 전달한다.
    void Invalidate(o::SquidDisplayRect rect,std::uint32_t flags) override { changed=true;if (displayHooks.invalidate) displayHooks.invalidate(rect,flags); }
    // 현재 raw 프레임의 SHP 크기로 실제 공간과 파생 postPop을 등록한다.
    void Pop(o::Sid sid,float x,float y,std::uint32_t flags) {
        const auto raw=pool.Slot(sid);const auto number=static_cast<std::int32_t>(Read(raw,edition==o::OriginalEdition::Patch1078 ? 36 : 34,edition==o::OriginalEdition::Patch1078 ? 4 : 1));
        const auto size=display->FrameSize(edition,raw,number);
        if (pop->Pop(sid,types[raw[10]],size[0],size[1],x,y,flags)!=o::RawPopResult::Registered) throw std::logic_error("raw 표면 월드 Pop 실패");
        changed=true;
    }
    // 가상 삭제가 거부되면 changed를 새로 켜지 않는다.
    bool Destroy(o::Sid sid,std::uint32_t flags) { const bool result=virtualDestroy->Destroy(sid,flags,host->Hooks());if (result) changed=true;return result; }
};
// 공개 객체는 내부 참조/콜백이 이동하지 않게 단일 구현 객체를 소유한다.
RawSurfaceWorld::RawSurfaceWorld(o::OriginalEdition edition,std::span<const o::RiftTypeRecord> types,
    std::span<const o::RiftTypeFrames> frames,std::span<const o::SquidDisplayShape> shapes,o::BridgeConnectState links,std::uint32_t terrainType)
    :impl_(std::make_unique<Impl>(edition,types,frames,shapes,links,terrainType)) {}
// 프로세스와 raw 데이터는 같은 월드 수명으로 해제된다. GUI의 전역 Kernel에는 form을 등록하지 않는다.
RawSurfaceWorld::~RawSurfaceWorld()=default;
// 저장 객체의 순서와 부착 생성 순서는 구별한다. Graph는 전체 초기 등록을 마친 뒤 한 번 재구성한다.
void RawSurfaceWorld::Load(std::span<const SurfaceSeed> seeds) {
    auto& state=*impl_;if (state.loaded) throw std::logic_error("raw 표면 월드 중복 로드");
    std::vector<SurfaceSeed> sorted(seeds.begin(),seeds.end());
    // 쓰기 전에 전체 초기 입력의 지원 타입/정수 좌표/프레임/소유자를 검사한다.
    for (const auto& seed:sorted) {
        if (seed.type!=state.terrainType && seed.type!=state.links.noIslandType && seed.type!=state.links.bridgeType)
            throw std::invalid_argument("raw 표면 월드의 미지원 초기 타입");
        if (!std::isfinite(seed.x) || !std::isfinite(seed.y) || seed.x<1 || seed.y<1 || seed.x>=256 || seed.y>=256 ||
            std::trunc(seed.x)!=seed.x || std::trunc(seed.y)!=seed.y || seed.owner>o::kPlayerCount || seed.frame<0 ||
            static_cast<std::size_t>(seed.frame)>=state.frames[seed.type].Codes().size()) throw std::out_of_range("raw 표면 월드 초기 필드 오류");
    }
    // 비유한 좌표를 거부한 뒤 정렬한다. 받침 접두가 서/북 이웃을 읽으므로 noIsland는 행 우선이다.
    const auto rank=[&state](std::uint32_t type) { return type==state.terrainType ? 0 : type==state.links.noIslandType ? 1 : 2; };
    std::stable_sort(sorted.begin(),sorted.end(),[&](const SurfaceSeed& first,const SurfaceSeed& second) {
        if (rank(first.type)!=rank(second.type)) return rank(first.type)<rank(second.type);
        return first.y==second.y ? first.x<second.x : first.y<second.y;
    });
    // 실제 생성자·가상 소유자·Pop을 사용하며 아직 Graph를 변경하지 않는다.
    for (const auto& seed:sorted) {
        const auto sid=state.factory->Create(seed.type);state.owner->Set(sid,seed.owner);
        Write(state.pool.AllocatedBytes(sid),state.edition==o::OriginalEdition::Patch1078 ? 36 : 34,static_cast<std::uint32_t>(seed.frame),state.edition==o::OriginalEdition::Patch1078 ? 4 : 1);
        state.Pop(sid,seed.x,seed.y,0);
    }
    state.graph->Rebuild(true);state.bookkeeping.graphsEnabled=true;state.loadState.loadingDepth=0;state.loaded=true;state.changed=true;
}
// GUI가 고정한 한 프레임의 시각을 모든 raw 프로세스가 공유한다.
void RawSurfaceWorld::RunFrame(o::FrameTime time,bool paused) {
    if (!std::isfinite(time.game) || time.game<impl_->time.game) throw std::invalid_argument("raw 월드의 역행/비유한 게임 시각");
    impl_->time=time;impl_->processState.now=time.game;if (!paused) impl_->kernel.RunFrame();
}
// 현재 카메라의 원본 픽셀 기준값과 GUI 변경 표를 연결한다.
void RawSurfaceWorld::SetDisplay(o::SquidDisplayView view,SurfaceDisplayHooks hooks) { impl_->display->SetView(view);impl_->displayHooks=std::move(hooks); }
// 일반 4단계 해시와 실제 타입 필터를 사용하는 조회다.
o::Sid RawSurfaceWorld::Find(float x,float y,std::uint32_t type) const { return impl_->connect->FindTypeAt(x,y,type); }
// 다리 거부 조건과 파생 pre/post를 동일한 호출로 실행한다.
bool RawSurfaceWorld::Destroy(o::Sid sid,std::uint32_t flags) { return impl_->Destroy(sid,flags); }
// 받침/종유석/9칸 소유자 갱신은 복원된 원본 몸체를 사용한다.
void RawSurfaceWorld::SetSupportOwner(float x,float y,std::uint32_t owner) { impl_->surface->SetSupportOwner(x,y,owner);impl_->changed=true; }
// GUI 렌더링에 임시 객체의 옛 프레임/좌표를 복사하지 않는다.
std::vector<SurfaceSnapshot> RawSurfaceWorld::Objects() const {
    std::vector<SurfaceSnapshot> result;const bool patch=impl_->edition==o::OriginalEdition::Patch1078;
    // 현재 살아 있는 자산 번호만 읽는다. ProcessForm/반납 슬롯은 표시 대상이 아니다.
    for (std::uint32_t id=5;id<impl_->pool.Capacity();++id) {
        const o::Sid sid{static_cast<std::uint16_t>(id)};const auto raw=impl_->pool.Slot(sid);
        if ((raw[11]&kInactive) || raw[10]<o::kFirstAssetTypeNumber) continue;
        result.push_back({sid,{raw[10],raw[patch ? 34 : 32],static_cast<std::int32_t>(Read(raw,patch ? 36 : 34,patch ? 4 : 1)),
            std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))}});
    }
    return result;
}
// 변경은 Renderer 억제 여부와 무관하게 한 번만 소비한다.
bool RawSurfaceWorld::TakeChanged() { return std::exchange(impl_->changed,false); }
// 기존 저장 객체 보고와 구별되는 현재 raw 슬롯/Graph/프로세스 상태다.
std::string RawSurfaceWorld::Report() const {
    std::ostringstream out;out.precision(12);std::uint32_t surfaces=0,graphs=0;
    // 예약 0번 다음의 실제 연결 성분을 센다.
    for (std::size_t i=1;i<o::Graph::kCount;++i) if (impl_->graph->Records()[i].inUse) { ++graphs;surfaces+=static_cast<std::uint16_t>(impl_->graph->Records()[i].surfaces); }
    out<<"raw_world\t"<<impl_->time.game<<'\t'<<impl_->time.number<<'\t'<<impl_->kernel.Size()<<'\t'<<surfaces<<'\t'<<graphs<<'\t'<<impl_->bookkeeping.totalCost<<'\t'<<impl_->bookkeeping.depth<<'\t'<<impl_->baseDestroy->PreDepth()<<'\t'<<impl_->baseDestroy->PostDepth()<<'\t'<<impl_->effectRequests<<'\t'<<impl_->deletionRequests<<'\n';
    // 렌더링과 같은 raw 조회 결과를 번호별로 제공한다.
    for (const auto& snapshot:Objects()) { const auto& seed=snapshot.object;out<<"raw_object\t"<<snapshot.sid.value<<'\t'<<seed.type<<'\t'<<seed.owner<<'\t'<<seed.frame<<'\t'<<seed.x<<'\t'<<seed.y<<'\n'; }
    return out.str();
}
}
