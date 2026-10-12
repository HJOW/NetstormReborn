// 원본 조각의 NUL 복사·가상 순회·문맥 변경과 콜백 뒤 현재 next/head 재조회를 구현한다.
#include "client/StyleText.h"
#include "client/MenuGumpLayout.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace netstorm::client {
namespace {
// 문자열 길이 -1과 토큰 최댓값의 DWORD 비트다. 0은 null/자동 토큰 발급 소진 상태다.
constexpr std::uint32_t kDwordMaximum=0xffffffffU;
// x86 signed 덧셈은 하위 DWORD만 보존한다.
std::int32_t Add(std::int32_t first,std::int32_t second) {
    return std::bit_cast<std::int32_t>(std::uint32_t(first)+std::uint32_t(second));
}
// x86 signed 뺄셈도 하위 DWORD만 보존한다.
std::int32_t Subtract(std::int32_t first,std::int32_t second) {
    return std::bit_cast<std::int32_t>(std::uint32_t(first)-std::uint32_t(second));
}
// 같은 토큰을 다시 순회하는 무효 호스트 체인을 거부한다. 정상 순회 순서는 바꾸지 않는다.
void Visit(std::unordered_set<std::uint32_t>& seen,std::uint32_t token) {
    if (!seen.insert(token).second) throw std::runtime_error("StyleText 순환 체인");
}
// 원본 단일 소문자 색 명령이다. CD판에는 a/d/p/s/u가 없다.
constexpr std::string_view kCommonColors="bgijklmnoqrtwxy";
constexpr std::string_view kPatchColors="adpsu";
// 용도가 확정된 기본 색은 실제 basic 별칭을 사용한다.
constexpr std::array<std::pair<char,std::size_t>,7> kBasicColors{{
    {'k',0},{'r',1},{'g',2},{'b',3},{'m',4},{'y',5},{'o',6}
}};
// 기본색 외 색 전역별 목표 RGB다. 중복 RGB의 별도 전역은 여기서 사용하지 않는다.
struct NamedCommandColor { char command;std::uint8_t red,green,blue; };
constexpr std::array<NamedCommandColor,11> kNamedColors{{
    {'a',255,153,0},{'d',0,255,102},{'i',249,233,145},{'j',233,214,186},
    {'l',151,215,210},{'p',255,102,204},{'q',200,175,144},{'s',99,66,255},
    {'t',219,213,203},{'u',51,255,102},{'x',47,24,17}
}};
}

// 경계는 실제 도달 시 확인한다. 파싱/연결만 수행하는 처리기에는 글꼴/출력이 없어도 된다.
StyleText::StyleText(StyleTextState& state,StyleTextHooks hooks,o::OriginalEdition edition)
    :state_(state),hooks_(std::move(hooks)),edition_(edition) {}

// map은 조각 삽입으로 기존 주소를 이동하지 않는다. 삭제된 토큰은 재사용하지 않는다.
StyleTextFragment& StyleText::Node(std::uint32_t token) const {
    const auto it=state_.fragments.find(token);
    if (!token || it==state_.fragments.end()) throw std::out_of_range("StyleText 조각 저장소 없음");
    return it->second;
}

// 명시 길이는 중간 NUL도 복사하지만 측정/출력은 원본처럼 첫 NUL에서 멈춘다.
std::uint32_t StyleText::NewText(std::span<const char> text,const std::array<std::uint32_t,8>& context,
    std::uint32_t length,std::uint32_t metadata) const {
    std::size_t count=length;
    if (length==kDwordMaximum) {
        const auto end=std::find(text.begin(),text.end(),'\0');
        if (end==text.end()) throw std::invalid_argument("StyleText 문자열 범위에 NUL 필요");
        count=static_cast<std::size_t>(end-text.begin());
    } else if (count>text.size()) throw std::out_of_range("StyleText 문자열 복사 범위 초과");
    StyleTextFragment fragment;
    fragment.metadata=metadata;
    fragment.context=context;
    fragment.text.assign(text.begin(),text.begin()+count);
    fragment.text.push_back('\0');
    auto token=state_.nextToken;
    // 외부 파생 조각이 먼저 등록한 토큰은 덮어쓰지 않고 다음 빈 토큰을 사용한다.
    while (token && state_.fragments.contains(token)) token=token==kDwordMaximum ? 0 : token+1;
    if (!token) throw std::length_error("StyleText 자동 토큰 발급 소진");
    state_.fragments.emplace(token,std::move(fragment));
    state_.nextToken=token==kDwordMaximum ? 0 : token+1;
    return token;
}

// 원본 호출 인자 push 0→push [+8]을 유지한다. +8을 문자열 길이와 무관한 ID로 추정하지 않는다.
std::uint32_t StyleText::CloneText(std::uint32_t token) const {
    const auto& fragment=Node(token);
    if (fragment.kind!=StyleTextFragmentKind::Text) throw std::invalid_argument("StyleText 문자열 조각 복제 필요");
    return NewText(fragment.text,fragment.context,fragment.metadata,0);
}

// 현재 next를 읽어가며 마지막 조각을 반환한다.
std::uint32_t StyleText::Tail(std::uint32_t head) const {
    std::unordered_set<std::uint32_t> seen;
    // 저장소 밖 조회와 무한 순회를 막으면서 원본 단방향 꼬리 순서를 유지한다.
    while (true) {
        Visit(seen,head);
        const auto next=Node(head).next;
        if (!next) return head;
        head=next;
    }
}

// Append는 단일 조각의 next를 0으로 만든다. 이미 연결된 체인을 이전하려면 Transfer를 사용한다.
void StyleText::Append(std::uint32_t& head,std::uint32_t fragment) const {
    Node(fragment);
    if (!head) head=fragment;
    else Node(Tail(head)).next=fragment;
    Node(fragment).next=0;
}

// source 전체 체인을 이전하므로 source의 첫 next를 지우지 않는다.
void StyleText::Transfer(std::uint32_t& head,std::uint32_t& source) const {
    if (&head==&source) throw std::invalid_argument("StyleText 소유 필드 자기 이전");
    if (!head) head=source;
    else Node(Tail(head)).next=source;
    source=0;
}

// 텍스트 문자열을 먼저 해제한 다음 next를 지우며 scalar 반납 직전의 경계를 호출한다.
void StyleText::Destroy(std::uint32_t token,std::uint8_t flags) const {
    auto& fragment=Node(token);
    if (fragment.kind==StyleTextFragmentKind::Text) {
        std::vector<char>{}.swap(fragment.text);
        fragment.next=0;
    } else {
        if (!hooks_.externalDestroy) throw std::invalid_argument("StyleText 파생 삭제 경계 필요");
        hooks_.externalDestroy(token,flags);
    }
    if (flags&1U) {
        if (hooks_.release) hooks_.release(token);
        state_.fragments.erase(token);
    }
}

// 현재 head는 마지막에 지운다. 삭제 콜백이 바꾼 새 head를 순회에 다시 넣지 않는다.
void StyleText::Clear(std::uint32_t& head) const {
    auto token=head;
    std::unordered_set<std::uint32_t> seen;
    // 다음 조각을 scalar 삭제 전에 저장한다.
    while (token) {
        Visit(seen,token);
        const auto next=Node(token).next;
        Destroy(token,1);
        token=next;
    }
    head=0;
}

// 첫 두 메트릭만 배치에 쓰고 하강 값은 원본 wrapper처럼 별도 임시 출력으로 버린다.
std::array<std::int32_t,2> StyleText::Baseline(std::uint32_t token) const {
    const auto& fragment=Node(token);
    if (fragment.kind==StyleTextFragmentKind::External) {
        if (!hooks_.externalMetrics) throw std::invalid_argument("StyleText 파생 메트릭 경계 필요");
        return hooks_.externalMetrics(token);
    }
    if (!hooks_.textMetrics) throw std::invalid_argument("StyleText 폰트 메트릭 경계 필요");
    const auto metrics=hooks_.textMetrics(fragment.context);
    return {metrics[0],metrics[1]};
}

// 측정은 저장 문맥을 사용한다. 출력 override 문맥으로 크기를 계산하지 않는다.
std::array<std::int32_t,2> StyleText::MeasureFragment(std::uint32_t token) const {
    const auto& fragment=Node(token);
    if (fragment.kind==StyleTextFragmentKind::External) {
        if (!hooks_.externalMeasure) throw std::invalid_argument("StyleText 파생 측정 경계 필요");
        return hooks_.externalMeasure(token);
    }
    if (!hooks_.textMeasure) throw std::invalid_argument("StyleText 문자열 측정 경계 필요");
    std::array<std::int32_t,2> result{};
    hooks_.textMeasure(fragment.text,fragment.context,result[0],result[1]);
    return result;
}

// override의 색/스타일/그림자 등은 사용하지만 폰트 슬롯은 원본 조각의 것을 유지한다.
void StyleText::DrawFragment(std::uint32_t token,ScreenPoint position,const std::array<std::uint32_t,8>* overrideContext) const {
    const auto& fragment=Node(token);
    if (fragment.kind==StyleTextFragmentKind::External) {
        if (!hooks_.externalDraw) throw std::invalid_argument("StyleText 파생 출력 경계 필요");
        hooks_.externalDraw(token,position,overrideContext);
        return;
    }
    if (!hooks_.textDraw) throw std::invalid_argument("StyleText 문자열 출력 경계 필요");
    if (overrideContext) {
        auto context=*overrideContext;
        context[1]=fragment.context[1];
        hooks_.textDraw(fragment.text,position,context);
    } else hooks_.textDraw(fragment.text,position,fragment.context);
}

// CD는 출력 초기화 전에 첫 head를 읽고, 패치는 초기화 뒤 읽는다. 두 번째 순회는 현재 소유 필드부터 시작한다.
void StyleText::Measure(const std::uint32_t& head,std::int32_t& width,std::int32_t& height) const {
    const bool patch=edition_==o::OriginalEdition::Patch1078;
    auto token=patch ? 0U : head;
    height=0;
    width=0;
    if (patch) token=head;
    if (!token) return;
    std::int32_t ascent=0;
    std::unordered_set<std::uint32_t> seen;
    // 첫 순회는 최대 상승만 구하고 각 가상 호출 후 현재 next를 읽는다.
    while (token) {
        Visit(seen,token);
        ascent=std::max(ascent,Baseline(token)[1]);
        token=Node(token).next;
    }
    token=head;
    seen.clear();
    // 현재 head부터 baseline을 다시 호출하고 실제 조각 크기로 높이/폭을 순서대로 쓴다.
    while (token) {
        Visit(seen,token);
        const auto baseline=Baseline(token);
        const auto size=MeasureFragment(token);
        // CD는 출력 쓰기 전에 next를 저장한다. 출력이 next와 겹치면 패치는 쓴 뒤의 next를 사용한다.
        const auto next=patch ? 0U : Node(token).next;
        height=std::max(height,Add(Subtract(size[1],baseline[1]),ascent));
        width=Add(width,size[0]);
        token=patch ? Node(token).next : next;
    }
}

// Draw의 head 인자는 값이므로 첫 순회 중 소유 필드가 바뀌어도 두 번째 시작 토큰은 유지한다.
std::int32_t StyleText::Draw(std::uint32_t head,ScreenPoint position,const std::array<std::uint32_t,8>* overrideContext) const {
    if (!head) return 0;
    std::int32_t height=0,ascent=0;
    auto token=head;
    std::unordered_set<std::uint32_t> seen;
    // 첫 순회에서 캐시 높이와 상승의 최대값을 각각 구한다.
    while (token) {
        Visit(seen,token);
        const auto baseline=Baseline(token);
        height=std::max(height,baseline[0]);
        ascent=std::max(ascent,baseline[1]);
        token=Node(token).next;
    }
    token=head;
    seen.clear();
    // 기준선으로 y를 맞추며 조각 출력 콜백 후에도 이번 측정 폭으로 x를 증가시킨다.
    while (token) {
        Visit(seen,token);
        const auto baseline=Baseline(token);
        const auto size=MeasureFragment(token);
        DrawFragment(token,{position.x,Add(position.y,Subtract(ascent,baseline[1]))},overrideContext);
        position.x=Add(position.x,size[0]);
        token=Node(token).next;
    }
    return height;
}

// 파서의 색 명령은 현재 원본 팔레트 전역을 조회한다.
std::uint32_t StyleText::Color(char command) const {
    if (!hooks_.color) throw std::invalid_argument("StyleText 팔레트 색 경계 필요");
    return hooks_.color(command);
}

// 기존 head를 Clear하지 않는다. 원본 파서는 매 구간의 문맥을 복사한 새 조각을 꼬리에 추가한다.
void StyleText::Parse(std::uint32_t& head,std::string_view text,const std::array<std::uint32_t,8>& initialContext) const {
    if (text.size()>=kDwordMaximum) throw std::length_error("StyleText 원본 DWORD 문자열 길이 한도 초과");
    const std::string source(text.substr(0,text.find('\0')));
    auto context=initialContext;
    const auto original=initialContext;
    const bool patch=edition_==o::OriginalEdition::Patch1078;
    std::size_t position=0;
    // ~ 앞 구간을 먼저 추가한 뒤 명령을 처리한다. 마지막 NUL 구간은 빈 경우에도 추가한다.
    while (true) {
        const auto mark=source.find('~',position);
        const auto remaining=std::span<const char>(source.c_str()+position,source.size()-position+1);
        if (mark==std::string::npos) {
            Append(head,NewText(remaining,context));
            return;
        }
        if (mark>position) Append(head,NewText(remaining,context,static_cast<std::uint32_t>(mark-position)));
        const auto command=source[mark+1];
        auto next=mark+2;
        switch (command) {
        case '*': context[0]=0;break;
        case '.': context=original;break;
        case '0': case '1': case '3': case '4': case '5': case '6': context[1]=command-'0';break;
        case 'B': context[3]=2;break;
        case 'I': context[3]=1;break;
        case 'N': context[0]&=~2U;context[3]=0;break;
        case 'E': context[0]|=2U;context[5]=Color('k');context[6]=1;context[7]=1;break;
        case 'S': if (patch) context[3]=3;else next=mark+1;break;
        case 'U': if (patch) context[3]=4;else next=mark+1;break;
        case 'n': context[2]=original[2];break;
        case '~': {
            // 리터럴 ~는 그 시점의 문맥을 보관한 별도 문자열 조각이다.
            constexpr std::array<char,2> literal{'~','\0'};
            Append(head,NewText(literal,context));
            break;
        }
        case '[': {
            const auto close=source.find(']',next);
            if (close!=std::string::npos) {
                if (!hooks_.bracket) throw std::invalid_argument("StyleText 대괄호 그림/옵션 경계 필요");
                hooks_.bracket(std::string_view(source).substr(next,close-next),context,head);
                next=close+1;
            }
            break;
        }
        default:
            if (kCommonColors.find(command)!=std::string_view::npos ||
                (patch && kPatchColors.find(command)!=std::string_view::npos)) context[2]=Color(command);
            else next=mark+1;
            break;
        }
        // 끝의 명령은 NUL 다음을 읽는 대신 원본의 마지막 빈 조각으로 종료한다.
        position=std::min(next,source.size());
    }
}

// 실제 측정기는 저장 문맥을 그대로 읽는다. 메트릭의 첫 두 DWORD는 캐시 높이/상승이다.
void BindStyleTextFonts(StyleTextHooks& hooks,const GumpText& text,const GumpTextDraw* draw) {
    hooks.textMetrics=[&text](const std::array<std::uint32_t,8>& context) {
        return text.Metrics(DecodeGumpTextContext(context));
    };
    // 조각 문자열은 NUL을 포함하므로 기존 -1 길이 계약을 사용한다.
    hooks.textMeasure=[&text](std::span<const char> bytes,const std::array<std::uint32_t,8>& context,std::int32_t& width,std::int32_t& height) {
        text.Measure(bytes,DecodeGumpTextContext(context),width,height);
    };
    if (draw) {
        // FontStore와 현재 픽셀 표면을 사용하는 기존 실제 글리프 출력으로 위임한다.
        hooks.textDraw=[draw](std::span<const char> bytes,ScreenPoint position,const std::array<std::uint32_t,8>& context) {
            draw->Draw(bytes,position,DecodeGumpTextContext(context));
        };
    }
}

// 기본색은 별칭을, 나머지는 원본 RGB 검색 결과를 사용한다. 별도 가상 팔레트를 만들지 않는다.
void BindStyleTextPalette(StyleTextHooks& hooks,const PaletteColorTable& palette) {
    hooks.color=[&palette](char command) -> std::uint32_t {
        if (command=='w') return palette.basic[7];
        // 원본 기본 색 전역에 해당하는 명령을 찾는다.
        for (const auto [code,index]:kBasicColors) if (code==command) return palette.basic[index];
        // 원본 추가 색과 UI 색의 RGB 항목을 찾는다.
        for (const auto& color:kNamedColors) {
            if (color.command!=command) continue;
            // 해당 원본 색 표에 실제로 있는 결과만 반환한다.
            for (const auto& named:palette.named) {
                if (named.red==color.red && named.green==color.green && named.blue==color.blue) return named.index;
            }
            throw std::invalid_argument("StyleText 원본 색 표 항목 없음");
        }
        throw std::invalid_argument("StyleText 팔레트 명령 범위 오류");
    };
}

// 메뉴 항목의 실제 소유 필드를 참조로 넘겨 Clear/Parse 이후 현재 head를 같은 저장소에 남긴다.
void BindMenuLayoutStyleText(MenuGumpLayoutHooks& hooks,const StyleText& text) {
    const auto lookup=hooks.node;
    if (!lookup) throw std::invalid_argument("StyleText 메뉴 저장소 필요");
    // 콜백 때 도달한 항목만 확인한다. node는 메뉴 크기/기본 저장소와 같은 안정된 객체여야 한다.
    const auto head=[lookup](std::uint32_t token,std::size_t index) -> std::uint32_t& {
        auto* node=lookup(token);
        if (!node || index>=node->items.size()) throw std::out_of_range("StyleText 메뉴 항목 범위 오류");
        return node->items[index].styled;
    };
    // 기존 조각을 scalar 삭제한 뒤 소유 필드를 비운다.
    hooks.clearStyle=[head,&text](std::uint32_t token,std::size_t index) { text.Clear(head(token,index)); };
    // MenuGumpLayout이 준비한 disabled 접두와 font=0 문맥을 원본 짧은 명령 파서에 넘긴다.
    hooks.parseStyle=[head,&text](std::uint32_t token,std::size_t index,std::string_view source,const std::array<std::uint32_t,8>& context) {
        text.Parse(head(token,index),source,context);
    };
    // 두 번 순회하는 실제 조각 측정 결과를 메뉴 레이아웃에 공급한다.
    hooks.styled=[head,&text](std::uint32_t token,std::size_t index) {
        std::array<std::int32_t,2> size{};
        text.Measure(head(token,index),size[0],size[1]);
        return size;
    };
}
}
