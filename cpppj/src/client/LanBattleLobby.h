// 종료된 공식 매칭 서버 대신 로컬 TCP 대기실에서 실제 참가자 명단을 합의한다.
#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace netstorm::client { // 대기실 제어 연결은 Native 게임 Player/Ready/서버 권한을 쓰지 않는다.
//004b4be0이 소비할71BYTE client 기록의 실제 사용자 선택값이다.
struct LanLobbyParticipant {
    std::uint8_t slot{}; // 원문 Zone의0..7 번호이며 실제 Player는 State7에서 slot+1로 설치한다.
    std::uint32_t subscriber{}; // 현재 Fort에서 읽은 실제 subscriber이며 중복/0은 가입을 거부한다.
    std::string name,address; // 원문 표시 부호화 이름31BYTE 이내와 IPv4:게임포트29BYTE 이내다.
};
// 대기실 합의/시작 ACK가 끝난 뒤 외부 State7 부모에 넘기는 요청이다.
struct LanLobbyBattle {
    bool host{}; // 이 대기실의 역할이며 Native server/boss 값을 대신하지 않는다.
    std::vector<std::uint8_t> zone; // host만 실제 명단의1a 이미지를 소유하고 client는 Zone 없이 서버에 접속한다.
    std::array<std::uint8_t,20> serverAddress{}; // client가 실제 접속한 서버 IPv4 주소이며 host의 내부 자기끝은 외부 부모가 선택한다.
    std::uint32_t gamePort{}; // 실제 원격 게임 접속에 사용할 같은1..65535 포트다.
};
// 별도 로컬 대기실 포트/등록표를 소유하고 기존 LocalTcpTransport의 실제 TCP 프레임·수명을 재사용한다.
class LanBattleLobby final {
public:
    LanBattleLobby(std::span<const std::uint8_t> players,std::shared_ptr<void> lifetime,std::function<double()> wallSeconds); // 현재 Player9 읽기 주소와 장기 owner를 함께 빌린다. 생성은 소켓을 열지 않는다.
    ~LanBattleLobby(); // 자기 대기실 socket/등록표만 닫으며 실제 게임의 연결표/Player를 반납하지 않는다.
    LanBattleLobby(const LanBattleLobby&)=delete; // 같은 socket/등록 callback을 복제하지 않는다.
    LanBattleLobby& operator=(const LanBattleLobby&)=delete; // 살아 있는 소유권의 대입을 금지한다.
    void Host(std::uint32_t subscriber,std::string_view name,std::uint32_t lobbyPort,std::uint32_t gamePort,std::uint32_t zoneId); // 실제 host 자신 한 명으로 열고 원격 Join을 기다린다.
    void Join(std::uint32_t subscriber,std::string_view name,std::string_view address,std::uint32_t lobbyPort,std::uint32_t gamePort); // 사설/loopback/link-local IPv4 literal로만 접속한다.
    void Start(); // host가 현재 합의된 명단을 고정하고 실제 모든 참가자의 시작 ACK를 기다린다.
    void Poll(); // 비동기 TCP/명단/timeout만 처리한다. World/State 전환은 실행하지 않는다.
    void Close(); // 현재 자기 socket만 닫고 이후 폴링을 멈춘다.
    bool IsHost() const; // 대기실 역할을 읽는다.
    bool Starting() const; // 고정 명단/시작 ACK 단계인지 읽는다.
    std::span<const LanLobbyParticipant> Participants() const; // 현재 합의된 명단이며 Native Player 기록의 사본이라고 보고하지 않는다.
    std::string_view Error() const; // 실제 가입/연결/timeout 실패를 표시할 UTF-8 문장이다.
    std::optional<LanLobbyBattle> TakeBattle(); // 실제 ACK/송신 drain이 끝난 요청만 한 번 인계한다.
private:
    struct Impl; // 고정 실제 TCP 노드와 제어 프로토콜/명단 수명의 소유자다.
    std::unique_ptr<Impl> impl_; // callback보다 먼저 명시 Close하고 마지막에 반납한다.
};
}
