// 적용 전 준비본: Login이 고정 zlocal 양끝 대신 현재 ID 기반 실제 전송을 빌리는 좁은 계약이다.
#pragma once
#include "o/RawPacketDispatch.h"
#include "o/RawConnectionRegistry.h"
#include "o/RawBattleBroadcast.h"
#include <memory>
#include <string>
#include <string_view>

namespace netstorm::zacket { class LocalMissionTransport; } // 기존 단독 미션의 실제 두 끝 owner다.
namespace netstorm::client { // 이 API 생성만으로 listener/소켓/게임 상태를 만들지 않는다.
class LocalTcpTransport;class EditorLocalTransport; // 실제 네트워크/내부 큐 owner이며 별도 Login을 합성하지 않는다.
class CompositeMissionTransport; // 같은 Registry/등록표/per-ID 순서로 실제 backend를 조립하는 owner다.
std::string CampaignPeerAddressText(const std::array<std::uint8_t,20>& address); // 현재20BYTE 주소를 원문 IP/포트 표시로 변환하며 DNS를 조회하지 않는다.
// 공유 Registry/분배 등록표의 정체성과 실제 하위 호출을 함께 빌린다.
// 복합 LAN host는 별도의 전역 번호순 coordinator가 이 계약을 채워야 한다.
// 독립 backend의 Poll 두 개를 이어 호출한 값을 복합 원문 순서라고 쓰면 안 된다.
struct CampaignTransportServices {
    std::shared_ptr<void> lifetime; // 실제 backend가 callbacks보다 먼저 사라지지 않는 강한 owner다.
    std::shared_ptr<CompositeMissionTransport> composite; //실제 복합 backend만 형식을 유지해 다음 State7에 같은 내부/TCP/UDP를 인계한다.
    std::function<void()> detachSession; // 외부 장기 owner 주입 시 필수인 단일 Session 등록 대여 반납이다. callback은 현재 대여 토큰만 해제하고 예외를 던지지 않아야 한다.
    std::function<bool()> callbacksActive; // 반납 중인 콜백까지 포함한 실제 전체 Runtime 정리 진입 관문이다.
    std::shared_ptr<o::RawConnectionRegistry> registry; // Client State/Chat/cleanup과 같은 현재22/32칸이다.
    o::RawPacketDispatch* dispatch{}; // 현재 실제 등록/observer/version/길이 표다. 이 포인터의 owner는 lifetime에 포함한다.
    std::function<void(std::uint32_t)> serve; // 선택한 실제 서버 열기이며 Editor에는 Login Serve 의미가 없다.
    std::function<void(std::uint32_t)> listen; // simulconnect 등에서 서버 권한과 별개로 명시 여는 실제 LAN TCP listener다.
    std::function<void(std::span<const std::uint8_t>,std::uint32_t)> connect; // 현재 typed LAN의20BYTE 주소/포트다. 원문 State8 저장 주소의 문자열40BYTE는connectPeerText로 전달한다.
    std::function<void(std::string_view,std::uint32_t)> connectPeerText; // 원문State8의+54..7b 문자열 영역/현재포트다. 기존20BYTE typed LAN 시작과 구별한다.
    std::function<void(std::int32_t,std::int32_t)> assign; // 같은 실제 pending 노드의 재배치/buffered 설정이다.
    std::function<bool(std::int32_t,std::span<const std::uint8_t>)> send; // 이미 길이를 확정한 현재 writer의 송신이다.
    std::function<bool(std::int32_t,std::span<std::uint8_t>)> sendMutable; // 현재 observer1을 실제 한 번 호출하는 송신이다.
    std::function<void()> flush; // 원문 현재0..21 송신 순서다. Editor는 unbuffered이므로 API에서 제공하지 않는다.
    std::function<std::size_t()> poll; // listener→pending각1회→현재0..21 순서를 제공해야 한다.
    std::function<o::BattlePacketSend(std::uint32_t)> peer; // 같은 현재 ID의 실제 broadcast 송신 장치다.
    std::function<o::PacketConnection*(std::int32_t)> connection; // callback 동안만 빌리는 현재 노드다.
    std::function<bool(std::int32_t)> exists,isOpen; // 객체 존재와 실제 하위 open/상위 closed를 구별한다.
    std::function<std::int32_t(std::int32_t)> status; // 완료1/진행0/실패-1이며 buffered bit2는 읽지 않는다.
    std::function<std::array<std::uint8_t,20>(std::int32_t)> address; // 실제 현재 peer 주소20BYTE다.
    std::function<std::string(std::int32_t)> peerAddress; // 원문004e0a30(1,0,0)의 IP:port 표현이다.
    std::function<void(std::int32_t)> close; // 현재 Registry의 원문 Disconnect를 사용한다.
};
// 독립 내부 미션 backend의 실제 API만 연결한다. Open/Connect/Poll은 여기서 실행하지 않는다.
CampaignTransportServices BindInternalCampaignTransport(std::shared_ptr<zacket::LocalMissionTransport> transport);
// 독립 TCP backend의 실제 API만 연결한다. Client 전용 또는 아직 단일 backend 범위다.
// 내부 자기끝+TCP 원격끝을 동시에 갖는 host coordinator의 대체가 아니다.
CampaignTransportServices BindTcpCampaignTransport(std::shared_ptr<LocalTcpTransport> transport);
// Editor의 실제 unbuffered 내부 전송이다. 로그인 serve/connect/assign/flush는 빈 채로 남긴다.
CampaignTransportServices BindEditorCampaignTransport(std::shared_ptr<EditorLocalTransport> transport);
// 실제 Session/World 전체 owner를 pin하는 등록 대여까지 포함한 복합 전송 바인더다.
// owner는 단순 Login 표식이 아니라 callback이 참조하는 전체 Runtime/Session을 유지해야 한다.
CampaignTransportServices BindCompositeCampaignTransport(std::shared_ptr<CompositeMissionTransport> transport,const std::shared_ptr<void>& sessionOwner);
}
