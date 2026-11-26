// 초기 미션의 추가 안내를 기존8비트 Gump 글리프로 그린다. 배경을 만들지 않고 입력 선택 후보에서 자기 줄만 제외한다.
import { ClientTextGump } from './client-text-gump.js';
import { GumpText } from '../render/gump-text.js';

// CP1372의 지도 여백·최대 줄 폭·줄 간격이다. 화면 해상도가 바뀌어도 논리 픽셀 단위를 유지한다.
export const TUTORIAL_OBJECTIVE_LAYOUT = Object.freeze({ margin: 8, maximumWidth: 440, lineSpacing: 2 });

// 완전한 유니코드 문자를 유지하고 가능한 영문 공백에서 줄을 나눈다. measure는 현재 실제 글꼴의 그림자 없는 폭을 돌려준다.
export function wrapTutorialObjective(text, width, measure) {
    if (!(width > 0) || typeof measure !== 'function') throw new TypeError('튜토리얼 안내의 실제 글꼴 폭/측정 연결 필요');
    const characters = Array.from(text), lines = []; let begin = 0;
    // 매 줄은 적어도 한 글자를 소비하므로 폭보다 긴 단어도 종료하며 같은 글자를 중복 제출하지 않는다.
    while (begin < characters.length) {
        while (characters[begin] === ' ') begin += 1;
        if (begin === characters.length) break;
        let end = begin, lastSpace = -1;
        // 한 글자의 두 UTF-16 단위나 혼합 문자 경계는 자르지 않고 완성된 후보만 측정한다.
        while (end < characters.length && characters[end] !== '\n') {
            if (measure(characters.slice(begin, end + 1).join('')) > width && end > begin) break;
            if (characters[end] === ' ') lastSpace = end;
            end += 1;
        }
        const overflow = end < characters.length && characters[end] !== '\n';
        const cut = overflow && characters[end] !== ' ' && lastSpace > begin ? lastSpace : end;
        const line = characters.slice(begin, cut).join('').replace(/ +$/, '');
        if (line) lines.push(line);
        begin = cut; if (characters[begin] === ' ' || characters[begin] === '\n') begin += 1;
    }
    return lines;
}

// 같은 Client 루트·글꼴에 자기 줄만 소유한다. sync는 현재 지도/문구가 달라질 때만 노드를 교체한다.
export class TutorialObjectiveGump {
    // 루트보다 먼저 clear해야 하는 실제 노드 소유자와 글자/측정 장치다.
    #root; #fonts; #text; #measure;
    // 토큰 재사용을 구분하는 serial과 마지막 제출 문구·지도·색이다.
    #records = []; #submitted = ''; #view = null; #color = 0; #shadow = 0;
    // 입력을 처리하지 않는 기존 TextGump와 같은 FontSet을 사용한다. 자체 DOM/Canvas 층을 만들지 않는다.
    constructor(root, fonts) {
        this.#root = root; this.#fonts = fonts;
        this.#text = new ClientTextGump(root, fonts, () => 0); this.#measure = new GumpText(fonts);
    }
    // 부모가 먼저 지운 토큰이나 새 serial은 건드리지 않고 자기 노드만 지운다.
    clear() {
        for (const { token, serial } of this.#records) {
            this.#root.setDisplayOnly(token, serial, false);
            const node = this.#root.node(token);
            if (node && node.serial === serial && !(node.flags & 0x10000)) this.#root.remove(token);
        }
        this.#records = []; this.#submitted = ''; this.#view = null;
    }
    // font0 normal과1픽셀 그림자를 사용한다. 빈 문구는 숨김이며 지도 우측 하단8픽셀·최대440폭으로 배치한다.
    sync(text, viewport, color, shadow) {
        if (!text && !this.#submitted && this.#records.length === 0) return false;
        const alive = this.#records.every(({ token, serial }) => { const node = this.#root.node(token); return node && node.serial === serial && !(node.flags & 0x10000); });
        const sameView = this.#view && ['left', 'top', 'right', 'bottom'].every((key) => this.#view[key] === viewport[key]);
        if (text === this.#submitted && sameView && color === this.#color && shadow === this.#shadow && alive) return false;
        this.clear(); const { margin, maximumWidth, lineSpacing } = TUTORIAL_OBJECTIVE_LAYOUT;
        if (!text || viewport.right - viewport.left <= 2 * margin || viewport.bottom - viewport.top <= 2 * margin) return true;
        const context = { flags: 0, font: 0, color, style: 0, extra: 0, shadowColor: shadow, shadowX: 1, shadowY: 1 };
        const measure = (line) => { const bytes = this.#fonts.encode(line); return this.#measure.measure(bytes, context, { width: 0, height: 0 }, bytes.length); };
        const lines = wrapTutorialObjective(text, Math.min(maximumWidth, viewport.right - viewport.left - 2 * margin), (line) => measure(line).width);
        const lineHeight = measure(text).height + lineSpacing;
        if (lines.length * lineHeight > viewport.bottom - viewport.top - 2 * margin) throw new RangeError('튜토리얼 안내가 지도 표시 높이를 초과했습니다');
        let y = viewport.bottom - margin - lines.length * lineHeight;
        // 각 줄은 배경 없는 기본 글리프 조각뿐이며 기존 노드의 표시/입력 플래그를 변조하지 않는다.
        for (const line of lines) {
            const token = this.#text.allocate(); this.#text.construct(token, 0, 0, 0, color);
            const node = this.#root.node(token), serial = node.serial; this.#records.push({ token, serial });
            this.#root.setDisplayOnly(token, serial, true); node.textContext.set([2, 0, color, 0, 0, shadow, 1, 1]);
            this.#text.style(token, line); this.#text.fit(token, 2, 2);
            this.#root.attach(token, viewport.right - margin - measure(line).width - 2, y, this.#root.createState().entries[0], 1);
            y += lineHeight;
        }
        this.#submitted = text; this.#view = { ...viewport }; this.#color = color; this.#shadow = shadow; return true;
    }
    // 보고서에는 별도 계산 문자열 대신 마지막으로 실제 제출한 문구를 제공한다.
    text() { return this.#submitted; }
}
