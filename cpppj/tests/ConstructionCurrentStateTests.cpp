// 원문 효과 뒤 현재 전역을 다시 읽는 건설/정리/지역 간헐천 몸체를 패킷 없이 검사한다.
#include "ConstructionSupport.h"
#include "o/RawConstructionRequest.h"
#include "o/RawLocalGeyser.h"
#include "o/RawGameAuxState.h"

using namespace netstorm::o;
using namespace netstorm::test::construction;
namespace { // 실제 물리 입력과 호출된 게임 효과만 기록하며 전송/수신은 실행하지 않는다.
// 이 검사에서 선택되지 않아야 할 외부 효과를 성공으로 대신하지 않는다.
template<class Result,class... Arguments> std::function<Result(Arguments...)> CurrentForbidden() { return [](Arguments...)->Result { throw std::logic_error("현재 건설 전역 검사에서 선택 금지된 효과"); }; }
// 같은188TYPE/프레임과 물리 지도를 원문 부품에 전달한다.
struct CurrentScene {
    SidPool pool{OriginalEdition::Patch1078,32768,false}; // 실제 서버/예측 SID 영역을 가진 유일한 입력 풀이다.
    SquidHash hash; // 현재 후보 조회의 실제 해시다.
    std::vector<RiftTypeRecord> types{188}; // 효과가 바꾸는 같은 TYPE 기록이다.
    std::vector<PriestPlainCanonType> canon{188}; // Request/Clear의 실제 Canon 입력이다.
    std::vector<RiftTypeFrames> frames{188,RiftTypeFrames({})}; // 로컬 간헐천의 현재 프레임 입력이다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); // 원문 BYTE 지형 표다.
    std::vector<std::uint8_t> players=std::vector<std::uint8_t>(1620); // 생성 순회가 읽는 현재9기록이다.
    SquidPostPopList recent; // 별도 최근5기록이다.
    RawGameAuxState aux; // 같은 실제 장기 마스크/수량/empty TYPE owner다.
    // 순수 게임 경계에 전달할 TYPE100의 실제 프레임/발자국을 준비한다.
    CurrentScene() { canon[100]={InputFrames(0),0};types[100].footX=types[100].footY=1;recent.entries={11,12,13,14,15};recent.count=3; }
    // 필수 실제 효과 형식만 제공하며 각 검사가 선택한 하위만 명시 교체한다.
    LocalGeyserHooks GeyserHooks() {
        LocalGeyserHooks hooks;hooks.configInt=[](auto,auto) { return 1; };hooks.wallSeconds=CurrentForbidden<double>();hooks.playerBounds=CurrentForbidden<FortRandomRectangle,std::uint32_t>();hooks.negativeQuantity=CurrentForbidden<void>();hooks.destroy=CurrentForbidden<void,Sid,std::uint32_t>();hooks.confirm=CurrentForbidden<void,const ConstructionConfirmRequest&>();hooks.create=CurrentForbidden<Sid,std::uint32_t,std::uint32_t>();hooks.pop=CurrentForbidden<void,Sid,float,float,std::uint32_t>();return hooks;
    }
};
}

// 편집 owner는 진입에서 고정하고 MayPlace/순회/Displace 뒤의 패턴·마스크·받침 TYPE는 다시 읽는다.
TEST_CASE(ConstructionCurrent_RequestSelectsGlobalsAfterOriginalEffects) {
    CurrentScene scene;ConstructionRequestState state;bool editor=true;std::uint32_t owner=1,island=94,noIsland=157;auto patterns=state.patternTypes;state.sharedEditor=&editor;state.sharedLocalPlayer=&owner;state.sharedIslandType=&island;state.sharedNoIslandType=&noIsland;state.sharedDisplaceGenus=&scene.aux.ConstructionDisplaceGenus();scene.aux.ConstructionDisplaceGenus()=0;state.localPlayer=8;scene.types[100].flags1=0x400;scene.types[100].flags2=0x200;scene.types[94].footX=scene.types[94].footY=1;scene.types[95].footX=3;scene.types[95].footY=4;std::vector<std::string> trace;std::size_t scans{},reads{};
    state.patternTypesRead=[&] { ++reads;trace.push_back("Pattern");return patterns; };ConstructionRequestHooks hooks;
    hooks.mayPlace=[&](const auto& query) { CHECK(query.owner==1 && reads==0);trace.push_back("May");owner=2;editor=false;patterns[0]=100;return true; };
    hooks.scan=[&](auto) { if (!scans++) trace.push_back("Scan");scene.aux.ConstructionDisplaceGenus()=0x200; };
    hooks.displace=[&](auto,auto,auto,auto player) { CHECK(player==1 && scans>1);trace.push_back("Displace");island=95;noIsland=156;return true; };
    hooks.confirm=[&](const ConstructionConfirmRequest& query) { CHECK(query.player==1);if (query.type==156) { CHECK(query.x==18 && query.y==18);trace.push_back("Base");return true; }CHECK(query.type==100);trace.push_back("Confirm");scene.types[100].flags1=0;return false; };
    hooks.cancel=CurrentForbidden<void,float,float,std::uint32_t,std::uint32_t>();RawConstructionRequest body(scene.pool,scene.hash,scene.types,scene.canon,scene.spots,state,std::move(hooks));CHECK(!body.Request({100,20,21,7,3,0,0,0,0,0}));CHECK((trace==std::vector<std::string>{"May","Pattern","Scan","Displace","Base","Confirm"}) && reads==1 && scans>1);
}

// decoder 생성은 현재 패턴을 고르고 후보 scan 뒤에는 현재 Bridge TYPE로 같은 방향 프레임을 비교한다.
TEST_CASE(ConstructionCurrent_ClearReadsBridgeAfterCandidateEffects) {
    CurrentScene scene;ConstructionClearState state;state.patternTypes[0]=100;std::uint32_t bridge=101;state.sharedBridgeType=&bridge;std::vector<std::string> trace;state.patternTypesRead=[&] { trace.push_back("Pattern");return std::array<std::uint32_t,8>{107,82,94,157,131,129,140,142}; };
    auto raw=InputRaw(scene.pool,Sid{50});std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=100;raw[40]=1;raw[34]=3;Put(raw,14,Bits(20.F));Put(raw,18,Bits(21.F));Put(raw,36,1);scene.hash.Bucket(0,20,21)=50;
    ConstructionClearHooks hooks;hooks.scan=[&](auto) { trace.push_back("Scan");bridge=100; };hooks.destroy=[&](auto sid,auto flags) { CHECK(sid.value==50 && flags==RawConstructionClear::kSamePieceFlag);trace.push_back("Delete"); };RawConstructionClear body(scene.pool,scene.hash,scene.types,scene.canon,state,std::move(hooks));body.Clear({100,20,21,0,0,3});CHECK((trace==std::vector<std::string>{"Pattern","Scan","Delete"}));
}

// 후보 재사용 TYPE는 constructor 시점을 유지하되 실제 교체/받침 TYPE는 앞선 삭제 효과 뒤 현재값이다.
TEST_CASE(ConstructionCurrent_LocalKeepsCandidateTypeAndReselectsReplacementType) {
    CurrentScene scene;scene.types[84].footX=scene.types[84].footY=256;scene.types[122].cost=2000;auto raw=InputRaw(scene.pool,Sid{9});std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=84;Put(raw,14,Bits(32.F));Put(raw,18,Bits(36.F));
    std::fill(scene.spots.begin(),scene.spots.end(),std::uint8_t{1}); // 재사용 후보가 없는 새 좌표는 원문 spot 관문에서 거부하도록 입력을 제한한다.
    for (int y=0;y<16;++y) { // 실제 광역 typed finder가 같은 명시 후보를 찾는 입력이다.
        for (int x=0;x<16;++x) scene.hash.Cell(3,x,y)=9; // 각 물리 버킷은 같은 생존 후보9로 시작한다.
    }
    bool fort=true;std::uint32_t noIsland=157,deleted{},created{},popped{};std::vector<std::uint32_t> bases;LocalGeyserState state;state.emptyGeyserType=70;state.sharedFort=&fort;state.sharedEmptyGeyserType=&scene.aux.EmptyGeyserType();state.sharedNoIslandType=&noIsland;auto hooks=scene.GeyserHooks();
    hooks.destroy=[&](auto sid,auto flags) { CHECK(sid.value==9 && flags==0x400000);++deleted;scene.aux.EmptyGeyserType()=85;noIsland=156; };hooks.confirm=[&](const auto& query) { bases.push_back(query.type);CHECK(query.x==30 && query.y==34); };hooks.create=[&](auto type,auto flags) { CHECK(type==122 && flags==0);return Sid{static_cast<std::uint16_t>(50+created++)}; };hooks.pop=[&](auto,float x,float y,auto flags) { CHECK(x==32 && y==36 && flags==0);++popped; };
    RawLocalGeyser body(scene.pool,scene.hash,scene.spots,scene.types,scene.frames,scene.players,scene.recent,state,std::move(hooks));scene.aux.ResourcePool()=20000;body.BindResourcePool(scene.aux.ResourcePool());GameRandom random(123);body.Populate(2,random,{16,16,48,48});CHECK(scene.recent.count==3 && random.State()==123 && created==0);fort=false;body.Populate(2,random,{16,16,48,48});CHECK(deleted==1 && created==2 && popped==2 && scene.recent.count==2 && scene.aux.ResourcePool()==16000 && (bases==std::vector<std::uint32_t>{156,156}));
}

// 각 Player 경계가 바꾼 signed 수량을 뒤에서 선택하고 기존 fresh 대여는 장기 다섯 전역을 덮지 않는다.
TEST_CASE(ConstructionCurrent_PlayerBoundsEffectsSelectCurrentQuantities) {
    CurrentScene scene;Put(scene.players,0xb4+0x50,2,1);Put(scene.players,0xb4+0x78,1);Put(scene.players,5*0xb4+0x50,2,1);Put(scene.players,5*0xb4+0x78,5);LocalGeyserState state;state.firstPlayerQuantity=100;state.lastPlayerQuantity=200;state.sharedFirstQuantity=&scene.aux.FirstLocalGeyserQuantity();state.sharedLastQuantity=&scene.aux.LastLocalGeyserQuantity();std::vector<std::uint32_t> owners;std::size_t negative{};auto hooks=scene.GeyserHooks();hooks.playerBounds=[&](auto owner) { owners.push_back(owner);if (owner==1) scene.aux.FirstLocalGeyserQuantity()=-7;else { CHECK(owner==5);scene.aux.LastLocalGeyserQuantity()=-8; }return FortRandomRectangle{16,16,48,48}; };hooks.negativeQuantity=[&] { ++negative; };
    RawLocalGeyser body(scene.pool,scene.hash,scene.spots,scene.types,scene.frames,scene.players,scene.recent,state,std::move(hooks));body.Generate();CHECK((owners==std::vector<std::uint32_t>{1,5}) && negative==2 && scene.recent.count==0);scene.aux.ConstructionBlocked()=0x80000000;scene.aux.ConstructionDisplaceGenus()=0x12345678;scene.aux.EmptyGeyserType()=85;scene.aux.ResetForGeneralWorld();CHECK(scene.aux.FirstLocalGeyserQuantity()==-7 && scene.aux.LastLocalGeyserQuantity()==-8 && scene.aux.ConstructionBlocked()==0x80000000 && scene.aux.ConstructionDisplaceGenus()==0x12345678 && scene.aux.EmptyGeyserType()==85);
}
