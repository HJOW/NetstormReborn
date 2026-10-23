// 원문439de0의 두 전체 순회와 binary53 연산 순서를 raw 물리에 그대로 적용한다.
#include "o/RawColorSortPrimary.h"
#include <algorithm>
#include <bit>
#include <cstring>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 원문503608의 실제 binary64 정규화 값이다. 기존 ParticleGradient의 같은 상수를 재사용하는 근거다.
constexpr double kRgbUnit=std::bit_cast<double>(std::uint64_t{0x3f70101010101010});
}
// 실제 할당 초기값만 복사하며 이 단계에서 원문 생성자를 먼저 실행한 것으로 세지 않는다.
RawColorSortPrimary::RawColorSortPrimary(std::span<const std::uint8_t> allocationBytes) {
    if (allocationBytes.size()!=bytes_.size()) throw std::invalid_argument("ColorSort primary 초기0x3800BYTE 계약");
    std::copy(allocationBytes.begin(),allocationBytes.end(),bytes_.begin());
}
// 저장소의 주소는 객체 수명 전체에 걸쳐 바뀌지 않는다.
std::span<std::uint8_t,kColorSortPrimaryBytes> RawColorSortPrimary::Bytes() { return bytes_; }
// 현재 raw를 빌릴 뿐 typed double 포인터로 별칭하지 않는다.
std::span<const std::uint8_t,kColorSortPrimaryBytes> RawColorSortPrimary::Bytes() const { return bytes_; }
// host 표식을 읽고 raw 미작성padding을 평가하지 않는다.
bool RawColorSortPrimary::Constructed() const { return constructed_; }
// 같은 raw offset의8BYTE만 읽는다. payload 전체의 C++ struct 배치는 가정하지 않는다.
double RawColorSortPrimary::ReadDouble(std::size_t offset) const {
    double value; // 원문에 이미 저장된 binary64를 받는 지역 값이다.
    std::memcpy(&value,bytes_.data()+offset,sizeof(value));
    return value;
}
// Native store 이후의 중단을 보존하므로 callback 예외 때 앞선 BYTE를 되돌리지 않는다.
void RawColorSortPrimary::StoreDouble(std::size_t offset,double value,std::uint32_t& writes,const ColorSortPrimaryHooks& hooks) {
    std::memcpy(bytes_.data()+offset,&value,sizeof(value));++writes;
    if (hooks.checkpoint) hooks.checkpoint(writes,static_cast<std::uint32_t>(offset),8);
}
// 4BYTE 번호를 써도 다음4BYTE padding은 이전 할당 입력 그대로 남는다.
void RawColorSortPrimary::StoreIndex(std::size_t offset,std::uint32_t value,std::uint32_t& writes,const ColorSortPrimaryHooks& hooks) {
    std::memcpy(bytes_.data()+offset,&value,sizeof(value));++writes;
    if (hooks.checkpoint) hooks.checkpoint(writes,static_cast<std::uint32_t>(offset),4);
}
// first1024 RGBX는 원문 BYTE로 읽는다. 같은 storage의 alias를 먼저 사본으로 바꾸지 않는다.
void RawColorSortPrimary::Construct(std::span<const std::uint8_t> logicalRgbx,const ColorSortPrimaryHooks& hooks) {
    if (logicalRgbx.size()<kColorSortPrimaryColors*4) throw std::invalid_argument("ColorSort primary 논리RGBX1024BYTE 계약");
    constructed_=false;
    std::uint32_t writes=0; // 원문 raw store만 센다. stack/host 표식은 포함하지 않는다.
    if (hooks.checkpoint) hooks.checkpoint(0,0,0);
    for (std::size_t index=0;index<kColorSortPrimaryColors;++index) { // 원문 첫 전체 순회: R→G→번호→B의 실제 저장 순서다.
        const auto base=index*kColorSortPrimaryRecordBytes; // 현재56BYTE 기록의 시작이다.
        StoreDouble(base+24,static_cast<double>(logicalRgbx[index*4])*kRgbUnit,writes,hooks);
        StoreDouble(base+32,static_cast<double>(logicalRgbx[index*4+1])*kRgbUnit,writes,hooks);
        const auto blue=logicalRgbx[index*4+2]; // 원문은 번호를 쓰기 전에 이 BYTE를 캡처한다.
        StoreIndex(base+48,static_cast<std::uint32_t>(index),writes,hooks);
        StoreDouble(base+40,static_cast<double>(blue)*kRgbUnit,writes,hooks);
    }
    for (std::size_t index=0;index<kColorSortPrimaryColors;++index) { // 원문 두번째 전체 순회는 저장된 같은 R/G/B에서 H/L/S를 만든다.
        const auto base=index*kColorSortPrimaryRecordBytes; // 논리입력 BYTE를 다시 읽지 않는 현재 기록이다.
        const auto red=ReadDouble(base+24),green=ReadDouble(base+32),blue=ReadDouble(base+40); // 첫 순회에서 binary64로 저장한 실제 세 값이다.
        const auto maximum=std::max({red,green,blue}),minimum=std::min({red,green,blue}); // 동률도 원문과 같은 유한 양수/0 값이다.
        const auto sum=minimum+maximum; // 원문 local_18에 먼저 저장한 합이다.
        const auto lightness=sum*0.5; // 같은 primary+8에 저장할 double이며 외부에서 재계산하지 않는다.
        double saturation=0.0,hue=-1.0; // 회색의 원문 S=0/H=-1 초기값이다.
        if (maximum!=minimum) {
            const auto delta=maximum-minimum; // 원문 두 명도 분기와 hue가 함께 쓰는 값이다.
            const auto denominator=lightness<=0.5 ? sum : (2.0-maximum)-minimum; // 원문 high-lightness 뺄셈 순서를 보존한다.
            saturation=delta/denominator;
            if (red==maximum) hue=(green-blue)/delta;
            else if (green==maximum) hue=(blue-red)/delta+2.0;
            else hue=(red-green)/delta+4.0;
            hue=hue*60.0;
            if (hue<0.0) hue=hue+360.0;
        }
        StoreDouble(base,hue,writes,hooks);
        StoreDouble(base+8,lightness,writes,hooks);
        StoreDouble(base+16,saturation,writes,hooks);
    }
    constructed_=true;
}
// 같은 저장 명도를 읽는 소비자는 owner를 유지해야 하며 미완성 constructor를 성공처럼 읽지 않는다.
double RawColorSortPrimary::Lightness(std::size_t index) const {
    if (!constructed_) throw std::logic_error("ColorSort primary 생성 미완료");
    if (index>=kColorSortPrimaryColors) throw std::out_of_range("ColorSort primary 명도 번호");
    return ReadDouble(index*kColorSortPrimaryRecordBytes+8);
}
}
