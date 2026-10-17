// 원본 정수 입력·DWORD 감김·절삭 및 외부 효과 뒤 현재 잠금 재조회를 보존한다.
#include "client/RawCursorPosition.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 원본503b54/504194의 단정도 비트다. 특히 Y는 double 1/11로 바꾸지 않는다.
constexpr float kInverseX=std::bit_cast<float>(0x3d800000U),kInverseY=std::bit_cast<float>(0x3dba2e8cU);
// 도달한 효과가 없으면 미복원을 명시한다.
template<class T> const T& Need(const T& effect) { if (!effect) throw std::logic_error("커서 위치 갱신 의존 동작 누락");return effect; }
// 정수 화면 산술의 DWORD 감김을 signed UB 없이 유지한다.
std::int32_t Add(std::int32_t left,std::int32_t right) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(left)+static_cast<std::uint32_t>(right)); }
std::int32_t Sub(std::int32_t left,std::int32_t right) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(left)-static_cast<std::uint32_t>(right)); }
// 실제 _ftol 하위 DWORD의 signed 해석이다. indefinite의 하위 DWORD는0이다.
std::int32_t Truncate(float value) {
    if (!std::isfinite(value) || static_cast<double>(value) < -0x1p63 || static_cast<double>(value)>=0x1p63) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
}
// 별도 카메라/위치 사본을 만들지 않는다. CD의 갱신 몸체는 아직 대응이 확인되지 않았다.
RawCursorPosition::RawCursorPosition(CursorDropState& drop,const CursorPlacementState& placement,const std::uint32_t& snapToGrid,
    const std::array<std::uint32_t,2>& camera,CursorPositionHooks hooks,o::OriginalEdition edition)
    :drop_(drop),placement_(placement),snapToGrid_(snapToGrid),camera_(camera),hooks_(std::move(hooks)) {
    if (edition!=o::OriginalEdition::Patch1078) throw std::invalid_argument("커서 위치 갱신10.78 판본 계약 오류");
}
// 실제497490의 fild32×단정도 역배율 뒤 최종 float 저장이다. 별도 사본의 카메라를 추정하지 않는다.
std::array<std::uint32_t,2> RawCursorPosition::CameraFromPixels(std::int32_t x,std::int32_t y) {
    return {std::bit_cast<std::uint32_t>(static_cast<float>(static_cast<double>(x)*kInverseX)),std::bit_cast<std::uint32_t>(static_cast<float>(static_cast<double>(y)*kInverseY))};
}
// 실제 fild32는 정수를 먼저 float로 좁히지 않는다. Y와 X 모두 마지막 저장에서만 좁힌다.
std::array<float,2> RawCursorPosition::ScreenToWorld(std::int32_t x,std::int32_t y,const std::array<std::uint32_t,2>& camera) {
    const float worldY=static_cast<float>(static_cast<double>(Add(y,11))*static_cast<double>(kInverseY)+static_cast<double>(std::bit_cast<float>(camera[1])));
    const float worldX=static_cast<float>(static_cast<double>(Add(x,16))*static_cast<double>(kInverseX)+static_cast<double>(std::bit_cast<float>(camera[0])));
    return {worldX,worldY};
}
// fild가 읽는 것은 EDX:EAX 전체가 아니라 EAX에 저장한 signed32다.
std::array<float,2> RawCursorPosition::TruncatePoint(std::array<float,2> point) {
    return {static_cast<float>(Truncate(point[0])),static_cast<float>(Truncate(point[1]))};
}
// 화면 좌표는 위치 불변/잠금 경로에서도 먼저 갱신한다.
void RawCursorPosition::Update(std::int32_t mouseX,std::int32_t mouseY) const {
    drop_.screenX=Sub(Add(mouseX,Sub(placement_.hotspotX,placement_.imageWidth/2)),16);
    drop_.screenY=Sub(Add(mouseY,Sub(placement_.hotspotY,placement_.imageHeight/2)),11);
    auto point=ScreenToWorld(drop_.screenX,drop_.screenY,camera_);if (snapToGrid_) point=TruncatePoint(point);
    if (point[0]!=drop_.x || point[1]!=drop_.y) {
        if (!drop_.lockDepth) Need(hooks_.activate)(0);
        drop_.x=point[0];drop_.y=point[1];
        if (!drop_.lockDepth) { Need(hooks_.createPreviews)();Need(hooks_.selectCursor)(drop_.legalGump ? 3U : 2U); }
    }
}
// 실제 수명/생성의 단일 풀 계약과 커서 선택기를 초기 연결에서 확인한다.
CursorPositionHooks MakeCursorPositionHooks(RawCursorLifecycle& lifecycle,RawCursorPreviews& previews,std::function<void(std::uint32_t)> selectCursor) {
    if (&lifecycle.Pool()!=&previews.Pool() || !selectCursor) throw std::invalid_argument("커서 위치 갱신 단일 풀/선택기 연결 오류");
    return {[&lifecycle](std::uint32_t force) { lifecycle.Activate(force); },std::move(selectCursor),[&previews] { previews.Create(); }};
}
// 실제 Take의 마지막 두 정수 인자를 보존한다.
void BindCursorPosition(CursorLifecycleHooks& lifecycle,RawCursorPosition& position) {
    lifecycle.updatePosition=[&position](std::int32_t x,std::int32_t y) { position.Update(x,y); };
}
}
