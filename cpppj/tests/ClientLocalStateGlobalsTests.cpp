// Client 수명의 State/현재 전역을 실제 요청 작성·로그인·내용물 통지가 같은 주소로 소비하는지 검사한다.
#include "DudeSpawnSupport.h"
#include "client/RawCampaignSession.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/ClientStateStorageCleanup.h"
#include "client/ClientMain.h"
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
#include <type_traits>
#include <utility>
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

// Session 없는 새0값으로 치환하지 않고 실제 Zone·ClockSync·MetaZone 주소와 내용을 두 대여에서 유지한다.
TEST_CASE(ClientLocalStateGlobals_ClockZoneMetaSurviveTwoActualSessions) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);LocalStateSessionDevices devices;auto owner=std::make_shared<ClientLocalStateGlobals>();auto clock=owner->ClockSynchronization();clock.connection=0x80000000U;clock.replyMode=0xffffffffU;owner->GameTicks()=0x12345678;owner->ZoneWords()={0x76543210,0xabcd9876};auto meta=owner->MetaZones();meta.firstReset=0x81;std::fill(meta.records.begin(),meta.records.end(),std::uint8_t{0xa5});
    const auto* zoneAddress=owner->Zone().data();const auto* metaAddress=meta.records.data();
    for (int index=0;index<2;++index) { //각 실제 Session이 사라져도 현재 장기 물리는 다시 초기화되지 않는다.
        { auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",devices.Make(owner,false));CHECK(owner->Zone().data()==zoneAddress && owner->MetaZones().records.data()==metaAddress); }
        CHECK(owner->SynchronizationConnection()==0x80000000U && owner->SynchronizationReplyMode()==0xffffffffU && owner->GameTicks()==0x12345678 && owner->ZoneWords()[0]==0x76543210 && owner->ZoneWords()[1]==0xabcd9876 && meta.firstReset==0x81 && std::all_of(meta.records.begin(),meta.records.end(),[](auto b) { return b==0xa5; }));
    }
}

// 실제 Join writer가 같은 Zone을 송신하고 비동기 Join reader가 같은6BYTE에 복사하는지 확인한다.
TEST_CASE(ClientLocalStateGlobals_ActualLoginSharesZoneAndPreservesTailWord) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);LocalStateSessionDevices devices;auto owner=std::make_shared<ClientLocalStateGlobals>();owner->ZoneWords()={0x11111111,0xabcdbeef};owner->GameTicks()=0x87654321;std::array<std::uint8_t,6> sent{};bool joined{};
    devices.observe=[&](auto,auto packet) { if (packet[1]==0x29 && !joined) { CHECK(packet.size()==34 && std::equal(owner->Zone().begin(),owner->Zone().end(),packet.begin()+8));std::copy_n(packet.begin()+8,6,sent.begin());std::fill(owner->Zone().begin(),owner->Zone().end(),std::uint8_t{0xcc});joined=true; } }; //실제 reader 앞에서만 현재 주소를 변경하여 송신 사본·수신 대상의 alias를 구별한다.
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",devices.Make(owner,true));session.LoadLocalConnection();
    CHECK(joined && session.LocalConnectionPhase()==14 && std::equal(sent.begin(),sent.end(),owner->Zone().begin()) && (owner->ZoneWords()[1]>>16)==0xabcd && owner->GameTicks()==0x87654321);ResetClientClockSynchronization(*owner);CHECK(owner->GameTicks()==0 && std::equal(sent.begin(),sent.end(),owner->Zone().begin()));ResetClientStateZone(*owner);CHECK(owner->ZoneWords()[0]==0 && owner->ZoneWords()[1]==0xabcd0000);
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

// 보조 State 입력/관측 helper는 기존 검사 장면과 함께 번역 단위 내부에서만 사용한다.
namespace {
// State 보조9DWORD와 별도 재접속 기한의 현재 원시 비트를 독립적으로 관측한다.
std::array<std::uint64_t,10> LocalStateAuxValues(const ClientLocalStateGlobals& owner) {
    return {owner.ScriptBreak(),owner.AbortPending(),owner.StateAuxCC(),owner.MetadataMode(),owner.ConnectionNotified(),owner.CachedPeerIndex(),owner.CachedPeerValue(),owner.CachedPeerFieldA4(),owner.CachedPeerFieldA8(),std::bit_cast<std::uint64_t>(owner.ReconnectSaveEnd())};
}
// 가짜 Native 실행 대신 서로 구별되는 DWORD와 NaN payload를 대여 경계의 명시 입력으로 넣는다.
void LocalStateAuxInputs(ClientLocalStateGlobals& owner) {
    owner.ScriptBreak()=0x80000000U;owner.AbortPending()=0xffffffffU;owner.StateAuxCC()=0x12345678U;owner.MetadataMode()=0xfedcba98U;owner.ConnectionNotified()=0x80000001U;
    owner.CachedPeerIndex()=71;owner.CachedPeerValue()=0;owner.CachedPeerFieldA4()=0xffffffffU;owner.CachedPeerFieldA8()=0x87654321U;owner.ReconnectSaveEnd()=std::bit_cast<double>(std::uint64_t{0x7ff8000012345678ULL});
}
}

// PE의 초기1/ffffffff/BSS +0, 정확한4/8BYTE 참조와 두 실제 Client의 별도 수명을 검사한다.
TEST_CASE(ClientLocalStateGlobals_StateAuxInitialBitsAndTwoActualClientsAreIndependent) {
    static_assert(std::is_same_v<decltype(std::declval<ClientLocalStateGlobals&>().ScriptBreak()),std::uint32_t&>); // DWORD를 bool이나 복사값으로 바꾸지 않는다.
    static_assert(std::is_same_v<decltype(std::declval<ClientLocalStateGlobals&>().StateAuxCC()),std::uint32_t&>); // 005c85cc는 인접 DWORD를 포함한 double이 아니다.
    static_assert(std::is_same_v<decltype(std::declval<ClientLocalStateGlobals&>().ReconnectSaveEnd()),double&>); // 재접속 기한은 정확히 double 참조다.
    static_assert(std::is_same_v<decltype(std::declval<const ClientLocalStateGlobals&>().StateAuxCC()),const std::uint32_t&>); // const 진단도 같은4BYTE를 빌린다.
    static_assert(sizeof(std::uint32_t)==4 && sizeof(double)==8); // Native 접근 폭과 C++ 저장소 폭을 명시한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);std::shared_ptr<ClientLocalStateGlobals> firstOwner;const std::array<std::uint64_t,10> initial{1,0,0,0,0,0xffffffffU,0xffffffffU,0xffffffffU,0xffffffffU,0}; // 보호 PE의 명시 초기 이미지와 첫 Client 밖에서도 유지할 실제 owner다.
    { ClientOptions options;options.gameDirectory=assets.root;options.noAudio=true;Client first(std::move(options));firstOwner=first.LocalStateGlobals();CHECK(LocalStateAuxValues(*firstOwner)==initial);LocalStateAuxInputs(*firstOwner); } // 실제 Client의 전역 singleton 계약을 지키며 owner의 공유 수명은 유지한다.
    const auto firstValues=LocalStateAuxValues(*firstOwner); // 첫 Client를 반납해도 현재 원시 비트는 남는다.
    { ClientOptions options;options.gameDirectory=assets.root;options.noAudio=true;Client second(std::move(options));const auto secondOwner=second.LocalStateGlobals();CHECK(firstOwner!=secondOwner && LocalStateAuxValues(*secondOwner)==initial);secondOwner->StateAuxCC()=0x80000000U;secondOwner->MetadataMode()=7;secondOwner->ReconnectSaveEnd()=-0.0;
      CHECK(LocalStateAuxValues(*firstOwner)==firstValues && std::bit_cast<std::uint64_t>(secondOwner->ReconnectSaveEnd())==0x8000000000000000ULL && firstOwner->StateAuxCC()==0x12345678U); }
    CHECK(LocalStateAuxValues(*firstOwner)==firstValues);const auto& constant=*firstOwner;CHECK(&constant.ScriptBreak()==&firstOwner->ScriptBreak() && &constant.StateAuxCC()==&firstOwner->StateAuxCC() && &constant.ReconnectSaveEnd()==&firstOwner->ReconnectSaveEnd()); // 현재 주소 관측을 실제 수명/소비자 검사와 함께 둔다.
}

// 일반 Session과 Login 모듈의 생성/반납은 새 Client 최초값을 반복 설치하지 않는다.
TEST_CASE(ClientLocalStateGlobals_StateAuxOrdinarySessionAndLoginLifetimesPreserveCurrentBits) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ClientOptions options;options.gameDirectory=assets.root;options.noAudio=true;Client client(std::move(options));LocalStateSessionDevices devices;const auto owner=client.LocalStateGlobals();LocalStateAuxInputs(*owner);
    owner->LoginEnd()=std::bit_cast<double>(std::uint64_t{0x8000000000000000ULL});owner->ActivityEnd()=17.25;owner->CaptureEnd()=23.5;owner->ConnectEnd()=31.75;const auto deadlines=LocalStateValues(*owner); // 서로 다른 Native 기한을 다른 비트로 구별한다.
    for (const bool login:{false,true}) { // 실제 Login 객체를 조립하는 경우도 같은 장기 DWORD를 보존한다.
        const auto before=LocalStateAuxValues(*owner);const auto* address=&owner->ScriptBreak();
        { auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",devices.Make(owner,login));CHECK(LocalStateAuxValues(*owner)==before && LocalStateValues(*owner)==deadlines && session.LocalConnectionPhase()==0);owner->StateAuxCC()^=0xffffffffU;owner->ScriptBreak()=0xffffffffU; }
        auto after=before;after[0]=0xffffffffU;after[2]^=0xffffffffU;CHECK(&owner->ScriptBreak()==address && LocalStateAuxValues(*owner)==after && LocalStateValues(*owner)==deadlines); // 반납은 현재 효과를 초기 입력으로 되돌리지 않는다.
    }
}

// 실제0x27→0x28→Ready→Timer 완료의0 store가 Client의 현재 DWORD에 도달하는지 본다.
TEST_CASE(ClientLocalStateGlobals_ActualLoginTimerClearsCurrentScriptBreakOnly) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    for (const std::uint32_t initial:{0U,0x80000000U,0xffffffffU}) { // 0/상위비트/모든비트 입력을 bool로 좁히지 않고 같은 실제 로그인에 넣는다.
        ClientOptions options;options.gameDirectory=assets.root;options.noAudio=true;Client client(std::move(options));LocalStateSessionDevices devices;const auto owner=client.LocalStateGlobals();LocalStateAuxInputs(*owner);owner->ScriptBreak()=initial;const auto before=LocalStateAuxValues(*owner);bool observed{};
        devices.observe=[&](auto sender,auto packet) { if (packet[1]==0x28 && !observed) { observed=true;CHECK(sender==9 && owner->ScriptBreak()==initial);owner->ScriptBreak()=0x80000000U; } }; // 현재 참조는 Login 생성 뒤 외부 효과에서 바뀌는 값도 같은 주소에 반영한다.
        { auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",devices.Make(owner,true));CHECK(LocalStateAuxValues(*owner)==before);session.LoadLocalConnection();CHECK(observed && session.LocalConnectionPhase()==14 && owner->ScriptBreak()==0); }
        auto expected=before;expected[0]=0;CHECK(LocalStateAuxValues(*owner)==expected); // 8610과8개의 다른 DWORD에는 실제 완료 writer의 효과가 없다.
    }
}

// 실제SendBegin/ClientBegin과LocalGeyser가같은595088 물리를signed/unsigned로소비한다.
TEST_CASE(GameAux_ActualLoginAndQueuedBeginShareSignedResourcePool) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);LocalStateSessionDevices devices;auto state=std::make_shared<ClientLocalStateGlobals>();auto gameplay=std::make_shared<ClientGameplayGlobals>();const auto owner=gameplay->GameAux();std::uint32_t begins{};
    const auto read=[](std::span<const std::uint8_t> packet) { std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(packet[30+byte])<<(byte*8);return value; }; // 실제34BYTE의현재마지막DWORD만읽는다.
    devices.observe=[&](auto sender,auto packet) { if (packet[1]!=0x29) return;CHECK(sender==9 && packet.size()==34);if (!begins) CHECK(read(packet)==0x89abcdefU);++begins;owner->StoreResourcePoolBits(0x12345678U); }; // 수신전효과가바꾼owner를실제ClientBegin writer가다시쓴다.
    auto services=devices.Make(state,true);services.gameplayGlobals=gameplay;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));owner->StoreResourcePoolBits(0x89abcdefU);session.LoadLocalConnection();CHECK(session.LocalConnectionPhase()==14 && begins==1 && owner->ResourcePoolBits()==0x89abcdefU);
    auto& pool=session.Construction().LocalGeysers().ResourcePool();CHECK(&pool==&owner->ResourcePool() && &session.World().Storage()->GeyserType()==&gameplay->GeyserType());pool=12345;
    std::array<std::uint8_t,34> packet{1,0x29,34,0,1};const std::uint32_t value=0xfedcba98U;for (std::size_t byte=0;byte<4;++byte) packet[30+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 원문헤더/폭과독립현재입력만구성한다.
    const auto recorded=session.LocalConnectionPackets().size();session.SendLocalPacket(1,packet);CHECK(pool==12345 && begins==1 && session.LocalConnectionPackets().size()==recorded);session.PumpLocalConnection();CHECK(begins==2 && owner->ResourcePoolBits()==value && std::bit_cast<std::uint32_t>(pool)==value);
}
// 같은Authority라도다른게임보조owner는Gameplay/Storage대여및fresh초기화전에거부한다.
TEST_CASE(GameAux_SessionIdentityFailureLeavesBothOwnersUnchanged) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);LocalStateSessionDevices devices;auto state=std::make_shared<ClientLocalStateGlobals>();auto gameplay=std::make_shared<ClientGameplayGlobals>();auto foreign=std::make_shared<RawGameAuxState>();auto surface=MakeRawSurfaceStorage(assets.game,gameplay->Authority(),foreign);gameplay->GameAux()->ResourcePool()=73;foreign->ResourcePool()=79;gameplay->cursor.screenX=83;
    auto services=devices.Make(state,false);services.gameplayGlobals=gameplay;services.surfaceStorage=surface;bool rejected{};
    try { auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services)); }catch (const std::invalid_argument& error) { rejected=std::string(error.what()).find("게임 보조")!=std::string::npos; }
    CHECK(rejected && !gameplay->Borrowed() && !surface->Borrowed() && surface->Generation()==0 && gameplay->GameAux()->ResourcePool()==73 && foreign->ResourcePool()==79 && gameplay->cursor.screenX==83);
}

#include "MissionEntryRuntimeTests.inc" // 같은 기존 Session 장치로 실제 시작 CD/Zone 뒤 Renderer 접합을 확인한다.
#include "MissionStartingTailTests.inc" // 실제 후반 UI/기한/인자/생성 전 팔레트를 같은 장치로 함께 확인한다.
#include "MissionAsyncConnectionTests.inc" // 실제 상태/패킷 프레임 경계와 준비 중 폐기/완료 공개를 확인한다.
#include "MissionStartingCommitTests.inc" // 실제7 공통 확정/안내 위치와 미확정 예외의 부분 반납을 확인한다.
#include "MissionStartingPrefixTests.inc" // 실제 기본 배경 분모/정리 전 지역 값과 살아 있는 Native 요청을 확인한다.
#include "MissionStartingParentTests.inc" // 앞단 전 최초 복사/로그와 두 World 사이의 같은 부모7/8 확정을 확인한다.
#include "EditorEntryPreludeTests.inc" // State1의 공통 출처/첫BYTE와 실제 로컬52h 예약을 cleanup 전에 확인한다.

#include "CampaignObjectStreamSessionTests.inc" // 실제 캠페인 객체 child도 원시 섹션과 같은 변환표/예외 수명을 사용하는지 확인한다.

#include "CampaignLoginObjectStreamTests.inc" // 로그인 재읽기도 객체의 실제 소비 시점과 부분 월드를 유지하는지 확인한다.
