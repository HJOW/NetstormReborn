// 전체 원본 생성/현재 재조회·기본 주제 우선순위와 그림 유무별 배치를 보존한다.
#include "client/QuickHelpWindow.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <new>
#include <stdexcept>
namespace netstorm::client {
namespace {
// 선택한 실제 하위 몸체가 없으면 빈 성공으로 숨기지 않는다.
template<class F> const F& Need(const F& effect) { if (!effect) throw std::logic_error("Quick Help 전용 창 실제 효과 누락");return effect; }
// 원본 C문자열의 첫NUL 뒤는 파일 경로/주제로 해석하지 않는다.
std::string_view CText(std::string_view value) { return value.substr(0,value.find('\0')); }
// 실제 ftol의 signed64 절삭 뒤 하위 DWORD다. indefinite의 하위 값은0이다.
std::int32_t Truncate(double value) { if (!std::isfinite(value) || value<-0x1p63 || value>=0x1p63) return 0;return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value))); }
// 버튼 배치의 signed32 정수 더하기는 DWORD로 감긴다.
std::int32_t Add(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(a)+std::bit_cast<std::uint32_t>(b)); }
// 원본의 엄격한 > 비교이며 동률/NaN에서는 둘째 값을 선택한다.
double Larger(double a,double b) { return a>b ? a : b; }
}
// 상태/노드 소유권은 기존 Client/공통 트리에 둔다.
QuickHelpWindow::QuickHelpWindow(QuickHelpWindowState& state,QuickHelpPopupState& popup,QuickHelpWindowHooks hooks):state_(state),popup_(popup),hooks_(std::move(hooks)) {}
// 호스트 식별 토큰은 원본 포인터 크기 검사의 대상이 아니며 실제 저장소 존재로 검사한다.
QuickHelpContentNode& QuickHelpWindow::Node(std::uint32_t token) const { auto* result=Need(hooks_.node)(token);if (!result) throw std::out_of_range("전용 Help 창 노드 누락");return *result; }
// 자식 토큰도 같은 공통 기본 필드의 주소를 반환해야 한다.
GumpCreateNode& QuickHelpWindow::Base(std::uint32_t token) const { auto* result=Need(hooks_.base)(token);if (!result) throw std::out_of_range("전용 Help 현재 자식 누락");return *result; }
// 부모 body 필드의 현재 토큰으로 실제 배열을 재조회한다.
HtmlBodyGumpNode& QuickHelpWindow::Body(std::uint32_t token) const { auto* result=Need(hooks_.html)(Node(token).body);if (!result) throw std::out_of_range("전용 Help HTML 부모 누락");return *result; }
// 부분 생성 이전의 이미 적용한 효과를 임의로 되돌리지 않는다.
std::uint32_t QuickHelpWindow::Allocate(std::size_t bytes) const { const auto token=Need(hooks_.allocate)(bytes);if (!token) throw std::bad_alloc();return token; }
// FST가 저장한 float를 다시 읽지 않고 같은 현재 계산값으로 배율 폭을 쓴다.
void QuickHelpWindow::Width(std::uint32_t token,double value) const { auto& node=Base(token);node.baseWidth=static_cast<float>(value);node.width=static_cast<float>(value*node.scale); }
// 높이도 각 원본 FST/FSTP의 단정도 저장 시점만 좁힌다.
void QuickHelpWindow::Height(std::uint32_t token,double value) const { auto& node=Base(token);node.baseHeight=static_cast<float>(value);node.height=static_cast<float>(value*node.scale); }
// 원본과 같은 세 배경·HTML 부모·OK 또는 Sacrifice/Back/선택 Cancel을 생성한다.
std::uint32_t QuickHelpWindow::Construct(std::uint32_t token,std::uint32_t learnType) const {
    Need(hooks_.constructBase)(token,0,0,0,1);Node(token).vtable=0x514ec4;Node(token).learnType=learnType;Node(token).stream=0;
    if (Need(hooks_.scripted)()) popup_.Mark(state_.scriptCommand,CText(state_.scriptName));
    const auto decoration=Allocate(0xc4);Need(hooks_.constructDecoration)(decoration,0,0);Node(token).decoration=decoration;Base(Node(token).decoration).alignFlags|=3;Base(Node(token).decoration).secondaryDepth=1;Need(hooks_.attach)(Node(token).decoration,0,0,token,0);
    const auto color=Need(hooks_.nearestColor)(55,51,54);const auto inner=Allocate(0xb8);Need(hooks_.constructBackground)(inner,0,0,0x04000000U|color,1,0,0);Node(token).innerBackground=inner;Need(hooks_.attach)(Node(token).innerBackground,0,0,token,0);
    const auto pictureBackground=Allocate(0xb8);Need(hooks_.constructBackground)(pictureBackground,0,0,0x03e40000,1,state_.backgroundType,state_.backgroundFrame);Node(token).pictureBackground=pictureBackground;Need(hooks_.attach)(Node(token).pictureBackground,0,0,token,0);
    const auto bodyBackground=Allocate(0xb8);Need(hooks_.constructBackground)(bodyBackground,0,0,0x03e40000,1,state_.backgroundType,state_.backgroundFrame);Node(token).bodyBackground=bodyBackground;Need(hooks_.attach)(Node(token).bodyBackground,0,0,token,0);Node(token).information=0;
    const auto body=Allocate(0x40a4);Need(hooks_.constructHtml)(body);Node(token).body=body;Need(hooks_.attach)(Node(token).body,0,0,token,0);
    const auto learn=Allocate(0x170);const auto label=Need(hooks_.translate)(learnType ? "Sacrifice" : "OK");Need(hooks_.constructButton)(learn,0,0,CText(label),state_.buttonUnused,0);Node(token).learnButton=learn;Need(hooks_.fitButton)(Node(token).learnButton);Width(Node(token).learnButton,static_cast<double>(Base(Node(token).learnButton).baseWidth)*1.2);Need(hooks_.attach)(Node(token).learnButton,0,0,token,0);
    const auto back=Allocate(0x170);const auto backLabel=Need(hooks_.translate)("Back");Need(hooks_.constructButton)(back,0,0,CText(backLabel),state_.buttonUnused,0);Node(token).backButton=back;Need(hooks_.fitButton)(Node(token).backButton);Need(hooks_.attach)(Node(token).backButton,0,0,token,0);Node(token).cancelButton=0;
    if (Node(token).learnType) { const auto cancel=Allocate(0x170);const auto cancelLabel=Need(hooks_.translate)("Cancel");Need(hooks_.constructButton)(cancel,0,0,CText(cancelLabel),state_.buttonUnused,0);Node(token).cancelButton=cancel;Need(hooks_.fitButton)(Node(token).cancelButton);Need(hooks_.attach)(Node(token).cancelButton,0,0,token,0); }
    Node(token).command=static_cast<std::uint32_t>(static_cast<std::int32_t>(state_.kind));
    if (!state_.helpData) { const auto name=std::string(CText(Need(hooks_.config)("Quick Help Spec")));if (name.size()>=256) throw std::length_error("Quick Help 원본 파일명 버퍼 초과");auto data=Need(hooks_.load)(name);if (data) { data->push_back('\n');state_.helpData=std::move(data); }else Need(hooks_.loadError)(name); }
    state_.id=Node(token).serial;Width(token,450);Height(token,350);Node(token).flags|=0x800;Node(token).picture=0;return token;
}
// 현재 그림/표시 자식을 먼저 반납하고 새 정보 pane을 실제 다중 줄로 생성한다.
void QuickHelpWindow::Reset(std::uint32_t token) const {
    if (Node(token).picture) { Need(hooks_.kill)(Node(token).picture);Node(token).picture=0; }Need(hooks_.clearBody)(Node(token).body);if (Node(token).information) Need(hooks_.kill)(Node(token).information);
    const auto pane=Allocate(0xd8);Need(hooks_.constructInformation)(pane,0,0,5,state_.color);Node(token).information=pane;Need(hooks_.attach)(Node(token).information,0,0,token,0);
}
// 그림→버튼→큰 본문 배경/안쪽 배경→HTML 부모→정보 순으로 실제 자식을 Push/부착한다.
void QuickHelpWindow::Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const {
    Need(hooks_.baseAttach)(token,x,y,parent,mode);
    if (Node(token).picture) {
        Need(hooks_.push)(Node(token).pictureBackground);Width(Node(token).pictureBackground,157);Height(Node(token).pictureBackground,126);Need(hooks_.attach)(Node(token).pictureBackground,15,17,token,0);
        Need(hooks_.push)(Node(token).picture);Width(Node(token).picture,153);Height(Node(token).picture,122);Need(hooks_.attach)(Node(token).picture,2,2,Node(token).pictureBackground,0);
    }
    const auto height=Truncate(Base(Node(token).learnButton).baseHeight);const auto width=[&](std::uint32_t child) { return child ? static_cast<double>(Base(child).baseWidth) : 0.0; }; // 필수 Learn의 높이와 선택 버튼의 현재 폭이다.
    const auto widest=Truncate(Larger(width(Node(token).backButton),Larger(width(Node(token).learnButton),width(Node(token).cancelButton))));std::int32_t count{};
    // 원본의 Back/Learn/Cancel 순서로 폭을 통일하고 실제 개수를 센다.
    for (const auto child:{Node(token).backButton,Node(token).learnButton,Node(token).cancelButton}) if (child) { Width(child,widest);++count; }
    auto offset=Truncate((static_cast<double>(Node(token).baseWidth)-static_cast<double>(count)*widest-static_cast<double>(count)*10)*0.5);
    // 각 Push 뒤에는 현재 창 높이/현재 버튼 토큰을 다시 읽는다.
    for (std::size_t index=0;index<3;++index) {
        const auto current=[&] { return index==0 ? Node(token).backButton : index==1 ? Node(token).learnButton : Node(token).cancelButton; }; // 같은 현재 자식 필드다.
        if (!current()) continue;Need(hooks_.push)(current());const auto bottom=Add(Add(Truncate(Node(token).baseHeight),-height),-10);Need(hooks_.attach)(current(),static_cast<float>(offset),static_cast<float>(bottom),token,0);if (index!=2) offset=Add(Add(offset,10),widest);
    }
    Need(hooks_.push)(Node(token).bodyBackground);Width(Node(token).bodyBackground,(static_cast<double>(Node(token).baseWidth)-15)-15);const double top=Node(token).picture ? 153.0 : 17.0;Height(Node(token).bodyBackground,(((static_cast<double>(Node(token).baseHeight)-top)-10)-10)-height);Need(hooks_.attach)(Node(token).bodyBackground,17,static_cast<float>(top),token,0);
    Need(hooks_.push)(Node(token).innerBackground);Width(Node(token).innerBackground,(static_cast<double>(Base(Node(token).bodyBackground).baseWidth)-2)-2);Height(Node(token).innerBackground,(static_cast<double>(Base(Node(token).bodyBackground).baseHeight)-2)-2);Need(hooks_.attach)(Node(token).innerBackground,2,2,Node(token).bodyBackground,0);
    Need(hooks_.push)(Node(token).body);Width(Node(token).body,Base(Node(token).innerBackground).baseWidth);Height(Node(token).body,Base(Node(token).innerBackground).baseHeight);Need(hooks_.attach)(Node(token).body,0,0,Node(token).innerBackground,0);
    if (Node(token).information) { Need(hooks_.push)(Node(token).information);if (!Need(hooks_.lines)(Node(token).information)) Base(Node(token).information).flags&=~1U;Width(Node(token).information,static_cast<double>(Node(token).baseWidth)-197);Height(Node(token).information,126);Need(hooks_.attach)(Node(token).information,182,15,token,0); }
}
// 같은 실제 body 배열을 치환하고 TYPE+120을 치환 뒤 읽어 그림을 선택한다.
void QuickHelpWindow::Show(std::uint32_t token,std::uint32_t type,std::string_view topic) const {
    topic=CText(topic);if (Node(token).picture) static_cast<void>(Base(Node(token).picture));std::string selected(topic);
    if (!type) { if (topic.empty()) selected=state_.battle ? "pyrosphereHelp" : state_.challenge ? "challengeHelp" : "introHelp";else if (topic.size()>=80) throw std::length_error("Quick Help 원본 주제80바이트 초과"); }
    Reset(token);if (type) state_.infoMode=1;Need(hooks_.render)(Node(token).body,type,selected,Node(token).information,Node(token).stream);state_.infoMode=0;
    auto& output=Body(token).body;const auto end=std::find(output.begin(),output.end(),0);if (end==output.end()) throw std::length_error("Quick Help 본문 NUL 누락");const std::string input(output.begin(),end);auto text=Need(hooks_.substitute)(input);text.resize(CText(text).size()); // 원본 local_2088의 같은 배열 주소를 치환 뒤에도 유지한다.
    if (Node(token).picture) static_cast<void>(Base(Node(token).picture));if (text.empty()) text=Need(hooks_.translate)("No help information available");text.resize(CText(text).size());if (text.size()>=kHtmlBodyCapacity) throw std::length_error("Quick Help 본문8192바이트 초과");std::copy(text.begin(),text.end(),output.begin());output[text.size()]=0;
    const auto frame=type ? Need(hooks_.helpFrame)(type) : std::nullopt;
    if (!frame || *frame==-1) { if (Node(token).picture) { static_cast<void>(Base(Node(token).picture));Need(hooks_.kill)(Node(token).picture);Node(token).picture=0; } }
    else if (type!=state_.noPictureA && type!=state_.noPictureB) { const auto picture=Allocate(0xb4);Need(hooks_.constructPicture)(picture,type,std::bit_cast<std::uint32_t>(*frame),8);Node(token).picture=picture;Need(hooks_.attach)(Node(token).picture,0,0,token,0); }
    Need(hooks_.bodyText)(Node(token).body,std::string_view(output.data(),text.size()));
}
}
