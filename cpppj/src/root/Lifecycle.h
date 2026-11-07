// R 프로세스의 실제 시작/정리를 같은 현재 상태와 복원한 장치에 연결한다.
#pragma once
#include "root/ConnectionState.h"

namespace netstorm::root { //00477e10/00478af0의 전체 호출 순서와 실제 메인 hook을 소유한다.
struct NetworkState;struct MainLoopHooks;class Configuration;class Clock;class ChallengeStore;class ServerList;class SharedFiles;class SharedFileStorage;class RootControl;class RootDirectory;class LoginRequest;class Routing;class ChallengeRuntime;class RemoteCommandServer;class AddressCodec;class TransceiverTransport; // 이 실제 모듈들은 Lifecycle 및 설치한 메인 callback보다 오래 살아야 한다.
struct LifecycleState { // 시작 시 지우는 나머지 프로세스 전역이며 메인 요청/예약 전역과 다르다.
    std::array<std::uintptr_t,50> processHandles{}; //004d20a0의50개 프로세스 핸들이다. 현재 원문은 단일 프로세스로 강제하므로 새 핸들을 발행하지 않는다.
    std::uint32_t unusedProcessValue{}; //004d2200은 제공된 R에서 시작 때0만 쓰고 읽지 않는 별도 DWORD다.
};
struct LifecycleModules { // 최종 R owner의 같은 객체들을 대여하며 별도 상태/옵션 사본을 만들지 않는다.
    ConnectionState& connections;NetworkState& network;LifecycleState& process; // 현재 번호/타임아웃·프로세스 전역이다.
    Configuration& configuration;Clock& clock;ChallengeStore& store; // 같은 옵션/실제 시계·CCh 소유자다.
    ServerList& servers;SharedFiles& files;SharedFileStorage& storage; // 현재 서버 목록/뉴스·관리 표와 사용자 홈 파일 장치다.
    RootControl& control;RootDirectory& directory;LoginRequest& login;Routing& routing;ChallengeRuntime& challenges; // 같은 실제 수신/송신 부모이며 전역 패킷 초기화가 직접 등록한다.
    RemoteCommandServer& remote;AddressCodec& addresses;TransceiverTransport& transport; // 기본 비활성 보조 장치/원문 주소 parser와 실제 고정 노드 전송이다.
};
class Lifecycle final { // 참조한 최종 owner와 이 객체는 메인 반복이 끝날 때까지 살아야 한다.
public:
    Lifecycle(LifecycleModules modules,std::string root,std::function<void(std::int32_t,std::string_view)> log); // 실제 모듈과 UTF-8 가상CWD/로그만 보관하며 생성 시 IO/OS/소켓 작업을 하지 않는다.
    Lifecycle(const Lifecycle&)=delete;Lifecycle& operator=(const Lifecycle&)=delete; // callback this와 같은 실제 모듈 참조를 복제하지 않는다.
    void Start(); //477e10의 현재 전역0/대기정리·목록/설정·시계/전체패킷·UDP공지/실제listener·CCh/뉴스 순서다. 선택된 레거시 IPX/SPX 장치는 명시 미지원이다.
    void Shutdown(); //478af0의 현재 일반0..255 close/0→CCh1..50→보조Shutdown만 수행한다. pending은 다음Start에서 닫는다.
    void Bind(MainLoopHooks& hooks); // 실제 시작/정리·서버 목록 감시와 같은 보조 장치 Active/Sets/Poll을 연결한다. 원문에 없는 원격 포트Open/handler 등록은 하지 않는다.
private:
    LifecycleModules modules_;std::string root_;std::function<void(std::int32_t,std::string_view)> log_; // 장기 모듈 대여/현재 UTF-8 설정 탐색 루트와 실제 로그 장치다.
    void Assert(std::string_view expression,std::uint32_t line); // 실제 r.cpp 진단을 호출하고 복귀하면 후속 원문 동작을 계속한다.
    void Log(std::int32_t level,const std::string& text); // 선택된 실제 완성 로그 장치만 요구한다.
    bool Open(std::uint32_t token); //478a70의 하위socket flags40/상위connection kill40 두 관문이다.
    void Broadcast(); // 현재 IP/레거시 선택·일반번호 발행/생성·공개·실패정리/유휴제외→같은 Announce다.
    void Listen(); // 현재Root 모드의 실제 TCP/SPX 선택·일반번호/4096수락·8BYTE listener 공개/실패정리다.
};
}
