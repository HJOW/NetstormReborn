// 원본71h 전투 설명 수신의 부분 복사·정제·실제 표시 갱신 순서를 복원한다.
#pragma once
#include "client/ClientChallengeGlobals.h"

namespace netstorm::client { // 실제 CS와 도달한 표시 장치를 동기 호출 동안만 빌린다.
struct NativeMetaBattleDescriptionHooks {
    std::function<void(std::string_view)> log; //수준9의 무효 전투/설명 길이 진단이다.
    std::function<void(std::uint32_t)> refreshObject; //현재 전투 기록+20 SID의 실제 가상88이며 복사/정제 뒤에 호출한다.
    std::function<void(std::uint32_t)> updateBattle; //00479c40의 현재 packet 전투 번호 갱신이다.
};
void ReceiveNativeMetaBattleDescription(ClientChallengeGlobals& challenge,std::span<const std::uint8_t> packet,const NativeMetaBattleDescriptionHooks& hooks); //0042cad0 전체이며 길이0의 f8 NUL 쓰기도 보존한다.
}
