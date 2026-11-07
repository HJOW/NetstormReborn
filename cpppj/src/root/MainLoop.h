// R 메인 반복/파일 감시·예약 리셋을 같은 현재 시계·CCh/전송 부모에 연결한다.
#pragma once
#include "root/Clock.h"
#include "root/SharedFiles.h"
#include "root/Routing.h"

namespace netstorm::root { //00479760/478fa0/479340과 리셋·종료 전역의 실제 실행 부모다.
struct MainLoopState { // 프로세스 재초기화에서도 같은 현재 전역을 보관한다.
    std::uint32_t resetRequests{},stopRequests{}; //004d220c/2210이며 요청은 원문처럼 증가하고 정상 cycle 후 reset만0으로 쓴다.
    double scheduledAt{};std::uint32_t goingDown{};double lastWarning{}; //004d2320/2098/2330의 예약 시각/종료 선택/마지막 안내 게임 시각이다.
    std::uint32_t watcherFlags{};double lastFilePoll{}; //004d2074 bit1/004d1b80의 최초 지연 초기화와60초 게임 시각 경계다.
};
struct MainLoopHooks { // 남은 실제 프로세스 초기화/정리·서버 목록/보조 객체만 상위에서 빌린다.
    std::function<void(bool)> initialize;std::function<void()> shutdown; //477e10의 최초 여부와478af0 전체 정리다. 빈 함수로 초기화를 성공 처리하지 않는다.
    std::function<void()> reloadRootServers; //471f20의 실제 rootservers.dat 목록 재로드다. Config 설정 재로드와 구별한다.
    std::function<void(std::int32_t,std::string_view)> log; // 원문의 Running/예약 안내 level·완성된 출력이다.
    NetworkHooks network; // 같은 실제 보조 객체/집합·Poll이며 CCh 수신/송신은 ChallengeStore에서 연결한다.
};
class MainLoop final { // 같은 전송/CCh·현재 상태와 모든 hook owner는 실행이 끝날 때까지 살아야 한다.
public:
    MainLoop(ConnectionState& connections,Clock& clock,TransceiverTransport& transport,ChallengeStore& store,ChallengeRuntime& challenges,Routing& routing,SharedFiles& files,MainLoopState& state,MainLoopHooks hooks); // 생성은 창/OS/시계/소켓/파일을 실행하지 않는다.
    MainLoop(const MainLoop&)=delete;MainLoop& operator=(const MainLoop&)=delete; // callback과 같은 현재 참조를 복제하지 않는다.
    std::int32_t Run(bool lanArgument); // 실제 LAN 사전 통지→최초/재초기화·현재 frame 반복→정상 종료/리셋 cycle이다. 새 R.exe entry에서 호출해야 한다.
    void Step(); //473b80→현재51CCh Tick/조건부 중복Tick→같은 통신→파일감시→예약처리를 원문 순서로 수행한다.
    void PollFiles();void PollSchedule(); //478fa0의60초 감시와479340의300/60초 안내·도달 시 요청 증가다.
private:
    ConnectionState& connections_;Clock& clock_;TransceiverTransport& transport_;ChallengeStore& store_;ChallengeRuntime& challenges_;Routing& routing_;SharedFiles& files_;MainLoopState& state_; // 같은 실제 R 전역/부모를 빌린다.
    MainLoopHooks hooks_; // 선택된 실제 상위 장치이며 미연결은 명시 오류다.
};
}
