// Player 부재 단계의 실제 구성과 원문 선택 위치의 실패/후속 할당을 비MP 경계로 검사한다.
#include "ConstructionSupport.h"
#include "client/ClientPlayerRecords.h"
#include "client/RawCampaignConstructionBindings.h"
#include "o/RawPlayerParticipation.h"
#include <utility>

using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::construction;
namespace { // 같은 작은 물리 입력만 구성하며 패킷/통신/게임 실행은 하지 않는다.
// 계약 오류 종류를 구별해 다른 부작용의 예외가 검사를 통과시키지 않게 한다.
template<class Error,class Function> bool LifetimeRejects(Function&& function) {
    try { function();return false; } catch (const Error&) { return true; }
}
// 선택되지 않은 생성/표시/전송 효과는 관찰 성공으로 대신하지 않는다.
template<class Result,class... Arguments> std::function<Result(Arguments...)> LifetimeForbidden() {
    return [](Arguments...)->Result { throw std::logic_error("Player 수명 검사에서 선택 금지된 실제 효과"); };
}
// 지연 구성에 필요한188TYPE/프레임과 실제9방향/현재 모드 입력이다.
struct LifetimeInputs {
    SidPool pool{OriginalEdition::Patch1078,32768}; // 실제 판본/서버 영역의 유일한 검사 풀이다.
    SquidHash hash; // 후보 계산이 읽는 같은 현재 해시다.
    ChunkMap chunks{2,2}; // record 검색/현재 지도 효과의 실제 저장소다.
    std::vector<RiftTypeRecord> types{188}; // 현재 puzzlePiece 발자국/간헐천 타입을 제공한다.
    std::vector<RiftTypeFrames> frames{188,RiftTypeFrames({})}; // 모든 현재 TYPE에 대응하는 물리 슬롯이다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); // 후보의 실제 BYTE 지형이다.
    std::vector<std::uint8_t> players; // 처음은 실제 미할당이며 테스트가 명시한 시점에만 기록을 만든다.
    SquidPostPopList recent; // 부모의 수량0/권한 guard가 보존/초기화하는 실제 목록이다.
    LocalGeyserState state; // 값형 간헐천 전역이며 Native 조회 경계와 독립이다.
    bool battle{true},mission{true},metamap{}; // Bounds가 직접 읽는 현재 세 모드다.
    std::uint8_t mapSize{}; // 현재 옵션0이다.
    std::uint32_t piece{107},reads{},diagnostics{},configs{}; // 현재 패턴과 조회/진단/설정 효과 횟수다.
    std::array<std::uint32_t,9> directions{0,4,0,6,2,7,3,5,1}; // 원본 Player별 기본 방향이다.
    // 같은 지역7의 현재 실제 패턴과 별도 최근5기록을 공급한다.
    LifetimeInputs() {
        frames[107]=InputFrames(0);types[107].footX=types[107].footY=1;
        chunks.At(0,0).island=7;chunks.At(0,0).record={0,0,0x21,0,0,0};recent.entries={11,12,13,14,15};recent.count=3;
    }
    PlayerGeyserBoundsState BoundsState() { return {battle,mission,metamap,mapSize,piece,directions}; } // 사본 전역 대신 현재 참조를 빌린다.
    std::function<std::span<const std::uint8_t>()> Reader() { return [this] { ++reads;return std::span<const std::uint8_t>(players); }; } // 실제 선택 때만 읽는다.
    // 생성 관련 하위는 선택 금지하며 설정/선택 this 부모만 명시 관찰한다.
    LocalGeyserHooks Hooks() {
        LocalGeyserHooks hooks;hooks.configInt=[this](auto,auto) { ++configs;return 1; };hooks.wallSeconds=LifetimeForbidden<double>();
        hooks.playerBounds=LifetimeForbidden<FortRandomRectangle,std::uint32_t>();hooks.negativeQuantity=LifetimeForbidden<void>();
        hooks.destroy=LifetimeForbidden<void,Sid,std::uint32_t>();hooks.confirm=LifetimeForbidden<void,const ConstructionConfirmRequest&>();
        hooks.create=LifetimeForbidden<Sid,std::uint32_t,std::uint32_t>();hooks.pop=LifetimeForbidden<void,Sid,float,float,std::uint32_t>();
        hooks.regionExists=[](auto region) { return region==7; };hooks.playerBoundsRecord=[](auto owner,auto record) { CHECK(owner==1 && Get(record,0x78)==7);return FortRandomRectangle{28,44,68,84}; };return hooks;
    }
};
}

// 구성/지도 helper에는 Player가 필요 없으며 실제 Bounds는 그 자리에서 현재 물리를 요구한다.
TEST_CASE(PlayerGeyserBounds_DeferredConstructionRequiresPlayersOnlyWhenSelected) {
    LifetimeInputs input;RawPlayerGeyserBounds body(input.Reader(),input.pool,input.chunks,input.types,input.frames,input.BoundsState(),[&](auto) { ++input.diagnostics; });
    CHECK(input.reads==0 && input.players.empty());CHECK(body.RegionRecord(7)[2]==0x21);CHECK(body.Placement(1,input.chunks.At(0,0).record).left==2);CHECK(input.reads==0);
    CHECK((LifetimeRejects<std::out_of_range>([&] { body.Bounds(9); })));CHECK(input.reads==0);
    CHECK((LifetimeRejects<std::out_of_range>([&] { body.Bounds(1); })));CHECK(input.reads==1 && input.diagnostics==0);
    input.players.resize(9*0xb4-1);CHECK((LifetimeRejects<std::out_of_range>([&] { body.Bounds(1); })));CHECK(input.reads==2 && input.diagnostics==0);
    input.players.resize(9*0xb4);Put(input.players,0xb4+0x50,2,1);Put(input.players,0xb4+0x78,7);const auto area=body.Bounds(1);CHECK(area.left==28 && area.top==44 && input.reads==3 && input.diagnostics==0);
    input.players.clear();CHECK(body.RegionRecord(7)[2]==0x21);CHECK((LifetimeRejects<std::out_of_range>([&] { body.Bounds(1); })));CHECK(input.reads==4);
    CHECK((LifetimeRejects<std::invalid_argument>([&] { RawPlayerGeyserBounds rejected(input.pool,input.chunks,input.types,input.frames,input.players,input.BoundsState(),[](auto) {}); })));
    CHECK((LifetimeRejects<std::invalid_argument>([&] { RawPlayerGeyserBounds rejected(std::function<std::span<const std::uint8_t>()>{},input.pool,input.chunks,input.types,input.frames,input.BoundsState(),[](auto) {}); })));
    CHECK((LifetimeRejects<std::invalid_argument>([&] { RawPlayerGeyserBounds rejected(input.Reader(),input.pool,input.chunks,input.types,input.frames,input.BoundsState(),{}); })));CHECK(input.reads==4);
}

// Populate의 권한 guard/최근목록 쓰기와 Generate의 앞선 설정 효과를 Player 조회 앞으로 옮기지 않는다.
TEST_CASE(LocalGeyser_DeferredConstructionPreservesSelectedGuardAndPartialEffects) {
    LifetimeInputs input;RawLocalGeyser body(input.Reader(),input.pool,input.hash,input.spots,input.types,input.frames,input.recent,input.state,input.Hooks());CHECK(input.reads==0 && input.configs==0 && input.recent.count==3);
    GameRandom random(123);input.state.boss=false;body.Populate(0,random,{}, {1,1,1});CHECK(input.reads==0 && input.recent.count==3 && random.State()==123);
    input.state.boss=true;body.Populate(0,random,{}, {0,1,1});CHECK(input.reads==0 && input.recent.count==0 && random.State()==123);
    input.recent.count=3;CHECK((LifetimeRejects<std::out_of_range>([&] { body.Populate(0,random,{}, {1,1,1}); })));CHECK(input.reads==1 && input.recent.count==0 && random.State()==123);
    input.state.boss=false;CHECK((LifetimeRejects<std::out_of_range>([&] { body.Generate(); })));CHECK(input.configs==1 && input.reads==2);
    input.players.resize(1620);Put(input.players,0xb4+0x50,1,1);Put(input.players,0xb4+0x78,7);const auto exclusions=body.PlayerExclusions();CHECK(exclusions.size()==1 && input.reads==10);if (!exclusions.empty()) CHECK(exclusions[0].left==16 && exclusions[0].top==32);
    CHECK((LifetimeRejects<std::invalid_argument>([&] { RawLocalGeyser rejected(std::function<std::span<const std::uint8_t>()>{},input.pool,input.hash,input.spots,input.types,input.frames,input.recent,input.state,input.Hooks()); })));
    auto missing=input.Hooks();missing.playerBoundsRecord={};CHECK((LifetimeRejects<std::invalid_argument>([&] { RawLocalGeyser rejected(input.Reader(),input.pool,input.hash,input.spots,input.types,input.frames,input.recent,input.state,std::move(missing)); })));CHECK(input.reads==10);
}

// 실제 ClientPlayerRecords의 첫 할당/재할당 앞뒤에서도 조립이 Player를 고정하거나 초기화하지 않는다.
TEST_CASE(CampaignConstruction_DeferredReaderUsesActualClientOwnerWithoutEagerAllocation) {
    LifetimeInputs input;std::vector<SquidDisplayShape> shapes(188);BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157;
    RawSurfaceWorld world(OriginalEdition::Patch1078,input.types,input.frames,shapes,links,97);const auto core=world.Core();GameRandom random(123);ScrambledSpStore sp(random,[] { return 1U; });SquidRewardState reward;SquidReward hp(core.pool,core.types,core.bookkeeping,reward,&sp);
    const auto records=std::make_shared<ClientPlayerRecords>([](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0xa5}); });BattlePacketPeers peers;std::uint32_t localOwner=1;bool server=true;std::array<std::uint8_t,1620> wirePlayers{};RawBattleBroadcast broadcast(OriginalEdition::Patch1078,wirePlayers,peers,localOwner,server);
    RawCampaignConstructionState state;SquidPostPopList all;RawCampaignConstructionEffects effects;effects.configInt=LifetimeForbidden<std::int32_t,std::string_view,std::int32_t>();effects.wallSeconds=LifetimeForbidden<double>();effects.refresh=LifetimeForbidden<void>();effects.negativeQuantity=LifetimeForbidden<void>();effects.boundsAssertion=LifetimeForbidden<void,PlayerGeyserBoundsAssertion>();
    const auto poolBefore=std::vector<std::uint8_t>(core.pool.Bytes().begin(),core.pool.Bytes().end());const auto generation=world.Storage()->Generation();std::uint32_t reads{};auto runtime=BindConstructionRuntime(world);runtime.players=[records,&reads] { ++reads;return records->Records(); };
    std::vector<PriestPlainCanonType> canon(188);canon[107].frames=InputFrames(0); // 모듈보다 오래 사는 같은 Canon 프레임 표다.
    RawCampaignConstructionBindings construction(std::move(runtime),hp,sp,reward,canon,input.chunks,{},broadcast,state,{core.ownerMode.battle,input.mission,core.ownerMode.challenge,input.mapSize,input.piece,input.directions},input.recent,all,std::move(effects));
    CHECK(reads==0 && records->Base()==0 && !records->Initialized() && world.Storage()->Generation()==generation);CHECK(std::equal(poolBefore.begin(),poolBefore.end(),core.pool.Bytes().begin()));CHECK(construction.PlayerBounds().RegionRecord(7)[2]==0x21);
    CHECK((LifetimeRejects<std::out_of_range>([&] { construction.PlayerBounds().Bounds(1); })));CHECK(reads==1);
    const auto initialize=[&] { records->Initialize([&](auto owner,auto mask) { std::array<std::uint8_t,9> hidden{};PlayerParticipationState participationState;RawPlayerParticipation participation(OriginalEdition::Patch1078,records->Records(),records->Alliances(),hidden,participationState,{});participation.SetAlliance(owner,mask); });auto players=records->Records();Put(players,0xb4+0x50,2,1);Put(players,0xb4+0x78,7); }; // 실제 원문 Player 할당과 owner/비트마스크를 받는 동맹 자식을 사용한다.
    initialize();const auto firstBase=records->Base();const auto first=construction.PlayerBounds().Bounds(1);CHECK(first.left==28 && first.top==44 && reads==2);initialize();CHECK(records->Base()!=firstBase);const auto second=construction.PlayerBounds().Bounds(1);CHECK(second.left==28 && second.top==44 && reads==3 && world.Storage()->Generation()==generation);
}
