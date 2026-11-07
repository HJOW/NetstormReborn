// 실제 R CCh의 참가자 로그인/명단·메시지·종료를 같은 저장소/전송에 연결한다.
#pragma once
#include "root/LoginRequest.h"
#include "root/Configuration.h"

namespace netstorm::root { // CCh0044e940/0044e0d0의 현재 부모와 실제 패킷 작성기를 소유한다.
struct ClockState;class Random; // 같은 R 프로세스의 현재 게임 시각과 별도 전역 난수 스트림이다.
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
    std::function<void(std::span<std::uint8_t,20>)> localAddress; //004870d0을 현재 주소 자체에 적용한다. 실패 때 family만0이고 나머지 기존 바이트를 보존한다.
    std::function<void(std::uint32_t,std::span<std::uint8_t>)> relayRoot; //00456f50→00474b00의 현재 zone/17h 통지를 실제 Root 라우터에 보낸다.
    std::function<void(std::span<const std::uint8_t>,bool)> broadcastRoot; //00474d30/474e80의 명시 길이/일반·확장41h를 같은 Root 라우터로 보낸다.
    std::function<void(std::uint32_t,double)> schedule; //00479090의 현재 reset/shutdown 선택과 초 단위 delay를 같은 메인 예약 상태에 적용한다.
    std::function<std::optional<std::string>(std::uint32_t)> guildName; //0045d7e0/004d0870의 현재 subscriber 소속 길드 이름 조회다.
    std::function<std::string()> commandTemplates;std::function<void(std::string&)> expandCommandPage; //00451f30/452570의 실제 현재 서버 템플릿/OS시각·존 치환 장치다. 같은 CommandPages owner가 연결한다.
    std::function<void(std::int32_t,std::string_view)> log; // 원문 level/이미 완성한 전체 로그다.
};
class ChallengeRuntime final { // 같은 CCh/전송·Root callback보다 오래 사는 실제 실행 부모다.
public:
    ChallengeRuntime(ConnectionState& state,TransceiverTransport& transport,ChallengeStore& challenges,Options& options,ChallengeMessages& messages,ClockState& clock,Random& random,ChallengeRuntimeHooks hooks); // 생성은 서버/소켓/파일을 열지 않으며 같은 프로세스 시계/난수를 빌린다.
    ChallengeRuntime(const ChallengeRuntime&)=delete;ChallengeRuntime& operator=(const ChallengeRuntime&)=delete; // 현재 전역/전역 scratch/콜백 this를 복제하지 않는다.
    void InstallPacketObservers(); // 실제 CCh 관찰기·버전/길이와10/1e/1f/41/67/68/71/83h 수신을 등록한다. 입력은 쓰기 가능한 전송 물리여야 한다.
    void Login(std::uint32_t challenge,std::uint32_t source,std::span<std::uint8_t> request); //0044e940의 실제 참가자 준비/명단·응답·뉴스/길드 이름 전체 부모다.
    void EndParticipant(std::uint32_t challenge,std::uint32_t participant); //0044e0d0의 Root 통지/전투 재선택·삭제/현재 socket close·명단 제거다.
    std::int32_t Tick(std::uint32_t challenge); //004581e0의 현재 전투 재생성/가상 참가자·명단 송신을 수행하고 원문처럼1을 반환한다.
    std::span<std::uint8_t> SharedPacket() { return scratch_; } //004cf220부터 다음 전역까지 확인한5000BYTE 공유 구간이며 재초기화 때 지우지 않는다.
    void SendCommandPage(std::uint32_t challenge,std::uint32_t participant,std::string_view page); //00452300의 객체8BYTE/확장41h 응답을 같은 현재 참가자에게 보낸다. 객체BYTE1..3 미작성은 호스트 입력으로 보존한다.
private:
    ConnectionState& state_;TransceiverTransport& transport_;ChallengeStore& challenges_;Options& options_;ChallengeMessages& messages_;ClockState& clock_;Random& random_; // 같은 현재 R 표/장치/옵션·뉴스/시계·프로세스 난수를 빌린다.
    ChallengeRuntimeHooks hooks_;std::array<std::uint8_t,5000> scratch_{}; // 실제 상위 장치와 PE 로더 초기0인 공유 작성 물리다.
    std::span<std::uint8_t> Player(std::uint32_t challenge,std::uint32_t participant); // 실제188BYTE 참가자0..39를 빌린다.
    std::span<std::uint8_t> Battle(std::uint32_t challenge,std::uint32_t battle); // 실제344BYTE 전투0..9를 빌린다.
    std::span<std::uint8_t> Shared(std::uint8_t type); // allocator의 capacity0처럼 header만 쓰고 기존 본문 바이트를 보존한다.
    void Send(std::uint32_t challenge,std::uint32_t participant,std::span<std::uint8_t> packet); //0044e4c0의 현재 참가자 노드가 있을 때만 실제 Send한다.
    void Broadcast(std::uint32_t challenge,std::span<std::uint8_t> packet); //0044e2c0의0..39 현재 노드/valid 진단/실제 Send다.
    void SendBattle(std::uint32_t challenge,std::uint32_t battle,std::span<std::uint8_t> packet); //0044e430의 유효 전투/8참가자 현재 명단 순서다.
    void Assert(std::string_view expression,std::uint32_t line); // 실제 ch.cpp 진단이 복귀하면 뒤의 원문 쓰기를 계속한다.
    void Observe(std::uint8_t type,std::span<std::uint8_t> packet,std::uint32_t sending); // 원문 동적 길이/진단만 같은 mutable packet에 쓴다.
    void ReceiveChat(std::uint32_t sender,std::span<std::uint8_t> packet); //00476880의 종료/CHAL 진단·signed 존/현재 토큰 관문을 거쳐 같은41h 물리를 전달한다.
    void Chat(std::uint32_t challenge,std::uint32_t participant,std::span<std::uint8_t> packet); //004512b0의 명령·줄/빈도 제한·직접 수신자/감시자 전달이며 입력 패킷을 제자리에서 수정한다.
    bool ChatCommand(std::uint32_t challenge,std::uint32_t participant,std::string_view text); //00451160의 대소문자 구분 부분 검색으로 .test/.test1을 찾으면 실제 특별41h 응답을 보낸다.
    void ReceiveBattleControl(std::uint8_t type,std::uint32_t sender,std::span<std::uint8_t> packet); //10/1e/1f/67/68/71/83h의 원문 진단 줄과 signed 존 관문 뒤 같은 CCh 몸체를 호출한다.
    void AdminCommand(std::uint32_t challenge,std::uint32_t participant,std::span<const std::uint8_t> packet); //00456b30의 전처리/현재clientAdmin 비교·실제하위소켓/예약·전투/이동/전체안내/관리자표시다.
    void BattleOptions(std::uint32_t challenge,std::uint32_t participant,std::span<std::uint8_t> packet); //0044d6d0의 소속/master 확인·실제 옵션 바이트/bit2 해제·공유22h와 입력67h 전송이다.
    void QueryBattleOptions(std::uint32_t challenge,std::uint32_t participant,std::span<const std::uint8_t> packet); //0044e580은 요청 전투의 현재 valid 뒤 항상160BYTE를67h로 응답한다. 소속/권한 검사를 추가하지 않는다.
    void UpdateBattleDescription(std::uint32_t challenge,std::uint32_t participant,std::span<std::uint8_t> packet); //0044dd90의 관리자 예외·현재 자기 전투 설명/길이0 쓰기와 입력71h 전체 통지를 보존한다.
    void MoveParticipant(std::uint32_t challenge,std::uint32_t participant,std::span<const std::uint8_t> request); //0044fad0의 목적 전투/자리·버전 검사→현재 소속 탈퇴→입장/master·같은 명단/옵션 전송이다.
    void SailResult(std::uint32_t challenge,std::uint32_t participant,std::uint16_t status); //00444dd0 지역25h의 미작성4/5BYTE를 호스트 할당 입력으로 보존하고 상태WORD만6에 쓴다.
    std::int32_t CountBattlePirates(std::uint32_t challenge,std::uint32_t battle); //0044df90의 현재 명단/valid 진단과 기존flag200 수를 읽는다. 원문 이동은 반환값을 사용하지 않으며 외부 검증을 호출하지 않는다.
    bool HasBattleMaster(std::uint32_t challenge,std::uint32_t battle); //00458950의 현재8명/valid 진단·첫 master bit4 조회다. 상태나 명단을 수정하지 않는다.
    void ReadyParticipant(std::uint32_t challenge,std::uint32_t participant,std::span<std::uint8_t> packet); //00450530의 대상 조회/실제 종료·준비 플래그/시작·퇴장과 현재 명단 통지다. 폐기 PIRATE 제한은 적용하지 않는다.
    void StartBattle(std::uint32_t challenge,std::uint32_t battle); //004570c0의 현재 명단/프로세스 난수·master/접속 안내→전체flush→현재연결종료→전투삭제다.
    void RepositionBattle(std::uint32_t challenge,std::uint32_t battle,std::optional<std::uint32_t> participant={}); //00450240/4503b0의 전체 또는 지정 참가자 재배치다. 매회 현재 명단을 읽고 이동 뒤 같은22h를 방송한다.
    void SailOut(std::uint32_t challenge,std::uint32_t participant,std::uint32_t battle,bool clearStart); //현재flag8/선택flag10·실제게임난수 위치→같은MoveParticipant→flag8복구이며 준비퇴장/재배치가 공유한다.
    void InitializePlayer(std::uint32_t challenge,std::uint32_t participant,std::span<const std::uint8_t> request); //0044c7b0/후속 실제 버전·주소 필드를 현재 참가자에 쓴다.
    void DeletePlayer(std::uint32_t challenge,std::uint32_t participant); //0044d550/0044c9a0의 전투 소속 정리·로그/188BYTE 지우기다.
    void RecalculateBattleVersions(std::uint32_t challenge,std::uint32_t battle); //0044d130의 현재8참가자+b4/+b2 집계다.
    void UpdateBattleMaster(std::uint32_t challenge,std::uint32_t battle); //00458a40의 master 선택/현재 ready·설명 초기화와 실제 통지다.
    void InitializeBattle(std::uint32_t challenge,std::uint32_t battle,std::span<const std::uint8_t,6> identity,const ChallengePoint& position); //0044c3f0의 지정 필드/8회원/현재 게임 시각을 쓰며 나머지 현재 바이트를 보존한다.
    std::array<std::uint8_t,6> RandomIdentity(); //00457cb0의 초기0 identity/두 난수·21개 형상/byte3/bit100이다. 위치 인자를 사용하지 않는 원문을 따른다.
    std::array<std::uint8_t,10> RandomCore(); //0044b900/4b960의 초기0 Core/현재26난수·능력22비트를 작성한다.
    ChallengePoint FindPosition(std::uint32_t challenge,const ChallengePoint& center,std::int32_t distance=0,std::int32_t range=0); //00457d40의5후보/두 난수 순서·실제 원문 거리 참조와 현재 best 선택이다.
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
