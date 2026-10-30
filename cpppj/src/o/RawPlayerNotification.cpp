// 원문 Player.cpp 프레임말 알림과 Ch 기술 합계를 실제 폭/순서로 계산한다.
#include "o/RawPlayerNotification.h"
#include <bit>
#include <stdexcept>
namespace netstorm::o { //포획/희생 자체를 실행하지 않는 별도 대기 결과 소비 부모다.
namespace { //선택된 장치와 원래 단일 메모리 접근 폭만 검증한다.
template<class F> const F& Need(const F& function) { if (!function) throw std::logic_error("Player 알림의 선택된 실제 하위 미연결");return function; } //미선택 효과를 빈 성공으로 대신하지 않는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset) { //원래 정렬되지 않은 DWORD를 읽는다.
    if (offset>bytes.size() || bytes.size()-offset<4) throw std::out_of_range("Player 알림의 실제 DWORD 물리 부족");return bytes[offset]|(std::uint32_t(bytes[offset+1])<<8)|(std::uint32_t(bytes[offset+2])<<16)|(std::uint32_t(bytes[offset+3])<<24);
}
void ClearPending(std::span<std::uint8_t> record) { //앞선 안내/획득/갱신이 끝난 뒤 캡처한 같은 기록만 지운다.
    for (std::size_t byte=0;byte<4;++byte) record[0xa0+byte]=0; //이웃 기록과 Core BYTE는 보존한다.
}
}
// Core 마스크와 마지막 +5 BYTE는 원문처럼 TYPE 조회 뒤에도 현재 물리에서 다시 읽는다.
std::uint8_t CoreTechnologyWeight(std::span<const std::uint8_t> core,const CoreTechnologyWeightHooks& hooks) {
    if (core.size()<10) throw std::out_of_range("Core 기술 합계의 실제10BYTE 누락");std::uint32_t total{};
    for (std::uint32_t bit=0;bit<32;++bit) { //원래32개 기술 비트를 낮은 번호부터 순회한다.
        if (!(Read(core,6)&(1U<<bit))) continue;
        for (std::int32_t index=0;index<Need(hooks.count)();++index) { //불일치 뒤 현재 개수를 다시 조회하며 첫 일치에서 끝난다.
            const auto type=Need(hooks.type)(index);if (type.bit==static_cast<std::int32_t>(bit)) { total+=1U+std::bit_cast<std::uint32_t>(type.level);break; }
        }
    }
    return static_cast<std::uint8_t>(total+core[5]);
}
// EDX는 Player+a0에서 캡처한 대기 타입이다. 48fc60은 EAX의 SID만 반환하며 EDX를 보존한다.
void NotifyPendingPlayerResult(OriginalEdition edition,const PlayerNotificationHooks& hooks) {
    if (edition!=OriginalEdition::Patch1078) throw std::invalid_argument("Player 프레임말 알림은 실제10.78 경계 필요");
    if (!Need(hooks.global)(0x595344) || !hooks.global(0x594fbc)) return;const auto owner=hooks.global(0x540c70);auto players=Need(hooks.players)();const auto offset=std::size_t(owner)*180;
    if (offset>players.size() || players.size()-offset<180) throw std::out_of_range("Player 알림의 현재 owner 기록 부족");auto record=players.subspan(offset,180);const auto pending=Read(record,0xa0);if (!pending || !Read(record,0x9c)) return;
    const auto technology=Need(hooks.technology)();
    if (pending==2 || pending==3) {
        const auto previous=record[0x71];const auto next=pending-1U;record[0x71]=static_cast<std::uint8_t>(next>previous ? next : previous);
        if (previous<record[0x71]) { Need(hooks.tell)("AltarUpgrade");ClearPending(record);return; }
    }else {
        bool grant{};
        if (pending==hooks.global(0x5411cc)) { grant=Need(hooks.weight)(record.subspan(0x6c,10))==43;record[0x71]=0; }
        else if (pending==hooks.global(0x5412c0)) { record[0x71]=0;ClearPending(record);return; }
        else grant=Need(hooks.contained)(technology,4,pending,0)==0;
        if (grant) { const auto currentOwner=hooks.global(0x540c70);const auto currentPending=Read(record,0xa0);Need(hooks.grant)(currentPending,currentOwner);Need(hooks.refresh)(); }
    }
    ClearPending(record);
}
}
