// 제공된 R.exe의 Root16h 요청과0Fh 존/친구 목록 응답을 실제 R 전송에 연결한다.
#pragma once
#include "root/ChallengeState.h"
#include "root/TransceiverTransport.h"
#include <utility>

namespace netstorm::root { // 클라이언트의 다른 크기 Challenge 저장소를 재사용하지 않는 R 전용 부모다.
class ChallengeDirectoryView final { // 실제 관리자가 소유한 현재 CCh 바이트를 읽기만 빌린다.
public:
    explicit ChallengeDirectoryView(std::span<const std::uint8_t> bytes); // 원문 전체 물리가 있는 경우만 조회를 허용한다.
    std::int32_t Count() const; // 0044d050: 참가자1..39의+64h bit1을 센다.
    std::pair<std::int32_t,std::int32_t> Statistics() const; // 00456e70: 같은 활성 참가자 수와+5eh 평균 레벨이다.
    bool ContainsSubscriber(std::uint32_t subscriber) const; // 0044d3b0: 활성 여부와 무관하게0..39의+4 DWORD를 찾는다.
    std::uint32_t Transceiver(std::uint32_t participant) const; // +2ad8의 실제40칸 토큰을 읽는다. 복합 번호의 lowBYTE가 인자다.
private:
    std::span<const std::uint8_t> bytes_; // 대여 중 관리자가 반납하거나 재할당하면 안 되는 실제 CCh 물리다.
};
struct RootDirectoryGlobals { // 옵션 재해석/서버 목록 변경 뒤에도 현재 DWORD를 직접 읽는 참조 뷰다.
    std::int32_t& groupByLevel; // 004d21f8/chalServerGroupByLevel이다.
    std::int32_t& maxPlayers; // 004d21fc/maxChalPlayersPerZone의 현재0..39 값이다.
    std::uint32_t& rootServersChecksum; // 004d0e8c의 현재 서버 목록 식별 DWORD다.
};
class RootDirectory final { // 등록 뒤 이 부모와 실제 CCh 관리자는 전송 callback보다 오래 살아야 한다.
public:
    RootDirectory(ConnectionState& state,TransceiverTransport& transport,RootDirectoryGlobals globals,std::function<std::span<const std::uint8_t>(std::uint32_t)> challenge); // 생성은 소켓/서버 상태를 만들지 않는다. challenge는 현재 토큰의 실제 물리만 돌려준다.
    RootDirectory(const RootDirectory&)=delete;RootDirectory& operator=(const RootDirectory&)=delete; // this를 빌린 실제 등록과 현재 전역 참조를 복제하지 않는다.
    void Install(); // 원문 초기화 위치에서16h 수신과16h/0Fh 관찰·버전/길이만 명시 등록한다.
private:
    ConnectionState& state_;TransceiverTransport& transport_;RootDirectoryGlobals globals_; // 같은 R의 현재 번호표/노드와 옵션 DWORD다.
    std::function<std::span<const std::uint8_t>(std::uint32_t)> challenge_; // 실제 관리자의 현재 CCh 토큰 조회 장치다. 빈 상태를 합성하지 않는다.
    ChallengeDirectoryView Challenge(std::uint32_t index) const; // 현재51칸에서 토큰을 다시 읽고 실제 물리를 조회한다.
    void Assert(std::string_view expression,std::string_view file,std::uint32_t line) const; // 선택된 원문 진단만 실제 전송의 프로세스 장치에 전달한다.
    void Observe(std::span<std::uint8_t> packet,std::uint32_t sending,bool reply) const; // 00444180/00443c20의 송신일 때만 길이 WORD를 작성한다.
    void Receive(std::int32_t sender,std::span<const std::uint8_t> request); // 00475660의 종료 관문/추천·목록/현재 연결별 응답과 pending guard다.
    void Reply(std::int32_t sender,std::span<std::uint8_t> packet); // 대기/일반/Challenge 복합 번호의 실제 현재 노드로 송신한다.
};
}
