// PATCH Chatmanager의 실제 100개 기록과 채널 플래그 변경을 같은 클라이언트 저장소에서 실행한다.
#pragma once
#include "o/RiftType.h"
#include <functional>
#include <optional>
#include <span>
#include <string_view>

namespace netstorm::client {
// 원본 하위 몸체 진입을 검사에서 관찰한다. 게임 계산이나 이름 정제를 대체하지 않는다.
enum class ChatChannelCall {
    NameChanged, // 이름/flags 쓰기 뒤 실제0042b0e0 이름 정제 본체에 진입하는 위치다.
    Refresh // 이름 정제 뒤 실제004437f0 빈 몸체에 진입하는 위치이며 Add마다 두 번 발생한다.
};
// 원본 00444ee0/00445080의 기록 선택·strncpy·BYTE 플래그·실제 이름 정제를 복원한다.
class RawChatChannels {
public:
    static constexpr std::size_t kCount=100; // 원본 00558ed0..0055a18f의 기록 개수다.
    static constexpr std::size_t kStride=0x30; // 한 기록은 이름40/플래그1/미사용3/subscriber4바이트다.
    static constexpr std::size_t kBytes=kCount*kStride; // 같은 실제 기록 저장소의 필수 물리 크기다.
    // 저장소/getter/관찰 대상은 처리기보다 오래 살아야 한다. 생성자는 기록을 초기화하지 않는다.
    RawChatChannels(o::OriginalEdition edition,std::span<std::uint8_t> records,
        std::function<std::uint32_t()> mySubscriber,
        std::function<void(ChatChannelCall,std::uint32_t)> observe={});
    // 원본 Add의 실사용 인자1/2/6이다. nullopt는 NULL 이름이고 flags는 하위 BYTE만 사용한다.
    // 실제 자기 subscriber는 이름·효과를 읽기 전에 -1을 반환하며, 가득 찬 신규 기록도 -1이다.
    std::int32_t Add(std::uint32_t subscriber,std::optional<std::string_view> name,std::uint32_t flags) const;
    // AI Add(sub,name,0,0,-1,channel,-1,0)의 동일한 실사용 인자만 실제 Add로 전달한다.
    std::int32_t AddToChannel(std::uint32_t subscriber,std::string_view name,std::uint32_t channel) const;
    // 원본 Clear는 subscriber가 비제로인 기록에만 flags &= ~BYTE(mask)를 쓰며 이름/효과는 건드리지 않는다.
    void Clear(std::uint32_t mask) const;
    // 원본00445140: Subscriber 비0인 행만48BYTE 전체0으로 정리한다. 빈 행의 stale 값과 두 Subscriber 전역은 유지한다.
    void ResetOccupied() const;
private:
    // 정렬되지 않은 실제 기록+2c의 DWORD subscriber를 읽는다.
    std::uint32_t Subscriber(std::size_t index) const;
    // 원본 strncpy40처럼 NUL 뒤를0으로 채운다. 기존 기록의 선행 이름0쓰기를 숨기지 않는다.
    void CopyName(std::span<std::uint8_t> record,std::string_view name) const;
    // 0042b0e0의 금지문자 삭제와 빈 이름의 Cheat<subscriber 하위BYTE> 쓰기다.
    void NameChanged(std::uint32_t subscriber,std::span<std::uint8_t> record) const;
    // 004437f0의 실제 빈 ret 몸체를 호출하며 검사 관찰은 원본 호출 위치에서만 실행한다.
    void Refresh() const;
    std::span<std::uint8_t> records_; // 외부 실제 클라이언트가 소유하는 100×48바이트이며 사본을 만들지 않는다.
    std::function<std::uint32_t()> mySubscriber_; // 각 Add 첫 비교의 현재 00540c74를 읽는다.
    std::function<void(ChatChannelCall,std::uint32_t)> observe_; // 선택 검사 전용 진입 관찰이며 계산 대체가 아니다.
};
}
