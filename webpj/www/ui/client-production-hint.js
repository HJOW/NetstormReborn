// 생산 실패 안내를 실제 TextGump·스타일·Gump 수명에 연결한다.
import { ProductionHintGump, ProductionHintNode, ProductionHintState } from './production-hint-gump.js';
import { RawUberRootLookup } from './raw-uber-root-lookup.js';

// 같은 Client 트리·글꼴·번역 및 현재 벽시계를 사용하는 안내 소유자다.
export class ClientProductionHint {
    // 원본 안내 상태와 실제 소유 문자열 토큰, 연결된 가상 처리기다.
    #root; #text; #devices; #state = new ProductionHintState(); #strings = new Map(); #next = 1; #hint; #lookup;
    // 번역과 현재 팔레트·시간·재출력은 호출 시점의 실제 장치다.
    constructor(root, text, devices) {
        if (!root || !text || !devices || ['now', 'translate', 'palette', 'changed'].some((name) => typeof devices[name] !== 'function')) throw new TypeError('생산 안내의 실제 장치 누락');
        this.#root = root; this.#text = text; this.#devices = devices; this.#lookup = new RawUberRootLookup(root);
        this.#hint = new ProductionHintGump(root.createState(), this.#state, {
            node: (token) => root.node(token), now: () => devices.now(),
            allocate: (size) => {
                if (size !== 0xb8) throw new RangeError('생산 안내 할당 크기 오류');
                return text.allocateDerived(new ProductionHintNode(), { idle: (token) => this.#hint.idle(token),
                    destroy: (token, flags) => this.#hint.deleteScalar(token, flags), draw: (token) => this.#hint.draw(token) });
            },
            constructText: (token) => text.construct(token, 0, 0, 6, 0),
            kill: (token) => { root.lifetime().kill(token); devices.changed(); },
            duplicate: (value) => { if (this.#next > 0xffffffff) throw new RangeError('생산 안내 문자열 토큰 소진'); const token = this.#next++; this.#strings.set(token, value); return token; },
            string: (token) => { if (!this.#strings.has(token)) throw new Error('생산 안내 문자열 누락'); return this.#strings.get(token); },
            free: (token) => { if (!this.#strings.delete(token)) throw new Error('생산 안내 문자열 중복 반납'); },
            translate: (value) => devices.translate(value), style: (token, value) => text.style(token, value), fit: (token, width, height) => text.fit(token, width, height),
            idle: (token, enabled) => root.lifetime().setIdle(token, enabled), bounds: (token) => root.collection().bounds(token), position: (token) => root.collection().position(token),
            draw: (head, position) => text.drawFragments(head, position), attach: (...args) => root.attachBase(...args), deleteText: (token, flags) => text.deleteScalar(token, flags),
        });
    }
    // 현재 항목 serial의 위치와 팔레트로 안내를 표시한다.
    show(empty, serial) {
        const reference = this.#lookup.reference(serial); if (!reference) throw new Error('생산 안내 기준 항목 누락');
        const palette = this.#devices.palette(); this.#state.white = palette.basic[7]; this.#state.black = palette.basic[0];
        const point = this.#root.collection().position(reference), token = this.#hint.show(empty, point.x, point.y, reference);
        this.#devices.changed(); return token;
    }
}
