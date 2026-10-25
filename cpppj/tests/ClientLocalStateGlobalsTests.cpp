// Client 수명의 State/현재 전역을 실제 요청 작성·로그인·내용물 통지가 같은 주소로 소비하는지 검사한다.
#include "DudeSpawnSupport.h"
#include "client/RawCampaignSession.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/MissionFortRequest.h"
#include "o/SquidForms.h"
#include <algorithm>
#include <array>
#include <bit>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace netstorm::o;
using namespace netstorm::client;
// 기존 CoreRefreshSessionDevices처럼 외부 음향/번역/표시/시계만 공급하고 실제 게임 계산은 Session에 맡긴다.
struct LocalStateSessionDevices {
    SoundList names{OriginalEdition::Patch1078}; // 모든 Session보다 오래 사는 실제 소리 이름표다.
    std::array<std::uint8_t,4> you{'Y','o','u',0}; // GUI 없는 명시 번역 입력이다.
    double wall{100.25}; // 실제 wall getter가 매번 읽는 외부 시각 입력이다.
    std::vector<std::string> logs; // 실제 metadata/Login이 선택한 로그 경계만 관측한다.
    std::vector<std::uint32_t> pickupSounds; // 실제 Carrier 내용물 통지의 TYPE+7c 음향 호출만 관측한다.
    std::function<void(std::uint32_t,std::span<const std::uint8_t>)> observe; // 수신 본문 직전의 원래 sender/패킷을 읽는 검사 관찰기다.
    RawCampaignSessionServices Make(const std::shared_ptr<ClientLocalStateGlobals>& owner,bool login) { // 같은 장기 owner를 실제 Session/Login/내용물 소비자에 제공한다.
        names.Initialize();RawCampaignSessionServices result;result.localStateGlobals=owner;result.soundNames=&names;result.unixSeconds=[] { return 946684800; };
        result.production.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };
        result.sound.wallSeconds=[this] { return wall; };
        result.sound.play=[](SoundHandle sound,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { return sound; };
        result.sound.playLoopAt=[](float,float,SoundHandle current,SoundHandle sound) { return current ? current : sound; };
        result.sound.playOnceAt=[](float,float,SoundHandle sound,std::uint32_t,std::uint32_t) { return sound; };
        result.sound.stop=[](SoundHandle) {};result.sound.isPlaying=[](SoundHandle) { return 0U; };
        result.install.translateYou=[this] { return std::span<const std::uint8_t>(you); };
        result.install.setConfig=[](std::string_view,std::span<const std::uint8_t>) {};result.install.refreshWindowTitle=[] {};result.install.resetCamera=[] {};
        result.ownership.refreshProduction=[] {};result.ownership.themeMode=[] { return 0U; };
        result.assets.postPop.playImpactSound=[](std::uint32_t) {};result.assets.queueCommand=[](std::uint32_t,std::int32_t,std::int32_t) {};
        result.priest.shield.noticeSound=[](const PriestShieldNotice&) {};result.priest.shield.translate=[](std::string_view key) { return std::string(key); };result.priest.shield.tell=[](std::string_view) {};
        result.playerContent.gump=[](std::uint32_t) { return 0U; };
        result.playerContent.handle=[](std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { throw std::logic_error("State 공유 검사의 미설치 UI"); };
        result.deletion.selected=[] { return Sid{}; };result.deletion.emit=[](const SquidDestroyEvent&) { throw std::logic_error("State 공유 검사의 미선택 삭제"); };
        result.player.log=[](std::uint32_t,std::string_view) {};result.missionOutcome.demo=[] { return 0U; };
        result.carrierContent.sound=[this](std::uint32_t sound) { pickupSounds.push_back(sound); };
        if (login) {
            CampaignLoginServices effects; // 실제 로컬 handshake의 장치 경계만 제공하며 응답/상태를 대신 쓰지 않는다.
            effects.configInt=[](std::string_view,std::int32_t fallback) { return fallback; };effects.wallSeconds=[this] { return wall; };effects.clock=[] { return 100250U; };
            effects.refresh=[] {};effects.playerUiBusy=[] { return false; };effects.playerNotice=[](std::uint32_t,std::uint32_t,std::uint32_t) {};
            effects.log=[this](std::string_view text) { logs.emplace_back(text); };effects.observePacket=[this](auto sender,bool,std::uint32_t,auto packet) { if (observe) observe(sender,packet); };
            result.localLogin=std::move(effects);result.ai.participation.clearChat=[](std::uint32_t) {};result.ai.participation.addChat=[](std::uint32_t,std::string_view,std::uint32_t) {};
        }
        return result;
    }
};
// 시각을 산술 변환하지 않고 현재 owner의 원시64비트와 별개 DWORD를 진단한다.
std::array<std::uint64_t,9> LocalStateValues(const ClientLocalStateGlobals& owner) {
    return {owner.ZonePending(),owner.SourceType(),owner.SavedGame() ? 1U : 0U,owner.Begin(),owner.CentralCreated(),std::bit_cast<std::uint64_t>(owner.CaptureEnd()),std::bit_cast<std::uint64_t>(owner.ConnectEnd()),std::bit_cast<std::uint64_t>(owner.LoginEnd()),std::bit_cast<std::uint64_t>(owner.ActivityEnd())};
}
// wire에 저장된 실제 double의8BYTE를 비정렬 little endian으로 관측한다.
double LocalStateReplyRemaining(std::span<const std::uint8_t> packet) {
    if (packet.size()!=18) throw std::out_of_range("State 공유 Reply18BYTE 누락");std::uint64_t bits{};
    for (std::size_t byte=0;byte<8;++byte) bits|=std::uint64_t(packet[10+byte])<<(byte*8); // packet의 인접 필드는 읽지 않는다.
    return std::bit_cast<double>(bits);
}
// 원문10.78 Reply18의 고정 버전/owner/accepted와 독립 remaining 입력만 작성한다.
std::array<std::uint8_t,18> LocalStateReply(double remaining) {
    std::array<std::uint8_t,18> packet{1,0x28,18,0,10,0,0x4e,0,1,1};const auto bits=std::bit_cast<std::uint64_t>(remaining);
    for (std::size_t byte=0;byte<8;++byte) packet[10+byte]=static_cast<std::uint8_t>(bits>>(byte*8)); // reader/수신기를 호출하여 입력을 대신 만들지 않는다.
    return packet;
}
}

// Native cleanup 전체와 구별하여 정상 생성/반납 자체가 Client owner를 재초기화하지 않는지 두 Session에서 확인한다.
TEST_CASE(ClientLocalStateGlobals_TwoSessionLifetimesPreserveCurrentOwner) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);LocalStateSessionDevices devices;auto owner=std::make_shared<ClientLocalStateGlobals>();
    owner->ZonePending()=0x12345678U;owner->SourceType()=0x76543210U;owner->SavedGame()=true;owner->Begin()=19;owner->CentralCreated()=37;owner->CaptureEnd()=501.25;owner->ConnectEnd()=609.5;owner->LoginEnd()=713.75;owner->ActivityEnd()=819.125;
    for (std::size_t block=0;block<owner->Blocks().size();++block) for (std::size_t word=0;word<owner->Blocks()[block].size();++word) owner->Blocks()[block][word]=static_cast<std::uint32_t>(block*1000+word+1); // stale 인자도 포함한 전체720BYTE를 구별하는 독립 입력이다.
    const auto values=LocalStateValues(*owner);const auto blocks=owner->Blocks();const auto* loginAddress=&owner->LoginEnd();
    for (int index=0;index<2;++index) { // 기존 Session 반납 뒤에도 같은 Client owner를 다음 Session에 빌린다.
        { auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",devices.Make(owner,false));CHECK(session.LocalConnectionPhase()==0 && LocalStateValues(*owner)==values && owner->Blocks()==blocks); }
        CHECK(&owner->LoginEnd()==loginAddress && LocalStateValues(*owner)==values && owner->Blocks()==blocks);
    }
}

// 실제 Zone/Reply/Connected/Timer writer와 비동기 재수신이 같은 owner를 갱신하고 중앙섬 분기도 현재 값을 읽는다.
TEST_CASE(ClientLocalStateGlobals_ActualLoginAndReplyUseCurrentOwner) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    for (const std::uint32_t central:{0U,1U}) { // 같은 metadata writer의 자연 로그 유무로 중앙섬 alias를 구별한다.
        LocalStateSessionDevices devices;auto owner=std::make_shared<ClientLocalStateGlobals>();owner->SourceType()=0x13572468U;owner->ZonePending()=0x24681357U;owner->Begin()=27;owner->CentralCreated()=central;owner->ActivityEnd()=42.5;
        std::vector<std::uint8_t> firstReply;double connecting{},capture{};std::uint32_t beginning{}; // 0x28 본문 전에만 관측하는 실제 중간 값이다.
        devices.observe=[&](auto sender,auto packet) { if (packet[1]==0x28 && firstReply.empty()) { CHECK(sender==9);firstReply.assign(packet.begin(),packet.end());connecting=owner->ConnectEnd();capture=owner->CaptureEnd();beginning=owner->Begin(); } };
        auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",devices.Make(owner,true));session.LoadLocalConnection();
        CHECK(session.LocalConnectionPhase()==14 && session.LocalOwner()==1 && owner->Blocks()[3][0]==0);CHECK(!firstReply.empty() && LocalStateReplyRemaining(firstReply)==150.0 && connecting==130.25 && capture==250.25 && beginning==1);
        CHECK(owner->CaptureEnd()==250.25 && owner->ConnectEnd()==0.0 && owner->LoginEnd()==0.0 && owner->Begin()==0 && !owner->SavedGame());CHECK(owner->SourceType()==0x13572468U && owner->ZonePending()==0x24681357U && owner->CentralCreated()==central && owner->ActivityEnd()==42.5);
        CHECK(std::count(devices.logs.begin(),devices.logs.end(),"No Central Island.\n")==static_cast<std::ptrdiff_t>(central ? 0 : 1)); // 실제SendMetadata가0일때만선택한로그다.
        owner->SourceType()=0xabcdef01U;owner->ZonePending()=0xfedcba98U;devices.wall=125.5;const auto before=session.LocalConnectionPackets().size();const auto reply=LocalStateReply(7.25);session.SendLocalPacket(1,reply); // 1은실제서버끝→client9방향이며9는반대방향이다.
        CHECK(session.LocalConnectionPackets().size()==before && owner->CaptureEnd()==250.25 && owner->SourceType()==0xabcdef01U && owner->Blocks()[4][0]==0);session.PumpLocalConnection();
        CHECK(session.LocalConnectionPackets().size()==before+1 && session.LocalConnectionPackets().back()==0x28 && session.LocalConnectionPhase()==14);CHECK(owner->CaptureEnd()==132.75 && owner->SourceType()==0 && owner->Blocks()[4][0]==3 && owner->ZonePending()==0xfedcba98U && owner->ConnectEnd()==0.0); // 실제Reply의phase14후반직접store이며State3실행은하지않는다.
    }
}

// 저장게임관문은Login사본이아닌같은owner를읽고실제내용물부착/부모통지의음향효과를생략하거나재개한다.
TEST_CASE(ClientLocalStateGlobals_SavedGameControlsActualCarrierContentNotice) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);LocalStateSessionDevices devices;auto owner=std::make_shared<ClientLocalStateGlobals>();auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",devices.Make(owner,false));session.Load();
    const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(priest.value);if (!priest.value) return;auto core=session.Core();CHECK(core.pool.Slot(priest)[10]==158 && core.pool.Slot(priest)[34]==1);devices.pickupSounds.clear();
    owner->SavedGame()=true;const auto first=session.CreateContained(priest,158,0x82,0x20,0,0);CHECK(first.value && SquidForms(core.pool).Parent(first)==priest && devices.pickupSounds.empty()); // 저장중에도실제Form생성/부착은수행하고Notice첫관문에서만생략한다.
    owner->SavedGame()=false;const auto second=session.CreateContained(priest,158,0x82,0x20,0,0);CHECK(second.value && second!=first && SquidForms(core.pool).Parent(second)==priest && devices.pickupSounds.size()==1); // 같은현재owner변경직후TYPE+7c의실제음향효과가도달해야한다.
}

// 실제State next와ZonePending을직접쓰는요청writer의지정폭·효과뒤현재값재조회·unsigned감김을확인한다.
TEST_CASE(ClientLocalStateGlobals_FortRequestWritesActualNextAndPending) {
    ClientLocalStateGlobals owner;auto state=owner.State();
    for (auto& block:owner.Blocks()) block.fill(0xa5a5a5a5U); // 전체720BYTE의지정필드밖stale입력을보존해야한다.
    owner.ZonePending()=17;const auto before=owner.Blocks();MissionFortLoadRequest request;request.next.fill(0x6b6b6b6bU);std::array<std::uint8_t,9> address{'z','l','o','c','a','l',',','*',0};std::uint32_t port=6799; // 실제로컬주소9BYTE와현재포트의독립입력이다.
    MissionFortRequestHooks effects;effects.selectFort=[&](auto name) { CHECK(name=="Tutorial1");request.fort=name; };
    effects.duplicate=[&](auto& packet) { CHECK(&state.next==&owner.Blocks()[4] && state.next[0]==7 && state.next[31]==6799 && packet.storage.size()==512 && packet.storage[2]==4);owner.ZonePending()=0xffffffffU;port=9876;state.next[24]=0x76543210U;return CaptureMissionFortZone(request,packet); }; // 실제복제효과중바뀐같은owner와별개staleDWORD를그뒤에도보존한다.
    WriteMissionFortRequest("Tutorial1",{state.next,owner.ZonePending(),address,port},effects);CHECK(owner.ZonePending()==0 && state.next[0]==7 && state.next[21]==0x636f6c7aU && state.next[22]==0x2a2c6c61U && state.next[23]==0xa5a5a500U && state.next[24]==0x76543210U && state.next[31]==6799 && state.next[32]==1);CHECK(request.zone.size()==86 && request.fort=="Tutorial1" && request.next[0]==0x6b6b6b6bU && request.sequence==0);
    for (std::size_t block=0;block<owner.Blocks().size();++block) for (std::size_t word=0;word<owner.Blocks()[block].size();++word) { // 지정한next필드와외부효과의DWORD외에는이웃블록/예약꼬리도그대로여야한다.
        if (block==4 && (word==0 || word==21 || word==22 || word==23 || word==24 || word==31 || word==32)) continue;CHECK(owner.Blocks()[block][word]==before[block][word]);
    }
}

// 실제 서버 Fort 수신이 같은 Client 갱신 DWORD에만 비동기로 쓰며 대여 자체는 값을 초기화하지 않는다.
TEST_CASE(ClientMapRevision_ActualFortReceiverSharesCurrentOwnerAndWraps) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);LocalStateSessionDevices devices;auto state=std::make_shared<ClientLocalStateGlobals>();auto gameplay=std::make_shared<ClientGameplayGlobals>(); // 기존 실제 장치/전역만 공급하며 게임 계산을 대체하지 않는다.
    const auto* address=&gameplay->MapRevision();gameplay->MapRevision()=0x12345678U;
    { // 월드를 설치하지 않은 대여의 생성/반납은 전체 갱신 요청을 지우지 않는다.
        auto services=devices.Make(state,false);services.gameplayGlobals=gameplay;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));CHECK(&gameplay->MapRevision()==address && gameplay->MapRevision()==0x12345678U);
    }
    CHECK(&gameplay->MapRevision()==address && gameplay->MapRevision()==0x12345678U);
    auto services=devices.Make(state,true);services.gameplayGlobals=gameplay;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.LoadLocalConnection();CHECK(session.LocalConnectionPhase()==14 && session.LocalOwner()==1);
    const std::array<std::uint8_t,17> packet{1,0x2a,17,0,2}; // 서버의 다른 owner2 Fort 수신은 원문처럼 재설치를 생략하고005949d4만 증가시킨다.
    gameplay->MapRevision()=0xffffffffU;const auto received=session.LocalConnectionPackets().size();session.SendLocalPacket(1,packet);CHECK(gameplay->MapRevision()==0xffffffffU && session.LocalConnectionPackets().size()==received);session.PumpLocalConnection();
    CHECK(&gameplay->MapRevision()==address && gameplay->MapRevision()==0 && session.LocalConnectionPackets().size()==received+1 && session.LocalConnectionPackets().back()==0x2a);
    session.SendLocalPacket(1,packet);CHECK(gameplay->MapRevision()==0);session.PumpLocalConnection();CHECK(gameplay->MapRevision()==1 && session.LocalConnectionPackets().size()==received+2 && session.LocalConnectionPhase()==14);
}
