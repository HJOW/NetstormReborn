// TAFF·.type·SHP의 경로와 인덱스를 실제 자산으로 연결한다.
#include "client/GameAssets.h"
#include "o/OriginalText.h"
#include <limits>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 사용자 입력 이름을 원본 ASCII 식별자 규칙으로 비교한다.
bool Equal(std::string_view left, std::string_view right) {
    return o::AsciiLower(left) == o::AsciiLower(right);
}
}
// 초기 원본 setup.cfg의 공통 팔레트 gifcloud를 읽는다. Config 경로 치환 연결은 후속 작업이다.
GameAssets::GameAssets(const o::BaseFileSystem& files, o::OriginalEdition edition)
    : shapes_(files.Read("d/_shapes.shp")), palette_(files.Read("d/gifcloud.col")) {
    const auto order = o::TypeLoadOrder(edition);
    if (shapes_.Blocks().size() != order.size()) throw std::runtime_error("Shape block count does not match selected edition (use --cd for CD data)");
    types_.reserve(order.size());
    // 다른 판본의 추가 타입이나 디렉터리 정렬 순서를 섞지 않는다.
    for (std::size_t i = 0; i < order.size(); ++i) {
        const auto name = std::string(order[i]);
        auto type = o::RiftTypeDefinition::Parse(o::DecodeOriginalText(files.Read("d/"+name+".type")));
        types_.push_back({name, std::move(type), i});
    }
}
// 목록에서 빠진 세 가지 미사용 .type 파일은 원본과 같이 SHP 블록에 배정하지 않는다.
std::span<const TypeAsset> GameAssets::Types() const { return types_; }
// typename이 파일 이름과 다른 경우도 읽기 도구에서 쉽게 찾게 한다.
const TypeAsset& GameAssets::Find(std::string_view name) const {
    // 파일 이름을 먼저 찾고 동명의 표시 타입은 다음 단계에서 찾는다.
    for (const auto& type : types_) if (Equal(type.assetName, name)) return type;
    // 원본 typename은 파일 별명과 별도로 보존한다.
    for (const auto& type : types_) if (Equal(type.definition.name, name)) return type;
    throw std::runtime_error("Unknown game asset type: "+std::string(name));
}
// 레이어별 연속 프레임 번호를 유지한다. GIF 파일 번호로 다시 정렬하지 않는다.
std::size_t GameAssets::FrameIndex(const TypeAsset& type, std::string_view clusterName, std::size_t layer) const {
    const auto stride = type.definition.clusters.size();
    if (type.block >= shapes_.Blocks().size()) throw std::out_of_range("Shape type block index");
    if (stride == 0 || layer > (std::numeric_limits<std::size_t>::max()-stride)/stride) throw std::out_of_range("Shape layer index");
    // 중복 클러스터도 원본 프레임 검색처럼 첫 일치를 선택한다.
    for (std::size_t i = 0; i < stride; ++i) {
        if (Equal(type.definition.clusters[i].name, clusterName)) {
            const auto index = layer*stride+i;
            if (index >= shapes_.Blocks()[type.block].frames.size()) throw std::out_of_range("Shape frame index");
            return index;
        }
    }
    throw std::runtime_error("Unknown shape cluster: "+std::string(clusterName));
}
// SHP 레코드의 원래 경계·메타데이터를 제공한다.
const ShapeDatabase& GameAssets::Shapes() const { return shapes_; }
// 팔레트는 투명 마스크와 별도로 적용한다.
const GamePalette& GameAssets::Palette() const { return palette_; }
}
