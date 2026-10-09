// GUI raw 월드 연결에 앞서 실제 SHP 추가 헤더를 같은 프레임/표시 모듈로 공급하는 읽기 전용 검사다.
#include "app/FrameBindingInspect.h"
#include "client/SquidRenderer.h"
#include "client/PriestPlacementAssets.h"
#include "o/CanonTypeDecoder.h"
#include "o/RawCanonPixelShape.h"
#include "o/RawCanonPlacement.h"
#include "o/RawCanonPlacementGeometry.h"
#include "o/SquidFrameBinding.h"
#include "o/SquidFactory.h"
#include <algorithm>
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
// 일반 자산의 기본 프레임과 모든 패턴/정상 회전을 실제 SHP getter로 계산한다.
void InspectCanonPixelShapes(const std::filesystem::path& root,o::OriginalEdition edition,bool inspectPlacement) {
    o::BaseFileSystem files(root);files.RegisterArchive(root/"netstorm.tarc");client::GameAssets assets(files,edition);client::PriestPlacementAssets data(assets);
    o::SidPool pool(edition,32768,true);o::PriestPlacementShapeState state;
    o::RawCanonPixelShape shape(pool,assets.TypeTable().Types(),data.frames,data.shapes,data.hotspots,data.geometry,state);
    // 각 특수 타입의 실제 패턴 개수다. 전투 네 타입은 같은 한 칸 표를 공유한다.
    constexpr std::array<int,8> kCounts{68,26,2,1,1,1,1,1};
    // 같은 관찰 형식으로 패턴과 일반 자산의 여섯 float 비트를 출력한다.
    const auto output=[&](std::uint32_t type,std::uint32_t argument,std::uint32_t direction) {
        if (inspectPlacement) {
            // 안쪽/여백 직전·직후/지도 끝·강제 허용·부호 BYTE 비교를 독립 관찰한다.
            constexpr std::array<float,8> kX{0,1.99999f,2,20.5f,255,256,-1,20.5f};
            // 후반 정책은 기록 경계이며 성공/실패를 교차해 접두의 위임을 확인한다.
            for (std::size_t profile=0;profile<kX.size();++profile) {
                o::PriestPlacementState placementState{profile==6 ? 1U : 0U,profile==7 ? -1 : 8,99};
                bool entered=false,local=false;const auto lower=profile%2==0;
                o::RawCanonPlacement placement(pool,assets.TypeTable().Types(),placementState,o::MakeCanonShapePlacementHooks(pool,shape,{{},
                    [&](const o::CanonPlacementQuery& request,bool localOwner) {
                        if (request.type!=type || request.argument!=argument || request.flags!=direction) throw std::logic_error("일반 배치 인자 전달 오류");
                        entered=true;local=localOwner;placementState.blockedRelation=0x77;return lower; }}));
                const auto owner=profile==7 ? 255U : 0x108U;
                const bool allowed=placement.MayPlace({type,argument,kX[profile],21.25f,direction,owner,0});
                std::printf("{\"type\":%u,\"argument\":%u,\"direction\":%u,\"profile\":%zu,\"allowed\":%u,\"entered\":%u,\"local\":%u,\"blocked\":%u}\n",
                    type,argument,direction,profile,allowed ? 1U : 0U,entered ? 1U : 0U,local ? 1U : 0U,placementState.blockedRelation);
            }
            return;
        }
        const auto measured=shape.Measure(type,argument,direction);
        std::printf("{\"type\":%u,\"argument\":%u,\"direction\":%u,\"shape_bits\":[%u,%u,%u,%u,%u,%u]}\n",type,argument,direction,
            std::bit_cast<std::uint32_t>(measured.bounds.left),std::bit_cast<std::uint32_t>(measured.bounds.top),std::bit_cast<std::uint32_t>(measured.bounds.right),
            std::bit_cast<std::uint32_t>(measured.bounds.bottom),std::bit_cast<std::uint32_t>(measured.anchorX),std::bit_cast<std::uint32_t>(measured.anchorY));
    };
    // 실제 자산 타입은 내장 번호 70 뒤의 원본 순서다.
    for (const auto& asset:assets.Types()) {
        const auto type=static_cast<std::uint32_t>(o::kFirstAssetTypeNumber+asset.block);
        const auto found=std::find(data.geometry.patternTypes.begin(),data.geometry.patternTypes.end(),type);
        if (found==data.geometry.patternTypes.end()) { output(type,type,0);continue; }
        const auto kind=static_cast<std::size_t>(found-data.geometry.patternTypes.begin());
        // CD의 홀수 방향 assert를 피하고 네 실제 회전의 전체 범위를 출력한다.
        for (int index=0;index<kCounts[kind];++index)
            // 패턴의 argument는 프레임 번호가 아니라 패턴 표의 번호다.
            for (std::uint32_t direction=0;direction<8;direction+=2) output(type,static_cast<std::uint32_t>(index),direction);
    }
}
// 일반 자산과 모든 특수 패턴의 모양 순회/현재 발자국 finder 사각형을 콘솔로 관찰한다.
void InspectCanonPlacementGeometry(const std::filesystem::path& root,o::OriginalEdition edition) {
    o::BaseFileSystem files(root);files.RegisterArchive(root/"netstorm.tarc");client::GameAssets assets(files,edition);client::PriestPlacementAssets data(assets);
    o::SidPool pool(edition,32768,true);o::RawCanonPlacementGeometry geometry(pool,assets.TypeTable().Types(),data.frames,data.geometry);
    // 특수 타입의 정상 패턴 개수다. 나머지 타입은 기본 프레임 한 칸 또는 빈 모양이다.
    constexpr std::array<int,8> kCounts{68,26,2,1,1,1,1,1};
    // 이 콘솔 검사는 모양 계산만 관찰한다. 지형/관계 판단은 호출하지 않는다.
    const o::CanonPlacementGeometryHooks hooks{[](const o::CanonTypeQuery&) {},{},
        [](const o::CanonTypeQuery&,const o::CanonPlacementCell&) { return true; },[](const o::CanonTypeQuery&) { return true; }};
    // 같은 출력 형식으로 초기 범위와 각 유효 모양의 실제 입력을 보존한다.
    const auto output=[&](std::uint32_t type,int argument,int direction) {
        const o::CanonTypeQuery query{type,argument,direction,20.5f,21.25f,false};const auto area=geometry.Bounds(query);bool first=true;
        std::printf("{\"type\":%u,\"argument\":%d,\"direction\":%d,\"bounds\":[%d,%d,%d,%d],\"cells\":[",type,argument,direction,area.left,area.top,area.right,area.bottom);
        geometry.Inspect(query,[&](const o::CanonPlacementCell& cell) {
            std::printf("%s[%d,%u,%u,%d,%u,%u,%u,%d,%d,%d,%d]",first ? "" : ",",cell.frame,std::bit_cast<std::uint32_t>(cell.x),std::bit_cast<std::uint32_t>(cell.y),cell.label,static_cast<unsigned>(cell.side),
                std::bit_cast<std::uint32_t>(cell.snappedX),std::bit_cast<std::uint32_t>(cell.snappedY),cell.area.left,cell.area.top,cell.area.right,cell.area.bottom);first=false;return true;
        },hooks);
        std::printf("]}\n");
    };
    // 실제 타입 순서로 모든 자산을 검사하고 특수 타입은 전체 패턴/네 회전을 펼친다.
    for (const auto& asset:assets.Types()) {
        const auto type=static_cast<std::uint32_t>(o::kFirstAssetTypeNumber+asset.block);const auto found=std::find(data.geometry.patternTypes.begin(),data.geometry.patternTypes.end(),type);
        if (found==data.geometry.patternTypes.end()) { output(type,static_cast<int>(type),0);continue; }
        const auto kind=static_cast<std::size_t>(found-data.geometry.patternTypes.begin());
        // 원본 정상 패턴 번호마다 CD도 허용하는 짝수 방향을 사용한다.
        for (int index=0;index<kCounts[kind];++index)
            // 각 회전의 칸 순서/사각형을 따로 기록한다.
            for (int direction=0;direction<8;direction+=2) output(type,index,direction);
    }
}
// 각 출력 줄은 하나의 실제 타입/패턴/방향이며 원본 파일/GUI 상태를 바꾸지 않는다.
void InspectCanonPatterns(const std::filesystem::path& root,o::OriginalEdition edition) {
    o::BaseFileSystem files(root);files.RegisterArchive(root/"netstorm.tarc");client::GameAssets assets(files,edition);client::PriestPlacementAssets data(assets);
    // 모든 타입별 표의 정상 패턴 개수와 앞 단계에서 복원한 일반 사제 번호다.
    constexpr std::array<int,9> kCounts{68,26,2,1,1,1,1,1,1};
    const auto types=assets.TypeTable().Types();
    // 실제 자산 프레임 표에서 모든 패턴을 네 회전으로 진행한다.
    for (std::size_t kind=0;kind<kCounts.size();++kind) {
        const auto type=kind<8 ? data.geometry.patternTypes[kind] : static_cast<std::uint32_t>(o::kFirstAssetTypeNumber+assets.Find("priest").block);
        const auto& record=types[type];const auto& meta=data.frames[type];
        // 비패턴 사제는 배치 경로처럼 타입 번호를 첫 인자로 보낸다.
        for (int index=0;index<kCounts[kind];++index) {
            const int argument=kind<8 ? index : static_cast<int>(type);
            // CD의 홀수 회전은 원본 assert이므로 정상 자산 검사는 짝수 방향만 사용한다.
            for (int direction=0;direction<8;direction+=2) {
                auto decoder=o::DecodeCanonType(meta.frames,meta.defaultFrame,data.geometry.patternTypes,{type,argument,direction,20.5f,21.25f,false},edition);
                const auto area=decoder.Bounds(record.footX,record.footY);
                std::printf("{\"kind\":%zu,\"type\":%u,\"argument\":%d,\"direction\":%d,\"bounds\":[%d,%d,%d,%d],\"sequence\":[",kind,type,argument,direction,area[0],area[1],area[2],area[3]);bool first=true;
                // 칸 좌표의 float 비트와 현재 프레임/라벨/방향을 원본 순서로 내보낸다.
                while (decoder.Valid()) {
                    std::printf("%s[%d,%u,%u,%d,%d]",first ? "" : ",",decoder.Frame(),std::bit_cast<std::uint32_t>(decoder.X()),std::bit_cast<std::uint32_t>(decoder.Y()),decoder.Label(),static_cast<int>(decoder.Side()));
                    first=false;decoder.Advance();
                }
                const auto end=decoder.Bounds(record.footX,record.footY);
                std::printf("],\"end\":[%d,%u,%u,%d],\"end_bounds\":[%d,%d,%d,%d]}\n",decoder.Frame(),std::bit_cast<std::uint32_t>(decoder.X()),std::bit_cast<std::uint32_t>(decoder.Y()),decoder.Label(),end[0],end[1],end[2],end[3]);
            }
        }
    }
}
}
