//004b8ac0의1dh 응답 필드·버전 검사와 동기 Run 순서를 복원한다.
#pragma once
#include "o/RawLocalMissionState.h"
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::o { // 현재 Client의 같은 State/Subscriber를 호출 동안 빌린다.
struct ChallengeLoginReplyHooks { // 실제 등록 시 이 하위 장치들이 수신 전체보다 오래 살아야 한다.
    std::function<void(std::string_view)> log; //원문 수준1dh의 현재 응답코드 문구를 기록한다.
    std::function<bool(std::uint32_t,std::uint32_t)> version; //004b4160의 실제 메타 버전 검사다.
    std::function<void()> run; //현재블록을 직접28/12로 쓴 뒤 실제004b6dd0을 동기 호출한다.
};
void ReceiveChallengeLoginReply(std::span<const std::uint8_t> packet,LocalMissionStateStorage state,std::uint32_t& subscriber,const ChallengeLoginReplyHooks& hooks); //sender/현재phase 필터나 next/immediate 요청으로 바꾸지 않는 원문 본체다.
}
