// 원문004b32f0의 현재 Fort Subscriber 변경과 큰 효과 뒤 재조회를 복원한다.
#pragma once
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string_view>

namespace netstorm::o { // 현재 Template/handle/My를 직접 빌리는 원문 공용 계층이다.
// 큰 하위가 현재 owner/handle을 바꿀 수 있으므로 진입 때 값으로 고정하지 않는다.
struct FortSubscriberChangeHooks {
    std::function<std::uint32_t()> owner; //00540cac의 현재 슬롯이다.
    std::function<std::uint32_t(std::uint32_t)> manager; // 같은20BYTE Template 헤더 첫DWORD다.
    std::function<std::uint32_t()> fortMode; //00594fb8이며 manager 비영 뒤에만 읽는다.
    std::function<std::span<std::uint8_t>()> handle; //00540ca8이 가리키는 현재 NUL 포함 물리 저장소다.
    std::function<void(std::string_view,std::string_view)> configure; // 이름 복사 뒤 fort.mySubHandle을 쓰는004414d0이다.
    std::function<void()> title; // 설정 복귀 뒤00460730이며 My 쓰기보다 앞이다.
    std::function<void(std::uint32_t,std::uint32_t,std::span<const std::uint8_t>)> subscriber; //004bd400의 고정 인자 ID/현재 handle/현재 슬롯이다.
    std::function<void(std::uint32_t)> notify; // writer 복귀 뒤 현재 슬롯의004bd360이다.
};
// name 부재는 handle/설정/제목을 건드리지 않으며 앞선 부분 효과를 예외 때 되돌리지 않는다.
void ChangeFortSubscriber(std::uint32_t id,std::optional<std::span<const std::uint8_t>> name,std::uint32_t& my,const FortSubscriberChangeHooks& hooks);
}
