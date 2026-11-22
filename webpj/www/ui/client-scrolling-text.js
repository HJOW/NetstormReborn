// 실제 Client Gump·글꼴·버퍼·Slider에 원본 다중 줄 표시와 입력을 연결한다.
import { GumpDrawState } from '../render/gump-draw.js';
import { ScrollingTextGump, ScrollingTextGumpNode } from './text-gump.js';
import { ScrollingTextGumpWidgets } from './scrolling-text-gump-widgets.js';
import { ScrollingTextGumpMarkup } from './scrolling-text-gump-markup.js';

// 같은 현재 트리와 글자 소유자의 다중 줄 파생 노드를 조립한다.
export class ClientScrollingText {
    // 공통 장치와 기본 표시·가상 입력·스타일 파서다.
    #root; #fonts; #text; #buffers; #scroll; #hooks; #clip = new GumpDrawState(); #body; #widgets; #parser;
    // styleMarkup은 같은 Client 글자의 실제 대괄호 처리기다.
    constructor(root, fonts, text, buffers, scroll, styleMarkup, state, cursor) {
        if (!root || !fonts || !text || !buffers || !scroll || !styleMarkup || !state || typeof cursor !== 'function') throw new TypeError('Client 다중 줄 Gump의 실제 장치 누락');
        this.#root = root; this.#fonts = fonts; this.#text = text; this.#buffers = buffers; this.#scroll = scroll;
        const node = (token) => root.node(token), slider = scroll.backend();
        this.#hooks = { node, baseNode: node, buffer: (token) => buffers.binding(token),
            sliderRange: (...args) => slider.setRange(...args), sliderPage: (...args) => slider.setPage(...args),
            sliderSetPosition: (...args) => slider.setPosition(...args), sliderPosition: (token) => slider.position(token), redraw: (token, erase) => root.redraw(token, erase) };
        this.#body = this.#makeBody(this.#clip);
        this.#widgets = new ScrollingTextGumpWidgets(root.createState(), state, root.factory(), { attach: (...args) => root.attachBase(...args) }, root.lifetime(), root.events(), root.input(),
            text.styleState(), buffers, this.#body, scroll, { node, slider: (token) => scroll.node(token), base: node,
                allocate: (size) => { if (size === 0x38) return buffers.allocate(); if (size === 0xd8) return scroll.allocate(); throw new RangeError('다중 줄 Gump 자식 할당 크기'); },
                push: (token) => root.tree().push(token), attach: (...args) => root.attach(...args), redraw: (token, erase) => root.redraw(token, erase), cursor });
        this.#parser = new ScrollingTextGumpMarkup(this.#body, text.styleEngine(), buffers.pictures(), styleMarkup, { node, base: node });
    }
    // 현재 클립만 바꾸고 같은 스타일·버퍼·Slider 저장소를 사용하는 표시기다.
    #makeBody(clip) { return new ScrollingTextGump(this.#root.createState(), this.#root.collection(), clip, this.#text.styleState(), this.#text.styleEngine(), this.#hooks); }
    // 공통 입력·부착·방송·수명과 현재 픽셀 출력을 같은 파생 노드에 연결한다.
    allocate() {
        const root = this.#root, node = new ScrollingTextGumpNode(); let token = 0;
        token = root.allocateDerived(node, (event, first, second) => this.#widgets.handle(token, event, first, second),
            (current, flags) => this.#widgets.deleteScalar(current, flags), (current, x, y, parent, mode) => this.#widgets.attach(current, x, y, parent, mode),
            (current, event, first, second) => this.#widgets.broadcast(current, event, first, second));
        root.bindDraw(token, (current, target, clip) => this.#text.withTarget(target, () => this.#makeBody(clip).draw(current))); return token;
    }
    // 실제 버퍼와 숨긴 Slider를 원본 순서로 생성한다.
    construct(token, width, height, font, color) { return this.#widgets.construct(token, width, height, font, color); }
    // 현재 글꼴 인코딩의 스타일 문자열을 같은 다중 줄 파서에 추가한다.
    style(token, value) { this.#parser.parse(token, this.#fonts.encode(value)); }
    // 줄바꿈 없이 실제 내용과 여백에 맞춘다.
    fit(token, width = 0, height = 0) { this.#body.fitContent(token, width, height); }
    // 현재 다중 줄 표시기의 원본 스크롤 요청을 전달한다.
    requestScroll(token, position) { this.#body.requestScroll(token, position); }
    // 현재 Slider 표시 여부를 원본 가상 부착으로 적용한다.
    setSlider(token, enabled) { this.#widgets.setSlider(token, enabled ? 1 : 0); }
    // 로그 본문 전체를 바이트 문자열로 바꾼다(원본 `ReplaceText`, 점 명령 `.clear` 가 빈 문자열로 비운다). (2026-11-22 WB04-E1172)
    replaceText(token, bytes, metadata = 0) { this.#body.replaceText(token, bytes, metadata); }
    // 현재 맨 아래 보기 여부(원본 `AtBottom`, 내용 높이 − 기본 높이 − 10 이하가 현재 위치)이다. 채팅 창이 로그를 추가하기 전 판정에 쓴다. (2026-11-22 WB04-E1169)
    atBottom(token) { return this.#body.atBottom(token); }
    // 맨 아래 보기를 다음 그리기 때 적용하도록 요청한다(원본 `RequestBottom`, extra 는 저장만 한다). (2026-11-22 WB04-E1169)
    requestBottom(token, extra = 0) { this.#body.requestBottom(token, extra); }
    // 현재 위치에서 delta 만큼 상대 스크롤을 요청한다(음수는 0 으로 제한). (2026-11-22 WB04-E1169)
    requestRelativeScroll(token, delta) { this.#body.requestRelativeScroll(token, delta); }
    // 스크롤 가능 비트의 최하위 비트를 바꾼다(원본 `SetScrollable`, 끄면서 Slider 만 켜져 있으면 Slider 도 끈다). (2026-11-22 WB04-E1169)
    setScrollable(token, value) { this.#widgets.setScrollable(token, value); }
    // 현재 소유 버퍼를 놓고 다른 버퍼 토큰으로 바꾼다(원본 `ReplaceBuffer`, 채팅 창이 공유 로그 버퍼를 빌려 쓰게 한다). (2026-11-22 WB04-E1169)
    replaceBuffer(token, buffer, owns) { this.#widgets.replaceBuffer(token, buffer, owns); }
}
