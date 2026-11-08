// 1ah/1bh의 전체 전투 시작 수신과 같은 실제 패킷 observer/장기 State 저장소를 연결한다.
#pragma once
#include "o/RawCheckingFlag.h"
#include "o/RawLocalMissionState.h"
#include <string_view>

namespace netstorm::o { //실제 소켓·FILE·복제 장부는 Client가 공급하며 성공 상태를 합성하지 않는다.
struct StateBattleStartGlobals { //콜백 복귀 뒤에도 원문과 같은 현재 전역을 다시 읽는다.
    LocalMissionStateStorage state; //같은 old/phase/immediate/current/next 144BYTE 블록이다.
    std::uint32_t& zonePending; //005c85c0의 실제 예약 수이며 State7 캡처/정리와 공유한다.
    const std::uint32_t& gamePort; //005318e8의 현재 게임 포트다.
    const std::uint32_t& subscriber; //00540c74의 현재 자기 subscriber다.
    const std::uint32_t& recordRequested; //0054dbac의 현재 요청 DWORD이며 FILE enabled와 다르다.
    RawCheckingFlag checking; //005e4794의 전체 현재 DWORD를 읽는 비소유 뷰다.
};
struct StateBattleStartHooks { //선택된 원문 위치에서만 실제 외부 장치를 요구한다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; //진단 뒤 복귀하여 같은 입력을 계속 처리한다.
    std::function<std::uint32_t(std::span<std::uint8_t>)> duplicate; //같은 mutable 패킷의 observer(1)→가변 복제/장부 token이다.
    std::function<void(std::string_view)> openRecord; //현재 이름으로 같은 Client FILE을 닫고 새로 연다.
    std::function<void(std::int32_t)> disconnect; //현재 실제10/11을 순서대로 닫는다.
    std::function<void()> run; //1bh의 현재7 쓰기 뒤 같은 실제 상태 처리기를 동기 실행한다.
};
void ObserveBattleStartZone(std::span<std::uint8_t> packet,std::uint32_t sending,const StateBattleStartHooks& hooks); //40a1e0: sending0은 읽기0이며 진단 복귀 뒤 현재 BYTE14로15+71*n을 쓴다.
void ReceiveBattleStartZone(std::span<std::uint8_t> packet,StateBattleStartGlobals globals,const StateBattleStartHooks& hooks); //4b4df0: 진단→next7/주소/포트→복제/예약→선택적 기록Open→10/11 정리다.
void ReceiveBattleStartPeer(std::span<const std::uint8_t> packet,StateBattleStartGlobals globals,const bool& server,const StateBattleStartHooks& hooks); //4b8b20: server/예약 조기반환→진단/기록→현재 주소/포트/7→직접Run이다.
}
