// 실제 조회를 각 외부 효과 다음에 실행하여 현재 상태/타입/좌표를 읽는다.
#include "o/RawPriestFall.h"
#include "o/RawPriestOwner.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::o {
namespace {
// raw 좌표와 원본 프레임 필드 위치다. CD 프레임은 BYTE다.
constexpr std::size_t kX=14,kY=18,kPatchFrame=36,kCdFrame=34;
// 낙하 시작 예약과 반복 반환의 실제 float 상수다.
constexpr float kInitialPayload=0.01f,kRepeatPayload=0.1f,kCoordinateBias=0.9999f;
// 정렬되지 않은 필드를 원본 폭으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 낮은 바이트부터 조합하여 호스트 정렬에 의존하지 않는다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return value;
}
}
// 모든 실행 경계를 미리 확인하여 미연결 상태에서 일부 효과만 실행되는 일을 막는다.
RawPriestFall::RawPriestFall(SidPool& pool,const RawPriestState& priest,std::span<const RiftTypeFrames> frames,
    std::span<const PriestFallFrames> choices,std::span<const std::uint16_t> surfaces,PriestFallState& state,PriestFallHooks hooks)
    :pool_(pool),priest_(priest),frames_(frames),choices_(choices),surfaces_(surfaces),state_(state),hooks_(std::move(hooks)) {
    if (&pool!=&priest.Pool() || frames.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) ||
        choices.size()!=frames.size() || surfaces.size()!=65536 || !hooks_.ensureShield || !hooks_.reserveRegular ||
        !hooks_.addSharedRegular || !hooks_.unpop || !hooks_.repop || !hooks_.sound || !hooks_.setFrame ||
        !hooks_.advanceFrame || !hooks_.clearShield) throw std::invalid_argument("사제 낙하 연결 오류");
}
// 확보 실패여도 권한 공간 전환/소리 효과를 계속한다. 권한은 생성 효과 다음에 다시 읽는다.
void RawPriestFall::Begin(Sid sid) const {
    (void)pool_.Slot(sid);hooks_.ensureShield(sid);
    if (hooks_.reserveRegular()) hooks_.addSharedRegular(sid,kPriestFallEvent,kInitialPayload);
    if (state_.authority) { hooks_.unpop(sid,0);hooks_.repop(sid,0x800); }
    const auto raw=pool_.Slot(sid);
    hooks_.sound(sid,std::bit_cast<float>(Read(raw,kX)),std::bit_cast<float>(Read(raw,kY)));
}
// float 입력의 합을 double로 유지한 뒤 0방향 절삭한다. 지도 WORD는 유효 SID 여부 없이 검사한다.
bool RawPriestFall::SurfaceAt(Sid sid) const {
    const auto raw=pool_.Slot(sid);
    const double x=std::trunc(static_cast<double>(std::bit_cast<float>(Read(raw,kX)))+kCoordinateBias);
    const double y=std::trunc(static_cast<double>(std::bit_cast<float>(Read(raw,kY)))+kCoordinateBias);
    if (!std::isfinite(x) || !std::isfinite(y)) throw std::out_of_range("사제 낙하 좌표 오류");
    if (x<0 || y<0 || x>=256 || y>=256) return false;
    return surfaces_[static_cast<std::size_t>(y)*256+static_cast<std::size_t>(x)]!=0;
}
// 시작/도착 프레임 효과 뒤 이동 불가를 다시 조회한다. 공통 Regular가 반환값으로 재예약/삭제한다.
float RawPriestFall::Handle(Sid sid,std::uint32_t event,std::uint32_t,float) const {
    if (event!=kPriestFallEvent) throw std::invalid_argument("사제 낙하 이벤트 번호 오류");
    if (priest_.Immobile(sid)) hooks_.ensureShield(sid);
    const auto raw=pool_.Slot(sid);const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (raw[10]>=frames_.size()) throw std::out_of_range("사제 낙하 프레임 타입 오류");
    const auto frame=patch ? std::bit_cast<std::int32_t>(Read(raw,kPatchFrame)) : static_cast<std::int32_t>(raw[kCdFrame]);
    if (RawPathAnimation::Direction(frames_[raw[10]],frame)==9) hooks_.advanceFrame(sid,1,0);
    else hooks_.setFrame(sid,choices_[raw[10]].falling,0);
    if (!SurfaceAt(sid)) return kRepeatPayload;
    if (pool_.Slot(sid)[10]>=choices_.size()) throw std::out_of_range("사제 도착 프레임 타입 오류");
    hooks_.setFrame(sid,choices_[pool_.Slot(sid)[10]].landed,0);
    if (!priest_.Immobile(sid)) hooks_.clearShield(sid);
    return 0.0f;
}
// 분배기에서 풀 주소를 직접 비교한다.
const SidPool& RawPriestFall::Pool() const { return pool_; }
// 기존 회복 분배기와 fallback으로 합성한다. 사제 미복원 사건은 기본 허용하지 않는다.
RegularHandler MakePriestFallHandler(const SidPool& pool,const RawPriestFall& fall,RegularHandler fallback) {
    if (&pool!=&fall.Pool()) throw std::invalid_argument("사제 낙하 분배 풀 불일치");
    return [&pool,&fall,fallback=std::move(fallback)](Sid sid,std::uint32_t event,std::uint32_t count,float payload) {
        const auto raw=pool.Slot(sid);
        const bool priest=Read(raw,0)==(pool.Edition()==OriginalEdition::Patch1078 ? kPatchPriestVtable : kCdPriestVtable);
        if (priest && event==kPriestFallEvent) return fall.Handle(sid,event,count,payload);
        if (fallback) return fallback(sid,event,count,payload);
        if (priest) throw std::invalid_argument("미복원 사제 이벤트 분배");
        return payload;
    };
}
// 실제 보호막/ProcessHost를 사용하며 공간/프레임/소리 전체 몸체는 공급된 경계에 둔다.
PriestFallHooks MakePriestFallHooks(SquidProcessHost& host,const RawPriestShield& shield,
    const RawPriestForcefield& lookup,std::function<void(Sid,std::uint32_t)> destroy,PriestFallHooks hooks) {
    const auto& pool=host.Pool();
    if (&pool!=&shield.Pool() || &pool!=&lookup.Pool() || !destroy) throw std::invalid_argument("사제 낙하 효과 풀 불일치");
    hooks.ensureShield=[&shield](Sid sid) { shield.Ensure(sid); };
    hooks.reserveRegular=[] { return true; };
    hooks.addSharedRegular=[&host](Sid sid,std::uint32_t event,float payload) { host.AddSharedRegular(sid,event,payload); };
    hooks.clearShield=[&pool,&lookup,destroy=std::move(destroy)](Sid sid) {
        const auto found=lookup.Find(sid);
        if (found.value>0 && found.value<pool.Capacity()) destroy(found,0);
    };
    return hooks;
}
}
