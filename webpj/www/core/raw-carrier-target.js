// 운반체(Carrier)의 대상 접근(4894e0)·귀환 접근(4276a0)·간헐천 이동(427600)·집으로 귀환(428fd0)을 같은 경로 시작기에 연결하는 모듈이다(cpppj `o/RawCarrierTarget.cpp/.h`, 10.78 전용).
// 순수 기하(접근점/변 중간점)와 현재 raw 조회는 내부에서 수행하고 발자국 조회·경로 시작·귀환 후보만 훅으로 둔다. 경로 시작 결과는 원본 EAX 의 Path 주소 숫자(0 이면 실패)이다.
//  - startTarget(parent, target, replaceStart, helper): 대상 발자국(footprint)을 구한 뒤 부모의 현재 위치(발자국 효과 뒤)에서 가장 가까운 접근점으로 경로를 시작하고, 실패하면 같은 사각형·인자를 유지한 채 북/동/남/서(0/2/4/6)
//    변의 중간점으로 최대 네 번 재시도한다(부모 위치를 다시 읽지 않는다).
//  - goReturn(parent, target): 부모의 소유자에게 AI 가 있으면(0 이 아닌 Player+b0 포인터) 0 이다. 아니면 발자국 효과 전에 부모 좌표를 보관하고 접근점 한 번만 시도한다.
//  - goGeyser(parent, target): 같은 AI 검사 뒤 대상이 free(bit0)이거나 TYPE flags2 에 간헐천 비트(0x10000000)가 없으면 0 이고, 아니면 goReturn 과 같이 접근점 한 번만 시도한다(dead/소유자/특정 TYPE 번호 필터를 추가하지 않는다).
//  - goHome(parent): 현재 boss 일 때만 귀환 후보(chooseReturn)를 구해 후보가 있으면 startTarget(parent, 후보, 0, 0)이다. 선택 효과가 권한을 바꿔도 다시 boss 를 검사하지 않는다.
// players 는 9명 Player 의 현재 바이트 표(0xb4바이트 간격, +0xb0 이 AI 포인터)이며 매 호출 직접 읽는다. boss 는 getter 또는 `{ value }`, hooks = { footprint(sid)→{ left, top, right, bottom }, start(request)→경로 주소(0=실패), chooseReturn(sid)→sid }이며
// 도달한 훅이 없으면 오류이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E100: 운반 대상 접근 이식으로 추가)

// 슬롯의 TYPE(+10)·상태(+11)·소유자(+34)·좌표(+14/+18) 위치, Player 표의 폭과 AI 포인터 위치, 간헐천 genus 비트이다. (2026-11-08 WB04-E100)
const TYPE_AT = 10;
const STATE_AT = 11;
const OWNER_AT = 34;
const X_AT = 14;
const Y_AT = 18;
const PLAYER_COUNT = 9;
const PLAYER_STRIDE = 0xb4;
const PLAYER_AI = 0xb0;
const TYPE_COUNT = 188;
const GEYSER = 0x10000000;

// 선택된 원본 하위가 없으면 빈 동작으로 대체하지 않는다. (2026-11-08 WB04-E100)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('운반 대상 이동의 실제 하위 미연결');
    return hook;
}

// 운반체 대상 접근 명령 처리기이다. (2026-11-08 WB04-E100)
export class RawCarrierTarget {
    // 실제 부모/대상 raw 의 동일 저장소이다. (2026-11-08 WB04-E100)
    #pool;
    // 현재 대상 TYPE 의 flags2 를 조회하는 188 TYPE 표이다. (2026-11-08 WB04-E100)
    #types;
    // 실제 9개 Player 의 현재 AI 포인터를 직접 읽는 바이트 표이다. (2026-11-08 WB04-E100)
    #players;
    // 귀환 wrapper 시작 때 현재 권한을 읽는 소유자(getter 또는 { value })이다. (2026-11-08 WB04-E100)
    #boss;
    // 실제 41d530/41d400 계산기(Graph 접근점과 구별)이다. (2026-11-08 WB04-E100)
    #board;
    // 발자국/경로 시작/귀환 선택의 큰 하위 연결이다. (2026-11-08 WB04-E100)
    #hooks;

    // 참조 수명과 PATCH 9 Player 폭을 검증한다. 아직 별도인 CD 이동을 PATCH 본문으로 실행하지 않는다. (2026-11-08 WB04-E100)
    constructor(pool, types, players, boss, board, hooks = {}) {
        if (pool?.edition !== 'patch1078' || !types || types.length !== TYPE_COUNT || !players || players.length !== PLAYER_COUNT * PLAYER_STRIDE) {
            throw new Error('운반 대상 이동의 판본/TYPE/Player9 오류');
        }
        this.#pool = pool;
        this.#types = types;
        this.#players = players;
        this.#boss = boss;
        this.#board = board;
        this.#hooks = hooks;
    }

    // 같은 Session 경로/공간의 혼합을 검사할 때 쓰는 풀이다. (2026-11-08 WB04-E100)
    get pool() { return this.#pool; }

    // 최초 또는 발자국 효과 뒤 원본 시점의 부모 두 float 이다. (2026-11-08 WB04-E100)
    #position(sid) {
        const raw = this.#pool.slot(sid);
        const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
        return { x: view.getFloat32(X_AT, true), y: view.getFloat32(Y_AT, true) };
    }

    // 40f100: owner 0 은 즉시 거짓, 그 외 현재 Player+b0 의 AI 포인터가 0 이 아니면 참이다. (2026-11-08 WB04-E100)
    #controlled(sid) {
        const owner = this.#pool.slot(sid)[OWNER_AT];
        if (!owner) return false;
        if (owner >= PLAYER_COUNT) throw new RangeError('운반 대상 이동의 현재 owner 범위 오류');
        const at = owner * PLAYER_STRIDE + PLAYER_AI;
        return (this.#players[at] | this.#players[at + 1] | this.#players[at + 2] | this.#players[at + 3]) !== 0;
    }

    // 4894e0: 첫 접근점 뒤 0/2/4/6 순서로 최대 네 번 재시도한다. 재시도는 최초 발자국/명령 인자를 유지하고 부모 위치를 다시 읽지 않는다. (2026-11-08 WB04-E100)
    startTarget(parent, target, replaceStart, helper) {
        const rectangle = need(this.#hooks.footprint)(target);
        const point = this.#board.nearest(rectangle, this.#position(parent), 1);
        let result = need(this.#hooks.start)({ parent, destination: point, replaceStart, helper, tailWord: 0, tailByte: 0, reserved: 0 });
        // 첫 시도 실패 뒤 북/동/남/서의 네 변만 순서대로 시도한다.
        for (let side = 0; side < 7 && result === 0; side += 2) {
            result = need(this.#hooks.start)({ parent, destination: this.#board.side(rectangle, side, 1), replaceStart, helper, tailWord: 0, tailByte: 0, reserved: 0 });
        }
        return result;
    }

    // 4276a0: AI 가 없을 때만, 발자국 효과 전에 부모 좌표를 보관하고 접근점 한 번만 시도한다. (2026-11-08 WB04-E100)
    goReturn(parent, target) {
        if (this.#controlled(parent)) return 0;
        const source = this.#position(parent);
        const rectangle = need(this.#hooks.footprint)(target);
        return need(this.#hooks.start)({ parent, destination: this.#board.nearest(rectangle, source, 1), replaceStart: 0, helper: 0, tailWord: 0, tailByte: 0, reserved: 0 });
    }

    // 427600: free 만 검사하고 dead 비트/owner/특정 타입 번호 필터를 임의로 추가하지 않는다. (2026-11-08 WB04-E100)
    goGeyser(parent, target) {
        if (this.#controlled(parent) || (this.#pool.slot(target)[STATE_AT] & 1)) return 0;
        const kind = this.#pool.slot(target)[TYPE_AT];
        if (kind >= this.#types.length) throw new RangeError('간헐천 이동 대상 TYPE 오류');
        if (!(this.#types[kind].flags2 & GEYSER)) return 0;
        const source = this.#position(parent);
        const rectangle = need(this.#hooks.footprint)(target);
        return need(this.#hooks.start)({ parent, destination: this.#board.nearest(rectangle, source, 1), replaceStart: 0, helper: 0, tailWord: 0, tailByte: 0, reserved: 0 });
    }

    // 428fd0: 현재 boss 와 귀환 후보를 조회한 뒤 startTarget(0, 0)이다. 선택 효과가 권한을 바꿔도 다시 검사하지 않는다. (2026-11-08 WB04-E100)
    goHome(parent) {
        if (typeof this.#boss === 'function' ? this.#boss() : this.#boss.value) {
            const target = need(this.#hooks.chooseReturn)(parent);
            if (target) return this.startTarget(parent, target, 0, 0);
        }
        return 0;
    }
}
