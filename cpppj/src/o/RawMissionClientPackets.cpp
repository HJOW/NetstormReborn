// 실제 클라이언트 수신의 작은 계산/쓰기는 직접 실행하고 큰 외부 하위는 필수 위치에 남긴다.
#include "o/RawMissionClientPackets.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
constexpr std::size_t kStride=0xb4,kCount=9; // PATCH Player9의 실제 물리 폭/개수다.
constexpr std::string_view kForbidden="\xa0%.?=()[]~\"`;: "; // 원본 이름 정제가 사용하는 실제 금지문자표다.
// 선택되지 않은 경계는 호출하지 않고 도달한 누락만 명시 오류로 처리한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("미션 클라이언트 패킷의 실제 소비자 미연결");return function; }
// 원본 정렬되지 않은 BYTE/WORD/DWORD를 읽는다.
std::uint32_t Get(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) {
    if (offset+width>bytes.size()) throw std::out_of_range("미션 수신 읽기 물리 범위");std::uint32_t result=0;
    // 필요한 폭의 낮은 바이트부터 조립한다.
    for (std::size_t index=0;index<width;++index) result|=std::uint32_t(bytes[offset+index])<<(index*8);return result;
}
// 원본 지정 폭만 쓰며 인접 stale 바이트를 유지한다.
void Write(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    if (offset+width>bytes.size()) throw std::out_of_range("미션 수신 쓰기 물리 범위");
    // 지정 BYTE/WORD/DWORD의 낮은 바이트만 쓴다.
    for (std::size_t index=0;index<width;++index) bytes[offset+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// NUL을 포함한 실제 strcpy 입력만 선택한다.
std::span<const std::uint8_t> CString(std::span<const std::uint8_t> bytes) {
    const auto end=std::find(bytes.begin(),bytes.end(),std::uint8_t{});if (end==bytes.end()) throw std::out_of_range("미션 수신 이름 NUL 누락");return bytes.first(static_cast<std::size_t>(end-bytes.begin())+1);
}
}
// 금지 문자 삭제는 현재 NUL까지 한 칸씩 이동하고 그 뒤 stale 바이트를 지우지 않는다.
void SanitizeMissionPlayerName(std::uint32_t subscriber,std::span<std::uint8_t> name) {
    auto length=CString(name).size()-1;std::size_t index=0;
    // 삭제한 위치는 다시 검사하여 연속 금지문자도 실제 원본처럼 제거한다.
    while (index<length) {
        if (kForbidden.find(static_cast<char>(name[index]))==std::string_view::npos) { ++index;continue; }
        // NUL을 포함해 현재 문자 뒤를 왼쪽으로 한 칸 옮긴다.
        for (std::size_t next=index;next<length;++next) name[next]=name[next+1];--length;
    }
    name[length]=0;
    if (!length) {
        const auto replacement=std::string("Cheat")+std::to_string(subscriber&0xffU);if (replacement.size()+1>name.size()) throw std::out_of_range("미션 빈 이름 대체 범위");
        std::copy(replacement.begin(),replacement.end(),name.begin());name[replacement.size()]=0;
    }
}
// 같은 물리 저장소의 크기와 현재 선택용 span을 먼저 확인한다.
RawMissionClientPackets::RawMissionClientPackets(std::span<std::uint8_t> players,RawPlayerParticipation& participation,ScrambledSpStore& sp,
    MissionClientPacketState state,MissionClientPacketHooks hooks):players_(players),participation_(participation),sp_(sp),state_(state),hooks_(std::move(hooks)) {
    if (players.size()!=kStride*kCount || state.cachedCoreData.size()!=10 || state.zoneAddress.size()!=6 || state.battleOptions.size()!=87 || state.hidden.size()!=9 || state.hidden.data()!=state.battleOptions.data()+0x3e)
        throw std::invalid_argument("미션 수신의 실제 Player/현재 전역 크기 오류");
}
// 현재9기록에서 원본 폭만 읽는다.
std::uint32_t RawMissionClientPackets::Read(std::uint32_t owner,std::size_t offset,std::size_t width) const {
    if (owner>=kCount || offset+width>kStride) throw std::out_of_range("미션 수신 Player 읽기 범위");return Get(players_,owner*kStride+offset,width);
}
// 같은 현재9기록의 필요한 필드만 쓴다.
void RawMissionClientPackets::Put(std::uint32_t owner,std::size_t offset,std::uint32_t value,std::size_t width) const {
    if (owner>=kCount || offset+width>kStride) throw std::out_of_range("미션 수신 Player 쓰기 범위");Write(players_,owner*kStride+offset,value,width);
}
// 원본 문자열은 선언필드 폭을 넘더라도 실제 Player9 물리 저장소 안에서는 NUL까지 복사한다.
void RawMissionClientPackets::Copy(std::uint32_t owner,std::size_t offset,std::span<const std::uint8_t> source) const {
    const auto text=CString(source);if (owner>=kCount || owner*kStride+offset+text.size()>players_.size()) throw std::out_of_range("미션 수신 Player 문자열 범위");
    std::copy(text.begin(),text.end(),players_.begin()+owner*kStride+offset);
}
// 현재 정제된 Player 이름의 실제 NUL 이전 문자열을 채팅에 전달한다.
std::string_view RawMissionClientPackets::Name(std::uint32_t owner) const {
    if (owner>=kCount) throw std::out_of_range("미션 수신 이름 Player 범위");const auto text=CString(players_.subspan(owner*kStride+12));return {reinterpret_cast<const char*>(text.data()),text.size()-1};
}
//004b3b40은 서버에서도 현재 번호/CoreData를 지정하고 원본 여섯바이트/두seed를 쓴다.
void RawMissionClientPackets::LoginBegin(std::span<const std::uint8_t> packet) const {
    if (state_.server && Get(packet,4)!=state_.player.battleOwner) Need(hooks_.assertion)("z->yourPlayerId == myPlayerId()");
    const auto owner=Get(packet,4);if (owner>=9) throw std::out_of_range("LoginBegin Player 범위");state_.player.templateOwner=owner;state_.player.battleOwner=owner;
    if (!owner) {
        if (!state_.currentCoreData.empty()) { if (state_.currentCoreData.size()<10) throw std::out_of_range("LoginBegin 이전CoreData 범위");std::copy_n(state_.currentCoreData.begin(),10,state_.cachedCoreData.begin()); }
        state_.currentCoreData={};state_.coreDataActive=0;
    } else { state_.player.playerKnown=1;state_.currentCoreData=state_.metamap ? state_.cachedCoreData : players_.subspan(owner*kStride+0x6c,10); }
    state_.beginSeed=Get(packet,22);std::copy_n(packet.begin()+8,6,state_.zoneAddress.begin());state_.battleSeed=Get(packet,30);
}
//004b4900의 현재 옵션 복사와 모든 파생 효과를 순서대로 실행한다.
void RawMissionClientPackets::Options(std::uint32_t sender,std::span<const std::uint8_t> packet) const {
    const auto length=Get(packet,5,2);if (length>87) Need(hooks_.assertion)("z->dataLen <= sizeof(globalBattleOptions)");
    if (length>87 || packet.size()<7+length) throw std::out_of_range("미션 옵션 실제 복사 범위");std::copy_n(packet.begin()+7,length,state_.battleOptions.begin());
    if (state_.outpost) Need(hooks_.outpostOptions)(sender,packet);Need(hooks_.updateRange)();const auto energy=Need(hooks_.requiredEnergy)();Need(hooks_.setEnergyType)(static_cast<float>(energy));state_.optionValue=Need(hooks_.readOptionValue)();
    if (state_.inBattle && !state_.missionPost && state_.savedGame) Need(hooks_.reconnectUi)();
}
//004b3d30은 로컬 미션 서버에서는 재설치를 생략하고 revision만 실제로 증가한다.
void RawMissionClientPackets::Fort(std::span<const std::uint8_t> packet) const {
    const auto owner=Get(packet,4);if (owner>=9) throw std::out_of_range("미션 Fort Player 범위");
    if (owner==state_.player.battleOwner) { if (!state_.missionPost) Need(hooks_.localFort)(owner); }
    else { if (state_.server) { ++state_.revision;return; }Need(hooks_.openFortImage)(owner); }
    if (!state_.server) {
        if (state_.missionPost || !state_.hidden[Get(packet,4)]) {
            const auto size=Get(packet,12);if (packet.size()<17 || size>packet.size()-17) throw std::out_of_range("미션 Fort image 실제 범위");
            Need(hooks_.readFortImage)(owner,packet.subspan(17,size));Need(hooks_.buildFort)(Get(packet,4));
        }
    }
    ++state_.revision;
}
//004912d0의 이름/flags/채널/방향별동맹/metadata/UI 순서를 현재 읽기로 실행한다.
void RawMissionClientPackets::Player(std::uint32_t sender,std::span<const std::uint8_t> packet) const {
    const auto owner=Get(packet,0x32,1);if (owner>=9 || packet.size()<0x3b) throw std::out_of_range("미션 Player packet 범위");Copy(owner,12,packet.subspan(0x3b));
    if (state_.missionPost && Get(packet,0x32,1)==state_.player.battleOwner) Copy(owner,12,state_.currentHandle);
    SanitizeMissionPlayerName(Get(packet,0x26),players_.subspan(owner*kStride+12));Put(owner,0x2c,Get(packet,0x26));Put(owner,0x71,Get(packet,0x3a,1),1);
    if (!state_.server) {
        Copy(owner,0x30,packet.subspan(4));
        if (players_[owner*kStride+0x30]=='*') { const auto address=Need(hooks_.peerAddress)(sender);std::vector<std::uint8_t> bytes(address.begin(),address.end());bytes.push_back(0);Copy(owner,0x30,bytes); }
        Put(owner,0x50,(Read(owner,0x50)&0x40U)|Get(packet,0x2a));Put(owner,0x54,Get(packet,0x22));
    }
    if (!(Read(owner,0x50,1)&1U)) Need(hooks_.removeChat)(Read(owner,0x2c),0x20);else Need(hooks_.addChat)(Read(owner,0x2c),Name(owner),0x20);
    participation_.SetAlliance(Get(packet,0x32,1),Get(packet,0x22));
    if (!state_.server) {
        Put(owner,0x5c,Get(packet,0x34,1));Put(owner,0x60,Get(packet,0x35,2));Put(owner,0xa0,Get(packet,0x37,1));Put(owner,0xa4,Get(packet,0x38,1),1);Put(owner,0xa5,Get(packet,0x39,1),1);
    }
    if (!state_.missionPost && Read(owner,0x2c)) Need(hooks_.restoreCoreData)(Get(packet,0x32,1));
    if (!Need(hooks_.playerUiBusy)()) {
        if (!state_.missionPost) Need(hooks_.saveReconnect)();
        if (Read(owner,0x50,1)&1U) { Need(hooks_.playerNotice)(5,Get(packet,0x32,1),0);Need(hooks_.playerNotice)(3,Get(packet,0x32,1),1);return; }
        Need(hooks_.playerNotice)(3,Get(packet,0x32,1),0);Need(hooks_.playerNotice)(2,Get(packet,0x32,1),0);
        if (Read(owner,0x50,1)&2U) Need(hooks_.playerNotice)(5,Get(packet,0x32,1),1);
    }
}
//004b8940의 현재phase 재조회와 동기 State 실행을 원본 위치에 남긴다.
void RawMissionClientPackets::Reply(std::span<const std::uint8_t> packet) const {
    const auto major=Get(packet,4,2),minor=Get(packet,6,2);
    if ((major!=10 || minor<0x4e) && !Need(hooks_.configInt)("suppressVersionMessage")) {
        Need(hooks_.tell)("GameVerDiff("+std::to_string(major)+"."+std::to_string(minor)+")");state_.currentState=12;state_.currentAux=0;Need(hooks_.runState)();return;
    }
    const auto bits=std::uint64_t(Get(packet,10))|(std::uint64_t(Get(packet,14))<<32);const auto remaining=std::bit_cast<double>(bits);const auto now=Need(hooks_.wallSeconds)();state_.captureEnd=remaining+now;
    if (state_.phase==16) { state_.currentState=18U-(Get(packet,9,1)!=0);Need(hooks_.runState)();return; }
    if (state_.phase==10) { state_.currentState=(Get(packet,9,1)!=0)*2U+11U;Need(hooks_.resetCamera)();Need(hooks_.runState)();return; }
    state_.sourceTicks=0;state_.pendingState=3;
}
//004b3e90→00447810의 각 현재 DWORD와실제 같은 SP 저장소 직접지정을 실행한다.
void RawMissionClientPackets::Sp(std::span<const std::uint8_t> packet) const {
    //현재 count는 각 반복 끝에 다시 읽으며 음수SP는 전달하지 않는다.
    for (std::uint32_t owner=0;std::bit_cast<std::int32_t>(owner)<std::bit_cast<std::int32_t>(Get(packet,4));++owner) {
        const auto value=Get(packet,8+owner*4);if (std::bit_cast<std::int32_t>(value)<0) continue;
        const auto amount=Get(packet,8+owner*4);const bool local=state_.player.playerKnown && owner==state_.player.battleOwner;
        if (local || (owner>0 && owner<9)) {
            const float number=state_.editor ? 194162.0F : static_cast<float>(std::bit_cast<std::int32_t>(amount));sp_.Set(local ? state_.player.battleOwner : owner,std::bit_cast<std::uint32_t>(number));if (local) Need(hooks_.refreshSp)();
        }
    }
}
//004a7050은 boss의서버9 입력을자연반환하며 client의mask와자기40응답은 별도로 실행한다.
void RawMissionClientPackets::Ready(std::uint32_t sender,std::span<const std::uint8_t> packet) const {
    if (!state_.boss) {
        if (std::bit_cast<std::int32_t>(Get(packet,4))<0) state_.begin=1;
        //원본1..8 각조건은 현재packet을다시 읽으며해당Player40만OR한다.
        for (std::uint32_t owner=1;owner<9;++owner) if (Get(packet,4)&(1U<<owner)) Put(owner,0x50,Read(owner,0x50)|0x40U);
        const auto owner=state_.player.battleOwner;if (!(Read(owner,0x50)&0x40U)) { Put(owner,0x50,Read(owner,0x50)|0x40U);Need(hooks_.send)(9,packet.first(8)); }
    } else if (sender!=9) {
        Put(sender,0x50,Read(sender,0x50)|0x40U);Need(hooks_.sendReady)(0);
        if (state_.loginEnd==0.0) { const std::array<std::uint8_t,8> begin{1,0x6e,8,0,0,0,0,0x80};Need(hooks_.send)(static_cast<std::int32_t>(sender),begin); }
    }
}
//원본등록이선택한실제작은몸체를분배한다.다른종류를무시하는성공은없다.
void RawMissionClientPackets::Receive(std::uint32_t sender,PacketConnection&,std::span<const std::uint8_t> packet) const {
    switch (Get(packet,1,1)) {
    case 0x29:LoginBegin(packet);break;
    case 0x67:Options(sender,packet);break;
    case 0x2b:Player(sender,packet);break;
    case 0x2a:Fort(packet);break;
    case 0x28:Reply(packet);break;
    case 0x81:Sp(packet);break;
    case 0x84: {
        const auto ticks=std::bit_cast<std::int32_t>(Get(packet,4));state_.clockStamp=Need(hooks_.clock)();
        //004a6f50의double1000곱과fsubp는decomp본문에서누락됐다. 정수입력의정확한64비트차하위DWORD다.
        const auto difference=static_cast<std::int64_t>(std::bit_cast<std::int32_t>(state_.clockStamp))-static_cast<std::int64_t>(ticks)*1000;
        state_.baseClock=static_cast<std::uint32_t>(static_cast<std::uint64_t>(difference));break;
    }
    case 0x6e:Ready(sender,packet);break;
    default:throw std::invalid_argument("미등록 미션 클라이언트 packet 종류");
    }
}
//원본버전/정책과실제수신기를등록한다.송신observer는이미완성바이트를쓰는전송계약과구분한다.
void RegisterMissionClientPackets(RawPacketDispatch& dispatch,RawMissionClientPackets& packets) {
    constexpr std::array<std::uint32_t,8> types{0x29,0x67,0x2b,0x2a,0x28,0x81,0x84,0x6e}; //실제복원한8종등록번호다.
    //각타입은버전1/정책0과동일처리기를가지며원본길이표는분배기가이미소유한다.
    for (const auto type:types) {
        dispatch.Observe(type,[type](std::uint32_t sending) {
            if (sending && (type==0x67 || type==0x2b || type==0x2a || type==0x81)) throw std::logic_error("미션 수신 observer의 송신 버퍼 경계 미연결");
        });dispatch.Version(type,1,0);
        dispatch.Register(type,[&packets](std::uint32_t sender,PacketConnection& connection,std::span<const std::uint8_t> packet) { packets.Receive(sender,connection,packet); });
    }
}
}
