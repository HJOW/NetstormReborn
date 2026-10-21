// 원본 이동 명령의 요청과 서버 소비를 분리하여 전송 이전 월드 변경을 막는다.
#include "o/RawCarrierMove.h"
#include <array>
#include <bit>
#include <stdexcept>
#include <utility>
namespace netstorm::o {
namespace {
// 선택된 실제 하위 누락을 성공으로 바꾸지 않는다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("운반체 이동의 실제 하위 미연결");return hook; }
// 비정렬 원문의 낮은 바이트부터 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) {
    std::uint32_t result{};
    // 원본 WORD/DWORD의 지정 폭만 조합한다.
    for (std::size_t i=0;i<width;++i) result|=std::uint32_t(bytes[at+i])<<(8*i);return result;
}
// 원본 지정 폭만 쓰며 앞뒤 바이트를 보존한다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) {
    // 호스트 정렬과 무관하게 낮은 바이트부터 쓴다.
    for (std::size_t i=0;i<width;++i) bytes[at+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// 수신 callback 뒤 바뀔 수 있는 caller의 두 float 비트를 현재 위치에서 읽는다.
RawPathPoint Point(std::span<const std::uint8_t> packet) { return {std::bit_cast<float>(Read(packet,6)),std::bit_cast<float>(Read(packet,10))}; }
}
// 원본 생성자처럼protocol의bit7만제거하며length를4로보정하지않는다.
std::size_t WriteCarrierMoveCommand(const CarrierMoveState& state,Sid parent,RawPathPoint destination,std::uint32_t sacrifice,std::span<std::uint8_t> packet) { if (packet.size()<18) throw std::out_of_range("이동 명령18바이트 저장소 부족");packet[0]=static_cast<std::uint8_t>(state.request.protocol&0x7fU);packet[1]=state.request.message;Put(packet,2,state.request.length,2);Put(packet,4,parent.value,2);Put(packet,6,std::bit_cast<std::uint32_t>(destination.x));Put(packet,10,std::bit_cast<std::uint32_t>(destination.y));Put(packet,14,sacrifice);return 18; }
// 기존 현재 상태를 빌리며 장치나 전송을 열지 않는다.
RawCarrierMove::RawCarrierMove(const SidPool& pool,std::span<const RiftTypeRecord> types,const bool& fort,
    CarrierMoveState& state,CarrierMoveHooks hooks):pool_(pool),types_(types),fort_(fort),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("운반체 이동 판본/TYPE 오류");
}
// 가상 효과 전후 TYPE를 캐시하지 않는다.
std::uint32_t RawCarrierMove::Genus(Sid parent) const { const auto kind=pool_.Slot(parent)[10];if (kind>=types_.size()) throw std::out_of_range("운반체 이동 현재TYPE 범위");return types_[kind].flags2; }
// 선택 해제는 carrier/연결 관문보다 먼저 실행된다.
void RawCarrierMove::Request(Sid parent,RawPathPoint destination,std::uint32_t sacrifice) const {
    if (Need(hooks_.selected)()==parent) Need(hooks_.select)(0,0,1);
    if (!(Genus(parent)&0x30000U)) Need(hooks_.assertion)(0x571);
    if (!(Genus(parent)&0x30000U)) return;
    if (!(Genus(parent)&0x20000U)) {
        const auto raw=pool_.Slot(parent);const RawPathPoint source{std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))};
        if (!Need(hooks_.connected)(source,destination)) return;
    }
    const auto movement=Need(hooks_.movement)(pool_.Slot(parent)[10]);auto* path=Need(hooks_.path)(parent,movement); //조회 효과 뒤 현재 경로만 읽는다.
    if (path) { const auto bytes=path->Bytes();if (std::bit_cast<float>(Read(bytes,0x186dc))==destination.x && std::bit_cast<float>(Read(bytes,0x186e0))==destination.y) return; }
    std::array<std::uint8_t,18> packet{};WriteCarrierMoveCommand(state_,parent,destination,sacrifice,packet);Need(hooks_.send)(9,packet);
}
// 소유자는 전체DWORD로 비교하며 fort 모드일 때만 다른 소유자의 이동을 허용한다.
void RawCarrierMove::Receive(std::uint32_t owner,std::span<const std::uint8_t> packet) const {
    if (packet.size()<18) throw std::invalid_argument("운반체 이동packet 물리 길이 부족");
    const auto number=[&packet] { return Sid{static_cast<std::uint16_t>(Read(packet,4,2))}; }; //외부 효과 뒤 원본이 다시 읽는 부모WORD다.
    const auto parent=number();if (pool_.Slot(parent)[11]&3U) return;if (!(Genus(parent)&0x30000U)) return;
    if (Need(hooks_.regular)(parent,0x5105)) return;if (pool_.Slot(parent)[34]!=owner && !fort_) return;if (Need(hooks_.regular)(number(),0x493)) return;
    if (!Need(hooks_.candidate)(parent,Point(packet)).first.value) {
        const auto place=Need(hooks_.returnPlace)(Point(packet));if (place.value) { Need(hooks_.target)(number(),place);return; }
        const auto geyser=Need(hooks_.genusAt)(Point(packet),0x10000000);
        if (geyser.value) { auto* path=Need(hooks_.geyser)(parent,geyser);if (path) { path->SetTargets(geyser.value,0);return; } }
        else {
            const auto altar=Need(hooks_.genusAt)(Point(packet),0x400000);
            if (altar.value && (Genus(parent)&0x200000U) && Need(hooks_.altarAllowed)(altar,number()) && Read(packet,14)) { Need(hooks_.sacrifice)(number(),altar);return; }
        }
    }
    auto* path=Need(hooks_.start)({number(),Point(packet),0,0,0,0,1}); //원본 마지막 인자1이며 앞선 직접 UI 경로의replaceStart1과 다르다.
    if (path) { path->SetMoveAwayDepth(0);path->SetTargets(0,0); }
}
// Session의 같은 단일 풀이다.
const SidPool& RawCarrierMove::Pool() const { return pool_; }
}
