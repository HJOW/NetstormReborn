// 실제 Commander의70개 시간 표본·서버 응답·빠른 절반 평균과 현재 게임 시계 보정을 복원한다.
#pragma once
#include "o/RawClockSyncReset.h"
#include "o/RawPacketDispatch.h"
#include <array>

namespace netstorm::o { // Client 수명의 같은 상태만 빌리며 네트워크/시계 소유자를 복제하지 않는다.
// 원문 BSS 표본과 진단값을 장기 보유하며 서버/Commander의 서로 다른 초기화 범위를 유지한다.
struct ClockSynchronizationData {
    std::uint32_t latencyMilliseconds{},sent{},received{}; //005c8bf0/8bf8/8bfc의 단방향 지연·송신·수신 현재 DWORD다.
    double next{}; //005c8c08의 현재 벽시각 기한이며0은 예약 없음이다.
    std::array<std::array<std::uint8_t,32>,70> samples{}; //005c8c10의 실제70×32BYTE다. 헤더/미작성4BYTE도 각 표본 안에 보존한다.
};
// 시각 조회와 이미 선택된 연결의 가상 송신만 실제 장치에서 공급한다.
struct ClockSynchronizationHooks {
    std::function<double()> gameSeconds,wallSeconds; //00460cd0/00460d70의 서로 다른 현재 시계를 조회한다.
    std::function<void(std::uint32_t,std::span<std::uint8_t>)> send; // 고정 연결 token의 실제 가상+4이며 Player/ID 관문을 다시 적용하지 않는다.
    std::function<void(PacketConnection&,std::span<std::uint8_t>,const std::array<std::uint8_t,20>*)> reply; // 수신 this의 가상+4/+8이며 주소는 선택된 호출 동안만 빌린다.
};
// 동일 전역을 사용하는 서버 응답과 Commander 측정 부모를 실제 등록표/프레임에 연결한다.
class RawClockSynchronization {
public:
    RawClockSynchronization(ClockSyncResetGlobals globals,ClockSynchronizationData& data,ClockSynchronizationHooks hooks); // 같은 연결·응답모드·게임보정 DWORD와 Client70표본을 빌린다.
    void InitializeServer(RawPacketDispatch& dispatch); //004c1e70: 등록→두 카운터0→응답모드1이며 기존 표본/연결을 지우지 않는다.
    void Attach(RawPacketDispatch& dispatch,std::uint32_t connection); //004c1eb0: 등록→표본 전체0→현재 기한→카운터/모드0→연결 순서다.
    void Receive(PacketConnection& connection,std::span<std::uint8_t> packet) const; //004c1f60: 응답이면 현재 게임시각/직접송신, 아니면 현재벽시각/동일 요청시간 첫 표본 복사다.
    void Poll() const; //004c2070: 만료 표본·빠른 절반 평균·기한별 실제 표본 송신을 순서대로 처리한다.
private:
    void Register(RawPacketDispatch& dispatch); // 같은 실제5번 receiver/빈 observer만 등록하며 version/length 표는 변경하지 않는다.
    static void ClearTimes(std::span<std::uint8_t> packet); //004c1de0:8/16/24의 세 double만0을 쓰고 헤더/미작성4BYTE는 유지한다.
    ClockSyncResetGlobals globals_;ClockSynchronizationData& data_;ClockSynchronizationHooks hooks_; // 원문 현재 저장소와 도달 위치의 실제 외부 장치다.
};
}
