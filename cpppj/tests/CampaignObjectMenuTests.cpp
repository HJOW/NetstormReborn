// 실제 세션의 사제/Carrier/객체/Player 메뉴를 같은 원본 Gump 입력과 커서 취득에 연결한다.
#include "DudeSpawnSupport.h"
#include "client/RawCampaignSession.h"
#include "client/RawCampaignCursor.h"
#include "client/RawButtonControls.h"
#include "client/RawMenuControls.h"
#include "client/RawKnowledgeMenu.h"
#include "client/QuickHelpHtml.h"
#include "client/RawUserHighlight.h"
#include "o/OriginalText.h"
#include "o/RawContentPreviewProcess.h"
#include <cmath>

using namespace netstorm::client;
namespace {
// 보호 자산의 실제 세션을 설치하기 위한 장치/번역만 명시한다. 게임 계산/생성 결과를 공급하지 않는다.
struct MenuServices {
    SoundList names{OriginalEdition::Patch1078}; // 실제 같은 이름표를 세션보다 오래 유지한다.
    std::array<std::uint8_t,4> you{'Y','o','u',0}; // 원본 부모의 명시 영어 번역 입력이다.
    Sid selected{}; // 이 headless UI의 현재 선택이며 미리보기 삭제도 같은 값을 조회한다.
    // 선택되는 장치/설정 경계만 공급하며 미복원 하위를 성공으로 채우지 않는다.
    RawCampaignSessionServices Make() {
        names.Initialize();RawCampaignSessionServices result;result.soundNames=&names;result.unixSeconds=[] { return 946684800; };
        result.production.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); }; //현재 지식 카드가 조회하는 실제 Deck의 미작성 바이트 입력이다.
        result.sound.wallSeconds=[] { return 100.0; };result.sound.play=[](SoundHandle sound,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { return sound; };
        result.sound.playLoopAt=[](float,float,SoundHandle current,SoundHandle sound) { return current ? current : sound; };
        result.sound.playOnceAt=[](float,float,SoundHandle sound,std::uint32_t,std::uint32_t) { return sound; };
        result.sound.stop=[](SoundHandle) {};result.sound.isPlaying=[](SoundHandle) { return 0U; };
        result.install.translateYou=[this] { return std::span<const std::uint8_t>(you); };
        result.install.setConfig=[](std::string_view,std::span<const std::uint8_t>) {};result.install.refreshWindowTitle=[] {};result.install.resetCamera=[] {};
        result.ownership.refreshProduction=[] {};result.ownership.themeMode=[] { return 0U; };
        result.assets.postPop.playImpactSound=[](std::uint32_t) {};result.assets.queueCommand=[](std::uint32_t,std::int32_t,std::int32_t) {};
        result.priest.shield.noticeSound=[](const PriestShieldNotice&) {};result.priest.shield.translate=[](std::string_view key) { return std::string(key); };result.priest.shield.tell=[](std::string_view) {};
        result.deletion.selected=[this] { return selected; };
        result.deletion.emit=[this](const SquidDestroyEvent& event) { if (event.effect==SquidDestroyEffect::ClearSelection) { selected={};return; }throw std::logic_error("메뉴 검사 다른 실제 삭제 효과 미연결"); };
        result.player.log=[](std::uint32_t,std::string_view) {};return result;
    }
};
// 실제 서식의 인자 순서만 처리하는 명시 영어 UI 경계다. 원본 게임 조건은 계산하지 않는다.
std::string PrintMenu(std::string_view pattern,std::span<const PriestMenuArgument> values) {
    std::string result;std::size_t argument=0;
    // 원본 고정 %s/%d/%%의 현재 인자만 순서대로 출력한다.
    for (std::size_t index=0;index<pattern.size();++index) {
        if (pattern[index]!='%') { result+=pattern[index];continue; }
        if (++index>=pattern.size()) throw std::logic_error("메뉴 검사 서식 끝");if (pattern[index]=='%') { result+='%';continue; }
        if (argument>=values.size()) throw std::logic_error("메뉴 검사 인자 부족");
        if (pattern[index]=='s') result+=std::get<std::string>(values[argument++]);else if (pattern[index]=='d') result+=std::to_string(std::get<std::int32_t>(values[argument++]));else throw std::logic_error("메뉴 검사 서식 종류");
    }
    CHECK(argument==values.size());return result;
}
// 동일 보호 글꼴/등록표/입력 큐와 메뉴 작성기의 한 수명을 제공한다.
struct ActualMenuUi {
    const netstorm::test::dudespawn::DudeAssets& assets; // 실제 보호 TYPE/SHP/팔레트를 읽는 같은 입력이다.
    ConfigInterface configuration; // 같은보호VFS의설정치환몸체이며정보class문구에사용한다.
    FontStore fonts; // 원본 글리프와 메뉴 측정이 사용하는 실제 글꼴 캐시다.
    RawButtonControls buttons; // 기존 실제 공통 트리/생성/수명/입력이다.
    RawMenuControls* menus{}; // 공통 트리에 연결된 메뉴 처리기를 빌린다.
    RawCampaignSession* currentSession{}; // Form효과가매번조회하는현재실제Session이며메뉴를닫은뒤해제한다.
    InputQueue commands; // 실제 메뉴가 전달한 원본 정수 명령을 보관한다.
    InputEvent mouse{InputCode::kRelease,320,240}; // 현재 장치 입력의 명시 외부 경계다.
    std::array<std::uint8_t,100*48> chat{}; // 실제 Identity가 직접 빌리는 명시 Client Chat100 원문이다.
    std::array<std::uint32_t,3> permissions{}; // 송금/제안/해제의 현재 외부 허용 DWORD 입력이다.
    std::vector<std::string> tells; // 실제 건설 선택 관문이 요청한 Tell 키다.
    std::vector<std::string> infoTitles,infoLines; // 실제controlled제목/머리에전달한원문바이트의관찰이다.
    std::vector<std::array<std::uint32_t,3>> buildRows; // 실제 사제 작성기의 command/parameter/enabled 관찰이다.
    std::uint32_t cursor{},highlight{},address{}; // 현재 장치/강조 모드/Identity 주소 입력이다.
    // 창/게임 실행 없이 같은 자산과 실제 공통 UI 처리기를 준비한다.
    explicit ActualMenuUi(const netstorm::test::dudespawn::DudeAssets& source):assets(source),configuration([this](std::string_view path) { return assets.files.TryRead(path); }),fonts(source.files,"Arial"),buttons(OriginalEdition::Patch1078,{[] { return 0.0; },[this](std::uint32_t) { return mouse; },[] {},[this](std::uint32_t form,std::uint32_t flags) {
        if (!currentSession || form>65535U) throw std::logic_error("검사현재Form삭제호스트미연결");currentSession->GumpForms().Destroy(Sid{static_cast<std::uint16_t>(form)},flags);
    }}) {
        ConfigInterface::StartupOptions startup;startup.majorVersion=10;startup.minorVersion=78;startup.installDir=source.root.string();startup.cdDir=startup.installDir;configuration.Startup(startup); // 파일을저장하지않고실제설정치환문맥만설치한다.
        RawMenuHooks hooks;hooks.command=[this](std::uint32_t code,std::uint32_t id,std::uint32_t parameter) { commands.PushCommand(code,std::bit_cast<std::int32_t>(id),std::bit_cast<std::int32_t>(parameter)); };
        hooks.readInt=[](std::string_view,std::int32_t& value) { value=0; };hooks.sound=[](std::string_view) {};hooks.poll=[this](std::uint32_t) { return mouse; };hooks.cursor=[this](std::uint32_t value) { cursor=value; };
        hooks.objectForms=[this]() -> GumpFormHost& { if (!currentSession) throw std::logic_error("검사현재메뉴Form호스트미연결");return currentSession->GumpForms(); };
        menus=&buttons.EnableMenus(source.game,fonts,std::move(hooks));menus->SetPalette(Palette());
        const auto& type=source.game.Find("fortGump");const auto frame=source.game.FrameIndex(type,"A00",0);const auto metrics=source.game.Shapes().SquidMetrics(type.block,frame);
        buttons.SetControls({}, {},1024,768,{static_cast<std::uint32_t>(70+type.block),static_cast<std::uint32_t>(frame),metrics.width,metrics.height,ButtonColors(Palette())});
    }
    // 현재 보호 RGB를 원본 색표 계산기에 입력한다.
    PaletteColorTable Palette() const {
        std::array<std::uint32_t,256> logical{};
        // 원본 DWORD의 낮은 바이트부터 RGB를 쓴다.
        for (std::size_t index=0;index<logical.size();++index) { const auto color=assets.game.Palette().Color(static_cast<std::uint8_t>(index));logical[index]=color.red|(std::uint32_t(color.green)<<8)|(std::uint32_t(color.blue)<<16); }
        return BuildPaletteColorTable(logical,OriginalEdition::Patch1078);
    }
    // Caller UI가 실제 controlled 작성기와 동일한 수명으로 연결되는 입력이다.
    CampaignObjectMenuUi Hooks(RawCampaignSession& session,const MenuGumpWidgets& widgets,const MenuGumpControls& controls,const MenuGump& raw) {
        CampaignObjectMenuUi result;result.chat=chat;result.objectState.helpWord="About";
        result.objectState.localOwner=8;result.objectState.editor=true;result.objectState.special=true;result.objectState.chargeBlocked=false; // 잘못된 값형 사본을 두어도 실제 세션 getter가 원본 현재 값을 읽어야 한다.
        result.identity.address=[this] { return address; };result.identity.unknownName=[] { return std::string_view("SOMEONE"); };
        result.priest.format=result.carrier.format=result.object.format=PrintMenu;
        result.priest.description=result.carrier.description=[&session](std::uint32_t type) { return session.Core().types[type].description; };
        result.priest.tell=[this](std::string_view key) { tells.emplace_back(key); };
        result.player.translate=[](std::string_view key) { return std::string(key); };result.player.print=PrintMenu;
        result.info.translate=result.player.translate;result.info.description=result.priest.description;result.info.print=PrintMenu;
        result.info.substitute=[this](std::string_view text) { const auto utf8=DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()});const auto bytes=EncodeOriginalText(configuration.SubstituteLocal(utf8));return std::string(bytes.begin(),bytes.end()); };
        BindObjectMenuInfoControls(result.info,controls);const auto infoTitle=result.info.title,infoHeader=result.info.header;
        result.info.title=[this,infoTitle](std::string_view text) { infoTitles.emplace_back(text);infoTitle(text); };result.info.header=[this,infoHeader](std::string_view text) { infoLines.emplace_back(text);infoHeader(text); };
        result.player.allowSend=[this] { return permissions[0]!=0; };result.player.allowOffer=[this] { return permissions[1]!=0; };result.player.allowBreak=[this] { return permissions[2]!=0; };
        result.player.ignored=[](std::uint32_t) { return false; };result.player.whispered=[](std::uint32_t) { return std::uint8_t{}; }; // 명시 빈 Chat 보조 목록 입력이다.
        result.player.assertion=[](PlayerMenuAssertion) { throw std::logic_error("실제 메뉴 검사 Player 진단"); };
        BindPriestMenuControls(result.priest,widgets,controls,raw);BindCarrierMenuControls(result.carrier,controls);BindPlayerMenuControls(result.player,widgets,controls,raw);
        const auto item=result.priest.item;result.priest.item=[this,item](std::string_view label,std::uint32_t command,std::uint32_t parameter,bool enabled) { buildRows.push_back({command,parameter,enabled ? 1U : 0U});item(label,command,parameter,enabled); };
        result.object.itemCount=[&raw] { return raw.ItemCount(); };result.object.separator=[&controls] { controls.AddSeparator(); };
        result.object.item=[&controls](std::string_view label,std::uint32_t command,const ObjectMenuParameter& parameter) { if (!std::holds_alternative<std::uint32_t>(parameter)) throw std::logic_error("검사에서 선택하지 않은 정적 문자열");controls.AddText(label,command,std::get<std::uint32_t>(parameter)); };
        result.object.submenu=[&controls](std::string_view label,std::uint32_t child) { controls.AddSubmenu(label,child); };result.object.setConfig=[](std::string_view,std::string_view) {};
        return result;
    }
    // 실제 트리의 폴링/입력 처리기로 선택한다. 명령 callback을 직접 호출하지 않는다.
    void Click(ScreenPoint point) { mouse={InputCode::kLeftButton,point.x,point.y};buttons.Poll(mouse);buttons.Event(mouse); }
    // 커서 장치/화면 경계만 공급하며 실제 취득/배치 계산은 세션이 수행한다.
    CampaignCursorEffects CursorEffects() {
        CampaignCursorEffects effects;effects.lifecycle.highlight=[this](std::uint32_t mode) { highlight=mode; };effects.lifecycle.selectCursor=[this](std::uint32_t value) { cursor=value; };
        effects.lifecycle.closePopups=[this] { menus->Close(); };effects.lifecycle.translate=[](std::string_view key) { return std::string(key); };
        effects.lifecycle.measure=[this](std::string_view value,const std::array<std::uint32_t,8>&) { return std::array<std::int32_t,2>{fonts.Get(0).Measure(value),fonts.Get(0).Height()}; };effects.lifecycle.mouse=[this] { return ScreenPoint{mouse.x,mouse.y}; };
        effects.placement.gumpAt=[](std::int32_t,std::int32_t) { return 0U; };effects.placement.invalidate=[](ScreenRect,std::uint32_t) {};return effects;
    }
};
}
// 실제 생성된 건설 행의 현재 금액 관문과 같은 커서 취득, 기본 객체/Player 메뉴 및 실제 정수 명령 입력을 확인한다.
// 실제 Session 기술·커서 취소와 같은 Gump 생성/부착/원본 StyleText 그림·지연 수명을 함께 연결한다.
TEST_CASE(CampaignKnowledge_ActualSessionAndMenuTree) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());session.Load();ActualMenuUi ui(assets);ui.currentSession=&session;session.Cursor().SetEffects(ui.CursorEffects());
    ui.menus->PopupState().Mark(0x2000abc,"previous"); // 실제 표식의 이전 값을 덮고 전체80바이트를 지우는지 확인한다.
    session.AddTechnology(1,"sunArcher;windBattery");CHECK(ui.menus->OpenKnowledge(session,[](std::string_view key) { const auto bytes=EncodeOriginalText(key);return std::string(bytes.begin(),bytes.end()); },1));
    CHECK(ui.menus->PopupState().command==0x20000a0 && ui.menus->PopupState().Argument().empty());CHECK(std::all_of(ui.menus->PopupState().argument.begin(),ui.menus->PopupState().argument.end(),[](char value) { return value==0; }));
    CHECK(ui.menus->KnowledgeOpen() && ui.buttons.BlockingGumpOpen() && ui.menus->HasMenus());CHECK(ui.menus->Point("WIND").has_value() && ui.menus->Point("RAIN").has_value() && ui.menus->Point("THUN.").has_value() && ui.menus->Point("SUN").has_value());
    std::array<std::uint32_t,256> logical{};
    // 실제 보호 RGB를 기존 메뉴 명암 표 생성기에 전달한다.
    for (std::size_t index=0;index<logical.size();++index) { const auto color=assets.game.Palette().Color(static_cast<std::uint8_t>(index));logical[index]=color.red|(std::uint32_t(color.green)<<8)|(std::uint32_t(color.blue)<<16); }
    std::vector<std::uint8_t> pixels(640*480,0);ui.menus->Draw(pixels,640,480,ui.Palette(),BuildGumpShadeMaps(logical)); // 실제 보호 그림/글꼴/팔레트를 같은 픽셀 소비자에 보낸다.
    CHECK(std::any_of(pixels.begin(),pixels.end(),[](std::uint8_t value) { return value!=0; }));
    ui.menus->RequestClosePopups();CHECK(!ui.menus->KnowledgeOpen());ui.menus->CleanupFrame();CHECK(!ui.menus->HasMenus());ui.currentSession=nullptr;
}
// 실제 사제/Carrier/Player 계층과 건설 선택을 기존 공통 트리에서 확인한다.
// 보호 TYPE의 현재 도움말 값은 실제 무기 보충 표와 같은 요구 에너지 후처리를 읽는다.
TEST_CASE(CampaignHelp_ActualCurrentTypeInfo) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());
    const auto cannon=static_cast<std::uint32_t>(assets.game.TypeTable().Find("sunCannon"));bool rejected{};try { static_cast<void>(session.HelpTypeInfo(cannon)); }catch (const std::out_of_range&) { rejected=true; }CHECK(rejected);session.Load();
    const auto info=session.HelpTypeInfo(cannon);CHECK(info.theme==3 && (info.flags1&0x10U) && info.range==20 && info.damage==16 && info.cost==400 && info.classification=="Shooter");CHECK(session.HitPoints().TypeHitPoints(cannon)==600);CHECK(info.mana=="s");
    const auto& definition=assets.game.Find("sunCannon").definition;CHECK(!definition.String("mana").has_value()); // 명시 mana가 없어도 후처리의 실제+a0은 빈 문자열이 아니다.
    rejected=false;try { static_cast<void>(session.HelpTypeInfo(188)); }catch (const std::out_of_range&) { rejected=true; }CHECK(rejected);
}
// 실제 사제/Carrier/Player 계층과 건설 선택을 기존 공통 트리에서 확인한다.
TEST_CASE(CampaignObjectMenus_ActualChainAndBuildSelection) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;ActualMenuUi ui(assets);
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());session.Load();
    const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(priest.value);std::unique_ptr<RawCampaignObjectMenus> chain;
    ui.menus->Open({320,240},[&](const auto& widgets,const auto& controls,const auto& menus) { widgets.BeginVertical();chain=session.MakeObjectMenus(ui.Hooks(session,widgets,controls,menus));chain->OpenPriest(priest); });
    const auto knowledge=ui.menus->Point("View Netstorm Knowledge");CHECK(knowledge.has_value());CHECK(ui.menus->Point("About").has_value());CHECK(ui.menus->Point("Player").has_value());CHECK(!ui.menus->Point("Destroy").has_value());
    if (knowledge) ui.Click(*knowledge);const auto command=ui.commands.Pop(false);CHECK(command.code==session.PriestMenuSettings().knowledgeCommand && command.x!=0 && command.y==0);
    const auto target=session.PriestMenuSettings().dais;const auto row=std::find_if(ui.buildRows.begin(),ui.buildRows.end(),[&](const auto& value) { return value[0]==session.PriestMenuSettings().buildCommand && (value[1]>>16)==target; });
    CHECK(row!=ui.buildRows.end());if (row==ui.buildRows.end()) return;CHECK(((*row)[1]&0xffffU)==priest.value && (*row)[2]==1);
    session.SetSp(1,0);CHECK(!chain->SelectBuild((*row)[1]>>16,Sid{static_cast<std::uint16_t>((*row)[1])}));CHECK(ui.tells==std::vector<std::string>({"BuildNoBucks"}));CHECK(!session.CursorDropSettings().preview);
    session.SetSp(1,100000);session.Cursor().SetEffects(ui.CursorEffects());const auto before=session.Sp(1);CHECK(chain->SelectBuild((*row)[1]>>16,Sid{static_cast<std::uint16_t>((*row)[1])}));
    // 실제 Pool을 읽는 위치에서만 전체 DWORD의 유효 범위를 검사해 SID16으로 변환한다.
    const auto form=CursorPreviewSid(session.CursorDropSettings().preview);CHECK(form.value && session.Core().pool.Slot(form)[10]==6);CHECK(Get(session.Core().pool.Slot(form),18,2)==target);CHECK(session.CursorDropSettings().costFlags==0x160 && session.Sp(1)==before);
    const auto preview=CursorPreviewSid(session.Cursor().Lifecycle().previews[0]);CHECK(session.Core().pool.Slot(preview)[10]==target && (session.Core().pool.Slot(preview)[40]&8));
    auto records=session.PlayerRecords();Put(records,180+0x94,preview.value);Put(records,180+0x98,123);Put(records,180+0x8c,priest.value);Put(records,180+0x90,456); // 실제 등록 쌍 하나만 일치시키고 다른 쌍은 표식으로 보존한다.
    session.Cursor().Cancel();ui.menus->Close();chain.reset();CHECK(!ui.menus->HasMenus());
    CHECK(Get(records,180+0x94)==0 && Get(records,180+0x98)==0 && Get(records,180+0x8c)==priest.value && Get(records,180+0x90)==456);
}
// 원본 슬롯의 TYPE/가상 개수 snapshot을 그대로 사용하며 현재 HP 환불만 계산하는 공개 입구다.
TEST_CASE(CampaignObjectMenus_ActualRefundAndUninstalledGuard) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());
    CHECK(Throws([&] { static_cast<void>(session.MakeObjectMenus({})); }));session.Load();const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};const auto raw=session.Core().pool.Slot(priest);const auto& hp=session.HitPoints();
    const auto count=std::uint32_t(raw[11]>>5);const auto before=session.Sp(1);CHECK(hp.SalvageRefund(priest,raw[10],static_cast<std::int32_t>(count))==hp.Plan(priest,1,true).refund);CHECK(session.Sp(1)==before);
    ActualMenuUi ui(assets);std::unique_ptr<RawCampaignObjectMenus> chain;
    ui.menus->Open({320,240},[&](const auto& widgets,const auto& controls,const auto& menus) { widgets.BeginVertical();auto hooks=ui.Hooks(session,widgets,controls,menus);hooks.priest.tell={};chain=session.MakeObjectMenus(std::move(hooks));chain->OpenPriest(priest); });
    session.SetSp(1,0);CHECK(Throws([&] { static_cast<void>(chain->SelectBuild(session.PriestMenuSettings().dais,priest)); }));
    ui.menus->Close();chain.reset();
}
// 명시 현재 peer 기록으로 원본 Player 전체 메뉴를 선택한다. 실제 로그인/전송 완주를 대신하는 검사는 아니다.
TEST_CASE(CampaignObjectMenus_CurrentPeerPlayerMenuUsesActualInput) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;ActualMenuUi ui(assets);
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());session.Load();
    auto players=session.PlayerRecords();Put(players,2*180+0x2c,22);players[2*180+0x50]=0x22;const std::string name="Current peer";std::copy(name.begin(),name.end(),players.begin()+2*180+0xc);players[2*180+0xc+name.size()]=0;
    const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(2))};CHECK(priest.value);std::unique_ptr<RawCampaignObjectMenus> chain;
    ui.menus->Open({320,240},[&](const auto& widgets,const auto& controls,const auto& menus) { widgets.BeginVertical();chain=session.MakeObjectMenus(ui.Hooks(session,widgets,controls,menus));chain->OpenPriest(priest); });
    const auto player=ui.menus->Point("Player");CHECK(player.has_value());if (player) {
        ui.Click(*player);
        // 실제 원본 하위 메뉴 확장의 두 프레임을 같은 트리에서 진행한다.
        for (std::uint32_t frame=0;frame<2;++frame) ui.buttons.Poll({InputCode::kRelease,player->x,player->y});
        const auto find=ui.menus->Point("Find Priest!");CHECK(find.has_value());CHECK(!ui.menus->Point("Request Alliance").has_value());if (find) ui.Click(*find);
    }
    const auto command=ui.commands.Pop(false);CHECK(command.code==0x020000b8 && command.y==2);ui.menus->Close();chain.reset();
    session.Core().bridgeState.mission=false; // 같은00594fa0을 명시 변경하여00594fb8이나 상수1을 읽는 오연결을 검출한다.
    ui.menus->Open({320,240},[&](const auto& widgets,const auto& controls,const auto& menus) { widgets.BeginVertical();chain=session.MakeObjectMenus(ui.Hooks(session,widgets,controls,menus));chain->OpenPriest(priest); });
    const auto changed=ui.menus->Point("Player");CHECK(changed.has_value());if (changed) {
        ui.Click(*changed);
        // 같은 현재 미션 전역을 읽어 달라진 원본 Player 하위 메뉴를 실제 확장한다.
        for (std::uint32_t frame=0;frame<2;++frame) ui.buttons.Poll({InputCode::kRelease,changed->x,changed->y});
        CHECK(ui.menus->Point("Request Alliance").has_value());
    }
    ui.menus->Close();chain.reset();session.Core().bridgeState.mission=true;
}

// 공통Open의게임조회가같은Session의현재풀/카메라를사용하는지확인한다. GUI는명시경계다.
TEST_CASE(CampaignObjectMenus_ActualCommonOpenUsesCurrentWorld) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external; // 보호자산과실제장치입력수명이다.
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());
    CHECK(Throws([&] { static_cast<void>(session.MakeObjectMenuOpen({})); }));session.Load();const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(priest.value);
    std::vector<std::uint32_t> sequence;std::vector<std::array<std::int32_t,2>> positions; // 실제부모가선택한GUI효과와부착좌표다.
    ObjectMenuOpenHooks hooks;hooks.beginFancy=[&] { sequence.push_back(1); };hooks.summary=[&](Sid sid) { CHECK(sid==priest);sequence.push_back(2); };hooks.populate=[&](Sid sid) { CHECK(sid==priest);sequence.push_back(3); };
    hooks.attach=[&](Sid sid,std::int32_t x,std::int32_t y) { CHECK(sid==priest);sequence.push_back(4);positions.push_back({x,y}); };
    hooks.menuWidth=[] { return 0.0F; };hooks.screenWidth=[] { return 100000; }; // 경계선택이바뀌지않는명시화면입력이다.
    hooks.altar=[](Sid)->Sid { throw std::logic_error("외부제단조회사용오류"); };hooks.project=[](float,float)->std::array<std::int32_t,2> { throw std::logic_error("외부투영사용오류"); };
    hooks.content=[](Sid)->Sid { throw std::logic_error("외부내용물조회사용오류"); };hooks.range=[](std::uint32_t)->float { throw std::logic_error("외부사정거리사용오류"); };
    auto open=session.MakeObjectMenuOpen(std::move(hooks));session.SetParticleView({0,0,0,0,1024,768},{});
    CHECK(open->Open(priest,{InputCode::kRightButton,320,240})==1);CHECK(sequence==std::vector<std::uint32_t>({1,2,3,4}));
    positions.clear();open->Position(priest);session.SetParticleView({17,-23,0,0,1024,768},{});open->Position(priest);
    CHECK(positions.size()==2);if (positions.size()==2) CHECK(positions[1][0]==positions[0][0]-17 && positions[1][1]==positions[0][1]+23);
    const auto original=session.Core().pool.Slot(priest);CHECK(!(original[40]&1)); // 자연설치객체의허용비트를검사하기위해직접고치지않았다.
}

// 공통Open→실제4인자요약/사제+a8→같은Pool Form3부착→닫기의종속복구를확인한다.
TEST_CASE(CampaignObjectMenus_ActualObjectFormBindingAndClose) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;ActualMenuUi ui(assets); // 실제자산/공통UI수명이다.
    auto services=external.Make();services.gumpNotice=ui.buttons.ObjectNotice();auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));CHECK(Throws([&] { static_cast<void>(session.GumpForms()); }));session.Load();ui.currentSession=&session;
    const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};const auto head=Get(session.Core().pool.Slot(priest),6,2);const auto previous=head ? Hex(session.Core().pool.Slot(Sid{static_cast<std::uint16_t>(head)})) : std::string{};std::uint32_t root{},result{}; // 자연설치사제의기존종속원문과새메뉴다.
    std::unique_ptr<RawCampaignObjectMenus> chain;ui.menus->OpenAttached([&](const auto& widgets,const auto& controls,const auto& menus) {
        chain=session.MakeObjectMenus(ui.Hooks(session,widgets,controls,menus));ObjectMenuOpenHooks hooks;hooks.beginFancy=[&] { root=widgets.BeginFancy(); };hooks.summary=[&](Sid sid) { CHECK(sid==priest);chain->Summary(sid); };
        hooks.populate=[&](Sid sid) { chain->OpenPriest(sid); };
        hooks.menuWidth=[&] { return ui.menus->Node(menus.Current())->baseWidth; };hooks.screenWidth=[] { return 1024; };
        hooks.attach=[&](Sid sid,std::int32_t x,std::int32_t y) { menus.AttachObjectCurrent(sid.value,x,y); };
        auto open=session.MakeObjectMenuOpen(std::move(hooks));result=open->Open(priest,{InputCode::kRightButton,320,240});return root;
    });
    CHECK(std::find(ui.infoLines.begin(),ui.infoLines.end(),"\nOwner: ~iYou~.")!=ui.infoLines.end());CHECK(!ui.infoTitles.empty());
    CHECK(result==1 && root);const auto* node=ui.menus->Node(root);CHECK(node && (node->flags&0x40) && node->object==priest.value);
    if (node) {
        const Sid form{static_cast<std::uint16_t>(node->form)};const auto core=session.Core();CHECK(form.value && core.pool.Slot(form)[10]==3);CHECK(session.GumpForms().Parent(form)==priest);CHECK(session.GumpForms().Owner(form)==node->id);
        CHECK(Get(core.pool.Slot(priest),6,2)==form.value && Get(core.pool.Slot(form),4,2)==head);const auto about=ui.menus->Point("About");CHECK(about.has_value());
        if (about) ui.Click(*about);const auto command=ui.commands.Pop(false);CHECK(command.code==0x02000017 && command.y==core.pool.Slot(priest)[10]); // 실제원본메뉴입력→DWORD큐를통과하며Client명령본문은실행하지않는다.
        ui.menus->Close();CHECK(core.pool.Slot(form)[11]&1);CHECK(Get(core.pool.Slot(priest),6,2)==head);
        if (head) CHECK(Hex(core.pool.Slot(Sid{static_cast<std::uint16_t>(head)}))==previous);
    }
    CHECK(!ui.menus->HasMenus());chain.reset();ui.currentSession=nullptr;
}

// 살아있는실제Form부모의월드삭제가자식Form통지→UI Kill→중첩삭제→지연Cleanup으로진행한다.
TEST_CASE(CampaignObjectMenus_ActualParentDeletionAndDeferredUiCleanup) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;ActualMenuUi ui(assets); // 모든Form은같은실제Session풀에서만생성한다.
    auto services=external.Make();services.gumpNotice=ui.buttons.ObjectNotice();auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();ui.currentSession=&session;auto& forms=session.GumpForms();const auto parent=forms.Create(2);std::uint32_t root{};
    ui.menus->OpenAttached([&](const auto& widgets,const auto& controls,const auto& menus) { root=widgets.BeginFancy();controls.AddTitle("Form lifetime boundary");controls.AddText("Owned",0x02000017,100);menus.AttachObjectCurrent(parent.value,320,240);return root; });
    const auto* node=ui.menus->Node(root);CHECK(node);if (node) {
        const Sid form{static_cast<std::uint16_t>(node->form)};const auto id=node->id;CHECK(forms.Parent(form)==parent);CHECK(ui.buttons.RegisteredGump(id)==root);
        CHECK(session.World().Destroy(parent));CHECK(session.Core().pool.Slot(parent)[11]&1);CHECK(session.Core().pool.Slot(form)[11]&1);
        CHECK(ui.menus->Node(root) && (ui.menus->Node(root)->flags&0x10000));CHECK(ui.buttons.RegisteredGump(id)==root); // Kill뒤에도같은노드/등록은Cleanup까지남는다.
        ui.menus->Close();CHECK(!ui.menus->Node(root));CHECK(!ui.buttons.RegisteredGump(id));
    }
    CHECK(!ui.menus->HasMenus());ui.currentSession=nullptr;
}

// 명시peer의현재이름/동맹/부분실패를실제Identity→4인자정보→같은controlled제목에서확인한다.
TEST_CASE(CampaignObjectMenus_ActualSummaryUsesCurrentIdentityAndAlliance) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;ActualMenuUi ui(assets); // 명시peer는자연로그인검증이아니다.
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());session.Load();
    auto players=session.PlayerRecords();const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(2))};CHECK(priest.value);std::unique_ptr<RawCampaignObjectMenus> chain;
    const auto name=[&](std::string_view text) { std::copy(text.begin(),text.end(),players.begin()+2*180+0xc);players[2*180+0xc+text.size()]=0; }; // 같은Player원문의이름만현재입력으로바꾼다.
    const auto open=[&] { ui.infoLines.clear();ui.infoTitles.clear();ui.menus->Open({320,240},[&](const auto& widgets,const auto& controls,const auto& menus) { widgets.BeginFancy();chain=session.MakeObjectMenus(ui.Hooks(session,widgets,controls,menus));chain->Summary(priest);chain->OpenPriest(priest); }); }; // 실제현재요약과파생메뉴를함께작성한다.
    Put(players,2*180+0x2c,22);name("Current peer");session.Alliances()[2*9+1]=0;open();CHECK(std::find(ui.infoLines.begin(),ui.infoLines.end(),"\nEnemy: ~iCurrent peer~.")!=ui.infoLines.end());ui.menus->Close();chain.reset();
    name("Changed peer");session.Alliances()[2*9+1]=1;open();CHECK(std::find(ui.infoLines.begin(),ui.infoLines.end(),"\nAlly: ~iChanged peer~.")!=ui.infoLines.end());ui.menus->Close();chain.reset();
    Put(players,2*180+0x2c,0);open();CHECK(std::find(ui.infoLines.begin(),ui.infoLines.end(),"\nAlly: ~iSOMEONE~.")!=ui.infoLines.end());ui.menus->Close();chain.reset();
    const auto hitPoints=session.Priests().PriestState().CurrentHitPoints(priest);Put(session.Core().pool.AllocatedBytes(priest),26,1); // 실제492090을선택하기위한명시저HP입력이며피해/자연전투검증은아니다.
    open();CHECK(std::find(ui.infoLines.begin(),ui.infoLines.end(),"~yCapturable!~.\n")!=ui.infoLines.end());ui.menus->Close();chain.reset();Put(session.Core().pool.AllocatedBytes(priest),26,std::bit_cast<std::uint32_t>(hitPoints));
}

// 같은Session의 실제선택bit/표시/491Regular와50ProcessForm 통지를 실행한다. UI/진단은 명시 경계다.
TEST_CASE(CampaignUserSelection_SamePoolMarksRegularAndManaNotice) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;
    std::uint32_t selected{},mouseType{},mode{};std::array<std::uint32_t,2> remembered{};std::array<std::uint32_t,0> multiple{};std::int32_t count{},mouseX{},mouseY{}; // 명시 UI의 같은 현재 저장소다.
    auto services=external.Make();services.deletion.selected=[&] { return Sid{static_cast<std::uint16_t>(selected)}; }; // 삭제도 선택의 같은 저장소를 읽는다.
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));
    const UserSelectionState state{selected,mouseType,remembered,multiple,count};CHECK(Throws([&] { static_cast<void>(session.MakeUserSelection(state,{})); }));session.Load();
    const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(priest.value);auto core=session.Core();CHECK(!(core.pool.Slot(priest)[40]&9));
    std::uint32_t dumps{},cursors{};UserHighlightHooks device;device.selectCursor=[&](std::uint32_t) { ++cursors; };RawUserHighlight highlight({mode,selected,mouseType,mouseX,mouseY},std::move(device)); // 같은 실제 강조 몸체이며 이 검사는 mode5 역투영을 선택하지 않는다.
    UserSelectionHooks hooks;hooks.paused=[] { return 0U; };hooks.enabled=[] { return 1U; };hooks.blocked=[] { return 0U; };hooks.dump=[&](Sid sid) { CHECK(sid==priest);++dumps; };hooks.highlight=[&](std::uint32_t value) { highlight.Highlight(value); };
    hooks.assertion=hooks.previewAssertion=[] { throw std::logic_error("선택 통합 검사 예상 밖 진단"); };
    hooks.selectNotice=[](Sid,std::uint32_t) { throw std::logic_error("실제 선택bit로 교체하지 않음"); };hooks.refresh=[](Sid) { throw std::logic_error("실제 표시로 교체하지 않음"); };hooks.movementPreview=hooks.refresh;
    hooks.rangeType=[](std::uint32_t) -> std::uint32_t { throw std::logic_error("실제 사정거리 종류로 교체하지 않음"); };hooks.showMana=hooks.showRange=hooks.selectNotice;
    auto selection=session.MakeUserSelection(state,std::move(hooks));CHECK(selection->Select(priest.value,0,0)==priest.value);CHECK(selected==priest.value && dumps==1 && (core.pool.Slot(priest)[40]&4));
    const auto* preview=core.processes.FindEvent(priest,0x491);CHECK(preview && preview->Payload()==std::bit_cast<float>(0x3e4ccccdU)); // 실제 Regular 생성까지 검증하며491 프레임 소비를 대신하지 않는다.
    CursorLifecycleHooks mana;session.BindCursorMana(mana);const auto token=mana.createMana(priest,0);auto* process=dynamic_cast<CursorManaProcess*>(core.kernel.Get(token));CHECK(process && process->Parent()==priest);
    if (process) CHECK(Get(process->Fields().bytes,0x94)==0);CHECK(selection->Select(0,0,1)==0);CHECK(selected==0 && !(core.pool.Slot(priest)[40]&4));if (process) CHECK(Get(process->Fields().bytes,0x94)==1);
    CHECK(dumps==1 && cursors==0); // 같은 mode0은 불필요한 장치 커서 변경을 하지 않는다.
}

// 실제 Form6 추가 통지가491을 예약하고 같은Session의그림/타이머/가상삭제까지 실행해야 한다.
TEST_CASE(CampaignContentPreview_RealNoticeCreatePopAndExpiry) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();
    services.carrierContent.checkCarrier=[](Sid) { throw std::logic_error("외부 지면 검사로 잘못 연결된 내용물 표시"); };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));
    CHECK(Throws([&] { static_cast<void>(session.CreateContained(Sid{5},100,0,1,0x10,0)); }));session.Load();auto core=session.Core();
    const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(priest.value);std::uint32_t kind{}; // 실제 생산 자산 중 표시 가능하고 폭탄/사제 전용 통지가 없는 첫 타입이다.
    // 현재 보호TYPE에서 원본 표시 genus8000의 실제 자산을 고르며 결과/프레임은 주입하지 않는다.
    for (std::size_t type=70;type<core.types.size();++type) if ((core.types[type].flags2&0x8000U) && !(core.types[type].flags2&0x100U)) { kind=static_cast<std::uint32_t>(type);break; }
    CHECK(kind>=70);CHECK(!core.processes.FindEvent(priest,0x491));
    const auto content=session.CreateContained(priest,kind,0x1234,1,0x10,0);CHECK(content.value);
    auto* regular=core.processes.FindEvent(priest,0x491);CHECK(regular && regular->Payload()==std::bit_cast<float>(0x3e4ccccdU));if (!regular) return;
    regular->RunFrame();ContentPreviewProcess* timer{};std::uint32_t timerPid{}; // 다른 사건/AI 프레임을 실행하지 않고 실제491 체인만 관찰한다.
    // 실제Kernel에 생긴 파생 타이머를 찾는다. SID나 만료 값을 검사용으로 대신 쓰지 않는다.
    for (std::uint32_t pid=1;pid<=core.kernel.Capacity();++pid) if (auto* candidate=dynamic_cast<ContentPreviewProcess*>(core.kernel.Get(pid))) { CHECK(!timer);timer=candidate;timerPid=pid; }
    CHECK(timer);if (!timer) return;const auto ghost=timer->Parent(),form=timer->Form();const auto deadline=timer->Fields().Deadline();const auto raw=core.pool.Slot(ghost);
    CHECK(raw[10]==kind && (raw[40]&8U) && raw[34]==core.pool.Slot(priest)[34]);CHECK(Get(raw,12,2)==0x1234U);
    CHECK(Get(raw,36)==static_cast<std::uint32_t>(assets.game.Types()[kind-70].definition.specialFrames.defaultFrame));CHECK(core.pool.Slot(form)[10]==47);
    CHECK(deadline==core.processState.now+static_cast<double>(std::bit_cast<float>(0x3e4ccccdU)));
    core.processState.now=std::nextafter(deadline,-INFINITY);timer->RunFrame();CHECK(core.kernel.Get(timerPid)==timer && !(core.pool.Slot(ghost)[11]&1U));
    core.processState.now=deadline;timer->RunFrame();CHECK(!core.kernel.Get(timerPid));CHECK((core.pool.Slot(ghost)[11]&1U) && (core.pool.Slot(form)[11]&1U));
    CHECK(!(core.pool.Slot(priest)[11]&3U) && !(core.pool.Slot(content)[11]&1U));CHECK(core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
}
// 미리보기가 아닌 최초 너겟도 실제 두 예약/확보 메모리/공유게임난수를 사용한다.
TEST_CASE(CampaignContentPreview_RealNuggetInitialRegulars) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());session.Load();auto core=session.Core();
    const auto nugget=session.World().Create(154);CHECK(!(core.pool.Slot(nugget)[40]&8U));session.World().Pop(nugget,10.5F,10.5F,0);
    const auto* first=core.processes.FindEvent(nugget,1);const auto* third=core.processes.FindEvent(nugget,3);CHECK(first && third && first!=third);if (!first || !third) return;
    CHECK(first->Parent()==nugget && third->Parent()==nugget && first->Count()==0 && third->Count()==0);
    CHECK(first->Time()==core.processState.now && third->Time()==core.processState.now);
    CHECK(first->Payload()>=0.0F && first->Payload()<=0.3F && third->Payload()>=0.0F && third->Payload()<=0.5F);
    CHECK(core.pool.Slot(first->Form())[10]==46 && core.pool.Slot(third->Form())[10]==46);
    CHECK(core.processes.FromForm(first->Form())==first && core.processes.FromForm(third->Form())==third);
    // 실제 너겟Regular485230의프레임소비는이번생성/부착검증과구분한다.
}
// 같은Session의실제예약으로프레임왕복과가속낙하/부모·예약삭제까지검사한다.
TEST_CASE(CampaignNuggetRegular_ActualAnimationAndFallLifecycle) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());session.Load();auto core=session.Core();
    const auto nugget=session.World().Create(154,2);session.World().Pop(nugget,10.5F,10.5F,0); // client번호의실체이며extra8미리보기로바꾸지않는다.
    const auto maximum=core.frames[154].Run('A').count-1;CHECK(maximum>0);auto* grow=core.processes.FindEvent(nugget,1);CHECK(grow);if (!grow) return;
    CHECK(grow->Payload()>0.0F);if (grow->Payload()<=0.0F) return;const auto growPid=core.kernel.Find(*grow);const auto initialFrame=Get(core.pool.Slot(nugget),36);grow->RunFrame();CHECK(Get(core.pool.Slot(nugget),36)==initialFrame && grow->Count()==1);
    // 다른부모의AI/예약을실행하지않고현재너겟의실제증가예약만끝점까지진행한다.
    for (std::int32_t step=0;step<maximum+3 && core.kernel.Get(growPid)==grow;++step) { core.processState.now=grow->Time();grow->RunFrame(); }
    CHECK(!core.kernel.Get(growPid));CHECK(Get(core.pool.Slot(nugget),36)==static_cast<std::uint32_t>(maximum));
    auto* shrink=core.processes.FindEvent(nugget,0);CHECK(shrink);if (!shrink) return;CHECK(shrink->Count()==0 && shrink->Payload()>=2.0F && shrink->Payload()<8.0F);
    const auto shrinkPid=core.kernel.Find(*shrink);shrink->RunFrame();CHECK(Get(core.pool.Slot(nugget),36)==static_cast<std::uint32_t>(maximum) && shrink->Count()==1);
    // 실제감소예약이0프레임에닿으면새증가예약을만들고자신의Form을정리한다.
    for (std::int32_t step=0;step<maximum+3 && core.kernel.Get(shrinkPid)==shrink;++step) { core.processState.now=shrink->Time();shrink->RunFrame(); }
    CHECK(!core.kernel.Get(shrinkPid));CHECK(Get(core.pool.Slot(nugget),36)==0);CHECK(core.processes.FindEvent(nugget,1));
    std::int32_t skyColumn{}; // 실제점유지도에서전체낙하경로가빈열을선택하며지도값은수정하지않는다.
    // 보호미션의실제하늘열을조회한다. 각열의지면/다리비트만읽는다.
    for (std::int32_t x=2;x<254 && !skyColumn;++x) {
        bool empty=true;
        // 100회가속낙하가통과할수있는실제지도전체열이다.
        for (std::int32_t y=2;y<255;++y) if (core.spots[static_cast<std::size_t>(y*256+x)]&6U) { empty=false;break; }
        if (empty) skyColumn=x;
    }
    CHECK(skyColumn);if (!skyColumn) return;session.World().Unpop(nugget,0);session.World().Pop(nugget,static_cast<float>(skyColumn),2.0F,0x40);
    auto* falling=core.processes.FindEvent(nugget,3);CHECK(falling && falling->Payload()>0.0F);if (!falling || falling->Payload()<=0.0F) return;const auto fallingPid=core.kernel.Find(*falling);const auto frameBeforeFall=Get(core.pool.Slot(nugget),36);
    bool moved{},sawZ{};
    // 같은 실제 낙하 예약만 기한마다 실행한다. 다른 부모를 포함한 전체Kernel 완주와는 구별한다.
    for (std::uint32_t step=0;step<105 && !(core.pool.Slot(nugget)[11]&1U);++step) {
        CHECK(core.kernel.Get(fallingPid)==falling);if (core.kernel.Get(fallingPid)!=falling) break;core.processState.now=std::max(core.processState.now,falling->Time());falling->RunFrame(); // 늦게처리하는기존예약때문에게임시각을과거로되돌리지않는다.
        if (!(core.pool.Slot(nugget)[11]&1U)) { moved=moved || std::bit_cast<float>(Get(core.pool.Slot(nugget),18))>2.0F;sawZ=sawZ || core.pool.Slot(nugget)[35]==20;CHECK(Get(core.pool.Slot(nugget),36)==frameBeforeFall); }
    }
    CHECK(moved && sawZ && (core.pool.Slot(nugget)[11]&1U));CHECK(!core.kernel.Get(fallingPid));CHECK(core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
}
// 실제 Carrier의 5105 예약을 진행해 포획 Form에서 새 사제 생성/배치/낙하 예약/기존 부모 삭제까지 확인한다.
TEST_CASE(CampaignCarrierFalling_ActualCapturedPriestRelease) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;
    auto services=external.Make();std::vector<std::array<std::uint32_t,3>> sounds;std::vector<std::array<std::uint32_t,6>> globalSounds; // 실제 위치/전역 음향 장치 경계의 요청만 기록한다.
    services.sound.playOnceAt=[&](float x,float y,SoundHandle sound,std::uint32_t flags,std::uint32_t extra) { CHECK(flags==0 && extra==0);sounds.push_back({std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),sound});return sound; };
    services.sound.play=[&](SoundHandle sound,std::uint32_t loop,std::uint32_t volume,std::uint32_t pan,std::uint32_t priority,std::uint32_t maximum) { globalSounds.push_back({sound,loop,volume,pan,priority,maximum});return sound; };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();sounds.clear();
    const auto carrier=session.World().Create(135,2);session.World().Pop(carrier,10.5F,10.5F,0);CHECK(Get(core.pool.Slot(carrier),0)==0x503e50U);
    const auto content=session.CreateContained(carrier,158,0x1281,0x20,0x10,0);CHECK(content.value && Get(core.pool.Slot(content),12,2)==0x1281U);
    const auto records=session.PlayerRecords();const Sid existing{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(existing.value);
    const auto homeX=Get(core.pool.Slot(existing),14),homeY=Get(core.pool.Slot(existing),18);CHECK(std::bit_cast<float>(homeX)>0 && std::bit_cast<float>(homeY)>0);
    Put(records,0xb4+0xa8,homeX);Put(records,0xb4+0xac,homeY); // 낙하 시작4287d0이 남기는 복귀 좌표의 명시 초기 입력이다. 그 시작 부모는 이번 통합 검사에서 실행하지 않는다.
    std::vector<std::uint16_t> oldPriests; // 기존 실제 사제는 새 생성 결과로 오인하지 않는다.
    // 현재 살아 있는 사제 번호만 저장한다. 결과 번호/위치는 주입하지 않는다.
    for (std::uint32_t sid=1;sid<core.pool.Capacity();++sid) { const auto raw=core.pool.Slot(Sid{static_cast<std::uint16_t>(sid)});if (raw[10]==158 && !(raw[11]&3U)) oldPriests.push_back(static_cast<std::uint16_t>(sid)); }
    auto* falling=core.processes.AddRegular(carrier,0x5105,0.01F);CHECK(falling);if (!falling) return;const auto pid=core.kernel.Find(*falling);const auto form=falling->Form();bool moved{},sawZ{};
    // 해당 캐리어의 실제 예약만 진행하며 전체 Kernel/다른 AI/사용자 입력 완주와 구별한다.
    for (std::uint32_t step=0;step<105 && !(core.pool.Slot(carrier)[11]&1U);++step) {
        CHECK(core.kernel.Get(pid)==falling);if (core.kernel.Get(pid)!=falling) break;core.processState.now=std::max(core.processState.now,falling->Time());falling->RunFrame();
        if (!(core.pool.Slot(carrier)[11]&1U)) { moved=moved || std::bit_cast<float>(Get(core.pool.Slot(carrier),18))>10.5F;sawZ=sawZ || core.pool.Slot(carrier)[35]==20; }
    }
    CHECK(moved && sawZ && (core.pool.Slot(carrier)[11]&1U));CHECK(core.pool.Slot(content)[11]&1U);CHECK(core.pool.Slot(form)[11]&1U);CHECK(!core.kernel.Get(pid));Sid released{};
    // 실제 생성된 새 사제의 번호/소유자/기억 WORD/좌표/낙하 예약을 관측한다.
    for (std::uint32_t sid=1;sid<core.pool.Capacity();++sid) {
        const auto current=Sid{static_cast<std::uint16_t>(sid)};const auto raw=core.pool.Slot(current);
        if (raw[10]==158 && !(raw[11]&3U) && std::find(oldPriests.begin(),oldPriests.end(),current.value)==oldPriests.end()) { CHECK(!released.value);released=current; }
    }
    CHECK(released.value);if (released.value) { const auto raw=core.pool.Slot(released);CHECK(Get(raw,12,2)==0x1281U);CHECK(raw[34]==1);CHECK(Get(raw,14)==homeX && Get(raw,18)==homeY);CHECK(core.processes.FindEvent(released,0x25b)); } // 실제 사제 SetOwner(0)은 전투에서 기억한 원래 소유자1을 복원한다.
    CHECK(std::find(sounds.begin(),sounds.end(),std::array<std::uint32_t,3>{homeX,homeY,external.names.Lookup("priestFall.wav")})!=sounds.end());
    CHECK(std::find(globalSounds.begin(),globalSounds.end(),std::array<std::uint32_t,6>{external.names.Lookup("priestFree.wav"),0,0,0,1,0})!=globalSounds.end());
    CHECK(core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
}
// 4287d0 자체가 복귀 좌표를 쓰고 즉시 사제를 해방하며 실제 경로/선택을 정리해야 한다.
TEST_CASE(CampaignCarrierFallStart_ActualSelectedMovementReleaseAndFall) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;
    std::uint32_t selected{},mouseType{},mode{};std::array<std::uint32_t,2> remembered{};std::array<std::uint32_t,0> multiple{};std::int32_t multipleCount{},mouseX{},mouseY{}; // 같은 실제 UI 저장소다.
    std::vector<std::vector<std::uint8_t>> pathPackets;std::uint32_t lost{},dumps{}; // 외부 패킷/유닛 손실/진단 요청만 기록한다.
    auto services=external.Make();services.deletion.selected=[&] { return Sid{static_cast<std::uint16_t>(selected)}; };services.unitLost=[&](Sid) { ++lost; };
    services.pathPacket=[&](std::span<const std::uint8_t> bytes) { pathPackets.emplace_back(bytes.begin(),bytes.end()); };services.motion.path.stepSound=[](Sid,float,float,std::uint32_t) {};services.dependencyTransmit=[](Sid,std::uint32_t) {};
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();
    const Sid existing{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(existing.value);const auto old=core.pool.Slot(existing);const auto x=std::bit_cast<float>(Get(old,14)),y=std::bit_cast<float>(Get(old,18));
    const auto carrier=session.World().Create(133,2);session.World().Pop(carrier,x,y,0);CHECK(Get(core.pool.Slot(carrier),0)==0x50e568U);
    const auto content=session.CreateContained(carrier,158,0x1281,0x20,0x10,0);core.owner.Set(carrier,1); // 포획된 내용물이 있는 캐리어를 초기 상태로 두며 포획 획득 부모는 실행하지 않는다.
    auto* path=session.StartCarrierMovement({carrier,{x,y},1,0,0,0,0});CHECK(path);if (!path) return;CHECK(path->MovementType()==40);const auto pathPid=core.kernel.Find(*path);const auto pathForm=path->Form();CHECK(!pathPackets.empty());
    RawUserHighlight highlight({mode,selected,mouseType,mouseX,mouseY},{});UserSelectionHooks hooks;hooks.paused=[] { return 0U; };hooks.enabled=[] { return 1U; };hooks.blocked=[] { return 0U; };hooks.dump=[&](Sid sid) { CHECK(sid==carrier);++dumps; };hooks.highlight=[&](std::uint32_t value) { highlight.Highlight(value); };hooks.assertion=hooks.previewAssertion=[] { throw std::logic_error("자연 낙하 선택 검사 예상 밖 진단"); };
    auto selection=session.MakeUserSelection({selected,mouseType,remembered,multiple,multipleCount},std::move(hooks));CHECK(selection->Select(carrier.value,0,0)==carrier.value);CHECK(selected==carrier.value && (core.pool.Slot(carrier)[40]&4U) && dumps==1);
    std::vector<std::uint16_t> priorPriests; // 실제 새 사제와 기존 사제를 구별한다.
    // 원래 살아 있는 사제 번호만 읽고 결과를 주입하지 않는다.
    for (std::uint32_t sid=1;sid<core.pool.Capacity();++sid) { const auto raw=core.pool.Slot(Sid{static_cast<std::uint16_t>(sid)});if (raw[10]==158 && !(raw[11]&3U)) priorPriests.push_back(static_cast<std::uint16_t>(sid)); }
    const auto records=session.PlayerRecords();const auto beforeX=Get(records,0xb4+0xa8),beforeY=Get(records,0xb4+0xac);CHECK(!core.processes.FindEvent(carrier,0x5105));
    CHECK(session.Priests().FallWalker(carrier));CHECK(selected==0 && !(core.pool.Slot(carrier)[40]&4U));
    RawContainedLookup remaining(core.pool,[&](Sid sid,std::uint32_t flags) { static_cast<void>(session.World().Destroy(sid,flags)); }); // 해방 중 PID/Form이 즉시 재사용될 수 있어 실제40 객체와 종속 체인을 조회한다.
    CHECK(dynamic_cast<RawPathProcess*>(core.kernel.Get(pathPid))==nullptr);CHECK(!remaining.Find(carrier,40).value);CHECK(core.pool.Slot(content)[11]&1U);static_cast<void>(pathForm);
    const auto homeX=Get(records,0xb4+0xa8),homeY=Get(records,0xb4+0xac);CHECK(homeX!=beforeX || homeY!=beforeY);CHECK(std::bit_cast<float>(homeX)>0 && std::bit_cast<float>(homeY)>0);Sid released{};
    // 실제 해방된 사제의 위치/소유권/새 낙하 예약은 시작 함수가 만든 현재 기록과 일치해야 한다.
    for (std::uint32_t sid=1;sid<core.pool.Capacity();++sid) { const Sid current{static_cast<std::uint16_t>(sid)};const auto raw=core.pool.Slot(current);if (raw[10]==158 && !(raw[11]&3U) && std::find(priorPriests.begin(),priorPriests.end(),current.value)==priorPriests.end()) { CHECK(!released.value);released=current; } }
    CHECK(released.value);if (released.value) { const auto raw=core.pool.Slot(released);CHECK(Get(raw,12,2)==0x1281U && raw[34]==1);CHECK(Get(raw,14)==homeX && Get(raw,18)==homeY);CHECK(core.processes.FindEvent(released,0x25b)); }
    auto* falling=core.processes.FindEvent(carrier,0x5105);CHECK(falling);if (!falling) return;const auto fallPid=core.kernel.Find(*falling);const auto fallForm=falling->Form();CHECK(core.pool.Slot(fallForm)[10]==46 && falling->Count()==0 && falling->Payload()==5.0F && falling->Time()==core.processState.now);
    CHECK(session.Priests().FallWalker(carrier));CHECK(core.processes.FindEvent(carrier,0x5105)==falling && falling->Count()==0); // 중복 시작은 새 예약/해방을 만들지 않는다.
    // 현재 캐리어의 원본 예약만 진행하며 다른 AI/전체 Kernel을 함께 실행하지 않는다.
    for (std::uint32_t step=0;step<105 && !(core.pool.Slot(carrier)[11]&1U);++step) { CHECK(core.kernel.Get(fallPid)==falling);if (core.kernel.Get(fallPid)!=falling) break;core.processState.now=std::max(core.processState.now,falling->Time());falling->RunFrame(); }
    CHECK(core.pool.Slot(carrier)[11]&1U);CHECK(!core.kernel.Get(fallPid) && (core.pool.Slot(fallForm)[11]&1U));CHECK(core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
    const auto empty=session.World().Create(187,2);CHECK(core.types[187].name.empty() && core.types[187].footX==0 && core.types[187].footY==0 && !core.shapes[187].loaded);CHECK(Throws([&] { static_cast<void>(session.StartCarrierMovement({empty,{x,y},1,0,0,0,0})); })); // 187은 생성자만 있는 미등록 슬롯이므로 실제 자산의 SHP/경로로 보정하지 않는다.
}
// UI 소유자가 먼저 사라지면 해제된 선택 객체를 호출하지 않고 필요한 경계에서 거부한다.
TEST_CASE(CampaignCarrierFallStart_ExpiredSelectionDoesNotDangle) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;std::uint32_t selected{},mouseType{},mode{},dumps{};std::array<std::uint32_t,2> remembered{};std::array<std::uint32_t,0> multiple{};std::int32_t count{},mouseX{},mouseY{}; // UI 저장소는 검사 내내 살아 있다.
    auto services=external.Make();services.deletion.selected=[&] { return Sid{static_cast<std::uint16_t>(selected)}; };auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();
    const auto carrier=session.World().Create(133,2);session.World().Pop(carrier,10.5F,10.5F,0);RawUserHighlight highlight({mode,selected,mouseType,mouseX,mouseY},{});UserSelectionHooks hooks;
    hooks.paused=[] { return 0U; };hooks.enabled=[] { return 1U; };hooks.blocked=[] { return 0U; };hooks.dump=[&](Sid sid) { CHECK(sid==carrier);++dumps; };hooks.highlight=[&](std::uint32_t value) { highlight.Highlight(value); };hooks.assertion=hooks.previewAssertion=[] { throw std::logic_error("선택 수명 검사 예상 밖 진단"); };
    auto selection=session.MakeUserSelection({selected,mouseType,remembered,multiple,count},std::move(hooks));CHECK(selection->Select(carrier.value,0,0)==carrier.value);CHECK(dumps==1 && selected==carrier.value && (core.pool.Slot(carrier)[40]&4U));selection.reset();
    bool rejected{};try { static_cast<void>(session.Priests().FallWalker(carrier)); }catch (const std::logic_error& error) { rejected=std::string_view(error.what()).find("낙하 선택 해제의 현재 UI 수명 미연결")!=std::string_view::npos; }
    CHECK(rejected);CHECK(!core.processes.FindEvent(carrier,0x5105));CHECK(selected==carrier.value && (core.pool.Slot(carrier)[40]&4U));
}

// 초기 너겟 Form의 실제 로컬 수신/재부착/표시 예약과 수신기의 좁은 책임을 확인한다.
TEST_CASE(CampaignCarrierTransfer_ActualFormReceiveAndPreview) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;std::uint32_t homeRequests{},geyserRequests{};Sid requestedHome{}; // 미복원 귀환/간헐천 선택 요청만 외부 경계로 관찰한다.
    auto services=external.Make();services.dependencyTransmit=[](Sid,std::uint32_t) {};services.pathPacket=[](std::span<const std::uint8_t>) {};services.motion.path.stepSound=[](Sid,float,float,std::uint32_t) {};
    services.motion.carrier.chooseGeyser=[&](Sid) { ++geyserRequests;return Sid{}; };services.motion.carrierGoHome=[&](Sid sid) { ++homeRequests;requestedHome=sid; };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(priest.value);const auto raw=core.pool.Slot(priest);const auto x=std::bit_cast<float>(Get(raw,14)),y=std::bit_cast<float>(Get(raw,18));
    const auto receiver=session.World().Create(133,2),donor=session.World().Create(133,2);session.World().Pop(receiver,x,y,0);session.World().Pop(donor,x,y,0);core.owner.Set(receiver,1);core.owner.Set(donor,1);
    const auto nugget=session.CreateContained(donor,154,7,0x20,0x10,0);CHECK(nugget.value);CHECK(core.types[154].flags2&0x8000U); // 초기 너겟만 명시하며 전달 결과/새 부모/예약은 공급하지 않는다.
    auto* movement=session.StartCarrierMovement({donor,{x,y},1,0,0,0,0});CHECK(movement);if (!movement) return;const auto oldPid=core.kernel.Find(*movement);RawContainedLookup lookup(core.pool,[](Sid,std::uint32_t) { throw std::logic_error("검사 조회에서 삭제 금지"); });CHECK(lookup.Find(donor,40).value);
    std::array<std::uint8_t,8> command{};Put(command,4,receiver.value,2);Put(command,6,nugget.value,2);session.CarrierTransfer().Receive(999,0xffffffffU,command);CHECK(&session.CarrierTransfer().Pool()==&core.pool);CHECK(Get(core.pool.Slot(nugget),16,2)==receiver.value);CHECK(session.Priests().Carrier().FindContained(receiver,0x20,154,0)==nugget);CHECK(!session.Priests().Carrier().FindContained(donor,0x20,0,0).value);
    CHECK(lookup.Find(donor,40).value);CHECK(core.kernel.Get(oldPid)==movement);CHECK(core.processes.FindEvent(receiver,0x491));CHECK(geyserRequests==0 && homeRequests==0 && !requestedHome.value); // 426830 자체는 기증자의 이동 삭제/경제·귀환을 실행하지 않는다.
    CHECK(session.CarrierTransfer().Transfer(receiver)==10);CHECK(geyserRequests==0 && homeRequests==0); // 이미 받은 너겟은 추가 전달/재시작을 만들지 않는다.
}

// 같은 미션의 실제 골렘/사제를 사용해 일반 접근점과 원본 BFS/Host40 시작을 확인한다.
TEST_CASE(CampaignCarrierTarget_ActualBfsTargetStartAndReturn) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;std::size_t packets{}; // 실제 직렬화된 경로 전송만 관찰한다.
    auto services=external.Make();services.dependencyTransmit=[](Sid,std::uint32_t) {};services.pathPacket=[&](std::span<const std::uint8_t> bytes) { CHECK(!bytes.empty());++packets; };services.motion.path.stepSound=[](Sid,float,float,std::uint32_t) {};
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(priest.value);const auto raw=core.pool.Slot(priest);const auto x=std::bit_cast<float>(Get(raw,14)),y=std::bit_cast<float>(Get(raw,18));
    const auto golem=session.World().Create(133,2);session.World().Pop(golem,x,y,0);core.owner.Set(golem,1);const auto first=session.CarrierTarget().GoReturn(golem,priest);CHECK(first);if (!first) return;auto* path=reinterpret_cast<RawPathProcess*>(first);CHECK(path->Parent()==golem && path->MovementType()==40 && path->Runtime().Count()>0);CHECK(core.processes.FromForm(path->Form())==path && packets==1);CHECK(&session.CarrierTarget().Pool()==&core.pool);
    const auto form=path->Form();CHECK(session.CarrierTarget().GoGeyser(golem,priest)==0);CHECK(core.processes.FromForm(form)==path && packets==1); // 사제를 간헐천으로 취급하지 않고 현재 경로를 유지한다.
    const auto second=session.CarrierTarget().StartTarget(golem,priest,1,0);CHECK(second);if (!second) return;auto* current=reinterpret_cast<RawPathProcess*>(second);CHECK(current->Parent()==golem && current->MovementType()==40 && packets==2);CHECK(core.processes.FromForm(current->Form())==current);RawContainedLookup lookup(core.pool,[](Sid,std::uint32_t) { throw std::logic_error("대상 이동 검사에서 삭제 조회 금지"); });CHECK(lookup.Find(golem,40)==current->Form());CHECK(!lookup.Find(golem,40,1).value); // 기존 경로 교체는 같은 실제 종속 체인에 하나만 남긴다.
    CHECK(core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
}
