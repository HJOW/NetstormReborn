// 원본 가짜좌표의 선행 제거/첫 최고점수/중심Y 중복 인자와 효과 뒤 현재 상태를 보존한다.
#include "o/RawAiFakeDrySpots.h"
#include "o/RawAiPriestActions.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr float kRadiusFactor=0.7F; //00501784의 float 비트3f333333이다.문자열 조각이 아니다.
constexpr std::uint32_t kUnset=0xc61c4000U,kDebug=29; //전략4의 미정좌표-10000과 원본 표식 활성 종류다.
constexpr std::size_t kDryStride=12; //부족좌표는 x/y float와 TYPE DWORD다.
// 실제 물리 블록 안의 비정렬 little endian DWORD를 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at) { if (at>raw.size() || raw.size()-at<4) throw std::out_of_range("AI 가짜좌표 DWORD 읽기 범위");std::uint32_t value{};for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[at+index])<<(index*8);return value; } //반복문은 지정 네 BYTE만 합친다.
// 원본 DWORD의 signed 비교를 보존한다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// CRT signed64절삭의 하위 DWORD이며 무효 FISTP의 하위값은0이다.
std::int32_t Ftol(double value) { if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;return Signed(static_cast<std::uint32_t>(static_cast<std::int64_t>(std::trunc(value)))); }
// 도달한 실제 하위가 없으면 앞선 쓰기를 유지하고 그 위치에서 거부한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("AI 가짜좌표 실제 하위 미연결");return function; }
}
// 생성 중 지도 탐색이나 목록 효과를 수행하지 않는다.
RawAiFakeDrySpots::RawAiFakeDrySpots(RawAiBrainMemory& memory,const SidPool& pool,AiFakeDryGlobals globals,AiFakeDryHooks hooks):memory_(memory),pool_(pool),globals_(globals),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("AI 가짜좌표의 같은 PATCH 풀 누락"); }
// 작은 가짜 본체/해제 토큰을 성공으로 취급하지 않는다.
std::span<std::uint8_t> RawAiFakeDrySpots::Body(std::uint32_t token) const { auto raw=memory_.Bytes(token);if (raw.size()!=RawAiBrainConstructor::kBytes) throw std::invalid_argument("AI 가짜좌표 실제0xb08 본체 누락");return raw; }
// 현재 본체를 하위 효과 뒤 다시 빌린다.
std::uint32_t RawAiFakeDrySpots::Field(std::uint32_t token,std::size_t at) const { return Read(Body(token),at); }
// 현재 지정 필드의 네 바이트만 기록한다.
void RawAiFakeDrySpots::Field(std::uint32_t token,std::size_t at,std::uint32_t value) const { auto raw=Body(token);if (at>raw.size() || raw.size()-at<4) throw std::out_of_range("AI 가짜좌표 DWORD 쓰기 범위");for (std::size_t index=0;index<4;++index) raw[at+index]=static_cast<std::uint8_t>(value>>(index*8)); } //반복문은 주변 미정 바이트를 보존한다.
// 풀 밖 SID를 잘린 WORD의 다른 객체로 바꾸지 않는다.
Sid RawAiFakeDrySpots::Object(std::uint32_t number) const { if (number>65535 || number>=pool_.Capacity()) throw std::out_of_range("AI 가짜좌표 현재 SID 물리 범위");return Sid{static_cast<std::uint16_t>(number)}; }
// 중심 계산과 raw 위치 조회의 원래 차이를 유지한다.
RawPathPoint RawAiFakeDrySpots::Position(Sid sid) const { const auto raw=pool_.Slot(sid);return {std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))}; }
// 원본은 로그가 꺼져도 두 ftol 인자를 먼저 평가한다.
void RawAiFakeDrySpots::Log(std::uint32_t token,AiFakeDryLog action,RawPathPoint point) const { const auto y=Ftol(point.y),x=Ftol(point.x);if (Field(token,0xb00)) Need(hooks_.log)(token,action,x,y); }
// 현재 TYPE가 없으면 full 검사조차 하지 않으며, full 제거는 다른 모든 하위보다 먼저다.
void RawAiFakeDrySpots::Extend(std::uint32_t token) const {
    if (!Field(token,0x280)) return;
    if (Signed(Field(token,0xae0))>=Signed(Field(token,0xadc))) Need(hooks_.remove)(token,0);
    RawPathPoint point{};
    if (!Field(token,0x2fc)) {
        const auto temple=Field(token,0x294);if (!temple) return;point=Position(Object(temple));static_cast<void>(Need(hooks_.add)(token,point,Field(token,0x280)));
    } else {
        bool added{};float best{};std::int32_t dryIndex{},selectedIndex{};std::uint32_t selected{};
        if (Signed(Field(token,0x2fc))>0) {
            const auto count=Signed(Field(token,0x2fc));const auto pointer=Field(token,0x2f4);
            // 진입 count/포인터를 유지하며 raw 위치/부족거리는 각 항목에서 다시 조회한다.
            for (std::int32_t index=0;index<count;++index) { const auto sid=Read(memory_.Bytes(pointer),static_cast<std::size_t>(index)*4);const auto score=Need(hooks_.score)(token,Position(Object(sid)),dryIndex);if (score>best) { best=score;selectedIndex=dryIndex;selected=sid; } }
            if (best>0) {
                const auto center=Need(hooks_.center)(Object(selected));const auto records=memory_.Bytes(Field(token,0xae4));if (selectedIndex<0) throw std::out_of_range("AI 가짜좌표 선택 Dry Spot 음수 index");const auto at=static_cast<std::size_t>(selectedIndex)*kDryStride;
                const RawPathPoint target{static_cast<float>(static_cast<double>(std::bit_cast<float>(Read(records,at)))-1.0),static_cast<float>(static_cast<double>(std::bit_cast<float>(Read(records,at+4)))-1.0)};
                point=Need(hooks_.candidate)({center.y,center.y},target);
                if (AiPriestPointValid(point,globals_.mapLimit)) {
                    if (AiPointDistance(point,center)>static_cast<double>(globals_.manaRadius)*kRadiusFactor) {
                        static_cast<void>(Need(hooks_.add)(token,point,Field(token,0x280)));const bool wandering=Field(token,0x2d4)==4;Field(token,0xaf0,std::bit_cast<std::uint32_t>(point.x));Field(token,0xaf4,std::bit_cast<std::uint32_t>(point.y));
                        if (wandering) { Field(token,0x3cc,0);Field(token,0x3d4,kUnset);Field(token,0x3d0,kUnset);Field(token,0x3d8,1); }
                        if (globals_.debug==kDebug) Need(hooks_.mark)(point);added=true;
                    } else Log(token,AiFakeDryLog::Discarded,point);
                }
            }
        }
        const RawPathPoint current{std::bit_cast<float>(Field(token,0xaf0)),std::bit_cast<float>(Field(token,0xaf4))};if (!AiPriestPointValid(current,globals_.mapLimit)) Need(hooks_.seed)(token);
        if (!added) return;
    }
    Log(token,AiFakeDryLog::Added,point);
}
// 같은 실제 월드의 풀인지 확인한다.
const SidPool& RawAiFakeDrySpots::Pool() const { return pool_; }
}
