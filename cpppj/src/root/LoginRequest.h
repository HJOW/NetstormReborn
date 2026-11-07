// Root의1Ch 요청 선택과 대기 소켓 이전을 실제 CCh/전송 소유자에 연결한다.
#pragma once
#include "root/ChallengeStore.h"
#include "root/SocketDevice.h"
#include <unordered_map>

namespace netstorm::root { // CCh0044e940의 로그인 본체와 Root00476180의 앞단을 구분한다.
struct LoginRequestHooks { // 현재 프로세스 장치와 실제 CCh 로그인 본체만 빌린다.
    std::function<std::array<std::uint8_t,20>()> localAddress; // 원문 로컬 호스트 IPv4/20BYTE를 공급한다. 새 장치는 폐기 외부 도메인/DNS 접속을 추가하지 않아야 한다.
    std::function<void(std::uint32_t,std::uint32_t,std::span<std::uint8_t>)> login; // 실제 CCh 토큰/원문 고정1/현재 요청을0044e940에 전달한다. 성공 응답을 합성하지 않는다.
    std::function<void(std::int32_t,std::string_view)> log; // 실제 선택된 level/완성된 줄번호·문구다.
};
class LoginRequest final { // 등록 뒤 같은 전송/CCh 장치와 이 부모는 receiver보다 오래 살아야 한다.
public:
    LoginRequest(ConnectionState& state,TransceiverTransport& transport,ChallengeStore& challenges,LoginRequestHooks hooks); // 생성은 실제 소켓/파일/서버를 열지 않는다.
    LoginRequest(const LoginRequest&)=delete;LoginRequest& operator=(const LoginRequest&)=delete; // 현재 변경 callback과 socket 닫힘 권한을 복제하지 않는다.
    void Install(); // Root 초기화의1Ch 관찰기/버전1·정책0/가변길이와00476180 수신 부모를 등록한다.
    std::unique_ptr<SocketDevice> ClaimSocket(std::uint32_t token); // 실제 CCh 로그인만 요청+29h의 내부 토큰을 한 번 소비한다. 핸들은 DWORD로 절삭하지 않는다.
private:
    ConnectionState& state_;TransceiverTransport& transport_;ChallengeStore& challenges_; // 현재 R 번호표/실제 노드/같은 CCh 물리다.
    LoginRequestHooks hooks_; // 원문 순서에서 실제로 선택한 장치만 호출한다.
    std::unordered_map<std::uint32_t,std::unique_ptr<SocketDevice>> sockets_;std::uint32_t serial_{}; // 콜백 대여 중 이전 핸들/WSA를 보관하는 비재사용 내부 토큰이다.
    void Observe(std::span<std::uint8_t> packet,std::uint32_t sending); //004444f0의 송신 nameLen 진단/최종 길이 WORD다.
    void Receive(std::int32_t sender,o::PacketConnection& connection,std::span<const std::uint8_t> request); //00476180의 종료/모드/주소/존/현재 소켓 이전 순서다.
    void Trace(std::uint32_t line); // 원문 level29의 숫자 한 줄 로그다.
};
}
