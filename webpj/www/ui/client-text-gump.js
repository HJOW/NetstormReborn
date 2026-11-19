// 실제 Client Gump·FontSet·StyleText를 한 줄 글자 생성과 출력에 연결한다.
// 사용: new ClientTextGump(root, fonts, baseHandle, styleHooks).allocate() 후 construct/style/fit.
import { TextGump, TextGumpNode, TextGumpState } from './text-gump.js';
import { StyleText, StyleTextState, bindStyleTextFonts, styleTextField } from './style-text-fragments.js';
import { GumpDrawState } from '../render/gump-draw.js';
import { GumpText } from '../render/gump-text.js';
import { GumpTextDraw, GumpGlyphColorState, decodeGumpTextContext, makeGumpGlyphPixelHooks } from '../render/gump-text-draw.js';

// 같은 Gump 노드와 스타일 조각·실제 글꼴 및 현재 클립을 소유한다.
export class ClientTextGump {
    // 생성·소멸용 문맥과 실제 출력 중의 픽셀 대상이다.
    #root; #fonts; #baseHandle; #state = new TextGumpState(); #clip = new GumpDrawState(); #styleState = new StyleTextState(); #style; #target = null; #colors = new GumpGlyphColorState();
    // 스타일의 이미지·색상 같은 선택 경계는 호출자가 제공한다.
    constructor(root, fonts, baseHandle, styleHooks = {}) {
        if (!root || !fonts || typeof baseHandle !== 'function') throw new TypeError('Client 글자 Gump의 실제 수명·글꼴 누락');
        this.#root = root; this.#fonts = fonts; this.#baseHandle = baseHandle;
        const hooks = typeof styleHooks === 'function' ? styleHooks(this.#styleState, () => this.#target) : { ...styleHooks };
        bindStyleTextFonts(hooks, new GumpText(fonts));
        hooks.textDraw = (bytes, position, words) => {
            if (!this.#target) throw new Error('Client 글자 Gump의 현재 출력 표면 누락');
            new GumpTextDraw(fonts, makeGumpGlyphPixelHooks(this.#target, this.#colors)).draw(bytes, position, decodeGumpTextContext(words));
        };
        this.#style = new StyleText(this.#styleState, hooks);
    }
    // 이미지 등 외부 스타일 조각의 실제 등록표를 빌린다.
    styleState() { return this.#styleState; }
    // 같은 조각 체인을 사용하는 실제 스타일 엔진을 빌린다.
    styleEngine() { return this.#style; }
    // 현재 노드와 공통 수명을 같은 TextGump 본체에 빌린다.
    #body(clip = this.#clip) {
        const root = this.#root; return new TextGump(root.createState(), this.#state, root.factory(), root.lifetime(), root.collection(), clip, this.#style, { node: (token) => root.node(token), baseNode: (token) => root.node(token) });
    }
    // 실제 한 줄 파생 노드의 가상 입력·출력·소멸을 등록한다.
    allocate() {
        const root = this.#root, node = new TextGumpNode(); let token = 0;
        token = root.allocateDerived(node, (event, first, second) => this.#baseHandle(token, event, first, second),
            (current, flags) => this.#body().deleteScalar(current, flags), null, () => {});
        root.bindDraw(token, (current, target, clip) => { const previous = this.#target; this.#target = target; try { this.#body(clip).draw(current); } finally { this.#target = previous; } });
        return token;
    }
    // 원본 TextGump를 초기화하고 스타일·크기를 같은 조각 체인에 적용한다.
    construct(token, width, height, font, color) { return this.#body().construct(token, width, height, font, color); }
    // 현재 글꼴 인코딩과 노드의 실제 문맥으로 스타일 문자열을 추가한다.
    style(token, text) { const node = this.#root.node(token); if (!(node instanceof TextGumpNode)) throw new Error('Client 글자 Gump 노드 누락'); this.#style.parse(styleTextField(node, 'head'), this.#fonts.encode(text), node.textContext); }
    // 기존 스타일 조각을 실제 소멸하고 노드 머리를 비운다.
    clear(token) { this.#body().clear(token); }
    // 현재 조각 치수와 여백으로 원본 기본 크기를 계산한다.
    fit(token, width = 0, height = 0) { this.#body().fit(token, width, height); }
}
