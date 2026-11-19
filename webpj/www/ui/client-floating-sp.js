// 원본 비용 표시를 실제 Client 글자·Gump 트리와 게임 시계에 연결한다.
import { FloatingSpGump, FloatingSpGumpNode } from './floating-sp-gump.js';

// 같은 Client 수명에서 생성한 비용 표시를 정리한다.
export class ClientFloatingSp {
    // 원본 작성기와 현재 살아 있는 파생 노드다.
    #root; #floating; #owned = new Set();
    constructor(root, text, now, changed) {
        if (!root || !text || typeof now !== 'function' || typeof changed !== 'function') throw new TypeError('비용 표시의 실제 Client·시계 누락');
        this.#root = root;
        this.#floating = new FloatingSpGump(root.createState(), {
            node: (token) => root.node(token), now,
            allocate: (size) => {
                if (size !== 0xb8) throw new RangeError('비용 표시 할당 크기 오류');
                const token = text.allocateDerived(new FloatingSpGumpNode(), { idle: (current) => this.#floating.idle(current),
                    destroy: (current, flags) => { this.#owned.delete(current); return this.#floating.deleteScalar(current, flags); } });
                this.#owned.add(token); return token;
            },
            constructText: (token) => text.construct(token, 0, 0, 0, 0), clearText: (token) => text.clear(token),
            fitText: (token) => text.fit(token), styledText: (token, value) => text.style(token, value),
            idle: (token, enabled) => root.lifetime().setIdle(token, enabled), redraw: (token, erase) => root.redraw(token, erase),
            kill: (token) => { root.lifetime().kill(token); changed(); }, attach: (...args) => root.attach(...args),
            reference: (token, object) => { root.bindObject(token, object); root.node(token).flags |= 0x2000; },
            deleteText: (token, flags) => text.deleteScalar(token, flags),
        });
    }
    // 같은 화면의 비용을 즉시 표시하고 등록된 Gump 토큰을 반환한다.
    showSp(cost, x, y, reference = 0) { return this.#floating.show(cost, x, y, reference); }
    // 미션 반납 시 아직 남은 비용 표시의 실제 수명을 닫는다.
    close() {
        // 원본 idle이 이미 제거한 토큰은 건너뛰고 살아 있는 표시만 반납한다.
        for (const token of [...this.#owned]) { if (this.#root.node(token)) this.#root.remove(token); else this.#owned.delete(token); }
    }
}
