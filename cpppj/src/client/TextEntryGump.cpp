// 실제0x28 편집 버퍼와 두 Slider를 공유하며 원본의 현재 필드 재조회/사건 순서를 유지한다.
#include "client/TextEntryGump.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace netstorm::client {
namespace {
constexpr std::uint32_t kEntryVtable=0x508e10,kConsoleVtable=0x506a6c,kVisible=1,kVoid=8,kDead=0x10000; // 원본10.78의 표와 기본 Gump 비트다.
constexpr std::uint32_t kCodeMask=0x03ffffff,kCommand=0x02000000,kRelease=0x80000000,kShift=0x20000000,kControl=0x10000000; // 원본 문자 입력 관문이다.
constexpr double kBlinkPeriod=0.5; //00503350의 점멸 주기다.
constexpr std::array<char,2> kMeasureLetter{'M',0}; // 원본005052fc의 줄/열 크기 측정 문자열이다.
// 원본 signed DWORD 산술의 하위 비트를 유지한다.
std::int32_t Add(std::int32_t first,std::int32_t second) { return std::bit_cast<std::int32_t>(std::uint32_t(first)+std::uint32_t(second)); }
// 음수와 INT_MIN도 C++ signed overflow 없이 원문의 감김을 유지한다.
std::int32_t Sub(std::int32_t first,std::int32_t second) { return std::bit_cast<std::int32_t>(std::uint32_t(first)-std::uint32_t(second)); }
// CRT _ftol의64비트 절삭 후 하위 DWORD다.
std::int32_t Ftol(double value) {
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
// 선택 삽입 API에 실제 NUL을 포함하는 안정된 원문 입력을 제공한다.
std::vector<char> Terminated(std::string_view text) { const auto end=text.find('\0');std::vector<char> bytes(text.begin(),text.begin()+static_cast<std::ptrdiff_t>(end==text.npos ? text.size() : end));bytes.push_back(0);return bytes; }
// 원본 C문자열의 현재 끝만 읽으며 잘못된 미종료 입력은 호스트 오류로 거부한다.
std::span<const char> CString(std::span<const char> bytes) {
    const auto end=std::find(bytes.begin(),bytes.end(),0);if (end==bytes.end()) throw std::runtime_error("TextEntry NUL 문자열 범위 없음");
    return bytes.first(static_cast<std::size_t>(end-bytes.begin())+1);
}
// Console 단어 찾기가 사용하는 원본 기본 C locale의 ASCII 공백이다.
bool Space(unsigned char value) { return value==' ' || (value>=9 && value<=13); }
}
// 실제 파생/기본/문자 버퍼 조회만 필수이며 장치/가상 함수는 각 정상 경로에서 확인한다.
TextEntryGump::TextEntryGump(TextEntryGumpState& state,TextEntryGumpHooks hooks):state_(state),hooks_(std::move(hooks)) {
    if (!hooks_.node || !hooks_.base || !hooks_.buffer) throw std::invalid_argument("TextEntry 같은 노드/편집 버퍼 필요");
}
// 콜백 중에도 기본 Gump와 파생 저장소는 같은 주소에서 살아 있어야 한다.
TextEntryGumpNode& TextEntryGump::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.node(token) : nullptr;
    if (!node || static_cast<GumpCreateNode*>(node)!=hooks_.base(token)) throw std::runtime_error("TextEntry 공유 노드 없음");return *node;
}
// Console의 기본 TextEntry 주소도 같아야 한다.
ConsoleGumpNode& TextEntryGump::Console(std::uint32_t token) const {
    auto* node=token && hooks_.console ? hooks_.console(token) : nullptr;
    if (!node || static_cast<TextEntryGumpNode*>(node)!=&Node(token)) throw std::runtime_error("Console 공유 파생 노드 없음");return *node;
}
// 원본 자식 포인터의 유효성은 실제 도달한 때만 검사한다.
GumpCreateNode& TextEntryGump::Base(std::uint32_t token) const {
    auto* node=token ? hooks_.base(token) : nullptr;if (!node) throw std::runtime_error("TextEntry 실제 자식 없음");return *node;
}
// 현재 buffer 토큰을 복사한 과거 State로 대체하지 않는다.
TextEntryBuffer& TextEntryGump::Buffer(std::uint32_t token) const {
    const auto current=Node(token).buffer;auto* buffer=current ? hooks_.buffer(current) : nullptr;if (!buffer) throw std::runtime_error("TextEntry 현재 편집 버퍼 없음");return *buffer;
}
// 실제 ScrollGump와 같은 기본 주소의 현재 position을 읽는다.
ScrollGumpNode& TextEntryGump::Slider(std::uint32_t token) const {
    auto* node=token && hooks_.slider ? hooks_.slider(token) : nullptr;
    if (!node || static_cast<GumpCreateNode*>(node)!=&Base(token)) throw std::runtime_error("TextEntry 실제 Slider 없음");return *node;
}
// 원문 재그리기는 현재 가상+8(false)에만 요청한다.
void TextEntryGump::Redraw(std::uint32_t token) const { if (!hooks_.redraw) throw std::invalid_argument("TextEntry Redraw 필요");hooks_.redraw(token,false); }
// 부모 통지는 첫 성공 처리기까지 가는 공통 Bubble에 맡긴다.
void TextEntryGump::Bubble(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
    if (!hooks_.bubble) throw std::invalid_argument("TextEntry 부모 사건 필요");hooks_.bubble(token,event,first,second);
}
// 원문의 기본 문맥을 각 측정 시점에 실제 글꼴 계층에서 읽는다.
std::array<std::int32_t,2> TextEntryGump::Measure(std::uint32_t token,std::span<const char> text,std::int32_t length) const {
    if (!hooks_.measure) throw std::invalid_argument("TextEntry 실제 글꼴 측정 필요");return hooks_.measure(token,text,length);
}
// 별도 제외문자 문자열은0/null과 빈 문자열을 구별한다.
std::span<const char> TextEntryGump::Excluded(std::uint32_t token) const {
    const auto address=Node(token).excludedChars;if (!address) return {};
    if (!hooks_.bytes) throw std::invalid_argument("TextEntry 제외문자 저장소 필요");const auto bytes=hooks_.bytes(address);
    return CString({reinterpret_cast<const char*>(bytes.data()),bytes.size()});
}
// 기본 배경/문자 버퍼/idle/두 실제 Slider를 순서대로 생성한다. Console의 기본 생성도 이 몸체다.
std::uint32_t TextEntryGump::Construct(std::uint32_t token,std::int32_t width,std::int32_t height,std::optional<std::string_view> initial,
    std::uint32_t font,std::uint32_t background,std::uint32_t color) const {
    auto& node=Node(token);if (!hooks_.constructBackground || !hooks_.allocate || !hooks_.constructSlider || !hooks_.idle || !hooks_.attach || !hooks_.now)
        throw std::invalid_argument("TextEntry 실제 생성 경계 필요");
    hooks_.constructBackground(token,background);node.vtable=kEntryVtable;node.blinkAt=0;node.shiftAnchor=0;node.shiftEnd=0;
    node.command=static_cast<std::uint32_t>(static_cast<std::int32_t>(state_.kind));
    const auto allocated=hooks_.allocate(0x28); // 원문의 고정 new 결과이며 가상 생성 뒤 root에 설치한다.
    if (allocated) { auto* buffer=hooks_.buffer(allocated);if (!buffer) throw std::runtime_error("TextEntry 새 버퍼 저장소 없음");buffer->Construct(0); }
    node.buffer=allocated;node.ownsBuffer=1;if (!allocated && hooks_.assertion) hooks_.assertion(0x31);
    node.maxLength=0;node.blinkPhase=0;node.blinkAt=hooks_.now()+kBlinkPeriod;
    node.baseWidth=static_cast<float>(width);node.width=static_cast<float>(double(width)*node.scale);
    node.textContext={0,font,color,0,0,0,1,1};node.baseHeight=static_cast<float>(height);node.height=static_cast<float>(double(height)*node.scale);
    node.scrollLine=0;node.scrollColumn=0;node.flags|=0x400400;hooks_.idle(token,true);
    const auto bytes=initial ? Terminated(*initial) : std::vector<char>{};Buffer(token).Reset(bytes);Redraw(token);
    node.readOnly=0;node.forwardInput=0;node.changeNotice=0;node.multiline=1;
    auto slider=hooks_.allocate(0xd8);if (slider) hooks_.constructSlider(slider,10,100,state_.brown,state_.black);node.verticalSlider=slider;
    slider=hooks_.allocate(0xd8);if (slider) hooks_.constructSlider(slider,100,10,state_.brown,state_.black);node.horizontalSlider=slider;
    Base(node.verticalSlider).flags&=~kVisible;Base(node.horizontalSlider).flags&=~kVisible;
    hooks_.attach(node.verticalSlider,static_cast<float>(double(static_cast<float>(width))-Base(node.verticalSlider).baseWidth),0,token,1);
    hooks_.attach(node.horizontalSlider,0,static_cast<float>(double(static_cast<float>(height))-Base(node.horizontalSlider).baseHeight),token,1);
    SetScroll(token,0,0);node.showVertical=0;node.showHorizontal=0;node.recipientHighlight=0;
    if (initial) { node.shiftAnchor=0;node.shiftEnd=static_cast<std::int32_t>(bytes.size()-1);Buffer(token).Select(0,node.shiftEnd); }
    node.excludedChars=0;return token;
}
// Console 확장은 기본 생성이 끝난 동일 노드에만 수행한다.
std::uint32_t TextEntryGump::ConstructConsole(std::uint32_t token,std::int32_t width,std::int32_t height,std::optional<std::string_view> initial,
    std::uint32_t font,std::uint32_t background,std::uint32_t color) const {
    Console(token);Construct(token,width,height,initial,font,background,color);auto& node=Console(token);
    node.vtable=kConsoleVtable;node.command=static_cast<std::uint32_t>(static_cast<std::int32_t>(state_.consoleKind));
    node.autocomplete=0;node.submit=0;node.wordLength=0;node.wordStart=0;node.resetWord=1;node.recipientHighlight=1;return token;
}
// 공통 부착 뒤 두 자식을 Push하고 크기/표시를 갱신한 다음 현재 scroll을 다시 적용한다.
void TextEntryGump::Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const {
    auto& node=Node(token);if (!hooks_.baseAttach || !hooks_.push || !hooks_.attach) throw std::invalid_argument("TextEntry 실제 부착 경계 필요");
    hooks_.baseAttach(token,x,y,parent,mode);hooks_.push(node.verticalSlider);hooks_.push(node.horizontalSlider);
    auto& vertical=Base(node.verticalSlider);vertical.baseHeight=node.baseHeight;vertical.height=static_cast<float>(double(node.baseHeight)*vertical.scale);
    auto& horizontal=Base(node.horizontalSlider);horizontal.baseWidth=node.baseWidth;horizontal.width=static_cast<float>(double(node.baseWidth)*horizontal.scale);
    if (node.showVertical) Base(node.verticalSlider).flags|=kVisible;else Base(node.verticalSlider).flags&=~kVisible;
    if (node.showHorizontal) Base(node.horizontalSlider).flags|=kVisible;else Base(node.horizontalSlider).flags&=~kVisible;
    hooks_.attach(node.verticalSlider,static_cast<float>(double(node.baseWidth)-Base(node.verticalSlider).baseWidth),0,token,1);
    hooks_.attach(node.horizontalSlider,0,static_cast<float>(double(node.baseHeight)-Base(node.horizontalSlider).baseHeight),token,1);
    SetScroll(token,node.scrollLine,node.scrollColumn);
}
// 열 clamp는 새 line이 아니라 쓰기 전의 현재 scrollLine으로 조회하는 원문 순서를 보존한다.
void TextEntryGump::SetScroll(std::uint32_t token,std::int32_t line,std::int32_t column) const {
    auto& node=Node(token);if (line<0) line=0;else if (Buffer(token).LineCount()<=line) line=Sub(Buffer(token).LineCount(),1);
    if (column<0) column=0;else if (Buffer(token).LineLength(node.scrollLine)<=column) column=Sub(Buffer(token).LineLength(node.scrollLine),1);
    node.scrollLine=line;node.scrollColumn=column;const auto metric=Measure(token,kMeasureLetter); // 현재 실제 M 폭/높이다.
    const auto rows=Ftol(double(node.baseHeight)/metric[1]),columns=Ftol(double(node.baseWidth)/metric[0]); // 원본 두 _ftol의 줄/열 page다.
    if (!hooks_.sliderRange || !hooks_.sliderPage || !hooks_.sliderPosition) throw std::invalid_argument("TextEntry 실제 Slider 값 처리 필요");
    Buffer(token).LineLength(node.scrollLine);const auto count=Buffer(token).LineLength(node.scrollLine); // 원문은 같은 현재 줄 길이를 두 번 읽는다.
    hooks_.sliderRange(node.horizontalSlider,0,Sub(count,1));hooks_.sliderPage(node.horizontalSlider,columns);hooks_.sliderPosition(node.horizontalSlider,column);
    node.scrollColumn=Slider(node.horizontalSlider).position;
    const auto lines=Buffer(token).LineCount();hooks_.sliderRange(node.verticalSlider,0,Sub(lines,1));hooks_.sliderPage(node.verticalSlider,rows);hooks_.sliderPosition(node.verticalSlider,line);
    node.scrollLine=Slider(node.verticalSlider).position;Redraw(token);
}
// 원본 delta는 각각 현재 signed DWORD와 감김 덧셈한다.
void TextEntryGump::RelativeScroll(std::uint32_t token,std::int32_t line,std::int32_t column) const {
    SetScroll(token,Add(Node(token).scrollLine,line),Add(Node(token).scrollColumn,column));
}
// 원문은 현재 시각을 두 번 읽으며 Redraw 콜백 뒤 두 번째 시각으로 다음 점멸을 예약한다.
void TextEntryGump::Idle(std::uint32_t token) const {
    auto& node=Node(token);if ((node.flags&kDead) || node.readOnly) return;if (!hooks_.now) throw std::invalid_argument("TextEntry 실제 점멸 시계 필요");
    if (node.blinkAt<=hooks_.now()) { ++node.blinkPhase;Redraw(token);node.blinkAt=hooks_.now()+kBlinkPeriod; }
}
// 초기화 뒤 Redraw는 가상 처리기로만 요청한다.
void TextEntryGump::SetText(std::uint32_t token,std::span<const char> text) const { Buffer(token).Reset(text);Redraw(token); }
// 선택을 끝으로 옮긴 뒤 삽입하고 요청한 경우에만 이전 선택을 복구한다.
void TextEntryGump::AppendText(std::uint32_t token,std::span<const char> text,bool restoreSelection) const {
    const auto begin=Buffer(token).State().selectionBegin,end=Buffer(token).State().selectionEnd;
    const auto at=Add(Buffer(token).State().length,1);Buffer(token).Select(at,at);Buffer(token).Insert(text);
    if (restoreSelection) Buffer(token).Select(begin,end);Redraw(token);
}
// 원본 출력 용량/거부 반환은 실제 문자 버퍼가 결정한다.
std::uint32_t TextEntryGump::CopyText(std::uint32_t token,std::span<char> output) const { return Buffer(token).CopyAll(output); }
// 제외문자 교체는 이전 소유 주소를 먼저 반납한다. 기본 소멸자의 누락 free를 임의로 추가하지 않는다.
void TextEntryGump::SetExcluded(std::uint32_t token,std::optional<std::string_view> text) const {
    auto& node=Node(token);if (node.excludedChars) { if (!hooks_.releaseBytes) throw std::invalid_argument("TextEntry 제외문자 반납 필요");hooks_.releaseBytes(node.excludedChars);node.excludedChars=0; }
    if (!text) return;if (!hooks_.allocateBytes || !hooks_.bytes) throw std::invalid_argument("TextEntry 제외문자 실제 소유 저장소 필요");
    const auto source=Terminated(*text);node.excludedChars=hooks_.allocateBytes(source.size());if (!node.excludedChars && hooks_.assertion) hooks_.assertion(0x85);
    auto target=hooks_.bytes(node.excludedChars);if (target.size()<source.size()) throw std::runtime_error("TextEntry 제외문자 할당/범위 실패");
    std::copy(source.begin(),source.end(),target.begin());
    std::size_t index{}; // 원문00504b0c는 LF 한 BYTE만 포함하며 최초 문자열의 할당 폭으로 이동한다.
    while (target[index]) { // 현재 NUL까지 반복하고 삭제했을 때 같은 위치의 다음 문자를 다시 검사한다.
        if (target[index]==10) std::memmove(target.data()+index,target.data()+index+1,source.size()-index-1);
        else ++index;
    }
}
// 가상 Push 뒤 현재 위치를 읽어 재부착하며 이미 void이면 필드만 쓴다.
void TextEntryGump::SetHorizontal(std::uint32_t token,std::uint32_t value) const {
    auto& node=Node(token);node.showHorizontal=value;if (!(node.flags&kVoid)) { if (!hooks_.push || !hooks_.attach) throw std::invalid_argument("TextEntry 표시 재부착 필요");hooks_.push(token);hooks_.attach(token,node.x,node.y,0,0); }
}
// 빌린 버퍼는 손대지 않는다. 자식 Slider는 공통 자식 우선 Destroy가 반납한다.
void TextEntryGump::Destruct(std::uint32_t token,bool console) const {
    auto& node=Node(token);if (console) { Console(token);node.vtable=kConsoleVtable; }node.vtable=kEntryVtable;
    if (node.buffer && node.ownsBuffer) { const auto saved=node.buffer;Buffer(token).Destroy();if (!hooks_.releaseBuffer) throw std::invalid_argument("TextEntry 버퍼 객체 반납 필요");hooks_.releaseBuffer(saved);node.buffer=0; }
    if (!hooks_.destructBase) throw std::invalid_argument("TextEntry 실제 기본 소멸자 필요");hooks_.destructBase(token);
}
// 선택/문자 삽입 뒤 통지·Redraw·원시 사건 전달 순서와 별도 Slider 사건 분기를 보존한다.
std::uint32_t TextEntryGump::Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
    auto& node=Node(token);auto value=event;std::uint32_t result{};const auto code=event&kCodeMask; // Enter만 전달 value를 LF10으로 바꿀 수 있다.
    if (!(event&kCommand) && code!=0x10000 && code!=0x20000 && !node.readOnly && !(event&kRelease)) {
        result=1;bool notify=true; // 인식하지 않는 문자/키는 공통 Redraw 뒤 반환0이다.
        switch (code) {
        case 0x250000: { // 왼쪽: 선택이 있어도 shift가 없으면 현재 begin−1 한 칸으로 옮긴다.
            const auto begin=Buffer(token).State().selectionBegin,end=Buffer(token).State().selectionEnd;
            if (!(event&kShift)) Buffer(token).Select(Sub(begin,1),Sub(begin,1));
            else { if (begin==end) { node.shiftAnchor=begin;node.shiftEnd=end; }node.shiftEnd=Sub(node.shiftEnd,1);Buffer(token).Select(node.shiftAnchor,node.shiftEnd); }break;
        }
        case 0x270000: { // 오른쪽은 현재 end+1을 기준으로 삼는다.
            const auto begin=Buffer(token).State().selectionBegin,end=Buffer(token).State().selectionEnd;
            if (!(event&kShift)) Buffer(token).Select(Add(end,1),Add(end,1));
            else { if (begin==end) { node.shiftAnchor=begin;node.shiftEnd=end; }node.shiftEnd=Add(node.shiftEnd,1);Buffer(token).Select(node.shiftAnchor,node.shiftEnd); }break;
        }
        case 0x230000: { // End는 length+1로 Select한 뒤 원문처럼 +dc 기준의 열 delta를 전달한다.
            const auto begin=Buffer(token).State().selectionBegin,end=Buffer(token).State().selectionEnd;
            if (!(event&kShift)) { const auto at=Add(Buffer(token).State().length,1);Buffer(token).Select(at,at);RelativeScroll(token,0,node.shiftEnd); }
            else { if (begin==end) { node.shiftAnchor=begin;node.shiftEnd=end; }node.shiftEnd=Add(Buffer(token).State().length,1);Buffer(token).Select(node.shiftAnchor,node.shiftEnd);RelativeScroll(token,0,node.shiftEnd); }break;
        }
        case 0x240000: { // Home의 shift 선택과 열0 요청은 서로 다른 상태를 사용한다.
            const auto begin=Buffer(token).State().selectionBegin,end=Buffer(token).State().selectionEnd;
            if (!(event&kShift)) Buffer(token).Select(0,0);
            else { if (begin==end) { node.shiftAnchor=begin;node.shiftEnd=end; }node.shiftEnd=0;Buffer(token).Select(node.shiftAnchor,node.shiftEnd); }
            RelativeScroll(token,0,Sub(0,node.scrollColumn));break;
        }
        case 8: { // Ctrl+Backspace는 단어 삭제가 아니라 전체 Reset이다.
            if (event&kControl) Buffer(token).Reset();
            else { auto begin=Buffer(token).State().selectionBegin;const auto end=Buffer(token).State().selectionEnd;if (begin==end) begin=Sub(begin,1);Buffer(token).Select(begin,end);Buffer(token).Insert(); }break;
        }
        case 13: { // multiline일 때만 LF를 삽입하나 Enter 부모 통지는 항상 보낸다.
            if (node.multiline) { value=10;const std::array<char,2> newline{10,0};Buffer(token).Insert(newline); }
            Bubble(token,state_.enterEvent,node.serial,0);break;
        }
        case 9:case 27:case 0x260000:case 0x280000:result=0;notify=false;break; // Tab/ESC/위/아래는 이 기본 입력기에서 처리하지 않는다.
        case 0x2d0000: // Insert는 Ctrl 복사 우선이며 shift가 있으면 붙여넣는다.
            if (event&kControl) Buffer(token).Copy();else if (event&kShift) Buffer(token).Paste(Excluded(token));break;
        case 0x2e0000: { // Shift+Delete는 Cut, 일반 Delete는 현재 선택 또는 오른쪽 한 문자다.
            if (event&kShift) Buffer(token).Cut();
            else { const auto begin=Buffer(token).State().selectionBegin;auto end=Buffer(token).State().selectionEnd;if (begin==end) end=Add(end,1);Buffer(token).Select(begin,end);Buffer(token).Insert(); }break;
        }
        default: {
            if (code<0x20 || code>=0x100) { result=0;notify=false;break; }
            if (node.maxLength<1 || Add(Buffer(token).State().length,1)<node.maxLength) {
                const auto excluded=Excluded(token); // 문자 제한은 현재 선택 길이를 빼지 않고 전체length+1로 판정한다.
                if (!excluded.empty() && std::find(excluded.begin(),excluded.end(),static_cast<char>(value))!=excluded.end()) { result=0;notify=false;break; }
                const std::array<char,2> character{static_cast<char>(value),0};Buffer(token).Insert(character);
            }break;
        }
        }
        if (notify && node.changeNotice) Bubble(token,state_.changeEvent,0,0);
        Redraw(token);if (node.forwardInput) Bubble(token,value,first,second);
    }
    if ((value&kCodeMask)==state_.sliderChange) {
        if (first==Base(node.verticalSlider).serial) SetScroll(token,Slider(node.verticalSlider).position,node.scrollColumn);
        else if (first==Base(node.horizontalSlider).serial) SetScroll(token,node.scrollLine,Slider(node.horizontalSlider).position);
        else if (hooks_.assertion) hooks_.assertion(0x246);
    }
    if ((value&kCodeMask)==state_.sliderStep) {
        const auto metric=Measure(token,kMeasureLetter); // 원문은 serial 일치 확인 전에 M을 측정한다.
        if (first==Base(node.horizontalSlider).serial && (second==1 || second==2 || second==3 || second==4)) {
            RelativeScroll(token,0,second==2 || second==4 ? Sub(0,metric[0]) : metric[0]);result=1;
        }
    }
    return result;
}
// 새 단어는 실제 문자열 포인터를, 반복Tab은 null을 callback에 보내는 원본 계약을 보존한다.
std::uint32_t TextEntryGump::HandleConsole(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
    auto& node=Console(token);const auto code=event&kCodeMask;
    if (!(event&kCommand) && code!=0x10000 && code!=0x20000 && !(event&kRelease)) {
        if (code==9) {
            const auto begin=Buffer(token).State().selectionBegin,end=Buffer(token).State().selectionEnd;
            if (begin==end && node.autocomplete) {
                std::optional<std::string_view> word; // null은 이전 단어의 다음/이전 후보를 뜻한다.
                if (node.resetWord) {
                    node.wordStart=static_cast<std::uint32_t>(begin);const auto data=Buffer(token).Data(); // 실제 현재 선택과 같은 바이트다.
                    while (node.wordStart>0) { // 공백/콜론 바로 뒤까지 단어 시작을 역방향으로 찾는다.
                        const auto at=node.wordStart-1;if (at>=data.size()) throw std::runtime_error("Console 단어 선택 범위");
                        const auto value=data[at];if (Space(value) || value==':') break;--node.wordStart;
                    }
                    node.wordLength=std::uint32_t(begin)-node.wordStart;
                    if (node.wordStart>=data.size()) throw std::runtime_error("Console 단어 시작 범위");
                    const auto raw=CString({reinterpret_cast<const char*>(data.data()+node.wordStart),data.size()-node.wordStart});word=std::string_view(raw.data(),raw.size()-1);
                }
                if (!hooks_.complete) throw std::invalid_argument("Console 실제 autocomplete callback 필요");
                const auto found=hooks_.complete(node.autocomplete,word,std::bit_cast<std::int32_t>(node.wordLength),(event&kShift)!=0);
                if (found) { Buffer(token).Select(std::bit_cast<std::int32_t>(node.wordStart),std::bit_cast<std::int32_t>(node.wordStart+node.wordLength));const auto bytes=Terminated(*found);Buffer(token).Insert(bytes);node.wordLength=static_cast<std::uint32_t>(bytes.size()-1);node.resetWord=0; }
            }
            return 1;
        }
        if (code==13) { if (node.submit) { if (!hooks_.submit) throw std::invalid_argument("Console 실제 제출 callback 필요");hooks_.submit(node.submit,Buffer(token).Text()); } }
        else if (code==0x100000) return Handle(token,event,first,second); // Shift 키 자체는 후보 순환 상태를 초기화하지 않는다.
        node.resetWord=1;
    }
    return Handle(token,event,first,second);
}
// 각 줄의 원시 선택 쌍과 동일 문자 바이트로 배경/일반·선택 글자/수신 대상/caret를 출력한다.
void TextEntryGump::Draw(std::uint32_t token) const {
    auto& node=Node(token);if (!hooks_.drawBackground || !hooks_.drawText || !hooks_.position || !hooks_.fill) throw std::invalid_argument("TextEntry 실제 픽셀/글자 출력 필요");
    hooks_.drawBackground(token);const auto metric=Measure(token,kMeasureLetter); // 원본은 배경을 그린 뒤 현재 M 높이로 표시 줄 수를 계산한다.
    const auto truncated=Ftol(node.baseHeight);if (!metric[1] || (truncated==std::numeric_limits<std::int32_t>::min() && metric[1]==-1)) throw std::runtime_error("TextEntry 원본 줄높이 나눗셈 범위");
    const auto partial=truncated%metric[1]!=0 ? 1 : 0,limit=Ftol(double(node.baseHeight)/metric[1]+partial); // 단순 ceil이 아니라 원본 정수 나머지 관문이다.
    std::array<char,1024> line{};std::array<std::int32_t,2> selection{};std::int32_t y{}; // 원본 한 줄 저장소와 native 두DWORD 출력이다.
    const auto write=[&](std::span<const char> bytes,std::int32_t x,std::int32_t atY,const std::array<std::uint32_t,8>* context=nullptr) { hooks_.drawText(token,bytes,{x,atY},context); }; // 모든 출력은 같은 기본 Gump 상대 좌표를 사용한다.
    const auto piece=[&](std::size_t at,std::size_t count) { // native 선택의 memcpy와 명시 NUL을 같은 바이트에서 수행한다.
        if (at>line.size() || count>=line.size() || count>line.size()-at) throw std::runtime_error("TextEntry 선택 그림 범위");
        std::vector<char> bytes(count+1);std::copy_n(line.begin()+static_cast<std::ptrdiff_t>(at),count,bytes.begin());bytes.back()=0;return bytes;
    };
    for (auto index=node.scrollLine;index<Buffer(token).LineCount();index=Add(index,1)) { // 줄 수는 각 반복 끝의 현재 문자 버퍼에서 다시 읽는다.
        if (limit<=Sub(index,node.scrollLine)) return;
        const auto length=Buffer(token).CopyLine(index,line,selection); // 수평 잘라내기 뒤에도 원문의 선택/길이 쌍을 임의 보정하지 않는다.
        if (length==0xffffffffU) { if (hooks_.assertion) hooks_.assertion(0x2ae);throw std::runtime_error("TextEntry 원본 줄 복사 범위 거부"); }
        if (node.scrollColumn>0 && node.scrollColumn<1024) {
            const auto visible=CString(line).size()-1,removed=static_cast<std::size_t>(node.scrollColumn); // 원본 String::erase는 요청 수가 길이를 넘으면 아무것도 바꾸지 않는다.
            if (removed<=visible) std::memmove(line.data(),line.data()+removed,visible-removed+1);
        }
        if (length) { if (length>line.size()) throw std::runtime_error("TextEntry 줄 끝 범위");if (line[length-1]=='\n') line[length-1]=' '; }
        else { line[0]=' ';line[1]=0; }
        const auto dimensions=Measure(token,line); // 원문의 줄 간격은 각 실제 줄의 현재 측정 높이다.
        if (selection[0]==-1 || selection[1]<=selection[0]) {
            const auto visible=CString(line);const auto colon=std::find(visible.begin(),visible.end(),':'); // 수신 대상 색은 선택 영역이 없을 때만 적용한다.
            if (!node.recipientHighlight || colon==visible.end()) write(line,0,y);
            else {
                const auto at=static_cast<std::size_t>(colon-visible.begin());const char saved=line[at+1];line[at+1]=0;line[at]=0;
                if (!hooks_.recipient) throw std::invalid_argument("Console 실제 수신 대상 검사 필요");const bool valid=hooks_.recipient(std::string_view(line.data(),at));line[at]=':';
                if (valid) { auto context=node.textContext;context[2]=state_.yellow;write(line,0,y,&context); }
                else write(line,0,y);
                const auto prefix=Measure(token,line);line[at+1]=saved;write(std::span<const char>(line).subspan(at+1),prefix[0],y);
            }
        } else {
            if (selection[0]<0 || selection[1]<0) throw std::runtime_error("TextEntry 원본 선택 인덱스 범위");
            std::int32_t x{}; // 선택 앞 문자열의 실제 측정 폭이다.
            if (selection[0]>0) { const auto before=piece(0,static_cast<std::size_t>(selection[0]));const auto size=Measure(token,before);write(before,0,y);x=size[0]; }
            auto context=node.textContext;context[2]=node.backgroundFlags&0xff; // 선택 글자는 평면 배경색으로 반전한다.
            const auto selected=piece(static_cast<std::size_t>(selection[0]),static_cast<std::size_t>(Sub(selection[1],selection[0])));const auto size=Measure(token,selected);
            const auto position=hooks_.position(token); // 선택 사각형은 현재 화면 위치를 다시 조회한다.
            const auto left=Add(position.x,x),top=Add(position.y,y);hooks_.fill({left,top,Add(left,size[0]),Add(top,size[1])},node.textContext[2]);
            write(selected,x,y,&context);
            if (selection[1]<std::bit_cast<std::int32_t>(length)) {
                const auto at=static_cast<std::size_t>(selection[1]);if (at>=line.size()) throw std::runtime_error("TextEntry 선택 뒤 그림 범위");write(std::span<const char>(line).subspan(at),Add(x,size[0]),y);
            }
        }
        if (selection[0]==selection[1] && selection[0]!=-1 && (node.blinkPhase&1) && !node.readOnly) {
            const auto position=hooks_.position(token);const auto caret=Measure(token,line,selection[0]); // 명시 길이는 중간 NUL 뒤 원문 BYTE도 측정한다.
            const auto left=Add(position.x,caret[0]),top=Add(position.y,y);hooks_.fill({left,top,Add(left,1),Add(top,caret[1])},node.textContext[2]);
        }
        y=Add(y,dimensions[1]);
    }
}
}
