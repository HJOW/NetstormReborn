// 배치 통지 처리기의 전체 분기와 효과 순서를 원본 판본별 차이까지 보존한다.
#include "o/RawConstructionNotice.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// raw 타입·상태·좌표와 판본별 소유자 필드 위치다.
constexpr std::size_t kType=10,kState=11,kX=14,kY=18,kPatchOwner=34,kCdOwner=32;
// HP 보유·건설 사제 필요 플래그와 소유 사제 이동 대상·사제 genus 비트다.
constexpr std::uint32_t kHasHp=0x10,kNeedsBuilder=0x8000,kPriestTargets=0x404200,kPriest=0x200000;
// 이력에서 제외하는 그룹 번호와 원본 SID 개수 상한이다.
constexpr std::int32_t kNoHistoryGroup=10;
constexpr std::size_t kMaxPieces=19;
// 원본 좌표 거의 올림에서 더하는 단정도 값이다(00502310 / CD 005017a0).
constexpr float kRoundUpBias=0.9999899864196777F;
// 정렬되지 않은 좌표 DWORD를 little endian으로 읽어 단정도로 해석한다.
float Coordinate(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t bits=0;
    // 낮은 바이트부터 좌표 비트를 합친다.
    for (std::size_t i=0;i<4;++i) bits|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return std::bit_cast<float>(bits);
}
// 0041d7a0 / CD 00440a80: bias를 더해 __ftol의 하위 signed DWORD로 절삭하고 단정도로 저장한다.
float Snap(float value) {
    const double sum=static_cast<double>(value)+static_cast<double>(kRoundUpBias);
    if (!std::isfinite(sum) || std::fabs(sum)>=9223372036854775808.0) return 0.0F;
    const auto low=static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(sum)));
    return static_cast<float>(std::bit_cast<std::int32_t>(low));
}
}
// 필수 효과 연결은 판본에 맞춰 검사한다.
RawConstructionNotice::RawConstructionNotice(const SidPool& pool,std::span<const RiftTypeRecord> types,
    ConstructionNoticeState& state,ConstructionNoticeHooks hooks):pool_(pool),types_(types),state_(state),hooks_(std::move(hooks)) {
    if (!hooks_.clear || !hooks_.place || !hooks_.moveBuilder || !hooks_.findPriest || !hooks_.immobile || !hooks_.approach ||
        !hooks_.tryMove || !hooks_.refund || !hooks_.destroy || !hooks_.start || !hooks_.refresh ||
        (pool_.Edition()==OriginalEdition::Patch1078 && (!hooks_.containedType || !hooks_.removeContained)))
        throw std::invalid_argument("건설 배치 통지의 효과 연결 오류");
}
// 배치 후 슬롯을 다시 읽는다. 통지 타입과 실제 조각 타입은 같다고 가정하지 않는다.
std::uint32_t RawConstructionNotice::Genus(Sid sid) const {
    const auto type=pool_.Slot(sid)[kType];
    if (type>=types_.size()) throw std::out_of_range("건설 배치 통지 객체 타입 오류");
    return types_[type].flags2;
}
// 재시도에서도 사제 좌표를 새로 읽는다. 첫 이동 경계가 좌표를 바꿀 수 있다.
bool RawConstructionNotice::Approach(Sid priest,Sid object,std::uint32_t flags) const {
    const auto raw=pool_.Slot(priest);
    const auto point=hooks_.approach(object,Coordinate(raw,kX),Coordinate(raw,kY),flags);
    return hooks_.tryMove(priest,Snap(point.x),Snap(point.y),object);
}
// 로컬 예측 정리는 항상 배치보다 먼저다. abstract가 아니어도 이력·배치·최종 갱신은 수행한다.
void RawConstructionNotice::Handle(const ConstructionNotice& notice) const {
    if (notice.type>=types_.size()) throw std::out_of_range("건설 배치 통지 타입 번호 오류");
    if (notice.sids.empty() || notice.sids.size()>kMaxPieces) throw std::invalid_argument("건설 배치 통지 SID 개수 오류");
    // 잘못된 목록은 정리나 이력 변경 전에 거부한다.
    for (const auto sid:notice.sids) {
        if (!sid) throw std::invalid_argument("건설 배치 통지 SID 0 오류");
        (void)pool_.Slot(Sid{sid});
    }
    hooks_.clear({notice.type,notice.x,notice.y,notice.argument,notice.direction,notice.player});
    const auto& type=types_[notice.type];
    const bool abstract=((type.flags1&kHasHp)!=0 || notice.type==state_.daisType) && !(notice.flags&1);
    const bool needsBuilder=(type.flags1&kNeedsBuilder)!=0 && !(notice.flags&1);
    if (type.group!=kNoHistoryGroup && notice.player==state_.localPlayer) {
        // 뒤에서부터 밀어 가장 오래된 좌표를 버리고 첫 칸에 새 좌표를 넣는다.
        for (std::size_t i=state_.history.size()-1;i>0;--i) state_.history[i]=state_.history[i-1];
        state_.history[0]={notice.x,notice.y};state_.historyCounter=0;
    }
    hooks_.place({notice.type,notice.x,notice.y,notice.argument,notice.direction,notice.player,notice.sids,abstract ? 1U : 0U,notice.quality});
    const Sid first{notice.sids.front()};
    if (abstract) {
        if (needsBuilder) {
            if (notice.player!=state_.localPlayer || state_.moveLocalBuilder)
                hooks_.moveBuilder(notice.player,first,notice.timeLow,notice.timeHigh);
        } else if (state_.authority) {
            if (Genus(first)&kPriestTargets) {
                const Sid priest=hooks_.findPriest(notice.player);
                if (pool_.Edition()==OriginalEdition::Patch1078) {
                    const auto contained=hooks_.containedType(state_.priestType);
                    const Sid current=hooks_.findPriest(notice.player);
                    hooks_.removeContained(current,contained);
                }
                const bool moved=(pool_.Slot(priest)[kState]&3)==0 && (Genus(priest)&kPriest)!=0 && !hooks_.immobile(priest) &&
                    (Approach(priest,first,7) || Approach(priest,first,3));
                if (!moved) {
                    const auto raw=pool_.Slot(first);
                    hooks_.refund(raw[kType],raw[pool_.Edition()==OriginalEdition::Patch1078 ? kPatchOwner : kCdOwner]);
                    hooks_.destroy(first,0);
                }
            } else hooks_.start(first,0);
        }
    }
    hooks_.refresh();
}
// 실제 풀을 돌려준다.
const SidPool& RawConstructionNotice::Pool() const { return pool_; }
// 외부 효과 가운데 이미 복원된 몸체만 실제 모듈로 교체한다.
ConstructionNoticeHooks MakeConstructionNoticeHooks(const SidPool& pool,const RawConstructionClear& clear,RawConstructionPlace& place,
    ConstructionNoticeHooks hooks,const RawPriestState* priest) {
    if (&pool!=&clear.Pool() || &pool!=&place.Pool() || (priest && &pool!=&priest->Pool()))
        throw std::invalid_argument("건설 배치 통지 어댑터의 풀 연결 오류");
    hooks.clear=[&clear](const ConstructionClearRequest& request) { clear.Clear(request); };
    hooks.place=[&place](const ConstructionPlaceRequest& request) { place.Place(request); };
    hooks.refund=[&place](std::uint32_t type,std::uint32_t player) { place.Refund(type,player); };
    if (priest) hooks.immobile=[priest](Sid sid) { return priest->Immobile(sid); };
    return hooks;
}
// 서버가 전송한 것과 같은 통지를 직접 처리한다.
ConstructionConfirmHooks MakeConstructionNoticeConfirmHooks(const SidPool& pool,const RawConstructionNotice& notice,ConstructionConfirmHooks hooks) {
    if (&pool!=&notice.Pool()) throw std::invalid_argument("건설 확정 통지 어댑터의 풀 연결 오류");
    hooks.handle=[&notice](const ConstructionNotice& value) { notice.Handle(value); };
    return hooks;
}
}
