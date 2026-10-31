// 원본 다중 줄 스타일 글자(ScrollingTextGump + StyleTextBuffer)를 복원하는 모듈이다. HTML 대화상자의 본문을 담당한다.
// 입력 줄(개행으로 끝난 줄)을 폭에 맞춰 표시 줄로 나누고, 크기 맞춤(FitContent·FitHeight)과 그리기(클립 + 줄 순서 출력)를 한다.
// 원본 주소: 버퍼 추가 004b9790, 줄 나눔 004b9a50(Layout)·분할(SplitText), 최대 폭 004b9ed0, 크기 맞춤 004b9fb0(FitContent)·004b9f20(FitHeight),
//            그리기 ScrollingTextGump::Draw. 복원 기준: cpppj StyleTextBuffer.cpp·TextGump.cpp(10.78 경로).
// 스크롤 위치(setScroll)·스크롤바(VerticalSlider, ui/slider.js)와의 연동·링크 태그 판정(hitTest)은 도움말 창(WB03-G)이 쓴다. Tell 대화상자는 스크롤 위치 0 에서 위쪽부터 그린다.
// 포인터 아래의 커서 모양(cursorAt)은 링크 위 손·스크롤 가능한 본문 위 상하 화살표·그 밖 기본 화살표를 고른다.
// (2026-10-28 WB03-D: Tell 대화상자 본문을 그리기 위해 추가, 2026-10-30 WB03-G: 스크롤·링크 판정 추가, 2026-10-31 WB03-L: 커서 모양 판정 추가)
import { clipRect } from '../render/indexed-surface.js';
import { createFragment, drawChain, drawFragment, measureFragment } from './style-text.js';
import { CURSOR_LINK, CURSOR_NORMAL, CURSOR_SCROLL } from './cursor.js';

// 원본 입력 줄 한 칸 수(생성 인자 128)이다. 이를 넘는 줄은 맨 앞부터 버린다. (2026-10-28 WB03-D)
export const SCROLL_TEXT_MAX_LINES = 128;

// 스크롤바 폭(픽셀)이다. 크기 맞춤이 스크롤바를 쓰는 상태에서 폭에 더하거나 뺀다(원본 생성 인자 9). (2026-10-28 WB03-D)
export const SCROLL_TEXT_SLIDER_WIDTH = 9;

// FitContent 가 줄 나눔을 계산할 때 임시로 쓰는 매우 넓은 폭이다(원본 상수 10000.0). 줄이 접히지 않은 자연 폭을 재려는 것이다. (2026-10-28 WB03-D)
const FIT_WIDTH = 10000;

// 32비트 정수 덧셈(감김 유지)이다. (2026-10-28 WB03-D)
function add(a, b) {
    return (a + b) | 0;
}

// 32비트 정수 뺄셈(감김 유지)이다. (2026-10-30 WB03-G)
function sub(a, b) {
    return (a - b) | 0;
}

// 원본 CRT _ftol 처럼 0 방향으로 잘라 32비트 정수로 만든다. 비유한 값은 0 이다. (2026-10-30 WB03-G)
function ftol(value) {
    return Number.isFinite(value) ? Math.trunc(value) | 0 : 0;
}

// 다중 줄 스타일 글자 Gump 하나이다. 입력 줄·표시 줄과 원본 노드 필드(크기·배율·위치)를 가진다.
// 사용: const text = new ScrollingText(fonts, defaultContext); text.append(bytes, context, tag)... → text.fitContent() / text.fitHeight() → text.draw(...).
// 위치(x, y)는 부모 기준이 아니라 호출자가 정한 화면 좌표이며, baseWidth/baseHeight 는 배율 적용 전 크기, width/height 는 배율 적용 크기(float32)다.
// (2026-10-28 WB03-D)
export class ScrollingText {
    // fonts 는 글자 측정·그리기 글꼴 묶음, defaultContext 는 `~.` 가 돌아갈 기본 글자 문맥이다.
    constructor(fonts, defaultContext) {
        // 글꼴 묶음(FontSet)이다.
        this.fonts = fonts;
        // 기본 글자 문맥이다(원본 +0x78..: {0, 글꼴, 색, 0, 0, 0, 1, 1}).
        this.defaultContext = defaultContext;
        // 배율 적용 전 폭(원본 +0x3c)이다.
        this.baseWidth = 0;
        // 배율 적용 전 높이(원본 +0x40)다.
        this.baseHeight = 0;
        // 배율(원본 +0x38)이다. 대화상자는 1 이다.
        this.scale = 1;
        // 배율 적용 폭(원본 +0x44, float32)이다. 그리기 클립에 쓴다.
        this.width = 0;
        // 배율 적용 높이(원본 +0x48, float32)다.
        this.height = 0;
        // 화면 x 위치다(float32). 부모 위치와 합쳐 정한다.
        this.x = 0;
        // 화면 y 위치다(float32).
        this.y = 0;
        // 스크롤바를 쓰는지 여부다(원본 textFlags 비트 2). 켜면 줄 나눔 폭에서 스크롤바 폭을 뺀다.
        this.useSlider = false;
        // 개행으로 끝난 입력 줄 목록이다. 각 줄은 조각 배열이다. 마지막 칸(미완료 줄)은 pending 에 둔다.
        this.hardLines = [];
        // 아직 개행하지 않은 끝 줄의 조각 배열이다.
        this.pending = [];
        // 폭에 맞춰 나눈 표시 줄 목록이다. 각 줄은 { fragments, height, width, y } 이다.
        this.softLines = [];
        // 줄 나눔을 막는지 여부다(원본 noWrap). 대화상자 본문은 false 이다.
        this.noWrap = false;
        // 마지막으로 줄 나눔에 쓴 폭이다(원본 +0xb4 layoutWidth). 폭이 바뀐 채 그리면 부착 시 dirty 가 켜져 다시 나누는 원본 동작을 같게 한다.
        this.layoutWidth = 0;
        // 글자를 더한 뒤 아직 줄 나눔을 하지 않았는지 여부다(원본 textFlags 의 dirty 비트). 스크롤·그리기·판정 전에 다시 나눈다. (2026-10-30 WB03-G)
        this.dirty = false;
        // 현재 스크롤 위치(픽셀, 원본 노드 scroll)이다. 위쪽에서 보이는 첫 높이이며 setScroll 이 범위 안으로 맞춘다. (2026-10-30 WB03-G)
        this.scroll = 0;
        // 연결된 스크롤바(VerticalSlider)이다. 없으면 null 이다. 연결하면 줄 나눔 폭에서 스크롤바 폭을 뺀다. (2026-10-30 WB03-G)
        this.slider = null;
    }

    // 스크롤바를 연결한다(원본 생성자가 만드는 폭 9 의 Slider). 연결 즉시 줄 나눔 폭에서 스크롤바 폭을 뺀다. (2026-10-30 WB03-G)
    attachSlider(slider) {
        this.slider = slider;
        this.useSlider = true;
        this.dirty = true;
    }

    // 스크롤바를 본문 오른쪽 가장자리에 붙이고 높이를 본문 높이로 맞춘다(원본 Attach: 스크롤바 위치 = 본문 폭 - 스크롤바 폭, y 0). 본문 위치(x, y)와 크기를 정한 뒤 부른다. (2026-10-30 WB03-G)
    placeSlider() {
        if (!this.slider) return;
        this.slider.setHeight(this.baseHeight);
        this.slider.attach(Math.fround(this.x + (this.baseWidth - this.slider.baseWidth)), this.y);
    }

    // 줄 나눔이 오래됐으면(글자를 더했거나 폭이 바뀜) 다시 나눈다(원본 EnsureLayout). (2026-10-30 WB03-G)
    ensureLayout() {
        if (this.dirty || this.layoutWidth !== this.baseWidth) this._relayout();
    }

    // 스크롤 위치를 정한다(원본 SetScroll). 위치를 (전체 높이 - 보이는 높이) 이하, 0 이상으로 맞추고 스크롤바의 범위·쪽 크기·위치에 반영한 뒤 스크롤바가 다시 맞춘 값을 받는다.
    // 스크롤바가 없으면 맞춘 값만 저장한다. 사용: body.setScroll(0) → 맨 위. (2026-10-30 WB03-G)
    setScroll(position) {
        this.ensureLayout();
        const height = this.totalHeight();
        const page = ftol(this.baseHeight < 0 ? 0 : this.baseHeight);
        let result = position | 0;
        if (height < add(page, result)) result = sub(height, page);
        if (result < 0) result = 0;
        this.scroll = result;
        if (this.slider) {
            this.slider.setRange(0, height);
            this.slider.setPage(page);
            this.slider.setPosition(this.scroll);
            this.scroll = this.slider.position;
        }
    }

    // 현재 위치에서 delta 만큼 상대 스크롤한다(원본 RequestRelativeScroll: 0 아래로는 가지 않음). (2026-10-30 WB03-G)
    scrollBy(delta) {
        this.setScroll(Math.max(add(delta, this.scroll), 0));
    }

    // 스크롤바 단계 요청을 처리한다(원본 본문 Handle 의 stepEvent): 1 = 아래 5, 2 = 위 5, 3 = 한 쪽(본문 높이) 아래, 4 = 한 쪽 위. 처리했으면 true 이다. (2026-10-30 WB03-G)
    handleStep(code) {
        let delta;
        if (code === 1) delta = 5;
        else if (code === 2) delta = -5;
        else if (code === 3) delta = ftol(this.baseHeight);
        else if (code === 4) delta = ftol(-this.baseHeight);
        else return false;
        this.scrollBy(delta);
        return true;
    }

    // 화면 좌표가 가리키는 글자 조각을 찾는다(원본 ScrollingTextGump::HitTest). 본문 경계 밖이면 null 이다.
    // 스크롤로 보이는 첫 줄부터 훑어 눌린 줄을 찾고, 줄 폭 밖이면 null, 줄 안에서는 조각 폭을 누적해 x 가 들어가며 세로로도 (줄 높이 - 조각 높이 + 줄 위쪽) 아래인 조각을 돌려준다.
    // 링크 조각이면 tag 가 0 이 아니다. 사용: const fragment = body.hitTest(x, y); if (fragment?.tag) ... (2026-10-30 WB03-G)
    hitTest(x, y) {
        const bounds = this.bounds();
        if (x < bounds.left || x >= bounds.right || y < bounds.top || y >= bounds.bottom) return null;
        this.ensureLayout();
        const localX = sub(x, bounds.left);
        const localY = sub(y, bounds.top);
        // 위 끝이 스크롤 위치에 걸치는 첫 줄을 찾는다(줄 끝 y 가 스크롤과 같으면 그 줄을 유지한다).
        let index = 0;
        while (index < this.softLines.length && add(this.softLines[index].y, this.softLines[index].height) < this.scroll) index += 1;
        if (index >= this.softLines.length) return null;
        let top = sub(this.softLines[index].y, this.scroll);
        // 본문 높이 아래로 나가지 않는 줄을 위에서 아래로 훑는다.
        for (; index < this.softLines.length && top < this.baseHeight; index += 1) {
            const line = this.softLines[index];
            if (localY < add(line.height, top)) {
                if (line.width <= localX) return null;
                let width = 0;
                // 조각 폭을 누적해 가로로 들어가는 조각을 찾는다.
                for (const fragment of line.fragments) {
                    const size = measureFragment(this.fonts, fragment);
                    width = add(width, size.width);
                    if (localX < width && add(sub(line.height, size.height), top) < localY) return fragment;
                }
                return null;
            }
            top = add(top, line.height);
        }
        return null;
    }

    // 화면 좌표 위에서 보일 커서 번호를 고른다(원본 ScrollingTextGumpWidgets::Handle 의 커서 사건).
    //  1) 링크 조각(tag 가 0 이 아님) 위면 CURSOR_LINK(16), 2) 그렇지 않고 스크롤바가 달려 있으며 쪽 크기가 범위(최대 - 최소)보다 작으면 CURSOR_SCROLL(15), 3) 그 밖은 CURSOR_NORMAL(1).
    // 커서 사건은 그 점에서 가장 위에 있는 위젯인 본문에만 간다(원본 GumpInput::Overlay 의 Pick). 본문 경계 밖이거나 본문 위에 얹힌 스크롤바 위면 본문이 받지 않아
    // 원본은 반환 0 의 기본값(번호 1)으로 되돌리므로 CURSOR_NORMAL 이다. 스크롤바가 없으면(slider 가 null, 원본 kScroll 비트 꺼짐) 스크롤 모양은 없다.
    // 사용: const number = body.cursorAt(pointer.x, pointer.y); (2026-10-31 WB03-L)
    cursorAt(x, y) {
        const bounds = this.bounds();
        if (x < bounds.left || x >= bounds.right || y < bounds.top || y >= bounds.bottom) return CURSOR_NORMAL;
        if (this.slider?.contains(x, y)) return CURSOR_NORMAL;
        const fragment = this.hitTest(x, y);
        if (fragment && fragment.tag) return CURSOR_LINK;
        if (this.slider && this.slider.page < sub(this.slider.maximum, this.slider.minimum)) return CURSOR_SCROLL;
        return CURSOR_NORMAL;
    }

    // 글자를 끝 줄에 추가한다(원본 StyleTextBuffer::AppendText). 줄바꿈(LF)마다 입력 줄을 닫고, LF 앞의 빈 구간도 빈 조각으로 저장한다.
    // 줄이 가득 차면(128줄) 맨 앞 완료 줄을 버린다. bytes 는 Windows-1252 바이트, context 는 글자 문맥, tag 는 링크 태그다.
    append(bytes, context, tag = 0) {
        this.dirty = true;
        let start = 0;
        // 문자열의 각 바이트에서 LF 를 찾아 입력 줄을 나눈다.
        for (let position = 0; position < bytes.length; position += 1) {
            if (bytes[position] !== 0x0a) continue;
            if (this.hardLines.length === SCROLL_TEXT_MAX_LINES - 1) this.hardLines.shift();
            this.pending.push(createFragment(bytes.subarray(start, position), context, tag));
            this.hardLines.push(this.pending);
            this.pending = [];
            start = position + 1;
        }
        // 마지막 LF 뒤 남은 글자는 아직 닫히지 않은 끝 줄의 조각이다.
        if (start < bytes.length) this.pending.push(createFragment(bytes.subarray(start), context, tag));
    }

    // 스타일 문자열(조각 배열로 나뉜 것)을 모두 추가한다. 조각마다 자신의 문맥으로 append 한다.
    // 그림 조각은 끝 줄에 그대로 붙인다(원본 AppendPicture: 줄바꿈 처리 없음). (2026-10-30 WB03-G)
    appendFragments(fragments) {
        // 각 조각을 같은 문맥·태그로 추가한다.
        for (const fragment of fragments) {
            if (fragment.picture) {
                this.dirty = true;
                this.pending.push(fragment);
            } else {
                this.append(fragment.bytes, fragment.context, fragment.tag);
            }
        }
    }

    // 표시 줄 목록의 새 줄을 만든다. y 는 만들 때 앞 줄의 y + 높이로 정한다(원본 NewSoftLine).
    _newSoftLine() {
        const previous = this.softLines[this.softLines.length - 1];
        const line = { fragments: [], height: 0, width: 0, y: previous ? add(previous.y, previous.height) : 0 };
        this.softLines.push(line);
        return line;
    }

    // 한 입력 조각이 한 표시 줄에 다 들어가지 않을 때 단어(공백) 경계로 나눠 여러 줄에 담는다(원본 SplitText).
    // 줄의 현재 폭 + 단어 폭이 width 이상이면 줄을 바꾼다. 줄이 비어 있으면 너무 긴 단어도 그대로 한 줄에 둔다.
    // 반환값은 마지막 단어가 놓인 표시 줄이다.
    _splitText(fragment, startLine, width) {
        const bytes = fragment.bytes;
        let line = startLine;
        let consumed = 0;
        // 입력 조각의 글자를 모두 배치할 때까지 반복한다.
        while (consumed < bytes.length) {
            let accepted = 0;
            let newLine = false;
            let copy = true;
            // 이미 받아들인 공백 뒤부터 다음 단어까지 접두 폭을 재며 한 줄에 들어갈 만큼 받아들인다.
            while (consumed + accepted < bytes.length) {
                const suffix = bytes.subarray(consumed);
                let end = accepted;
                // 다음 공백(또는 끝)을 찾는다.
                while (end < suffix.length && suffix[end] !== 0x20) end += 1;
                const size = this.fonts.measure(suffix.subarray(0, end), fragment.context);
                if (add(line.width, size.width) >= width) {
                    newLine = true;
                    if (accepted === 0) {
                        if (line.width !== 0) copy = false;
                        else accepted = end + 1;
                    }
                    break;
                }
                accepted = end + 1;
            }
            if (copy) {
                const piece = createFragment(bytes.subarray(consumed, Math.min(consumed + accepted, bytes.length)), fragment.context, fragment.tag);
                const size = measureFragment(this.fonts, piece);
                line.height = Math.max(line.height, size.height);
                line.width = add(line.width, size.width);
                line.fragments.push(piece);
                consumed += accepted;
            }
            if (newLine) line = this._newSoftLine();
        }
        return line;
    }

    // 입력 줄을 폭 width 로 표시 줄로 나눈다(원본 StyleTextBuffer::Layout). width 가 0 이하이면 기존 표시 줄을 유지한다.
    // 개행으로 끝난 입력 줄만 대상이며, 한 입력 줄은 최소 한 개의 표시 줄이 된다(빈 줄도 글꼴 높이를 가진다).
    layout(width) {
        if (width <= 0) return;
        this.softLines = [];
        let line = null;
        // 완료된 입력 줄마다 새 표시 줄을 시작하고 조각을 차례로 배치한다.
        for (const hard of this.hardLines) {
            line = this._newSoftLine();
            // 입력 줄의 각 조각을 현재 표시 줄에 담거나, 넘치면 단어 단위로 나눠 담는다.
            for (const fragment of hard) {
                const size = measureFragment(this.fonts, fragment);
                if (add(line.width, size.width) < width || this.noWrap) {
                    line.height = Math.max(line.height, size.height);
                    line.width = add(line.width, size.width);
                    line.fragments.push(fragment);
                } else if (fragment.picture) {
                    // 그림은 쪼갤 수 없으므로 줄에 이미 내용이 있으면 새 줄에서 시작한다.
                    if (line.width !== 0) line = this._newSoftLine();
                    line.height = Math.max(line.height, size.height);
                    line.width = add(line.width, size.width);
                    line.fragments.push(fragment);
                } else {
                    line = this._splitText(fragment, line, width);
                }
            }
        }
    }

    // 표시 줄 중 가장 넓은 폭이다(원본 MaximumWidth). 줄이 없으면 0 이다.
    maximumWidth() {
        let result = 0;
        // 모든 표시 줄의 폭 중 최댓값을 찾는다.
        for (const line of this.softLines) result = Math.max(result, line.width);
        return result;
    }

    // 마지막 표시 줄의 y + 높이, 곧 전체 본문 높이다(원본 TotalHeight). 줄이 없으면 0 이다.
    totalHeight() {
        const last = this.softLines[this.softLines.length - 1];
        return last ? add(last.y, last.height) : 0;
    }

    // 현재 폭(스크롤바 폭 제외)으로 줄 나눔을 다시 한다(원본 ScrollingTextGump::Layout: ftol(baseWidth - 스크롤바 폭)).
    _relayout() {
        const width = this.baseWidth - (this.useSlider ? SCROLL_TEXT_SLIDER_WIDTH : 0);
        this.layoutWidth = this.baseWidth;
        this.dirty = false;
        this.layout(Math.trunc(width) | 0);
    }

    // 배율 적용 폭·높이를 기본 폭·높이에서 다시 계산한다(원본: 기본 값 저장 뒤 × 배율을 float32 로 저장).
    setSize(baseWidth, baseHeight) {
        this.baseWidth = Math.fround(baseWidth);
        this.width = Math.fround(baseWidth * this.scale);
        this.baseHeight = Math.fround(baseHeight);
        this.height = Math.fround(baseHeight * this.scale);
    }

    // 폭만 바꾼다(원본 대화상자 맞춤이 기본 폭 → 배율 폭을 따로 쓰는 부분).
    setWidth(baseWidth) {
        this.baseWidth = Math.fround(baseWidth);
        this.width = Math.fround(baseWidth * this.scale);
    }

    // 높이만 바꾼다.
    setHeight(baseHeight) {
        this.baseHeight = Math.fround(baseHeight);
        this.height = Math.fround(baseHeight * this.scale);
    }

    // 현재 폭에서 줄을 나누고 높이를 맞춘다(원본 FitHeight, 004b9f20). 높이 = 전체 본문 높이 + 1 이며 빈 본문도 높이 1 이 된다.
    fitHeight() {
        this._relayout();
        this.setHeight(add(this.totalHeight(), 1));
    }

    // 내용에 맞춰 크기를 정한다(원본 FitContent, 004b9fb0). 폭을 아주 넓게(10000) 잡고 줄 나눔을 한 뒤,
    // 폭 = 최대 줄 폭 + 여백 + 스크롤바 폭(쓸 때) + 1, 높이 = 전체 높이 + 여백으로 정한다.
    fitContent(padWidth = 0, padHeight = 0) {
        this.setWidth(FIT_WIDTH);
        this._relayout();
        const extra = this.useSlider ? SCROLL_TEXT_SLIDER_WIDTH : 0;
        this.setWidth(this.maximumWidth() + padWidth + extra + 1);
        this.setHeight(add(this.totalHeight(), padHeight));
    }

    // 화면 정수 경계(원본 GumpCollection::Bounds)다. 왼쪽 위는 위치를 절삭한 값, 오른쪽 아래는 그 값 + 크기를 절삭한 값이다.
    bounds() {
        const left = Math.trunc(this.x) | 0;
        const top = Math.trunc(this.y) | 0;
        return { left, top, right: Math.trunc(left + this.width) | 0, bottom: Math.trunc(top + this.height) | 0 };
    }

    // 본문을 그린다(원본 ScrollingTextGump::Draw). 경계로 클립하고, 표시 줄을 위에서 아래로 그린다.
    // 각 줄은 줄 안 최대 기준선에 맞춰 조각을 이어 그리며, 줄 시작 y 는 앞 줄 높이를 누적한다. 경계 아래로 나가는 줄은 그리지 않는다.
    // 스크롤 위치(scroll)만큼 위쪽이 가려진 첫 줄부터 그린다(원본 FirstVisible: 줄 끝 y 가 스크롤보다 작은 줄은 건너뜀). 스크롤바는 그리지 않는다(drawSlider).
    draw(surface, fonts, clip = surface.bounds) {
        // 줄 나눔 뒤 폭이 바뀌었으면(대화상자 맞춤이 마지막에 폭만 되돌린 경우) 그리기 전에 다시 나눈다(원본 부착 때 dirty → EnsureLayout).
        this.ensureLayout();
        const bounds = this.bounds();
        const area = clipRect(bounds, clipRect(clip, surface.bounds));
        let index = 0;
        // 위 끝이 스크롤 위치에 걸치는 첫 줄을 찾는다.
        while (index < this.softLines.length && add(this.softLines[index].y, this.softLines[index].height) < this.scroll) index += 1;
        let y = index < this.softLines.length ? sub(this.softLines[index].y, this.scroll) : 0;
        // 표시 줄을 위에서 아래로 그린다. 줄 위쪽이 본문 높이 이상이면 멈춘다(경계 동률은 그리지 않음).
        for (; index < this.softLines.length; index += 1) {
            const line = this.softLines[index];
            if (!(y < this.baseHeight)) break;
            drawChain(surface, fonts, line.fragments, bounds.left, (bounds.top + y) | 0, area);
            y = add(y, line.height);
        }
    }

    // 연결된 스크롤바를 그린다(본문 위에 겹쳐 그림). 스크롤바가 없으면 아무것도 하지 않는다. resources 는 돌 버튼과 같은 자원 묶음이다. (2026-10-30 WB03-G)
    drawSlider(surface, resources) {
        this.slider?.draw(surface, resources);
    }
}
