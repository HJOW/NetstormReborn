// 두 실제 PE의 Fort 객체/내용물/전체 순회를 저장 관찰과 실제 월드 어댑터로 검사한다.
#include "ConstructionSupport.h"
#include "o/RawFortObjectLoad.h"
#include "o/ContentFormHost.h"
#include "o/SquidPop.h"
#include "o/SquidOwner.h"
#include <memory>

namespace {
using namespace netstorm::test::construction;
// 고유 fixture 입력 열이다. 상태/자료 입력만 공급하며 기대 owner/frame/난수는 계산하지 않는다.
enum Field : std::size_t { Stored,FortTypeInput,Position,Flags1,Flags2,Version,DefaultOwner,OwnerInput,Special,Mission,Fort,Blocked,
    Quarter,Seed,FramesInput,TerrainInput,Mutation,OriginX,OriginY,ContentsInput,FrameInput,Qa,Qb,BridgeFrame,Legacy,Factory,ChaffMode,Slot };
// native 정상 factory/효과가 쓰는 가상 표 기록값과 입력 수다.
constexpr std::uint32_t kFortVtable=0x13000000;
constexpr std::size_t kCases=609;
// 정렬되지 않은 원본 HP 필드의 실제 저장 폭만 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width) {
    std::uint32_t result=0;
    // 낮은 바이트부터 지정 폭을 조립한다.
    for (std::size_t byte=0;byte<width;++byte) result|=static_cast<std::uint32_t>(raw[offset+byte])<<(8*byte);
    return result;
}
// 저장 관찰만 읽는다. 일반 CTest에서는 원본 게임/PE/Python을 실행하지 않는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_FORTOBJECTLOAD_FIXTURE).rows;return rows; }
// 고정 P/P·fallback/없는 방향·256 후보 입력 표다.
RiftTypeFrames Frames(std::uint32_t profile) {
    std::vector<FrameCode> codes;
    if (profile==1) codes={{'P','P',0,0},{'P','P',1,255},{'A','A',0,0}};
    if (profile==2) codes={{'P','A',0,0},{'P','A',1,0}};
    if (profile==3) codes={{'P','C',0,0},{'P','D',0,0}};
    if (profile==4) codes={{'A','A',0,0}};
    // 번호와 flags는 BYTE 입력이며 기대 선택 인덱스는 실제 PE fixture에서만 읽는다.
    if (profile==5) for (std::uint32_t index=0;index<300;++index) codes.push_back({'P','P',static_cast<std::uint8_t>(index),static_cast<std::uint8_t>(index)});
    return RiftTypeFrames(std::move(codes));
}
// 중첩 부모가 실제 ContentForm 타입6인 저장 목록 입력이다.
FortContentList Contents(std::uint32_t profile) {
    FortContent a;a.storedType=110;a.type=110;a.quantity=9;
    FortContent b;b.storedType=111;b.type=111;b.quantity=-3;b.listFlags=0x20;b.listFlagsStored=true;
    FortContent nested;nested.storedType=112;nested.type=112;nested.contents={b};
    if (profile==1) return {{a,b},true};if (profile==2) return {{nested},true};if (profile==3) return {{},false};return {};
}
// native packet에 쓴 데이터만 FortObject 모델로 공급한다. 현재 분기에 없는 optional 값은 사용되지 않는다.
FortObject Record(const std::vector<std::uint32_t>& v) {
    FortObject object;object.position=static_cast<std::uint8_t>(v[Position]);object.storedType=static_cast<std::uint8_t>(v[Stored]);object.type=static_cast<int>(v[FortTypeInput]);
    object.frame=static_cast<std::uint8_t>(v[FrameInput]);object.quantityByte=static_cast<std::uint8_t>(v[Qa]);
    object.quantityWord=std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(v[Qb]));object.bridgeFrame=static_cast<std::uint8_t>(v[BridgeFrame]);
    object.legacyState=static_cast<std::uint8_t>(v[Legacy]);object.factoryState=static_cast<std::uint8_t>(v[Factory]);object.owner=static_cast<std::uint8_t>(v[OwnerInput]);
    object.contents=Contents(v[ContentsInput]);return object;
}
// native 입력 경계와 같은 실제 pool/형식 자료를 공급하는 검사 장면이다.
struct FortLoadScene {
    // pool: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀. 입력 구성 또는 실행 결과 대조에 사용한다.
    SidPool pool;/* hash: 좌표별 객체 후보를 찾는 공간 해시. 입력 구성 또는 실행 결과 대조에 사용한다. */ SquidHash hash;/* types: 타입 번호로 속성·플래그를 조회하는 타입 표. 입력 구성 또는 실행 결과 대조에 사용한다. */ std::vector<RiftTypeRecord> types;/* frames: 타입별 프레임 자료. 입력 구성 또는 실행 결과 대조에 사용한다. */ std::vector<RiftTypeFrames> frames;
    // random: 원본 FUN_004558c0: 상태 0이면 0x0bad0bad로 바꾼 뒤 상태*0x10003+3의 상위 16비트를 limit로 나눈 나머지를 쓴다 (GameRandom).
    GameRandom random;
    std::uint32_t serializationVersion{}; // 독립원문입력의판본DWORD이며상태가같은주소를빌린다.
    FortObjectLoadState state{serializationVersion}; // 검사입력과실행뒤결과가같이읽고쓰는실제판본참조와로딩상태다.
    SquidPostPopState books; // 실제객체후처리장부이며실행결과대조에사용한다.
    SquidRewardState rewardState; // 보상계산이읽거나갱신하는판본별전역과AI지갑이다.
    // store: 패치판이 플레이어별 SP를 32개의 9칸 DWORD 배열에 흩뿌려 보관하는 방식이다 (std::unique_ptr<ScrambledSpStore>).
    std::unique_ptr<ScrambledSpStore> store;/* hp: 객체의 HP를 조회·변경하는 처리기 또는 HP 값. 입력 구성 또는 실행 결과 대조에 사용한다. */ std::unique_ptr<SquidReward> hp;/* v: V. 입력 구성 또는 실행 결과 대조에 사용한다. */ std::vector<std::uint32_t> v;
    // events: 발생한 이벤트 또는 이벤트 처리기. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<std::string> events;/* born: 생성시각. 입력 구성 또는 실행 결과 대조에 사용한다. */ /* children: 자식. 입력 구성 또는 실행 결과 대조에 사용한다. */ std::uint16_t born{},children{};/* patch: 10.78 패치판의 상태 또는 패치판 검사용 자료. 입력 구성 또는 실행 결과 대조에 사용한다. */ bool patch;
    // 합성 native factory가 쓰는 번호를 실제로 할당한다. 같은 pool의 raw 영역만 사용한다.
    FortLoadScene(OriginalEdition edition,std::vector<std::uint32_t> values):pool(edition,kCapacity,true),types(edition==OriginalEdition::Patch1078 ? 188 : 171),
        random(values[Seed]),v(std::move(values)),patch(edition==OriginalEdition::Patch1078) {
        // native 경계와 같은 고정 객체/Content 번호를 확보한다.
        for (std::uint16_t sid=5;sid<=240;++sid) CHECK(pool.Allocate(2)==Sid{sid});
        // 타입 번호마다 빈 프레임 입력을 만든다. 실제 코드 입력은 아래 필요한 타입에만 넣는다.
        for (std::size_t i=0;i<types.size();++i) frames.emplace_back(std::vector<FrameCode>{});
        // 실제 PE에 공급한 현재 타입·HP·코드를 같은 자료로 설정한다.
        for (auto type:{v[FortTypeInput],101U}) { types[type].flags1=v[Flags1];types[type].flags2=v[Flags2];types[type].maxHitPoints=803;types[type].footX=types[type].footY=1;frames[type]=Frames(v[FramesInput]); }
        types[164].flags1=0x10;types[164].maxHitPoints=803;types[110].flags1=0x1000;types[110].contentListFlags=0x110;
        types[111].flags1=0x2000;types[112].flags1=0x20000;types[112].contentListFlags=0x20;
        // 기존 Native 입력은 명시적으로 island97/stalag98을 제공한다. 실제 PE 기본94/95와 합치지 않는다.
        state.islandType=97;state.largeIslandType=98;
        state.version=v[Version];state.loadingDepth=3;state.chunksX=state.chunksY=2;state.special=v[Special]!=0;state.mission=v[Mission]!=0;
        state.fort=v[Fort]!=0;state.fortDestroyBlocked=v[Blocked]!=0;state.chaffMode=v[ChaffMode]!=0;rewardState.priestQuarterHp=v[Quarter]!=0;
        if (patch) store=std::make_unique<ScrambledSpStore>(random,[] { return 1U; });
        hp=std::make_unique<SquidReward>(pool,types,books,rewardState,store.get());
        if (v[TerrainInput]) {
            const Sid ground{50};auto raw=pool.AllocatedBytes(ground);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,kFortVtable);raw[10]=97;
            const auto x=v[OriginX]+(v[Position]>>4),y=v[OriginY]+(v[Position]&15);Put(raw,14,Bits(static_cast<float>(x)));Put(raw,18,Bits(static_cast<float>(y)));
            const auto frame=v[TerrainInput]==2 ? 0xffffffffU : 7U;if (patch) Put(raw,36,frame);else raw[34]=static_cast<std::uint8_t>(frame);
            types[97].flags2=2;types[97].footX=types[97].footY=1;hash.Cell(0,static_cast<int>(x),static_cast<int>(y))=ground.value;
        }
    }
    // 실제 경계 호출의 DWORD 순서를 기록한다.
    void Event(char letter,std::initializer_list<std::uint32_t> args) {
        std::string event(1,letter);
        // 값은 endian 변환/프레임 기대 계산 없이 그대로 직렬화한다.
        for (auto value:args) event+=':'+std::to_string(value);events.push_back(std::move(event));
    }
    // native와 같은 명시 효과만 대체한다. owner와 변경된 현재 타입/상태는 실제 raw/state에 쓴다.
    FortObjectLoadHooks Hooks() {
        FortObjectLoadHooks h;
        h.create=[&](std::uint32_t type,std::uint32_t flags) {
            const Sid sid{static_cast<std::uint16_t>(100+born++)};auto raw=pool.AllocatedBytes(sid);
            // 원본 factory 경계의 전체 패턴 보존을 비교한다.
            for (std::size_t i=0;i<raw.size();++i) raw[i]=static_cast<std::uint8_t>(i*37+11);
            Put(raw,0,kFortVtable);raw[10]=static_cast<std::uint8_t>(type);raw[11]=4;raw[6]=raw[7]=0;Event('C',{type,flags,sid.value});return sid;
        };
        h.notifySurface=[&](Sid sid) { Event('N',{sid.value}); };
        h.setState=[&](Sid sid,std::uint32_t value) { Event('S',{sid.value,value}); };
        h.setOwner=[&](Sid sid,std::uint32_t value) {
            Event('O',{sid.value,value});pool.AllocatedBytes(sid)[patch ? 34 : 32]=static_cast<std::uint8_t>(value);
            if (v[Mutation]==1) pool.AllocatedBytes(sid)[10]=101;if (v[Mutation]==2) state.fortDestroyBlocked=true;
        };
        h.hasHitPoints=[&](Sid sid) { return hp->HasHitPoints(sid); };h.maxHitPoints=[&](Sid sid) { return hp->MaxHitPoints(sid); };
        h.virtualCount=[&](Sid sid) { Event('Q',{sid.value});return std::bit_cast<std::int32_t>(v[Qa]); };
        h.pop=[&](Sid sid,float x,float y,std::uint32_t flags) { Event('P',{sid.value,Bits(x),Bits(y),flags});if (v[Mutation]==3) types[pool.Slot(sid)[10]].flags1=0; };
        h.destroy=[&](Sid sid,std::uint32_t flags) { Event('D',{sid.value,flags}); };
        h.createContent=[&](Sid parent,std::uint32_t type,std::uint32_t quantity,std::uint32_t listFlags,std::uint32_t flags,std::uint32_t quality) {
            const Sid sid{static_cast<std::uint16_t>(200+children++)};auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,kFortVtable);raw[10]=6;raw[11]=8;
            Event('I',{parent.value,type,quantity,listFlags,flags,quality,sid.value});return sid;
        };
        h.chunkRegion=[&](std::int32_t x,std::int32_t y) { Event('G',{static_cast<std::uint32_t>(x),static_cast<std::uint32_t>(y)});return static_cast<std::int8_t>((x+y*2)%3); };
        h.territoryAllowed=[&](std::int8_t region,std::uint32_t owner,std::int32_t territory) {
            Event('A',{static_cast<std::uint32_t>(region),owner,static_cast<std::uint32_t>(territory)});return (region+territory+v[Qa])%2==0;
        };
        h.loadingDepthChanged=[&](std::uint32_t depth) { CHECK(depth==state.loadingDepth); }; // 실제 공유 깊이 전달값을 검증한다.
        h.finishOwner=[&](std::uint32_t owner) { Event('F',{owner}); };return h;
    }
    // 실제 관찰 문자열을 저장된 기대 형식으로 묶는다.
    std::string Events() const { std::string out;for (const auto& e:events) { if (!out.empty()) out+=';';out+=e; }return out.empty() ? "-" : out; }
    // 객체 root stride 전체를 native 기록과 대조한다. 가상 Pop/contained 경계의 내부 몸체는 별도 통합 검사다.
    std::string Raws() const { std::string out;for (std::uint16_t i=0;i<born;++i) { if (!out.empty()) out+=';';out+=Hex(pool.Slot(Sid{static_cast<std::uint16_t>(100+i)})); }return out.empty() ? "-" : out; }
};
// whole template fixture의 저장 청크 입력만 제공한다. 선택 결과/위치는 실제 Load가 계산한다.
FortTemplate TemplateInput(const std::vector<std::uint32_t>& v) {
    FortTemplate fort;auto object=Record(v);object.position=0x21;
    // 원본 실행기에 준 두 Terr/Chaff는 각각 저장 청크 네 개다.
    for (auto* section:{&fort.territories[0],&fort.territories[1],&fort.chaff}) { section->present=true;section->version=2;section->chunks.assign(4,FortChunk{{object}}); }
    return fort;
}
// 동일 입력의 모든 효과/전체 raw/현재 난수/깊이·scope를 실제 PE 관찰과 비교한다.
void Replay(std::string_view edition) {
    std::size_t count=0;CHECK(Fixture().size()==2*kCases);
    // 한 행마다 실제 자료/풀을 새로 만들어 callback 변경이 다음 행에 남지 않는다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==8);FortLoadScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,Inputs(row[2]));
        RawFortObjectLoad loader(scene.pool,scene.hash,scene.types,scene.frames,scene.random,scene.state,scene.Hooks());
        if (row[1]=="O") loader.Object(Record(scene.v),std::bit_cast<std::int32_t>(scene.v[OriginX]),std::bit_cast<std::int32_t>(scene.v[OriginY]),scene.v[DefaultOwner]);
        else loader.Load(TemplateInput(scene.v),static_cast<std::int32_t>(scene.v[Slot]),0,20);
        const auto state=std::to_string(scene.state.version)+','+std::to_string(scene.state.loadingDepth)+','+std::to_string(scene.state.scope)+','+std::to_string(scene.state.fortDestroyBlocked);
        const bool same=scene.Events()==row[4] && scene.Raws()==row[5] && std::to_string(scene.random.State())==row[6] && state==row[7];
        CHECK(same);if (!same) { std::printf("Fort actual %s %s 행%zu 불일치\n",row[0].c_str(),row[1].c_str(),count);break; }++count;
    }
    CHECK(count==kCases);
}
}
// 패치판의 실제 Fort 객체/내용물/전체 순회 관찰이다.
TEST_CASE(FortObjectLoad_ReplaysPatch) { Replay("originals"); }
// CD판의 BYTE 프레임/WORD HP·원본 소유자/전체 순회를 별도로 대조한다.
TEST_CASE(FortObjectLoad_ReplaysCd) { Replay("originalCD"); }

// 실제 factory/owner/HP 조회/Pop9→ContentForm 연결과 현재 hash/부모 알림을 검사한다.
TEST_CASE(FortObjectLoad_ActualFactoryOwnerHpPopAndContent) {
    // 두 판본 모두 실제 일반 Pop/서버 SID 생성·ContentForm 정렬 부착을 실행한다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        auto values=Inputs(Fixture().front()[2]);values[Version]=2;values[Flags1]=0x23030;values[Flags2]=0;values[ContentsInput]=1;values[Mission]=1;
        FortLoadScene scene(edition,values);SquidFactory factory(scene.pool,scene.types);SquidOwnerMode mode;mode.battle=true;SquidOwner owner(scene.pool,scene.types,scene.books,mode);
        std::vector<std::uint8_t> spots(65536);SquidPop pop(scene.pool,scene.hash,spots);std::vector<Sid> notices;std::size_t transmits=0;
        ContentFormHost content(scene.pool,scene.types,[&](Sid parent,Sid form,std::uint32_t reason) { CHECK(reason==1 && !(scene.pool.Slot(parent)[11]&4));notices.push_back(form); },
            [&](Sid,std::uint32_t) { ++transmits; });
        ContainedCreateState containedState;RawContainedCreate contained(scene.pool,containedState,content.BindCreate(factory,ContainedCreateHooks{{},[&](Sid sid) { scene.Event('N',{sid.value}); },{}}));
        auto hooks=MakeFortObjectLoadHooks(scene.pool,factory,*scene.hp,contained,scene.Hooks());hooks.setOwner=[&](Sid sid,std::uint32_t player) { owner.Set(sid,player); };
        hooks.pop=[&](Sid sid,float x,float y,std::uint32_t flags) { CHECK(flags==9);CHECK(pop.Pop(sid,scene.types[scene.pool.Slot(sid)[10]],1,1,x,y,flags)==RawPopResult::Registered); };
        RawFortObjectLoad loader(scene.pool,scene.hash,scene.types,scene.frames,scene.random,scene.state,hooks);const auto sid=loader.Object(Record(values),16,32,2);
        CHECK(sid.value==scene.pool.Layout().serverFirst && scene.pool.Slot(sid)[scene.patch ? 34 : 32]==2);
        CHECK(Read(scene.pool.Slot(sid),26,scene.patch ? 4 : 2)==803 && notices.size()==2 && transmits==0);
        CHECK(scene.hash.Cell(1,15,17)==sid.value); // 현재 위치(30,35)의 실제1단계 해시 머리다.
        CHECK(notices.size()==2 && SquidForms(scene.pool).Parent(notices[0])==sid && SquidForms(scene.pool).Parent(notices[1])==sid);
    }
}

// 누락 필드/실제 효과·다른 풀 어댑터 및 범위 오류를 임의 기본값으로 통과시키지 않는다.
TEST_CASE(FortObjectLoad_RejectsMissingFieldsEffectsAndForeignPools) {
    FortLoadScene scene(OriginalEdition::Patch1078,Inputs(Fixture().front()[2]));auto hooks=scene.Hooks();hooks.pop={};
    CHECK(Throws([&] { RawFortObjectLoad bad(scene.pool,scene.hash,scene.types,scene.frames,scene.random,scene.state,hooks); }));
    RawFortObjectLoad loader(scene.pool,scene.hash,scene.types,scene.frames,scene.random,scene.state,scene.Hooks());auto record=Record(scene.v);
    scene.types[scene.v[FortTypeInput]].flags1=0x20;record.frame.reset();CHECK(Throws([&] { loader.Object(record,16,32,1); }));
    CHECK(Throws([&] { loader.Load({},1,0,0); }));CHECK(scene.state.loadingDepth==3);
    SidPool foreign(OriginalEdition::Patch1078,kCapacity,true);SquidFactory factory(foreign,scene.types);ContainedCreateState state;
    RawContainedCreate contained(scene.pool,state,ContainedCreateHooks{[&](std::uint32_t,std::uint32_t) { return Sid{5}; },[&](Sid sid) { scene.Event('N',{sid.value}); },[&](Sid,Sid,std::uint32_t) { scene.Event('N',{5}); }});
    CHECK(Throws([&] { MakeFortObjectLoadHooks(scene.pool,factory,*scene.hp,contained,scene.Hooks()); }));
}
