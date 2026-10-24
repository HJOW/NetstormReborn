// ChatWindow는 root Draw를 칠하지 않으며 실제 네 자식/공유 버퍼/controlled 메뉴 본체를 구성한다.
#include "client/ChatWindow.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
constexpr std::uint32_t kChatVtable=0x504a94,kVisible=1,kCodeMask=0x03ffffff,kCommand=0x02000000,kRelease=0x80000000; // 실제10.78 표/기본 비트다.
// 도달한 장치를 빈 성공으로 바꾸지 않는다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("ChatWindow의 실제 하위 연결 누락");return function; }
// 원본 C문자열은 첫 NUL까지만 사용한다.
std::string_view Z(std::string_view text) { const auto end=text.find('\0');return text.substr(0,end); }
// 실제 편집 API가 빌리는 안정된 NUL 포함 입력이다.
std::vector<char> Terminated(std::string_view text) { text=Z(text);std::vector<char> result(text.begin(),text.end());result.push_back(0);return result; }
// DWORD 감김을 정의된 호스트 산술로 표현한다.
std::int32_t Sub(std::int32_t first,std::int32_t second) { return std::bit_cast<std::int32_t>(std::uint32_t(first)-std::uint32_t(second)); }
// 원본 CRT _ftol의 signed64 절삭 후 하위 DWORD다.
std::int32_t Ftol(double value) { if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;return std::bit_cast<std::int32_t>(std::uint32_t(static_cast<std::int64_t>(value))); }
// 이름 lookup에 쓰는 실제 subscriber DWORD만 원문 record+4에 쓴다.
void Subscriber(o::PlayerIdentityRecord& record,std::uint32_t value) { for (std::size_t i=0;i<4;++i) record[4+i]=static_cast<std::uint8_t>(value>>(i*8)); } // 비정렬 little endian이다.
}
// 외부 전체 객체를 복사하지 않고 같은 현재 root/기본 저장소를 조회한다.
ChatWindow::ChatWindow(ChatWindowState& state,ChatWindowHooks hooks):state_(state),hooks_(std::move(hooks)) { Need(hooks_.chat);Need(hooks_.base); }
// 실제 base 주소와 root 파생 주소는 같은 객체여야 한다.
HudChatNode& ChatWindow::Node(std::uint32_t token) const { auto* node=token ? hooks_.chat(token) : nullptr;if (!node || static_cast<GumpCreateNode*>(node)!=hooks_.base(token)) throw std::runtime_error("ChatWindow 공유 root 없음");return *node; }
// 실제 사용 시점의 자식 포인터를 읽는다.
GumpCreateNode& ChatWindow::Base(std::uint32_t token) const { auto* node=token ? hooks_.base(token) : nullptr;if (!node) throw std::runtime_error("ChatWindow 실제 자식 없음");return *node; }
// Console은 같은 기본 Gump 저장소를 공유한다.
ConsoleGumpNode& ChatWindow::Console(std::uint32_t token) const { const auto child=Node(token).console;auto* node=child ? Need(hooks_.console)(child) : nullptr;if (!node || static_cast<GumpCreateNode*>(node)!=&Base(child)) throw std::runtime_error("ChatWindow 실제 Console 없음");return *node; }
// 로그도 같은 실제 ScrollingText 저장소여야 한다.
ScrollingTextGumpNode& ChatWindow::Text(std::uint32_t token) const { const auto child=Node(token).text;auto* node=child ? Need(hooks_.text)(child) : nullptr;if (!node || static_cast<GumpCreateNode*>(node)!=&Base(child)) throw std::runtime_error("ChatWindow 실제 ScrollingText 없음");return *node; }
// 원본처럼 각 호출마다 현재 +108을 다시 읽는다.
TextEntryBuffer& ChatWindow::Buffer(std::uint32_t token) const { const auto address=Console(token).buffer;auto* buffer=address ? Need(hooks_.buffer)(address) : nullptr;if (!buffer) throw std::runtime_error("ChatWindow 현재 편집 버퍼 없음");return *buffer; }
// 포인터만 채운 임시 Console로 성공하지 않게 실제 편집 본체를 요구한다.
const TextEntryGump& ChatWindow::Entry() const { if (!hooks_.entry) throw std::runtime_error("ChatWindow 실제 TextEntry 본체 없음");return *hooks_.entry; }
// 사건·색·모드·원문 포인터를 사용하는 위치마다 현재 값을 조회한다.
std::uint32_t ChatWindow::Global(std::uint32_t address) const { return Need(hooks_.global)(address); }
// 현재 입력의 전체NUL이128BYTE를 넘는 원문 stack overflow는 호스트에서 거부한다.
std::string ChatWindow::CopyInput(std::uint32_t token) const { std::array<char,128> bytes{};if (Entry().CopyText(Node(token).console,bytes)==0xffffffffU) throw std::runtime_error("ChatWindow 원본 입력 복사128 범위");const auto end=std::find(bytes.begin(),bytes.end(),0);if (end==bytes.end()) throw std::runtime_error("ChatWindow 원본 입력 NUL 없음");return {bytes.begin(),end}; }
// 원본 접두의 미작성 꼬리는 덮지 않는다.
void ChatWindow::Prefix(std::uint32_t token,std::string_view text) const { text=Z(text);auto& prefix=Node(token).inputPrefix;if (text.size()>=prefix.size()) throw std::runtime_error("ChatWindow 원본 접두128 범위");std::copy(text.begin(),text.end(),prefix.begin());prefix[text.size()]=0; }
//43f820/4bc060의 실제 자식 생성과 같은 버퍼 소유 교체를 원문 순서대로 연결한다.
std::uint32_t ChatWindow::Construct(std::uint32_t token,std::int32_t width,std::int32_t height) const {
    auto& node=Node(token);Need(hooks_.constructBase)(token,static_cast<float>(width),static_cast<float>(height));node.vtable=kChatVtable;node.command=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(Global(0x540424)))));
    auto child=Need(hooks_.allocate)(0x170);if (child) Need(hooks_.constructHelper)(child,">",Global(0x5acd24));node.helper=child;Need(hooks_.fitHelper)(node.helper);
    const auto helperTitle=Need(hooks_.translate)("Chat Helper");Need(hooks_.title)(Node(token).helper,helperTitle);Need(hooks_.help)(Node(token).helper,"chathelper");
    node.baseWidth=static_cast<float>(width);node.width=static_cast<float>(double(width)*node.scale);node.baseHeight=static_cast<float>(height);node.height=static_cast<float>(double(height)*node.scale);node.helperVisible=1;
    const auto metric=Need(hooks_.measure)(token,"M");node.capacity=128;node.leftPadding=0;child=Need(hooks_.allocate)(0xb8);
    if (child) Need(hooks_.constructBackground)(child,static_cast<float>(Sub(width,20)),static_cast<float>(Sub(height,metric[1])),Global(0x5b5dc0)|0x04000000U);node.background=child;
    child=Need(hooks_.allocate)(0x128);if (child) Entry().ConstructConsole(child,Sub(width,20),metric[1],{},5,Global(0x5b5e60)|0x04000000U,Global(0x5b5dd4));node.console=child;Console(token).changeNotice=1;Console(token).maxLength=80;
    child=Need(hooks_.allocate)(0xd8);if (child) Need(hooks_.constructScrolling)(child,Sub(width,20),Sub(height,metric[1]),5,Global(0x5b5dd8));node.text=child;
    Need(hooks_.replaceBuffer)(Node(token).text,Need(hooks_.sharedBuffer)(),0);Text(token).callback=1;
    Need(hooks_.attach)(Node(token).background,static_cast<float>(Node(token).leftPadding),0,token,0);
    Need(hooks_.attach)(Node(token).console,static_cast<float>(Node(token).leftPadding),static_cast<float>(Sub(height,metric[1])),token,0);
    Need(hooks_.attach)(Node(token).helper,static_cast<float>(Node(token).leftPadding),static_cast<float>(Sub(height,metric[1])),token,0);
    Need(hooks_.attach)(Node(token).text,0,0,Node(token).background,0);Need(hooks_.setSlider)(Node(token).text,1);Need(hooks_.setScrollable)(Node(token).text,1);Text(token).textFlags&=~0x10U;
    Console(token).multiline=0;Console(token).flags&=~kVisible;node.flags|=0xe00;node.inputPrefix[0]=0;Entry().SetText(Node(token).console,Terminated(""));NormalizePrefix(token);Console(token).autocomplete=0x42e540;
    if (!state_.completionReady) Completions();state_.completionReady=1;return token;
}
// clear/quit 명령은 처음 한 번만 등록하고 같은 현재 로그 버퍼에 실제 append callback을 설치한다.
std::uint32_t ChatWindow::Create() const {
    if (!state_.registered) { Need(hooks_.registerCommand)("clear",0x42f680,1);Need(hooks_.registerCommand)("quit",0x42e520,1);state_.registered=1; }
    const auto token=Need(hooks_.allocate)(0x13c);state_.current=token ? Construct(token,200,100) : 0;if (!state_.current) throw std::runtime_error("ChatWindow 원본 new 실패 뒤 null 자식 접근");
    Need(hooks_.bufferAppendCallback)(Text(state_.current).buffer,0x42f590);return state_.current;
}
// 실제 표시 재부착의 수식/콜백 순서는 이미 동결한 기하 본체를 사용한다.
void ChatWindow::Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const { if (!hooks_.geometry) throw std::runtime_error("ChatWindow 실제 Attach 본체 없음");hooks_.geometry->Attach(token,x,y,parent,mode); }
// 이 목록은 색 번호를 의미 색 이름으로 재계산하지 않고 원문 문자열 그대로 만든다.
void ChatWindow::Completions() const {
    for (std::uint32_t color=0;color<256;++color) { state_.colors.push_back("~[C."+std::to_string(color)+']');state_.backgrounds.push_back("~[B."+std::to_string(color)+']'); } // 두 실제256개 문자열 목록이다.
    if (state_.completionReady) return;
    for (std::int32_t type=0;type<std::bit_cast<std::int32_t>(Global(0x541348));++type) { // 원문은 TYPE 수를 각 반복에 다시 읽는다.
        const auto count=Need(hooks_.frameCount)(std::uint32_t(type)); // 이 TYPE의 프레임 코드 수는 안쪽 반복 전에 한 번 읽는다. SHP 추가 레이어 수와 구별한다.
        for (std::int32_t frame=0;frame<count;++frame) { const auto height=Need(hooks_.frameHeight)(std::uint32_t(type),std::uint32_t(frame));if (height>7 && height<17) state_.icons.push_back("~[I"+std::to_string(type)+'.'+std::to_string(frame)+']'); } // 높이8..16만 아이콘 후보로 저장한다.
    }
}
// 접두와 본문은 실제 입력을 두 번 복사한 뒤 다시 합친다.
void ChatWindow::NormalizePrefix(std::uint32_t token) const {
    auto prefix=CopyInput(token);const auto colon=prefix.find(':');if (colon==prefix.npos) prefix.clear();else prefix.resize(colon);if (!prefix.empty()) prefix+=':';
    auto body=CopyInput(token);const auto secondColon=body.find(':');if (secondColon!=body.npos) body.erase(0,secondColon+1);
    Entry().SetText(Node(token).console,Terminated(prefix));Entry().AppendText(Node(token).console,Terminated(body),false);Prefix(token,prefix);
}
// 원문 수신자 목록은 대소문자를 바꾸지 않는 strstr/공백 구분 문자열이다.
void ChatWindow::ChangeRecipient(std::uint32_t token,std::optional<std::string_view> name,std::uint32_t operation) const {
    if (operation>1 && hooks_.assertion) hooks_.assertion(0x561);if (!name) return;const auto target=Z(*name);auto prefix=CopyInput(token);const auto colon=prefix.find(':');bool valid=true;
    if (colon!=prefix.npos) { prefix.resize(colon);valid=Need(hooks_.recipient)(prefix); }else prefix.clear();if (!valid) prefix.clear();
    const auto found=prefix.find(target);
    if (operation==0 && found==prefix.npos) { if (!prefix.empty() && prefix.back()!=' ') prefix+=' ';prefix+=target;prefix+=' '; }
    else if (operation==1 && found!=prefix.npos) { auto end=found;while (end<prefix.size() && prefix[end]!=' ') ++end;while (end<prefix.size() && prefix[end]==' ') ++end;prefix.erase(found,end-found); } // 먼저 찾은 substring에서 다음 비공백까지 지운다.
    if (!prefix.empty()) prefix+=':';auto body=CopyInput(token);const auto bodyColon=body.find(':');if (bodyColon!=body.npos && valid) body.erase(0,bodyColon+1);
    Entry().SetText(Node(token).console,Terminated(prefix));Entry().AppendText(Node(token).console,Terminated(body),false);Prefix(token,prefix);
}
// command 분기에는 일반 release 관문을 추가하지 않으며 기본 Help 위임 순서도 원문을 따른다.
std::uint32_t ChatWindow::Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second,std::optional<std::string_view> text) const {
    auto& node=Node(token);const auto code=event&kCodeMask;const auto base=[&] { return Need(hooks_.baseHandle)(token,event,first,second); }; // 나머지는 현재 기본 Help 입력에 위임한다.
    if (!(event&kCommand)) {
        if (code!=0x10000 && code!=0x20000 && !(event&kRelease)) {
            if (code==0x240000) { Need(hooks_.requestScroll)(Node(token).text,0);return 1; }
            if (code==0x230000) { Need(hooks_.requestBottom)(Node(token).text);return 1; }
            if (code==0x210000 || code==0x220000 || code==0x260000 || code==0x280000) { const auto delta=code==0x210000 ? Ftol(-double(node.baseHeight)) : code==0x220000 ? Ftol(node.baseHeight) : code==0x260000 ? -5 : 5;Need(hooks_.relativeScroll)(Node(token).text,delta);return 1; }
        }
        return base();
    }
    if (code==Global(0x542b24)) {
        if (!text) throw std::runtime_error("ChatWindow 삽입 사건의 실제 문자열 없음");const auto value=Z(*text);
        if (value=="copy") Buffer(token).Copy();else if (value=="paste") Buffer(token).Paste();else Entry().AppendText(Node(token).console,Terminated(value),false);
    }
    if (code==Global(0x5427a8) && first==Base(Node(token).helper).serial) HelperMenu(token);
    if (code==Global(0x5429f0)) { if (state_.current) Need(hooks_.bubble)(state_.current,Global(0x542914),0,1);return 1; }
    if (code==Global(0x5427d8) && first==Base(Node(token).console).serial) {
        Entry().AppendText(Node(token).console,Terminated("\n"),false);if (Buffer(token).State().length!=-2) Need(hooks_.sendChat)(Global(0x548714),Buffer(token).Text());
        auto prefix=CopyInput(token);const auto colon=prefix.find(':');if (colon!=prefix.npos) { prefix.resize(colon);if (Need(hooks_.recipient)(prefix)) prefix+=':';else prefix.clear(); }else prefix.clear();
        Prefix(token,prefix);Entry().SetText(Node(token).console,Terminated(prefix));NormalizePrefix(token);return 1;
    }
    const bool add=code==Global(0x542794);const bool remove=!add && code==Global(0x542af8); // 원본은 add가 아니어야 remove 사건을 조회한다.
    if (add || remove) {
        o::PlayerIdentityRecord record{};o::RawPlayerIdentity::Initialize(record,0);Subscriber(record,second);
        if (Need(hooks_.resolve)(record)) { const auto begin=reinterpret_cast<const char*>(record.data()+8);const auto end=std::find(begin,begin+40,0);ChangeRecipient(token,std::string_view(begin,static_cast<std::size_t>(end-begin)),remove ? 1U : 0U);return 1; }
        Need(hooks_.log)(0x13,"Could not lookup a name for subId == %d\n",first);return 1;
    }
    if (code==Global(0x54298c)) { Need(hooks_.push)(token);Need(hooks_.attach)(token,Node(token).x,Node(token).y,0,0); }
    return base();
}
// 원본은 현재 전역 root를 조건 없이0으로 쓰고 공통 자식 우선 수명에 나머지를 맡긴다.
void ChatWindow::Destruct(std::uint32_t token) const { Node(token).vtable=kChatVtable;state_.current=0;Need(hooks_.destructBase)(token); }

// 실제 helper 메뉴는 하위 메뉴를 완성한 뒤 현재 controlled 스택에 연결한다.
void ChatWindow::HelperMenu(std::uint32_t token) const {
    static_cast<void>(token);const auto begin=[&] { Need(hooks_.menuBegin)(); };const auto tr=[&](std::string_view value) { return Need(hooks_.translate)(value); }; // 현재 builder/번역 조회다.
    const auto item=[&](std::string_view label,std::uint32_t command,std::optional<std::string_view> payload) { Need(hooks_.menuItem)(label,command,payload); }; // include/enabled1·force0이다.
    const auto translatedItem=[&](std::string_view label,std::uint32_t address,std::optional<std::string_view> payload) { const auto translated=tr(label);const auto command=Global(address);item(translated,command,payload); }; // 번역 callback 뒤 현재 사건 전역을 읽는다.
    const auto sub=[&](std::string_view label) { const auto translated=tr(label);const auto child=Need(hooks_.menuTake)();Need(hooks_.menuSubmenu)(translated,child); }; // 원문은 번역 뒤 실제 현재 메뉴를 pop한다.
    const auto toggle=[&](std::string_view key,std::string_view label) { const auto first=Need(hooks_.config)(key);const auto assignment=std::string(key)+(first ? "=0" : "=1");const auto translated=tr(label);const auto command=Global(0x5428ac);const auto checked=Need(hooks_.config)(key);Need(hooks_.menuChecked)(std::uint32_t(checked),translated,command,assignment); }; // 원문은 두 번째 설정 조회 전에 사건값을 스택에 저장한다.
    begin();Need(hooks_.menuTitle)(tr("Chat Helper:"));
    if (Global(0x594fc8) || Global(0x594fbc)) {
        begin();translatedItem("Edit Back Format",0x542b08,{});translatedItem("Edit Front Format",0x542b0c,{});sub("Format");
        begin();toggle("allowzonesounds","Allow Chat Sound");toggle("zonebgcolor","Allow Background Colors");toggle("zonefgcolor","Allow Foreground Colors");toggle("zoneoffset","Allow Embossed Offset");sub("Chat Effects ");
        if (Global(0x594fbc)) {
            begin();begin();toggle("ShowEveryone","Show General Channel");toggle("ShowAllies","Show Allies Channel");toggle("ShowObserver","Show Observer Channel");toggle("ShowWisper","Show Whisper Channel");sub("Channel Controls");
            begin();toggle("ShowMyAllies","Show Allies Chat");toggle("ShowMyEmenys","Show Enemies Chat");toggle("ShowMyNeutrals","Show Observers Chat");sub("Player Controls");toggle("ShowChatWindow","Open Window on Message");toggle("PlaySoundMessage","Play sound on Message");sub("Chat Controls");
        }
    }
    begin();begin();
    const auto grouped=[&](std::vector<std::string>& list,std::uint32_t step,std::string_view prefix,std::string_view title) {
        if (list.empty()) throw std::runtime_error("ChatWindow 원본 helper의 빈 자동완성 벡터 범위");bool secondGroup{};std::uint32_t index{};
        for (;index<std::uint32_t(list.size()-1);++index) { // 원문은 매 반복 현재 size를 읽고 마지막 원소를 제외한다.
            if (!index) begin();const auto value=list.at(index);const auto label=value+'~'+value;item(label,Global(0x542b24),value);
            if (index && index%step==0) { const auto first=index-(step==23 || secondGroup ? step-1 : step);secondGroup=true;sub(std::string(prefix)+std::to_string(first)+" to "+std::to_string(index));begin(); }
        }
        const auto size=std::uint32_t(list.size());sub(std::string(prefix)+std::to_string(size-(size-1U)%step)+" to "+std::to_string(size-2U));sub(title);begin();
    }; // 색/아이콘의 실제 그룹 크기와 마지막 제목 수식을 유지한다.
    grouped(state_.colors,21,"Colors ","Text Colors");grouped(state_.backgrounds,21,"BGColors ","Background Colors");grouped(state_.icons,23,"Icons ","Icons");
    item("Mimic",Global(0x542b24),"~[]");item("Sound",Global(0x542b24),"~[S.name]");item("Icon",Global(0x542b24),"~[I.name]");item("Emboss Offset",Global(0x542b24),"~[E.X#xY#]");sub("KeyWords");begin();
    constexpr std::array<std::pair<std::string_view,std::string_view>,8> styles{{{"~BBold","~B"},{"~EEmboss","~E"},{"~IItalic","~I"},{"~SStrikeout","~S"},{"~UUnderline","~U"},{"~.Reset Style","~."},{"~NReset Color & Emboss","~N"},{"~*Reset to Default","~*"}}}; // 실제 스타일 항목 순서다.
    for (const auto& [label,value]:styles) translatedItem(label,0x542b24,value);sub("Styles");begin(); // 개별 기본 스타일 문자열을 입력에 삽입한다.
    constexpr std::array<std::pair<std::string_view,std::string_view>,20> colors{{{"~kBlack","~k"},{"~xBrown","~x"},{"~qDark Tan","~q"},{"~jLight Tan","~j"},{"~rRed","~r"},{"~gGreen","~g"},{"~uLight Green","~u"},{"~dTeal","~d"},{"~bBlue","~b"},{"~lLight Blue","~l"},{"~mMagenta","~m"},{"~sDark Magenta","~s"},{"~pPink","~p"},{"~yYellow","~y"},{"~iLight Yellow","~i"},{"~oOrange","~o"},{"~aLight Orange","~a"},{"~wWhite","~w"},{"~tSilver","~t"},{"~nReset Color","~n"}}}; // 실제20개 미리 정의된 색 문자열이다.
    for (const auto& [label,value]:colors) translatedItem(label,0x542b24,value);sub("Prefabricated Colors");begin(); // 임의 RGB 팔레트 이름으로 재계산하지 않는다.
    for (const auto value:{".front",".back",".format"}) translatedItem(value,0x542b24,value);sub("Commands");sub("Insert");translatedItem("Copy",0x542b24,"copy");translatedItem("Paste",0x542b24,"paste");Need(hooks_.menuFinalize)();Need(hooks_.menuOpen)(Global(0x5cae94),Global(0x5cae98),Global(0x563e38));
}
}
