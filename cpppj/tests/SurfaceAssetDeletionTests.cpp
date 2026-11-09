// 같은 Client 저장소의 비MP 자산 삭제를 World/Session/송신 없이 검사한다.
#include "DudeSpawnSupport.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientRandomGlobals.h"
#include "client/ClientSpGlobals.h"
#include "client/RawSurfaceWorld.h"
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace { // 명시 물리 입력과 실제 장기 원문 전역만 소유한다.
using namespace netstorm::client;using namespace netstorm::o; // 제품의 같은 타입을 사용한다.

// 선택하지 않아야 하는 생성/통신/시각 효과는 즉시 실패로 드러낸다.
template<class Result,class... Args> std::function<Result(Args...)> DeletionForbidden() {
    return [](Args...)->Result { throw std::logic_error("비MP 삭제 검사에서 선택 금지된 효과"); };
}

// 실제 raw DWORD만 little endian으로 써 가상 선택/프레임 입력을 만든다.
void DeletionPut(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value) {
    for (std::size_t i=0;i<4;++i) raw[offset+i]=static_cast<std::uint8_t>(value>>(i*8)); // 주위 바이트를 보존한다.
}

struct DeletionScene {
    std::shared_ptr<ClientGameplayGlobals> gameplay{std::make_shared<ClientGameplayGlobals>()}; // 현재 권한/선택/finder owner다.
    std::shared_ptr<RawSurfaceStorage> storage{MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game,gameplay->Authority(),gameplay->GameAux())}; // 원본 자산의 같은 물리다.
    std::shared_ptr<ClientRandomGlobals> random{std::make_shared<ClientRandomGlobals>()}; // 장기 원문 게임 난수다.
    std::shared_ptr<ClientSpGlobals> sp{std::make_shared<ClientSpGlobals>(random,[] { return 321; })}; // 초기화하지 않은 실제 SP 저장소다.
    std::optional<std::string> detail; // 원문 진단 상세를 보유하며 저장소보다 먼저 수동 반납하지 않는다.
    std::function<void()> onTrace; // 실제 공통 삭제 로그에서 현재 필드 재조회 순서를 관찰한다.
    std::uint32_t traces{},clears{},remembered{},legacyDeletes{}; // 선택된 실제 장치 호출 수다.

    // 실제 장치를 연결만 하며 World/Player 대여나 원본 게임 실행은 하지 않는다.
    DeletionScene() {
        storage->BindTransmissionDevices({DeletionForbidden<void,std::span<std::uint8_t>>(),DeletionForbidden<bool,std::int32_t>(),DeletionForbidden<void,std::int32_t,std::span<std::uint8_t>>()});
        storage->BindSerializationGlobals(std::make_shared<FortSerializationGlobals>());
        storage->BindDisplayDiagnostics(detail,[](auto,auto,auto) { throw std::logic_error("유효 삭제 입력에서 원문 단언 발생"); });
        storage->BindGraphTrace([this](auto,std::string_view) { ++traces;if (onTrace) onTrace(); });storage->EnablePatchDestroyDiagnostics();
        RawSurfaceDisplayBindings display;display.view=[] { return SquidDisplayView{0,0,65536,{0,0,1024,768}}; };display.suppressed=[] { return true; };display.invalidate=DeletionForbidden<void,SquidDisplayRect,std::uint32_t>();storage->BindDisplayDevices(std::move(display));
        storage->BindRewardDevices({std::shared_ptr<ScrambledSpStore>(sp,&sp->Store()),std::shared_ptr<const std::uint32_t>(gameplay,&gameplay->world.editor),DeletionForbidden<void>(),DeletionForbidden<void,std::uint32_t,std::int32_t>()});
        storage->BindPostPopDevices({DeletionForbidden<void,std::uint32_t,Sid,bool>(),[this] { return gameplay->player.battleOwner; },DeletionForbidden<void>()});
        storage->BindDeletionDevices({[this](auto,auto,auto) { ++remembered; },DeletionForbidden<void,Sid>()});
        storage->BindProcessDevices({[this] { return gameplay->selection.selected; },[this] { ++clears;gameplay->selection.selected=0; }});
        storage->BindDamageableDevices({std::shared_ptr<ContainedFinderState>(gameplay,&gameplay->finder),std::shared_ptr<const std::uint32_t>(gameplay,&gameplay->FortKeepGenus()),DeletionForbidden<void,Sid>(),DeletionForbidden<void,Sid>(),DeletionForbidden<void,const DamageableSoundEvent&>(),DeletionForbidden<void,float,float,std::uint32_t,std::uint16_t>(),DeletionForbidden<void,Sid,std::int32_t,bool>(),DeletionForbidden<void,float,float,std::uint32_t,std::uint32_t>()});
        storage->BindResourceDevices({[this]()->GameRandom& { return random->Game(); },DeletionForbidden<void,Sid>(),DeletionForbidden<float,Sid,std::uint32_t,std::uint32_t,float>(),DeletionForbidden<void,Sid>(),[this](auto,auto)->bool { ++legacyDeletes;throw std::logic_error("World 없는 삭제의 Session 우회"); },DeletionForbidden<void,Sid,std::uint32_t>()});
        storage->links.bridgeType=82;storage->links.islandType=94;storage->links.stalagType=95;storage->links.connectorType=155;storage->links.noIslandType=157;
    }
    ~DeletionScene() { storage.reset(); } //장기Host/장치를모든검사관찰기보다먼저반납한다.

    // 실제 TYPE에서 지정 VT의 생성자를 찾아 같은 Factory로 void/abstract 자산을 만든다.
    Sid Asset(std::uint32_t table) {
        for (std::uint32_t number=70;number<storage->types.size();++number) { // 미등록 생성자는 건너뛰고 실제 표의 최종 VT만 비교한다.
            const auto& type=storage->types[number];
            std::uint32_t current{};try { current=storage->Factory().ConstructorVtable(type); }catch (const std::logic_error&) { continue; } //실제사용불가생성자항목은이VT의자산후보가아니다.
            if (current!=table) continue;
            const auto sid=storage->CreateAsset(number,2);auto raw=storage->pool.AllocatedBytes(sid);raw[40]|=9U;return sid;
        }
        throw std::logic_error("실제 TYPE 표에 필요한 삭제 VT가 없음: "+std::to_string(table));
    }
};
}

// 공통·피해가능·다리·섬·noIsland·간헐천·너겟은 같은 실제 장기 Pre/Post로 반납한다.
TEST_CASE(SurfaceAssetDeletion_NoWorldActualParentsBalanceAndRelease) {
    for (const auto table:{0x503360U,0x509930U,0x5034c8U,0x506d98U,0x506f08U,0x50da88U,0x50dc30U,0x503d80U,0x50a748U,0x502c38U,0x502cf0U,0x502db0U}) { //공중3종을제외하고실제41f910점프네표의공통부모까지새물리에서검사한다.
        DeletionScene scene;const auto sid=scene.Asset(table);const auto type=scene.storage->pool.Slot(sid)[10];const auto free=scene.storage->pool.FreeCount();
        CHECK(scene.storage->DestroyAsset(sid,0x10));CHECK((scene.storage->pool.Slot(sid)[11]&3U)==3U);CHECK(scene.storage->pool.Slot(sid)[10]==type && scene.storage->pool.FreeCount()==free+1);
        CHECK(scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);CHECK(scene.traces==1 && scene.legacyDeletes==0 && !scene.storage->Borrowed() && scene.storage->Generation()==0);
    }
}

// 첫 로그 효과가 VT를 바꿔도 실제 Pre/Post가 현재 VT를 읽으며 선택 DWORD 상위 비트를 자르지 않는다.
TEST_CASE(SurfaceAssetDeletion_CurrentVtableAfterTraceAndFullSelection) {
    DeletionScene scene;const auto sid=scene.Asset(0x50da88U);scene.gameplay->selection.selected=0x10000U|sid.value;
    scene.onTrace=[&] { DeletionPut(scene.storage->pool.AllocatedBytes(sid),0,0x503360U);DeletionPut(scene.storage->pool.AllocatedBytes(sid),36,0x12345678U); };
    CHECK(scene.storage->DestroyAsset(sid,0x10));CHECK(scene.clears==0 && scene.remembered==0 && scene.legacyDeletes==0);CHECK(scene.gameplay->selection.selected==(0x10000U|sid.value));CHECK(scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);
}

// 공통 Pre의 선택 기억과 해제를 실제 순서로 한 번 실행하고 후행 선택 비교는 새 값을 읽는다.
TEST_CASE(SurfaceAssetDeletion_CurrentSelectionClearedOnce) {
    DeletionScene scene;const auto sid=scene.Asset(0x503360U);scene.gameplay->selection.selected=sid.value;
    CHECK(scene.storage->DestroyAsset(sid,0x10));CHECK(scene.clears==1 && scene.remembered==1 && scene.gameplay->selection.selected==0 && scene.legacyDeletes==0);
}

// 미연결 고유 Pre는 앞선 dead/로그/깊이 효과를 남기며 공통 성공이나 Session 우회로 처리하지 않는다.
TEST_CASE(SurfaceAssetDeletion_UnrestoredSpecialPrePreservesPartialEffects) {
    DeletionScene scene;const auto sid=scene.Asset(0x5076b8U);const auto free=scene.storage->pool.FreeCount();bool rejected{}; // 복원한 벌룬 대신 실제 미연결44a4d0 부모를 선택하며 실패 단언은 유지한다.
    try { scene.storage->DestroyAsset(sid,0x10); }catch (const std::logic_error& error) { rejected=std::string_view(error.what()).find("전용 삭제 몸체 미연결")!=std::string_view::npos; }
    CHECK(rejected && (scene.storage->pool.Slot(sid)[11]&3U)==2U);CHECK(scene.traces==1 && scene.legacyDeletes==0 && scene.storage->pool.FreeCount()==free);CHECK(scene.storage->BaseDestroy().PreDepth()==1 && scene.storage->BaseDestroy().PostDepth()==0);
}

// 실제 장기 가상10의 다리 거부는 dead/깊이/로그/선택/풀에 아무 효과도 만들지 않는다.
TEST_CASE(SurfaceAssetDeletion_BridgeVirtualRejectionLeavesCurrentState) {
    DeletionScene scene;const auto sid=scene.Asset(0x5034c8U);auto raw=scene.storage->pool.AllocatedBytes(sid);raw[40]=0;raw[30]=0;
    scene.gameplay->Authority()->boss=true;scene.storage->graph.records[0].surfaces=5;const auto codes=scene.storage->frames[raw[10]].Codes();std::uint32_t frame{};
    while (frame<codes.size() && !(codes[frame].flags&0x40U)) ++frame; // 실제 SHP의 거부 프레임을 찾는다.
    CHECK(frame<codes.size());if (frame>=codes.size()) return;DeletionPut(raw,36,frame);const auto free=scene.storage->pool.FreeCount();const auto state=raw[11];
    CHECK(!scene.storage->DestroyAsset(sid,0x10));CHECK(scene.storage->pool.Slot(sid)[11]==state && scene.storage->pool.FreeCount()==free && scene.traces==0 && scene.legacyDeletes==0);CHECK(scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);
}

// 실제삭제대여가없으면물리World대여증빙이아직살아있어도반납된Session장치로우회하지않는다.
TEST_CASE(SurfaceAssetDeletion_PhysicalLeaseWithoutDeletionRuntimeUsesLongBody) {
    DeletionScene scene;auto lease=scene.storage->AcquirePrepared();CHECK(scene.storage->Borrowed());const auto sid=scene.Asset(0x503360U);
    CHECK(scene.storage->DestroyAsset(sid,0x10));CHECK((scene.storage->pool.Slot(sid)[11]&3U)==3U && scene.legacyDeletes==0);CHECK(scene.storage->BaseDestroy().PreDepth()==0 && scene.storage->BaseDestroy().PostDepth()==0);
}

// 실제Carrier는Damageable/공통Pre뒤현재내용물을조회하고선택한보충실패의부분효과를보존한다.
TEST_CASE(SurfaceAssetDeletion_CarrierReplenishesAfterActualParent) {
    DeletionScene scene;std::uint32_t calls{};Sid root{};
    scene.storage->BindCarrierReplenishment([&] { ++calls;CHECK((scene.storage->pool.Slot(root)[11]&3U)==2U && scene.storage->BaseDestroy().PreDepth()==0);throw std::runtime_error("실제 보충 선택 실패"); });
    root=scene.Asset(0x503d80U);const auto child=scene.storage->pool.Allocate(2);auto raw=scene.storage->pool.AllocatedBytes(child);raw[10]=6;raw[11]=4;raw[4]=raw[5]=0;raw[20]=1;auto parent=scene.storage->pool.AllocatedBytes(root);parent[6]=static_cast<std::uint8_t>(child.value);parent[7]=static_cast<std::uint8_t>(child.value>>8);scene.gameplay->Boss()=true;
    bool failed{};try { scene.storage->DestroyAsset(root,0x10); }catch (const std::runtime_error& error) { failed=std::string_view(error.what())=="실제 보충 선택 실패"; }
    CHECK(failed && calls==1 && scene.legacyDeletes==0 && scene.storage->BaseDestroy().PostDepth()==0);CHECK((scene.storage->pool.Slot(root)[11]&3U)==2U && !(scene.storage->pool.Slot(child)[11]&1U));
}
