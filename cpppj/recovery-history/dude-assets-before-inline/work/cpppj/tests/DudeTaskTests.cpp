// 두 원본 일반 task setter와 같은 실제 Host의 Regular 재사용/생성을 대조한다.
#include "RawSceneSupport.h"
#include "o/RawDudeTask.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 독립 원본 출력은 저장한 fixture에서 읽으며 입력 계산에 사용하지 않는다.
const std::vector<std::vector<std::string>>& TaskRows() {
    static const auto rows=LoadFixture(NETSTORM_DUDETASK_FIXTURE).rows;return rows;
}
// 같은 풀/Host/시계와 현재 Regular 객체를 실제로 소유하는 task 장면이다.
struct TaskScene {
    SidPool pool; // 원본 source50와 Form83을 공유하는 판본별 raw 저장소다.
    SquidHash hash; // 현재 Host 삭제 경로에 필요한 같은 월드 해시다.
    std::vector<std::uint8_t> spots; // 현재 삭제 어댑터의 같은 점유 저장소다.
    std::vector<RiftTypeRecord> types; // 현재 Dude70/다리82와 실제 Regular 타입의 표다.
    SquidUnpop unpop; // 같은 풀/해시의 실제 삭제 연결이다.
    RawSquidDestroy destroy; // 실제 Host의 Form/종속 수명 처리기다.
    Kernel kernel; // 실제 Regular 객체를 소유하는 현재 커널이다.
    SquidProcessState clock; // task 지연에 더하는 현재 double 시각이다.
    SquidProcessHost host; // 실제 Regular 조회/생성/반복 실행을 수행한다.
    RegularProcess* current{}; // 생성하거나 재사용하는 현재 실제 Regular 객체다.
    std::vector<std::string> events; // 원본과 같은 조회/할당/부착/표면 요청 순서다.
    const std::vector<std::string>& row; // 기대값과 분리된 현재 입력행 참조다.
    // 실제 client 할당으로 원본 Regular 생성 번호83 직전까지 확보한다.
    explicit TaskScene(const std::vector<std::string>& input)
        :pool(input[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,kCapacity,true),spots(65536),types(input[0]=="originals" ? 188 : 171),unpop(pool,hash,spots),destroy(pool,unpop,types),host(pool,types,kernel,destroy,clock,[](Sid,std::uint32_t,std::uint32_t,float) { return 1.0f; }),row(input) {
        // 원본 입력번호와 같은 일반 client 슬롯을 실제 확보한다.
        for (std::uint16_t id=5;id<=82;++id) CHECK(pool.Allocate(2)==Sid{id});
        auto raw=pool.AllocatedBytes(kSource);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=static_cast<std::uint8_t>(Number(row[6]));Put(raw,12,Number(row[3]),2);Put(raw,14,std::bit_cast<std::uint32_t>(20.75f));Put(raw,18,std::bit_cast<std::uint32_t>(21.9f));
        if (Number(row[5])) {
            clock.now=20;current=host.AddRegular(kSource,0x5104,1,0x50);CHECK(current && current->Form()==Sid{83});if (!current) throw std::logic_error("Dude task 검사 현재 Regular 생성 실패");
            // 기존 count7/time27은 원본 기대값 대입 없이 실제 RunFrame 일곱 번으로 준비한다.
            for (int count=0;count<7;++count) { clock.now=current->Time();current->RunFrame(); }
            CHECK(current->Count()==7 && current->Time()==27 && current->Payload()==1);
        }
        pool.AllocatedBytes(kSource)[11]=static_cast<std::uint8_t>(Number(row[2]));clock.now=std::bit_cast<double>(std::stoull(row[8]));
    }
    // 실제 Host의 조회/생성을 쓰고 native의 외부 효과 관찰 순서를 함께 기록한다.
    DudeTaskHooks Hooks() {
        auto hooks=MakeDudeTaskHostHooks(pool,host,{});auto find=std::move(hooks.taskRegular);auto add=std::move(hooks.addTaskRegular);
        hooks.taskRegular=[this,find=std::move(find)](Sid sid) { events.push_back("R:50:65520:20736");return find(sid); };
        hooks.addTaskRegular=[this,add=std::move(add)](Sid sid,std::uint32_t event) { events.push_back("N:40");current=add(sid,event);events.push_back("B:46:50:0:80");return current; };
        hooks.notifySurface=[this](Sid sid) { events.push_back("S:"+std::to_string(sid.value)); };return hooks;
    }
    // 원본 결과칸을 읽지 않고 현재 입력으로 실행한 raw/Regular 상태를 직렬화한다.
    std::vector<std::string> Run() {
        DudeTaskState state{clock};RawDudeTask task(pool,state,Hooks());std::string status="return";
        try { task.Set(Sid{static_cast<std::uint16_t>(Number(row[1]))},Number(row[4]),std::bit_cast<float>(Number(row[7]))); }
        catch (const std::logic_error& error) { if (std::string(error.what())!="Dude task FALL 교체 원본 assert") throw;events.push_back("A:156");status="assert"; }
        std::string actions,regular="-";
        // 실제 관찰 순서만 구분자로 묶으며 기대 상태를 계산하지 않는다.
        for (const auto& event:events) { if (!actions.empty()) actions+=';';actions+=event; }
        if (current) regular=std::to_string(std::bit_cast<std::uint64_t>(current->Time()))+','+std::to_string(current->Event())+','+std::to_string(std::bit_cast<std::uint32_t>(current->Payload()))+','+std::to_string(current->Count());
        return {status,actions,Hex(pool.Slot(kSource)),regular};
    }
};
// 정상 반환과 원본 assert 분류를 구분하여 판본별 모든 독립 입력을 재생한다.
void ReplayTask(std::string_view edition) {
    std::size_t count=0,expectedCount=0;int failures=0;
    // 판본별 행 수만 계산하고 기대 출력은 실제 실행 후 비교한다.
    for (const auto& row:TaskRows()) if (row[0]==edition) ++expectedCount;
    // 원본 입력과 마지막 네 출력칸을 분리해 전수 대조한다.
    for (const auto& row:TaskRows()) {
        if (row[0]!=edition) continue;CHECK(row.size()==13);TaskScene scene(row);const auto output=scene.Run();const auto expected=std::vector<std::string>(row.begin()+9,row.end());const bool same=output==expected;CHECK(same);
        if (!same) {
            // 첫 차이는 입력행과 두 실제 결과를 함께 기록한다.
            for (std::size_t index=0;index<output.size();++index) if (output[index]!=expected[index]) std::printf("%s DudeTask 행%zu 결과%zu 기대%s / 실제%s\n",row[0].c_str(),count,index,expected[index].c_str(),output[index].c_str());
            if (++failures>=5) break;
        }
        ++count;
    }
    CHECK(failures==0 && count==expectedCount);
}
}
// 실제10.78 task DWORD/WORD와 낙하 교체 진단·정밀 double 지연을 대조한다.
TEST_CASE(DudeTask_PatchNativeSetterAndFallAssert) { ReplayTask("originals"); }
// CD10.72 빈 SID/free/dead 반환과 낙하 유지·기존/새 Regular를 대조한다.
TEST_CASE(DudeTask_CdNativeSetterAndFallRetention) { ReplayTask("originalCD"); }
// 미연결 Regular 생성과 표면 효과를 원본 호출시점에서 거부한다.
TEST_CASE(DudeTask_MissingCreationAndSurfaceRejected) {
    auto row=TaskRows().front();TaskScene scene(row);auto hooks=scene.Hooks();hooks.addTaskRegular={};DudeTaskState state{scene.clock};RawDudeTask task(scene.pool,state,std::move(hooks));CHECK(Throws([&] { task.Set(kSource,0x5103,0); }));CHECK(Get(scene.pool.Slot(kSource),12,2)==0x5101);
    row[6]="82";TaskScene bridge(row);auto bridgeHooks=bridge.Hooks();bridgeHooks.notifySurface={};DudeTaskState bridgeState{bridge.clock};RawDudeTask bridgeTask(bridge.pool,bridgeState,std::move(bridgeHooks));CHECK(Throws([&] { bridgeTask.Set(kSource,0x5103,.1f); }));CHECK(Get(bridge.pool.Slot(kSource),12,2)==0x5103);CHECK(bridge.host.FindEvent(kSource,0x5103)!=nullptr);
}
