// 서버 요청 전체의 독립 PE 관찰과 실제 MayPlace→확정→통지→정리/배치 연결을 검사한다.
#include "ConstructionNoticeSupport.h"
#include "o/RawConstructionRequest.h"
#include <cmath>

namespace {
using namespace netstorm::test::construction;
// 판본별 독립 관찰 행 수와 분석 입력의 다른 타입/표면 객체 번호다.
constexpr std::size_t kPatchRows=816,kCdRows=688;
constexpr std::uint32_t kOtherType=102;
constexpr Sid kSurface{80};
// 도구/PE 없이 저장된 실제 출력만 읽는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_CONSTRUCTIONREQUEST_FIXTURE).rows;return rows; }
// 같은 저장소를 유지하며 각 행의 타입·프레임·후보·spot과 기록 경계를 다시 공급한다.
struct RequestScene {
    SidPool pool;
    SquidHash hash;
    std::vector<RiftTypeRecord> types;
    std::vector<PriestPlainCanonType> frames;
    std::vector<std::uint8_t> spots;
    ConstructionRequestState state;
    bool patch;
    std::vector<std::uint32_t> input;
    std::string events;
    std::size_t confirmIndex{};
    // 자료의 주소는 행 사이에 바뀌지 않는다.
    explicit RequestScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),
        frames(types.size()),spots(RawConstructionRequest::kSpotCount),patch(edition==OriginalEdition::Patch1078) {}
    // 원본 실행기와 같은 타입/전역/spot 입력을 만든다.
    void Prepare(const std::vector<std::uint32_t>& v) {
        input=v;types.assign(types.size(),RiftTypeRecord{});frames.assign(frames.size(),PriestPlainCanonType{});hash.Reset();events.clear();confirmIndex=0;
        types[v[0]].footX=static_cast<int>(v[7]);types[v[0]].footY=static_cast<int>(v[8]);frames[v[0]]={InputFrames(v[9]),std::bit_cast<int>(v[10])};
        types[v[0]].flags1=v[15];types[v[0]].flags2=v[16];types[kOtherType].footX=static_cast<int>(v[11]);types[kOtherType].footY=static_cast<int>(v[12]);
        types[kTypeIsland].footX=static_cast<int>(v[24]);types[kTypeIsland].footY=static_cast<int>(v[25]);
        if (v[0]!=kTypeNoIsland) types[kTypeNoIsland].footX=types[kTypeNoIsland].footY=1;
        state=ConstructionRequestState{};state.editor=v[13]!=0;state.localPlayer=v[14];state.displaceGenus=v[17];
        std::fill(spots.begin(),spots.end(),static_cast<std::uint8_t>(v[22]==8 ? 0 : v[22]));if (v[22]==8) spots[22*256+21]=2;
        auto zero=InputRaw(pool,Sid{});std::fill(zero.begin(),zero.end(),std::uint8_t{});
    }
    // 후보 하나를 지정한 단계의 버킷 머리에 잇는다.
    void Node(const std::vector<std::uint32_t>& n) {
        auto raw=InputRaw(pool,Sid{static_cast<std::uint16_t>(n[0])});std::fill(raw.begin(),raw.end(),std::uint8_t{});
        Put(raw,0,kVtable);raw[10]=static_cast<std::uint8_t>(n[1]);raw[patch ? 40 : 35]=static_cast<std::uint8_t>(n[2]);
        Put(raw,14,n[3]);Put(raw,18,n[4]);raw[patch ? 34 : 32]=static_cast<std::uint8_t>(n[5]);Put(raw,patch ? 36 : 34,n[6],patch ? 4 : 1);
        auto& head=hash.Bucket(static_cast<int>(n[7]),Float(n[3]),Float(n[4]));Put(raw,4,head,2);head=static_cast<std::uint16_t>(n[0]);
    }
    // buried 객체는 finder에서는 건너뛰고 표면 조회에서는 abstract로 보인다.
    void Surface() {
        if (!input[23]) return;
        const int x=static_cast<int>(static_cast<double>(Float(input[3]))+0.9998999834060669),y=static_cast<int>(static_cast<double>(Float(input[4]))+0.9998999834060669);
        if (x<0 || y<0 || x>=256 || y>=256) { InputRaw(pool,Sid{})[patch ? 40 : 35]=1;return; }
        auto raw=InputRaw(pool,kSurface);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=static_cast<std::uint8_t>(kTypeNoIsland);raw[patch ? 40 : 35]=9;
        Put(raw,14,Bits(static_cast<float>(x)));Put(raw,18,Bits(static_cast<float>(y)));auto& head=hash.Cell(0,x,y);Put(raw,4,head,2);head=kSurface.value;
    }
    // 사건의 인자를 순서대로 문자열에 잇는다.
    void Append(char letter,std::initializer_list<std::uint32_t> values) {
        if (!events.empty()) events+=';';events+=letter;
        // DWORD 좌표/인자 비트를 10진수로 보존한다.
        for (const auto value:values) events+=':'+std::to_string(value);
    }
    // 실제 원본에서 대체한 네 효과만 기록한다. scan은 대체하지 않은 finder Begin의 관찰이다.
    ConstructionRequestHooks Hooks() {
        ConstructionRequestHooks hooks;
        hooks.mayPlace=[this](const CanonPlacementQuery& q) { Append('M',{q.type,q.argument,Bits(q.x),Bits(q.y),q.flags,q.owner,q.mode,input[18]});return input[18]!=0; };
        hooks.displace=[this](float x,float y,std::uint32_t type,std::uint32_t player) { Append('D',{Bits(x),Bits(y),type,player,input[19]});return input[19]!=0; };
        hooks.displaceCd=[this](float x,float y,std::uint32_t type) { Append('V',{Bits(x),Bits(y),type}); };
        hooks.confirm=[this](const ConstructionConfirmRequest& q) {
            const auto result=input[confirmIndex++==0 ? 20 : 21];Append('C',{q.type,Bits(q.x),Bits(q.y),q.player,q.argument,q.direction,q.flags,q.timeLow,q.timeHigh,q.quality,result});return result!=0;
        };
        hooks.cancel=[this](float x,float y,std::uint32_t flags,std::uint32_t player) { Append('K',{Bits(x),Bits(y),flags,player}); };
        hooks.scan=[this](const SquidSearchArea& a) { Append('A',{std::bit_cast<std::uint32_t>(a.left),std::bit_cast<std::uint32_t>(a.top),std::bit_cast<std::uint32_t>(a.right),std::bit_cast<std::uint32_t>(a.bottom)}); };
        return hooks;
    }
};
// 판본의 관찰을 재생하고 반환·효과 인자·순서를 그대로 비교한다.
void Replay(std::string_view edition) {
    RequestScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;CHECK(Fixture().size()==kPatchRows+2*kCdRows);
    // 각 행에서 현재 입력을 다시 준비한다. 기대값은 C++로 계산하지 않는다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==5);const auto v=Inputs(row[1]);CHECK(v.size()==31);scene.Prepare(v);
        if (row[2]!="-") {
            // 원본 입력 순서대로 후보를 등록한다.
            for (const auto& entry:Split(row[2],';')) { const auto n=Inputs(entry);CHECK(n.size()==8);scene.Node(n); }
        }
        scene.Surface();RawConstructionRequest request(scene.pool,scene.hash,scene.types,scene.frames,scene.spots,scene.state,scene.Hooks());
        const bool result=request.Request({v[0],Float(v[3]),Float(v[4]),v[5],v[1],v[2],v[26],v[27],v[28],v[29]});const bool same=result==(Number(row[3])!=0) && scene.events==row[4];CHECK(same);
        if (!same) { std::printf("서버 건설 요청 %s 행 %zu 불일치\n 입력 %s | %s\n 반환 %d/%s\n 기대 %s\n 실제 %s\n",std::string(edition).c_str(),count,row[1].c_str(),row[2].c_str(),result,row[3].c_str(),row[4].c_str(),scene.events.c_str());break; }++count;
    }
    CHECK(count==(edition=="originals" ? kPatchRows : kCdRows));
}
}
// 10.78 요청 전체의 독립 원본 관찰을 비교한다.
TEST_CASE(ConstructionRequest_ReplaysOriginals) { Replay("originals"); }
// CD의 void 밀어내기와 좌표 helper 차이도 비교한다.
TEST_CASE(ConstructionRequest_ReplaysCd) { Replay("originalCD"); }
// 추가 10.37 PE 관찰을 별도 기대값으로 비교한다.
TEST_CASE(ConstructionRequest_ReplaysExtra1037) { Replay("original1037"); }

// 누락 경계·짧은 지도·유효하지 않은 입력은 첫 효과 전에 거부한다.
TEST_CASE(ConstructionRequest_RejectsInvalidInputsBeforeEffects) {
    RequestScene scene(OriginalEdition::Patch1078);std::size_t effects=0;ConstructionRequestHooks hooks;
    hooks.mayPlace=[&](const CanonPlacementQuery&) { ++effects;return true; };hooks.confirm=[](const ConstructionConfirmRequest&) { return true; };
    hooks.displace=[](float,float,std::uint32_t,std::uint32_t) { return true; };hooks.cancel=[](float,float,std::uint32_t,std::uint32_t) {};
    CHECK(Throws([&] { RawConstructionRequest bad(scene.pool,scene.hash,scene.types,scene.frames,scene.spots,scene.state,{}); }));
    CHECK(Throws([&] { RawConstructionRequest bad(scene.pool,scene.hash,scene.types,scene.frames,std::span<const std::uint8_t>{},scene.state,hooks); }));
    RawConstructionRequest request(scene.pool,scene.hash,scene.types,scene.frames,scene.spots,scene.state,hooks);
    CHECK(Throws([&] { request.Request({300,20,21,3}); }));CHECK(Throws([&] { request.Request({kTypePlain,Float(0x7fc01234),21,3}); }));CHECK(effects==0);
}

// 확정 경계가 바꾼 현재 타입 플래그로 실패 정리 여부를 다시 판정한다.
TEST_CASE(ConstructionRequest_ReadsCurrentFlagsAfterConfirm) {
    RequestScene scene(OriginalEdition::Patch1078);scene.types[kTypePlain].footX=scene.types[kTypePlain].footY=1;scene.frames[kTypePlain]={InputFrames(0),3};
    std::fill(scene.spots.begin(),scene.spots.end(),std::uint8_t{2});std::size_t cancelled=0;ConstructionRequestHooks hooks;
    hooks.mayPlace=[](const CanonPlacementQuery&) { return true; };hooks.displace=[](float,float,std::uint32_t,std::uint32_t) { return true; };
    hooks.confirm=[&](const ConstructionConfirmRequest&) { scene.types[kTypePlain].flags1^=0x400;return false; };hooks.cancel=[&](float,float,std::uint32_t,std::uint32_t) { ++cancelled; };
    RawConstructionRequest request(scene.pool,scene.hash,scene.types,scene.frames,scene.spots,scene.state,hooks);
    CHECK(!request.Request({kTypePlain,20,21,3}) && cancelled==1);CHECK(!request.Request({kTypePlain,20,21,3}) && cancelled==1);
}

// 실제 전체 MayPlace가 거부하면 예측 조각을 보존하고, 허용하면 서버 확정/통지/정리/배치로 아홉 조각을 교체한다.
TEST_CASE(ConstructionRequest_ConnectsPipelineConfirmAndNotice) {
    // 두 판본에서 같은 저장소를 공유한 원본 모듈 연결을 검사한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,kCapacity,true);SquidHash hash;
        std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<PriestPlainCanonType> frames(types.size());
        types[kTypeNoIsland].footX=types[kTypeNoIsland].footY=1;types[kTypeNoIsland].group=1;frames[kTypeNoIsland]={InputFrames(0),3};
        std::vector<SquidDisplayShape> shapes(types.size());std::vector<PriestTypeHotspot> hotspots(types.size());
        shapes[kTypeNoIsland]={static_cast<int>(frames[kTypeNoIsland].frames.Codes().size()),true,
            std::vector<SquidDisplayFrame>(frames[kTypeNoIsland].frames.Codes().size(),{16,11,8,5,0,0})};
        std::vector<std::uint8_t> spots(65536);std::vector<std::uint16_t> islands(65536);std::vector<CanonPlacementIslandRegion> regions(128,{0,1});
        SquidFactory factory(pool,types);SquidPostPopState books;SquidOwnerMode ownerMode;SquidOwner owner(pool,types,books,ownerMode);
        SquidRewardState rewardState;GameRandom rng;ScrambledSpStore store(rng,[] { return 1U; });
        SquidReward reward(pool,types,books,rewardState,patch ? &store : nullptr);ConstructionPlaceState placeState;placeState.localPlayer=1;
        ConstructionPlaceHooks placeHooks;
        // 관찰하지 않는 금액/문구 효과다. 실제 소유자 지정과 SID 수신은 어댑터가 연결한다.
        placeHooks.take=[](std::uint32_t,Sid sid) { return sid; };placeHooks.setOwner=[](Sid,std::uint32_t) {};placeHooks.notifySurface=[](Sid) {};
        placeHooks.sendMoney=[](std::uint32_t,const ConstructionMoneyNotice&) {};placeHooks.storedSp=[](std::uint32_t) { return 0.0F; };
        placeHooks.addLocalSp=[](float) {};placeHooks.addPlayerSp=[](std::uint32_t,float) {};
        placeHooks.format=[](std::string_view,std::string_view) { return std::string{}; };placeHooks.tellOthers=[](std::uint32_t,std::string_view) {};
        placeHooks.tellPlayer=[](std::uint32_t,std::string_view) {};placeHooks.tell=[](std::string_view) {};placeHooks.sendReject=[](std::uint32_t,const ConstructionRejectNotice&) {};
        // Pop의 미복원 공간 효과는 좌표/void/해시 삽입 경계로 제한한다.
        placeHooks.pop=[&](Sid sid,float x,float y,std::uint32_t) {
            auto raw=pool.AllocatedBytes(sid);Put(raw,14,Bits(x));Put(raw,18,Bits(y));raw[11]=static_cast<std::uint8_t>(raw[11]&~4);
            auto& head=hash.Bucket(0,x,y);Put(raw,4,head,2);head=sid.value;
        };
        RawConstructionPlace place(pool,types,frames,placeState,reward,MakeConstructionPlaceHooks(pool,factory,owner,reward,patch ? &store : nullptr,placeState,placeHooks));
        std::vector<std::uint16_t> predicted,removed;
        // 클라이언트 영역의 아홉 void 객체를 예측 배치의 실제 입력으로 만든다.
        for (int i=0;i<9;++i) predicted.push_back(factory.Create(kTypeNoIsland,2).value);
        place.Place({kTypeNoIsland,20.25F,21.5F,0,0,1,predicted,1,0});
        ConstructionClearState clearState;
        // 가상 삭제 경계는 해당 칸의 머리를 빼고 SID를 반납한다. 실제 삭제 전체를 실행했다고 주장하지 않는다.
        RawConstructionClear clear(pool,hash,types,frames,clearState,{[&](Sid sid,std::uint32_t flags) {
            CHECK(flags==RawConstructionClear::kSamePieceFlag);removed.push_back(sid.value);auto raw=pool.AllocatedBytes(sid);
            auto& head=hash.Bucket(0,Float(Get(raw,14)),Float(Get(raw,18)));CHECK(head==sid.value);head=static_cast<std::uint16_t>(Get(raw,4,2));
            raw[11]=static_cast<std::uint8_t>(raw[11]|4);pool.Release(sid);
        },{}});
        ConstructionNoticeState noticeState;noticeState.localPlayer=1;std::size_t refreshed=0,broadcasts=0;
        auto noticeHooks=SilentNoticeHooks();noticeHooks.refresh=[&] { ++refreshed; };
        RawConstructionNotice notice(pool,types,noticeState,MakeConstructionNoticeHooks(pool,clear,place,noticeHooks));
        ConstructionConfirmState confirmState;ConstructionConfirmHooks confirmHooks;
        // 사제 불필요 타입이므로 조회/기록 효과는 사용되지 않는다. 통지 방송 횟수는 직접 처리와 분리해 센다.
        confirmHooks.findBuilder=[](std::uint32_t,std::uint32_t,float,float) { return Sid{}; };confirmHooks.broadcast=[&](const ConstructionNotice&) { ++broadcasts; };
        confirmHooks.recorderActive=[] { return false; };confirmHooks.record=[](Sid) {};
        RawConstructionConfirm confirm(pool,types,frames,confirmState,MakeConstructionConfirmHooks(pool,factory,MakeConstructionNoticeConfirmHooks(pool,notice,confirmHooks)));
        CanonPlacementPipelineState placementState;placementState.placement.localPlayer=1;placementState.permission.graphReady=1;
        // 최종 소유 관계는 동일 소유자도 동맹 표를 직접 읽는다. 실제 초기값처럼 1번의 자기 관계를 켠다.
        placementState.permission.alliances[10]=1;
        SquidPostPopList additional;ContainedFinderState contained;
        RawCanonPlacementPipeline pipeline(pool,hash,{types,frames,shapes,hotspots,spots,islands,regions},books,additional,contained,placementState);
        ConstructionRequestState requestState;requestState.localPlayer=1;ConstructionRequestHooks requestHooks;
        // 이 장면의 타입은 밀어내기/받침을 사용하지 않는다. 두 경계에 도달하면 검사 실패다.
        requestHooks.displace=[](float,float,std::uint32_t,std::uint32_t) { CHECK(false);return false; };
        requestHooks.displaceCd=[](float,float,std::uint32_t) { CHECK(false); };requestHooks.cancel=[](float,float,std::uint32_t,std::uint32_t) { CHECK(false); };
        RawConstructionRequest request(pool,hash,types,frames,spots,requestState,MakeConstructionRequestHooks(pool,pipeline,confirm,requestHooks));
        CHECK(&pipeline.Pool()==&pool);SidPool other(edition,kCapacity,false);CHECK(Throws([&] { MakeConstructionRequestHooks(other,pipeline,confirm,requestHooks); }));
        // 지면이 없는 첫 요청은 실제 최종 지형 정책이 거부한다. SID 생성·방송·삭제·배치는 일어나지 않는다.
        CHECK(!request.Request({kTypeNoIsland,20.25F,21.5F,1}) && broadcasts==0 && refreshed==0 && removed.empty());
        // 원본의 지면 허용 전역만 켠다. 강제 MayPlace는 0이며 모양/미리보기/충돌/지형/최종 관계 몸체는 모두 실행한다.
        placementState.relations.bypassGroundPermission=1;
        CHECK(request.Request({kTypeNoIsland,20.25F,21.5F,1}) && broadcasts==1 && refreshed==1 && removed==predicted);
        CHECK(noticeState.history[0].x==20.25F && noticeState.history[0].y==21.5F && placementState.placement.forcePlacement==0);
        RawSquidFinder finder(pool,hash,types);std::size_t found=0;
        // 조각을 교체한 전체 범위에는 서버 SID·올바른 소유자·비abstract 조각 아홉 개만 남아야 한다.
        for (Sid current=finder.Begin({20,21,23,24});current.value;current=finder.Next()) {
            ++found;const auto raw=pool.Slot(current);CHECK(current.value>=pool.Layout().serverFirst && raw[10]==kTypeNoIsland && raw[patch ? 34 : 32]==1 && !(raw[patch ? 40 : 35]&1));
        }
        CHECK(found==9);
    }
}
