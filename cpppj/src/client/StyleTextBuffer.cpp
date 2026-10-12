// 입력/표시 줄의 독립 소유권과 원본의 엄격한 폭 비교·공백 포함 복사를 복원한다.
#include "client/StyleTextBuffer.h"
#include "client/Markup.h"
#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>
#include <unordered_set>

namespace netstorm::client {
namespace {
// NUL 탐색 길이 -1과 자동 표시 줄 토큰의 마지막 DWORD다.
constexpr std::uint32_t kDwordMaximum=0xffffffffU;
// 원본 signed 크기/위치/줄 수 합산의 하위 DWORD 감김을 보존한다.
std::int32_t Add(std::int32_t first,std::int32_t second) {
    return std::bit_cast<std::int32_t>(std::uint32_t(first)+std::uint32_t(second));
}
// 잘못된 호스트 링크로 무한 순회하는 경우만 거부한다.
void Visit(std::unordered_set<std::uint32_t>& seen,std::uint32_t token) {
    if (!seen.insert(token).second) throw std::runtime_error("StyleTextBuffer 순환 링크");
}
// CD 004b5ba0/패치의 동일 인라인 반복이다. space/NUL 앞까지 읽으며 다른 공백 문자는 유지한다.
std::size_t WordEnd(std::span<const char> bytes,std::size_t position) {
    // 원본 시작 인덱스부터 단어의 끝만 찾는다. 연속 space도 하나씩 처리한다.
    while (position<bytes.size() && bytes[position]!='\0' && bytes[position]!=' ') ++position;
    if (position==bytes.size()) throw std::out_of_range("StyleTextBuffer 단어 범위에 NUL 필요");
    return position;
}
}

// 원본 생성자처럼 전체 입력 칸을 비우고 표시 센티널/카운터/상태/문자열 통지를 초기화한다.
StyleTextBuffer::StyleTextBuffer(StyleTextBufferState& state,StyleTextState& fragments,const StyleText& style,
    std::int32_t maxLines,StyleTextBufferHooks hooks)
    :state_(state),fragments_(fragments),style_(style),hooks_(std::move(hooks)) {
    if (maxLines<1) throw std::invalid_argument("StyleTextBuffer 양수 최대 줄 수 필요");
    if (state.initialized || !state.hardLines.empty() || !state.softLines.empty() || state.sentinel.head)
        throw std::invalid_argument("StyleTextBuffer 새 저장소 필요");
    state_.hardLines.assign(static_cast<std::size_t>(maxLines),0);
    state_.maxLines=maxLines;state_.lineCount=0;state_.softCount=0;state_.sentinel={};
    state_.reserved=0;state_.dirty=0;state_.noWrap=0;state_.nextSoftToken=1;state_.onAppend={};state_.initialized=true;
}

// 외부 상태가 배열 크기를 바꾼 경우에도 원본 범위 밖 접근 대신 명확한 오류로 멈춘다.
void StyleTextBuffer::RequireStorage() const {
    if (!state_.initialized || state_.maxLines<1 || state_.hardLines.size()!=static_cast<std::size_t>(state_.maxLines))
        throw std::invalid_argument("StyleTextBuffer 입력 저장소 없음");
}

// 완료 줄 수와 미완료 칸을 혼동하지 않는다. 마지막 한 칸도 정상 입력 저장소다.
std::uint32_t& StyleTextBuffer::CurrentLine() const {
    RequireStorage();
    if (state_.lineCount<0 || state_.lineCount>=state_.maxLines) throw std::out_of_range("StyleTextBuffer 끝 줄 범위 초과");
    return state_.hardLines[static_cast<std::size_t>(state_.lineCount)];
}

// 그림/문자열과 표시 복제도 같은 공통 토큰 표에 있다.
StyleTextFragment& StyleTextBuffer::Fragment(std::uint32_t token) const {
    const auto it=fragments_.fragments.find(token);
    if (!token || it==fragments_.fragments.end()) throw std::out_of_range("StyleTextBuffer 공통 조각 없음");
    return it->second;
}

// map 삽입은 기존 줄 주소를 이동시키지 않는다. 센티널은 일반 소유 조각이 아니다.
StyleTextBufferLine& StyleTextBuffer::Line(std::uint32_t token) const {
    if (!token) return state_.sentinel;
    const auto it=state_.softLines.find(token);
    if (it==state_.softLines.end()) throw std::out_of_range("StyleTextBuffer 표시 줄 없음");
    return it->second;
}

// release 경계에서 소유 head를 관찰할 수 있도록 삭제 중에는 해당 head를 바꾸지 않는다.
void StyleTextBuffer::DeleteChain(std::uint32_t head) const {
    std::unordered_set<std::uint32_t> seen;
    // next를 먼저 저장하고 실제 공통 문자열/그림 scalar 삭제로 반납한다.
    while (head) {
        Visit(seen,head);
        const auto next=Fragment(head).next;
        style_.Destroy(head,1);
        head=next;
    }
}

// 문자열 통지는 길이/NUL 탐색 전에 수행한다. 이후의 실제 입력/문맥은 현재 값을 읽는다.
void StyleTextBuffer::AppendText(std::span<const char> bytes,const std::array<std::uint32_t,8>& context,
    std::int32_t length,std::uint32_t metadata) const {
    RequireStorage();
    const auto notice=state_.onAppend;
    if (notice) notice(bytes);
    std::size_t count=0;
    if (length<0) {
        const auto end=std::find(bytes.begin(),bytes.end(),'\0');
        if (end==bytes.end()) throw std::invalid_argument("StyleTextBuffer 음수 길이에 NUL 필요");
        count=static_cast<std::size_t>(end-bytes.begin());
    } else count=static_cast<std::size_t>(length);
    if (count>bytes.size()) throw std::out_of_range("StyleTextBuffer 입력 길이 범위 초과");
    if (count>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
        throw std::length_error("StyleTextBuffer 원본 signed 입력 길이 초과");
    std::size_t start=0;
    // LF 앞의 빈 구간도 조각으로 저장한다. CR/탭은 개행으로 정규화하지 않는다.
    for (std::size_t position=0;position<count;++position) {
        if (bytes[position]!='\n') continue;
        CurrentLine();
        if (state_.lineCount==state_.maxLines-1) RemoveLine(0);
        const auto token=style_.NewText(bytes.subspan(start,position-start),context,static_cast<std::uint32_t>(position-start),metadata);
        style_.Append(CurrentLine(),token);
        state_.lineCount=Add(state_.lineCount,1);
        start=position+1;
    }
    if (start<count) {
        const auto token=style_.NewText(bytes.subspan(start,count-start),context,static_cast<std::uint32_t>(count-start),metadata);
        style_.Append(CurrentLine(),token);
    }
    state_.dirty=1;
}

// 그림 추가는 줄 수를 늘리지 않는다. 새 조각의 배율/색 기본값은 기존 Markup 생성자가 담당한다.
void StyleTextBuffer::AppendPicture(const Markup& pictures,std::uint32_t type,std::uint32_t frame,
    std::uint32_t metadata,std::int32_t fitWidth,std::int32_t fitHeight) const {
    RequireStorage();
    const auto token=pictures.NewPicture(type,frame,metadata);
    if (fitWidth || fitHeight) pictures.Fit(token,fitWidth,fitHeight);
    style_.Append(CurrentLine(),token);
    state_.dirty=1;
}

// 이동은 완료 줄 수가 아닌 전체 최대 칸까지 수행하므로 끝의 미완료 체인도 앞으로 옮겨진다.
void StyleTextBuffer::RemoveLine(std::int32_t index) const {
    RequireStorage();
    if (index<0 || index>=state_.maxLines-1 || index>=state_.lineCount)
        throw std::out_of_range("StyleTextBuffer 삭제 완료 줄 범위 오류");
    DeleteChain(state_.hardLines[static_cast<std::size_t>(index)]);
    // 각 칸을 옮긴 뒤 원래 칸을 0으로 쓴다. 마지막 미사용 칸도 같은 순서로 비운다.
    for (auto position=index+1;position<state_.maxLines;++position) {
        state_.hardLines[static_cast<std::size_t>(position-1)]=state_.hardLines[static_cast<std::size_t>(position)];
        state_.hardLines[static_cast<std::size_t>(position)]=0;
    }
    state_.lineCount=Add(state_.lineCount,-1);state_.dirty=1;
}

// 입력 원본은 남기고 이전 표시 조각만 scalar 반납한다.
void StyleTextBuffer::ClearSoft(bool resetCount) const {
    auto token=state_.sentinel.next;
    std::unordered_set<std::uint32_t> seen;
    // 현재 줄의 next를 삭제 전에 저장하며 마지막에는 센티널로 돌아온다.
    while (token) {
        Visit(seen,token);
        const auto next=Line(token).next;
        DeleteChain(Line(token).head);
        auto& line=Line(token);
        line.head=0;line.next=0;line.previous=0;
        state_.softLines.erase(token);
        token=next;
    }
    state_.sentinel.next=0;state_.sentinel.previous=0;
    if (resetCount) state_.softCount=0;
}

// 지정 전임 바로 뒤에 연결한다. y는 삽입 시 전임 높이로만 정하며 이후 다시 계산하지 않는다.
std::uint32_t StyleTextBuffer::NewSoftLine(std::uint32_t previous) const {
    const auto next=Line(previous).next;
    auto token=state_.nextSoftToken;
    // 반납된 토큰은 재사용하지 않고 외부에서 먼저 넣은 토큰도 덮어쓰지 않는다.
    while (token && state_.softLines.contains(token)) token=token==kDwordMaximum ? 0 : token+1;
    if (!token) throw std::length_error("StyleTextBuffer 표시 줄 토큰 소진");
    StyleTextBufferLine line;
    line.next=next;line.previous=previous;line.y=Add(Line(previous).y,Line(previous).height);
    state_.softLines.emplace(token,line);
    state_.nextSoftToken=token==kDwordMaximum ? 0 : token+1;
    Line(next).previous=token;Line(previous).next=token;
    state_.softCount=Add(state_.softCount,1);
    return token;
}

// 저장 문자열 끝의 실제 NUL도 명시 길이 복사에 사용할 수 있게 범위에 포함한다.
std::span<const char> StyleTextBuffer::TextBytes(std::uint32_t token) const {
    const auto& node=Fragment(token);
    if (node.kind!=StyleTextFragmentKind::Text) throw std::invalid_argument("StyleTextBuffer 문자열 조각 필요");
    const auto end=std::find(node.text.begin(),node.text.end(),'\0');
    if (end==node.text.end()) throw std::invalid_argument("StyleTextBuffer 저장 문자열에 NUL 필요");
    const auto count=static_cast<std::size_t>(end-node.text.begin());
    if (count>=static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
        throw std::length_error("StyleTextBuffer 원본 signed 문자열 길이 초과");
    return {node.text.data(),count+1};
}

// 마지막 단어 뒤 NUL도 length에 포함할 수 있다. 부분 조각 측정은 그 조각의 NUL 탐색을 사용한다.
void StyleTextBuffer::SplitText(std::uint32_t token,std::uint32_t& line,std::int32_t width) const {
    const auto length=TextBytes(token).size()-1;
    std::size_t consumed=0;
    // 이미 배치한 바이트를 제외한 문자열에서 다음 표시 조각을 찾는다.
    while (consumed<length) {
        std::size_t accepted=0;
        bool newLine=false,copy=true;
        // 이전에 허용한 공백 뒤부터 다음 단어까지의 전체 접두 폭을 측정한다.
        while (consumed+accepted<length) {
            const auto suffix=TextBytes(token).subspan(consumed);
            const auto end=WordEnd(suffix,accepted);
            const auto size=hooks_.measureText(suffix,Fragment(token).context,static_cast<std::int32_t>(end));
            if (Add(Line(line).width,size[0])>=width) {
                newLine=true;
                if (!accepted) {
                    if (Line(line).width) copy=false;
                    else accepted=end+1;
                }
                break;
            }
            accepted=end+1;
        }
        if (copy) {
            const auto& source=Fragment(token);
            const auto fragment=style_.NewText(TextBytes(token).subspan(consumed),source.context,static_cast<std::uint32_t>(accepted),source.metadata);
            const auto size=style_.MeasureFragment(fragment);
            Line(line).height=std::max(Line(line).height,size[1]);
            Line(line).width=Add(Line(line).width,size[0]);
            style_.Append(Line(line).head,fragment);
            consumed+=accepted;
        }
        if (newLine) line=NewSoftLine(line);
    }
}

// 완료 입력 줄마다 빈 표시 줄을 먼저 만들고 원본/표시 조각의 소유권을 분리한다.
void StyleTextBuffer::Layout(std::int32_t width) const {
    RequireStorage();
    if (width<=0) return;
    ClearSoft(true);
    std::uint32_t line=0;
    // 마지막 미완료 칸은 순회하지 않는다. 개행이 완료한 원본 줄만 표시한다.
    for (std::int32_t index=0;index<state_.lineCount;++index) {
        if (index>=state_.maxLines) throw std::out_of_range("StyleTextBuffer 완료 줄 수 범위 초과");
        auto token=state_.hardLines[static_cast<std::size_t>(index)];
        line=NewSoftLine(line);
        std::unordered_set<std::uint32_t> seen;
        // 가상 측정/복제 이후 현재 원본 next를 다시 읽는다.
        while (token) {
            Visit(seen,token);
            if (Fragment(token).kind!=StyleTextFragmentKind::Text) {
                const auto size=style_.MeasureFragment(token);
                // 비문자 조각의 높이는 이동 전에 현재 줄에 쓴다. 새 줄에서 임의로 다시 보정하지 않는다.
                Line(line).height=std::max(Line(line).height,size[1]);
                if (Add(Line(line).width,size[0])>=width && !state_.noWrap && (Line(line).head || size[0]<width))
                    line=NewSoftLine(line);
                Line(line).width=Add(Line(line).width,size[0]);
                if (!hooks_.cloneExternal) throw std::invalid_argument("StyleTextBuffer 비문자 가상 복제 경계 필요");
                const auto clone=hooks_.cloneExternal(token);
                style_.Append(Line(line).head,clone);
            } else {
                if (!hooks_.measureText) throw std::invalid_argument("StyleTextBuffer 현재 문자열 측정 경계 필요");
                const auto size=hooks_.measureText(TextBytes(token),Fragment(token).context,-1);
                if (Add(Line(line).width,size[0])<width || state_.noWrap) {
                    Line(line).height=std::max(Line(line).height,size[1]);
                    Line(line).width=Add(Line(line).width,size[0]);
                    const auto& source=Fragment(token);
                    const auto clone=style_.NewText(TextBytes(token),source.context,kDwordMaximum,source.metadata);
                    style_.Append(Line(line).head,clone);
                } else SplitText(token,line,width);
            }
            token=Fragment(token).next;
        }
    }
}

// 부호 있는 최대값의 초기값은 0이며 순회 횟수는 현재 softCount다.
std::int32_t StyleTextBuffer::MaximumWidth() const {
    RequireStorage();
    std::int32_t result=0;
    auto token=state_.sentinel.next;
    // 센티널이 예상보다 빨리 나오면 잘못된 호스트 카운터를 거부한다.
    for (std::int32_t index=0;index<state_.softCount;++index) {
        if (!token) throw std::out_of_range("StyleTextBuffer 표시 줄 수/링크 불일치");
        result=std::max(result,Line(token).width);
        token=Line(token).next;
    }
    return result;
}

// 원본 TextGump의 스크롤 상한 계산에 쓰는 마지막 두 필드의 합이다.
std::int32_t StyleTextBuffer::TotalHeight() const {
    RequireStorage();
    if (state_.softCount<1) return 0;
    const auto& last=Line(state_.sentinel.previous);
    return Add(last.y,last.height);
}

// 원본 반납 순서와 완료 줄/미완료 칸의 구분을 유지한다. 카운터를 소멸자에서 재초기화하지 않는다.
void StyleTextBuffer::Destroy() const {
    RequireStorage();
    // 완료 입력 줄만 scalar 삭제하고 각 해당 칸을 0으로 쓴다.
    for (std::int32_t index=0;index<state_.lineCount;++index) {
        if (index>=state_.maxLines) throw std::out_of_range("StyleTextBuffer 소멸 완료 줄 수 범위 초과");
        DeleteChain(state_.hardLines[static_cast<std::size_t>(index)]);
        state_.hardLines[static_cast<std::size_t>(index)]=0;
    }
    std::vector<std::uint32_t>{}.swap(state_.hardLines);
    ClearSoft(false);
    DeleteChain(state_.sentinel.head);
    state_.sentinel.head=0;state_.sentinel.next=0;state_.sentinel.previous=0;
    state_.initialized=false;
}

// 기존 FontStore의 첫 폭 표와 별도 논리 슬롯 높이를 명시 길이/NUL 측정에 공급한다.
void BindStyleTextBufferFonts(StyleTextBufferHooks& hooks,const GumpText& text) {
    hooks.measureText=[&text](std::span<const char> bytes,const std::array<std::uint32_t,8>& context,std::int32_t length) {
        std::array<std::int32_t,2> size{};
        text.Measure(bytes,DecodeGumpTextContext(context),size[0],size[1],length);
        return size;
    };
}

// 그림 외 다른 파생 복제는 기존 경계를 유지한다. 출력 조각은 같은 Markup 표에 다시 등록한다.
void BindStyleTextBufferMarkup(StyleTextBufferHooks& hooks,const Markup& pictures) {
    const auto previous=hooks.cloneExternal;
    hooks.cloneExternal=[&pictures,previous](std::uint32_t token) {
        if (pictures.Contains(token)) return pictures.ClonePicture(token);
        if (!previous) throw std::invalid_argument("StyleTextBuffer 그림 외 파생 복제 경계 필요");
        return previous(token);
    };
}
}
