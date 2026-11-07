// R의 실제 원격 명령 TCP/4세션과 메인 통신 보조 장치 접합을 복원한다.
#pragma once
#include "root/SocketDevice.h"
#include <vector>

namespace netstorm::root { //remotecommand.cpp의 바이트/장기세션을 게임 Player/패킷 노드와 분리한다.
class RemoteCommandSession; // 실제 callback은 안정된 동일 세션/현재 행을 빌린다.
enum class RemoteCommandEvent { Started,Ended,Line }; // 원문 인자0/1/실제행 포인터를 호스트 포인터 절삭 없이 구별한다.
using RemoteCommandHandler=std::function<bool(RemoteCommandEvent,RemoteCommandSession&,std::span<std::uint8_t>)>; // Line은 같은160BYTE mutable 입력이다. Started의false/Line의false는 종료하며 Ended 반환은 무시한다.
struct RemoteCommandState { // 실제 전역 세션의 현재 DWORD/버퍼와 별도 포인터 폭 사용자 값이다.
    std::uint32_t negotiation{},subnegotiation{};std::uint8_t command{}; //+0/+4/+8의 현재 IAC 상태/입력 차단/마지막 명령BYTE다.
    std::array<std::uint8_t,4800> output{};std::array<std::uint8_t,160> line{}; //+14..12d3/+12d4..1373의 최초BSS0이며 세션 재시작 때 지우지 않는다.
    std::uint32_t lineLength{},outputLength{},masking{};std::uint8_t maskCharacter{0x2a}; //+1374/+1378/+1380·+137c의 현재 길이/마스킹과 최초 '*'다.
    std::uintptr_t userData{}; //+1384의 원문 사용자 값이며 호스트 실제 포인터를 DWORD로 자르지 않는다.
};
class RemoteCommandSession final { // 서버4칸 안에서 안정된 주소로 소켓/현재 handler를 소유한다.
public:
    RemoteCommandSession()=default; // 실제 전역BSS0/e6b0의 초기 상태만 구성하고 IO를 하지 않는다.
    ~RemoteCommandSession() noexcept(false); //e720은 실제 socket 포인터가 남아 있으면 End를 호출한다. callback 예외는 숨기지 않는다.
    RemoteCommandSession(const RemoteCommandSession&)=delete;RemoteCommandSession& operator=(const RemoteCommandSession&)=delete; // 현재 socket/행·callback 주소를 복제하지 않는다.
    bool Active() const { return socket_!=nullptr; } //7fa00의 포인터 존재이며 OS open bit와 다르다.
    std::uintptr_t Handle() const; //7fa30은 socket 포인터가 없으면 INVALID_SOCKET을 반환한다.
    RemoteCommandState& State() { return state_; }const RemoteCommandState& State() const { return state_; } // 실제 handler가 현재 마스킹/사용자 값·행에 직접 접근한다.
    void Handler(std::shared_ptr<RemoteCommandHandler> handler); //7fa80의 현재 callback 교체이며 기존 실행은 lease로 수명을 유지한다.
    bool Start(std::unique_ptr<SocketDevice> socket,std::shared_ptr<RemoteCommandHandler> handler); //7e750의 실제 소켓 채택/길이·masking0/두 협상문자열 queue다. 협상상태·사용자 값은 초기화하지 않는다.
    bool End(); //7e7e0의 실제 socket 반납→NULL→현재 Ended callback이며 반환은 항상true다.
    std::uint32_t Write(std::span<const std::uint8_t> bytes); //7e8b0의 signed(outLength+len)<4801 관문/실제4800BYTE 붙이기다.
    void Print(std::string_view text); //7e870의 첫NUL 전 strlen만 현재 출력에 붙인다.
    void Printf(const char* format,...); //7e930의 printf 입력을 실제1024BYTE 지역 범위에서 서식화해 Print한다. 원문 stack 넘침은 거부한다.
    bool FlushComplete(); //7e9b0은 한 송신이 현재 길이보다 짧으면 End한다. 일반 Poll의 부분송신 보관과 구별한다.
    bool Poll(); //7ea80의 실제20BYTE 수신/원문IAC·CR/BS/에코·현재callback/부분출력 이동이다.
private:
    RemoteCommandState state_;std::unique_ptr<SocketDevice> socket_;std::shared_ptr<RemoteCommandHandler> handler_; // 같은 현재 버퍼/실제 OS 소유권·세션 callback이다.
    SocketDevice& Socket(); // 선택된 원문 NULL 소켓 접근은 명시 물리 오류다.
    bool Notify(RemoteCommandEvent event); // 현재 handler를 lease로 빌리고 Line일 때만 같은160BYTE를 전달한다.
    void Byte(std::uint8_t value); // 원문의 길이1 가상 쓰기를 현재 출력 버퍼에 전달한다.
    void Consume(std::int32_t sent); // 실제 송신 길이만 앞에서 제거하고 겹치는 나머지는 memmove로 보존한다.
};
struct RemoteCommandHooks { // 실제 프로세스의 원문 console/진단 장치만 빌린다.
    std::function<void(std::int32_t,std::string_view)> log; // 원문 printf의 완성된 줄을 level0으로 전달한다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 선택된 listening==NULL 진단333이다.
};
class RemoteCommandServer final { //004d23f8의 listener/4세션/현재 callback을 같은 수명으로 소유한다.
public:
    explicit RemoteCommandServer(RemoteCommandHooks hooks); //7ee90의 실제 비활성 전역 상태다. rootTelnetPort를 읽거나 포트를 자동으로 열지 않는다.
    ~RemoteCommandServer() noexcept(false); //7ef90의 Shutdown→현재handler해제→역순 세션 소멸이다.
    RemoteCommandServer(const RemoteCommandServer&)=delete;RemoteCommandServer& operator=(const RemoteCommandServer&)=delete; // OS 핸들/고정 세션·callback this를 복제하지 않는다.
    bool Open(std::int32_t port); //7f020의 낮은WORD 포트/실제TCP listener와 open판정이다. 실패한 listener 객체도 Shutdown까지 보관한다.
    bool Active() const; //479bb0은 생성자가 아니라 listener!=NULL/하위flags40 판정이다.
    void Handler(RemoteCommandHandler handler); //7f220의 현재 handler 저장→현재 활성 세션만 같은 handler로 교체한다.
    bool Poll(); //7f2b0의 한번accept/첫빈4칸·Started거부 종료→현재 활성4칸 Poll이며 항상true다.
    std::uintptr_t Sets(NetworkSocketSet* read,NetworkSocketSet* write,NetworkSocketSet* error); //7f510의 nullable세집합·현재listener/활성세션/중복보존·64상한과 최대핸들이다.
    bool Shutdown(); //7f140의 현재listener open일때만 활성세션End→listener반납/NULL이다. 반환은true다.
    RemoteCommandSession& Session(std::size_t index) { return sessions_.at(index); } // 실제 고정4칸 세션만 빌린다. 변경/종료 callback도 같은 주소를 쓴다.
    void Bind(NetworkHooks& hooks); // 메인 부모의 실제 auxiliaryActive/Sets(read,NULL,error)/Poll을 같은 owner에 연결한다.
private:
    RemoteCommandHooks hooks_;std::unique_ptr<SocketDevice> listener_;std::array<RemoteCommandSession,4> sessions_;std::shared_ptr<RemoteCommandHandler> handler_; // 실제 프로세스 장치와 현재 listener/4세션/서버handler다.
    std::vector<std::unique_ptr<SocketDevice>> displacedListeners_; // 중복Open 진단이 복귀하면 원문처럼 옛 소켓을 즉시 닫지 않고 소유자 종료까지 보관한다. 정상 최초Open에서는 비어 있다.
    SocketDevice& Listener(); // 원문 Poll/Sets의 NULL listener 읽기는 선택 시 명시 거부한다.
};
}
