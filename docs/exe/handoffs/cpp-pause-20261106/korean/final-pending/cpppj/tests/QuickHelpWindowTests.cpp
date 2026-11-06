#include "client/QuickHelpFileRead.h"
#include "o/OriginalText.h"
#include "QuickHelpFileReadTestSupport.h"
#include "o/RawColorMapCache.h"
#include "o/RawFileName.h"
// 최초98행의 전용 창/HTML 부모 전체 경로를 실제 제품 몸체와 명시 GUI 경계로 대조한다.
#include "GumpRawSupport.h"
#include "client/QuickHelpWindow.h"
#include "client/QuickHelpHtml.h"
#include "client/QuickHelpRequest.h"
#include <map>
using namespace netstorm::client;
using namespace netstorm::test::gumpraw;
namespace {
constexpr std::uint32_t kPool=0x12000000,kWindow=kPool+0x40000,kDecoration=kPool+0x41000,kInner=kPool+0x42000,kPictureBg=kPool+0x43000,kBodyBg=kPool+0x44000,kBody=kPool+0x45000,kLearn=kPool+0x4a000,kBack=kPool+0x4b000,kCancel=kPool+0x4c000,kInfo=kPool+0x4d000,kPicture=kPool+0x4e000,kScroll=kPool+0x4f000,kTable=kPool+0x51000,kWindowHelpSource=0x13000000; // 독립 원문이 사용한 의미별 호스트 식별값이다.
constexpr std::array<std::uint32_t,12> kNodes{kWindow,kDecoration,kInner,kPictureBg,kBodyBg,kBody,kLearn,kBack,kCancel,kInfo,kPicture,kScroll}; // 원문 raw 비교 순서다.
// 문자열은 원문 바이트16진수로만 직렬화한다.
std::string TextHex(std::string_view value) { return Hex({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()}); }
// 이번 고정 입력 배열은 쉼표/escape가 없는 문자열과 정수뿐이다.
std::vector<std::string> InputsForWindow(std::string_view value) { CHECK(value.front()=='[' && value.back()==']');return Split(value.substr(1,value.size()-2),','); }
// 인용 문자열의 원래 내부를 읽으며 HTML/이름을 정규화하지 않는다.
std::string Topic(const std::string& value) { CHECK(value.size()>=2 && value.front()=='"' && value.back()=='"');return value.substr(1,value.size()-2); }
// native의 고정%s/%d 서식만 공급하며 실제 정보 분기는 제품이 선택한다.
std::string PrintForWindow(std::string_view pattern,std::span<const PriestMenuArgument> arguments) {
    std::string result;std::size_t argument{};
    // 실제 서식이 읽는 인자만 사용한다.
    for (std::size_t i=0;i<pattern.size();++i) { if (pattern[i]!='%') { result+=pattern[i];continue; }CHECK(i+1<pattern.size() && argument<arguments.size());const auto code=pattern[++i];result+=code=='s' ? std::get<std::string>(arguments[argument++]) : std::to_string(std::get<std::int32_t>(arguments[argument++])); }
    CHECK(argument==arguments.size());return result;
}
// 기본/자식 GUI와 Config/VFS는 native와 같은 명시 입력이다. 창/HTML/요청은 실제 제품 몸체다.
struct WindowFixtureScene {
    QuickHelpContentNode window;HtmlBodyGumpNode body;std::map<std::uint32_t,GumpCreateNode> bases; // 실제 부모와 안정된 같은 자식 기본 필드다.
    QuickHelpWindowState state;QuickHelpPopupState popup;HtmlBodyGumpState bodyState;HtmlTextParserGlobals globals; // 원본과 같은 단일 현재 상태다.
    std::unique_ptr<HtmlBodyGump> html;std::unique_ptr<QuickHelpWindow> help;std::map<std::size_t,std::size_t> allocations; // 실제 제품 처리기와 명시 할당 순서다.
    float scale{1};bool script{},empty{},failure{};std::int32_t lineCount{1},frame{3};std::string trace,helpText="<a name=\"unitType\"><info><p>BODY</a>\nEND"; // 독립 GUI/파일/Config 입력이다.
    std::function<void(std::uint32_t)> backgroundCreated; //질감 자식 생성 뒤 원본 전역이 바뀐 경우의 현재 값 재조회를 검사한다.
    std::function<std::optional<std::string>(std::string_view)> fileRead; // 선택한 실제/명시 FILE reader를 기존 창 몸체에 공급한다.
    std::function<std::string()> fileSpec; // 언어cache추가검사만현재파일키를공급하며기존fixture의기본영문키는그대로다.
    std::function<std::string()> fileSpec; // 언어cache추가검사만현재파일키를공급하며기존fixture의기본영문키는그대로다.
    std::function<void(std::string_view)> fileError; // NULL 반환의 원문 로그와 실제 Open 예외를 구별한다.
    // 첫 관측과 같은 현재 노드/기본 전역을 공급하고 제품 처리기만 조립한다.
    WindowFixtureScene() {
        // 실제 생성자가 덮기 전의 독립 기본 필드를 설치한다.
        for (const auto token:kNodes) { Fill(token,450,350,1);Base(token).flags=1; }
        window.vtable=0x514ec4;window.command=19;window.decoration=kDecoration;window.information=kInfo;window.body=kBody;window.cancelButton=kCancel;window.learnButton=kLearn;window.backButton=kBack;window.innerBackground=kInner;window.pictureBackground=kPictureBg;window.bodyBackground=kBodyBg;
        state.helpData=helpText;state.scriptName="mission";state.color=47;state.noPictureA=2;state.noPictureB=3;popup.Mark(0x20000a0,"before");bodyState.screenWidth=1024;bodyState.screenHeight=768;bodyState.color=47;
        HtmlBodyGumpHooks b;b.node=[this](auto token) { CHECK(token==kBody);return &body; };b.base=[this](auto token) { return &Base(token); };b.constructBase=[this](auto token,auto command,float width,float height,auto flags) { ConstructBase(token,command,width,height,flags); };
        b.allocate=[this](auto bytes) { return Allocate(bytes); };b.constructText=[this](auto token,auto width,auto height,auto font,auto color) { ConstructText(token,width,height,font,color); };
        b.kill=[this](auto token) { Kill(token); };b.push=[this](auto token) { Push(token); };b.attach=[this](auto token,float x,float y,auto parent,auto mode) { Attach(token,x,y,parent,mode); };b.baseAttach=[this](auto token,float x,float y,auto parent,auto mode) { BaseAttach(token,x,y,parent,mode); };
        b.styled=[this](auto token,std::string_view text) { Event("Styled,"+std::to_string(token)+','+TextHex(text)); };b.scroll=[this](auto token,auto value) { Event("Scroll,"+std::to_string(token)+','+std::to_string(value)); };b.slider=[this](auto token,auto value) { Event("Slider,"+std::to_string(token)+','+std::to_string(value)); };b.scrollable=[this](auto token,auto value) { Event("Scrollable,"+std::to_string(token)+','+std::to_string(value)); };html=std::make_unique<HtmlBodyGump>(bodyState,std::move(b));
        QuickHelpWindowHooks h;h.node=[this](auto token) { CHECK(token==kWindow);return &window; };h.base=[this](auto token) { return &Base(token); };h.html=[this](auto token) { CHECK(token==kBody);return &body; };h.constructBase=[this](auto token,auto command,float width,float height,auto flags) { ConstructBase(token,command,width,height,flags); };h.allocate=[this](auto bytes) { return Allocate(bytes); };
        h.constructDecoration=[this](auto token,float width,float height) { Event("Decoration,"+std::to_string(token)+','+std::to_string(Bits(width))+','+std::to_string(Bits(height)));Fill(token,width,height,1);Base(token).alignFlags=0x20;Base(token).secondaryDepth=0x123; };
        h.constructBackground=[this](auto token,float width,float height,auto background,auto flags,auto type,auto image) { Event("Background,"+std::to_string(token)+','+std::to_string(Bits(width))+','+std::to_string(Bits(height))+','+std::to_string(background)+','+std::to_string(flags)+','+std::to_string(type)+','+std::to_string(image));Fill(token,width,height,flags);if (backgroundCreated) backgroundCreated(token); };
        h.constructHtml=[this](auto token) { html->Construct(token); };
        h.constructButton=[this](auto token,auto width,auto height,std::string_view label,auto unused,auto context) { Event("Stone,"+std::to_string(token)+','+std::to_string(width)+','+std::to_string(height)+','+TextHex(label)+','+std::to_string(unused)+','+std::to_string(context));Fill(token,60,20,0); };h.fitButton=[this](auto token) { Event("Fit,"+std::to_string(token)); };
        h.constructInformation=[this](auto token,auto width,auto height,auto font,auto color) { ConstructText(token,width,height,font,color); };
        h.constructPicture=[this](auto token,auto type,auto image,auto mode) { Event("Picture,"+std::to_string(token)+','+std::to_string(type)+','+std::to_string(image)+','+std::to_string(mode));Fill(token,99,88,1); };
        h.translate=[](std::string_view value) { return std::string(value); };h.config=[this](std::string_view key) { Event("Config,"+TextHex(key));return fileSpec ? fileSpec():std::string("d/help.english"); };
        h.load=[this](std::string_view name) -> std::optional<std::string> { Event("Load,"+TextHex(name)+",0,2");if (fileRead) return fileRead(name);return failure ? std::nullopt : std::optional<std::string>(helpText); };h.loadError=[this](std::string_view name) { Event("LoadError,9,"+TextHex(name));if (fileError) fileError(name); };
        h.substitute=[this](std::string_view text) { Event("Substitute,"+TextHex(text)+",8192");return empty ? std::string{} : std::string(text); };h.nearestColor=[this](auto red,auto green,auto blue) { Event("Color,"+std::to_string(red)+','+std::to_string(green)+','+std::to_string(blue));return 35U; };h.scripted=[this] { Event("Script");return script; };
        h.kill=[this](auto token) { Kill(token); };h.push=[this](auto token) { Push(token); };h.clearBody=[this](auto token) { html->Clear(token); };h.attach=[this](auto token,float x,float y,auto parent,auto mode) { Attach(token,x,y,parent,mode); };h.baseAttach=[this](auto token,float x,float y,auto parent,auto mode) { BaseAttach(token,x,y,parent,mode); };h.lines=[this](auto token) { CHECK(token==kInfo);return lineCount; };
        h.render=[this](auto token,auto type,std::string_view topic,auto information,auto stream) { Render(token,type,topic,information,stream); };h.helpFrame=[this](auto) { return std::optional<std::int32_t>(frame); };h.bodyText=[this](auto token,std::string_view value) { html->SetText(token,value); };help=std::make_unique<QuickHelpWindow>(state,popup,std::move(h));
    }
    // 부모와 자식 모두 동일한 현재 기본 필드를 반환한다.
    GumpCreateNode& Base(std::uint32_t token) { if (token==kWindow) return window;if (token==kBody) return body;return bases[token]; }
    // 명시 기본 GUI 생성 입력이다. 의미/배치/필터 결과는 제품이 결정한다.
    void Fill(std::uint32_t token,float width,float height,std::uint32_t flags) { auto& node=Base(token);node=GumpCreateNode{};node.vtable=kTable;node.flags=flags|8U;node.serial=100+static_cast<std::uint32_t>(std::find(kNodes.begin(),kNodes.end(),token)-kNodes.begin());node.scale=scale;node.baseWidth=width;node.baseHeight=height;node.width=width*scale;node.height=height*scale; }
    // 명시 효과의 실제 순서만 문자열에 기록한다.
    void Event(std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
    // 실제 기본 생성자의 호출 인자 경계다.
    void ConstructBase(std::uint32_t token,std::uint32_t command,float width,float height,std::uint32_t flags) { Event("Base,"+std::to_string(token)+','+std::to_string(command)+','+std::to_string(Bits(width))+','+std::to_string(Bits(height))+','+std::to_string(flags));Fill(token,width,height,flags); }
    // 원본 크기별로 서로 독립인 현재 할당 식별값을 준다.
    std::uint32_t Allocate(std::size_t bytes) {
        const auto index=allocations[bytes]++;std::uint32_t token{};
        if (bytes==0xc4) token=kDecoration;else if (bytes==0xb8) token=std::array{kInner,kPictureBg,kBodyBg}.at(index);else if (bytes==0x40a4) token=kBody;else if (bytes==0x170) token=std::array{kLearn,kBack,kCancel}.at(index);else if (bytes==0xd8) token=std::array{kInfo,kScroll}.at(index);else { CHECK(bytes==0xb4);token=kPicture; }
        Event("New,"+std::to_string(bytes)+','+std::to_string(token));if (token==kBody) { body.body.fill(static_cast<char>(0xa5));body.links.fill(static_cast<char>(0xa5)); }return token;
    }
    // 실제 다중 줄 생성의 명시 입력/현재 버퍼 줄 수다.
    void ConstructText(std::uint32_t token,std::int32_t width,std::int32_t height,std::uint32_t font,std::uint32_t color) { Event("Text,"+std::to_string(token)+','+std::to_string(width)+','+std::to_string(height)+','+std::to_string(font)+','+std::to_string(color));Fill(token,0,0,1);lineCount=1; }
    // 지연 삭제와 가상 Push의 GUI 효과를 공급한다.
    void Kill(std::uint32_t token) { Event("Kill,"+std::to_string(token));Base(token).flags|=0x10000; }
    void Push(std::uint32_t token) { Event("Push,"+std::to_string(token));Base(token).flags|=8; } // 필드 변경 뒤 실제 제품은 현재 값을 다시 읽는다.
    // 원본 트리 자체는 별도 검증 대상이며 현재 부착 필드만 공급한다.
    void BaseAttach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) { Event("Attach,"+std::to_string(token)+','+std::to_string(Bits(x))+','+std::to_string(Bits(y))+','+std::to_string(parent)+','+std::to_string(mode));auto& node=Base(token);node.x=x;node.y=y;node.flags&=~8U;if (parent) node.parent=parent; }
    // 실제 전용 창/HTML 부모의 파생 부착은 제품 몸체를 실행한다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) { if (token==kWindow) help->Attach(token,x,y,parent,mode);else if (token==kBody && body.vtable==0x50bd38) html->Attach(token,x,y,parent,mode);else BaseAttach(token,x,y,parent,mode); }
    // 정보/본문은 이전 단위의 실제 파생 HTML/같은 링크 저장소·현재 body 배열을 사용한다.
    void Render(std::uint32_t token,std::uint32_t type,std::string_view topic,std::uint32_t information,std::uint32_t stream) {
        CHECK(token==kBody && information==kInfo && stream==0);HtmlTextParserState parserState;HtmlLinkTextState links;links.base=kBody+0x20a4;links.capacity=8192;HtmlTextParserHooks base;BindHtmlPictureText(base);BindHtmlLinkText(base,links);QuickHelpTypeInfo data;data.theme=3;
        QuickHelpHtmlHooks h;h.type=[&]() -> const QuickHelpTypeInfo& { return data; };h.description=[] { return std::string("Unit Label"); };h.hitPoints=[] { return 0; };h.translate=h.substitute=h.config=[](std::string_view value) { return std::string(value); };h.print=PrintForWindow;h.pane=[&] { return &Base(kInfo); };
        h.paneText=[&](std::string_view value) { Event("Styled,"+std::to_string(kInfo)+','+TextHex(value)); };h.paneScroll=[&](auto value) { Event("Scroll,"+std::to_string(kInfo)+','+std::to_string(value)); };h.panePush=[&] { Push(kInfo); };h.paneAttach=[&] { Event("Reattach,"+std::to_string(kInfo));Base(kInfo).flags&=~8U; };
        QuickHelpHtml parser(parserState,globals,state.infoMode,type,topic,false,{},std::move(h),std::move(base),&links);std::span<char> target,linkTarget;
        HtmlBodyParserHooks hooks;hooks.hasOutput=[] { return false; };hooks.bind=[&](auto text,auto rawLinks) { target=text;linkTarget=rawLinks; };hooks.run=[&] { const auto value=parser.Parse(*state.helpData);CHECK(value.size()<target.size() && links.cursor<linkTarget.size());std::copy(value.begin(),value.end(),target.begin());target[value.size()]=0;std::copy(links.bytes.begin(),links.bytes.end(),linkTarget.begin()); };
        html->Parse(token,hooks);
    }
    // 원본의 전체0xa0 기본 필드와0xd0 창의 추가12DWORD를 같은 순서로 기록한다.
    std::string Output() {
        std::string result;
        // 기대값 대신 현재 실제 노드의 필드를 읽는다.
        for (const auto token:kNodes) { if (!result.empty()) result+=';';result+=std::to_string(token)+':'+Encode(Base(token));if (token==kWindow) { std::array<std::uint8_t,48> raw{};const std::array values{window.picture,window.decoration,window.information,window.body,window.cancelButton,window.learnButton,window.backButton,window.stream,window.innerBackground,window.pictureBackground,window.bodyBackground,window.learnType};
                // 원본+a0..cc의 물리 순서를 유지한다.
                for (std::size_t i=0;i<values.size();++i) Put(raw,i*4,values[i]);result+=Hex(raw);
            } }
        return result+'|'+std::to_string(state.id)+','+std::to_string(state.helpData ? kWindowHelpSource : 0U)+','+std::to_string(popup.command)+'|'+TextHex(popup.Argument())+'|'+trace;
    }
    // native에 공급한 생성/배치/본문 입력을 그대로 적용한다.
    void Run(std::string_view kind,const std::vector<std::string>& input,bool paths) {
        if (paths) { helpText="<a name=\"introHelp\">INTRO</a>\n<a name=\"challengeHelp\">CHALLENGE</a>\n<a name=\"pyrosphereHelp\">BATTLE</a>\n<a name=\"unitType\">UNIT</a>\n";state.helpData=helpText; }
        if (kind=="C") { const auto teach=Number(input[0]);if (paths) failure=true;else { script=Number(input[2])!=0;scale=Float(Number(input[3])); }if (!Number(input[1])) state.helpData.reset();help->Construct(kWindow,teach); }
        else if (kind=="L") {
            const auto picture=Number(input[0]),information=Number(input[1]),buttons=Number(input[2]),profile=Number(input[3]);window.picture=picture ? kPicture : 0;window.information=information ? kInfo : 0;window.cancelButton=(buttons&1) ? kCancel : 0;window.backButton=(buttons&2) ? kBack : 0;window.baseWidth=std::array{450.0f,641.0f,320.0f}.at(profile);window.baseHeight=std::array{350.0f,481.0f,240.0f}.at(profile);
            // 각 자식의 독립 배율/버튼 치수다.
            for (std::size_t i=1;i<kNodes.size();++i) { auto& node=Base(kNodes[i]);node.scale=std::array{1.0f,1.25f,0.75f}.at(profile);node.baseWidth=30.75f+static_cast<float>((i-1)*7);node.baseHeight=20.75f+static_cast<float>(i-1); }
            lineCount=profile==2 ? 0 : 1;help->Attach(kWindow,13,21,0,0);
        } else if (kind=="H") { body.vtable=0x50bd38;body.text=Number(input[0]) ? kScroll : 0;body.flags=Number(input[1]) ? 9 : 1;body.x=17;body.y=19;allocations[0xd8]=1;html->SetText(kBody,"~Nstyled"); }
        else if (kind=="R") {
            state.battle=Number(input[0])!=0;state.challenge=Number(input[1])!=0;QuickHelpRequestHooks h;h.lookup=[&](std::string_view name) { Event("Lookup,"+TextHex(name));return 0; };h.print=[](std::string_view,std::string_view name) { return std::string(name); };h.history=[&](std::string_view name) { Event("History,"+TextHex(name)); };h.show=[&](auto type,std::string_view name) { help->Show(kWindow,type,name); };h.push=[&] { Push(kWindow); };h.attach=[&](float x,float y,auto parent,auto mode) { help->Attach(kWindow,x,y,parent,mode); };QuickHelpRequest request(window,std::move(h));request.Open(0,"");
        } else { CHECK(kind=="S");const auto type=Number(input[0]),old=Number(input[3]);frame=std::stoi(input[2]);window.picture=old ? kPicture : 0;window.information=old ? kInfo : 0;body.vtable=0x50bd38;body.text=old ? kScroll : 0;if (paths) { state.battle=Number(input[4])!=0;state.challenge=Number(input[5])!=0;empty=Number(input[6])!=0; }help->Show(kWindow,type,Topic(input[1])); }
    }
};
}
// 두 질감 자식은 각각 현재005ca920/924를 읽고 단색 내부 배경에는 TYPE0을 유지한다.
TEST_CASE(QuickHelpWindow_CurrentBackgroundGlobalsForEachChild) {
    WindowFixtureScene scene;scene.state.backgroundType=92;scene.state.backgroundFrame=1;
    scene.backgroundCreated=[&](auto token) { if (token==kPictureBg) { scene.state.backgroundType=93;scene.state.backgroundFrame=2; } };
    scene.Run("C",{"0","1","0","1065353216"},false);
    CHECK(scene.trace.find("Background,"+std::to_string(kInner)+",0,0,"+std::to_string(0x04000000U|35U)+",1,0,0")!=std::string::npos);
    CHECK(scene.trace.find("Background,"+std::to_string(kPictureBg)+",0,0,"+std::to_string(0x03e40000U)+",1,92,1")!=std::string::npos);
    CHECK(scene.trace.find("Background,"+std::to_string(kBodyBg)+",0,0,"+std::to_string(0x03e40000U)+",1,93,2")!=std::string::npos);
}
// 원본의 전체84행과 추가14경로를 대조한다. 실제 GUI/OS 실행은 포함하지 않는다.
TEST_CASE(QuickHelpWindow_Native98) {
    std::size_t count{}; // 실제 대조한 독립 행 수다.
    // 독립 원문의 전체 노드/캐시/표식과 호출 순서를 대조한다.
    for (const auto& path:std::array{NETSTORM_QUICK_HELP_WINDOW_FIXTURE,NETSTORM_QUICK_HELP_WINDOW_PATHS_FIXTURE}) {
        const bool paths=path==std::string(NETSTORM_QUICK_HELP_WINDOW_PATHS_FIXTURE);
        // 각 행은 독립 노드와 같은 공개 제품 몸체를 사용한다.
        for (const auto& row:netstorm::test::rawscene::LoadFixture(path).rows) { CHECK(row.size()==3);if (row.size()!=3) return;WindowFixtureScene scene;scene.Run(row[0],InputsForWindow(row[1]),paths);const auto value=scene.Output();if (value!=row[2]) std::printf("Quick Help 전체 창 원문 차이 %zu %s %s\n%s\n",count,row[0].c_str(),row[1].c_str(),value.c_str());CHECK(value==row[2]);++count; }
    }
    CHECK(count==98);
}

namespace { // 전체 창/41baa0의 신규 원문 입력 장치와 현재 File 관측이다.
constexpr std::uintptr_t kHelpFile=0x12200000,kHelpKey=0x12318000; // 독립 CPU의 FILE/키 토큰이다.
constexpr std::uint32_t kHelpPoison=0x65656565; // 원문 미작성 지역에 준 명시 seed다.
struct HelpReadFailure { std::string kind; }; // Open/할당/read/close 예외 지점의 이름이다.
std::string HelpReadFields(const netstorm::o::RawBaseFileState& file) { // 실제 여섯 필드를 원문 DWORD와 같은 순서로 보존한다.
    const std::array<std::uintptr_t,6> values{{file.file,file.flags,static_cast<std::uint32_t>(file.offset),static_cast<std::uint32_t>(file.length),file.key,file.keyLength}};std::string result;
    for (const auto value:values) { if (!result.empty()) result+=',';result+=std::to_string(value); } // 포인터 토큰도 native 입력 폭 그대로 기록한다.
    return result;
}
std::string HelpReadEvents(const std::vector<std::string>& events) { std::string result;for (const auto& event:events) { if (!result.empty()) result+=';';result+=event; }return result.empty()?"-":result; } // 실제 장치 방문 순서다.
}

TEST_CASE(QuickHelpFileRead_Native62WholeReadExtraTwoLfNulAndConstructorCache) { // GUI는 기존 장치이며 생성자/전체읽기/FILE 반납/캐시는 모두 실제 제품 몸체다.
    using namespace netstorm::o;std::size_t rows{};
    for (const auto& row:netstorm::test::rawscene::LoadFixture(NETSTORM_QUICK_HELP_FILE_READ_FIXTURE).rows) { // 앞7열만 독립 입력으로 쓰고 원문 결과열은 수정하지 않는다.
        CHECK(row.size()==14);if (row.size()!=14) return;const auto length=Number(row[0]),key=Number(row[1]),subfile=Number(row[2]),scenario=Number(row[4]),cached=Number(row[5]),repeat=Number(row[6]);auto available=Number(row[3]);std::uint32_t used{},position{},calls{};std::vector<std::string> events;std::vector<std::uint8_t> allocation;std::array<std::uint8_t,4096> scratch;scratch.fill(101);std::int32_t output=99;
        RawBaseFileState file{kHelpPoison,kHelpPoison,static_cast<std::int32_t>(kHelpPoison),static_cast<std::int32_t>(kHelpPoison),kHelpPoison,kHelpPoison};RawBaseFileIOHooks io;BaseFileWholeReadHooks whole;WindowFixtureScene scene;scene.state.helpData=cached ? std::optional<std::string>("OLD\n"):std::nullopt;
        whole.probe=[](auto)->std::uint32_t { throw std::logic_error("필수 Help의 사전Probe 호출"); }; // 원문 고정 check0이면 도달하지 않는다.
        whole.open=[&](auto& current,auto physical,auto mode,auto create) { const auto end=std::find(physical.begin(),physical.end(),std::uint8_t{});const std::string name(reinterpret_cast<const char*>(physical.data()),static_cast<std::size_t>(end-physical.begin()));events.push_back("O:"+TextHex(name)+":"+std::to_string(mode)+":"+std::to_string(create)+":"+HelpReadFields(current));current={scenario==2?0:kHelpFile,0x80U|subfile,subfile?37:0,subfile?static_cast<std::int32_t>(length):0,key?kHelpKey:0,key?3U:0U};position=subfile?37U:0U;if (scenario==1) throw HelpReadFailure{"open"}; };
        whole.allocate=[&](auto size) { events.push_back("N:"+std::to_string(size));if (scenario==3) throw HelpReadFailure{"allocation"};allocation.assign(size,205);return BaseFileWholeAllocation{kWindowHelpSource,allocation}; };
        io.tell=[&](auto token) { events.push_back("T:"+std::to_string(token));return static_cast<std::int32_t>(position); };io.seek=[&](auto token,auto offset,auto origin) { events.push_back("S:"+std::to_string(token)+":"+std::to_string(static_cast<std::uint32_t>(offset))+":"+std::to_string(origin));position=origin==2?length:static_cast<std::uint32_t>(offset); };
        io.read=[&](auto token,auto destination,auto size,auto count) { const auto actual=std::min(size*count,available);events.push_back("R:"+std::to_string(token)+":"+std::to_string(size)+":"+std::to_string(count)+":"+std::to_string(actual));if (scenario==4) throw HelpReadFailure{"read"};for (std::uint32_t index=0;index<actual;++index) destination[index]=static_cast<std::uint8_t>((used+index)*29+7);used+=actual;available-=actual;position+=actual; }; // fread가 쓰지 않은 입력/암호scratch는 명시 seed 그대로 남긴다.
        const std::array<std::uint8_t,3> keyBytes{{'a','b','c'}};io.key=[&](auto token,auto size) { CHECK(token==kHelpKey && size==3);return std::span<const std::uint8_t>(keyBytes); };io.close=[&](auto token) { events.push_back("C:"+std::to_string(token));if (scenario==5) throw HelpReadFailure{"close"}; };io.freeKey=[&](auto token) { events.push_back("F:"+std::to_string(token)); };
        scene.fileRead=[&](std::string_view name)->std::optional<std::string> { ++calls;events.push_back("W:"+TextHex(name)+":0:2");std::string input(name);input+='\0';const auto result=ReadQuickHelpFileBytes(file,{reinterpret_cast<const std::uint8_t*>(input.data()),input.size()},output,io,whole,scratch);if (!result) return {};return std::string(reinterpret_cast<const char*>(allocation.data()),static_cast<std::size_t>(output)); };
        scene.fileError=[&](auto) { events.push_back("E:9"); };std::string outcome="ok";
        try { for (std::uint32_t index=0;index<repeat;++index) { scene.allocations.clear();scene.help->Construct(kWindow,0); } }catch (const HelpReadFailure& failure) { outcome=failure.kind; } // 명시 GUI 주소만 재사용하며 같은 Help 전역 캐시는 지우지 않는다.
        CHECK(outcome==row[7] && static_cast<bool>(scene.state.helpData)==static_cast<bool>(Number(row[8])) && (scene.state.helpData?TextHex(*scene.state.helpData):"-")==row[9]);CHECK((calls?HelpReadFields(file):"-")==row[11] && HelpReadEvents(events)==row[12] && calls==Number(row[13]));
        if (allocation.empty()) CHECK(row[10]=="-");
        else { const auto physical=Bytes(row[10]);CHECK(physical.size()==allocation.size());if (scene.state.helpData && !cached) { // 값형 문자열은 LF/NUL까지 같은 논리 바이트를 소유하고 원문 미작성 padding은 노출하지 않는다.
                CHECK(scene.state.helpData->size()+1<=physical.size());CHECK(std::equal(scene.state.helpData->c_str(),scene.state.helpData->c_str()+scene.state.helpData->size()+1,physical.begin(),[](char a,std::uint8_t b) { return static_cast<std::uint8_t>(a)==b; }));CHECK(std::equal(allocation.begin(),allocation.begin()+output,physical.begin()));
            }else CHECK(allocation==physical); }
        ++rows;
    }
    CHECK(rows==62);
}

TEST_CASE(QuickHelpFileRead_ActualClientSuccessfulCacheRetainsFirstBytesAndSingleLf) { // 기존 창 몸체가 실제 Client 읽기를 한 번만 수행하고 재생성에서는 성공 캐시를 유지한다.
    using namespace netstorm::test::helpread;const auto root=Work();const std::string original("old\0tail",8);Text(root/"d/help.english",original);netstorm::client::Client instance(Options(root));WindowFixtureScene scene;scene.state.helpData.reset();std::size_t reads{};scene.fileRead=[&](auto name) { ++reads;return ReadClientQuickHelpFile(instance,name); };scene.help->Construct(kWindow,0);
    CHECK(scene.state.helpData && *scene.state.helpData==original+"\n" && scene.state.helpData->c_str()[scene.state.helpData->size()]==0 && reads==1);const auto index=instance.Files().FileNameOwner()->Index();Text(root/"d/help.english","new");scene.allocations.clear();scene.help->Construct(kWindow,0);CHECK(*scene.state.helpData==original+"\n" && reads==1 && instance.Files().FileNameOwner()->Index()==index);
}

TEST_CASE(QuickHelpFileRead_ActualMissingThrowsBeforeCacheAndEmptyFileCanRetry) { // 필수 missing을 optional 없음으로 바꾸지 않고 실패 전 자식 생성과 다음 빈 파일 성공을 보존한다.
    using namespace netstorm::test::helpread;const auto root=Work();std::filesystem::create_directories(root);netstorm::client::Client instance(Options(root));WindowFixtureScene scene;scene.state.helpData.reset();std::size_t errors{};scene.fileRead=[&](auto name) { return ReadClientQuickHelpFile(instance,name); };scene.fileError=[&](auto) { ++errors; };bool rejected{};
    try { scene.help->Construct(kWindow,0); }catch (const netstorm::o::ColorCacheUnableToOpen&) { rejected=true; }
    CHECK(rejected && !scene.state.helpData && scene.state.id==0 && errors==0 && scene.window.body==kBody && scene.window.backButton==kBack);Text(root/"d/help.english","");scene.allocations.clear();scene.help->Construct(kWindow,0);CHECK(scene.state.helpData==std::string("\n") && scene.state.helpData->c_str()[1]==0 && scene.state.id==scene.window.serial && errors==0);
}

TEST_CASE(KoreanIntegration_ActualHelpCacheReloadsAcrossLanguageInvalidation) { // 같은실제창/cache와FILE해독을영어/한국어/영어순서로대조한다.
    using namespace netstorm::test::helpread;const auto root=Work();const std::string english="<h2>English Help</h2>";Text(root/"d/help.english",english);Text(root/"d/help.korean","\xef\xbb\xbf<h2>한국어 도움말</h2>");netstorm::client::Client instance(Options(root));WindowFixtureScene scene;scene.state.helpData.reset();std::string selected="d/help.english";std::size_t reads{}; // fixture기대값을바꾸지않는새독립입력이다.
    scene.fileSpec=[&] { return selected; };scene.fileRead=[&](auto name) { ++reads;return ReadClientQuickHelpFile(instance,name); };scene.help->Construct(kWindow,0);CHECK(scene.state.helpData==english+"\n" && reads==1);
    selected="d/help.korean";scene.allocations.clear();scene.help->Construct(kWindow,0);CHECK(scene.state.helpData==english+"\n" && reads==1); // 기존성공cache는명시무효화전까지유지된다.
    scene.help->InvalidateCache();CHECK(!scene.state.helpData);scene.allocations.clear();scene.help->Construct(kWindow,0);CHECK(scene.state.helpData && reads==2);
    if (scene.state.helpData) CHECK(netstorm::o::DecodeDisplayText({reinterpret_cast<const std::uint8_t*>(scene.state.helpData->data()),scene.state.helpData->size()})=="<h2>한국어 도움말</h2>\n");
    selected="d/help.english";scene.help->InvalidateCache();scene.allocations.clear();scene.help->Construct(kWindow,0);CHECK(scene.state.helpData==english+"\n" && reads==3);CHECK(netstorm::o::ReadFileBytes(root/"d/help.english")==std::vector<std::uint8_t>(english.begin(),english.end()));
}
