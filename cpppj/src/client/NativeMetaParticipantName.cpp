// 현재 기록 포인터를 고정하고 하위 호출 뒤 필요한 필드만 원문 위치에서 읽는다.
#include "client/NativeMetaParticipantName.h"
#include <cstdio>
#include <stdexcept>

namespace netstorm::client { //88h에는 메타 모드/송신자/known/현재 phase 관문을 추가하지 않는다.
namespace { //원문 비정렬 DWORD/첫 NUL과 실제 저장소 범위만 다룬다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("메타 참가자 이름의 실제 장치 미연결");return hook; } //도달한 원문 효과 위치에서만 연결을 요구한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) { //현재 BYTE/WORD/DWORD를 낮은 BYTE부터 읽는다.
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("메타 참가자 이름의 실제 필드 부족");std::uint32_t value{};
    for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(8*byte); //선택된 폭 밖의 인접 필드를 읽지 않는다.
    return value;
}
std::string_view Text(std::span<const std::uint8_t> bytes,std::size_t at) { //고정 필드명과 무관하게 원문처럼 첫 NUL까지만 읽는다.
    if (at>bytes.size()) throw std::out_of_range("메타 참가자 문자열 시작 범위");const auto tail=bytes.subspan(at);std::size_t length{};
    while (length<tail.size() && tail[length]) ++length; //첫 NUL 뒤 stale BYTE는 읽거나 정리하지 않는다.
    if (length==tail.size()) throw std::out_of_range("메타 참가자 문자열 NUL 물리 부족");return {reinterpret_cast<const char*>(tail.data()),length};
}
}
// rank 인자는 weight 호출 전 push되어 남는 원문 스택 인자다. Ghidra가 누락한 다섯 번째 printf 인자를 보존한다.
void WriteNativeMetaParticipantLabel(std::span<std::uint8_t> record,std::string_view prefix,const NativeMetaParticipantNameHooks& h) {
    const auto warning=(Read(record,0x64)&0x800U) ? "~[IfortGump.L1]":"";
    const auto pattern=Need(h.translate)("~j%s%s~w%s\n~yLevel ~w%d ~yRank ~w%d\n");
    const auto rank=Read(record,0x58,1)+1U;const auto level=Need(h.weight)(record.subspan(0x54,10));const std::string name(Text(record,0x26)),guild(prefix);
    const auto destination=record.subspan(0xd8);const auto count=std::snprintf(reinterpret_cast<char*>(destination.data()),destination.size(),pattern.c_str(),guild.c_str(),warning,name.c_str(),static_cast<int>(level),static_cast<int>(rank));
    if (count<0 || static_cast<std::size_t>(count)>=destination.size()) throw std::out_of_range("메타 참가자 이름표의 실제 CS 문자열 물리 초과"); //원문 범위 밖 메모리 훼손은 실제 할당 경계에서 거부하며 앞선 복사는 되돌리지 않는다.
}
// 길드 접두사는 기록+8을 사용하며 참가자의 정제된 이름+26과 구별한다.
void RebuildNativeMetaParticipantLabel(std::span<std::uint8_t> record,const NativeMetaParticipantNameHooks& h) {
    std::string prefix="~a";prefix+=Text(record,8);prefix+=':';if (Read(record,8,1)==0) prefix.clear();WriteNativeMetaParticipantLabel(record,prefix,h);
}
void RefreshNativeMetaPlayers(ClientChallengeGlobals& challenge,const NativeMetaParticipantNameHooks& h) { //표시/NSInfo장치가CS를교체해도다음반복은같은owner의현재할당을다시읽는다.
    for (std::uint32_t index=0;index<o::kChallengeParticipantCount;++index) { //원문0..39의현재activeBYTE만검사하며이름표/표시호출뒤그기록을다시읽지않는다.
        const auto record=challenge.Participant(index);if (!(Read(record,0x64,1)&1U)) continue;RebuildNativeMetaParticipantLabel(record,h);Need(h.update)(index,0xffU);Need(h.refreshInfo)();
    }
}
// 미발견 NULL 역참조는 호스트 오류로 표시하고 active가0이면 입력 이름/장치를 읽지 않는다.
void ReceiveNativeMetaParticipantName(ClientChallengeGlobals& challenge,std::span<const std::uint8_t> packet,const NativeMetaParticipantNameHooks& h) {
    const auto participant=challenge.FindParticipantBySubscriber(Read(packet,4));if (!participant) throw std::out_of_range("88h subscriber의 실제 CS 참가자 포인터 없음");const auto record=challenge.Participant(*participant);
    if (!(Read(record,0x64,1)&1U)) return;
    for (std::size_t offset=0;;++offset) { //strcpy처럼 같은 BYTE를 읽고 바로 쓴 뒤 NUL을 검사하며 길이/송신자 선행 필터를 추가하지 않는다.
        const auto value=Read(packet,8+offset,1);if (8+offset>=record.size()) throw std::out_of_range("88h 이름 복사의 실제 CS 물리 초과");record[8+offset]=static_cast<std::uint8_t>(value);if (!value) break;
    }
    RebuildNativeMetaParticipantLabel(record,h);Need(h.update)(Read(record,0),0xffU);Need(h.refreshInfo)();
}
}
