// 로그인 서버가 보낸 실제8종 패킷의 현재 Player/State/SP/Template 수신 처리를 복원한다.
#pragma once
#include "o/RawPacketDispatch.h"
#include "o/RawPlayerParticipation.h"
#include "o/RawPlayerWorld.h"
#include "o/SpStore.h"
#include <string>
#include <vector>

namespace netstorm::o {
// 원본 클라이언트 수신기가 읽고 쓰는 실제 전역이다. 서버/클라이언트 사본을 따로 만들지 않는다.
struct MissionClientPacketState {
    const bool& server; // 00540bc0이며 자기 로컬 연결에서도 실제 서버다.
    const bool& boss; // 00540bc4이며 Ready의 수신 방향을 결정한다.
    const bool& missionPost; // 00594fa4의 미션 후처리 생략 조건이다.
    const bool& savedGame; // 005c85d4의 재접속/저장 경로다.
    const bool& metamap; // 00594fc8의 현재 CoreData 선택이다.
    const bool& inBattle; // 00594fbc의 현재 전투 여부다.
    const std::uint32_t& editor; // 005c85a4의194162 SP 선택이다.
    const std::uint32_t& outpost; // 005467d8의 실제 메타 게임 객체 존재 여부다.
    PlayerWorldState& player; // 같은 현재 두owner/known이다.
    std::uint32_t& beginSeed; // 005409c4의 LoginBegin 복사 대상이다.
    std::uint32_t& battleSeed; // 00595088의 LoginBegin 복사 대상이다.
    std::uint32_t& revision; // 005949d4의 Fort 수신 증가다.
    std::uint32_t& optionValue; // 005424bc의 현재 옵션 파생값이다.
    std::uint32_t& begin; // 005c7a74의 실제 Begin 수신이다.
    std::uint32_t& clockStamp; // 00594fb0의 현재 CRT clock 결과다.
    std::uint32_t& baseClock; // 00594fb4의 시간 패킷 정수값이다.
    const std::uint32_t& phase; // 005c8758의 현재 State 종류다.
    std::uint32_t& currentState; // 005c87e8의 동기 State 실행 요청이다.
    std::uint32_t& currentAux; // 005c8820의 버전 거부0쓰기다.
    std::uint32_t& pendingState; // 005c8878의 다음 State 요청이다.
    std::uint32_t& sourceTicks; // 0055a420의 잘못된 응답 상태 초기화다.
    std::uint32_t& coreDataActive; // 00595348의 owner0 선택 때0쓰기다.
    double& captureEnd; // 005c8618의 현재 wall 기반 종료시각이다.
    const double& loginEnd; // 005c85f0의 현재 로그인 타이머다.
    std::span<std::uint8_t>& currentCoreData; // 00595350의 현재 선택 포인터다.
    std::span<std::uint8_t> cachedCoreData; // 005953f0의 실제10바이트다.
    std::span<std::uint8_t> zoneAddress; // 005c86c0의6바이트다.
    std::span<std::uint8_t> battleOptions; // 0052f7e0의87바이트다.
    std::span<const std::uint8_t> currentHandle; // 00540ca8의 현재 NUL 종료 좁은 이름이다.
    std::span<const std::uint8_t> hidden; // 0052f81e의 현재9BYTE이며battleOptions+0x3e와동일해야한다.
};
// 실제 큰 외부 몸체는 도달 위치에서 필수다. 빈 수신 성공을 허용하지 않는다.
struct MissionClientPacketHooks {
    std::function<void(std::string_view)> assertion; // 실제 진단 뒤 계속하는 원본 assertion이다.
    std::function<std::int32_t(std::string_view)> configInt; // 버전 불일치 때 suppressVersionMessage를 읽는다.
    std::function<void(std::string_view)> tell; // 버전 불일치의 실제 GameVerDiff 안내다.
    std::function<void()> runState,resetCamera,refreshSp; // 004b6dd0/004340d0/0043dad0의 실제 소비자다.
    std::function<double()> wallSeconds; // 00460d70의 현재 double 벽시각이다.
    std::function<std::uint32_t()> clock; // 실제 CRT clock32비트 결과다.
    std::function<void(std::uint32_t,std::span<const std::uint8_t>)> outpostOptions; // 0042c920의 현재 메타 게임 옵션 처리다.
    std::function<void()> updateRange; // 004b4860의 실제 현재 옵션 범위/제곱 쓰기다.
    std::function<std::int32_t()> requiredEnergy; // 0041c8b0의 현재 선택 옵션 조회다.
    std::function<void(float)> setEnergyType; // 현재00541240 TYPE+c4의 실제float 쓰기다.
    std::function<std::uint32_t()> readOptionValue; // 0041ca70의 현재 파생값이다.
    std::function<void()> reconnectUi; // 004c7880의 실제 재접속 화면 변경이다.
    std::function<void(std::uint32_t)> localFort; // 일반 미션 밖00460680→파일/Flags/Player/metadata 사슬 전체다.
    std::function<void(std::uint32_t)> openFortImage; // 004606d0→004bfba0의 실제 Template 이미지 수신 준비다.
    std::function<void(std::uint32_t,std::span<const std::uint8_t>)> readFortImage; // 004bfc60의 실제 Template image 읽기다.
    std::function<void(std::uint32_t)> buildFort; // 0046d9b0→0046da60→0046e2c0의 실제 배치다.
    std::function<std::string(std::uint32_t)> peerAddress; // '*' IP를 현재 sender 연결의 실제 주소로 바꾼다.
    std::function<void(std::uint32_t,std::string_view,std::uint32_t)> addChat; // 실제00444ee0의20채널 추가다.
    std::function<void(std::uint32_t,std::uint32_t)> removeChat; // 실제00444ff0의 해당 subscriber채널 제거다.
    std::function<void(std::uint32_t)> restoreCoreData; // !missionPost의0048fb30 전체다.
    std::function<bool()> playerUiBusy; // 00449200의 실제 현재 창 관문이다.
    std::function<void()> saveReconnect; // !missionPost의004b54e0 전체다. 미션은 원문처럼 자연 반환한다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> playerNotice; //004d3090(kind,owner,arg,0,"")의 실제 소비자다.
    std::function<void(std::int32_t,std::span<const std::uint8_t>)> send; // 현재 실제 연결/접속gate를 지나는 송신이다.
    std::function<void(std::uint32_t)> sendReady; //004a6f70의 실제현재mask/전체분배이며 RawMissionLogin에 연결할 수 있다.
};
// 0042b0e0의 실제 이름 삭제/빈 이름 규칙이다. Player와 이후 송신 이름 처리도 같은 몸체를 사용할 수 있다.
void SanitizeMissionPlayerName(std::uint32_t subscriber,std::span<std::uint8_t> name);
// 같은 Session의 Player9/SP/참여 모듈을 참조하며 실제8종 수신 분기를 실행한다.
class RawMissionClientPackets {
public:
    // 현재 저장소는 처리기보다 오래 살아야 하며 caller는 같은 Player9를 participation에 공급한다.
    RawMissionClientPackets(std::span<std::uint8_t> players,RawPlayerParticipation& participation,ScrambledSpStore& sp,
        MissionClientPacketState state,MissionClientPacketHooks hooks);
    // 실제 등록된 타입을 처리한다. 모르는 타입이나 물리 범위 밖의 원본 메모리 손상은 거부한다.
    void Receive(std::uint32_t sender,PacketConnection& connection,std::span<const std::uint8_t> packet) const;
private:
    // 실제 Player 필드와 문자열의 원본 폭/종료를 보존한다.
    std::uint32_t Read(std::uint32_t owner,std::size_t offset,std::size_t width=4) const;
    void Put(std::uint32_t owner,std::size_t offset,std::uint32_t value,std::size_t width=4) const;
    void Copy(std::uint32_t owner,std::size_t offset,std::span<const std::uint8_t> source) const;
    std::string_view Name(std::uint32_t owner) const;
    // 같은 현재 state를 직접 갱신하는 각 실제 수신 몸체다.
    void LoginBegin(std::span<const std::uint8_t> packet) const;
    void Options(std::uint32_t sender,std::span<const std::uint8_t> packet) const;
    void Fort(std::span<const std::uint8_t> packet) const;
    void Player(std::uint32_t sender,std::span<const std::uint8_t> packet) const;
    void Reply(std::span<const std::uint8_t> packet) const;
    void Sp(std::span<const std::uint8_t> packet) const;
    void Ready(std::uint32_t sender,std::span<const std::uint8_t> packet) const;
    std::span<std::uint8_t> players_; // 실제1620바이트의 단일 Player9다.
    RawPlayerParticipation& participation_; // 같은 방향별 동맹/채팅/현재owner 처리기다.
    ScrambledSpStore& sp_; // 실제 게임 난수를 공유하는 단일 SP 저장소다.
    MissionClientPacketState state_; // 모든 원본 현재 읽기의 참조 묶음이다.
    MissionClientPacketHooks hooks_; // 실제 선택된 큰 외부 몸체와 UI/전송 경계다.
};
// 원본8종의 observer/버전1/정책0/수신기를 실제 분배기에 등록한다.
void RegisterMissionClientPackets(RawPacketDispatch& dispatch,RawMissionClientPackets& packets);
}
