// 프레임 코드/기본 프레임/타입 기준점과 실제 SHP 추가 헤더를 같은 번호로 연결한다.
#include "client/PriestPlacementAssets.h"
#include "client/SquidRenderer.h"

namespace netstorm::client {
// 내장 타입도 포함하여 모든 배치 조회가 원본 타입 번호를 그대로 사용하게 한다.
PriestPlacementAssets::PriestPlacementAssets(const GameAssets& assets)
    :frames(assets.TypeTable().Types().size()),hotspots(frames.size()),shapes(SquidRenderer::Shapes(assets,assets.Edition())) {
    const auto types=assets.TypeTable().Types();
    // 속성 변환은 타입 로더에서 한 번 수행하며 조회 때 발자국/화면 배율을 다시 곱하지 않는다.
    for (std::size_t i=0;i<types.size();++i) hotspots[i]={types[i].hotspotX,types[i].hotspotY};
    // 자산 번호 70부터 프레임 순서와 특수 기본 프레임을 복사한다.
    for (const auto& asset:assets.Types()) frames[o::kFirstAssetTypeNumber+asset.block]={asset.definition.FrameTable(),asset.definition.specialFrames.defaultFrame};
    // CanonDecoder가 패턴 몸체로 분기하는 원본 전역의 순서다. 몸체 복원 전에는 기존 검사가 거부한다.
    constexpr std::array<std::string_view,8> kPatterns{"puzzlePiece","bridge","island","noIsland","thunderCannon","rainCannon","windArcher","windBlocker"};
    // 이름은 typename 대신 실제 로딩 파일 이름으로 해석한다.
    for (std::size_t i=0;i<kPatterns.size();++i) geometry.patternTypes[i]=static_cast<std::uint32_t>(o::kFirstAssetTypeNumber+assets.Find(kPatterns[i]).block);
}
}
