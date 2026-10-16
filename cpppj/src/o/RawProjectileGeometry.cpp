// 원본 x87 나눗셈/덧셈 뒤 float 저장과 현재 raw frame 폭을 보존한다.
#include "o/RawProjectileGeometry.h"
#include "o/RawGunMode.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 실제10.78 방향 helper의 근사 원주/직각/역원주 double 비트다.
constexpr double kTwoPi=std::bit_cast<double>(std::uint64_t{0x401921f9f01b866e}),
    kHalfPi=std::bit_cast<double>(std::uint64_t{0x3ff921f9f01b866e}),
    kInverseTwoPi=std::bit_cast<double>(std::uint64_t{0x3fc45f318e7adaf5});
constexpr std::array<std::int32_t,4> kSunCannonFrameDirections{4,0,6,2}; // 발사 가능 프레임0..3과24..27의 실제 cardinal 방향이다.
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
    std::span<const ProjectileGeometryType> geometry,float scaleX,float scaleY,const RawGunMode* gunMode)
    :pool_(pool),types_(types),geometry_(geometry),scaleX_(scaleX),scaleY_(scaleY),gunMode_(gunMode) {
    if (types.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || geometry.size()!=types.size() ||
        !std::isfinite(scaleX) || !std::isfinite(scaleY) || scaleX<=0 || scaleY<=0) throw std::invalid_argument("Projectile geometry 판본/배율 오류");
    if (gunMode_ && &gunMode_->Pool()!=&pool_) throw std::invalid_argument("Projectile geometry/방향 getter 풀 불일치");
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
// 실제 VT50b888/50b6e8/50b9b8의+b4는 각각46a780/41c110/41c110으로 포구를 반환한다.
std::array<float,2> RawProjectileGeometry::Muzzle(Sid firing) const {
    const auto raw=Slot(firing);
    if (raw[10]==71) return ArcherMuzzle(firing);
    // PATCH TYPE128/131의 +0xb4 getter(0041c110)는 원시 x/y 좌표를 그대로 복사한다.
    if (pool_.Edition()==OriginalEdition::Patch1078 && (raw[10]==128 || raw[10]==131))
        return {std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))};
    if (pool_.Edition()==OriginalEdition::Patch1078 && raw[10]==140)
        return {std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))};
    throw std::logic_error("Projectile 실제 가상 포구 미복원 TYPE"+std::to_string(raw[10]));
}
// 실제00481750의 썬/썬더 분기다. 방향 불일치는 난수 없이 TYPE 사정거리의 정수 변환으로 보정한다.
std::array<float,2> RawProjectileGeometry::CannonRayEnd(std::array<float,2> from,std::array<float,2> to,Sid firing) const {
    const auto raw=Slot(firing);
    if (pool_.Edition()!=OriginalEdition::Patch1078 || (raw[10]!=128 && raw[10]!=131)) throw std::logic_error("Cannon rayEnd 미복원 판본/TYPE");
    if (!std::isfinite(from[0]) || !std::isfinite(from[1]) || !std::isfinite(to[0]) || !std::isfinite(to[1])) throw std::invalid_argument("Cannon rayEnd 비유한 좌표");
    const float dx=static_cast<float>(static_cast<double>(to[0])-from[0]),dy=static_cast<float>(static_cast<double>(to[1])-from[1]);
    const std::int32_t direction=std::abs(dx)<=std::abs(dy) ? (dy>0 ? 4 : 0) : (dx<=0 ? 6 : 2);
    const auto frame=std::bit_cast<std::int32_t>(Read(raw,36));
    std::int32_t frameDirection=-1;
    if (raw[10]==131) {
        if (!gunMode_) throw std::logic_error("썬더 사정선의 실제 방향 getter 미연결");
        frameDirection=static_cast<std::int32_t>(gunMode_->Get(firing));
    } else if (frame>=0 && frame<4) frameDirection=kSunCannonFrameDirections[static_cast<std::size_t>(frame)];
    else if (frame>=24 && frame<28) frameDirection=kSunCannonFrameDirections[static_cast<std::size_t>(frame-24)];
    else throw std::logic_error("Cannon rayEnd 미복원 물리 프레임"+std::to_string(frame));
    if (frameDirection!=0 && frameDirection!=2 && frameDirection!=4 && frameDirection!=6) throw std::logic_error("캐논 사정선의 원본 cardinal 방향 진단");
    if (frameDirection!=direction) {
        const auto range=geometry_[raw[10]].range;
        if (!std::isfinite(range) || range<-2147483648.0 || range>=2147483648.0)
            throw std::out_of_range("Cannon rayEnd 사정거리 정수 변환 범위 오류");
        const auto distance=static_cast<std::int32_t>(range);
        // 원본 imul의 하위 DWORD를 보존해 INT_MIN의 서/북 방향 부호 반전도 정의된 연산으로 처리한다.
        const auto backwards=std::bit_cast<std::int32_t>(0U-static_cast<std::uint32_t>(distance));
        const auto offsetX=frameDirection==2 ? distance : frameDirection==6 ? backwards : 0;
        const auto offsetY=frameDirection==4 ? distance : frameDirection==0 ? backwards : 0;
        to=Clamp({static_cast<float>(static_cast<double>(offsetX)+from[0]),static_cast<float>(static_cast<double>(offsetY)+from[1])});
    }
    if (frameDirection==0 || frameDirection==4) return {from[0],to[1]};
    return {to[0],from[1]};
}
// 가상 포구 계산 뒤 Missile ctor가 별도로 요구하는 현재 SHP 기준점이다.
std::array<std::int32_t,2> RawProjectileGeometry::Hotspot(Sid firing) const {
    const auto raw=Slot(firing);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    const auto offset=Offset(raw[10],patch ? std::bit_cast<std::int32_t>(Read(raw,36)) : raw[34],(raw[patch ? 40U : 35U]&1)!=0);
    return {offset[0],offset[1]};
}
// 20칸 비균등 발사기 회전표와 달리 이 helper는 표시 타입의 균등 방향을 사용한다.
std::int32_t RawProjectileGeometry::DirectionFrame(std::array<float,2> from,std::array<float,2> to,std::uint32_t type) const {
    if (pool_.Edition()!=OriginalEdition::Patch1078 || type>=geometry_.size()) throw std::out_of_range("Projectile 방향 helper 판본/TYPE 오류");
    const auto& data=geometry_[type];
    if (data.directions<=0 || data.frameCount/data.directions<=0) throw std::logic_error("Projectile 방향/물리 프레임 자료 누락");
    if (!std::isfinite(from[0]) || !std::isfinite(from[1]) || !std::isfinite(to[0]) || !std::isfinite(to[1])) throw std::invalid_argument("Projectile 비유한 방향 좌표");
    double angle=std::atan2(-(static_cast<double>(to[1])-from[1]),static_cast<double>(to[0])-from[0])-kHalfPi;
    if (angle<0) angle+=kTwoPi;
    const float scaled=static_cast<float>((kTwoPi-angle)*data.directions*kInverseTwoPi);
    const auto direction=static_cast<std::int32_t>(static_cast<double>(scaled)+0.5);
    return (direction%data.directions)*(data.frameCount/data.directions);
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
