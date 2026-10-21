// 원본 양방향의 현재 경계/연속 빈칸과 디버그 표식 순서를 보존한다.
#include "o/RawAiDryGeometry.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr float kCellBias=0.9999F; //00500edc의 float 비트3f7ff972이며 사제의0.99999와 구분한다.
constexpr double kHalf=0.5; //00500ee4의 중점 상수다.합 뒤 float 저장 전에 곱한다.
constexpr std::uint32_t kDebug=29;constexpr std::uint16_t kMarkerWord=93; //원본 활성 디버그 종류와 표식 구분 WORD다.
constexpr std::int32_t kSide=256,kRun=4; //물리 지도 한 변과 방향별 필요한 연속 빈칸 수다.
// CRT signed64절삭의 하위DWORD이며 잘못된 FISTP의 하위값은0이다.
std::int32_t Ftol(double value) { if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(std::trunc(value)))); }
// 도달한 실제 효과가 없으면 앞서 수행한 효과를 유지하고 그 위치에서 거부한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("AI 가짜좌표 탐색의 실제 하위 미연결");return function; }
}
// 두 축 차이를 중간 float로 줄이지 않고 제곱근까지 계산한다.
double AiPointDistance(RawPathPoint first,RawPathPoint second) { const double dy=static_cast<double>(first.y)-second.y,dx=static_cast<double>(first.x)-second.x;return std::sqrt(dy*dy+dx*dx); }
// 생성할 때 지도를 복사하거나 표식을 만들지 않는다.
RawAiDryGeometry::RawAiDryGeometry(const SidPool& pool,AiDryGeometryGlobals globals,AiDryGeometryHooks hooks):pool_(pool),globals_(globals),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078 || pool.Capacity()>65536) throw std::invalid_argument("AI 가짜좌표 탐색의 PATCH WORD 풀 누락"); }
// 현재 경계의 less/equal만 허용하는 원본 parity 분기를 일반 ordered 비교로 표현한다.
bool RawAiDryGeometry::Inside(RawPathPoint point) const { const auto& bounds=globals_.bounds;return bounds.left<=point.x && point.x<=bounds.right && bounds.top<=point.y && point.y<=bounds.bottom; }
// 각 축의 편향 합은 ftol 전에 float로 저장하지 않는다. 지도 밖은 빈칸으로 계산한다.
bool RawAiDryGeometry::Empty(RawPathPoint point) const { const auto x=Ftol(static_cast<double>(point.x)+kCellBias),y=Ftol(static_cast<double>(point.y)+kCellBias);return x<0 || y<0 || x>=kSide || y>=kSide || !Need(hooks_.surface)(x,y); }
// 원본은 SID1에서 Next를 먼저 호출하며, 삭제 전에 다음 SID를 미리 잡는다.
void RawAiDryGeometry::ClearMarkers() const {
    const auto end=pool_.Capacity();std::uint32_t cursor=1;
    // 원본 iterator는 free bit만 건너뛰고 dead 객체는 방문한다.
    const auto next=[&] { ++cursor;while (cursor<end && (pool_.Slot(Sid{static_cast<std::uint16_t>(cursor)})[11]&1U)) ++cursor; };
    next();
    // 가상 삭제 효과 뒤 현재 TYPE 전역/슬롯을 읽으며 논리 순서를 바꾸지 않는다.
    while (cursor>=1 && cursor<pool_.Capacity()) { const auto sid=Sid{static_cast<std::uint16_t>(cursor)};next();const auto raw=pool_.Slot(sid);const auto word=static_cast<std::uint16_t>(raw[12]|(static_cast<std::uint16_t>(raw[13])<<8));if (raw[10]==globals_.markerType && word==kMarkerWord) Need(hooks_.destroy)(sid,0); }
}
// 생성/WORD/Pop 경계가 실패해도 원본처럼 앞선 생성이나 쓰기를 취소하지 않는다.
void RawAiDryGeometry::Mark(RawPathPoint point) const { const auto sid=Need(hooks_.create)(globals_.markerType,2);Need(hooks_.word)(sid,kMarkerWord);Need(hooks_.pop)(sid,point,0); }
// 진행이 경계에서 정지하는 원본 경로에 임의 성공/예산/대체 좌표를 넣지 않는다.
RawPathPoint RawAiDryGeometry::Candidate(RawPathPoint source,RawPathPoint target) const {
    if (globals_.debug==kDebug) { ClearMarkers();Mark(source);Mark(target); }
    const auto distance=static_cast<float>(AiPointDistance(target,source));const double inverse=1.0/static_cast<double>(distance);
    const auto dx=static_cast<float>((static_cast<double>(target.x)-source.x)*inverse),dy=static_cast<float>((static_cast<double>(target.y)-source.y)*inverse);
    const RawPathPoint middle{static_cast<float>((static_cast<double>(target.x)+source.x)*kHalf),static_cast<float>((static_cast<double>(target.y)+source.y)*kHalf)};
    RawPathPoint along=source;float progress{};
    // 디버그가 꺼져도 선분의 float 누적/횟수 진행은 원래 순서대로 수행한다.
    while (static_cast<double>(progress)<distance) { along.x=static_cast<float>(static_cast<double>(along.x)+dx);along.y=static_cast<float>(static_cast<double>(along.y)+dy);if (globals_.debug==kDebug) Mark(along);const double advanced=static_cast<double>(progress)+1.0;progress=static_cast<float>(advanced);if (!(advanced<distance)) break; }
    RawPathPoint left=middle,right=middle;std::int32_t leftRun{},rightRun{};
    // 왼쪽을 먼저 움직이고 오른쪽도 같은 반복에서 검사한 뒤 왼쪽 성공을 우선 반환한다.
    for (;;) {
        if (rightRun>=kRun) return leftRun>=kRun ? left : right;
        if (Inside(left)) { left.x=static_cast<float>(static_cast<double>(left.x)-dy);left.y=static_cast<float>(static_cast<double>(left.y)+dx);leftRun=Empty(left) ? leftRun+1 : 0; }
        if (Inside(right)) { right.x=static_cast<float>(static_cast<double>(right.x)+dy);right.y=static_cast<float>(static_cast<double>(right.y)-dx);rightRun=Empty(right) ? rightRun+1 : 0; }
        if (globals_.debug==kDebug) { Mark(left);Mark(right); }
        if (leftRun>=kRun) return left;
    }
}
// 같은 실제 월드의 풀인지 확인한다.
const SidPool& RawAiDryGeometry::Pool() const { return pool_; }
}
