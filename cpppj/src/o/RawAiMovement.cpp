// 원본 AI 이동의 권한 분기·현재 재조회·로그 전 이름 조회와 거리 unordered를 유지한다.
#include "o/RawAiMovement.h"
#include <array>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr float kHomeDistance=2.0F; //PE00501444의원본복귀거리경계다.
// 비정렬DWORD를범위검사뒤낮은BYTE부터읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at) { if (at>raw.size() || raw.size()-at<4) throw std::out_of_range("AI 이동 DWORD 범위");std::uint32_t value{};for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(raw[at+index])<<(index*8);return value; } //반복문은실제네BYTE만조합한다.
// 선택된실제하위누락을가짜경로성공/빈이름으로대신하지않는다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("AI 이동 실제 하위 미연결");return function; }
// 원본ftol의signed64절삭하위DWORD와무효입력의0을보존한다.
std::int32_t Ftol(double value) { if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(std::trunc(value)))); }
}
// 원본은내부점에서음수거리를허용하며최종unordered도x차이를돌려준다.
double AiDistanceToFootprint(RawPathPoint point,SquidFootprint bounds) { const float x=point.x<bounds.left ? static_cast<float>(static_cast<double>(bounds.left)-point.x) : static_cast<float>(static_cast<double>(point.x)-bounds.right);const double y=point.y<bounds.top ? static_cast<double>(bounds.top)-point.y : static_cast<double>(point.y)-bounds.bottom;return y>static_cast<double>(x) ? y : static_cast<double>(x); }
// 생성시이동/전송/이름조회효과를실행하지않는다.
RawAiMovement::RawAiMovement(RawAiBrainMemory& memory,const SidPool& pool,const bool& boss,const RawBoardTarget& board,const CarrierMoveState& packet,AiMovementHooks hooks)
    :memory_(memory),pool_(pool),boss_(boss),board_(board),packet_(packet),hooks_(std::move(hooks)) { if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("AI 이동 같은PATCH풀 누락"); }
// 가짜/해제토큰과다른크기의본체를거부한다.
std::uint32_t RawAiMovement::Field(std::uint32_t token,std::size_t offset) const { const auto raw=memory_.Bytes(token);if (raw.size()!=RawAiBrainConstructor::kBytes) throw std::invalid_argument("AI 이동 실제0xb08본체 누락");return Read(raw,offset); }
// 위치조회에소유자/생존필터를추가하지않는다.
RawPathPoint RawAiMovement::Position(Sid actor) const { const auto raw=pool_.Slot(actor);return {std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))}; }
// 잘못된DWORD를다른SID로절단하지않는다.
Sid RawAiMovement::PhysicalSid(std::uint32_t value) const { if (value>=pool_.Capacity() || value>std::numeric_limits<std::uint16_t>::max()) throw std::out_of_range("AI 이동 현재목표SID 범위");return Sid{static_cast<std::uint16_t>(value)}; }
// 순번0/remove0은경로시작의다른추가인자가아니다.
Sid RawAiMovement::MovementForm(Sid actor) const { const auto type=Need(hooks_.movement)(pool_.Slot(actor)[10]);return Need(hooks_.lookup)(actor,type,0,false); }
// actor0일때는TYPE나목록에접근하지않는다.
void RawAiMovement::Stop(Sid actor) const { if (!actor.value) return;const auto type=Need(hooks_.movement)(pool_.Slot(actor)[10]);static_cast<void>(Need(hooks_.lookup)(actor,type,0,true)); }
// 헤더는실제송신직전현재공유값이며사용자선택/중복목적지검사를추가하지않는다.
void RawAiMovement::Send(Sid actor,RawPathPoint point,std::uint32_t sacrifice) const { std::array<std::uint8_t,18> packet{};WriteCarrierMoveCommand(packet_,actor,point,sacrifice,packet);Need(hooks_.send)(9,packet); }
// 처음서버였던희생분기만즉시의식으로돌아가며송신뒤권한변경은대상접근분기를따른다.
void RawAiMovement::ToTarget(std::uint32_t token,Sid actor,Sid target,std::uint32_t sacrifice) const {
    if (!boss_) { const auto footprint=Need(hooks_.footprint)(target);const auto source=Position(actor);const auto point=board_.Nearest(footprint,source,1);Send(actor,point,sacrifice); }
    else if (sacrifice) { Need(hooks_.sacrifice)(actor,target);return; }
    if (boss_ && !Need(hooks_.target)(actor,target,0,0)) return;
    const auto targetName=Need(hooks_.name)(target);const auto sourceName=Need(hooks_.name)(actor);if (Field(token,0xb00)) Need(hooks_.logTarget)(token,sourceName,targetName);
}
// 송신뒤현재권한이생겼으면동일한인자0의실제경로를시작한다.실패하면이름/로그도생략한다.
void RawAiMovement::ToPoint(std::uint32_t token,Sid actor,RawPathPoint point,std::uint32_t sacrifice) const {
    if (!boss_) Send(actor,point,sacrifice);if (boss_ && !Need(hooks_.start)(RawPathStartRequest{actor,point,0,0,0,0,0})) return;
    const auto y=Ftol(point.y),x=Ftol(point.x);const auto name=Need(hooks_.name)(actor);if (Field(token,0xb00)) Need(hooks_.logPoint)(token,name,x,y);
}
// 거리 판정 뒤 현재 신전 SID를 다시 읽어 원본 이동에 전달한다.
void RawAiMovement::Home(std::uint32_t token,Sid actor) const { const auto temple=Field(token,0x294);if (!temple) return;const auto point=Position(actor);const auto distance=Need(hooks_.distance)(point,PhysicalSid(temple));if (!(distance<kHomeDistance)) ToTarget(token,actor,PhysicalSid(Field(token,0x294)),0); }
// 같은 실제 월드의 풀인지 확인한다.
const SidPool& RawAiMovement::Pool() const { return pool_; }
}
