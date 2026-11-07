// 실제 TCP/UDP와 원문 메타 연결/검색 상태를 같은 Client 전역에 직접 연결한다.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN //창제목조회에구형Winsock선언을불러오지않는다.
#endif
#ifndef NOMINMAX
#define NOMINMAX //표준min/max와Windows매크로의충돌을막는다.
#endif
#include <windows.h>
#include "client/MetaConnectionRuntime.h"
#include "client/ClientMain.h"
#include "client/ClientStaticStrings.h"
#include "client/ClientConnectionGlobals.h"
#include "client/ClientConnectionActivityGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/ClientNsInfoGlobals.h"
#include "client/ClientPlayerRecords.h"
#include "client/ClientCoreDataGlobals.h"
#include "client/ClientSubscriberGlobals.h"
#include "client/ClientCdGlobals.h"
#include "client/ClientChatRuntime.h"
#include "client/CurrentAllocationBytes.h"
#include "client/CdDirectory.h"
#include "client/RootServerEndpoint.h"
#include "client/LocalUdpTransport.h"
#include "client/NativeLocalServerLaunch.h"
#include "client/LocalServerProcess.h"
#include "client/RootServerListDisk.h"
#include "client/ClientStateStorageCleanup.h"
#include "client/ClientRandomGlobals.h"
#include "client/CampaignTransportServices.h"
#include "client/RawSurfaceWorld.h"
#include "o/RawConnectionEnded.h"
#include "o/RawPlayerNotification.h"
#include "o/RawFileName.h"
#include "o/RawRootLoginReply.h"
#include "platform/Utf8Path.h"
#include "platform/PortableStartup.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace netstorm::client { //Meta owner는 현재 Client/Player/일반 입력 분배기보다 짧게 산다.
namespace { //실제 packet/현재 상태의 비정렬 BYTE/WORD/DWORD만 조작한다.
// 전달한 폭만 덮고 그 외 원문 미작성 scratch를 보존한다.
void Put(std::span<std::uint8_t> packet,std::size_t at,std::uint32_t value,std::size_t width) {
    if (at>packet.size() || width>packet.size()-at) throw std::out_of_range("메타 요청의 실제 쓰기 물리 부족");
    for (std::size_t byte=0;byte<width;++byte) packet[at+byte]=static_cast<std::uint8_t>(value>>(8*byte)); //낮은 지정 BYTE부터 기록한다.
}
// df590의 header0/종류/version/초기 길이0만 작성한다.
void Header(std::span<std::uint8_t> packet,std::uint8_t type) { Put(packet,0,1,1);Put(packet,1,type,1);Put(packet,2,0,2); }
// 00435a60의제목부분일치를검사하고모든창의열거를계속한다.실제LPARAM폭으로결과주소를전달한다.
BOOL CALLBACK FindRootWindow(HWND window,LPARAM value) {
    std::array<char,256> title{};GetWindowTextA(window,title.data(),static_cast<int>(title.size()));if (std::strstr(title.data(),"Netstorm Root Server")) *reinterpret_cast<bool*>(value)=true;return TRUE;
}
}
// 현재 저장소를 검증하고 observer/기본 연결 종료를 설치하며 실제 소켓 열기는 Connect까지 지연한다.
MetaConnectionRuntime::MetaConnectionRuntime(Client& client,std::function<void(std::string_view)> progress,std::function<void(std::uint32_t)> request,std::function<void()> processState,std::function<void(std::string_view)> tell)
    :client_(client),progress_(std::move(progress)),request_(std::move(request)),processState_(std::move(processState)),tell_(std::move(tell)) {
    if (!progress_ || !request_ || !processState_ || !tell_ || !client_.PlayerRecords()->Initialized()) throw std::invalid_argument("메타 연결의 현재 Client/Player/HTML/실제Run 장치 필요");
    o::PacketDispatchHooks hooks;hooks.now=[this] { return static_cast<std::uint32_t>(client_.WallSeconds()); };
    hooks.record=[](auto&,auto,auto) {}; //World 없는 현재 메타의 선택적 기록은 미연결이다. 수신/로그인 성공을 합성하지 않는다.
    hooks.terminate=[](auto& connection) { connection.flags|=0x40U; }; //실제소켓/노드반납은각TCP/UDP소유자가수행한다.
    hooks.close=[](auto&) {};hooks.release=[](auto&) {}; //이 위치의 추가 외부 장치가 없으며 TCP 노드가 실제 close/free를 소유한다.
    const auto udpHooks=hooks;tcp_=std::make_unique<LocalTcpTransport>(client_.PlayerRecords()->Records(),client_.GameplayGlobals()->Server(),std::move(hooks),client_.ConnectionGlobals());
    udp_=std::make_unique<LocalUdpTransport>(client_.PlayerRecords()->Records(),client_.GameplayGlobals()->Server(),udpHooks,client_.ConnectionGlobals(),tcp_->Dispatch().Registrations()); //실제등록표와연결표를복제하지않고공유한다.
    auto& dispatch=tcp_->Dispatch();dispatch.Register(2,[this](auto sender,auto& connection,auto) {
        auto& local=*client_.LocalStateGlobals();o::ConnectionEndedHooks ended;ended.disconnect=[this](auto id) { client_.ConnectionGlobals()->Disconnect(id); };
        ended.saveServerFailure=[] { throw std::logic_error("메타10/11 밖의 Game Server9 종료 부모 미연결"); };ended.broadcastPlayer=[](auto) { throw std::logic_error("메타 전송의 게임 Player 종료 통지 미연결"); };
        o::ReceiveConnectionEnded(std::bit_cast<std::int32_t>(sender),client_.ConnectionGlobals()->TokenFor(connection),{client_.GameplayGlobals()->Server(),local.ClockSynchronization().connection,local.State().phase[0],local.State().next[0],client_.ConnectionActivityGlobals()->Frames(),[this] { return client_.PlayerRecords()->Records(); }},ended);
    }); //sender10/phase23의 실제 종료는 다음20을 예약한다.
    dispatch.ObservePacket(0x0f,[this](auto packet,auto sending) { o::ObserveRootLoginReply(packet,sending,[this](auto expression,auto line) { client_.SurfaceStorage()->AssertDisplay(expression,"\\Ns\\O\\_p_packets.cpp",line); }); });dispatch.Version(0x0f,1,0);dispatch.RequireLength(0x0f,0xffff);
    dispatch.Register(0x0f,[this](auto,auto&,auto packet) { ReceiveRootReply(packet); }); //원문과같이sender/연결번호·현재phase 관문을 수신 앞에 추가하지 않는다.
    dispatch.ObservePacket(0x16,[](auto packet,auto sending) {
        if (!sending) return;if (packet.size()<6) throw std::out_of_range("16h observer 필드 부족");if (packet[5]==0xffU) std::clog<<"_p_packets.cpp1112 numFriends 진단\n";
        const auto count=std::bit_cast<std::int8_t>(packet[5]);Put(packet,2,static_cast<std::uint16_t>(std::int32_t(count)*4+14),2);
    });dispatch.Version(0x16,1,0);dispatch.RequireLength(0x16,0xffff);
    dispatch.ObservePacket(0x1c,[](auto packet,auto sending) {
        if (!sending) return;if (packet.size()<46) throw std::out_of_range("1ch observer 필드 부족");if (packet[45]==0xffU) std::clog<<"_p_packets.cpp1226 nameLen 진단\n";Put(packet,2,std::uint32_t(packet[45])+46U,2);
    });dispatch.Version(0x1c,1,0);dispatch.RequireLength(0x1c,0xffff);
    dispatch.Observe(0x66,{});dispatch.Version(0x66,1,0);dispatch.RequireLength(0x66,12);
    dispatch.Register(0x66,[this](auto sender,auto&,auto packet) {
        o::LocalServerDiscoveryReceiveHooks receive;receive.wall=[this] { return client_.WallSeconds(); };receive.peerAddress=[this](auto id) { return CampaignPeerAddressText(client_.ConnectionGlobals()->Address(std::bit_cast<std::int32_t>(id))); };receive.processState=processState_;
        o::ReceiveLocalServerDiscovery(client_.LocalStateGlobals()->State(),Discovery(),sender,packet,receive);
    }); //서버발견은현재36직접쓰기뒤실제Run을동기실행한다.검색토큰비교만인경우Run하지않는다.
}
MetaConnectionRuntime::~MetaConnectionRuntime()=default; //자기OS/노드수명은실제TCP/UDP소유자의소멸자가반납한다.
// minor 차이는 로그만 남기고 major 차이만 현재 설정값/실제 Tell 결과로 거부한다.
bool MetaConnectionRuntime::CheckMetaVersion(std::uint32_t major,std::uint32_t minor) {
    if (major!=10 || minor!=78) {
        client_.WriteDiagnosticLog(0x1d,"VERSION DIFFERENCE: You are 10.78. Meta Server is "+std::to_string(major)+'.'+std::to_string(minor)+".\n");
        if (major!=10 && !client_.Configuration().GetInt("suppressVersionMessage")) { tell_("MetaVerDiff("+std::to_string(major)+'.'+std::to_string(minor)+')');return false; }
    }return true;
}
// 현재 Client의 같은 기록/phase/목록을 빌리며 별도 메타 표나 접속성공을 만들지 않는다.
void MetaConnectionRuntime::ReceiveRootReply(std::span<const std::uint8_t> packet) {
    auto& local=*client_.LocalStateGlobals();o::RootLoginReplyHooks hooks;hooks.version=[this](auto major,auto minor) { return CheckMetaVersion(major,minor); };
    hooks.clearZones=[this] { ClearClientMetaZoneRecords(client_,*client_.SurfaceStorage()); };
    hooks.checksum=[this](auto value) { client_.NsInfoGlobals()->SetServerValue(std::bit_cast<std::int32_t>(client_.LocalStateGlobals()->State().phase[0x44/4]),value); };
    hooks.server=[this](auto row) { client_.NsInfoGlobals()->UpsertServer(row,true); };hooks.saveServers=[this] { SaveClientRootServerList(client_); };
    o::ReceiveRootLoginReply(packet,{local.State(),local.MetaZones().records,local.MetaRefresh()},hooks);
}
bool MetaConnectionRuntime::CallbackActive() const { return pumping_; } //현재 대여 중인 이 owner를 Native cleanup이 반납하지 않게 한다.
// 기존slot4의TCP뒤UDP를진행한다.66h발견은원문처럼수신뒤같은Run을동기실행한다.
void MetaConnectionRuntime::Pump() { if (pumping_) throw std::logic_error("메타 TCP/UDP 중첩 Poll 금지");pumping_=true;try { tcp_->Flush();static_cast<void>(tcp_->Poll());static_cast<void>(udp_->Poll()); }catch (...) { pumping_=false;throw; }pumping_=false; }
// 완성한UTF-8문구를한번만번역한뒤호스트HTML장치가글꼴바이트로변환한다.
void MetaConnectionRuntime::Progress(std::string_view source,bool protocol) { auto text=client_.Translate(source);if (protocol) { const auto at=text.find("%s");if (at!=std::string::npos) text.replace(at,2,"TCP/IP"); }progress_(text); }
// 전송owner교체와무관한같은Client현재DWORD/double세개를빌린다.
o::LocalServerDiscoveryGlobals MetaConnectionRuntime::Discovery() { auto& local=*client_.LocalStateGlobals();return {local.LocalServerToken(),local.LocalServerSearchEnd(),local.LocalServerRetryEnd()}; }
// TCP/IP에해당하는원문분기만구현하며IPX의성공값/주소를대체하지않는다.
o::LocalServerDiscoveryHooks MetaConnectionRuntime::DiscoveryHooks() {
    o::LocalServerDiscoveryHooks hooks;hooks.wall=[this] { return client_.WallSeconds(); };hooks.open=[this] { static_cast<void>(udp_->Open(std::bit_cast<std::uint32_t>(client_.NsInfoGlobals()->rootServerBroadcastPort))); };
    hooks.broadcast=[this](auto packet) { static_cast<void>(udp_->Broadcast(packet,std::bit_cast<std::uint32_t>(client_.NsInfoGlobals()->rootServerBroadcastPort))); };
    hooks.progress=[this] { Progress("Searching for local %s server.",true); };hooks.palette=[this] { const auto path=client_.Configuration().PathSpec("chalPal");client_.ChangeFortPalette({reinterpret_cast<const std::uint8_t*>(path.data()),path.size()+1}); };
    hooks.serverWindowExists=[] { bool found=false;EnumWindows(FindRootWindow,reinterpret_cast<LPARAM>(&found));return found; };return hooks;
}
// 지역80BYTE/기본 포트 조회 뒤 같은 기존 socket 재사용/로컬 IPv4 정책을 적용한다.
bool MetaConnectionRuntime::Connect(std::string_view endpoint,std::uint32_t port,std::uint32_t challenge) {
    const auto selected=ResolveRootServerEndpoint(endpoint,port,[this] { return client_.Configuration().GetInt("rootServerBasePort"); });
    try { return tcp_->ConnectPeerText(selected.host,selected.port,challenge ? 11 : 10); }
    catch (const std::invalid_argument& error) { client_.WriteDiagnosticLog(9,std::string("메타 로컬 endpoint 접속 거부: ")+error.what()+"\n");return false; } //DNS/공개 주소와 잘못된 포트에 실제 외부 접속을 시도하지 않는다.
}
// 진단이 정상 복귀하면 같은 실제 current 포인터를 다시 읽는 기존 Core getter다.
std::span<std::uint8_t> MetaConnectionRuntime::Core() { const std::uint32_t mode=client_.SurfaceStorage()->ownerMode.challenge;return client_.CoreDataGlobals()->Read(client_.GameplayGlobals()->player,mode,{[](auto line) { std::clog<<"Player.cpp Core 진단:"<<line<<'\n'; }}); }
// 메타에서도 Session 사본 대신 현재 Client의 변경 가능한 TYPE 표를 조회한다.
std::uint8_t MetaConnectionRuntime::Weight(std::span<const std::uint8_t> core) {
    o::CoreTechnologyWeightHooks hooks;hooks.count=[this] { return static_cast<std::int32_t>(client_.SurfaceStorage()->types.size()); };
    hooks.type=[this](auto index) { const auto& type=client_.SurfaceStorage()->types.at(static_cast<std::size_t>(index));return o::CoreTechnologyWeightType{type.techBitIndex,type.level}; };return o::CoreTechnologyWeight(core,hooks);
}
// 미작성 scratch를 현재 할당 입력으로 보존하고 observer가 실제18BYTE 길이를 확정한다.
void MetaConnectionRuntime::SendRootLogin(std::uint32_t connection) {
    std::array<std::uint8_t,78> packet;CurrentAllocationBytes(packet);Header(packet,0x16);Put(packet,4,0,1);std::int32_t serverSubscriber{};
    client_.NsInfoGlobals()->ReadServerFields(std::bit_cast<std::int32_t>(client_.LocalStateGlobals()->State().phase[0x44/4]),{},{},&serverSubscriber);
    Put(packet,6,std::bit_cast<std::uint32_t>(serverSubscriber),4);Put(packet,5,1,1);Put(packet,14,client_.CurrentSubscriber(),4);Put(packet,10,Weight(Core()),4);static_cast<void>(tcp_->SendMutable(connection,packet));
}
// 현재 채팅 재생성→원문300BYTE 요청→Core/언어/CD/진단/이름 쓰기 순서를 유지한다.
void MetaConnectionRuntime::SendChallengeLogin() {
    InitializeClientChatManager(client_);std::array<std::uint8_t,300> packet;CurrentAllocationBytes(packet);Header(packet,0x1c);Put(packet,4,10,2);Put(packet,6,78,2);
    const auto subscriber=client_.CurrentSubscriber();Put(packet,11,subscriber ? subscriber : 0xffffffffU,4);auto& state=client_.LocalStateGlobals()->State().current;
    Put(packet,19,state[0x34/4],1);Put(packet,20,state[0x48/4],4);Put(packet,24,state[0x4c/4],2);const auto core=Core();if (core.size()<10) throw std::out_of_range("1ch 현재 Core10BYTE 부족");std::copy_n(core.begin(),10,packet.begin()+26);
    Put(packet,36,Weight(Core()),1);Put(packet,37,0,4);Put(packet,15,0,4);Put(packet,8,78,2);Put(packet,10,static_cast<std::uint32_t>(client_.CurrentLanguage()),1);
    if (!client_.CdGlobals()->Check(MakeWindowsCdPresenceHooks(&client_.StaticStrings()->FileNames()))) Put(packet,37,0x200,4);
    if (!client_.NsInfoGlobals()->passedDiags) { const auto flags=std::uint32_t(packet[37])|(std::uint32_t(packet[38])<<8);Put(packet,37,flags|0x800U,4); }
    const auto& handle=client_.GameplayGlobals()->Subscribers()->Handle();const auto end=std::find(handle.begin(),handle.end(),0);if (end==handle.end()) throw std::out_of_range("1ch 현재 handle의 실제 NUL 없음");const auto length=static_cast<std::size_t>(end-handle.begin())+1;
    Put(packet,45,static_cast<std::uint32_t>(length),1);std::copy_n(handle.begin(),length,packet.begin()+46);static_cast<void>(tcp_->SendMutable(11,packet)); //+29h의 미작성DWORD는 그대로 보존한다.
}
// 각 상태는 실제 현재 블록만 사용하고 아직 없는 서버 응답/메타 표시 상태를 만들지 않는다.
o::LocalMissionParentResult MetaConnectionRuntime::Run(std::uint32_t selected) {
    auto state=client_.LocalStateGlobals()->State();if (state.current[0]!=selected) throw std::logic_error("메타 연결의 실제 current 계약");
    switch (selected) {
    case 20:Progress("Failed to connect.");state.next[0]=3;break;
    case 21: {
        Progress("Contacting Root Server.");std::array<char,80> endpoint;endpoint[0]=0;client_.NsInfoGlobals()->ReadServerFields(std::bit_cast<std::int32_t>(state.current[0x44/4]),std::span<char>(endpoint).first(79),{},nullptr);
        if (!endpoint[0]) client_.WriteDiagnosticLog(9,"nextState.serverNum is invalid.\n");static_cast<void>(Connect(endpoint.data(),0,0));state.next[0]=22;break;
    }
    case 22: {
        const auto status=tcp_->Status(10);const auto open=tcp_->IsOpen(10);if (!open || status<0) { client_.ConnectionGlobals()->Disconnect(10);Progress("Root Server login failure.");request_(20); }
        else if (!status) state.next[0]=22;else { SendRootLogin(10);request_(23); }break;
    }
    case 23:Progress("Root Server connected. Awaiting login reply.");break;
    case 24:request_(25);break;
    case 25: {
        Progress("Contacting Challenge Server.");const auto index=std::bit_cast<std::int32_t>(state.current[0x44/4]);const auto& servers=client_.NsInfoGlobals()->servers;
        if (index<0 || static_cast<std::size_t>(index)>=servers.size()) { client_.WriteDiagnosticLog(9,"nextState.serverNum is invalid.\n");throw std::out_of_range("원문 NULL challenge endpoint의 물리 읽기 거부"); }
        static_cast<void>(Connect(servers[static_cast<std::size_t>(index)],0,1));state.next[0]=26;break;
    }
    case 26: {
        const auto status=tcp_->Status(11);const auto open=tcp_->IsOpen(11);if (!open || status<0) { client_.ConnectionGlobals()->Disconnect(11);state.next[0]=38; }
        else if (!status) state.next[0]=26;else { SendChallengeLogin();Progress("Challenge Server connected. Awaiting login reply.");request_(27); }break;
    }
    case 27:break; //원문4b874c 표의 실제4b86bd 공통 확정 직행 상태이며 로그인 응답은 별도 수신기가 예약한다.
    case 34:o::StartLocalServerSearch(state,Discovery(),client_.RandomGlobals()->Crt(),DiscoveryHooks());break;
    case 35:o::ContinueLocalServerSearch(state,Discovery(),client_.RandomGlobals()->Crt(),DiscoveryHooks());break;
    case 36: {
        Progress("Local %s server found.",true);udp_->Close();client_.ConnectionGlobals()->Disconnect(13);const auto bytes=std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(state.current.data()),sizeof(state.current));const auto first=bytes.begin()+8;const auto end=std::find(first,bytes.end(),0);
        if (end==bytes.end()) throw std::out_of_range("메타36현재주소NUL물리없음");static_cast<void>(Connect({reinterpret_cast<const char*>(bytes.data()+8),static_cast<std::size_t>(end-first)},state.current[0x30/4],1));state.next[0]=26;state.next[0x34/4]=1;break;
    }
    case 37: {
        NativeLocalServerLaunchHooks hooks;hooks.progress=[this] { Progress("Becoming local %s server.",true); };hooks.closeDiscovery=[this] { udp_->Close();client_.ConnectionGlobals()->Disconnect(13); };
        hooks.executable=[this] { std::string value;client_.Configuration().ReadString("rootServerExe",value);return value; }; //같은ReadString의변경여부와실제현재이름을구별한다.
        hooks.format=[this](auto prefix,auto name) { const auto root=platform::PathToUtf8(platform::CurrentExecutableFile().parent_path());const std::array<o::FileNamePart,3> parts{{{-2,root},{-2,prefix},{-3,name}}};return std::string(client_.Files().FileNameOwner()->Format(parts)); }; // 자료폴더와분리된현대배포에서는새R을클론모듈폴더기준의같은여섯순서로검색한다.
        hooks.exists=[this](auto name) { const auto source=client_.Files().Probe(name);return source && IsRebuiltLocalServer(name) ? source:0U; }; // 실제출처조회효과뒤새11.01서버식별을확인하며원본R을대신실행하지않는다.
        hooks.launch=[this](auto program,auto arguments,auto wait) { static_cast<void>(StartRebuiltLocalServer(program,arguments,wait,client_.GameDirectory())); }; // 실제시작실패도원문void부모처럼반환값을버리고다음38을확정한다. 준비성공알림은합성하지않는다.
        return InstallNativeLocalServerLaunch(state,hooks);
    }
    case 38: {
        const auto address=udp_->LocalAddress();if (!address[0] && !address[1]) { Progress("Root Server login failure.");request_(20); }
        else { auto endpoint=CampaignPeerAddressText(address);endpoint.resize(endpoint.rfind(':'));static_cast<void>(Connect(endpoint,0,1));state.immediate[0]=26;state.immediate[0x34/4]=1; }break;
    }
    default:throw std::logic_error("메타 연결/검색 밖의 상태 미연결");
    }return o::LocalMissionParentResult::Commit;
}
}
