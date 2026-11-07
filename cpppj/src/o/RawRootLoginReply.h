// 10.78의 Root0Fh 수신과 제공된10.71 R의 좁은 존 형식 호환 경계를 분리한다.
#pragma once
#include "o/RawLocalMissionState.h"
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::o { // 현재 State/메타 기록을 빌리며 전송이나 별도 목록을 소유하지 않는다.
struct RootLoginReplyGlobals { //004b4250이 쓰는 같은 장기 물리다.
    LocalMissionStateStorage state; // phase23이면 next24/+34를 쓰고 버전 거부이면 next12/+50을 쓴다.
    std::span<std::uint8_t> zones; //00546908의51×144BYTE이며 기록0은 수신 존 앞의 빈 항목이다.
    std::uint32_t& refresh; //005c85d8의 실제 현재 갱신 예약이다.
};
struct RootLoginReplyHooks { // 선택된 원문 효과만 실제 호출 위치에서 요구한다.
    std::function<bool(std::uint32_t,std::uint32_t)> version; //004b4160의 로그/옵션/MetaVerDiff 안내를 수행한다.
    std::function<void()> clearZones; //0042b1b0의 현재 마커 삭제/표 정리다.
    std::function<void(std::uint32_t)> checksum; //004594a0의 phase+44 서버 행 값 갱신이다.
    std::function<void(std::string_view)> server; //00458fe0(text,1)의 현재 목록 추가/기존 값 보존이다.
    std::function<void()> saveServers; //00458e30의 현재 목록을 사용자 홈에 저장한다.
};
void ReceiveRootLoginReply(std::span<const std::uint8_t> packet,RootLoginReplyGlobals globals,const RootLoginReplyHooks& hooks); // 네트워크 바이트 순서의 서버 목록→존→친구→현재 phase 분기를 원문 순서로 수행한다.
void ObserveRootLoginReply(std::span<std::uint8_t> packet,std::uint32_t sending,const std::function<void(std::string_view,std::uint32_t)>& assertion); //0040a070의47BYTE 송신 길이/세 진단이며 수신 방향은 무효과다.
}
