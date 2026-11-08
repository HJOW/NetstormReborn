// CS 참가자 이름표의 원문 서식과 88h 길드 표시 수신을 같은 기록/표시 장치에 연결한다.
#pragma once
#include "client/ClientChallengeGlobals.h"
#include <string>

namespace netstorm::client { // 별도 참가자/길드 캐시를 만들지 않고 현재 CS 물리를 빌린다.
struct NativeMetaParticipantNameHooks { // 원문이 선택한 현재 번역/기술 합계/표시만 요구한다.
    std::function<std::string(std::string_view)> translate; //004de9a0의 현재 언어/실제 표시 바이트다.
    std::function<std::uint8_t(std::span<const std::uint8_t>)> weight; //0042b210의 현재 TYPE/기술 비트와 Core+5 합계다.
    std::function<void(std::uint32_t,std::uint8_t)> update; //0047d8c0의 지정한 참가자/마스크이며 전체 Poll로 대체하지 않는다.
    std::function<void()> refreshInfo; //0047cab0의 같은 현재 NSInfo Push/Attach다.
};
void WriteNativeMetaParticipantLabel(std::span<std::uint8_t> record,std::string_view prefix,const NativeMetaParticipantNameHooks& hooks); //두 생성/갱신 부모의 경고 그림·번역·rank/weight·이름 서식을 같은+d8에 쓴다. prefix는 caller가 원문대로 구성한다.
void RebuildNativeMetaParticipantLabel(std::span<std::uint8_t> record,const NativeMetaParticipantNameHooks& hooks); //0042b490 전체: 현재+8 길드를 ~a/콜론으로 감싸되 빈 길드는 빈 접두사를 사용한다.
void ReceiveNativeMetaParticipantName(ClientChallengeGlobals& challenge,std::span<const std::uint8_t> packet,const NativeMetaParticipantNameHooks& hooks); //004b4680 전체: 첫 subscriber 일치의 active만 검사해 +8 복사→서식→지정 표시→NSInfo를 수행한다.
}
