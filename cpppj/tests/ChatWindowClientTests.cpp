// 실제 Client 전역/명령 표와 전체 Chat 공장·공통 자식·공유 로그·픽셀·수명을 검증한다.
#include "DudeSpawnSupport.h"
#include "client/ClientMain.h"
#include "client/ClientChatWindowGlobals.h"
#include "client/ClientChatManagerGlobals.h"
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
