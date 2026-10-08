// 실제 프레임 훅·표시 계산·Renderer 변경 표와 공간 재등록을 같은 raw 풀로 검사한다.
#include "RawSceneSupport.h"
#include "o/SquidFrameBinding.h"
#include "o/SquidFactory.h"
#include "client/SquidRenderer.h"

using namespace netstorm;
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 공통 가상 표시/Pop을 쓰는 원본 연결 객체 타입 번호다.
constexpr std::uint32_t kType=155;
struct Notice { SquidDisplayRect rect;std::uint32_t flags,frame; };
// 실제 Renderer 어댑터 앞에서 전달 순서와 그 시점의 raw 프레임만 관찰한다.
struct Sink final:SquidDisplaySink {
    client::Renderer renderer{640,480};client::SquidRenderer target{renderer};SidPool& pool;Sid object{};std::vector<Notice> notices;
    // raw 관찰은 같은 풀을 참조한다.
    explicit Sink(SidPool& source):pool(source) {}
    // 실제 전체 갱신 상태를 그대로 사용한다.
    bool Suppressed() const override { return target.Suppressed(); }
    // 원본 영역은 변형하지 않고 실제 Draw/Present 변경 표에 전달한다.
    void Invalidate(SquidDisplayRect rect,std::uint32_t flags) override {
        const bool patch=pool.Edition()==OriginalEdition::Patch1078;
        notices.push_back({rect,flags,Get(pool.Slot(object),patch ? 36U : 34U,patch ? 4U : 1U)});target.Invalidate(rect,flags);
    }
    // 이전 Draw/Present를 끝내 다음 부분 갱신을 관찰한다.
    void Flush(std::vector<std::uint8_t>& pixels,std::uint8_t color) { renderer.Draw(pixels,640,color);renderer.Present([](client::ScreenRect){});notices.clear(); }
};
// 원본 번호 체계에서 해당 타입의 생성자와 일반 공간 플래그를 입력한다.
std::vector<RiftTypeRecord> Types(OriginalEdition edition,bool shadow=false) {
    std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188 : 171);
    auto& type=types[kType];type.constructorAddress=TypeConstructorAddress(edition,kType);type.flags1=0x28000002|(shadow ? 0x40000U : 0U);
    type.flags2=0x02000000;type.footX=type.footY=1;return types;
}
// 다른 픽셀 경계와 칸 크기를 가진 합성 SHP 메타데이터다.
std::vector<SquidDisplayShape> Shapes(std::size_t count,bool shadow=false) {
    std::vector<SquidDisplayShape> shapes(count);
    shapes[kType]={shadow ? 2 : 4,true,{{11,17,12,16,1,1},{26,10,5,2,2,1.5f},{8,4,2,1,5,5},{6,3,1,0,6,6}}};
    if (shadow) { shapes[kType].frames[2].cellWidth=1;shapes[kType].frames[2].cellHeight=1; }
    return shapes;
}
// 사각형은 원본 정수 모서리별로 비교한다.
bool Same(SquidDisplayRect a,SquidDisplayRect b) { return a.left==b.left && a.top==b.top && a.right==b.right && a.bottom==b.bottom; }
// 한 판본의 old/new 변경 영역과 단계 이동 경로를 확인한다.
void Run(OriginalEdition edition,bool shadow) {
    const bool patch=edition==OriginalEdition::Patch1078;const std::size_t frameOffset=patch ? 36 : 34,frameWidth=patch ? 4 : 1,levelOffset=patch ? 33 : 31;
    SidPool pool(edition,32768,true);auto types=Types(edition,shadow);auto shapes=Shapes(types.size(),shadow);Sink sink(pool);
    SquidDisplay display(edition,types,shapes,sink,{0,0,65536,{0,0,640,480}});SquidHash hash;std::vector<std::uint8_t> spots(65536);
    SquidUnpop unpop(pool,hash,spots,&display);SquidPop pop(pool,hash,spots,&display);SquidFactory factory(pool,types,false,&unpop);
    const Sid object=factory.Create(kType);sink.object=object;
    if (shadow) pool.AllocatedBytes(object)[patch ? 40 : 35]|=4;
    std::vector<std::uint8_t> pixels(640*480);sink.Flush(pixels,7);
    CHECK(pop.Pop(object,types[kType],1,1,20,20)==RawPopResult::Registered);sink.Flush(pixels,7);
    SquidFrame frame(pool,types,MakeSquidFrameHooks(pool,types,display,unpop,pop));
    frame.Set(object,1,shadow ? kFrameUpdateAlternate : 0);
    if (shadow) {
        CHECK(sink.notices.size()==4);
        CHECK(Same(sink.notices[0].rect,{296,patch ? 189 : 195,332,222}));
        CHECK(Same(sink.notices[1].rect,{318,219,327,224}));
        CHECK(Same(sink.notices[2].rect,{312,patch ? 203 : 209,345,229}));
        CHECK(Same(sink.notices[3].rect,{319,220,326,224}));
        // old main/shadow 뒤 new main/shadow다. 0x2000 자체는 변경 표 플래그에 섞이지 않는다.
        for (std::size_t i=0;i<sink.notices.size();++i) CHECK(sink.notices[i].flags==16 && sink.notices[i].frame==(i<2 ? 0U : 1U));
        sink.Flush(pixels,9);frame.Set(object,1,kFrameUpdateAlternate);CHECK(sink.notices.size()==4);
        sink.Flush(pixels,9);sink.renderer.InvalidateAll();frame.Set(object,0,0);
        CHECK(sink.notices.empty() && Get(pool.Slot(object),frameOffset,frameWidth)==0);
    } else {
        CHECK(sink.notices.size()==2 && sink.notices[0].frame==0 && sink.notices[1].frame==1);
        CHECK(Same(sink.notices[0].rect,{308,204,320,222}) && Same(sink.notices[1].rect,{315,218,342,229}));
        auto painted=sink.renderer.Draw(pixels,640,9);CHECK(!painted.empty());sink.renderer.Present([](client::ScreenRect){});
        CHECK(pixels[204*640+308]==9 && pixels[228*640+341]==9 && pixels[200*640+300]==7);
        sink.notices.clear();frame.Set(object,2,0);
        CHECK(sink.notices.size()==2 && pool.Slot(object)[levelOffset]==1 && hash.Bucket(1,20,20)==object.value);
        sink.Flush(pixels,9);frame.Set(object,3,0);
        CHECK(sink.notices.size()==2 && sink.notices[0].frame==2 && sink.notices[1].frame==3);
        CHECK(pool.Slot(object)[levelOffset]==3 && hash.Bucket(1,20,20)==0 && hash.Bucket(3,20,20)==object.value && (pool.Slot(object)[11]&4)==0);
        CHECK(Same(sink.notices[0].rect,{318,219,327,224}) && Same(sink.notices[1].rect,{319,220,326,224}));
        CHECK(display.FrameSize(edition,pool.Slot(object),3)==(std::array<float,2>{6,6}));
    }
}
}
// 픽셀 경계와 칸 크기가 다른 입력으로 두 판본의 표시/해시 연결을 확인한다.
TEST_CASE(frame_display_binding_updates_old_new_pixels_and_repop_level) {
    // 판본별 프레임 폭과 실제 Factory를 모두 실행한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) Run(edition,false);
}
// 선택 표식·그림자·alternate 표시 경로·같은 프레임 반복과 전체 갱신 억제를 확인한다.
TEST_CASE(frame_display_binding_preserves_shadow_marker_and_suppression) {
    // CD는 선택 표식의 위쪽 확장량이 패치와 다르다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) Run(edition,true);
}
// 현재 프레임 범위 오류는 단계 바이트/해시/표시 영역 쓰기 전에 거부된다.
TEST_CASE(frame_display_binding_rejects_bad_current_frame_and_missing_shape) {
    const auto edition=OriginalEdition::Patch1078;SidPool pool(edition,32768,true);auto types=Types(edition);auto shapes=Shapes(types.size());Sink sink(pool);
    SquidDisplay display(edition,types,shapes,sink,{0,0,65536,{0,0,640,480}});SquidHash hash;std::vector<std::uint8_t> spots(65536);
    SquidUnpop unpop(pool,hash,spots,&display);SquidPop pop(pool,hash,spots,&display);SquidFactory factory(pool,types,false,&unpop);
    const Sid sid=factory.Create(kType);sink.object=sid;Put(pool.AllocatedBytes(sid),36,4);
    SquidFrame frame(pool,types,MakeSquidFrameHooks(pool,types,display,unpop,pop));const auto before=Hex(pool.Slot(sid));
    CHECK(Throws([&] { frame.Set(sid,0,0); }) && Hex(pool.Slot(sid))==before && sink.notices.empty());
    Put(pool.AllocatedBytes(sid),36,0);shapes[kType].loaded=false;const auto missing=Hex(pool.Slot(sid));
    CHECK(Throws([&] { frame.StoreLevel(sid); }) && Hex(pool.Slot(sid))==missing);
    shapes[kType].loaded=true;types[kType].flags1|=0x40000;
    CHECK(display.FrameSize(edition,pool.Slot(sid),4-1)==(std::array<float,2>{6,6}));
    CHECK(Throws([&] { display.FrameSize(edition,pool.Slot(sid),4); }));
    CHECK(Throws([&] { display.FrameSize(OriginalEdition::Cd1072,pool.Slot(sid),0); }));
}
// 표시가 미연결인 공간 모듈·다른 풀/판본/타입 표 조합은 구성 시점에 거부한다.
TEST_CASE(frame_display_binding_rejects_mismatched_connections) {
    const auto edition=OriginalEdition::Patch1078;SidPool pool(edition,32768,true),other(edition,32768,true);
    auto types=Types(edition);auto shapes=Shapes(types.size());Sink sink(pool);SquidDisplay display(edition,types,shapes,sink,{0,0,65536,{0,0,640,480}});
    SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidUnpop unpop(pool,hash,spots,&display);SquidPop pop(pool,hash,spots,&display);
    SquidUnpop silentUnpop(pool,hash,spots);SquidPop silentPop(pool,hash,spots);
    CHECK(Throws([&] { MakeSquidFrameHooks(pool,types,display,silentUnpop,pop); }));
    CHECK(Throws([&] { MakeSquidFrameHooks(pool,types,display,unpop,silentPop); }));
    CHECK(Throws([&] { MakeSquidFrameHooks(other,types,display,unpop,pop); }));
    SquidHash otherHash;std::vector<std::uint8_t> otherSpots(65536);SquidUnpop wrongHash(pool,otherHash,spots,&display),wrongSpots(pool,hash,otherSpots,&display);
    CHECK(Throws([&] { MakeSquidFrameHooks(pool,types,display,wrongHash,pop); }));
    CHECK(Throws([&] { MakeSquidFrameHooks(pool,types,display,wrongSpots,pop); }));
    auto copied=types;copied[kType].footX=2;CHECK(Throws([&] { MakeSquidFrameHooks(pool,copied,display,unpop,pop); }));
    CHECK(Throws([&] { display.ValidateFrameBinding(OriginalEdition::Cd1072,types); }));
}
// 패치의 logical frameCheck와 CD의 물리 범위 보호를 구분하고 shadow 두 배 범위를 유지한다.
TEST_CASE(frame_display_shp_size_preserves_double_frames_and_cd_range_rules) {
    // CD는 logical 클러스터 범위를 검사하지 않지만 물리 배열 밖은 새 코드에서 거부한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        SidPool pool(edition,32768,true);auto types=Types(edition);auto shapes=Shapes(types.size());shapes[kType].frameCount=1;
        Sink sink(pool);SquidDisplay display(edition,types,shapes,sink,{0,0,65536,{0,0,640,480}});
        std::vector<std::uint8_t> raw(64);raw[10]=static_cast<std::uint8_t>(kType);
        if (edition==OriginalEdition::Patch1078) CHECK(Throws([&] { display.FrameSize(edition,raw,1); }));
        else CHECK(display.FrameSize(edition,raw,1)==(std::array<float,2>{2,1.5f}));
        types[kType].flags1|=0x40000;
        CHECK(display.FrameSize(edition,raw,1)==(std::array<float,2>{2,1.5f}));
        if (edition==OriginalEdition::Patch1078) CHECK(Throws([&] { display.FrameSize(edition,raw,2); }));
        else CHECK(display.FrameSize(edition,raw,2)==(std::array<float,2>{5,5}));
        // 원본 frameCheck의 해제 흔적 값이다. 이 값은 CD의 직접 조회에는 영향을 주지 않는다.
        types[kType].maxHitPoints=-0x22222223;
        if (edition==OriginalEdition::Patch1078) CHECK(Throws([&] { display.FrameSize(edition,raw,0); }));
        else CHECK(display.FrameSize(edition,raw,0)==(std::array<float,2>{1,1}));
        CHECK(Throws([&] { display.FrameSize(edition,raw,-1); }));
        CHECK(Throws([&] { display.FrameSize(edition,raw,4); }));
    }
}
