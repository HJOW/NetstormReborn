// 원본 x87 나눗셈/덧셈 뒤 float 저장과 현재 raw frame 폭을 보존한다.
#include "o/RawProjectileGeometry.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 현재 원본 little endian DWORD를 정렬 가정 없이 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value=0;
    // 해당 필드 네 바이트만 조합한다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);return value;
}
// 원본 Rect min/max는 들어오는 corner만1..255에 clamp한다.
std::array<float,2> Clamp(std::array<float,2> point) { return {std::clamp(point[0],1.0f,255.0f),std::clamp(point[1],1.0f,255.0f)}; }
}
// 실제 판본 전체 표/유한 양수 픽셀 배율을 확인한다.
RawProjectileGeometry::RawProjectileGeometry(const SidPool& pool,std::span<const RiftTypeRecord> types,
    std::span<const ProjectileGeometryType> geometry,float scaleX,float scaleY)
    :pool_(pool),types_(types),geometry_(geometry),scaleX_(scaleX),scaleY_(scaleY) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || geometry.size()!=types.size() ||
        !std::isfinite(scaleX) || !std::isfinite(scaleY) || scaleX<=0 || scaleY<=0) throw std::invalid_argument("Projectile geometry 판본/배율 오류");
}
// 유효 실제 자산만 허용하며 free/dead 여부는 호출자 몸체가 판단한다.
std::span<const std::uint8_t> RawProjectileGeometry::Slot(Sid sid) const {
    if (sid.value<5 || sid.value>=pool_.Capacity()) throw std::out_of_range("Projectile geometry SID 오류");
    const auto raw=pool_.Slot(sid);if (raw[10]<70 || raw[10]>=types_.size()) throw std::out_of_range("Projectile geometry 현재 TYPE 오류");return raw;
}
// 실제 포구 getter의 abstract 인자는 진단에만 쓰며 반환 물리 번호에 레이어를 더하지 않는다.
std::array<std::int16_t,2> RawProjectileGeometry::Offset(std::uint32_t type,std::int32_t frame,bool) const {
    std::int64_t physical=frame;
    if (physical<0 || static_cast<std::size_t>(physical)>=geometry_[type].shotOffsets.size()) throw std::out_of_range("Projectile 포구 SHP 물리 프레임 오류");
    return geometry_[type].shotOffsets[static_cast<std::size_t>(physical)];
}
// 두 SHP 헤더의 signed short를 먼저 정수로 더하며 타입 hotspot을 추가로 빼지 않는다.
std::array<float,2> RawProjectileGeometry::ArcherMuzzle(Sid firing) const {
    const auto raw=Slot(firing);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (raw[10]!=71) throw std::logic_error("Projectile 포구의 썬 디스크 발사기 외 파생 미복원");
    const bool abstract=(raw[patch ? 40U : 35U]&1)!=0;
    const auto first=Offset(raw[10],geometry_[raw[10]].defaultFrame,abstract);
    const auto current=Offset(raw[10],patch ? std::bit_cast<std::int32_t>(Read(raw,36)) : raw[34],abstract);
    const auto x=std::bit_cast<float>(Read(raw,14)),y=std::bit_cast<float>(Read(raw,18));
    return {static_cast<float>(static_cast<double>(first[0]+current[0])/scaleX_+x),
            static_cast<float>(static_cast<double>(first[1]+current[1])/scaleY_+y)};
}
// ctor의 TYPE 치수 helper는 확장 플래그0의 현재 footX/footY다.
std::array<std::int32_t,2> RawProjectileGeometry::Dimensions(Sid target) const { const auto& type=types_[Slot(target)[10]];return {type.footX,type.footY}; }
// 원본 full 크기를 빼고 입력 위치가 유효하지 않으면 들어온 corner로 바꾸는 Rect helper를 보존한다.
std::array<float,4> RawProjectileGeometry::TargetBounds(Sid target,std::array<float,2> position) const {
    const auto number=Slot(target)[10];const auto& type=types_[number];
    const auto low=Clamp({static_cast<float>(position[0]-type.footX),static_cast<float>(position[1]-(type.footY+geometry_[number].heightExtension))});
    // CD의 cmp DWORD,0은 float 비트의 signed 정수 비교다. 유한 양수에서는 PATCH의 float>0과 같다.
    const bool valid=position[0]>0 && position[1]>0 && position[0]<256 && position[1]<256;
    return valid ? std::array<float,4>{std::min(position[0],low[0]),std::min(position[1],low[1]),std::max(position[0],low[0]),std::max(position[1],low[1])} :
        std::array<float,4>{low[0],low[1],low[0],low[1]};
}
// 같은 Pool 계약의 읽기 전용 확인이다.
const SidPool& RawProjectileGeometry::Pool() const { return pool_; }
}
