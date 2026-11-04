// 실제 세션의 사제/Carrier/객체/Player 메뉴를 같은 원본 Gump 입력과 커서 취득에 연결한다.
#include "DudeSpawnSupport.h"
#include "client/RawCampaignSession.h"
#include "client/RawCampaignCursor.h"
#include "client/RawButtonControls.h"
#include "client/RawMenuControls.h"
#include "client/RawKnowledgeMenu.h"
#include "client/QuickHelpHtml.h"
#include "client/RawUserHighlight.h"
#include "client/GameWorld.h"
#include "CameraTestDevices.h"
#include "client/GumpFormBinding.h"
#include "client/RawCameraView.h"
#include "client/ClientKencloudGlobals.h" //실제 월드 재그리기와 미니맵 갱신의 서로 다른 공유 주소를 검사한다.
#include "client/ClientMain.h" //Run 없이 실제 Client 전역을 일반 마우스 입력에 공급한다.
#include "client/UserInput.h" //편집 좌클릭을 메서드 직접 호출이 아닌 실제 입력 부모로 전달한다.
#include "client/CampaignObjectMenuClient.h" //제품과 같은 객체메뉴 반환을 창없는 트리에서 검사한다.
#include "client/RawObjectMenuOpen.h" //실제가상ac의 성공/거부와 열기음향 부모를 함께 검사한다.
#include "client/ClientDiagnostics.h" //현재로그버퍼와 깊이를 관찰하여 직접진단/Form재귀를 구별한다.
#include "client/RawChatWindowContext.h" //제품 Player 하위메뉴가 조회하는 실제 빈 Chat 입력버퍼를 같은 트리에 생성한다.
#include "client/ScriptCommand.h" //Chat 공장이 빌리는 실제80칸 문자열 고리의 수명을 검사 안에 유지한다.
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
    std::uint32_t demo{},demoReads{}; // 현재 Guide의 demo DWORD와 실제 조회 횟수이며 선택기 생성 후에도 바뀐다.
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
        result.player.log=[](std::uint32_t,std::string_view) {};result.missionOutcome.demo=[this] { ++demoReads;return demo; };return result;
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
    double gameSeconds{100.0}; // 비용 표시가 읽는 명시 게임 시계이며 벽시계와 섞지 않는다.
    // 창/게임 실행 없이 같은 자산과 실제 공통 UI 처리기를 준비한다.
    explicit ActualMenuUi(const netstorm::test::dudespawn::DudeAssets& source):assets(source),configuration([this](std::string_view path) { return assets.files.TryRead(path); }),fonts(source.files,"Arial"),buttons(OriginalEdition::Patch1078,{[] { return 0.0; },[this](std::uint32_t) { return mouse; },[] {},[this](std::uint32_t form,std::uint32_t flags) {
        if (!currentSession || form>65535U) throw std::logic_error("검사현재Form삭제호스트미연결");currentSession->GumpForms().Destroy(Sid{static_cast<std::uint16_t>(form)},flags);
    },{},{},[this](auto object) { if (!currentSession) throw std::logic_error("검사현재Gump객체투영Session없음");return ProjectGumpObject(currentSession->Core().pool,currentSession->CameraGlobals().Snapshot(),object); }}) {
        ConfigInterface::StartupOptions startup;startup.majorVersion=10;startup.minorVersion=78;startup.installDir=source.root.string();startup.cdDir=startup.installDir;configuration.Startup(startup); // 파일을저장하지않고실제설정치환문맥만설치한다.
        RawMenuHooks hooks;hooks.command=[this](std::uint32_t code,std::uint32_t id,std::uint32_t parameter) { commands.PushCommand(code,std::bit_cast<std::int32_t>(id),std::bit_cast<std::int32_t>(parameter)); };
        hooks.readInt=[](std::string_view,std::int32_t& value) { value=0; };hooks.sound=[](std::string_view) {};hooks.poll=[this](std::uint32_t) { return mouse; };hooks.cursor=[this](std::uint32_t value) { cursor=value; };
        hooks.objectForms=[this]() -> GumpFormHost& { if (!currentSession) throw std::logic_error("검사현재메뉴Form호스트미연결");return currentSession->GumpForms(); };
        hooks.gameSeconds=[this] { return gameSeconds; }; // 비용 표시의 생성/idle이 같은 현재 시각을 읽는다.
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

// 참조0의 일반 비용 표시와 실제 사제에 붙인 비용 표시가 같은 등록표/Form 수명에서 반납된다.
TEST_CASE(CampaignFloatingSp_ActualReferenceAndUiCleanup) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;ActualMenuUi ui(assets);
    const auto plain=ui.menus->ShowSp(25,320,240,0);const auto* plainNode=ui.menus->Node(plain);CHECK(plainNode && !(plainNode->flags&0x2040U) && !plainNode->form && plainNode->x==320 && plainNode->y==240);
    ui.buttons.KillGump(plain);ui.buttons.CleanupGumps();CHECK(!ui.menus->Node(plain)); // 참조0은 현재 Session/객체 투영 장치를 요구하지 않는다.
    auto services=external.Make();services.gumpNotice=ui.buttons.ObjectNotice();auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();ui.currentSession=&session;
    const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(priest.value);auto core=session.Core();const auto head=Get(core.pool.Slot(priest),6,2); // 기존 실제 종속 체인의 머리를 보존한다.
    const auto token=ui.menus->ShowSp(-75,777,555,priest.value);const auto* node=ui.menus->Node(token);CHECK(node && (node->flags&0x2040U)==0x2040U && node->object==priest.value);
    if (node) {
        const Sid form{static_cast<std::uint16_t>(node->form)};const auto id=node->id;CHECK(core.pool.Slot(form)[10]==3 && session.GumpForms().Parent(form)==priest && session.GumpForms().Owner(form)==id);
        CHECK(Get(core.pool.Slot(priest),6,2)==form.value && Get(core.pool.Slot(form),4,2)==head);CHECK(ui.buttons.RegisteredGump(id)==token);
        std::array<RenderCollectionEntry,64> entries{};const auto collected=ui.menus->CollectDrawing({0,0,1024,768},entries,64);CHECK(collected>0 && std::any_of(entries.begin(),entries.begin()+collected,[&](const auto& entry) { return entry.token==token; })); // 부착뿐 아니라 실제 공통 수집기의 월드 투영도 사용한다.
        std::array<std::uint32_t,256> logical{};for (std::size_t index=0;index<logical.size();++index) { const auto color=assets.game.Palette().Color(static_cast<std::uint8_t>(index));logical[index]=color.red|(std::uint32_t(color.green)<<8)|(std::uint32_t(color.blue)<<16); } // 보호 팔레트를 실제 글꼴/명암 출력에 전달한다.
        std::vector<std::uint8_t> pixels(1024*768,0);ui.menus->Draw(pixels,1024,768,ui.Palette(),BuildGumpShadeMaps(logical));CHECK(std::any_of(pixels.begin(),pixels.end(),[](auto value) { return value!=0; })); // 참조 비용 표시도 기존 StyleText/클립/픽셀 경로에서 글자를 출력한다.
        ui.buttons.KillGump(token);CHECK(ui.menus->Node(token) && (ui.menus->Node(token)->flags&0x10000U));ui.buttons.CleanupGumps();
        CHECK(!ui.menus->Node(token) && !ui.buttons.RegisteredGump(id) && (core.pool.Slot(form)[11]&1U) && Get(core.pool.Slot(priest),6,2)==head && !(core.pool.Slot(priest)[11]&3U));
    }
    ui.currentSession=nullptr;
}

// 월드 부모의 삭제 통지가 실제 비용 Gump를 Kill하고 후반 Cleanup이 소멸/등록 반납을 끝낸다.
TEST_CASE(CampaignFloatingSp_ParentDeletionKeepsDeferredUiLifetime) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;ActualMenuUi ui(assets);
    auto services=external.Make();services.gumpNotice=ui.buttons.ObjectNotice();auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();ui.currentSession=&session;
    auto& forms=session.GumpForms();const auto parent=forms.Create(2);const auto token=ui.menus->ShowSp(10,17,29,parent.value);const auto* node=ui.menus->Node(token);CHECK(node);
    if (node) {
        const Sid form{static_cast<std::uint16_t>(node->form)};const auto id=node->id;CHECK(forms.Parent(form)==parent && forms.Owner(form)==id);
        CHECK(session.World().Destroy(parent) && (session.Core().pool.Slot(parent)[11]&1U) && (session.Core().pool.Slot(form)[11]&1U));
        CHECK(ui.menus->Node(token) && (ui.menus->Node(token)->flags&0x10000U) && ui.buttons.RegisteredGump(id)==token);ui.buttons.CleanupGumps();CHECK(!ui.menus->Node(token) && !ui.buttons.RegisteredGump(id));
    }
    ui.currentSession=nullptr;
}

// UI 연결이 같은 실제 raw 위치/현재 카메라를 사용하며 넓은 DWORD를 하위 SID로 접지 않는다.
TEST_CASE(CampaignFloatingSp_ProjectCurrentObjectAndCameraWithoutSidTruncation) {
    SidPool pool(OriginalEdition::Patch1078,24000);const auto sid=pool.Allocate(2);auto raw=pool.AllocatedBytes(sid);RawCameraViewState camera;camera.pixels={17,-23}; // 명시 raw 위치와 원문 픽셀 snapshot이다.
    Put(raw,14,std::bit_cast<std::uint32_t>(1.25F));Put(raw,18,std::bit_cast<std::uint32_t>(2.5F));auto point=ProjectGumpObject(pool,camera,sid.value);CHECK(point.x==3 && point.y==51);
    Put(raw,14,std::bit_cast<std::uint32_t>(2.5F));Put(raw,18,std::bit_cast<std::uint32_t>(1.25F));point=ProjectGumpObject(pool,camera,sid.value);CHECK(point.x==23 && point.y==37);camera.pixels={25,-7};point=ProjectGumpObject(pool,camera,sid.value);CHECK(point.x==15 && point.y==21);
    CHECK(Throws([&] { ProjectGumpObject(pool,camera,0x10000U|sid.value); }));CHECK(Throws([&] { ProjectGumpObject(pool,camera,24000U); })); // 잘못된 표현/Pool 범위는 실제 읽기 전에 거부한다.
    SidPool cd(OriginalEdition::Cd1072,24000);CHECK(Throws([&] { ProjectGumpObject(cd,camera,0); })); // CD의 다른 raw 배치를10.78 offset으로 해석하지 않는다.
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

TEST_CASE(CampaignObjectMenus_CurrentCountVtable_BaseLevelIgnoresTypeConstructorPair) { // 현재 VT의 공통 레벨을 같은 실제 Summary/UI에서 조회한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;ActualMenuUi ui(assets);
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());session.Load();ui.currentSession=&session;auto core=session.Core();
    const auto type=static_cast<std::uint32_t>(assets.game.TypeTable().Find("windFactory"));const auto object=core.factory.Create(type,2);auto raw=core.pool.AllocatedBytes(object);const auto table=Get(raw,0);auto& current=session.World().Storage()->types[type];const auto savedConstructor=current.constructorAddress;std::unique_ptr<RawCampaignObjectMenus> chain;
    const std::array<std::pair<std::uint32_t,std::string_view>,2> cases{{{0," Level III"},{0x12345678," Level IV"}}};std::uint32_t level=2; // 생성자 metadata와 실제 VT를 독립 입력으로 유지한다.
    for (const auto& [constructor,title]:cases) { // 동일 객체/VT에서 현재 TYPE 생성자 metadata만 바뀌어도 공통 +80은 계속 실행된다.
        current.constructorAddress=constructor;raw[11]=static_cast<std::uint8_t>((raw[11]&31U)|(level++<<5));ui.infoTitles.clear();ui.infoLines.clear();
        ui.menus->Open({320,240},[&](const auto& widgets,const auto& controls,const auto& menus) { widgets.BeginFancy();chain=session.MakeObjectMenus(ui.Hooks(session,widgets,controls,menus));chain->Summary(object);chain->OpenObject(object); }); // 제품과 같은 Summary/controlled 제목/기본 메뉴를 작성한다.
        CHECK(Get(core.pool.Slot(object),0)==table && std::find(ui.infoTitles.begin(),ui.infoTitles.end(),title)!=ui.infoTitles.end());
        CHECK(std::find(ui.infoLines.begin(),ui.infoLines.end(),"\nOwner: ~iNobody~.")!=ui.infoLines.end());ui.menus->Close();chain.reset();CHECK(!ui.menus->HasMenus());
    }
    current.constructorAddress=savedConstructor;ui.currentSession=nullptr; // 현재 TYPE 입력과 실제 메뉴 수명을 원래 소유자에 반납한다.
}

TEST_CASE(CampaignObjectMenus_CurrentCountVtable_AltarDelegatesToCurrentDaisCount) { // 제단/다이스의 두 파생 +80은 실제 희생 없이 초기 프레임의 개수만 조회한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;ActualMenuUi ui(assets);
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());session.Load();ui.currentSession=&session;auto core=session.Core();auto& types=session.World().Storage()->types;
    const auto altar=core.factory.Create(164,2),dais=core.factory.Create(165,2);auto altarRaw=core.pool.AllocatedBytes(altar),daisRaw=core.pool.AllocatedBytes(dais);CHECK(Get(altarRaw,0)==0x5076b8U && Get(daisRaw,0)==0x507950U && Get(daisRaw,36)==0); // 생성자의 실제 VT와 Allocate의 초기 프레임0만 사용한다.
    constexpr float coordinate=220.0F; // 기존 맵 배치와 별개인 finder 명시 입력 위치다.
    Put(altarRaw,14,std::bit_cast<std::uint32_t>(coordinate));Put(altarRaw,18,std::bit_cast<std::uint32_t>(coordinate));Put(daisRaw,14,std::bit_cast<std::uint32_t>(coordinate));Put(daisRaw,18,std::bit_cast<std::uint32_t>(coordinate));
    daisRaw[11]=static_cast<std::uint8_t>((daisRaw[11]&31U)|(7U<<5));altarRaw[11]=static_cast<std::uint8_t>((altarRaw[11]&31U)|(5U<<5)); // 공통 state 레벨과 파생 Count1을 구별하기 위한 입력이다.
    auto& bucket=core.hash.Bucket(1,coordinate,coordinate);const auto savedHead=bucket;const auto savedNext=Get(daisRaw,4,2);Put(daisRaw,4,savedHead,2);bucket=dais.value; // 실제 일반 finder에만 void 다이스를 보이며 Pop/후처리/의식을 실행하지 않는다.
    const auto savedAltarConstructor=types[164].constructorAddress,savedDaisConstructor=types[165].constructorAddress;types[164].constructorAddress=0;types[165].constructorAddress=0;std::unique_ptr<RawCampaignObjectMenus> chain;
    const auto open=[&](Sid object) { ui.infoTitles.clear();ui.infoLines.clear();ui.menus->Open({320,240},[&](const auto& widgets,const auto& controls,const auto& menus) { widgets.BeginFancy();chain=session.MakeObjectMenus(ui.Hooks(session,widgets,controls,menus));chain->Summary(object);chain->OpenObject(object); }); }; // 현재 두 VT의 Summary를 같은 실제 메뉴 소유자에서 요청한다.
    open(dais);CHECK(Get(core.pool.Slot(dais),36)==0 && !ui.infoTitles.empty());ui.menus->Close();chain.reset(); // 다이스 Count는 상태 상위7과 무관한 초기 프레임 조회만 실행한다.
    open(altar);CHECK(std::find(ui.infoTitles.begin(),ui.infoTitles.end()," Level I")!=ui.infoTitles.end());CHECK(std::find(ui.infoTitles.begin(),ui.infoTitles.end()," Level V")==ui.infoTitles.end());ui.menus->Close();chain.reset(); // 실제 다이스 Count1을 제단 Describe가0으로 보정한다.
    CHECK(Get(core.pool.Slot(altar),0)==0x5076b8U && Get(core.pool.Slot(dais),0)==0x507950U && Get(core.pool.Slot(dais),36)==0 && (core.pool.Slot(dais)[11]>>5)==7);
    bucket=savedHead;Put(daisRaw,4,savedNext,2);types[164].constructorAddress=savedAltarConstructor;types[165].constructorAddress=savedDaisConstructor;ui.currentSession=nullptr; // finder 입력/현재 TYPE를 복원하고 메뉴와 Session의 대여를 끝낸다.
}

TEST_CASE(CampaignObjectMenus_CurrentCountVtable_UnknownTableFailsAndReopens) { // 미확인 VT를 TYPE 생성자나 공통 레벨로 임의 대체하지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;ActualMenuUi ui(assets);
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());session.Load();ui.currentSession=&session;auto core=session.Core();const auto type=static_cast<std::uint32_t>(assets.game.TypeTable().Find("windFactory"));const auto object=core.factory.Create(type,2);auto raw=core.pool.AllocatedBytes(object);const auto savedTable=Get(raw,0);std::unique_ptr<RawCampaignObjectMenus> chain;
    const auto open=[&] { ui.infoTitles.clear();ui.infoLines.clear();ui.menus->Open({320,240},[&](const auto& widgets,const auto& controls,const auto& menus) { widgets.BeginFancy();chain=session.MakeObjectMenus(ui.Hooks(session,widgets,controls,menus));chain->Summary(object);chain->OpenObject(object); }); }; // 지원 VT로 builder/controlled 스택을 완성한 실제 메뉴만 연다.
    open();CHECK(ui.menus->HasMenus() && std::find(ui.infoTitles.begin(),ui.infoTitles.end()," Level I")!=ui.infoTitles.end());const auto titlesBefore=ui.infoTitles,linesBefore=ui.infoLines;Put(raw,0,0x12345678);const auto before=std::vector<std::uint8_t>(raw.begin(),raw.end()); // 메뉴 생성이 끝난 뒤 count의 현재 VT 실패만 독립 입력으로 준다.
    CHECK(Throws([&] { chain->Summary(object); }));CHECK(std::equal(before.begin(),before.end(),core.pool.Slot(object).begin()) && ui.infoTitles==titlesBefore && ui.infoLines==linesBefore && ui.menus->HasMenus()); // builder 밖의 count 실패는 객체 바이트와 기존 제목/머리 출력을 변경하지 않는다.
    Put(raw,0,savedTable);ui.menus->Close();chain.reset();CHECK(!ui.menus->HasMenus());open();CHECK(std::find(ui.infoTitles.begin(),ui.infoTitles.end()," Level I")!=ui.infoTitles.end());ui.menus->Close();chain.reset();CHECK(!ui.menus->HasMenus());ui.currentSession=nullptr;
}

TEST_CASE(CampaignObjectMenus_NestedCurrentCountVtable_TypedDaisUsesItsCurrentVirtualEntry) { // 제단이 찾은 TYPE165의 실제 현재 +80을 같은 Summary에서 다시 분배한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;ActualMenuUi ui(assets);
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());session.Load();ui.currentSession=&session;auto core=session.Core();
    const auto altar=core.factory.Create(164,2),dais=core.factory.Create(165,2);auto altarRaw=core.pool.AllocatedBytes(altar),daisRaw=core.pool.AllocatedBytes(dais);const auto daisTable=Get(daisRaw,0);CHECK(Get(altarRaw,0)==0x5076b8U && daisTable==0x507950U && Get(daisRaw,36)==0);
    constexpr float coordinate=220.0F; // finder의 독립 명시 입력이며 새 건설/Pop으로 등록하지 않는다.
    Put(altarRaw,14,std::bit_cast<std::uint32_t>(coordinate));Put(altarRaw,18,std::bit_cast<std::uint32_t>(coordinate));Put(daisRaw,14,std::bit_cast<std::uint32_t>(coordinate));Put(daisRaw,18,std::bit_cast<std::uint32_t>(coordinate));daisRaw[11]=static_cast<std::uint8_t>((daisRaw[11]&31U)|(7U<<5));
    auto& bucket=core.hash.Bucket(1,coordinate,coordinate);const auto savedHead=bucket;const auto savedNext=Get(daisRaw,4,2);Put(daisRaw,4,savedHead,2);bucket=dais.value;std::unique_ptr<RawCampaignObjectMenus> chain; // 원래 해시/next는 마지막에 복원한다.
    const std::array<std::pair<std::uint32_t,std::string_view>,2> cases{{{0x501dd0," Level VII"},{0x507950," Level I"}}}; // 공통 레벨7과 초기 프레임의 다이스 Count1은 서로 다른 기대값이다.
    for (const auto& [table,title]:cases) { // TYPE165/프레임0/레벨7은 그대로 두고 현재 VT만 변경한다.
        Put(daisRaw,0,table);ui.infoTitles.clear();ui.infoLines.clear();
        ui.menus->Open({320,240},[&](const auto& widgets,const auto& controls,const auto& menus) { widgets.BeginFancy();chain=session.MakeObjectMenus(ui.Hooks(session,widgets,controls,menus));chain->Summary(altar);chain->OpenObject(altar); }); // 메뉴→제단 Count→현재 다이스 VT의 공통/파생 Count를 실행한다.
        CHECK(std::find(ui.infoTitles.begin(),ui.infoTitles.end(),title)!=ui.infoTitles.end());CHECK(daisRaw[10]==165 && (daisRaw[11]>>5)==7 && Get(daisRaw,36)==0 && Get(daisRaw,0)==table);ui.menus->Close();chain.reset();CHECK(!ui.menus->HasMenus());
    }
    Put(daisRaw,0,daisTable);bucket=savedHead;Put(daisRaw,4,savedNext,2);ui.currentSession=nullptr; // 가상 표/검색 입력과 실제 메뉴 대여를 원래 소유자에 반납한다.
}

TEST_CASE(CampaignObjectMenus_NestedCurrentCountVtable_CycleFailsAndUnwindsForNextMenu) { // 동일 SID로 재진입하는 가상 Count 순환을0으로 숨기지 않고 복구 뒤 다시 조회한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;ActualMenuUi ui(assets);
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());session.Load();ui.currentSession=&session;auto core=session.Core();
    const auto altar=core.factory.Create(164,2),dais=core.factory.Create(165,2);auto altarRaw=core.pool.AllocatedBytes(altar),daisRaw=core.pool.AllocatedBytes(dais);const auto daisTable=Get(daisRaw,0);CHECK(Get(altarRaw,0)==0x5076b8U && daisTable==0x507950U && Get(daisRaw,36)==0);
    constexpr float coordinate=220.0F; // 일반 finder가 같은 TYPE165를 다시 찾게 할 명시 입력 좌표다.
    Put(altarRaw,14,std::bit_cast<std::uint32_t>(coordinate));Put(altarRaw,18,std::bit_cast<std::uint32_t>(coordinate));Put(daisRaw,14,std::bit_cast<std::uint32_t>(coordinate));Put(daisRaw,18,std::bit_cast<std::uint32_t>(coordinate));
    auto& bucket=core.hash.Bucket(1,coordinate,coordinate);const auto savedHead=bucket;const auto savedNext=Get(daisRaw,4,2);Put(daisRaw,4,savedHead,2);bucket=dais.value;std::unique_ptr<RawCampaignObjectMenus> chain;
    const auto open=[&] { ui.infoTitles.clear();ui.infoLines.clear();ui.menus->Open({320,240},[&](const auto& widgets,const auto& controls,const auto& menus) { widgets.BeginFancy();chain=session.MakeObjectMenus(ui.Hooks(session,widgets,controls,menus));chain->Summary(altar);chain->OpenObject(altar); }); }; // 정상 VT로 실제 builder/controlled 메뉴를 먼저 완성한다.
    open();CHECK(std::find(ui.infoTitles.begin(),ui.infoTitles.end()," Level I")!=ui.infoTitles.end());const auto titlesBefore=ui.infoTitles,linesBefore=ui.infoLines;Put(daisRaw,0,0x5076b8);const auto altarBefore=std::vector<std::uint8_t>(altarRaw.begin(),altarRaw.end()),daisBefore=std::vector<std::uint8_t>(daisRaw.begin(),daisRaw.end()); // 다이스의 현재 VT만 제단으로 바꾸면 자기 자신을 재조회한다.
    std::string cycleError;try { chain->Summary(altar); }catch (const std::logic_error& error) { cycleError=error.what(); } // 실패는 builder 밖의 count 조회에서만 선택한다.
    CHECK(cycleError=="현재 가상80 조회의 SID 재귀 순환");CHECK(ui.infoTitles==titlesBefore && ui.infoLines==linesBefore && ui.menus->HasMenus() && std::equal(altarBefore.begin(),altarBefore.end(),core.pool.Slot(altar).begin()) && std::equal(daisBefore.begin(),daisBefore.end(),core.pool.Slot(dais).begin())); // 순환 실패는 객체와 기존 UI 출력을 바꾸지 않는다.
    Put(daisRaw,0,daisTable);ui.menus->Close();chain.reset();open();CHECK(std::find(ui.infoTitles.begin(),ui.infoTitles.end()," Level I")!=ui.infoTitles.end());ui.menus->Close();chain.reset();CHECK(!ui.menus->HasMenus()); // 예외 뒤 활성 SID가 남았다면 같은 두 객체의 재조회가 실패한다.
    bucket=savedHead;Put(daisRaw,4,savedNext,2);ui.currentSession=nullptr;
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

// 실제 Session factory가 현재 미션 관문을 연결하며 초기 demo 값이나 호출자의 가짜 관문을 보관하지 않는다.
TEST_CASE(CampaignUserSelection_CurrentMissionDemoBlocksBeforeSelectionEffects) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());session.Load();
    const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(priest.value);auto core=session.Core();
    std::uint32_t selected{},mouseType{},paused{},enabled{1},dumps{},highlights{},fakeReads{};std::array<std::uint32_t,2> remembered{};std::array<std::uint32_t,1> multiple{priest.value};std::int32_t count{}; // 같은 실제 선택과 명시 UI 관찰이다.
    UserSelectionHooks hooks;hooks.paused=[&] { return paused; };hooks.enabled=[&] { return enabled; };hooks.blocked=[&] { ++fakeReads;return 0U; };
    hooks.dump=[&](Sid sid) { CHECK(sid==priest);++dumps; };hooks.highlight=[&](auto mode) { CHECK(mode==0);++highlights; };hooks.assertion=hooks.previewAssertion=[] { throw std::logic_error("현재 미션 선택 검사 예상 밖 진단"); };
    auto selection=session.MakeUserSelection({selected,mouseType,remembered,multiple,count},std::move(hooks));external.demoReads=0;
    CHECK(selection->Select(priest.value,0,0)==priest.value && selected==priest.value && dumps==1 && (core.pool.Slot(priest)[40]&4));CHECK(external.demoReads==1 && fakeReads==0);
    count=1;const auto initialHighlights=highlights; // 관문 뒤 가상 통지/표시/강조가 아직 실행되지 않아야 한다.
    for (const std::uint32_t value:{1U,2U,0x80000000U,0xffffffffU}) { // 모든 비영 DWORD를 차단하며 SID 유효성 검사보다 먼저 반환한다.
        external.demo=value;const auto reads=external.demoReads;
        CHECK(!Throws([&] { CHECK(selection->Select(0xffffffffU,1,1)==0); }));CHECK(selection->Select(0,0,0)==0);
        CHECK(selected==priest.value && count==1 && multiple[0]==priest.value && dumps==1 && highlights==initialHighlights && (core.pool.Slot(priest)[40]&4));CHECK(external.demoReads==reads+2 && fakeReads==0);
    }
    const auto reads=external.demoReads;paused=1;CHECK(selection->Select(priest.value,0,0)==0);paused=0;enabled=0;CHECK(selection->Select(0,0,0)==0);CHECK(external.demoReads==reads && selected==priest.value && count==1); // 원문 앞쪽 정지/비활성 관문은 demo까지 읽지 않는다.
    enabled=1;selection->Clear();CHECK(external.demoReads==reads && selected==0 && count==0 && !(core.pool.Slot(priest)[40]&4)); // 독립 Clear는 demo를 읽지 않고 미션 전환 정리를 수행한다.
    external.demo=0;CHECK(selection->Select(priest.value,0,0)==priest.value);CHECK(selection->Select(0,0,0)==0 && selected==0 && !(core.pool.Slot(priest)[40]&4));CHECK(fakeReads==0);
}

// 실제 World 입력의 Select/ESC 초기화와 미션 전환 Clear가 동일한 현재 Guide/선택 저장소를 사용한다.
TEST_CASE(GameWorldUserSelection_CurrentMissionDemoKeepsResetAndClearDistinct) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();services.cameraCommit=netstorm::test::camera::CommitScreenless;
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),{},"thewarbegins",std::move(services),{640,480});
    UserSelectionHooks selection;selection.paused=[] { return 0U; };world.BindUserInput(std::move(selection),{},{});auto& owner=world.SelectionGlobals();auto& session=world.Session();const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(priest.value);auto raw=session.Core().pool.AllocatedBytes(priest);
    owner.selected=priest.value;owner.multiple={priest.value};owner.multipleCount=1;raw[40]|=4;external.demo=0x80000000U;external.demoReads=0;
    world.ResetSelection();CHECK(owner.selected==priest.value && owner.multipleCount==1 && (raw[40]&4) && external.demoReads==1);
    CHECK(!Throws([&] { world.Select(0xffffffffU); }));CHECK(owner.selected==priest.value && owner.multipleCount==1 && external.demoReads==2);
    world.ClearSelection();CHECK(owner.selected==0 && owner.multipleCount==0 && !(raw[40]&4) && external.demoReads==2);
    owner.selected=priest.value;owner.multipleCount=1;raw[40]|=4;external.demo=0;world.ResetSelection();CHECK(owner.selected==0 && owner.multipleCount==0 && !(raw[40]&4) && external.demoReads==3);
}

TEST_CASE(GameWorldEditorOwner_RightClickUsesWorldRedrawAndKeepsMinimapDirty) { //선택이 정지로 거부되어도 원문4d5290은 클릭한 객체의 같은 소유자를 뒤에서 읽는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();services.cameraCommit=netstorm::test::camera::CommitScreenless;
    auto gameplay=std::make_shared<ClientGameplayGlobals>();auto kencloud=std::make_shared<ClientKencloudGlobals>();services.gameplayGlobals=gameplay;services.kencloudGlobals=kencloud; //복사본이 아닌 Session과 같은 두 전역 소유자다.
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),{},"thewarbegins",std::move(services),{640,480});
    UserSelectionHooks selection;selection.paused=[] { return 1U; };world.BindUserInput(std::move(selection),{},{});auto& session=world.Session();const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(priest.value);
    gameplay->world.editor=1;gameplay->selection.selected=0;kencloud->WorldRedraw()=0xffffffffU;gameplay->MapRevision()=0x12345678U;world.PrepareEditorObjectMenu(priest.value);
    CHECK(gameplay->selection.selected==0 && gameplay->player.templateOwner==1);CHECK(kencloud->WorldRedraw()==0);CHECK(gameplay->MapRevision()==0x12345678U);
    auto raw=session.Core().pool.AllocatedBytes(priest);const auto saved=raw[34];raw[34]=0;kencloud->WorldRedraw()=17;world.PrepareEditorObjectMenu(priest.value);raw[34]=saved; //중립 입력은 명시 물리 검사이며 실제 소유자 변경/포획을 실행하지 않는다.
    CHECK(kencloud->WorldRedraw()==17 && gameplay->MapRevision()==0x12345678U && gameplay->player.templateOwner==1);
    gameplay->world.editor=0;world.PrepareEditorObjectMenu(0xffffffffU);CHECK(kencloud->WorldRedraw()==17 && gameplay->MapRevision()==0x12345678U);
}

// 비활성 선택이 보존한 현재 객체도 일반 좌클릭 후반의 편집 Core 추적을 실행한다.
TEST_CASE(GameWorldEditorPrimary_ActualInputFollowsPreservedSelection) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ClientOptions options;options.gameDirectory=assets.root;options.noAudio=true;Client client(std::move(options));MenuServices external;auto services=external.Make();
    services.cameraCommit=netstorm::test::camera::CommitScreenless;services.gameplayGlobals=client.GameplayGlobals();services.kencloudGlobals=client.KencloudGlobals();services.coreDataGlobals=client.CoreDataGlobals();services.subscriberGlobals=client.SubscriberGlobals();services.playerRecords=client.PlayerRecords(); // 실제 입력/Session이 같은 전역·Subscriber·Core에 연결된 Player 소유자를 빌린다.
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),{},"thewarbegins",std::move(services),{640,480});
    UserSelectionHooks selection;selection.paused=[] { return 0U; };world.BindUserInput(std::move(selection),{},{});auto& session=world.Session();auto& gameplay=*client.GameplayGlobals();auto& redraw=client.KencloudGlobals()->WorldRedraw();
    const auto priest=session.FindPriestParticipation(1);CHECK(priest);gameplay.world.editor=1;gameplay.selection.enabled=0;gameplay.selection.mode=0;gameplay.selection.selected=priest;gameplay.player.templateOwner=7;redraw=0xffffffffU;gameplay.MapRevision()=0x12345678U;session.CursorDropSettings().lockDepth=1;
    UserInput input(client,world);CHECK(!input.Event({InputCode::kLeftButton,0,0})); // 맵 밖 좌표의 후보0을 disabled 선택이 거부하여 현재 SID가 유지된다.
    CHECK(gameplay.selection.selected==priest && gameplay.player.templateOwner==1);CHECK(redraw==0);CHECK(gameplay.MapRevision()==0x12345678U);
    const auto bridge=session.World().Create(82,2);CHECK(bridge.value);const auto frameTable=assets.game.Types()[82-kFirstAssetTypeNumber].definition.FrameTable();const auto frameFlags=[&] { const auto raw=session.Core().pool.Slot(bridge);const auto frame=Get(raw,36);const auto codes=frameTable.Codes();if (frame>=codes.size()) throw std::out_of_range("편집 입력 검사 실제 다리 프레임 범위");return codes[frame].flags; }; //포획/희생 대신 편집용 새 다리의 실제 프레임만 관찰한다.
    CHECK(session.Core().pool.Slot(bridge)[34]==0);CHECK((frameFlags()&0x60U)==0);gameplay.selection.selected=bridge.value;gameplay.selection.mode=0;gameplay.player.templateOwner=7;
    CHECK(!input.Event({InputCode::kLeftButton|InputCode::kControl,0,0}));CHECK(session.Core().pool.Slot(bridge)[34]==session.LocalOwner());CHECK(gameplay.player.templateOwner==1 && gameplay.selection.selected==bridge.value);CHECK(redraw==1);CHECK((frameFlags()&0x60U)==0);
    CHECK(!input.Event({InputCode::kLeftButton|InputCode::kControl,0,0}));CHECK(frameFlags()&0x40U);
    CHECK(!input.Event({InputCode::kLeftButton|InputCode::kControl,0,0}));CHECK(frameFlags()&0x20U);
    CHECK(!input.Event({InputCode::kLeftButton|InputCode::kControl,0,0}));CHECK((frameFlags()&0x60U)==0);
}

// 비기본 모드의 우클릭은 객체 메뉴/Pick 대신 먼저 강조를 해제하고 현재 선택 관문을 따른다.
TEST_CASE(GameWorldPointerModes_EmptyPickupCancelAndPollingCoordinates) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ClientOptions options;options.gameDirectory=assets.root;options.noAudio=true;Client client(std::move(options));MenuServices external;auto services=external.Make();
    services.cameraCommit=netstorm::test::camera::CommitScreenless;services.gameplayGlobals=client.GameplayGlobals();services.kencloudGlobals=client.KencloudGlobals();services.coreDataGlobals=client.CoreDataGlobals();services.subscriberGlobals=client.SubscriberGlobals();services.playerRecords=client.PlayerRecords(); //같은 실제 입력/Session 소유자를 사용한다.
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),{},"thewarbegins",std::move(services),{640,480});
    std::vector<std::uint32_t> cursors;UserSelectionHooks selection;selection.paused=[] { return 1U; };UserHighlightHooks highlight;highlight.selectCursor=[&](auto value) { cursors.push_back(value); };world.BindUserInput(std::move(selection),std::move(highlight),{});
    auto& session=world.Session();auto& globals=world.SelectionGlobals();const auto priest=session.FindPriestParticipation(1);CHECK(priest);session.CursorDropSettings().lockDepth=1;globals.enabled=0;globals.selected=priest;UserInput input(client,world);world.SetMousePoint({320,200});
    globals.mode=1;globals.mouseType=82;CHECK(!input.Event({InputCode::kLeftButton,0,0}));CHECK(globals.mode==0 && globals.mouseType==0 && globals.selected==priest);CHECK(globals.mouse.x==320 && globals.mouse.y==200);CHECK(cursors==std::vector<std::uint32_t>{1});
    for (const auto mode:{1U,2U,3U,4U,5U}) { //멀티플레이 전용6은 새 검사에서 제외한다. enabled0이어도 선행 강조0은 적용한다.
        globals.mode=mode;globals.mouseType=82;cursors.clear();CHECK(!input.Event({InputCode::kRightButton,0,0}));CHECK(globals.mode==0 && globals.mouseType==0 && globals.selected==priest);CHECK(cursors==std::vector<std::uint32_t>{1});
    }
    globals.enabled=1;globals.mode=2;cursors.clear();CHECK(!input.Event({InputCode::kRightButton,0,0}));CHECK(globals.selected==0 && globals.mode==0 && cursors==std::vector<std::uint32_t>{1}); //정지1이어도0번 선택 해제는 원문대로 실행한다.
    globals.selected=priest;globals.mode=2;cursors.clear();CHECK(!input.Event({InputCode::kRightButton|InputCode::kOutside,0,0}));CHECK(globals.selected==0 && globals.mode==0 && cursors==std::vector<std::uint32_t>{1}); // 큐 outside도 같은 선행 강조0/선택0을 실행한다.
    globals.selected=priest;globals.mode=1;globals.mouseType=82;cursors.clear();CHECK(!input.Event({InputCode::kLeftButton|InputCode::kOutside,0,0}));CHECK(globals.selected==priest && globals.mode==0 && globals.mouseType==0 && cursors==std::vector<std::uint32_t>{1}); // 대상0인 편집 좌클릭도 원문의 강조 해제 꼬리를 실행한다.
    globals.selected=priest;globals.mode=0;CHECK(!input.Event({InputCode::kRightButton,0,0}));CHECK(globals.selected==0 && globals.mode==0);CHECK(globals.mouse.x==320 && globals.mouse.y==200);
    const auto type=static_cast<std::uint32_t>(assets.game.TypeTable().Find("windFactory"));const auto source=session.World().Create(type,2);session.Core().owner.Set(source,1);session.World().Pop(source,220,220,0);client.GameplayGlobals()->Camera().ApplyPixelsToRendererAndSnapshot(220*16-320,220*11-240); //전투 대신 편집용 워크샾을 빈 좌표에 배치하고 실제 카메라 setter로 클릭 영역을 만든다.
    const auto center=world.ToScreen(220,220);std::optional<ScreenPoint> hit;
    for (int y=center.y-30;y<=center.y+30 && !hit;++y) for (int x=center.x-30;x<=center.x+30 && !hit;++x) { if (world.Pick({x,y})==source.value) hit=ScreenPoint{x,y}; } //실제SHP 투명픽셀/해시Pick에서 해당 객체를 찾으며 선택 결과를 주입하지 않는다.
    CHECK(hit.has_value());if (!hit) return;ActualMenuUi ui(assets);ui.currentSession=&session;ui.mouse={InputCode::kLeftButton,hit->x,hit->y};auto effects=ui.CursorEffects();effects.lifecycle.highlight=[&](auto value) { world.Highlight(value); };session.Cursor().SetEffects(std::move(effects));globals.mode=1;globals.selected=0;const auto before=session.Sp(1);
    CHECK(!input.Event(ui.mouse));const auto form=CursorPreviewSid(session.CursorDropSettings().preview);CHECK(form.value && session.Core().pool.Slot(form)[10]==6);CHECK(Get(session.Core().pool.Slot(form),18,2)==type);CHECK(session.CursorDropSettings().costFlags==0x100 && session.CursorDropSettings().lockDepth==0);CHECK(globals.mode==0 && globals.selected==0 && session.Sp(1)==before);CHECK(session.Core().pool.Slot(source)[11]&4U);
    session.Cursor().Cancel();ui.currentSession=nullptr; //실제생성된커서/미리보기를같은수명으로정리하며플레이승리나공격을실행하지않는다.
}
// 실제 제품 메뉴의 반환값/부착/소리 부모와 Ctrl+special의 직접 진단을 같은 현재 월드에서 확인한다.
TEST_CASE(CampaignPointerMenu_ActualReturnSoundAndDirectSpecialTrace) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ClientOptions options;options.gameDirectory=assets.root;options.noAudio=true;Client client(std::move(options));client.Configuration().Set("helpWord","Help");MenuServices external;auto services=external.Make();
    services.cameraCommit=netstorm::test::camera::CommitScreenless;services.gameplayGlobals=client.GameplayGlobals();services.kencloudGlobals=client.KencloudGlobals();services.coreDataGlobals=client.CoreDataGlobals();services.subscriberGlobals=client.SubscriberGlobals();services.playerRecords=client.PlayerRecords(); //제품과 같은 현재 전역을 사용한다.
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());GameWorld world(assets.game,assets.files,std::move(fort),{},"thewarbegins",std::move(services),{640,480});UserSelectionHooks selection;selection.paused=[] { return 1U; };world.BindUserInput(std::move(selection),{},{});auto& session=world.Session();auto core=session.Core();
    const auto type=static_cast<std::uint32_t>(assets.game.TypeTable().Find("windFactory"));const auto source=session.World().Create(type,2);core.owner.Set(source,1);session.World().Pop(source,220,220,0);client.GameplayGlobals()->Camera().ApplyPixelsToRendererAndSnapshot(220*16-320,220*11-240);
    ScriptStringRing strings;ActualMenuUi ui(assets);ui.currentSession=&session;RawChatWindowContext chat;chat.globals=client.ChatWindowGlobals();chat.commands=client.DotCommandGlobals();chat.editor.now=[] { return 0.0; };chat.translate=[](auto text) { return std::string(text); }; //문자열고리가UI소멸보다오래살며Player조회에쓰는실제초기빈Console/공유로그를생성한다. 전송은실행하지않는다.
    chat.text=[&](auto token) { return strings.Resolve(token); };chat.storeText=[&](auto text) { return strings.Store(text); };chat.recipient=[](auto)->bool { throw std::logic_error("빈 초기Chat 검사의 예상 밖 비어있지 않은 접두 검사"); };
    chat.read=[&](auto address)->std::uint32_t { switch (address) { case 0x594fc8:return core.ownerMode.challenge;case 0x594fbc:return core.ownerMode.battle;case 0x563e38:return ui.buttons.RootToken();case 0x5cae94:return 304;case 0x5cae98:return 229;default:throw std::logic_error("포인터 메뉴 검사의 예상 밖 Chat 현재전역"); } };static_cast<void>(ui.menus->CreateChatWindow(std::move(chat)));
    CampaignObjectMenuClient menu(client,ui.buttons,[](std::string_view) {});std::vector<std::string> sounds;const auto play=[&](auto name) { sounds.emplace_back(name); };const InputEvent event{InputCode::kRightButton,304,229};
    auto raw=core.pool.AllocatedBytes(source);raw[40]|=1;CHECK(OpenPointerObjectMenu([&] { return menu.Open(world,source,event); },play)==0);CHECK(!menu.Active() && sounds.empty());
    raw=core.pool.AllocatedBytes(source);raw[40]&=0xfeU;CHECK(OpenPointerObjectMenu([&] { return menu.Open(world,source,event); },play)==1);CHECK(menu.Active() && sounds==std::vector<std::string>{"opengump.wav"});menu.Clear();
    const auto content=session.CreateContained(source,154,7,0x20,0x10,0);CHECK(content.value);auto& diagnostic=client.DiagnosticGlobals();diagnostic.depth=7;diagnostic.log.indent=3;diagnostic.log.buffer.fill(0);auto& gameplay=*client.GameplayGlobals();gameplay.selection.enabled=0;gameplay.selection.selected=0;gameplay.selection.mode=0;gameplay.player.special=1;session.CursorDropSettings().lockDepth=1;world.SetMousePoint({123,145});UserInput input(client,world);
    CHECK(!input.Event({InputCode::kLeftButton|InputCode::kControl,304,229}));const std::string line(diagnostic.log.buffer.data());CHECK(line.find("Main")!=std::string::npos);CHECK(line.find("END") ==std::string::npos);CHECK(diagnostic.depth==7 && diagnostic.log.indent==3);CHECK(gameplay.selection.mouse.x==123 && gameplay.selection.mouse.y==145); //같은 지점의 자식Form은 재귀 출력하지 않으며 현재 들여쓰기와 폴링도 유지한다.
    diagnostic.log.buffer.fill(0);gameplay.player.special=0;CHECK(!input.Event({InputCode::kLeftButton|InputCode::kControl,304,229}));CHECK(diagnostic.log.buffer[0]==0);ui.menus->CloseChatWindows();ui.currentSession=nullptr;
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

// Native 중복 공간 입력을 실제 Session의 후처리 장치에 공급해 경고 뒤 기존 객체 삭제를 확인한다.
TEST_CASE(CampaignFactory_WorkshopDuplicatePreservesWarningAndDeletesOld) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make(); // 실제 보호 단일 플레이 자산과 창 없는 기존 장치를 쓴다.
    RawCampaignSession* current{};std::vector<std::array<std::int32_t,3>> warnings; // 경고가 삭제보다 먼저 기존 SID/좌표를 읽었는지 관찰한다.
    services.assets.factory.warnDuplicate=[&](Sid sid,std::int32_t x,std::int32_t y) {
        CHECK(current);if (!current) throw std::logic_error("워크샾 경고의 현재 검사 Session 누락");const auto raw=current->Core().pool.Slot(sid); // 같은 실제 슬롯을 경고 시점에 읽는다.
        CHECK(!(raw[11]&3U));warnings.push_back({sid.value,x,y});
    };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));current=&session;session.Load();warnings.clear();auto core=session.Core(); // 생성/삭제는 현재 실제 월드가 수행한다.
    const auto type=static_cast<std::uint32_t>(assets.game.TypeTable().Find("windFactory"));constexpr float x=220.0F,y=220.0F; // 기존 객체와 겹치지 않는 검사 좌표이며 합법 건설 위치라는 주장은 하지 않는다.
    const auto old=session.World().Create(type,2);core.owner.Set(old,1);session.World().Pop(old,x,y,0);CHECK(warnings.empty()); // 원본 맵에 없는 타입을 실제 공통 Pop으로 먼저 배치한다.
    const auto replacement=session.World().Create(type,2);core.owner.Set(replacement,1);auto input=core.pool.AllocatedBytes(replacement);Put(input,14,std::bit_cast<std::uint32_t>(x));Put(input,18,std::bit_cast<std::uint32_t>(y)); // 새 Factory 후처리가 읽을 좌표만 명시 입력으로 공급한다.
    const auto inputState=input[11];core.postPop.Activate(replacement,9); // 일반 Pop의 spot 겹침 거부와 분리한 Native +20 장치 접합 검사다. GUI 배치 성공을 합성하지 않는다.
    CHECK(warnings.size()==1);if (!warnings.empty()) CHECK(warnings[0][0]==old.value && warnings[0][1]==static_cast<std::int32_t>(x) && warnings[0][2]==static_cast<std::int32_t>(y));
    CHECK(core.pool.Slot(old)[11]&1U);CHECK(core.pool.Slot(replacement)[11]==inputState);CHECK(core.pool.Slot(replacement)[10]==type);CHECK(core.destroy.PreDepth()==0 && core.destroy.PostDepth()==0);
}

#include "CampaignDetonateInputTests.inc" // 같은 실제 메뉴/자산 장치를 사용해 Detonate 입력을 확인한다.
#include "CampaignEditorCheatMenuTests.inc" // 실제 controlled 메뉴의 편집 전용 CHEAT 노출과 원문 추가 조건을 확인한다(725).

TEST_CASE(FactoryTakeDiagnostics_ActualSessionUsesSameStorageDetail) { // 실제 비멀티 Session 설치 뒤 같은 Storage 상세와 서비스 진단을 사용한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();
    std::optional<std::string> detail;std::uint32_t assertions{};RawCampaignSession* current{};Sid object{};
    services.ai.assertion=[&](auto expression) { CHECK(current && expression=="0" && detail.has_value());CHECK(current->Core().pool.Slot(object)[10]==75);++assertions; };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));current=&session;session.Load();
    session.World().Storage()->BindDisplayDiagnostics(detail,[](auto,auto,auto) { throw std::logic_error("Session은 실제 서비스 진단을 사용해야 한다"); });
    auto& factory=session.Core().factory;object=factory.Create(74);auto bytes=session.Core().pool.AllocatedBytes(object);bytes[10]=75;
    const auto expected=std::string("Invalid take. Server type ")+session.Core().types[74].name+", client type "+session.Core().types[75].name;
    CHECK(factory.Take(74,object)==object && assertions==1 && detail==expected && bytes[10]==74);
}

TEST_CASE(SquidPopParent_ActualSessionLateFrameLookup) { // 실제 Session이 다시 연결한 Pop도 좌표/spot 뒤 현재 SHP를 조회한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());session.Load();auto core=session.Core();
    auto& type=session.World().Storage()->types[74];type.flags1=0;type.flags2=1;type.footX=type.footY=1;std::array<int,2> point{};
    for (int y=1;y<256 && !point[0];++y) { // 기존 월드에서 비어 있는 실제 점유 칸을 찾는다.
        for (int x=1;x<256;++x) if (!core.spots[y*256+x]) { point={x,y};break; } // 원래 데이터를 지우거나 적/유닛을 조작하지 않는다.
    }
    CHECK(point[0]!=0);const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);
    for (std::size_t index=0;index<4;++index) raw[36+index]=0xff; // 명시한 새 슬롯의 현재 SHP 프레임만 범위 밖으로 둔다.
    CHECK(Throws([&] { session.World().Pop(source,float(point[0]),float(point[1]),0x40); }));
    CHECK(core.spots[point[1]*256+point[0]]==1 && raw[11]==4 && !(raw[40]&0x80U));
    CHECK(std::bit_cast<float>(std::uint32_t(raw[14])|(std::uint32_t(raw[15])<<8)|(std::uint32_t(raw[16])<<16)|(std::uint32_t(raw[17])<<24))==float(point[0]));
}

TEST_CASE(SquidActivate_ActualSessionUsesCurrentServiceAfterPostPop) { //Session이재연결한실제진단서비스와공통장부의현재깊이를확인한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();RawCampaignSession* current{};Sid source{};std::uint32_t diagnoses{},previous{};bool ready{};
    services.ai.assertion=[&](auto expression) { CHECK(ready && current && expression=="curPostPop == basePostPop");if (!ready || !current) throw std::logic_error("부모검사전의예상밖Session진단");auto core=current->Core();CHECK(core.bookkeeping.depth==8 && core.bookkeeping.GlobalCounts()[74]==previous+1);++diagnoses;core.bookkeeping.depth=2; };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));current=&session;session.Load();auto core=session.Core();
    auto& type=session.World().Storage()->types[74];type.flags1=type.flags2=0;type.footX=type.footY=1;source=core.factory.Create(74);core.pool.AllocatedBytes(source)[34]=1;
    core.postPop.BindPatchAiNotice([&](auto,Sid sid,bool) { CHECK(sid==source && core.bookkeeping.depth==3);core.bookkeeping.depth=9; });
    previous=core.bookkeeping.GlobalCounts()[74];core.bookkeeping.depth=2;ready=true;session.World().Pop(source,30,40,0x2000040); //noGraph인기존비멀티일반객체등록만실행하며Frame/공격/의식은호출하지않는다.
    CHECK(diagnoses==1 && core.bookkeeping.depth==2 && !(core.pool.Slot(source)[11]&4U));
}

TEST_CASE(RiftShapeAccess_ActualSessionRetainsInputFrameAndCurrentCost) { //실제Session의상세/e1뒤SHP와현재후처리비용을같은World에서확인한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());session.Load();auto core=session.Core();auto storage=session.World().Storage();
    auto& type=storage->types[74];type.name="session";type.flags1=0;type.flags2=1;type.footX=type.footY=1;auto& shape=storage->shapes[74];shape.loaded=true;shape.frameCount=1;shape.frames.resize(2);shape.frames[1].cellWidth=shape.frames[1].cellHeight=5;
    std::array<int,2> point{};for (int y=1;y<256 && !point[0];++y) { //기존실제점유의행을차례로확인한다.
        for (int x=1;x<256;++x) if (!core.spots[y*256+x]) { point={x,y};break; } //점유를지우지않고현재행의비어있는한칸을찾는다.
    }
    CHECK(point[0]!=0);const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);raw[36]=1;std::optional<std::string> detail;std::uint32_t diagnoses{};const auto before=core.bookkeeping.TotalCost();storage->BindChecking(RawCheckingFlag{0});
    storage->BindDisplayDiagnostics(detail,[&](auto expression,auto file,auto line) { CHECK(expression=="frameCheck( abstract, frame, false )" && file==R"(..\o\rifttype.h)" && line==0xe1);CHECK(detail=="Frame assertion failed. Asking for type=74 (session) frame=1 abs=0\n\nType has 1 frames\n" && core.spots[point[1]*256+point[0]]==1);++diagnoses;raw[36]=0;type.cost=21; });
    session.World().Pop(source,float(point[0]),float(point[1]),0x2000040);CHECK(diagnoses==1 && raw[33]==3 && raw[36]==0 && core.hash.Bucket(3,float(point[0]),float(point[1]))==source.value && core.bookkeeping.TotalCost()==before+21 && core.bookkeeping.depth==0);
}

TEST_CASE(PostPopCurrent_ActualSessionUsesCurrentOwnerAndRememberedSelection) { //실제AI통지뒤현재Client owner와같은World검색/선택거부tail을확인한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();auto gameplay=std::make_shared<ClientGameplayGlobals>();services.gameplayGlobals=gameplay;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();auto storage=session.World().Storage();
    for (const auto type:{74U,75U}) { auto& row=storage->types[type];row.flags1=row.flags2=0;row.footX=row.footY=1;auto& shape=storage->shapes[type];shape.loaded=true;shape.frameCount=1;shape.frames.resize(1);shape.frames[0].cellWidth=shape.frames[0].cellHeight=1; } //일반공통몸체만사용하고프레임/공격/의식함수를실행하지않는다.
    const auto found=core.factory.Create(74);session.World().Pop(found,10.5F,10.5F,0x2000040);auto& selectionOwner=gameplay->selection;selectionOwner.selected=0;selectionOwner.mouseType=74;selectionOwner.remembered={std::bit_cast<std::uint32_t>(10.5F),std::bit_cast<std::uint32_t>(10.5F)};UserSelectionHooks hooks;std::uint32_t pausedReads{};hooks.paused=[&] { ++pausedReads;return 1U; };auto selection=session.MakeUserSelection(selectionOwner.SelectionState(),std::move(hooks));
    const auto source=core.factory.Create(75);core.pool.AllocatedBytes(source)[34]=1;core.bookkeeping.localOwner=1;const auto beforeLocal=core.bookkeeping.LocalCounts()[75],beforeGlobal=core.bookkeeping.GlobalCounts()[75];std::uint32_t notices{};
    core.postPop.BindPatchAiNotice([&](auto owner,Sid sid,bool added) { CHECK(owner==1 && sid==source && added);++notices;core.pool.AllocatedBytes(source)[34]=2;gameplay->player.battleOwner=2; });session.World().Pop(source,11.5F,10.5F,0x2000040);
    CHECK(notices==1 && pausedReads==1 && selectionOwner.selected==0 && selectionOwner.mouseType==0 && session.World().Find(10.5F,10.5F,74)==found && core.bookkeeping.LocalCounts()[75]==beforeLocal+1 && core.bookkeeping.GlobalCounts()[75]==beforeGlobal+1 && core.bookkeeping.depth==0 && core.bookkeeping.localOwner==1);
    selection.reset();selectionOwner.selected=source.value;selectionOwner.mouseType=74; //weak수명반납뒤현재다른선택이면원문은Select없이반환한다.
    core.postPop.BindPatchAiNotice([](auto,Sid,bool) {});++core.bookkeeping.depth;core.postPop.PostPopBase(found,0x2000001);
    CHECK(selectionOwner.selected==source.value && selectionOwner.mouseType==74 && core.bookkeeping.depth==0);selectionOwner.selected=selectionOwner.mouseType=0;
}

TEST_CASE(CarrierParentCurrent_ActualSessionUsesCurrentTypeLoadingAndGeyser) { //현재VT의직접Damageable부모가같은World의지면하위와현재전역을소비한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();auto gameplay=std::make_shared<ClientGameplayGlobals>();services.gameplayGlobals=gameplay;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();
    const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);const auto saved=std::vector<std::uint8_t>(raw.begin(),raw.end());auto& type=session.World().Storage()->types[74];const auto savedFlags=type.flags1,savedGenus=type.flags2,savedGeyser=gameplay->GeyserType();std::array<int,2> point{};
    for (int y=2;y<256 && !point[0];++y) { //실제현재표면지도의빈칸을찾고기존지도를지우지않는다.
        for (int x=2;x<256;++x) if (!core.hash.Cell(0,x,y)) { point={x,y};break; } //보정된지면조회가실제빈칸을읽도록한좌표만선택한다.
    }
    CHECK(point[0]!=0);if (!point[0]) throw std::logic_error("지면하위검사의빈표면칸없음");
    const auto write=[&](std::size_t offset,std::uint32_t value) { for (std::size_t index=0;index<4;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8)); }; //명시검사슬롯의VT/좌표DWORD만원문폭으로쓴다.
    write(0,0x50a820);write(14,std::bit_cast<std::uint32_t>(float(point[0])-0.5F));write(18,std::bit_cast<std::uint32_t>(float(point[1])-0.5F));raw[11]=raw[40]=0;type.flags2=0;session.World().SetLoadingDepth(1);static_cast<void>(session.World().TakeChanged());
    const auto run=[&] { ++core.bookkeeping.depth;core.postPop.PostPopVirtual(source,0x2000000);CHECK(core.bookkeeping.depth==0); }; //최초flags/Frame/전투를실행하지않고현재VT+20의공통체인을호출한다.
    type.flags1&=~0x400U;run();CHECK(!session.World().TakeChanged());type.flags1|=0x400U;run();CHECK(session.World().TakeChanged()); //끄기/켜기둘다확인해고정사본으로는통과할수없게한다.
    gameplay->GeyserType()=74;run();CHECK(!session.World().TakeChanged());gameplay->GeyserType()=savedGeyser;session.World().SetLoadingDepth(0);run();CHECK(!session.World().TakeChanged());
    std::copy(saved.begin(),saved.end(),raw.begin());type.flags1=savedFlags;type.flags2=savedGenus;CHECK(core.bookkeeping.depth==0); //명시VT/상태/TYPE/전역변경을돌리고실제소유권변경검증으로확대하지않는다.
}

TEST_CASE(FactoryParentCurrent_ActualSessionRedispatchesCountAfterCommon) { // 같은 Session의 공통 효과 뒤 프레임 보정이 현재 VT의 가상80을 다시 선택한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());session.Load();auto core=session.Core();
    const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);const auto saved=std::vector<std::uint8_t>(raw.begin(),raw.end());auto storage=session.World().Storage();auto& type=storage->types[74];const auto savedFlags=type.flags1,savedGenus=type.flags2;const auto savedFrames=storage->frames[74];
    const auto write=[&](std::size_t offset,std::uint32_t value) { for (std::size_t index=0;index<4;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8)); }; // 새 일반 슬롯의 명시 VT·프레임 DWORD만 쓴다.
    write(0,0x509148);write(36,7);raw[11]=raw[40]=0;raw[34]=1;type.flags1=type.flags2=0;storage->frames[74]=RiftTypeFrames(std::vector<FrameCode>{{'A','A',0,0}});std::uint32_t notices{};
    core.postPop.BindPatchAiNotice([&](auto owner,Sid sid,bool added) { CHECK(owner==1 && sid==source && added);++notices;write(0,0); }); // 공통 부모의 도달한 콜백에서만 현재 VT를 바꾼다.
    ++core.bookkeeping.depth;CHECK(Throws([&] { core.postPop.PostPopVirtual(source,0x2000001); }));CHECK(notices==1 && core.bookkeeping.depth==0 && raw[36]==7);
    write(0,0x509148);write(36,0);++core.bookkeeping.depth;core.postPop.PostPopVirtual(source,0x2000000);CHECK(core.bookkeeping.depth==0 && raw[36]==0 && notices==1);
    std::copy(saved.begin(),saved.end(),raw.begin());type.flags1=savedFlags;type.flags2=savedGenus;storage->frames[74]=savedFrames; // 실제 Frame 틱·생산·전투·의식·멀티플레이는 실행하지 않는다.
}

TEST_CASE(FactoryGlobalsCurrent_ActualTimersShareClientAcrossSessions) { // 실제 일반 Factory의 예약 생성자가 같은 Client의 현재 두 TYPE DWORD를 미션 교체 뒤에도 사용한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);auto gameplay=std::make_shared<ClientGameplayGlobals>();CHECK(gameplay->FirstTimerFactoryType()==85 && gameplay->SecondTimerFactoryType()==87 && gameplay->OutpostType()==126);auto* firstAddress=&gameplay->FirstTimerFactoryType();gameplay->FirstTimerFactoryType()=74;gameplay->SecondTimerFactoryType()=75;
    for (std::uint32_t index=0;index<2;++index) { // 서로 다른 실제 Session의 같은 장기 저장소 주소와 예약 Form을 확인하고 틱은 실행하지 않는다.
        MenuServices external;auto services=external.Make();services.gameplayGlobals=gameplay;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();CHECK(&gameplay->FirstTimerFactoryType()==firstAddress && gameplay->FirstTimerFactoryType()==(index ? 75U : 74U) && gameplay->SecondTimerFactoryType()==(index ? 74U : 75U));
        const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);const auto write=[&](std::size_t offset,std::uint32_t value) { for (std::size_t byte=0;byte<4;++byte) raw[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8)); }; // 새 일반 슬롯의 명시 VT와 프레임 DWORD를 준비한다.
        write(0,0x509148);write(36,0);raw[11]=raw[40]=0;raw[34]=1;auto storage=session.World().Storage();storage->types[74].flags1=storage->types[74].flags2=0;storage->frames[74]=RiftTypeFrames(std::vector<FrameCode>{{'A','A',0,0}});core.postPop.BindPatchAiNotice([](auto,Sid,bool) {});
        ++core.bookkeeping.depth;core.postPop.PostPopVirtual(source,0x2000001);auto* chosen=core.processes.FindEvent(source,index ? 1U : 2U);CHECK(chosen && core.processes.FindEvent(source,5) && !core.processes.FindEvent(source,index ? 2U : 1U) && core.bookkeeping.depth==0);
        if (chosen) CHECK(chosen->Payload()==(index ? 0.5F : 8.0F) && chosen->Count()==0 && chosen->Time()==core.processState.Now());
        gameplay->FirstTimerFactoryType()=75;gameplay->SecondTimerFactoryType()=74; // 첫 Session 반납 뒤에도 새85/87을 쓰지 않도록 현재값을 보존한다.
    }
    CHECK(gameplay->FirstTimerFactoryType()==75 && gameplay->SecondTimerFactoryType()==74); // 실제 생산/행동·전투·의식·통신은 실행하지 않았다.
}
TEST_CASE(FactoryGlobalsCurrent_ActualFrameHelperReadsSharedTypeAfterCommon) { // 일반 Factory의 공통 콜백이 바꾼 현재 제외 DWORD를 실제 프레임 helper가 같은 호출에서 읽는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto gameplay=std::make_shared<ClientGameplayGlobals>();auto services=external.Make();services.gameplayGlobals=gameplay;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);
    const auto write=[&](std::size_t offset,std::uint32_t value) { for (std::size_t index=0;index<4;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8)); }; // 일반 Factory VT이며 아웃포스트 wrapper를 선택하지 않는다.
    write(0,0x509148);write(36,7);raw[11]=raw[40]=0;raw[34]=1;auto storage=session.World().Storage();storage->types[74].flags1=storage->types[74].flags2=0;storage->frames[74]=RiftTypeFrames(std::vector<FrameCode>{{'A','A',0,0}});std::uint32_t notices{};
    core.postPop.BindPatchAiNotice([&](auto,Sid sid,bool) { CHECK(sid==source);++notices;gameplay->OutpostType()=74;write(0,0); });++core.bookkeeping.depth;core.postPop.PostPopVirtual(source,0x2000001);CHECK(notices==1 && core.bookkeeping.depth==0 && raw[36]==7 && gameplay->OutpostType()==74);
    gameplay->OutpostType()=126;write(0,0x509148);write(36,0);++core.bookkeeping.depth;core.postPop.PostPopVirtual(source,0x2000000);CHECK(core.bookkeeping.depth==0 && raw[36]==0 && notices==1); // 현재 값을 돌린 뒤 일반 Count에 복귀하며 MP/Frame 틱은 실행하지 않는다.
}

TEST_CASE(AssetParentsCurrent_ActualSessionUsesSharedListsAndRealRegulars) { // 실제 Session의 두 부모가 같은 장기 목록/현재 비용과 실제 예약 Form을 사용한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());session.Load();auto core=session.Core();auto storage=session.World().Storage();std::array<int,2> point{};
    for (int y=2;y<254 && !point[0];++y) { // 기존 지형을 바꾸지 않고 실제 finder의 네 단계가 모두 빈 위치를 찾는다.
        for (int x=2;x<254;++x) { // 각 단계에서 방문하는 최대 네 버킷이 비었는지 확인한다.
            bool empty=true;
            for (int level=0;level<4 && empty;++level) { // 현재 일반 finder와 같은 단계별 좌표를 사용한다.
                const auto scale=SquidHash::kScales[level],last=SquidHash::Side(level)-1;const auto left=x/scale,top=y/scale;
                for (int by=top;by<=std::min(top+1,last) && empty;++by) { // 실제 Next가 보는 두 행만 읽는다.
                    for (int bx=left;bx<=std::min(left+1,last);++bx) if (core.hash.Cell(level,bx,by)) empty=false; // 현재 머리를 수정하지 않는다.
                }
            }
            if (empty) { point={x,y};break; }
        }
    }
    CHECK(point[0]!=0);if (!point[0]) throw std::logic_error("자산 부모 검사의 실제 빈 해시 위치 없음");
    auto players=session.PlayerRecords();std::array<std::array<std::uint8_t,4>,8> savedAi{};
    for (std::size_t owner=1;owner<9;++owner) { // 원문이 지원하는 null AI 입력으로 등록 순회만 실행하며 실제 AI/적 행동은 실행하지 않는다.
        const auto offset=owner*0xb4+0xb0;std::copy_n(players.begin()+offset,4,savedAi[owner-1].begin());std::fill_n(players.begin()+offset,4,std::uint8_t{});
    }
    core.postPop.BindPatchAiNotice([](auto,Sid,bool) {}); // 공통 부모의 외부 통지 경계이며 Geyser의 실제 null-AI 순회와 구별한다.
    for (const auto typeNumber:{74U,122U}) { // 명시 일반 Residence VT와 실제 간헐천 TYPE의 첫 예약만 만들고 Process/Frame 틱은 실행하지 않는다.
        const bool residence=typeNumber==74;auto& list=residence ? storage->sidLists.residences : storage->sidLists.geysers;const auto before=list.Count();const auto source=core.factory.Create(typeNumber);auto raw=core.pool.AllocatedBytes(source);
        const auto write=[&](std::size_t offset,std::uint32_t value) { for (std::size_t index=0;index<4;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8)); }; // 이 새 슬롯의 VT/좌표/프레임만 명시한다.
        write(0,residence ? 0x50f540U : 0x50da88U);write(14,std::bit_cast<std::uint32_t>(float(point[0])));write(18,std::bit_cast<std::uint32_t>(float(point[1])));write(36,5);raw[11]=raw[40]=0;raw[34]=1;storage->types[typeNumber].flags1=storage->types[typeNumber].flags2=0;storage->types[typeNumber].cost=123.75F;
        ++core.bookkeeping.depth;core.postPop.PostPopVirtual(source,0x2000001);CHECK(core.bookkeeping.depth==0 && list.Count()==before+1 && std::ranges::find(list.Items(),source.value)!=list.Items().end());
        if (residence) CHECK(core.processes.FindEvent(source,0) && core.processes.FindEvent(source,1) && core.processes.FindEvent(source,3) && raw[42]==5);
        else { auto* process=core.processes.FindEvent(source,0);CHECK(process && raw[12]==123 && raw[13]==0);if (process) CHECK(process->Payload()==0.1F && process->Count()==0); }
    }
    for (std::size_t owner=1;owner<9;++owner) std::copy(savedAi[owner-1].begin(),savedAi[owner-1].end(),players.begin()+owner*0xb4+0xb0); // 같은 실제 Player 포인터 DWORD 입력을 복구한다.
}

TEST_CASE(BuildingNotifyCurrent_ActualSessionReadsCurrentTypeAndService) { // 같은 Session은 현재 TYPE/checking과 실제 서비스의 진단 정상 복귀를 사용한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();auto gameplay=std::make_shared<ClientGameplayGlobals>();services.gameplayGlobals=gameplay;std::function<void(std::string_view)> current;std::uint32_t calls{};
    services.ai.assertion=[&](auto expression) { if (!current) throw std::logic_error("명시 Session 진단 입력 전 호출");current(expression); };auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);const auto saved=std::vector<std::uint8_t>(raw.begin(),raw.end());const auto savedBridge=core.bridgeState.bridgeType;auto& checking=gameplay->AiDebug().Checking();const auto savedChecking=checking;
    raw[12]=0x7f;raw[40]=0;core.bridgeState.bridgeType=74;checking=6;current=[&](auto expression) { CHECK(expression=="num >= 0 && num <= 7");++calls;raw[10]=75;raw[11]=9;raw[12]=0x40;core.bridgeState.bridgeType=75;checking=0; };core.building.NotifySurface(source);CHECK(calls==1 && raw[11]==9 && raw[12]==0x40 && checking==0);
    current=[&](auto expression) { CHECK(expression=="num >= 0 && num <= 7");++calls; };checking=std::bit_cast<std::int32_t>(0x80000000U);core.building.NotifySurface(source);CHECK(calls==2 && raw[11]==9 && core.bridgeState.bridgeType==75 && checking==std::bit_cast<std::int32_t>(0x80000000U));
    std::copy(saved.begin(),saved.end(),raw.begin());core.bridgeState.bridgeType=savedBridge;checking=savedChecking; // 명시 입력만 복구하며 Frame/공격/포획/희생/승리는 실행하지 않는다.
}

TEST_CASE(AssetGunCurrent_ActualSessionGraphDiagnosticReturnsToPhysicalSlot) { // 실제 Gun 각도 부모가 같은 World Graph의 TYPE 진단과 서비스 복귀를 사용한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();std::function<void(std::string_view)> current;std::uint32_t calls{}; // Load 이후 명시 장면에서만 진단 입력을 공급한다.
    services.ai.assertion=[&](auto expression) { if (!current) throw std::logic_error("명시 Gun Graph 진단 입력 전 호출");current(expression); };auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);auto storage=session.World().Storage();
    const auto write=[&](std::size_t offset,std::uint32_t value) { for (std::size_t index=0;index<4;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8)); }; // 새 일반 슬롯에만 각도 VT와 프레임을 명시한다.
    write(0,0x50b6e8);write(36,25);raw[11]=raw[40]=0;raw[30]=77;storage->types[74].flags1=storage->types[74].flags2=0;
    current=[&](auto expression) { CHECK(expression=="getTypeFlags() & TYPE_FLAG_IS_SURFACE || type == fencemarkType || type == windArcherType");++calls;raw[10]=75;raw[11]=9;raw[40]=1; };++core.bookkeeping.depth;core.postPop.PostPopVirtual(source,0x2000002);
    CHECK(calls==1 && raw[10]==75 && raw[11]==9 && raw[30]==1 && core.bookkeeping.depth==0); // Frame/Process 틱·생산·발사·적 공격·통신을 실행하지 않는다.
    raw[11]=raw[40]=0; // Session 반납 시 같은 새 슬롯의 명시 수명 입력을 복구한다.
}

TEST_CASE(AssetFenceCurrent_ActualSessionCreatesOnlySelectedRegulars) { // 같은 Session의 두 펜스 부모가 실제 예약 Form을 만들며 바리케이드 flags0은 앞단을 생략한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());session.Load();auto core=session.Core();auto storage=session.World().Storage();core.postPop.BindPatchAiNotice([](auto,Sid,bool) {}); // 공통 부모의 외부 통지 경계만 제공한다.
    for (std::size_t kind=0;kind<3;++kind) { // 일반 슬롯에 명시 VT를 넣고 실제 공격/Process·Frame 틱은 실행하지 않는다.
        constexpr std::array<std::uint32_t,3> tables{0x50e940,0x50e668,0x50e738}; // 현재 부모의 실제 원문 가상 표다.
        const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);const auto write=[&](std::size_t offset,std::uint32_t value) { for (std::size_t index=0;index<4;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8)); }; // 이 새 슬롯의 VT만 명시한다.
        write(0,tables[kind]);raw[11]=raw[40]=0;raw[34]=1;storage->types[74].flags1=storage->types[74].flags2=0;++core.bookkeeping.depth;core.postPop.PostPopVirtual(source,0x2000000U+(kind ? 1U : 0U));auto* process=core.processes.FindEvent(source,0);CHECK(core.bookkeeping.depth==0);
        if (kind) { CHECK(process);if (process) CHECK(process->Payload()==(kind==1 ? 1.0F : .3F) && process->Count()==0 && process->Time()==core.processState.Now()); }else CHECK(!process);
    }
}

TEST_CASE(AssetTailCurrent_ActualSessionSharesGrowthAndNuggetRegulars) { // 실제 세 하위 몸체를 같은 가상 진입/Host/현재 시계에 연결하고 틱은 실행하지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());session.Load();auto core=session.Core();auto storage=session.World().Storage();core.postPop.BindPatchAiNotice([](auto,Sid,bool) {}); // 공통 부모의 외부 통지 경계를 고정한다.
    for (std::size_t kind=0;kind<3;++kind) { // 일반 새 슬롯의 명시 VT와 전용 예약만 검사하며 성장/생산/행동은 실행하지 않는다.
        constexpr std::array<std::uint32_t,3> tables{0x50e888,0x509470,0x50dc30}; // 성장 아이스/썬 바리케이드/너겟의 실제 가상 표다.
        const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);const auto write=[&](std::size_t offset,std::uint32_t value) { for (std::size_t index=0;index<4;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8)); }; // 새 슬롯의 VT와 필수 입력만 명시한다.
        write(0,tables[kind]);raw[11]=raw[40]=0;raw[34]=1;storage->types[74].flags1=storage->types[74].flags2=0;storage->frames[74]=RiftTypeFrames(std::vector<FrameCode>(4,FrameCode{'A','A',0,0}));++core.bookkeeping.depth;core.postPop.PostPopVirtual(source,0x2000000U+(kind==1 ? 0U : 1U));CHECK(core.bookkeeping.depth==0);
        if (kind==0) { auto* process=core.processes.FindEvent(source,0x3a6);CHECK(process);if (process) CHECK(process->Payload()==10.0F && process->Count()==0 && process->Time()==core.processState.Now()); }
        else if (kind==1) CHECK(!core.processes.FindEvent(source,0) && !core.processes.FindEvent(source,2));
        else { auto* first=core.processes.FindEvent(source,1);auto* second=core.processes.FindEvent(source,3);CHECK(first && second);if (first && second) CHECK(first->Payload()>=0 && first->Payload()<=.3F && second->Payload()>=0 && second->Payload()<=.5F && first->Time()==core.processState.Now() && second->Time()==core.processState.Now()); }
    }
}
