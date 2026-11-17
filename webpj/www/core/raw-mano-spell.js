// 맨 오 워 주문의 현재 자격·소환 순서를 옮긴다(cpppj o/RawManoSpell.cpp, 225553df, 10.78).
// 사용: new RawManoSpell(pool, server, counts, hooks).mayTarget(parent, target).
// 실제 Pool·서버·188개 현재 spawns와 호출 시 선택되는 World 장치를 빌리며 항상 false를 반환한다.
import { RawDeckDraw } from './raw-deck-draw.js';
import { windCirclePoint } from './raw-wind-circle.js';

// 원문의 세 주문 가상표 주소를 레벨 순서로 비교한다.
const TABLES = Object.freeze([0x5026a0, 0x502778, 0x502840]);
// 이식 대상의 현재 TYPE 행 개수다.
const TYPE_COUNT = 188;

// 선택한 위치에 실제 효과가 없으면 앞선 효과를 보존하고 오류를 낸다.
function need(hook) {
    if (typeof hook !== 'function') throw new Error('맨 오 워 주문의 선택된 실제 하위 미연결');
    return hook;
}

// 현재 시전자·운반체·생성 객체의 같은 물리 슬롯을 사용하는 주문 자격이다.
export class RawManoSpell {
    // Pool·서버 getter·현재 spawns와 실제 효과를 복사하지 않고 빌린다.
    #pool; #server; #counts; #hooks;

    // PATCH Pool과 정확한 188개 TYPE 및 현재 서버 getter를 검증한다.
    constructor(pool, server, counts, hooks = {}) {
        if (pool?.edition !== 'patch1078' || typeof server !== 'function' || counts?.length !== TYPE_COUNT) throw new TypeError('맨 오 워 주문 PATCH/현재188TYPE 계약 오류');
        this.#pool = pool; this.#server = server; this.#counts = counts; this.#hooks = hooks;
    }

    // 현재 슬롯에서 선택한 폭의 비정렬 little endian 값을 조회한다.
    #read(sid, offset, width = 4) {
        const raw = this.#pool.slot(sid); let result = 0;
        // 필요한 바이트만 순서대로 조합하고 주변 슬롯 상태는 검사하지 않는다.
        for (let byte = 0; byte < width; byte += 1) result += raw[offset + byte] * 2 ** (byte * 8);
        return result >>> 0;
    }

    // 선택된 TYPE의 현재 signed spawns를 매번 읽는다.
    #count(type) {
        if (type >= this.#counts.length) throw new RangeError('맨 오 워 주문 현재 TYPE spawns 범위 오류');
        return this.#counts[type] | 0;
    }

    // 서버·대상 관문 뒤 현재 테이블의 수만큼 실제 객체를 만들고 항상 false를 반환한다.
    mayTarget(parent, target) {
        if (!this.#server() || target !== this.#read(parent, 12, 2)) return false;
        const level = TABLES.indexOf(this.#read(parent, 0));
        if (level < 0) throw new TypeError('맨 오 워 주문 자격의 실제 가상 표 불일치');
        const hooks = this.#hooks;
        RawDeckDraw.wallSeed(need(hooks.wallSeconds)());
        const type = need(hooks.spellType)(level) >>> 0;
        // 각 소환과 원주 좌표 계산 시 선택된 TYPE의 현재 개수를 다시 읽는다.
        for (let index = 0; index < this.#count(type); index = (index + 1) | 0) {
            const created = need(hooks.create)(need(hooks.flyerType)() >>> 0, 0);
            const raw = this.#pool.physical(created);
            new DataView(raw.buffer, raw.byteOffset, raw.byteLength).setUint32(36, 0, true);
            need(hooks.owner)(created, this.#read(created, 0), this.#read(target, 34, 1));
            const shape = need(hooks.header)(created, this.#read(created, 40, 1) & 1, level !== 0);
            const center = need(hooks.center)(target);
            const x = Math.fround(center[0] + 1);
            const y = Math.fround(center[1] + Math.fround(((shape[0] << 16) >> 16) - ((shape[1] << 16) >> 16) + 1));
            const point = windCirclePoint(x, y, this.#count(type), index);
            need(hooks.pop)(created, this.#read(created, 0), point[0], point[1], 0);
        }
        return false;
    }

    // 같은 실제 Pool을 사용하는지 호출자가 확인한다.
    get pool() { return this.#pool; }
}
