// 실제 R CCh의 참가자 로그인/명단·메시지·종료를 같은 저장소/전송에 연결한다.
#pragma once
#include "root/LoginRequest.h"
#include "root/Configuration.h"

namespace netstorm::root { // CCh0044e940/0044e0d0의 현재 부모와 실제 패킷 작성기를 소유한다.
struct ChallengeMessages { // 실제 R 프로세스가 뉴스 파일 로드 결과를 갱신하는 같은 BSS다.
    std::optional<std::string> news;std::uint32_t newsChecksum{}; //004d05c4/c8의 NULL/소유 문자열과 현재 checksum이다.
};
struct ChallengeRuntimeHooks { // 실제 상위 표/파일/주소·Root 라우팅만 장치로 빌린다.
    std::function<std::unique_ptr<SocketDevice>(std::uint32_t)> claimSocket; // LoginRequest의 내부+29h 토큰을 한 번 소비한다.
    std::function<bool(std::uint32_t)> blockedSubscriber,markedSubscriber; //004d7220/004d723c의 현재 회원 여부이며 서로 다른 표다.
    std::function<void(std::uint32_t)> banAndSave; //004741f0 삽입→00458770 전체 blacklist 저장을 실제 사용자 홈 장치로 수행한다.
    std::function<bool(std::string_view)> blockedAddress; //004d23e8의 현재 문자열 표 조회 결과다.
    std::function<std::string(std::span<const std::uint8_t,20>,bool)> addressText; //004864b0의 첫 flag0/1과 나머지0/0 서식이다.
    std::function<std::array<std::uint8_t,20>(const std::optional<std::string>&)> parseAddress; //00486870의 NULL/현재 문자열 주소 해석이다. 폐기 외부 DNS를 복구하지 않는다.
    std::function<void(std::uint32_t,std::span<std::uint8_t>)> relayRoot; //00456f50→00474b00의 현재 zone/17h 통지를 실제 Root 라우터에 보낸다.
    std::function<std::optional<std::string>(std::uint32_t)> guildName; //0045d7e0/004d0870의 현재 subscriber 소속 길드 이름 조회다.
    std::function<void(std::int32_t,std::string_view)> log; // 원문 level/이미 완성한 전체 로그다.
};
class ChallengeRuntime final { // 같은 CCh/전송·Root callback보다 오래 사는 실제 실행 부모다.
public:
    ChallengeRuntime(ConnectionState& state,TransceiverTransport& transport,ChallengeStore& challenges,Options& options,ChallengeMessages& messages,ChallengeRuntimeHooks hooks); // 생성은 서버/소켓/파일을 열지 않는다.
    ChallengeRuntime(const ChallengeRuntime&)=delete;ChallengeRuntime& operator=(const ChallengeRuntime&)=delete; // 현재 전역/전역 scratch/콜백 this를 복제하지 않는다.
    void InstallPacketObservers(); // 실제 CCh 송신이 사용하는 원문 관찰기·버전/고정 길이를 명시 등록한다.
    void Login(std::uint32_t challenge,std::uint32_t source,std::span<std::uint8_t> request); //0044e940의 실제 참가자 준비/명단·응답·뉴스/길드 이름 전체 부모다.
    void EndParticipant(std::uint32_t challenge,std::uint32_t participant); //0044e0d0의 Root 통지/전투 재선택·삭제/현재 socket close·명단 제거다.
    std::span<std::uint8_t> SharedPacket() { return scratch_; } //004cf220부터 다음 전역까지 확인한5000BYTE 공유 구간이며 재초기화 때 지우지 않는다.
private:
    ConnectionState& state_;TransceiverTransport& transport_;ChallengeStore& challenges_;Options& options_;ChallengeMessages& messages_; // 같은 현재 R 표/장치/옵션·뉴스를 빌린다.
    ChallengeRuntimeHooks hooks_;std::array<std::uint8_t,5000> scratch_{}; // 실제 상위 장치와 PE 로더 초기0인 공유 작성 물리다.
    std::span<std::uint8_t> Player(std::uint32_t challenge,std::uint32_t participant); // 실제188BYTE 참가자0..39를 빌린다.
    std::span<std::uint8_t> Battle(std::uint32_t challenge,std::uint32_t battle); // 실제344BYTE 전투0..9를 빌린다.
    std::span<std::uint8_t> Shared(std::uint8_t type); // allocator의 capacity0처럼 header만 쓰고 기존 본문 바이트를 보존한다.
    void Send(std::uint32_t challenge,std::uint32_t participant,std::span<std::uint8_t> packet); //0044e4c0의 현재 참가자 노드가 있을 때만 실제 Send한다.
    void Broadcast(std::uint32_t challenge,std::span<std::uint8_t> packet); //0044e2c0의0..39 현재 노드/valid 진단/실제 Send다.
    void SendBattle(std::uint32_t challenge,std::uint32_t battle,std::span<std::uint8_t> packet); //0044e430의 유효 전투/8참가자 현재 명단 순서다.
    void Assert(std::string_view expression,std::uint32_t line); // 실제 ch.cpp 진단이 복귀하면 뒤의 원문 쓰기를 계속한다.
    void Observe(std::uint8_t type,std::span<std::uint8_t> packet,std::uint32_t sending); // 원문 동적 길이/진단만 같은 mutable packet에 쓴다.
    void InitializePlayer(std::uint32_t challenge,std::uint32_t participant,std::span<const std::uint8_t> request); //0044c7b0/후속 실제 버전·주소 필드를 현재 참가자에 쓴다.
    void DeletePlayer(std::uint32_t challenge,std::uint32_t participant); //0044d550/0044c9a0의 전투 소속 정리·로그/188BYTE 지우기다.
    void RecalculateBattleVersions(std::uint32_t challenge,std::uint32_t battle); //0044d130의 현재8참가자+b4/+b2 집계다.
    void UpdateBattleMaster(std::uint32_t challenge,std::uint32_t battle); //00458a40의 master 선택/현재 ready·설명 초기화와 실제 통지다.
    std::span<std::uint8_t> PlayerPacket(std::uint32_t challenge,std::uint32_t participant,bool existing); //0044c540의20h 전체 참가자/53BYTE+이름 작성이다.
    std::span<std::uint8_t> PlayerChange(std::uint32_t challenge,std::uint32_t participant); //0044c700의22h17BYTE 변경 패킷이다.
    std::span<std::uint8_t> BattlePacket(std::uint32_t challenge,std::uint32_t battle,bool existing); //0044bfa0의23h 작성이며 flag2를 기존 scratch에 OR한다.
    std::span<std::uint8_t> BattleDescription(std::uint32_t challenge,std::uint32_t battle); //0044c0c0의71h 현재 전투 설명이다.
    void LoginReply(std::uint32_t connection,std::uint8_t status,std::uint32_t subscriber); //0044e520의1dh13BYTE/10.71·상태1 성공·subscriber DWORD다.
    void NotifyRoot(std::uint32_t challenge,std::uint32_t participant,bool joined,std::uint8_t level); //17h14BYTE 가입/종료를 현재 Root 라우터로 보낸다.
    void AddressPacket(std::uint32_t challenge,std::uint32_t participant,std::string_view address,std::uint32_t destination); //7bh/주소 길이 진단·지정 참가자 또는 전체 전송이다.
    void Message(std::uint32_t challenge,std::uint32_t participant,std::string_view text,std::span<const std::uint8_t> object,bool extended); //0044e690/0044e7e0의41h version3 메시지·객체/수신 subscriber다.
    void SendGuildName(std::uint32_t challenge,std::uint32_t subscriber,std::uint32_t destination); //00451d70/51df0의88h 현재 길드 이름을 실제 고정98BYTE로 보낸다.
};
}
