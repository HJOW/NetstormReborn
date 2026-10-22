// 원본의 float 저장 경계마다 먼저 반올림하고 표준 pi나 VFX 높이로 치환하지 않는다.
#include "o/RawWindCircle.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
constexpr double kPi=3.141593; // 원본502328의 double이다.
constexpr float kDegreeScale=0.0055555556900799274F; // 원본502320의 float이다.
constexpr float kFlyerOffset=1.399999976158142F,kHeightScale=0.09090909361839294F; // 원본50a380/504194의 두 float 보정이다.
// 현재 비정렬 원문 DWORD를 지정 위치에서 조합한다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at) {
    std::uint32_t result{};for (std::size_t i=0;i<4;++i) result|=std::uint32_t(raw[at+i])<<(8*i); // 인접 BYTE 폭을 섞지 않는다.
    return result;
}
// 큰 효과 뒤 현재 종류의 발자국을 다시 조회한다.
const RiftTypeRecord& Type(const SidPool& pool,std::span<const RiftTypeRecord> types,Sid sid) {
    const auto kind=pool.Slot(sid)[10];if (kind>=types.size()) throw std::out_of_range("Wind 원주 현재 TYPE 범위");return types[kind];
}
}
std::array<float,2> WindCirclePoint(float x,float y,std::int32_t count,std::int32_t index) {
    if (!count) throw std::invalid_argument("Wind 원주 개수0 나눗셈");
    const auto next=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(index)+1U); // 원본 add의 하위DWORD 감김이다.
    const auto angle=static_cast<double>(360/count)*static_cast<double>(next)*kPi*static_cast<double>(kDegreeScale);
    return {static_cast<float>(std::cos(angle)*static_cast<double>(count)+x),static_cast<float>(std::sin(angle)*static_cast<double>(count)+y)};
}
std::array<float,2> WindCirclePosition(const SidPool& pool,std::span<const RiftTypeRecord> types,Sid parent,Sid flyer,const WindAviaryProcessFields& fields,const WindCircleHeaderRead& readHeader) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || !readHeader) throw std::invalid_argument("Wind 원주 PATCH/188TYPE/현재 헤더 필요");
    const auto header=readHeader(flyer,pool.Slot(flyer)[40]&1U,13); // owner 콜백 뒤 현재 생성 비행체를 조회한다.
    const auto& type=Type(pool,types,parent);const auto raw=pool.Slot(parent);
    const auto x=static_cast<float>(static_cast<double>(std::bit_cast<float>(Read(raw,14)))-static_cast<double>(type.footX)*0.5);
    const auto y=static_cast<float>(static_cast<double>(std::bit_cast<float>(Read(raw,18)))-static_cast<double>(type.footY)*0.5);
    const auto centerX=static_cast<float>(static_cast<double>(x)+1.0),centerY=static_cast<float>(static_cast<double>(y)+static_cast<float>(int(header.height)-int(header.hotspotY)+1));
    const auto point=WindCirclePoint(centerX,centerY,std::bit_cast<std::int32_t>(Read(fields.bytes,68)),std::bit_cast<std::int32_t>(Read(fields.bytes,72)));
    return {static_cast<float>(static_cast<double>(point[0])-kFlyerOffset),static_cast<float>(static_cast<double>(header.walkStepY)*kHeightScale-static_cast<double>(Type(pool,types,parent).footY)+point[1])};
}
}
