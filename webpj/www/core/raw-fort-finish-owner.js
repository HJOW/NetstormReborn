// Fort 적재의 소유자 후처리(finishOwner)와 단일 객체 임의 배치(AddRandom)를 옮긴 모듈이다(cpppj `o/RawFortFinishOwner.cpp/.h`, 10.78 전용: 0040e1a0 Finish·0040ded0 PlaceRandom·004bcab0 시각 seed).
//  - finish(owner)(0040e1a0): 시작할 때 chaff/special 모드이면 자연 반환(소유자 범위 조회보다 앞섬). 아니면 시작 지역을 정하고(fort 이면 0, 아니면 Player 첫 지역; invalidRegion 이면 반환) **그 지역은 반복 중 다시 정하지 않는다.**
//    반복마다 현재 거주 구역 목록(count·SID·현재 TYPE·owner)을 끝까지 읽어 genus 0x40000000 이고 owner 가 같은 항목이 있으면 반환하고, 없으면 지역 사각형 → 호출별 벽시계로 만든 local seed → 현재 거주 구역 TYPE 로 placeRandom(수량 1) 을 반복한다
//    (1000회 실패 뒤에도 다음 지역 사각형/시각 seed 로 계속한다. 가짜 타이머/성공을 넣지 않는다).
//  - placeRandom(random, 사각형, 지역, 수량, 타입, force)(0040ded0): 타입 0/-1 은 진단 후 0 반환. 높이 다음 폭의 local PRNG 로 좌표를 고르고(원본 by-value 사각형 snapshot) 성공마다 수량을 줄이며 0 이 되면 멈춘다(수량 0 도 첫 시도 실행, 최대 1000회), 마지막 결과를 돌려준다.
//  - timeSeed(wallSeconds)(004bcab0): 시각×1013 의 CRT 절삭 하위 DWORD 의 SAR 4 + 33배(DWORD 감김)이며 전역 게임 난수를 바꾸지 않는다.
//  - bindPlayerRecords(players): 신전/섬 목록이 함께 갱신하는 같은 9 Player 기록(0xb4 간격)의 +0x78 첫 지역 DWORD 를 현재 원문에서 읽고 모듈 사본(`state.playerRegions`)은 쓰지 않는다.
//  - makeFortFinishOwnerHooks(pool, finish, hooks): 같은 풀의 finish 를 Fort 적재 훅의 필수 `finishOwner` 로 연결한다.
// 외부 하위(지역 사각형·벽시계·배치·진단)는 모두 실제 경계이며 누락하면 구성 때 거부한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E221: Fort 소유자 후처리 이식으로 추가)

import { GameRandom } from './random.js';

// 원본 거주 구역 genus 와 raw 타입/소유자 위치, 각 AddRandom 의 최대 시도 횟수이다. (2026-11-10 WB04-E221)
const RESIDENCE = 0x40000000;
const ATTEMPTS = 1000;
const TYPE_OFFSET = 10;
const OWNER_OFFSET = 34;
const TYPE_COUNT = 188;

// 시각 seed 의 원본 double 배율과 DWORD 계수, Player 기록 간격과 첫 지역 위치이다. (2026-11-10 WB04-E221)
const TIME_SCALE = 1013;
const SEED_FACTOR = 33;
const PLAYER_STRIDE = 0xb4;
const PLAYER_FIRST_REGION = 0x78;
const PLAYER_COUNT = 9;

// __ftol 의 64비트 절삭 하위 DWORD 이다(signed). 비유한/불확정 정수는 원본 하위 DWORD 0 으로 처리한다. (2026-11-10 WB04-E221)
function truncate(value) {
    if (!Number.isFinite(value) || Math.abs(value) >= 9223372036854775808) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// limit<1 은 local PRNG 를 소비하지 않는 원본 Random helper 의 분기이다. (2026-11-10 WB04-E221)
function nextRandom(random, limit) {
    return limit < 1 ? 0 : random.next(limit);
}

// Fort owner 후처리이다. pool 은 10.78 SidPool, types 는 188칸 TYPE 표(flags2), residences 는 AssetPostPopState.residences 자체(SquidPostPopList), state 는 createFortFinishOwnerState() 의 모드 상태,
// hooks = { regionBounds(region) → { left, top, right, bottom }, wallSeconds() → double, place(random, { type, x, y, region, mode, force }) → 비0 성공, invalidType() } 모두 필수이다.
// types/residences/state/pool 은 이 객체보다 오래 살아야 한다. 사용: new RawFortFinishOwner(pool, types, residences, state, hooks).finish(owner). (2026-11-10 WB04-E221)
export class RawFortFinishOwner {
    // 거주 구역 SID 를 읽는 풀, 현재 TYPE 표, 현재 거주 구역 목록, 최초 진입만 읽는 모드와 매 반복 읽는 현재 거주 구역 TYPE 전역, 외부 효과, 공유 Player 기록이다.
    #pool;
    #types;
    #residences;
    #state;
    #hooks;
    #players = null;

    // 같은 판본의 현재 타입과 모든 외부 효과를 요구한다. (2026-11-10 WB04-E221)
    constructor(pool, types, residences, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== TYPE_COUNT ||
            typeof hooks?.regionBounds !== 'function' || typeof hooks.wallSeconds !== 'function' || typeof hooks.place !== 'function' || typeof hooks.invalidType !== 'function') {
            throw new TypeError('Fort owner 후처리의 실제 자료/경계 누락');
        }
        this.#pool = pool;
        this.#types = types;
        this.#residences = residences;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 004bcab0: wallSeconds×1013 의 CRT 절삭 하위 DWORD 의 SAR 4 + 33배이다. 글로벌 게임 난수 seed 를 바꾸지 않는다. (2026-11-10 WB04-E221)
    static timeSeed(wallSeconds) {
        const value = truncate(wallSeconds * TIME_SCALE);
        return (((value >> 4) >>> 0) + (Math.imul(value, SEED_FACTOR) >>> 0)) >>> 0;
    }

    // 같은 9 Player BYTE 의 +0x78 을 공유하며 최초 지역을 현재 원문에서 읽는다. 실제 9기록 크기를 확인해 다른 Player 사본을 쓰지 않는다. (2026-11-10 WB04-E221)
    bindPlayerRecords(players) {
        if (!(players instanceof Uint8Array) || players.length !== PLAYER_COUNT * PLAYER_STRIDE) throw new TypeError('FortFinishOwner 실제 9 Player 크기 오류');
        this.#players = players;
    }

    // entry 최초의 지역 snapshot 만 같은 원문에서 읽는다. 후속 반복에서 지역을 다시 고르지 않는다. 미연결 독립 모듈은 기존 상태를 쓴다. (2026-11-10 WB04-E221)
    #playerRegion(owner) {
        if (!this.#players) {
            if (owner >= this.#state.playerRegions.length) throw new RangeError('Fort owner 기본 지역 범위 오류');
            return this.#state.playerRegions[owner];
        }
        const field = owner * PLAYER_STRIDE + PLAYER_FIRST_REGION;
        // 정확 네 BYTE 를 조합하여 stale 주변 필드를 보존한다.
        return (this.#players[field] | (this.#players[field + 1] << 8) | (this.#players[field + 2] << 16) | (this.#players[field + 3] << 24)) | 0;
    }

    // 0040e1a0: mode/invalid region 이면 자연 반환하고 없으면 실제 region → seed → placeRandom 을 반복한다. 처음 정한 region 은 반복 중 모드/Player 가 바뀌어도 다시 정하지 않고, 현재 residences/type 은 매 순회 읽는다. (2026-11-10 WB04-E221)
    finish(owner) {
        const state = this.#state;
        if (state.chaffMode || state.special) return;
        if (owner >= state.playerRegions.length) throw new RangeError('Fort owner 기본 지역 범위 오류');
        const region = state.fort ? 0 : this.#playerRegion(owner);
        if (region === state.invalidRegion) return;
        // 원본은 실패 1000회 뒤에도 다음 region/시각 seed 를 구해 계속한다.
        for (;;) {
            let found = false;
            // 전체 목록을 끝까지 읽은 뒤 일치 여부를 판단한다. free/dead/void 를 추가로 제외하지 않는다.
            for (let index = 0; index < (this.#residences.readCount() | 0); index += 1) {
                const entries = this.#residences.storage();
                if (index >= entries.length) throw new RangeError('Fort residence 현재 목록 저장소 부족');
                const number = entries[index];
                if (number >= this.#pool.capacity) throw new RangeError('Fort residence SID 범위 오류');
                const raw = this.#pool.slot(number);
                if (raw[TYPE_OFFSET] >= this.#types.length) throw new RangeError('Fort residence 현재 타입 오류');
                if ((this.#types[raw[TYPE_OFFSET]].flags2 & RESIDENCE) && raw[OWNER_OFFSET] === owner) found = true;
            }
            if (found) return;
            const rectangle = this.#hooks.regionBounds(region);
            const random = new GameRandom(RawFortFinishOwner.timeSeed(this.#hooks.wallSeconds()));
            this.placeRandom(random, rectangle, region, 1, state.residenceType, 0);
        }
    }

    // 0040ded0: 원본이 전달한 rectangle/value 인자는 snapshot 이다. placement 는 같은 local random 을 소비/변경할 수 있다. 각 성공에서만 quantity 를 감소시키고 현재 last result 를 반환한다(수량 0 도 첫 시도 실행). (2026-11-10 WB04-E221)
    placeRandom(random, rectangle, region, quantity, type, force) {
        if (!type || type === 0xffffffff) {
            this.#hooks.invalidType();
            return 0;
        }
        // 원본 by-value 인자를 보존하여 callback 의 외부 사각형 변경과 분리한다.
        const area = { left: rectangle.left, top: rectangle.top, right: rectangle.right, bottom: rectangle.bottom };
        const height = truncate(area.bottom - area.top);
        const width = truncate(area.right - area.left);
        let result = 0;
        let remaining = quantity >>> 0;
        for (let attempt = 0; attempt < ATTEMPTS; attempt += 1) {
            const dy = nextRandom(random, height);
            const dx = nextRandom(random, width);
            const x = Math.fround(dx + area.left);
            const y = Math.fround(dy + area.top);
            result = this.#hooks.place(random, { type, x, y, region, mode: 0, force }) >>> 0;
            if (result) remaining = (remaining - 1) >>> 0;
            if (!remaining) break;
        }
        return result;
    }

    // loader 훅과 같은 풀인지 검사하는 참조이다. (2026-11-10 WB04-E221)
    get pool() { return this.#pool; }
}

// 같은 월드의 finish 를 Fort 적재 훅의 필수 owner 후처리에 연결한다. 누락된 finishOwner 를 자연 반환/반복의 실제 몸체로만 교체하며 다른 풀이면 거부한다. (2026-11-10 WB04-E221)
export function makeFortFinishOwnerHooks(pool, finish, hooks = {}) {
    if (pool !== finish?.pool) throw new TypeError('Fort loader/owner 후처리 SID 풀이 다릅니다');
    return { ...hooks, finishOwner: (owner) => finish.finish(owner) };
}
