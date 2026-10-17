// CarrierArrival 제한 Native 입력을 같은 풀의 기존 HP/내용물/Graph/검색 몸체에 연결한다.
#pragma once
#include "RawSceneSupport.h"
#include "o/RawCarrierArrival.h"
#include "o/RawDamageablePostDestroy.h"

namespace netstorm::test::carrierarrival {
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
// 선택하지 않은 큰 포획/전달/경제/프로토콜/공간 효과가 실행되면 명시 예외로 드러낸다.
template<class Result,class... Arguments> std::function<Result(Arguments...)> ArrivalForbidden() {
    return [](Arguments...)->Result { throw std::logic_error("Carrier 도착의 미선택 큰 하위 효과"); };
}
// 결과 계산 없이 Native 입력 JSON의 정수 필드만 읽는다.
inline std::map<std::string,std::string> ArrivalInput(const std::string& json) {
    std::map<std::string,std::string> value;
    // 공백 없는 현재 fixture 형식의 모든 필드를 보존한다.
    for (const auto& field:Split(std::string_view(json).substr(1,json.size()-2),',')) { const auto colon=field.find(':');value.emplace(field.substr(1,colon-2),field.substr(colon+1)); }
    return value;
}
// 같은 풀의 기존 helper를 직접 공급하며 actual Host 걷기와 독립 Native의 Path 입력 경계를 구분한다.
struct ArrivalScene {
    OriginalEdition edition; // 현재 Native 입력 판본이다.
    std::map<std::string,std::string> input; // 원본 입력을 계산 없이 보관한다.
    SidPool pool; // 모든 부모/후보가 같은 실제 client free list를 사용한다.
    SquidHash hash; // Native와 같은 현재 네 단계 버킷이다.
    std::vector<RiftTypeRecord> types; // 현재 genus/발자국/HP metadata다.
    std::vector<RiftTypeFrames> frames; // 기존 getter가 참조하는 같은 타입 수의 코드 목록이다.
    std::vector<std::uint8_t> spots; // Native의 격리된 같은 지면 입력이다.
    std::vector<std::string> effects; // actual 부모가 요청한 큰 회피 경계의 인자 기록이다.
    GameRandom random; // 패치 SP getter의 실제 저장소를 구성하는 기존 난수 처리기다.
    ScrambledSpStore store; // HP getter는 SP를 읽지 않지만 실제 Reward 생성 계약을 유지한다.
    SquidPostPopState book; // 기존 실제 HP/Graph 모듈이 공유하는 현재 장부다.
    SquidRewardState rewardState; // 기존 HP 모드/디버그 입력이다.
    ContainedFinderState containedState; // 기존 실제 내용물 타입/권한/커서 상태다.
    PathCleanupState cleanupState; // 기존 ReturnPlaceAt/NearbyType의 mutable 타입 상태다.
    PlayerPlacementAnchorState anchorState; // 기존 GraphFor의 현재 활성/sentinel 전역이다.
    SquidPostPopList additional; // 기존 anchor가 참조하는 별도 빈 후보 목록이다.
    ContainedCreateState createState; // 실제 contained 생성기의 현재6/82 타입 전역이다.
    DamageablePostDestroyState postState; // 공통 GenusAt getter를 가진 기존 모듈의 상태다.
    CarrierArrivalState state; // 부모 도착의 현재 권한/타입/Player AI 존재 입력이다.
    RawPathRuntime runtime; // 현재 Path lookup 경계가 공급하는 실제 판본 레이아웃 입력이다.
    std::function<RawPathRuntime*(Sid)> actualPath; // 별도 소비자는 같은 실제 Host의 현재 Path lookup으로 전수 입력 경계를 교체한다.
    std::unique_ptr<SquidReward> reward; // 같은 실제 HP helper다.
    std::unique_ptr<RawPriestState> priest; // 같은 실제 signed HP/지면 불가 helper다.
    std::unique_ptr<RawCarrierPreDestroy> carrier; // 같은 실제 contained finder다.
    std::unique_ptr<RawPathCleanup> cleanup; // 같은 실제 귀환장소/가까운타입 조회다.
    std::unique_ptr<RawPlayerPlacementAnchor> anchor; // 같은 실제 GraphFor다.
    std::unique_ptr<RawContainedCreate> create; // 큰 포획 선택 전 기존 완전한 내용물 생성 body다.
    std::unique_ptr<RawSquidNeighbors> footprint; // 기존 actual Center/Footprint helper다.
    std::unique_ptr<RawDamageablePostDestroy> post; // 이미 복원한 공통 GenusAt body를 공유한다.
    std::unique_ptr<RawCarrierArrival> arrival; // 새 실제 부모 도착 본문이다.
    // 입력에 맞는 실제 슬롯과 기존 helper를 구성하고 Native 결과를 재계산하지 않는다.
    ArrivalScene(OriginalEdition selected,const std::string& json,std::function<bool(std::uint32_t)> controlled={}):edition(selected),input(ArrivalInput(json)),pool(selected,24000,true),types(selected==OriginalEdition::Patch1078 ? 188 : 171),frames(types.size(),RiftTypeFrames({})),spots(65536),store(random,[] { return 1U; }),runtime(selected,true) {
        // 기록된 부모50/후보60 번호를 실제 client free list에서 확보한다.
        for (std::uint16_t sid=5;sid<=60;++sid) { CHECK(pool.Allocate(2)==Sid{sid});auto raw=pool.AllocatedBytes(Sid{sid});std::fill(raw.begin(),raw.end(),std::uint8_t{}); }
        Add(Sid{50},158,20.75F,21.9F,1);const auto scene=Value("scene");
        if (scene>=1 && scene<=4) Add(Sid{60},scene==2 ? 84 : 150,20.2F,scene==4 ? 23.2F : 21.3F,1);else if (scene==5) Add(Sid{60},165,23.75F,24.9F,3);
        const bool patch=edition==OriginalEdition::Patch1078;auto raw=pool.AllocatedBytes(Sid{50});Put(raw,26,200,patch ? 4 : 2);raw[patch ? 34 : 32]=static_cast<std::uint8_t>(Value("owner"));types[158].maxHitPoints=200;types[158].flags2=Value("genus");
        if (scene>=1 && scene<=4) types[scene==2 ? 84 : 150].flags2=std::array<std::uint32_t,5>{{0,0x10000,0,0x4200,0x200}}[scene];else if (scene==5) types[165].flags2=0x400000;spots[22*256+21]=6;spots[21*256+20]=6;state.boss=Value("boss")!=0;state.controlledOwners[Value("owner")]=Value("controlled")!=0;cleanupState.boss=state.boss;containedState.boss=state.boss;runtime.SetMoveAwayDepth(static_cast<std::int32_t>(Value("depth")));runtime.SetCommandFields(Value("helper"),0,0,{0,0});
        reward=std::make_unique<SquidReward>(pool,types,book,rewardState,patch ? &store : nullptr);priest=std::make_unique<RawPriestState>(pool,*reward,types,spots);CarrierPreDestroyHooks carrierHooks;
        carrierHooks.validateDamageable=ArrivalForbidden<void,Sid,std::uint32_t>();carrierHooks.damageablePre=ArrivalForbidden<void,Sid,std::uint32_t>();carrierHooks.dependentFound=ArrivalForbidden<void>();carrier=std::make_unique<RawCarrierPreDestroy>(pool,containedState,std::move(carrierHooks));PathCleanupHooks cleanupHooks;
        cleanupHooks.reserveShared=ArrivalForbidden<bool>();cleanupHooks.addShared=ArrivalForbidden<void,Sid,std::uint32_t,float>();cleanupHooks.contained=ArrivalForbidden<Sid,Sid,std::uint32_t,std::uint32_t,std::uint32_t>();cleanupHooks.graphFor=ArrivalForbidden<std::uint32_t,Sid>();cleanupHooks.ritual=ArrivalForbidden<void,Sid,Sid,std::uint32_t>();cleanupHooks.refund=ArrivalForbidden<void,std::uint32_t,std::uint32_t>();cleanupHooks.destroy=ArrivalForbidden<void,Sid,std::uint32_t>();cleanup=std::make_unique<RawPathCleanup>(pool,hash,types,cleanupState,std::move(cleanupHooks));anchor=std::make_unique<RawPlayerPlacementAnchor>(pool,types,hash.Entries(0),book,additional,containedState,anchorState);ContainedCreateHooks createHooks;
        createHooks.create=ArrivalForbidden<Sid,std::uint32_t,std::uint32_t>();createHooks.notifySurface=ArrivalForbidden<void,Sid>();createHooks.attach=ArrivalForbidden<void,Sid,Sid,std::uint32_t>();create=std::make_unique<RawContainedCreate>(pool,createState,std::move(createHooks));footprint=std::make_unique<RawSquidNeighbors>(pool,hash,spots,types,frames);DamageablePostDestroyHooks postHooks;
        postHooks.dropDude=ArrivalForbidden<void,Sid,std::int32_t,bool>();postHooks.dropIsland=ArrivalForbidden<void,float,float,std::uint32_t,std::uint32_t>();postHooks.regular=ArrivalForbidden<void,Sid,std::uint32_t,float,std::uint32_t>();postHooks.hasBridgeFall=ArrivalForbidden<bool,Sid>();postHooks.notifySurface=ArrivalForbidden<void,Sid>();postHooks.displayUpdate=ArrivalForbidden<void,Sid>();postHooks.destroy=ArrivalForbidden<void,Sid,std::uint32_t>();postHooks.base=ArrivalForbidden<void,Sid,std::uint32_t>();post=std::make_unique<RawDamageablePostDestroy>(pool,hash,types,frames,*reward,*footprint,postState,std::move(postHooks));CarrierArrivalHooks hooks;
        hooks.currentControlled=std::move(controlled); // 실제 현재 Player 조회를 검증할 때만 값형 표 대신 공급한다.
        hooks.path=[this](Sid sid) { CHECK(sid==Sid{50});return actualPath ? actualPath(sid) : Value("path") ? &runtime : nullptr; };hooks.genusAt=[this](float x,float y,std::uint32_t genus) { return post->GenusAt(x,y,genus); };hooks.cdPriestPickupPriest=ArrivalForbidden<bool>();hooks.pickup=ArrivalForbidden<void,Sid>();
        hooks.escape=[this](Sid sid,float delay,std::uint32_t event) { CHECK(sid==Sid{50});effects.push_back("E:50:"+std::to_string(std::bit_cast<std::uint32_t>(delay))+":"+std::to_string(event)+":"+std::to_string(Value("reply")));return Value("reply"); };hooks.chooseReturn=ArrivalForbidden<Sid,Sid>();hooks.chooseGeyser=ArrivalForbidden<Sid,Sid>();hooks.goReturn=ArrivalForbidden<void,Sid,Sid>();hooks.goGeyser=ArrivalForbidden<void,Sid,Sid>();hooks.minedPacket=ArrivalForbidden<void,Sid>();hooks.destroy=ArrivalForbidden<void,Sid,std::uint32_t>();hooks.cdAddSp=ArrivalForbidden<void,std::uint32_t,std::int32_t>();hooks.cdOwnerPacket=ArrivalForbidden<void,std::uint32_t>();hooks.playAt=ArrivalForbidden<void,float,float,std::string_view,std::uint32_t>();hooks.robust=ArrivalForbidden<bool,Sid>();arrival=std::make_unique<RawCarrierArrival>(pool,hash,types,*carrier,*cleanup,*priest,*anchor,*create,*footprint,state,std::move(hooks));
    }
    // 입력값만 읽고 예상 분기/반환을 계산하지 않는다.
    std::uint32_t Value(const char* key) const { return Number(input.at(key)); }
    // Native 입력과 같은 합성 raw/발자국/버킷 등록이다. 전체 actual Pop 완료 근거가 아니다.
    void Add(Sid sid,std::uint32_t type,float x,float y,int width) { auto raw=pool.AllocatedBytes(sid);Put(raw,0,0x11018000);raw[10]=static_cast<std::uint8_t>(type);Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y));types[type].footX=types[type].footY=width;auto& head=hash.Bucket(1,x,y);Put(raw,4,head,2);head=sid.value; }
    // 요청된 큰 회피 효과의 순서/비트/반환만 읽는다.
    std::string Effects() const { std::string value;for (const auto& effect:effects) { if (!value.empty()) value+=';';value+=effect; }return value.empty() ? "-" : value; }
};
}
