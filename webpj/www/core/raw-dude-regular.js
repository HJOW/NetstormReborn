// Dude(일반 보행 유닛)의 정기 분배 450fa0·방황 451190·복귀 451210·투하 451270·낙하 451340·목표 이동 4508b0을 옮긴 모듈이다(cpppj `o/RawDudeRegular.cpp/.h`, 10.78 전용).
// 원본의 signed 비교·float 저장 지점·효과 뒤 현재 프레임 재조회 순서를 보존하며, 좌표 복사본을 보관하지 않고 효과 뒤마다 현재 raw 를 다시 읽는다. 큰 효과(공간·경로·task 재예약·거주 구역·진입점·삭제·진단)는 훅이다.
//  - initializeDudePopulation(population): 원본 static initializer 의 30칸/count 0 을 빈 목록에 준비한다(중복 초기화는 거부).
//  - handles(sid): 타입 70 이고 가상 표 0x508cc8 인 슬롯만 정기로 분배한다.
//  - handle(sid, event, count, payload): poll 사건이면 0, 슬롯이 free 이거나 사건이 0x51xx 가 아니면 진단 모드에서 진단 399 후 payload 를 돌려준다. 그 밖에는 사건 0x5105 가 아니면 지면 판정(checkSpace)이 참일 때 낙하로 가고,
//    현재 경로 종속이 있고 count 가 0 이 아니며 사건이 0x5107 이 아니면 payload 를 돌려준다. 이어 task WORD 를 쓰고 사건별(대기 0x5101=진단 0x179 후 방황·보행 0x5102=방황·복귀 0x5103·투하 0x5104·추락 0x5105·막힘/알림 0x5106/0x5107[count>10 이면 삭제·알림 0x5107 이고 경로가 없으면 빠른 방황])로 분배한다.
//  - wander(sid, count, quickly): count 가 0 이 아니면 게임 난수 100 미만 10 의 확률 관문을 통과해야 공급자 active 목록을 순환(providerIndex 를 난수 1(상한 제외, 간격 1)을 소비해 +1)해 목표로 이동을 시도하고, 실패하면 task 0x5103 을 걸고 0.1 을 돌려준다.
//  - return(sid, path, count): count 5 이상이면 삭제하고 -1, 아니면 가장 가까운 거주 구역으로 이동해 성공이면 10 아니면 2. drop(sid, path, count): count 가 0 이 아니면 진단 후 1, 0 이면 원래 위치를 보존하고 거주 구역 진입점으로 옮긴 뒤 원래 위치로 걸어 나오며 성공이면 1, 아니면 삭제하고 -1.
//  - fall(sid, count): dead 면 0, count 100 초과는 삭제하고 -1, count 10 에서 +23 을 20 으로 쓰고, 단계 8 이면 프레임을 'A' 구간 첫 칸으로 바꾼다. y 를 `(count×0.5+1)×0.0625+y`(float, 상한 254)로 올리고 현재 단계의 Run 안에서 난수로 프레임을 정해 쓴 뒤 Pop 하고 0.05 를 돌려준다.
//  - moveTo(sid, target, quickly): target 이 0 이면 false, 죽은 목표는 진단 0xde 뒤 계속, 진입점을 얻어 이동 훅을 부른다.
// state = { checking(진단 모드: 불리언 또는 함수), providers: SquidPostPopList, providerIndex(살아 있는 signed 순환 값), pollEvent = 0x1a2b, pathType = 24 }, frames 는 188칸 FrameCodeTable 배열이다.
// 훅 { checkSpace, findPath, task, nearestResidence, entrance, move, setFrame, unpop, destroy, pop, assertion } 는 도달한 지점에서만 요구한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E122: RawDudeRegular 이식으로 추가)
import { pathDirection } from './raw-path-animation.js';

// 정기 목록의 실제 초기 capacity, 판본의 타입 수, 정기 사건 분류 상수이다. (2026-11-08 WB04-E122)
export const DUDE_POPULATION_CAPACITY = 30;
const TYPE_COUNT = 188;
const DUDE_TYPE = 70;
const DUDE_TABLE = 0x508cc8;
const floatView = new DataView(new ArrayBuffer(4));
const fround = Math.fround;

// 원본 static initializer 의 30칸/count 0 을 빈 Session 목록에 준비한다. 배열의 미사용 heap 값은 게임 상태가 아니다. (2026-11-08 WB04-E122)
export function initializeDudePopulation(population) {
    if (population.entries.length !== 0 || population.count) throw new Error('Dude population 중복 초기화');
    population.entries = new Array(DUDE_POPULATION_CAPACITY).fill(0);
}

// 누락된 원본 효과는 선택한 시점에만 진단한다. (2026-11-08 WB04-E122)
function need(effect) {
    if (typeof effect !== 'function') throw new Error('Dude 정기 실제 효과 미연결');
    return effect;
}

// 비정렬 raw 의 지정 폭을 리틀엔디언으로 읽는다. (2026-11-08 WB04-E122)
function read(raw, offset, width = 4) {
    let value = 0;
    // 주변 필드를 합치거나 호스트 포인터로 해석하지 않는다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// 단정도 좌표를 읽는다. (2026-11-08 WB04-E122)
function float(raw, offset) {
    floatView.setUint32(0, read(raw, offset), true);
    return floatView.getFloat32(0, true);
}

// Dude 정기 처리기이다. 사용: new RawDudeRegular(pool, frames, random, state, hooks). (2026-11-08 WB04-E122)
export class RawDudeRegular {
    // 부모·목표·실제 종속의 유일한 저장소, 현재 타입의 코드/Run 표, 게임 전역의 같은 32비트 난수, 같은 목록/순환/진단 전역의 참조, 실제 선택한 하위 효과이다.
    #pool;
    #frames;
    #random;
    #state;
    #hooks;

    // 다른 판본/누락된 Run 배열을 첫 실행 전에 거부한다. (2026-11-08 WB04-E122)
    constructor(pool, frames, random, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || !frames || frames.length !== TYPE_COUNT || !state?.providers) throw new Error('Dude 정기 판본/프레임 연결 오류');
        state.providers.items();
        this.#pool = pool;
        this.#frames = frames;
        this.#random = random;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 현재 Session 의 같은 풀 연결을 검증한다. (2026-11-08 WB04-E122)
    get pool() { return this.#pool; }

    // 진단 모드(005e4794)가 비영인지 호출 시점에 읽는다. (2026-11-08 WB04-E122)
    #checking() {
        const checking = this.#state.checking;
        return typeof checking === 'function' ? checking() : checking;
    }

    // 가상 +5c = 450fa0 인 실제 부모만 이 분배에 들어온다. (2026-11-08 WB04-E122)
    handles(sid) {
        const raw = this.#pool.slot(sid);
        return raw[10] === DUDE_TYPE && read(raw, 0) === DUDE_TABLE;
    }

    // free 는 handle 의 원래 fallback 관문이 읽으므로 여기서 제거하지 않는다. 효과 뒤 현재 raw 를 다시 읽는다. (2026-11-08 WB04-E122)
    #raw(sid) {
        if (sid < 5 || sid >= this.#pool.capacity) throw new RangeError('Dude 정기 SID 범위');
        const raw = this.#pool.slot(sid);
        if (raw[10] >= this.#frames.length) throw new RangeError('Dude 정기 현재 타입 범위');
        return raw;
    }

    // 현재 비정렬 float 두 좌표이다. (2026-11-08 WB04-E122)
    #position(sid) {
        const raw = this.#raw(sid);
        return { x: float(raw, 14), y: float(raw, 18) };
    }

    // 4ad710 의 현재 signed side−A 이다(범위 진단은 기존 원본 프레임 조회를 재사용). (2026-11-08 WB04-E122)
    #stage(sid) {
        const raw = this.#raw(sid);
        return pathDirection(this.#frames[raw[10]], read(raw, 36) | 0);
    }

    // WORD task 와 BYTE 35 및 frame DWORD 의 이웃 바이트를 보존하며 원본 폭만 낮은 바이트부터 쓴다. (2026-11-08 WB04-E122)
    #write(sid, offset, value, width) {
        const raw = this.#pool.allocated(sid);
        // 원본 폭만 낮은 바이트부터 쓴다.
        for (let index = 0; index < width; index += 1) raw[offset + index] = ((value >>> 0) >>> (8 * index)) & 0xff;
    }

    // 4508b0: 진단 callback 이 복귀하면 원본처럼 목표 진입점과 이동 호출을 계속한다. 사용: moveTo(sid, target, quickly). (2026-11-08 WB04-E122)
    moveTo(sid, target, quickly) {
        if (!target) return false;
        if (this.#raw(target)[11] & 3) need(this.#hooks.assertion)(0xde);
        const move = need(this.#hooks.move);
        return move(sid, need(this.#hooks.entrance)(target), quickly);
    }

    // 451190: 난수 호출 순서를 바꾸지 않고 원본 공급자 active 목록의 현재 항목을 선택한다. 반환은 float 대기 시간이다. (2026-11-08 WB04-E122)
    wander(sid, count, quickly) {
        const state = this.#state;
        if (!count || this.#random.next(100) >= 10) {
            const providers = state.providers.items();
            if (providers.length) {
                // 원본 4558f0(1,2)는 상한 제외라 간격은 1 이며 난수는 한 번 소비한다.
                const next = ((state.providerIndex >>> 0) + this.#random.next(1) + 1) | 0;
                state.providerIndex = next % providers.length;
                if (state.providerIndex < 0) throw new RangeError('Dude 공급자 순환 음수 색인');
                const number = providers[state.providerIndex];
                if (number > 0xffff) throw new RangeError('Dude 공급자 SID 폭');
                if (this.moveTo(sid, number, quickly)) return 10;
            }
        }
        need(this.#hooks.task)(sid, 0x5103, 0);
        return fround(0.1);
    }

    // 451210: 5회부터 삭제하며 그 전에는 같은 거주 구역으로 이동한다(path 인자는 원래 전달되지만 읽지 않는다). (2026-11-08 WB04-E122)
    return(sid, path, count) {
        if (count > 4) {
            need(this.#hooks.destroy)(sid, 0);
            return -1;
        }
        return this.moveTo(sid, need(this.#hooks.nearestResidence)(sid), false) ? 10 : 2;
    }

    // 451270: 0회에 진입점으로 옮긴 뒤 원래 위치로 걸어 나온다. 원래 위치는 공간 효과 전에 보존한다. (2026-11-08 WB04-E122)
    drop(sid, path, count) {
        const hooks = this.#hooks;
        if (count) {
            if (this.#checking() && !path) need(hooks.assertion)(0x127);
            return 1;
        }
        const origin = this.#position(sid);
        const residence = need(hooks.nearestResidence)(sid);
        if (residence) {
            need(hooks.unpop)(sid, 0);
            const point = need(hooks.entrance)(residence);
            need(hooks.pop)(sid, point, 0);
            if (need(hooks.move)(sid, origin, false)) return 1;
        }
        need(hooks.destroy)(sid, 0);
        return -1;
    }

    // 451340: 원본의 fild→곱/합→float 저장과 unordered y 상한을 같은 순서로 처리한다. (2026-11-08 WB04-E122)
    fall(sid, count) {
        const hooks = this.#hooks;
        if (this.#raw(sid)[11] & 2) return 0;
        if (count > 100) {
            need(hooks.destroy)(sid, 0);
            return -1;
        }
        if (count === 10) this.#write(sid, 35, 20, 1);
        if (this.#stage(sid) === 8) {
            const type = this.#raw(sid)[10];
            need(hooks.setFrame)(sid, this.#frames[type].run('A').first, 0);
        }
        const point = this.#position(sid);
        let y = fround((count * 0.5 + 1.0) * 0.0625 + point.y);
        if (254.0 < y) y = 254.0;
        need(hooks.unpop)(sid, 0);
        const direction = this.#stage(sid);
        if (direction < 0 || direction > 15) throw new RangeError('Dude 낙하 단계 범위');
        const run = this.#frames[this.#raw(sid)[10]].run(0x41 + direction);
        if (run.count <= 0) throw new RangeError('Dude 낙하 현재 Run 0');
        const frame = (run.first + this.#random.next(run.count)) >>> 0;
        this.#write(sid, 36, frame, 4);
        need(hooks.pop)(sid, { x: point.x, y }, 0x40);
        return fround(0.05);
    }

    // 450fa0: poll 우선·free/마스크 fallback·낙하 우선·경로 대기·task 쓰기 순서이다. 반환은 float 대기 시간(또는 payload)이다. (2026-11-08 WB04-E122)
    handle(sid, event, count, payload) {
        const hooks = this.#hooks;
        if (!this.handles(sid)) throw new Error('Dude 정기 실제 타입/가상 표 오류');
        if (event === this.#state.pollEvent) return 0;
        if ((this.#raw(sid)[11] & 1) || (event & 0xfff0) !== 0x5100) {
            if (this.#checking()) need(hooks.assertion)(399);
            return payload;
        }
        const signedCount = count | 0;
        if (event !== 0x5105 && need(hooks.checkSpace)(sid)) return this.fall(sid, signedCount);
        const path = need(hooks.findPath)(sid, this.#state.pathType);
        if (path && count && event !== 0x5107) return payload;
        this.#write(sid, 12, event, 2);
        switch (event) {
        case 0x5101:
            need(hooks.assertion)(0x179);
            return this.wander(sid, signedCount, false);
        case 0x5102:
            return this.wander(sid, signedCount, false);
        case 0x5103:
            return this.return(sid, path, signedCount);
        case 0x5104:
            return this.drop(sid, path, signedCount);
        case 0x5105:
            return this.fall(sid, signedCount);
        case 0x5106:
        case 0x5107:
            if (signedCount > 10) {
                need(hooks.destroy)(sid, 0);
                return -1;
            }
            if (event === 0x5107 && !path) this.wander(sid, 0, true);
            return fround(0.2);
        default:
            need(hooks.assertion)(0x185);
            return payload;
        }
    }
}
