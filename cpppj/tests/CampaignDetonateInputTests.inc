// 실제 Session·보호 자산·Gump 클릭·입력 큐·가상 부모를 함께 확인한다.
#include "client/DetonateInput.h"

// 직접 구성한 주문은 제네레이터/공중유닛 생산을 우회하는 게임 성공 증거가 아닌 명시 단위 입력이다.
TEST_CASE(CampaignDetonateInput_ActualSpellMenuQueueAndUnchangedWorld) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();std::uint32_t requests{};services.carrierRequest=[&](auto,auto) { ++requests; }; // 실제 요청이 생기면0유지 검사가 실패한다.
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();
    const auto spell=session.World().Create(114,2);core.owner.Set(spell,1);session.World().Pop(spell,10.5F,10.5F,0);CHECK(Get(core.pool.Slot(spell),0)==0x502370U);CHECK(!session.Editor());CHECK(!core.processes.FindEvent(spell,0));
    ActualMenuUi ui(assets);ui.currentSession=&session;std::unique_ptr<RawCampaignObjectMenus> chain;
    ui.menus->Open({320,240},[&](const auto& widgets,const auto& controls,const auto& raw) {
        widgets.BeginVertical();auto hooks=ui.Hooks(session,widgets,controls,raw);hooks.variants.item=[&controls](auto label,auto command,auto parameter,auto include,auto enabled) { controls.AddText(label,command,parameter,include!=0,enabled!=0,false); };chain=session.MakeObjectMenus(std::move(hooks));chain->OpenVariant(spell,0x420b30);
    });
    const auto point=ui.menus->Point("Detonate!");CHECK(point.has_value());if (!point) return;ui.Click(*point);const auto event=ui.commands.Pop(false);CHECK(event.code==kDetonateBombCommand && event.y==spell.value);
    const auto before=Hex(core.pool.Slot(spell));const auto sp=session.Sp(1);std::uint32_t objectCalls{};
    CHECK(DispatchDetonateInput(event,[&](Sid target,auto code) { CHECK(target==spell);++objectCalls;CHECK(session.CommandObject(target,code)==0); }));
    CHECK(objectCalls==1 && requests==0);CHECK(Hex(core.pool.Slot(spell))==before && session.Sp(1)==sp);CHECK(!core.processes.FindEvent(spell,0));ui.menus->Close();chain.reset();
}
