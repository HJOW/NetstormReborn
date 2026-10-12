// 원본 메뉴의 DWORD 감김·CRT 절삭과 콜백 뒤 현재 항목 재조회 순서를 구현한다.
#include "client/MenuGumpLayout.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <string>

namespace netstorm::client {
namespace {
// 항목/메뉴의 실제 비트와 원본 disabled StyleText 접두 및 파싱 버퍼 크기다.
constexpr std::uint32_t kCommand=1,kSeparator=0x10,kChild=0x20,kParameter=0x80,kDisabled=0x200,kStyled=0x400,kDetached=0x800,kText=0x10000;
constexpr std::uint32_t kHorizontal=0x40000,kVertical=0x80000,kFancy=0x800000,kCompact=0x1000000;
constexpr std::string_view kDisabledPrefix="~*~x";
constexpr std::size_t kStyleBuffer=256;
// signed DWORD 연산은 x86처럼 하위 32비트만 보존한다.
std::int32_t Add(std::int32_t first,std::int32_t second) { return std::bit_cast<std::int32_t>(std::uint32_t(first)+std::uint32_t(second)); }
// 원본 imul의 하위 DWORD를 signed 결과로 읽는다.
std::int32_t Multiply(std::uint32_t first,std::int32_t second) { return std::bit_cast<std::int32_t>(first*std::uint32_t(second)); }
// 실제 _ftol의 signed 64비트 결과 하위 DWORD를 반환한다. 범위 밖/비유한 결과의 하위 DWORD는 0이다.
std::int32_t Ftol(double value) {
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
// signed 항목 수를 매 반복에 다시 읽는다. 음수 입력은 원본처럼 반복을 생략한다.
std::int32_t Count(const MenuGumpNode& node) { return std::bit_cast<std::int32_t>(node.itemCount); }
}
// 파생/기본 조회만 필수다. 텍스트/제목/부착 경계는 실제 도달 시 요구한다.
MenuGumpLayout::MenuGumpLayout(GumpCreateState& create,const MenuGump& menu,MenuGumpLayoutHooks hooks)
    :create_(create),menu_(menu),hooks_(std::move(hooks)) {
    if (!hooks_.node || !hooks_.baseNode) throw std::invalid_argument("메뉴 레이아웃 공유 저장소 필요");
}
// 다른 복사본을 반환하는 파생 조회를 거부한다.
MenuGumpNode& MenuGumpLayout::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.node(token) : nullptr;
    if (!node || static_cast<GumpCreateNode*>(node)!=hooks_.baseNode(token)) throw std::runtime_error("메뉴 레이아웃 저장소 없음");return *node;
}
// 자식/제목은 기본 Gump 파생 타입도 허용한다.
GumpCreateNode& MenuGumpLayout::BaseNode(std::uint32_t token) const {
    auto* node=token ? hooks_.baseNode(token) : nullptr;
    if (!node) throw std::runtime_error("메뉴 자식/제목 저장소 없음");return *node;
}
// 항목 수가 콜백에서 손상되어도 원본 배열 밖 호스트 메모리를 읽거나 쓰지 않는다.
MenuGumpItem& MenuGumpLayout::Item(std::uint32_t token,std::size_t index) const {
    auto& node=Node(token);if (index>=node.items.size()) throw std::out_of_range("메뉴 항목 범위 오류");return node.items[index];
}
// 명령/인자/자식은 조건에 따라 일부 필드만 쓰며 미작성 값은 보존한다.
void MenuGumpLayout::Append(std::uint32_t token,const char* text,std::uint32_t flags,std::uint32_t command,std::uint32_t parameter,std::uint32_t child) const {
    auto& node=Node(token);if (Count(node)>=static_cast<std::int32_t>(kMenuItemCapacity)) return;
    if (flags&kSeparator) {
        if (child || (text && *text)) throw std::invalid_argument("메뉴 구분선의 빈 문자열/자식 없음 필요");
        if (Count(node)<1 || (Item(token,node.itemCount-1).flags&kSeparator)) return;
    }
    if (flags&kCommand) Item(token,node.itemCount).command=command;
    if (flags&kParameter) Item(token,node.itemCount).parameter=parameter;
    if (child) {
        auto& below=BaseNode(child);
        if (flags&kDetached) Item(token,node.itemCount).childId=below.id;
        else {
            // 패치는 ID 저장→숨김, CD는 숨김→ID 저장 뒤 같은 가상 부착에 도달한다.
            if (create_.collection.edition==o::OriginalEdition::Patch1078) { Item(token,node.itemCount).childId=below.id;below.flags&=~1U; }
            else { below.flags&=~1U;Item(token,node.itemCount).childId=below.id; }
            if (!hooks_.attach) throw std::invalid_argument("메뉴 자식 Attach 경계 필요");hooks_.attach(child,0,0,token,0);flags|=kChild;
        }
    }
    Item(token,node.itemCount).flags=flags|kText;
    const auto index=node.itemCount;const auto context=node.textContext;SetText(token,index,text,context);
    ++node.itemCount;
    if (Count(node)>static_cast<std::int32_t>(kMenuItemCapacity)) throw std::length_error("메뉴 추가 후 항목 수 한도 초과");
    Reflow(token);
}
// plain은 실제 40바이트만 복사하고 styled는 원본 문맥 복사 시점을 유지한다.
void MenuGumpLayout::SetText(std::uint32_t token,std::size_t index,const char* text,const std::array<std::uint32_t,8>& context) const {
    auto& item=Item(token,index);if (!text) throw std::invalid_argument("메뉴 항목 문자열 필요");
    if (!(item.flags&kStyled)) {
        bool ended=false;
        // NUL을 읽은 뒤 나머지를 0으로 채우며 원본처럼 40번째 뒤에 별도 종료 문자를 쓰지 않는다.
        for (std::size_t i=0;i<item.label.size();++i) { const auto value=ended ? '\0' : text[i];item.label[i]=value;if (!value) ended=true; }
        return;
    }
    auto copy=context;copy[1]=0;
    if (!hooks_.clearStyle) throw std::invalid_argument("메뉴 StyleText 소유 경계 필요");
    hooks_.clearStyle(token,index);
    std::string source=(item.flags&kDisabled) ? std::string(kDisabledPrefix) : std::string{};source+=text;
    if (source.size()>=kStyleBuffer) throw std::length_error("메뉴 StyleText 원본 버퍼 한도 초과");
    if (!hooks_.parseStyle) throw std::invalid_argument("메뉴 StyleText 파싱 경계 필요");
    hooks_.parseStyle(token,index,source,copy);
}
// 측정 함수가 읽는 시점의 항목 플래그로 styled/plain을 선택한다.
std::array<std::int32_t,2> MenuGumpLayout::MeasureItem(std::uint32_t token,std::size_t index,const std::array<std::uint32_t,8>& context) const {
    if (Item(token,index).flags&kStyled) { if (!hooks_.styled) throw std::invalid_argument("메뉴 StyleText 측정 경계 필요");return hooks_.styled(token,index); }
    if (!hooks_.plain) throw std::invalid_argument("메뉴 일반 글자 측정 경계 필요");return hooks_.plain(token,index,context);
}
// 측정 중 바뀐 문맥/항목 수/플래그를 다시 읽고 남는 배열 칸은 초기화하지 않는다.
void MenuGumpLayout::Reflow(std::uint32_t token) const {
    auto& node=Node(token);node.margins={node.menuFlags&kVertical ? 15.0F : 4.0F,node.menuFlags&kVertical ? 15.0F : 4.0F,1,1};
    if (node.menuFlags&kCompact) node.margins[0]=node.margins[1]=3;
    std::int32_t width=0,height=0,total=0;
    // 원본은 매 항목에서 현재 기본 문맥을 복사하고 콜백 뒤 현재 항목 수를 비교한다.
    for (std::size_t i=0;static_cast<std::int32_t>(i)<Count(node);++i) {
        const auto context=node.textContext;auto size=MeasureItem(token,i,context);
        if ((node.menuFlags&kVertical) && (Item(token,i).flags&(kChild|kDetached))) {
            if (!hooks_.marker) throw std::invalid_argument("메뉴 자식 표시 측정 경계 필요");
            const auto marker=hooks_.marker(token,node.textContext);size[0]=Add(size[0],marker[0]);if (size[1]<=marker[1]) size[1]=marker[1];
        }
        size[0]=Ftol(static_cast<double>(size[0])+node.margins[0]+node.margins[1]);
        size[1]=Ftol(static_cast<double>(size[1])+node.margins[2]+node.margins[3]);
        width=std::max(width,size[0]);height=std::max(height,size[1]);total=Add(total,size[1]);
        node.heights[i]=static_cast<float>(size[1]);
    }
    if (node.menuFlags&kFancy) {
        if (node.menuFlags&kHorizontal) throw std::invalid_argument("장식 메뉴의 세로 방향 필요");
        if (!hooks_.push) throw std::invalid_argument("메뉴 제목 Push 경계 필요");
        hooks_.push(node.title);
        if (!hooks_.titleFit) throw std::invalid_argument("메뉴 제목 Fit 경계 필요");
        hooks_.titleFit(node.title,0);
        if (!hooks_.titleLayout) throw std::invalid_argument("메뉴 제목 Layout 경계 필요");
        hooks_.titleLayout(node.title,0,0);
        const auto title=node.title;const auto& above=BaseNode(title);
        width=Ftol(static_cast<double>(width)>above.baseWidth ? static_cast<double>(width) : static_cast<double>(above.baseWidth));
        if (!hooks_.attach) throw std::invalid_argument("메뉴 제목 Attach 경계 필요");
        hooks_.attach(title,above.x,above.y,0,0);
    }
    if (node.menuFlags&kVertical) {
        if (node.menuFlags&kHorizontal) throw std::invalid_argument("일반 세로 메뉴의 가로 비트 없음 필요");
        node.targetWidth=static_cast<float>(width);node.targetHeight=static_cast<float>(total);
        menu_.SetWidth(token,node.targetWidth);menu_.SetHeight(token,node.targetHeight);std::int32_t offset=0;
        // 크기 변경 콜백 뒤 현재 항목 수와 측정 높이를 다시 읽어 누적 offset을 계산한다.
        for (std::size_t i=0;static_cast<std::int32_t>(i)<Count(node);++i) {
            Item(token,i);node.offsets[i]=static_cast<float>(offset);offset=Ftol(static_cast<double>(offset)+node.heights[i]);
        }
    } else if (node.menuFlags&kHorizontal) {
        node.targetWidth=static_cast<float>(Multiply(node.itemCount,width));node.targetHeight=static_cast<float>(height);
        const auto commonHeight=static_cast<float>(height);
        menu_.SetWidth(token,node.targetWidth);menu_.SetHeight(token,node.targetHeight);
        // 모든 현재 항목 높이를 같은 높이로 맞추고 위치 offset은 0으로 쓴다.
        for (std::size_t i=0;static_cast<std::int32_t>(i)<Count(node);++i) { Item(token,i);node.heights[i]=commonHeight;node.offsets[i]=0; }
    }
    node.itemWidth=static_cast<float>(width);
}
// 라벨 끝에 NUL을 추가하지 않으며 뒤의 필드도 원본 little endian 바이트로 기록한다.
std::array<char,60> MenuGumpItemBytes(const MenuGumpItem& item) {
    std::array<char,60> result;std::copy(item.label.begin(),item.label.end(),result.begin());
    const std::array<std::uint32_t,5> words{item.childId,item.command,item.parameter,item.styled,item.flags};
    // 라벨 뒤 다섯 DWORD를 원본 필드 순서대로 쓴다.
    for (std::size_t i=0;i<words.size();++i) {
        // 각 DWORD의 낮은 BYTE부터 네 바이트를 기록한다.
        for (std::size_t b=0;b<4;++b) result[40+i*4+b]=static_cast<char>((words[i]>>(b*8))&255);
    }
    return result;
}
// 일반 항목과 자식 표시의 측정을 실제 글꼴 캐시에 연결한다.
void BindMenuLayoutText(MenuGumpLayoutHooks& hooks,const GumpText& text) {
    const auto lookup=hooks.node;if (!lookup) throw std::invalid_argument("메뉴 글자 측정 저장소 필요");
    hooks.plain=[lookup,&text](std::uint32_t token,std::size_t index,const std::array<std::uint32_t,8>& context) {
        const auto* node=lookup(token);if (!node || index>=node->items.size()) throw std::out_of_range("메뉴 글자 측정 항목 범위 오류");
        const auto bytes=MenuGumpItemBytes(node->items[index]);std::array<std::int32_t,2> size{};
        text.Measure(bytes,DecodeGumpTextContext(context),size[0],size[1]);return size;
    };
    hooks.marker=[&text](std::uint32_t,const std::array<std::uint32_t,8>& context) {
        // 원본 두 BYTE 문자와 NUL을 같은 글꼴로 측정한다.
        constexpr std::array<char,3> marker{' ','>','\0'};std::array<std::int32_t,2> size{};
        text.Measure(marker,DecodeGumpTextContext(context),size[0],size[1]);return size;
    };
}
}
