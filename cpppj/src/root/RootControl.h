// 실제 R 인사와 LAN 공지 제어를 같은 일반/대기 연결표·전송에 연결한다.
#pragma once
#include "root/TransceiverTransport.h"

namespace netstorm::root { //00475070/475040/475f90/475e60의 실제 Root 제어 부모다.
struct Options;class AddressCodec;class Clock; // 같은 현재 포트 옵션/기존 주소 parser·실제 게임 시계를 참조로 빌린다.
class RootControl final { // 이 부모는 등록 callback 실행보다 오래 살며 참조한 현재 옵션/장치는 이 부모보다 오래 살아야 한다.
public:
    RootControl(ConnectionState& state,TransceiverTransport& transport,Options& options,AddressCodec& addresses,Clock& clock,std::function<void(std::int32_t,std::string_view)> log); // 생성은 서버·소켓/파일/OS 작업을 수행하지 않으며 같은 실제 로그·게임 시계를 빌린다.
    RootControl(const RootControl&)=delete;RootControl& operator=(const RootControl&)=delete; // 같은 표/등록 this와 현재 장치 참조를 복제하지 않는다.
    void Install(); // 원문의0b/0c 가변 관찰과11/12/66h 고정12·버전1/정책0·실제 nullable 수신을 현재 표에 등록한다.
    void Announce(); //00475e60의 동일66h 명령1/현재basePort를 현재 UDP→IPX 번호의 각 노드에3번 보낸다. 시작 부모와66h 요청이 공유한다.
    std::uint32_t Connect(const std::string& text,std::uint32_t capacity=512); //00474920의 모드 진단/두 실제 CaptureGame·번호/연결·시간 로그다. 현재 IPv4/client TCP 장치 범위만 허용한다.
private:
    ConnectionState& state_;TransceiverTransport& transport_;Options& options_;AddressCodec& addresses_; // 같은 현재 R 연결 번호/포트·전송·주소 장치다.
    Clock& clock_;std::function<void(std::int32_t,std::string_view)> log_; // 같은 원문 CaptureGame과 완성된 로그의 실제 프로세스 장치다.
    void CheckExit(); // 원문 exitRequested가 비영일 때 실제_exit0 장치를 호출하며 정상 복귀는 거부한다.
    void Assert(std::string_view expression,std::string_view file,std::uint32_t line); // 원문 표현식/파일·줄의 선택 진단만 실제 프로세스 장치에 전달한다.
    void ObserveGreeting(std::span<std::uint8_t> packet,std::uint32_t sending,bool reply); //00443a00/443b10의extraDataLen DWORD/진단 뒤 최종WORD 길이+12를 쓴다.
    void ReceiveGreeting(std::int32_t sender,o::PacketConnection* connection); //00475070의 현재상위조회/실제 연결 동일성 진단과10.71·extra0 응답/같은노드flush다.
    void ReceiveDiscovery(std::int32_t sender,std::span<const std::uint8_t> packet); //00475f90의 현재Root명령0 재공지/비Root 일반종료·실제pending sender guard다.
    void ReceiveDiagnostic(std::int32_t sender,std::span<const std::uint8_t> packet); //00475260의 exit/diag 관문·실제pending guard/역방향 TCP·같은11h 응답/flush·현재입력 로그다.
    void ReceiveDiagnosticRequest(std::int32_t sender,o::PacketConnection* connection); //00475580의 diagRecv/상위조회·동일성/11h code1 응답·flush와 현재 주소 로그다. 원문에 없는 exit 관문은 추가하지 않는다.
};
}
