// 두 실제 드롭 부모와 같은 Dude 생성/Pop/Regular Host 소비자를 대조한다.
#include "DudeSpawnSupport.h"
#include "o/RawDudeDrop.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
using namespace netstorm::test::dudespawn;
namespace {
// 실제 PE의 기대 출력은 fixture에서만 읽으며 현재 입력 준비에 사용하지 않는다.
const std::vector<std::vector<std::string>>& DropRows() { static const auto rows=LoadFixture(NETSTORM_DUDEDROP_FIXTURE).rows;return rows; }
// 부모 입력 중 현재 population/blocked만 기존 실제 생성 장면에 전달한다.
std::vector<std::string> SpawnInput(const std::vector<std::string>& row) { return {row[0],row[10],row[11],row[12],"0","0",row[6],"0"}; }
// 실제 보호 Dude와 같은 풀/공간/시계의 현재 source 및 Regular Host를 구성한다.
struct DropScene:SpawnScene {
    std::vector<RiftTypeRecord> sourceTypes; // 부모89의 명시 발자국/genus 입력이며 새 Dude70은 보호 자료다.
    std::vector<DudeNoticeTypeData> speeds; // 진입점과 같은 현재 타입 배열이며 이동 요청은 실행하지 않는다.
    SquidPostPopList residences; // 현재 부모의 진입점 계산은 거주지 후보 목록을 조회하지 않는다.
    RawSquidDestroy destroy; // 실제 Regular Form의 같은 풀/공간 수명 처리기다.
    Kernel kernel; // 실제 새 Regular 객체를 소유한다.
    SquidProcessState clock; // 부모의 float 지연에 더하는 실제 double 입력 시각이다.
    SquidProcessHost host; // 조회/Regular 생성/ProcessForm 부착의 실제 소비자다.
    GameRandom random; // 드롭 두 좌표와 공유하는 원본32비트 게임 난수다.
    RawDudeNotice notice; // 원본 진입점의 별도 복원 몸체를 재사용한다.
    RegularProcess* current{}; // 드롭이 실제 Host에 생성한 Regular이며 기대값을 대입하지 않는다.
    std::vector<std::string> events; // 원본 명시 조회/힙/부착 경계와 같은 순서를 기록한다.
    const std::vector<std::string>& row; // 현재 독립 입력이며 출력은 실행 후에만 읽는다.
    // 실제 준비 SID5를 부모 raw로 바꾸고 새 Dude6/Regular7의 같은 할당 순서를 사용한다.
    explicit DropScene(const std::vector<std::string>& input):SpawnScene(SpawnInput(input)),sourceTypes(assets.types),speeds(sourceTypes.size()),destroy(pool,unpop,sourceTypes),host(pool,sourceTypes,kernel,destroy,clock,[](Sid,std::uint32_t,std::uint32_t,float) { return 1.0f; }),random(Number(input[9])),notice(pool,hash,sourceTypes,assets.frames,speeds,residences,{clock},{}),row(input) {
        sourceTypes[89].footX=static_cast<std::int32_t>(Number(row[3]));sourceTypes[89].footY=static_cast<std::int32_t>(Number(row[4]));sourceTypes[89].flags2=Number(row[5]);
        auto raw=pool.AllocatedBytes(Sid{5});std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=89;raw[pool.Edition()==OriginalEdition::Patch1078 ? 34U : 32U]=static_cast<std::uint8_t>(Number(row[6]));Put(raw,14,std::bit_cast<std::uint32_t>(20.75f));Put(raw,18,std::bit_cast<std::uint32_t>(21.9f));raw[11]=static_cast<std::uint8_t>(Number(row[7]));clock.now=std::bit_cast<double>(std::stoull(row[13]));
    }
    // 원본 관찰 순서를 남기면서 조회/생성 자체는 현재 Host의 실제 몸체를 수행한다.
    DudeTaskHooks Hooks() {
        auto hooks=MakeDudeTaskHostHooks(pool,host,{});auto find=std::move(hooks.taskRegular);auto add=std::move(hooks.addTaskRegular);
        hooks.taskRegular=[this,find=std::move(find)](Sid sid) { events.push_back("R:"+std::to_string(sid.value)+":65520:20736");return find(sid); };
        hooks.addTaskRegular=[this,add=std::move(add)](Sid sid,std::uint32_t event) { events.push_back("N:40");current=add(sid,event);events.push_back("B:46:"+std::to_string(sid.value)+":0:80");return current; };return hooks;
    }
    // 실제 생성/Pop/후처리와 task 예약을 실행한 뒤 원본과 같은 전체 상태를 직렬화한다.
    std::vector<std::string> RunDrop() {
        DudeSpawnHooks spawnHooks;spawnHooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { const auto raw=pool.Slot(sid);const auto size=display.FrameSize(pool.Edition(),raw,0);if (pop.Pop(sid,assets.types[raw[10]],size[0],size[1],x,y,flags)!=RawPopResult::Registered) throw std::logic_error("Dude drop 실제 Pop 실패"); };
        RawDudeSpawn spawn(pool,factory,owner,spawnState,std::move(spawnHooks));RawDudeTask task(pool,{clock},Hooks());RawDudeDrop drop(pool,sourceTypes,spawn,task,notice,random);std::string status="return";
        try { drop.Drop(Sid{static_cast<std::uint16_t>(Number(row[8]))},std::bit_cast<std::int32_t>(Number(row[2])),Number(row[1])!=0); }
        catch (const std::logic_error& error) { if (std::string(error.what())!="Dude drop sourceSID 원본 assert") throw;status="assert";events.push_back("A:0"); }
        const Sid born{pool.Slot(Sid{6})[10]==70 ? std::uint16_t{6} : std::uint16_t{0}};std::string actions,regular="-",populationText,counts="0,0",rectangles;
        // 경계 호출 순서는 현재 실행으로만 만든다.
        for (const auto& event:events) { if (!actions.empty()) actions+=';';actions+=event; }
        if (current) regular=std::to_string(std::bit_cast<std::uint64_t>(current->Time()))+','+std::to_string(current->Event())+','+std::to_string(std::bit_cast<std::uint32_t>(current->Payload()))+','+std::to_string(current->Count());
        // 활성 count 밖의 population stale 메모리도 보존한다.
        for (const auto entry:population.entries) { if (!populationText.empty()) populationText+=',';populationText+=std::to_string(entry); }
        if (populationText.empty()) populationText="-";
        // 실제 공통 owner 목록의 현재 count를 모두 기록한다.
        for (const auto& list:bookkeeping.ownerFactories) counts+=','+std::to_string(list.count);
        // 같은 표시 병합기의 전체 변경 영역/flags 순서를 기록한다.
        for (const auto& entry:sink.dirty.Entries()) { if (!rectangles.empty()) rectangles+=';';rectangles+=std::to_string(entry.rect.left)+','+std::to_string(entry.rect.top)+','+std::to_string(entry.rect.right)+','+std::to_string(entry.rect.bottom)+','+std::to_string(entry.flags); }
        if (rectangles.empty()) rectangles="-";const auto raw=Hex(pool.Slot(born.value ? born : Sid{6}));
        return {status,std::to_string(born.value),actions,regular,std::to_string(random.State()),std::to_string(population.count),populationText,raw,std::to_string(bookkeeping.totalCost),std::to_string(bookkeeping.productionDirty),std::to_string(bookkeeping.depth),std::to_string(WordAdler<std::uint32_t>(bookkeeping.localCounts)),std::to_string(WordAdler<std::uint32_t>(bookkeeping.localSecondaryCounts)),std::to_string(WordAdler<std::uint32_t>(bookkeeping.globalCounts)),counts,"1",raw,sink.Suppressed() ? "1" : "0",rectangles};
    }
};
// 판본별 정상/진단 입력을 모두 같은 실제 Host/생성/공간 소비자에서 재생한다.
void ReplayDrop(std::string_view edition) {
    std::size_t count=0,expectedCount=0;int failures=0;
    // 고유 판본 행 수만 먼저 세며 기대 출력은 실행 뒤 비교한다.
    for (const auto& row:DropRows()) if (row[0]==edition) ++expectedCount;
    // 부모 입력13개와 전체 출력19개를 명시적으로 구분해 대조한다.
    for (const auto& row:DropRows()) {
        if (row[0]!=edition) continue;CHECK(row.size()==33);DropScene scene(row);const auto output=scene.RunDrop();const auto expected=std::vector<std::string>(row.begin()+14,row.end());const bool same=output==expected;CHECK(same);
        if (!same) {
            // 최초 차이의 실제 native/C++ 값을 함께 남긴다.
            for (std::size_t index=0;index<output.size();++index) if (output[index]!=expected[index]) std::printf("%s DudeDrop 행%zu 결과%zu 기대%s / 실제%s\n",row[0].c_str(),count,index,expected[index].c_str(),output[index].c_str());
            if (++failures>=5) break;
        }
        ++count;
    }
    CHECK(failures==0 && count==expectedCount);
}
}
// 실제10.78 두 부모의 난수/진입점/float 지연과 sourceSID0 진단을 대조한다.
TEST_CASE(DudeDrop_PatchNativeParentsAndActualHost) { ReplayDrop("originals"); }
// CD10.72의 생성실패 taskSID0 반환과 같은 실제 Dude/Regular 소비자를 대조한다.
TEST_CASE(DudeDrop_CdNativeParentsAndActualHost) { ReplayDrop("originalCD"); }
