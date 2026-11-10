// PATCH 로그인004a7800의 현재 Player/일반 전투·미션/패킷/Ready 부모 순서를 복원한다.
#pragma once
#include <bit>
#include <array> //현재0ch 생성 헤더 두BYTE의 실제 주소를 빌린다.
#include "o/RawPlayerParticipation.h"
#include "o/RawPlayerWorld.h"
#include "o/RawPacketDispatch.h"
#include "o/RawMissionLoginTimer.h"
#include <string>
#include <vector>

namespace netstorm::o {
// 로그인과 실제 Session/타이머가 공유하는 현재 전역 참조다. 별도 Ready 성공 사본은 없다.
struct MissionLoginGlobals {
    const bool& server; // 00540bc0의 현재 서버 여부다.
    const bool& boss; // 00540bc4의 Ready 송신 권한이다.
    const bool& simulconnect; // Config0054db50이며 로컬 전송이라는 이유로 켜지 않는다.
    const bool& missionWorld; // 00594fa0의 미션 저장 월드 선택이다.
    const bool& missionPost; // 00594fa4의 미션 후처리 생략 조건이다.
    const bool& metamap; // 00594fc8의 Player 번호 진단 조건이다.
    PlayerWorldState& player; // 같은00540c70/00540cac/00595344 저장소다.
    const std::uint32_t& mySubscriber; // 00540c74의 현재 자기 subscriber다.
    std::uint32_t& fortCount; // 005951f8의 현재 로그인 Fort 순서 수다.
    std::uint32_t& lastLoginPlayer; // 005951fc의 마지막 새 Fort 소유자다.
    std::uint32_t& predictableCount; // 005424b0의 현재 결정적 난수 소비 수다.
    std::uint32_t& begin; // 005c7a74의 실제 시작 전역이다.
    const std::uint32_t& beginSeed; // 005409c4의 LoginBegin DWORD다.
    const std::uint32_t& battleSeed; // 00595088의 LoginBegin 마지막 DWORD다.
    const std::uint32_t& debugBurst; // 0054dbc4의 현재DWORD가 비영이면63000BYTE 패턴 패킷을 세 번 전송한다.
    std::uint32_t& clockStamp; // 00594fb0의 CRT clock 저장 결과다.
    const std::uint32_t& baseClock; // 00594fb4의 이전 CRT clock이다.
    const double& captureEnd; // 005c8618의 현재 벽시계 마감이다.
    std::span<std::uint8_t>& currentCoreData; // 00595350의 현재10바이트 선택 포인터다.
    std::span<std::uint8_t> cachedCoreData; // metamap 선택의005953f0 원문10바이트다.
    std::span<const std::uint8_t> zoneAddress; // 005c86c0의 실제6바이트다.
    std::span<const std::uint8_t> battleOptions; // 0052f7e0의 현재87바이트다.
    const std::int32_t* sharedBattleSeed{}; // signed 자원과 같은 DWORD를 빌리며 송신은 unsigned bits로 읽는다.
    const bool* savedGame{}; // 일반 자원 초기화가 선택될 때 같은005c85d4를 직접 빌려야 한다. 미션Post1의 기존 호출자는 생략할 수 있다.
    const bool* inBattle{}; // 일반 자원 초기화의 같은00594fbc이며 임의로 true를 공급하지 않는다.
    const std::array<std::uint8_t,2>* debugPacketHeader{}; //0052ed54/55의 현재메시지/프로토콜을 각 debug 예약에서 읽는다.
    const std::uint16_t* debugPacketInitialLength{}; //00544c24의 현재 생성길이WORD이며 실제 송신observer의 확정 길이와 구별한다.
    std::uint32_t BattleSeed() const { return sharedBattleSeed ? std::bit_cast<std::uint32_t>(*sharedBattleSeed) : battleSeed; } // 다른 타입의 ref를 만들지 않는다.
};
// 큰 외부 몸체는 선택된 실제 위치에서 필수다. 패킷을 버리는 성공 처리기를 공급하지 않는다.
struct MissionLoginHooks {
    std::function<void(std::int32_t)> disconnect; // 004e2940의 실제 연결 해제다.
    std::function<void(std::int32_t,std::span<std::uint8_t>)> send; //로컬writer의전체mutable예약을004e2dc0의현재연결gate에넘기며선택된장치가observer후현재선언길이를사용한다.
    std::function<void(std::span<const std::uint8_t>)> broadcast; // target0 Ready의004e2e30 실제 현재 연결 전체분배다.
    std::function<void(PacketConnection&,std::span<const std::uint8_t>)> pendingReply; // owner 미발견의 연결 가상+4 직접 송신이다.
    std::function<void()> flush; // 004e2920의 실제 모든 전송 폴링이다.
    std::function<double()> wallSeconds; // 00460d70의 정지와 무관한 현재 double 벽시각이다.
    std::function<std::uint32_t()> clock; // CRT _clock의 현재32비트 결과다.
    std::function<std::uint32_t(std::uint32_t)> spBits; // 0040ec50의 현재 분산 SP float비트다.
    std::function<void(std::span<const std::uint8_t>)> applyOptions; // 004b4900(0,0,packet)의 실제 옵션 소비 전체다.
    std::function<void()> clearTypes; // 0041c320의 실제256바이트 허용 이름표 초기화다.
    std::function<void(std::uint32_t,std::uint32_t)> sendPlayer; // 0048f9f0의 실제 이름/IP/Player packet을 owner,target으로 보낸다.
    std::function<void(std::uint32_t)> sendFortMetadata; // 004a6b40의 현재 중앙 Fort metadata 처리/조건부 송신이다.
    std::function<void(std::uint32_t)> loadCurrentFort; // 00460680→004bf550의 실제 자기 server 파일을 해당 Template에 읽는다.
    std::function<void(std::uint32_t,std::span<const std::uint8_t>)> loadFortImage; // 다른 owner의004606d0→004bfba0→004bfc60 전체다.
    std::function<std::string(std::uint32_t)> peerAddress; // 004e2ae0의 현재 peer IP 좁은문자열이다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> sendFort; // 004a7240의 현재 Template 직렬화/패킷 송신이다.
    std::function<void(std::uint32_t)> synchronizePeer; // 004a7350의 다른 owner 월드 동기화 전체다.
    std::function<void()> loadWorld; // 새 로그인의004b5300이며 같은 Session.Load를 이 위치에 연결한다.
    std::function<void(std::uint32_t)> sendSettings; // 0044dbc0의 현재7DWORD 설정 패킷이다.
    std::function<void(std::string_view)> assertion; // 원본 진단 후 계속하는 assertion 경계다.
    std::function<void(std::uint32_t)> restoreCoreData; //004a796c의 previous0에서만 호출하는 같은0048fb30 부모다. 독립 미션Post1의 누락 장치는 하위 첫 자연 반환과 같다.
    std::function<void(std::uint32_t,std::int32_t)> setSp; //00447810의 현재 owner/정수 SP 지정 전체이며 로컬 HUD/편집 모드도 실제 구현이 처리해야 한다.
    std::function<void(std::uint32_t)> makeBattleIsland; //0046d9b0→0046da60(owner)→0046e2c0의 실제 생성/배치/소멸 전체다. watcher 자연 생략을 대신 성공시키지 않는다.
    std::function<void()> cleanupReconnectClients; //004a7510의 실제 client SID5..14999 비용 환불/현재 가상count·삭제 순회다.
    std::function<bool()> checkingAssertions; //004a7510의 subscriber 미발견 꼬리가 현재005e4794를 읽는다.
    std::function<void(std::string_view)> log; //004a68e0의 재접속 원문 진행 로그를 현재 장치에 전달한다.
};
// 같은 현재 일반/미션 분기와 실제 CheckLogin을 조립하고 선택된 큰 하위는 필수 장치로 요구한다.
class RawMissionLogin {
public:
    // caller는 현재 Player9와 참여 처리기에 동일 저장소를 공급해야 하며 모든 참조 대상이 더 오래 살아야 한다.
    RawMissionLogin(std::span<std::uint8_t> players,RawPlayerParticipation& participation,MissionLoginGlobals globals,MissionLoginHooks hooks);
    // 004a7800의 sender/실제 연결/0x27 버퍼를 처리한다. CheckLogin의 반환으로 원본에 없는 조기 반환을 만들지 않는다.
    void Receive(std::int32_t pending,PacketConnection& connection,std::span<const std::uint8_t> packet) const;
    void ReceiveDirect(std::int32_t pending,PacketConnection* connection,std::span<const std::uint8_t> packet) const; //Native F의 nullable 두번째 인자를 거부 응답의 실제 사용 지점까지 보존한다.
    // 004a6920의 실제18바이트 응답을 보내며 참여 처리기의 거부 응답에도 사용할 수 있다.
    void SendReply(std::int32_t owner,std::uint8_t accepted) const;
    // 004a6f70의 현재 owner1..8 Ready mask를 실제0x6e 패킷으로 보낸다.0은 원본 전체분배다.
    void SendReady(std::uint32_t target) const;
    void ReceiveReconnect(std::int32_t pending,std::span<const std::uint8_t> packet) const; //004a7510: 실제63h의subscriber를 찾아 기존CheckLogin/응답/Joined 뒤 현재 장부/클라이언트 정리/Ready를 수행한다.
private:
    // 현재9기록의 비정렬 필드를 원본 폭대로 읽고 쓴다.
    std::uint32_t Read(std::uint32_t owner,std::size_t offset,std::size_t width=4) const;
    void Put(std::uint32_t owner,std::size_t offset,std::uint32_t value) const;
    // 실제 fixed/variable 패킷 helper와 현재 Player 데이터 전송 순서다.
    std::vector<std::uint8_t> Reply(std::int32_t owner,std::uint8_t accepted) const;
    void SendBegin(std::uint32_t owner) const;
    void SendSp(std::uint32_t owner) const;
    void Joined(std::uint32_t owner,std::uint32_t subscriber) const;
    std::span<std::uint8_t> players_; // 실제1620바이트의 단일 Player9 저장소다.
    RawPlayerParticipation& participation_; // 실제 flags3/연결 재배치/거부·해제 몸체다.
    MissionLoginGlobals globals_; // 각 원본 위치에서 다시 읽는 참조 묶음이다.
    MissionLoginHooks hooks_; // 큰 외부 몸체와 실제 전송/시계 소비자다.
    MissionLoginPlayerCounts counts_; // 타이머와 같은 실제 집계 몸체가 같은 Player9를 직접 읽는다.
};
// 원본 초기화처럼0x27의 실제 버전1/정책0/처리기를 등록한다. login은 dispatch보다 오래 살아야 한다.
void RegisterMissionLogin(RawPacketDispatch& dispatch,RawMissionLogin& login);
}
