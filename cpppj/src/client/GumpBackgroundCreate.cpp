// 배경 생성자의 현재 자산 읽기와 판본별 필드 쓰기 순서를 보존한다.
#include "client/GumpBackgroundCreate.h"
#include <stdexcept>

namespace netstorm::client {
namespace {
// PE 가상 표 주소는 직렬화 근거용이며 실행할 함수 포인터로 사용하지 않는다.
constexpr std::uint32_t kPatchBackgroundVtable=0x50ae40,kCdBackgroundVtable=0x5048e8;
}
// 호출 중 필요한 타입/프레임 경계만 요구한다. 기본 생성 노드의 공유 여부는 Construct에서 확인한다.
GumpBackgroundCreate::GumpBackgroundCreate(GumpCreateState& state,const GumpCreate& factory,
    const std::int8_t& color,GumpBackgroundCreateHooks hooks)
    :state_(state),factory_(factory),color_(color),hooks_(std::move(hooks)) {
    if (!hooks_.node) throw std::invalid_argument("배경 Gump 저장소 필요");
}
// 기존 수집 훅에서 얻는 기본 노드 주소와 파생 노드 주소를 비교한다.
GumpBackgroundNode& GumpBackgroundCreate::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.node(token) : nullptr;
    if (!node || factory_.CollectionHooks().node(token)!=static_cast<GumpCollectionNode*>(node))
        throw std::runtime_error("배경 Gump 토큰/공유 저장소 없음");
    return *node;
}
// 등록/serial 갱신을 복제하지 않는다. 외부 자산 조회 뒤에는 원본이 읽는 현재 필드를 사용한다.
std::uint32_t GumpBackgroundCreate::Construct(std::uint32_t token,float width,float height,
    std::uint32_t backgroundFlags,std::uint32_t flags,std::uint32_t type,std::uint32_t frame) const {
    auto& node=Node(token);factory_.Construct(token,0,width,height,flags);
    const bool patch=state_.collection.edition==o::OriginalEdition::Patch1078;
    node.vtable=patch ? kPatchBackgroundVtable : kCdBackgroundVtable;
    node.command=static_cast<std::uint32_t>(static_cast<std::int32_t>(color_));
    node.backgroundFlags=backgroundFlags;node.type=type;node.frame=frame;
    if (type) {
        if (!hooks_.type || !hooks_.frame) throw std::invalid_argument("배경 타입/SHP 프레임 경계 필요");
        const auto* asset=hooks_.type(type);
        if (!asset) throw std::runtime_error("배경 타입 저장소 없음");
        if (!patch) node.shape=asset->shape;
        const auto* header=hooks_.frame(asset->token,0,node.frame);
        if (!header) throw std::runtime_error("배경 SHP 프레임 헤더 없음");
        node.tileWidth=header->width;node.tileHeight=header->height;
        if (patch) node.shape=asset->shape;
    }
    return token;
}
}
