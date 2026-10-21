// 원본 경계 절삭·행 재개·시간 equality/unordered와거리 저장/효과 순서를 유지한다.
#include "o/RawAiBuildSearch.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr double kWorkBudget=static_cast<double>(0.03F); //PE00501230의정확한double값0.029999999329447746이다.
constexpr std::uint32_t kUnset=0xc61c4000U; //0040f660이쓰는미정x/y -10000.0f 비트다.
// 비정렬 원문 DWORD를 물리 범위 안에서 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at) { if (at>raw.size() || raw.size()-at<4) throw std::out_of_range("AI 배치 탐색 DWORD 범위");std::uint32_t value{};for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[at+index])<<(index*8);return value; } //반복문은 지정 네BYTE만 조합한다.
// 지정 DWORD만 바꾸고 주변/미초기화 상태를 보존한다.
void Put(std::span<std::uint8_t> raw,std::size_t at,std::uint32_t value) { if (at>raw.size() || raw.size()-at<4) throw std::out_of_range("AI 배치 탐색 쓰기 범위");for (std::size_t index=0;index<4;++index) raw[at+index]=static_cast<std::uint8_t>(value>>(index*8)); } //반복문은 네BYTE만 쓴다.
// 원본 signed DWORD의 비트 해석이다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// 원본 DWORD 덧셈/뺄셈의 감김을 signed 값으로 보존한다.
std::int32_t Add(std::int32_t first,std::uint32_t second) { return Signed(static_cast<std::uint32_t>(first)+second); }
// 원본ftol의 signed64 절삭 하위DWORD이며 무효 FISTP의 하위값은0이다.
std::uint32_t Ftol(double value) { if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;return static_cast<std::uint32_t>(static_cast<std::int64_t>(std::trunc(value))); }
// 선택되지 않은 하위는 요구하지 않고 선택된 누락만 명시 오류로 처리한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("AI 배치 탐색 실제 하위 미연결");return function; }
}
// 생성은 시계나 배치 검사를 실행하지 않는다.
RawAiBuildSearch::RawAiBuildSearch(RawAiBrainMemory& memory,std::span<const RiftTypeRecord> types,std::span<const std::uint8_t> spots,std::uint32_t typeBase,AiBuildSearchHooks hooks):memory_(memory),types_(types),spots_(spots),typeBase_(typeBase),hooks_(std::move(hooks)) { if (types.size()!=188 || spots.size()!=65536 || !typeBase || typeBase>std::numeric_limits<std::uint32_t>::max()-187U*kTypeStride) throw std::invalid_argument("AI 배치 탐색 실제188TYPE/65536spot/TYPE핸들 누락"); }
// 해제/다른 크기 본체를 거부한다.
std::span<std::uint8_t> RawAiBuildSearch::Body(std::uint32_t token) const { auto raw=memory_.Bytes(token);if (raw.size()!=RawAiBrainConstructor::kBytes) throw std::invalid_argument("AI 배치 탐색 실제0xb08본체 누락");return raw; }
// 원본 상대 필드를 현재 본체에서 읽는다.
std::uint32_t RawAiBuildSearch::Field(std::uint32_t token,std::size_t at) const { return Read(Body(token),kState+at); }
// 해당 상대 필드만 바꾼다.
void RawAiBuildSearch::Field(std::uint32_t token,std::size_t at,std::uint32_t value) const { Put(Body(token),kState+at,value); }
// TYPE를 별도 사본으로 캐시하지 않는다.
const RiftTypeRecord& RawAiBuildSearch::Type(std::uint32_t type) const { if (type>=types_.size()) throw std::out_of_range("AI 배치 탐색 현재TYPE 범위");return types_[type]; }
// 실제 표와500stride의 안정된 opaque32 대응이다.
std::uint32_t RawAiBuildSearch::Handle(std::uint32_t type) const { static_cast<void>(Type(type));return typeBase_+type*kTypeStride; }
// 손상된 캐시를 원래TYPE나 다른 기본값으로 보정하지 않는다.
std::uint32_t RawAiBuildSearch::TypeFromHandle(std::uint32_t pointer) const { if (pointer<typeBase_ || (pointer-typeBase_)%kTypeStride) throw std::out_of_range("AI 배치 탐색 cached TYPE핸들 범위");const auto type=(pointer-typeBase_)/kTypeStride;static_cast<void>(Type(type));return type; }
// 원본 지도 좌표를 clamp하지 않는다.
std::uint8_t RawAiBuildSearch::Spot(std::int32_t x,std::int32_t y) const { if (x<0 || y<0 || x>=256 || y>=256) throw std::out_of_range("AI 배치 탐색 실제spot 주소 범위");return spots_[static_cast<std::size_t>(y)*256+static_cast<std::size_t>(x)]; }
// 원본 여덟 인자와 두 TYPE조회/상태쓰기 순서다.
void RawAiBuildSearch::Initialize(std::uint32_t token,std::uint32_t type,AiBuildSearchBounds bounds,RawPathPoint avoid,std::uint32_t owner) const {
    Field(token,0,type);Field(token,0x18,Handle(type));Field(token,0x10,std::bit_cast<std::uint32_t>(avoid.x));Field(token,0x14,std::bit_cast<std::uint32_t>(avoid.y));Field(token,0x1c,owner);Field(token,4,0);Field(token,0xc,kUnset);Field(token,8,kUnset);
    const auto& current=Type(Field(token,0));const auto width=current.footX,height=current.footY;Field(token,0x20,Ftol(static_cast<double>(width)+bounds.left));const auto top=Ftol(static_cast<double>(height)+bounds.top);Field(token,0x24,top);Field(token,0x28,Ftol(bounds.right));const auto bottom=Ftol(bounds.bottom);Field(token,0x30,top);Field(token,0x2c,bottom);
}
// 원본은 x를바깥쪽에두며0이하footprint는칸에접근하지않고성공할수있다.
bool RawAiBuildSearch::FreeFootprint(std::uint32_t type,std::int32_t x,std::int32_t y) const {
    const auto& current=Type(type);auto column=Add(x,0U-static_cast<std::uint32_t>(current.footX));const auto firstRow=Add(Add(y,0U-static_cast<std::uint32_t>(current.footY)),1U);
    // 각 x 열에서 y 시작을 다시 사용하며 최초 실패 칸에서 즉시 종료한다.
    for (;;) { column=Add(column,1U);if (column>x) return true;if (firstRow>y) continue;auto row=firstRow;do { if ((Spot(column,row)&0x3fU)!=2U) return false;row=Add(row,1U); }while (row<=y); } //안쪽반복문은원본inclusive y footprint를순회한다.
}
// 각 호출의 deadline과 행 시작 clock만 사용하며 한 행은 예산을 넘어도 끝까지 처리한다.
void RawAiBuildSearch::Scan(std::uint32_t token,std::uint32_t budgeted) const {
    if (!Field(token,0)) return;const double deadline=Need(hooks_.wall)()+kWorkBudget;if (Signed(Field(token,0x30))>=Signed(Field(token,0x2c))) return;
    // 원본은행마다현재cursor/maxY를재조회하고중단시미처리행cursor를보존한다.
    do {
        if (budgeted && !(Need(hooks_.wall)()<=deadline)) return;auto x=Signed(Field(token,0x20));const auto y=Signed(Field(token,0x30));
        // x 시작과y는이행snapshot이며maxX만각후보효과뒤현재값으로다시읽는다.
        while (x<Signed(Field(token,0x28))) {
            if ((Spot(x,y)&0x3fU)==2U && FreeFootprint(Field(token,0),x,y)) {
                const float pointX=static_cast<float>(x),pointY=static_cast<float>(y);const auto avoidX=std::bit_cast<float>(Field(token,0x10)),avoidY=std::bit_cast<float>(Field(token,0x14));const float dx=static_cast<float>(static_cast<double>(x)-avoidX);const double dy=static_cast<double>(y)-avoidY,distance=static_cast<double>(dx)*dx+dy*dy;const float saved=static_cast<float>(distance);
                if (distance>std::bit_cast<float>(Field(token,4))) { const CanonPlacementQuery query{TypeFromHandle(Field(token,0x18)),0,pointX,pointY,0,Field(token,0x1c)&0xffU,1};if (Need(hooks_.mayPlace)(query)) { Field(token,4,std::bit_cast<std::uint32_t>(saved));Field(token,8,std::bit_cast<std::uint32_t>(pointX));Field(token,0xc,std::bit_cast<std::uint32_t>(pointY)); } }
            }
            x=Add(x,1U);
        }
        Field(token,0x30,Field(token,0x30)+1U);
    }while (Signed(Field(token,0x30))<Signed(Field(token,0x2c)));
}
}
