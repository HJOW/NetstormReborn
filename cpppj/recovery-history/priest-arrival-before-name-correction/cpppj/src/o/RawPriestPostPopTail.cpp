// Carrier 이후 현재 값과 원본 거의 올림 지면을 읽어 사제 후처리 순서를 보존한다.
#include "o/RawPriestPostPopTail.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// raw 좌표·프레임 위치와 지면의 거의 올림 배율이다(00500edc / CD 0050038c).
constexpr std::size_t kX=14,kY=18,kPatchFrame=36,kCdFrame=34;
constexpr float kCoordinateBias=0.9999f;
// 강제 낙하 재등록 flag와 지면의 섬/다리 비트, 낙하 중 프레임 J의 방향 번호다.
constexpr std::uint32_t kFallingPop=0x800,kGroundMask=6;
constexpr int kFallingDirection=9;
// 비정렬 little endian 필드를 지정 폭만큼 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 낮은 바이트부터 조합하여 원본 DWORD/BYTE 폭을 유지한다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return value;
}
// 원본 x87의 float+float 합을 좁히지 않고 0방향 절삭한다. 지도 밖/비유한 좌표는 접근 전에 거부한다.
int Cell(float value) {
    const double shifted=static_cast<double>(value)+static_cast<double>(kCoordinateBias);
    if (!std::isfinite(value) || shifted<0 || shifted>=kWorldCells) throw std::out_of_range("사제 postPop 지면 좌표 오류");
    return static_cast<int>(shifted);
}
}
// 풀·표·필수 효과의 연결 오류를 첫 목록/회복 효과 전에 거부한다.
RawPriestPostPopTail::RawPriestPostPopTail(SidPool& pool,const RawPriestPostPop& prefix,const RawCarrierPostPop& carrier,
    const RawPriestState& state,std::span<const RiftTypeFrames> frames,std::span<const std::uint8_t> spots,PriestPostPopTailHooks hooks)
    :pool_(pool),prefix_(prefix),carrier_(carrier),state_(state),frames_(frames.begin(),frames.end()),spots_(spots),hooks_(std::move(hooks)) {
    if (&pool!=&prefix.Pool() || &pool!=&carrier.Pool() || &pool!=&state.Pool() ||
        frames.size()!=(pool.Edition()==OriginalEdition::Patch1078 ? 188U : 171U) || spots.size()!=kWorldCells*kWorldCells ||
        !hooks_.fall || !hooks_.ensureShield || !hooks_.clearShield) throw std::invalid_argument("사제 postPop 후반 연결 오류");
}
// 낙하 효과로 extra/지면이 바뀌어도 원본이 선택한 이동 불가 분기의 보호막 요청을 유지한다.
void RawPriestPostPopTail::PostPop(Sid sid,std::uint32_t flags) const {
    prefix_.Prefix(sid,flags);carrier_.PostPop(sid,flags);
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (pool_.Slot(sid)[patch ? 40 : 35]&9) return;
    if (!state_.Immobile(sid)) { hooks_.clearShield(sid);return; }
    if (!(flags&kFallingPop)) {
        const auto raw=pool_.Slot(sid);
        const int x=Cell(std::bit_cast<float>(Read(raw,kX))),y=Cell(std::bit_cast<float>(Read(raw,kY)));
        if (!(spots_[static_cast<std::size_t>(y*kWorldCells+x)]&kGroundMask)) {
            const auto frame=patch ? std::bit_cast<std::int32_t>(Read(raw,kPatchFrame)) : static_cast<std::int32_t>(Read(raw,kCdFrame,1));
            if (RawPathAnimation::Direction(frames_.at(raw[10]),frame)!=kFallingDirection) hooks_.fall(sid);
        }
    }
    hooks_.ensureShield(sid);
}
// 공간 변경 전에는 현재 위치의 이동 불가/지면을 읽지 않고 접두 계약만 확인한다.
void RawPriestPostPopTail::Validate(Sid sid) const { prefix_.Validate(sid); }
// Pop/Activate 분배가 같은 raw 풀을 사용하는지 확인한다.
const SidPool& RawPriestPostPopTail::Pool() const { return pool_; }
// 이동 가능한 현재 좌표의 조회 결과만 삭제하고 기존 보호막 생성을 그대로 연결한다.
PriestPostPopTailHooks MakePriestPostPopTailShieldHooks(const SidPool& pool,const RawPriestShield& shield,
    const RawPriestForcefield& lookup,std::function<void(Sid,std::uint32_t)> destroy,std::function<void(Sid)> fall) {
    if (&pool!=&shield.Pool() || &pool!=&lookup.Pool() || !destroy || !fall) throw std::invalid_argument("사제 postPop 보호막 연결 오류");
    return {std::move(fall),[&shield](Sid sid) { shield.Ensure(sid); },
        [&pool,&lookup,destroy=std::move(destroy)](Sid sid) {
            const auto found=lookup.Find(sid);
            if (found.value>0 && found.value<pool.Capacity()) destroy(found,0);
        }};
}
}
