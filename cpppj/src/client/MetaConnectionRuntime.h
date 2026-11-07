// World 없는 메타의 실제 TCP10/11·UDP12와 원문 연결/검색 상태를 소유한다.
#pragma once
#include "client/LocalTcpTransport.h"
#include "o/RawLocalMissionState.h"
#include "o/RawLocalServerDiscovery.h"
#include <functional>
#include <string_view>

namespace netstorm::client { //같은 Client Player/CS/시계/현재 상태를 빌리는 메타 전송 부모다.
class Client; //실제 Client는 이 장기 EditorRuntime 자식보다 오래 살아야 한다.
class LocalUdpTransport; //실제UDP12의물리소유자이며구현은같은Meta부모의cpp에서결합한다.
class MetaConnectionRuntime {
public:
    MetaConnectionRuntime(Client& client,std::function<void(std::string_view)> progress,std::function<void(std::uint32_t)> request,std::function<void()> processState,std::function<void(std::string_view)> tell); //생성은소켓을열지않으며현재Player/표·실제Run과버전Tell 부모를연결한다.
    MetaConnectionRuntime(const MetaConnectionRuntime&)=delete;MetaConnectionRuntime& operator=(const MetaConnectionRuntime&)=delete; //등록된 callback의 주소/실제 socket owner를 복제하지 않는다.
    ~MetaConnectionRuntime(); //Native CloseAll 뒤 남은 자기 노드만 실제 TCP owner가 반납한다.
    o::LocalMissionParentResult Run(std::uint32_t selected); //20~27/34~38의부모와37의실제새R lan실행을연결하고공통144BYTE확정은바깥분배기에남긴다.
    void Pump(); //같은 원문 slot4의 flush→수신을 World 없이 진행한다.
    bool CallbackActive() const; //동기 수신/송신 중 전체 Native 정리의 재진입을 막는다.
private:
    bool Connect(std::string_view endpoint,std::uint32_t port,std::uint32_t challenge); //004b3ed0의현재 해석 뒤 TCP10/11을 실제 선택한다. 공개/DNS 주소는 기존 LAN 장치가 거부한다.
    std::span<std::uint8_t> Core(); //0048f0d0의 같은 현재 Client Core와 실제 진단을 읽는다.
    std::uint8_t Weight(std::span<const std::uint8_t> core); //0042b210의 현재 TYPE/기술 비트/level 합계다.
    void SendRootLogin(std::uint32_t connection); //004b40c0의16h/18BYTE 요청을 실제 인자10/11로 보내며 현재서버 subscriber/Core 기술 합계를 쓴다.
    void SendChallengeLogin(); //004b4480의1ch/현재Core/이름·언어·CD/진단 플래그 요청이다.
    bool CheckMetaVersion(std::uint32_t major,std::uint32_t minor); //004b4160의10.78 비교/로그·major 차이에서만 옵션/Tell 거부다.
    void ReceiveRootReply(std::span<const std::uint8_t> packet); //004b4250의 실제 현재 표/서버 목록/사용자 홈 저장과 next24·메타 갱신을 연결한다.
    void Progress(std::string_view source,bool protocol=false); //원문문구를한번번역하고로컬검색의고정%s만TCP/IP로치환해실제HTML에전달한다.
    o::LocalServerDiscoveryGlobals Discovery(); //같은Client의005c85a8/005c8600/005c8608실제주소세개를빌린다.
    o::LocalServerDiscoveryHooks DiscoveryHooks(); //현재UDP12/공유CRT/실제벽시각/서버창/팔레트장치를원문검색몸체에결합한다.
    Client& client_; //현재 Player/권한/연결/Config/시계/CS의 실제 소유자다.
    std::function<void(std::string_view)> progress_; //004cf570의 실제 현재 transient HTML 장치다.
    std::function<void(std::uint32_t)> request_; //실제 분배기의004b2bc0 즉시 요청 장치다.
    std::function<void()> processState_; //66h서버발견의현재36직접쓰기뒤같은실제004b6dd0을동기실행한다.
    std::function<void(std::string_view)> tell_; //실제 현재 HTML Tell 장치이며 버전 차이 안내를 고정 문자열 창으로 대체하지 않는다.
    std::unique_ptr<LocalTcpTransport> tcp_; //같은 Registry10/11의 실제 OS 노드/분배 owner다.
    std::unique_ptr<LocalUdpTransport> udp_; //TCP와같은Registry/등록표의실제검색12이며생성은소켓을열지않는다.
    bool pumping_{}; //호스트 재진입 보호이며 접속 성공/Native 상태를 나타내지 않는다.
};
}
