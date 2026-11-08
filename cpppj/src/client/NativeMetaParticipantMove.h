// 22h 참가자 이동의 캡처 CS/현재 packet 쓰기와 실제 좌석 효과·항해·master/Chat 후행을 복원한다.
#pragma once
#include "client/NativeMetaSail.h"

namespace netstorm::client { //실제 전송 소유 mutable packet을 사용하며 이동 성공/ACK를 합성하지 않는다.
struct NativeMetaBattleSlotHooks { //479cf0/478720의 실제 현재 객체·SHP·지면 효과/음향 장치다.
    std::function<std::span<const std::uint8_t>(std::uint32_t)> raw; //현재 Pool의 같은50BYTE이며 free/active 필터를 더하지 않는다.
    NativeMetaSeatHooks shape; //실제 객체 TYPE의 C 프레임/원문8자리 포구를 읽는다.
    std::function<void(float,float)> ground,sound; //기존470ac0의 실제 지면 효과 뒤 explodeSlot.wav(원래 좌표,0)다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; //Metadisplay737/1a5의 복귀형 진단이다.
};
struct NativeMetaParticipantMoveHooks { //42c2e0의 선택된 실제 하위/현재 전역 조회만 제공한다.
    std::function<std::uint32_t()> owner; //40da40의 현재 local owner이며 초기 캡처와 음향 뒤 재조회가 다르다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; //원문 Ch.cpp의 현재 battle/좌석 진단이다.
    std::function<void(std::uint32_t,std::uint32_t)> kick; //진입 옛 전투/현재 슬롯에 실제479cf0 효과를 요청한다.
    std::function<void(std::uint32_t)> battle; //캡처 Battle+4 ID의 실제 특정 표시 갱신이다.
    std::function<void()> kickedSound,dropSound,music; //kickedoff priority1/dropPiece 기본/anticipation 실제 음악 요청이다.
    std::function<void(float,float)> thunder; //같은 참가자의 당시 위치에서 thundercrack.wav를 재생한다.
    std::function<std::array<float,2>(std::span<const std::uint8_t>,std::uint32_t,std::uint32_t)> seat; //캡처 참가자와 현재 전투 포구/카메라의 기존42bd10이다.
    std::function<void(std::span<const std::uint8_t>)> animate; //현재 좌표를 쓴 같은 참가자의 실제419520 Rig다.
    std::function<void(std::span<std::uint8_t>)> resetOptions; //같은 CS Battle+59의 실제87BYTE 기본값을41cb90으로 초기화한다.
    std::function<void(std::uint32_t,std::span<const std::uint8_t>)> sendOptions; //현재packet 전투를 다시 읽어 같은67h/87BYTE를 송신한다.
    std::function<void(std::uint32_t)> chatMode;std::function<void()> chatMembers; //42f3d0(0/1)과 접두 변경 없는42f330을 구별한다.
    std::function<void()> refreshInfo,refreshCore; //조건부 NSInfo 뒤 지정 참가자 표시/현재48f190 장부다.
    std::function<void(std::uint32_t,std::uint8_t)> update; //현재packet BYTE 참가자만ff로 표시한다.
    std::function<void(std::uint32_t)> rebuildName; //마지막에는 현재CS의 같은 BYTE 참가자 이름표 서식만 갱신한다.
};
void ExplodeNativeMetaBattleSlot(ClientChallengeGlobals& challenge,std::int32_t battle,std::int32_t slot,const RawCameraViewState& snapshot,const NativeMetaBattleSlotHooks& hooks); //479cf0/478720 전체:현재전투 유효진단→객체TYPE/SHP포구→snapshot 위치 보정→같은 지면/음향이다.
void ReceiveNativeMetaParticipantMove(ClientChallengeGlobals& challenge,std::span<std::uint8_t> packet,const std::uint32_t& subscriber,const std::uint32_t& known,const NativeMetaParticipantMoveHooks& hooks); //42c2e0 전체이며 packet5 bit8을 원문 중간 위치에서 직접 지운다.
}
