// 원문 R의 현재 전역 재조회와 정상 cycle 시작/정리 순서를 보존한다.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Windows 구 선언 충돌을 피한다.
#endif
#ifndef NOMINMAX
#define NOMINMAX // 표준 함수와 Windows 매크로 충돌을 피한다.
#endif
#include <windows.h>
#include "root/Lifecycle.h"
#include "root/MainLoop.h"
#include "root/Configuration.h"
#include "root/ChallengeStore.h"
#include "root/RootControl.h"
#include "root/RootDirectory.h"
#include "root/LoginRequest.h"
#include "root/AddressCodec.h"
#include "root/ServerList.h"
#include "root/RemoteCommand.h"
#include "root/PacketRegistration.h"
#include "platform/Utf8Path.h"
#include <bit>
#include <cstdio>
#include <stdexcept>

namespace netstorm::root { // 최종 새R.exe owner의 실제 호출은 별도 entry에서 연결해야 한다.
namespace { // 원문 모드 출력의 대문자16진수만 호스트 문자열로 소유한다.
std::string Hex(std::uint32_t value) { std::array<char,9> buffer{};std::snprintf(buffer.data(),buffer.size(),"%X",static_cast<unsigned>(value));return buffer.data(); } // %X는 최소폭이며0도 한 글자로 출력한다.
}
Lifecycle::Lifecycle(LifecycleModules modules,std::string root,std::function<void(std::int32_t,std::string_view)> log):modules_(modules),root_(std::move(root)),log_(std::move(log)) {}
void Lifecycle::Assert(std::string_view expression,std::uint32_t line) { const auto hook=modules_.transport.Lifecycle().assertion;if (!hook) throw std::logic_error("R 시작의 실제 진단 장치 누락");hook(expression,"c:\\ns\\r\\r.cpp",line); }
void Lifecycle::Log(std::int32_t level,const std::string& text) { if (!log_) throw std::logic_error("R 시작의 실제 로그 장치 누락");log_(level,text); }
bool Lifecycle::Open(std::uint32_t token) { return (modules_.transport.SocketFlags(token)&0x40U) && !(modules_.transport.Connection(token).flags&0x40U); }
void Lifecycle::Start() {
    auto& m=modules_;if (m.transport.CallbacksActive()) throw std::logic_error("R 전송 callback 중 시작/전역 재초기화 금지");auto& options=m.configuration.Values();
    m.connections.modeFlags=0;m.network.emptyExitTimeout=0;options.basePort=0;options.broadcastPort=0;m.connections.udpBroadcastId=0;m.connections.ipxBroadcastId=0;m.connections.challengeStateCount=0;options.challengeStatesPerProcess=0;m.network.mainTimeout=0;m.network.pendingTimeout=0;m.network.challengeTimeout=0;options.killLowVersions=0;
    CloseAllPending(m.connections,m.transport.Lifecycle());m.process.processHandles.fill(0);m.process.unusedProcessValue=0;m.store.ProcessOwners().fill(0);m.connections.transceivers.fill(0);
    Log(29,"options\n");options.flushAfterTrace=1;m.servers.Reload();const auto count=std::bit_cast<std::int32_t>(m.servers.Count());std::array<char,80> name,description;bool haveText=false;
    for (std::int32_t index=0;index<count;++index) { // 원문은 목록 개수만 최초 캡처하고 각 현재 행/80BYTE 출력을 읽은 뒤 로그를 수행한다.
        const bool present=m.servers.Entry(index)!=nullptr;if (!present && !haveText) throw std::out_of_range("R 시작 목록의 최초 미작성 문자열 읽기 거부");std::int32_t value{};m.servers.Decode(index,name,description,&value);haveText=haveText || present;Log(0,std::to_string(index)+' '+name.data()+' '+description.data()+' '+std::to_string(value)+'\n'); // 이후 사라진 행은 Decode가 보존한 이전 버퍼와 새 value0을 사용한다.
    }
    m.configuration.Startup(root_);m.storage.DataDirectory(platform::PathFromUtf8(m.configuration.DataPrefix())); // 원문 dataPrefix 확정 뒤 같은 호스트 파일 장치의 읽기 경로만 갱신한다. 실제 뉴스 읽기는 아래 원문 위치다.
    if (!(m.connections.modeFlags&0x17U)) Assert("smModeFlags & (smDIAG|smROOT|smCHAL)",1900);if (!options.basePort) Assert("rootServerBasePort",1901);
    m.connections.transceivers.fill(0);Log(29,"init full time\n");m.clock.Initialize();Log(29,"init packet\n");InitializePackets(m.transport,m.control,m.directory,m.login,m.routing,m.challenges);Log(29,"baseport = "+std::to_string(options.basePort)+'\n');
    if (m.connections.modeFlags&kLocalMode) Broadcast();if (m.connections.modeFlags&kRootMode) Listen();if (m.connections.modeFlags&kRootMode) m.store.InitializeRoot(options,log_);
    if (m.connections.modeFlags&kChallengeMode) { m.files.CheckNews();m.files.CheckBlacklist(); }const auto mode=m.connections.modeFlags;const auto process=GetCurrentProcessId();Log(0,"pid="+std::to_string(std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(process)))+" mode flags = "+Hex(mode)+", 0x01=diagrecv 0x02=root 0x04=chal 0x08=lan 0x10=diagxmit\n");
}
void Lifecycle::Broadcast() {
    auto& m=modules_;auto& options=m.configuration.Values();const bool ip=options.useIPAsLanProtocol!=0;auto& id=ip ? m.connections.udpBroadcastId:m.connections.ipxBroadcastId;id=FindFreeTransceiver(m.connections);if (!id) Assert(ip ? "udpBroadcastId":"ipxBroadcastId",ip ? 1926:1944);
    const auto address=m.addresses.Parse(std::string(ip ? "udp,*:":"ipx,*:")+std::to_string(options.broadcastPort)+",s");const auto token=m.transport.CreateServer(id,address,512);m.connections.transceivers.at(id)=token;
    if (!Open(m.connections.transceivers.at(id))) { m.transport.Close(m.connections.transceivers.at(id));m.connections.transceivers.at(id)=0;id=0; }
    else { Log(ip ? 29:0,std::string(ip ? "bound udp ":"bound ipx ")+std::to_string(options.broadcastPort)+'\n');m.transport.NoMainTimeout(m.connections.transceivers.at(id))=1; }
    m.control.Announce(); // 원문의 두 선택 분기 뒤 공통 공지이며 실패에서도 생략하지 않는다. 현재 실제 노드가 없는 프로토콜은 기존 guard가 건너뛴다.
}
void Lifecycle::Listen() {
    auto& m=modules_;auto& options=m.configuration.Values();const auto id=FindFreeTransceiver(m.connections);if (!id) Assert("id",1969);const bool ip=!(m.connections.modeFlags&kLocalMode) || options.useIPAsLanProtocol!=0;
    const auto address=m.addresses.Parse(std::string(ip ? "tcp,*:":"spx,*:")+std::to_string(options.basePort)+",s");const auto token=m.transport.CreateServer(0,address,4096);m.connections.transceivers.at(id)=token;if (!Open(m.connections.transceivers.at(id))) { m.transport.Close(m.connections.transceivers.at(id));m.connections.transceivers.at(id)=0; }
}
void Lifecycle::Shutdown() {
    auto& m=modules_;if (m.transport.CallbacksActive()) throw std::logic_error("R 전송 callback 중 시작 모듈 반납 금지");
    for (std::size_t index=0;index<m.connections.transceivers.size();++index) { // 원문 일반0..255의 현재 노드 close 복귀 뒤 같은 칸0이며 pending은 여기서 닫지 않는다.
        const auto current=m.connections.transceivers[index];if (current) { m.transport.Close(current);m.connections.transceivers[index]=0; }
    }
    m.store.Shutdown();static_cast<void>(m.remote.Shutdown()); // 현재CCh1..50 뒤 같은 실제 보조 객체다. 옵션/시계/난수·요청/예약 전역은 임의 초기화하지 않는다.
}
void Lifecycle::Bind(MainLoopHooks& hooks) { hooks.initialize=[this](bool first) { static_cast<void>(first);Start(); };hooks.shutdown=[this] { Shutdown(); };modules_.servers.Bind(hooks);modules_.remote.Bind(hooks.network); } //477e10은 최초/재시작에 같은 몸체를 사용하며 포트Open/handler 등록을 추가하지 않는다.
}
