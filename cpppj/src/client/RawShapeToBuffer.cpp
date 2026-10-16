// 실제 캡처의 plain/mapped/투명 fallback을 복원하며 미연결 추가 가상 그림을 거부한다.
#include "client/RawShapeToBuffer.h"
#include "client/VFXMappedDraw.h"
#include <bit>
#include <limits>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 원본 해제 가상 표와 타입 maxHP의 두 진단 표식이다.
constexpr std::uint32_t kFreedA=0xddddddddU,kFreedB=0xcdcdcdcdU;
// 현재 raw의 비정렬 DWORD를 호스트 정렬과 무관하게 읽는다.
std::uint32_t BufferDword(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value=0;
    // 원본 low byte부터 네 바이트를 조합한다.
    for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[offset+index])<<(index*8);
    return value;
}
}
// 현재 TYPE/SHP 번호는 내부Form을 제외한 자산70부터 같은 로딩 순서여야 한다.
RawShapeToBuffer::RawShapeToBuffer(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,
    std::span<const o::SquidDisplayShape> shapes,const ShapeDatabase& database,const ShapeToBufferState& state)
    :pool_(pool),types_(types),shapes_(shapes),database_(database),state_(state) {
    const auto count=pool.Edition()==o::OriginalEdition::Patch1078 ? 188U : 171U;
    if (types.size()!=count || shapes.size()!=count) throw std::invalid_argument("ShapeToBuffer TYPE/SHP 판본 크기 오류");
}
// 원본 fixed16의 곱셈 감김과 음수의 산술 시프트를 C++ signed 넘침 없이 계산한다.
std::int32_t RawShapeToBuffer::Scale(std::int32_t value) const {
    const auto product=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)*state_.zoom));
    return product>=0 ? product/65536 : static_cast<std::int32_t>(-((-static_cast<std::int64_t>(product)+65535)/65536));
}
// Sprite source의 현재 프레임은 그림자 레이어가 아닌 헤더/물리표의 실제 번호다.
o::ConstructionVisualImage RawShapeToBuffer::Capture(o::Sid sid) const {
    const auto raw=pool_.Slot(sid);const bool patch=pool_.Edition()==o::OriginalEdition::Patch1078;const auto type=raw[10];
    if (type<o::kFirstAssetTypeNumber || type>=types_.size()) throw std::out_of_range("ShapeToBuffer 현재 자산 타입 오류");
    const auto vtable=BufferDword(raw,0);const auto frame=patch ? std::bit_cast<std::int32_t>(BufferDword(raw,36)) : static_cast<std::int32_t>(raw[34]);const auto& shape=shapes_[type];
    const bool valid=!(raw[11]&1) && vtable && vtable!=kFreedA && vtable!=kFreedB && frame>=0 && frame<shape.frameCount && shape.loaded;
    if (!valid) {
        if (patch) throw std::logic_error("ShapeToBuffer 현재 source/frame 원본 진단");
        return {8,8,4,4,std::vector<std::uint8_t>(64,255)};
    }
    const auto& record=types_[type];
    if (patch && (std::bit_cast<std::uint32_t>(record.maxHitPoints)==kFreedA || std::bit_cast<std::uint32_t>(record.maxHitPoints)==kFreedB)) throw std::logic_error("ShapeToBuffer frameCheck 원본 진단");
    if (raw[patch ? 40U : 35U]&2) throw std::logic_error("ShapeToBuffer 추가 가상 그림 미연결");
    const auto owner=raw[patch ? 34U : 32U];std::span<const std::uint8_t> colors; // mapped일 때만 현재 실제 소유자 색표를 선택한다.
    if (owner) {
        if (type>=state_.ownerTypeMaps.size()) throw std::logic_error("ShapeToBuffer 현재 소유자 색표 미연결");
        if (state_.ownerTypeMaps[type]) {
            if (!state_.ownerColors) throw std::logic_error("ShapeToBuffer mapped 소유자 색 소비자 미연결");
            colors=state_.ownerColors(type,owner);if (colors.size()!=256) throw std::logic_error("ShapeToBuffer mapped 소유자 색표 크기 오류");
        }
    }
    if (static_cast<std::size_t>(frame)>=shape.frames.size()) throw std::out_of_range("ShapeToBuffer 실제 프레임 헤더 오류");
    const auto& header=shape.frames[static_cast<std::size_t>(frame)];o::ConstructionVisualImage image{Scale(header.width),Scale(header.height),Scale(header.hotspotX),Scale(header.hotspotY),{}};
    const auto size=static_cast<std::int64_t>(image.width)*image.height;
    if (image.width<0 || image.height<0 || size>std::numeric_limits<std::int32_t>::max()) throw std::out_of_range("ShapeToBuffer 버퍼 크기 오류");
    image.pixels.assign(static_cast<std::size_t>(size),255);
    // 캡처는 새 픽셀 버퍼 전체를 pane으로 사용하며 그리기는 메타 배율로 RLE를 다시 늘리지 않는다.
    if (colors.empty()) static_cast<void>(DrawShape(image.pixels,image.width,image.height,database_,type-o::kFirstAssetTypeNumber,static_cast<std::size_t>(frame),image.hotspotX,image.hotspotY,{0,0,image.width-1,image.height-1}));
    else static_cast<void>(DrawMappedShape(image.pixels,image.width,image.height,database_,type-o::kFirstAssetTypeNumber,static_cast<std::size_t>(frame),image.hotspotX,image.hotspotY,{0,0,image.width-1,image.height-1},std::span<const std::uint8_t,256>(colors.data(),256)));
    if (state_.addTypeHotspot) {
        image.hotspotX=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(image.hotspotX)+static_cast<std::uint32_t>(Scale(record.hotspotX)));
        image.hotspotY=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(image.hotspotY)+static_cast<std::uint32_t>(Scale(record.hotspotY)));
    }
    return image;
}
// 실제 Burning/Construction capture 어댑터가 같은 raw 월드인지 확인한다.
const o::SidPool& RawShapeToBuffer::Pool() const { return pool_; }
}
