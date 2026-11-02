// 같은 실제 메뉴/Gump/QuickHelp/기본 HTML 파서가 지역파일 stream을 기록하는지 검사한다.
#include "GumpRawSupport.h"
#include "client/RawMenuControls.h"
#include "client/RawButtonControls.h"
#include "client/QuickHelpWindow.h"
#include "client/ScriptCommand.h"
#include "client/GameAssets.h" //실제 보호 TYPE/SHP/팔레트를 같은 공통 UI에 공급한다.
#include "client/BitmapFont.h" //같은 원본 CHFNT 저장소이며 GDI/창을 새로 실행하지 않는다.
#include "client/PaletteColors.h"
#include "client/HtmlConditionEffect.h"
#include "o/BaseFile.h"
#include "o/Config.h"
#include <filesystem>
#include <stdexcept>

using namespace netstorm::client; //실제 공통 등록표/파서/파일 부모를 직접 조립한다.
using namespace netstorm::o; //보호 자산/VFS는 읽기만 한다.
using namespace netstorm::test::gumpraw; //Native 입력/출력의 바이트 해독만 사용한다.
namespace { //게임/장치 실행 없이 현재 Client 경계만 공급하는 집중 검사다.
std::string SourceBytes(std::string_view hex) { const auto value=Bytes(std::string(hex));return {value.begin(),value.end()}; } //원문 CRLF/HTML을 그대로 공급한다.
//같은 원본 객체/글꼴과 실제 메뉴/버튼/Gump 수명을 소유한다.
struct HelpClient {
    std::filesystem::path root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals"; //읽기 전용 보호 원문이다.
    BaseFileSystem files{root}; //같은 실제 TAFF/VFS 공급자다.
    GameAssets assets;FontStore fonts;RawButtonControls buttons; //현재 실제 자산/글꼴/공통 등록표다.
    RawMenuControls* menus{};HtmlTextParserGlobals globals;ScriptStringRing strings; //같은 실제 메뉴/단락 전역/문자열 수명이다.
    ConfigInterface config; //조건 파서가 읽는 실제 Config다.
    const BaseFileSystem& Register() { files.RegisterArchive(root/"netstorm.tarc");return files; } //자산 생성 전에 보호 TAFF를 읽기만 한다.
    PaletteColorTable Colors() const { std::array<std::uint32_t,256> colors{};for (std::size_t i=0;i<colors.size();++i) { const auto c=assets.Palette().Color(static_cast<std::uint8_t>(i));colors[i]=c.red|(std::uint32_t(c.green)<<8)|(std::uint32_t(c.blue)<<16); }return BuildPaletteColorTable(colors,assets.Edition()); } //실제 팔레트의 같은논리 색이다.
    ~HelpClient() { menus->CloseHelp(); } //Config/문자열/전역을 반납하기 전에 같은 실제 Help/자식을 정리한다.
    explicit HelpClient(std::string source):assets(Register(),OriginalEdition::Patch1078),fonts(files,"Arial"),buttons(OriginalEdition::Patch1078),config([this](auto name) { return files.TryRead(name); }) { //창/오디오/게임 없이 같은 본체만 생성한다.
        RawMenuHooks menu;menu.command=[](auto,auto,auto) { throw std::logic_error("Help 목록 검사에서 새 메뉴 명령은 요청하지 않는다"); };menu.readInt=[](auto,auto& value) { value=0; };menu.sound=[](auto) {};menu.poll=[](auto) { return InputEvent{InputCode::kRelease,0,0}; };menu.cursor=[](auto) {};
        menus=&buttons.EnableMenus(assets,fonts,std::move(menu));menus->SetPalette(Colors());
        const auto& fort=assets.Find("fortGump");const auto frame=assets.FrameIndex(fort,"A00",0);const auto shape=assets.Shapes().SquidMetrics(fort.block,frame);const std::array<GumpControl,0> controls{};const std::array<std::string,0> labels{};
        buttons.SetControls(controls,labels,640,480,{static_cast<std::uint32_t>(fort.block+70),static_cast<std::uint32_t>(frame),shape.width,shape.height,ButtonColors(Colors())});
        RawHelpHooks help;help.globals=&globals;BindHtmlPictureText(help.html);config.SetInt("global.inMission",0);BindHtmlConditionConfig(help.html,config,assets.Edition());
        help.translate=help.substitute=[](auto text) { return std::string(text); };help.config=[](auto key) { return key=="Quick Help Spec" ? std::string("d/help.english") : std::string(key); };help.load=[source=std::move(source)](auto) -> std::optional<std::string> { return source; };
        help.text=[this](auto token) { return std::string(strings.Resolve(token&0x0fffffffU)); };help.storeText=[this](auto text) { return 0x70000000U|strings.Store(text); };help.lookup=[](auto) { return -1; };help.typeName=[](auto) { return std::string("Alpha"); };
        help.type=[](auto) { QuickHelpTypeInfo type;type.theme=1;type.flags1=0x10;type.range=23.75F;type.damage=3.25F;type.cost=17.75F;type.classification="Class Label";return type; }; //새 Native HTML profile1과 같은 독립 TYPE 경계이며 실제 월드 TYPE로 가장하지 않는다.
        help.description=[](auto) { return std::string("Unit Label"); };help.hitPoints=[](auto) { return 101; };help.helpFrame=[](auto) { return -1; }; //Native49fce0의 같은 profile1 현재값이다. 그림 생성은 선택하지 않는다.
        help.nearestColor=[](auto,auto,auto) { return 9U; };help.queue=[](auto,auto,auto) { throw std::logic_error("Help 목록 직접 Show는 이력/Back 큐를 만들지 않는다"); };help.script=help.external=[](auto) { throw std::logic_error("Help 목록 검사에서 script/외부 실행은 금지한다"); };help.learn=[](auto) { throw std::logic_error("Help 목록은 기술을 학습하지 않는다"); };
        help.scripted=help.battle=help.challenge=[] { return false; };help.scriptName=[] { return std::string{}; };menus->EnableHelp(std::move(help));
    }
};
//같은 실제 파서의 순서/byte 크기를 읽는 지역 BaseFile 장치다. 파일 부모 선택은 제품을 실행한다.
struct MemoryFile {
    bool live{},opened{},failStreaming{}; //현재 수명과 명시 첫 한BYTE stream 실패다.
    std::uint32_t closes{},destructs{},writes{},smallWrites{}; //정상/실패 경로의 실제 효과 횟수다.
    std::string bytes;const std::uint8_t* identity{}; //동일 지역24BYTE 주소와 기록한 원시 바이트다.
    QuickHelpListFileHooks Hooks() { QuickHelpListFileHooks h;
        h.construct=[this](auto file) { CHECK(!live);live=true;identity=file.data(); };
        h.open=[this](auto file,auto name,auto mode,auto create) { CHECK(live && file.data()==identity && name=="NetStormPieceList.html" && mode==1 && create==1);opened=true; };
        h.write=[this](auto file,auto text,auto offset) { CHECK(live && opened && file.data()==identity && offset==-1);++writes;bytes+=text;if (text.size()==1) { ++smallWrites;if (failStreaming) throw std::runtime_error("명시 파서 stream 실패"); } };
        h.close=[this](auto file) { CHECK(live && file.data()==identity && opened);opened=false;++closes; };
        h.destruct=[this](auto file) { CHECK(live && file.data()==identity);if (opened) { opened=false;++closes; }live=false;identity=nullptr;++destructs; };
        return h;
    } //호스트장치는 Construct부터 Destruct까지 같은 현재 FILE만 유지한다.
};
//합성 TYPE1을 포함하고0을 제외한다. 현재 TYPE 정보/본문/가상 Show/파서는 실제 처리기다.
QuickHelpListContext Context(HelpClient& client,MemoryFile& file,std::uint32_t& cleanup,std::uint32_t& tell) {
    QuickHelpListContext c;c.file=file.Hooks();c.typeCount=[] { return 2; };c.flags2=[](auto type) { return type==1 ? 1U : 0U; };c.typeName=[](auto) { return std::string("Alpha"); };
    c.cleanup=[&] { ++cleanup;client.buttons.CleanupGumps(); };c.tell=[&](auto section) { CHECK(section=="pieceDump" && !file.opened && file.live);++tell; };return c; //실제 Tell HTML 표시는 일반 부모가 공급하며 이 검사는 정확한 호출/수명만 읽는다.
}
}

TEST_CASE(QuickHelpListIntegration_NativeHtmlBytesAndDeferredKill) { //기존 원문 parser/실제 공통UI를 같은 출력 부모에 연결하여 스트림을 검증한다.
    const auto rows=LoadFixture(NETSTORM_QUICK_HELP_LIST_HTML_FIXTURE).rows;CHECK(rows.size()==3);
    for (const auto& row:rows) { //평문/태그/동적정보/링크/CRLF의 기대 바이트는 새 실제 PE 관측에서만 읽는다.
        CHECK(row.size()==3);if (row.size()!=3) continue;CHECK(row[2]=="101");HelpClient client(SourceBytes(row[0]));const auto token=client.menus->OpenHelp(0,0,0);auto* node=static_cast<QuickHelpContentNode*>(client.menus->Node(token));CHECK(node && !node->stream);if (!node) continue;
        const auto stringsBefore=client.strings.Index();MemoryFile file;std::uint32_t cleanup{},tell{};client.menus->CreateHelpList(Context(client,file,cleanup,tell));
        // 직접parser fixture뒤LF는실제4c5380의본문끝LF/NULstore다. 원문ctor1회/세parser추가6회로별도대조했다.
        CHECK(file.bytes=="<html>"+SourceBytes(row[1])+"\n<p></html>");CHECK(file.destructs==1 && file.closes==1 && !file.live && !file.opened && file.smallWrites>10 && cleanup==2 && tell==1);CHECK(client.strings.Index()==stringsBefore); //빈 주제 및 직접 Show는 문자열 고리 인자를 새로 만들지 않는다.
        CHECK(!node->stream && (node->flags&0x10000U) && !client.menus->HelpOpen() && client.menus->Node(token)); //Kill 뒤에도 같은 실제 등록/본문은 후반 Cleanup까지 남는다.
        CHECK(client.menus->Report().find("raw_help_body\t")!=std::string::npos);client.buttons.CleanupGumps();CHECK(!client.menus->Node(token));
    }
}

TEST_CASE(QuickHelpListIntegration_FailedStreamRetiresCallbackKeepsOriginalToken) { //실패의 원문 +bc 잔여값을 유지하며 반납된 지역 callback의 재실행만 거부한다.
    HelpClient client("<a name=\"AlphaType\">Text</a>\nSTOP");const auto token=client.menus->OpenHelp(0,0,0);MemoryFile file;file.failStreaming=true;std::uint32_t cleanup{},tell{};bool failed{};
    try { client.menus->CreateHelpList(Context(client,file,cleanup,tell)); }catch (const std::runtime_error&) { failed=true; }
    auto* node=static_cast<QuickHelpContentNode*>(client.menus->Node(token));CHECK(failed && node && node->stream && file.destructs==1 && file.closes==1 && !file.live && !tell && cleanup==1 && file.smallWrites==1);const auto writes=file.writes;
    bool stale{};try { client.menus->OpenHelp(1,0,0); }catch (const std::logic_error&) { stale=true; }CHECK(stale && file.writes==writes); //기존 Help의 다음 실제 파서 진입도 이미 반납한 파일에 쓰지 않는다.
    client.menus->CloseHelp();CHECK(!client.menus->Node(token));
}
