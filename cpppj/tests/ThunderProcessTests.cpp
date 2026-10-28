// 번개를 실제 World/Host/Form에 붙여 자연 종료·중단·부모 삭제의 단일 수명을 검사한다.
#include "DudeSpawnSupport.h"
#include "client/ThunderProcess.h"
#include "client/RawSurfaceWorld.h"
#include "o/RawProcessSummary.h"
#include <algorithm>
#include <array>
#include <string>

namespace { // 입력 팔레트 장치만 명시하고 객체/프로세스 계산은 실제 공통 모듈을 쓴다.
using namespace netstorm::client;using namespace netstorm::o; // 검사 대상 제품의 타입을 직접 사용한다.
// 본래 지면/다리 TYPE를 공유하는 실제 World 처리기를 구성한다.
BridgeConnectState ThunderLinks() { BridgeConnectState value;value.bridgeType=82;value.islandType=94;value.stalagType=95;value.connectorType=155;value.noIslandType=157;return value; }
// 벽시계/표시 모드/팔레트만 외부 입력이며 Kernel과 물리 Form은 실제 제품이다.
struct ThunderScene {
    std::shared_ptr<RawSurfaceStorage> storage=MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game); // 같은 실제188TYPE/팔레트 자산이다.
    RawSurfaceWorld world{OriginalEdition::Patch1078,storage->types,storage->frames,storage->shapes,ThunderLinks(),97,nullptr,storage}; // 같은 물리 Pool/Host의 짧은 대여다.
    RawWorldCore core=world.Core(); // 각 모듈이 사용하는 실제 참조만 빌린다.
    std::array<ScreenColor,256> palette{};double wall{};bool full{};unsigned reads{},applies{},restores{},destructs{};ProcessId removed{}; // 순수 장치 입력과 효과 관측이다.
    // 현재 화면의 같은 부분 팔레트 장치를 효과에 전달한다.
    PaletteFlashHooks Hooks() {
        return {[this] { return wall; },[this] { return full; },[this](unsigned start,unsigned count,ScreenColor* destination) { ++reads;std::copy_n(palette.data()+start,count,destination); },
            [this](unsigned,unsigned,ScreenColor*) { ++applies; },[this] { ++restores; }};
    }
    // 실제 타입 생성자가 초기화한 void 다리를 부모로 사용한다. 별도 공간 Pop은 필요하지 않다.
    Sid Parent() { return core.factory.Create(82,2); }
    Sid Head(Sid parent) const { const auto raw=core.pool.Slot(parent);return Sid{static_cast<std::uint16_t>(raw[6]|(std::uint16_t(raw[7])<<8))}; } // 실제 부모+6 WORD를 읽어 별도 종속 목록을 만들지 않는다.
    // 실제 TYPE55·flags10으로 등록하며 초기화 직전의 원문 장치 수명도 검사한다.
    ThunderProcess* Attach(Sid parent) {
        auto process=std::make_unique<ThunderProcess>(full,Hooks(),[this](ProcessId id) { ++destructs;removed=id; });CHECK(reads==0);
        return static_cast<ThunderProcess*>(core.processes.Attach(std::move(process),kThunderProcessType,parent,0x10));
    }
};
}

TEST_CASE(ThunderProcess_RealFormNaturalEndAndSummary) {
    ThunderScene scene;const auto parent=scene.Parent();auto* process=scene.Attach(parent);CHECK(process!=nullptr);if (!process) return;
    const auto form=process->Form();const auto id=scene.core.kernel.Find(*process);CHECK(id!=0 && scene.reads==2);CHECK(process->Parent()==parent);CHECK(scene.core.pool.Slot(form)[10]==55);
    CHECK(SquidForms(scene.core.pool).Parent(form)==parent);CHECK(scene.Head(parent)==form);
    std::string summary;ProcessSummaryHooks hooks;
    hooks.slot=[&](auto slot) { return reinterpret_cast<ProcessSummaryHandle>(scene.core.kernel.Get(slot)); };
    hooks.form=[](auto token) { return static_cast<SquidProcess*>(reinterpret_cast<BaseProcess*>(token))->Form().value; };
    hooks.parent=[](auto token) { return std::int32_t(static_cast<SquidProcess*>(reinterpret_cast<BaseProcess*>(token))->Parent().value); };
    hooks.open=[](auto path,auto mode) { CHECK(path=="procSumm.txt" && mode=="wb");return ProcessSummaryHandle{1}; };
    hooks.write=[&](auto,std::string_view row) { summary+=row; };hooks.close=[](auto) {};
    WriteRawProcessSummary(scene.core.pool,scene.core.types,hooks);CHECK(summary.find(std::string(ProcessSummaryTypeName(55)))!=std::string::npos);
    scene.wall=1.0;scene.core.kernel.RunFrame();CHECK(scene.core.kernel.Get(id)==nullptr);CHECK(scene.destructs==1 && scene.removed==id);CHECK(scene.applies==0 && scene.restores==0);
    CHECK(scene.core.pool.Slot(form)[11]&1);CHECK(scene.Head(parent).value==0);
}

TEST_CASE(ThunderProcess_RealFormCancelRestoresCurrentScreenOnce) {
    ThunderScene scene;scene.full=true;const auto parent=scene.Parent();auto* process=scene.Attach(parent);CHECK(process!=nullptr);if (!process) return;
    const auto form=process->Form();scene.core.kernel.RunFrame();CHECK(scene.applies==1 && scene.restores==0);
    scene.core.processes.Kill(*process,0x10);CHECK(scene.destructs==1 && scene.restores==1);CHECK(scene.core.kernel.Size()==0);CHECK(scene.core.pool.Slot(form)[11]&1);CHECK(scene.Head(parent).value==0);
}

TEST_CASE(ThunderProcess_ParentDestroyReleasesSameProcess) {
    ThunderScene scene;scene.full=true;const auto parent=scene.Parent();auto* process=scene.Attach(parent);CHECK(process!=nullptr);if (!process) return;
    const auto form=process->Form();const auto id=scene.core.kernel.Find(*process);scene.core.destroy.Destroy(parent,0x10,scene.core.processes.Hooks());
    CHECK(scene.core.kernel.Get(id)==nullptr);CHECK(scene.destructs==1 && scene.restores==1);CHECK((scene.core.pool.Slot(parent)[11]&1) && (scene.core.pool.Slot(form)[11]&1));
}
