// 기존 Editor의004e22c0 로컬 양끝과004e28b0의 비동기 수신 순서만 소유한다.
#pragma once
#include "zacket/LocalMissionTransport.h"
#include "o/RawConnectionRegistry.h"

namespace netstorm::client { // 게임 계산은 등록된 기존 패킷 처리기에 맡기는 실제 Client 전송 경계다.
// 원문단일listener/client9수명에실제처리기를주입하며로그인/Player번호재배치를추가하지않는다.
class EditorLocalTransport {
public:
    const std::shared_ptr<o::RawConnectionRegistry>& Registry() const; //같은 실제 Client 연결표의 정체성을 Session 설치 전에 확인한다.
    EditorLocalTransport(std::span<const std::uint8_t> players,const bool& server,o::PacketDispatchHooks hooks,std::shared_ptr<o::RawConnectionRegistry> registry={}); //같은 실제 Client 표를 빌리며 독립 검사만 별도 표를 소유한다.
    ~EditorLocalTransport(); //호스트 owner 최종 반납 전에 자기 노드의 남은 별칭/장치를 떼며 Native 정리 성공을 합성하지 않는다.
    EditorLocalTransport(const EditorLocalTransport&)=delete; // 콜백이 빌리는 연결 주소와 큐를 복제하지 않는다.
    EditorLocalTransport& operator=(const EditorLocalTransport&)=delete; // 생존 연결의 장치 참조를 교체하지 않는다.
    void Open(std::uint32_t port); // 로그인 등록 없이 listener→client9를 만들며 서버 모드·Player 성공 값은 쓰지 않는다.
    bool Send(std::int32_t destination,std::span<const std::uint8_t> packet); // 현재9/accepted -1 끝의 단일 바이트를 반대 큐에 쓰며 동기 수신하지 않는다.
    bool SendMutable(std::int32_t destination,std::span<std::uint8_t> packet); // 길이를 쓰는 실제 observer에 같은 원시 packet this를 공급한다.
    std::size_t Poll(); // listener 수락→pending 한 패킷→client9 전체 패킷을 같은 등록표로 분배한다.
    void Disconnect(std::int32_t destination); // 현재 끝에 종료를 기록하고 가상 처리 중이면 실제 최외곽 복귀까지 반납을 미룬다.
    void CloseListener(); // 새 accepted를 만들던 listener만 실제close/release로반납한다. 양끝을 대신 종료하지 않는다.
    o::RawPacketDispatch& Dispatch(); // 건설/이동/파괴의 실제 기존 처리기를 같은 표에 등록한다.
    std::span<const std::uint8_t> Players() const; // 연결이 빌리는 같은 Player9 물리의 정체성을 Session 대여 전에 확인한다.
    o::PacketConnection& Listener(); // 원문listener의별도초기시각/소멸장치를현재수명과연결한다.
    o::PacketConnection& Client(); // 등록9의 고정 연결 저장소다. 반납 뒤 조회는 상태 관측용이며 송신 권한은 아니다.
    o::PacketConnection& Pending(); // 수락된 음수-1의 고정 저장소다. 로그인 owner로 재배치하지 않는다.
    bool ClientAlive() const; // 현재 transceivers[9]의 존재만 읽는다.
    bool PendingAlive() const; // 현재 pending[-1]의 존재만 읽는다.
    bool ListenerAlive() const; // 현재 listeningZLocal의 존재만 읽는다.
    std::uint32_t Port() const; // 이 로컬 수명이 사용한 실제 설정 포트다.
    std::size_t QueuedFrames() const; // 아직 폴링되지 않은 두 방향의 단일 프레임 수다.
private:
    bool SendPacket(std::int32_t destination,std::span<const std::uint8_t> packet,std::span<std::uint8_t> writable); // 같은 연결/observer/큐 몸체를 기존 방향 전용·실제 this 작성기가 함께 사용한다.
    o::PacketDispatchHooks WrappedHooks(); // 기존 분배의 close/release를 같은 연결 소유권 반납과 이어 준다.
    std::uint32_t Track(o::PacketConnection& connection); //같은 실제 노드의 close/free와 현재 Client 표를 연결한다.
    std::uint32_t Token(const o::PacketConnection& connection) const; //현재 양끝의 숫자 식별자를 순수 조회한다.
    void Initialize(o::PacketConnection& connection,std::uint32_t sender); //004df2c0의 두 별도 시각 조회와 unbuffered 초기0x24를 쓴다.
    void Accept(); //004e02d0의 첫 빈 pending칸-1에 실제 한 accepted끝을 만든다.
    void Connected(o::PacketConnection& connection); //004dfc00의 bit0x20 해제→004dfa40의 선택된type1 분배다.
    void Terminated(o::PacketConnection& connection); //zlocal상대소멸뒤004df9f0의한번뿐인type2분배를재사용한다.
    bool PeerLost(const o::PacketConnection& connection) const; //004e1770이상대socket에기록한0x1000의같은수명상태다.
    void SocketClosed(o::PacketConnection& connection); //실제close복귀뒤상대의peer소멸표식과더는읽을수없는바이트를정리한다.
    std::vector<std::uint8_t>& ReceiveBuffer(o::PacketConnection& connection); //같은끝의실제현재수신버퍼를빌려합성type1/2의보존BYTE도이어쓴다.
    void Retire(o::PacketConnection& connection); //depth0인 선택된 끝의 실제 close→release만 실행하며 이미 닫힌 수신 끝도 이 경계에서 반납한다.
    struct ReceiveResult { //원문길이0sentinel의반복중단과이미소비한실제바이트를별도로기록한다.
        bool consumed{}; //한프레임을socket큐에서꺼냈는지다.무효/거부도소비에는포함한다.
        bool again{}; //004e28b0가같은등록끝의가상수신을다시호출할실제정상packet반환인지다.
    };
    ReceiveResult Receive(bool pending); // 한 큐에서 실제 소유 프레임 하나만 꺼내 현재 등록표를 분배한다.
    bool Alive(const o::PacketConnection& connection) const; // 같은 고정 두 연결의 현재 존재를 읽는다.
    o::PacketDispatchHooks hooks_; // 상위가 제공한 실제 시계/기록/종료/반납 장치이며 이 객체보다 오래 살아야 한다.
    std::span<const std::uint8_t> players_; // Session과 같은 실제 Player9인지 확인할 읽기 뷰이며 사본을 만들지 않는다.
    std::shared_ptr<o::RawConnectionRegistry> registry_; //실제 생성/수락/정리가 함께 읽는 Client 장기 표다.
    zacket::LocalMissionTransport dispatcher_; // Open/Login/자체 큐를 사용하지 않고 기존 DispatchFrame/등록표만 재사용한다.
    o::PacketConnection listener_,client_,pending_; // 원문 listener와 별개 client9/accepted-1의 실제 필드 저장소다.
    std::vector<std::uint8_t> clientReceive_,pendingReceive_; //현재수신원문을각끝이보유하며004df9f0은헤더+1/+2만덮고+0을보존한다.
    std::deque<std::vector<std::uint8_t>> toPending_,toClient_; // unbuffered 송신의 반대쪽 실제 단일 프레임 소유 큐다.
    bool listenerAlive_{},clientAlive_{},pendingAlive_{},accepted_{},polling_{}; // 존재·첫 수락·호스트 재진입 보호이며 게임 상태/로그인 성공 값이 아니다.
    bool clientPeerLost_{},pendingPeerLost_{}; //PacketConnection+34와다른원문socket의0x1000상태를각실제끝에보관한다.
    std::uint32_t port_{}; //004e22c0이 실제 설정에서 받은 두 끝의 같은 포트다.
    std::uint32_t clientToken_{},pendingToken_{}; //같은 실제 양끝의 현재 세대 토큰이며 재실행 때 새로 발급한다.
};
}
