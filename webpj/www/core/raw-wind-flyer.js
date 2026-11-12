// 더스트 데빌(윈드 플라이어)의 전용 초기화·애니메이션·주기 피해를 원본 10.78 순서로 실행하는 모듈이다(cpppj `o/RawWindFlyer.cpp/.h`, 10.78 전용: 45e1c0 전용 표 쓰기·45e230 초기화·45c020 애니메이션·45e8b0 주기 처리·45e9a0 한 칸 범위 피해).
//  - new RawWindFlyer(pool, hash, types, soundFrames, activeSounds, globals, hooks): pool 은 같은 PATCH SidPool, hash 는 SquidHash, types = 188칸 TYPE({ footX, footY, flags2 })의 **살아 있는 참조**(후보의 현재 genus 를 읽음),
//    soundFrames = 길이 188 타입별 마지막 소리 재생 프레임 장부(호출자와 공유, 이 모듈이 씀), activeSounds = 길이 188 타입별 activeSound 토큰, globals = { now(), boss(), frame() } 는 모두 현재 값을 읽는 함수이고,
//    hooks = { runCount(sid)→int32, assertion(line), damageable(sid, event, count, period)→float, destroy(sid, flags), sound(x, y, token), mayTarget(source, target)→bool, damageRate(source, target)→float, damage(target, amount, owner, source), weaken(target) } 이다.
//    선택하지 않은 경계는 요구하지 않으며 선택된 누락은 앞선 효과를 유지한 채 거부한다.
//  - RawWindFlyer.specialize(fields): 45e1c0 — 기본 생성자의 부착 뒤 전용 표·inner VT·타이머 96·단계 84 만 쓴다(E352 `specializeWind` 와 같은 쓰기). initialize(fields): 45e230 — 애니메이션 타이머 96 = now·공격/착륙 0·초기화 표식 1·최초 20초 타이머(72) = now + 20.
//  - animation(fields): 45c020 — 부모 정지(+40 의 0x20)이거나 애니메이션 기한(96)이 아직이면 현재 프레임, 아니면 다음 기한 = now + 0.07999999821186066 을 쓰고 (프레임 + 1) % 글자 수(부호 있는 나머지, 0 나눗셈·INT_MIN/−1 거부). 부모를 읽을 때마다 종속 head 가 비어 있으면 진단 0xa4.
//  - handle(sid, event, count, period): 45e8b0 — event 가 0 이 아니면 공통 피해 훅, 아니면 count < 80 이면 period 가 0.25 인지 확인(0x19e)하고 주기 피해, 80 이상이면 boss 일 때 삭제(−1 반환). count % 40 == 0 이면 타입별 한 프레임에 한 번만 소리. 정상 반환 0.25.
//  - attack(sid, period): 45e9a0 — 위치를 올림 근사(+0.9999899864196777f)→정수 절삭→float 저장→절삭으로 칸 하나의 사각형을 만들어 일반 finder 로 후보를 모두 모은 뒤(최대 10개: 발사 가능이고 flags2 0x120000 이 없는 후보 또는 flags2 비트 4 후보) 각 후보의 현재 free 비트와 공격자 owner 를 다시 읽으며 약화(flags2 비트 4)나 피해(= 정수 절삭한 DPS × period)를 낸다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E358: 7-P1 RawWindFlyer 이식으로 추가)
import { RawSquidFinder } from './raw-squid-finder.js';
import { specializeWind } from './raw-flyer-process.js';

// 원본 509c60 의 double 상수(0.08초 그림 갱신 간격)이다. (2026-11-13 WB04-E358)
const ANIMATION_DELAY = 0.07999999821186066;
// 원본 41d7a0 의 거의 올림 보정(float 0.9999899864196777)이다. (2026-11-13 WB04-E358)
const NEAR_CEIL = Math.fround(0.9999899864196777);
// TYPE 표 크기(PATCH)이다. (2026-11-13 WB04-E358)
const TYPE_COUNT = 188;
// 원본 Buffer(10)의 최대 피해 후보 수이다. (2026-11-13 WB04-E358)
const MAX_VICTIMS = 10;

// 비정렬 원문 DWORD 를 계산 없이 조합한다. (2026-11-13 WB04-E358)
function read(raw, at) {
    let value = 0;
    // 원문 네 바이트만 읽는다.
    for (let index = 0; index < 4; index += 1) value += raw[at + index] * 2 ** (8 * index);
    return value;
}

// 선택한 폭만 쓰고 주변 미정 바이트를 보존한다. (2026-11-13 WB04-E358)
function put(raw, at, value, width = 4) {
    // 지정한 폭의 바이트만 쓴다.
    for (let index = 0; index < width; index += 1) raw[at + index] = Math.floor(value / 2 ** (8 * index)) & 0xff;
}

// float/double 비트 변환용 공용 보기이다. (2026-11-13 WB04-E358)
const SCRATCH = new DataView(new ArrayBuffer(8));

// 원문 DWORD 비트를 float 로 읽는다. (2026-11-13 WB04-E358)
function floatOf(raw, at) {
    SCRATCH.setUint32(0, read(raw, at), true);
    return SCRATCH.getFloat32(0, true);
}

// 필드의 raw double 을 비트 그대로 읽는다. (2026-11-13 WB04-E358)
function time(fields, at) {
    SCRATCH.setUint32(0, read(fields.bytes, at), true);
    SCRATCH.setUint32(4, read(fields.bytes, at + 4), true);
    return SCRATCH.getFloat64(0, true);
}

// double 의 두 DWORD 만 쓴다(별도 dirty 를 더하지 않음). (2026-11-13 WB04-E358)
function putTime(fields, at, value) {
    SCRATCH.setFloat64(0, value, true);
    put(fields.bytes, at, SCRATCH.getUint32(0, true));
    put(fields.bytes, at + 4, SCRATCH.getUint32(4, true));
}

// 원본 CRT 의 정상 signed64 절삭 뒤 하위 DWORD(int32)이다. 범위 밖·비유한은 0 이다. (2026-11-13 WB04-E358)
function truncate(value) {
    if (!(value >= -9223372036854775808 && value < 9223372036854775808)) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 원본 41d7a0 의 거의 올림 → float 저장 → 절삭이다. (2026-11-13 WB04-E358)
function cell(value) {
    return truncate(Math.fround(truncate(value + NEAR_CEIL)));
}

// 선택된 하위 누락은 앞선 효과를 유지하며 명시적으로 거부한다. (2026-11-13 WB04-E358)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('더스트 데빌 실제 하위 미연결');
    return hook;
}

// 더스트 데빌 로직이다. (2026-11-13 WB04-E358)
export class RawWindFlyer {
    #pool;
    #hash;
    #types;
    #soundFrames;
    #activeSounds;
    #globals;
    #hooks;

    // PATCH 와 188칸 TYPE/소리 장부 표·현재 전역 조회기를 확인한다. TYPE 표는 살아 있는 참조이다. (2026-11-13 WB04-E358)
    constructor(pool, hash, types, soundFrames, activeSounds, globals, hooks) {
        if (pool?.edition !== 'patch1078' || types?.length !== TYPE_COUNT || soundFrames?.length !== TYPE_COUNT || activeSounds?.length !== TYPE_COUNT) throw new TypeError('더스트 데빌 PATCH188 자료 불일치');
        // 모든 전역은 처리 도중 바뀔 수 있어 값이 아니라 현재 값 조회 함수여야 한다.
        for (const name of ['now', 'boss', 'frame']) {
            if (typeof globals?.[name] !== 'function') throw new TypeError(`더스트 데빌 현재 전역 조회기 누락: ${name}`);
        }
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#soundFrames = soundFrames;
        this.#activeSounds = activeSounds;
        this.#globals = globals;
        this.#hooks = hooks ?? {};
    }

    // Host 가 같은 풀인지 확인할 수 있게 공개한다. (2026-11-13 WB04-E358)
    get pool() { return this.#pool; }

    // 45e1c0: 기본 생성자의 부착 뒤 실제 전용 표만 쓴다(E352 의 같은 쓰기를 재사용). (2026-11-13 WB04-E358)
    static specialize(fields) { specializeWind(fields); }

    // 원본 45e230 의 저장 순서이다. (2026-11-13 WB04-E358)
    initialize(fields) {
        const globals = this.#globals;
        putTime(fields, 96, globals.now());
        put(fields.bytes, 80, 0);
        put(fields.bytes, 88, 0);
        put(fields.bytes, 84, 1);
        putTime(fields, 72, globals.now() + 20);
    }

    // 실제 41bfb0 은 각 부모 조회마다 종속 head 가 0 인지 진단하며 반환한 뒤에도 다시 호출된다. (2026-11-13 WB04-E358)
    #parentRaw(fields) {
        const raw = this.#pool.slot(fields.parent);
        if (!raw[6] && !raw[7]) need(this.#hooks.assertion)(0xa4);
        return raw;
    }

    // 45c020: 0.08초 주기와 signed 나머지이다. (2026-11-13 WB04-E358)
    animation(fields) {
        const globals = this.#globals;
        if ((this.#parentRaw(fields)[40] & 0x20) || !(time(fields, 96) <= globals.now())) return read(this.#parentRaw(fields), 36) | 0;
        putTime(fields, 96, globals.now() + ANIMATION_DELAY);
        const current = read(this.#parentRaw(fields), 36);
        const count = need(this.#hooks.runCount)(fields.parent);
        const next = (current + 1) | 0;
        if (!count || (next === -2147483648 && count === -1)) throw new RangeError('더스트 데빌 원본 idiv 범위 오류');
        return (next % count) | 0;
    }

    // 45e8b0: 80회 공격/40회 소리/삭제이다. event 가 0 이 아니면 공통 피해 훅으로 넘긴다. (2026-11-13 WB04-E358)
    handle(sid, event, count, period) {
        const pool = this.#pool;
        const globals = this.#globals;
        if (event) return need(this.#hooks.damageable)(sid, event, count, period);
        const signedCount = count | 0;
        if (signedCount < 80) {
            if (period !== 0.25) need(this.#hooks.assertion)(0x19e);
            this.attack(sid, 0.25);
        } else if (globals.boss()) {
            need(this.#hooks.destroy)(sid, 0);
            return -1;
        }
        if (signedCount % 40 === 0) {
            const type = pool.slot(sid)[10];
            if ((this.#soundFrames[type] >>> 0) !== (globals.frame() >>> 0)) {
                this.#soundFrames[pool.slot(sid)[10]] = globals.frame() >>> 0;
                const raw = pool.slot(sid);
                const x = floatOf(raw, 14);
                const y = floatOf(pool.slot(sid), 18);
                need(this.#hooks.sound)(x, y, this.#activeSounds[pool.slot(sid)[10]]);
            }
        }
        return 0.25;
    }

    // 45e9a0: 한 칸 탐색·10항목 저장·범위 피해이다. (2026-11-13 WB04-E358)
    attack(sid, period) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        const types = this.#types;
        const x = floatOf(pool.slot(sid), 14);
        const y = floatOf(pool.slot(sid), 18);
        const cx = cell(x);
        const cy = cell(y);
        // 후보/대상의 현재 TYPE 기록이다(표 밖 번호는 거부).
        const typeOf = (target) => {
            const type = types[pool.slot(target)[10]];
            if (!type) throw new RangeError('더스트 데빌 TYPE 번호가 표 밖입니다');
            return type;
        };
        const finder = new RawSquidFinder(pool, this.#hash, types);
        // 원본 422230 의 반경 0/flags 0 으로 표면까지 포함한다.
        let candidate = finder.begin({ left: cx, top: cy, right: cx, bottom: cy }, 0);
        // 원본 Buffer(10)는 가득 차면 추가하지 않는다.
        const victims = [];
        // 효과를 시작하기 전에 실제 finder 순서의 후보 목록을 완성한다.
        for (; candidate; candidate = finder.next()) {
            if (need(hooks.mayTarget)(sid, candidate) && !(typeOf(candidate).flags2 & 0x120000) && victims.length < MAX_VICTIMS) victims.push(candidate);
            if ((typeOf(candidate).flags2 & 4) && victims.length < MAX_VICTIMS) victims.push(candidate);
        }
        // 각 피해 효과 뒤 후보의 현재 free/genus 와 공격자의 현재 owner 를 다시 읽는다.
        for (const target of victims) {
            if (pool.slot(target)[11] & 1) continue;
            if (typeOf(target).flags2 & 4) need(hooks.weaken)(target);
            else {
                const rate = need(hooks.damageRate)(sid, target);
                need(hooks.damage)(target, truncate(rate * period), pool.slot(sid)[34], sid);
            }
        }
    }
}
