// 25h 항해 실패의 첫 자기 참가자 선택과 원문 안내/Tell/표시 순서를 복원한다.
#pragma once
#include "client/ClientChallengeGlobals.h"
#include <functional>
#include <string>

namespace netstorm::client { //같은 현재 CS와 실제 표시 장치만 빌린다.
struct NativeMetaSailErrorHooks { //42c7f0에서 선택된 실제 효과다.
    std::function<std::string(std::string_view)> translate; //오류1~4의 원문을 한 번 번역한 표시 바이트다.
    std::function<void(std::uint32_t,std::string_view)> note; //현재 기록+b8의 같은 객체에472ad0 안내를 붙인다.
    std::function<void(std::string_view)> tell; //오류5~7의 원문 Tell 섹션을 실행한다.
    std::function<void(std::uint32_t,std::uint8_t)> update; //마지막 현재 기록+0 ID를ff로 표시한다.
};
void ReceiveNativeMetaSailError(ClientChallengeGlobals& challenge,std::span<const std::uint8_t> packet,const std::uint32_t& subscriber,const NativeMetaSailErrorHooks& hooks); //0번/비활성도 포함한 첫 subscriber 일치만 사용하며 실패를 성공으로 바꾸지 않는다.
}
