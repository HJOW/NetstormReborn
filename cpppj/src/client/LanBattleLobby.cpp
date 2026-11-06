// 원문 Zone1a/71BYTE 명단을 재사용하되 공식 매칭/인터넷 접속은 로컬 대기실 합의로 교체한다.
#include "client/LanBattleLobby.h"
#include "client/LocalTcpTransport.h"
#include <algorithm>
#include <bit>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // Native 게임 전송과 다른 대기실 전용 등록표/포트만 소유한다.
namespace { // 제어 포장의 원문 폭과 현재 바이트 범위를 확인하는 지역 함수다.
constexpr std::uint8_t kLobbyType=0x87; // 별도 대기실 포트에서만 NRL1 서명을 요구한다. Native 게임 Zsend 등록은 변경하지 않는다.
constexpr std::size_t kEnvelope=11,kZoneHead=15,kZoneClient=71; // 실제 원문4BYTE 프레임 뒤 서명4/명령1/게임포트2와 Zone 폭이다.
enum class LobbyCommand : std::uint8_t { Join=1,Roster,Start,Acknowledge,Reject }; //11.01 로컬 대기실 제어 확장이며 원문 게임 명령으로 위장하지 않는다.
// 지정 폭의 비정렬 little endian 값만 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) {
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("LAN 대기실 입력 폭 초과");std::uint32_t value{};
    for (std::size_t i=0;i<width;++i) value|=std::uint32_t(bytes[at+i])<<(8*i);return value; // 실제 낮은 BYTE부터 원문 폭만 조합한다.
}
// 같은 저장소에 지정 BYTE/WORD/DWORD만 기록한다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) {
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("LAN 대기실 출력 폭 초과");
    for (std::size_t i=0;i<width;++i) bytes[at+i]=static_cast<std::uint8_t>(value>>(8*i)); // 인접 필드는 보존한다.
}
// 고정 필드 안의 첫 NUL까지만 읽고 잘못된 범위/종료는 거부한다.
std::string Text(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width) {
    if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("LAN 대기실 문자열 범위 초과");const auto field=bytes.subspan(at,width);const auto end=std::find(field.begin(),field.end(),std::uint8_t{});
    if (end==field.end()) throw std::invalid_argument("LAN 대기실 문자열 NUL 누락");return {reinterpret_cast<const char*>(field.data()),static_cast<std::size_t>(end-field.begin())};
}
// 원문 이름/주소 필드보다 긴 입력을 잘라 다른 사용자로 변경하지 않는다.
void Name(std::string_view name) { if (name.empty() || name.size()>31 || std::any_of(name.begin(),name.end(),[](unsigned char byte) { return byte<32 || byte==127; })) throw std::invalid_argument("LAN 이름은 제어 문자 없는 표시 부호화 기준1~31바이트여야 합니다."); }
// 두 포트는 서로 다른 제어/게임 listener이며1..65535 범위를 요구한다.
void Ports(std::uint32_t lobby,std::uint32_t game) { if (!lobby || lobby>65535 || !game || game>65535 || lobby==game) throw std::invalid_argument("LAN 대기실과 게임 포트는 서로 다른1~65535 값이어야 합니다."); }
// 고정 필드가 보관할 수 있는 실제 문자열과 NUL만 복사한다.
void Copy(std::span<std::uint8_t> bytes,std::size_t at,std::size_t width,std::string_view text) { if (text.size()>=width || at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("LAN 대기실 문자열 출력 범위");std::copy(text.begin(),text.end(),bytes.begin()+static_cast<std::ptrdiff_t>(at));bytes[at+text.size()]=0; }
// 원문 프레임과 분리된 NRL1 제어 포장을 만든다. DNS/서버 유효성 검사를 호출하지 않는다.
std::vector<std::uint8_t> Envelope(LobbyCommand command,std::uint32_t port,std::span<const std::uint8_t> payload={}) {
    std::vector<std::uint8_t> bytes(kEnvelope+payload.size());bytes[0]=1;bytes[1]=kLobbyType;Put(bytes,2,static_cast<std::uint32_t>(bytes.size()),2);bytes[4]='N';bytes[5]='R';bytes[6]='L';bytes[7]='1';bytes[8]=static_cast<std::uint8_t>(command);Put(bytes,9,port,2);std::copy(payload.begin(),payload.end(),bytes.begin()+kEnvelope);return bytes;
}
// 실제 peer IPv4와 참가자가 선택한 게임 포트를 원문 주소 문자열로 작성한다.
std::string Peer(std::span<const std::uint8_t> address,std::uint32_t port) { return std::to_string(address[4])+'.'+std::to_string(address[5])+'.'+std::to_string(address[6])+'.'+std::to_string(address[7])+':'+std::to_string(port); }
}
// 현재 대기실의 단일 명단/실제 연결과 비동기 제어 상태다.
struct LanBattleLobby::Impl {
    std::span<const std::uint8_t> players; // 게임 자료는 읽기만 하며 대기실은 Player flags/이름을 쓰지 않는다.
    std::shared_ptr<void> lifetime; // 실제 Player 주소를 소유한 Client 수명을 함께 보관한다.
    std::function<double()> now; // 매 폴링의 실제 벽시각이며 게임 정지와 독립이다.
    std::shared_ptr<o::RawConnectionRegistry> registry{std::make_shared<o::RawConnectionRegistry>()}; // 대기실 전용 표이며 Native 게임 표와 혼용하지 않는다.
    std::unique_ptr<LocalTcpTransport> tcp; // 실제 대기실 소켓/원문 프레임 처리기다.
    bool host{},open{},starting{},joinSent{},ackSent{},ready{},taken{}; // 제어 단계이며 게임 Ready/loaded를 대신하지 않는다.
    std::uint32_t subscriber{},gamePort{},zoneId{}; // 현재 Fort ID/게임 포트/호스트가 선택한 Zone 식별자다.
    std::array<std::uint8_t,20> serverAddress{}; // 실제 client 접속 완료 때 캡처하며 host가 ACK 뒤 제어 연결을 닫아도 주소를 보존한다.
    std::string name,error; // 현재 실제 사용자 이름과 실패 문장이다.
    std::vector<LanLobbyParticipant> participants; // 슬롯/ID가 모두 고유한 현재 합의 명단이다.
    std::map<std::uint8_t,std::uint32_t> tokens; // 원격 슬롯→현재 실제 TCP 노드 token이다. 포인터를 저장하지 않는다.
    std::set<std::uint8_t> acknowledged; // 실제 시작 ACK를 받은 원격 슬롯만 보관한다.
    std::set<std::uint32_t> rejecting; // 거부 사유의 실제 partial 송신을 끝낸 뒤 닫을 pending token만 보관한다.
    std::map<std::uint32_t,double> pendingSince; // 미인증 pending의 실제 최초 관측 벽시각이다.
    std::vector<std::uint8_t> frozenZone; // Start가 고정한 원문 Zone 이미지이며 후속 가입/이탈로 바꾸지 않는다.
    double deadline{}; // 접속/시작 합의의 실제 마감이며 정상 대기는 제한하지 않는다.
    // 장치 연결은 생성만 수행하고 Host/Join이 열기 전까지 소켓을 만들지 않는다.
    Impl(std::span<const std::uint8_t> input,std::shared_ptr<void> owner,std::function<double()> wall):players(input),lifetime(std::move(owner)),now(std::move(wall)) { if (players.size()!=9*180 || !lifetime || !now) throw std::invalid_argument("LAN 대기실의 실제 Player/장기 owner/벽시계 누락"); }
    // 수명 callback이 참조하는 명단/시각을 반납하기 전에 TCP를 먼저 없앤다.
    ~Impl() { tcp.reset(); } // TCP 최종 소멸자는 callback 예외 없이 자기 노드/OS 핸들을 직접 반납한다.
    // 같은 실제 전송의 진단/등록을 대기실 전용 표에 연결한다.
    void Prepare() {
        if (tcp || open) throw std::logic_error("LAN 대기실 새 수명 필요");o::PacketDispatchHooks hooks;hooks.now=[this] { return static_cast<std::uint32_t>(now()); };
        hooks.record=[](auto&,auto,auto) {}; // 선택적 원문 기록 장치이며 제어 수신/게임 성공값을 만들지 않는다.
        hooks.terminate=[this](auto& connection) { tcp->Close(std::bit_cast<std::int32_t>(connection.sender)); };hooks.close=[](auto&) {};hooks.release=[](auto&) {}; // 실제 OS/노드 반납은 TCP owner의 공통 장치가 수행한다.
        tcp=std::make_unique<LocalTcpTransport>(players,host,std::move(hooks),registry);auto& dispatch=tcp->Dispatch();dispatch.Observe(kLobbyType,{});dispatch.Version(kLobbyType,1,0);dispatch.RequireLength(kLobbyType,0xffff);
        dispatch.Register(kLobbyType,[this](auto,auto& connection,auto bytes) {
            try { Receive(connection,bytes); }
            catch (const std::invalid_argument&) { if (!host) error="LAN 대기실 서버의 명단/제어 형식이 잘못됐습니다.";tcp->Close(std::bit_cast<std::int32_t>(connection.sender)); }
            catch (const std::out_of_range&) { if (!host) error="LAN 대기실 서버의 명단/제어 범위가 잘못됐습니다.";tcp->Close(std::bit_cast<std::int32_t>(connection.sender)); }
        });open=true; // 잘못된 외부 제어 입력만 해당 연결에서 거부하며 메모리/장치 실패를 성공으로 숨기지 않는다.
    }
    // 이미 선택한 실제 노드의 가상 송신으로 대기실 제어만 전달한다. 게임 Player gate는 적용 대상이 아니다.
    void Send(std::uint32_t token,LobbyCommand command,std::span<const std::uint8_t> payload={}) { auto packet=Envelope(command,gamePort,payload);registry->SendDirect(token,packet); }
    // 현재 명단을 원문 Zone1a의 실제 가변 길이로 작성한다.8명은512BYTE보다 길므로 고정 예약으로 자르지 않는다.
    std::vector<std::uint8_t> Zone() const {
        if (participants.empty() || participants.size()>8) throw std::logic_error("LAN Zone 실제 참가자1~8명 필요");std::vector<std::uint8_t> bytes(kZoneHead+participants.size()*kZoneClient,0xff);bytes[0]=1;bytes[1]=0x1a;Put(bytes,2,static_cast<std::uint32_t>(bytes.size()),2);Put(bytes,4,0xb8000112U);Put(bytes,8,0,2);Put(bytes,10,zoneId);bytes[14]=static_cast<std::uint8_t>(participants.size()); //004b4ed0의 일반 예제 Zone 정책 비트를 로컬 기본값으로 사용한다.
        for (std::size_t i=0;i<participants.size();++i) { // 실제 가입 순서/고유 슬롯과 이름/주소만 원문71BYTE에 쓴다.
            const auto& participant=participants[i];const auto at=kZoneHead+i*kZoneClient;bytes[at]=participant.slot;Put(bytes,at+1,participant.subscriber);Put(bytes,at+5,0);Copy(bytes,at+9,32,participant.name);Copy(bytes,at+41,30,participant.address);
        }return bytes;
    }
    // 원격 원문 명단의 물리/개수/고유성/자기 등록을 확인한다. Native Player 설치는 수행하지 않는다.
    void ReadZone(std::span<const std::uint8_t> bytes) {
        if (bytes.size()<kZoneHead || bytes[0]!=1 || bytes[1]!=0x1a || Read(bytes,2,2)!=bytes.size() || !bytes[14] || bytes[14]>8 || bytes.size()!=kZoneHead+std::size_t(bytes[14])*kZoneClient) throw std::invalid_argument("LAN 대기실 Zone 형식 오류");
        std::vector<LanLobbyParticipant> result;std::set<std::uint32_t> ids;std::set<std::uint8_t> slots;bool own{};
        for (std::size_t i=0;i<bytes[14];++i) { // 현재 원문 슬롯 번호와 subscriber의 중복을 거부한다.
            const auto at=kZoneHead+i*kZoneClient;LanLobbyParticipant participant{bytes[at],Read(bytes,at+1),Text(bytes,at+9,32),Text(bytes,at+41,30)};Name(participant.name);
            if (participant.slot>=8 || !participant.subscriber || !ids.insert(participant.subscriber).second || !slots.insert(participant.slot).second) throw std::invalid_argument("LAN 대기실 참가자 중복/번호 오류");own=own || participant.subscriber==subscriber;result.push_back(std::move(participant));
        }if (!own) throw std::invalid_argument("LAN 대기실 명단에 자기 subscriber 없음");participants=std::move(result);zoneId=Read(bytes,10);
    }
    // 가입/이탈 뒤 현재 명단을 실제 연결된 모든 참가자에게 보낸다.
    void Publish() { const auto zone=Zone();for (const auto& [slot,token]:tokens) { static_cast<void>(slot);Send(token,LobbyCommand::Roster,zone); } } // 현재 원격 슬롯만 순회하며 연결/Ready를 합성하지 않는다.
    // 실제 제어 프레임은 역할과 단계별로만 소비한다. 명단 합의 실패를 게임 Login 성공으로 바꾸지 않는다.
    void Receive(o::PacketConnection& connection,std::span<const std::uint8_t> bytes) {
        const auto token=registry->TokenFor(connection);const auto id=std::bit_cast<std::int32_t>(connection.sender);
        if (rejecting.contains(token)) return; // 이미 거부된 pending의 추가 입력으로 명단을 바꾸지 않고 사유 송신 완료만 기다린다.
        if (bytes.size()<kEnvelope || bytes[4]!='N' || bytes[5]!='R' || bytes[6]!='L' || bytes[7]!='1' || Read(bytes,9,2)!=gamePort) { tcp->Close(id);return; }const auto command=static_cast<LobbyCommand>(bytes[8]);const auto payload=bytes.subspan(kEnvelope);
        if (host && command==LobbyCommand::Join) {
            if (starting || id>=0 || payload.size()!=36) { tcp->Close(id);return; }const auto incoming=Read(payload,0);const auto incomingName=Text(payload,4,32);Name(incomingName);
            const auto duplicate=std::any_of(participants.begin(),participants.end(),[incoming](const auto& participant) { return participant.subscriber==incoming; });
            if (!incoming || duplicate || participants.size()>=8) { const std::string reason=duplicate ? "동일한 Fort ID가 이미 참가했습니다." : "대기실이 가득 찼거나 Fort ID가 잘못됐습니다.";Send(token,LobbyCommand::Reject,{reinterpret_cast<const std::uint8_t*>(reason.c_str()),reason.size()+1});rejecting.insert(token);return; }
            std::uint8_t slot=1;while (std::any_of(participants.begin(),participants.end(),[slot](const auto& participant) { return participant.slot==slot; })) ++slot; // 원문0..7 중 실제 빈 슬롯만 선택한다.
            tcp->AssignConnection(id,slot+1);participants.push_back({slot,incoming,incomingName,Peer(connection.address,gamePort)});tokens.emplace(slot,token);pendingSince.erase(token);Publish();return;
        }
        if (host && command==LobbyCommand::Acknowledge && starting && payload.size()==4 && Read(payload,0)==zoneId) {
            for (const auto& [slot,current]:tokens) if (current==token) { acknowledged.insert(slot);return; }return; // 실제 해당 연결의 고정 슬롯만 ACK한다.
        }
        if (!host && id==9) {
            if (command==LobbyCommand::Reject) { error=Text(payload,0,payload.size());return; }
            if (command==LobbyCommand::Roster && !starting) { ReadZone(payload);deadline=0;return; }
            if (command==LobbyCommand::Start && !starting) { ReadZone(payload);starting=true;frozenZone.assign(payload.begin(),payload.end());std::array<std::uint8_t,4> ack{};Put(ack,0,zoneId);Send(token,LobbyCommand::Acknowledge,ack);ackSent=true;deadline=now()+20.0;return; }
        }
        tcp->Close(id); // 다른 역할/시점의 제어 프레임은 게임 분배기로 넘기지 않는다.
    }
    // 실제 끊긴 노드는 대기 중에만 명단에서 빼며 시작 중 끊김은 실패로 보존한다.
    void RetireDisconnected() {
        bool changed{};for (auto it=tokens.begin();it!=tokens.end();) { // 매 단계에서 현재 같은 token의 OS/closed 상태를 재조회한다.
            const auto id=static_cast<std::int32_t>(it->first)+1;if (!registry->Owns(it->second) || !tcp->IsOpen(id)) { if (starting) { if (!acknowledged.contains(it->first)) error="시작 확인 전에 참가자 연결이 끊겼습니다."; ++it;continue; }const auto slot=it->first;std::erase_if(participants,[slot](const auto& participant) { return participant.slot==slot; });it=tokens.erase(it);changed=true; }else ++it;
        }if (changed) Publish();
    }
};
// 같은 장기 물리를 빌리고 구현 객체 주소는 TCP callback 동안 이동하지 않는다.
LanBattleLobby::LanBattleLobby(std::span<const std::uint8_t> players,std::shared_ptr<void> lifetime,std::function<double()> wallSeconds):impl_(std::make_unique<Impl>(players,std::move(lifetime),std::move(wallSeconds))) {}
LanBattleLobby::~LanBattleLobby()=default; // Impl이 현재 실제 TCP를 먼저 반납한다.
// 실제 host 자신만 등록한 뒤 별도 제어 listener를 연다.
void LanBattleLobby::Host(std::uint32_t subscriber,std::string_view name,std::uint32_t lobbyPort,std::uint32_t gamePort,std::uint32_t zoneId) {
    Name(name);Ports(lobbyPort,gamePort);if (!subscriber) throw std::invalid_argument("LAN host Fort ID0 금지");auto& s=*impl_;if (s.tcp || s.open) throw std::logic_error("LAN host 새 대기실 수명 필요");s.host=true;s.subscriber=subscriber;s.name=name;s.gamePort=gamePort;s.zoneId=zoneId;s.Prepare();s.participants.push_back({0,subscriber,s.name,{}});s.tcp->Listen(lobbyPort);
}
// 실제 비동기 TCP 연결이 완료되기 전에는 Join 패킷도 보내지 않는다.
void LanBattleLobby::Join(std::uint32_t subscriber,std::string_view name,std::string_view address,std::uint32_t lobbyPort,std::uint32_t gamePort) {
    Name(name);Ports(lobbyPort,gamePort);if (!subscriber) throw std::invalid_argument("LAN client Fort ID0 금지");auto& s=*impl_;if (s.tcp || s.open) throw std::logic_error("LAN client 새 대기실 수명 필요");s.subscriber=subscriber;s.name=name;s.gamePort=gamePort;s.Prepare();s.deadline=s.now()+30.0;s.tcp->Connect(address,lobbyPort,9);
}
// 원문 Zone 이미지 자체를 고정하고 실제 모든 client의 ACK를 요구한다.
void LanBattleLobby::Start() {
    auto& s=*impl_;if (!s.open || !s.host || s.starting || !s.error.empty()) throw std::logic_error("LAN 시작의 정상 host 대기실 필요");s.frozenZone=s.Zone();s.starting=true;s.deadline=s.now()+20.0;
    for (const auto& [slot,token]:s.tokens) { static_cast<void>(slot);s.Send(token,LobbyCommand::Start,s.frozenZone); } // 현재 실제 가입한 원격 연결에만 고정 명단을 전달한다.
}
// TCP callback 바깥에서만 완료/실패를 관측하므로 같은 수신 중 게임 World를 반납하지 않는다.
void LanBattleLobby::Poll() {
    auto& s=*impl_;if (!s.open || !s.error.empty() || s.taken) return;s.tcp->Flush();static_cast<void>(s.tcp->Poll());
    if (s.host) { s.RetireDisconnected();auto pending=s.registry->Cleanup().pending;const auto now=s.now();for (std::size_t index=1;index<pending.size();++index) { // 등록만 하고 Join하지 않는 실제 pending은30초 뒤 반납한다.
        const auto token=pending[index];if (!token) continue;const auto [entry,inserted]=s.pendingSince.emplace(token,now);static_cast<void>(inserted);if (entry->second+30.0<=now) s.tcp->Close(-static_cast<std::int32_t>(index));
    }std::erase_if(s.pendingSince,[&](const auto& entry) { return !s.registry->Owns(entry.first); }); }
    else {
        if (s.tcp->Status(9)<0 || !s.tcp->IsOpen(9)) { if (!s.ackSent) s.error="LAN 대기실 서버 연결이 끊겼습니다."; }
        else if (!s.joinSent && s.tcp->Status(9)==1) { s.serverAddress=s.tcp->Address(9);std::array<std::uint8_t,36> payload{};Put(payload,0,s.subscriber);Copy(payload,4,32,s.name);s.Send(s.registry->TokenFor(*s.tcp->Connection(9)),LobbyCommand::Join,payload);s.joinSent=true; }
    }
    s.tcp->Flush();if (s.tcp->QueuedBytes()==0) {
        for (const auto token:s.rejecting) if (s.registry->Owns(token)) s.tcp->Close(std::bit_cast<std::int32_t>(s.registry->Node(token).sender)); // 사유가 실제 OS 송신 큐로 넘어간 뒤에만 해당 거부 연결을 닫는다.
        s.rejecting.clear();if (s.starting && s.error.empty() && (s.host ? s.acknowledged.size()==s.tokens.size() : s.ackSent)) s.ready=true;
    }
    if (!s.ready && s.deadline!=0.0 && s.deadline<=s.now()) s.error="LAN 대기실 연결 또는 시작 확인 시간이 초과됐습니다.";
}
// 자기 대기실의 명시 종료이며 Native 게임 상태는 변경하지 않는다.
void LanBattleLobby::Close() { auto& s=*impl_;if (s.tcp) s.tcp->CloseAll();s.open=false; }
bool LanBattleLobby::IsHost() const { return impl_->host; } // 실제 제어 역할만 읽는다.
bool LanBattleLobby::Starting() const { return impl_->starting; } // 시작 합의 단계만 읽는다.
std::span<const LanLobbyParticipant> LanBattleLobby::Participants() const { return impl_->participants; } // 명단을 재작성하지 않는 읽기뷰다.
std::string_view LanBattleLobby::Error() const { return impl_->error; } // 실패를 성공으로 숨기지 않는다.
// 실제 ACK와 송신 drain이 완료된 같은 요청을 한 번만 공개한다.
std::optional<LanLobbyBattle> LanBattleLobby::TakeBattle() {
    auto& s=*impl_;if (!s.ready || s.taken || !s.error.empty()) return std::nullopt;LanLobbyBattle result;result.host=s.host;result.gamePort=s.gamePort;if (s.host) result.zone=s.frozenZone;else result.serverAddress=s.serverAddress;s.taken=true;return result;
}
}
