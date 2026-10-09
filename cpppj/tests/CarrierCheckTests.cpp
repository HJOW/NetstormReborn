// Carrier 지면 조회·사제 가상 낙하와 실제 표면 삭제→finder→공유 Regular 연결을 검사한다.
#include "RawSceneSupport.h"
#include "o/RawCarrierCheck.h"
#include "o/RawPriestFall.h"
#include "o/RawIslandLifecycle.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 각 판본의 독립 입력 수와 합성 가상 표 기록값이다. 합성 가상 표는 호스트 함수 주소가 아니다.
constexpr std::size_t kRows=676;
constexpr std::uint32_t kSyntheticVtable=0x11313000;
// CTest는 원본 PE/Python 없이 저장된 독립 관찰만 읽는다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto data=LoadFixture(NETSTORM_CARRIERCHECK_FIXTURE);return data.rows;
}
// 원본 입력과 동일한 raw 자산/표/외부 효과를 소유하는 콘솔 검사 장면이다.
struct CheckScene {
    SidPool pool;
    GameRandom random;
    ScrambledSpStore store;
    SquidPostPopState book;
    SquidRewardState hpMode;
    std::vector<RiftTypeRecord> types;
    std::vector<RiftTypeFrames> frames;
    std::vector<PriestFallFrames> choices;
    std::vector<std::uint8_t> spots;
    std::vector<std::uint16_t> surfaces;
    std::unique_ptr<SquidReward> hp;
    PriestFallState mode;
    std::vector<std::string> events;
    bool patch{},allocated{};
    std::uint32_t change{},reply{};
    // 실제 클라이언트 풀로 SID 50까지 확보하고 정상 사제/HP 입력을 구성한다.
    explicit CheckScene(OriginalEdition edition):pool(edition,kCapacity,false),store(random,[] { return 1U; }),
        types(edition==OriginalEdition::Patch1078 ? 188 : 171),frames(types.size(),RiftTypeFrames({})),
        choices(types.size()),spots(65536),surfaces(65536),patch(edition==OriginalEdition::Patch1078) {
        // 원본 번호 정책을 유지하며 예약 슬롯을 건너뛴다.
        for (std::uint16_t sid=5;sid<=kSource.value;++sid) CHECK(pool.Allocate(2)==Sid{sid});
        types[kPriestType].maxHitPoints=200;types[kPriestType].flags1=0x69012;types[kPriestType].flags2=0x210000;
        types[kPriestType].footX=types[kPriestType].footY=1;
        hp=std::make_unique<SquidReward>(pool,types,book,hpMode,patch ? &store : nullptr);
    }
    // 한 행의 raw/전역/지면 입력만 공급한다. 낙하 여부/반환은 생산 코드가 결정한다.
    void Prepare(const std::vector<std::string>& row) {
        CHECK(row.size()==19);events.clear();types[kPriestType].flags2=Number(row[2]);
        mode.authority=Number(row[6])!=0;allocated=Number(row[7])!=0;change=Number(row[8]);reply=Number(row[10]);
        frames[kPriestType]=RiftTypeFrames({{static_cast<std::uint8_t>(Number(row[5])),80,1,0}});
        std::fill(spots.begin(),spots.end(),std::uint8_t{});spots[21*256+20]=static_cast<std::uint8_t>(Number(row[3]));
        spots[22*256+21]=static_cast<std::uint8_t>(Number(row[4]));
        auto raw=pool.AllocatedBytes(kSource);std::fill(raw.begin(),raw.end(),std::uint8_t{});
        Put(raw,0,Number(row[9]) ? (patch ? kPatchPriestVtable : kCdPriestVtable) : kSyntheticVtable);
        raw[10]=kPriestType;raw[11]=static_cast<std::uint8_t>(Number(row[14]));raw[patch ? 34 : 32]=1;
        raw[patch ? 40 : 35]=static_cast<std::uint8_t>(Number(row[13]));Put(raw,26,20,patch ? 4 : 2);
        Put(raw,14,Number(row[11]));Put(raw,18,Number(row[12]));
    }
    // 보호막/확보/공간 효과에 공급한 권한/좌표 변이만 실행한다.
    void Change(std::string_view stage) {
        if ((stage=="shield" && change==1) || (stage=="new" && change==2)) mode.authority=!mode.authority;
        if (stage=="repop" && change==3) {
            auto raw=pool.AllocatedBytes(kSource);Put(raw,14,std::bit_cast<std::uint32_t>(30.75f));Put(raw,18,std::bit_cast<std::uint32_t>(31.9f));
        }
    }
    // 낙하 요청의 명시 효과 인자를 기록한다. 0x25b 처리 효과는 이 fixture에서 실행하지 않는다.
    PriestFallHooks FallHooks() {
        return {[this](Sid sid) { events.push_back("S:"+std::to_string(sid.value));Change("shield"); },
            [this] { events.push_back("A:40");Change("new");return allocated; },
            [this](Sid sid,std::uint32_t event,float payload) {
                events.push_back("R:"+std::to_string(sid.value)+':'+std::to_string(event)+':'+std::to_string(std::bit_cast<std::uint32_t>(payload)));
            },[this](Sid sid,std::uint32_t flags) { events.push_back("U:"+std::to_string(sid.value)+':'+std::to_string(flags)); },
            [this](Sid sid,std::uint32_t flags) { events.push_back("P:"+std::to_string(sid.value)+':'+std::to_string(flags));Change("repop"); },
            [this](Sid sid,float x,float y) {
                events.push_back("W:"+std::to_string(sid.value)+':'+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y)));
            },[](Sid,std::int32_t,std::uint32_t) { CHECK(false); },[](Sid,std::uint32_t,std::uint32_t) { CHECK(false); },
              [](Sid) { CHECK(false); }};
    }
    // 합성 가상 +0xc8의 DWORD 입력을 기록하고 0 아님으로 반환한다.
    bool OtherWalker(Sid sid) {
        events.push_back("V:"+std::to_string(sid.value)+':'+std::to_string(reply));return reply!=0;
    }
    // 원본과 같은 구분자로 모든 사건의 순서를 보존한다.
    std::string Events() const {
        std::string text;
        // 발생한 순서대로 합친다.
        for (const auto& event:events) { if (!text.empty()) text+=';';text+=event; }
        return text.empty() ? "-" : text;
    }
};
// 해당 판본의 결과·순서·raw 전체·현재 권한을 C++ 조회/낙하 wrapper와 대조한다.
void Replay(const char* edition) {
    CheckScene scene(std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);
    std::size_t count=0;CHECK(Fixture().size()==kRows*3);
    // 기대값을 계산하지 않고 실제 C++ 모듈을 각 입력에 연결한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;scene.Prepare(row);
        RawPriestState state(scene.pool,*scene.hp,scene.types,scene.spots);
        RawPriestFall fall(scene.pool,state,scene.frames,scene.choices,scene.surfaces,scene.mode,scene.FallHooks());
        RawCarrierCheck check(scene.pool,scene.types,scene.spots,MakePriestFallWalker(scene.pool,fall,[&](Sid sid) { return scene.OtherWalker(sid); }));
        const bool result=row[1]=="Try" ? fall.TryFall(kSource) : check.Check(kSource);
        const bool same=static_cast<unsigned>(result)==Number(row[15]) && scene.Events()==row[16] && Hex(scene.pool.Slot(kSource))==row[17]
            && scene.mode.authority==(Number(row[18])!=0);
        CHECK(same);++count;
        if (!same) { std::printf("%s Carrier 행 %zu: 기대 %s / 실제 %s\n",edition,count,row[16].c_str(),scene.Events().c_str());break; }
    }
    CHECK(count==kRows && !scene.store.Initialized());
}
}
// 패치의 전체 +0xcc/사제 +0xc8와 기존 실제 낙하 요청 관찰을 재생한다.
TEST_CASE(CarrierCheck_ReplaysOriginals) { Replay("originals"); }
// CD의 별도 상수 주소·BYTE 프레임·J bool helper 관찰을 재생한다.
TEST_CASE(CarrierCheck_ReplaysCd) { Replay("originalCD"); }
// 추가 10.37 PE의 독립 관찰도 재생한다.
TEST_CASE(CarrierCheck_ReplaysExtra1037) { Replay("original1037"); }

// 실제 표면 삭제/finder→사제 가상 낙하→공유 form/Kernel과 postPop/생성의 실제 조회를 합성한다.
TEST_CASE(CarrierCheck_ComposesSurfaceFallAndProcessScheduling) {
    // 두 원본 raw 배치에서 같은 실제 모듈 흐름을 검사한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        CheckScene scene(edition);scene.Prepare(Fixture().front());scene.types[kPriestType].flags2=0x210000;
        scene.change=0;scene.allocated=true;scene.mode.authority=true;
        auto raw=scene.pool.AllocatedBytes(kSource);Put(raw,0,scene.patch ? kPatchPriestVtable : kCdPriestVtable);
        raw[11]=0;raw[scene.patch ? 40 : 35]=32;
        Put(raw,14,std::bit_cast<std::uint32_t>(20.75f));Put(raw,18,std::bit_cast<std::uint32_t>(21.9f));
        scene.frames[kPriestType]=RiftTypeFrames({{65,80,1,0},{74,80,1,0}});scene.choices[kPriestType]={3,4};
        SquidHash hash;hash.Bucket(1,20.75f,21.9f)=kSource.value;
        SquidUnpop unpop(scene.pool,hash,scene.spots);RawSquidDestroy destroy(scene.pool,unpop,scene.types);
        Kernel kernel;SquidProcessState processMode;processMode.now=12.5;
        RawPriestState state(scene.pool,*scene.hp,scene.types,scene.spots);std::unique_ptr<RawPriestFall> fall;
        SquidProcessHost host(scene.pool,scene.types,kernel,destroy,processMode,[&](Sid sid,std::uint32_t event,std::uint32_t count,float payload) {
            return MakePriestFallHandler(scene.pool,*fall)(sid,event,count,payload);
        });
        auto hooks=scene.FallHooks();hooks.addSharedRegular=[&](Sid sid,std::uint32_t event,float payload) { host.AddSharedRegular(sid,event,payload); };
        hooks.setFrame=[&](Sid sid,std::int32_t frame,std::uint32_t flags) {
            CHECK(frame==3 && flags==0);Put(scene.pool.AllocatedBytes(sid),scene.patch ? 36 : 34,1,scene.patch ? 4 : 1);
        };
        hooks.advanceFrame=[](Sid,std::uint32_t steps,std::uint32_t flags) { CHECK(steps==1 && flags==0); };
        fall=std::make_unique<RawPriestFall>(scene.pool,state,scene.frames,scene.choices,scene.surfaces,scene.mode,std::move(hooks));
        auto walker=MakePriestFallWalker(scene.pool,*fall);
        const Sid surface=scene.pool.Allocate(2);auto land=scene.pool.AllocatedBytes(surface);std::fill(land.begin(),land.end(),std::uint8_t{});
        land[10]=kNoIslandType;Put(land,14,std::bit_cast<std::uint32_t>(20.75f));Put(land,18,std::bit_cast<std::uint32_t>(21.9f));
        scene.types[kNoIslandType].footX=scene.types[kNoIslandType].footY=1;
        RawSquidNeighbors neighbors(scene.pool,hash,scene.spots,scene.types,scene.frames);int baseCalls=0;
        RawIslandLifecycle islands(scene.pool,neighbors,{[](Sid,float,float) { CHECK(false); },
            [&](Sid sid) { CHECK(sid==kSource);CHECK(walker(sid)); },[](Sid,std::uint32_t) { CHECK(false); },
            [&](Sid sid,std::uint32_t flags) { CHECK(sid==surface && flags==0);++baseCalls; }});
        islands.SurfacePostDestroy(surface,0);CHECK(baseCalls==1 && kernel.Size()==1);
        // 첫 이벤트가 J를 쓰기 전에는 원본처럼 새 요청이 다시 예약된다.
        islands.SurfacePostDestroy(surface,0);CHECK(baseCalls==2 && kernel.Size()==2);
        kernel.RunFrame();auto* process=host.FindEvent(kSource,kPriestFallEvent);
        CHECK(process && process->Count()==1 && process->Payload()==0.1f);
        const auto before=scene.Events();islands.SurfacePostDestroy(surface,0);
        CHECK(baseCalls==3 && kernel.Size()==2 && scene.Events()==before && walker(kSource));
        // 실제 사제 genus는 +0xcc에서 제외된다. 비권한 postPop은 조회 뒤 공통 장부를 계속한다.
        RawCarrierCheck check(scene.pool,scene.types,scene.spots,walker);CHECK(!check.Check(kSource));
        SquidPostPop base(scene.pool,scene.types,scene.book);CarrierPostPopState carrierMode;carrierMode.boss=false;
        auto postHooks=MakeCarrierCheckPostPopHooks(scene.pool,check,MakeCarrierPostPopHooks(scene.pool,base,
            [](Sid) { CHECK(false); },[](float,float,std::uint8_t) { CHECK(false); }));
        RawCarrierPostPop carrier(scene.pool,scene.types,carrierMode,std::move(postHooks));
        scene.events.clear();scene.book.depth=1;carrier.PostPop(kSource,0);CHECK(scene.book.depth==0 && scene.events.empty() && kernel.Size()==2);
        // 실제 form 삭제로 부모 체인을 정리한다. 삭제한 포인터는 다시 읽지 않는다.
        while (auto* current=host.FindEvent(kSource,kPriestFallEvent)) host.Kill(*current,0x10);
        CHECK(kernel.Size()==0);
        // 나선 생성의 실제 genus 래퍼 뒤 +0xcc를 연결한다. 생성/소유자/Pop은 명시 입력 경계다.
        PriestSpawnState spawnMode;Sid born;
        auto spawnHooks=MakeCarrierCheckSpawnHooks(scene.pool,check,{[](const PriestPlacementQuery&) { return false; },
            [&](std::uint32_t type,std::uint32_t flags) {
                CHECK(type==kPriestType && flags==0);born=scene.pool.Allocate(2);auto created=scene.pool.AllocatedBytes(born);
                std::fill(created.begin(),created.end(),std::uint8_t{});created[10]=static_cast<std::uint8_t>(type);
                Put(created,0,scene.patch ? kPatchPriestVtable : kCdPriestVtable);return born;
            },[&](Sid sid,std::uint32_t owner) { CHECK(owner==1);scene.pool.AllocatedBytes(sid)[scene.patch ? 34 : 32]=1; },
            [&](Sid sid,float x,float y,std::uint32_t flags) {
                CHECK(flags==0);auto created=scene.pool.AllocatedBytes(sid);Put(created,14,std::bit_cast<std::uint32_t>(x));Put(created,18,std::bit_cast<std::uint32_t>(y));
            },[](Sid) { CHECK(false); },[](Sid) { CHECK(false); }});
        RawPriestSpawn spawn(scene.pool,scene.types,scene.spots,spawnMode,std::move(spawnHooks));
        spawn.Spawn(20.75f,21.9f,kPriestType,0x81);CHECK(born.value!=0 && !check.Check(born) && scene.events.empty());
    }
}

// 현재 표 재조회와 제외 조건의 좌표/프레임 생략, 명시 분배 및 같은 풀 연결 계약을 검사한다.
TEST_CASE(CarrierCheck_RejectsInvalidBindingsAndReadsCurrentTables) {
    CheckScene scene(OriginalEdition::Patch1078);scene.Prepare(Fixture().front());int calls=0;
    RawCarrierCheck check(scene.pool,scene.types,scene.spots,[&](Sid sid) { CHECK(sid==kSource);++calls;return true; });
    auto raw=scene.pool.AllocatedBytes(kSource);Put(raw,14,std::bit_cast<std::uint32_t>(20.75f));Put(raw,18,std::bit_cast<std::uint32_t>(21.9f));
    std::fill(scene.spots.begin(),scene.spots.end(),std::uint8_t{});scene.types[kPriestType].flags2=0;
    CHECK(check.Check(kSource) && calls==1);scene.spots[22*256+21]=4;CHECK(!check.Check(kSource) && calls==1);
    scene.spots[22*256+21]=0;CHECK(check.Check(kSource) && calls==2);
    scene.types[kPriestType].flags2=0x210000;Put(raw,14,0x7fc12345);Put(raw,18,0x7f800000);Put(raw,36,0xffffffff);
    CHECK(!check.Check(kSource) && calls==2);
    scene.types[kPriestType].flags2=0;CHECK(Throws([&] { check.Check(kSource); }));
    CHECK(Throws([&] { RawCarrierCheck invalid(scene.pool,{},scene.spots,[](Sid) { return true; }); }));
    CHECK(Throws([&] { RawCarrierCheck invalid(scene.pool,scene.types,scene.spots,{}); }));
    RawPriestState state(scene.pool,*scene.hp,scene.types,scene.spots);
    RawPriestFall fall(scene.pool,state,scene.frames,scene.choices,scene.surfaces,scene.mode,scene.FallHooks());
    SidPool other(OriginalEdition::Patch1078,kCapacity,false);
    CHECK(Throws([&] { MakeCarrierCheckPostPopHooks(other,check,{}); }));
    CHECK(Throws([&] { MakeCarrierCheckSpawnHooks(other,check,{}); }));
    CHECK(Throws([&] { MakePriestFallWalker(other,fall); }));
    Put(raw,0,kSyntheticVtable);auto walker=MakePriestFallWalker(scene.pool,fall);
    CHECK(Throws([&] { walker(kSource); }));
    auto fallback=MakePriestFallWalker(scene.pool,fall,[&](Sid sid) { CHECK(sid==kSource);return true; });
    CHECK(fallback(kSource) && scene.events.empty());
}
