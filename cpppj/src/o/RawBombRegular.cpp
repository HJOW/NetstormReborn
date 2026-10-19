// 시간 비교의 단정도 저장과 배포→로컬수신→상태 캡처→해제 순서를 보존한다.
#include "o/RawBombRegular.h"
#include <algorithm>
#include <bit>
#include <new>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::size_t kTargetCount=100,kPacketSize=210; // 원본new400의DWORD100개와스택 패킷210바이트다.
// 선택한 경계 누락을 무동작 성공으로 처리하지 않는다.
template<class F> const F& Need(const F& hook) {
    if (!hook) throw std::logic_error("Bomb Regular 실제 하위 미연결");
    return hook;
}
// 원본의 비정렬 WORD/DWORD 필드를 little endian으로 쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width) {
    // 지정한 폭 밖의FF 초기화 바이트는 유지한다.
    for (std::size_t i=0;i<width;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
}
// 다른 판본의타입 배치와 수신 몸체를 섞지 않는다.
RawBombRegular::RawBombRegular(const SidPool& pool,std::span<const BombRegularType> types,BombRegularState& state,BombRegularHooks hooks)
    :pool_(pool),types_(types),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("Bomb Regular 판본/TYPE 오류");
}
// 효과 후 변경된 현재 TYPE를 검사하여 조회한다.
const BombRegularType& RawBombRegular::Type(Sid parent) const {
    const auto type=pool_.Slot(parent)[10];
    if (type>=types_.size()) throw std::out_of_range("Bomb Regular 현재TYPE 범위");
    return types_[type];
}
// event 종류와 무관하게 count0에서 초기화하고 매 반복에서 현재 권한/시간을 확인한다.
float RawBombRegular::Handle(Sid parent,std::uint32_t event,std::uint32_t count,float payload) const {
    static_cast<void>(event); // 원본 기본496e70은 사건 번호를 읽지 않고 payload만 반환한다.
    if (!count) {
        const auto raw=pool_.Slot(parent);
        const Sid carrier{static_cast<std::uint16_t>(raw[12]|(std::uint16_t(raw[13])<<8))};
        if (carrier.value) Need(hooks_.notice)(carrier,6,std::bit_cast<std::uint32_t>(Type(parent).castTime)*2U);
        Need(hooks_.frame)(parent,std::bit_cast<std::uint32_t>(Type(parent).firstFrame),0);
    } else Need(hooks_.animate)(parent,1,0);
    if (!state_.boss) return payload;
    const float elapsed=static_cast<float>(static_cast<double>(std::bit_cast<std::int32_t>(count))*static_cast<double>(payload)); // 원본x87 곱의float32 저장 뒤 비교다.
    if (!(static_cast<double>(Type(parent).castTime)<static_cast<double>(elapsed))) return payload;
    if (!Need(hooks_.allocate)(400)) throw std::bad_alloc(); // 원본의NULL 쓰기 대신 실패를 명시하며 시전 성공으로 세지 않는다.
    std::array<std::uint32_t,kTargetCount> targets{};
    std::array<std::uint8_t,kPacketSize> packet;
    try {
        const auto countTargets=Need(hooks_.targets)(parent,targets);
        if (countTargets>targets.size()) throw std::out_of_range("Bomb Regular 대상100개 초과");
        packet.fill(0xff);
        packet[0]=state_.effect.protocol&0x7fU;
        packet[1]=state_.effect.message;
        Put(packet,2,std::max<std::uint16_t>(state_.effect.length,4),2);
        Put(packet,4,parent.value,2);
        Put(packet,6,countTargets,4);
        // 디컴파일에서 누락된실제40ace0 반복으로 대상DWORD의낮은WORD만 복사한다.
        for (std::size_t i=0;i<countTargets;++i) Put(packet,10+2*i,targets[i],2);
        Need(hooks_.broadcast)(packet);
        Need(hooks_.receive)(state_.localOwner,0,packet);
    } catch (...) {
        Need(hooks_.release)(400);
        throw;
    }
    const bool stopped=(pool_.Slot(parent)[11]&7U)!=0; // free 이전에 캡처하며 free의재진입 변화는 반환값에 영향을 주지 않는다.
    Need(hooks_.release)(400);
    return stopped ? -1.0F : 0.0F;
}
// 같은 세션 연결 여부를 조회한다.
const SidPool& RawBombRegular::Pool() const { return pool_; }
}
