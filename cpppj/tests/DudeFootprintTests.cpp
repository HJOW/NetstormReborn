// 제품 전 관측한0크기 후보의 전체 커서와 실제 Dude24 실패·겹침·재시도를 대조한다.
#include "RawSceneSupport.h"
#include "o/RawPathStart.h"
#include "client/RawPriestWorldBindings.h"
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 실제 원본 finder+4..+60의 모든 DWORD를 같은 순서로 기록한다.
std::string Cursor(const RawSquidFinder& finder) {
    const auto& state=finder.State();const auto& area=state.area;
    const std::array<int,15> values{area.left,area.top,area.right,area.bottom,state.firstX,state.firstY,state.lastX,state.lastY,state.level,state.x,state.y,state.next.value,state.current.value,state.skipSurface ? 1 : 0,state.extendedHeight ? 4 : 0};
    std::string result;
    // 결과 SID뿐 아니라 다음 체인·버킷·마지막 빈 반환까지 보존한다.
    for (const auto value:values) { if (!result.empty()) result+=',';result+=std::to_string(value); }return result;
}
// 도달하지 않은 원본 가상/검색/표시 효과를 빈 구현으로 숨기지 않는다.
[[noreturn]] void Unused() { throw std::logic_error("발자국0 실패 대조의 미실행 효과"); }
// 명시 raw/TYPE/네 해시 입력만 준비하고 후보나 경로 결과는 계산하지 않는다.
struct ZeroScene {
    SidPool pool; // 원본과 같은 폭과24,000슬롯의 실제 저장소다.
    SquidHash hash; // 후보 next와 네 단계 버킷 머리가 공유하는 저장소다.
    std::vector<RiftTypeRecord> types; // 현재0/양수 발자국과genus 입력이다.
    std::vector<int> heights; // 같은 타입의 확장 높이 입력이다.
    // fixture에 쓰인 번호5..64만 실제 client free list에서 확보한다.
    explicit ZeroScene(OriginalEdition edition):pool(edition,24000,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),heights(types.size()) {
        // 후보 raw 입력은 실제 할당된 번호에만 쓴다.
        for (std::uint16_t sid=5;sid<=64;++sid) CHECK(pool.Allocate(2)==Sid{sid});
    }
    // native 입력의 실제 바이트·크기·버킷 등록 순서를 그대로 공급한다.
    void Setup(const std::string& description) {
        hash.Reset();
        // 후보를 원문 순서로 머리에 삽입하며 반환 순서를 미리 만들지 않는다.
        for (const auto& item:Split(description,';')) {
            const auto fields=Split(item,',');CHECK(fields.size()==10);if (fields.size()!=10) return;std::array<std::uint32_t,10> values{};
            // float 좌표는 원문 DWORD 그대로 보존한다.
            for (std::size_t index=0;index<values.size();++index) values[index]=Number(fields[index]);
            const Sid sid{static_cast<std::uint16_t>(values[0])};auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=static_cast<std::uint8_t>(values[1]);raw[11]=static_cast<std::uint8_t>(values[2]);raw[pool.Edition()==OriginalEdition::Patch1078 ? 40 : 35]=static_cast<std::uint8_t>(values[3]);Put(raw,14,values[4]);Put(raw,18,values[5]);
            types[values[1]].footX=static_cast<int>(values[6]);types[values[1]].footY=static_cast<int>(values[7]);heights[values[1]]=static_cast<int>(values[8]);auto& head=hash.Bucket(static_cast<int>(values[9]),std::bit_cast<float>(values[4]),std::bit_cast<float>(values[5]));Put(raw,4,head,2);head=sid.value;
        }
    }
};
// 명시 경계의 native Dispose는 C++의 미부착 unique_ptr 소멸과 구별하고 공유 생성/겹침만 비교한다.
std::string Shared(const std::string& native) {
    std::string result;
    // 원본 순서를 유지하면서 제품 외부에서 관측할 수 있는 효과만 선택한다.
    for (const auto& event:Split(native,';')) if (event!="D") { if (!result.empty()) result+=';';result+=event; }return result;
}
}

// 두 판본의0/양수 크기·부류 시작·확장 높이·접촉을 전체 커서까지 원문과 비교한다.
TEST_CASE(DudeFootprint_NativeFinder480) {
    std::size_t rows{};
    // 최초 동결한 원본480입력의 두 정밀도가 일치해야 한다.
    for (const auto& row:LoadFixture(NETSTORM_DUDE_FOOTPRINT_FIND).rows) {
        CHECK(row.size()==7);if (row.size()!=7) return;CHECK(row[5]==row[6]);ZeroScene scene(row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);scene.Setup(row[2]);const auto area=Split(row[3],',');CHECK(area.size()==4);if (area.size()!=4) return;
        const auto before=std::vector<std::uint8_t>(scene.pool.Bytes().begin(),scene.pool.Bytes().end());RawSquidFinder finder(scene.pool,scene.hash,scene.types,scene.heights);
        auto current=finder.Begin({std::stoi(area[0]),std::stoi(area[1]),std::stoi(area[2]),std::stoi(area[3])},Number(row[4]));std::string result=Cursor(finder);std::size_t returned{};
        // 첫 반환부터 마지막 SID0까지 같은 실제 커서를 순회한다.
        while (current.value && returned<32) { current=finder.Next();result+=';'+Cursor(finder);++returned; }
        CHECK(returned<32 && result==row[5] && std::equal(before.begin(),before.end(),scene.pool.Bytes().begin()));++rows;
    }
    CHECK(rows==480);
}

// 빈 실제 표면 Init에서0크기 부모/후보·좌표0·겹침 후 매장·재시도 비트를 검사한다.
TEST_CASE(DudeFootprint_NativeFailedStart640) {
    std::size_t rows{};
    // 기존 foot1 fixture를 바꾸지 않고 새 원본640행을 실제 Start에 전달한다.
    for (const auto& row:LoadFixture(NETSTORM_DUDE_FOOTPRINT_FAIL).rows) {
        CHECK(row.size()==3);if (row.size()!=3) return;CHECK(row[1]==row[2]);const auto input=Split(row[0],',');const auto expected=Split(row[1],'|');CHECK(input.size()==9 && expected.size()==6);if (input.size()!=9 || expected.size()!=6) return;
        ZeroScene scene(OriginalEdition::Patch1078);std::vector<RiftTypeFrames> frames(scene.types.size(),RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(scene.types.size());std::vector<std::uint8_t> spots(65536);SquidProcessState clock;clock.now=123.125;RawPathProcessState state{clock};
        const std::string parent="5,70,0,0,"+input[2]+','+input[3]+','+input[0]+','+input[1]+",0,1";const auto size=Number(input[6]) ? "0,0" : "1,1";scene.Setup(parent+";53,150,0,0,"+input[4]+','+input[5]+','+size+",0,1");scene.types[70].flags2=0x400;scene.types[150].flags2=0x40000;scene.pool.AllocatedBytes(Sid{5})[34]=2;
        SquidUnpop unpop(scene.pool,scene.hash,spots);RawSquidDestroy destroy(scene.pool,unpop,scene.types);Kernel kernel(32);SquidProcessHost host(scene.pool,scene.types,kernel,destroy,clock);
        SquidFrame frame(scene.pool,scene.types,{[](Sid,std::int32_t)->std::array<float,2> { Unused(); },[](Sid,std::uint32_t) { Unused(); },[](Sid,std::uint32_t) { Unused(); },[](Sid,float,float,std::uint32_t) { Unused(); }});
        RawContainedLookup lookup(scene.pool,[](Sid,std::uint32_t) { Unused(); });std::uint32_t dude=70,teleport=134,mana=105,priest=158,geyser=122;bool snap{},retry=Number(input[7])!=0;std::string trace;std::size_t created{};
        const auto append=[&](std::string event) { if (!trace.empty()) trace+=';';trace+=event; }; // 원본 생성/겹침 요청의 순서를 유지한다.
        netstorm::client::RawMovementProcessTypes movement;RawPathStartHooks hooks;hooks.processType=[&](std::uint32_t type) { return netstorm::client::SelectMovementProcessType(OriginalEdition::Patch1078,scene.types,movement,type); };hooks.create=[](bool)->std::unique_ptr<RawPathProcess> { Unused(); };hooks.metadata=[](Sid)->RawPathStartType { Unused(); };
        hooks.createType=[&](std::uint32_t type) {
            CHECK(type==24);append("C");++created;RawPathProcessHooks effects;
            effects.unpop=[](Sid,std::uint32_t) { Unused(); };effects.repop=[](Sid,std::uint32_t) { Unused(); };effects.pop=[](Sid,RawPathPoint,std::uint32_t) { Unused(); };
            effects.halt=[](Sid)->std::uint32_t { Unused(); };effects.arrival=[](Sid,std::uint32_t,std::uint32_t)->std::uint32_t { Unused(); };effects.soundId=[](Sid)->std::uint32_t { Unused(); };effects.stepSound=[](Sid,float,float,std::uint32_t) { Unused(); };
            effects.clearBlocked=[](Sid,RawPathPoint,std::uint32_t,std::uint32_t) { Unused(); };effects.cleanup=[](RawPathRuntime&,Sid,std::uint32_t) { Unused(); };effects.search=[](RawPathRuntime&,Sid,RawPathPoint,RawPathPoint,std::uint32_t) { Unused(); };
            return std::make_unique<RawPathProcess>(scene.pool,scene.hash,scene.types,frames,shapes,spots,state,std::move(effects),false,false,false,false,true);
        };
        hooks.clearBlocked=[&](RawPathPoint point,std::uint32_t type,std::uint32_t owner) {
            append("B:"+std::to_string(std::bit_cast<std::uint32_t>(point.x))+','+std::to_string(std::bit_cast<std::uint32_t>(point.y))+','+std::to_string(type)+','+std::to_string(owner));if (Number(input[8])) scene.pool.AllocatedBytes(Sid{53})[40]=8;
        };
        RawPathStart start(scene.pool,scene.hash,scene.types,host,lookup,frame,{clock,dude,teleport,mana,priest,geyser,snap,retry},std::move(hooks));CHECK(start.Start({Sid{5},{13.2F,13.3F},0,0x12345678,0x87654321,0xa5fedcba,0x13579bdf})==nullptr);
        CHECK(trace==Shared(expected[1]) && created>0 && created<=2);CHECK(Get(scene.pool.Slot(Sid{5}),36)==Number(expected[2]) && std::string("0,0,")+(retry ? "1" : "0")==expected[3] && scene.pool.Slot(Sid{53})[40]==Number(expected[5]));CHECK(!kernel.Get(1) && Get(scene.pool.Slot(Sid{5}),6,2)==0);++rows;
    }
    CHECK(rows==640);
}
