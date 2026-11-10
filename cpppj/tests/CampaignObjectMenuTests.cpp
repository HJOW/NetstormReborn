// 실제 세션의 사제/Carrier/객체/Player 메뉴를 같은 원본 Gump 입력과 커서 취득에 연결한다.
#include "DudeSpawnSupport.h"
#include "client/RawCampaignSession.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/ClientRandomGlobals.h"
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
#include "client/RitualChoiceInput.h" // 현재 다이스 번호 getter만 읽으며 전송/의식 명령은 실행하지 않는다.
#include "client/ClientDiagnostics.h" //현재로그버퍼와 깊이를 관찰하여 직접진단/Form재귀를 구별한다.
#include "client/RawChatWindowContext.h" //제품 Player 하위메뉴가 조회하는 실제 빈 Chat 입력버퍼를 같은 트리에 생성한다.
#include "client/ScriptCommand.h" //Chat 공장이 빌리는 실제80칸 문자열 고리의 수명을 검사 안에 유지한다.
#include "client/MenuGump.h" // 실제 팝업 항목의 명령/활성 플래그를 읽기 전용으로 관측한다.
#include "client/RawGameMenus.h" // 현재 Session의 Game/View/Options를 같은 실제 공통 메뉴에 작성한다.
#include "client/GameMenuCommands.h" // 원본 명령 전역을 화면 장치 전역과 구별해 읽는다.
#include "o/RawSquidLevel.h" // 메뉴 상태 행의 레벨을 원문 BYTE setter로 입력한다.
#include "o/OriginalText.h"
#include "o/RawBombEffectTimer.h" // Init 없는 timer cdecl의 실제 필드를 조회한다.
#include "o/RawContentPreviewProcess.h"
#include "client/VFXEllipse.h" // 실제 표시 통합의 원문 대조된 타원 소비자를 사용한다.
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
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);ClientOptions options;options.gameDirectory=assets.root;options.noAudio=true;Client client(std::move(options));client.Configuration().Set("helpWord","Help");client.LocalStateGlobals()->HelpWord().Interpret(std::string("Help"));MenuServices external;auto services=external.Make();
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

TEST_CASE(RiftShapeAccess_ActualSessionRetainsInputFrameAndLoadedCost) { // 실제Session의상세/e1뒤SHP와 최초 비용캐시를 현재TYPE float와 구분한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());session.Load();auto core=session.Core();auto storage=session.World().Storage();
    auto& type=storage->types[74];type.name="session";type.flags1=0;type.flags2=1;type.footX=type.footY=1;auto& shape=storage->shapes[74];shape.loaded=true;shape.frameCount=1;shape.frames.resize(2);shape.frames[1].cellWidth=shape.frames[1].cellHeight=5;
    std::array<int,2> point{};for (int y=1;y<256 && !point[0];++y) { //기존실제점유의행을차례로확인한다.
        for (int x=1;x<256;++x) if (!core.spots[y*256+x]) { point={x,y};break; } //점유를지우지않고현재행의비어있는한칸을찾는다.
    }
    CHECK(point[0]!=0);const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);raw[36]=1;std::optional<std::string> detail;std::uint32_t diagnoses{};const auto before=core.bookkeeping.TotalCost();CHECK(assets.game.TypeTable().Types()[74].cost==0);storage->BindChecking(RawCheckingFlag{0});
    storage->BindDisplayDiagnostics(detail,[&](auto expression,auto file,auto line) { CHECK(expression=="frameCheck( abstract, frame, false )" && file==R"(..\o\rifttype.h)" && line==0xe1);CHECK(detail=="Frame assertion failed. Asking for type=74 (session) frame=1 abs=0\n\nType has 1 frames\n" && core.spots[point[1]*256+point[0]]==1);++diagnoses;raw[36]=0;type.cost=21; });
    session.World().Pop(source,float(point[0]),float(point[1]),0x2000040);std::printf("SHP session observed: diagnoses=%u level=%u frame=%u bucket=%u sid=%u costDelta=%lld liveCost=%g loadedCost=%g depth=%u\n",diagnoses,unsigned(raw[33]),unsigned(raw[36]),unsigned(core.hash.Bucket(3,float(point[0]),float(point[1]))),unsigned(source.value),static_cast<long long>(core.bookkeeping.TotalCost())-before,double(type.cost),double(assets.game.TypeTable().Types()[74].cost),core.bookkeeping.depth);CHECK(diagnoses==1 && raw[33]==3 && raw[36]==0 && core.hash.Bucket(3,float(point[0]),float(point[1]))==source.value && core.bookkeeping.TotalCost()==before && type.cost==21 && core.bookkeeping.depth==0);
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

TEST_CASE(VortexCurrent_ActualSessionDiagnosticWritesCurrentPlayerRecord) { // 같은 실제 Session 서비스의 진단 복귀 뒤 변경된 owner의 Player BYTE에 새 신전 두 DWORD를 쓴다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();std::function<void(std::string_view)> current;std::uint32_t calls{}; // 명시 장면 이전에는 추가 진단 입력을 만들지 않는다.
    services.ai.assertion=[&](auto expression) { if (!current) throw std::logic_error("명시 Vortex 진단 입력 전 호출");current(expression); };auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);const auto savedRaw=std::vector<std::uint8_t>(raw.begin(),raw.end());auto records=session.PlayerRecords();const auto offset=8U*0xb4+0x84;std::array<std::uint8_t,8> savedFields;std::copy_n(records.begin()+offset,8,savedFields.begin());
    const auto write=[&](std::size_t at,std::uint32_t value) { for (std::size_t index=0;index<4;++index) raw[at+index]=static_cast<std::uint8_t>(value>>(index*8)); }; // 이 새 일반 슬롯의 원문 가상 표와 현재 owner0만 공급한다.
    const auto read=[&](std::size_t at) { std::uint32_t value{};for (std::size_t index=0;index<4;++index) value|=std::uint32_t(records[at+index])<<(index*8);return value; }; // 실제 Player 네 BYTE를 직접 읽는다.
    write(0,0x5196f0);raw[11]=raw[40]=raw[34]=0;current=[&](auto expression) { CHECK(expression=="getPlayerId() != INVALID_PLAYER_ID");++calls;raw[34]=8;raw[11]=9; };++core.bookkeeping.depth;core.postPop.PostPopVirtual(source,0x2000002);CHECK(calls==1 && raw[11]==9 && read(offset)==source.value && read(offset+4)==source.value && core.bookkeeping.depth==0);
    std::copy(savedRaw.begin(),savedRaw.end(),raw.begin());std::copy(savedFields.begin(),savedFields.end(),records.begin()+offset); // 실제 Slot/Player의 명시 입력을 복구하며 의식·공격·승리는 실행하지 않는다.
}

TEST_CASE(PostPrefixCurrent_ActualSessionBridgeNoFirstAndForcefieldRegulars) { // 같은 Session의 VT 분배로 다리 미도달 연결과 보호막의 실제 두 예약만 확인한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());session.Load();auto core=session.Core();auto storage=session.World().Storage();
    for (const bool forcefield:{false,true}) { // 새 일반 슬롯의 명시 VT만 바꾸며 Frame/Process 틱·보호막 유지/공격 검사는 하지 않는다.
        const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);const auto table=forcefield ? 0x507770U : 0x5034c8U;for (std::size_t index=0;index<4;++index) raw[index]=static_cast<std::uint8_t>(table>>(index*8)); // 이 슬롯의 +20 후처리 표만 명시한다.
        raw[11]=raw[40]=0;raw[34]=1;storage->types[74].flags1=storage->types[74].flags2=0;++core.bookkeeping.depth;core.postPop.PostPopVirtual(source,0x2000000U+(forcefield ? 1U : 0U));auto* first=core.processes.FindEvent(source,1);auto* second=core.processes.FindEvent(source,2);CHECK(core.bookkeeping.depth==0);
        if (forcefield) { CHECK(first && second);if (first && second) CHECK(first->Payload()==.08F && second->Payload()==.01F && first->Count()==0 && second->Count()==0 && first->Time()==core.processState.Now() && second->Time()==core.processState.Now()); }else CHECK(!first && !second && raw[40]==0);
    }
}
TEST_CASE(BuriedRuneCurrent_ActualSessionVtablesAndRuneRegular) { // 실제 Session의 별도 TYPE 슬롯으로 buried·Rune 가상 진입과304 예약만 확인하며 틱/의식/전투를 실행하지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());session.Load();auto core=session.Core();auto storage=session.World().Storage();
    for (const bool rune:{false,true}) { // 명시한 두 VT의 접두만 같은 Host/공통 깊이에 연결한다.
        const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);const auto table=rune ? 0x5075d8U : 0x50a748U;for (std::size_t index=0;index<4;++index) raw[index]=static_cast<std::uint8_t>(table>>(index*8)); // 현재 TYPE74는 유지하고 실제 가상 후처리만 선택한다.
        raw[11]=raw[40]=0;raw[34]=1;storage->types[74].flags1=storage->types[74].flags2=0;++core.bookkeeping.depth;core.postPop.PostPopVirtual(source,0x2000000U+(rune ? 1U : 0U));const auto* regular=core.processes.FindEvent(source,0x304);CHECK(core.bookkeeping.depth==0);
        if (rune) { CHECK(regular);if (regular) CHECK(regular->Payload()==.01F && regular->Count()==0 && regular->Time()==core.processState.Now()); }else CHECK(!regular && raw[10]==74);
    }
}
TEST_CASE(PlayerParentCurrent_ActualSessionPhysicalLinkAndInitialRegular) { // 같은 실제9기록/공통 부모/Host를 사용하며 일반 슬롯의 두 가상 후처리만 실행한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();auto gameplay=std::make_shared<ClientGameplayGlobals>();services.gameplayGlobals=gameplay;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();auto storage=session.World().Storage();auto records=session.PlayerRecords();const auto previous=netstorm::test::rawscene::Get(records,0xb4+0x9c);gameplay->player.battleOwner=1;gameplay->player.fortMode=false;gameplay->player.special=1;
    for (const bool first:{false,true}) { // free/contained flags0와 정상 최초 예약을 분리해 Frame/Process 틱 없이 확인한다.
        const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);netstorm::test::rawscene::Put(raw,0,0x50ee08);raw[11]=first ? 0 : 1;raw[40]=first ? 0 : 8;raw[34]=1;storage->types[74].flags1=storage->types[74].flags2=0;++core.bookkeeping.depth;core.postPop.PostPopVirtual(source,0x2000000U+(first ? 1U : 0U));const auto* regular=core.processes.FindEvent(source,1);CHECK(netstorm::test::rawscene::Get(records,0xb4+0x9c)==source.value && core.bookkeeping.depth==0);
        if (first) { CHECK(regular);if (regular) CHECK(regular->Payload()==5 && regular->Count()==0 && regular->Time()==core.processState.Now());CHECK(netstorm::test::rawscene::Get(raw,36)==1); }else CHECK(!regular && raw[11]==1 && raw[40]==8);raw[11]=0;raw[40]=0;
    }
    netstorm::test::rawscene::Put(records,0xb4+0x9c,previous); // 명시 검사로 바꾼 기술 부모를 실제 Session 정리 전에 복구한다.
}
TEST_CASE(PlayerParentCurrent_ActualSessionDiagnosticReturnAndTypedCreate) { // 같은 Client 진단 정상 복귀 뒤 현재 TYPE/좌표로 실제 Factory/Owner/Pop까지 연결한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();auto gameplay=std::make_shared<ClientGameplayGlobals>();services.gameplayGlobals=gameplay;bool ready=false;std::vector<std::string> diagnoses;services.ai.assertion=[&](auto expression) { CHECK(ready);diagnoses.emplace_back(expression);if (expression=="playerIdKnown") { gameplay->player.playerKnown=1;gameplay->player.playerType=152;gameplay->player.special=1; } };auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();ready=true;gameplay->player.playerKnown=0;gameplay->player.playerType=188;const auto source=session.CreatePlayer(8);const auto raw=session.Core().pool.Slot(source);CHECK(!diagnoses.empty() && diagnoses.front()=="playerIdKnown" && raw[10]==152 && raw[34]==8 && std::bit_cast<float>(netstorm::test::rawscene::Get(raw,14))==18 && std::bit_cast<float>(netstorm::test::rawscene::Get(raw,18))==10 && netstorm::test::rawscene::Get(raw,36)==1 && netstorm::test::rawscene::Get(session.PlayerRecords(),8*0xb4+0x9c)==source.value && session.Core().bookkeeping.depth==0);
}
TEST_CASE(DudeParentCurrent_ActualSessionPhysicalVtableAndCurrentPopulationFrame) { // 같은 Session의 실제 population/QA와 I 프레임 장치를 연결하며 틱/행동/전투를 실행하지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());session.Load();auto core=session.Core();auto storage=session.World().Storage();auto& population=storage->sidLists.dudes;const auto previous=population.Count();const auto entries=std::vector<std::uint32_t>(population.Storage().begin(),population.Storage().end());
    const auto physical=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(physical);netstorm::test::rawscene::Put(raw,0,0x508cc8);netstorm::test::rawscene::Put(raw,12,0xbeef,2);raw[11]=1;raw[40]=8;storage->types[74].flags1=storage->types[74].flags2=0;++core.bookkeeping.depth;core.postPop.PostPopVirtual(physical,0x2000000);CHECK(population.Count()==previous && netstorm::test::rawscene::Get(raw,12,2)==0xbeef && core.bookkeeping.depth==0 && raw[11]==1);raw[11]=0;raw[40]=0;
    const auto dude=core.factory.Create(70);auto born=core.pool.AllocatedBytes(dude);born[34]=1;static_cast<void>(core.frame.StoreLevel(dude));++core.bookkeeping.depth;core.postPop.PostPopVirtual(dude,0x2000001);CHECK(netstorm::test::rawscene::Get(born,12,2)==0x5101 && netstorm::test::rawscene::Get(born,36)==64 && core.bookkeeping.depth==0);CHECK(previous<population.Capacity() && population.Count()==previous+1);if (previous<population.Storage().size()) CHECK(population.Storage()[previous]==dude.value);
    std::copy(entries.begin(),entries.end(),population.Storage().begin());population.SetCount(previous); // 명시 생성/등록으로 바꾼 현재 목록을 Session 정리 전에 복구한다.
}
namespace { // 현재 부모의 실제 Kernel 객체/Form만 읽고 프로세스를 진행하지 않는다.
netstorm::o::FieryTailVisualProcess* TeleportCurrentTail(RawWorldCore& core,Sid parent) { // 같은 Kernel의 실제 파생 객체와 부모를 확인한다.
    for (netstorm::o::ProcessId pid=1;pid<=core.kernel.Capacity();++pid) { auto* tail=dynamic_cast<netstorm::o::FieryTailVisualProcess*>(core.kernel.Get(pid));if (tail && tail->Parent()==parent) return tail; } // 등록 순서로 실제 파생 타입을 확인한 뒤 그 부모를 읽는다.
    return nullptr;
}
}
namespace { // 실제 Session의 기존 표면 머리를 지우지 않고 비어 있는 작은 영역을 찾는다.
std::array<int,2> IslandCurrentEmptyPoint(const RawWorldCore& core) { // 5×5 범위가 비었으면 연결/전투 없는 진단 입력을 그 가운데 둔다.
    for (int y=3;y<253;++y) { // 행 우선으로 기존 현재 지도를 읽는다.
        for (int x=3;x<253;++x) { bool empty=true; // 후보마다 주변 표면의 현재 머리를 확인한다.
            for (int dy=-2;dy<=2;++dy) for (int dx=-2;dx<=2;++dx) if (core.hash.Cell(0,x+dx,y+dy)) empty=false; // 주변 두 축의 기존 머리는 변경하지 않는다.
            if (empty) return {x,y};
        }
    }
    throw std::logic_error("표면 진단 입력의 빈 영역이 없다");
}
}
TEST_CASE(IslandCurrent_ActualSessionNonFirstPhysicalVtablesUseCommon) { // 같은 World의 두 실제 VT에서 비최초는 TYPE74/free/contained를 허용하고 자식을 만들지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());session.Load();auto core=session.Core();auto storage=session.World().Storage();storage->types[74].flags1=storage->types[74].flags2=0;
    for (const auto vtable:{0x506d98U,0x506f08U}) { // 두 현재 가상 몸체의 실제 상위 연결을 별도로 확인한다.
        const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);netstorm::test::rawscene::Put(raw,0,vtable);raw[11]=1;raw[40]=8;const auto count=core.pool.FreeCount();const auto processes=core.kernel.Size();++core.bookkeeping.depth;core.postPop.PostPopVirtual(source,0x2000000);CHECK(core.bookkeeping.depth==0 && raw[11]==1 && core.pool.FreeCount()==count && core.kernel.Size()==processes);raw[11]=0;raw[40]=0;
    }
}
TEST_CASE(IslandCurrent_ActualSessionMissingUpperLeftDiagnosisReturnsToCommon) { // 실제 Client 저장 게임 주소/진단과 World의 H 접두·공통 깊이 복귀를 틱 없이 연결한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();auto local=std::make_shared<ClientLocalStateGlobals>();services.localStateGlobals=local;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();auto storage=session.World().Storage();const auto point=IslandCurrentEmptyPoint(core);const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);storage->types[74].flags1=storage->types[74].flags2=0;storage->frames[74]=netstorm::o::RiftTypeFrames(std::vector<netstorm::o::FrameCode>{{'H','P',1,0}});netstorm::test::rawscene::Put(raw,0,0x506f08);netstorm::test::rawscene::Put(raw,36,0);netstorm::test::rawscene::Put(raw,14,std::bit_cast<std::uint32_t>(static_cast<float>(point[0])));netstorm::test::rawscene::Put(raw,18,std::bit_cast<std::uint32_t>(static_cast<float>(point[1])));raw[11]=raw[40]=0;raw[34]=0;local->SavedGame()=false;session.World().SetLoadingDepth(0);std::optional<std::string> detail;std::uint32_t diagnoses{};
    storage->BindDisplayDiagnostics(detail,[&](auto expression,auto file,auto line) { CHECK(expression=="gSid" && file==R"(\Ns\O\Construction.cpp)" && line==0x10a && core.bookkeeping.depth==1);++diagnoses; });++core.bookkeeping.depth;core.postPop.PostPopVirtual(source,0x2000001);CHECK(diagnoses==1 && !detail && core.bookkeeping.depth==0);
}
TEST_CASE(IslandCurrent_ActualWorldRecordsSupportDetailWithSharedTypeWas) { // 같은 실제 World의 소유자/표시 뒤 상세 Client 소유자와 현재 TYPE 문맥을 전달한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();auto gameplay=std::make_shared<ClientGameplayGlobals>();services.gameplayGlobals=gameplay;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();auto storage=session.World().Storage();const auto point=IslandCurrentEmptyPoint(core);const auto source=core.factory.Create(storage->links.noIslandType);auto raw=core.pool.AllocatedBytes(source);const auto before=std::vector<std::uint8_t>(raw.begin(),raw.end());raw[11]=raw[40]=0;netstorm::test::rawscene::Put(raw,14,std::bit_cast<std::uint32_t>(static_cast<float>(point[0])));netstorm::test::rawscene::Put(raw,18,std::bit_cast<std::uint32_t>(static_cast<float>(point[1])));netstorm::test::rawscene::Put(raw,36,0);core.hash.Cell(0,point[0],point[1])=source.value;gameplay->GameAux()->SupportTypeWas()=0xffffffffU;std::optional<std::string> detail;std::uint32_t diagnoses{};
    storage->BindDisplayDiagnostics(detail,[&](auto expression,auto file,auto line) { CHECK(expression=="0" && file==R"(\Ns\O\Damageable.cpp)" && line==0x18b);CHECK(detail && *detail=="--------X (No islandSid)  (No islandStalagSid)  TypeWas -1" && raw[34]==5);++diagnoses; });session.World().SetSupportOwner(static_cast<float>(point[0]),static_cast<float>(point[1]),5);CHECK(diagnoses==1 && storage->GameAux()==gameplay->GameAux());core.hash.Cell(0,point[0],point[1])=0;std::copy(before.begin(),before.end(),raw.begin()); // 명시 진단 입력의 임시 지도/물리만 Session 정리 전에 원래 void 상태로 돌린다.
}
TEST_CASE(TeleportCurrent_ActualSessionNoFirstSkipsInvalidEffectType) { // 다른 현재 객체 TYPE/free에서도 flags0은 잘못된 process TYPE 표를 읽지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();auto gameplay=std::make_shared<ClientGameplayGlobals>();services.gameplayGlobals=gameplay;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);netstorm::test::rawscene::Put(raw,0,0x5070c0);raw[11]=1;raw[40]=8;session.World().Storage()->types[74].flags1=session.World().Storage()->types[74].flags2=0;gameplay->GameAux()->StreakProcessType()=188;const auto before=core.kernel.Size();++core.bookkeeping.depth;core.postPop.PostPopVirtual(source,0x2000000);CHECK(core.bookkeeping.depth==0 && core.kernel.Size()==before && raw[11]==1);raw[11]=0;raw[40]=0;
}
TEST_CASE(TeleportCurrent_ActualSessionUsesCurrentConstructorAndItsOwnTypeGlobal) { // 현재 TYPE 항목의 함수 주소와 선택된 생성자 내부 TYPE를 독립해서 실제 Form에 부착한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();auto gameplay=std::make_shared<ClientGameplayGlobals>();services.gameplayGlobals=gameplay;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();auto storage=session.World().Storage();storage->types[57]=storage->types[12];storage->types[56]=storage->types[11];storage->types[58]=storage->types[11];gameplay->GameAux()->FieryTailProcessType()=56;
    for (const bool streak:{true,false}) { // Streak 함수를 고른57과 Fiery 함수를 고른58은 서로 다른 현재 전역으로 실제 TYPE를 정한다.
        gameplay->GameAux()->StreakProcessType()=streak ? 57U : 58U;const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);netstorm::test::rawscene::Put(raw,0,0x5070c0);raw[11]=raw[40]=0;raw[34]=1;storage->types[74].flags1=storage->types[74].flags2=0;const auto before=core.kernel.Size();++core.bookkeeping.depth;core.postPop.PostPopVirtual(source,0x2000001);auto* tail=TeleportCurrentTail(core,source);CHECK(tail && core.kernel.Size()==before+1 && core.bookkeeping.depth==0);if (tail) { const auto& fields=tail->Fields();CHECK(core.pool.Slot(tail->Form())[10]==(streak ? 57U : 56U) && fields.particles.size()==120 && fields.maximumLife==(streak ? 2 : 4) && fields.offset==(streak ? -8 : 0)); }
    }
}
TEST_CASE(TeleportCurrent_ActualSessionSelectedPhysicalOrUnknownConstructorFailsBeforeCommon) { // 선택된 표 범위/다른 코드 주소는 고정12 성공으로 대신하지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();auto gameplay=std::make_shared<ClientGameplayGlobals>();services.gameplayGlobals=gameplay;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();auto storage=session.World().Storage();storage->types[12].constructorAddress=0xdeadbeef;
    for (const auto type:{188U,12U}) { // 실제 역참조 밖 TYPE와 미복원 코드 주소 모두 생성 앞의 명시 경계다.
        gameplay->GameAux()->StreakProcessType()=type;const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);netstorm::test::rawscene::Put(raw,0,0x5070c0);raw[11]=raw[40]=0;const auto before=core.kernel.Size();++core.bookkeeping.depth;bool failed=false;try { core.postPop.PostPopVirtual(source,0x2000001); }catch (const std::exception&) { failed=true; }CHECK(failed && core.bookkeeping.depth==1 && core.kernel.Size()==before);core.bookkeeping.depth=0;
    }
}
TEST_CASE(TeleportCurrent_ActualSessionTailBindingKeepsOriginalOwner) { // 이미 연결한 현재 TYPE 주소는 재연결 실패 뒤에도 원래 장기 소유자를 읽는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();auto gameplay=std::make_shared<ClientGameplayGlobals>();services.gameplayGlobals=gameplay;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();session.Particles().BindTailProcessTypes(*gameplay->GameAux());netstorm::o::RawGameAuxState other;bool rejected=false;try { session.Particles().BindTailProcessTypes(other); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected);auto core=session.Core();session.World().Storage()->types[57]=session.World().Storage()->types[12];gameplay->GameAux()->StreakProcessType()=57;const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);netstorm::test::rawscene::Put(raw,0,0x5070c0);raw[11]=raw[40]=0;session.World().Storage()->types[74].flags1=session.World().Storage()->types[74].flags2=0;++core.bookkeeping.depth;core.postPop.PostPopVirtual(source,0x2000001);auto* tail=TeleportCurrentTail(core,source);CHECK(tail && core.bookkeeping.depth==0);if (tail) CHECK(core.pool.Slot(tail->Form())[10]==57);
}
TEST_CASE(NuggetMenuCurrent_ActualSessionSchedulesSuccessorWithActualVtable) { // 단위872: 표시 TYPE70/너겟 VT의 끝점에서 같은 Host에 후속 Regular를 생성한다. 이동/낙하/공격/의식은 실행하지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());session.Load();auto core=session.Core();const auto source=core.pool.Allocate(2);auto raw=core.pool.AllocatedBytes(source);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x50dc30);raw[10]=70;raw[11]=0;const auto baseline=core.kernel.Size();auto* first=core.processes.AddRegular(source,0,.25F);CHECK(first);if (!first) { core.pool.Release(source);return; }const auto form=first->Form();first->SetInitialCount(1);first->RunFrame();
    auto* next=core.processes.FindEvent(source,1);CHECK(next && !core.processes.FromForm(form) && core.kernel.Size()==baseline+1);if (next) { CHECK(next->Parent()==source && next->Count()==0 && next->Payload()>=2 && next->Payload()<8);core.processes.Kill(*next,0x10); }
    if (auto* remaining=core.processes.FromForm(form)) core.processes.Kill(*remaining,0x10);CHECK(core.kernel.Size()==baseline && Get(core.pool.Slot(source),6,2)==0);raw=core.pool.AllocatedBytes(source);raw[11]=4;core.pool.Release(source); // 합성 슬롯과 새 예약을 반납해 Session 정리에 섞지 않는다.
}
TEST_CASE(NuggetTypeNumber_ActualSessionChoiceGetterReadsCurrentNuggetOwner) { // 단위871: 선택 입력 getter가 같은 장기 owner의 현재 너겟 번호를 읽는다. 명령/패킷/의식은 호출하지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();auto gameplay=std::make_shared<ClientGameplayGlobals>();services.gameplayGlobals=gameplay;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto hooks=session.RitualChoiceInputs();auto& current=gameplay->GameAux()->NuggetType();const auto saved=current;CHECK(hooks.nuggetType()==saved);
    current=70;CHECK(hooks.nuggetType()==70);current=71;CHECK(hooks.nuggetType()==71);current=saved; // Session 정리 전에 원래 번호를 복원하고 실제 요청은 보내지 않는다.
}
TEST_CASE(RuneTypeNumber_ActualSessionChoiceGetterReadsCurrentDaisOwner) { // 단위870: 실제 설치 후 만든 입력 getter가 같은 현재 번호를 읽는지만 확인하고 명령/패킷/의식은 호출하지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();auto gameplay=std::make_shared<ClientGameplayGlobals>();services.gameplayGlobals=gameplay;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto hooks=session.RitualChoiceInputs();auto& current=gameplay->GameAux()->DaisType();const auto saved=current;
    CHECK(hooks.daisType()==saved);current=70;CHECK(hooks.daisType()==70);current=71;CHECK(hooks.daisType()==71);current=saved; // Session 해제 전에 합성 번호 입력을 복원한다.
}
TEST_CASE(RewardTypeNumber_ActualSessionsBorrowSameCurrentOwnerWithoutLoad) { // 단위869: 실제 Session 보상 연결의 다섯 현재 번호와 재대여를 조회만으로 확인한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto gameplay=std::make_shared<ClientGameplayGlobals>();auto storage=MakeRawSurfaceStorage(assets.game,gameplay->Authority(),gameplay->GameAux());auto& owner=*gameplay->GameAux();owner.AltarType()=70;owner.DaisType()=71;owner.NuggetType()=128;owner.RainBlockerType()=128;
    for (const auto iteration:{0,1}) { // 같은 Client/World owner를 두 Session이 순차 대여하며 Load/Kernel/행동은 실행하지 않는다.
        static_cast<void>(iteration);auto services=external.Make();services.gameplayGlobals=gameplay;services.surfaceStorage=storage;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));auto core=session.Core();const auto source=core.pool.Allocate(2);auto raw=core.pool.AllocatedBytes(source);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=128;const auto flags1=storage->types[128].flags1,flags2=storage->types[128].flags2;storage->types[128].flags1=storage->types[128].flags2=0; // HP 비례 없이 명시 환불 비용만 조회한다.
        const auto& hp=session.HitPoints();CHECK(hp.SalvageRefund(source,70,0)==0 && hp.SalvageRefund(source,71,0)==0 && hp.SalvageRefund(source,128,0)==400);CHECK(!hp.Plan(source,1,false).applied); // rain 제외는 가상Count/미등록 VT 조회보다 앞이다.
        owner.AltarType()=128;CHECK(hp.SalvageRefund(source,128,0)==0);owner.AltarType()=70;owner.DaisType()=128;CHECK(hp.SalvageRefund(source,128,0)==0);owner.DaisType()=71;
        owner.GrowingRainBlockerType()=128;CHECK(hp.SalvageRefund(source,128,1)==0);owner.GrowingRainBlockerType()=150;owner.NuggetType()=154;CHECK(hp.SalvageRefund(source,128,0)==100);owner.NuggetType()=128;
        storage->types[128].flags1=flags1;storage->types[128].flags2=flags2;raw[11]=4;core.pool.Release(source); // 합성 슬롯/현재 TYPE 입력을 반납하고 번호 owner만 다음 Session에 유지한다.
    }
}
TEST_CASE(LoadedCostTable_ActualSessionsKeepOriginalAssetCostsAcrossMutableWorldReuse) { // 단위865: 같은 장기 TYPE를 바꿔 재대여해도 최초 자산의 썬 캐논 비용400을 사용한다. 로드/틱/행동은 실행하지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);CHECK(assets.game.TypeTable().Types()[128].cost==400.0F);MenuServices external;auto gameplay=std::make_shared<ClientGameplayGlobals>();auto storage=MakeRawSurfaceStorage(assets.game,gameplay->Authority(),gameplay->GameAux());
    // 서로 다른 현재float를 둔 두 Session을 순서대로 설치한다. 장기 자산/TYPE owner를 복제하지 않는다.
    for (const auto cost:{9001.0F,9002.0F}) {
        storage->types[128].cost=cost;auto services=external.Make();services.gameplayGlobals=gameplay;services.surfaceStorage=storage;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));CHECK(session.HitPoints().TypeCostUnits(128)==400 && storage->types[128].cost==cost);
        auto core=session.Core();const auto source=core.pool.Allocate(2);auto raw=core.pool.AllocatedBytes(source);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x501dd0);raw[10]=128;const auto flags1=storage->types[128].flags1,flags2=storage->types[128].flags2;storage->types[128].flags1=storage->types[128].flags2=0; // 순수 공통 부모만 선택하는 명시 입력이다.
        const auto before=core.bookkeeping.TotalCost();const auto global=core.bookkeeping.GlobalCounts()[128],local=core.bookkeeping.LocalCounts()[128],secondary=core.bookkeeping.LocalSecondaryCounts()[128];++core.bookkeeping.depth;core.postPop.PostPopBase(source,0x2000001);CHECK(core.bookkeeping.TotalCost()==before+400 && core.bookkeeping.depth==0);
        core.bookkeeping.TotalCost()=before;core.bookkeeping.GlobalCounts()[128]=global;core.bookkeeping.LocalCounts()[128]=local;core.bookkeeping.LocalSecondaryCounts()[128]=secondary;storage->types[128].flags1=flags1;storage->types[128].flags2=flags2;raw[11]=4;core.pool.Release(source); // 명시 장부/슬롯 입력을 반납해 다음 Session에 섞지 않는다.
    }
}
TEST_CASE(ForcefieldRegularCurrent_ActualSessionUsesVtableAndCurrentClientDiagnostic) { // 단위864: 실제 VT 분배와 현재 Client checking/진단을 확인하며 틱/행동은 실행하지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();auto gameplay=std::make_shared<ClientGameplayGlobals>();services.gameplayGlobals=gameplay;std::uint32_t calls{};services.ai.assertion=[&](auto expression) { CHECK(expression=="0");++calls; };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();const auto shield=core.factory.Create(167);auto raw=core.pool.AllocatedBytes(shield);const auto saved=std::vector<std::uint8_t>(raw.begin(),raw.end()); // 미등록 보호막의 물리 입력만 잠시 바꾼다.
    raw[10]=74;raw[11]=9;auto handler=session.Priests().RegularHandler([](Sid,std::uint32_t,std::uint32_t,float) { CHECK(false);return 123.0F; });const auto checking=gameplay->AiDebug().Checking();gameplay->AiDebug().Checking()=0;
    CHECK(handler(shield,99,0,0)==0 && calls==0);gameplay->AiDebug().Checking()=std::bit_cast<std::int32_t>(0x80000000U);CHECK(handler(shield,99,0,0)==0 && calls==1);
    gameplay->AiDebug().Checking()=checking;std::copy(saved.begin(),saved.end(),raw.begin());
}
TEST_CASE(ForcefieldRegularCurrent_ActualSessionSearchesWithCurrentWorldTypes) { // 실제 살아 있는 사제 위치 검색의 유지 반환만 확인한다. 자기 삭제/프레임 틱은 실행하지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());session.Load();auto core=session.Core();const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(priest.value);const auto location=core.pool.Slot(priest);
    const auto shield=core.factory.Create(167);auto raw=core.pool.AllocatedBytes(shield);Put(raw,14,Get(location,14));Put(raw,18,Get(location,18));const auto before=netstorm::test::rawscene::Hex(raw);CHECK(session.Priests().RegularHandler()(shield,2,0xffffffffU,0)==std::bit_cast<float>(0x3c23d70aU));CHECK(netstorm::test::rawscene::Hex(raw)==before);
}
TEST_CASE(RewardCurrentTypes_ActualSessionSharesWorldTableForHpAndPriestGenus) { // 단위863: 실제 로드 후 같은 현재 TYPE 필드를 바꾸며 getter만 호출한다. 틱/행동/포획/희생은 실행하지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());session.Load();auto core=session.Core();
    const Sid priest{static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(priest.value);auto& type=session.World().Storage()->types[158];const auto saved=type;const auto raw=core.pool.AllocatedBytes(priest);const auto previousHp=Get(raw,26); // 검사 후 실제 표와 HP를 복구한다.
    Put(raw,26,150);type.maxHitPoints=400;type.flags2|=0x200000;CHECK(session.HitPoints().MaxHitPoints(priest)==400);CHECK(session.Priests().PriestState().GroundImmobile(priest));
    type.flags2&=~0x200000U;CHECK(!session.Priests().PriestState().GroundImmobile(priest));type.flags1&=~0x10U;CHECK(!session.HitPoints().HasHitPoints(priest));
    type=saved;Put(raw,26,previousHp);CHECK(session.HitPoints().MaxHitPoints(priest)==saved.maxHitPoints);
}
TEST_CASE(PriestPostPopCurrent_ActualSessionVirtualPrefixListRegularAndMobileQuery) { // 단위862: 같은 Session의 실제 사제 VT 분배로 장기 목록·실제 회복 Regular·소유자 칸·이동 가능 보호막 조회를 연결하며 틱/행동/포획/희생은 실행하지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());session.Load();auto core=session.Core();auto storage=session.World().Storage();
    auto& priests=storage->sidLists.priests;const auto previous=priests.Count();const auto entries=std::vector<std::uint32_t>(priests.Storage().begin(),priests.Storage().end());auto& owners=session.Priests().Priests().OwnerState();const auto ownerBefore=owners;
    std::size_t ground=0;while (ground<core.spots.size() && !(core.spots[ground]&6U)) ++ground; // 실제 지도의 첫 섬/다리 지면 칸에 둔다.
    CHECK(ground<core.spots.size());
    const auto priest=core.factory.Create(158);auto raw=core.pool.AllocatedBytes(priest);raw[34]=1;raw[40]=0;netstorm::test::rawscene::Put(raw,12,1,2);
    netstorm::test::rawscene::Put(raw,14,std::bit_cast<std::uint32_t>(static_cast<float>(ground%256)));netstorm::test::rawscene::Put(raw,18,std::bit_cast<std::uint32_t>(static_cast<float>(ground/256)));
    const auto maximum=session.HitPoints().MaxHitPoints(priest);netstorm::test::rawscene::Put(raw,26,static_cast<std::uint32_t>(maximum));owners[1]=7; // 최대 HP의 이동 가능 사제이며 소유자 칸 쓰기를 관찰한다.
    ++core.bookkeeping.depth;core.postPop.PostPopVirtual(priest,0x2000001);
    auto* regen=core.processes.FindEvent(priest,0x25a);const auto type=session.HitPoints().TypeHitPoints(158); // 현재 장기 사제 TYPE DWORD의 초기값158이다.
    const auto expected=static_cast<float>(static_cast<double>(static_cast<float>(static_cast<double>(type/6)*50.0))/static_cast<double>(maximum));
    CHECK(regen!=nullptr && core.bookkeeping.depth==0 && owners[1]==0 && session.Priests().Forcefield().Find(priest)==netstorm::o::Sid{});
    if (regen) CHECK(regen->Payload()==expected && regen->Parent()==priest);
    CHECK(previous<priests.Capacity() && priests.Count()==previous+1);if (previous<priests.Storage().size()) CHECK(priests.Storage()[previous]==priest.value);
    std::copy(entries.begin(),entries.end(),priests.Storage().begin());priests.SetCount(previous);owners=ownerBefore; // 명시 등록으로 바꾼 장기 목록/소유자 칸을 Session 정리 전에 복구한다.
}

TEST_CASE(DaisClassCurrent_ActualSessionDispatchesReadOnlyRegularsByVtable) { // 단위873: 실제 Session의 세 가상 진입에서 무효과 반환만 조회한다. 의식/예약/틱/삭제는 호출하지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());session.Load();auto core=session.Core();const auto source=core.pool.Allocate(2);auto raw=core.pool.AllocatedBytes(source);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=70;raw[11]=4;const auto baseline=core.kernel.Size();auto handler=session.Priests().RegularHandler();
    Put(raw,0,0x5075d8);CHECK(!core.postPop.HandlesDais(source) && handler(source,99,0,.25F)==.25F);Put(raw,0,0x507950);CHECK(core.postPop.HandlesDais(source) && handler(source,99,0,.5F)==.5F);Put(raw,0,0x5076b8);CHECK(core.postPop.HandlesDais(source) && handler(source,23,0,.75F)==.75F);CHECK(core.kernel.Size()==baseline);core.pool.Release(source); // 합성 슬롯은 가상 삭제 효과 없이 직접 반납한다.
}

TEST_CASE(RuneSharedCurrent_ActualSessionPreflightUsesCurrentRuneOwnerWithoutEffects) { // 단위874: 실제 Session의 명시 사전 검사만 조회하고 Pop/의식/틱/명령은 호출하지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();auto gameplay=std::make_shared<ClientGameplayGlobals>();services.gameplayGlobals=gameplay;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();auto& current=gameplay->GameAux()->RuneType();const auto saved=current;const auto source=core.pool.Allocate(2);auto raw=core.pool.AllocatedBytes(source);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x5075d8);raw[10]=70;raw[11]=4;const auto before=std::vector<std::uint8_t>(raw.begin(),raw.end());const auto baseline=core.kernel.Size();current=70;
    bool accepted=true;try { core.postPop.Validate(source,0x2000000); }catch (const std::exception&) { accepted=false; }CHECK(accepted);current=71;bool rejected=false;try { core.postPop.Validate(source,0x2000000); }catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected && core.kernel.Size()==baseline && std::equal(before.begin(),before.end(),raw.begin()));current=saved;core.pool.Release(source); // 현재 번호와 합성 슬롯을 정리 전에 복원한다.
}

TEST_CASE(TeleportStubCurrent_SelectedNullConstructorsKeepExactDiagnosticAndCallableLifetime) { // 원문 두 NULL 생성자의 진단/자기 연결 반납만 검사하며 게임 프레임을 실행하지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());auto core=session.Core();
    for (const auto address:{0x00470b00U,0x00487ef0U}) { // 원문 TYPE13/22의 정확한 진단 위치와 확보 없는 복귀를 각각 확인한다.
        const auto before=core.kernel.Size();const auto free=core.pool.FreeCount();bool missing=false;try { session.Particles().CreateTailByConstructor(address,Sid{65535}); }catch (const std::logic_error&) { missing=true; }CHECK(missing && core.kernel.Size()==before && core.pool.FreeCount()==free);
        auto owner=std::make_shared<int>(7);std::weak_ptr<int> weak=owner;std::function<void(std::string_view,std::string_view,std::uint32_t)> diagnostic;bool called=false; // 현재 선택 callable의 소유자와 반환 여부만 관찰한다.
        diagnostic=[&,owner](auto expression,auto file,auto line) { diagnostic={};CHECK(*owner==7 && !weak.expired() && expression=="0" && file==(address==0x00470b00U ? ".\\Kaboom.cpp" : ".\\Particle.cpp") && line==(address==0x00470b00U ? 0xbcU : 0x1b0U));called=true; };owner.reset();session.Particles().CreateTailByConstructor(address,Sid{65535},diagnostic);CHECK(called && weak.expired() && core.kernel.Size()==before && core.pool.FreeCount()==free);
    }
}
TEST_CASE(TeleportStubCurrent_ActualSessionResumesCommonAfterNullAndCurrentTypeMutation) { // 실제 Client 진단이 복귀하면 선택 함수를 다시 읽지 않고 변경된 현재 객체의 공통 부모로 진행한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();auto gameplay=std::make_shared<ClientGameplayGlobals>();services.gameplayGlobals=gameplay;Sid source{};RawCampaignSession* current{};std::uint32_t diagnoses=0; // 검사에서 선택한 부모와 진단 관측이며 의식/행동을 호출하지 않는다.
    services.ai.assertion=[&](auto expression) { CHECK(current && source.value!=0 && expression=="0");++diagnoses;current->Core().pool.AllocatedBytes(source)[10]=75;gameplay->GameAux()->StreakProcessType()=188; };
    auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();current=&session;auto core=session.Core();auto storage=session.World().Storage();storage->types[74].flags1=storage->types[74].flags2=storage->types[75].flags1=storage->types[75].flags2=0;
    for (const auto type:{13U,22U}) { // 서로 다른 두 현재 항목의 실제 생성 주소를 읽고 공통 깊이/통계 복귀를 관찰한다.
        gameplay->GameAux()->StreakProcessType()=type;source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);netstorm::test::rawscene::Put(raw,0,0x5070c0);raw[11]=raw[40]=0;raw[34]=1;const auto before=core.kernel.Size();const auto free=core.pool.FreeCount(),count=core.bookkeeping.GlobalCounts()[75];++core.bookkeeping.depth;core.postPop.PostPopVirtual(source,0x2000001);CHECK(core.bookkeeping.depth==0 && core.kernel.Size()==before && core.pool.FreeCount()==free && raw[10]==75 && core.bookkeeping.GlobalCounts()[75]==count+1 && gameplay->GameAux()->StreakProcessType()==188);
    }
    CHECK(diagnoses==2);
}
TEST_CASE(TeleportStubCurrent_ActualSessionThrownDiagnosticKeepsPrefixMutationAndDepth) { // 선택 진단이 중단되면 그 앞 쓰기를 남기고 공통 부모와 새 프로세스 생성은 실행하지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();auto gameplay=std::make_shared<ClientGameplayGlobals>();services.gameplayGlobals=gameplay;RawCampaignSession* current{};Sid source{}; // 예외 직전의 실제 부모 바이트만 관찰한다.
    services.ai.assertion=[&](auto expression) { CHECK(current && expression=="0");current->Core().pool.AllocatedBytes(source)[34]=2;throw std::runtime_error("명시 진단 중단"); };auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();current=&session;auto core=session.Core();gameplay->GameAux()->StreakProcessType()=13;source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);netstorm::test::rawscene::Put(raw,0,0x5070c0);raw[11]=raw[40]=0;raw[34]=1;const auto before=core.kernel.Size();const auto free=core.pool.FreeCount();++core.bookkeeping.depth;bool stopped=false;try { core.postPop.PostPopVirtual(source,0x2000001); }catch (const std::runtime_error&) { stopped=true; }CHECK(stopped && raw[34]==2 && core.bookkeeping.depth==1 && core.kernel.Size()==before && core.pool.FreeCount()==free);core.bookkeeping.depth=0;
}

namespace { // 현재 Kernel의 실제10 객체만 읽는 생성 통합 검사 도우미다.
netstorm::o::EvilRayVisualProcess* CurrentEvilRay(RawWorldCore& core,Sid parent) { // 다른 입자나 기본 클래스의 필드를 잘못 읽지 않는다.
    for (netstorm::o::ProcessId pid=1;pid<=core.kernel.Capacity();++pid) { auto* effect=dynamic_cast<netstorm::o::EvilRayVisualProcess*>(core.kernel.Get(pid));if (effect && effect->Parent()==parent) return effect; } // 실제 등록 순서의 현재 파생 객체를 확인한다.
    return nullptr;
}
}
TEST_CASE(EvilRayCurrent_ActualTeleportSelectsConstructorAndItsCurrentTypeSource) { // 실제 VT/현재 함수 선택과 생성자 자체의 TYPE/source를 구분하며 게임/시각 진행은 하지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();auto gameplay=std::make_shared<ClientGameplayGlobals>();services.gameplayGlobals=gameplay;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();auto storage=session.World().Storage();storage->types[57]=storage->types[58]=storage->types[10];gameplay->GameAux()->StreakProcessType()=57;gameplay->GameAux()->EvilRayProcessType()=58;gameplay->GameAux()->ParticleSource()=0x12345678U;
    const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);netstorm::test::rawscene::Put(raw,0,0x5070c0);raw[11]=raw[40]=0;raw[34]=1;storage->types[74].flags1=storage->types[74].flags2=0;const auto before=core.kernel.Size(),rendered=session.Particles().Rendering().Size();++core.bookkeeping.depth;core.postPop.PostPopVirtual(source,0x2000001);auto* effect=CurrentEvilRay(core,source);CHECK(effect && core.bookkeeping.depth==0 && core.kernel.Size()==before+1 && session.Particles().Rendering().Size()==rendered+1);if (!effect) return;CHECK(core.pool.Slot(effect->Form())[10]==58 && effect->Fields().source==0x12345678U && effect->Fields().particles.size()==50 && effect->Fields().total==0 && !effect->Fields().first);
    gameplay->GameAux()->ParticleSource()=77;CHECK(effect->Fields().source==0x12345678U);storage->types[59]=storage->types[10];gameplay->GameAux()->EvilRayProcessType()=59;auto* next=session.Particles().AddEvilRay(source);CHECK(next && next->Fields().source==77 && core.pool.Slot(next->Form())[10]==59); // 다음 생성만 현재 두 전역을 다시 읽는다.
}
TEST_CASE(EvilRayCurrent_ActualVisualFrameNotifyPixelsAndSelfDeletionUseSameHost) { // 같은 실제 Host/형상/장치/표면을 연결하고 이 시각 효과만 호출한다. 게임/AI/전투 Kernel은 진행하지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();auto gameplay=std::make_shared<ClientGameplayGlobals>();services.gameplayGlobals=gameplay;std::vector<std::uint32_t> sounds;std::vector<std::string> diagnoses;bool ready=false; // 실제 음향 장치의 인자만 기록하며 물리 재생은 하지 않는다.
    services.sound.playOnceAt=[&](float,float,SoundHandle sound,std::uint32_t,std::uint32_t) { if (ready) sounds.push_back(sound);return sound; };services.ai.assertion=[&](auto expression) { if (ready) diagnoses.emplace_back(expression); };auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();ready=true;auto core=session.Core();auto storage=session.World().Storage();CHECK(storage->shapes[70].loaded && !storage->shapes[70].frames.empty());const auto source=core.factory.Create(70);auto raw=core.pool.AllocatedBytes(source);raw[11]=raw[40]=0;netstorm::test::rawscene::Put(raw,14,std::bit_cast<std::uint32_t>(20.0F));netstorm::test::rawscene::Put(raw,18,std::bit_cast<std::uint32_t>(21.0F));netstorm::test::rawscene::Put(raw,36,0);gameplay->GameAux()->ParticleSource()=source.value;const auto free=core.pool.FreeCount();auto* effect=session.Particles().AddEvilRay(source);CHECK(effect);if (!effect) return;const auto form=effect->Form();const auto pid=core.kernel.Find(*effect);const auto token=effect->Fields().registration.token;
    auto& width=storage->shapes[70].frames[0].width;const auto savedWidth=width;width=static_cast<std::int16_t>(width+2);session.Particles().PrepareFrame(1.0,1);effect->RunFrame();CHECK(effect->Fields().total==1 && effect->Fields().particles[0].x==static_cast<float>(320-width/2) && effect->Fields().particles[0].y==231 && effect->Fields().particles[0].life==13 && sounds.size()==1);width=savedWidth;const auto centerX=static_cast<int>(effect->Fields().particles[0].x),centerY=static_cast<int>(effect->Fields().particles[0].y); // 현재 SHP 변경은 이번 방출 위치에서 읽고 Native 원래 입력을 복원한다.
    std::vector<std::uint8_t> pixels(1024*768,199),expected=pixels;std::array<std::uint32_t,256> rgb{}; // 원문 대조된 팔레트/래스터 부품을 같은 출력에 합성하는 통합 기대값이다.
    for (std::size_t index=0;index<rgb.size();++index) { const auto color=assets.game.Palette().Color(static_cast<std::uint8_t>(index));rgb[index]=std::uint32_t(color.red)|(std::uint32_t(color.green)<<8)|(std::uint32_t(color.blue)<<16); } // Native 입력과 같은 RGBX다.
    const auto colors=netstorm::o::BuildEvilRayPalette(rgb);DrawVfxEllipse(expected,1024,768,{0,0,640,480},{centerX,centerY},-1,-2,colors[2]);DrawVfxEllipse(expected,1024,768,{0,0,640,480},{centerX,centerY},0,-1,colors[1]);DrawVfxEllipse(expected,1024,768,{0,0,640,480},{centerX,centerY},1,0,colors[0]);session.Particles().DrawEntry(token,pixels,1024,768,{0,0,1,1});CHECK(pixels==expected && std::count(pixels.begin(),pixels.end(),199)<static_cast<std::ptrdiff_t>(pixels.size())); // 원문 고정clip이 입력 cut을 대체하는 실제 경로다.
    effect->Notify(3,0x11223344,0x55667788);effect->Notify(3,7,8);CHECK(effect->Fields().stopping==1 && diagnoses.size()==1 && diagnoses[0]=="stopping == false");session.Particles().PrepareFrame(3.0,2);effect->RunFrame();CHECK(effect->Fields().particles[0].life<0 && sounds.size()==2);effect->RunFrame();effect=nullptr;CHECK(!core.kernel.Get(pid) && !session.Particles().Rendering().Lookup(token) && !core.processes.FromForm(form) && core.pool.FreeCount()==free); // 자기 삭제 뒤에는 저장한 번호/토큰만 조회한다.
}
TEST_CASE(EvilRayCurrent_ParentZeroRemainsRenderedWithoutKernelAndOwnerPersists) { // 원문 parent0의 부착 생략·표시 등록과 장기 DWORD 주소를 실제 구성에서 읽는다.
    netstorm::o::RawGameAuxState independent;CHECK(independent.EvilRayProcessType()==10 && independent.ParticleSource()==0);const auto* type=&independent.EvilRayProcessType();const auto* source=&independent.ParticleSource();independent.EvilRayProcessType()=57;independent.ParticleSource()=0xffffffffU;independent.ResetForGeneralWorld();CHECK(type==&independent.EvilRayProcessType() && source==&independent.ParticleSource() && *type==57 && *source==0xffffffffU);
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto gameplay=std::make_shared<ClientGameplayGlobals>();auto services=external.Make();services.gameplayGlobals=gameplay;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));auto core=session.Core();const auto before=core.kernel.Size(),rendered=session.Particles().Rendering().Size();gameplay->GameAux()->ParticleSource()=0xffffffffU;auto* effect=session.Particles().AddEvilRay({});CHECK(effect && core.kernel.Size()==before && session.Particles().Rendering().Size()==rendered+1);if (!effect) return;CHECK(effect->Form()==Sid{} && effect->Parent()==Sid{} && effect->Fields().source==0xffffffffU && effect->Fields().particles.size()==50);std::array<std::uint8_t,16*16> pixels;pixels.fill(199);session.Particles().DrawEntry(effect->Fields().registration.token,pixels,16,16,{0,0,16,16});CHECK(std::all_of(pixels.begin(),pixels.end(),[](auto value) { return value==199; })); // 사용 꼬리0은 clip만 요청하며 잘못된 source/미선택 metadata를 읽지 않는다.
}

TEST_CASE(FormConstructorCurrent_ActualTeleportUsesSelectedSlotConstructorThenCommon) { // 실제 caller의 SID를 첫 인자로 전달하며 새Form/타입 변경 없이 원문 공통 장부로 복귀한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();auto gameplay=std::make_shared<ClientGameplayGlobals>();services.gameplayGlobals=gameplay;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();auto storage=session.World().Storage();storage->types[74].flags1=storage->types[74].flags2=0;const std::array<std::array<std::uint32_t,2>,4> constructors{{{1,0x512110U},{2,0x512098U},{3,0x512188U},{6,0x512208U}}}; // 실제 원문 표의 네 기본 TYPE/VT다.
    for (const auto& constructor:constructors) { // 같은 실제 Teleport 가상 몸체에서 각 현재 생성 함수를 선택한다.
        gameplay->GameAux()->StreakProcessType()=constructor[0];const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);const auto saved=netstorm::test::rawscene::Get(raw,0);netstorm::test::rawscene::Put(raw,0,0x5070c0);raw[11]=raw[40]=0;raw[34]=1;const auto before=core.kernel.Size(),rendered=session.Particles().Rendering().Size();const auto free=core.pool.FreeCount(),count=core.bookkeeping.GlobalCounts()[74];++core.bookkeeping.depth;core.postPop.PostPopVirtual(source,0x2000001);CHECK(netstorm::test::rawscene::Get(raw,0)==constructor[1] && raw[10]==74 && core.bookkeeping.depth==0 && core.bookkeeping.GlobalCounts()[74]==count+1 && core.kernel.Size()==before && session.Particles().Rendering().Size()==rendered && core.pool.FreeCount()==free);netstorm::test::rawscene::Put(raw,0,saved); // 검사 종료 수명에서는 명시 입력의 원래VT로 복원하며 이후 Form 행동은 실행하지 않는다.
    }
}

TEST_CASE(PathConstructorCurrent_ActualTeleportUsesAliasedTypeFunctionWithoutStartingPath) { // 현재 TYPE61의 함수를 바꿔8종을 선택하고 부착 없이 같은 공통 장부로 복귀한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();auto gameplay=std::make_shared<ClientGameplayGlobals>();services.gameplayGlobals=gameplay;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();auto storage=session.World().Storage();storage->types[74].flags1=storage->types[74].flags2=0;gameplay->GameAux()->StreakProcessType()=61;const std::array<std::uint32_t,8> constructors{0x48be40,0x48bf30,0x48bed0,0x48bea0,0x48bf90,0x48bf00,0x48be70,0x48bf60}; // 원문 주소 순서이며 TYPE61의 고정 종류를 가정하지 않는다.
    for (const auto address:constructors) { storage->types[61].constructorAddress=address;const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);netstorm::test::rawscene::Put(raw,0,0x5070c0);raw[11]=raw[40]=0;raw[34]=1;const auto before=core.kernel.Size(),rendered=session.Particles().Rendering().Size();const auto free=core.pool.FreeCount(),count=core.bookkeeping.GlobalCounts()[74];++core.bookkeeping.depth;core.postPop.PostPopVirtual(source,0x2000001);CHECK(netstorm::test::rawscene::Get(raw,0)==0x5070c0 && raw[10]==74 && core.bookkeeping.depth==0 && core.bookkeeping.GlobalCounts()[74]==count+1 && core.kernel.Size()==before && session.Particles().Rendering().Size()==rendered && core.pool.FreeCount()==free); } // 현재 SID는 cdecl 잔여 인자일 뿐이며 부모로 부착하지 않는다.
}

TEST_CASE(PathConstructorCurrent_ActualSessionCreatesFixedClassesWithoutInitOrAttach) { // 실제 주소별 생성의 전체 객체를 Native로 대조하고 부모/Kernel/공간을 시작하지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());CHECK(netstorm::test::rawscene::Throws([&] { static_cast<void>(session.MakeUnstartedPath(0x48be40)); }));session.Load();auto core=session.Core();const auto free=core.pool.FreeCount();const auto kernel=core.kernel.Size();std::size_t count=0;const auto fixture=netstorm::test::rawscene::LoadFixture((std::filesystem::path(__FILE__).parent_path()/"fixtures/path-constructor-x86.tsv").string().c_str()); // 전체 바이트는 제품 수식으로 다시 만들지 않는다.
    for (const auto& row:fixture.rows) { if (row[2]!="0" || row[3]!="0") continue;auto path=session.MakeUnstartedPath(static_cast<std::uint32_t>(std::stoul(row[1])));CHECK(path);if (!path) return;std::vector<std::uint8_t> expected; // 반환 객체는 Session보다 먼저 해제한다.
        for (const auto& run:netstorm::test::rawscene::Split(row[4],',')) { const auto pair=netstorm::test::rawscene::Split(run,':');expected.insert(expected.end(),std::stoul(pair[0]),static_cast<std::uint8_t>(std::stoul(pair[1]))); } // Native 전체 이미지의 각 반복BYTE를 푼다.
        CHECK(expected.size()==path->Runtime().Bytes().size());CHECK(&path->Pool()==&core.pool && path->Parent()==Sid{} && path->Form()==Sid{} && path->MovementType()==std::stoul(row[0]) && std::equal(path->Runtime().Bytes().begin(),path->Runtime().Bytes().end(),expected.begin()) && core.pool.FreeCount()==free && core.kernel.Size()==kernel);++count;
    } // 각 현재 함수 주소가 고정한 클래스의 원문 전체 이미지와 비교한다.
    CHECK(count==8 && !session.MakeUnstartedPath(0) && !session.MakeUnstartedPath(0x4536a0)); // 다른 생성자를 경로 기본 클래스로 대체하지 않는다.
}

TEST_CASE(BombTimerConstructorCurrent_ActualSessionCreatesUnattachedBeforeLoadWithoutClockDeadline) { // 현재 월드 장치를 빌리되 설치/부모/시각 초기화 없이 빈 생성만 수행한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",external.Make());auto core=session.Core();core.processState.Now()=100.25;const auto kernel=core.kernel.Size();const auto free=core.pool.FreeCount();auto timer=session.MakeUnstartedBombTimer();CHECK(timer && timer->Parent()==Sid{} && timer->Form()==Sid{} && timer->Fields().members.empty() && timer->Fields().members.capacity()>=100 && timer->Fields().capacity==100 && timer->Fields().endTime==0 && timer->Fields().flags==0 && core.kernel.Size()==kernel && core.pool.FreeCount()==free);core.processState.Now()=9999.5;CHECK(timer->Fields().endTime==0); // 타이머는 Session보다 먼저 소멸하며 Frame/효과/패킷은 실행하지 않는다.
}
TEST_CASE(BombTimerConstructorCurrent_ActualTeleportSelectsEmptyUnattachedConstructor) { // TYPE61의 함수 주소만 timer57로 바꾸며 현재 시각의 Init이나 Form 부착을 추가하지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();auto gameplay=std::make_shared<ClientGameplayGlobals>();services.gameplayGlobals=gameplay;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();auto storage=session.World().Storage();storage->types[61].constructorAddress=storage->types[57].constructorAddress;CHECK(storage->types[61].constructorAddress==0x420ad0U);gameplay->GameAux()->StreakProcessType()=61;storage->types[74].flags1=storage->types[74].flags2=0;const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);netstorm::test::rawscene::Put(raw,0,0x5070c0);raw[11]=raw[40]=0;raw[34]=1;const auto before=core.kernel.Size(),rendered=session.Particles().Rendering().Size();const auto free=core.pool.FreeCount(),count=core.bookkeeping.GlobalCounts()[74];++core.bookkeeping.depth;core.postPop.PostPopVirtual(source,0x2000001);CHECK(netstorm::test::rawscene::Get(raw,0)==0x5070c0 && raw[10]==74 && core.bookkeeping.depth==0 && core.bookkeeping.GlobalCounts()[74]==count+1 && core.kernel.Size()==before && session.Particles().Rendering().Size()==rendered && core.pool.FreeCount()==free); // 버린 결과는 Session이 소유하며 글로벌 Kernel을 진행하지 않는다.
}

TEST_CASE(CursorConstructorCurrent_ActualTeleportUsesFixedBodyAndItsCurrentAttachType) { // 같은 TYPE61 함수 주소를 마나/사정거리로 바꾸고 각 현재 TYPE의 실제 Form/8표시를 확인한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();auto gameplay=std::make_shared<ClientGameplayGlobals>();services.gameplayGlobals=gameplay;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();auto storage=session.World().Storage();storage->types[74].flags1=storage->types[74].flags2=0;storage->types[74].theme=0;storage->types[74].group=0;storage->types[74].footX=storage->types[74].footY=1;gameplay->GameAux()->StreakProcessType()=61;session.CursorManaSettings().processType=58;session.CursorRangeSettings().processType=57;const std::array<std::array<std::uint32_t,3>,2> constructors{{{0x4741d0U,58,103},{0x474900U,57,104}}};RawContainedLookup lookup(core.pool,[](Sid,std::uint32_t) { throw std::logic_error("UI 생성 검사에서 삭제 조회를 요청함"); }); // 삭제하지 않는 실제 종속 조회다.
    for (const auto& entry:constructors) { storage->types[61].constructorAddress=entry[0];const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);netstorm::test::rawscene::Put(raw,0,0x5070c0);raw[11]=raw[40]=0;raw[34]=1;const auto kernel=core.kernel.Size();const auto free=core.pool.FreeCount(),count=core.bookkeeping.GlobalCounts()[74];++core.bookkeeping.depth;core.postPop.PostPopVirtual(source,0x2000001);const auto form=lookup.Find(source,entry[1]);auto* process=core.processes.FromForm(form);CHECK(form.value && process && process->Parent()==source && process->Form()==form && core.pool.Slot(form)[10]==entry[1] && core.kernel.Size()==kernel+1 && core.pool.FreeCount()+9==free && core.bookkeeping.depth==0 && core.bookkeeping.GlobalCounts()[74]==count+1 && raw[10]==74 && netstorm::test::rawscene::Get(raw,0)==0x5070c0); // 실제8표시+Form1을 할당하며 caller의 선택 TYPE61과 부착 TYPE를 구분한다.
        std::span<const std::uint8_t> fields;std::size_t list=0; // 현재 실제 클래스의 유일한 필드와8SID 목록 시작이다.
        if (entry[0]==0x4741d0U) { auto* mana=dynamic_cast<CursorManaProcess*>(process);CHECK(mana);if (!mana) return;fields=mana->Fields().bytes;list=12;CHECK(netstorm::test::rawscene::Get(fields,0)==0x50c468U && netstorm::test::rawscene::Get(fields,0x84)==8); }
        else { auto* range=dynamic_cast<CursorRangeProcess*>(process);CHECK(range);if (!range) return;fields=range->Fields().bytes;list=20;CHECK(netstorm::test::rawscene::Get(fields,0)==0x50c498U && netstorm::test::rawscene::Get(fields,0x1a4)==8); }
        CHECK(netstorm::test::rawscene::Get(fields,4)==form.value && netstorm::test::rawscene::Get(fields,8)==source.value);
        for (std::size_t index=0;index<8;++index) { const auto sid=Sid{static_cast<std::uint16_t>(netstorm::test::rawscene::Get(fields,list+index*4))};CHECK(sid.value && core.pool.Slot(sid)[10]==entry[2] && (core.pool.Slot(sid)[40]&8U)); } // 표시 자식의 실제 현재 타입/숨김 비트만 조회하며 Frame/행동은 실행하지 않는다.
    }
}

TEST_CASE(AssetConstructorCurrent_ActualTeleportUsesFunctionAddressWithoutRetypingOrAllocation) { // 일반 두 클래스와 NULL을 같은 현재 TYPE61에서 선택하고 공통 장부로 복귀한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();auto gameplay=std::make_shared<ClientGameplayGlobals>();services.gameplayGlobals=gameplay;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();auto storage=session.World().Storage();storage->types[74].flags1=storage->types[74].flags2=0;gameplay->GameAux()->StreakProcessType()=61;
    for (const auto kind:{89U,103U,153U}) { storage->types[61].constructorAddress=TypeConstructorAddress(OriginalEdition::Patch1078,kind);CHECK(storage->types[61].constructorAddress!=0);const auto expected=kind==153 ? 0x5070c0U : core.factory.ConstructorVtable(storage->types[kind]);const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);const auto saved=netstorm::test::rawscene::Get(raw,0);netstorm::test::rawscene::Put(raw,0,0x5070c0);raw[11]=raw[40]=0;raw[34]=1;const auto kernel=core.kernel.Size(),rendered=session.Particles().Rendering().Size();const auto free=core.pool.FreeCount(),count=core.bookkeeping.GlobalCounts()[74];++core.bookkeeping.depth;core.postPop.PostPopVirtual(source,0x2000001);CHECK(netstorm::test::rawscene::Get(raw,0)==expected && raw[10]==74 && core.bookkeeping.depth==0 && core.bookkeeping.GlobalCounts()[74]==count+1 && core.kernel.Size()==kernel && session.Particles().Rendering().Size()==rendered && core.pool.FreeCount()==free);netstorm::test::rawscene::Put(raw,0,saved); } // 검증된 recipe의 실제VT 합성만 비교하고 잘못된TYPE/VT의 후속 행동은 실행하지 않는다.
}
TEST_CASE(AssetConstructorCurrent_ActualFactoryUsesClientDiagnosticBeforePhysicalDaisStore) { // 의식/희생 없이 실제 Client 진단 정상 복귀 뒤 한DWORD 저장만 확인한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();auto mutate=std::make_shared<std::function<void()>>();auto diagnoses=std::make_shared<std::uint32_t>(0);services.ai.assertion=[mutate,diagnoses](std::string_view expression) { CHECK(expression=="false");++*diagnoses;if (!*mutate) throw std::logic_error("Dais 생성 진단 입력 준비 전 호출");(*mutate)(); };auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));auto core=session.Core();const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);const auto saved=std::vector<std::uint8_t>(raw.begin(),raw.end());const auto kernel=core.kernel.Size();const auto free=core.pool.FreeCount();*mutate=[raw] { raw[10]=75;raw[34]=4; };CHECK(core.factory.ConstructByAddress(0x44a420U,source));CHECK(*diagnoses==1 && netstorm::test::rawscene::Get(raw,0)==0x507950U && raw[10]==75 && raw[34]==4 && core.kernel.Size()==kernel && core.pool.FreeCount()==free);std::copy(saved.begin(),saved.end(),raw.begin());*mutate=nullptr; // 공유Storage가보관한진단은heap상태만소유하고지역span은반납한다. 원문169의가상몸체/포획/희생/Frame/Pop은 호출하지 않고 명시 입력을 복원한다.
}

TEST_CASE(PopWorldBounds_ActualSessionUsesSameCurrentClientSizeAsCursorAndCamera) { // 같은 장기 WorldCells를 Pop/커서가 직접 읽으며 미션/AI/전투 Frame은 진행하지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();auto gameplay=std::make_shared<ClientGameplayGlobals>();services.gameplayGlobals=gameplay;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();auto storage=session.World().Storage();auto logs=std::make_shared<std::vector<std::string>>();storage->BindGraphTrace([logs](auto level,auto text) { CHECK(level==29);logs->emplace_back(text); });storage->types[74].flags1=storage->types[74].flags2=0; // 실제 Client 로그의 수명도 지역 참조를 남기지 않는다.
    for (const auto limit:{32,64}) { gameplay->WorldCells()=limit;CHECK(session.CursorManaSettings().mapLimit==limit);const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);raw[11]=4;raw[40]=0x80;raw[34]=1;const auto kernel=core.kernel.Size();const auto free=core.pool.FreeCount();CHECK(core.pop.PopCurrent(source,40,20,0x2000040U)==RawPopResult::Registered);const auto expected=limit==32 ? 10.0F : 40.0F;CHECK(std::bit_cast<float>(netstorm::test::rawscene::Get(raw,14))==expected && std::bit_cast<float>(netstorm::test::rawscene::Get(raw,18))==(limit==32 ? 10.0F : 20.0F) && core.bookkeeping.depth==0 && core.kernel.Size()==kernel && core.pool.FreeCount()==free); } // 실제 일반 Pop과 같은 장부/표시에서 현재 경계만 바꾼다.
    CHECK(logs->size()==1 && logs->front().find("Set to( 10,10)")!=std::string::npos);gameplay->WorldCells()=256; // 뒤 Session에도 같은 장기 주소를 보존한다.
}

TEST_CASE(BombPostPop_ActualSessionSharesRandomAndDeduplicatesRealRegular) { // 실제 Host 예약/현재 난수만 대조하며 시전/전투 Kernel은 진행하지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();auto random=std::make_shared<ClientRandomGlobals>();services.randomGlobals=random;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();auto storage=session.World().Storage();storage->types[74].flags1=storage->types[74].flags2=0; // 일반 자산의 TYPE는 고정하고 실제 가상 표만 선택한다.
    for (const auto table:{0x502c38U,0x502cf0U,0x502db0U}) { random->Game().SetState(55);const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);const auto original=netstorm::test::rawscene::Get(raw,0);netstorm::test::rawscene::Put(raw,0,table);raw[11]=0;raw[40]=0;raw[34]=1;const auto kernel=core.kernel.Size();++core.bookkeeping.depth;core.postPop.PostPopVirtual(source,0x2000001U);const auto* task=core.processes.FindEvent(source,1);CHECK(task && task->Event()==1 && task->Count()==0 && task->Time()==core.processState.Now() && core.bookkeeping.depth==0 && core.kernel.Size()==kernel+1);const auto firstSeed=random->Game().State();CHECK((table==0x502c38U && firstSeed==55) || (table!=0x502c38U && firstSeed==0x3700a8U));if (task) CHECK(task->Payload()>=0.01F && task->Payload()<0.1F);++core.bookkeeping.depth;core.postPop.PostPopVirtual(source,0x2000001U);CHECK(core.processes.FindEvent(source,1)==task && random->Game().State()==firstSeed && core.kernel.Size()==kernel+1 && core.bookkeeping.depth==0);netstorm::test::rawscene::Put(raw,0,original); } // 두 번째 최초 요청은 실제 종속 목록에서 기존event1을 찾아 추가 예약/난수를 생략한다.
}

TEST_CASE(RegularCurrent_ActualSessionNumbersAndHostSurviveWorldReuse) { // 두 Session이 같은 장기 TYPE owner/Host를 빌리는지만 확인하며 Load/Frame은 실행하지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto gameplay=std::make_shared<ClientGameplayGlobals>();auto aux=gameplay->GameAux();auto storage=MakeRawSurfaceStorage(assets.game,gameplay->Authority(),aux);aux->RegularProcessType()=47;aux->SharedRegularProcessType()=62;SquidProcessHost* firstHost=nullptr;
    for (int pass=0;pass<2;++pass) { auto services=external.Make();services.gameplayGlobals=gameplay;services.surfaceStorage=storage;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));auto core=session.Core();if (!firstHost) firstHost=&core.processes;CHECK(firstHost==&core.processes && &core.processState.RegularType()==&aux->RegularProcessType() && &core.processState.SharedRegularType()==&aux->SharedRegularProcessType() && &core.processState.ProcessTypeEnd()==&aux->ProcessTypeEnd());const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);raw[11]=0;const auto kernel=core.kernel.Size();auto* regular=core.processes.AddRegular(source,0x6101,0.05F);auto* shared=core.processes.AddSharedRegular(source,0x6102,0.1F);CHECK(regular && shared && core.pool.Slot(regular->Form())[10]==47 && core.pool.Slot(shared->Form())[10]==62 && core.processes.FindEvent(source,0x6101)==regular && core.processes.FindEvent(source,0x6102)==shared && core.kernel.Size()==kernel+2);aux->RegularProcessType()=303;CHECK(core.processes.FindEvent(source,0x6101)==nullptr);aux->RegularProcessType()=47;aux->ProcessTypeEnd()=62;CHECK(core.processes.FindEvent(source,0x6102)==nullptr);aux->ProcessTypeEnd()=63; } // 전체 DWORD/현재 상한 변경을 즉시 읽고 다음 대여에도 같은47/62를 보존한다.
    CHECK(aux->RegularProcessType()==47 && aux->SharedRegularProcessType()==62 && aux->ProcessTypeEnd()==63);
}
TEST_CASE(RegularCurrent_ActualPopUsesCurrentTypeAndKeepsOneEffectReservation) { // 실제 공간Pop→가상접두→Host에서 세 효과의 현재47을 확인하며 시전/전투 Kernel은 진행하지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();auto gameplay=std::make_shared<ClientGameplayGlobals>();services.gameplayGlobals=gameplay;auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));session.Load();auto core=session.Core();auto storage=session.World().Storage();storage->types[74].flags1=storage->types[74].flags2=0;gameplay->GameAux()->RegularProcessType()=47;
    for (const auto table:{0x502c38U,0x502cf0U,0x502db0U}) { const auto source=core.factory.Create(74);auto raw=core.pool.AllocatedBytes(source);const auto original=Get(raw,0);Put(raw,0,table);raw[11]=4;raw[40]=0;raw[34]=1;const auto kernel=core.kernel.Size();CHECK(core.pop.PopCurrent(source,40,20,0x2000040U)==RawPopResult::Registered);auto* task=core.processes.FindEvent(source,1);CHECK(task && core.pool.Slot(task->Form())[10]==47 && core.kernel.Size()==kernel+1 && core.bookkeeping.depth==0 && !(raw[11]&4U));++core.bookkeeping.depth;core.postPop.PostPopVirtual(source,0x2000001U);CHECK(core.processes.FindEvent(source,1)==task && core.kernel.Size()==kernel+1 && core.bookkeeping.depth==0);Put(raw,0,original); } // 두 번째 최초 요청은 실제 종속 목록에서 같은 현재 TYPE47을 찾아 추가 예약을 생략한다.
}
TEST_CASE(RegularCurrent_ActualSharedConstructorUsesClientDiagnosticThenCurrentType) { // 실제 Client 진단 정상복귀 뒤48을 읽고 Pop 깊이를 보정하지 않는지 확인한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);MenuServices external;auto services=external.Make();auto gameplay=std::make_shared<ClientGameplayGlobals>();services.gameplayGlobals=gameplay;auto aux=gameplay->GameAux();auto calls=std::make_shared<std::uint32_t>(0);auto storage=MakeRawSurfaceStorage(assets.game,gameplay->Authority(),aux);services.surfaceStorage=storage;storage->BindDisplayDiagnostics(gameplay->AiDebug().AssertionDetail(),[aux,calls](std::string_view expression,std::string_view file,std::uint32_t line) { CHECK(expression=="basePostPop == 0" && file==R"(\Ns\O\Regular.cpp)" && line==0x70);++*calls;aux->SharedRegularProcessType()=48; });auto fort=FortTemplate::Parse(assets.files.Read("d/thewarbegins.fort"),assets.game.TypeTable());RawCampaignSession session(assets.game,assets.files,std::move(fort),"thewarbegins",std::move(services));auto core=session.Core();const auto source=core.factory.Create(74);core.pool.AllocatedBytes(source)[11]=0;aux->SharedRegularProcessType()=62;core.bookkeeping.depth=1;const auto kernel=core.kernel.Size();auto* process=core.processes.AddSharedRegular(source,0x6103,0.05F);CHECK(*calls==1 && process && core.pool.Slot(process->Form())[10]==48 && core.bookkeeping.depth==1 && core.kernel.Size()==kernel+1);core.bookkeeping.depth=0; // 같은 Storage가 보관하는 진단은 장기 aux/heap 계수만 캡처한다.
}

namespace { // 실제 Client 메뉴와 원본 controlled 입력/같은 Session 수명만 조립한다.
ClientOptions SpecialUiOptions(const netstorm::test::dudespawn::DudeAssets& assets) { ClientOptions options;options.gameDirectory=assets.root;options.noAudio=true;return options; } // 게임 Run/창을 시작하지 않는 현재 자산 설정이다.
struct SpecialObjectUiScene { // 특수 대상 메뉴의 현재 모드/소유자/클릭을 같은 제품 경로에 연결한다.
    const netstorm::test::dudespawn::DudeAssets& assets; // 보호 TYPE/SHP/VFS의 같은 현재 입력이다.
    Client client; // 제품과 같은 장기 게임/Chat/설정 전역이다.
    MenuServices external; // 시계/음향 등의 명시 외부 장치이며 게임 판단은 공급하지 않는다.
    ScriptStringRing strings; // 실제 Chat 입력의 문자열 고리가 UI보다 오래 산다.
    std::unique_ptr<GameWorld> world; // 현재 Session을 UI보다 먼저 소유해 생성 실패 시에도 UI 반납 동안 살려 둔다.
    ActualMenuUi ui; // 실제 Gump/StyleText/입력 큐와 같은 Form 수명이다.
    std::unique_ptr<CampaignObjectMenuClient> menu; // 제품의 실제 VT+a8/+ac 분배와 정적 문자열이다.
    std::vector<Sid> traced; // 창 없는 검사에서 현재 대상 dump 장치가 받은 실제 SID만 관측한다.
    std::vector<std::string> factoryNotices; // 실제 Factory 입력이 요청한 안내 섹션만 관측하는 출력 장치다.
    std::vector<Sid> lostNotices; // 실제 삭제가 요청한 손실 음향의 대상 SID만 관측한다.
    std::uint32_t constructionRefreshes{}; // 건설/업그레이드가 외부 화면에 요청한 갱신 횟수이며 게임 상태를 대신 계산하지 않는다.
    explicit SpecialObjectUiScene(const netstorm::test::dudespawn::DudeAssets& source,std::string_view mission="thewarbegins"):assets(source),client(SpecialUiOptions(source)),ui(source) { // 기존 기본 맵 또는 명시한 보호 Fort를 읽기만 하며 게임/창은 실행하지 않는다.
        client.Configuration().Set("helpWord","About");client.LocalStateGlobals()->HelpWord().Interpret(std::string("About")); /* M7.1: Run 없이 만든 Client에는 InterpretOptions의 helpWord 해석이 없으므로 같은 규칙으로 현재 캐시를 설치한다. */ auto services=external.Make();services.cameraCommit=netstorm::test::camera::CommitScreenless;services.gameplayGlobals=client.GameplayGlobals();services.kencloudGlobals=client.KencloudGlobals();services.coreDataGlobals=client.CoreDataGlobals();services.subscriberGlobals=client.SubscriberGlobals();services.playerRecords=client.PlayerRecords();services.gumpNotice=ui.buttons.ObjectNotice();
        services.factoryTell=[this](std::string_view section) { factoryNotices.emplace_back(section); }; // 자금 판단은 제품에 맡기고 안내 창의 출력 경계만 기록한다.
        services.unitLost=[this](Sid sid) { lostNotices.push_back(sid); }; // 삭제 관문/손실 위치는 제품에서 계산하고 실제 소리 장치만 대체한다.
        services.construction.refresh=[this] { ++constructionRefreshes; };services.construction.wallSeconds=[] { return 100.0; }; // 창 없는 업그레이드 검사의 화면 요청과 벽시계 장치만 공급한다.
        services.production.effects.configInt=[this](std::string_view key) { std::int32_t value{};static_cast<void>(client.Configuration().ReadInt(key,value));return value; }; // 생산 재계산도 같은 Client의 실제 설정 reader와 선택적0 기본값을 사용한다.
        auto fort=FortTemplate::Parse(assets.files.Read("d/"+std::string(mission)+".fort"),assets.game.TypeTable());world=std::make_unique<GameWorld>(assets.game,assets.files,std::move(fort),netstorm::o::MissionPlayers{},std::string(mission),std::move(services),ScreenPoint{640,480});ui.currentSession=&world->Session();world->Session().Cursor().SetEffects(ui.CursorEffects());UserSelectionHooks selection;selection.paused=[this] { return client.Paused() ? 1U : 0U; };selection.dump=[this](Sid sid) { CHECK(world->Session().Core().pool.Slot(sid).size()==50);traced.push_back(sid); };UserHighlightHooks highlight;highlight.selectCursor=[this](std::uint32_t value) { ui.cursor=value; };CarrierInputHooks carrier;carrier.selectCursor=highlight.selectCursor;world->BindUserInput(std::move(selection),std::move(highlight),std::move(carrier));client.GameplayGlobals()->Camera().ApplyPixelsToRendererAndSnapshot(220*16-320,220*11-240);
        RawChatWindowContext chat;chat.globals=client.ChatWindowGlobals();chat.commands=client.DotCommandGlobals();chat.editor.now=[] { return 0.0; };chat.translate=[](auto text) { return std::string(text); };chat.text=[this](auto token) { return strings.Resolve(token); };chat.storeText=[this](auto text) { return strings.Store(text); };chat.recipient=[](auto)->bool { throw std::logic_error("특수 메뉴의 빈 Chat 예상 밖 접두 검사"); };
        chat.read=[this](auto address)->std::uint32_t { const auto core=world->Session().Core();switch (address) { case 0x594fc8:return core.ownerMode.challenge;case 0x594fbc:return core.ownerMode.battle;case 0x563e38:return ui.buttons.RootToken();case 0x5cae94:return 304;case 0x5cae98:return 229;default:throw std::logic_error("특수 메뉴의 예상 밖 Chat 현재 전역"); } };static_cast<void>(ui.menus->CreateChatWindow(std::move(chat)));menu=std::make_unique<CampaignObjectMenuClient>(client,ui.buttons,[](std::string_view) {});
    }
    ~SpecialObjectUiScene() { menu->Clear();ui.menus->CloseChatWindows();ui.menus->CleanupFrame();menu.reset();world.reset();ui.currentSession=nullptr; } // 메뉴/Chat을 먼저 반납한 뒤 같은 Session을 해제한다.
    Sid Create(std::uint32_t type,std::uint32_t owner) { auto& session=world->Session();const auto sid=session.World().Create(type,2);session.Core().owner.Set(sid,owner);session.World().Pop(sid,220,220,0x10);return sid; } // 적 공격/포획·프레임 진행 없이 UI 대상만 실제 생성/Pop한다.
    void Observe(Sid object) { const auto core=world->Session().Core();const auto raw=core.pool.Slot(object);std::printf("UI source=%u type=%u owner=%u selected=%u head=%u\n",object.value,raw[10],raw[34],client.GameplayGlobals()->selection.selected,Get(raw,6,2));auto next=Get(raw,6,2);for (std::uint32_t count=0;next && count<16;++count) { const Sid sid{static_cast<std::uint16_t>(next)};const auto item=core.pool.Slot(sid);std::printf(" UI link=%u type=%u state=%u next=%u pid=%u live=%u\n",next,item[10],item[11],Get(item,4,2),Get(item,18),core.kernel.Get(Get(item,18)) ? 1U : 0U);next=Get(item,4,2); } } // 검사 실패 위치의 실제 링크/PID만 관측하고 판단/상태를 바꾸지 않는다.
    void Open(Sid object) { Observe(object);CHECK(menu->Open(*world,object,{InputCode::kRightButton,304,229})==1);CHECK(menu->Active()); } // 제품의 실제 우클릭 가상 분배를 실행한다.
    InputEvent Click(std::string_view label) { const auto point=ui.menus->Point(label);CHECK(point.has_value());if (!point) throw std::logic_error("특수 메뉴 실제 클릭 항목 없음");ui.Click(*point);return ui.commands.Pop(false); } // callback 직접 호출 없이 controlled 적중/입력 큐를 통과한다.
};
// 실제 대상의 부착 Form에서 팝업 루트를 찾아 현재 명령/인자 항목만 복사한다.
std::optional<MenuGumpItem> BuildingUiItem(SpecialObjectUiScene& scene,Sid source,std::uint32_t command) {
    auto& session=scene.world->Session();const Sid form{static_cast<std::uint16_t>(Get(session.Core().pool.Slot(source),6,2))};
    CHECK(session.GumpForms().IsGumpForm(form));const auto token=scene.ui.buttons.RegisteredGump(session.GumpForms().Owner(form));const auto* base=scene.ui.menus->Node(token);CHECK(base!=nullptr);if (!base) return {};
    const auto* menu=static_cast<const MenuGumpNode*>(base);
    for (std::uint32_t index=0;index<menu->itemCount;++index) { // 원문 메뉴의 저장 순서대로 현재 항목을 조회하며 자식 메뉴는 열지 않는다.
        if (menu->items[index].command==command) return menu->items[index];
    }
    return {};
}
// 동명이 항목을 문자열 추측 없이 현재 두 DWORD로 실제 controlled 좌표에 연결한다.
std::string BuildingUiCommand(std::uint32_t command,std::uint32_t parameter) { char text[64]{};std::snprintf(text,sizeof(text),"menu-command:%x,%x",command,parameter);return text; }
// 팝업의 정상 지연 정리를 마친 뒤 대상의 같은 링크/슬롯 반납을 다음 행에서 재사용한다.
void CloseBuildingUi(SpecialObjectUiScene& scene) { scene.menu->Clear();scene.ui.menus->CleanupFrame();CHECK(!scene.menu->Active() && !scene.ui.menus->HasMenus()); }
// 현재 루트의 실제 적중 행에서 자식 등록ID를 따라가 숨긴 하위 메뉴의 항목도 읽기 전용으로 복사한다.
std::vector<MenuGumpItem> ProductionUiItems(SpecialObjectUiScene& scene,Sid source,std::string_view label) {
    const auto point=scene.ui.menus->Point(label);CHECK(point.has_value());if (!point) throw std::logic_error("실제 생산 하위 메뉴 항목 없음");auto& session=scene.world->Session();const Sid form{static_cast<std::uint16_t>(Get(session.Core().pool.Slot(source),6,2))};CHECK(session.GumpForms().IsGumpForm(form));const auto root=scene.ui.buttons.RegisteredGump(session.GumpForms().Owner(form));const auto* menu=static_cast<const MenuGumpNode*>(scene.ui.menus->Node(root));CHECK(menu!=nullptr);if (!menu) throw std::logic_error("실제 생산 메뉴 루트 없음");
    for (const auto& region:scene.ui.menus->Controls()) { // 글자 추측이나 별도 메뉴 모형 없이 같은 실제 사각형/저장 행을 연결한다.
        if (point->x>=region.rect.left && point->x<region.rect.right && point->y>=region.rect.top && point->y<region.rect.bottom) { const auto& item=menu->items.at(static_cast<std::size_t>(region.id));const auto child=scene.ui.buttons.RegisteredGump(item.childId);const auto* below=static_cast<const MenuGumpNode*>(scene.ui.menus->Node(child));CHECK(item.childId && below);if (!below) throw std::logic_error("생산 하위 메뉴의 실제 자식 없음");std::printf("PRODUCTION child=%s token=%u count=%u\n",std::string(label).c_str(),child,below->itemCount);for (std::uint32_t index=0;index<below->itemCount;++index) { const auto& row=below->items[index];std::printf(" PROD row=%u flags=%x command=%x parameter=%x\n",index,row.flags,row.command,row.parameter); }return {below->items.begin(),below->items.begin()+below->itemCount}; } // 현재 행의 진단은 메뉴를 바꾸지 않으며 실패 시 실제 DWORD를 보존한다.
    }
    throw std::logic_error("생산 하위 메뉴의 실제 적중 행 없음");
}
// 명령 플래그가 있는 원문 저장 행만 세며 구분선/자식/제목의 미사용 DWORD를 명령으로 오해하지 않는다.
std::size_t ProductionUiCount(const std::vector<MenuGumpItem>& items,std::uint32_t command,std::uint32_t parameter) { return static_cast<std::size_t>(std::count_if(items.begin(),items.end(),[=](const auto& item) { return (item.flags&1U) && item.command==command && item.parameter==parameter; })); }
// 하위 메뉴의 원문 확장 애니메이션만 충분히 진행한다. 게임 Kernel/공격/생산 시각은 진행하지 않는다.
void FinishProductionUiExpansion(SpecialObjectUiScene& scene) { for (int frame=0;frame<64;++frame) static_cast<void>(scene.ui.buttons.IdleGumps()); } // 실제 Client와 같은 Gump idle이며 노드의 크기/flags를 직접 보정하지 않는다.
struct StateUiAltarPair { Sid altar{},dais{}; }; // 같은 원래 좌표의 실제 제단/사제 없는 다이스 SID를 보관하며 소유권을 복제하지 않는다.
// 보호 Fort에서 실제 로드된 한 쌍을 찾는다. 포획/희생 상태를 만들거나 해당 행동을 진행하지 않는다.
StateUiAltarPair FindStateUiAltarPair(SpecialObjectUiScene& scene) {
    auto& session=scene.world->Session();auto core=session.Core();
    for (std::uint32_t number=5;number<core.pool.Capacity();++number) { // 실제 풀의 살아 있는 다이스만 읽고 사제 내용물이 있는 대상은 선택하지 않는다.
        const Sid dais{static_cast<std::uint16_t>(number)};const auto raw=core.pool.Slot(dais);if ((raw[11]&3U) || raw[10]!=165 || session.Priests().Carrier().FindContained(dais,0,158,0,false).value) continue;
        for (std::uint32_t other=5;other<core.pool.Capacity();++other) { // 좌표 DWORD가 같은 실제 제단을 연결하며 가상표/해시를 조작하지 않는다.
            const Sid altar{static_cast<std::uint16_t>(other)};const auto target=core.pool.Slot(altar);if (!(target[11]&3U) && target[10]==164 && Get(target,14)==Get(raw,14) && Get(target,18)==Get(raw,18)) return {altar,dais};
        }
    }
    throw std::logic_error("보호 Fort에 사제 없는 실제 제단/다이스 쌍 없음");
}
const std::array<std::uint32_t,8> kCarrierUiTypes{111,133,135,137,141,146,156,158}; // 보호 TYPE의genus30000 전체8종이며 공격 공중3종은 포함하지 않는다.
// 사제는 원래 미션의 실제 참여자를 사용하고 나머지 운반체는 같은 실제 시작 섬의 빈 점에 배치한다.
Sid CarrierUiActor(SpecialObjectUiScene& scene,std::uint32_t type) {
    auto& session=scene.world->Session();auto core=session.Core();Sid source{};
    if (type==158) { source={static_cast<std::uint16_t>(session.FindPriestParticipation(1))};CHECK(source.value); }
    else { source=session.World().Create(type,2);core.owner.Set(source,1);session.World().SetLoadingDepth(1);session.World().Pop(source,94,110,0x10);session.World().SetLoadingDepth(0); }
    if (!source.value) throw std::logic_error("운반체 UI의 실제 대상 없음");CHECK(core.pool.Slot(source)[10]==type && core.pool.Slot(source)[34]==1);Put(core.pool.AllocatedBytes(source),26,std::bit_cast<std::uint32_t>(session.HitPoints().MaxHitPoints(source)));return source; // 이동 가능한 현재 최대HP를 입력하며 공격/회복 몸체는 실행하지 않는다.
}
// 원문 화면 역투영을 사용하는 현재 마우스 폴링만 진행하고 최종 커서를 관측한다.
void PollCarrierUi(SpecialObjectUiScene& scene,ScreenPoint point,std::uint32_t expected) { scene.world->SetMousePoint(point);scene.world->UpdateMovementCursor();std::printf("CARRIER cursor=%u expected=%u mouse=%d,%d selected=%u mode=%u\n",scene.ui.cursor,expected,point.x,point.y,scene.world->Selected(),scene.world->InputMode());CHECK(scene.ui.cursor==expected); }
// 보관 좌표는 셀 우하단이므로 원문4971e0의+16/+11 뒤 같은 셀로 절삭될 내부 픽셀을 사용한다.
ScreenPoint CarrierUiCell(SpecialObjectUiScene& scene,float x,float y) { const auto corner=scene.world->ToScreen(x,y);return {corner.x-8,corner.y-6}; }
// 실제 카메라/현재 좌표 변환으로 검사 점을 화면 안에 놓으며 지형/Graph/커서 판단을 주입하지 않는다.
void PollCarrierUiWorld(SpecialObjectUiScene& scene,float x,float y,std::uint32_t expected) { scene.world->Session().CameraGlobals().ApplyPixelsToRendererAndSnapshot(std::max(0,static_cast<int>(x*16)-320),std::max(0,static_cast<int>(y*11)-240));const auto point=CarrierUiCell(scene,x,y);CHECK(point.x>=0 && point.x<640 && point.y>=0 && point.y<480);PollCarrierUi(scene,point,expected); }
// 장치 전역만 명시 공급하고 모드/Player/게임 조회는 실제 BindGameMenus에 맡긴다. 반환 토큰은 현재 공통 메뉴 루트다.
std::uint32_t OpenStateGameUi(SpecialObjectUiScene& scene,std::uint32_t kind,std::uint32_t displayFlags=0,std::uint32_t soundReady=0,std::uint32_t editing=0,GameMenuBarState* barState=nullptr,bool technical=false) {
    std::uint32_t root{};scene.ui.menus->OpenAttached([&](const auto& widgets,const auto& controls,const auto& menus) {
        GameMenusHooks hooks;hooks.global=[&](std::uint32_t address)->std::uint32_t { if (const auto command=GameMenuCommandValue(address)) return *command;switch (address) { case 0x531860:return 640;case 0x531864:return 480;case 0x5c78d4:return displayFlags;case 0x5c7910:return 1;case 0x5c7b14:return soundReady;case 0x531900:return 3;case 0x531904:return 4;case 0x55b4b0:return scene.client.Paused() ? 1U : 0U;case 0x59a8ac:return 0;default:throw std::logic_error("현재 메뉴 UI의 미명시 장치 전역"); } };
        hooks.config=[&](auto key) { return scene.client.Configuration().GetInt(key); };hooks.readInt=[&](auto key,std::int32_t& value) { static_cast<void>(scene.client.Configuration().ReadInt(key,value)); };hooks.translate=[](auto text) { return std::string(text); };hooks.demo=[&] { return scene.external.demo; };hooks.editingMission=[&] { return editing; };
        hooks.playerMenu=[&](auto subscriber) { return scene.menu->BuildPlayerMenu(*scene.world,subscriber,widgets,controls,menus); };BindGameMenusControls(hooks,widgets,controls,menus,[&](auto text) { return scene.strings.Store(text); });auto bound=scene.world->Session().BindGameMenus(std::move(hooks));const auto globals=bound.global;const auto translate=bound.translate;RawGameMenus body(std::move(bound));
        if (kind==3) {
            if (!barState) throw std::logic_error("현재 가로 메뉴의 장기 추적 상태 없음");GameMenuBarHooks bar;bar.prepare=[&] { scene.world->Session().PrepareGameMenus(); };bar.translate=translate;bar.about=[&] { return *scene.client.Configuration().Configuration().Get("helpWord"); };bar.technicalHelp=[&] { return technical ? 1U : 0U; };
            bar.read=[&](GameMenuBarValue value)->std::uint32_t { switch (value) { case GameMenuBarValue::Editor:return globals(0x5c85a4);case GameMenuBarValue::Battle:return globals(0x594fbc);case GameMenuBarValue::Mission:return globals(0x594fa4);case GameMenuBarValue::Cheat:return globals(0x5cab30);case GameMenuBarValue::Width:return 640;case GameMenuBarValue::Left:return 85;case GameMenuBarValue::Root:return scene.ui.buttons.RootToken(); }throw std::logic_error("현재 가로 메뉴의 장치 조회 종류"); };
            bar.child=[&](GameMenuBarChild child) { switch (child) { case GameMenuBarChild::Game:return body.Game();case GameMenuBarChild::Edit:return body.Edit();case GameMenuBarChild::View:return body.View();case GameMenuBarChild::Options:return body.Options();case GameMenuBarChild::Players:return body.Players();case GameMenuBarChild::Cheat:return scene.ui.menus->CreateCheatMenu(translate,[globals] { return globals(0x54287c); }); }throw std::logic_error("현재 가로 메뉴의 하위 종류"); };BindGameMenuBarControls(bar,widgets,controls,menus,[&](auto token) { return scene.ui.menus->Node(token); },[&](auto text) { return scene.strings.Store(text); });root=RawGameMenuBar(*barState,std::move(bar)).Open();
        }else { switch (kind) { case 0:root=body.Game();break;case 1:root=body.View();break;case 2:root=body.Options();break;default:throw std::logic_error("현재 메뉴 UI 종류 범위"); }widgets.Attach(root,100,20,scene.ui.buttons.RootToken(),1); }return root;
    });return root;
}
// 대상 객체 없는 공통 메뉴의 저장 행을 현재 명령으로 조회하며 구분선/자식 행은 제외한다.
std::optional<MenuGumpItem> StateGameUiItem(SpecialObjectUiScene& scene,std::uint32_t root,std::uint32_t command) {
    const auto* node=static_cast<const MenuGumpNode*>(scene.ui.menus->Node(root));if (!node) throw std::logic_error("현재 게임 UI 메뉴 루트 없음");
    for (std::uint32_t index=0;index<node->itemCount;++index) { const auto& item=node->items[index];if ((item.flags&1U) && item.command==command) return item; } // 살아 있는 원문 명령 행만 저장 순서로 방문한다.
    return {};
}
// 객체 메뉴가 소유하지 않은 가로/하위 팝업은 실제 공통 입력 부모에 닫기를 예약한 뒤 정리한다.
void CloseStateGameUi(SpecialObjectUiScene& scene) { scene.ui.menus->RequestClosePopups();scene.ui.menus->CleanupFrame();CHECK(!scene.ui.menus->HasMenus()); }
}
TEST_CASE(SpecialObjectUi_CrystalOwnersAndEditorUseActualMenuAndRelease) { // 스톰 크리스탈의 raw owner/일반·편집 메뉴와 About 실제 인자를 같은 경로로 확인한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SpecialObjectUiScene scene(assets);auto& session=scene.world->Session();auto core=session.Core();
    for (const auto editor:{0U,1U}) for (const auto owner:{0U,1U,2U,8U}) { // 명시 모드2×소유자4의 UI 입력이며 자연 게임 상태 도달을 주장하지 않는다.
        scene.client.GameplayGlobals()->world.editor=editor;CHECK(scene.client.GameplayGlobals()->player.special==0);const auto source=scene.Create(154,owner);const auto before=Hex(core.pool.Slot(source));const auto head=Get(core.pool.Slot(source),6,2),free=core.pool.FreeCount();scene.Open(source);
        CHECK(scene.ui.menus->Point("About").has_value());CHECK(scene.ui.menus->Point("Player").has_value()==(owner!=0));CHECK(scene.ui.menus->Point("Destroy").has_value()==(editor!=0));CHECK(!scene.ui.menus->Point("CHEAT Dig Up").has_value() && !scene.ui.menus->Point("Turn To Spell!").has_value());
        const auto form=Sid{static_cast<std::uint16_t>(Get(core.pool.Slot(source),6,2))};CHECK(session.GumpForms().IsGumpForm(form) && session.GumpForms().Parent(form)==source);const auto root=scene.ui.buttons.RegisteredGump(session.GumpForms().Owner(form));const auto* node=scene.ui.menus->Node(root);CHECK(node && node->object==source.value && node->form==form.value);
        const auto command=scene.Click("About");CHECK(command.code==0x02000017 && command.y==154);scene.menu->Clear();scene.ui.menus->CleanupFrame();CHECK(!scene.menu->Active() && !scene.ui.menus->HasMenus() && Get(core.pool.Slot(source),6,2)==head && core.pool.FreeCount()==free && Hex(core.pool.Slot(source))==before);CHECK(core.pool.Slot(form)[11]&1U);CHECK(scene.world->Session().World().Destroy(source,0x10));
    }
}
TEST_CASE(SpecialObjectUi_ObeliskSpellRoundTripFromActualMenuClick) { // 편집 메뉴의 실제 클릭→명령 큐→같은 가상6c 전환과 재개방/반납을 연결한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SpecialObjectUiScene scene(assets);scene.client.GameplayGlobals()->world.editor=1;CHECK(scene.client.GameplayGlobals()->player.special==0);auto& session=scene.world->Session();auto core=session.Core();
    for (const auto owner:{0U,1U,2U,8U}) { // 원문 Devastation114/비석112의 네 raw 소유자를 공격/프레임 진행 없이 입력한다.
        const auto source=scene.Create(114,owner);const auto original=core.pool.Slot(source);const auto x=Get(original,14),y=Get(original,18),hp=Get(original,26);
        for (const auto target:{112U,114U}) { // 같은 SID에서 두 UI 명령을 연속 실행하고 현재 메뉴를 다시 작성한다.
            const auto traceCount=scene.traced.size();scene.Open(source);CHECK(scene.traced.size()==traceCount+(target==112 ? 1U : 0U));if (target==112 && !scene.traced.empty()) CHECK(scene.traced.back()==source);const auto label=target==112 ? "Turn To Obelisk!" : "Turn To Spell!";CHECK(!scene.ui.menus->Point("CHEAT Dig Up").has_value());const auto command=scene.Click(label);CHECK(command.code==0x030000ce && std::bit_cast<std::uint32_t>(command.y)==source.value);scene.menu->Clear();scene.ui.menus->CleanupFrame();const auto beforeCommand=core.pool.FreeCount();CHECK(session.CommandObject(source,command.code)==1);const auto raw=core.pool.Slot(source);CHECK(raw[10]==target && raw[34]==owner && Get(raw,14)==x && Get(raw,18)==y && Get(raw,26)==hp && core.pool.FreeCount()==beforeCommand);if (target==112) CHECK(Get(raw,12,2)==114 && Get(raw,36)==0);
        }
        scene.client.GameplayGlobals()->world.editor=0;scene.Open(source);CHECK(!scene.ui.menus->Point("Turn To Obelisk!").has_value() && !scene.ui.menus->Point("CHEAT Dig Up").has_value());CHECK(scene.ui.menus->Point("Detonate!").has_value()==(owner==session.LocalOwner()));scene.menu->Clear();scene.ui.menus->CleanupFrame();CHECK(scene.world->Session().World().Destroy(source,0x10));scene.client.GameplayGlobals()->world.editor=1;
    }
}
TEST_CASE(SpecialObjectUi_CrystalParentDeletionDefersActualPopupCleanup) { // 대상 삭제가 실제 Form 통지로 팝업을 닫고 같은 삭제 큐에서 UI를 반납한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SpecialObjectUiScene scene(assets);auto& session=scene.world->Session();auto core=session.Core();const auto source=scene.Create(154,0);scene.Open(source);const auto form=Sid{static_cast<std::uint16_t>(Get(core.pool.Slot(source),6,2))};const auto root=scene.ui.buttons.RegisteredGump(session.GumpForms().Owner(form));CHECK(scene.ui.menus->Node(root));CHECK(scene.world->Session().World().Destroy(source,0x10));CHECK(!scene.menu->Active() && (core.pool.Slot(form)[11]&1U));CHECK(scene.ui.menus->Node(root) && (scene.ui.menus->Node(root)->flags&0x10000U));scene.ui.menus->CleanupFrame();CHECK(!scene.ui.menus->Node(root));scene.menu->Clear();CHECK(!scene.ui.menus->HasMenus());
}

TEST_CASE(SpecialObjectUi_SelectedSpellDeletionClearsWithoutRevisitingReleasedRange) { // 선택된 스펠의 종속 삭제 뒤 원문 Clear만 실행하고 실제 팝업을 지연 반납한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SpecialObjectUiScene scene(assets);scene.client.GameplayGlobals()->world.editor=1;auto& session=scene.world->Session();auto core=session.Core();const auto source=scene.Create(114,0);scene.Open(source);auto& selected=scene.client.GameplayGlobals()->selection;CHECK(selected.selected==source.value);const Sid form{static_cast<std::uint16_t>(Get(core.pool.Slot(source),6,2))},range{static_cast<std::uint16_t>(Get(core.pool.Slot(form),4,2))};CHECK(range.value && core.pool.Slot(range)[10]==51 && core.processes.FromForm(range));const auto root=scene.ui.buttons.RegisteredGump(session.GumpForms().Owner(form));CHECK(scene.world->Session().World().Destroy(source,0x10));CHECK(selected.selected==0 && selected.multipleCount==0 && selected.mode==0);CHECK((core.pool.Slot(source)[11]&1U) && (core.pool.Slot(form)[11]&1U) && (core.pool.Slot(range)[11]&1U));CHECK(!scene.menu->Active() && scene.ui.menus->Node(root) && (scene.ui.menus->Node(root)->flags&0x10000U));scene.ui.menus->CleanupFrame();CHECK(!scene.ui.menus->Node(root));scene.menu->Clear();CHECK(!scene.ui.menus->HasMenus());
}

TEST_CASE(BuildingObjectUi_FourWorkshopsUseCurrentOwnerLevelMoneyAndEditor) { // 네 테마의 실제 메뉴/가상 개수와 입력 큐를 같은 제품에서 연결한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SpecialObjectUiScene scene(assets);auto& session=scene.world->Session();auto core=session.Core();RawSquidLevel levels(core.pool,core.types,[](auto) { throw std::logic_error("정상 메뉴 레벨의 예상 밖 범위 진단"); });
    for (const auto type:{85U,86U,87U,88U}) for (const auto owner:{0U,1U,2U,8U}) for (const auto editor:{0U,1U}) { // 네 실제 테마×raw 소유자×명시 편집 모드이며 편집 포인터의 소유자 따라가기는 별도 기존 검사다.
        scene.client.GameplayGlobals()->world.editor=editor;const auto source=scene.Create(type,owner);const auto cost=session.HitPoints().TypeCostUnits(type);CHECK(cost>0);const bool own=owner==session.LocalOwner();
        for (const auto level:{0U,1U,2U,3U}) for (const auto money:{0.0F,static_cast<float>(cost)}) { // 원문 BYTE setter로 주어진 레벨과 실제 현재 SP의 경계 두 값을 입력한다.
            levels.Set(source,level);session.SetSp(session.LocalOwner(),money);const auto before=Hex(core.pool.Slot(source));const auto free=core.pool.FreeCount();scene.Open(source);
            CHECK(scene.ui.menus->Point("View Current Production").has_value()==own);CHECK(scene.ui.menus->Point("Put Knowledge into Production").has_value()==own);CHECK(scene.ui.menus->Point("Destroy").has_value()==(editor!=0));CHECK(scene.ui.menus->Point("Player").has_value()==(owner!=0));
            const auto upgrade=BuildingUiItem(scene,source,0x03000032U);CHECK(upgrade.has_value()==(own && level<3));
            if (upgrade) { CHECK(upgrade->parameter==source.value && !(upgrade->flags&0x200U));const auto event=scene.Click(BuildingUiCommand(upgrade->command,upgrade->parameter));CHECK(event.code==0x03000032U && std::bit_cast<std::uint32_t>(event.y)==source.value); } // SP 부족도 활성 항목으로 남고 실제 Factory 입력에서 거부해야 한다.
            CloseBuildingUi(scene);CHECK(Hex(core.pool.Slot(source))==before && core.pool.FreeCount()==free && session.Sp(session.LocalOwner())==money);
        }
        CHECK(session.World().Destroy(source,0x10));
    }
}

TEST_CASE(BuildingObjectUi_WorkshopClickInsufficientFundsAndCycleCurrentFrame) { // UI 클릭→같은 가상 입력의 자금 안내와 실제 상태/프레임 setter를 연결하며 패킷 요청은 실행하지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SpecialObjectUiScene scene(assets);auto& session=scene.world->Session();auto core=session.Core();
    for (const auto type:{85U,86U,87U,88U}) { // 네 실제 Factory의0→1→2→0을 같은 SID에서 연속 입력하고 메뉴를 다시 작성한다.
        const auto source=scene.Create(type,session.LocalOwner());session.SetSp(session.LocalOwner(),0);CHECK((core.pool.Slot(source)[11]>>5)==0);
        for (const auto expected:{1U,2U,0U}) { // 레벨 순환의 실제+84와 그림 보정을 거친 뒤 자금 부족 클릭은 상태를 보존한다.
            scene.client.GameplayGlobals()->world.editor=1;CHECK(session.CommandFactory(source,0x03000063U,source.value)==1);CHECK(std::uint32_t(core.pool.Slot(source)[11]>>5)==expected);scene.client.GameplayGlobals()->world.editor=0;const auto before=Hex(core.pool.Slot(source));const auto kernel=core.kernel.Size();scene.Open(source);const auto item=BuildingUiItem(scene,source,0x03000032U);CHECK(item.has_value());if (!item) throw std::logic_error("워크샾 실제 업그레이드 항목 없음");
            const auto event=scene.Click(BuildingUiCommand(item->command,item->parameter));const auto notices=scene.factoryNotices.size();CHECK(scene.menu->Command(event));CHECK(scene.factoryNotices.size()==notices+1 && scene.factoryNotices.back()=="upgradeNoBucks");CloseBuildingUi(scene);CHECK(Hex(core.pool.Slot(source))==before && core.kernel.Size()==kernel && session.Sp(session.LocalOwner())==0);
        }
        CHECK(session.World().Destroy(source,0x10));
    }
}

TEST_CASE(BuildingObjectUi_GeneratorsUseStrictHalfHpAndMissionOwnerGates) { // 세 제네레이터의 실제 최대HP/현재 모드와 비활성 controlled 클릭을 함께 확인한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SpecialObjectUiScene scene(assets);auto& session=scene.world->Session();auto core=session.Core();
    for (const auto type:{75U,76U,77U}) for (const auto owner:{0U,1U,2U,8U}) { // 실제 Battery VT인 세 테마만 사용하며 공중3종이나 공격을 실행하지 않는다.
        const auto source=scene.Create(type,owner);const auto maximum=session.HitPoints().MaxHitPoints(source);CHECK(maximum>2);
        for (const auto mission:{0U,1U}) for (const auto hp:{maximum/2-1,maximum/2,maximum/2+1,maximum}) { // 자연 상태 도달 대신 명시된 mission/HP 경계에서 메뉴 조건을 분리한다.
            scene.client.GameplayGlobals()->world.missionPost=mission;Put(core.pool.AllocatedBytes(source),26,std::bit_cast<std::uint32_t>(hp));const auto before=Hex(core.pool.Slot(source));const auto free=core.pool.FreeCount();scene.Open(source);const auto meltdown=BuildingUiItem(scene,source,0x030000f8U);CHECK(meltdown.has_value()==(!mission && owner==session.LocalOwner()));
            if (meltdown) { CHECK(meltdown->parameter==source.value && ((meltdown->flags&0x200U)==0)==(hp>maximum/2));const auto point=scene.ui.menus->Point("Meltdown!");CHECK(point.has_value());if (!point) throw std::logic_error("제네레이터 실제 Meltdown 항목 없음");scene.ui.Click(*point);if (hp>maximum/2) { const auto event=scene.ui.commands.Pop(false);CHECK(event.code==0x030000f8U && std::bit_cast<std::uint32_t>(event.y)==source.value); }else CHECK(scene.ui.commands.Empty()); } // 활성 클릭도 입력 큐까지만 확인하고 자폭/공격 효과를 실행하지 않는다.
            CloseBuildingUi(scene);CHECK(Hex(core.pool.Slot(source))==before && core.pool.FreeCount()==free);
        }
        CHECK(session.World().Destroy(source,0x10));
    }
}

TEST_CASE(BuildingObjectUi_ConstructionBlocksMenusWithoutAllocatingForms) { // 건설 중 extra 비트의 거부가 실제 가상ac에서 UI/선택/풀을 변경하지 않는지 확인한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SpecialObjectUiScene scene(assets);auto& session=scene.world->Session();auto core=session.Core();
    for (const auto type:{75U,76U,77U,85U,86U,87U,88U}) { // 제네레이터 공통ac와 네 워크샾 전용ac의 같은 앞단을 대조한다.
        const auto source=scene.Create(type,session.LocalOwner());auto raw=core.pool.AllocatedBytes(source);raw[40]|=1U;const auto before=Hex(raw);const auto free=core.pool.FreeCount(),selected=scene.client.GameplayGlobals()->selection.selected;CHECK(scene.menu->Open(*scene.world,source,{InputCode::kRightButton,304,229})==0);CHECK(!scene.menu->Active() && !scene.ui.menus->HasMenus() && Hex(raw)==before && core.pool.FreeCount()==free && scene.client.GameplayGlobals()->selection.selected==selected);raw[40]&=~1U;scene.Open(source);CloseBuildingUi(scene);CHECK(session.World().Destroy(source,0x10));
    }
}

TEST_CASE(ProductionObjectUi_FourThemesKeepDuplicateRowsAndDistinctSlots) { // 실제 내용물/Deck과 두 하위 메뉴를 연결하고0/양수/음수 슬롯의 controlled 입력을 확인한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    const std::array<std::pair<std::uint32_t,std::vector<std::uint32_t>>,4> cases{{{85,{71,128,136,75,76,77}},{86,{75,140,142,139}},{87,{76,129,145,147}},{88,{77,131,148,149}}}}; // 보호 자산의 각 테마와 썬 워크샾의 교차 제네레이터 후보이며 공중 유닛은 포함하지 않는다.
    for (const auto& [factory,candidates]:cases) { // 네 테마마다 같은 초기 미션에서 시작해 다른 워크샾의 전역 생산 영향을 분리한다.
        SpecialObjectUiScene scene(assets);auto& session=scene.world->Session();auto core=session.Core();session.ClearTechnology(1);session.AddTechnology(1,"sunArcher;sunCannon;sunFence;windBattery;rainBattery;thunderBattery;windArcher;windBlocker;windAviary;rainCannon;rainFence;rainBlocker;thunderCannon;thunderArcher;thunderFence;sunFactory;windVortex");session.TotalMade().AllowAll(1);const auto source=scene.Create(factory,1);CHECK(session.Production().FactoryCapacity(source)==2);
        CHECK(session.CreateContained(source,candidates.back(),0,0x20,0x10,0).value); // 생산 bit16이 없는 실제 내용물은 표시/슬롯/Deck에서 제외되어야 한다.
        for (const auto stage:{0U,1U,2U,3U}) { // 비생산 내용물→중복2개→다른 종류2개→다른 종류3개로 같은 부모를 누적한다.
            if (stage==1) { CHECK(session.CreateContained(source,candidates[0],0,0x10,0x40,0).value>=15000);CHECK(session.CreateContained(source,candidates[0],0,0x10,0x40,0).value>=15000); } // 서버 내용물은 예측 교정 대상이 아니며 pfDONT_TRANSMIT(40)으로 통신 없이 실제 중복 행을 유지한다.
            if (stage>1) CHECK(session.CreateContained(source,candidates[stage-1],0,0x10,0x40,0).value>=15000);static_cast<void>(session.Production().RefreshCurrent());const auto before=Hex(core.pool.Slot(source));const auto free=core.pool.FreeCount();scene.Open(source);
            const auto current=ProductionUiItems(scene,source,"View Current Production");
            for (std::size_t index=0;index<candidates.size();++index) { const auto expected=stage && index==0 ? 2U : (index<stage ? 1U : 0U);CHECK(ProductionUiCount(current,0x02000017U,candidates[index])==expected); } // 중복 행은2개이며 슬롯의 distinct 계산과 구별한다.
            CHECK(scene.Click("View Current Production").code==0);FinishProductionUiExpansion(scene);if (stage) { const auto event=scene.Click(BuildingUiCommand(0x02000017U,candidates[0]));std::printf("PROD help event=%x parameter=%x\n",event.code,std::bit_cast<std::uint32_t>(event.y));CHECK(event.code==0x02000017U && std::bit_cast<std::uint32_t>(event.y)==candidates[0]); }CloseBuildingUi(scene);CHECK(Hex(core.pool.Slot(source))==before && core.pool.FreeCount()==free);scene.Open(source);
            const auto knowledge=ProductionUiItems(scene,source,"Put Knowledge into Production");std::size_t rows{};
            for (std::size_t index=0;index<candidates.size();++index) { // 현재 전역 Deck에 생산 중인 종류만 숨기고 나머지는0/음수 슬롯에서 비활성화한다.
                const auto parameter=(candidates[index]<<16)|source.value;CHECK(ProductionUiCount(knowledge,0x03000080U,parameter)==(index<stage ? 0U : 1U));if (index>=stage) { const auto found=std::find_if(knowledge.begin(),knowledge.end(),[=](const auto& item) { return item.command==0x03000080U && item.parameter==parameter; });CHECK(found!=knowledge.end());if (found!=knowledge.end()) CHECK(((found->flags&0x200U)==0)==(stage<2));++rows; }
            }
            CHECK(static_cast<std::size_t>(std::count_if(knowledge.begin(),knowledge.end(),[](const auto& item) { return (item.flags&1U) && item.command==0x03000080U; }))==rows);CHECK(scene.Click("Put Knowledge into Production").code==0);FinishProductionUiExpansion(scene);const auto command=BuildingUiCommand(0x03000080U,(candidates.back()<<16)|source.value);const auto point=scene.ui.menus->Point(command);CHECK(point.has_value());if (!point) throw std::logic_error("남은 실제 지식 클릭 행 없음");scene.ui.Click(*point);
            if (stage<2) { const auto event=scene.ui.commands.Pop(false);CHECK(event.code==0x03000080U && std::bit_cast<std::uint32_t>(event.y)==((candidates.back()<<16)|source.value)); }else CHECK(scene.ui.commands.Empty());CloseBuildingUi(scene);CHECK(Hex(core.pool.Slot(source))==before && core.pool.FreeCount()==free);
        }
        CHECK(session.World().Destroy(source,0x10));
    }
}

TEST_CASE(ProductionObjectUi_DisallowedExcludedAndAllProducedRemainDifferent) { // 허용 표/생성 제외/전역 생산 중은 다른 조건이며 같은 실제 메뉴에서 구별해야 한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SpecialObjectUiScene scene(assets);auto& session=scene.world->Session();session.ClearTechnology(1);session.AddTechnology(1,"windBattery;windArcher");session.TotalMade().AllowAll(1);const auto source=scene.Create(86,1);CHECK(session.CreateContained(source,75,0,0x10,0x10,0).value);session.TotalMade().Allow(75,0);static_cast<void>(session.Production().RefreshCurrent());CHECK(session.Production().Contains(75));scene.Open(source);
    const auto current=ProductionUiItems(scene,source,"View Current Production");CHECK(ProductionUiCount(current,0x02000017U,75)==0);CHECK(scene.ui.menus->Report().find("No Production Yet.")!=std::string::npos);auto knowledge=ProductionUiItems(scene,source,"Put Knowledge into Production");CHECK(ProductionUiCount(knowledge,0x03000080U,(75U<<16)|source.value)==0);CHECK(ProductionUiCount(knowledge,0x03000080U,(140U<<16)|source.value)==1);CloseBuildingUi(scene);
    auto& excluded=session.ConstructionExcludedTypes();excluded.entries.resize(1);excluded.Append(140);CHECK(excluded.Items().size()==1 && excluded.Items()[0]==140);scene.Open(source);knowledge=ProductionUiItems(scene,source,"Put Knowledge into Production");CHECK(knowledge.empty());CHECK(scene.ui.menus->Report().find("All Knowledge is in Production")==std::string::npos);CloseBuildingUi(scene); // 실제 빈 목록에 저장 용량을 주고 후보는 있지만 include가false인 창을 모든 생산 완료 안내와 구별한다.
    session.ConstructionExcludedTypes().RemoveAll(140);CHECK(session.CreateContained(source,140,0,0x10,0x10,0).value);static_cast<void>(session.Production().RefreshCurrent());scene.Open(source);knowledge=ProductionUiItems(scene,source,"Put Knowledge into Production");CHECK(knowledge.size()==1 && knowledge[0].command==0x02000089U && (knowledge[0].flags&0x200U));CHECK(scene.Click("Put Knowledge into Production").code==0);const auto point=scene.ui.menus->Point("All Knowledge is in Production");CHECK(point.has_value());if (point) scene.ui.Click(*point);CHECK(scene.ui.commands.Empty());CloseBuildingUi(scene);CHECK(session.World().Destroy(source,0x10));
}

TEST_CASE(ProductionObjectUi_ClientPredictionAndServerGapsUseFirstMatchingForm) { // 예측 정리 뒤 순서 빈자리와 서버 삽입이 바꾸는 첫 검색 결과를 실제 부모 Notice/메뉴에서 구별한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SpecialObjectUiScene scene(assets);auto& session=scene.world->Session();auto core=session.Core();session.ClearTechnology(1);session.AddTechnology(1,"windBattery;windArcher");session.TotalMade().AllowAll(1);const auto source=scene.Create(86,1);
    const auto first=session.CreateContained(source,75,0,0x10,0x10,0),second=session.CreateContained(source,75,0,0x10,0x10,0);CHECK(first.value<15000 && second.value<15000 && first!=second);CHECK((core.pool.Slot(first)[11]&1U) && !(core.pool.Slot(second)[11]&1U));scene.Open(source);CHECK(ProductionUiCount(ProductionUiItems(scene,source,"View Current Production"),0x02000017U,75)==1);CloseBuildingUi(scene);
    CHECK(core.pool.Slot(second)[23]==1);const auto server=session.CreateContained(source,75,0,0x10,0x40,0);CHECK(server.value>=15000 && !(core.pool.Slot(second)[11]&1U) && core.pool.Slot(server)[23]==0);const auto duplicate=session.CreateContained(source,75,0,0x10,0x40,0);CHECK(duplicate.value>=15000 && server!=duplicate && !(core.pool.Slot(server)[11]&1U) && core.pool.Slot(duplicate)[23]==2);scene.Open(source);CHECK(ProductionUiCount(ProductionUiItems(scene,source,"View Current Production"),0x02000017U,75)==3);const auto knowledge=ProductionUiItems(scene,source,"Put Knowledge into Production");CHECK(ProductionUiCount(knowledge,0x03000080U,(75U<<16)|source.value)==0 && ProductionUiCount(knowledge,0x03000080U,(140U<<16)|source.value)==1);const auto remaining=std::find_if(knowledge.begin(),knowledge.end(),[=](const auto& item) { return item.command==0x03000080U && item.parameter==((140U<<16)|source.value); });CHECK(remaining!=knowledge.end() && !(remaining->flags&0x200U));CloseBuildingUi(scene);CHECK(session.World().Destroy(source,0x10)); // 원문453b50은 첫 일치만 검사하므로 자신보다 뒤의 client를 추가 순회해 지우지 않는다.
}

TEST_CASE(ProductionObjectUi_SalvageUsesCurrentOwnerEditorDenyAndConfirmation) { // 회수/금지/워크샾·신전 확인/고갈 간헐천 억제 메뉴를 실제 상태와 연결하되 요청은 보내지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SpecialObjectUiScene scene(assets);auto& session=scene.world->Session();auto core=session.Core();
    for (const auto type:{75U,128U,85U,123U,122U}) for (const auto owner:{0U,1U,2U,8U}) for (const auto editor:{0U,1U}) for (const auto deny:{0,1}) { // 일반 직접 회수2종·두 확인 종류·간헐천의 네 소유자와 명시 편집/금지 조건이다.
        scene.client.GameplayGlobals()->world.editor=editor;scene.client.GameplayGlobals()->DenySalvage()=deny;Sid source{};std::uint32_t savedOwner{};if (type==123) { for (std::uint32_t number=1;number<core.pool.Capacity();++number) { const Sid candidate{static_cast<std::uint16_t>(number)};const auto raw=core.pool.Slot(candidate);if (!(raw[11]&3U) && raw[10]>=70 && (core.types[raw[10]].flags2&0x200U)) { source=candidate;savedOwner=raw[34];break; } }CHECK(source.value);if (!source.value) throw std::logic_error("원래 미션의 신전 없음");core.owner.Set(source,owner); }else { session.World().SetLoadingDepth(1);source=scene.Create(type,owner);session.World().SetLoadingDepth(0); }const auto before=Hex(core.pool.Slot(source));const auto free=core.pool.FreeCount();scene.Open(source);const auto direct=BuildingUiItem(scene,source,0x0200005dU),tell=BuildingUiItem(scene,source,0x02000095U);const bool own=owner==session.LocalOwner() && !editor; // 신전은 실제 로드된 섬/QA를 유지하고 나머지는 저장 오브젝트와 같은 로딩 구간에서 client Pop한다.
        CHECK(direct.has_value()==(own && !deny && (type==75 || type==128)));if (direct) { const auto event=scene.Click(BuildingUiCommand(direct->command,direct->parameter));CHECK(event.code==0x0200005dU && std::bit_cast<std::uint32_t>(event.y)==source.value); }
        if (own && (deny || type==85 || type==123)) { CHECK(tell.has_value());if (!tell) throw std::logic_error("회수 안내의 실제 Tell 항목 없음");const auto expected=deny ? "DenySalvage" : (type==85 ? "SalvageInf" : "SalvageVortex");CHECK(scene.menu->StaticText(tell->parameter)==expected);const auto event=scene.Click(BuildingUiCommand(tell->command,tell->parameter));CHECK(event.code==0x02000095U && std::bit_cast<std::uint32_t>(event.y)==tell->parameter); }
        CloseBuildingUi(scene);CHECK(Hex(core.pool.Slot(source))==before && core.pool.FreeCount()==free);if (type==123) core.owner.Set(source,savedOwner);else CHECK(session.World().Destroy(source,0x10)); // 미션의 원래 신전은 소유자를 복원하며 새 검사 대상만 반납한다.
    }
}

TEST_CASE(StateObjectUi_DamagedSalvageReadsCurrentBattleMode) { // 실제 메뉴가 현재 전투 모드를 읽어 HP 절반 미만의 환불만0으로 바꾸는지 확인한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SpecialObjectUiScene scene(assets);auto& session=scene.world->Session();auto core=session.Core();RawSquidLevel levels(core.pool,core.types,[](auto) { throw std::logic_error("정상 손상 메뉴 레벨의 예상 밖 진단"); });const auto savedBattle=core.ownerMode.battle;
    for (const auto type:{75U,86U}) for (const auto level:{0U,2U}) { // 직접 회수/워크샾 확인의 두 명령과 원문 비용 곡선의 첫/세 번째 레벨을 사용한다.
        const auto source=scene.Create(type,session.LocalOwner());levels.Set(source,level);const auto maximum=session.HitPoints().MaxHitPoints(source),cost=session.HitPoints().TypeCostUnits(type);CHECK(maximum>2 && cost>0);
        for (const auto hp:{maximum/2-1,maximum/2,maximum/2+1,maximum}) for (const auto battle:{false,true,false}) { // 같은 세계에서0→1→0을 바꾸어 초기 모드 사본이 아닌 현재 값을 요구한다.
            core.ownerMode.battle=battle;Put(core.pool.AllocatedBytes(source),26,std::bit_cast<std::uint32_t>(hp));const auto before=Hex(core.pool.Slot(source));const auto free=core.pool.FreeCount();scene.Open(source);const auto normal=static_cast<std::int32_t>((std::int64_t(cost)*(level+1)*hp)/(4*maximum));const auto amount=battle && hp<maximum/2 ? 0 : normal;CHECK(normal>0);const auto label="Salvage gains "+std::to_string(amount);const auto point=scene.ui.menus->Point(label);CHECK(point.has_value());if (!point) std::printf("DAMAGE type=%u level=%u hp=%d maximum=%d battle=%u expected=%d\n%s",type,level,hp,maximum,battle ? 1U : 0U,amount,scene.ui.menus->Report().c_str());
            if (point) { const auto event=scene.Click(label);CHECK(event.code==(type==75 ? 0x0200005dU : 0x02000095U));if (type==75) CHECK(std::bit_cast<std::uint32_t>(event.y)==source.value);else CHECK(scene.menu->StaticText(std::bit_cast<std::uint32_t>(event.y))=="SalvageInf"); }CloseBuildingUi(scene);CHECK(Hex(core.pool.Slot(source))==before && core.pool.FreeCount()==free);
        }
        core.ownerMode.battle=savedBattle;CHECK(session.World().Destroy(source,0x10));
    }
}

TEST_CASE(StateObjectUi_AltarDaisDelegationUsesCurrentFramesOwnersAndEditor) { // 사제 없는 원래 제단/다이스의 실제 프레임 setter와 가상ac 재분배·제목·인자를 연결한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SpecialObjectUiScene scene(assets,"breakingthrough");auto& session=scene.world->Session();auto core=session.Core();const auto pair=FindStateUiAltarPair(scene);const auto savedFrame=Get(core.pool.Slot(pair.dais),36);const auto altarOwner=core.pool.Slot(pair.altar)[34],daisOwner=core.pool.Slot(pair.dais)[34];
    const std::array<std::pair<std::int32_t,std::string_view>,3> frames{{{3,"I"},{64,"II"},{120,"III"}}}; // 원문 세 레벨의 실제 첫 프레임이며 의식/희생 단계 진행은 실행하지 않는다.
    for (const auto& [frame,level]:frames) for (const auto owner:{0U,1U,2U,8U}) for (const auto editor:{0U,1U}) { // 프레임3×현재 소유자4×명시 편집2의 같은 실제 한 쌍을 재사용한다.
        core.frame.Set(pair.dais,frame,0);core.owner.Set(pair.altar,owner);core.owner.Set(pair.dais,owner);scene.client.GameplayGlobals()->world.editor=editor;CHECK(!session.Priests().Carrier().FindContained(pair.dais,0,158,0,false).value);
        for (const auto source:{pair.altar,pair.dais}) { // 다이스 우클릭도 같은 제단에 메뉴 Form을 붙이고 About은 TYPE164를 전달해야 한다.
            const auto beforeAltar=Hex(core.pool.Slot(pair.altar)),beforeDais=Hex(core.pool.Slot(pair.dais));const auto free=core.pool.FreeCount();scene.Open(source);CHECK(scene.ui.menus->Report().find("Altar Level "+std::string(level))!=std::string::npos);const auto about=BuildingUiItem(scene,pair.altar,0x02000017U),salvage=BuildingUiItem(scene,pair.altar,0x0200005dU);CHECK(about && about->parameter==164);CHECK(salvage.has_value()==(owner==session.LocalOwner() && !editor));if (salvage) CHECK(salvage->parameter==pair.altar.value);CHECK(scene.ui.menus->Point("Destroy").has_value()==(editor!=0));CHECK(scene.ui.menus->Point("Player").has_value()==(owner!=0));const auto event=scene.Click("About");CHECK(event.code==0x02000017U && event.y==164);CloseBuildingUi(scene);CHECK(Hex(core.pool.Slot(pair.altar))==beforeAltar && Hex(core.pool.Slot(pair.dais))==beforeDais && core.pool.FreeCount()==free);
        }
    }
    core.owner.Set(pair.altar,altarOwner);core.owner.Set(pair.dais,daisOwner);core.frame.Set(pair.dais,std::bit_cast<std::int32_t>(savedFrame),0);
}

TEST_CASE(StateObjectUi_EmptyAltarStatusHeadersAndLevelClampReleaseMenus) { // 다이스 없는 제단의Count0 표시와 두 상태 안내를 효과/포획 없이 현재 raw 입력에서 확인한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SpecialObjectUiScene scene(assets);auto& session=scene.world->Session();auto core=session.Core();
    for (const auto owner:{0U,1U,2U,8U}) for (const auto editor:{0U,1U}) { // 원래 빈 좌표에 제단 본체만 배치하며 다이스나 사제를 만들지 않는다.
        scene.client.GameplayGlobals()->world.editor=editor;const auto source=scene.Create(164,owner);const auto extra=core.pool.Slot(source)[40];
        for (const auto status:{0U,0x10U,0x20U,0x30U}) { // 미적용/투명/마비/두 상태의 안내 BYTE만 명시 입력으로 제공한다.
            core.pool.AllocatedBytes(source)[40]=static_cast<std::uint8_t>((extra&~0x30U)|status);const auto before=Hex(core.pool.Slot(source));const auto free=core.pool.FreeCount();scene.Open(source);const auto report=scene.ui.menus->Report();CHECK(report.find("Altar Level I")!=std::string::npos);CHECK((report.find("Invisible!")!=std::string::npos)==((status&0x10U)!=0));CHECK((report.find("Paralyzed!")!=std::string::npos)==((status&0x20U)!=0));if (owner==session.LocalOwner() && !editor) CHECK(scene.ui.menus->Point("Salvage gains 0").has_value());const auto event=scene.Click("About");CHECK(event.code==0x02000017U && event.y==164);CloseBuildingUi(scene);CHECK(Hex(core.pool.Slot(source))==before && core.pool.FreeCount()==free);
        }
        core.pool.AllocatedBytes(source)[40]=extra;CHECK(session.World().Destroy(source,0x10)); // 상태 효과의 적용/해제 몸체를 실행하지 않고 UI 입력을 복원한 뒤 반납한다.
    }
}

TEST_CASE(StateObjectUi_EditorDestroyClickReleasesPairAndPopupWithoutTransmission) { // 편집 메뉴의 실제 명령/SID와 전송을 생략한 같은 가상 삭제/팝업 지연 반납을 확인한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    for (const auto clickDais:{false,true}) { // 두 실제 우클릭 진입점마다 보호 Fort를 새로 읽으며 저장 파일은 쓰지 않는다.
        SpecialObjectUiScene scene(assets,"breakingthrough");scene.client.GameplayGlobals()->world.editor=1;auto& session=scene.world->Session();auto core=session.Core();const auto pair=FindStateUiAltarPair(scene);scene.Open(clickDais ? pair.dais : pair.altar);const Sid form{static_cast<std::uint16_t>(Get(core.pool.Slot(pair.altar),6,2))};CHECK(session.GumpForms().IsGumpForm(form));const auto root=scene.ui.buttons.RegisteredGump(session.GumpForms().Owner(form));const auto event=scene.Click("Destroy");CHECK(event.code==0x03000015U && std::bit_cast<std::uint32_t>(event.y)==pair.altar.value);CHECK(session.World().Destroy(Sid{static_cast<std::uint16_t>(event.y)},0x40));CHECK((core.pool.Slot(pair.altar)[11]&1U) && (core.pool.Slot(pair.dais)[11]&1U) && (core.pool.Slot(form)[11]&1U));CHECK(!scene.menu->Active() && scene.ui.menus->Node(root) && (scene.ui.menus->Node(root)->flags&0x10000U));scene.ui.menus->CleanupFrame();CHECK(!scene.ui.menus->Node(root));CloseBuildingUi(scene); // 본체는 server SID이므로 pfDONT_TRANSMIT(40)을 명시한다. 공통 명령의flags0 분배는 기존 원문fixture로 확인하며 통신 효과를 실행하지 않는다.
    }
}

TEST_CASE(CarrierObjectUi_AllEightTypesUseActualProjectionAndTargetCursors) { // 보호 운반체 전체의 자기/지면/간헐천·신전/빈 공간 커서를 실제 선택/카메라/폴링에서 확인한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);std::vector<std::uint32_t> discovered;
    for (std::uint32_t type=70;type<assets.types.size();++type) if (assets.types[type].flags2&0x30000U) discovered.push_back(type); // 실제 로드된188TYPE에서 전체 대상 집합을 확인한다.
    CHECK(discovered==std::vector<std::uint32_t>(kCarrierUiTypes.begin(),kCarrierUiTypes.end()));
    for (const auto type:kCarrierUiTypes) { // 판본 자산은 공유하지만 각 타입의 실제 World/선택/이동 상태는 독립시킨다.
        SpecialObjectUiScene scene(assets);auto& session=scene.world->Session();auto core=session.Core();const auto source=CarrierUiActor(scene,type);const auto raw=core.pool.Slot(source);const auto x=std::bit_cast<float>(Get(raw,14)),y=std::bit_cast<float>(Get(raw,18));scene.world->SetMousePoint(CarrierUiCell(scene,x,y));scene.world->Select(source.value);CHECK(scene.world->Selected()==source.value && scene.world->InputMode()==5);const auto before=Hex(core.pool.Slot(source));const auto free=core.pool.FreeCount();const auto kernel=core.kernel.Size();const auto sp=session.Sp(1);
        for (const auto camera:std::array<std::array<int,2>,2>{{{976,793},{1040,880}}}) { //536의 독립 화면 점을 두 실제 카메라에서 재사용한다.
            session.CameraGlobals().ApplyPixelsToRendererAndSnapshot(camera[0],camera[1]);PollCarrierUi(scene,{1611-camera[0],1148-camera[1]},3);PollCarrierUi(scene,{1611-camera[0],1115-camera[1]},18);PollCarrierUi(scene,CarrierUiCell(scene,x,y),6);
        }
        Sid temple{};for (std::uint32_t number=5;number<core.pool.Capacity();++number) { const Sid candidate{static_cast<std::uint16_t>(number)};const auto item=core.pool.Slot(candidate);if (!(item[11]&3U) && item[10]>=70 && item[34]==1 && (core.types[item[10]].flags2&0x200U)) { temple=candidate;break; } } // 실제 현재 소유자의 원래 신전을 찾는다.
        CHECK(temple.value);if (!temple.value) throw std::logic_error("운반체 커서의 원래 신전 없음");const auto templeRaw=core.pool.Slot(temple);const auto templeX=std::bit_cast<float>(Get(templeRaw,14)),templeY=std::bit_cast<float>(Get(templeRaw,18));CHECK(core.types[templeRaw[10]].footX>2 && core.types[templeRaw[10]].footY>2); // 신전의 저장 좌표는 발자국 오른쪽 아래 경계이므로 내부8비트와 구별한다.
        CHECK(!(core.spots[static_cast<std::size_t>(templeY)*256+static_cast<std::size_t>(templeX)]&8U));CHECK(core.spots[(static_cast<std::size_t>(templeY)-1)*256+static_cast<std::size_t>(templeX)-1]&8U);PollCarrierUiWorld(scene,templeX,templeY,18);PollCarrierUiWorld(scene,templeX-1,templeY-1,3);PollCarrierUiWorld(scene,20,20,(core.types[type].flags2&0x20000U) ? 18U : 6U);CHECK(Hex(core.pool.Slot(source))==before && core.pool.FreeCount()==free && core.kernel.Size()==kernel && session.Sp(1)==sp);
        scene.world->Select(0);
    }
}

TEST_CASE(CarrierObjectUi_CargoAndCrystalKeepNativeGroundCursorHistory) { // 실제 너겟 내용물과 후보를 사용해 빈/운반 중·지상/벌룬의 원문 강조 이력을 구별한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    for (const auto type:kCarrierUiTypes) { // 실제8종에서 포획 대상 사제 없이 스톰 크리스탈만 후보/내용물로 제공한다.
        SpecialObjectUiScene scene(assets);auto& session=scene.world->Session();auto core=session.Core();const auto source=CarrierUiActor(scene,type);const auto crystal=session.World().Create(154,2);session.World().Pop(crystal,102,102,0x10);const bool flying=(core.types[type].flags2&0x20000U)!=0;
        for (const auto cargo:{false,true}) { // 클라이언트 내용물은 실제 부모 알림으로 부착하며 게임 Kernel은 진행하지 않는다.
            scene.world->Select(0);if (cargo) CHECK(session.CreateContained(source,154,25,0x20,0x10,0).value);const auto raw=core.pool.Slot(source);const auto x=std::bit_cast<float>(Get(raw,14)),y=std::bit_cast<float>(Get(raw,18));scene.world->SetMousePoint(CarrierUiCell(scene,x,y));scene.world->Select(source.value);const auto before=Hex(core.pool.Slot(source));const auto target=Hex(core.pool.Slot(crystal));const auto free=core.pool.FreeCount();const auto kernel=core.kernel.Size();
            PollCarrierUiWorld(scene,101,102,18);PollCarrierUiWorld(scene,102,102,cargo ? 18U : 13U);session.CameraGlobals().ApplyPixelsToRendererAndSnapshot(976,793);PollCarrierUi(scene,{635,355},3);PollCarrierUiWorld(scene,102,102,cargo ? (flying ? 18U : 3U) : 13U);PollCarrierUiWorld(scene,x,y,6);CHECK(Hex(core.pool.Slot(source))==before && Hex(core.pool.Slot(crystal))==target && core.pool.FreeCount()==free && core.kernel.Size()==kernel); // 운반 중 지상 후보에서는 이전cursor를 유지하며 벌룬은 직접18을 반환한다.
        }
        scene.world->Select(0);
    }
}

TEST_CASE(CarrierObjectUi_CurrentOwnerEditorParalysisAndRealGroupOverride) { // 현재 소유자/편집/마비 선택 조건과 실제 Shift 추가 선택의 강제 이동 커서를 확인한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    for (const auto type:kCarrierUiTypes) { // 현재 selected/여러 선택을 각 실제 World의 같은 장기 전역에 둔다.
        SpecialObjectUiScene scene(assets);auto& session=scene.world->Session();auto core=session.Core();const auto source=CarrierUiActor(scene,type);const auto extra=core.pool.Slot(source)[40];scene.world->Highlight(4);scene.world->Highlight(0); // 실제 Client의 초기 화살표1을 모드 전환으로 준비하며 같은모드0의 무효과를 유지한다.
        for (const auto owner:{0U,1U,2U}) for (const auto editor:{0U,1U}) for (const auto paralyzed:{false,true}) { // 편집 포인터의 소유자 따라가기는 기존 검사이며 여기서는 현재 로컬1을 고정한 명시 상태다.
            scene.world->Select(0);core.owner.Set(source,owner);scene.client.GameplayGlobals()->world.editor=editor;core.pool.AllocatedBytes(source)[40]=static_cast<std::uint8_t>((extra&~0x20U)|(paralyzed ? 0x20U : 0U));const auto raw=core.pool.Slot(source);scene.world->SetMousePoint(CarrierUiCell(scene,std::bit_cast<float>(Get(raw,14)),std::bit_cast<float>(Get(raw,18))));scene.world->Select(source.value);CHECK(scene.world->Selected()==source.value);const bool moving=editor || owner==session.LocalOwner();CHECK(scene.world->InputMode()==(moving ? 5U : 0U));session.CameraGlobals().ApplyPixelsToRendererAndSnapshot(976,793);PollCarrierUi(scene,{635,355},moving ? (paralyzed ? 6U : 3U) : 1U);
        }
        scene.world->Select(0);core.owner.Set(source,1);scene.client.GameplayGlobals()->world.editor=0;core.pool.AllocatedBytes(source)[40]=extra;const auto group=session.World().Create(133,2);core.owner.Set(group,1);session.World().SetLoadingDepth(1);session.World().Pop(group,95,110,0x10);session.World().SetLoadingDepth(0);const auto raw=core.pool.Slot(source);scene.world->SetMousePoint(CarrierUiCell(scene,std::bit_cast<float>(Get(raw,14)),std::bit_cast<float>(Get(raw,18))));scene.world->Select(source.value);scene.world->ExtendSelection(group.value,0);CHECK(scene.world->SelectionGlobals().multipleCount>0);session.CameraGlobals().ApplyPixelsToRendererAndSnapshot(976,793);PollCarrierUi(scene,{635,355},18);core.pool.AllocatedBytes(source)[40]|=0x20U;PollCarrierUi(scene,{635,355},18);scene.world->Select(0);core.pool.AllocatedBytes(source)[40]=extra; // 마비보다 다중 강조의18이 우선이며 실제 추가 선택 목록을 정리한다.
    }
}

TEST_CASE(CarrierObjectUi_AllEightCargoMenusUseActualDisabledFlagsAndSid) { // 같은 실제 Carrier/사제 메뉴가 운반물·현재 소유자/편집/마비에 맞는Drop 행을 만드는지 확인한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    for (const auto type:kCarrierUiTypes) { // 여덟 운반체에 스톰 크리스탈 내용물만 실제 부착하며 포획/희생을 하지 않는다.
        SpecialObjectUiScene scene(assets);auto& session=scene.world->Session();auto core=session.Core();const auto source=CarrierUiActor(scene,type);CHECK(session.CreateContained(source,154,25,0x20,0x10,0).value);const auto extra=core.pool.Slot(source)[40];
        for (const auto owner:{1U,2U}) for (const auto editor:{0U,1U}) for (const auto paralyzed:{false,true}) { // 일반/명시 편집의 두 소유자와 두 이동 상태에서 같은 실제 팝업을 작성한다.
            scene.world->Select(0);core.owner.Set(source,owner);scene.client.GameplayGlobals()->world.editor=editor;core.pool.AllocatedBytes(source)[40]=static_cast<std::uint8_t>((extra&~0x20U)|(paralyzed ? 0x20U : 0U));scene.Open(source);const Sid form{static_cast<std::uint16_t>(Get(core.pool.Slot(source),6,2))};CHECK(session.GumpForms().IsGumpForm(form));const auto free=core.pool.FreeCount();const auto about=BuildingUiItem(scene,source,0x02000017U),drop=BuildingUiItem(scene,source,0x03000076U);CHECK(about && about->parameter==type);CHECK(drop.has_value()==(owner==session.LocalOwner()));CHECK(scene.ui.menus->Report().find("Carrying:")!=std::string::npos);
            if (drop) { CHECK(drop->parameter==source.value && ((drop->flags&0x200U)==0)==!paralyzed);const auto point=scene.ui.menus->Point(BuildingUiCommand(drop->command,drop->parameter));CHECK(point.has_value());if (!point) throw std::logic_error("운반물 실제 Drop 행 없음");scene.ui.Click(*point);if (paralyzed) CHECK(scene.ui.commands.Empty());else { const auto event=scene.ui.commands.Pop(false);CHECK(event.code==0x03000076U && std::bit_cast<std::uint32_t>(event.y)==source.value); } }else { const auto event=scene.Click("About");CHECK(event.code==0x02000017U && std::bit_cast<std::uint32_t>(event.y)==type); }CloseBuildingUi(scene);CHECK((core.pool.Slot(form)[11]&1U) && core.pool.FreeCount()==free+1 && core.pool.Slot(source)[10]==type && core.pool.Slot(source)[34]==owner); // 실제 요청/내려놓기는 실행하지 않고 UI 명령 인자와 반납만 확인한다.
        }
        scene.world->Select(0);core.owner.Set(source,1);core.pool.AllocatedBytes(source)[40]=extra;
    }
}

TEST_CASE(CarrierObjectUi_SelfCellCrystalAndCargoUseCurrentCandidateGraph) { // 원문4277f0의 자기 셀은 같은 Graph의 크리스탈 후보일 때8이며 운반 중에는6이다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    for (const auto type:kCarrierUiTypes) { // 여덟 실제 운반체에서 자신과 겹치는 크리스탈만 제공하고 집기 요청은 실행하지 않는다.
        SpecialObjectUiScene scene(assets);auto& session=scene.world->Session();auto core=session.Core();const auto source=CarrierUiActor(scene,type);const auto raw=core.pool.Slot(source);const auto x=std::bit_cast<float>(Get(raw,14)),y=std::bit_cast<float>(Get(raw,18));const auto crystal=session.World().Create(154,2);session.World().SetLoadingDepth(1);session.World().Pop(crystal,x,y,0x10);session.World().SetLoadingDepth(0);
        scene.world->SetMousePoint(CarrierUiCell(scene,x,y));scene.world->Select(source.value);const auto before=Hex(core.pool.Slot(source)),target=Hex(core.pool.Slot(crystal));const auto free=core.pool.FreeCount();const auto kernel=core.kernel.Size();
        CHECK(!(core.types[type].flags1&0x1000000U));PollCarrierUiWorld(scene,x,y,8);CHECK(Hex(core.pool.Slot(source))==before && Hex(core.pool.Slot(crystal))==target && core.pool.FreeCount()==free && core.kernel.Size()==kernel);
        CHECK(session.CreateContained(source,154,25,0x20,0x10,0).value);const auto loaded=Hex(core.pool.Slot(source));const auto loadedFree=core.pool.FreeCount();const auto loadedKernel=core.kernel.Size();PollCarrierUiWorld(scene,x,y,6);CHECK(Hex(core.pool.Slot(source))==loaded && Hex(core.pool.Slot(crystal))==target && core.pool.FreeCount()==loadedFree && core.kernel.Size()==loadedKernel);
    }
}

TEST_CASE(CarrierObjectUi_PriestHalfHpAndUnsupportedGroundBlockHover) { // 원문492090의 HP 절반·현재 지면 조건을 같은 선택의 폴링에서 읽고 손상/회복 행동은 실행하지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SpecialObjectUiScene scene(assets);auto& session=scene.world->Session();auto core=session.Core();const auto source=CarrierUiActor(scene,158);auto raw=core.pool.AllocatedBytes(source);const auto hp=Get(raw,26);const auto half=session.HitPoints().MaxHitPoints(source)/2;const auto x=std::bit_cast<float>(Get(raw,14)),y=std::bit_cast<float>(Get(raw,18));scene.world->SetMousePoint(CarrierUiCell(scene,x,y));scene.world->Select(source.value);const auto free=core.pool.FreeCount();const auto kernel=core.kernel.Size();
    for (const auto value:{half-1,half,half+1}) { // 절반 바로 아래만 정지하고 절반 이상은 실제 지면의 간헐천 커서3을 유지한다.
        Put(raw,26,std::bit_cast<std::uint32_t>(value));const auto before=Hex(raw);session.CameraGlobals().ApplyPixelsToRendererAndSnapshot(976,793);PollCarrierUi(scene,{635,355},value<half ? 6U : 3U);CHECK(Hex(raw)==before && core.pool.FreeCount()==free && core.kernel.Size()==kernel);
    }
    Put(raw,26,hp);session.World().Unpop(source,0x10);session.World().SetLoadingDepth(1);session.World().Pop(source,20,20,0x10);session.World().SetLoadingDepth(0);CHECK(!(core.spots[20*256+20]&6U));CHECK(session.Priests().PriestState().GroundImmobile(source));const auto before=Hex(core.pool.Slot(source));PollCarrierUiWorld(scene,101,102,6);CHECK(Hex(core.pool.Slot(source))==before);
}

TEST_CASE(CarrierObjectUi_EmptyDaisUsesOffsetWithoutSacrificeRequest) { // 원문449710의 사제 없는 다이스를 가리키면 의식17 대신 일반 접근3과 좌표−3을 사용한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    for (const auto type:kCarrierUiTypes) { // 실제 빈 다이스와 여덟 운반체를 같은 지면에 두고 평가만 수행한다.
        SpecialObjectUiScene scene(assets);auto& session=scene.world->Session();auto core=session.Core();const auto source=CarrierUiActor(scene,type);const auto dais=session.World().Create(165,2);core.owner.Set(dais,1);session.World().SetLoadingDepth(1);session.World().Pop(dais,102,102,0x10);session.World().SetLoadingDepth(0);CHECK(!session.Priests().Carrier().FindContained(dais,0,158,0,false).value);
        const auto raw=core.pool.Slot(source);scene.world->SetMousePoint(CarrierUiCell(scene,std::bit_cast<float>(Get(raw,14)),std::bit_cast<float>(Get(raw,18))));scene.world->Select(source.value);const auto before=Hex(core.pool.Slot(source)),target=Hex(core.pool.Slot(dais));const auto free=core.pool.FreeCount();const auto kernel=core.kernel.Size();PollCarrierUiWorld(scene,102,102,3);
        auto input=session.MakeCarrierInput({});RawPathPoint point{102,102};CHECK(input->Evaluate(source,point)==3 && point.x==99 && point.y==99);CHECK(Hex(core.pool.Slot(source))==before && Hex(core.pool.Slot(dais))==target && core.pool.FreeCount()==free && core.kernel.Size()==kernel);
    }
}

TEST_CASE(CarrierObjectUi_CurrentSurfaceGraphsKeepGroundHistoryAndFlyingCandidate) { // 같은 실제 앵커가 매 폴링의 현재 Graph BYTE를 읽으며 지상/벌룬의 후보 관문을 구별한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const std::array<std::array<std::uint8_t,2>,6> numbers{{{7,7},{7,9},{9,7},{254,254},{254,7},{7,254}}}; // 정상 같은/다른 번호와 원문 무효254 동등 경계를 입력한다.
    for (const auto type:kCarrierUiTypes) { // 여덟 운반체의 크리스탈 강조만 검사하며 집기/이동/포획 요청을 호출하지 않는다.
        SpecialObjectUiScene scene(assets);auto& session=scene.world->Session();auto core=session.Core();const auto source=CarrierUiActor(scene,type);const auto raw=core.pool.Slot(source);const auto x=std::bit_cast<float>(Get(raw,14)),y=std::bit_cast<float>(Get(raw,18));const auto from=core.building.Surface(static_cast<int>(x),static_cast<int>(y)),to=core.building.Surface(102,102);CHECK(from.value && to.value && from!=to);if (!from.value || !to.value || from==to) throw std::logic_error("Graph UI의서로다른실제표면필요");
        const auto fromGraph=core.pool.Slot(from)[30],toGraph=core.pool.Slot(to)[30];const auto crystal=session.World().Create(154,2);session.World().SetLoadingDepth(1);session.World().Pop(crystal,102,102,0x10);session.World().SetLoadingDepth(0);scene.world->SetMousePoint(CarrierUiCell(scene,x,y));scene.world->Select(source.value);const auto before=Hex(core.pool.Slot(source)),target=Hex(core.pool.Slot(crystal));const auto free=core.pool.FreeCount();const auto kernel=core.kernel.Size();
        for (const auto& number:numbers) { // 실제 setter로 BYTE만 바꾼 명시 상태이며 Graph 레코드/flood를 새로 구성하지 않는다.
            core.graph.SetNumber(from,number[0]);core.graph.SetNumber(to,number[1]);const bool flying=(core.types[type].flags2&0x20000U)!=0;const auto previous=flying || number[0]==254 ? 18U : 6U;PollCarrierUiWorld(scene,20,20,previous);const auto expected=flying || number[0]==number[1] ? 13U : previous;PollCarrierUiWorld(scene,102,102,expected);auto input=session.MakeCarrierInput({});RawPathPoint point{102,102};CHECK(input->Evaluate(source,point)==expected && point.x==102 && point.y==102);CHECK(Hex(core.pool.Slot(source))==before && Hex(core.pool.Slot(crystal))==target && core.pool.FreeCount()==free && core.kernel.Size()==kernel); // 자기 셀의 직접 반환6은5453d8을 갱신하지 않으므로 후보 없는 점에서 원문 이력을 쓴 뒤 비교한다.
        }
        core.graph.SetNumber(from,fromGraph);core.graph.SetNumber(to,toGraph);scene.world->Select(0); // 원래 표면 번호를 복구한 뒤 선택과 월드 대여를 반납한다.
    }
}

TEST_CASE(BuildingUpgradeUi_FourThemesCompleteAndReopenAtEveryLevel) { // 실제 event3의 그림 생성/완료가 현재 메뉴의 레벨·업그레이드 항목·Form 수명과 함께 바뀌는지 확인한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    for (const auto type:{85U,86U,87U,88U}) for (const auto level:{0U,1U,2U}) { // 네 테마의 세 정상 레벨을 각각 새 Session에서 확인하며 요청/패킷은 실행하지 않는다.
        SpecialObjectUiScene scene(assets);auto& session=scene.world->Session();auto core=session.Core();const auto source=scene.Create(type,session.LocalOwner());scene.client.GameplayGlobals()->world.editor=1;
        for (std::uint32_t step=0;step<level;++step) { CHECK(session.CommandFactory(source,0x03000063U,source.value)==1); } // 실제 편집 setter로 시작 레벨만 준비하고 성공 업그레이드 효과는 event3에 맡긴다.
        scene.client.GameplayGlobals()->world.editor=0;session.SetSp(session.LocalOwner(),10000);const auto money=session.Sp(session.LocalOwner());Put(core.pool.AllocatedBytes(source),26,1);scene.Open(source);CHECK(BuildingUiItem(scene,source,0x03000032U).has_value());CloseBuildingUi(scene);
        auto* regular=core.processes.AddSharedRegular(source,3,15);CHECK(regular && session.FactoryUpgrading(source));if (!regular) throw std::logic_error("UI 업그레이드의 실제 event3 생성 실패");regular->RunFrame();CHECK(regular->Count()==1);Sid image{};
        for (std::uint32_t value=5;value<15000;++value) { // 같은 실제 Depend가 연결한 예측 그림을 클라이언트 풀에서 찾아 다른 건물과 구별한다.
            const Sid candidate{static_cast<std::uint16_t>(value)};const auto raw=core.pool.Slot(candidate);if (!(raw[11]&3U) && raw[10]==type && (raw[40]&8U) && core.dependencies.Has(source,candidate)) { image=candidate;break; }
        }
        CHECK(image.value && session.FactoryUpgrading(source));if (!image.value) throw std::logic_error("UI 업그레이드의 실제 그림 없음");CHECK(std::uint32_t(core.pool.Slot(image)[11]>>5)==level+1 && (core.pool.Slot(image)[40]&9U)==9U);const auto x=Get(core.pool.Slot(source),14);CHECK(Get(core.pool.Slot(image),14)==x);
        scene.Open(source);const auto row=BuildingUiItem(scene,source,0x03000032U);CHECK(row.has_value());if (!row) throw std::logic_error("진행 중 UI 업그레이드 행 없음");const auto event=scene.Click(BuildingUiCommand(row->command,row->parameter));CHECK(scene.menu->Command(event));CHECK(!scene.factoryNotices.empty() && scene.factoryNotices.back()=="alreadyUpgrading");CloseBuildingUi(scene);
        scene.Open(source);const Sid popup{static_cast<std::uint16_t>(Get(core.pool.Slot(source),6,2))};CHECK(session.GumpForms().IsGumpForm(popup));core.processState.now=15;regular->RunFrame();CHECK(std::uint32_t(core.pool.Slot(source)[11]>>5)==level+1 && Get(core.pool.Slot(source),26)==static_cast<std::uint32_t>(session.HitPoints().MaxHitPoints(source)));CHECK(!session.FactoryUpgrading(source) && !core.processes.FindEvent(source,3) && !core.processes.FindEvent(source,4));CHECK((core.pool.Slot(image)[11]&1U) && !core.dependencies.Has(source,image));CHECK(session.GumpForms().IsGumpForm(popup) && !(core.pool.Slot(popup)[11]&1U));CloseBuildingUi(scene);
        scene.Open(source);CHECK(BuildingUiItem(scene,source,0x03000032U).has_value()==(level+1<3));CloseBuildingUi(scene);CHECK(session.Sp(session.LocalOwner())==money && core.pool.Slot(source)[34]==session.LocalOwner() && Get(core.pool.Slot(source),14)==x);std::printf("UPGRADE UI type=%u level=%u->%u completed, popup released\n",type,level,level+1); // 수신/비용 차감은 실행하지 않았으므로 동일 SP가 유지돼야 한다.
    }
}

TEST_CASE(StateGameUi_GameAndViewUseCurrentMissionEditorAndDemo) { // 동일 Session의 모드 변경을 실제 controlled 메뉴에서 재조회하고 게임 전환 명령은 실행하지 않는다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SpecialObjectUiScene scene(assets);auto& session=scene.world->Session();auto core=session.Core();
    for (const auto editor:{0U,1U}) for (const auto demo:{0U,1U}) for (const auto editing:{0U,1U}) { // 미션의 편집/데모/편집복귀 여덟 조합은 현재 공유 값으로만 입력한다.
        scene.client.GameplayGlobals()->world.editor=editor;scene.external.demo=demo;const auto free=core.pool.FreeCount();const auto kernel=core.kernel.Size();const auto game=OpenStateGameUi(scene,0,0,0,editing);CHECK(StateGameUiItem(scene,game,0x020000c2U).has_value()==(editor || editing));CHECK(StateGameUiItem(scene,game,0x02000091U).has_value()==(!editor && !demo));CHECK(StateGameUiItem(scene,game,0x0200008dU).has_value()==!editor);CHECK(StateGameUiItem(scene,game,0x02000094U).has_value());const auto quit=scene.Click("Quit Game");CHECK(quit.code==0x02000094U);CloseStateGameUi(scene);CHECK(!scene.ui.menus->Node(game));
        scene.client.Configuration().Set("theammode","0");scene.client.Configuration().Set("useBanners","1");const auto view=OpenStateGameUi(scene,1);CHECK(!StateGameUiItem(scene,view,0x02000088U).has_value());CHECK(StateGameUiItem(scene,view,0x020000a0U).has_value()==!editor);CHECK(StateGameUiItem(scene,view,0x020000b8U)->parameter==session.LocalOwner());const auto banner=StateGameUiItem(scene,view,0x0200006dU);CHECK(banner && scene.strings.Resolve(banner->parameter)=="useBanners=0" && (banner->flags&4U));const auto event=scene.Click("View Your Priest - F5");CHECK(event.code==0x020000b8U && std::bit_cast<std::uint32_t>(event.y)==session.LocalOwner());CloseStateGameUi(scene);CHECK(!scene.ui.menus->Node(view) && core.pool.FreeCount()==free && core.kernel.Size()==kernel);
    }
}

TEST_CASE(StateGameUi_OptionsCurrentConfigDeviceAndDisplayFlags) { // 실제 설정과 화면/음향 장치 경계가 체크·비활성·반전 인자로 이어지고 반납 뒤 현재 값을 다시 읽는지 확인한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SpecialObjectUiScene scene(assets);auto core=scene.world->Session().Core();
    for (const auto display:{0U,1U}) for (const auto ready:{0U,1U}) for (const auto enabled:{0U,1U}) { // 창/전체화면·음향 가용성·현재 설정의 여덟 조합을 실제 장치 실행 없이 공급한다.
        scene.client.Configuration().Set("sound",enabled ? "1" : "0");scene.client.Configuration().Set("music",enabled ? "1" : "0");scene.client.Configuration().Set("edgeScroll",enabled ? "1" : "0");const auto free=core.pool.FreeCount();const auto kernel=core.kernel.Size();const auto root=OpenStateGameUi(scene,2,display,ready);CHECK(StateGameUiItem(scene,root,0x02000030U).has_value()==(display!=0));CHECK(scene.ui.menus->Point(display ? "Adjust Direct Draw" : "Direct Draw / Full Screen").has_value());const auto sound=scene.ui.menus->Point("Sound On");CHECK(sound.has_value());if (!sound) throw std::logic_error("Options 실제 Sound 행 없음");scene.ui.Click(*sound);
        if (ready) { const auto event=scene.ui.commands.Pop(false);CHECK(event.code==0x0200006dU && scene.strings.Resolve(std::bit_cast<std::uint32_t>(event.y))==(enabled ? "sound=0" : "sound=1")); }else CHECK(scene.ui.commands.Empty());CloseStateGameUi(scene);CHECK(!scene.ui.menus->Node(root));const auto reopened=OpenStateGameUi(scene,2,display,ready);const auto edge=scene.Click("Edge Scroll in Fullscreen");CHECK(edge.code==0x0200006dU && scene.strings.Resolve(std::bit_cast<std::uint32_t>(edge.y))==(enabled ? "edgeScroll=0" : "edgeScroll=1"));CloseStateGameUi(scene);CHECK(!scene.ui.menus->Node(reopened) && core.pool.FreeCount()==free && core.kernel.Size()==kernel);
    }
}

TEST_CASE(StateGameBarUi_ActualChildrenTrackingAndAboutReleaseTogether) { // 전체 가로막대와 실제 여섯 하위·공유 SP 추적·등록 순번·About 입력/반납을 한 수명에서 확인한다.
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);SpecialObjectUiScene scene(assets);auto& session=scene.world->Session();auto core=session.Core();GameMenuBarState state(session.GameMenuTrackingOffset());
    for (const auto editor:{0U,1U}) for (const auto technical:{false,true}) { // 명시 편집/기술 도움말 가용성을 바꾸며 같은 Session/공통 등록표에서 네 번 재개방한다.
        scene.client.GameplayGlobals()->world.editor=editor;CHECK(scene.client.GameplayGlobals()->energy.userCheat==0);const auto tracking=state.checksum;const auto free=core.pool.FreeCount();const auto kernel=core.kernel.Size();std::array<std::uint32_t,9> money{};
        for (std::uint32_t owner=0;owner<9;++owner) money[owner]=std::bit_cast<std::uint32_t>(session.Sp(owner)); // 실제 메뉴 준비의 분산 SP 회전 전 값을 독립 보관한다.
        const auto root=OpenStateGameUi(scene,3,0,0,0,&state,technical);const auto* node=scene.ui.menus->Node(root);CHECK(node && node->parent==scene.ui.buttons.RootToken() && node->x==85 && node->baseWidth==555 && state.menuSerial==node->serial);CHECK(state.checksum==tracking+3 && &state.checksum==&session.GameMenuTrackingOffset());
        for (const auto label:{"Game","View","Options","Players","About"}) CHECK(scene.ui.menus->Point(label).has_value()); // 원문 상황에서 항상 작성하는 실제 다섯 가로 행을 확인한다.
        CHECK(scene.ui.menus->Point("Edit").has_value()==(editor!=0) && !scene.ui.menus->Point("Cheat").has_value());CHECK(scene.Click("About").code==0);FinishProductionUiExpansion(scene);CHECK(scene.ui.menus->Point("Technical Help").has_value()==technical);const auto version=scene.Click("Version");CHECK(version.code==0x02000095U && scene.strings.Resolve(std::bit_cast<std::uint32_t>(version.y))=="About");CloseStateGameUi(scene);scene.menu->Clear();CHECK(!scene.ui.menus->Node(root) && !scene.menu->Active() && core.pool.FreeCount()==free && core.kernel.Size()==kernel);
        for (std::uint32_t owner=0;owner<9;++owner) CHECK(std::bit_cast<std::uint32_t>(session.Sp(owner))==money[owner]); // 추적 검산/하위 Player 갱신이 게임 자금을 바꾸지 않아야 한다.
        std::printf("GAME BAR editor=%u technical=%u tracking=%d serial=%u released\n",editor,technical,state.checksum,state.menuSerial);
    }
}
