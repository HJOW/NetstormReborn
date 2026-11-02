// 실제 Session/controlled 메뉴에서 사용자 지정 CHEAT 편집 전용 규칙과 각 원문 추가 조건을 확인한다.
#include "client/ClientGameplayGlobals.h"

// 일반/오벨리스크 메뉴를 editor/special 네 조합으로 작성하고 실제 화면 항목 조회를 대조한다.
TEST_CASE(CampaignEditorCheatMenu_ActualSessionCommonAndGemVisibility) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto globals=std::make_shared<ClientGameplayGlobals>();auto services=external.Make();services.gameplayGlobals=globals;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"editor-cheat-policy",std::move(services));session.Load();const auto core=session.Core();
    const auto gem=session.World().Create(112,2);CHECK(Get(core.pool.Slot(gem),0)==0x50a748U); // 실제 오벨리스크 생성자 VT이며 노출 검사만 하므로 Pop/보상/전환 명령은 실행하지 않는다.
    ActualMenuUi ui(assets);ui.currentSession=&session;std::unique_ptr<RawCampaignObjectMenus> chain;
    for (const bool editor:{false,true}) for (const bool special:{false,true}) for (const bool variant:{false,true}) { // 같은 현재 Session의 실제 두 모드를 바꿔 공통/파생 메뉴8개를 작성한다.
        globals->world.editor=editor;globals->player.special=special;
        ui.menus->Open({320,240},[&](const auto& widgets,const auto& controls,const auto& raw) {
            widgets.BeginVertical();auto hooks=ui.Hooks(session,widgets,controls,raw);
            BindObjectMenuControls(hooks.object,controls,raw,ui.configuration,[](std::string_view text) { CHECK(text=="SetOwner");return 0x60000001U; }); // 검사 정적 문자열 한 개의 명시 토큰이며 Tell/명령은 실행하지 않는다.
            hooks.variants.item=[&controls](auto label,auto command,auto parameter,auto include,auto enabled) { controls.AddText(label,command,parameter,include!=0,enabled!=0,false); };
            chain=session.MakeObjectMenus(std::move(hooks));if (variant) chain->OpenVariant(gem,0x462390);else chain->OpenObject(gem);
        });
        CHECK(ui.menus->Point("About").has_value());
        if (!editor) {
            for (const auto label:{"CHEAT Hurt Me!","CHEAT Destroy","CHEAT Make Yours","CHEAT Set Owner","CHEAT Dig Up"}) CHECK(!ui.menus->Point(label).has_value()); // special이 켜져도 실제 전투 메뉴의 CHEAT 항목은 모두 없어야 한다.
        } else {
            CHECK(ui.menus->Point("Destroy").has_value() && ui.menus->Point("CHEAT Set Owner").has_value());
            CHECK(ui.menus->Point("CHEAT Dig Up").has_value()==(variant && special)); // Edit라도 오벨리스크 파생/special이라는 원문 추가 조건을 만족해야 한다.
            if (variant) CHECK(ui.menus->Point("Turn To Spell!").has_value());
        }
        ui.menus->Close();chain.reset();ui.menus->CleanupFrame();
    }
    ui.currentSession=nullptr;
}
