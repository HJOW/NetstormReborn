// 로컬 건설 전체의 독립 PE 관찰과 Form 값 캡처·현재 타입 플래그·잘못된 입력의 거부를 검사한다.
#include "ConstructionSupport.h"
#include "o/RawConstructionLocal.h"

namespace {
using namespace netstorm::test::construction;
// 분석기의 미리보기/새 조각 번호와 관찰 행 수다. 기대 출력은 저장 fixture에서만 읽는다.
constexpr Sid kPreview{50};
constexpr std::uint16_t kBorn=100;
// 판본별 저장 독립 원본 관찰 수다.
constexpr std::size_t kPatchRows=558,kCdRows=446;
// Python이나 원본 PE 없이 저장된 관찰을 한 번 읽는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_CONSTRUCTIONLOCAL_FIXTURE).rows;return rows; }
// 비교에 필요한 입력과 명시 경계만 갖는 장면이다.
struct LocalScene {
    SidPool pool;
    std::vector<RiftTypeRecord> types;
    std::vector<PriestPlainCanonType> frames;
    std::vector<std::uint8_t> spots;
    ConstructionLocalState state;
    bool patch;
    std::vector<std::uint32_t> input;
    std::string events;
    std::uint16_t born{};
    // 행마다 자료 주소는 유지하고 내용만 다시 채운다.
    explicit LocalScene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),
        frames(types.size()),spots(65536),patch(edition==OriginalEdition::Patch1078) {}
    // 원본 실행기와 같은 합성 타입/프레임/Form/spot을 공급한다.
    void Prepare(const std::vector<std::uint32_t>& v) {
        input=v;types.assign(types.size(),RiftTypeRecord{});frames.assign(frames.size(),PriestPlainCanonType{});events.clear();born=0;
        types[v[0]].flags1=v[9];frames[v[0]]={InputFrames(v[13]),std::bit_cast<int>(v[14])};
        if (v[0]!=kTypeNoIsland) frames[kTypeNoIsland]={InputFrames(v[13]),3};
        types[kTypeIsland].footX=static_cast<int>(v[11]);types[kTypeIsland].footY=static_cast<int>(v[12]);state.localPlayer=v[5];
        std::fill(spots.begin(),spots.end(),static_cast<std::uint8_t>(v[10]==8 ? 0 : v[10]));
        if (v[10]==8) {
            const int x=static_cast<int>(static_cast<double>(Float(v[1]))+0.9998999834060669),y=static_cast<int>(static_cast<double>(Float(v[2]))+0.9998999834060669);
            if (x>=0 && x<256 && y>=0 && y<256) spots[static_cast<std::size_t>(y)*256+static_cast<std::size_t>(x)]=2;
        }
        auto raw=InputRaw(pool,kPreview);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,kVtable);Put(raw,18,v[0],2);Put(raw,20,v[6],2);raw[patch ? 30 : 26]=static_cast<std::uint8_t>(v[7]);
    }
    // 기록 사건 하나를 세미콜론으로 잇는다.
    void Append(char letter,std::initializer_list<std::uint32_t> values) {
        if (!events.empty()) events+=';';events+=letter;
        // 좌표 비트와 DWORD를 원본 기록처럼 십진수로 보존한다.
        for (const auto value:values) events+=':'+std::to_string(value);
    }
    // 원본에서 대체한 효과만 기록하고 같은 입력으로 raw/타입 변경을 공급한다.
    ConstructionLocalHooks Hooks() {
        ConstructionLocalHooks hooks;
        hooks.previewSource=[this](Sid sid) {
            Append('Q',{sid.value,input[8]});auto raw=InputRaw(pool,sid);
            if (input[16]) { Put(raw,20,input[6]^0x108,2);raw[patch ? 30 : 26]=static_cast<std::uint8_t>(input[7]^3); }return input[8];
        };
        hooks.destroyPreview=[this](Sid sid,std::uint32_t flags) { Append('D',{sid.value,flags});auto raw=InputRaw(pool,sid);std::fill(raw.begin(),raw.end(),std::uint8_t{}); };
        hooks.returnPiece=[this](std::uint32_t flags,std::uint32_t source,std::uint32_t quality) { Append('R',{flags,source,quality}); };
        hooks.create=[this](std::uint32_t type,std::uint32_t flags) { const Sid sid{static_cast<std::uint16_t>(kBorn+born++)};Append('C',{type,flags,sid.value});return sid; };
        hooks.place=[this](const ConstructionPlaceRequest& q) {
            Append('P',{q.type,Bits(q.x),Bits(q.y),q.argument,q.direction,q.player,static_cast<std::uint32_t>(q.sids.size()),q.abstract,q.quality});events+=':';
            // 목록은 원본의 조각 순서를 보존한다.
            for (std::size_t i=0;i<q.sids.size();++i) events+=(i ? "," : "")+std::to_string(q.sids[i]);
            if (q.sids.empty()) events+='-';if (q.type==input[0]) types[q.type].flags1^=input[15];
        };
        hooks.moveBuilder=[this](std::uint32_t player,Sid sid,std::uint32_t low,std::uint32_t high) { Append('B',{player,sid.value,low,high}); };
        hooks.send=[this](std::uint32_t target,const ConstructionLocalMessage& q) {
            std::array<std::uint8_t,21> bytes{};bytes[0]=q.type;Put(bytes,1,Bits(q.x));Put(bytes,5,Bits(q.y));Put(bytes,9,q.argument,2);
            bytes[11]=q.direction;bytes[12]=q.quality;Put(bytes,13,q.timeLow);Put(bytes,17,q.timeHigh);Append('S',{target});events+=':'+Hex(bytes);
        };
        return hooks;
    }
};
// 두 번째 실행기가 계산한 기대값 없이 저장된 원본 사건을 재생한다.
void Replay(std::string_view edition) {
    LocalScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;
    CHECK(Fixture().size()==kPatchRows+2*kCdRows);
    // 각 관찰 행에 동일한 입력만 공급해 실제 사건 순서/인자를 비교한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==3);const auto v=Inputs(row[1]);CHECK(v.size()==19);scene.Prepare(v);
        RawConstructionLocal local(scene.pool,scene.types,scene.frames,scene.spots,scene.state,scene.Hooks());
        local.Place({kPreview,Float(v[1]),Float(v[2]),v[3],v[4]});const bool same=scene.events==row[2] && scene.born==v[17]+v[18];CHECK(same);
        if (!same) { std::printf("로컬 건설 %s 행 %zu 불일치\n 입력 %s\n 기대 %s\n 실제 %s\n",std::string(edition).c_str(),count,row[1].c_str(),row[2].c_str(),scene.events.c_str());break; }++count;
    }
    CHECK(count==(edition=="originals" ? kPatchRows : kCdRows));
}
}
// 10.78 로컬 전체 관찰을 비교한다.
TEST_CASE(ConstructionLocal_ReplaysOriginals) { Replay("originals"); }
// CD의 품질 필드와 같은 요청 메시지 본문도 비교한다.
TEST_CASE(ConstructionLocal_ReplaysCd) { Replay("originalCD"); }
// 추가 10.37 PE를 독립된 관찰로 비교한다.
TEST_CASE(ConstructionLocal_ReplaysExtra1037) { Replay("original1037"); }

// 잘못된 자료/좌표는 첫 Form 효과 전에 거부하고 생성 번호 오류는 배치/전송 전에 거부한다.
TEST_CASE(ConstructionLocal_RejectsInvalidInputs) {
    LocalScene scene(OriginalEdition::Patch1078);auto v=Inputs(Fixture().front()[1]);scene.Prepare(v);auto hooks=scene.Hooks();
    CHECK(Throws([&] { RawConstructionLocal bad(scene.pool,scene.types,scene.frames,scene.spots,scene.state,{}); }));
    CHECK(Throws([&] { RawConstructionLocal bad(scene.pool,scene.types,scene.frames,{},scene.state,hooks); }));
    RawConstructionLocal local(scene.pool,scene.types,scene.frames,scene.spots,scene.state,hooks);
    CHECK(Throws([&] { local.Place({Sid{},20,21}); }));CHECK(Throws([&] { local.Place({kPreview,Float(0x7fc01234),21}); }));
    Put(InputRaw(scene.pool,kPreview),18,300,2);CHECK(Throws([&] { local.Place({kPreview,20,21}); }));CHECK(scene.events.empty());
    scene.Prepare(v);scene.types[v[0]].flags1=0x400;CHECK(Throws([&] { local.Place({kPreview,-3,21}); }));CHECK(scene.events.empty());
    scene.Prepare(v);hooks.create=[](std::uint32_t,std::uint32_t) { return Sid{}; };
    RawConstructionLocal failed(scene.pool,scene.types,scene.frames,scene.spots,scene.state,hooks);
    CHECK(Throws([&] { failed.Place({kPreview,20,21}); }));CHECK(scene.events.find("P:")==std::string::npos && scene.events.find("S:")==std::string::npos);
}

// 미리보기 삭제 뒤에도 캡처된 품질을 쓰며 배치 경계의 현재 사제 플래그를 다시 읽는다.
TEST_CASE(ConstructionLocal_CapturesFormValuesAndReadsCurrentFlags) {
    LocalScene scene(OriginalEdition::Patch1078);auto v=Inputs(Fixture().front()[1]);v[5]=0xffffff03;v[6]=8;v[7]=3;v[8]=0;v[9]=0;v[15]=0x8000;v[16]=0;scene.Prepare(v);
    ConstructionLocalRequest request{kPreview,20,21,0x12345678,0x100};auto hooks=scene.Hooks();const auto source=hooks.previewSource;
    // 원본의 스택 인자처럼 입력 사본을 유지한다. 효과가 호출자 요청을 바꿔도 배치/전송 인자는 처음 값이다.
    hooks.previewSource=[&](Sid sid) { request.argument=0;request.direction=0;return source(sid); };
    RawConstructionLocal local(scene.pool,scene.types,scene.frames,scene.spots,scene.state,hooks);local.Place(request);
    CHECK(scene.events.starts_with("Q:50:0;D:50:0;R:8:0:3"));CHECK(scene.events.find("B:4294967043:100:0:0")!=std::string::npos);
    CHECK(request.argument==0 && request.direction==0 && scene.events.find(":305419896:256:4294967043:")!=std::string::npos);
    const auto raw=scene.pool.Slot(kPreview);CHECK(Get(raw,18,2)==0 && raw[30]==0);CHECK(&local.Pool()==&scene.pool);
}

// 실제 factory/배치/소유자 연결로 본 SID를 먼저 확보하고 받침 아홉 칸부터 놓으며 다리 품질을 적용한다.
TEST_CASE(ConstructionLocal_ConnectsFactoryBasePlacementAndQuality) {
    // 두 판본의 실제 raw 폭과 품질 위치를 함께 검사한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,kCapacity,true);
        std::vector<RiftTypeRecord> types(patch ? 188 : 171);std::vector<PriestPlainCanonType> frames(types.size());
        types[kTypeBridge].flags1=0x8410;types[kTypeBridge].maxHitPoints=40;types[kTypeBridge].cost=250;
        types[kTypeBridge].footX=types[kTypeBridge].footY=types[kTypeNoIsland].footX=types[kTypeNoIsland].footY=1;
        types[kTypeIsland].footX=3;types[kTypeIsland].footY=2;frames[kTypeBridge]=frames[kTypeNoIsland]={InputFrames(0),3};
        SquidFactory factory(pool,types);SquidPostPopState books;SquidOwnerMode ownerMode;SquidOwner owner(pool,types,books,ownerMode);
        SquidRewardState rewardState;GameRandom rng;ScrambledSpStore store(rng,[] { return 1U; });
        SquidReward reward(pool,types,books,rewardState,patch ? &store : nullptr);ConstructionPlaceState placeState;placeState.localPlayer=1;
        ConstructionPlaceHooks placeHooks;std::vector<Sid> pops;
        // 부착된 실제 모듈 이외의 공간/메시지/문구 효과만 경계로 둔다.
        placeHooks.take=[](std::uint32_t,Sid sid) { return sid; };placeHooks.setOwner=[](Sid,std::uint32_t) {};
        placeHooks.notifySurface=[](Sid) {};placeHooks.sendMoney=[](std::uint32_t,const ConstructionMoneyNotice&) {};
        placeHooks.storedSp=[](std::uint32_t) { return 0.0F; };placeHooks.addLocalSp=[](float) {};placeHooks.addPlayerSp=[](std::uint32_t,float) {};
        placeHooks.format=[](std::string_view,std::string_view) { return std::string{}; };placeHooks.tellOthers=[](std::uint32_t,std::string_view) {};
        placeHooks.tellPlayer=[](std::uint32_t,std::string_view) {};placeHooks.tell=[](std::string_view) {};placeHooks.sendReject=[](std::uint32_t,const ConstructionRejectNotice&) {};
        placeHooks.pop=[&](Sid sid,float x,float y,std::uint32_t flags) {
            CHECK(flags==2);pops.push_back(sid);const auto raw=pool.Slot(sid);
            if (pops.size()==1) CHECK(x==18 && y==20);
            CHECK(raw[patch ? 40 : 35]&1);CHECK(raw[patch ? 34 : 32]==1);
        };
        RawConstructionPlace place(pool,types,frames,placeState,reward,MakeConstructionPlaceHooks(pool,factory,owner,reward,patch ? &store : nullptr,placeState,placeHooks));
        const Sid preview=factory.Create(kTypeBridge,2);auto raw=pool.AllocatedBytes(preview);Put(raw,18,kTypeBridge,2);Put(raw,20,8,2);raw[patch ? 30 : 26]=1;
        ConstructionLocalHooks hooks;std::size_t returned=0,moved=0,sent=0;Sid target{};
        hooks.previewSource=[](Sid) { return 77U; };hooks.destroyPreview=[&](Sid sid,std::uint32_t flags) { CHECK(flags==0);pool.Release(sid); };
        hooks.returnPiece=[&](std::uint32_t flags,std::uint32_t source,std::uint32_t quality) { CHECK(flags==8 && source==77 && quality==1 && pops.empty());++returned; };
        hooks.moveBuilder=[&](std::uint32_t player,Sid sid,std::uint32_t low,std::uint32_t high) { CHECK(player==1 && low==0 && high==0 && pops.size()>9);target=sid;++moved; };
        hooks.send=[&](std::uint32_t player,const ConstructionLocalMessage& message) { CHECK(player==9 && message.type==kTypeBridge && message.quality==1 && message.direction==2 && moved==1);++sent; };
        ConstructionLocalState state;state.localPlayer=1;std::vector<std::uint8_t> spots(65536);
        RawConstructionLocal local(pool,types,frames,spots,state,MakeConstructionLocalHooks(pool,factory,place,hooks));local.Place({preview,20,21,0,2});
        CHECK(returned==1 && moved==1 && sent==1 && target==pops[9]);
        // 공간 Pop은 경계지만 그 전에 쓴 타입·abstract·소유자·품질 프레임/수명/HP는 실제 raw다.
        for (std::size_t i=0;i<pops.size();++i) {
            const auto piece=pool.Slot(pops[i]);CHECK(pops[i].value<pool.Layout().serverFirst);CHECK(piece[10]==(i<9 ? kTypeNoIsland : kTypeBridge));
            if (i>=9) {
                CHECK(Get(piece,26,patch ? 4 : 2)==0);CHECK((Get(piece,12,2)&0x78)==32);
                const auto frame=Get(piece,patch ? 36 : 34,patch ? 4 : 1);CHECK(frames[kTypeBridge].frames.Codes()[frame].flags&0x20);
            }
        }
        SidPool other(edition,kCapacity,true);SquidFactory foreign(other,types);
        CHECK(Throws([&] { MakeConstructionLocalHooks(pool,foreign,place,hooks); }));
    }
}
