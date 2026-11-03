// 펜스 두 진단 부모의 원문 전체와 현재 raw/전역 재판정을 창 없이 대조한다.
#include "RawSceneSupport.h"
#include "o/RawFenceTypeParent.h"
#include "o/RawFenceLifetime.h"
#include "o/RawFenceRebuild.h"

using namespace netstorm::test::rawscene;
using namespace netstorm::o;
namespace { // 원문 입력만 읽고 부모의 기대 분기는 계산하지 않는다.
std::vector<std::uint32_t> FenceTypeNumbers(std::string_view text) { // 입력 DWORD/현재 전역을 십진수 그대로 읽는다.
    std::vector<std::uint32_t> values; // 원문 열 순서의 명시 입력이다.
    for (const auto& word:Split(text,',')) values.push_back(Number(word));return values; // 각 열의 모든 비트를 유지한다.
}
}

// 두 부모의 정상 반환/진단 중단과 복귀 뒤 raw/전역 변경을432행 전체와 비교한다.
TEST_CASE(FenceTypeParent_NativeFullParents432) {
    std::size_t count{}; // 원문 고유 입력 행 수다.
    for (const auto& row:LoadFixture(NETSTORM_FENCE_TYPE_PARENT_FIXTURE).rows) { // 저장한 최초 원문만 읽으며 게임/PE를 실행하지 않는다.
        CHECK(row.size()==6);const auto input=FenceTypeNumbers(row[0]);CHECK(input.size()==6);std::array<std::uint8_t,50> raw{};std::uint32_t checking=input[1],type=input[3];std::string trace,outcome="return";
        for (std::size_t index=0;index<raw.size();++index) raw[index]=static_cast<std::uint8_t>(index*17+3); // 무관한49BYTE 쓰기까지 검출하는 표식이다.
        raw[10]=static_cast<std::uint8_t>(input[2]);FenceTypeParentHooks hooks; // 선택된 효과만 원문 장치와 동일하게 기록한다.
        hooks.assertion=[&](std::string_view expression,std::string_view file,std::uint32_t line) {
            CHECK(expression=="fence.getType() == sunFenceType" && file=="\\Ns\\O\\Fence.cpp");trace="D:66656e63652e676574547970652829203d3d2073756e46656e636554797065:5c4e735c4f5c46656e63652e637070:"+std::to_string(line);
            if (!input[5]) throw std::runtime_error("명시 진단 중단");if (input[4]==1) { raw[10]=static_cast<std::uint8_t>(type);checking=0; }if (input[4]==2) { type=(type&0xffffff00U)|raw[10];checking=0; }
        }; // 진단 복귀 뒤 checking은 재검사하지 않고 같은 슬롯 BYTE와 전역 DWORD를 다시 읽는다.
        hooks.body=[&](Sid sid) { CHECK(sid==Sid{50});if (!trace.empty()) trace+=';';trace+=(input[0]==0 ? "B:50" : "F:50"); };
        try { RunFenceTypeParent(raw,Sid{50},type,RawCheckingFlag{checking},input[0]==0 ? FenceTypeOperation::Rebuild : FenceTypeOperation::Refresh,hooks); }
        catch (const std::runtime_error& error) { if (std::string_view(error.what())!="명시 진단 중단") throw;outcome="assertion"; }
        CHECK((trace.empty() ? "-" : trace)==row[1]);CHECK(Hex(raw)==row[2]);CHECK(checking==Number(row[3]));CHECK(type==Number(row[4]));CHECK(outcome==row[5]);++count;
    }
    CHECK(count==432);
}

// 같은 실제 풀의 진단 복귀가 타입을 고치면 두 기존 자식이 동일 SID를 처리한다.
TEST_CASE(FenceTypeParent_DiagnosticReturnUsesActualChildrenAndSameRaw) {
    Scene scene(OriginalEdition::Patch1078);const auto sid=kSource;std::uint32_t type=136,checking=0x80000000U;std::vector<std::uint32_t> order; // 독립 명시 입력이며 전투/주문 시전은 실행하지 않는다.
    FenceRebuildHooks rebuildHooks;rebuildHooks.neighbor=[&](Sid target,std::uint32_t direction) { CHECK(target==sid);order.push_back(direction);return Sid{}; };RawFenceRebuild rebuild(scene.pool,rebuildHooks);
    FenceLifetimeHooks lifetimeHooks;lifetimeHooks.neighbor=[&](Sid target,std::uint32_t direction,bool allied) { CHECK(target==sid && allied);order.push_back(direction);return Sid{}; };lifetimeHooks.unpop=[&](Sid target,std::uint32_t flags) { CHECK(target==sid && flags==0);order.push_back(8); };lifetimeHooks.repop=[&](Sid target,std::uint32_t flags) { CHECK(target==sid && flags==0x40);order.push_back(9); };RawFenceLifetime lifetime(scene.pool,lifetimeHooks);
    for (const auto operation:{FenceTypeOperation::Rebuild,FenceTypeOperation::Refresh}) { // 두 dispatcher가 서로 다른 기존 자식으로 들어가는 순서를 비교한다.
        auto raw=scene.pool.AllocatedBytes(sid);raw[10]=128;raw[11]=raw[40]=0;order.clear();FenceTypeParentHooks hooks;
        hooks.assertion=[&](auto,auto,std::uint32_t line) { CHECK(line==(operation==FenceTypeOperation::Rebuild ? 0x60fU : 0x602U));order.push_back(99);scene.pool.AllocatedBytes(sid)[10]=136; };
        hooks.body=[&](Sid target) { if (operation==FenceTypeOperation::Rebuild) rebuild.Rebuild(target);else lifetime.Refresh(target); };
        RunFenceTypeParent(raw,sid,type,RawCheckingFlag{checking},operation,hooks);
        CHECK(order==(operation==FenceTypeOperation::Rebuild ? std::vector<std::uint32_t>{99,0,2,4,6} : std::vector<std::uint32_t>{99,0,2,4,6,0,2,4,6,8,9}));
    }
}

// 진단/자식은 선택될 때만 필수이며 물리 슬롯 오류는 역참조 전에 중단한다.
TEST_CASE(FenceTypeParent_SelectedBoundariesAndDwordTypeWidth) {
    std::array<std::uint8_t,50> raw{};std::uint32_t type=136,checking{}; // 원문과 달리 임의 주소를 역참조하지 않는 호스트 경계다.
    RunFenceTypeParent(raw,Sid{0},type,RawCheckingFlag{checking},FenceTypeOperation::Rebuild,{});checking=1;bool rejected{};
    try { RunFenceTypeParent(raw,Sid{0},type,RawCheckingFlag{checking},FenceTypeOperation::Rebuild,{}); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
    raw[10]=136;rejected=false;try { RunFenceTypeParent(raw,Sid{0},type,RawCheckingFlag{checking},FenceTypeOperation::Refresh,{}); } catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
    rejected=false;try { RunFenceTypeParent(std::span(raw).first(10),Sid{},type,RawCheckingFlag{checking},FenceTypeOperation::Rebuild,{}); } catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
    type=0xffffff88U;checking=0;FenceTypeParentHooks hooks;std::uint32_t calls{};hooks.body=[&](Sid sid) { CHECK(sid.value==0);++calls; };RunFenceTypeParent(raw,Sid{},type,RawCheckingFlag{checking},FenceTypeOperation::Refresh,hooks);CHECK(calls==0);type=136;RunFenceTypeParent(raw,Sid{},type,RawCheckingFlag{checking},FenceTypeOperation::Refresh,hooks);CHECK(calls==1); // 슬롯 BYTE는 zero-extend하며 전역 DWORD 전체와 비교한다.
}
