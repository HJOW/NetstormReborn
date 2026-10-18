// 요청과 서버 권한/현재 가상 삭제를 원본과 같은 시점에 분리한다.
#include "o/RawSalvageCommand.h"
#include <array>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 선택된 실제 하위가 비었으면 빈 성공으로 바꾸지 않는다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("실제 회수 명령 하위 미연결");return hook; }
// 지정한 비정렬WORD를 인접 바이트와 섞지 않고 읽는다.
std::uint32_t Word(std::span<const std::uint8_t> raw,std::size_t at) { return raw[at]|(std::uint32_t(raw[at+1])<<8); }
// SID/flags/길이의 원래 하위WORD만 저장한다.
void PutWord(std::span<std::uint8_t> raw,std::size_t at,std::uint32_t value) { raw[at]=static_cast<std::uint8_t>(value);raw[at+1]=static_cast<std::uint8_t>(value>>8); }
}
// CD판의 별도 수신/회수 본문을10.78과 같다고 추정하지 않는다.
RawSalvageCommand::RawSalvageCommand(const SidPool& pool,std::span<const RiftTypeRecord> types,SalvageCommandState& state,SalvageCommandHooks hooks)
    :pool_(pool),types_(types),state_(state),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188) throw std::invalid_argument("회수 명령의10.78 풀/타입 계약 오류");
}
// 최초SID0은 정상 무동작이며, 제단 조회 뒤에는 다음SID의 상태/타입을 새로 읽는다.
void RawSalvageCommand::Request(std::uint32_t number) const {
    while (number) { // 제단에서 실제 다이스로 옮겨 같은 원본 관문을 반복한다.
        if (number>=pool_.Capacity() || number>65535) throw std::out_of_range("회수 요청SID 물리 범위");const Sid sid{static_cast<std::uint16_t>(number)};
        if (pool_.Slot(sid)[11]&3U) return;
        if (state_.checking) {
            if (state_.altarType>=types_.size() || state_.daisType>=types_.size()) throw std::out_of_range("회수 비용 진단TYPE 범위");
            if (types_[state_.altarType].cost!=types_[state_.daisType].cost) Need(hooks_.assertion)(0x295);
        }
        if (pool_.Slot(sid)[10]==state_.altarType) { number=Need(hooks_.daisAt)(sid).value;continue; }
        const auto raw=pool_.Slot(sid);const auto kind=raw[10]<70 ? Word(raw,18) : raw[10]; // 내용물은 현재 클래스BYTE 대신 실제 종류WORD를 비교한다.
        if (raw[34]==state_.localOwner && state_.denySalvage && kind!=state_.nuggetType) { Need(hooks_.tell)("DenySalvage");return; }
        Send(number,0x200000);return;
    }
}
// 전체DWORD 입력의 낮은SID WORD와 높은flags WORD를 원문 그대로 전송한다.
void RawSalvageCommand::Send(std::uint32_t sid,std::uint32_t flags) const {
    std::array<std::uint8_t,8> packet{static_cast<std::uint8_t>(state_.request.protocol&0x7f),state_.request.message};
    PutWord(packet,2,state_.request.length);PutWord(packet,4,sid);PutWord(packet,6,flags>>16);Need(hooks_.send)(9,packet);
}
// dead는 추가로 거르지 않는다. 진단 뒤packet flags를 읽고 환불 뒤에도 보관한flags를 유지한다.
void RawSalvageCommand::Receive(std::uint32_t sender,std::span<const std::uint8_t> packet) const {
    if (packet.size()<8) throw std::invalid_argument("회수54h 패킷 길이 오류");const Sid sid{static_cast<std::uint16_t>(Word(packet,4))};
    if (!sid.value || sid.value>=pool_.Capacity() || (pool_.Slot(sid)[11]&1U)) return;
    if (!state_.mission && pool_.Slot(sid)[34]!=sender) return;
    if (!state_.boss) { Need(hooks_.assertion)(0x563);if (!state_.boss) return; }
    const auto flags=Word(packet,6)<<16; // 원본은 권한 진단 이후의 caller WORD를 한 번만 보관한다.
    if (pool_.Slot(sid)[10]>=70 && (flags&0x200000U)) Need(hooks_.refund)(sid,pool_.Slot(sid)[34],true);
    Need(hooks_.destroy)(sid,flags);
}
}
