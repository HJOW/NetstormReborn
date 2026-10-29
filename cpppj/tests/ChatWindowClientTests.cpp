// 실제 Client 전역/명령 표와 전체 Chat 공장·공통 자식·공유 로그·픽셀·수명을 검증한다.
#include "DudeSpawnSupport.h"
#include "client/ClientMain.h"
#include "client/ClientChatWindowGlobals.h"
#include "client/ClientChatManagerGlobals.h"
#include "client/ClientChatRuntime.h"
#include "client/ClientCoreDataGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientPlayerRecords.h"
#include "client/ClientDotCommandGlobals.h"
#include "client/RawChatWindowContext.h"
#include "client/RawButtonControls.h"
#include "client/RawMenuControls.h"
#include "client/RawTextEntryControls.h"
#include "client/RawSurfaceWorld.h"
#include "client/TextEntryStorage.h"
#include "client/ScriptCommand.h"
#include "client/PaletteShade.h"
#include "client/TraceChat.h" // 같은 실제 UI의 비영 진단 전달도 공유 로그/문맥/스크롤을 사용한다.
#include "platform/Bitmap.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <set>
#include <sstream>
using namespace netstorm::client;
namespace { // 실제 조립과 명시 외부 장치의 입력/관측만 모은다.
ClientOptions ChatOptions() { ClientOptions value;value.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";value.noAudio=true;return value; } // Run/창 없이 Client 소유자를 만든다.
std::uint32_t ChatDword(std::span<const std::uint8_t> bytes,std::size_t offset) { std::uint32_t value{};std::memcpy(&value,bytes.data()+offset,4);return value; } // 실제 인접 표의 같은 네BYTE를 읽는다.
struct ChatScene { // 모든 Client/글꼴/공통 UI 소비자의 실제 수명을 유지한다.
    const netstorm::test::dudespawn::DudeAssets& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);Client client{ChatOptions()}; // 보호 파일과 실제 전역이다.
    FontStore fonts{assets.files,"Arial"};std::shared_ptr<RawSurfaceStorage> storage=MakeRawSurfaceStorage(assets.game);ScriptStringRing strings; // 같은 실제 자산/물리와 입력 고리다.
    std::array<std::uint32_t,256> logical{};PaletteColorTable palette;double seconds{};bool failTranslation{},battle{true},validFile{true}; // 실제 팔레트와 명시 외부 장치 입력이다.
    std::uint32_t quitCalls{},validCalls{};std::vector<std::string> writes; // 원문 파일/종료 장치의 순수 관측이다.
    RawButtonControls buttons{OriginalEdition::Patch1078,{[this] { return seconds; }}};RawMenuControls* menus{}; // 같은 실제 공통 등록표/수명이다.
    ChatScene() { // 창과 소리 장치 없이 실제 자산 기반 공통 UI를 만든다.
        for (std::size_t index=0;index<logical.size();++index) { const auto c=assets.game.Palette().Color(static_cast<std::uint8_t>(index));logical[index]=c.red|(std::uint32_t(c.green)<<8)|(std::uint32_t(c.blue)<<16); } // 원문 채널 순서다.
        palette=BuildPaletteColorTable(logical);RawMenuHooks hooks;hooks.command=[](auto,auto,auto) {};hooks.readInt=[](auto,std::int32_t& value) { value=0; };hooks.sound=[](auto) {};hooks.poll=[](auto) { return InputEvent{InputCode::kRelease,0,0}; };hooks.cursor=[](auto) {};
        menus=&buttons.EnableMenus(assets.game,fonts,std::move(hooks));menus->SetPalette(palette);Resize();
    }
    void Resize() { const auto& asset=assets.game.Find("fortGump");const auto frame=assets.game.FrameIndex(asset,"A01",0);const auto size=assets.game.Shapes().SquidMetrics(asset.block,frame);buttons.SetControls({}, {},640,480,{static_cast<std::uint32_t>(asset.block+70),static_cast<std::uint32_t>(frame),size.width,size.height,{}}); } // 실제 페이지 수명이다.
    RawChatWindowContext Context() { // 같은 Client 전역과 실제 현재 물리에 경계를 연결한다.
        RawChatWindowContext value;value.globals=client.ChatWindowGlobals();value.commands=client.DotCommandGlobals();value.editor.now=[this] { return seconds; };value.editor.recipient=[](auto text) { return text=="EVERYONE"; };value.recipient=value.editor.recipient;
        value.read=[this](auto address) { switch (address) { case 0x542944:return 0x02000094U;case 0x594fc8:return std::uint32_t(storage->ownerMode.challenge);case 0x594fbc:return std::uint32_t(battle);case 0x548714:return client.ChatManagerGlobals()->Current();case 0x5cae94:return 25U;case 0x5cae98:return 35U;case 0x563e38:return buttons.RootToken();default:throw std::logic_error("검사의 예상 밖 Chat 전역 조회"); } };
        value.translate=[this](auto text) { if (failTranslation && text=="Chat Helper") throw std::runtime_error("명시 번역 장치 중단");return std::string(text); };value.storeText=[this](auto text) { return 0x70000000U|strings.Store(text); };value.text=[this](auto token) { return strings.Resolve(token&0x0fffffffU); };value.config=[](auto) { return 0; };
        value.fileValid=[this](auto token) { CHECK(token==123);++validCalls;return validFile; };value.writeLog=[this](auto token,auto bytes) { CHECK(token==123);writes.emplace_back(bytes.begin(),bytes.end()); };value.quit=[this](auto event,auto first,auto second) { CHECK(event==0x02000094U && first==0 && second==0);++quitCalls; };return value;
    }
    std::uint32_t Create() { return menus->CreateChatWindow(Context()); } // 원문 전체 공장을 실제 자식에 실행한다.
    HudChatNode& Root(std::uint32_t token) { return *static_cast<HudChatNode*>(menus->Node(token)); } // 표시 복사본을 만들지 않는다.
    ConsoleGumpNode& Console(std::uint32_t root) { return *menus->TextEntryControls()->Console(Root(root).console); } // 같은 실제 Console이다.
    TextEntryBuffer& Input(std::uint32_t root) { return *menus->TextEntryControls()->Storage().Find(Console(root).buffer); } // 같은 편집 물리다.
    std::uint32_t Count() const { std::istringstream input(buttons.Report());std::string name;std::uint32_t controls{},count{};input>>name>>controls>>count;CHECK(name=="RawButtons");return count; } // 실제 현재 등록 수만 읽는다.
    std::vector<std::uint8_t> Draw(std::optional<ScreenRect> clip={}) { std::vector<std::uint8_t> pixels(647*480,0x5a);menus->Draw(pixels,647,480,palette,BuildGumpShadeMaps(logical),clip);return pixels; } // 같은 실제 수집/정렬/배경/글리프다.
};
}

// 실제 네 자식/공유 로그/별도 편집 버퍼/세 Slider와 같은 인접 명령 표의 최초 등록을 검사한다.
TEST_CASE(ChatWindowClient_FullFactoryActualChildrenClientLogAndCommands) {
    ChatScene scene;const auto before=scene.Count();auto globals=scene.client.ChatWindowGlobals();CHECK(globals->window.current==0 && globals->window.registered==0 && !globals->log.initialized);CHECK(ChatDword(scene.client.DotCommandGlobals()->Bytes(),0xa04)==1);
    const auto token=scene.Create();auto& root=scene.Root(token);auto& console=scene.Console(token);const auto* log=static_cast<ScrollingTextGumpNode*>(scene.menus->Node(root.text));
    CHECK(scene.Count()==before+24 && root.vtable==0x504a94 && root.command==9 && root.capacity==128 && root.helperVisible==1 && root.baseWidth==200 && root.baseHeight==100 && scene.menus->ChatWindowRoot()==token);
    CHECK(scene.menus->Node(root.helper)->parent==token && scene.menus->Node(root.console)->parent==token && scene.menus->Node(root.background)->parent==token && log->parent==root.background);
    CHECK(log->buffer==0x548758 && log->ownsBuffer==0 && log->callback==1 && log->slider && globals->log.initialized && globals->log.maxLines==128 && globals->uiOwner);
    CHECK(console.maxLength==80 && console.autocomplete==0x42e540 && console.changeNotice==1 && console.multiline==0 && !(console.flags&1) && console.ownsBuffer && console.verticalSlider && console.horizontalSlider && scene.Input(token).Text().empty());
    CHECK(globals->window.registered==1 && globals->window.completionReady==1 && globals->window.colors.size()==256 && globals->window.backgrounds.size()==256 && !globals->window.icons.empty() && scene.strings.Index()==0);
    const auto commands=scene.client.DotCommandGlobals()->Bytes();CHECK(ChatDword(commands,0xa04)==3 && ChatDword(commands,0x804)==0x42f680 && ChatDword(commands,0x808)==0x42e520);CHECK(std::string(reinterpret_cast<const char*>(commands.data()+32))=="clear" && std::string(reinterpret_cast<const char*>(commands.data()+64))=="quit");
    scene.menus->CloseChatWindows();CHECK(scene.Count()==before && globals->window.current==0 && globals->log.initialized && scene.menus->TextEntryControls()->Storage().BufferCount()==0);
}
//실제 옵션 DWORD·현재 Console·공통 입력 경로가 한 버퍼를 유지하며 표시만 전환하는지 검사한다.
TEST_CASE(ChatWindowClient_EnterTypingToggleAndCurrentOptionRequest) {
    ChatScene scene;auto globals=scene.client.ChatWindowGlobals();CHECK(!globals->inputLineOff);scene.client.Configuration().SetInt("inputLineOff",-1);CHECK(globals->inputLineOff==-1);
    ConsumeChatInputToggle(globals->inputLineOff,[&] { scene.menus->ToggleChatInput(); });CHECK(!globals->inputLineOff);scene.client.Configuration().SetInt("tracelevel",4);CHECK(globals->inputLineOff==-1);ConsumeChatInputToggle(globals->inputLineOff,[&] { scene.menus->ToggleChatInput(); });scene.client.Configuration().SetInt("inputLineOff",0); //원문처럼 설정값은 남고 다른 옵션의 재해석 때 요청이 다시 읽힌다.
    const auto token=scene.Create();scene.menus->Attach(token,10,200,scene.buttons.RootToken(),0);CHECK(!scene.menus->ChatGeometryBackend()->InputVisible());CHECK(!scene.menus->OpenChatInput({0x8000000dU,0,0}));CHECK(scene.menus->OpenChatInput({13,0,0}));CHECK(scene.menus->ChatGeometryBackend()->InputVisible()==1);
    for (const auto byte:std::string_view("EVERYONE: hello")) static_cast<void>(scene.buttons.RouteInput({std::uint32_t(static_cast<unsigned char>(byte)),0,0})); //일반 Gump 큐의 문자 사건을 같은 실제 Console로 보낸다.
    CHECK(scene.Input(token).Text()=="EVERYONE: hello");scene.menus->ToggleChatInput();CHECK(!scene.menus->ChatGeometryBackend()->InputVisible() && scene.Input(token).Text()=="EVERYONE: hello");CHECK(scene.menus->OpenChatInput({13,0,0}));CHECK(scene.Input(token).Text()=="EVERYONE: hello");
    scene.menus->CloseChatWindows();CHECK(!globals->window.current);scene.menus->ToggleChatInput();
}
//먼저만든TextEntry에도실제Client완성을연결하고Tab/Shift+Tab과단어경계를같은버퍼에서실행한다.
TEST_CASE(ChatWindowClient_TabLateBindingRealCyclesAndPersistentClientState) {
    ChatScene scene;RawTextEntryContext early;early.now=[&] { return scene.seconds; };early.complete=[](auto,auto,auto,bool)->std::optional<std::string> { throw std::logic_error("교체되어야할이전완성장치"); };auto* editor=&scene.menus->EnableTextEntry(std::move(early));auto context=scene.Context();BindClientChatRecipients(context,scene.client,scene.storage,[] { return std::string_view("Local"); });const auto callback=context.editor.complete;
    const auto token=scene.menus->CreateChatWindow(std::move(context));scene.menus->Attach(token,10,200,scene.buttons.RootToken(),0);CHECK(scene.menus->TextEntryControls()==editor);CHECK(scene.menus->OpenChatInput({13,0,0}));
    const auto key=[&](std::uint32_t code) { static_cast<void>(scene.buttons.RouteInput({code,0,0})); }; //공통Gump입력에서실제Console과선택/단어상태를거친다.
    key(9);CHECK(scene.Input(token).Text()=="BATTLE");key(9);CHECK(scene.Input(token).Text()=="EVERYONE");key(9|InputCode::kShift);CHECK(scene.Input(token).Text()=="BATTLE");key(9|InputCode::kShift);CHECK(scene.Input(token).Text()=="OBSERVER");key(':');key(9);CHECK(scene.Input(token).Text()=="OBSERVER:BATTLE");key(' ');key('a');key('l');key(9);CHECK(scene.Input(token).Text()=="OBSERVER:BATTLE ALLIES");
    const auto saved=scene.client.ChatWindowGlobals()->completion.bytes;scene.menus->CloseChatWindows();const auto replacement=scene.Create();CHECK(replacement!=token && scene.client.ChatWindowGlobals()->completion.bytes==saved);CHECK(callback(0x42e540,std::nullopt,999,false)=="ALLIES");scene.menus->CloseChatWindows();
}

// 기존/새 창을 동시에 실제 생성한 뒤 원문 무조건 current0/공유 로그 보존/개별 반납 순서를 검사한다.
TEST_CASE(ChatWindowClient_ConcurrentRootsSharedLogAndNativeCurrentRetirement) {
    ChatScene scene;const auto before=scene.Count(),first=scene.Create();auto globals=scene.client.ChatWindowGlobals();scene.menus->AppendChatLog(first,"First\n");const auto* lines=globals->log.hardLines.data();const auto second=scene.Create();CHECK(scene.Count()==before+48 && first!=second && globals->window.current==second && globals->log.lineCount==1 && globals->log.hardLines.data()==lines);
    scene.menus->AppendChatLog(second,"Second\n");scene.buttons.KillGump(first);scene.buttons.CleanupGumps();CHECK(!scene.menus->Node(first) && scene.menus->Node(second) && globals->window.current==0 && !scene.menus->ChatWindowRoot() && globals->log.lineCount==2 && scene.Count()==before+24);
    CHECK(scene.menus->InvokeChatCommand(0x42f680)==0 && globals->log.lineCount==2);const auto third=scene.Create();CHECK(globals->window.current==third && globals->log.hardLines.data()==lines && globals->log.lineCount==2 && ChatDword(scene.client.DotCommandGlobals()->Bytes(),0xa04)==3);
    scene.menus->CloseChatWindows();CHECK(scene.Count()==before && globals->log.lineCount==2);const auto fourth=scene.Create();CHECK(fourth!=third && globals->log.lineCount==2);scene.Resize();CHECK(scene.Count()==before && !scene.menus->ChatWindowRoot() && globals->log.initialized && globals->log.lineCount==2);
}

// 실제 append 파일 callback과 원문 clear의 미완료 끝 줄 보존·quit 예약을 검사한다.
TEST_CASE(ChatWindowClient_ActualLogCallbackClearTailAndQuitBoundary) {
    ChatScene scene;const auto token=scene.Create();auto globals=scene.client.ChatWindowGlobals();globals->logFile=123;scene.validFile=false;scene.menus->AppendChatLog(token,"First\n");CHECK(scene.validCalls>0 && scene.writes.empty() && globals->log.lineCount==1);
    scene.validFile=true;scene.menus->AppendChatLog(token,"Second\n");CHECK(!scene.writes.empty() && globals->log.lineCount==2);globals->logFile=0;const auto calls=scene.validCalls;scene.menus->AppendChatLog(token,"tail");CHECK(scene.validCalls==calls && globals->log.lineCount==2 && globals->log.hardLines[2]);
    scene.menus->InvokeChatCommand(0x42f680);const auto* text=static_cast<ScrollingTextGumpNode*>(scene.menus->Node(scene.Root(token).text));CHECK(globals->log.lineCount==0 && globals->log.hardLines[0] && text->scrollRequest==-2);scene.menus->AppendChatLog(token," done\n");CHECK(globals->log.lineCount==1);CHECK(scene.menus->InvokeChatCommand(0x42e520)==0 && scene.quitCalls==1);scene.menus->CloseChatWindows();
}

// 전체 helper의 수백 실제 인자는80칸 입력 고리로 덮이지 않아야 하며 현재 삽입 입력도 같은 바이트를 써야 한다.
TEST_CASE(ChatWindowClient_ActualHelperMenusPreserveStablePayloadsBeyondInputRing) {
    ChatScene scene;const auto token=scene.Create();auto globals=scene.client.ChatWindowGlobals();scene.menus->Attach(token,10,10,scene.buttons.RootToken(),0);const auto serial=scene.menus->Node(scene.Root(token).helper)->serial;
    scene.buttons.HandleGump(token,0x0200002c,serial,0);CHECK(scene.menus->HasMenus() && scene.strings.Index()==0);std::set<std::uint32_t> payloads;std::uint32_t firstColor{},nullPayloads{};
    for (std::uint32_t id=1;id<1000;++id) if (const auto* base=scene.menus->Node(scene.buttons.RegisteredGump(id));base && base->vtable==0x50c6f0) { const auto& menu=*static_cast<const MenuGumpNode*>(base);for (std::uint32_t index=0;index<menu.itemCount;++index) { const auto& item=menu.items[index];if (item.command==0x02000115 && item.parameter) { payloads.insert(item.parameter);CHECK(globals->strings.contains(item.parameter));if (globals->strings.at(item.parameter)=="~[C.0]") firstColor=item.parameter; }if ((item.command==0x02000105 || item.command==0x02000106) && item.parameter==0) ++nullPayloads; } } // 현재 실제 메뉴 항목과 그 원시 포인터만 읽는다.
    CHECK(payloads.size()>500 && firstColor && nullPayloads==2);for (int i=0;i<100;++i) scene.strings.Store("unrelated input");CHECK(globals->strings.at(firstColor)=="~[C.0]"); // 임시 입력 고리가 원문 벡터/정적 포인터를 바꾸지 않는다.
    scene.buttons.HandleGump(token,0x02000115,0,firstColor);CHECK(scene.Input(token).Text()=="~[C.0]");scene.menus->Close();scene.menus->CloseChatWindows();
}

// 원문 일반 부착·입력 표시 전환·실제 회색 배경/스타일 로그/선택·클립을 검사한다.
TEST_CASE(ChatWindowClient_RealGeometryInputAndStyledPixels) {
    ChatScene scene;const auto token=scene.Create();auto& root=scene.Root(token);root.baseWidth=root.width=420;root.baseHeight=root.height=110;scene.menus->Attach(token,30,200,scene.buttons.RootToken(),0);scene.menus->ChatGeometryBackend()->SetInputVisible(token,1);
    scene.menus->TextEntryControls()->Backend().SetText(root.console,std::span("EVERYONE: hello",16));scene.Input(token).Select(10,15);scene.menus->AppendChatLog(token,"~rRed ~gGreen ~wLog\n");CHECK(scene.Console(token).flags&1);const auto pixels=scene.Draw();const ScreenRect clip{35,205,135,225};const auto clipped=scene.Draw(clip);std::size_t changed{};
    for (int y=0;y<480;++y) for (int x=0;x<647;++x) { const auto value=clipped[y*647+x];if (x>=clip.left && x<clip.right && y>=clip.top && y<clip.bottom) changed+=value!=0x5a;else CHECK(value==0x5a); } // 실제 현재 클립과 행 padding 밖은 보존한다.
    CHECK(changed>0);if (const auto* output=std::getenv("NETSTORM_CHAT_QA")) { std::vector<std::uint8_t> rgba(640*480*4);for (int y=0;y<480;++y) for (int x=0;x<640;++x) { const auto color=scene.assets.game.Palette().Color(pixels[y*647+x]);const auto at=(y*640+x)*4;rgba[at]=color.red;rgba[at+1]=color.green;rgba[at+2]=color.blue;rgba[at+3]=255; }netstorm::platform::WriteBitmap(output,640,480,rgba); } // 명시 검사 경로에만 같은 실제 표면을 기록한다.
    scene.menus->CloseChatWindows();
}

namespace { //실제Client장치를사용하는추가원문대조의입력준비만공유한다.
//원문fixture의좁은바이트입력을그대로복원한다.
std::string ChatInputBytes(std::string_view text) {
    if (text=="-") return {};std::string result;
    for (std::size_t index=0;index<text.size();index+=2) result+=static_cast<char>(std::stoul(std::string(text.substr(index,2)),nullptr,16)); //두hex문자를한BYTE로읽는다.
    return result;
}
//실제필드의정확한DWORD입력만쓴다.
void ChatIdentityPut(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) { std::memcpy(bytes.data()+at,&value,4); }
//조회에사용하는이름영역과Subscriber만원문과같은명시입력으로만든다.
void ChatIdentityInputs(ChatScene& scene) {
    auto records=scene.client.PlayerRecords();records->Initialize([](auto,auto) {}); //동맹자식은이이름조회검사의명시외부장치이며조회/기대값을계산하지않는다.
    auto bytes=records->Records();
    for (std::size_t owner=0;owner<9;++owner) { //원문과같은9행의이름/Subscriber입력을쓰며factory/다른필드는보존한다.
        std::fill_n(bytes.begin()+owner*180+12,32,std::uint8_t{});ChatIdentityPut(bytes,owner*180+44,0);
    }
    const auto text=[](std::span<std::uint8_t> target,std::size_t at,std::string_view value) { std::copy(value.begin(),value.end(),target.begin()+at);target[at+value.size()]=0; }; //입력의NUL까지만쓴다.
    text(bytes,180+12,"LocalUser");ChatIdentityPut(bytes,180+44,100);text(bytes,360+12,"Alice");ChatIdentityPut(bytes,360+44,200);
    const auto view=scene.client.ChatRecords();std::span<std::uint8_t> chat{const_cast<std::uint8_t*>(view.data()),view.size()}; //검사만실제mutable소유자의읽기뷰에원문과같은40BYTE/flags0경계입력을쓴다.제품은읽기전용API를유지한다.
    std::fill(chat.begin(),chat.end(),std::uint8_t{});text(chat,0,"Bob");ChatIdentityPut(chat,44,300);text(chat,48,std::string(40,'B'));ChatIdentityPut(chat,48+44,400);
    scene.client.GameplayGlobals()->player.battleOwner=1;scene.client.GameplayGlobals()->world.mySubscriber=100;scene.client.GameplayGlobals()->LocalAddress()=0;
}
//동일한실제ConsoleAPI로입력을교체하며별도현재입력사본을제품에주입하지않는다.
void ChatSetInput(ChatScene& scene,std::uint32_t token,std::string_view text) {
    std::vector<char> bytes(text.begin(),text.end());bytes.push_back(0);scene.menus->TextEntryControls()->Backend().SetText(scene.Root(token).console,bytes);
}
}

//원문720행을실제Client의현재Identity/대상검사와공통ChatWindow/편집버퍼에대조한다.
TEST_CASE(ChatWindowClient_ActualRecipientInputNative720) {
    ChatScene scene;ChatIdentityInputs(scene);std::string currentName="LocalUser";auto context=scene.Context();BindClientChatRecipients(context,scene.client,scene.storage,[&]() -> std::string_view { return currentName; });const auto resolve=context.resolve;const auto token=scene.menus->CreateChatWindow(std::move(context));
    std::ifstream input(NETSTORM_CHAT_INPUT_RECIPIENTS_FIXTURE);if (!input) throw std::runtime_error("실제Chat입력원문관측없음");std::string line;std::getline(input,line);std::size_t count{};
    while (std::getline(input,line)) { //원문각행의입력만실제같은장치에공급하고최종반환을고정기대값과비교한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();std::vector<std::string> row;std::size_t start{};
        for (;;) { const auto end=line.find('\t',start);row.push_back(line.substr(start,end==line.npos ? line.npos : end-start));if (end==line.npos) break;start=end+1; } //빈열도보존한다.
        if (row.size()!=10) throw std::runtime_error("실제Chat입력관측열수오류");const auto mode=std::stoul(row[1]);scene.storage->ownerMode.battle=mode==1;scene.client.GameplayGlobals()->world.editor=mode==2;scene.client.ChatManagerGlobals()->Current()=static_cast<std::uint32_t>(std::stoul(row[2]));ChatSetInput(scene,token,ChatInputBytes(row[3]));const std::string before(scene.Input(token).Text());bool result{};
        if (row[0]=="contains") result=scene.menus->ChatContainsRecipient(ChatInputBytes(row[4]));
        else { netstorm::o::PlayerIdentityRecord record;record.fill(0xa5);netstorm::o::RawPlayerIdentity::Initialize(record,0);ChatIdentityPut(record,4,static_cast<std::uint32_t>(std::stoul(row[5])));result=resolve(record) && scene.menus->ChatContainsRecipient({reinterpret_cast<const char*>(record.data()+8)}); } //42f640의조회실패시입력복사를선행하지않는같은사슬이다.
        CHECK(std::to_string(result)==row[6]);CHECK(scene.Input(token).Text()==before);++count;
    }CHECK(count==720);scene.menus->CloseChatWindows();
}

//먼저만든Console의수명은유지하면서실제대상검사/현재이름·주소·Playerbase변경을따라야한다.
TEST_CASE(ChatWindowClient_RecipientLateBindingCurrentNameAddressAndPlayers) {
    ChatScene scene;ChatIdentityInputs(scene);std::string currentName="LocalUser";RawTextEntryContext early;early.now=[&] { return scene.seconds; };auto& entries=scene.menus->EnableTextEntry(early);const auto previous=entries.ConstructConsole(200,20,"kept",5,0,0);const auto buffer=entries.Console(previous)->buffer;
    auto context=scene.Context();BindClientChatRecipients(context,scene.client,scene.storage,[&]() -> std::string_view { return currentName; });const auto valid=context.recipient;const auto resolve=context.resolve;const auto token=scene.menus->CreateChatWindow(std::move(context));scene.client.ChatManagerGlobals()->Current()=1;scene.storage->ownerMode.battle=false;scene.client.GameplayGlobals()->world.editor=0;
    CHECK(scene.menus->TextEntryControls()==&entries && entries.Console(previous)->buffer==buffer && entries.Storage().Find(buffer)->Text()=="kept");CHECK(valid("LocalUser ALLIES Bob") && !valid("Alice") && !valid(std::string(40,'A')));
    currentName="Renamed";CHECK(valid("Renamed") && !valid("LocalUser"));scene.client.GameplayGlobals()->LocalAddress()=0x81234567U;netstorm::o::PlayerIdentityRecord record{};netstorm::o::RawPlayerIdentity::Initialize(record,0);ChatIdentityPut(record,48,0x81234567U);CHECK(resolve(record) && ChatDword(record,48)==0x81234567U && std::string(reinterpret_cast<const char*>(record.data()+8))=="Renamed");
    scene.menus->Attach(token,10,200,scene.buttons.RootToken(),0);scene.menus->ChatGeometryBackend()->SetInputVisible(token,1);ChatSetInput(scene,token,"Renamed: hello");CHECK(!scene.Draw().empty()); //실제Console Draw의recipientHighlight가먼저만든편집처리기의현재검사까지통과해야한다.
    const auto oldBase=scene.client.PlayerRecords()->Base();scene.client.PlayerRecords()->Initialize([](auto,auto) {});const auto players=scene.client.CoreDataGlobals()->Players();CHECK(scene.client.PlayerRecords()->Base()!=oldBase);constexpr std::string_view changed="Changed";std::copy(changed.begin(),changed.end(),players.begin()+360+12);players[360+12+changed.size()]=0;ChatIdentityPut(players,360+44,222);scene.client.GameplayGlobals()->world.editor=1;CHECK(valid("Changed"));scene.menus->CloseChatWindows();scene.menus->CloseTextEntries();
}

// 번역 중단과 선택된 전송/자동완성 미연결은 부분 효과를 유지한 명시 실패이며 가짜 성공이 아니다.
TEST_CASE(ChatWindowClient_PartialCreationAndSelectedUnboundDevicesAreExplicit) {
    ChatScene scene;const auto before=scene.Count();scene.failTranslation=true;bool rejected{};try { scene.Create(); }catch (const std::runtime_error&) { rejected=true; }CHECK(rejected && scene.Count()>before && scene.client.ChatWindowGlobals()->window.current==0);scene.menus->CloseChatWindows();CHECK(scene.Count()==before);
    scene.failTranslation=false;const auto token=scene.Create();scene.menus->TextEntryControls()->Backend().SetText(scene.Root(token).console,std::span("hello",6));rejected=false;try { scene.buttons.HandleGump(token,0x02000038,scene.Console(token).serial,0); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected && scene.Input(token).Text()=="hello\n");scene.Input(token).Select(6,6);
    rejected=false;try { scene.buttons.HandleGump(scene.Root(token).console,9,0,0); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected);scene.menus->CloseChatWindows();CHECK(scene.Count()==before && scene.menus->TextEntryControls()->Storage().BufferCount()==0);
}

// 비영 진단 관문과 현재색/공유 로그/파서/스크롤·부모 문맥 복원을 실제 UI에 함께 실행한다.
TEST_CASE(ChatWindowClient_TraceRouteActualLogContextAndScrollPreservation) {
    ChatScene scene;const auto token=scene.Create();auto& root=scene.Root(token);auto& text=*static_cast<ScrollingTextGumpNode*>(scene.menus->Node(root.text));const auto saved=text.textContext; // 같은 실제 부모/자식 물리다.
    root.textContext.fill(0x12345678);text.scroll=-1000000000;text.scrollRequest=12345;auto globals=scene.client.ChatWindowGlobals(); // 맨 아래가 아닌 명시 실제 스크롤 입력이다.
    scene.menus->ForwardDiagnosticChat(0,"ignored\n");CHECK(globals->log.lineCount==0 && root.textContext[0]==0x12345678);
    scene.menus->ForwardDiagnosticChat(1,"100%% trace\n");CHECK(globals->log.lineCount==1 && root.textContext==saved && text.textContext==saved && text.scrollRequest==12345);
    text.scroll=1000000000;text.scrollRequest=7;scene.menus->ForwardDiagnosticChat(0xffffffffU,"second\n");CHECK(globals->log.lineCount==2 && text.scrollRequest==-2 && root.textContext==saved);
    scene.client.ChatWindowGlobals()->window.current=0;scene.menus->ForwardDiagnosticChat(1,"no current\n");CHECK(globals->log.lineCount==2);scene.menus->CloseChatWindows();
}

// 원문 세 callback29행과 실제 Client/로그/자식/예약 장치를 같은 독립 입력에서 대조한다.
TEST_CASE(ChatWindowClient_IndependentNativeCallbackMutationsAndCurrentReads) {
    std::ifstream input(NETSTORM_CHAT_CALLBACKS_FIXTURE);CHECK(input.good());std::string line;std::getline(input,line);std::size_t rows{};
    const auto hex=[](std::span<const char> bytes) { std::string result;const char digits[]="0123456789abcdef";for (const auto byte:bytes) { const auto value=static_cast<unsigned char>(byte);result+=digits[value>>4];result+=digits[value&15]; }return result; }; // 실제 장치 바이트의 소문자16진 관측이다.
    while (std::getline(input,line)) { // 기대 결과를 계산하지 않고 각 원문 입력으로 새 실제 Client/공장을 만든다.
        if (!line.empty() && line.back()=='\r') line.pop_back();std::vector<std::string> fields;std::size_t begin{};
        for (;;) { const auto end=line.find('\t',begin);fields.push_back(line.substr(begin,end==line.npos ? line.npos : end-begin));if (end==line.npos) break;begin=end+1; } // 빈 문자열 입력 열도 보존한다.
        CHECK(fields.size()==12);if (fields.size()!=12) continue;ChatScene scene;auto globals=scene.client.ChatWindowGlobals();auto context=scene.Context();std::vector<std::string> calls;const auto originalRead=context.read;
        context.read=[&](auto address) { return address==0x542944U ? static_cast<std::uint32_t>(std::stoull(fields[6])) : originalRead(address); };
        context.fileValid=[&](auto token) { calls.push_back("valid:"+std::to_string(token));if (fields[4]=="fileZero") globals->logFile=0;if (fields[4]=="fileNext") globals->logFile=2;return fields[3]=="1"; };
        context.writeLog=[&](auto token,auto bytes) { calls.push_back("write:"+std::to_string(token)+':'+hex(bytes)); };
        context.quit=[&](auto event,auto first,auto second) { calls.push_back("queue:"+std::to_string(event)+':'+std::to_string(first)+':'+std::to_string(second)); };
        const auto first=scene.menus->CreateChatWindow(std::move(context)),second=scene.Create();const auto textA=scene.Root(first).text,textB=scene.Root(second).text;auto* a=static_cast<ScrollingTextGumpNode*>(scene.menus->Node(textA));auto* b=static_cast<ScrollingTextGumpNode*>(scene.menus->Node(textB));a->scrollRequest=b->scrollRequest=-1;a->textFlags&=~1U;b->textFlags&=~1U;
        globals->window.current=fields[1]=="1" ? first : 0;globals->logFile=static_cast<std::uint32_t>(std::stoul(fields[2]));std::uint32_t result{};
        if (fields[0]=="clear") {
            globals->log.onAppend=[&](auto bytes) { CHECK(!bytes.empty() && bytes.front()==0);calls.push_back("replace");if (fields[4]=="textNext" || fields[4]=="textNextCurrentZero") scene.Root(first).text=textB;if (fields[4]=="currentZero" || fields[4]=="textNextCurrentZero") globals->window.current=0;if (fields[4]=="currentNext") globals->window.current=second; };
            result=scene.menus->InvokeChatCommand(0x42f680);if (!calls.empty()) { CHECK(calls.size()==1 && bool(a->textFlags&1U)!=bool(b->textFlags&1U));calls.front()+=a->textFlags&1U ? ":1" : ":2"; }if (a->scrollRequest==-2) calls.push_back("bottom:1");if (b->scrollRequest==-2) calls.push_back("bottom:2");
        } else if (fields[0]=="log") {
            std::vector<char> bytes;for (std::size_t index=0;index<fields[5].size();index+=2) bytes.push_back(static_cast<char>(std::stoul(fields[5].substr(index,2),nullptr,16)));bytes.push_back(0);globals->log.onAppend(bytes); // 같은 실제 파일 callback에 NUL 뒤 BYTE도 입력한다.
        } else { CHECK(fields[0]=="quit");result=scene.menus->InvokeChatCommand(0x42e520); }
        std::string trace;for (const auto& call:calls) { if (!trace.empty()) trace+=';';trace+=call; } // 실제 장치 호출 순서를 그대로 이어 붙인다.
        const auto current=globals->window.current==first ? 1U : globals->window.current==second ? 2U : 0U;
        CHECK(trace==fields[7] && current==std::stoul(fields[8]) && globals->logFile==std::stoul(fields[9]) && result==std::stoul(fields[10]));scene.menus->CloseChatWindows();++rows;
    }
    CHECK(rows==29);
}

namespace { // 원문 대상·제출·분배 부품을 실제 Client/Console에 연결하는 검사 구성이다.
// 이름/Client/메뉴는 반환한 문맥보다 오래 유지하며 기존 초기화와 동일한 명령표를 쓴다.
RawChatWindowContext ChatLocalSubmitContext(ChatScene& scene,std::string& currentName) {
    InitializeClientChatManager(scene.client);auto context=scene.Context();
    BindClientChatRecipients(context,scene.client,scene.storage,[&currentName]() -> std::string_view { return currentName; });
    BindClientChatSubmit(context,scene.client,*scene.menus);return context;
}
}

// 실제 문자 큐→Console Enter→대상 해석→등록 clear→공유 로그 정리와 접두 보존을 검사한다.
TEST_CASE(ChatWindowClient_LocalSubmitConsoleClearKeepsRecipientPrefix) {
    ChatScene scene;ChatIdentityInputs(scene);scene.storage->ownerMode.battle=1;std::string currentName="LocalUser";auto context=ChatLocalSubmitContext(scene,currentName);
    const auto token=scene.menus->CreateChatWindow(std::move(context));scene.menus->Attach(token,10,200,scene.buttons.RootToken(),0);
    scene.menus->AppendChatLog(token,"first\nsecond\n");CHECK(scene.client.ChatWindowGlobals()->log.lineCount==2);CHECK(scene.menus->OpenChatInput({13,0,0}));
    for (const auto byte:std::string_view("Alice:.clear")) static_cast<void>(scene.buttons.RouteInput({std::uint32_t(static_cast<unsigned char>(byte)),0,0})); // 실제 편집 버퍼에 문자 사건을 순서대로 넣는다.
    CHECK(scene.Input(token).Text()=="Alice:.clear");
    static_cast<void>(scene.buttons.RouteInput({13,0,0}));
    CHECK(scene.client.ChatWindowGlobals()->log.lineCount==0 && scene.Input(token).Text()=="Alice:" && (scene.Console(token).flags&1));
    CHECK(scene.client.DotCommandGlobals()->DispatchState().separatorOffset>=0 && scene.quitCalls==0);scene.menus->CloseChatWindows();
}

// 한 제출의 세미콜론 명령들이 같은 표/상태를 쓰며 quit은 기존 실제 예약 장치에 한 번 전달한다.
TEST_CASE(ChatWindowClient_LocalSubmitClearThenQuitUsesSameRegistry) {
    ChatScene scene;ChatIdentityInputs(scene);std::string currentName="LocalUser";auto context=ChatLocalSubmitContext(scene,currentName);const auto submit=context.sendChat;
    const auto token=scene.menus->CreateChatWindow(std::move(context));scene.menus->AppendChatLog(token,"kept until clear\n");
    const auto* state=&scene.client.DotCommandGlobals()->DispatchState();submit(scene.client.ChatManagerGlobals()->Current(),"EVERYONE:.clear;.quit");
    CHECK(scene.client.ChatWindowGlobals()->log.lineCount==0 && scene.quitCalls==1 && scene.menus->ChatWindowRoot()==token);
    CHECK(&scene.client.DotCommandGlobals()->DispatchState()==state);scene.menus->CloseChatWindows();
}

// 빈 실제 Enter는 일반 모드에서 숨기고 challenge에서는 같은 원문 기하의 표시 관문을 유지한다.
TEST_CASE(ChatWindowClient_LocalSubmitEmptyEnterUsesCurrentChallenge) {
    for (const bool challenge:{false,true}) { // 같은 실제 UI에 두 원문 모드 입력을 따로 구성한다.
        ChatScene scene;ChatIdentityInputs(scene);scene.storage->ownerMode.challenge=challenge;std::string currentName="LocalUser";
        auto context=ChatLocalSubmitContext(scene,currentName);const auto token=scene.menus->CreateChatWindow(std::move(context));scene.menus->Attach(token,10,200,scene.buttons.RootToken(),0);
        scene.menus->ChatGeometryBackend()->SetInputVisible(token,1);CHECK(scene.Console(token).flags&1);
        static_cast<void>(scene.buttons.RouteInput({13,0,0}));CHECK(bool(scene.Console(token).flags&1)==challenge && scene.Input(token).Text().empty() && scene.quitCalls==0);
        scene.menus->CloseChatWindows();
    }
}

// 일반 본문 서식/송신을 성공으로 삼키지 않고 앞선 실제 제출 효과와 명시 경계를 남긴다.
TEST_CASE(ChatWindowClient_LocalSubmitUnrestoredFormattingRemainsExplicit) {
    ChatScene scene;ChatIdentityInputs(scene);std::string currentName="LocalUser";auto context=ChatLocalSubmitContext(scene,currentName);const auto submit=context.sendChat;
    const auto token=scene.menus->CreateChatWindow(std::move(context));scene.menus->AppendChatLog(token,"preserved\n");
    bool stopped{};try { submit(scene.client.ChatManagerGlobals()->Current(),"hello"); }catch (const std::logic_error& error) { stopped=std::string_view(error.what())=="Chat 일반 본문의 실제 서식·송신 미연결"; }
    CHECK(stopped && scene.client.ChatWindowGlobals()->log.lineCount==1 && scene.quitCalls==0);
    scene.menus->CloseChatWindows();
}

// 실제 문자/Console Enter가 front/back/format alias의 현재 manager와 같은 ID 설정 키에 도달한다.
TEST_CASE(ChatWindowClient_FormatCommandsUseActualConsoleAndConfig) {
    ChatScene scene;ChatIdentityInputs(scene);std::string currentName="LocalUser";auto context=ChatLocalSubmitContext(scene,currentName);const auto token=scene.menus->CreateChatWindow(std::move(context));
    scene.menus->Attach(token,10,200,scene.buttons.RootToken(),0);CHECK(scene.menus->OpenChatInput({13,0,0}) && scene.client.CurrentSubscriber()==100);
    for (const auto command:{".front ~0~g~B",".back ~r",".format ~b"}) { // 같은 실제 Console에서 세 등록 명령을 연속 제출한다.
        for (const auto byte:std::string_view(command)) static_cast<void>(scene.buttons.RouteInput({std::uint32_t(static_cast<unsigned char>(byte)),0,0})); // 실제 Gump 문자 큐로 전체 명령을 입력한다.
        CHECK(scene.Input(token).Text()==command);static_cast<void>(scene.buttons.RouteInput({13,0,0}));CHECK(scene.Input(token).Text().empty() && (scene.Console(token).flags&1));
        const bool front=std::string_view(command).starts_with(".front");const auto expected=front ? "~g~B" : std::string_view(command).starts_with(".back") ? "~r" : "~b";
        std::string value;scene.client.Configuration().ReadString(front ? "100chatFormat2" : "100chatFormat",value);CHECK(value==expected);
        const auto manager=scene.client.ChatManagerGlobals()->Bytes(scene.client.ChatManagerGlobals()->Current());const auto offset=front ? 0x50U : 0x14U;
        CHECK(std::string_view(reinterpret_cast<const char*>(manager.data()+offset))==expected);
    }
    CHECK(scene.quitCalls==0);scene.menus->CloseChatWindows();
}
