// 서버 건설 확정의 사제 조회 조건·조각별 SID 확보·통지 필드·전송/처리/기록 순서를 원본대로 유지한다.
#include "o/RawConstructionConfirm.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 타입 flags1의 "사제가 지어야 함" 비트와 확정 플래그의 "사제 조회 생략" 비트다.
constexpr std::uint32_t kNeedsBuilder=0x8000,kSkipBuilderFlag=1;
// 통지 하나에 담을 수 있는 SID 수의 상한이다. 원본은 20번째를 넣은 뒤 assert(sidCount < MAX_SIDS)한다.
constexpr std::size_t kMaxSids=20;
// 일반 객체의 첫 번호다. 번호 0~4는 풀의 예약 슬롯이라 새 조각이 될 수 없다.
constexpr std::uint16_t kFirstSid=5;
}
// 풀과 필수 경계는 첫 효과 전에 검사한다.
RawConstructionConfirm::RawConstructionConfirm(const SidPool& pool,std::span<const RiftTypeRecord> types,
    std::span<const PriestPlainCanonType> frames,const ConstructionConfirmState& state,ConstructionConfirmHooks hooks)
    :pool_(pool),types_(types),frames_(frames),state_(state),hooks_(std::move(hooks)) {
    if (!hooks_.findBuilder || !hooks_.create || !hooks_.broadcast || !hooks_.handle || !hooks_.recorderActive || !hooks_.record)
        throw std::invalid_argument("건설 확정 필수 효과 연결 오류");
}
// 원본 순서: 사제 조회 → decoder 생성 → 조각마다 SID 생성 → 통지 필드 → 전송 → 직접 처리 → 기록기.
bool RawConstructionConfirm::Confirm(const ConstructionConfirmRequest& request) const {
    if (request.type>=types_.size() || request.type>=frames_.size() || request.type>0xff) throw std::out_of_range("건설 확정 타입 번호 오류");
    if ((types_[request.type].flags1&kNeedsBuilder) && !(request.flags&kSkipBuilderFlag) && !state_.skipBuilderCheck &&
        hooks_.findBuilder(request.player,request.type,request.x,request.y).value==0) return false;
    const auto& meta=frames_[request.type];
    auto decoder=DecodeCanonType(meta.frames,meta.defaultFrame,state_.patternTypes,{request.type,std::bit_cast<int>(request.argument),
        std::bit_cast<int>(request.direction),request.x,request.y,false},pool_.Edition());
    std::size_t pieces=0;
    // 사본으로 유효 칸을 세어 상한을 넘는 요청을 SID 생성 전에 거부한다.
    for (auto probe=decoder;probe.Valid();probe.Advance()) ++pieces;
    if (pieces>=kMaxSids) throw std::length_error("건설 확정 조각 수가 통지 상한을 넘는다(sidCount < MAX_SIDS assert)");
    ConstructionNotice notice;
    // 유효 칸마다 서버 SID의 void 객체를 하나 만들고 번호를 통지에 순서대로 담는다.
    for (;decoder.Valid();decoder.Advance()) {
        const Sid sid=hooks_.create(request.type,0);
        if (sid.value<kFirstSid || sid.value>=pool_.Capacity()) throw std::invalid_argument("건설 확정 SID 생성이 유효한 번호를 돌려주지 않았습니다");
        notice.sids.push_back(sid.value);
    }
    // 좌표와 시각은 비트 그대로, 나머지 인자는 하위 바이트만 싣는다.
    notice.type=static_cast<std::uint8_t>(request.type);notice.x=request.x;notice.y=request.y;
    notice.argument=static_cast<std::uint8_t>(request.argument);notice.direction=static_cast<std::uint8_t>(request.direction);
    notice.player=static_cast<std::uint8_t>(request.player);notice.flags=static_cast<std::uint8_t>(request.flags);
    notice.quality=static_cast<std::uint8_t>(request.quality);notice.timeLow=request.timeLow;notice.timeHigh=request.timeHigh;
    hooks_.broadcast(notice);
    hooks_.handle(notice);
    // 조각이 없을 때 원본은 초기화되지 않은 지역 값을 기록기에 넘긴다. 여기서는 0을 넘기며 원본 값이라고 주장하지 않는다.
    if (state_.recorderEnabled && hooks_.recorderActive()) hooks_.record(Sid{notice.sids.empty() ? std::uint16_t{0} : notice.sids.front()});
    return true;
}
// 어댑터와 호출자가 같은 풀을 쓰는지 확인할 수 있게 실제 풀을 돌려준다.
const SidPool& RawConstructionConfirm::Pool() const { return pool_; }
// 같은 풀의 실제 factory로 서버 SID를 만든다. 다른 풀의 factory는 거부한다.
ConstructionConfirmHooks MakeConstructionConfirmHooks(const SidPool& pool,SquidFactory& factory,ConstructionConfirmHooks hooks) {
    if (&pool!=&factory.Pool()) throw std::invalid_argument("건설 확정/factory SID 풀이 다릅니다");
    hooks.create=[&factory](std::uint32_t type,std::uint32_t flags) { return factory.Create(type,flags); };
    return hooks;
}
}
