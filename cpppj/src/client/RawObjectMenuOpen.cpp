// 공통 Open의 조회 시점과 원본 위치 계산의32비트 감김·부호 비교를 보존한다.
#include "client/RawObjectMenuOpen.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 미연결 제목/가상 메뉴/부착을 빈 성공으로 처리하지 않는다.
template<class F> const F& Required(const F& hook) {
    if (!hook) throw std::logic_error("공통 객체 메뉴 실제 하위 미연결");return hook;
}
// DWORD의 비트를 원본 signed 비교용 값으로 바꾼다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
}
// 현재 구현은 보호10.78의 실제 슬롯 배치와188개 TYPE만 받는다.
RawObjectMenuOpen::RawObjectMenuOpen(const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,ObjectMenuOpenHooks hooks)
    :pool_(pool),types_(types),hooks_(std::move(hooks)) {
    if (pool.Edition()!=o::OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("공통 객체 메뉴10.78 풀/TYPE 오류");
}
// 부모 함수에는 free/dead 검사가 없으므로 host의 물리 범위만 검증한다.
std::span<const std::uint8_t> RawObjectMenuOpen::Raw(o::Sid object) const {
    if (!object.value || object.value>=pool_.Capacity()) throw std::out_of_range("공통 객체 메뉴 SID 범위 오류");return pool_.Slot(object);
}
// 재조회한 현재 객체/내용물의 TYPE 번호만 사용한다.
const o::RiftTypeRecord& RawObjectMenuOpen::Type(std::uint32_t number) const {
    if (number>=types_.size()) throw std::out_of_range("공통 객체 메뉴 TYPE 범위 오류");return types_[number];
}
// 정렬되지 않은 슬롯 좌표와 contained kind를 포인터 캐스트 없이 읽는다.
std::uint32_t RawObjectMenuOpen::Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width) {
    std::uint32_t value=0;
    // 필요한 바이트만 원본의 낮은 바이트부터 조립한다.
    for (std::size_t index=0;index<width;++index) value|=std::uint32_t(raw[offset+index])<<(index*8);
    return value;
}
// 유한 signed64 범위만 호스트 정수로 변환해 부정확한 signed32 직접 캐스트를 피한다.
std::uint32_t RawObjectMenuOpen::Truncate(float value) {
    const double wide=value;if (!std::isfinite(wide) || wide<-0x1p63 || wide>=0x1p63) return 0;
    return static_cast<std::uint32_t>(static_cast<std::int64_t>(wide));
}
// 실제+a8 뒤 TYPE가 바뀌면 마지막 위치 분기도 바뀐다. extra는 맨 처음 한 번만 검사한다.
std::uint32_t RawObjectMenuOpen::Open(o::Sid object,const InputEvent& event) const {
    if (Raw(object)[40]&1) return 0;
    if (Type(Raw(object)[10]).flags2&0x400000U) {
        const auto altar=Required(hooks_.altar)(object);
        if (altar.value && altar.value<pool_.Capacity()) return Required(hooks_.delegate)(altar,event);
    }
    Required(hooks_.beginFancy)();Required(hooks_.summary)(object);Required(hooks_.populate)(object);
    if (Type(Raw(object)[10]).flags2&0x30100U) Position(object);
    else Required(hooks_.attach)(object,event.x,event.y);
    return 1;
}
// 음수와INT_MIN의 절댓값도DWORD로 유지하여 signed overflow 없이 실제 명령 결과를 재현한다.
void RawObjectMenuOpen::Position(o::Sid object) const {
    const auto raw=Raw(object);const auto point=Required(hooks_.project)(std::bit_cast<float>(Read(raw,14,4)),std::bit_cast<float>(Read(raw,18,4)));
    auto x=std::bit_cast<std::uint32_t>(point[0]);const auto form=Required(hooks_.content)(object);
    if (form.value) {
        const auto kind=Read(Raw(form),18,2);Type(kind);
        auto distance=Truncate(Required(hooks_.range)(kind));if (Signed(distance)>=2) distance=2;
        const auto padding=(distance+1U)<<4;const auto width=Truncate(Required(hooks_.menuWidth)());
        auto right=width-std::bit_cast<std::uint32_t>(Required(hooks_.screenWidth)())+padding+x;
        if (Signed(right)<0) right=0;
        auto left=x-width-padding;const auto leftPosition=left;
        if (Signed(left)>0) left=0;
        if (Signed(right)<0) right=0U-right;
        if (Signed(left)<0) left=0U-left;
        x=Signed(right)<=Signed(left) ? x+padding : leftPosition;
    }
    Required(hooks_.attach)(object,Signed(x),point[1]);
}
}
