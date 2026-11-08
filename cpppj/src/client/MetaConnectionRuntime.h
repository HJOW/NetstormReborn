// World 없는 메타의 실제 TCP10/11·UDP12와 원문 연결/검색 상태를 소유한다.
#pragma once
#include "client/LocalTcpTransport.h"
#include "o/RawLocalMissionState.h"
#include "o/RawLocalServerDiscovery.h"
#include "client/MissionNetwork.h" //메타와 일반 전투가 같은 실제 TCP/UDP 소유권을 인계한다.
#include <functional>
#include <array> //같은 실제 전역87BYTE 옵션의 공유 소유권을 형식으로 보관한다.
#include <memory>
#include <string_view>

namespace netstorm::client { //같은 Client Player/CS/시계/현재 상태를 빌리는 메타 전송 부모다.
class Client; //실제 Client는 이 장기 EditorRuntime 자식보다 오래 살아야 한다.
class LocalUdpTransport; //실제UDP12의물리소유자이며구현은같은Meta부모의cpp에서결합한다.
struct NativeMetaSailHooks; //같은 CS/현재 카메라·SHP·TCP11·Rig의 동기 장치 묶음이다.
struct NativeMetaSeatHooks; //같은 실제 C프레임/포구 조회 장치의 반환 형식을 전방 선언한다.
struct NativeMetaConnectedHooks; //같은28~33 부모에 현재 HTML serial/카메라 장치만 외부에서 공급한다.
class MetaConnectionRuntime:public std::enable_shared_from_this<MetaConnectionRuntime> { //수신 중 일반 전투가 같은 callback/장치를 끝까지 pin할 수 있다.
public:
    MetaConnectionRuntime(Client& client,std::function<void(std::string_view)> progress,std::function<void(std::uint32_t)> request,std::function<void()> processState,std::function<void(std::string_view)> tell,std::function<void()> refreshInfo,std::shared_ptr<std::array<std::uint8_t,87>> options,std::function<void()> recreateHud,std::function<void(std::shared_ptr<MetaConnectionRuntime>,bool)> battleState); //생성은소켓을열지않고실제전역옵션·CS/NSInfo·전체HUD·전투전환Source 장치를같은수명으로연결한다.
    MetaConnectionRuntime(const MetaConnectionRuntime&)=delete;MetaConnectionRuntime& operator=(const MetaConnectionRuntime&)=delete; //등록된 callback의 주소/실제 socket owner를 복제하지 않는다.
    ~MetaConnectionRuntime(); //Native CloseAll 뒤 남은 자기 노드만 실제 TCP owner가 반납한다.
    o::LocalMissionParentResult Run(std::uint32_t selected); //20~27/34~38의부모와37의실제새R lan실행을연결하고공통144BYTE확정은바깥분배기에남긴다.
    o::LocalMissionParentResult RunConnected(std::uint32_t selected,const NativeMetaConnectedHooks& ui); //28~33의 같은 CS/State/포트·실제 TCP/표시/전송에 현재 HTML/카메라 장치를 결합한다.
    void Pump(); //같은 원문 slot4의 flush→수신을 World 없이 진행한다.
    bool CallbackActive() const; //동기 수신/송신 중 전체 Native 정리의 재진입을 막는다.
    MissionNetwork SharedNetwork(); //현재 안정된 TCP/UDP와 이 전체 callback owner를 반환하며 새 접속/등록표를 만들지 않는다.
    void NotifyMetaMaster(); //004b2c00의 현재 자기 flags에2000h를 저장하고 같은 TCP11로1fh/12BYTE를 전송한다.
    void SendMetaStatus(const std::function<void(std::span<std::uint8_t>)>& writePayload); //0040a300의실제12BYTE 헤더뒤caller가+4/+8과부분효과를쓰고같은11로송신한다.
    void SendMetaData(std::uint32_t key,std::span<const std::uint8_t> data,std::uint32_t count); //0042bfb0: signed160관문·FF초기화/67h생성·BYTE키/WORD길이·현재원문복사→같은11송신이다.
    void SendMetaGameLogin(); //004b62b0의 실제 master/자기 CS 기록으로7ch/28BYTE를 작성해 같은 TCP9로 전송한다.
    void SendBattleDescription(std::string_view description); //0042c040(-1,text,0)의 현재 권한 관문과 실제71h/TCP11 전송이다.
    void JoinBattle(std::uint32_t parameter); //0042be00의 원문 BYTE 전투/좌석·현재 자기 좌표 쓰기와1eh 전송을 실행한다.
    void SailPosition(std::array<std::uint32_t,2> position); //0042beb0의 송신 선행·현재 자기 재조회/좌표 쓰기·실제 Rig를 실행한다.
    void RefreshZones(); //일반 MetaRefresh(c6)의4b40c0(11)을 기존 실제16h writer/관찰기/TCP에 연결하며 접속 상태를 합성하지 않는다.
    void DescendToZone(std::int32_t zone); //004b8810/0042bf40의 실제 잠금 안내·1fh/항해·현재19 직접Run과7344BYTE 존 보관/복원을 실행한다.
private:
    void ReceiveParticipantAdd(std::span<const std::uint8_t> packet); //4b4710의 실제 메타/CS/자기Core 선택 뒤 전체42c180 생성/후행 표시를 실행한다.
    void ConstructParticipant(std::span<std::uint8_t> record,std::span<const std::uint8_t> packet); //현재 실제 World/Gump/Config/Core/카메라·Chat 장치를 전체42b580에 결합한다.
    void RefreshParticipantCore(); //48f190의 현재 일반 관문/같은 전투40참가자→실제1..8 Core 장부다.
    void ReceiveParticipantMove(std::span<const std::uint8_t> packet); //4b4820의 실제 메타/CS 진단 뒤 mutable 수신 버퍼를 전체42c2e0에 전달한다.
    void ReceiveSailError(std::span<const std::uint8_t> packet); //4b47e0의 메타/CS 진단 뒤 첫 자기 기록/25h의 실제 안내·Tell·지정 표시를 실행한다.
    void ExplodeBattleSlot(std::uint32_t battle,std::uint32_t slot); //479cf0/478720의 실제 현재 Pool/SHP/Snapshot/지면 효과·음향 장치다.
    NativeMetaSeatHooks SeatShapeDevices(); //좌석 배치와 추방 효과가 같은 실제 C 프레임/포구·복귀형 진단을 공유한다.
    NativeMetaSailHooks SailDevices(); //조회만으로 효과를 실행하지 않고 같은 현재 CS/SHP/카메라·전송/프로세스를 결합한다.
    bool Connect(std::string_view endpoint,std::uint32_t port,std::uint32_t challenge); //004b3ed0의현재 해석 뒤 TCP10/11을 실제 선택한다. 공개/DNS 주소는 기존 LAN 장치가 거부한다.
    std::span<std::uint8_t> Core(); //0048f0d0의 같은 현재 Client Core와 실제 진단을 읽는다.
    std::uint8_t Weight(std::span<const std::uint8_t> core); //0042b210의 현재 TYPE/기술 비트/level 합계다.
    void SendRootLogin(std::uint32_t connection); //004b40c0의16h/18BYTE 요청을 실제 인자10/11로 보내며 현재서버 subscriber/Core 기술 합계를 쓴다.
    void SendChallengeLogin(); //004b4480의1ch/현재Core/이름·언어·CD/진단 플래그 요청이다.
    bool CheckMetaVersion(std::uint32_t major,std::uint32_t minor); //004b4160의10.78 비교/로그·major 차이에서만 옵션/Tell 거부다.
    void ReceiveRootReply(std::span<const std::uint8_t> packet); //004b4250의 실제 현재 표/서버 목록/사용자 홈 저장과 next24·메타 갱신을 연결한다.
    void ReceiveBattleOptions(std::uint32_t sender,std::span<const std::uint8_t> packet); //004b4900 전체 부모를 현재 전역87BYTE·선택CS·공유 파생값·실제 HUD에 연결한다.
    void Progress(std::string_view source,bool protocol=false); //원문문구를한번번역하고로컬검색의고정%s만TCP/IP로치환해실제HTML에전달한다.
    o::LocalServerDiscoveryGlobals Discovery(); //같은Client의005c85a8/005c8600/005c8608실제주소세개를빌린다.
    o::LocalServerDiscoveryHooks DiscoveryHooks(); //현재UDP12/공유CRT/실제벽시각/서버창/팔레트장치를원문검색몸체에결합한다.
    Client& client_; //현재 Player/권한/연결/Config/시계/CS의 실제 소유자다.
    std::function<void(std::string_view)> progress_; //004cf570의 실제 현재 transient HTML 장치다.
    std::function<void(std::uint32_t)> request_; //실제 분배기의004b2bc0 즉시 요청 장치다.
    std::function<void()> processState_; //66h서버발견의현재36직접쓰기뒤같은실제004b6dd0을동기실행한다.
    std::function<void(std::shared_ptr<MetaConnectionRuntime>,bool)> battleState_; //1ah는다음7의현재Source만연결하고1bh는같은Source로동기Run한다. 호스트 큐를 추가하지 않는다.
    std::function<void(std::string_view)> tell_; //실제 현재 HTML Tell 장치이며 버전 차이 안내를 고정 문자열 창으로 대체하지 않는다.
    std::function<void()> refreshInfo_; //같은현재NSInfo의0047cab0이며수신전체보다오래사는실제UI를빌린다.
    std::shared_ptr<std::array<std::uint8_t,87>> options_;std::function<void()> recreateHud_; //일반/메타가 같은 전역87BYTE를 공유하고 현재004c7880을 선택한다.
    std::shared_ptr<LocalTcpTransport> tcp_; //일반 전투에 인계한 뒤에도 같은9/10/11의 실제 노드 주소를 보존한다.
    std::shared_ptr<LocalUdpTransport> udp_; //현재12도 같은 표/물리를 유지하고 마지막 실제 owner가 반납한다.
    bool pumping_{}; //호스트 재진입 보호이며 접속 성공/Native 상태를 나타내지 않는다.
};
}
