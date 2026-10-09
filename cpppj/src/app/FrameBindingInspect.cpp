// GUI raw 월드 연결에 앞서 실제 SHP 추가 헤더를 같은 프레임/표시 모듈로 공급하는 읽기 전용 검사다.
#include "app/FrameBindingInspect.h"
#include "client/SquidRenderer.h"
#include "client/PriestPlacementAssets.h"
#include "o/SquidFrameBinding.h"
#include "o/SquidFactory.h"
#include <bit>
#include <cstdio>
#include <stdexcept>

namespace netstorm::app {
namespace {
// 실제 Renderer에 전달된 유효 영역 수를 함께 세는 관찰 어댑터다.
class Sink final:public o::SquidDisplaySink {
public:
    // Renderer는 sink와 실제 표시 계산기보다 오래 살아야 한다.
    explicit Sink(client::Renderer& renderer):target_(renderer) {}
    // 전체 갱신 억제는 실제 Renderer 상태를 따른다.
    bool Suppressed() const override { return target_.Suppressed(); }
    // 픽셀 영역은 바꾸지 않고 실제 변경 표로 전달한다.
    void Invalidate(o::SquidDisplayRect rect,std::uint32_t flags) override { ++count;target_.Invalidate(rect,flags); }
    std::size_t count{};
private:
    client::SquidRenderer target_;
};
}
// 게임 파일에는 쓰지 않는다. 메타데이터 검사 뒤 실제 raw 프레임 변경과 부분 Draw/Present를 수행한다.
void InspectFrameBinding(const std::filesystem::path& root,o::OriginalEdition edition) {
    o::BaseFileSystem files(root);files.RegisterArchive(root/"netstorm.tarc");client::GameAssets assets(files,edition);
    const auto types=assets.TypeTable().Types();auto shapes=client::SquidRenderer::Shapes(assets,edition);
    client::Renderer renderer(640,480);Sink sink(renderer);o::SquidDisplay display(edition,types,shapes,sink,{0,0,65536,{0,0,640,480}});
    std::size_t physical=0,checked=0;std::vector<std::uint8_t> raw(64);
    // 모든 자산 블록과 공유/그림자 프레임의 칸 크기 및 픽셀 메타데이터를 확인한다.
    for (std::size_t block=0;block<assets.Types().size();++block) {
        const auto number=o::kFirstAssetTypeNumber+block;raw[10]=static_cast<std::uint8_t>(number);
        const auto& shape=shapes[number];const auto limit=shape.frameCount*((types[number].flags1&0x440000)!=0 ? 2 : 1);
        // 실제 물리 프레임은 VFX 사각형이 아니라 Squid의 별도 헤더로 비교한다.
        for (std::size_t frame=0;frame<shape.frames.size();++frame) {
            const auto original=assets.Shapes().SquidMetrics(block,frame);const auto& cached=shape.frames[frame];
            if (std::bit_cast<std::uint32_t>(original.cellWidth)!=std::bit_cast<std::uint32_t>(cached.cellWidth) ||
                std::bit_cast<std::uint32_t>(original.cellHeight)!=std::bit_cast<std::uint32_t>(cached.cellHeight) ||
                original.width!=cached.width || original.height!=cached.height || original.hotspotX!=cached.hotspotX || original.hotspotY!=cached.hotspotY)
                throw std::logic_error("실제 SHP 표시/칸 메타데이터 불일치");
            ++physical;
            if (frame<static_cast<std::size_t>(limit)) {
                const auto size=display.FrameSize(edition,raw,static_cast<int>(frame));
                if (std::bit_cast<std::uint32_t>(size[0])!=std::bit_cast<std::uint32_t>(original.cellWidth) ||
                    std::bit_cast<std::uint32_t>(size[1])!=std::bit_cast<std::uint32_t>(original.cellHeight)) throw std::logic_error("실제 SHP 프레임 공급 불일치");
                ++checked;
            }
        }
    }
    // 두 판본에 공통인 bridgeConnector는 일반 Pop/표시 가상 함수를 사용한다.
    constexpr std::uint32_t connector=155;
    o::SidPool pool(edition,32768,true);o::SquidHash hash;std::vector<std::uint8_t> spots(65536),pixels(640*480);
    o::SquidUnpop unpop(pool,hash,spots,&display);o::SquidPop pop(pool,hash,spots,&display);o::SquidFactory factory(pool,types,false,&unpop);
    o::SquidFrame frames(pool,types,o::MakeSquidFrameHooks(pool,types,display,unpop,pop));
    renderer.Draw(pixels,640,7);renderer.Present([](client::ScreenRect){});
    const auto object=factory.Create(connector);const auto size=display.FrameSize(edition,pool.Slot(object),0);
    if (pop.Pop(object,types[connector],size[0],size[1],20,20)!=o::RawPopResult::Registered) throw std::logic_error("실제 연결 객체 Pop 실패");
    renderer.Draw(pixels,640,7);renderer.Present([](client::ScreenRect){});sink.count=0;
    frames.Set(object,1,0);frames.Set(object,1,o::kFrameUpdateAlternate);
    if (sink.count!=4) throw std::logic_error("실제 연결 객체 old/new 영역 전달 누락");
    const auto painted=renderer.Draw(pixels,640,9).size();renderer.Present([](client::ScreenRect){});
    if (!painted) throw std::logic_error("실제 프레임 부분 Draw 누락");
    std::printf("{\"edition\":\"%s\",\"types\":%zu,\"physical_frames\":%zu,\"frame_size_queries\":%zu,\"invalidations\":%zu,\"painted_rects\":%zu,\"passed\":true}\n",
        edition==o::OriginalEdition::Patch1078 ? "10.78" : "CD",assets.Types().size(),physical,checked,sink.count,painted);
}
// 창을 만들지 않고 모든 자산의 배치 자료와 사제 getter 결과를 독립 대조 도구에 공급한다.
void InspectPriestAssets(const std::filesystem::path& root,o::OriginalEdition edition) {
    o::BaseFileSystem files(root);files.RegisterArchive(root/"netstorm.tarc");client::GameAssets assets(files,edition);
    client::PriestPlacementAssets data(assets);o::SidPool pool(edition,32768,true);o::PriestPlacementShapeState state;
    o::RawPriestPlacementShape shape(pool,assets.TypeTable().Types(),data.frames,data.shapes,data.hotspots,data.geometry,state);
    std::printf("{\"patterns\":[");
    // 패턴 식별 자료는 실제 PE 전역의 타입 번호와 비교한다.
    for (std::size_t i=0;i<data.geometry.patternTypes.size();++i) std::printf("%s%u",i ? "," : "",data.geometry.patternTypes[i]);
    std::printf("],\"types\":[");bool first=true;
    // 내장 타입은 SHP가 없으므로 자산 타입 70부터 원래 순서대로 출력한다.
    for (const auto& asset:assets.Types()) {
        const auto type=static_cast<std::uint32_t>(o::kFirstAssetTypeNumber+asset.block);const auto& meta=data.frames[type];const auto& hot=data.hotspots[type];
        std::printf("%s{\"number\":%u,\"name\":\"%s\",\"default\":%d,\"hotspot\":[%d,%d],\"codes\":\"",first ? "" : ",",type,asset.assetName.c_str(),meta.defaultFrame,hot.x,hot.y);first=false;
        // 4바이트 코드 순서를 독립 parser와 대조할 수 있게 그대로 보존한다.
        for (const auto code:meta.frames.Codes()) std::printf("%02x%02x%02x%02x",code.side,code.variant,code.number,code.flags);
        std::printf("\",\"physical\":%zu",data.shapes[type].frames.size());
        if ((assets.TypeTable().Types()[type].flags2&0x200000)!=0) {
            const auto measured=shape.Measure(type,type,0);
            std::printf(",\"shape_bits\":[%u,%u,%u,%u,%u,%u]",std::bit_cast<std::uint32_t>(measured.bounds.left),std::bit_cast<std::uint32_t>(measured.bounds.top),
                std::bit_cast<std::uint32_t>(measured.bounds.right),std::bit_cast<std::uint32_t>(measured.bounds.bottom),std::bit_cast<std::uint32_t>(measured.anchorX),std::bit_cast<std::uint32_t>(measured.anchorY));
        }
        std::printf("}");
    }
    std::printf("]}\n");
}
}
