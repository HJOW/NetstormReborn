// 세 PE의 종속 조회/삭제 관찰과 실제 건설 진행음·Kernel·form 삭제 연결을 검사한다.
#include "ConstructionNoticeSupport.h"
#include "o/RawContainedLookup.h"
#include "o/RawConstructionProcess.h"
#include "client/SoundProcess.h"
#include <cstdio>

namespace {
using namespace netstorm::test::construction;
using namespace netstorm::client;
// 원본 실행기와 같은 부모·가상 표 표식과 판본별 독립 입력 수다.
constexpr Sid kParent{50};
constexpr std::uint32_t kTable=0x12010000;
constexpr std::size_t kRows=805;
// fixture 입력 열의 순서다. 조회/삭제 기대값 계산에는 쓰지 않는다.
enum Input:std::size_t { Server,Pattern,Type,Ordinal,Remove,Mutate,InputCount };
// 저장 관찰만 읽는다. 기본 검사는 실제 PE/게임/Python을 실행하지 않는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_CONTAINED_FIXTURE).rows;return rows; }
// 실제 풀에 입력 체인과 가상 삭제의 응답만 공급하는 장면이다.
struct Scene {
    SidPool pool; // 서버 여부별로 다른 실제 권한을 사용한다.
    std::array<Sid,7> children; // 예약/client/서버/예측 경계의 입력 번호다.
    std::vector<std::uint32_t> input; // fixture의 입력 DWORD다.
    std::vector<std::string> events; // 실제 가상 삭제 요청의 순서다.
    RawContainedLookup lookup; // 타입/순번/권한/다음 링크를 처리하는 실제 몸체다.
    // 삭제 응답은 같은 풀의 지정 raw만 변경한다.
    Scene(OriginalEdition edition,bool server):pool(edition,32768,server),
        children{Sid{1},Sid{4},Sid{5},Sid{6},Sid{static_cast<std::uint16_t>(pool.Layout().serverFirst-1)},
            Sid{static_cast<std::uint16_t>(pool.Layout().serverFirst)},Sid{static_cast<std::uint16_t>(pool.Layout().predictableFirst+1)}},
        lookup(pool,[this](Sid sid,std::uint32_t flags) { Delete(sid,flags); }) {}
    // 입력 표식과 상태/타입만 설정하며 결과를 재계산하지 않는다.
    void Prepare(std::vector<std::uint32_t> values) {
        input=std::move(values);events.clear();CHECK(input.size()==InputCount && pool.IsServer()==(input[Server]!=0));
        // 부모와 모든 종속 슬롯의 주변 바이트를 실행기와 같은 값으로 채운다.
        for (std::size_t index=0;index<=children.size();++index) {
            const Sid sid=index ? children[index-1] : kParent;auto raw=InputRaw(pool,sid);
            // 표식 바이트는 whole-slot 보존 대조에 사용한다.
            for (std::size_t i=0;i<raw.size();++i) raw[i]=static_cast<std::uint8_t>(i*29+index*17+7);
            Put(raw,0,kTable);Put(raw,4,0,2);Put(raw,6,index==0 && input[Pattern]!=4 ? children[0].value : 0,2);
            raw[10]=index ? 47 : 100;raw[11]=index ? 8 : 0;
            if (!index) continue;
            Put(raw,4,index<children.size() ? children[index].value : 0,2);
            // 세 패턴의 타입/상태는 PE 실행에 공급한 입력과 동일하다.
            constexpr std::array<std::uint8_t,7> kTypes1{46,47,47,46,47,47,47},kTypes3{0,255,46,47,47,255,0};
            constexpr std::array<std::uint8_t,7> kStates2{2,10,8,1,4,0,8},kStates3{0,8,2,10,1,4,0};
            if (input[Pattern]==1) raw[10]=kTypes1[index-1];
            if (input[Pattern]==2) raw[11]=kStates2[index-1];
            if (input[Pattern]==3) { raw[10]=kTypes3[index-1];raw[11]=kStates3[index-1]; }
        }
    }
    // 명시 삭제 경계에서 호출만 기록하고 입력에 지정된 raw 변화를 공급한다.
    void Delete(Sid sid,std::uint32_t flags) {
        events.push_back("D:"+std::to_string(sid.value)+':'+std::to_string(flags));auto raw=InputRaw(pool,sid);
        if (input[Mutate]==1) Put(raw,4,children[3].value,2);
        if (input[Mutate]==2) Put(raw,4,0,2);
        if (input[Mutate]==3) InputRaw(pool,children[1])[11]=2;
        if (input[Mutate]==4) { const auto type=raw[10];std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=type;raw[11]=3; }
        if (input[Mutate]==5) Put(InputRaw(pool,kParent),6,0,2);
    }
    // 지정 raw 전체를 원본 실행기와 같은 순서로 합친다.
    std::string Dump() const {
        std::string text=Hex(pool.Slot(kParent));
        // 부모 다음에 지정 종속 7개를 붙인다.
        for (Sid sid:children) text+=':'+Hex(pool.Slot(sid));
        return text;
    }
    // 실제 가상 삭제의 사건 문자열이다.
    std::string Events() const {
        std::string text;
        // 호출 순서를 그대로 보존한다.
        for (const auto& event:events) { if (!text.empty()) text+=';';text+=event; }
        return text.empty() ? "-" : text;
    }
};
// 한 판본의 독립 관찰을 모두 재생한다. 반환/삭제 인자/순서/전체 raw를 비교한다.
void Replay(std::string_view edition) {
    const auto kind=edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072;
    Scene client(kind,false),server(kind,true);std::size_t count=0;CHECK(Fixture().size()==3*kRows);
    // 입력만 공급하고 원본이 저장한 기대 관찰을 읽는다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==5);auto values=Inputs(row[1]);auto& scene=values[Server] ? server : client;
        scene.Prepare(std::move(values));const auto result=scene.lookup.Find(kParent,scene.input[Type],scene.input[Ordinal],scene.input[Remove]!=0);
        const bool same=result.value==Number(row[2]) && scene.Events()==row[3] && scene.Dump()==row[4];CHECK(same);
        if (!same) { std::printf("contained %s 행 %zu 불일치\n",std::string(edition).c_str(),count);break; }++count;
    }
    CHECK(count==kRows);
}
}
// 10.78 전체 실제 명령 관찰을 재생한다.
TEST_CASE(ContainedLookup_PatchNative) { Replay("originals"); }
// CD 10.72 전체 실제 명령 관찰을 재생한다.
TEST_CASE(ContainedLookup_CdNative) { Replay("originalCD"); }
// 추가 10.37 PE를 별도로 읽은 관찰을 재생한다.
TEST_CASE(ContainedLookup_AdditionalNative) { Replay("original1037"); }

// 범위 밖/순환 링크와 삭제 뒤 손상된 next는 무한 반복이나 배열 밖 접근 전에 거부한다.
TEST_CASE(ContainedLookup_RejectsMalformedLinksIncludingAfterDelete) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,true);std::size_t calls=0;
    auto parent=InputRaw(pool,kParent);auto child=InputRaw(pool,Sid{5});Put(parent,6,5,2);child[10]=47;child[11]=8;Put(child,4,5,2);
    RawContainedLookup lookup(pool,[&](Sid,std::uint32_t) { ++calls;Put(child,4,kCapacity,2); });
    CHECK(Throws([&] { lookup.Find(kParent,46); }));
    CHECK(Throws([&] { lookup.Find(kParent,47,0,true); }) && calls==1);
    Put(child,4,0,2);CHECK(!lookup.Find(kParent,46).value);
    CHECK(Throws([&] { lookup.Find(Sid{static_cast<std::uint16_t>(kCapacity)},47); }));
}

// 삭제 경계 누락과 다른 월드의 어댑터 연결은 효과를 실행하기 전에 거부한다.
TEST_CASE(ContainedLookup_RejectsMissingDeleteAndForeignPool) {
    SidPool pool(OriginalEdition::Patch1078,kCapacity,true),other(OriginalEdition::Patch1078,kCapacity,true);
    CHECK(Throws([&] { RawContainedLookup invalid(pool,{}); }));
    RawContainedLookup lookup(pool,[](Sid,std::uint32_t) {});
    CHECK(Throws([&] { MakeContainedConstructionHooks(other,lookup,{}); }));
    CHECK(!Throws([&] { MakeContainedConstructionHooks(pool,lookup,{}); }));
}

// 실제 건설 완료는 첫 진행음 form을 지워 반복음을 멈추고 Kernel 슬롯을 제거한다. 다른 종속 프로세스는 남긴다.
TEST_CASE(ContainedLookup_ConstructionStopsActualSoundAndKeepsOtherProcess) {
    // 두 판본 모두 실제 건설/소리 프로세스·host·삭제·form Unpop·SID 반납을 거친다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const bool patch=edition==OriginalEdition::Patch1078;SidPool pool(edition,kCapacity,true);SquidHash hash;
        std::vector<std::uint8_t> spots(65536);std::vector<RiftTypeRecord> types(patch ? 188 : 171);
        SquidUnpop unpop(pool,hash,spots);RawSquidDestroy destroy(pool,unpop,types);Kernel kernel;SquidProcessState clock;
        SquidProcessHost host(pool,types,kernel,destroy,clock);std::vector<std::string> events;SoundProcessState soundState;
        SoundProcessHooks soundHooks;
        soundHooks.wallSeconds=[] { return 0.0; };
        soundHooks.playLoopAt=[&](float,float,SoundHandle,SoundHandle) { events.push_back("L");return 77U; };
        soundHooks.playOnceAt=[](float,float,SoundHandle,std::uint32_t,std::uint32_t) { return 0U; };
        soundHooks.play=[](SoundHandle,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { return 0U; };
        soundHooks.stop=[&](SoundHandle current) { CHECK(current==77);events.push_back("S"); };
        soundHooks.isPlaying=[](SoundHandle) { return 0U; };
        SoundProcessSystem sound(host,soundState,soundHooks);
        RawContainedLookup lookup(pool,[&](Sid sid,std::uint32_t flags) { destroy.Destroy(sid,flags,host.Hooks()); });
        const Sid parent=pool.Allocate(0);auto raw=pool.AllocatedBytes(parent);raw[10]=100;raw[11]=0;raw[patch ? 34 : 32]=1;raw[patch ? 40 : 35]=1;
        ConstructionProcessState state;ConstructionProcessHooks hooks;
        hooks.reserve=[] { return true; };hooks.effect=[](Sid,bool) {};hooks.loadSound=[](std::string_view) { return 11U; };
        hooks.reserveSound=[] { return true; };
        hooks.attachSound=[&](Sid sid,std::uint32_t item,std::uint32_t alternate,std::uint32_t flags) { CHECK(sound.Add(sid,item,alternate,flags)!=nullptr); };
        hooks.refresh=[&] { events.push_back("R"); };hooks.maxHitPoints=[](Sid) { return 100; };hooks.updateDisplay=[](Sid) {};
        hooks.playAt=[&](float,float,std::string_view,std::uint32_t) { events.push_back("W"); };
        hooks.readySound=[](std::uint32_t) { return 0U; };hooks.play=[](std::uint32_t) {};
        hooks.unpop=[&](Sid sid,std::uint32_t) { pool.AllocatedBytes(sid)[11]|=4; };
        hooks.repop=[&](Sid sid,std::uint32_t) { pool.AllocatedBytes(sid)[11]&=0xfb; };
        hooks.push=[](float,float,std::uint32_t,std::uint32_t) {};
        RawConstructionProcess construction(pool,types,state,MakeContainedConstructionHooks(pool,lookup,hooks),&host);
        auto* other=host.AddRegular(parent,123,-1.0F);const Sid otherForm=other->Form();
        construction.Start(parent,8.0F);CHECK(kernel.Size()==3);const Sid soundForm=lookup.Find(parent,47);
        auto* soundProcess=dynamic_cast<SoundProcess*>(host.FromForm(soundForm));CHECK(soundProcess!=nullptr);
        soundState.frame=1;soundProcess->RunFrame();CHECK(soundProcess->Current()==77 && events.back()=="L");
        events.clear();state.now=8.0;kernel.RunFrame();
        CHECK(events==std::vector<std::string>({"S","R","W"}));
        CHECK(kernel.Size()==1 && host.FromForm(otherForm)==other && lookup.Find(parent,47)==Sid{} && lookup.Find(parent,30)==Sid{});
        CHECK(Get(pool.Slot(parent),6,2)==otherForm.value && pool.Slot(soundForm)[11]==3);
        CHECK(Get(pool.Slot(parent),26,patch ? 4 : 2)==100 && !(pool.Slot(parent)[patch ? 40 : 35]&1));
        host.Kill(*other,0);CHECK(kernel.Size()==0 && Get(pool.Slot(parent),6,2)==0);
    }
}
