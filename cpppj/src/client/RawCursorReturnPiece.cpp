// 생산 칸 반환은 현재 옵션을 생성 뒤 다시 읽고, 할당 뒤 모드와 소멸 시간을 읽는다.
#include "client/RawCursorReturnPiece.h"
#include "o/CanonDecoder.h"
#include <array>
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// 세 소멸 시간과 요새 모드의 단정도 원본 비트다. 원본 배열은 로컬 스택에 놓인다.
constexpr std::array<float,3> kLifetimes{10.0F,5.0F,1.0F};
constexpr std::uint32_t kFortLifetime=0x38d1b717;
// 어댑터가 다른 SID 풀을 참조하는 반환 몸체를 연결하는 것을 막는다.
void CheckPool(const o::SidPool& pool,const RawCursorReturnPiece& body) {
    if (&body.Pool()!=&pool) throw std::invalid_argument("조각 반환 SID 풀이 다릅니다");
}
}
RawCursorReturnPiece::RawCursorReturnPiece(o::RawContainedCreate& contained,const CursorReturnPieceState& state,
    o::GameRandom& random,CursorReturnPieceHooks hooks)
    :contained_(contained),state_(state),random_(random),hooks_(std::move(hooks)) {
    if (!hooks_.allocateTimer || !hooks_.createTimer) throw std::invalid_argument("조각 반환 타이머 경계 누락");
}
void RawCursorReturnPiece::Return(std::uint32_t flags,std::uint32_t source,std::uint32_t quality) const {
    if (source==0 || source>=Pool().Capacity()) throw std::out_of_range("조각 반환 부모 SID 범위 오류");
    const o::Sid parent{static_cast<std::uint16_t>(source)};flags&=~0x100U;
    if (state_.type==state_.bridgeType) {
        const auto pattern=o::SelectBridgePattern(static_cast<std::int32_t>(random_.Next(10000)),Pool().Edition());
        contained_.Create(parent,state_.type,static_cast<std::uint32_t>(pattern),flags,0x10,0);return;
    }
    if (state_.keepPieces==0) flags|=0x100;
    const auto decreased=quality-1U;
    const auto sid=contained_.Create(parent,state_.type,0,flags,0x10,std::bit_cast<std::int32_t>(decreased)<0 ? 0U : decreased);
    if (state_.keepPieces==0 && hooks_.allocateTimer(0x28)) {
        if (state_.fort==0 && state_.lifetimeIndex>=kLifetimes.size()) throw std::out_of_range("조각 반환 소멸 시간 번호 오류");
        const float duration=state_.fort ? std::bit_cast<float>(kFortLifetime) : kLifetimes[state_.lifetimeIndex];
        hooks_.createTimer(sid,0,duration);
    }
}
const o::SidPool& RawCursorReturnPiece::Pool() const { return contained_.Pool(); }
CursorReturnPieceHooks MakeCursorReturnPieceHooks(const o::SidPool& pool,o::SquidProcessHost& host,CursorReturnPieceHooks hooks) {
    if (&host.Pool()!=&pool) throw std::invalid_argument("조각 반환/프로세스 SID 풀이 다릅니다");
    hooks.createTimer=[&host](o::Sid sid,std::uint32_t event,float duration) { host.AddRegular(sid,event,duration); };
    return hooks;
}
o::ConstructionLocalHooks MakeReturnPieceHooks(const o::SidPool& pool,RawCursorReturnPiece& body,o::ConstructionLocalHooks hooks) {
    CheckPool(pool,body);hooks.returnPiece=[&body](std::uint32_t flags,std::uint32_t source,std::uint32_t quality) { body.Return(flags,source,quality); };return hooks;
}
CursorDropHooks MakeReturnPieceHooks(const o::SidPool& pool,RawCursorReturnPiece& body,CursorDropHooks hooks) {
    CheckPool(pool,body);hooks.returnPiece=[&body](std::uint32_t flags,std::uint32_t source,std::uint32_t quality) { body.Return(flags,source,quality); };return hooks;
}
}
