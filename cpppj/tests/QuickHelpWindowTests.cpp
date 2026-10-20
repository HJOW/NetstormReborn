// 최초98행의 전용 창/HTML 부모 전체 경로를 실제 제품 몸체와 명시 GUI 경계로 대조한다.
#include "GumpRawSupport.h"
#include "client/QuickHelpWindow.h"
#include "client/QuickHelpHtml.h"
#include "client/QuickHelpRequest.h"
#include <map>
using namespace netstorm::client;
using namespace netstorm::test::gumpraw;
namespace {
constexpr std::uint32_t kPool=0x12000000,kWindow=kPool+0x40000,kDecoration=kPool+0x41000,kInner=kPool+0x42000,kPictureBg=kPool+0x43000,kBodyBg=kPool+0x44000,kBody=kPool+0x45000,kLearn=kPool+0x4a000,kBack=kPool+0x4b000,kCancel=kPool+0x4c000,kInfo=kPool+0x4d000,kPicture=kPool+0x4e000,kScroll=kPool+0x4f000,kTable=kPool+0x51000,kSource=0x13000000; // 독립 원문이 사용한 의미별 호스트 식별값이다.
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
        h.constructBackground=[this](auto token,float width,float height,auto background,auto flags,auto type,auto image) { Event("Background,"+std::to_string(token)+','+std::to_string(Bits(width))+','+std::to_string(Bits(height))+','+std::to_string(background)+','+std::to_string(flags)+','+std::to_string(type)+','+std::to_string(image));Fill(token,width,height,flags); };
        h.constructHtml=[this](auto token) { html->Construct(token); };
        h.constructButton=[this](auto token,auto width,auto height,std::string_view label,auto unused,auto context) { Event("Stone,"+std::to_string(token)+','+std::to_string(width)+','+std::to_string(height)+','+TextHex(label)+','+std::to_string(unused)+','+std::to_string(context));Fill(token,60,20,0); };h.fitButton=[this](auto token) { Event("Fit,"+std::to_string(token)); };
        h.constructInformation=[this](auto token,auto width,auto height,auto font,auto color) { ConstructText(token,width,height,font,color); };
        h.constructPicture=[this](auto token,auto type,auto image,auto mode) { Event("Picture,"+std::to_string(token)+','+std::to_string(type)+','+std::to_string(image)+','+std::to_string(mode));Fill(token,99,88,1); };
        h.translate=[](std::string_view value) { return std::string(value); };h.config=[this](std::string_view key) { Event("Config,"+TextHex(key));return std::string("d/help.english"); };
        h.load=[this](std::string_view name) -> std::optional<std::string> { Event("Load,"+TextHex(name)+",0,2");return failure ? std::nullopt : std::optional<std::string>(helpText); };h.loadError=[this](std::string_view name) { Event("LoadError,9,"+TextHex(name)); };
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
        return result+'|'+std::to_string(state.id)+','+std::to_string(state.helpData ? kSource : 0U)+','+std::to_string(popup.command)+'|'+TextHex(popup.Argument())+'|'+trace;
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
