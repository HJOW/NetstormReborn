// 실제 편집 메모리·공통 등록표·글꼴/픽셀·화면 교체 수명을 창 없이 검증한다.
#include "DudeSpawnSupport.h"
#include "client/RawButtonControls.h"
#include "client/RawMenuControls.h"
#include "client/RawTextEntryControls.h"
#include "client/TextEntryStorage.h"
#include "client/PaletteShade.h"
#include "platform/Bitmap.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <sstream>
using namespace netstorm::client;
namespace { // 실제 장치의 검사 문맥과 원시 관측만 모은다.
struct EntryScene { // 원본 자산/공통 UI와 명시 외부 입력 장치를 같은 수명으로 묶는다.
    const netstorm::test::dudespawn::DudeAssets& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078); // 보호 파일을 읽기만 한다.
    FontStore fonts{assets.files,"Arial"};std::array<std::uint32_t,256> logical{};PaletteColorTable palette; // 같은 실제 글꼴/팔레트다.
    double seconds{};std::string clipboard,submitted;std::uint32_t reads{},writes{},completions{}; // 외부 장치의 입력/순수 관측이다.
    std::optional<std::string> completionWord;std::int32_t completionLength{};bool completionDirection{}; // 자동완성 callback의 원시 인자다.
    RawButtonControls buttons{OriginalEdition::Patch1078,{[this] { return seconds; }}};RawMenuControls* menus{};RawTextEntryControls* entries{}; // 같은 실제 공통 노드/가상 처리기다.
    EntryScene() { // 게임/창 없이 실제 UI 장치를 설치한다.
        for (std::size_t index=0;index<logical.size();++index) { const auto c=assets.game.Palette().Color(static_cast<std::uint8_t>(index));logical[index]=c.red|(std::uint32_t(c.green)<<8)|(std::uint32_t(c.blue)<<16); } // 원본 채널 순서다.
        palette=BuildPaletteColorTable(logical);RawMenuHooks hooks;hooks.command=[](auto,auto,auto) {};hooks.readInt=[](auto,std::int32_t& value) { value=0; };hooks.sound=[](auto) {};hooks.poll=[](auto) { return InputEvent{InputCode::kRelease,0,0}; };hooks.cursor=[](auto) {};
        menus=&buttons.EnableMenus(assets.game,fonts,std::move(hooks));menus->SetPalette(palette);Resize();RawTextEntryContext context;context.now=[this] { return seconds; };
        context.clipboardRead=[this](auto output) { ++reads;CHECK(output.size()==256);const auto count=std::min(clipboard.size(),output.size()-1);std::copy_n(clipboard.data(),count,output.data());output[count]=0; };
        context.clipboardWrite=[this](auto bytes) { ++writes;clipboard.assign(bytes.begin(),bytes.end()); };
        context.recipient=[](auto text) { return text=="EVERYONE"; };
        context.complete=[this](auto callback,auto word,auto length,bool direction)->std::optional<std::string> { CHECK(callback==0x42e540);++completions;completionWord=word ? std::optional<std::string>(*word) : std::nullopt;completionLength=length;completionDirection=direction;return completions==1 ? "EVERYONE" : "ALLIES"; };
        context.submit=[this](auto callback,auto text) { CHECK(callback==0x42f440);submitted=text; };entries=&menus->EnableTextEntry(std::move(context));
    }
    void Resize() { const auto& asset=assets.game.Find("fortGump");const auto frame=assets.game.FrameIndex(asset,"A01",0);const auto size=assets.game.Shapes().SquidMetrics(asset.block,frame);buttons.SetControls({}, {},640,480,{static_cast<std::uint32_t>(asset.block+70),static_cast<std::uint32_t>(frame),size.width,size.height,{}}); } // 실제 화면 교체 수명이다.
    std::uint32_t Count() const { std::istringstream input(buttons.Report());std::string name;std::uint32_t controls{},count{};input>>name>>controls>>count;CHECK(name=="RawButtons");return count; } // 현재 등록 수를 직접 읽는다.
    std::uint32_t Create(std::string_view text="Alpha beta") { return entries->ConstructConsole(220,48,text,0,palette.named[30].index,palette.named[31].index); } // 실제 Console/버퍼/두 Slider다.
    std::vector<std::uint8_t> Draw(std::optional<ScreenRect> clip={}) { std::vector<std::uint8_t> pixels(647*480,0x5a);menus->Draw(pixels,647,480,palette,BuildGumpShadeMaps(logical),clip);return pixels; } // 실제 공통 수집/정렬/클립/글리프다.
};
}

// 같은 물리의 오프셋/재할당/줄표 확장과 독립 버퍼 수명을 검사한다.
TEST_CASE(TextEntryClient_RealStorageAddressesGrowthAndIndependentLifetimes) {
    TextEntryStorage storage({},{});auto& first=storage.Create(1);auto& second=storage.Create(2);first.Construct();second.Construct();first.Reset();second.Reset();const auto initial=first.State().data;
    std::string input(4301,'A');input.push_back(0);first.Insert(input);CHECK(first.State().data!=initial && first.State().capacity==5120 && first.Text().size()==4301 && first.Data()[4301]==0);
    auto inside=storage.Bytes(first.State().data+4000);CHECK(inside.data()==first.Data().data()+4000 && inside[300]=='A');second.Reset(std::span("isolated",9));CHECK(second.Text()=="isolated");
    std::string lines;for (int i=0;i<260;++i) lines+="x\n";lines.push_back(0);first.Reset(lines);CHECK(first.LineCount()==261 && first.State().offsetCapacity==384 && first.LineLength(0)==2 && first.LineLength(260)==0); // 실제 줄offset 물리도 두 번 확장된다.
    std::uint32_t last{};std::memcpy(&last,storage.Bytes(first.State().offsets+260*4).data(),4);CHECK(last==520);first.Destroy();storage.ReleaseBuffer(1);CHECK(storage.BufferCount()==1 && storage.AllocationCount()==2 && second.Text()=="isolated");second.Destroy();storage.ReleaseBuffer(2);CHECK(storage.BufferCount()==0 && storage.AllocationCount()==0);
}

// 실제 공통 노드와 두 Slider, 선택/복사/붙여넣기, Console callback 인자를 검사한다.
TEST_CASE(TextEntryClient_ActualConsoleTreeInputClipboardAndCompletion) {
    EntryScene scene;const auto before=scene.Count(),token=scene.Create();auto* node=scene.entries->Console(token);auto* buffer=scene.entries->Storage().Find(node->buffer);
    CHECK(scene.menus->Node(token)==node && scene.menus->Background(token)==node && node->vtable==0x506a6c && node->command==10 && node->ownsBuffer && node->autocomplete==0 && node->submit==0);
    CHECK(scene.Count()==before+13 && scene.entries->Storage().BufferCount()==1 && scene.entries->Storage().AllocationCount()==2 && buffer->Text()=="Alpha beta" && buffer->State().selectionBegin==0 && buffer->State().selectionEnd==10);
    CHECK(scene.menus->Node(node->verticalSlider)->parent==token && scene.menus->Node(node->horizontalSlider)->parent==token);scene.menus->Attach(token,20,30,scene.buttons.RootToken(),0);CHECK(node->parent==scene.buttons.RootToken() && !(scene.menus->Node(node->verticalSlider)->flags&1));
    CHECK(scene.buttons.HandleGump(token,'Q',0,0)==1 && buffer->Text()=="Q");scene.buttons.HandleGump(token,'R',0,0);CHECK(buffer->Text()=="QR");buffer->Select(0,1);scene.buttons.HandleGump(token,0x102d0000,0,0);CHECK(scene.clipboard=="Q" && scene.writes==1);scene.clipboard="p\tq\n";scene.entries->Backend().SetExcluded(token,"\t\n");buffer->Select(1,2);scene.buttons.HandleGump(token,0x202d0000,0,0);CHECK(buffer->Text()=="Qpq\n" && scene.reads==1);CHECK(scene.entries->Storage().Bytes(node->excludedChars)[0]==9 && scene.entries->Storage().Bytes(node->excludedChars)[1]==0); // 원문 SetExcluded는 LF를 제외 목록에서 지워 줄바꿈 붙여넣기를 허용한다.
    scene.entries->Backend().SetText(token,std::span("Ev",3));buffer->Select(2,2);node->autocomplete=0x42e540;node->submit=0x42f440;scene.buttons.HandleGump(token,9,0,0);CHECK(buffer->Text()=="EVERYONE" && scene.completionWord=="Ev" && scene.completionLength==2 && !scene.completionDirection);
    scene.buttons.HandleGump(token,0x20000009,0,0);CHECK(buffer->Text()=="ALLIES" && !scene.completionWord && scene.completionLength==8 && scene.completionDirection);node->multiline=0;scene.buttons.HandleGump(token,13,0,0);CHECK(scene.submitted=="ALLIES" && buffer->Text()=="ALLIES" && node->resetWord==1);
    scene.entries->Backend().SetExcluded(token,std::nullopt);scene.menus->CloseTextEntries();CHECK(scene.Count()==before && scene.entries->Storage().BufferCount()==0 && scene.entries->Storage().AllocationCount()==0);
}

// 실제 글꼴·반전 선택·caret/점멸과 공통 클립/행 padding을 검사한다.
TEST_CASE(TextEntryClient_RealGlyphSelectionCaretClipAndIdle) {
    EntryScene scene;const auto token=scene.Create("Alpha beta\nGamma");auto* node=scene.entries->Console(token);auto* buffer=scene.entries->Storage().Find(node->buffer);scene.menus->Attach(token,20,30,scene.buttons.RootToken(),0);node->recipientHighlight=0;buffer->Select(1,5);const auto selected=scene.Draw();
    buffer->Select(5,5);node->blinkPhase=0;const auto plain=scene.Draw();node->blinkPhase=1;const auto caret=scene.Draw();CHECK(selected!=plain && caret!=plain);scene.seconds=2.0;const auto phase=node->blinkPhase;scene.buttons.IdleGumps();CHECK(node->blinkPhase==phase+1 && node->blinkAt>scene.seconds);
    const ScreenRect clip{24,32,70,43};const auto clipped=scene.Draw(clip);std::size_t changed{};for (int y=0;y<480;++y) for (int x=0;x<647;++x) { const auto value=clipped[y*647+x];if (x>=clip.left && x<clip.right && y>=clip.top && y<clip.bottom) changed+=value!=0x5a;else CHECK(value==0x5a); } // 같은 실제 출력이 클립/행 padding 밖을 쓰지 않아야 한다.
    CHECK(changed>0);if (const auto* output=std::getenv("NETSTORM_TEXT_ENTRY_QA")) { std::vector<std::uint8_t> rgba(640*480*4);for (int y=0;y<480;++y) for (int x=0;x<640;++x) { const auto c=scene.assets.game.Palette().Color(selected[y*647+x]);const auto at=(y*640+x)*4;rgba[at]=c.red;rgba[at+1]=c.green;rgba[at+2]=c.blue;rgba[at+3]=255; }netstorm::platform::WriteBitmap(output,640,480,rgba); } // 명시 검사 경로에서만 실제 표면을 이미지로 기록한다.
    scene.menus->CloseTextEntries();CHECK(scene.entries->Storage().AllocationCount()==0);
}

// 새/기존 실제 Console 공존과 개별 자식 우선 반납/화면 교체를 검사한다.
TEST_CASE(TextEntryClient_ConcurrentConsolesRetirementAndPartialCreationCleanup) {
    EntryScene scene;const auto before=scene.Count(),first=scene.Create("First"),second=scene.Create("Second");auto* a=scene.entries->Console(first);auto* b=scene.entries->Console(second);CHECK(first!=second && a->buffer!=b->buffer && scene.Count()==before+26);
    scene.menus->Attach(first,10,10,scene.buttons.RootToken(),0);scene.menus->Attach(second,10,100,scene.buttons.RootToken(),0);scene.buttons.KillGump(first);scene.buttons.CleanupGumps();CHECK(!scene.entries->Node(first) && scene.entries->Node(second)==b && scene.entries->Storage().BufferCount()==1 && scene.Count()==before+13);
    scene.Resize();CHECK(!scene.entries->Node(second) && scene.entries->Storage().BufferCount()==0 && scene.entries->Storage().AllocationCount()==0 && scene.Count()==before);const auto partial=scene.entries->Allocate(true);CHECK(scene.entries->Node(partial) && scene.Count()==before);scene.menus->CloseTextEntries();CHECK(!scene.entries->Node(partial) && scene.Count()==before);
}

// 전역 로그 자체를 같은 실제 글꼴/조각 처리기에 빌리고 재생성/중복 생성을 검사한다.
TEST_CASE(TextEntryClient_BorrowedGlobalLogUsesSameStateAndActualFontFragments) {
    EntryScene scene;StyleTextState fragments;GumpText metrics(scene.fonts,OriginalEdition::Patch1078);StyleTextHooks hooks;BindStyleTextFonts(hooks,metrics);StyleText style(fragments,std::move(hooks));StyleTextBufferHooks bufferHooks;BindStyleTextBufferFonts(bufferHooks,metrics);std::vector<std::uint32_t> released;StyleTextBufferStore store(fragments,style,std::move(bufferHooks),[&](auto token) { released.push_back(token); });StyleTextBufferState log;
    store.ConstructBorrowed(0x548758,log,128);const auto binding=store.Binding(0x548758);CHECK(binding.state==&log && log.maxLines==128 && log.initialized);std::uint32_t calls{};log.onAppend=[&](auto text) { ++calls;CHECK(text.size()==5); };const std::array<std::uint32_t,8> context{0,0,scene.palette.basic[7],0,0,0,1,1};binding.handler->AppendText(std::span("log\n",5),context,4);CHECK(calls==1 && log.lineCount==1 && log.hardLines[0] && !fragments.fragments.empty());binding.handler->Layout(100);CHECK(log.softCount>0 && binding.handler->TotalHeight()>0);
    bool rejected{};try { store.ConstructBorrowed(99,log,128); }catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected && store.Binding(0x548758).state==&log);store.Destroy(0x548758);CHECK(!log.initialized && fragments.fragments.empty() && released==std::vector<std::uint32_t>{0x548758});store.ConstructBorrowed(0x548758,log,128);CHECK(store.Binding(0x548758).state==&log && log.lineCount==0);store.Destroy(0x548758);
}
