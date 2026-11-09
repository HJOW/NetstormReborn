// Factory 의 실제 event 3 예측 그림·완료와 종속 그림 정리를 같은 월드에서 처리하는 모듈이다(cpppj `o/RawFactoryUpgrade.cpp/.h`, 10.78 전용).
//  - nextLevel(00453750): 가상 level 을 읽어 +1 이 3 을 넘으면(signed) 3, 아니면 두 번째 가상 조회로 +1 을 돌려준다(하위가 값을 바꿔도 원본 재조회를 유지).
//  - begin(004549c0 count 0): 다음 레벨의 abstract client 그림 생성 → 레벨 → extra 8 → (void 가 아니면 진단 0x282) → extra 1 → 상태 WORD 0 → 소유자 → 공간 등록(y 에 0.001 오프셋) → 로그 → 건설 진행 → 감시 쌍이다.
//  - complete(004545e0): 레벨/최대 HP(DWORD 쓰기)/프레임 보정/표시/위치 소리/로컬 안내 소리/플레어 → 모든 event 4 와 3 예약 제거 → 감시 Form 의 예측 그림 삭제. 아웃포스트는 모든 완료 효과와 예약/그림 삭제까지 생략한다.
//  - finish(004549c0 count 양수): 로그 → 완료 → 현재 AI 통지 → -1. hasPicture(004537f0)·isUpgrading(00453c90: 그림 → event 4 → event 3 의 단락 조회).
// 감시 Form 순회는 후보를 돌려주기 전에 next 를 저장하므로 예측 그림 삭제가 순회를 깨지 않으며, 순환/풀 밖 체인은 오류이다. 별도 복사 그림/예약 목록 없이 같은 raw 풀과 Host 를 쓴다.
// 선택한 하위만 호출 지점에서 요구하고 앞선 원문 쓰기는 유지한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E218: Factory 업그레이드 진행/완료 이식으로 추가)

import { setRawSquidHitPoints } from './raw-squid-word.js';

// PE 00509144 의 실제 y 오프셋(단정도)과 client 예측 SID 범위, 첫 자산 TYPE 이다. (2026-11-10 WB04-E218)
const PICTURE_OFFSET = Math.fround(0.0010000000474974513);
const FIRST_ASSET_TYPE = 70;

// 필요한 하위만 호출 지점에서 요구하고 앞선 원문 쓰기는 유지한다. (2026-11-10 WB04-E218)
function need(effect) {
    if (typeof effect !== 'function') throw new Error('Factory 업그레이드 실제 하위 미연결');
    return effect;
}

// 정렬되지 않은 원문을 낮은 BYTE 부터 읽는다(물리 범위 검사). (2026-11-10 WB04-E218)
function read(raw, offset, width) {
    if (offset > raw.length || raw.length - offset < width) throw new RangeError('Factory 업그레이드 원문 범위');
    let value = 0;
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// 원문 DWORD 를 단정도 float 로 해석하는 도구이다. (2026-11-10 WB04-E218)
const floatView = new DataView(new ArrayBuffer(4));
function floatOf(bits) {
    floatView.setUint32(0, bits >>> 0, true);
    return floatView.getFloat32(0, true);
}

// Factory 업그레이드이다. pool 은 10.78 SidPool, host 는 같은 풀의 SquidProcessHost, state = { outpost(), dependType(), localPlayer(), checking(), challenge(), boss() } 현재 전역 getter(각 호출 지점에서 다시 읽음),
// hooks = { level(sid), setLevel(sid, level), maxHp(sid), adjustFrame(sid), display(sid), soundAt(x, y, name), sound(name), flare(sid, count, event), create(type, flags), owner(sid, owner), pop(sid, x, y, flags),
//   construction(sid, duration), depend(watched, watcher), watcher(form), destroy(sid, flags), aiPresent(sid), notifyAi(owner, sid), log(kind, text), assertion(line) } 이다.
// 사용: new RawFactoryUpgrade(pool, host, state, hooks).begin(parent, 15). (2026-11-10 WB04-E218)
export class RawFactoryUpgrade {
    // 실제 Factory/예측 그림/종속 Form 의 같은 저장소, 실제 SharedRegular/Regular 검색·삭제 Host, 원본 전역, 선택된 실제 가상·장치·AI 경계이다.
    #pool;
    #host;
    #state;
    #hooks;

    // 같은 PATCH 풀/Host 계약만 확인하며 실행 효과는 없다. (2026-11-10 WB04-E218)
    constructor(pool, host, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || pool !== host?.pool) throw new TypeError('Factory 업그레이드 같은 PATCH 풀/Host 누락');
        this.#pool = pool;
        this.#host = host;
        this.#state = state;
        this.#hooks = hooks ?? {};
    }

    // 00453750: 상한 3 과 상한 미만의 두 번째 가상 조회이다. 가상 getter 가 하위 효과로 바뀌어도 원본 재조회를 유지한다. (2026-11-10 WB04-E218)
    nextLevel(parent) {
        const next = (need(this.#hooks.level)(parent) + 1) >>> 0;
        if ((next | 0) > 3) return 3;
        return (need(this.#hooks.level)(parent) + 1) >>> 0;
    }

    // 004549c0 count 0: 다음 레벨 abstract client 그림 → 건설 → 감시 쌍이다. 추상 상태는 void 일 때만 설정하며 상태 WORD 와 프레임 DWORD 를 혼동하지 않는다. (2026-11-10 WB04-E218)
    begin(parent, payload) {
        const hooks = this.#hooks;
        const pool = this.#pool;
        const image = need(hooks.create)(pool.slot(parent)[10], 2);
        const level = this.nextLevel(parent);
        need(hooks.setLevel)(image, level);
        pool.allocated(image)[40] |= 8;
        if (!(pool.slot(image)[11] & 4)) need(hooks.assertion)(0x282);
        pool.allocated(image)[40] |= 1;
        const raw = pool.allocated(image);
        raw[12] = 0;
        raw[13] = 0;
        need(hooks.owner)(image, pool.slot(parent)[34]);
        const current = pool.slot(parent);
        const x = floatOf(read(current, 14, 4));
        const y = Math.fround(floatOf(read(current, 18, 4)) + PICTURE_OFFSET);
        need(hooks.pop)(image, x, y, 0);
        need(hooks.log)(4, 'making the client upgrade picture');
        need(hooks.construction)(image, payload);
        need(hooks.depend)(parent, image);
    }

    // 004b22d0: finder 의 검사 조건도 호출 시점의 원본 전역을 읽는다. dead 부모·비 challenge·비 boss 에서 checking 이면 진단 0x158 뒤 현재 head(+6 WORD)를 읽는다. (2026-11-10 WB04-E218)
    #first(parent) {
        const state = this.#state;
        if (state.checking() && !state.challenge() && (this.#pool.slot(parent)[11] & 2) && !state.boss()) need(this.#hooks.assertion)(0x158);
        return read(this.#pool.slot(parent), 6, 2);
    }

    // 004b1b80/004b1bd0: 후보 반환 전에 next 를 저장하므로 현재 그림 삭제가 순회를 깨지 않는다. 원본처럼 asset 과 다른 Form 만 건너뛰며 free/dead 를 추가 제외하지 않는다. cursor = { next, steps }. (2026-11-10 WB04-E218)
    #next(cursor) {
        const pool = this.#pool;
        while (cursor.next) {
            cursor.steps += 1;
            if (cursor.next >= pool.capacity || cursor.steps > pool.capacity) throw new Error('Factory 업그레이드 종속 체인 손상');
            const current = cursor.next;
            const raw = pool.slot(current);
            cursor.next = read(raw, 4, 2);
            if (raw[10] < FIRST_ASSET_TYPE && raw[10] === this.#state.dependType()) return current;
        }
        return 0;
    }

    // client 번호 5..14999 만 예측 그림이며 소유자/free/dead 조건은 추가하지 않는다. 타입/extra 8/번호 범위만 원본 순서로 검사한다. (2026-11-10 WB04-E218)
    #picture(parent, candidate) {
        const raw = this.#pool.slot(candidate);
        return raw[10] === this.#pool.slot(parent)[10] && (raw[40] & 8) !== 0 && candidate > 4 && candidate < 15000;
    }

    // 004545e0: 아웃포스트는 모든 완료 효과와 예약/그림 삭제까지 생략한다. 완료 효과가 추가한 동일 사건까지 원본의 반복 검색 순서 4→3 으로 제거한다. (2026-11-10 WB04-E218)
    complete(parent) {
        const hooks = this.#hooks;
        const pool = this.#pool;
        if (pool.slot(parent)[10] === this.#state.outpost()) return;
        const level = this.nextLevel(parent);
        need(hooks.setLevel)(parent, level);
        const hp = need(hooks.maxHp)(parent);
        setRawSquidHitPoints(pool, parent, hp | 0);
        need(hooks.adjustFrame)(parent);
        need(hooks.display)(parent);
        const raw = pool.slot(parent);
        need(hooks.soundAt)(floatOf(read(raw, 14, 4)), floatOf(read(raw, 18, 4)), 'upgradeDone.wav');
        if (pool.slot(parent)[34] === this.#state.localPlayer()) need(hooks.sound)('upgradeComplete.wav');
        need(hooks.flare)(parent, 6, 2);
        // 현재 실행 중인 event 3 도 실제 Host 의 수명/등록 검사로 안전하게 제거된다.
        for (let process = this.#host.findEvent(parent, 4); process; process = this.#host.findEvent(parent, 4)) this.#host.kill(process, 0);
        for (let process = this.#host.findEvent(parent, 3); process; process = this.#host.findEvent(parent, 3)) this.#host.kill(process, 0);
        const cursor = { next: this.#first(parent), steps: 0 };
        // 감시 Form 의 다음 링크는 그림/양쪽 Depend 삭제 전에 저장한다.
        for (let form = this.#next(cursor); form; form = this.#next(cursor)) {
            const watcher = need(hooks.watcher)(form);
            if (watcher === parent) need(hooks.assertion)(0xdd);
            if (this.#picture(parent, watcher)) need(hooks.destroy)(watcher, 0);
        }
    }

    // 004549c0 count 양수: 로그 → 완료 → 현재 AI 통지 → -1 이다. 완료 효과 뒤 현재 소유자/AI 를 다시 조회하며 아웃포스트에도 원본 통지 접두를 적용한다. (2026-11-10 WB04-E218)
    finish(parent) {
        const hooks = this.#hooks;
        need(hooks.log)(3, 'Upgrading!\n');
        this.complete(parent);
        if (need(hooks.aiPresent)(parent)) need(hooks.notifyAi)(this.#pool.slot(parent)[34], parent);
        return -1;
    }

    // 004537f0: 같은 TYPE/extra 8/client 범위의 첫 감시자를 찾는다. 원본 첫 일치에서 멈추고 후속 감시자는 읽지 않는다. (2026-11-10 WB04-E218)
    hasPicture(parent) {
        const hooks = this.#hooks;
        const cursor = { next: this.#first(parent), steps: 0 };
        // 저장 next 규칙으로 실제 감시 Form 을 방문한다.
        for (let form = this.#next(cursor); form; form = this.#next(cursor)) {
            const watcher = need(hooks.watcher)(form);
            if (watcher === parent) need(hooks.assertion)(0xad);
            if (this.#picture(parent, watcher)) return true;
        }
        return false;
    }

    // 00453c90: 그림 → event 4 → event 3 의 단락 조회이다. 예측 그림이 있으면 예약 목록을 조회하지 않는다. (2026-11-10 WB04-E218)
    isUpgrading(parent) {
        return this.hasPicture(parent) || this.#host.findEvent(parent, 4) != null || this.#host.findEvent(parent, 3) != null;
    }

    // 실제 월드 연결 진단에 쓰며 복사 저장소를 만들지 않는다. (2026-11-10 WB04-E218)
    get pool() { return this.#pool; }
}
