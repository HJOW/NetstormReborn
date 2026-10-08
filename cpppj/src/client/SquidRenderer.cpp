// o/에서는 client 헤더를 포함하지 않도록 표시 대상과 자산 변환을 클라이언트에 둔다.
#include "client/SquidRenderer.h"
#include <limits>
#include <stdexcept>

namespace netstorm::client {
// 변경 표의 소유권은 기존 Renderer에 둔다.
SquidRenderer::SquidRenderer(Renderer& renderer):renderer_(renderer) {}
// 원본 displayOff 전역과 같은 전체 변경 상태다.
bool SquidRenderer::Suppressed() const { return renderer_.FullRedrawPending(); }
// 범위를 다시 viewport로 자르면 선택 표식 확장을 잃으므로 그대로 전달한다.
void SquidRenderer::Invalidate(o::SquidDisplayRect rect,std::uint32_t flags) {
    renderer_.Invalidate({rect.left,rect.top,rect.right,rect.bottom},flags);
}
// 실제 SHP의 표시 헤더를 VFX 압축 픽셀 영역과 구분한다.
std::vector<o::SquidDisplayShape> SquidRenderer::Shapes(const GameAssets& assets,o::OriginalEdition edition) {
    if (assets.Types().size()!=o::TypeLoadOrder(edition).size()) throw std::invalid_argument("표시 자산 판본 불일치");
    std::vector<o::SquidDisplayShape> shapes(assets.TypeTable().Types().size());
    // 번호 70부터 원본 타입 로딩 순서와 같은 블록을 연결한다.
    for (std::size_t i=0;i<assets.Types().size();++i) {
        const auto& asset=assets.Types()[i]; auto& shape=shapes.at(o::kFirstAssetTypeNumber+i);
        if (asset.definition.clusters.size()>static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw std::out_of_range("표시 클러스터 수 범위 오류");
        shape.frameCount=static_cast<int>(asset.definition.clusters.size()); shape.loaded=true;
        // 그림자/abstract 레이어와 공유 프레임도 물리 테이블 순서를 보존한다.
        for (std::size_t frame=0;frame<assets.Shapes().Blocks()[asset.block].frames.size();++frame) {
            const auto m=assets.Shapes().SquidMetrics(asset.block,frame);
            shape.frames.push_back({m.width,m.height,m.hotspotX,m.hotspotY,m.cellWidth,m.cellHeight});
        }
    }
    return shapes;
}
}
