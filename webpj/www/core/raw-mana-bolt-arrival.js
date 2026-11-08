// 마나 볼트(타입 105)의 현재 건설 대상 검색과 실제 도착 분기를 같은 월드에 적용하는 모듈이다(cpppj `o/RawManaBoltArrival.cpp/.h`, 10.78 0045aab0/0045ab30). 사제 도착이나 teleport(타입 134) 도착과 다른 몸체이다.
//  - target(bolt): 볼트 현재 좌표를 ftol 한 칸 하나에서 일반 finder(표면 단계 생략 플래그 1)를 돌려, abstract 비트(+40 bit0)가 있고 preview 비트(bit3)가 없고 소유자(+34)가 볼트와 같은 첫 현재 후보를 돌려준다. 없으면 0.
//    dead/void/HP 같은 추가 필터는 없고 후보 extra 와 두 소유자만 현재 값으로 비교한다.
//  - arrive(bolt): 볼트 좌표에서 `sunCannonRaiseLower5.wav` 를 한 번 재생하고(loop 0) 대상이 있으면 볼트의 두 프로세스(일반 Regular 46·공유 Regular 61 의 현재 번호)를 차례로 제거하고, 새 Regular 확보에 성공하면
//    event 1 로 부착(payload 0.05f)한 뒤 1000 을 반환한다. 대상이 없으면 부모를 플래그 0 으로 삭제하고 1001 을 반환한다(삭제 뒤에는 볼트를 읽지 않는다). 도달한 위치의 효과 경계가 없으면 생략하지 않고 거부한다.
// 10.72(CD)의 다른 raw 폭은 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E143: 마나 볼트 도착 이식으로 추가)
import { RawSquidFinder } from './raw-squid-finder.js';

// 도착 때 재생하는 소리, 일반 finder 의 표면 단계 생략 플래그, raw 좌표(+14/+18)·extra(+40)·소유자(+34) 위치, 타입 표 크기, 반환 코드이다. (2026-11-09 WB04-E143)
const SOUND = 'sunCannonRaiseLower5.wav';
const SKIP_SURFACE = 1;
const X_AT = 14;
const Y_AT = 18;
const EXTRA = 40;
const OWNER = 34;
const TYPE_COUNT = 188;
const ARRIVED_WITH_TARGET = 1000;
const ARRIVED_WITHOUT_TARGET = 1001;

// 원본 마나 볼트가 도착 때 제거/예약하는 현재 전역이다: regularType(00541128, 46)·sharedRegularType(00541164, 61)·delay(00532750, 0.05f)와 실제 Session 이 같은 장기 전역을 빌려주는 함수
// currentRegularType/currentSharedRegularType 이다. 사용: 월드마다 하나를 두고 RawManaBoltArrival 에 넘긴다. (2026-11-09 WB04-E143)
export class ManaBoltArrivalState {
    regularType = 46;
    sharedRegularType = 61;
    delay = Math.fround(0.05);
    currentRegularType = null;
    currentSharedRegularType = null;

    // 첫 제거 직전의 현재 전체 번호이다. (2026-11-09 WB04-E143)
    regularTypeNow() { return this.currentRegularType ? this.currentRegularType() >>> 0 : this.regularType; }
    // 첫 제거의 외부 효과 뒤 공유 번호를 다시 읽는다. (2026-11-09 WB04-E143)
    sharedRegularTypeNow() { return this.currentSharedRegularType ? this.currentSharedRegularType() >>> 0 : this.sharedRegularType; }
}

// 비정렬 float DWORD 를 리틀엔디언으로 읽는다. (2026-11-09 WB04-E143)
const view = new DataView(new ArrayBuffer(4));
function coordinate(raw, offset) {
    for (let index = 0; index < 4; index += 1) view.setUint8(index, raw[offset + index]);
    return view.getFloat32(0, true);
}

// 실제 41d770 의 _ftol 하위 DWORD 를 signed 로 재해석한다(비유한/64비트 범위 밖은 0). (2026-11-09 WB04-E143)
function cell(value) {
    if (!Number.isFinite(value) || Math.abs(value) >= 9223372036854775808) return 0;
    const whole = Math.trunc(value);
    if (Math.abs(whole) <= 2147483647) return whole | 0;
    return Number(BigInt.asIntN(32, BigInt(whole)));
}

// 도달한 실제 효과가 미연결이면 생략하지 않는다. (2026-11-09 WB04-E143)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('실제 마나 볼트 도착 경계 미연결');
    return hook;
}

// 마나 볼트 도착 몸체이다. pool(10.78 SidPool)·hash(SquidHash)·types(188칸)·state·hooks 는 이 모듈보다 오래 살아야 한다.
// hooks: sound(x, y, name)·removeProcesses(parent, type)·reserveRegular()→bool·addRegular(parent, event, delay)·destroy(parent, flags). 사용: new RawManaBoltArrival(pool, hash, types, state, hooks).arrive(boltSid). (2026-11-09 WB04-E143)
export class RawManaBoltArrival {
    #pool;
    #hash;
    #types;
    #state;
    #hooks;

    // 판본(10.78)과 타입 표 크기를 확인한다. 선택된 위치의 효과 경계는 도달할 때 확인한다. (2026-11-09 WB04-E143)
    constructor(pool, hash, types, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== TYPE_COUNT || !state) throw new TypeError('마나 볼트 도착 판본/TYPE 오류');
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 다른 월드의 경로와 잘못 연결하지 않게 하는 실제 풀 참조이다. (2026-11-09 WB04-E143)
    get pool() { return this.#pool; }

    // 같은 절삭 칸에서 abstract 이고 preview 가 아니며 동일 소유자인 첫 현재 후보를 돌려준다(실제 4202f0 flags1 일반 탐색 순서). (2026-11-09 WB04-E143)
    target(bolt) {
        const raw = this.#pool.slot(bolt);
        const x = cell(coordinate(raw, X_AT));
        const y = cell(coordinate(raw, Y_AT));
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        for (let sid = finder.begin({ left: x, top: y, right: x, bottom: y }, SKIP_SURFACE); sid !== 0; sid = finder.next()) {
            const candidate = this.#pool.slot(sid);
            if ((candidate[EXTRA] & 1) !== 0 && (candidate[EXTRA] & 8) === 0 && candidate[OWNER] === this.#pool.slot(bolt)[OWNER]) return sid;
        }
        return 0;
    }

    // 음향 → 대상 → 두 프로세스 제거/예약 또는 자기 삭제 뒤 1000/1001 이다. 부모 삭제가 경로 자신도 지울 수 있으므로 가상 삭제 뒤에는 현재 객체를 읽지 않는다. (2026-11-09 WB04-E143)
    arrive(bolt) {
        const hooks = this.#hooks;
        const state = this.#state;
        const raw = this.#pool.slot(bolt);
        need(hooks.sound)(coordinate(raw, X_AT), coordinate(raw, Y_AT), SOUND);
        if (this.target(bolt) !== 0) {
            // 도달한 각 제거 위치의 같은 현재 번호를 다시 읽는다.
            need(hooks.removeProcesses)(bolt, state.regularTypeNow());
            need(hooks.removeProcesses)(bolt, state.sharedRegularTypeNow());
            if (need(hooks.reserveRegular)()) need(hooks.addRegular)(bolt, 1, state.delay);
            return ARRIVED_WITH_TARGET;
        }
        need(hooks.destroy)(bolt, 0);
        return ARRIVED_WITHOUT_TARGET;
    }
}
