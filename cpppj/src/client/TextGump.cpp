// 원본의 DWORD 감김·float32 저장 위치·현재 필드 재조회와 표시/선택의 서로 다른 세로 경계를 유지한다.
#include "client/TextGump.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 두 PE의 한 줄 가상 표와 다중 줄 dirty/slider 사용 비트다. 원본 주소는 실행 포인터가 아닌 식별값이다.
constexpr std::uint32_t kTextVtable=0x005144d4,kCdTextVtable=0x00505dc0,kDirty=1,kSlider=2;
// 원본 임시 레이아웃 폭과 맨 아래 판정의 실제 ROM float 값이다.
constexpr double kFitWidth=10000.0,kBottomTolerance=10.0;
// 원본 정수 ADD/SUB의 하위 DWORD를 signed로 해석하며 C++ signed overflow를 피한다.
std::int32_t Add(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(std::uint32_t(a)+std::uint32_t(b)); }
std::int32_t Subtract(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(std::uint32_t(a)-std::uint32_t(b)); }
// 원본 CRT _ftol의 signed 64비트 절삭 결과에서 하위 DWORD를 취한다. 무효 변환의 하위 DWORD는 0이다.
std::int32_t Ftol(double value) {
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
// 순환 목록만 거부하고 정상 토큰의 가상 측정/출력 호출 수와 순서는 바꾸지 않는다.
void Visit(std::set<std::uint32_t>& seen,std::uint32_t token) {
    if (!seen.insert(token).second) throw std::runtime_error("텍스트 표시 목록 순환");
}
// CD는 x87 C0만 검사해 NaN 높이에서도 행을 순회한다. 패치는 C0/C2 parity로 unordered를 제외한다.
bool BelowViewport(std::int32_t y,float height,o::OriginalEdition edition) {
    return static_cast<double>(y)<height || (edition==o::OriginalEdition::Cd1072 && std::isnan(height));
}
}
// 같은 노드를 조회하는 경계는 필수이며 실제 문자열/그림 출력 경계는 StyleText가 검사한다.
TextGump::TextGump(GumpCreateState& create,TextGumpState& state,const GumpCreate& factory,
    const GumpLifetime& lifetime,const GumpCollection& collection,GumpDrawState& clip,
    const StyleText& style,TextGumpHooks hooks)
    :create_(create),state_(state),factory_(factory),lifetime_(lifetime),collection_(collection),clip_(clip),style_(style),hooks_(std::move(hooks)) {
    if (!hooks_.node || !hooks_.baseNode) throw std::invalid_argument("TextGump 공유 저장소 필요");
}
// 기본 수집/생성과 다른 복사본을 출력하는 잘못된 연결은 실제 호출 전에 거부한다.
TextGumpNode& TextGump::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.node(token) : nullptr;
    if (!node || static_cast<GumpCreateNode*>(node)!=hooks_.baseNode(token)) throw std::runtime_error("TextGump 공유 노드 없음");
    return *node;
}
// integer를 먼저 float로 좁히지 않고 현재 scale과 곱한다. 원본 기본 생성은 기존 head를 반납하지 않는다.
std::uint32_t TextGump::Construct(std::uint32_t token,std::int32_t width,std::int32_t height,
    std::uint32_t font,std::uint32_t color) const {
    Node(token);factory_.Construct(token,0,0,0,1);auto& node=Node(token);
    const bool patch=create_.collection.edition==o::OriginalEdition::Patch1078;
    if (patch) node.vtable=kTextVtable;
    node.head=0;
    if (!patch) node.vtable=kCdTextVtable;
    node.command=static_cast<std::uint32_t>(static_cast<std::int32_t>(state_.alignment));
    node.baseWidth=static_cast<float>(width);node.width=static_cast<float>(static_cast<double>(width)*node.scale);
    node.baseHeight=static_cast<float>(height);node.height=static_cast<float>(static_cast<double>(height)*node.scale);
    node.textContext={0,font,color,0,0,0,1,1};node.flags|=kGumpDrawClip;return token;
}
// head 참조는 파생 저장소의 실제 소유 필드다. Clear 내부의 삭제/통지 순서를 그대로 사용한다.
void TextGump::Clear(std::uint32_t token) const { style_.Clear(Node(token).head); }
// 범위 안 NUL 탐색/명시 길이를 구분한다. 원본의 그 밖의 음수 길이 복사는 호스트에서 거부한다.
void TextGump::AppendText(std::uint32_t token,std::span<const char> bytes,const std::array<std::uint32_t,8>& context,
    std::int32_t length) const {
    if (length==-1) {
        const auto end=std::find(bytes.begin(),bytes.end(),'\0');
        if (end==bytes.end()) throw std::invalid_argument("TextGump NUL 범위 필요");
        const auto size=end-bytes.begin();
        if (size>std::numeric_limits<std::int32_t>::max()) throw std::length_error("TextGump 문자열 길이 초과");
        length=static_cast<std::int32_t>(size);
    }
    if (!length) return;
    if (length<0) throw std::invalid_argument("TextGump 명시 길이 음수");
    const auto fragment=style_.NewText(bytes,context,static_cast<std::uint32_t>(length));
    style_.Append(Node(token).head,fragment);
}
// 측정 콜백이 바꾼 현재 scale을 읽는다. 정수 여백은 DWORD에서 합산한 뒤 float32 크기에 저장한다.
void TextGump::Fit(std::uint32_t token,std::int32_t padWidth,std::int32_t padHeight) const {
    std::int32_t width=0,height=0;style_.Measure(Node(token).head,width,height);auto& node=Node(token);
    width=Add(width,padWidth);node.baseWidth=static_cast<float>(width);node.width=static_cast<float>(static_cast<double>(width)*node.scale);
    height=Add(height,padHeight);node.baseHeight=static_cast<float>(height);node.height=static_cast<float>(static_cast<double>(height)*node.scale);
}
// 공통 Draw에 클립 플래그가 없어도 원본 파생 Draw 자체의 내부 클립은 적용한다.
void TextGump::Draw(std::uint32_t token) const {
    if (!Node(token).head) return;
    const auto bounds=collection_.Bounds(token);const GumpClipScope guard(clip_,bounds);
    const auto position=collection_.Position(token);style_.Draw(Node(token).head,position);
}
// 두 번째 Clear는 원본 멤버 소멸 순서다. 첫 삭제 콜백이 갱신한 현재 head를 다시 읽는다.
std::uint32_t TextGump::DeleteScalar(std::uint32_t token,std::uint32_t flags) const {
    Node(token).vtable=create_.collection.edition==o::OriginalEdition::Patch1078 ? kTextVtable : kCdTextVtable;
    Clear(token);Clear(token);return lifetime_.DeleteScalar(token,flags);
}
// buffer 조회/파생·기본 조회만 생성 시 요구한다. 미복원 slider 동작을 가짜 기본값으로 대체하지 않는다.
ScrollingTextGump::ScrollingTextGump(GumpCreateState& create,const GumpCollection& collection,GumpDrawState& clip,
    StyleTextState& fragments,const StyleText& style,ScrollingTextGumpHooks hooks)
    :create_(create),collection_(collection),clip_(clip),fragments_(fragments),style_(style),hooks_(std::move(hooks)) {
    if (!hooks_.node || !hooks_.baseNode || !hooks_.buffer) throw std::invalid_argument("다중 줄 TextGump 공유 저장소 필요");
}
// 기본 생성/수집/파생 표시가 같은 주소를 사용하도록 실제 호출 시 확인한다.
ScrollingTextGumpNode& ScrollingTextGump::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.node(token) : nullptr;
    if (!node || static_cast<GumpCreateNode*>(node)!=hooks_.baseNode(token)) throw std::runtime_error("다중 줄 TextGump 공유 노드 없음");
    return *node;
}
// slider의 현재 기본 폭은 동일한 기본 Gump 저장소에서 읽는다.
GumpCreateNode& ScrollingTextGump::BaseNode(std::uint32_t token) const {
    auto* node=token ? hooks_.baseNode(token) : nullptr;
    if (!node) throw std::runtime_error("TextGump slider 노드 없음");return *node;
}
// handler/state는 같은 원본 버퍼를 나타내야 한다. 소멸 버퍼를 layout/표시하는 입력은 거부한다.
TextGumpBufferBinding ScrollingTextGump::Buffer(std::uint32_t token) const {
    const auto buffer=hooks_.buffer(Node(token).buffer);
    if (!buffer.state || !buffer.handler || !buffer.state->initialized) throw std::runtime_error("TextGump 연결 버퍼 없음");
    return buffer;
}
// 패치는 버퍼 dirty를 먼저 지우고 CD는 노드 비트를 먼저 지운다. 이후 Layout의 실패도 저장은 되돌리지 않는다.
void ScrollingTextGump::ClearDirty(std::uint32_t token) const {
    if (create_.collection.edition==o::OriginalEdition::Patch1078) {
        Buffer(token).state->dirty=0;Node(token).textFlags&=~kDirty;
    } else { Node(token).textFlags&=~kDirty;Buffer(token).state->dirty=0; }
}
// CD는 slider 폭을 뺀 값을 메모리 float에 한 번 저장한 뒤 _ftol한다. 패치는 x87 중간값을 유지한다.
void ScrollingTextGump::Layout(std::uint32_t token) const {
    const auto& node=Node(token);double width=node.baseWidth;
    if (node.textFlags&kSlider) {
        width-=BaseNode(node.slider).baseWidth;
        if (create_.collection.edition==o::OriginalEdition::Cd1072) width=static_cast<float>(width);
    }
    Buffer(token).handler->Layout(Ftol(width));
}
// 두 변경 플래그의 현재 값을 읽는다. 폭 0 이하에서 이전 표시 목록을 유지하는 동작은 버퍼에 위임한다.
void ScrollingTextGump::EnsureLayout(std::uint32_t token) const {
    if (!(Node(token).textFlags&kDirty) && !Buffer(token).state->dirty) return;
    ClearDirty(token);Layout(token);
}
// 마지막 표시 줄의 원본 합에 1을 더하며 빈 버퍼도 높이 1이 된다.
void ScrollingTextGump::FitHeight(std::uint32_t token) const {
    ClearDirty(token);Layout(token);const auto height=Add(Buffer(token).handler->TotalHeight(),1);auto& node=Node(token);
    node.baseHeight=static_cast<float>(height);node.height=static_cast<float>(static_cast<double>(height)*node.scale);
}
// 최대 폭 측정 이전에 slider 폭을 저장한다. scaled 폭은 base 폭의 float32 좁힘 전 합계로 계산한다.
void ScrollingTextGump::FitContent(std::uint32_t token,std::int32_t padWidth,std::int32_t padHeight) const {
    auto& node=Node(token);node.baseWidth=static_cast<float>(kFitWidth);node.width=static_cast<float>(kFitWidth*node.scale);
    ClearDirty(token);Layout(token);
    const double extra=Node(token).textFlags&kSlider ? BaseNode(Node(token).slider).baseWidth : 0.0;
    const double width=static_cast<double>(Buffer(token).handler->MaximumWidth())+static_cast<double>(padWidth)+extra+1.0;
    node.baseWidth=static_cast<float>(width);node.width=static_cast<float>(width*node.scale);
    const auto height=Add(Buffer(token).handler->TotalHeight(),padHeight);
    node.baseHeight=static_cast<float>(height);node.height=static_cast<float>(static_cast<double>(height)*node.scale);
}
// 문자열 통지/버퍼 추가 콜백 이후의 현재 textFlags를 갱신하고 표시 변경을 통지한다.
void ScrollingTextGump::AppendText(std::uint32_t token,std::span<const char> bytes,const std::array<std::uint32_t,8>& context,
    std::int32_t length,std::uint32_t metadata) const {
    Buffer(token).handler->AppendText(bytes,context,length,metadata);Node(token).textFlags|=kDirty;Redraw(token);
}
// 기본 문맥은 추가 전에 복사하며 onAppend가 원래 문맥을 바꾸어도 이번 입력은 복사본을 사용한다.
void ScrollingTextGump::AppendText(std::uint32_t token,std::span<const char> bytes,std::int32_t length,std::uint32_t metadata) const {
    const auto context=Node(token).textContext;AppendText(token,bytes,context,length,metadata);
}
// 그림은 버퍼 문자열 통지를 호출하지 않는다. 추가 후 현재 노드의 dirty와 가상 표시 요청만 갱신한다.
void ScrollingTextGump::AppendPicture(std::uint32_t token,const Markup& pictures,std::uint32_t type,std::uint32_t frame,
    std::uint32_t metadata,std::int32_t fitWidth,std::int32_t fitHeight) const {
    Buffer(token).handler->AppendPicture(pictures,type,frame,metadata,fitWidth,fitHeight);
    Node(token).textFlags|=kDirty;Redraw(token);
}
// 원본 교체는 완료 줄만 반복 삭제한다. 마지막 미완료 문자열도 지우는 Clear로 바꾸지 않는다.
void ScrollingTextGump::ReplaceText(std::uint32_t token,std::span<const char> bytes,std::uint32_t metadata) const {
    const auto context=Node(token).textContext;
    const auto buffer=Buffer(token);
    // 각 제거 직후 같은 버퍼의 현재 완료 개수를 다시 읽는다. 표시 복제는 다음 Layout까지 남는다.
    while (buffer.state->lineCount!=0) buffer.handler->RemoveLine(0);
    buffer.handler->AppendText(bytes,context,-1,metadata);Node(token).textFlags|=kDirty;Redraw(token);
}
// 저장한 내용/viewport는 세 콜백 동안 유지하고 slider 포인터/scroll은 각 호출 직전에 재조회한다.
void ScrollingTextGump::SetScroll(std::uint32_t token,std::int32_t position) const {
    const auto height=Buffer(token).handler->TotalHeight();const double viewport=Node(token).baseHeight;
    const auto page=Ftol(viewport<0.0 ? 0.0 : viewport);
    if (height<Add(page,position)) position=Subtract(height,page);
    if (position<0) position=0;
    Node(token).scroll=position;
    if (!hooks_.sliderRange) throw std::invalid_argument("TextGump slider range 경계 필요");
    hooks_.sliderRange(Node(token).slider,0,height);
    if (!hooks_.sliderPage) throw std::invalid_argument("TextGump slider page 경계 필요");
    hooks_.sliderPage(Node(token).slider,page);
    if (!hooks_.sliderSetPosition) throw std::invalid_argument("TextGump slider position 쓰기 경계 필요");
    hooks_.sliderSetPosition(Node(token).slider,Node(token).scroll);
    if (!hooks_.sliderPosition) throw std::invalid_argument("TextGump slider position 조회 경계 필요");
    Node(token).scroll=hooks_.sliderPosition(Node(token).slider);
}
// extra는 원본 ret 4의 인자지만 몸체에서 읽지 않는다. 내용-실수 높이를 먼저 절삭한다.
void ScrollingTextGump::ScrollBottom(std::uint32_t token,std::int32_t extra) const {
    (void)extra;const double content=Buffer(token).handler->TotalHeight();SetScroll(token,Ftol(content-Node(token).baseHeight));
}
// 모든 요청은 가상 +8에 erase=false를 전달한다. slider 콜백 없이 요청 저장만 필요한 경우도 동일하다.
void ScrollingTextGump::Redraw(std::uint32_t token) const {
    if (!hooks_.redraw) throw std::invalid_argument("TextGump Redraw 경계 필요");hooks_.redraw(token,false);
}
// 요청은 원본처럼 음수도 저장한다. 잘못된 음수의 assert 계약은 실제 적용 시 검사한다.
void ScrollingTextGump::RequestScroll(std::uint32_t token,std::int32_t position) const {
    Node(token).scrollRequest=position;Redraw(token);
}
// signed ADD의 하위 DWORD가 음수이면 0을 요청한다. 현재 대기 요청에 delta를 더하지 않는다.
void ScrollingTextGump::RequestRelativeScroll(std::uint32_t token,std::int32_t delta) const {
    RequestScroll(token,std::max(Add(delta,Node(token).scroll),0));
}
// 추가 여백 저장 순서만 판본에 따라 다르며 현재 단계의 ScrollBottom은 원본처럼 값을 읽지 않는다.
void ScrollingTextGump::RequestBottom(std::uint32_t token,std::int32_t extra) const {
    if (create_.collection.edition==o::OriginalEdition::Patch1078) {
        Node(token).scrollRequest=-2;Node(token).bottomPadding=extra;
    } else { Node(token).bottomPadding=extra;Node(token).scrollRequest=-2; }
    Redraw(token);
}
// 콜백에서 새 요청을 써도 정상 반환 뒤 -1로 덮는다. 예외로 끝나면 원본 정상 반환 저장은 수행하지 않는다.
void ScrollingTextGump::ProcessRequest(std::uint32_t token) const {
    const auto request=Node(token).scrollRequest;if (request==-1) return;
    if (request==-2) ScrollBottom(token,Node(token).bottomPadding);
    else {
        if (request<0) throw std::invalid_argument("TextGump scrollRequest 음수");
        SetScroll(token,Node(token).scrollRequest);
    }
    Node(token).scrollRequest=-1;
}
// 패치/CD는 피연산자 순서와 상태 비트 검사가 다르지만 동률/NaN 결과는 같다. float32 임시 저장은 없다.
bool ScrollingTextGump::AtBottom(std::uint32_t token) const {
    const double threshold=static_cast<double>(Buffer(token).handler->TotalHeight())-Node(token).baseHeight-kBottomTolerance;
    return threshold<=static_cast<double>(Node(token).scroll);
}
// 센티널 자체도 원본 필드가 있으므로 0을 별도 자료로 반환한다. 다른 미등록 줄은 거부한다.
StyleTextBufferLine& ScrollingTextGump::Line(StyleTextBufferState& storage,std::uint32_t line) const {
    if (!line) return storage.sentinel;
    const auto found=storage.softLines.find(line);if (found==storage.softLines.end()) throw std::runtime_error("TextGump 표시 줄 없음");
    return found->second;
}
// 측정/출력 콜백 뒤 next는 같은 공통 표의 현재 값이다. 잘못 삭제한 조각은 참조 전에 거부한다.
StyleTextFragment& ScrollingTextGump::Fragment(std::uint32_t fragment) const {
    const auto found=fragments_.fragments.find(fragment);if (!fragment || found==fragments_.fragments.end()) throw std::runtime_error("TextGump 표시 조각 없음");
    return found->second;
}
// 줄 끝점과 scroll이 같으면 그 줄을 유지한다. 원본 assert에 도달하는 센티널 초과는 예외로 처리한다.
std::uint32_t ScrollingTextGump::FirstVisible(std::uint32_t token,StyleTextBufferState& storage) const {
    auto line=storage.sentinel.next;std::set<std::uint32_t> seen;
    // 현재 줄의 저장 y+높이로 시작 줄만 찾는다. 이후 출력/선택은 높이를 누적한다.
    while (Add(Line(storage,line).y,Line(storage,line).height)<Node(token).scroll) {
        if (!line) throw std::runtime_error("TextGump scroll이 센티널을 넘음");
        Visit(seen,line);line=Line(storage,line).next;
    }
    return line;
}
// 출력에서 사용하는 ascent가 아닌 rowHeight-fragmentHeight로 클릭의 세로 유효 범위를 계산한다.
std::uint32_t ScrollingTextGump::HitTest(std::uint32_t token,ScreenPoint position) const {
    const auto bounds=collection_.Bounds(token);
    if (position.x<bounds.left || position.x>=bounds.right || position.y<bounds.top || position.y>=bounds.bottom) return 0;
    auto& storage=*Buffer(token).state;auto line=FirstVisible(token,storage);
    auto y=Subtract(Line(storage,line).y,Node(token).scroll);const auto localX=Subtract(position.x,bounds.left),localY=Subtract(position.y,bounds.top);
    std::set<std::uint32_t> rows;
    // 원본은 x87 FILD 정수를 float로 저장하지 않고 현재 baseHeight와 비교한다.
    while (line && BelowViewport(y,Node(token).baseHeight,create_.collection.edition)) {
        Visit(rows,line);
        if (localY<Add(Line(storage,line).height,y)) {
            if (Line(storage,line).width<=localX) return 0;
            auto fragment=Line(storage,line).head;std::int32_t width=0;std::set<std::uint32_t> seen;
            // 다음 조각은 가상 측정 이후의 현재 next다. 정확한 위 경계에서는 선택하지 않는다.
            while (fragment) {
                Visit(seen,fragment);const auto size=style_.MeasureFragment(fragment);width=Add(width,size[0]);
                if (localX<width && Add(Subtract(Line(storage,line).height,size[1]),y)<localY) return fragment;
                fragment=Fragment(fragment).next;
            }
            return 0;
        }
        const auto height=Line(storage,line).height;line=Line(storage,line).next;y=Add(y,height);
    }
    return 0;
}
// Layout/스크롤 적용 후의 현재 경계/위치를 사용한다. 실제 모든 글리프/그림은 기존 StyleText 가상 분배다.
void ScrollingTextGump::Draw(std::uint32_t token) const {
    EnsureLayout(token);ProcessRequest(token);
    const auto bounds=collection_.Bounds(token);const GumpClipScope guard(clip_,bounds);
    const auto position=collection_.Position(token);auto& storage=*Buffer(token).state;auto line=FirstVisible(token,storage);
    auto y=Subtract(Line(storage,line).y,Node(token).scroll);std::set<std::uint32_t> rows;
    // 저장 y는 첫 줄에만 쓰고 이후 행은 이전 행 높이를 누적한다. viewport 끝 동률은 그리지 않는다.
    while (line && BelowViewport(y,Node(token).baseHeight,create_.collection.edition)) {
        Visit(rows,line);std::int32_t ascent=0;auto fragment=Line(storage,line).head;std::set<std::uint32_t> seen;
        // 첫 순회는 현재 줄의 최대 상승만 구한다. 빈 줄도 높이 이동은 수행한다.
        while (fragment) {
            Visit(seen,fragment);ascent=std::max(ascent,style_.Baseline(fragment)[1]);fragment=Fragment(fragment).next;
        }
        const auto baseline=Add(ascent,y);std::int32_t x=0;fragment=Line(storage,line).head;seen.clear();
        // 두 번째 순회는 기준선을 다시 측정하고 가상 크기→출력→저장 폭으로 전진한다.
        while (fragment) {
            Visit(seen,fragment);const auto metrics=style_.Baseline(fragment);const auto size=style_.MeasureFragment(fragment);
            style_.DrawFragment(fragment,{Add(position.x,x),Add(Subtract(position.y,metrics[1]),baseline)});
            x=Add(x,size[0]);fragment=Fragment(fragment).next;
        }
        const auto height=Line(storage,line).height;line=Line(storage,line).next;y=Add(y,height);
    }
}
// 현재 토큰의 처리기를 매번 조회해 등록/수명 변경을 반영하며 기존 버튼/배경 분배를 이어 붙인다.
void BindTextGumpDraw(GumpDrawHooks& hooks,std::function<const TextGump*(std::uint32_t)> single,
    std::function<const ScrollingTextGump*(std::uint32_t)> scrolling) {
    if (!single || !scrolling) throw std::invalid_argument("TextGump Draw 분류 경계 필요");
    const auto previous=hooks.draw;
    // 한 줄/다중 줄 등록은 서로 겹치면 안 된다. 한 줄을 먼저 조회하는 원본 클래스 분배 경계다.
    hooks.draw=[single=std::move(single),scrolling=std::move(scrolling),previous](auto token) {
        if (const auto* text=single(token)) text->Draw(token);
        else if (const auto* multi=scrolling(token)) multi->Draw(token);
        else if (previous) previous(token);
        else throw std::runtime_error("Gump 가상 Draw 처리기 없음");
    };
}
// 파생 삭제 분배를 실제 수명 처리기 생성 전에 구성할 수 있도록 resolve는 호출 때 평가한다.
void BindTextGumpLifetime(GumpLifetimeHooks& hooks,std::function<const TextGump*(std::uint32_t)> resolve) {
    if (!resolve) throw std::invalid_argument("TextGump 수명 분류 경계 필요");
    const auto previous=hooks.deletingDestructor;
    // 파생 head 반납 뒤 기본 소멸자로 직접 위임하므로 가상 분배 재귀는 생기지 않는다.
    hooks.deletingDestructor=[resolve=std::move(resolve),previous](auto token,auto flags) {
        if (const auto* text=resolve(token)) text->DeleteScalar(token,flags);
        else if (previous) previous(token,flags);
        else throw std::runtime_error("Gump scalar 소멸자 처리기 없음");
    };
}
}
