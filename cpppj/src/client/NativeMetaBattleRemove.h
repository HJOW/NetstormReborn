// Ch.cpp의24h 전투 삭제를 실제 Rig/CS Image와 같은 참가자 표에 적용한다.
#pragma once
#include "client/ClientChallengeGlobals.h"

namespace netstorm::client { // 전투 기록과 참가자 검사는 같은 진입 CS 물리를 유지한다.
struct NativeMetaBattleRemoveHooks {
    std::function<void(std::string_view)> log; // 수준9의 없는 전투 진단이다.
    std::function<void(std::uint32_t)> leave; //00419390의 현재 SID/높이12·지연30/kill1 Rig다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; //Ch.cpp696의 참가자 잔류 진단 후 원문 반복을 계속한다.
};
void ReceiveNativeMetaBattleRemove(ClientChallengeGlobals& challenge,std::span<const std::uint8_t> packet,const NativeMetaBattleRemoveHooks& hooks); //0042c6e0 전체:active 관문→Rig→Image 반납→73DWORD 초기화/버전→40참가자 검사다.
}
