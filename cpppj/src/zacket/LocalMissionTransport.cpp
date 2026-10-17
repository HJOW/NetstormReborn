// 원본 단독 미션의 바이너리 패킷·현재 Player 설치·zlocal 두 끝의 실제 분배를 연결한다.
#include "zacket/LocalMissionTransport.h"
#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::zacket {
namespace {
constexpr std::size_t kPlayerBytes=180,kPlayers=9,kHeader=4,kZoneReserve=512,kZoneClient=71,kZoneHead=15; //원본 Player/패킷/Zone 레코드의 실제 폭이다.
constexpr std::uint8_t kZoneType=0x1a,kLoginType=0x27,kVersion=1; //PATCH의 실제 초기 패킷 종류와 버전이다.
constexpr std::array<std::string_view,4> kZoneNames{"zsimpson","jgreer","kdemarest","tbratton"}; //004b51e0의 실제 네 정적 이름이다.
// 실제 비정렬 little endian BYTE/WORD/DWORD를 지정 폭으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) {
    if (offset>bytes.size() || width>bytes.size()-offset) throw std::out_of_range("로컬 미션 원문 읽기 범위 오류");std::uint32_t value=0;
    // 낮은 바이트부터 같은 폭의 원본 비트를 모은다.
    for (std::size_t index=0;index<width;++index) value|=std::uint32_t(bytes[offset+index])<<(8*index);return value;
}
// 주변 stale 바이트를 유지하며 지정 원본 폭만 쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    if (offset>bytes.size() || width>bytes.size()-offset) throw std::out_of_range("로컬 미션 원문 쓰기 범위 오류");
    // 원본 little endian의 바이트 위치만 변경한다.
    for (std::size_t index=0;index<width;++index) bytes[offset+index]=static_cast<std::uint8_t>(value>>(8*index));
}
// 원본 C문자열의 첫 NUL까지만 빌리며 host 저장소 밖의 읽기는 거부한다.
std::string_view Text(std::span<const std::uint8_t> bytes,std::size_t offset) {
    if (offset>bytes.size()) throw std::out_of_range("로컬 미션 문자열 위치 오류");const auto first=bytes.begin()+static_cast<std::ptrdiff_t>(offset),last=std::find(first,bytes.end(),std::uint8_t{});
    if (last==bytes.end()) throw std::out_of_range("로컬 미션 원문 문자열 NUL 누락");return {reinterpret_cast<const char*>(bytes.data()+offset),static_cast<std::size_t>(last-first)};
}
// 원본 strcpy의 NUL까지 복사한다. 선언 필드 폭을 넘어도 실제 물리 저장소 안에서는 원본 쓰기를 유지한다.
void Copy(std::span<std::uint8_t> bytes,std::size_t offset,std::string_view text) {
    text=text.substr(0,text.find('\0'));if (offset>=bytes.size() || text.size()>=bytes.size()-offset) throw std::out_of_range("로컬 미션 strcpy 물리 저장소 부족");
    std::copy(text.begin(),text.end(),bytes.begin()+static_cast<std::ptrdiff_t>(offset));bytes[offset+text.size()]=0;
}
// 선택된 실제 외부 몸체의 누락을 빈 성공으로 바꾸지 않는다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("로컬 미션 실제 하위 소비자 미연결");return function; }
//004df590의 실제 전체0xff/종류/버전/최소4바이트 헤더다.
LocalMissionPacket Packet(std::size_t reserve,std::uint8_t type) {
    LocalMissionPacket result;result.storage.assign(reserve,std::uint8_t{0xff});result.storage[0]=kVersion;result.storage[1]=type;Put(result.storage,2,4,2);return result;
}
}
// 외부 송신은 예약 바이트 전체가 아닌 원본 WORD 길이만 소비한다.
std::span<const std::uint8_t> LocalMissionPacket::Bytes() const {
    const auto length=Read(storage,2,2);if (length>storage.size()) throw std::out_of_range("로컬 미션 패킷 선언 길이 초과");return std::span<const std::uint8_t>(storage).first(length);
}
// numClients1과 별개로 원본이 실제 쓰는 네 슬롯을 모두 작성한 뒤 observer 길이를 적용한다.
LocalMissionPacket MakeLocalMissionZone() {
    auto packet=Packet(kZoneReserve,kZoneType);auto& bytes=packet.storage;Put(bytes,4,0x82000004);Put(bytes,8,0,2);Put(bytes,10,0);bytes[14]=1;
    //004b51e0의 네 번 쓰기를 보존한다. State에 복제되는 길이는 마지막에 첫 슬롯 하나로 확정된다.
    for (std::size_t index=0;index<kZoneNames.size();++index) {
        const auto offset=kZoneHead+index*kZoneClient;bytes[offset]=static_cast<std::uint8_t>(index);Put(bytes,offset+1,static_cast<std::uint32_t>(index+1));Put(bytes,offset+5,0);Copy(bytes,offset+9,kZoneNames[index]);bytes[offset+41]=0;
    }
    Put(bytes,2,static_cast<std::uint32_t>(kZoneHead+std::size_t(bytes[14])*kZoneClient),2);return packet;
}
// 실제 파일 이미지 이후의 작성만 복원한다. Template 읽기/파일 선택은 상위 원본 부모의 책임이다.
LocalMissionPacket MakeLocalMissionLogin(std::uint32_t subscriber,std::string_view handle,std::span<const std::uint8_t> fortImage) {
    handle=handle.substr(0,handle.find('\0'));if (fortImage.size()>std::numeric_limits<std::uint32_t>::max()-67U) throw std::out_of_range("로컬 로그인 이미지 크기 초과");
    auto packet=Packet(fortImage.size()+67U,kLoginType);auto& bytes=packet.storage;Put(bytes,8,subscriber);bytes[12]=static_cast<std::uint8_t>(handle.size()+1);Copy(bytes,17,handle);
    Put(bytes,13,static_cast<std::uint32_t>(fortImage.size()));const auto start=17U+std::uint32_t(bytes[12]);
    if (start>bytes.size() || fortImage.size()>bytes.size()-start) throw std::out_of_range("로컬 로그인 실제 이미지 복사 범위 초과");
    std::copy(fortImage.begin(),fortImage.end(),bytes.begin()+start);Put(bytes,2,17U+std::uint32_t(bytes[12])+Read(bytes,13,2),2);return packet;
}
// 선언 길이를 고쳐 만든 포인터를 재사용하되 손상된 원본의 범위 밖 host 접근은 명시적으로 거부한다.
LocalMissionLoginView ReadLocalMissionLogin(std::span<const std::uint8_t> packet) {
    if (packet.size()<17 || packet[1]!=kLoginType) throw std::invalid_argument("실제0x27 로그인 패킷 누락");const auto start=17U+Read(packet,12,1),size=Read(packet,13);
    if (start>packet.size() || size>packet.size()-start) throw std::out_of_range("실제0x27 이미지 물리 범위 부족");const auto name=Text(packet,17);
    if (name.size()+1>Read(packet,12,1)) throw std::out_of_range("실제0x27 nameLen의 NUL 범위 부족");return {Read(packet,8),name,packet.subspan(start,size)};
}
// 원본 설정/시각/Chat callback 사이에도 현재 Player/전역을 재조회한다.
void InstallLocalMissionZone(std::span<const std::uint8_t> zone,std::span<std::uint8_t> players,o::PlayerWorldState& player,LocalMissionZoneGlobals globals,LocalMissionZoneHooks hooks,std::uint32_t port) {
    if (players.size()!=kPlayers*kPlayerBytes || zone.size()<kZoneHead) throw std::invalid_argument("로컬 Zone 실제 Player9/헤더 부족");const auto clients=zone[14];
    if (!clients || clients>8) Need(hooks.assertion)("numClients > 0 && numClients < 9");
    if (kZoneHead+std::size_t(clients)*kZoneClient>zone.size()) throw std::out_of_range("로컬 Zone 실제 client 레코드 부족");
    globals.server=true;globals.boss=true;globals.activeStamp=globals.pendingStamp;globals.activeZoneWord=Read(zone,10);globals.zoneFlags=Read(zone,4);
    if (globals.adjustZoneFlags && !globals.missionWorld) globals.zoneFlags=(globals.zoneFlags&~0x3fU)|(globals.zoneFlagOverride&0x3fU);
    if (globals.earlyCaptureEnd!=0.0) Need(hooks.assertion)("!earlyCaptureTimer.isRunning()");
    const auto timeout=Need(hooks.loginTimeout)();const auto delay=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(timeout)+120U);globals.earlyCaptureEnd=static_cast<double>(delay)+Need(hooks.seconds)();
    // 원본 numClients의 현재 슬롯 순서만 방문한다. 작성돼 있지만 count 밖인 나머지 세 슬롯은 설치하지 않는다.
    for (std::uint32_t index=0;index<zone[14];++index) {
        const auto offset=kZoneHead+index*kZoneClient;if (offset+kZoneClient>zone.size()) throw std::out_of_range("현재 로컬 Zone client 원문 부족");
        if (!Read(zone,offset+1)) Need(hooks.assertion)("zac->subId");const auto owner=std::uint32_t(zone[offset])+1;
        if (owner<1 || owner>=kPlayers) throw std::out_of_range("로컬 Zone 실제 Player 번호 초과");const auto record=owner*kPlayerBytes;
        if (Read(zone,offset+1)==globals.mySubscriber) {
            player.templateOwner=owner;player.battleOwner=owner;player.playerKnown=1;
            if (globals.metamap) { if (globals.cachedCoreData.size()!=10) throw std::invalid_argument("실제 metamap coreData10BYTE 부족");globals.currentCoreData=globals.cachedCoreData; }
            else globals.currentCoreData=players.subspan(record+0x6c,10);
            Put(players,record+0x50,Read(players,record+0x50)|8U);
        }
        Copy(players,record+12,Text(zone,offset+9));Need(hooks.storeName)(Read(zone,offset+1),Text(players,record+12));Put(players,record+0x2c,Read(zone,offset+1));
        if (!Read(players,record+0x2c)) Need(hooks.assertion)("p->subscriberId");Copy(players,record+0x30,Text(zone,offset+41));
        if (zone[offset+5]&1U) Put(players,record+0x50,Read(players,record+0x50)|0x200U);
    }
    Need(hooks.serve)(port);
}
// 모든 도달한 분배 효과는 기존 RawPacketDispatch와 같은 콜백을 사용한다.
LocalMissionTransport::LocalMissionTransport(std::span<const std::uint8_t> players,const bool& server,o::PacketDispatchHooks hooks)
    :players_(players),serverMode_(server),hooks_(std::move(hooks)),dispatch_(o::OriginalEdition::Patch1078,hooks_) {
    if (players.size()!=kPlayers*kPlayerBytes) throw std::invalid_argument("로컬 전송 같은 실제 Player9 누락");
}
// 실제 큰 로그인 몸체를 제공해야 분배할 수 있으며 대용 flags2를 쓰지 않는다.
void LocalMissionTransport::RegisterLogin(decltype(o::PacketRegistration::receive) receive) {
    if (!receive) throw std::invalid_argument("실제0x27 로그인 처리기 누락");dispatch_.Version(kLoginType,kVersion,0);dispatch_.Register(kLoginType,std::move(receive));
}
// 실제 두 PacketConnection을 만들고 각각 다른 endpoint/수신 큐를 사용한다.
void LocalMissionTransport::Open(std::uint32_t port,bool connectImmediately) {
    if (open_ || !port || port>65535 || !dispatch_.Registration(kLoginType).receive) throw std::logic_error("로컬 서버 포트/새 수명/실제 로그인 처리기 계약 오류");
    port_=port;client_={};server_={};client_.sender=9;server_.sender=std::bit_cast<std::uint32_t>(std::int32_t{-1});
    if (connectImmediately) { client_.received=hooks_.now();client_.heartbeat=hooks_.now(); }
    server_.received=hooks_.now();server_.heartbeat=hooks_.now();
    client_.flags=connectImmediately ? 0x26U : 0U;server_.flags=0x24;open_=true;pending_=true;serverBuffered_=false;owner_=0;
}
// 서버열기는성공한접속과별개다.같은클라이언트기록의실제접속시각/flags만지정한다.
void LocalMissionTransport::Connect() {
    if (!open_ || client_.flags || !pending_ || owner_) throw std::logic_error("로컬 클라이언트 새접속/현재서버 계약 오류");
    client_.received=hooks_.now();client_.heartbeat=hooks_.now();client_.flags=0x26;
}
// 같은 실제 pending 객체의 sender/등록 위치만 바꾼다. 새 연결이나 연결 성공 flag를 합성하지 않는다.
void LocalMissionTransport::AssignConnection(std::int32_t pending,std::int32_t owner) {
    if (!serverMode_ || !open_ || !pending_ || pending!=-1 || owner<1 || owner>=static_cast<std::int32_t>(kPlayers) || owner_)
        throw std::logic_error("로컬 실제 pending-1 재배치 계약 오류");
    owner_=static_cast<std::uint32_t>(owner);server_.sender=owner_;server_.flags|=2;serverBuffered_=true;serverOutput_.clear();pending_=false;
}
// packet observer가 확정한 WORD 바이트를 전송하며 없는 연결의 자연 생략과 실제 송신을 구분한다.
bool LocalMissionTransport::Send(std::int32_t destination,std::span<const std::uint8_t> packet) {
    if (!open_) throw std::logic_error("로컬 전송 연결을 열기 전 송신");
    if (destination==9 && !(client_.flags&2U)) return false; // 서버만열린상태의클라이언트송신은아직연결이없다.
    if (destination>=1 && destination<static_cast<std::int32_t>(kPlayers) && !(players_[std::size_t(destination)*kPlayerBytes+0x50]&1U)) return false;
    const bool client=destination==9,server=(destination==-1 && pending_) || (destination>=1 && static_cast<std::uint32_t>(destination)==owner_);
    if (!client && !server) return false;const auto length=Read(packet,2,2);if (length<4 || length>packet.size()) throw std::out_of_range("로컬 전송 패킷 실제 선언 바이트 부족");
    auto& output=client ? clientOutput_ : serverOutput_;const auto bytes=packet.first(length);
    if (!client && !serverBuffered_) toClient_.emplace_back(bytes.begin(),bytes.end());
    else { if (output.size()+length>65531U) throw std::out_of_range("로컬 buffered 전송65535바이트 범위 초과");output.insert(output.end(),bytes.begin(),bytes.end()); }
    return true;
}
// 원본 초기0xff 헤더의 첫 BYTE는 그대로 두고 kind4/전체WORD만 확정한다.
void LocalMissionTransport::FlushOne(bool serverSide) {
    auto& output=serverSide ? serverOutput_ : clientOutput_;if (output.empty()) return;std::vector<std::uint8_t> frame(output.size()+4,std::uint8_t{0xff});
    frame[1]=4;Put(frame,2,static_cast<std::uint32_t>(frame.size()),2);std::copy(output.begin(),output.end(),frame.begin()+4);output.clear();
    (serverSide ? toClient_ : toServer_).push_back(std::move(frame));
}
// 실제 번호순 송신을 유지하며 pending 끝은 원본 flush 순회의 대상이 아니다.
void LocalMissionTransport::Flush() {
    if (!open_) throw std::logic_error("로컬 연결을 열기 전 flush");
    if (owner_) FlushOne(true);FlushOne(false);
}
// 같은 등록표를 사용하며 집합 경로만 원본의 now→정책→기록→observer→receive 순서로 추가한다.
bool LocalMissionTransport::DispatchFrame(o::PacketConnection& connection,std::span<const std::uint8_t> frame) {
    if (frame.size()<4 || frame[1]!=4) return dispatch_.Dispatch(connection,frame);if (connection.flags&0x40U) return false;
    if (connection.delayed) throw std::logic_error("로컬 집합의 지연 분배 미복원");const auto total=Read(frame,2,2);std::size_t cursor=4;
    if (total>frame.size()) { hooks_.terminate(connection);return false; }
    // 현재 집합의 원문 순서를 보존하고 callback 뒤 다음 등록/연결 필드를 다시 읽는다.
    while (cursor<total) {
        if (total-cursor<4) { hooks_.terminate(connection);return false; }const auto length=Read(frame,cursor+2,2);
        if (!length || length>total-cursor) { hooks_.terminate(connection);return false; }const auto packet=frame.subspan(cursor,length);cursor+=length;
        if (!dispatch_.Validate(packet)) continue;if (packet[1]==4) throw std::logic_error("로컬 중첩 집합 패킷 미복원");const auto type=packet[1];
        const auto now=hooks_.now();if (type==6) connection.heartbeat=now;else connection.received=now;
        const auto& registration=dispatch_.Registration(type);const auto incoming=packet[0]&0x7f;
        if (incoming!=registration.version) {
            const bool lower=incoming<registration.version;if (registration.mismatch&(lower ? 8U : 4U)) { hooks_.terminate(connection);return false; }
            if (registration.mismatch&(lower ? 2U : 1U)) continue;
        }
        hooks_.record(connection,packet,static_cast<std::uint16_t>(length));const auto observer=dispatch_.Registration(type).observer;if (observer) observer(0);
        const auto receive=dispatch_.Registration(type).receive;if (receive) {
            ++connection.depth;try { receive(connection.sender,connection,packet); } catch (...) { --connection.depth;throw; }--connection.depth;
        }
    }
    if (cursor!=total) { hooks_.terminate(connection);return false; }
    if (!connection.depth && (connection.flags&0x40U)) { hooks_.close(connection);hooks_.release(connection);return false; }return true;
}
// 수신 프레임은 callback 동안 고정된 로컬 소유 벡터에 두고 같은 실제 연결을 넘긴다.
bool LocalMissionTransport::ReceiveOne(bool serverSide) {
    auto& queue=serverSide ? toServer_ : toClient_;if (queue.empty()) return false;auto bytes=std::move(queue.front());queue.pop_front();
    return DispatchFrame(serverSide ? server_ : client_,bytes);
}
// pending이 같은 호출 중 owner로 이동하면 그 뒤 실제 등록 연결 순회에서 계속 읽는다.
std::size_t LocalMissionTransport::Poll() {
    if (!open_) throw std::logic_error("로컬 연결을 열기 전 수신");std::size_t count=0;
    if (pending_ && !toServer_.empty()) { ReceiveOne(true);++count; }
    // pending에서 이동한 현재 owner 끝의 나머지 프레임을 같은 번호순 방문에서 처리한다.
    if (owner_) { while (!toServer_.empty() && !(server_.flags&0x40U)) { ReceiveOne(true);++count; } }
    // 마지막 서버ID9 끝은 앞선 로그인 callback이 보낸 실제 응답도 이 방문에서 읽는다.
    while (!toClient_.empty() && !(client_.flags&0x40U)) { ReceiveOne(false);++count; }return count;
}
// 후속 실제 로컬 packet들은 이 한 분배기의 현재 등록을 공유한다.
o::RawPacketDispatch& LocalMissionTransport::Dispatch() { return dispatch_; }
// 원본 서버ID9 방향에서 들어오는 현재 연결 객체다.
o::PacketConnection& LocalMissionTransport::Client() { return client_; }
// 원본 pending-1/재배치owner의 같은 현재 연결 객체다.
o::PacketConnection& LocalMissionTransport::Server() { return server_; }
// 실제 broadcast가 호출할 때도 현재 Player flags1과 연결 위치를 Send에서 다시 확인한다.
o::BattlePacketSend LocalMissionTransport::Peer(std::uint32_t owner) { return [this,owner](std::span<const std::uint8_t> packet) { Send(static_cast<std::int32_t>(owner),packet); }; }
// 실제 두 끝의 열린 수명만 보고하며 flags2나 Ready를 대신하지 않는다.
bool LocalMissionTransport::IsOpen() const { return open_; }
// 현재 pending이 실제 owner로 옮겨졌는지 반환한다.
std::uint32_t LocalMissionTransport::Owner() const { return owner_; }
// 상대가 실제로 받을 수 있게 flush된 프레임 수다.
std::size_t LocalMissionTransport::QueuedFrames() const { return toServer_.size()+toClient_.size(); }
}
