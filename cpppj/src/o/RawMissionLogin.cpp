// 원본의 현재 조회·중복 Fort 전송·Ready 전역 쓰기를 같은 실제 저장소에서 보존한다.
#include "o/RawMissionLogin.h"
#include <algorithm>
#include <array>
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::size_t kStride=0xb4,kPlayers=9,kFlags=0x50; // PATCH의 실제 Player 폭/개수/참여 필드다.
// 선택된 외부 몸체만 도달 시 필수로 검사한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("미션 로그인 실제 외부 몸체 미연결");return function; }
// 비정렬 little endian 필드의 지정 폭을 기록한다.
void Write(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    if (offset+width>bytes.size()) throw std::out_of_range("미션 로그인 쓰기 범위");
    // 실제 폭의 낮은 바이트만 쓴다.
    for (std::size_t index=0;index<width;++index) bytes[offset+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// 입력 span의 실제 폭을 현재 위치에서 읽는다.
std::uint32_t ReadBytes(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) {
    if (offset+width>bytes.size()) throw std::out_of_range("미션 로그인 읽기 범위");std::uint32_t value=0;
    // 주변 Player 필드나 헤더를 섞지 않는다.
    for (std::size_t index=0;index<width;++index) value|=std::uint32_t(bytes[offset+index])<<(index*8);return value;
}
// fixed packet은 생성자의 실제 초기값을, variable은004df590의ff 채움을 사용한다.
std::vector<std::uint8_t> Packet(std::uint8_t type,std::size_t size,std::uint8_t fill=0,std::uint16_t initialLength=0) {
    std::vector<std::uint8_t> bytes(size,fill);bytes[0]=1;bytes[1]=type;Write(bytes,2,initialLength ? initialLength:static_cast<std::uint32_t>(size),2);return bytes; //df590의가변예약은실제초기WORD4와전체예약크기를구별하고고정생성자는기존선언크기를사용한다.
}
// CRT _ftol의64비트 절삭값 하위 DWORD이며 비유한/64비트 범위 밖은 하위0이다.
std::uint32_t Ftol(float value) {
    const double wide=value;if (!(wide>-9223372036854775808.0 && wide<9223372036854775808.0)) return 0;
    return static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(wide)));
}
}
// 같은 저장소의 물리 범위를 먼저 확인한다. 실제 판본은 PATCH만 지원한다.
RawMissionLogin::RawMissionLogin(std::span<std::uint8_t> players,RawPlayerParticipation& participation,MissionLoginGlobals globals,MissionLoginHooks hooks)
    :players_(players),participation_(participation),globals_(globals),hooks_(std::move(hooks)),counts_(OriginalEdition::Patch1078,players) {
    if (players.size()!=kStride*kPlayers || globals.zoneAddress.size()!=6 || globals.battleOptions.size()!=87 || globals.cachedCoreData.size()!=10)
        throw std::invalid_argument("미션 로그인 실제 Player/Zone/옵션 크기 오류");
}
// 원본0번을 포함한 현재 Player 필드만 읽는다.
std::uint32_t RawMissionLogin::Read(std::uint32_t owner,std::size_t offset,std::size_t width) const {
    if (owner>=kPlayers || offset+width>kStride) throw std::out_of_range("미션 로그인 Player 번호/필드 오류");return ReadBytes(players_,owner*kStride+offset,width);
}
// 원본 DWORD 쓰기의 다른 비트는 호출자가 보존한다.
void RawMissionLogin::Put(std::uint32_t owner,std::size_t offset,std::uint32_t value) const {
    if (owner>=kPlayers || offset+4>kStride) throw std::out_of_range("미션 로그인 Player 쓰기 오류");Write(players_,owner*kStride+offset,value);
}
// 현재 벽시각을 얻은 뒤의 captureEnd를 읽으므로 callback 변경도 반영된다.
std::vector<std::uint8_t> RawMissionLogin::Reply(std::int32_t owner,std::uint8_t accepted) const {
    auto bytes=Packet(0x28,18);Write(bytes,4,10,2);Write(bytes,6,0x4e,2);bytes[8]=static_cast<std::uint8_t>(owner);bytes[9]=accepted;
    const auto now=Need(hooks_.wallSeconds)();const auto bits=std::bit_cast<std::uint64_t>(globals_.captureEnd-now);Write(bytes,10,static_cast<std::uint32_t>(bits));Write(bytes,14,static_cast<std::uint32_t>(bits>>32));return bytes;
}
// 같은 응답 writer를 CheckLogin의 실제 거부 경계에도 빌려 준다.
void RawMissionLogin::SendReply(std::int32_t owner,std::uint8_t accepted) const { auto bytes=Reply(owner,accepted);Need(hooks_.send)(owner,bytes); }
//004a6990의34바이트 전부는 생성자 초기화와 실제 현재 필드로 정해진다.
void RawMissionLogin::SendBegin(std::uint32_t owner) const {
    auto bytes=Packet(0x29,34);Write(bytes,4,owner);std::copy(globals_.zoneAddress.begin(),globals_.zoneAddress.end(),bytes.begin()+8);
    Write(bytes,22,globals_.beginSeed);Write(bytes,30,globals_.BattleSeed());Need(hooks_.send)(static_cast<std::int32_t>(owner),bytes);
    if (globals_.debugBurst) throw std::logic_error("미션 로그인 디버그63000바이트3회 분기 미복원");
}
// 원본은 owner0 슬롯을ff로 남기며 현재1..8 float SP를 각각 절삭한다.
void RawMissionLogin::SendSp(std::uint32_t owner) const {
    auto bytes=Packet(0x81,44,0xff,4); //004a6e70의df590은BSS00544ca0의0을최소선언4로올리고observer가나중에44로확정한다.
    // 각 SP 조회는 실제0040ec50을 한 번 실행하여 그 시점의 현재 값을 쓴다.
    for (std::uint32_t player=1;player<9;++player) Write(bytes,8+player*4,Ftol(std::bit_cast<float>(Need(hooks_.spBits)(player))));
    Write(bytes,4,9);Need(hooks_.send)(static_cast<std::int32_t>(owner),bytes);
}
//004a6ab0→00491270 뒤의 두 순회는 합치지 않는다. 콜백 뒤 flags를 매번 다시 읽는다.
void RawMissionLogin::Joined(std::uint32_t owner,std::uint32_t subscriber) const {
    Put(owner,0x2c,subscriber);
    if (owner!=globals_.player.battleOwner) {
        const auto address=Need(hooks_.peerAddress)(owner);if (address.size()+1>players_.size()-(owner*kStride+0x30)) throw std::out_of_range("미션 로그인 peer주소 범위");
        std::copy(address.begin(),address.end(),players_.begin()+owner*kStride+0x30);players_[owner*kStride+0x30+address.size()]=0;
    }
    Put(owner,kFlags,(Read(owner,kFlags)&~0x100U)|3U);
    //00491270은 가입한 Player 하나를 현재 접속한 모든 대상에게 보낸다.
    for (std::uint32_t target=1;target<9;++target) if (Read(target,kFlags,1)&1U) Need(hooks_.sendPlayer)(owner,target);
    //004a6ab0은 현재 사람 참여 Player 각각을 새 대상에게 다시 보낸다.
    for (std::uint32_t player=1;player<9;++player) if (Read(player,kFlags,1)&2U) Need(hooks_.sendPlayer)(player,owner);
}
//Ready mask는 실제 참여/접속 여부와 무관하게각 BYTE40만 검사한다.
void RawMissionLogin::SendReady(std::uint32_t target) const {
    if (!globals_.boss) Need(hooks_.assertion)("boss");auto bytes=Packet(0x6e,8);std::uint32_t mask=0;
    // 실제 비트 번호는 owner번호와 같다.
    for (std::uint32_t owner=1;owner<9;++owner) if (Read(owner,kFlags,1)&0x40U) mask|=1U<<owner;
    Write(bytes,4,mask);if (!target) Need(hooks_.broadcast)(bytes);else Need(hooks_.send)(static_cast<std::int32_t>(target),bytes);
}
//004a7800의 일반/미션 부모를 같은 현재 Player와 실제 선택 효과로 실행한다.
void RawMissionLogin::Receive(std::int32_t pending,PacketConnection& connection,std::span<const std::uint8_t> packet) const {
    if (!globals_.server) { Need(hooks_.disconnect)(pending);return; }
    const auto subscriber=ReadBytes(packet,8);std::uint32_t owner=1;
    // subscriber가 같은 현재 첫 실제 Player만 선택한다.
    for (;owner<9;++owner) if (Read(owner,0x2c)==subscriber) break;if (owner==9) owner=0;
    if (!globals_.player.playerKnown) {
        // 최초 조건은 한 번만 읽고 루프 안에서는 현재 subscriber 전역을 다시 읽는다.
        for (std::uint32_t player=1;player<9;++player) if (Read(player,0x2c)==globals_.mySubscriber) {
            globals_.player.templateOwner=player;globals_.player.battleOwner=player;globals_.player.playerKnown=1;
            globals_.currentCoreData=globals_.metamap ? globals_.cachedCoreData : players_.subspan(player*kStride+0x6c,10);
        }
    }
    if (!owner) { const auto bytes=Reply(0,0);Need(hooks_.pendingReply)(connection,bytes);return; }
    if (globals_.simulconnect) {
        if (pending<0) Need(hooks_.disconnect)(static_cast<std::int32_t>(owner)); // 실제 새 pending을 이전 owner 연결과 구별하여 먼저 이전 끝만 닫는다.
        else if (pending<1 || pending>8) Need(hooks_.assertion)("FIRST_REAL_PLAYER <= pendingId && pendingId < END_REAL_PLAYERS"); // 원문 진단이 복귀하면 CheckLogin 순서를 계속한다.
    }
    const auto previous=(Read(owner,kFlags)>>1)&1U; //CheckLogin 이전의 사람 참여 bit2를 끝까지 보존한다.
    if (!previous && (hooks_.restoreCoreData || !globals_.missionPost)) Need(hooks_.restoreCoreData)(owner); // 실제 Client는 원문처럼 previous0에서 부모를 호출한다. 독립 미션Post1의 장치 누락만 하위 첫 자연 반환과 같다.
    participation_.CheckLogin(pending,static_cast<std::int32_t>(owner));Need(hooks_.flush)();SendBegin(owner);Need(hooks_.flush)();
    auto options=Packet(0x67,94,0xff,4);options[4]=0;Write(options,5,87,2);std::copy(globals_.battleOptions.begin(),globals_.battleOptions.end(),options.begin()+7); //004a7998의df590초기선언4를유지하고전체94BYTE예약을같은송신gate에전달한다.
    Need(hooks_.send)(static_cast<std::int32_t>(owner),options);if (counts_.Participating()<2) Need(hooks_.applyOptions)(options);Need(hooks_.flush)();
    if (!globals_.fortCount) Need(hooks_.clearTypes)();Need(hooks_.flush)();
    //004a6a70은 subscriber가 있는 모든 현재 Player를 순서대로 보낸다.
    for (std::uint32_t player=1;player<9;++player) if (Read(player,0x2c)) Need(hooks_.sendPlayer)(player,owner);
    Need(hooks_.flush)();Need(hooks_.sendFortMetadata)(owner);Need(hooks_.flush)();
    if (!previous) {
        const auto size=ReadBytes(packet,13),start=17U+ReadBytes(packet,12,1); //실제0040a4b0 위치이며 name을 별도모델로 해석하지 않는다.
        if (start>packet.size() || size>packet.size()-start) throw std::out_of_range("미션 로그인 Fort image 물리 범위");
        Put(owner,0x5c,globals_.fortCount);Put(owner,0x60,globals_.predictableCount);++globals_.fortCount;globals_.lastLoginPlayer=owner;
        if (globals_.server && owner==globals_.player.battleOwner) Need(hooks_.loadCurrentFort)(owner);
        else Need(hooks_.loadFortImage)(owner,packet.subspan(start,size));
    }
    Joined(owner,ReadBytes(packet,8));SendReply(static_cast<std::int32_t>(owner),1);
    if (!globals_.missionPost && !*Need(globals_.savedGame) && *Need(globals_.inBattle)) {
        Put(owner,0x58,globals_.battleOptions[0x3e+owner]==1 ? 1U:0U); // 원문 조건의 쉼표식처럼 watcher 쓰기는 +64 검사보다 먼저 실행한다.
        if (!Read(owner,0x64)) {
            for (std::uint32_t player=1;player<9;++player) { // 각 지정 효과 뒤 watcher/현재 시작 SP 옵션을 다시 읽는다.
                std::int32_t amount{}; // watcher는 실제 SP0이며 다른 Player만 시작 SP+4000을 받는다.
                if (globals_.battleOptions[0x3e+player]!=1) {
                    constexpr std::array<std::int32_t,3> startingSp{1000,2500,3000}; //0041c7d0의 실제3항목이며 옵션+0e가 현재 색인을 고른다.
                    const auto index=globals_.battleOptions[0x0e];if (index>=startingSp.size()) throw std::out_of_range("일반 로그인 시작 SP 옵션 색인");amount=startingSp[index]+4000;
                }
                Need(hooks_.setSp)(player,amount); // 실제00447810의 분산 SP/HUD 효과를 호출하며 별도 성공값을 만들지 않는다.
            }
        }
    }
    Need(hooks_.flush)();
    if (globals_.metamap) Need(hooks_.assertion)("!metamap");SendSp(owner);Need(hooks_.flush)();
    globals_.clockStamp=Need(hooks_.clock)();const auto difference=std::bit_cast<std::int32_t>(globals_.clockStamp-globals_.baseClock)/1000;
    // 시간 packet은 실제1..8 전체를 요청한다. 실제 접속gate/연결부재 처리는send가 담당한다.
    for (std::uint32_t target=1;target<9;++target) { auto bytes=Packet(0x84,8);Write(bytes,4,std::bit_cast<std::uint32_t>(difference));Need(hooks_.send)(static_cast<std::int32_t>(target),bytes); }
    Need(hooks_.flush)();Put(owner,0x64,1);
    // 현재 fortCount는 바깥 반복마다 다시 읽으며 새 자기 Fort는 이 첫 순회에서 제외한다.
    for (std::uint32_t order=0;std::bit_cast<std::int32_t>(order)<std::bit_cast<std::int32_t>(globals_.fortCount);++order) {
        //각 효과 뒤 현재flags/순서/예측수를 다시 읽는다.
        for (std::uint32_t player=1;player<9;++player) if ((Read(player,kFlags,1)&2U) && Read(player,0x5c)==order && (previous || player!=owner)) {
            Need(hooks_.sendFort)(player,owner,Read(player,0x60));Need(hooks_.flush)();
        }
    }
    if (owner!=globals_.player.battleOwner) Need(hooks_.synchronizePeer)(owner);Need(hooks_.flush)();
    if (!previous) {
        // 두번째 순회는 자기대상도 포함하여 새 Fort를 현재 접속자에게 보낸다.
        for (std::uint32_t target=1;target<9;++target) if (Read(target,kFlags,1)&1U) {
            if (Read(globals_.lastLoginPlayer,0x60)!=globals_.predictableCount) Need(hooks_.assertion)("predictableCount");
            Need(hooks_.sendFort)(owner,target,globals_.predictableCount);
        }
        if (globals_.missionWorld) Need(hooks_.loadWorld)(); // 원문 미션 저장 월드만 현재 Load를 실행한다.
        else if (globals_.missionPost || !Read(owner,0x58)) Need(hooks_.makeBattleIsland)(owner); // 일반 watcher는 실제 세 단계의 전투섬 생성 없이 settings/Ready로 간다.
    }
    Need(hooks_.sendSettings)(owner);Need(hooks_.flush)();Need(hooks_.flush)();
    const auto current=globals_.player.battleOwner;Put(current,kFlags,Read(current,kFlags)|0x40U);SendReady(owner);Need(hooks_.flush)();
    if (counts_.Subscribers()!=1) { const auto disconnected=counts_.Disconnected();if (counts_.Subscribers()-disconnected!=1) return; }globals_.begin=1;
}
//63h 재접속은 CheckLogin 거부 뒤에도 원문 후반을 계속하며 새 Fort/월드 생성 분기를 실행하지 않는다.
void RawMissionLogin::ReceiveReconnect(std::int32_t pending,std::span<const std::uint8_t> packet) const {
    if (packet.size()!=8 || packet[1]!=0x63 || ReadBytes(packet,2,2)!=8) throw std::invalid_argument("재접속63h의 실제8BYTE 입력 오류");
    Need(hooks_.log)("Reconnecting "+std::to_string(pending)+"\n");std::uint32_t owner=1;
    // 원문은 subscriber0도 비교하며 별도 flags/서버 권한 관문을 추가하지 않는다.
    for (;owner<9;++owner) if (ReadBytes(packet,4)==Read(owner,0x2c)) break;
    if (owner==9) { if (Need(hooks_.checkingAssertions)()) Need(hooks_.assertion)("0");return; } //Serverboard.cpp1b2의 실제 진단 복귀 뒤 반환이다.
    if (participation_.CheckLogin(pending,static_cast<std::int32_t>(owner))) SendReply(static_cast<std::int32_t>(owner),1);
    if (ReadBytes(packet,4)==globals_.mySubscriber) Put(owner,kFlags,Read(owner,kFlags)|0x18U);
    Joined(owner,ReadBytes(packet,4));globals_.fortCount=0;globals_.lastLoginPlayer=0;std::int32_t marker{};
    // PE는 counter의 최대값 대신 앞서 고른 owner번호와 다음+5c를 비교한다. 이 원문 선택을 정규화하지 않는다.
    for (std::uint32_t player=1;player<9;++player) if (Read(player,kFlags,1)&2U) {
        ++globals_.fortCount;if (std::bit_cast<std::int32_t>(Read(player,0x5c))>marker) { marker=static_cast<std::int32_t>(player);globals_.lastLoginPlayer=player; }
    }
    Need(hooks_.cleanupReconnectClients)();const auto local=globals_.player.battleOwner;Put(local,kFlags,Read(local,kFlags)|0x40U);SendReady(owner);
    if (counts_.Subscribers()!=1) { const auto disconnected=counts_.Disconnected();if (counts_.Subscribers()-disconnected!=1) return; }globals_.begin=1; // 뒤4437f0은 빈 함수라 새 효과를 추가하지 않는다.
}
// 분배기의 현재 callback 수명 안에서 같은 원본 버퍼를 전달한다.
void RegisterMissionLogin(RawPacketDispatch& dispatch,RawMissionLogin& login) {
    dispatch.Observe(0x27,{});dispatch.Version(0x27,1,0);
    dispatch.Register(0x27,[&login](std::uint32_t pending,PacketConnection& connection,std::span<const std::uint8_t> packet) { login.Receive(std::bit_cast<std::int32_t>(pending),connection,packet); });
    dispatch.Observe(0x63,{});dispatch.Version(0x63,1,0);dispatch.RequireLength(0x63,8); // 원문99의 observer40b380은 빈 함수이며 실제63h/1/8 수신을 등록한다.
    dispatch.Register(0x63,[&login](std::uint32_t pending,PacketConnection&,std::span<const std::uint8_t> packet) { login.ReceiveReconnect(std::bit_cast<std::int32_t>(pending),packet); });
}
}
