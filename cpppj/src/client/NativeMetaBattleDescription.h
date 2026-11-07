// 원본71h 전투 설명 수신의 부분 복사·정제·실제 표시 갱신 순서를 복원한다.
#pragma once
#include "client/ClientChallengeGlobals.h"
#include <optional>
#include <string>

namespace netstorm::client { // 실제 CS와 도달한 표시 장치를 동기 호출 동안만 빌린다.
struct NativeMetaBattleDescriptionHooks {
    std::function<void(std::string_view)> log; //수준9의 무효 전투/설명 길이 진단이다.
    std::function<void(std::uint32_t)> refreshObject; //현재 전투 기록+20 SID의 실제 가상88이며 복사/정제 뒤에 호출한다.
    std::function<void(std::uint32_t)> updateBattle; //00479c40의 현재 packet 전투 번호 갱신이다.
};
void ReceiveNativeMetaBattleDescription(ClientChallengeGlobals& challenge,std::span<const std::uint8_t> packet,const NativeMetaBattleDescriptionHooks& hooks); //0042cad0 전체이며 길이0의 f8 NUL 쓰기도 보존한다.
struct NativeMetaDescriptionSendHooks { //설명 확정의 현재 권한·진단·안내와 실제 TCP11 전송을 빌린다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; //Ch.cpp580의 복귀형 진단이다.
    std::function<void()> denied; //권한 없는 설명 변경의 원문 Tell을 연다.
    std::function<void(std::span<std::uint8_t>)> send; //71h의 실제 가변 길이 관찰기와 전송을 실행한다.
};
void SendNativeMetaBattleDescription(ClientChallengeGlobals& challenge,const std::uint32_t& localOwner,std::int32_t battle,std::optional<std::string_view> description,std::uint32_t parameter,const NativeMetaDescriptionSendHooks& hooks); //0042c040: -1은 현재 자기 전투이며 권한 거부 뒤 문자열을 읽지 않는다.
std::string ConfirmNativeMetaBattleDescription(std::span<const char> copied); //004c92c0의256BYTE 입력/NUL/LF 순서를 보존하고 후행 공백을 임의 제거하지 않는다.
}
