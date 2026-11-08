// 실제 TCP/UDP와 원문 메타 연결/검색 상태를 같은 Client 전역에 직접 연결한다.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN //창제목조회에구형Winsock선언을불러오지않는다.
#endif
#ifndef NOMINMAX
#define NOMINMAX //표준min/max와Windows매크로의충돌을막는다.
#endif
#include <windows.h>
#include "client/MetaConnectionRuntime.h"
#include "client/ClientCampaignTransport.h" //같은 Client storage의 기본 미션 송신 관찰기를 TCP/UDP 공통 표에 설치한다.
#include "client/ClientMain.h"
#include "client/ClientPacketRecord.h" //메타도 같은 Client 수명의 선택적 실제 FILE에 기록한다.
#include "client/ClientStaticStrings.h"
#include "client/ClientConnectionGlobals.h"
#include "client/ClientConnectionActivityGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/RawCampaignSession.h" //type2가 빌린 현재 Session의 실제 장애 저장/Player 방송 본체를 호출한다.
#include "client/ClientLocalStateGlobals.h"
#include "client/ClientNsInfoGlobals.h"
#include "client/ClientPlayerRecords.h"
#include "client/ClientCoreDataGlobals.h"
#include "client/ClientSubscriberGlobals.h"
#include "client/ClientCdGlobals.h"
#include "client/ClientChatRuntime.h"
#include "client/RawChatRelay.h" //RequestMenu의확장41h응답도같은원문중계/관찰기와현재UI로전달한다.
#include "client/ClientChatWindowGlobals.h" //메타/일반이같은현재수신maskDWORD를공유한다.
#include "client/ClientChallengeGlobals.h"
#include "client/NativeMetaConnectedState.h"
#include "client/CurrentAllocationBytes.h"
#include "client/CdDirectory.h"
#include "client/RootServerEndpoint.h"
#include "client/LocalUdpTransport.h"
#include "client/NativeLocalServerLaunch.h"
#include "client/LocalServerProcess.h"
#include "client/RootServerListDisk.h"
#include "client/NativeMetaBattleOptions.h" //현재CS의실제67h수신/음향·NSInfo갱신을공통등록표에결합한다.
#include "client/NativeMetaParticipantName.h" //88h는 현재 CS 길드 표시/실제 이름표와 지정 참가자 갱신을 사용한다.
#include "client/NativeMetaParticipantCreate.h" //20h의 전체 실제 참가자 생성/CS 좌석·Chat/후행 표시다.
#include "client/NativeMetaParticipantRemove.h" //21h의 전체 캡처 CS 제거/같은 Chat/Rig/Image 수명이다.
#include "client/NativeMetaParticipantMove.h" //22h의 실제 같은 packet 변경/좌석·master·항해/Chat 후행이다.
#include "client/NativeMetaSailError.h" //25h의 실제 자기 객체 안내/Tell/지정 참가자 표시다.
#include "client/NativeCameraRig.h" //자기 참가자의 실제 입장 카메라를 같은 Kernel에 연결한다.
#include "client/RawChatChannels.h" //원문 packet 이름/채널20h를 같은 실제 Chat100기록에 저장한다.
#include "client/NativeMetaGeometry.h" //26h는 같은 Renderer 한계와 장기 CS 원점/반경을 갱신한다.
#include "client/Screen.h" //497f80은 표시 목표 크기와 구별되는 현재 논리 화면 폭/높이를 사용한다.
#include "o/RawStateMetaBattleDescription.h" //일반과 같은71h 전체 observer/State CS 관문이다.
#include "client/ClientMetaBattleDescription.h" //실제 CS 설명 자식과 현재 표시 장치다.
#include "o/RawStateBattleOptions.h" //메타도 일반 수신과 같은67h 전체 부모/복사 순서를 사용한다.
#include "o/RawMissionBattleOptions.h" //현재 공급 반경·TYPE 비용·환불 비율의 같은 순수 getter다.
#include "client/NativeMetaBattleCreate.h" //23h 생성 부모를 실제 공장·QA·Pop·Rig에 연결한다.
#include "client/NativeMetaSail.h" //좌석/임의 항해의 서로 다른 현재 기록 쓰기/송신 순서를 연결한다.
#include "o/RawRiftShapeAccess.h" //같은 현재 전투 TYPE의 C 프레임/포구와 복귀형 진단이다.
#include "client/ClientMetaBattleObjects.h"
#include "client/NativeIslandRig.h"
#include "client/NativeMetaBattleRemove.h" //24h 삭제는 같은 진입 CS와 실제 Rig/Image를 사용한다.
#include "client/ClientAudio.h" //원문의guy.wav는같은Client장기음향을사용한다.
#include "client/ClientStateStorageCleanup.h"
#include "client/NativeMetaDisplay.h" //영역 하강도 같은 현재CS/존의 실제 메타 객체 반납을 사용한다.
#include "client/ClientRandomGlobals.h"
#include "client/CampaignTransportServices.h"
#include "client/RawSurfaceWorld.h"
#include "o/RawConnectionEnded.h"
#include "o/RawPlayerNotification.h"
#include "o/RawFileName.h"
#include "o/RawRootLoginReply.h"
#include "o/RawChallengeLoginReply.h" //1dh는 현재28/12 직접 쓰기 뒤 같은 실제 Run을 동기 호출한다.
#include "o/RawStateBattleStart.h" //1ah/1bh는 같은 실제 Zone/State/기록 FILE·정리/직접Run을 사용한다.
#include "o/OriginalText.h" //현재 이름표 번역을 공통 영어/한국어 표시 바이트로 변환한다.
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
// 캡처한 같은 CS의 참가자 물리만 빌리며 active/master 조건을 추가하지 않는다.
std::span<std::uint8_t> PeerRecord(std::span<std::uint8_t> bytes,std::uint32_t index) {
    if (index>=o::kChallengeParticipantCount) throw std::out_of_range("메타 요청의 실제 참가자40칸 범위");
    const auto at=o::kChallengeParticipantOffset+index*o::kChallengeParticipantBytes;
    if (at>bytes.size() || o::kChallengeParticipantBytes>bytes.size()-at) throw std::out_of_range("메타 요청의 실제 CS 물리 부족");
    return bytes.subspan(at,o::kChallengeParticipantBytes);
}
// 원문 비정렬 DWORD를 낮은 BYTE부터 조립해 호스트 정렬/포인터 폭과 분리한다.
std::uint32_t PeerWord(std::span<const std::uint8_t> bytes,std::size_t at) {
    if (at>bytes.size() || 4>bytes.size()-at) throw std::out_of_range("메타 요청의 실제 DWORD 물리 부족");
    std::uint32_t value{};
    // 선택된 네 BYTE만 읽으며 다른 현재 기록을 복사하거나 초기화하지 않는다.
    for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[at+byte])<<(8*byte);
    return value;
}
//004b4160의 현재 Client 진단/Config/Tell만 사용하는 고정 버전 본문이다. 실제 Source나 Runtime을 요구하지 않는다.
bool CheckClientMetaVersion(Client& client,const std::function<void(std::string_view)>& tell,std::uint32_t major,std::uint32_t minor) {
    if (major!=10 || minor!=78) {
        client.WriteDiagnosticLog(0x1d,"VERSION DIFFERENCE: You are 10.78. Meta Server is "+std::to_string(major)+'.'+std::to_string(minor)+".\n");
        if (major!=10 && !client.Configuration().GetInt("suppressVersionMessage")) { tell("MetaVerDiff("+std::to_string(major)+'.'+std::to_string(minor)+')');return false; }
    }return true;
}
// 00435a60의제목부분일치를검사하고모든창의열거를계속한다.실제LPARAM폭으로결과주소를전달한다.
BOOL CALLBACK FindRootWindow(HWND window,LPARAM value) {
    std::array<char,256> title{};GetWindowTextA(window,title.data(),static_cast<int>(title.size()));if (std::strstr(title.data(),"Netstorm Root Server")) *reinterpret_cast<bool*>(value)=true;return TRUE;
}
}
// 현재 저장소를 검증하고 observer/기본 연결 종료를 설치하며 실제 소켓 열기는 Connect까지 지연한다.
MetaConnectionRuntime::MetaConnectionRuntime(Client& client,std::function<void(std::string_view)> progress,std::function<void(std::uint32_t)> request,std::function<void()> processState,std::function<void(std::string_view)> tell,std::function<void()> refreshInfo,std::shared_ptr<std::array<std::uint8_t,87>> options,std::function<void()> recreateHud,std::function<void(std::shared_ptr<MetaConnectionRuntime>,bool)> battleState,std::function<void(std::span<std::uint8_t>)> chatReceive)
    :client_(client),progress_(std::move(progress)),request_(std::move(request)),processState_(std::move(processState)),battleState_(std::move(battleState)),tell_(std::move(tell)),chatReceive_(std::move(chatReceive)),refreshInfo_(std::move(refreshInfo)),options_(std::move(options)),recreateHud_(std::move(recreateHud)) {
    if (!progress_ || !request_ || !processState_ || !battleState_ || !tell_ || !chatReceive_ || !refreshInfo_ || !options_ || !recreateHud_ || !client_.PlayerRecords()->Initialized()) throw std::invalid_argument("메타 연결의 현재 Client/Player/HTML/실제Run/전투Source/NSInfo/전역옵션/HUD/41h수신 장치 필요");
    o::PacketDispatchHooks hooks;hooks.now=[this] { return static_cast<std::uint32_t>(client_.WallSeconds()); };
    hooks.players=[players=client_.PlayerRecords()] { return std::span<const std::uint8_t>(players->Records()); }; //Native 송신이 선택한 시점의 현재 base이며 ctor의 낡은 span을 재사용하지 않는다.
    const auto record=client_.PacketRecordOwner();hooks.record=[record](auto& connection,auto packet,auto length) { record->Write(false,connection,packet,length); };hooks.sent=[record](auto& connection,auto packet,auto length) { record->Write(true,connection,packet,length); }; //World 없는 메타도 같은 현재 실제 프레임/주소/FILE를 사용하며 초기 disabled에서는 파일을 만들지 않는다.
    hooks.terminate=[](auto& connection) { connection.flags|=0x40U; }; //실제소켓/노드반납은각TCP/UDP소유자가수행한다.
    hooks.close=[](auto&) {};hooks.release=[](auto&) {}; //이 위치의 추가 외부 장치가 없으며 TCP 노드가 실제 close/free를 소유한다.
    const auto udpHooks=hooks;tcp_=std::make_shared<LocalTcpTransport>(client_.PlayerRecords()->Records(),client_.GameplayGlobals()->Server(),std::move(hooks),client_.ConnectionGlobals());
    udp_=std::make_shared<LocalUdpTransport>(client_.PlayerRecords()->Records(),client_.GameplayGlobals()->Server(),udpHooks,client_.ConnectionGlobals(),tcp_->Dispatch().Registrations()); //실제등록표와연결표를복제하지않고공유한다.
    auto& dispatch=tcp_->Dispatch();InstallClientPacketDefaults(client_,dispatch);ChatRelayHooks chat; //TCP/UDP 생성 뒤 같은 비임대 기본 표에 Client C3/관찰기/metadata를 설치한다.
    chat.read=[this](auto address)->std::uint32_t { switch (address) { case 0x594fbc:return client_.SurfaceStorage()->ownerMode.battle;case 0x540bc0:return client_.GameplayGlobals()->Server();case 0x540c74:return client_.CurrentSubscriber();default:throw std::out_of_range("메타41h중계의현재전역주소범위"); } };
    chat.address=[this](auto sender) { return client_.ConnectionGlobals()->Address(std::bit_cast<std::int32_t>(sender)); };
    chat.player=[this](auto subscriber) { if (!subscriber) client_.SurfaceStorage()->AssertDisplay("subId != INVALID_SUBSCRIBER_ID","\\Ns\\O\\Player.cpp",0xa5);const auto players=client_.PlayerRecords()->Records();for (std::uint32_t owner=1;owner<9;++owner) if (PeerWord(players,owner*180U+44U)==subscriber) return owner;return 0U; }; //같은현재Player1..8의첫subscriber일치만찾으며별도active관문을추가하지않는다.
    chat.channel=[this](auto owner) { if (client_.SurfaceStorage()->ownerMode.challenge) client_.SurfaceStorage()->AssertDisplay("!inMetamapMode","\\Ns\\O\\Player.cpp",0x134);if (owner>=9) throw std::out_of_range("메타41h중계의현재Player번호범위");return static_cast<std::int32_t>(owner&255U); };
    chat.connected=[this](auto id) { const auto registry=client_.ConnectionGlobals();const auto slots=registry->Cleanup();if (id<0 || static_cast<std::size_t>(id)>=slots.transceivers.size()) throw std::out_of_range("메타41h중계의현재연결번호범위");const auto token=slots.transceivers[static_cast<std::size_t>(id)];return token && !(registry->Node(token).flags&0x40U); }; //원문현재슬롯/NULL/닫힘관문이며socketACK를합성하지않는다.
    chat.send=[this](auto id,auto packet) { static_cast<void>(client_.ConnectionGlobals()->SendMutable(id,packet,client_.PlayerRecords()->Records())); };chat.receive=[this](auto packet,auto) { chatReceive_(packet); };
    chat.assertion=[this](auto line) { const auto expression=line==0x81dU ? "nameLen != (short)-1":line==0x81eU ? "bufferLen != (short)-1":"addrLen != (unsigned char)-1";client_.SurfaceStorage()->AssertDisplay(expression,"\\Ns\\O\\_p_packets.cpp",line); }; //현재패킷관찰기의실제식/줄번호를같은Client진단에전달하고복귀뒤길이계산을계속한다.
    RegisterChatPacket(dispatch,client_.ChatWindowGlobals()->receivedMask,std::move(chat)); //메타의같은TCP/UDP등록표에실제41h중계를설치하며일반Session인계시기존동일슬롯재등록을사용한다.
    dispatch.Register(2,[this](auto sender,auto& connection,auto) {
        auto& local=*client_.LocalStateGlobals();o::ConnectionEndedHooks ended;ended.disconnect=[this](auto id) { client_.ConnectionGlobals()->Disconnect(id); };
        ended.saveServerFailure=[this] { client_.GameplayGlobals()->RequireSession().SaveServerFailure(); };ended.broadcastPlayer=[this](auto owner) { client_.GameplayGlobals()->RequireSession().BroadcastConnectionPlayer(owner); }; //현재 실제 메타/게임 Session의 flags·공유 저장 writer·접속자별 방송을 사용한다.
        o::ReceiveConnectionEnded(std::bit_cast<std::int32_t>(sender),client_.ConnectionGlobals()->TokenFor(connection),{client_.GameplayGlobals()->Server(),local.ClockSynchronization().connection,local.State().phase[0],local.State().next[0],client_.ConnectionActivityGlobals()->Frames(),[this] { return client_.PlayerRecords()->Records(); }},ended);
    }); //sender9의 실제 장애 저장/1..8 방송과 sender10/phase23의 다음20을 원문 같은 부모가 선택한다. type1은 TCP 생성자가 같은 Registry에 이미 등록한다.
    dispatch.ObservePacket(0x0f,[this](auto packet,auto sending) { o::ObserveRootLoginReply(packet,sending,[this](auto expression,auto line) { client_.SurfaceStorage()->AssertDisplay(expression,"\\Ns\\O\\_p_packets.cpp",line); }); });dispatch.Version(0x0f,1,0);dispatch.RequireLength(0x0f,0xffff);
    dispatch.Register(0x0f,[this](auto,auto&,auto packet) { ReceiveRootReply(packet); }); //원문과같이sender/연결번호·현재phase 관문을 수신 앞에 추가하지 않는다.
    dispatch.ObservePacket(0x1a,[this](auto packet,auto sending) { o::StateBattleStartHooks h;h.assertion=[this](auto expression,auto file,auto line) { client_.SurfaceStorage()->AssertDisplay(expression,file,line); };o::ObserveBattleStartZone(packet,sending,h); });dispatch.Version(0x1a,1,0);dispatch.RequireLength(0x1a,0xffff);
    dispatch.Register(0x1a,[this](auto,auto&,auto packet) {
        auto& local=*client_.LocalStateGlobals();auto& game=*client_.GameplayGlobals();const auto record=client_.PacketRecordOwner();o::StateBattleStartHooks h;h.assertion=[this](auto expression,auto file,auto line) { client_.SurfaceStorage()->AssertDisplay(expression,file,line); };h.duplicate=[this,&local](auto bytes) { return local.ZonePackets().Duplicate(bytes,tcp_->Dispatch()); };h.openRecord=[record](auto name) { record->Open(name); };h.disconnect=[this](auto id) { client_.ConnectionGlobals()->Disconnect(id); };
        o::ReceiveBattleStartZone({const_cast<std::uint8_t*>(packet.data()),packet.size()},{local.State(),local.ZonePending(),game.GameServerPort(),game.Subscribers()->MySubscriber(),record->Requested(),client_.SurfaceStorage()->Checking()},h);battleState_(shared_from_this(),false); //원문 수신이 끝난 뒤 다음7의 원래 전체Source만 빌리며 새 상태/명령은 쓰지 않는다.
    }); //원문처럼 sender/known/메타 모드 관문 없이 checking 복귀 뒤 실제 같은 버퍼를 처리한다.
    dispatch.Observe(0x1b,{});dispatch.Version(0x1b,1,0);dispatch.RequireLength(0x1b,38);
    dispatch.Register(0x1b,[this](auto,auto&,auto packet) {
        auto& local=*client_.LocalStateGlobals();auto& game=*client_.GameplayGlobals();const auto record=client_.PacketRecordOwner();o::StateBattleStartHooks h;h.assertion=[this](auto expression,auto file,auto line) { client_.SurfaceStorage()->AssertDisplay(expression,file,line); };h.openRecord=[record](auto name) { record->Open(name); };h.run=[this] { battleState_(shared_from_this(),true); };
        o::ReceiveBattleStartPeer(packet,{local.State(),local.ZonePending(),game.GameServerPort(),game.Subscribers()->MySubscriber(),record->Requested(),client_.SurfaceStorage()->Checking()},game.Server(),h);
    }); //실제 세 번째 packet 인자를 넘기며 sender/known/메타 모드/추가 phase 관문을 합성하지 않는다.
    dispatch.Observe(0x1d,{});dispatch.Version(0x1d,1,0);dispatch.RequireLength(0x1d,13); //원문0b380의 빈 관찰기/버전1과 실제13BYTE의 고정 응답이다.
    dispatch.Observe(0x88,{});dispatch.Version(0x88,1,0);dispatch.RequireLength(0x88,98); //0040b380 빈 관찰기와 보호 PE0052ec40의 고정98BYTE다.
    dispatch.ObservePacket(0x20,[this](auto packet,auto sending) { ObserveNativeMetaParticipantAdd(packet,sending,[this](auto expression,auto line) { client_.SurfaceStorage()->AssertDisplay(expression,"\\Ns\\O\\_p_packets.cpp",line); }); });dispatch.Version(0x20,1,0);dispatch.RequireLength(0x20,0xffff);
    dispatch.Register(0x20,[this](auto,auto&,auto packet) { ReceiveParticipantAdd(packet); }); //원문 State의 메타 관문 뒤만 처리하며 sender/known/phase를 더 검사하지 않는다.
    dispatch.Observe(0x22,{});dispatch.Version(0x22,1,0);dispatch.RequireLength(0x22,17);dispatch.Register(0x22,[this](auto,auto&,auto packet) { ReceiveParticipantMove(packet); }); //원문 빈 observer/고정17BYTE의 전체 State 부모다.
    dispatch.Observe(0x25,{});dispatch.Version(0x25,1,0);dispatch.RequireLength(0x25,8);dispatch.Register(0x25,[this](auto,auto&,auto packet) { ReceiveSailError(packet); }); //원문 빈 observer/version1/policy0/고정8BYTE이며 sender/known 관문을 더하지 않는다.
    dispatch.ObservePacket(0x21,[this](auto packet,auto sending) { ObserveNativeMetaParticipantRemove(packet,sending,[this](auto expression,auto line) { client_.SurfaceStorage()->AssertDisplay(expression,"\\Ns\\O\\_p_packets.cpp",line); }); });dispatch.Version(0x21,1,0);dispatch.RequireLength(0x21,0xffff);
    dispatch.Register(0x21,[this](auto,auto&,auto packet) {
        if (!client_.SurfaceStorage()->ownerMode.challenge) return;if (!client_.ChallengeGlobals()->Current()) client_.SurfaceStorage()->AssertDisplay("CS",".\\State.cpp",0xd04);NativeMetaParticipantRemoveHooks h;h.checking=client_.SurfaceStorage()->Checking();h.assertion=[this](auto expression,auto file,auto line) { client_.SurfaceStorage()->AssertDisplay(expression,file,line); };h.log=[this](auto text) { client_.WriteDiagnosticLog(9,text); };
        h.battle=[this](auto id) { client_.UpdateNativeMetaBattle(id); };h.chat=[this](auto subscriber) { client_.ChatChannels().Forget(subscriber); };h.leave=[this](auto object,auto downward) { auto& rig=client_.AcquireNativeIslandRig(object,0,1);LeaveNativeIslandRig(rig,downward); };h.refreshInfo=refreshInfo_;ReceiveNativeMetaParticipantRemove(*client_.ChallengeGlobals(),packet,h);
    }); //4b46d0의 현재 메타/CS 진단 뒤 전체 제거를 연결하며 phase/known/sender를 추가하지 않는다.
    dispatch.ObservePacket(0x26,[this](auto packet,auto sending) { ObserveNativeMetaGeometry(packet,sending,[this](auto expression,auto line) { client_.SurfaceStorage()->AssertDisplay(expression,"\\Ns\\O\\_p_packets.cpp",line); }); });dispatch.Version(0x26,1,0);dispatch.RequireLength(0x26,0xffff);
    dispatch.Register(0x26,[this](auto,auto&,auto packet) {
        auto& challenge=*client_.ChallengeGlobals();ReceiveNativeMetaGeometry(packet,challenge.Geometry(),challenge.GeometryMode(),[this](float width,float height) { auto& screen=client_.GetScreen();client_.GameplayGlobals()->Camera().UpdateTargetLimits(screen.Width(),screen.Height(),width,height); });
    }); //4b45a0에는 CS NULL 진단/메타 모드/송신자/known 관문이 없다.
    dispatch.Register(0x88,[this](auto,auto&,auto packet) {
        NativeMetaParticipantNameHooks receive;receive.translate=[this](auto source) { const auto encoded=o::EncodeDisplayText(client_.Translate(source));return std::string(encoded.begin(),encoded.end()); };receive.weight=[this](auto core) { return Weight(core); };
        receive.update=[this](auto participant,auto mask) { client_.UpdateNativeMetaParticipant(participant,mask); };receive.refreshInfo=refreshInfo_;ReceiveNativeMetaParticipantName(*client_.ChallengeGlobals(),packet,receive);
    }); //원문의88h에는 메타 모드/CS 진단/known/송신자 관문이 없다.
    dispatch.RegisterCanonicalDirect(0x1d,0x004b8ac0U,[client=&client_,tell=tell_,run=processState_](auto,auto*,auto packet) { //Client의 현재 State/장기 UI Run만 필요하며 old Meta/Session이나 Runtime pin을 보유하지 않는다.
        o::ChallengeLoginReplyHooks reply;reply.log=[client](auto text) { client->WriteDiagnosticLog(0x1d,text); };reply.version=[client,tell](auto major,auto minor) { return CheckClientMetaVersion(*client,tell,major,minor); };reply.run=run;
        o::ReceiveChallengeLoginReply(packet,client->LocalStateGlobals()->State(),client->GameplayGlobals()->Subscribers()->MySubscriber(),reply);
    }); //sender/phase/known 필터나 가짜 성공을 추가하지 않고 실제 현재 Subscriber와 다섯 State 블록을 사용한다.
    dispatch.Version(0x16,1,0);dispatch.RequireLength(0x16,0xffff); //40a190은 앞선 공용 실제 진단/관찰기를 유지하고 Client receiver는 NULL이다.
    dispatch.Version(0x1c,1,0);dispatch.RequireLength(0x1c,0xffff); //40a270도 같은 진단 복귀/현재 BYTE 재조회 본체를 사용한다.
    dispatch.Observe(0x66,{});dispatch.Version(0x66,1,0);dispatch.RequireLength(0x66,12);
    dispatch.RegisterCanonicalDirect(0x66,0x004b8a10U,[client=&client_,run=processState_](auto sender,auto*,auto packet) { //고정 Native F만 보관하고 실제 sender 주소는 본문이 선택한 때 같은 현재 Registry에서 조회한다.
        o::LocalServerDiscoveryReceiveHooks receive;receive.wall=[client] { return client->WallSeconds(); };receive.peerAddress=[client](auto id) { return CampaignPeerAddressText(client->ConnectionGlobals()->Address(std::bit_cast<std::int32_t>(id))); };receive.processState=run;
        auto& local=*client->LocalStateGlobals();o::ReceiveLocalServerDiscovery(local.State(),{local.LocalServerToken(),local.LocalServerSearchEnd(),local.LocalServerRetryEnd()},sender,packet,receive);
    }); //서버발견은현재36직접쓰기뒤실제Run을동기실행한다.검색토큰비교만인경우Run하지않는다.
    dispatch.Version(0x1f,1,0);dispatch.RequireLength(0x1f,12); //고정12BYTE/master 준비 통지는 앞선 비NULL 빈 관찰기를 유지하며 Client 수신기는 NULL이다.
    dispatch.Version(0x1e,1,0);dispatch.RequireLength(0x1e,14); //고정14BYTE 통지도 앞선 비NULL40b380 관찰기/NULL Client 수신기를 보존한다.
    dispatch.Observe(0x7c,{});dispatch.Version(0x7c,1,0);dispatch.RequireLength(0x7c,28); //0040b340/0040b380의 고정28BYTE/빈 관찰기다. 게임 로그인 수신은 별도 부모다.
    dispatch.Version(0x67,1,0);dispatch.RequireLength(0x67,0xffff); //송신 관찰기는 앞선 공용 기본 표를 유지하고 기존 버전/길이/수신 부모만 연결한다.
    dispatch.Register(0x67,[this](auto sender,auto&,auto packet) { ReceiveBattleOptions(sender,packet); }); //원문과같이sender/phase/known 관문 없이 전체 State 부모를 호출한다.
    o::RegisterStateMetaBattleDescription(dispatch,client_.ChallengeGlobals()->Current(),[this](auto,auto packet) { ReceiveNativeMetaBattleDescription(*client_.ChallengeGlobals(),packet,MakeClientMetaBattleDescriptionHooks(client_)); },[this](auto expression) { client_.SurfaceStorage()->AssertDisplay(expression,"\\Ns\\O\\_p_packets.cpp",0xae3); }); //sender/phase/known/mode 조건을 추가하지 않는다.
    dispatch.Observe(0x23,{});dispatch.Version(0x23,1,0);dispatch.RequireLength(0x23,20); //0040b380의 빈 관찰기와 보호 PE의 고정20BYTE다.
    dispatch.Register(0x23,[this](auto,auto&,auto packet) {
        if (!client_.SurfaceStorage()->ownerMode.challenge) return;if (!client_.ChallengeGlobals()->Current()) client_.SurfaceStorage()->AssertDisplay("CS",".\\State.cpp",0xce6);NativeMetaBattleCreateHooks receive;BindClientMetaBattleObjects(receive,client_);
        receive.animate=[this](auto object,auto delay) { auto& rig=client_.AcquireNativeIslandRig(object,12.0F,0);EnterNativeBattleRig(rig,delay); };ReceiveNativeMetaBattleAdd(*client_.ChallengeGlobals(),packet,receive);
    }); //004b4640의 메타 관문/CS 진단 뒤 전체 생성 부모를 호출하며 sender/phase/known 조건은 추가하지 않는다.
    dispatch.Observe(0x24,{});dispatch.Version(0x24,1,0);dispatch.RequireLength(0x24,5); //원문의 빈 관찰기와 고정5BYTE다.
    dispatch.Register(0x24,[this](auto,auto&,auto packet) {
        if (!client_.SurfaceStorage()->ownerMode.challenge) return;if (!client_.ChallengeGlobals()->Current()) client_.SurfaceStorage()->AssertDisplay("CS",".\\State.cpp",0xcdd);NativeMetaBattleRemoveHooks receive;receive.log=[this](auto text) { client_.WriteDiagnosticLog(9,text); };receive.assertion=[this](auto expression,auto file,auto line) { client_.SurfaceStorage()->AssertDisplay(expression,file,line); };
        receive.leave=[this](auto object) { auto& rig=client_.AcquireNativeIslandRig(object,0.0F,1);LeaveNativeIslandRig(rig,false); };ReceiveNativeMetaBattleRemove(*client_.ChallengeGlobals(),packet,receive);
    }); //004b4600의 원래 메타 관문/CS 진단·삭제 부분 효과를 그대로 유지한다.
}
MetaConnectionRuntime::~MetaConnectionRuntime()=default; //자기OS/노드수명은실제TCP/UDP소유자의소멸자가반납한다.
// 현재 자기 전투를 한 번 캡처하고0..39의 실제 active/같은 전투 기록만 같은 장부에 쓴다.
void MetaConnectionRuntime::RefreshParticipantCore() {
    auto& game=*client_.GameplayGlobals();if (game.world.missionPost || !game.player.playerKnown) return;auto& challenge=*client_.ChallengeGlobals();const auto own=challenge.Participant(game.player.battleOwner);const auto battle=PeerWord(own,0xa4);if (!battle) return;
    for (std::uint32_t index=0;index<40;++index) { //원문의8묶음×5기록을 같은 인덱스 순서로 읽는다.
        const auto peer=challenge.Participant(index);if (!(peer[0x64]&1U) || PeerWord(peer,0xa4)!=battle) continue;std::array<std::uint8_t,10> core;std::copy_n(peer.begin()+0x54,10,core.begin());client_.CoreDataGlobals()->StoreMetadata(PeerWord(peer,0xa8)+1U,PeerWord(peer,4),core); //595370+slot*16은595360의slot+1행이며0번 장부를 덮지 않는다.
    }
}
// 생성 앞의 active 진단이 끝난 뒤 현재 실제 Frame/UI 장치를 선택한다.
void MetaConnectionRuntime::ConstructParticipant(std::span<std::uint8_t> record,std::span<const std::uint8_t> packet) {
    NativeMetaParticipantCreateHooks h;client_.BindNativeMetaParticipantDevices(h);h.participantType=[this] { return client_.ChallengeGlobals()->ParticipantType(); };
    h.guildChanged=[this](auto subscriber) { std::array<std::uint8_t,80> scratch;CurrentAllocationBytes(scratch);const auto key=std::string("M")+std::to_string(std::bit_cast<std::int32_t>(subscriber));return client_.Configuration().ReadStringBytes(key,scratch); };
    h.guild=[this](auto subscriber,auto destination) { std::array<std::uint8_t,80> scratch;CurrentAllocationBytes(scratch);const auto key=std::string("M")+std::to_string(std::bit_cast<std::int32_t>(subscriber));if (client_.Configuration().ReadStringBytes(key,scratch)) static_cast<void>(client_.Configuration().ReadStringBytes(key,destination)); }; //42ca60은 두 번째 scratch 확인이 바뀐 경우에만 실제 목적을 읽는다.
    h.refreshCore=[this] { RefreshParticipantCore(); };h.status=[this](auto kind,auto participant,auto enabled) { client_.SetNativeMetaParticipantStatus(kind,std::bit_cast<std::int32_t>(participant),enabled); };
    h.image=[this](auto header,auto identity) { auto bitmap=client_.GameplayGlobals()->RequireSession().BuildEditorIslandChoiceImage(std::span<const std::uint8_t,6>(identity));client_.ChallengeGlobals()->ConvertImage(header,std::move(bitmap),true,[this](auto expression,auto line) { client_.SurfaceStorage()->AssertDisplay(expression,".\\Image.cpp",line); }); };
    h.name.translate=[this](auto text) { const auto bytes=o::EncodeDisplayText(client_.Translate(text));return std::string(bytes.begin(),bytes.end()); };h.name.weight=[this](auto core) { return Weight(core); };
    h.enter=[this](auto object,auto self) { auto& rig=client_.AcquireNativeIslandRig(object,0,0);EnterNativeParticipantRig(rig,self,0); };h.intro=[this] { StartClientMetaIntroCamera(client_); };h.noIntro=[this] { return client_.Configuration().GetInt("noShowChalIntro"); };h.tell=tell_;h.monitor=[this](auto name) { client_.MonitorMetaParticipantName(name); };
    ConstructNativeMetaParticipant(record,packet,client_.GameplayGlobals()->GameAux()->SubscriberMarker(),client_.GameplayGlobals()->Subscribers()->MySubscriber(),h);
}
// 자기Core 선택/두 예약 증가가 전체 생성 앞에 일어나며 실패해도 앞선 효과를 되돌리지 않는다.
void MetaConnectionRuntime::ReceiveParticipantAdd(std::span<const std::uint8_t> packet) {
    if (!client_.SurfaceStorage()->ownerMode.challenge) return;auto& challenge=*client_.ChallengeGlobals();if (!challenge.Current()) client_.SurfaceStorage()->AssertDisplay("CS",".\\State.cpp",0xd0d);
    if (PeerWord(packet,5)==client_.CurrentSubscriber()) { if (packet.size()<5) throw std::out_of_range("20h 자기 참가자 BYTE 물리 부족");auto& game=*client_.GameplayGlobals();const std::uint32_t metamap=client_.SurfaceStorage()->ownerMode.challenge;client_.CoreDataGlobals()->Select(game.player,metamap,packet[4],1);++client_.LocalStateGlobals()->MetaPending();++client_.LocalStateGlobals()->MetaRefresh(); }
    NativeMetaParticipantAddHooks h;h.assertion=[this](auto expression,auto file,auto line) { client_.SurfaceStorage()->AssertDisplay(expression,file,line); };h.construct=[this](auto record,auto source) { ConstructParticipant(record,source); };h.battle=[this](auto id) { client_.UpdateNativeMetaBattle(id); };h.seat=SailDevices().seat;
    h.chat=[this](auto subscriber,auto name,auto flags) { if (subscriber==client_.CurrentSubscriber()) { static_cast<void>(client_.ChatChannels().Add(subscriber,std::nullopt,flags));return; }const auto end=std::find(name.begin(),name.end(),0);if (end==name.end()) throw std::out_of_range("20h Chat packet 이름의 실제 NUL 물리 부족");static_cast<void>(client_.ChatChannels().Add(subscriber,std::string_view(reinterpret_cast<const char*>(name.data()),static_cast<std::size_t>(end-name.begin())),flags)); }; //자기 채널의 원문 조기 반환에서는 이름을 먼저 읽지 않는다.
    h.update=[this](auto participant,auto mask) { client_.UpdateNativeMetaParticipant(participant,mask); };h.refreshInfo=refreshInfo_;ReceiveNativeMetaParticipantAdd(challenge,packet,h);
}
// 실제 수신 frame은 TCP/UDP/zlocal이 소유한 mutable 저장소이며 기존 집합 observer도 같은 버퍼를 사용한다.
void MetaConnectionRuntime::ReceiveParticipantMove(std::span<const std::uint8_t> packet) {
    if (!client_.SurfaceStorage()->ownerMode.challenge) return;if (!client_.ChallengeGlobals()->Current()) client_.SurfaceStorage()->AssertDisplay("CS",".\\State.cpp",0xd3c);NativeMetaParticipantMoveHooks h;h.owner=[this] { return client_.GameplayGlobals()->player.battleOwner; };h.assertion=[this](auto expression,auto file,auto line) { client_.SurfaceStorage()->AssertDisplay(expression,file,line); };
    h.kick=[this](auto battle,auto slot) { ExplodeBattleSlot(battle,slot); };h.battle=[this](auto id) { client_.UpdateNativeMetaBattle(id); };h.kickedSound=[this] { client_.WorldAudio().Sounds().PlayByName("kickedoff.wav",0,0,0,1,0); };h.dropSound=[this] { client_.WorldAudio().Sounds().PlayByName("dropPiece.wav",0,0,0,0,0); };h.thunder=[this](float x,float y) { client_.WorldAudio().Sounds().PlayNameAt(x,y,"thundercrack.wav",0); };h.music=[this] { client_.WorldAudio().RequestMusic("anticipation.mus"); };const auto sail=SailDevices();h.seat=sail.seat;h.animate=sail.animate;
    h.resetOptions=[](auto options) { o::ResetMissionBattleOptions(o::OriginalEdition::Patch1078,options); };h.sendOptions=[this](auto battle,auto options) { SendMetaData(battle,options,87); };h.chatMode=[this](auto mode) { client_.RefreshNativeMetaChatMode(mode); };h.chatMembers=[this] { RefreshClientBattleChatMembers(client_); };h.refreshInfo=refreshInfo_;h.update=[this](auto participant,auto mask) { client_.UpdateNativeMetaParticipant(participant,mask); };h.refreshCore=[this] { RefreshParticipantCore(); };
    h.rebuildName=[this](auto participant) { NativeMetaParticipantNameHooks name;name.translate=[this](auto text) { const auto bytes=o::EncodeDisplayText(client_.Translate(text));return std::string(bytes.begin(),bytes.end()); };name.weight=[this](auto core) { return Weight(core); };RebuildNativeMetaParticipantLabel(client_.ChallengeGlobals()->Participant(participant),name); };auto& game=*client_.GameplayGlobals();ReceiveNativeMetaParticipantMove(*client_.ChallengeGlobals(),{const_cast<std::uint8_t*>(packet.data()),packet.size()},game.Subscribers()->MySubscriber(),game.player.playerKnown,h); //원본packet5 bit8을 같은 실제 입력에만 쓰며 사본/readonly 임시 입력을 만들지 않는다.
}
// 메타가0이면 CS/packet을 읽지 않으며 원문 CS 진단이 복귀하면 전체 자식으로 계속한다.
void MetaConnectionRuntime::ReceiveSailError(std::span<const std::uint8_t> packet) {
    if (!client_.SurfaceStorage()->ownerMode.challenge) return;if (!client_.ChallengeGlobals()->Current()) client_.SurfaceStorage()->AssertDisplay("CS",".\\State.cpp",0xd33);NativeMetaSailErrorHooks h;
    h.translate=[this](auto text) { const auto bytes=o::EncodeDisplayText(client_.Translate(text));return std::string(bytes.begin(),bytes.end()); };h.note=[this](auto object,auto text) { client_.ShowNativeObjectNote(object,text); };h.tell=tell_;h.update=[this](auto participant,auto mask) { client_.UpdateNativeMetaParticipant(participant,mask); };ReceiveNativeMetaSailError(*client_.ChallengeGlobals(),packet,client_.GameplayGlobals()->Subscribers()->MySubscriber(),h);
}
// 비영/활성 관문과 포구 처리는 원문 자식에 맡기며 실제 지면 효과의 공유 난수도 그대로 소비한다.
void MetaConnectionRuntime::ExplodeBattleSlot(std::uint32_t battle,std::uint32_t slot) {
    NativeMetaBattleSlotHooks h;h.assertion=[this](auto expression,auto file,auto line) { client_.SurfaceStorage()->AssertDisplay(expression,file,line); };h.shape=SeatShapeDevices();h.raw=[this](auto object)->std::span<const std::uint8_t> { const auto core=client_.GameplayGlobals()->RequireSession().Core();if (object>65535U || object>=core.pool.Capacity()) throw std::out_of_range("메타 슬롯 효과의 실제 DWORD SID 물리 범위");return core.pool.Slot(o::Sid{static_cast<std::uint16_t>(object)}); };
    h.ground=[this](float x,float y) { client_.GameplayGlobals()->RequireSession().GroundImpact(x,y); };h.sound=[this](float x,float y) { client_.WorldAudio().Sounds().PlayNameAt(x,y,"explodeSlot.wav",0); };ExplodeNativeMetaBattleSlot(*client_.ChallengeGlobals(),std::bit_cast<std::int32_t>(battle),std::bit_cast<std::int32_t>(slot),client_.GameplayGlobals()->Camera().Snapshot(),h);
}
// 장치/전역은 실제 Client 주소를 빌리며 CS가0이어도 전역 옵션과 파생값 갱신을 수행한다.
void MetaConnectionRuntime::ReceiveBattleOptions(std::uint32_t sender,std::span<const std::uint8_t> packet) {
    auto& game=*client_.GameplayGlobals();auto& local=*client_.LocalStateGlobals();auto& storage=*client_.SurfaceStorage();
    o::ReceiveStateBattleOptions(sender,packet,{*options_,client_.ChallengeGlobals()->Current(),game.GameAux()->RefundPercent(),storage.ownerMode.battle,game.world.missionPost,local.SavedGame()},{
        [this](auto expression) { client_.SurfaceStorage()->AssertDisplay(expression,".\\State.cpp",0xd51); },
        [this](auto,auto bytes) { NativeMetaBattleOptionsHooks receive;receive.owner=[this] { return client_.GameplayGlobals()->player.battleOwner; };receive.log=[this](auto text) { client_.WriteDiagnosticLog(9,text); };receive.guySound=[this] { client_.WorldAudio().Sounds().PlayByName("guy.wav",0,0,0,0,0); };receive.refreshInfo=refreshInfo_;ReceiveNativeMetaBattleOptions(*client_.ChallengeGlobals(),bytes,receive); },
        [this] { auto& current=*client_.GameplayGlobals();o::UpdateMissionOptionSupplyRange(*options_,current.ManaRanges(),current.ManaRadius(),current.energy.radiusSquared); },
        [this] { return o::MissionOptionTypeCost(*options_); },
        [this](float cost) { auto& current=*client_.SurfaceStorage();const auto type=client_.GameplayGlobals()->GameAux()->GeyserType();if (type>=current.types.size()) throw std::out_of_range("메타67h 현재 비용 TYPE 물리 범위");current.types[type].cost=cost; },
        [this] { return o::MissionOptionRefundPercent(*options_); },recreateHud_}); //임시 함수 장치의 수명은 이 전체 동기 호출 끝까지 유지된다.
}
// minor 차이는 로그만 남기고 major 차이만 현재 설정값/실제 Tell 결과로 거부한다.
bool MetaConnectionRuntime::CheckMetaVersion(std::uint32_t major,std::uint32_t minor) { return CheckClientMetaVersion(client_,tell_,major,minor); } //기존 Root 응답도 같은 현재 버전 본문을 공유한다.
// 현재 Client의 같은 기록/phase/목록을 빌리며 별도 메타 표나 접속성공을 만들지 않는다.
void MetaConnectionRuntime::ReceiveRootReply(std::span<const std::uint8_t> packet) {
    auto& local=*client_.LocalStateGlobals();o::RootLoginReplyHooks hooks;hooks.version=[this](auto major,auto minor) { return CheckMetaVersion(major,minor); };
    hooks.clearZones=[this] { ClearClientMetaZoneRecords(client_,*client_.SurfaceStorage()); };
    hooks.checksum=[this](auto value) { client_.NsInfoGlobals()->SetServerValue(std::bit_cast<std::int32_t>(client_.LocalStateGlobals()->State().phase[0x44/4]),value); };
    hooks.server=[this](auto row) { client_.NsInfoGlobals()->UpsertServer(row,true); };hooks.saveServers=[this] { SaveClientRootServerList(client_); };
    o::ReceiveRootLoginReply(packet,{local.State(),local.MetaZones().records,local.MetaRefresh()},hooks);
}
bool MetaConnectionRuntime::CallbackActive() const { return pumping_; } //현재 대여 중인 이 owner를 Native cleanup이 반납하지 않게 한다.
MissionNetwork MetaConnectionRuntime::SharedNetwork() { return {tcp_,udp_,shared_from_this()}; } //같은 실제 owner를 pin하며 원문 연결/Player 값을 새로 쓰지 않는다.
// 실제 C 프레임/포구와 복귀형 진단을 항해와 추방 효과가 공유한다.
NativeMetaSeatHooks MetaConnectionRuntime::SeatShapeDevices() {
        auto& storage=*client_.SurfaceStorage();NativeMetaSeatHooks shape;shape.firstFrame=[&storage](auto type) { return storage.frames.at(type).Run('C').first; };
        shape.muzzle=[&storage](auto type,auto frame) { const auto& selected=storage.types.at(type);const auto& current=storage.shapes.at(type);o::RiftShapeAccessHooks access;
            access.typeNumber=[](auto value) { return value.token; };access.frameError=[&storage](auto number,auto index,auto abstract) { storage.RecordDisplayFrameError(number,index,abstract); };access.assertion=[&storage](auto expression,auto file,auto line) { storage.AssertDisplay(expression,file,line); };
            access.shape=[&storage](auto number,auto index) { const auto& value=storage.shapes.at(number);if (!value.loaded || index<0 || static_cast<std::size_t>(index)>=value.frames.size()) throw std::out_of_range("메타 좌석 진단복귀 뒤 SHP 포구 물리 범위");const auto& header=value.frames[static_cast<std::size_t>(index)];return std::array<float,2>{static_cast<float>(header.muzzleX),static_cast<float>(header.muzzleY)}; };
            return o::ReadRiftShapeFrame({type,selected.flags1,current.frameCount,current.loaded,selected.maxHitPoints},frame,0,access);
        };return shape;
}
// 실제 포구/SHP 조회 뒤 같은 CS 좌표를 읽고 현재 Rig의 목표만 변경한다.
NativeMetaSailHooks MetaConnectionRuntime::SailDevices() {
    NativeMetaSailHooks hooks;hooks.subscriber=[this] { return client_.CurrentSubscriber(); };hooks.send=[this](auto packet) { static_cast<void>(tcp_->SendMutable(11,packet)); };
    hooks.animate=[this](auto record) { auto& rig=client_.AcquireNativeIslandRig(PeerWord(record,0xb8),0,0);MoveNativeParticipantRig(rig,record); };
    hooks.seat=[this](auto participant,auto battle,auto seat) { const auto shape=SeatShapeDevices();return NativeMetaSeatPosition(*client_.ChallengeGlobals(),participant,battle,seat,client_.GameplayGlobals()->Camera().RendererView(),shape); };return hooks;
}
void MetaConnectionRuntime::JoinBattle(std::uint32_t parameter) { JoinNativeMetaBattle(*client_.ChallengeGlobals(),parameter,SailDevices()); } //실제 입력을 선택한 때만 현재 장치를 구성한다.
void MetaConnectionRuntime::SailPosition(std::array<std::uint32_t,2> position) { SailNativeMetaParticipant(*client_.ChallengeGlobals(),position,SailDevices()); } //송신 뒤 조회하는 현재 참가자와 같은 Rig를 사용한다.
//설명 모달이 닫히기 전 현재 자기 CS와 권한을 다시 읽고 같은 TCP11/가변 길이 관찰기를 사용한다.
void MetaConnectionRuntime::SendBattleDescription(std::string_view description) {
    NativeMetaDescriptionSendHooks hooks;hooks.assertion=[this](auto expression,auto file,auto line) { client_.SurfaceStorage()->AssertDisplay(expression,file,line); };
    hooks.denied=[this] { tell_("MayNotBattleUpdate"); };hooks.send=[this](auto packet) { static_cast<void>(tcp_->SendMutable(11,packet)); };
    SendNativeMetaBattleDescription(*client_.ChallengeGlobals(),client_.GameplayGlobals()->player.battleOwner,-1,description,0,hooks);
}
// 현재 자기 CS 기록을 먼저 캡처하고 원문처럼 로컬 flags를 실제 송신 관문보다 앞서 쓴다.
void MetaConnectionRuntime::NotifyMetaMaster() {
    const auto own=client_.ChallengeGlobals()->Participant(client_.GameplayGlobals()->player.battleOwner);
    SendMetaStatus([this,own](auto packet) { Put(packet,4,client_.CurrentSubscriber(),4);const auto flags=PeerWord(own,0x64)|0x2000U;Put(own,0x64,flags,4);Put(packet,8,flags,4); }); //실제헤더→현재subscriber→원문flags쓰기→같은송신관문의순서를보존한다.
}
// 헤더뒤에만caller의현재필드/부분효과를실행하므로준비/시작/추방부모도원문순서를유지한다.
void MetaConnectionRuntime::SendMetaStatus(const std::function<void(std::span<std::uint8_t>)>& writePayload) {
    std::array<std::uint8_t,12> packet;Header(packet,0x1f);Put(packet,2,12,2);if (!writePayload) throw std::logic_error("메타1fh의 실제 payload writer 미연결");writePayload(packet);static_cast<void>(tcp_->SendMutable(11,packet)); //송신실패여도앞서쓴CS/버튼/음향등을되돌리지않는다.
}
// 87BYTE 전투옵션을포함한원문160미만자료를같은실제67h 관찰기/전송장치에전달한다.
void MetaConnectionRuntime::SendMetaData(std::uint32_t key,std::span<const std::uint8_t> data,std::uint32_t count) {
    if (std::bit_cast<std::int32_t>(count)>=160) return;if (count>159U) throw std::out_of_range("메타자료의 음수 원문 할당/복사 물리 범위");std::array<std::uint8_t,166> scratch;const auto size=count+7U;auto packet=std::span(scratch).first(size);std::fill(packet.begin(),packet.end(),std::uint8_t{0xff});Header(packet,0x67);Put(packet,2,4,2);Put(packet,4,key,1);Put(packet,5,count,2);
    // rep movsd는각실제DWORD를읽고쓴뒤마지막BYTE들을진행한다.미정다른scratch는초기화하지않는다.
    std::uint32_t offset{};for (;offset+4U<=count;offset+=4U) { if (offset>data.size() || 4U>data.size()-offset) throw std::out_of_range("메타자료의 실제 source DWORD 부족");const std::array<std::uint8_t,4> word{data[offset],data[offset+1],data[offset+2],data[offset+3]};std::copy(word.begin(),word.end(),packet.begin()+7+offset); }
    for (;offset<count;++offset) { if (offset>=data.size()) throw std::out_of_range("메타자료의 실제 source BYTE 부족");packet[7+offset]=data[offset]; } //원문나머지0..3BYTE만추가로쓴다.
    static_cast<void>(tcp_->SendMutable(11,packet)); //같은실제sender관문을통과할때만0040ae80이WORD길이를확정한다.
}
// master 진단이 복귀하면 현재 CS를 다시 읽고 같은 스냅샷의 master/자기 필드를 순서대로 전송한다.
void MetaConnectionRuntime::SendMetaGameLogin() {
    std::array<std::uint8_t,28> packet;Header(packet,0x7c);Put(packet,2,28,2);Put(packet,4,0,4);
    auto& challenge=*client_.ChallengeGlobals();const auto master=FindMetaBattleMaster(challenge,client_.GameplayGlobals()->player.battleOwner);
    if (master<0) client_.SurfaceStorage()->AssertDisplay("master >= 0",".\\State.cpp",0x6f7);
    const auto bytes=challenge.Bytes();Put(packet,8,PeerWord(PeerRecord(bytes,std::bit_cast<std::uint32_t>(master)),4),4);
    const auto subscriber=client_.CurrentSubscriber();const auto owner=client_.GameplayGlobals()->player.battleOwner;
    Put(packet,12,owner,4);const auto own=PeerRecord(bytes,owner);Put(packet,16,subscriber,4);
    Put(packet,20,PeerWord(own,0xa4),4);Put(packet,24,PeerWord(own,0xa8),4);
    static_cast<void>(tcp_->SendMutable(9,packet)); //PlayerKnown/접속 상태/ACK를 새로 합성하지 않고 기존 송신 관문을 사용한다.
}
// 영역 확인창의 Yes가 현재 메타를 떠나 같은19 부모를 직접 실행한다. 원문은 표시 정리 후의 존 바이트를 보관한다.
void MetaConnectionRuntime::DescendToZone(std::int32_t zone) {
    if (!client_.SurfaceStorage()->ownerMode.challenge) return;
    if (zone<0 || static_cast<std::uint32_t>(zone)>=o::kMetaZoneCleanupCount) throw std::out_of_range("메타 영역 하강의 실제51기록 물리 범위");
    auto& local=*client_.LocalStateGlobals();
    if (PeerWord(local.MetaZoneRecords(),static_cast<std::size_t>(zone)*o::kMetaZoneCleanupStride+0x84)) { tell_("ZoneLocked");return; }
    const auto owner=shared_from_this(); //같은19의 정리가 메타 전송 슬롯을 반납해도 현재 동기 호출과 원문 송신 owner를 복귀까지 유지한다.
    SendMetaStatus([this](auto packet) { Put(packet,4,client_.CurrentSubscriber(),4);Put(packet,8,0x80,4); });
    for (std::uint32_t index=0;index<o::kMetaZoneCleanupCount;++index) { //송신 뒤 현재51기록을 다시 읽어 요청 ID와 처음 일치하는 영역의 원문 위치로 항해한다.
        const auto record=local.MetaZoneRecords().subspan(std::size_t(index)*o::kMetaZoneCleanupStride,o::kMetaZoneCleanupStride); //같은 장기 QA 표의 현재 한 기록이며 잠금 검사 인덱스와 ID 검색을 구별한다.
        if (PeerWord(record,0)!=std::bit_cast<std::uint32_t>(zone)) continue;SailPosition({PeerWord(record,0x88),PeerWord(record,0x8c)});break;
    }
    auto state=local.State();state.current[0x34/4]=std::bit_cast<std::uint32_t>(zone);state.current[0]=19;state.current[0x40/4]=1;
    std::array<std::uint8_t,o::kMetaZoneCleanupBytes> saved; //원문42b1a0은 빈 생성자이며 실제7344BYTE 전체는 표시 정리가 끝난 뒤 복사한다.
    NativeMetaDisplayHooks display;display.destroy=[this](auto sid,auto flags) { auto& session=client_.GameplayGlobals()->RequireSession();if (sid>65535U || sid>=session.Core().pool.Capacity()) throw std::out_of_range("영역 하강 표시 정리의 실제DWORD SID 범위");static_cast<void>(session.World().Destroy(o::Sid{static_cast<std::uint16_t>(sid)},flags)); };
    LeaveMetaDisplay(*client_.ChallengeGlobals(),local.MetaZones().records,display);
    std::copy_n(local.MetaZoneRecords().begin(),saved.size(),saved.begin());processState_();
    std::copy(saved.begin(),saved.end(),local.MetaZones().records.begin()); //동기Run이 정상 복귀한 뒤에만 같은 장기 존 표에 원문 보관값을 복원한다.
}
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
void MetaConnectionRuntime::RefreshZones() { SendRootLogin(11); } //004d80fd의 현재 인자11이며 별도 상태 요청·known 관문·성공 ACK를 추가하지 않는다.
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
//HTML/카메라의 실제 동기 장치만 바깥에서 받고 나머지는 이 owner의 같은 TCP/Client에 연결한다.
o::LocalMissionParentResult MetaConnectionRuntime::RunConnected(std::uint32_t selected,const NativeMetaConnectedHooks& ui) {
    auto& local=*client_.LocalStateGlobals();auto& game=*client_.GameplayGlobals();NativeMetaConnectedHooks hooks=ui;hooks.request=request_;hooks.translate=[this](auto source) { return client_.Translate(source); };hooks.progress=progress_;
    hooks.display=[this] { client_.RefreshNativeMetaDisplay(true); };hooks.rootLogin=[this](auto connection) { SendRootLogin(connection); };hooks.listen=[this](auto port) { tcp_->Listen(port); };
    hooks.closePending=[this] { client_.ConnectionGlobals()->ClosePending(); };hooks.closeConnection=[this](auto id) { client_.ConnectionGlobals()->Disconnect(std::bit_cast<std::int32_t>(id)); };hooks.open=[this](auto id) { return tcp_->IsOpen(std::bit_cast<std::int32_t>(id)); };hooks.status=[this](auto id) { return tcp_->Status(std::bit_cast<std::int32_t>(id)); };
    hooks.connect=[this](auto id,auto address,auto port) { static_cast<void>(tcp_->ConnectPeerText(address,port,std::bit_cast<std::int32_t>(id))); };hooks.notifyMaster=[this] { NotifyMetaMaster(); };hooks.sendLogin=[this] { SendMetaGameLogin(); };
    return InstallNativeMetaConnectedState(selected,{local.State(),*client_.ChallengeGlobals(),game.player,client_.NsInfoGlobals()->zone,local.MetadataMode(),local.MetaConnected(),local.ConnectionNotified(),{local.CachedPeerIndex(),local.CachedPeerValue(),local.CachedPeerFieldA4(),local.CachedPeerFieldA8()},game.GameServerPort()},hooks);
}
}
