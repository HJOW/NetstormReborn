// 원본 다중 줄 스타일 입력 파서(004ba8f0)를 같은 TextGump 문맥/태그·입력 버퍼·그림·가상 표시 요청에 연결한다.
// (cpppj client/ScrollingTextGumpMarkup.cpp/.h, 기준선 1f00a25a, 10.78 전용 — CD 10.72 의 004b6c80·W 소리 형식은 옮기지 않는다)
// 짧은 명령(`~B` `~I` `~.` 등)과 대괄호 명령은 기존 StyleText/StyleTextMarkup 처리기를 공유하고, 이 파서는 구간 분할·리터럴 `~~`·현재 문맥/태그/제한 필드 재조회만 맡는다.
// 구간/리터럴/그림마다 appendText → dirty → redraw(false) 를 호출하며 마지막 빈 구간도 버퍼 문자열 통지를 수행한다.
// 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// (2026-11-14 WB04-E424: RawMenuControls 선행 ScrollingTextGumpMarkup 이식)

// 원본 signed 구간 길이 한도(이 값을 넘는 입력은 원본 signed 범위 밖)다.
const INT32_MAXIMUM = 0x7fffffff;
// 원문 바이트 문자열(바이트 하나가 문자 하나)을 NUL 로 끝낸 바이트 배열로 바꾼다. 입력 버퍼의 NUL 은 마지막 구간의 NUL 탐색 종료 기준이다.
function terminated(source) { const bytes = new Uint8Array(source.length + 1); for (let i = 0; i < source.length; i += 1) bytes[i] = source.charCodeAt(i) & 255; return bytes; }

// 다중 줄 스타일 파서다. 같은 판본·조각/그림/팔레트/옵션을 쓰는 처리기와 노드 저장소는 이 파서보다 오래 살아야 한다.
// 사용: new ScrollingTextGumpMarkup(scrollingTextGump, style, pictures, styleTextMarkup, { node, base }).parse(token, '안내 ~[I fortGump.*] ~B굵게~N')
export class ScrollingTextGumpMarkup {
    // 같은 다중 줄 표시기·StyleText·그림·대괄호 처리기와 노드 조회 장치다.
    #text; #style; #pictures; #markup; #hooks;
    // 조회는 생성/표시/입력 처리기와 같은 안정된 객체를 돌려줘야 한다(복사본이나 파서 전용 노드를 만들지 않는다).
    constructor(text, style, pictures, markup, hooks = {}) {
        if (typeof hooks.node !== 'function' || typeof hooks.base !== 'function') throw new TypeError('다중 줄 스타일 공유 노드 경계 필요');
        this.#text = text; this.#style = style; this.#pictures = pictures; this.#markup = markup; this.#hooks = hooks;
    }

    // 상위 처리기의 현재 기본 객체와 동일한 저장소를 써야 태그/문맥 변경이 입력/표시에 바로 반영된다. 소멸·미등록 토큰은 거부한다.
    #node(token) {
        const node = token ? this.#hooks.node(token) : null;
        if (!node || node !== this.#hooks.base(token)) throw new Error('다중 줄 스타일 공유 노드 없음');
        return node;
    }

    // 현재 노드의 +c8 태그를 읽고 쓰는 소유 필드 별칭이다(호출 시점의 노드에 묶인다).
    #tag(token) { const node = this.#node(token); return { get value() { return node.metadata; }, set value(value) { node.metadata = value >>> 0; } }; }

    // 문자열의 첫 NUL 까지만 처리한다. 현재 기본 문맥을 복사해 시작하되 `~.`/`~n` 은 현재 기본 문맥을 다시 읽는다. T 태그는 +c8 에 호출 간 유지된다.
    // 원본 끝 `~` 의 버퍼 밖 접근 대신 마지막 빈 입력을 수행하고, 닫히지 않은 대괄호는 `~[` 두 글자만 소비한다. text 는 바이트 문자열(또는 Uint8Array)이다.
    parse(token, text) {
        if (text instanceof Uint8Array) { let value = ''; for (let i = 0; i < text.length; i += 1) value += String.fromCharCode(text[i]); text = value; }
        if (text.length > INT32_MAXIMUM) throw new RangeError('다중 줄 스타일 원본 signed 구간 길이 한도 초과');
        const context = Uint32Array.from(this.#node(token).textContext);
        const nul = text.indexOf('\0'); const source = nul < 0 ? text : text.slice(0, nul);
        let position = 0;
        // 명령 앞의 문자열을 먼저 더한다. 소리/통지가 바꾼 현재 태그/기본 문맥은 다음 구간에서 다시 사용한다.
        for (;;) {
            const mark = source.indexOf('~', position);
            const remaining = terminated(source.slice(position));
            if (mark < 0) { this.#text.appendText(token, remaining, context, -1, this.#node(token).metadata); return; }
            if (mark > position) this.#text.appendText(token, remaining, context, mark - position, this.#node(token).metadata);
            const command = source[mark + 1] ?? '\0'; let next = mark + 2;
            if (command === '~') {
                // 리터럴도 길이 1 의 버퍼 입력이므로 줄 추가 통지와 redraw 를 생략하지 않는다.
                this.#text.appendText(token, Uint8Array.of(126, 0), context, 1, this.#node(token).metadata);
            } else if (command === '[') {
                const close = source.indexOf(']', next);
                if (close >= 0) {
                    // 현재 +d4 콜백은 실제 그림 검사 시 읽는다. 그림 태그도 추가 직전에 현재 +c8 을 재조회한다.
                    this.#markup.applyScrolling(source.slice(next, close), context, this.#tag(token), () => this.#node(token).callback !== 0,
                        (type, frame, width, height) => { this.#text.appendPicture(token, this.#pictures, type, frame, this.#node(token).metadata, width, height); });
                    next = close + 1;
                }
            } else if (!this.#style.applyShortCommand(command, context, this.#node(token).textContext, this.#node(token).callback !== 0)) next = mark + 1;
            // 끝에서 멈추도록 위치를 제한한다(원본 끝 ~ 뒤의 마지막 빈 입력은 다음 반복이 수행한다).
            position = Math.min(next, source.length);
        }
    }
}
