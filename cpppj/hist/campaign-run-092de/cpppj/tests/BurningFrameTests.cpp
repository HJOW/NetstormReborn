// 실제 두 PE의 전체 Burning 프레임을 보호 캡처·기존 Sources/Append·dirty 소비자에 대조한다.
#include "ConstructionSupport.h"
#include "client/BurningFrameCapture.h"
#include "client/Renderer.h"
#include "client/SquidRenderer.h"
#include <cstdio>
#include <filesystem>

using namespace netstorm::o;
using namespace netstorm::test::construction;
namespace {
// 실제 기대출력은 마지막16열에만 있으며 입력 객체 준비에는 사용하지 않는다.
const std::vector<std::vector<std::string>>& FrameRows() { static const auto rows=LoadFixture(NETSTORM_BURNINGFRAME_FIXTURE).rows;return rows; }
// 보호 TYPE/SHP를 창·장치 없이 읽는 현재 판본 자산이다.
struct FrameAssets {
    std::filesystem::path root; // 원본판의 보호 자산 루트다.
    BaseFileSystem files; // 현재 TAFF/실제 셰이프를 읽는 기존 저장소다.
    netstorm::client::GameAssets game; // 실제 Dude 메타데이터와 RLE 자료를 소유한다.
    std::vector<RiftTypeRecord> types; // 원본과 동일 로딩 순서의 현재 타입 표다.
    std::vector<SquidDisplayShape> shapes; // 같은 물리 프레임의 표시 헤더다.
    // 같은 자산을 읽기 전용으로 해석하고 원본의 명시 HP/guy 입력과 일치함을 확인한다.
    explicit FrameAssets(OriginalEdition edition):root(std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/(edition==OriginalEdition::Patch1078 ? "originals" : "originalCD")),files(root),game(Register(),edition),types(game.TypeTable().Types().begin(),game.TypeTable().Types().end()),shapes(netstorm::client::SquidRenderer::Shapes(game,edition)) { CHECK(types[70].maxHitPoints==20 && (types[70].flags1&0x10U) && (types[70].flags2&0x400U)); }
    // GameAssets를 만들기 전에 실제 archive를 등록한다.
    const BaseFileSystem& Register() { files.RegisterArchive(root/"netstorm.tarc");return files; }
};
// 각 판본의 전체 보호 자산은 입력 행 사이에 불변으로 공유한다.
const FrameAssets& Assets(OriginalEdition edition) { static const FrameAssets patch(OriginalEdition::Patch1078),cd(OriginalEdition::Cd1072);return edition==OriginalEdition::Patch1078 ? patch : cd; }
// 기존 DirtyRegions 실제 병합 몸체를 같은 clipping 결과에 사용한다.
struct FrameSink final:SquidDisplaySink {
    netstorm::client::DirtyRegions regions{1024,768}; // 원본 명시 viewport와 같은 현재 화면 크기다.
    // 표 넘침/전체 갱신 전역에 해당하는 현재 결과다.
    bool Suppressed() const override { return regions.FullRedraw(); }
    // 실제 원본 순서와 flags0을 기존 Renderer 몸체에 전달한다.
    void Invalidate(SquidDisplayRect rect,std::uint32_t flags) override { regions.Add({rect.left,rect.top,rect.right,rect.bottom},flags); }
};
// little endian double 원문을 정밀도 손실 없이 독립 대조 버퍼에 쓴다.
void PutDouble(std::span<std::uint8_t> bytes,std::size_t offset,double value) {
    const auto bits=std::bit_cast<std::uint64_t>(value);Put(bytes,offset,static_cast<std::uint32_t>(bits));Put(bytes,offset+4,static_cast<std::uint32_t>(bits>>32));
}
// 실제24바이트 입자의 source 포인터만0/1로 정규화한다.
std::string ParticleBytes(const BurningVisualFields& fields) {
    std::vector<std::uint8_t> bytes(fields.capacity*24);
    // 호스트padding을 사용하지 않고 원본 여섯DWORD를 순서대로 쓴다.
    for (std::size_t index=0;index<fields.particles.size();++index) { const auto& particle=fields.particles[index];auto raw=std::span<std::uint8_t>(bytes).subspan(index*24,24);Put(raw,0,Bits(particle.x));Put(raw,4,Bits(particle.y));Put(raw,8,Bits(particle.velocityX));Put(raw,12,Bits(particle.velocityY));Put(raw,16,Bits(particle.life));Put(raw,20,particle.source ? 1U : 0U); }
    return bytes.empty() ? "-" : Hex(bytes);
}
// 원본 정규화 출력과 같은 전체 시각·메타데이터·dirty표를 직렬화한다.
std::vector<std::string> Output(const BurningVisualFields& fields,const BurningFrameFields& frame,const GameRandom& random,const FrameSink& sink,bool captured) {
    std::array<std::uint8_t,16> rect{},metadata{};std::array<std::uint8_t,24> timer{};
    Put(rect,0,Bits(fields.registration.left));Put(rect,4,Bits(fields.registration.top));Put(rect,8,Bits(fields.registration.right));Put(rect,12,Bits(fields.registration.bottom));PutDouble(timer,0,fields.lastTime);PutDouble(timer,8,fields.elapsed);Put(timer,16,Bits(frame.delta));Put(timer,20,Bits(frame.scaledDelta));Put(metadata,0,std::bit_cast<std::uint32_t>(fields.image.width));Put(metadata,4,std::bit_cast<std::uint32_t>(fields.image.height));Put(metadata,8,std::bit_cast<std::uint32_t>(fields.image.hotspotX));Put(metadata,12,std::bit_cast<std::uint32_t>(fields.image.hotspotY));std::string dirty;
    // 병합/무시된 항목까지 순서대로 실제 사각형/flags를 보존한다.
    for (const auto& entry:sink.regions.Entries()) { if (!dirty.empty()) dirty+=';';dirty+=std::to_string(entry.rect.left)+','+std::to_string(entry.rect.top)+','+std::to_string(entry.rect.right)+','+std::to_string(entry.rect.bottom)+','+std::to_string(entry.flags); }
    return {Hex(rect),std::to_string(fields.registration.zOrder),std::to_string(fields.total),Hex(timer),std::to_string(fields.stage),Hex(metadata),fields.image.pixels.empty() ? "-" : Hex(fields.image.pixels),fields.rows.empty() ? "-" : Hex(fields.rows),std::to_string(fields.selected),fields.off ? "1" : "0",ParticleBytes(fields),std::to_string(random.State()),sink.regions.FullRedraw() ? "1" : "0",dirty.empty() ? "-" : dirty,captured ? std::to_string(fields.image.pixels.size())+','+std::to_string(fields.rows.size()) : "-"};
}
// 현재 입력만 만들고 실제 Frame→Capture/Sources/Append/dirty 사슬을 수행한다.
std::vector<std::string> RunFrame(const std::vector<std::string>& row) {
    const auto edition=row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,32768,true);
    // 명시 피해50은 실제 client 슬롯 번호로 확보한다.
    for (std::uint16_t sid=5;sid<=50;++sid) CHECK(pool.Allocate(2)==Sid{sid});
    auto raw=pool.AllocatedBytes(Sid{50});std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,patch ? 0x508cc8U : 0x5072f0U);raw[10]=70;Put(raw,14,Bits(20.75f));Put(raw,18,Bits(21.9f));Put(raw,26,10,patch ? 4U : 2U);raw[patch ? 35U : 33U]=static_cast<std::uint8_t>(std::stoi(row[9]));Put(raw,patch ? 36U : 34U,Number(row[10]),patch ? 4U : 1U);
    const auto& assets=Assets(edition);netstorm::client::ShapeToBufferState captureState{std::bit_cast<std::int32_t>(Number(row[11])),false,{}};netstorm::client::RawShapeToBuffer capture(pool,assets.types,assets.shapes,assets.game.Shapes(),captureState);GameRandom random(Number(row[6]));RenderRegistryHooks registryHooks;registryHooks.reserve=[](std::uint32_t)->bool { throw std::logic_error("프레임의 미관찰 표시 목록 확보"); };registryHooks.releaseArray=[] { throw std::logic_error("프레임의 미관찰 표시 목록 반납"); };registryHooks.attach=[](RenderProcessFields&,std::uint32_t,Sid,std::uint32_t) { throw std::logic_error("프레임의 미관찰 Host 부착"); };const auto rejectReserve=registryHooks.reserve;RawRenderRegistry registry(pool,std::move(registryHooks));BurningVisualState visualState;BurningVisualHooks visualHooks;visualHooks.reserve=rejectReserve;RawBurningVisual visual(pool,registry,visualState,std::move(visualHooks));BurningSourcesState sourceState;RawBurningSources sources(pool,assets.types,visual,random,sourceState);BurningFrameState state;state.now=std::bit_cast<double>(std::stoull(row[7]));state.pixelScaleX=Float(Number(row[12]));state.pixelScaleY=state.pixelScaleX;state.cameraX=0.25f;state.cameraY=0.5f;state.wind=Float(Number(row[8]));state.deltaMultiplier=0.3f;state.viewport={0,0,1024,768};FrameSink sink;bool captured=false;auto hooks=netstorm::client::MakeBurningFrameCapture(capture,pool);const auto actualCapture=hooks.capture;hooks.capture=[&](Sid sid) { captured=true;return actualCapture(sid); };RawBurningFrame logic(pool,assets.types,visual,sources,random,state,sink,std::move(hooks));BurningVisualFields fields;fields.capacity=Number(row[1]);fields.particles.resize(fields.capacity);fields.registration.zOrder=1234;fields.registration.left=Number(row[4]) ? 12.25f : 1100.25f;fields.registration.top=Number(row[4]) ? 13.75f : 1100.75f;fields.registration.right=Number(row[4]) ? 30.5f : 1120.5f;fields.registration.bottom=Number(row[4]) ? 35.5f : 1125.5f;fields.lastTime=1;fields.elapsed=0.123456789;fields.off=Number(row[3])!=0;fields.victim=Sid{50};BurningFrameFields frame{123,-456};const auto stage=Number(row[2]);
    if (fields.capacity && stage) logic.Capture(fields);
    fields.stage=stage;fields.victim=Sid{static_cast<std::uint16_t>(Number(row[13]))};const auto pattern=Number(row[5]);
    if (fields.capacity && stage) {
        fields.total=pattern==1 ? std::min(fields.capacity,2U) : pattern==2 ? std::min(fields.capacity,3U) : pattern==3 ? std::min(fields.capacity,4U) : 0U;fields.selected=pattern ? 1 : 0;
        // 원본과 같은 source 좌표/수명비트·이동입자 패턴만 공급한다.
        for (std::size_t index=0;index<fields.particles.size();++index) {
            const bool source=pattern==1 && index<2;const auto x=source ? (index ? 7.0f : 2.0f) : 25.125f+static_cast<float>(index),y=source ? 3.0f : 30.375f+static_cast<float>(index);const std::array<std::uint32_t,4> exceptional{0x7fc00001U,0xffc00001U,0x7f800000U,0x80000000U};const auto life=source ? 9999.0f : pattern==2 ? (index==0 ? 5.25f : index==1 ? 0.1f : 0.0f) : pattern==3 && index<4 ? Float(exceptional[index]) : 0.0f;fields.particles[index]={x,y,0.1f,-1.25f,life,source};
            if (source) { const auto group=(static_cast<std::int32_t>(x)+static_cast<std::int32_t>(y))/5;if (static_cast<std::size_t>(group)<fields.rows.size()) fields.rows[group]=1; }
        }
    }
    logic.Step(fields,frame);return Output(fields,frame,random,sink,captured);
}
// 현재 입력을 실행한 뒤에만 실제 독립출력과 비교하고 첫차이를 상세히 기록한다.
void ReplayFrame(std::string_view edition) {
    std::size_t count=0,expectedCount=0;int failures=0;
    // 해당판본의 고유입력 수를 출력과 무관하게 집계한다.
    for (const auto& row:FrameRows()) if (row[0]==edition) ++expectedCount;
    // 마지막 actual_calls는 원본 실행기의 경로증거이며 C++semantic15필드는 따로 대조한다.
    for (const auto& row:FrameRows()) {
        if (row[0]!=edition) continue;CHECK(row.size()==30);const auto output=RunFrame(row);const auto expected=std::vector<std::string>(row.begin()+14,row.end()-1);const bool same=output==expected;CHECK(same);
        if (!same) {
            // 첫불일치의 원본/C++각필드와 현재행을 표시한다.
            for (std::size_t index=0;index<output.size();++index) if (output[index]!=expected[index]) std::printf("%s BurningFrame 행%zu 결과%zu 기대%s / 실제%s\n",row[0].c_str(),count,index,expected[index].c_str(),output[index].c_str());
            if (++failures>=5) break;
        }
        ++count;
    }
    CHECK(!failures && count==expectedCount);
}
}
// 실제10.78의 초기캡처/단계·visible·timer·source·이동입자와 전체dirty표를 대조한다.
TEST_CASE(BurningFrame_PatchNativeCaptureSourcesParticlesAndDirty) { ReplayFrame("originals"); }
// CD10.72의 signedDWORDlife와 저장된vx재읽기를 포함한 같은 실제 사슬을 대조한다.
TEST_CASE(BurningFrame_CdNativeCaptureSourcesParticlesAndDirty) { ReplayFrame("originalCD"); }
