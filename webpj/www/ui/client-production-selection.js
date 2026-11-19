// 생산 패널의 실제 항목 선택을 Session 커서·비용·재고에 연결한다.
import { RawContainedFinder } from '../core/raw-contained-finder.js';

// 같은 현재 Session과 생산 항목 장치를 사용한다.
export class ClientProductionSelection {
    // 원본 선택에 필요한 현재 Session과 외부 표시·소리 장치다.
    #session; #items; #devices;
    // technology(owner)는 현재 Player 기록의 기술 SID를 반환한다.
    constructor(session, items, devices) {
        if (!session?.core || typeof items?.affordable !== 'function' || !devices || ['technology', 'templateOwner', 'stock', 'failure', 'flash', 'sound', 'changed'].some((name) => typeof devices[name] !== 'function')) throw new TypeError('생산 선택의 실제 장치 누락');
        this.#session = session; this.#items = items; this.#devices = devices;
    }
    // 패널 항목 선택은 비활성·지식 잠금·비용·재고·실제 TYPE 순서로 검사한다.
    choose(content, serial) {
        const session = this.#session, core = session.core(), raw = core.pool.slot(content), flags = raw[20] | (raw[21] << 8);
        if ((flags & 0x100) || session.cursorDropSettings().lockDepth === 0) return 1;
        if (flags & 8) {
            if (!this.#items.affordable(content)) { this.#devices.flash(); this.#devices.failure(false, serial); return 1; }
            if (this.#devices.stock() && !raw[30]) { this.#devices.failure(true, serial); return 1; }
        }
        const type = raw[18] | (raw[19] << 8);
        if (!core.types[type]) throw new RangeError('생산 항목 TYPE 범위 오류');
        if (!core.types[type].flags2) return 1;
        session.cursor().take(content, flags & 8 ? 0x41 : 1); this.#devices.sound('getPiece.wav'); this.#devices.changed(); return 1;
    }
    // 원본 슬롯 단축키는 현재 기술 내용물을 앞에서부터 검색하고 같은 커서에 집는다.
    shortcut(slot) {
        const session = this.#session, core = session.core(), parent = this.#devices.technology(this.#devices.templateOwner());
        const finder = new RawContainedFinder(core.pool, session.productionFinder());
        // 기술의 실제 항목 순서와 슬롯 BYTE를 보존한다.
        for (let sid = finder.begin(parent); sid; sid = finder.next()) {
            const raw = core.pool.slot(sid);
            if (((raw[20] | (raw[21] << 8)) & 0x40) && raw[23] === (slot >>> 0)) { session.cursor().take(sid, 0); this.#devices.sound('getPiece.wav'); return 1; }
        }
        return 0;
    }
}
