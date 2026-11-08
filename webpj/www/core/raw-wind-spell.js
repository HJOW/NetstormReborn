// 세 윈드 소환 주문(데빌메이커 생산기)의 가상 bc 가 생산 프로세스를 만들고 0 을 반환하는 실제 동작(41ecf0/41fb60/41fcc0)을 옮긴 모듈이다(cpppj `o/RawWindSpell.cpp/.h`, 10.78 전용).
//  - mayTarget(spell): 주문 객체의 현재 가상 표(0x5025d8/0x502e68/0x502f30)로 세 변형 중 하나를 고르고(모르는 표는 거부), 서버가 아니면 아무것도 하지 않고 false 를 돌려준다.
//    서버이면 주문의 운반체 WORD(+0c)에 이미 생산 프로세스(현재 processType)가 있는지 조회하고(있으면 false), 없으면 변형의 소환 주문 TYPE 의 spawns 만큼(부착 뒤 바뀌는 현재 한도를 매번 읽음)
//    80바이트 생산기를 확보해(실패는 거부) 서버 생성자로 초기화하고 +44 에 원주 개수, +48 에 번호를 쓴 뒤 현재 운반체/processType/flags 8 로 부착한다. 항상 false 를 반환한다(대상 검색 중 부수 효과).
// state = { processType = 59, spells = [171, 172, 173], counts(188 TYPE 의 signed spawns, 부착 뒤에도 같은 저장소) }, env = { server, now }(살아 있는 서버 여부/게임 시각, 확보 뒤에도 다시 읽음),
// 훅 { exists(운반체, processType), allocate()→WindAviaryProcessFields | null, attach(fields, 운반체, processType, flags) } 는 도달한 지점에서만 요구한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E121: RawWindSpell 이식으로 추가)
import { RawWindAviaryState } from './raw-wind-aviary-state.js';

// 판본의 TYPE 수, 확보한 생산기의 원주 개수/번호 필드 위치(+44/+48)이다. (2026-11-08 WB04-E121)
const TYPE_COUNT = 188;
const COUNT_AT = 68;
const INDEX_AT = 72;

// 윈드 주문 논리이다. 사용: new RawWindSpell(pool, env, state, hooks). (2026-11-08 WB04-E121)
export class RawWindSpell {
    // 주문의 운반체 WORD(+0c)와 현재 가상 표를 시점마다 조회하는 같은 풀, 최초 생성 관문과 생성자가 읽는 서버/시계, 같은 spawns 레코드의 장기 상태, 원본이 도달한 큰 하위 훅이다.
    #pool;
    #env;
    #state;
    #hooks;

    // 같은 10.78 풀/현재 서버·시계/188 TYPE spawns 를 빌린다. (2026-11-08 WB04-E121)
    constructor(pool, env, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || !env || !state || !state.counts || state.counts.length !== TYPE_COUNT) throw new Error('Wind 주문 PATCH/188TYPE 필요');
        this.#pool = pool;
        this.#env = env;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 부모 WORD 와 현재 가상 표를 원본 폭으로 읽는다. (2026-11-08 WB04-E121)
    #read(sid, offset, width = 4) {
        const raw = this.#pool.slot(sid);
        let result = 0;
        // 낮은 바이트부터 지정한 폭만 조합한다.
        for (let index = 0; index < width; index += 1) result += raw[offset + index] * 2 ** (8 * index);
        return result;
    }

    // 원본이 선택한 실제 하위의 누락을 빈 성공으로 숨기지 않는다. (2026-11-08 WB04-E121)
    static #need(hook) {
        if (typeof hook !== 'function') throw new Error('Wind 주문 실제 하위 미연결');
        return hook;
    }

    // 41ecf0/41fb60/41fcc0: 부수 효과를 실행하며 항상 false 를 반환한다. 사용: mayTarget(spellSid). (2026-11-08 WB04-E121)
    mayTarget(spell) {
        const table = this.#read(spell, 0);
        let variant;
        if (table === 0x5025d8) variant = 0;
        else if (table === 0x502e68) variant = 1;
        else if (table === 0x502f30) variant = 2;
        else throw new Error('Wind 주문 현재 VT 불일치');
        const env = this.#env;
        const state = this.#state;
        const hooks = this.#hooks;
        if (!env.server) return false;
        if (RawWindSpell.#need(hooks.exists)(this.#read(spell, 12, 2), state.processType)) return false;
        const kind = state.spells[variant];
        if (kind >= state.counts.length) throw new RangeError('Wind 주문 현재 TYPE 범위');
        // 부착 뒤 변경된 같은 TYPE 의 현재 한도를 매번 읽는다.
        for (let index = 0; index < (state.counts[kind] | 0); index += 1) {
            const fields = RawWindSpell.#need(hooks.allocate)();
            if (!fields) throw new Error('Wind 주문 생산기 확보 실패');
            RawWindAviaryState.initialize(fields, env.server, env.now);
            const count = state.counts[kind] >>> 0;
            for (let at = 0; at < 4; at += 1) fields.bytes[COUNT_AT + at] = (count >>> (8 * at)) & 0xff;
            for (let at = 0; at < 4; at += 1) fields.bytes[INDEX_AT + at] = (index >>> (8 * at)) & 0xff;
            RawWindSpell.#need(hooks.attach)(fields, this.#read(spell, 12, 2), state.processType, 8);
        }
        return false;
    }
}
