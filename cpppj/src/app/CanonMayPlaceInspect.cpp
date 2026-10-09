// 실제 요청자 TYPE/SHP와 합성 주변 지형/장부로 전체 배치를 창 없이 관찰한다.
#include "app/FrameBindingInspect.h"
#include "client/PriestPlacementAssets.h"
#include "o/RawCanonPlacementPipeline.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdio>

namespace netstorm::app {
namespace {
// 전체 대조 대상으로 삼는 원본 자산 이름과 실제 패턴 개수다.
constexpr std::array<const char*,7> kNames{"sunArcher","sunBlocker","sunFactory","priest","bridge","noIsland","island"};
constexpr std::array<std::uint32_t,8> kCounts{68,26,2,1,1,1,1,1};
// 독립 원본 관찰과 같은 합성 장면의 예약 내장 타입/슬롯 수다.
constexpr std::uint32_t kSurface=10,kWorkshop=11,kObstacle=12,kSlots=128,kCapacity=24000;
// 분석 입력의 비정렬 little endian 필드를 주변 바이트를 보존하며 기록한다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    // 원본 필드의 저장 폭만큼 낮은 바이트부터 쓴다.
    for (std::size_t i=0;i<width;++i) raw[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// 출력 자료만 16진수로 기록하고 원본 배치 기대값을 계산하지 않는다.
void Hex(std::span<const std::uint8_t> bytes) {
    // 배열의 실제 순서/바이트를 그대로 출력한다.
    for (const auto value:bytes) std::printf("%02x",value);
}
}
// 실제 7개 요청자 자산/모든 정상 패턴 회전과 네 장면을 새 파이프라인에 공급한다.
void InspectCanonMayPlace(const std::filesystem::path& root,o::OriginalEdition edition) {
    o::BaseFileSystem files(root);files.RegisterArchive(root/"netstorm.tarc");client::GameAssets assets(files,edition);client::PriestPlacementAssets data(assets);
    const auto originals=assets.TypeTable().Types();std::vector<o::RiftTypeRecord> types(originals.begin(),originals.end());
    o::SidPool pool(edition,kCapacity,false);o::SquidHash hash;const bool patch=edition==o::OriginalEdition::Patch1078;
    std::vector<std::uint16_t> islands(65536);std::vector<std::uint8_t> spots(65536,6);std::vector<o::CanonPlacementIslandRegion> regions(128,{0,1});
    o::CanonPlacementPipelineState state;state.geometry=data.geometry;state.relations.islandCount=128;
    o::SquidPostPopState books;books.ownerFactories[1].entries={90};books.ownerFactories[1].count=1;o::SquidPostPopList additional;o::ContainedFinderState contained;
    // 합성 지형은 자산 번호와 겹치지 않는 내장 타입에만 입력한다.
    types[kSurface].flags1=0x800;types[kSurface].flags2=2;types[kSurface].footX=types[kSurface].footY=1;
    types[kWorkshop].flags1=0x800;types[kWorkshop].flags2=6;types[kWorkshop].footX=types[kWorkshop].footY=1;
    types[kObstacle].flags1=2;types[kObstacle].flags2=0;types[kObstacle].footX=types[kObstacle].footY=1;
    data.frames[kSurface].frames=o::RiftTypeFrames(std::vector<o::FrameCode>{{'P','P',1,0}});
    o::RawCanonPlacementPipeline pipeline(pool,hash,{types,data.frames,data.shapes,data.hotspots,spots,islands,regions},books,additional,contained,state);
    auto bytes=pool.Bytes().first(kSlots*pool.Layout().stride);std::span<std::uint8_t> raw{const_cast<std::uint8_t*>(bytes.data()),bytes.size()};
    // 자산 정의와 SHP는 모두 실제 로더 결과를 유지한다.
    for (const auto& name:kNames) {
        const auto type=static_cast<std::uint32_t>(o::kFirstAssetTypeNumber+assets.Find(name).block);const auto found=std::find(state.geometry.patternTypes.begin(),state.geometry.patternTypes.end(),type);
        const bool pattern=found!=state.geometry.patternTypes.end();const auto count=pattern ? kCounts[static_cast<std::size_t>(found-state.geometry.patternTypes.begin())] : 1U;
        // 실제 패턴 인자는 각 판본의 정상 범위만 순회한다.
        for (std::uint32_t argument=0;argument<count;++argument) {
            // CD 홀수 방향 assert를 피하고 원본의 네 정상 회전을 조회한다.
            for (std::uint32_t direction=0;direction<(pattern ? 8U : 1U);direction+=2) {
                // 정상 지면·다른 단계 충돌·지도 끝·외국 소유자의 네 장면을 검사한다.
                for (std::uint32_t profile=0;profile<4;++profile) {
                    std::fill(raw.begin(),raw.end(),std::uint8_t{});hash.Reset();std::fill(islands.begin(),islands.end(),std::uint16_t{});std::uint16_t sid=50;
                    // 합성 6×6 지면만 입력하며 finder/허용 결과는 실제 모듈이 결정한다.
                    for (int y=19;y<25;++y) {
                        // 현재 표면과 해시 0단계를 같은 저장소에 넣는다.
                        for (int x=18;x<24;++x) {
                            auto slot=raw.subspan(sid*pool.Layout().stride,pool.Layout().stride);slot[10]=kSurface;slot[patch ? 34 : 32]=profile==3 ? 2 : 1;slot[patch ? 30 : 28]=7;slot[patch ? 40 : 35]=1;
                            Put(slot,8,9,2);Put(slot,14,std::bit_cast<std::uint32_t>(static_cast<float>(x)));Put(slot,18,std::bit_cast<std::uint32_t>(static_cast<float>(y)));
                            hash.Bucket(0,static_cast<float>(x),static_cast<float>(y))=sid;islands[y*256+x]=sid;++sid;
                        }
                    }
                    auto work=raw.subspan(90*pool.Layout().stride,pool.Layout().stride);work[10]=kWorkshop;work[patch ? 34 : 32]=1;work[patch ? 30 : 28]=7;Put(work,14,std::bit_cast<std::uint32_t>(18.0F));Put(work,18,std::bit_cast<std::uint32_t>(20.0F));
                    if (profile==1) {
                        auto obstacle=raw.subspan(91*pool.Layout().stride,pool.Layout().stride);obstacle[10]=kObstacle;Put(obstacle,14,std::bit_cast<std::uint32_t>(20.0F));Put(obstacle,18,std::bit_cast<std::uint32_t>(21.0F));hash.Bucket(1,20,21)=91;
                    }
                    state.placement.localPlayer=profile==3 ? 2 : 1;state.placement.blockedRelation=73;state.permission.graphReady=1;state.permission.editor=profile==3 ? 1 : 0;state.permission.useAlliances=profile==3 ? 0 : 1;
                    state.permission.alliances.fill(0);state.permission.alliances[10]=state.permission.alliances[19]=profile==3 ? 0 : 1;state.relations.bypassGroundPermission=profile==3 ? 1 : 0;
                    state.preview.blocked.fill(0x55);state.terrain.regions.fill(0x55);
                    // 존재하는 지역의 제한 DWORD는 마지막 장면에서만 켠다.
                    for (auto& region:regions) region={profile==3 ? 1U : 0U,1};
                    // CD의 범위 밖 spot 읽기가 생기지 않는 지도 끝 안쪽에서 긴 다리도 검사한다.
                    const float x=profile==2 ? 250.0F : profile==1 ? 20.25F : 20.0F,y=profile==2 ? 250.0F : profile==1 ? 21.5F : 21.0F;
                    const auto actualArgument=pattern ? argument : type;const bool allowed=pipeline.MayPlace({type,actualArgument,x,y,direction,1,0});
                    std::printf("{\"type\":%u,\"argument\":%u,\"direction\":%u,\"profile\":%u,\"allowed\":%u,\"blocked\":%u,\"preview\":\"",type,actualArgument,direction,profile,allowed ? 1U : 0U,state.placement.blockedRelation);
                    Hex(state.preview.blocked);std::printf("\",\"regions\":\"");Hex(state.terrain.regions);std::printf("\"}\n");
                }
            }
        }
    }
}
}
