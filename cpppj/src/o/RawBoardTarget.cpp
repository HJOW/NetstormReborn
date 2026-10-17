// 사각형 오른쪽의 확장만 x87에 남고 다른 세 변은 float 저장을 거친다.
#include "o/RawBoardTarget.h"
#include <bit>
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
constexpr double kHalf=0.5,kExpand=1.0; // 실제500ee4/5012d0의 중간점 비율과 변 확장 폭이다.
// 잘못된 방향의 원본 FLD는 flags를 float로 읽으며 signaling NaN을 quiet으로 바꾼다.
float FlagFloat(std::uint32_t bits) { if ((bits&0x7f800000U)==0x7f800000U && (bits&0x7fffffU)) bits|=0x400000U;return std::bit_cast<float>(bits); }
}
// 좌표를 별도 보정하거나 Graph getter를 요구하지 않는다.
RawBoardTarget::RawBoardTarget(BoardTargetState& state,std::function<void(std::uint32_t)> assertion):state_(state),assertion_(std::move(assertion)) {}
// 원본은 사각형을 복사해 확장하며 결과를 지도 범위로 clamp하지 않는다.
RawPathPoint RawBoardTarget::Side(SquidFootprint r,std::uint32_t side,std::uint32_t flags) const {
    double right=r.right;
    if (flags&4U) { r.left=static_cast<float>(static_cast<double>(r.left)-kExpand);r.top=static_cast<float>(static_cast<double>(r.top)-kExpand);right+=kExpand;r.bottom=static_cast<float>(static_cast<double>(r.bottom)+kExpand); }
    if (side==0 || side==4) return {static_cast<float>((right-r.left)*kHalf+r.left),side==0 ? r.top : r.bottom};
    if (side==2 || side==6) return {side==2 ? static_cast<float>(right) : r.left,static_cast<float>((static_cast<double>(r.bottom)-r.top)*kHalf+r.top)};
    if (!assertion_) throw std::logic_error("사각형 변 중간점 원본 진단 미연결");assertion_(0x1f9);const auto value=FlagFloat(flags);return {value,value};
}
// flags2 비교는 순서 없는 값도 동쪽으로 가므로 <= 조건으로 바꾸지 않는다.
RawPathPoint RawBoardTarget::Nearest(SquidFootprint r,RawPathPoint source,std::uint32_t flags) const {
    const float x=static_cast<float>((static_cast<double>(r.left)+r.right)*kHalf),y=static_cast<float>((static_cast<double>(r.bottom)+r.top)*kHalf);
    auto side=(static_cast<std::uint32_t>(RawSquidNeighbors::Direction(source.x,source.y,x,y))-4U)&7U;
    if ((flags&2U) && side==0) side=source.x<(static_cast<double>(r.left)+r.right)*kHalf ? 6U : 2U;
    if (flags&1U) return Side(r,side,flags);
    if (!assertion_) throw std::logic_error("일반 접근점 원본 진단 미연결");assertion_(0x262);return state_.invalid;
}
}
