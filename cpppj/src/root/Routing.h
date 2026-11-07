// CCh의 Root 통지 직접 호출과 Root17h 원격 요약 수신을 같은 실제 등록표에 연결한다.
#pragma once
#include "root/ParticipantState.h"
#include "root/ChallengeStore.h"

namespace netstorm::root { // 원문474b00/474ce0/76070/456f80의 R 내부 부모다.
struct ChallengeRuntimeHooks; // 같은 CCh의 Root 통지/명시 길이 안내 장치를 연결하는 참조 선언이다.
struct MirrorPlayerDefaults { // 실제 R 프로세스의 별도 초기 기본 바이트이며 외부 현재 전역으로 보관한다.
    std::array<std::uint8_t,6> identity{}; //004d8510 DWORD/004d8514 WORD다.
    std::array<std::uint8_t,10> core{}; //004d06c0/c4/c8의 앞10BYTE다.
    std::array<std::uint8_t,8> position{}; //004d23e0/e4의 현재 두DWORD다. 임의 로컬 grid 좌표로 바꾸지 않는다.
};
class Routing final { // 이 부모와 실제 표/기본값은 등록/대여 callback보다 오래 살아야 한다.
public:
    Routing(ConnectionState& state,TransceiverTransport& transport,ChallengeStore& challenges,MirrorPlayerDefaults& defaults,std::function<void(std::int32_t,std::string_view)> log); // 생성은 실제 서버/소켓을 열지 않는다.
    Routing(const Routing&)=delete;Routing& operator=(const Routing&)=delete; // 현재 등록/공유 전역 참조를 복제하지 않는다.
    void Install(); // Root17h의 같은 nullable 수신기/observer 없음·버전1/길이14를 등록한다.
    void Relay(std::uint32_t challenge,std::span<std::uint8_t> packet); //00474b00: 비Root는 현재 일반1번 송신, Root는 현재 수신기를sender0/NULL로 직접 호출한다. challenge 인자는 원문처럼 사용하지 않는다.
    void SendChallenge(std::int32_t challenge,std::span<std::uint8_t> packet,std::int32_t descriptor=-1); //00474bd0의 Root/현재 CCh·localflag1/INVALID_SOCKET 관문 뒤 sender zone<<8/NULL로 직접 수신한다.
    void BroadcastText(std::string_view text,bool extended=false); //00474d30/474e80의 strlen+NUL/일반·확장41h version3 안내를 모든 현재 CCh에 전달한다.
    void BroadcastBuffer(std::span<const std::uint8_t> text,bool extended=false); //명시 길이 입력으로 첫NUL 뒤도 복사한다. 확장은 원문474e80처럼byte5만81로 쓴다.
    void SetLoginBlocked(std::uint32_t value); //00474fd0의10h8BYTE/현재51표 전체 직접 전달이며 원문 값 자체를 패킷에 쓴다.
    void Bind(ChallengeRuntimeHooks& hooks); //현재 CCh의 Root 통지/일반·확장 안내를 이 같은 라우터에 연결한다. CCh 생성 전에 호출한다.
private:
    ConnectionState& state_;TransceiverTransport& transport_;ChallengeStore& challenges_;MirrorPlayerDefaults& defaults_; // 같은 현재 모드/연결·CCh/원격 기본 바이트다.
    std::function<void(std::int32_t,std::string_view)> log_; // 원문 실제 level/완성된 로그를 받는 프로세스 장치다.
    void ReceiveChange(std::int32_t sender,std::span<const std::uint8_t> packet); //00476070의 종료/음수송신자/존·현재 flags/요약 반영·Root 후처리다.
    void ApplyChange(std::uint32_t challenge,std::span<const std::uint8_t> packet); //00456f80: 종료는0..39 첫subscriber 삭제, 가입은1..39 첫빈칸/실제 생성자/레벨이다.
    void RootPostChange(); //00474ce0은 Root mode 진단 후 반환하는 실제 몸체다. 임의 방송을 추가하지 않는다.
    void Assert(std::string_view expression,std::uint32_t line); // 원문 r.cpp의 선택된 진단을 같은 실제 장치로 보낸다.
};
}
