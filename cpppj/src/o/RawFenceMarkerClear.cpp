// 후보를 미리 모으지 않고 삭제 효과 뒤의 실제 finder Next와 현재 태그를 읽는다.
#include "o/RawFenceMarkerClear.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr float kMarkerCeilBias=0.9999f; //0041d7a0이 x87에서 더한 뒤 절삭하는 원본float 상수다.
// 지정한 WORD/DWORD 폭만 little endian으로 읽는다.
std::uint32_t MarkerRead(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value{}; //현재 후보의 비정렬 기록값이다.
    for (std::size_t index=0;index<width;++index) value|=std::uint32_t(raw[offset+index])<<(index*8); //이웃 필드에 걸치지 않는다.
    return value;
}
// 보드 중심의 유효 float만 원본 CRT 절삭으로 바꾼다.
int MarkerInt(double value) { if (!std::isfinite(value) || value<-2147483648.0 || value>=2147483648.0) throw std::out_of_range("fencemark 검색 CRT 범위 오류");return static_cast<int>(value); }
}
// 다른 판본/월드의 가상 공간을 하나의 원본 처리기로 혼합하지 않는다.
RawFenceMarkerClear::RawFenceMarkerClear(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    const RawGunTarget& target,const std::uint32_t& markerType,std::function<void(Sid,std::uint32_t)> destroy)
    :pool_(pool),hash_(hash),types_(types),target_(target),markerType_(markerType),destroy_(std::move(destroy)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || &target.Pool()!=&pool)
        throw std::invalid_argument("fencemark 기존 표식 삭제 판본/TYPE/풀 불일치");
}
// 원본은 다른 끝점 중심을 먼저 읽고 Rect의 들어오는 corner만1..255로 clamp한다.
SquidSearchArea RawFenceMarkerClear::Bounds(Sid parent,Sid other) const {
    const auto second=target_.Center(other),first=target_.Center(parent);
    if (!std::isfinite(first[0]) || !std::isfinite(first[1]) || !std::isfinite(second[0]) || !std::isfinite(second[1])) throw std::out_of_range("fencemark 비유한 중심 좌표");
    const auto x=std::clamp(second[0],1.0f,255.0f),y=std::clamp(second[1],1.0f,255.0f);
    const bool valid=first[0]>0 && first[1]>0 && first[0]<256 && first[1]<256;
    const auto left=valid ? std::min(first[0],x) : x,top=valid ? std::min(first[1],y) : y;
    const auto right=valid ? std::max(first[0],x) : x,bottom=valid ? std::max(first[1],y) : y;
    return {MarkerInt(left),MarkerInt(top),MarkerInt(static_cast<double>(right)+kMarkerCeilBias),MarkerInt(static_cast<double>(bottom)+kMarkerCeilBias)};
}
// 실제flags1은 표면을 건너뛰되 삭제되는 항목의 next는 같은 finder가 미리 보관한다.
void RawFenceMarkerClear::Clear(Sid parent,Sid other) const {
    RawSquidFinder finder(pool_,hash_,types_);
    for (auto sid=finder.Begin(Bounds(parent,other),1);sid.value;sid=finder.Next()) { //모든 후보를 현재 순서대로 읽고 효과 뒤의 체인/태그 변경을 반영한다.
        const auto raw=pool_.Slot(sid);if (raw[10]!=markerType_) continue;
        if (MarkerRead(raw,12,2)!=parent.value && ((MarkerRead(raw,26)<<8)|raw[30])!=parent.value) continue;
        if (!destroy_) throw std::logic_error("fencemark 기존 표식의 실제 가상 삭제 미연결");
        destroy_(sid,0x40);
    }
}
}
