// x폭의 float 저장과 y높이의 x87 double 유지 및 원문의 재조회 순서를 보존한다.
#include "o/RawWorldGeyserBounds.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o { // 원본 세계 생성 전용이며 별도 가상 지도/예산을 소유하지 않는다.
namespace { // 원문 폭 변환과 선택된 경계 검사를 모은 내부 도구다.
constexpr std::size_t kPlayerStride=0xb4,kPresence=0x2c,kHidden=63; // 실제9 Player 및 옵션87BYTE의 필드 위치다.
constexpr std::int32_t kMaximumHostGrid=4096; // 게임은3을 쓰며 손상된 입력의 수십억 반복은 호스트 오류로 구별한다.
// DWORD의 원본 감긴 비트를 signed 값으로 해석한다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// 비정렬 Player 필드의 정확한 네 BYTE만 읽는다.
std::uint32_t Word(std::span<const std::uint8_t> bytes,std::size_t offset) {
    std::uint32_t result{};
    for (std::size_t byte=0;byte<4;++byte) result|=std::uint32_t(bytes[offset+byte])<<(byte*8); // 낮은 BYTE부터 원문 DWORD를 조합한다.
    return result;
}
// 원본 CRT는 signed64 절삭의 하위 DWORD를 반환한다. 불확정값의 하위 DWORD는0이다.
std::int32_t Truncate(double value) {
    if (!std::isfinite(value) || value< -9223372036854775808.0 || value>=9223372036854775808.0) return 0;
    return Signed(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
// 실제 NEG의 INT_MIN 감김도 보존한다.
std::int32_t Absolute(std::int32_t value) { return value<0 ? Signed(0U-static_cast<std::uint32_t>(value)) : value; }
// 실제 도달한 하위만 요구하며 앞선 공유 쓰기와 청크 조회를 되돌리지 않는다.
template<class Function> const Function& Need(const Function& function) {
    if (!function) throw std::logic_error("전체 간헐천 범위의 실제 하위 미연결");return function;
}
}
// 원본의 스택 표는 BYTE0~3일 때만 정의된 네 항목이다.
std::int32_t WorldGeyserSpacing(std::uint8_t selection) {
    constexpr std::array<std::int32_t,4> values{3,4,5,6}; //0041ca10의 실제 네 DWORD다.
    if (selection>=values.size()) throw std::out_of_range("세계 간헐천 간격의 원문 표 범위");return values[selection];
}
// flags50/지역/AI를 세는 다른 Player 관문과 합치지 않는다.
std::int32_t WorldGeyserBudget(OriginalEdition edition,bool battle,std::span<const std::uint8_t> players,std::span<const std::uint8_t> options) {
    if (edition!=OriginalEdition::Patch1078 || players.size()!=9*kPlayerStride || options.size()!=87) throw std::invalid_argument("세계 간헐천 예산의 PATCH Player/옵션 물리 범위");
    std::int32_t count{};
    if (battle) for (std::size_t owner=1;owner<=8;++owner) { // 실제 Player1~8의 현재+2c를 먼저 읽고 필요한 숨김 BYTE만 검사한다.
        if (Word(players,owner*kPlayerStride+kPresence) && options[kHidden+owner-1]==0) ++count;
    }
    return (12-count)*count;
}
// edition만 생성 시 고정하며 실제 하위는 그 분기에 도달했을 때 요구한다.
RawWorldGeyserBounds::RawWorldGeyserBounds(OriginalEdition edition,WorldGeyserBoundsState state,WorldGeyserBoundsHooks hooks):state_(state),hooks_(std::move(hooks)) {
    if (edition!=OriginalEdition::Patch1078) throw std::invalid_argument("전체 간헐천 범위는10.78 전용");
}
// ChunkMap의 실제 결과를 clamp하지 않고 DWORD16배/단정도 저장한다.
FortRandomRectangle RawWorldGeyserBounds::Cells(std::int32_t region) const {
    const auto raw=Need(hooks_.chunkBounds)(region);
    const auto convert=[](std::int32_t value) { return static_cast<float>(Signed(static_cast<std::uint32_t>(value)<<4)); }; //0040d900의 각 좌표 변환이다.
    return {convert(raw.left),convert(raw.top),convert(raw.right),convert(raw.bottom)};
}
// 공유 여백 쓰기는 실제 getter 뒤이며 좌표의 수정보다 먼저 발생한다.
std::int32_t RawWorldGeyserBounds::Padding() const {
    state_.spacingMargin=Signed(static_cast<std::uint32_t>(Need(hooks_.spacing)())-2U);
    return Signed(static_cast<std::uint32_t>(state_.spacingMargin)<<4);
}
// 입력점의 clamp는 원래1~255이고 현재 유효성 상한00531928과 별개다.
void RawWorldGeyserBounds::Extend(FortRandomRectangle& rectangle,float x,float y) const {
    x=std::clamp(x,1.0F,255.0F);y=std::clamp(y,1.0F,255.0F);
    const auto valid=[this](float px,float py) { const float bound=static_cast<float>(state_.mapLimit);return px>0.0F && px<bound && py>0.0F && py<bound; }; //0040e800의 두 축 열린 범위다.
    if (!valid(rectangle.left,rectangle.top)) { rectangle.left=x;rectangle.top=y; }
    else { rectangle.left=std::min(rectangle.left,x);rectangle.top=std::min(rectangle.top,y); }
    if (!valid(rectangle.right,rectangle.bottom)) { rectangle.right=x;rectangle.bottom=y; }
    else { rectangle.right=std::max(rectangle.right,x);rectangle.bottom=std::max(rectangle.bottom,y); }
}
// 첫 청크 조회가 바꾼 현재 missionPost/옵션도 이 분기에 그대로 반영한다.
FortRandomRectangle RawWorldGeyserBounds::Whole() const {
    auto result=Cells(0);
    if (state_.missionPost) {
        const auto other=Cells(1);Extend(result,other.left,other.top);Extend(result,other.right,other.bottom);
    } else {
        const auto padding=Padding(),leading=Signed(static_cast<std::uint32_t>(padding)-16U);
        result.left=static_cast<float>(static_cast<double>(result.left)-leading);result.top=static_cast<float>(static_cast<double>(result.top)-leading);
        result.right=static_cast<float>(static_cast<double>(result.right)+padding);result.bottom=static_cast<float>(static_cast<double>(result.bottom)+padding);
    }
    return result;
}
// 원본은 분할 fallback에서도 이미 지역0을 한 번 조회했으며 Whole에서 다시 조회한다.
FortRandomRectangle RawWorldGeyserBounds::Partition(std::int32_t cell,std::int32_t grid) const {
    auto rectangle=Cells(0);const auto count=Signed(static_cast<std::uint32_t>(grid)*static_cast<std::uint32_t>(grid));
    if (state_.missionPost || cell>count) return Whole();
    if (grid<1 || grid>kMaximumHostGrid) throw std::out_of_range("세계 간헐천 분할의 호스트 반복 범위");
    const auto padding=Padding(),leading=Signed(static_cast<std::uint32_t>(padding)-16U);
    rectangle.left=static_cast<float>(static_cast<double>(rectangle.left)-leading);
    const double top=static_cast<double>(rectangle.top)-leading,bottom=static_cast<double>(rectangle.bottom)+padding; // 원문 y축은 이 시점에 float로 저장하지 않는다.
    rectangle.right=static_cast<float>(static_cast<double>(rectangle.right)+padding);
    const double inverse=1.0/static_cast<double>(grid);
    const float width=static_cast<float>(Absolute(Truncate(static_cast<double>(rectangle.right)-rectangle.left))*inverse); // 원문 x폭만 한 번 단정도로 저장한다.
    const double height=Absolute(Truncate(bottom-top))*inverse;
    std::int32_t column=1,row=1;
    for (std::int32_t current=1;current<=count;++current) { //0 이하 cell은 원문처럼 전체 순회를 마친 다음의 열/행을 사용한다.
        if (column>grid) { column=1;++row; }if (current==cell) break;++column;
    }
    const auto left=static_cast<float>((column-1)*static_cast<double>(width)+rectangle.left);
    const auto upper=static_cast<float>((row-1)*height+top);
    const auto right=static_cast<float>(static_cast<double>(rectangle.right)-(grid-column)*static_cast<double>(width));
    const auto lower=static_cast<float>(bottom-(grid-row)*height);
    return {left,upper,right,lower};
}
}
