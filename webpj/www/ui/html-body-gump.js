// 원본 HTML 부모(46c9c0/ca50/ca70/cac0/cbc0/cc30)의 두 8192BYTE 영역과 실제 다중 줄 자식의 생성/갱신/부착을 옮긴다(cpppj client/HtmlBodyGump.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 일반 Tell 팝업 부모(HtmlWindowGump)와 구별되는 내장 설명(EmbeddedTell)용 부모다. 표시 자식은 공통 GumpLifetime 이 소유하고 본문/링크 두 배열은 이 부모와 같은 수명이다.
// 원본 부모/자식의 현재 필드를 각 효과 뒤 다시 읽고, 가상 Push 뒤 위치/크기를 다시 조회하며, 첫 NUL 뒤의 기존 바이트는 생성/파싱 중 임의로 지우지 않는다.
// 사용: const body = new HtmlBodyGump(state, hooks); body.construct(token); body.setHtml(token, source, parse); body.fit(token, 0). (2026-11-15 WB04-E466)
import { GumpCreateNode } from './gump-create.js';

// 원본 +a4 본문/+20a4 링크 영역의 같은 물리 크기다.
export const HTML_BODY_CAPACITY = 8192;
// 선택된 실제 처리기가 없으면 미연결 경계를 드러낸다(C++ std::logic_error → Error).
function need(hook) { if (typeof hook !== 'function') throw new Error('HTML 부모 실제 처리기 누락'); return hook; }
// x87 의 현재 계산값으로 기본/배율 크기를 각각 저장한다(값을 먼저 float 로 좁히지 않고 배율과 곱한 배정도 값을 float32 로 저장).
function setWidth(node, value) { node.baseWidth = Math.fround(value); node.width = Math.fround(value * node.scale); }
function setHeight(node, value) { node.baseHeight = Math.fround(value); node.height = Math.fround(value * node.scale); }

// 표시 노드: 공통 기본 필드 위에 +a0 의 현재 다중 줄 자식 토큰과 본문/링크 8192BYTE 두 영역을 가진다.
export class HtmlBodyGumpNode extends GumpCreateNode {
    // +a0 현재 다중 줄 표시 자식이다(0 이면 없음).
    text = 0;
    // 첫 NUL 뒤 기존 바이트도 보존하는 본문/링크 영역이다.
    body = new Uint8Array(HTML_BODY_CAPACITY); links = new Uint8Array(HTML_BODY_CAPACITY);
}

// 생성 시 읽는 현재 화면 크기(531860/64, signed)·종류 BYTE(54042f, signed)·다중 줄 기본 색(5b5dc8)이다.
export class HtmlBodyGumpState {
    constructor(screenWidth = 0, screenHeight = 0, kind = 20, color = 0) { this.screenWidth = screenWidth; this.screenHeight = screenHeight; this.kind = kind; this.color = color; }
}

// 부모 하나의 동작이다. hooks = { node(token)→HtmlBodyGumpNode, base(token)→기본 노드, constructBase(token, command, width, height, flags), allocate(bytes)→토큰(0xd8), constructText(token, width, height, font, color),
// kill(token), push(token), attach(token, x, y, parent, mode), baseAttach(token, x, y, parent, mode), styled(token, text), scroll(token, value), slider(token, value), scrollable(token, value) }.
export class HtmlBodyGump {
    // 현재 화면/식별/색 전역과 기존 실제 생성/StyleText/트리/수명 경계다(호출보다 오래 유지).
    #state; #hooks;
    constructor(state, hooks) { this.#state = state; this.#hooks = hooks; }

    // 실제 파생 저장소가 없으면 원본 포인터 접근의 호스트 계약 위반이다.
    #node(token) { const node = need(this.#hooks.node)(token); if (!node) throw new RangeError('HTML 부모 노드 누락'); return node; }
    // 같은 기본 필드는 현재 트리의 주소여야 한다.
    #base(token) { const node = need(this.#hooks.base)(token); if (!node) throw new RangeError('HTML 부모/자식 기본 노드 누락'); return node; }

    // 46c9c0: 기본 생성(0,0,0,1) 뒤 가상 표 0x50bd38·종류(부호 확장)·자식 0·두 영역의 첫 바이트 0·화면 절반 크기(정수 나눗셈)를 설치한다. 나머지 영역 바이트는 보존한다.
    construct(token) {
        need(this.#hooks.constructBase)(token, 0, 0, 0, 1); const node = this.#node(token);
        node.vtable = 0x50bd38; node.command = ((this.#state.kind << 24) >> 24) >>> 0; node.text = 0; node.body[0] = 0; node.links[0] = 0;
        setWidth(node, Math.trunc(this.#state.screenWidth / 2)); setHeight(node, Math.trunc(this.#state.screenHeight / 2)); return token;
    }

    // 46ca50: 현재 표시 자식을 Kill 한 뒤(지연 삭제) 소유 필드를 0 으로 쓴다. 버퍼/다른 자식은 지우지 않는다.
    clear(token) { if (this.#node(token).text) { need(this.#hooks.kill)(this.#node(token).text); this.#node(token).text = 0; } }

    // 46ca70: 두 첫 바이트를 0 으로 쓰고, 현재 파서 출력이 없을 때만 두 배열/8192 한도를 설치한 뒤 실제 순회를 실행한다. parser = { hasOutput(), bind(body, links), run() }.
    parse(token, parser) {
        this.#node(token).body[0] = 0; this.#node(token).links[0] = 0;
        if (!need(parser.hasOutput)()) need(parser.bind)(this.#node(token).body, this.#node(token).links);
        need(parser.run)();
    }

    // 46cac0: 기존 자식을 Kill 하되 새 생성 전까지 그 토큰을 보존하고, 새 다중 줄(0,0,5,색)을 만들어 부착→StyleText→scroll 0→Slider 1→scrollable 1 을 하고,
    // 부모에 가상 Push 표시(flags 8)가 없으면 Push 뒤 같은 현재 x/y 로 부모를 다시 부착한다. 노드는 각 효과 뒤 다시 읽는다. text 는 첫 NUL 앞만 쓴다.
    setText(token, text) {
        const nul = text.indexOf('\0'); const source = nul < 0 ? text : text.slice(0, nul);
        if (this.#node(token).text) need(this.#hooks.kill)(this.#node(token).text);
        const child = need(this.#hooks.allocate)(0xd8); if (!child) throw new Error('HTML 다중 줄 자식 확보 실패');
        need(this.#hooks.constructText)(child, 0, 0, 5, this.#state.color); this.#node(token).text = child;
        need(this.#hooks.attach)(this.#node(token).text, 0, 0, token, 0); need(this.#hooks.styled)(this.#node(token).text, source);
        need(this.#hooks.scroll)(this.#node(token).text, 0); need(this.#hooks.slider)(this.#node(token).text, 1); need(this.#hooks.scrollable)(this.#node(token).text, 1);
        if (!(this.#node(token).flags & 8)) { need(this.#hooks.push)(token); need(this.#hooks.attach)(token, this.#node(token).x, this.#node(token).y, 0, 0); }
    }

    // 46cc60: 두 첫 NUL → 같은 8192BYTE 본문/링크를 쓰는 기본 파서(parse(source, body, links)) → 현재 본문의 setText 이다. 본문에 NUL 이 없으면 거부한다(RangeError).
    setHtml(token, source, parse) {
        this.#node(token).body[0] = 0; this.#node(token).links[0] = 0; need(parse)(source, this.#node(token).body, this.#node(token).links);
        const bytes = this.#node(token).body; const end = bytes.indexOf(0); if (end < 0) throw new RangeError('내장 TELL 본문의 실제8192BYTE NUL 범위 초과');
        let text = ''; for (let index = 0; index < end; index += 1) text += String.fromCharCode(bytes[index]);
        this.setText(token, text);
    }

    // 46cbc0: 현재 표시 자식을 Push 한 뒤 부모 기본 폭에서 가로 여백을 뺀 값(배정도)과 같은 높이를 기본/배율 크기에 각각 저장하고 (inset, 0)에 부착한다. 자식이 없으면 아무것도 하지 않는다.
    fit(token, inset) {
        if (!this.#node(token).text) return;
        need(this.#hooks.push)(this.#node(token).text);
        setWidth(this.#base(this.#node(token).text), this.#node(token).baseWidth - inset); setHeight(this.#base(this.#node(token).text), this.#node(token).baseHeight);
        need(this.#hooks.attach)(this.#node(token).text, inset, 0, token, 0);
    }

    // 46cc30: 부모의 기본 부착 뒤 현재 자식 크기를 여백 0 으로 다시 계산한다.
    attach(token, x, y, parent, mode) { need(this.#hooks.baseAttach)(token, x, y, parent, mode); this.fit(token, 0); }
}
