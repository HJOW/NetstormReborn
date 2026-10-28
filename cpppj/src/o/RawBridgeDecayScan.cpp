// 원문의 x87 기본53비트 산술·signed DWORD 커서와 칸마다의 catch를 보존한다.
#include "o/RawBridgeDecayScan.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o { //원문과 다른 CD 슬롯 배치를 PATCH 실행 성공으로 합산하지 않는다.
namespace { //호스트의 signed overflow/범위 밖 float 변환을 사용하지 않는다.
//004e49c0은 signed64로 절삭한 뒤 EAX만 소비한다. 비유한/64비트 범위 밖이면 indefinite의 하위0이다.
std::int32_t Budget(double value) {
    if (!std::isfinite(value) || value<-0x1p63 || value>=0x1p63) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
//원문 sub/inc는 DWORD에서 감기므로 C++ signed overflow를 피하고 같은 비트를 반환한다.
std::int32_t Add(std::int32_t first,std::uint32_t second) {
    return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(first)+second);
}
}
//구성은 현재 스캔을 쓰지 않으며 원본의 최초 BSS/이전 World 반납 상태도 보존한다.
RawBridgeDecayScan::RawBridgeDecayScan(const SidPool& pool,std::span<const RiftTypeRecord> types,
    BridgeDecayScanState& state,const float& period,const BridgeDecayMode& mode,std::function<void(Sid)> cell)
    :pool_(pool),types_(types),state_(state),period_(period),mode_(mode),cell_(std::move(cell)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || !cell_)
        throw std::invalid_argument("실제 다리 붕괴 스캔의 PATCH 풀/TYPE/칸 처리 연결 오류");
}
//기한이 지난 프레임은 range개만 방문하며 끝 슬롯을 포함한다. 재설정은 모든 칸 처리 뒤다.
void RawBridgeDecayScan::Frame(const FrameTime& time,bool paused) const {
    if (paused || mode_.editor || !mode_.authority) return;
    const auto range=Add(state_.last,0U-std::bit_cast<std::uint32_t>(state_.first));
    std::int32_t remaining=range; //기한을 넘겼으면 delta/주기를 읽지 않는 원문 분기다.
    if (state_.next-time.game>0.0) remaining=Budget((time.delta/static_cast<double>(period_))*static_cast<double>(range));
    while (remaining>0 && state_.cursor<=state_.last) { //매 칸 효과가 바꾼 현재 커서/끝 번호를 다음 방문에 다시 읽는다.
        --remaining;
        try {
            if (state_.cursor<0 || static_cast<std::uint32_t>(state_.cursor)>=pool_.Capacity())
                throw std::out_of_range("다리 붕괴 스캔의 실제 물리 SID 범위");
            const Sid sid{static_cast<std::uint16_t>(state_.cursor)};const auto raw=pool_.Slot(sid);
            if ((raw[11]&7U)==0) {
                if (raw[10]>=types_.size()) throw std::out_of_range("다리 붕괴 스캔의 현재 TYPE 범위");
                if ((types_[raw[10]].flags2&TypeFlag2::kBridge) && !(raw[40]&1U)) cell_(sid);
            }
        } catch (...) { //00422c93은 이 슬롯의 예외만 삼키고 이미 발생한 효과를 되돌리지 않는다.
        }
        state_.cursor=Add(state_.cursor,1);
    }
    if (time.game>=state_.next) { //기한과 현재 주기도 칸 효과 뒤 다시 읽으며 밀린 옛 기한에 누적하지 않는다.
        const double next=static_cast<double>(period_)+time.game;
        state_.cursor=state_.first;state_.next=next;
    }
}
}
