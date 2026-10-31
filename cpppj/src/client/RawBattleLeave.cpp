// 원본0040aa60·004a71a0·004b3540의 직접 쓰기를 실제 로컬 전송/상태에 연결한다.
#include "client/RawBattleLeave.h"
#include "client/RawCampaignSession.h"
#include "client/MissionGuideInput.h"
#include <stdexcept>

namespace netstorm::client { // 패킷과 Player는 호출자가 소유한 같은 물리만 변경한다.
// 기존 패킷 작성/로컬 전송을 Editor 복귀와 메인 복귀가 같은 순서로 호출한다.
void SendBattleLeaveForState(RawCampaignSession& session) { const auto packet=MakeBattleLeavePacket();session.SendLocalPacket(9,packet); }
// 생성자0040aa60의 고정 헤더만 쓰고 원문이 지정하지 않은 본문은 호출자에게 남긴다.
void InitializeBattleLeavePacket(std::span<std::uint8_t> packet) {
    if (packet.size()<4) throw std::out_of_range("전투 종료 패킷 헤더 물리 부족");
    packet[0]=1;packet[1]=0x52;packet[2]=5;packet[3]=0;
}
// 실제 State3의 local_b44=0까지 포함하고 지정하지 않은 추가 필드를 만들지 않는다.
std::array<std::uint8_t,kBattleLeavePacketBytes> MakeBattleLeavePacket() {
    std::array<std::uint8_t,kBattleLeavePacketBytes> packet;InitializeBattleLeavePacket(packet);packet[4]=0;return packet;
}
// 원본은 패킷 본문을 읽지 않고 분배기의 현재 sender만 사용한다.
void ReceiveBattleLeave(std::uint32_t sender,std::span<std::uint8_t> players) {
    const auto at=std::size_t(sender)*180+80;if (sender>=9 || at+4>players.size()) throw std::out_of_range("전투 종료 수신 Player 물리 범위");
    players[at]|=4; // 리틀엔디언 DWORD의 비트2만 바꾸므로 나머지 세 BYTE도 그대로 보존한다.
}
// 등록된 span의 수명은 해당 Session의 수신 등록 해제보다 길어야 한다.
void RegisterBattleLeavePacket(o::RawPacketDispatch& dispatch,std::span<std::uint8_t> players) {
    dispatch.Observe(0x52,[](std::uint32_t) {});dispatch.Version(0x52,1,0);
    dispatch.Register(0x52,[players](auto sender,auto&,auto) { ReceiveBattleLeave(sender,players); });
}
// 다른 부가인자/블록을 지우지 않는 원문 단일 store다.
void RequestMainMenuAfterLeave(o::LocalMissionStateStorage state) { state.next[0]=3; }
//004b6350의 다음3/6 선택과 세+84 store 뒤에만 저장 파일 장치로 넘어간다.
void PrepareBattleAbortState(BattleAbortStateGlobals g,const std::function<void()>& deleteSavedState) {
    g.abortPending=1;
    if (!g.missionPost && !g.editor && !g.metamap) {
        if (g.battle) { if (!g.outcome) g.outcome=3;g.state.next[0]=6; } //원격 복귀 선택은 원문 정적 복원만 했으며 새 멀티플레이 검사는 하지 않는다.
    } else { g.outcome=0;g.state.next[0]=3; }
    g.state.next[33]=0;g.state.immediate[33]=0;g.state.current[33]=0;
    if (!deleteSavedState) throw std::logic_error("전투 종료의 실제 저장 경로/삭제 장치 미연결");deleteSavedState();
}
// 원문004d8035의 signed JL을 보존한다. 이유0은 이탈 실행이 아니라 확인창 요청이다.
bool DispatchBattleRetreatConfirmation(const InputEvent& event,const std::function<void(std::string_view)>& tell) {
    if ((event.code&0x03ffffffU)!=kLeaveBattleCommand) return false;
    if (event.code&InputCode::kRelease) return true;
    if (event.y>=1) return false;
    if (!tell) throw std::logic_error("전투 이탈 확인의 실제 Tell 소비자 미연결");
    tell("ConfirmRetreat");return true;
}
}
