// 사제 postPop 전체 wrapper의 원본 관찰과 실제 회복/장부/보호막 모듈 결합을 검사한다.
#include "RawSceneSupport.h"
#include "o/RawPriestPostPopTail.h"
#include "o/SquidFactory.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 두 정밀도의 관찰이 같은 판본별 독립 입력 수다.
constexpr std::size_t kRows=2048;
// 저장한 원본 관찰을 한 번만 읽는다. 일반 검사는 원본 PE/Python 없이 실행한다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto data=LoadFixture(NETSTORM_PRIESTPOSTPOPTAIL_FIXTURE);return data.rows;
}
// 원본에 공급한 raw 사제·타입·지면·프레임·외부 효과만 준비하는 콘솔 장면이다.
struct TailScene {
    SidPool pool;
    GameRandom random;
    ScrambledSpStore store;
    SquidPostPopState book;
    SquidRewardState hpMode;
    PriestPostPopState priests;
    std::vector<RiftTypeRecord> types;
    std::vector<RiftTypeFrames> frames;
    std::vector<std::uint8_t> spots;
    std::unique_ptr<SquidReward> hp;
    std::vector<std::string> events;
    bool patch{},found{},allocated{};
    std::uint32_t change{};
    // 같은 클라이언트 풀의 입력 SID 50까지 확보하고 고정 크기 표를 만든다.
    explicit TailScene(OriginalEdition edition):pool(edition,kCapacity,false),store(random,[] { return 1U; }),
        types(edition==OriginalEdition::Patch1078 ? 188 : 171),frames(types.size(),RiftTypeFrames({})),spots(65536),
        patch(edition==OriginalEdition::Patch1078) {
        // 실제 풀 할당으로 예약 번호/자산 번호 정책을 유지한다.
        for (std::uint16_t id=5;id<=kSource.value;++id) CHECK(pool.Allocate(2)==Sid{id});
    }
    // 한 행의 입력만 복원한다. 존재/낙하/보호막 기대 분기는 계산하지 않는다.
    void Prepare(const std::vector<std::string>& row) {
        CHECK(row.size()==14);events.clear();found=Number(row[7])!=0;allocated=Number(row[8])!=0;change=Number(row[9]);
        types[kPriestType].maxHitPoints=200;types[kPriestType].flags1=0x69012;types[kPriestType].flags2=0x210000;
        hp=std::make_unique<SquidReward>(pool,types,book,hpMode,patch ? &store : nullptr);
        frames[kPriestType]=RiftTypeFrames({{static_cast<std::uint8_t>(Number(row[6])),80,1,0}});
        std::fill(spots.begin(),spots.end(),std::uint8_t{});spots[21*256+20]=static_cast<std::uint8_t>(Number(row[4]));
        spots[22*256+21]=static_cast<std::uint8_t>(Number(row[5]));
        priests.priests={{60,50,70,80},0};
        // 원본의 전체 소유자 칸 입력을 같은 순서로 공급한다.
        for (std::size_t i=0;i<priests.ownerState.size();++i) priests.ownerState[i]=0xabc000+static_cast<std::uint32_t>(i);
        auto raw=pool.AllocatedBytes(kSource);std::fill(raw.begin(),raw.end(),std::uint8_t{});
        Put(raw,0,patch ? kPatchPriestVtable : kCdPriestVtable);raw[10]=kPriestType;raw[patch ? 34 : 32]=1;
        raw[patch ? 40 : 35]=static_cast<std::uint8_t>(Number(row[2]));Put(raw,12,1,2);
        Put(raw,14,std::bit_cast<std::uint32_t>(20.75f));Put(raw,18,std::bit_cast<std::uint32_t>(21.9f));
        Put(raw,26,Number(row[3]),patch ? 4 : 2);
    }
    // Regular 검색/확보/부착 경계의 인자와 목록 등록 시점을 관찰한다.
    PriestPostPopHooks PrefixHooks() {
        return {[this](Sid sid,std::uint32_t event) {
            events.push_back("F:"+std::to_string(sid.value)+':'+std::to_string(event)+':'+std::to_string(priests.priests.count));return found;
        },[this] { events.push_back("A:40");return allocated; },[this](Sid sid,std::uint32_t event,float payload) {
            events.push_back("R:"+std::to_string(sid.value)+':'+std::to_string(event)+':'+std::to_string(std::bit_cast<std::uint32_t>(payload)));
        }};
    }
    // Carrier 경계의 명시 입력 변이를 공급한다. 이후 실제 상태 조회가 현재 값을 읽는다.
    void Carrier(Sid sid,std::uint32_t flags) {
        events.push_back("C:"+std::to_string(sid.value)+':'+std::to_string(flags));auto raw=pool.AllocatedBytes(sid);
        if (change==1) raw[patch ? 40 : 35]=1;
        else if (change==2 || change==3) {
            raw[patch ? 40 : 35]=static_cast<std::uint8_t>(change==2 ? 32 : 0);Put(raw,26,150,patch ? 4 : 2);
            Put(raw,14,std::bit_cast<std::uint32_t>(30.75f));Put(raw,18,std::bit_cast<std::uint32_t>(31.9f));
            spots[31*256+30]=static_cast<std::uint8_t>(change==2 ? 0 : 6);spots[32*256+31]=6;
        }
    }
    // 낙하/보호막 경계 호출 순서를 기록한다. 낙하 후 extra 변경은 입력 변이다.
    PriestPostPopTailHooks TailHooks() {
        return {[this](Sid sid) {
            events.push_back("D:"+std::to_string(sid.value));if (change==4) pool.AllocatedBytes(sid)[patch ? 40 : 35]=1;
        },[this](Sid sid) { events.push_back("S:"+std::to_string(sid.value)); },
          [this](Sid sid) { events.push_back("X:"+std::to_string(sid.value)); }};
    }
    // 원본과 같은 사건 구분자를 사용한다.
    std::string Events() const {
        std::string text;
        // 효과를 발생한 순서로 합친다.
        for (const auto& value:events) { if (!text.empty()) text+=';';text+=value; }
        return text.empty() ? "-" : text;
    }
    // 목록의 비활성 꼬리까지 보존했는지 비교하기 위한 문자열이다.
    std::string List() const {
        std::string text=std::to_string(priests.priests.count)+':';
        // 전체 확보 저장소를 읽는다.
        for (std::size_t i=0;i<priests.priests.entries.size();++i) { if (i) text+=',';text+=std::to_string(priests.priests.entries[i]); }
        return text;
    }
    // 칸 0을 포함한 모든 소유자 상태를 관찰한다.
    std::string Owners() const {
        std::string text;
        // 소유자 번호 순서대로 읽는다.
        for (auto value:priests.ownerState) { if (!text.empty()) text+=',';text+=std::to_string(value); }
        return text;
    }
};
// 각 판본의 전체 출력·raw 슬롯·목록·소유자 칸을 자신의 원본 관찰과 비교한다.
void Replay(const char* edition) {
    TailScene scene(std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);
    CHECK(Fixture().size()==kRows*3);std::size_t count=0;int failures=0;
    // 서로 다른 분기/효과 입력마다 실제 모듈을 새로 연결한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;scene.Prepare(row);
        RawPriestPostPop prefix(scene.pool,*scene.hp,scene.priests,scene.PrefixHooks());CarrierPostPopState carrierMode;
        RawCarrierPostPop carrier(scene.pool,scene.types,carrierMode,{[](Sid) {},[](float,float,std::uint8_t) {},
            [](Sid,std::uint32_t) {},[&](Sid sid,std::uint32_t flags) { scene.Carrier(sid,flags); }});
        RawPriestState state(scene.pool,*scene.hp,scene.types,scene.spots);
        RawPriestPostPopTail tail(scene.pool,prefix,carrier,state,scene.frames,scene.spots,scene.TailHooks());tail.PostPop(kSource,Number(row[1]));
        const bool same=scene.Events()==row[10] && Hex(scene.pool.Slot(kSource))==row[11] && scene.List()==row[12] && scene.Owners()==row[13];
        CHECK(same);++count;
        if (!same) { std::printf("%s 사제 후반 행 %zu: 기대 %s / 실제 %s\n",edition,count,row[10].c_str(),scene.Events().c_str());if (++failures>=5) break; }
    }
    CHECK(count==kRows && failures==0 && !scene.store.Initialized());
}
}
// 패치의 전체 wrapper/실제 상태·거의 올림 지면·signed 방향을 대조한다.
TEST_CASE(PriestPostPopTail_ReplaysOriginals) { Replay("originals"); }
// CD의 inline 지면과 방향 9 bool helper를 대조한다.
TEST_CASE(PriestPostPopTail_ReplaysCd) { Replay("originalCD"); }
// 추가 10.37 PE의 독립 관찰도 재생한다.
TEST_CASE(PriestPostPopTail_ReplaysExtra1037) { Replay("original1037"); }

// 실제 회복 Form/Kernel·공통 장부·factory/owner·보호막 조회/삭제 요청을 한 흐름에 연결한다.
TEST_CASE(PriestPostPopTail_ComposesProcessBooksAndShieldLifetimeRequests) {
    // 두 원본 raw 배치에서 같은 클라이언트 흐름을 검사한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        TailScene scene(edition);scene.Prepare(Fixture().front());scene.types[167].constructorAddress=TypeConstructorAddress(edition,167);
        scene.types[167].footX=scene.types[167].footY=1;scene.types[kPriestType].cost=123.75f;
        SquidHash hash;SquidUnpop unpop(scene.pool,hash,scene.spots);RawSquidDestroy destroy(scene.pool,unpop,scene.types);
        Kernel kernel;SquidProcessState processMode;processMode.now=11.5;
        SquidProcessHost host(scene.pool,scene.types,kernel,destroy,processMode);
        RawPriestPostPop prefix(scene.pool,*scene.hp,scene.priests,MakePriestPostPopProcessHooks(host));
        SquidPostPop base(scene.pool,scene.types,scene.book);CarrierPostPopState carrierMode;
        RawCarrierPostPop carrier(scene.pool,scene.types,carrierMode,MakeCarrierPostPopHooks(scene.pool,base,[](Sid) {},[](float,float,std::uint8_t) {}));
        SquidFactory factory(scene.pool,scene.types);SquidOwnerMode ownerMode;SquidOwner owner(scene.pool,scene.types,scene.book,ownerMode);
        PriestForcefieldState fieldMode;RawPriestForcefield lookup(scene.pool,hash,scene.types,fieldMode);PriestShieldState shieldMode{1,1,0};
        Sid created;int pops=0,falls=0,deletes=0;
        auto effects=MakePriestShieldCreationHooks(scene.pool,factory,owner,{{},{},[&](Sid sid,float x,float y,std::uint32_t flags) {
            CHECK(flags==0);created=sid;++pops;auto raw=scene.pool.AllocatedBytes(sid);
            // 미복원 보호막 Pop 경계가 공간 등록 입력을 공급한다.
            Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y));raw[11]=0;
            auto& bucket=hash.Bucket(0,x,y);Put(raw,4,bucket,2);bucket=sid.value;
        },[] { return false; },[](std::string_view) { return 0U; },[](Sid,std::uint32_t,std::uint32_t,std::uint32_t) {},
          [](const PriestShieldNotice&) {},[](std::string_view) { return std::string{}; },[](std::string_view) {}});
        RawPriestShield shield(scene.pool,lookup,shieldMode,effects);RawPriestState state(scene.pool,*scene.hp,scene.types,scene.spots);
        auto hooks=MakePriestPostPopTailShieldHooks(scene.pool,shield,lookup,[&](Sid sid,std::uint32_t flags) {
            CHECK(sid==created && flags==0);++deletes;
            // 미복원 가상 삭제 경계가 등록 해제/반납 입력을 공급한다.
            hash.Bucket(0,20.75f,21.9f)=0;scene.pool.AllocatedBytes(sid)[11]=4;scene.pool.Release(sid);
        },[&](Sid sid) { CHECK(sid==kSource);++falls; });
        RawPriestPostPopTail tail(scene.pool,prefix,carrier,state,scene.frames,scene.spots,hooks);
        scene.book.localOwner=1;scene.book.depth=1;tail.PostPop(kSource,1);
        CHECK(kernel.Size()==1 && host.FindEvent(kSource,kPriestRegenEvent)!=nullptr && scene.priests.priests.count==1);
        CHECK(scene.book.depth==0 && scene.book.totalCost==123 && scene.book.globalCounts[kPriestType]==1);
        CHECK(lookup.Find(kSource)==created && falls==1 && pops==1);
        // 재등록 flag는 낙하 재진입을 막고 기존 보호막/회복을 중복 생성하지 않는다.
        scene.book.depth=1;tail.PostPop(kSource,0x800);CHECK(falls==1 && pops==1 && kernel.Size()==1);
        Put(scene.pool.AllocatedBytes(kSource),26,150,scene.patch ? 4 : 2);std::fill(scene.spots.begin(),scene.spots.end(),std::uint8_t{6});
        scene.book.depth=1;tail.PostPop(kSource,0);CHECK(deletes==1 && lookup.Find(kSource)==Sid{});
        scene.pool.AllocatedBytes(kSource)[scene.patch ? 40 : 35]=32;scene.book.depth=1;tail.PostPop(kSource,0x800);
        CHECK(pops==2 && falls==1 && lookup.Find(kSource)==created && kernel.Size()==1);
        host.Kill(*host.FindEvent(kSource,kPriestRegenEvent),0x10);CHECK(kernel.Size()==0);
    }
}

// 필수 경계·표 크기·다른 풀을 첫 접두 효과 전에 거부한다.
TEST_CASE(PriestPostPopTail_RejectsInvalidBindings) {
    TailScene scene(OriginalEdition::Patch1078);scene.Prepare(Fixture().front());
    RawPriestPostPop prefix(scene.pool,*scene.hp,scene.priests,scene.PrefixHooks());CarrierPostPopState mode;
    RawCarrierPostPop carrier(scene.pool,scene.types,mode,{[](Sid) {},[](float,float,std::uint8_t) {},[](Sid,std::uint32_t) {},[](Sid,std::uint32_t) {}});
    RawPriestState state(scene.pool,*scene.hp,scene.types,scene.spots);
    CHECK(Throws([&] { RawPriestPostPopTail invalid(scene.pool,prefix,carrier,state,scene.frames,scene.spots,{}); }));
    SidPool other(OriginalEdition::Patch1078,kCapacity,false);
    CHECK(Throws([&] { RawPriestPostPopTail invalid(other,prefix,carrier,state,scene.frames,scene.spots,scene.TailHooks()); }));
    CHECK(Throws([&] { RawPriestPostPopTail invalid(scene.pool,prefix,carrier,state,{},scene.spots,scene.TailHooks()); }));
    CHECK(Throws([&] { RawPriestPostPopTail invalid(scene.pool,prefix,carrier,state,scene.frames,{},scene.TailHooks()); }));
    CHECK(scene.events.empty() && scene.priests.priests.count==0);
}

// 낙하 flag는 지면/프레임 조회 자체를 생략하고 Carrier 후 extra 차단도 먼저 적용한다.
TEST_CASE(PriestPostPopTail_SkipsUnneededInvalidCoordinateAndFrameReads) {
    TailScene scene(OriginalEdition::Patch1078);scene.Prepare(Fixture().front());
    RawPriestPostPop prefix(scene.pool,*scene.hp,scene.priests,scene.PrefixHooks());CarrierPostPopState mode;
    RawCarrierPostPop carrier(scene.pool,scene.types,mode,{[](Sid) {},[](float,float,std::uint8_t) {},[](Sid,std::uint32_t) {},[](Sid,std::uint32_t) {}});
    RawPriestState state(scene.pool,*scene.hp,scene.types,scene.spots);
    RawPriestPostPopTail tail(scene.pool,prefix,carrier,state,scene.frames,scene.spots,scene.TailHooks());
    auto raw=scene.pool.AllocatedBytes(kSource);raw[40]=32;Put(raw,14,0x7fc12345);Put(raw,36,0xffffffff);
    tail.PostPop(kSource,0x800);CHECK(scene.Events()=="S:50");scene.events.clear();
    CHECK(Throws([&] { tail.PostPop(kSource,0); }));CHECK(scene.events.empty());
    raw[40]=8;tail.PostPop(kSource,0);CHECK(scene.events.empty());
}
