// 21h 참가자 제거의 실제 CS/Chat/Rig/Image 수명과 현재 packet 개수 순회를 복원한다.
#pragma once
#include "client/ClientChallengeGlobals.h"
#include "o/RawCheckingFlag.h"

namespace netstorm::client { //진입 CS를 보유하며 다른 참가자/장면을 임의로 초기화하지 않는다.
struct NativeMetaParticipantRemoveHooks { //원문이 선택한 진단/로그/같은 실제 하위만 요구한다.
    o::RawCheckingFlag checking; //005e4794의 현재 전체 DWORD를 도달 위치에서 읽는 비소유 뷰다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; //Ch.cpp499/4a1의 복귀형 진단이다.
    std::function<void(std::string_view)> log; //원문 수준9의 illegal/invalid 참가자 로그다.
    std::function<void(std::uint32_t)> battle,chat; //캡처 Battle+4 ID의 표시/현재 subscriber의 실제 Chat100기록 제거다.
    std::function<void(std::uint32_t,bool)> leave; //419460(downward true)/419390(false)의 실제 Rig 검색/이미지 확보/이탈이다.
    std::function<void()> refreshInfo; //packet 순회가 끝난 뒤 항상 같은 NSInfo Push/Attach를 실행한다.
};
void ObserveNativeMetaParticipantRemove(std::span<std::uint8_t> packet,bool sending,const std::function<void(std::string_view,std::uint32_t)>& assertion); //40a390:송신 numPlayersff 진단 뒤 현재 BYTE+5로 길이를 쓴다.
void DestroyNativeMetaParticipant(std::span<std::uint8_t> record,ClientChallengeGlobals& challenge,const NativeMetaParticipantRemoveHooks& hooks); //42b980 전체:현재a0 플래그의 Rig 이탈→같은 Image 해제→115DWORD0이다.
void RemoveNativeMetaParticipant(std::span<std::uint8_t> cs,std::int32_t participant,ClientChallengeGlobals& challenge,const NativeMetaParticipantRemoveHooks& hooks); //42bc10 전체:범위/checking/active→같은 좌석/버전→Chat→실제 제거다.
void ReceiveNativeMetaParticipantRemove(ClientChallengeGlobals& challenge,std::span<const std::uint8_t> packet,const NativeMetaParticipantRemoveHooks& hooks); //42c2a0:캡처CS에서 각 제거 뒤 현재 packet BYTE 개수를 다시 읽고 마지막 NSInfo를 갱신한다.
}
