// 전투에서 State3로 돌아갈 때의 원본52h 통지와 다음 상태 예약을 제공한다.
#pragma once
#include "client/InputEvent.h"
#include "o/RawPacketDispatch.h"
#include "o/RawLocalMissionState.h"

namespace netstorm::client { // 같은 실제 Player/상태 저장소를 빌리는 전투 종료 경계다.
inline constexpr std::size_t kBattleLeavePacketBytes=5; //0052ee16의 type52/version1/length5 고정 규격이다.
void InitializeBattleLeavePacket(std::span<std::uint8_t> packet); //0040aa60처럼 앞4BYTE만 쓰고 나머지 입력 물리는 보존한다.
std::array<std::uint8_t,kBattleLeavePacketBytes> MakeBattleLeavePacket(); //State3가 생성자 뒤 다섯째 BYTE를0으로 쓴 실제 통지다.
void ReceiveBattleLeave(std::uint32_t sender,std::span<std::uint8_t> players); //004a71a0처럼 현재 sender의 Player+50 DWORD에4를 합친다.
void RegisterBattleLeavePacket(o::RawPacketDispatch& dispatch,std::span<std::uint8_t> players); //원본 빈 observer/버전1/정책0/실제 수신기를 같은 표에 등록한다.
void RequestMainMenuAfterLeave(o::LocalMissionStateStorage state); //004b3540처럼 next의 첫DWORD만3으로 예약하며 즉시 정리하지 않는다.
struct BattleAbortStateGlobals { //004b6350이 실제로 읽고 쓰는 같은 Client 전역이다.
    o::LocalMissionStateStorage state; // 다음 상태와 세 블록의+84 부가인자만 바꾼다.
    std::uint32_t& abortPending;std::uint32_t& outcome; //005c85e0과005c85ac의 별도 현재 DWORD다.
    const bool& missionPost;const std::uint32_t& editor;const bool& metamap;const bool& battle; //00594fa4/005c85a4/00594fc8/00594fbc의 현재 모드다. 실제 Editor는 DWORD 그대로 빌린다.
};
void PrepareBattleAbortState(BattleAbortStateGlobals globals,const std::function<void()>& deleteSavedState); //직접 store 뒤 실제 경로 계산/삭제 장치를 호출한다. 선택된 장치 중단도 앞선 쓰기를 보존한다.
// LeaveBattle의 뗌은 소비하고 signed 이유가0 이하이면 실제 ConfirmRetreat 안내만 연다.
// 양수 이유/다른 명령은 false로 남겨 기존 Native 예약 본문이 처리한다. Tell 실패도 현재 전투를 반납하지 않는다.
bool DispatchBattleRetreatConfirmation(const InputEvent& event,const std::function<void(std::string_view)>& tell);
}
