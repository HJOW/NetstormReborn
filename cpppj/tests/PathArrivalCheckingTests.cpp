// 실제CPU 도착 부모와 같은 raw 상태·실제 일반 finder를 독립 관측에 대조한다.
#include "RawSceneSupport.h"
#include "o/RawPathArrivalParent.h"
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // 결과는 fixture에서 읽고 외부 장치의 응답만 구성한다.
struct PathArrivalScene { // 물리 풀과 논리 상한을 구별해 원문의 범위 진단을 확인한다.
    SidPool pool{OriginalEdition::Patch1078,24000};SquidHash hash;RawPathRuntime runtime{OriginalEdition::Patch1078}; // 같은 raw 경로/공간 자료다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188);std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); // 실제 finder 입력이다.
    std::uint32_t checking{},parent{},capacity{},halted{},result{},deletedInput{},blockers{},mutation{},xb{},yb{},robustCalls{},mask{0x200};bool alive{true}; // 원본 외부 응답/현재 전역이다.
    std::string events;PathArrivalParentHooks hooks; // 실제 요청 순서와 선택 경계다.
    std::span<std::uint8_t> Bytes() { const auto bytes=runtime.Bytes();return {const_cast<std::uint8_t*>(bytes.data()),bytes.size()}; } // 입력 raw 필드만 바꾼다.
    std::span<std::uint8_t> Raw(std::uint32_t sid) { const auto bytes=pool.Slot(Sid{static_cast<std::uint16_t>(sid)});return {const_cast<std::uint8_t*>(bytes.data()),bytes.size()}; } // 물리 SID 슬롯을 빌린다.
    void ChangeParent(std::uint32_t x,std::uint32_t y,std::uint8_t owner) { Put(Raw(parent),14,x);Put(Raw(parent),18,y);Raw(parent)[10]=2;Raw(parent)[34]=owner; } // 진단/도착 뒤 현재 metadata 입력이다.
    void Assertion(std::string_view expression,std::string_view file,std::uint32_t line) { // 원문 진단 복귀에서 현재 상태를 바꾼다.
        events+="A,"+std::to_string(line)+':'+std::string(expression)+':'+std::string(file)+';';
        if (mutation==10 || mutation==11) { checking=mutation==10 ? 0U : 0x80000000U;Put(Bytes(),4,5);Put(Bytes(),8,50);runtime.SetDeathWatchToken(0);runtime.SetTargets(95,96);Raw(parent)[11]=0;ChangeParent(0x40700000,0x40900000,7); }
        if (mutation==12 && line==0x69) { Put(Bytes(),4,5);Put(Bytes(),8,90);capacity=64; }
    }
    explicit PathArrivalScene(const std::vector<std::string>& row) { // CPU와 동일한 입력값이며 판단 결과를 만들지 않는다.
        checking=Number(row[1]);parent=Number(row[2]);capacity=Number(row[4]);halted=Number(row[6]);result=Number(row[7]);deletedInput=Number(row[8]);blockers=Number(row[9]);mutation=Number(row[10]);xb=Number(row[13]);yb=Number(row[14]);Put(Bytes(),4,Number(row[3]));Put(Bytes(),8,parent);runtime.SetTargets(60,61);Put(Bytes(),RawPathRuntime::Layout(OriginalEdition::Patch1078).finished,Number(row[12]));runtime.SetDeathWatchToken(Number(row[11]));
        for (const auto sid:{0U,5U,50U,70U,71U,90U,128U}) { auto raw=Raw(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=sid==70 ? 2U : sid==71 ? 3U : 1U;raw[34]=2;Put(raw,14,0x3fa00000);Put(raw,18,0xc0200000); } // 범위끝/예약0도 물리 메모리는 유효하다.
        Raw(parent)[11]=static_cast<std::uint8_t>(Number(row[5]));
        for (const auto type:{1U,2U,3U}) types[type].footX=types[type].footY=1; // 후보의 실제 발자국 입력이다.
        types[3].flags2=mask;spots[21*256+11]=blockers ? 8U : 0U;
        for (const auto sid:{70U,71U}) { Put(Raw(sid),14,xb);Put(Raw(sid),18,yb); } // 실제 finder가 교차 판정할 양수 좌표다.
        hash.Cell(0,static_cast<int>(std::bit_cast<float>(xb)),static_cast<int>(std::bit_cast<float>(yb)))=static_cast<std::uint16_t>(blockers==4 ? 71U : blockers>=2 ? 70U : 0U);Put(Raw(70),4,blockers==3 ? 71U : 0U,2);
        hooks.parent=[this] { return Sid{static_cast<std::uint16_t>(Get(runtime.Bytes(),8))}; };hooks.validAlive=[this](Sid sid) { return sid.value && sid.value<capacity && !(Raw(sid.value)[11]&3U); };hooks.alive=[this] { return alive; };
        hooks.step=[this] { events+="S,"+std::to_string(xb)+','+std::to_string(yb)+';';if (mutation==1) { checking=0x80000000;Put(Bytes(),4,128); }return RawPathPoint{std::bit_cast<float>(xb),std::bit_cast<float>(yb)}; };
        hooks.unpop=[this](Sid sid) { events+="U,"+std::to_string(sid.value)+",0;"; };
        hooks.pop=[this](Sid sid,RawPathPoint point) { const auto x=std::bit_cast<std::uint32_t>(point.x),y=std::bit_cast<std::uint32_t>(point.y);events+="P,"+std::to_string(sid.value)+','+std::to_string(x)+','+std::to_string(y)+",0;";Put(Raw(sid.value),14,x);Put(Raw(sid.value),18,y);if (mutation==2) { checking=0;Put(Bytes(),4,128); }if (mutation==3) Raw(sid.value)[11]=3; };
        hooks.assertion=[this](auto expression,auto file,auto line) { Assertion(expression,file,line); };
        hooks.robust=[this] { ++robustCalls;events+="Q,"+std::to_string(Get(runtime.Bytes(),4))+','+std::to_string(Get(runtime.Bytes(),8))+';';PathBaseRobustHooks effects;effects.form=[this] { return Get(runtime.Bytes(),4); };effects.parent=[this] { return Get(runtime.Bytes(),8); };effects.capacity=[this] { return capacity; };effects.assertion=hooks.assertion;CHECK(RunPathBaseRobust(effects)); };
        hooks.blocked=[this](RawPathPoint point) { return PathArrivalBlocked(pool,hash,types,spots,mask,point); };
        hooks.halt=[this] { events+="T,"+std::to_string(halted)+';';runtime.SetDeathWatchToken(0);if (mutation==4) checking=0;if (mutation==5) checking=0x80000000;if (mutation==6) runtime.SetDeathWatchToken(0x12345678);if (mutation==7) { runtime.SetTargets(85,86);ChangeParent(0x40700000,0x40900000,7); }return halted!=0; };
        hooks.finished=[this](bool value) { runtime.SetFinished(value); };hooks.deadOrFree=[this](Sid sid) { return (Raw(sid.value)[11]&3U)!=0; };hooks.watchPresent=[this] { return runtime.DeathWatchToken()!=0; };hooks.armWatch=[this] { runtime.SetDeathWatchToken(1); };hooks.clearWatch=[this] { runtime.SetDeathWatchToken(0); };
        hooks.arrival=[this](Sid sid) { events+="R,"+std::to_string(sid.value)+','+std::to_string(runtime.TargetA())+','+std::to_string(runtime.TargetB())+','+std::to_string(result)+';';if (mutation==8) { checking=0;ChangeParent(0x40b00000,0x40d00000,6); }if (mutation==9) { checking=0x80000000;Put(Bytes(),4,128); }if (deletedInput) alive=false;return result; };
        hooks.kill=[this] { events+="K,"+std::to_string(Get(runtime.Bytes(),4))+",0;";alive=false; };
        hooks.clear=[this](Sid sid) { const auto raw=Raw(sid.value);events+="C,"+std::to_string(Get(raw,14))+','+std::to_string(Get(raw,18))+','+std::to_string(raw[10])+','+std::to_string(raw[34])+';'; };
    }
    std::vector<std::string> Run() { // 관측기가 삭제한 payload도 출력만 보존하며 실제 Host 삭제는 별도 검사한다.
        RunPathArrivalParent(RawCheckingFlag{checking},hooks);const auto raw=Raw(parent);return {std::to_string(!alive),std::to_string(checking),std::to_string(Get(runtime.Bytes(),4)),std::to_string(Get(runtime.Bytes(),8)),std::to_string(capacity),std::to_string(raw[11]),std::to_string(Get(runtime.Bytes(),RawPathRuntime::Layout(OriginalEdition::Patch1078).finished)),std::to_string(runtime.DeathWatchToken()!=0),std::to_string(runtime.TargetA()),std::to_string(runtime.TargetB()),std::to_string(Get(raw,14)),std::to_string(Get(raw,18)),std::to_string(raw[10]),std::to_string(raw[34]),std::to_string(robustCalls),events};
    }
};
std::string ArrivalTrace(std::string_view trace) { // 실제 finder는 내부 관측 API가 없으므로 명시 경계의 F/N 요청만 비교에서 제외한다.
    std::string result;for (const auto& event:Split(trace,';')) if (!event.empty() && event[0]!='F' && event[0]!='N') result+=event+';';return result; // 후보 효과/겹침 결과와 나머지 순서는 그대로 비교한다.
}
}
// 860개 실제CPU 행의 현재 필드/진단/도착/제거와 실제 finder 판정 결과를 대조한다.
TEST_CASE(PathArrivalChecking_NativeParentAndActualFinder860) {
    std::size_t count{};const auto fixture=LoadFixture(NETSTORM_PATH_ARRIVAL_CHECKING_FIXTURE);
    for (const auto& row:fixture.rows) { // 처음 틀린 필드와 원래 입력을 보존한다.
        CHECK(row.size()==33);PathArrivalScene scene(row);const auto actual=scene.Run();std::vector<std::string> expected(row.begin()+15,row.begin()+30);expected.push_back(ArrivalTrace(row[32]));CHECK(actual==expected);
        if (actual!=expected) { for (std::size_t index=0;index<actual.size();++index) if (actual[index]!=expected[index]) std::printf("도착 행%s 필드%zu 기대%s 실제%s\n",row[0].c_str(),index,expected[index].c_str(),actual[index].c_str());return; }++count; // finder 호출수 두 칸은 내부 API가 없어 판정 대조와 구별한다.
    }CHECK(count==860);
}
// 선택된 진단 장치만 요구하며 앞선 Step/공간 효과는 예외 뒤에도 남는다.
TEST_CASE(PathArrivalChecking_SelectedDiagnosticAndPartialEffects) {
    const auto fixture=LoadFixture(NETSTORM_PATH_ARRIVAL_CHECKING_FIXTURE);PathArrivalScene scene(fixture.rows.front());scene.result=1003;scene.hooks.assertion={};CHECK(Throws([&] { scene.Run(); }));CHECK(scene.events.find("S,")==0 && scene.events.find("R,50,60,61,1003;")!=std::string::npos && scene.alive);
    scene.hooks.assertion=[&](auto,auto,auto line) { CHECK(line==0x28d);scene.checking=0; };scene.events.clear();scene.Run();CHECK(!scene.alive && scene.events.find("K,5,0;")!=std::string::npos);
}
