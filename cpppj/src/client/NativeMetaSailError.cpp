// 안내 뒤에도 진입 CS의 같은 기록을 읽으며 추가 known/active/phase 관문을 만들지 않는다.
#include "client/NativeMetaSailError.h"
#include <stdexcept>

namespace netstorm::client { //수신의 실제 BYTE/WORD/DWORD와 선택된 효과만 처리한다.
namespace { //원문의 NULL 역참조/물리 밖 읽기는 선택된 위치에서 호스트 오류로 표시한다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("항해 오류의 실제 장치 미연결");return hook; } //선택하지 않은 안내 장치를 먼저 요구하지 않는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("항해 오류의 실제 기록/패킷 물리 부족");std::uint32_t value{};for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(8*byte);return value; } //지정 폭만 낮은 BYTE부터 읽는다.
}
// 번역/Tell 뒤의 실제 기록 재조회 순서와 미일치 NULL의 부분 효과도 유지한다.
void ReceiveNativeMetaSailError(ClientChallengeGlobals& challenge,std::span<const std::uint8_t> packet,const std::uint32_t& subscriber,const NativeMetaSailErrorHooks& h) {
    const auto owner=subscriber;const auto cs=challenge.Bytes();std::span<std::uint8_t> record; //PE의 진입 subscriber와 선택한 원문 포인터를 보유하며 현재 CS로 다시 바꾸지 않는다.
    for (std::size_t index=0;index<o::kChallengeParticipantCount;++index) { const auto candidate=cs.subspan(o::kChallengeParticipantOffset+index*o::kChallengeParticipantBytes,o::kChallengeParticipantBytes);if (Read(candidate,4)==owner) { record=candidate;break; } } //원문40기록의 첫 subscriber만 검색하며0/비활성도 제외하지 않는다.
    std::string_view message,section; //선택된 분기의 정적 원문 문자열이다.
    switch (Read(packet,6,2)) {
    case 1:message="You have been kicked off!";break;
    case 2:message="ERROR: Invalid battle id";break;
    case 3:message="ERROR: Invalid battle slot id";break;
    case 4:message="That battle began before you got there";break;
    case 5:section="TooManyPirates";break;
    case 6:section="BattleVersionMismatch";break;
    case 7:section="BattleFailedDiags";break;
    default:break;
    }
    if (!message.empty()) { const auto text=Need(h.translate)(message);Need(h.note)(Read(record,0xb8),text); }
    else if (!section.empty()) Need(h.tell)(section);
    Need(h.update)(Read(record,0),0xff);
}
}
