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
}
