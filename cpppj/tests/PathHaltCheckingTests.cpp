// 실제CPU 공통 Halt의 현재 상태/진단 복귀와 기존 재지정 몸체를 대조한다.
#include "RawSceneSupport.h"
#include "o/RawPathHaltParent.h"
#include "o/RawPathRetarget.h"
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace { // 원문 입력/외부 장치만 구성하고 결과는 fixture에서 읽는다.
// 원문과 같은 현재 raw 부모/경로/전역을 한 호출 동안 유지한다.
struct PathHaltScene {
    SidPool pool{OriginalEdition::Patch1078,24000};RawPathRuntime runtime{OriginalEdition::Patch1078}; // 같은 유효 물리 풀/패치 경로 이미지다.
    std::uint32_t result{},checking{},boss{},form{},genus{},first{},second{},choice{},action{},finds{};bool alive{true}; // CPU와 같은 외부 응답/현재 전역 입력이다.
    std::string events;PathHaltParentHooks hooks;std::unique_ptr<RawPathRetarget> retarget;std::uint32_t nuggetType{154}; // 원문 호출 순서와 기존 실제 재지정 몸체다.
    explicit PathHaltScene(const std::vector<std::string>& row) { // stack 포인터 주소는 비교하지 않고 독립 수명 관찰로 보존한다.
        result=Number(row[1]);checking=Number(row[2]);boss=Number(row[3]);form=Number(row[4]);genus=Number(row[6]);first=Number(row[8]);second=Number(row[9]);choice=Number(row[10]);action=Number(row[11]);runtime.SetAttachment(Sid{5},Sid{50});Put(Bytes(),4,form);runtime.SetTargets((Number(row[7])&1U) ? 60U : 0U,(Number(row[7])&2U) ? 61U : 0U);
        for (const auto sid:{50U,51U,52U,60U,61U}) { auto raw=Raw(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=1;raw[11]=sid>=60 ? 2U : 0U; } // 유효 부모와 죽은 두 대상의 명시 입력이다.
        Raw(50)[40]=static_cast<std::uint8_t>(Number(row[5]));hooks.parent=[] { return Sid{50}; };hooks.alive=[this] { return alive; };
        hooks.halt=[this](Sid parent) { CHECK(parent==Sid{50});events+="H,50,"+std::to_string(result)+';';if (action&1U) alive=false;if (action&2U) checking=0x80000000;if (action&4U) checking=0;if (action&8U) boss=boss ? 0U : 1U;if (action&16U) Raw(50)[40]^=0x20;if (action&32U) Raw(50)[10]=2;if (action&64U) Put(Bytes(),8,51);return result; };
        hooks.assertion=[this](auto expression,auto file,auto line) { events+="A,"+std::to_string(line)+':'+std::string(expression)+':'+std::string(file)+';';if (action&512U) { checking=0;boss=0;Put(Bytes(),8,52); } };
        hooks.forcedStop=[this](Sid parent) { return (Raw(parent.value)[40]&0x20U)!=0; };hooks.mayKill=[this] { const auto signedForm=std::bit_cast<std::int32_t>(form);return boss || (signedForm>4 && signedForm<15000); };hooks.kill=[this] { events+="K,"+std::to_string(form)+",0;";alive=false; };
        hooks.boss=[this] { return boss!=0; };hooks.carrier=[this](Sid parent) { const auto value=Raw(parent.value)[10]==1 ? genus : genus^0x30000U;return (value&0x30000U)!=0; };hooks.hasTargets=[this] { return runtime.TargetB()!=0 || runtime.TargetA()!=0; };
        PathRetargetHooks effects;effects.find=[this](Sid parent,auto mask,auto type,auto index) { events+="F,"+std::to_string(parent.value)+','+std::to_string(mask)+','+std::to_string(type)+','+std::to_string(index)+';';return Sid{static_cast<std::uint16_t>(finds++ ? second : first)}; };
        effects.chooseReturn=[this](Sid parent) { events+="C,"+std::to_string(parent.value)+';';return Sid{static_cast<std::uint16_t>(choice)}; };effects.chooseGeyser=[this](Sid parent) { events+="Y,"+std::to_string(parent.value)+';';return Sid{static_cast<std::uint16_t>(choice)}; };
        effects.goReturn=[this](Sid parent,Sid target) { Go('R',parent,target); };effects.goGeyser=[this](Sid parent,Sid target) { Go('G',parent,target); };retarget=std::make_unique<RawPathRetarget>(pool,nuggetType,std::move(effects));hooks.retarget=[this](Sid parent) { retarget->Update(runtime,parent); };
    }
    std::span<std::uint8_t> Bytes() { const auto raw=runtime.Bytes();return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // 현재 경로의 명시 raw 입력만 변경한다.
    std::span<std::uint8_t> Raw(std::uint32_t sid) { const auto raw=pool.Slot(Sid{static_cast<std::uint16_t>(sid)});return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // 주소값 모델 없이 실제 슬롯을 빌린다.
    void Go(char name,Sid parent,Sid target) { events+=name;events+=','+std::to_string(parent.value)+','+std::to_string(target.value)+';';if (action&128U) alive=false; } // CPU 장치와 같은 명시 삭제 입력이며 실제 Host 삭제는 별도 검사한다.
    std::vector<std::string> Run() { const auto stop=RunPathHaltParent(RawCheckingFlag{checking},hooks);return {std::to_string(stop),std::to_string(!alive),std::to_string(checking),std::to_string(boss),std::to_string(Get(runtime.Bytes(),8)),std::to_string(form),std::to_string(runtime.TargetA()),std::to_string(runtime.TargetB()),std::to_string(Raw(50)[10]),std::to_string(Raw(50)[40]),std::to_string(finds),events}; } // 현재 출력과 실제 요청만 기록한다.
};
const auto& PathHaltRows() { static const auto rows=LoadFixture(NETSTORM_PATH_HALT_CHECKING_FIXTURE).rows;return rows; } // 최초 실제CPU2288행을 읽기 전용으로 사용한다.
}
// 저장 부모/현재checking·권한·TYPE/강제정지와 기존 재지정의 순서를 CPU에 대조한다.
TEST_CASE(PathHaltChecking_NativeParentAndActualRetarget2288) {
    std::size_t count{};
    for (const auto& row:PathHaltRows()) { // 첫 불일치의 입력/기대값을 유지한다.
        CHECK(row.size()==24);PathHaltScene scene(row);const auto actual=scene.Run();const auto expected=std::vector<std::string>(row.begin()+12,row.end());CHECK(actual==expected);
        if (actual!=expected) { for (std::size_t index=0;index<actual.size();++index) if (actual[index]!=expected[index]) std::printf("경로 Halt 행%s 필드%zu 기대%s 실제%s\n",row[0].c_str(),index,expected[index].c_str(),actual[index].c_str());return; }++count; // 불일치한 각 출력 필드만 기록한다.
    }CHECK(count==2288);
}
// 삭제 후에는 현재 진단/수명 관찰만 쓰고 미선택 경계와 앞선 효과를 보존한다.
TEST_CASE(PathHaltChecking_SelectedDevicesAndDiagnosticResume) {
    std::uint32_t checking{};bool alive{true};PathHaltParentHooks hooks;hooks.parent=[] { return Sid{50}; };hooks.halt=[&](Sid) { alive=false;return 0U; };hooks.alive=[&] { return alive; };CHECK(RunPathHaltParent(RawCheckingFlag{checking},hooks));
    checking=0x80000000;CHECK(Throws([&] { RunPathHaltParent(RawCheckingFlag{checking},hooks); }));hooks.assertion=[&](auto expression,auto file,auto line) { CHECK(expression=="result == haltALREADY_DEAD" && file=="\\Ns\\O\\Pathprocess.cpp" && line==0x2d5);checking=0; };CHECK(RunPathHaltParent(RawCheckingFlag{checking},hooks) && !checking);
    checking=1;hooks.assertion=[&](auto,auto,auto) { checking=0;throw std::runtime_error("원문 진단 중단"); };CHECK(Throws([&] { RunPathHaltParent(RawCheckingFlag{checking},hooks); }) && !checking);
    hooks.assertion={};hooks.halt=[&](Sid) { alive=false;return 12U; };checking=0xffffffff;CHECK(RunPathHaltParent(RawCheckingFlag{checking},hooks));
}
