// 아이스 Missile 의 반사점 00482080 과 후보 선택/후속 생성 요청 00482190 을 옮긴 모듈이다(cpppj `o/RawIceRicochet.cpp/.h`, 10.78 전용).
// 원문의 float 저장 위치·현재 필드 재조회·후보 순서와 직접 FCOS/FSIN 각 입력을 보존한다. 전체 반사탄 수명(RawMissile)과 분리한 반사점/확률/표적 선택 몸체이다.
//  - point(fields, output): 표시 자산 부모의 현재 위치와 원점(필드 +38/+3c)이 같으면 output 과 난수를 건드리지 않고 false 이다. 다르면 원점→현재 방향각을 구하고(directionAngle) 난수 두 번
//    (100 중 50 이상이면 부호 −, 65536 분수×120+30 도 편차)으로 각을 정규화(음수 보정·360 초과 fmod)해 `cos/sin(각)×사거리+현재 위치`를 [1,255] 로 clamp 한 두 float 를 output 에 쓰고 true 이다.
//  - generate(fields): 난수 99 중 19 이하이면 끝낸다. 아니면 반사점을 구하고(변위 0 이면 호출 전 지역 8바이트 `initialPointBytes` 훅 입력을 점으로 사용; 없으면 거부), 점이 보드 유효 범위(0<x,y<256)가 아니면 끝낸다.
//    점의 거의 올림 칸 3×3 사각형으로 일반 finder(begin/next 훅, flags 0)를 돌려 HP 보유(훅)·서버 예약 번호(5..14999) 제외·raw+40 비트 1 제외·비행(0x120000) 제외·현재 표적 제외 후보 중
//    점에서 가장 가까운 것(동률은 최초)을 고르고, 표적이 없어도 후속 Missile 생성 요청(속도=필드 +30, 표시=표시 자산 TYPE, 반사 1, 포구=표시 자산 위치)을 createProjectile 훅에 넘긴다.
// missileTypes[type].range 는 현재 표시 타입의 반사 사거리(float)이다. 훅 { begin(area, flags)→sid, next()→sid, hasHitPoints(sid), createProjectile(request), initialPointBytes(bytes8), angles(진단 장치) } 는 도달한 지점에서만 요구한다.
//  - makeIceRicochetWorldHooks(pool, hash, types, reward, hooks): 같은 풀의 실제 일반 finder 와 SquidReward.hasHitPoints 를 begin/next/hasHitPoints 훅에 묶는다(풀이 다르면 거부).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E124: RawIceRicochet 이식으로 추가)
import { RawArcherAnimation } from './raw-archer-animation.js';
import { RawSquidFinder } from './raw-squid-finder.js';

// 실제 00502310 과 0050b534 의 binary32 상수(0.99999f, 1/360f)와 후보의 두 비행 플래그이다. (2026-11-08 WB04-E124)
const SNAP_BIAS = Math.fround(0.99999);
const INVERSE_TURN = Math.fround(1.0 / 360.0);
const AIRBORNE = 0x120000;
const TYPE_COUNT = 188;
const fround = Math.fround;
const floatView = new DataView(new ArrayBuffer(4));

// 현재 비정렬 리틀엔디언 DWORD 의 float 좌표를 읽는다. (2026-11-08 WB04-E124)
function float(raw, at) {
    floatView.setUint32(0, ((raw[at] | (raw[at + 1] << 8) | (raw[at + 2] << 16) | (raw[at + 3] << 24)) >>> 0), true);
    return floatView.getFloat32(0, true);
}

// 좌표 쌍을 비트 그대로 읽는다. (2026-11-08 WB04-E124)
function point(raw, at) { return [float(raw, at), float(raw, at + 4)]; }

// 00468e70: 정확히 360 인 값은 유지하며 음수 보정에서 float 역수를 사용한다. (2026-11-08 WB04-E124)
function normalize(angle) {
    let value = angle;
    if (value < 0) value = fround(value + (1 - Math.trunc(value * INVERSE_TURN)) * 360.0);
    return value > 360 ? fround(value % 360.0) : value;
}

// 00422230: 보정/절삭 후 float 를 거쳐 다시 절삭한다. 유효 지도 점만 호출한다. (2026-11-08 WB04-E124)
function cell(value) { return Math.trunc(fround(Math.trunc(value + SNAP_BIAS))); }

// 원본 BoardCoord 유효 범위이며 NaN 또한 거부한다. (2026-11-08 WB04-E124)
function valid(at) { return at[0] > 0 && at[0] < 256 && at[1] > 0 && at[1] < 256; }

// std::clamp 와 같은 비교 순서(NaN 은 그대로 통과)이다. (2026-11-08 WB04-E124)
function clamp(value, low, high) { return value < low ? low : high < value ? high : value; }

// 아이스 반사 논리이다. 사용: new RawIceRicochet(pool, types, missileTypes, random, hooks). (2026-11-08 WB04-E124)
export class RawIceRicochet {
    // 표시와 후보가 사용하는 같은 raw 슬롯 저장소, 후보 HP/비행 속성의 타입 표, 현재 표시 타입의 반사 사거리, SP/공격/소리도 소비하는 같은 32비트 난수, 같은 Finder/HP/후속 생성 경계이다.
    #pool;
    #types;
    #missileTypes;
    #random;
    #hooks;

    // 같은 패치판 자료와 공유 난수만 허용한다. (2026-11-08 WB04-E124)
    constructor(pool, types, missileTypes, random, hooks = {}) {
        if (pool?.edition !== 'patch1078' || !types || types.length !== TYPE_COUNT || !missileTypes || missileTypes.length !== types.length) throw new Error('아이스 반사 PATCH/타입 자료 오류');
        this.#pool = pool;
        this.#types = types;
        this.#missileTypes = missileTypes;
        this.#random = random;
        this.#hooks = hooks;
    }

    // 원문이 허용하는 저번호 서버 슬롯도 물리 저장소 안에서 읽는다. TYPE 범위를 검사한 읽기 사본을 돌려준다. (2026-11-08 WB04-E124)
    #slot(sid) {
        if (sid >= this.#pool.capacity) throw new RangeError('아이스 반사 SID 범위');
        const raw = this.#pool.slot(sid);
        if (raw[10] >= this.#types.length) throw new RangeError('아이스 반사 TYPE 범위');
        return raw;
    }

    // 00482080: 실제 float 방향/두 난수/정규화 뒤 원본처럼 각을 변환하지 않고 삼각함수에 넣는다. 변위 0 이면 output 과 난수를 보존하고 false, 나머지는 원본 점을 쓰고 true 이다. (2026-11-08 WB04-E124)
    point(fields, output) {
        const raw = this.#slot(fields.parent);
        const current = point(raw, 14);
        const origin = point(fields.bytes, 56);
        if (current[0] === origin[0] && current[1] === origin[1]) return false;
        const direction = RawArcherAnimation.directionAngle(origin, current, this.#hooks.angles);
        const sign = this.#random.next(100) > 49 ? -1.0 : 1.0;
        const offset = this.#random.next(65536) * (1.0 / 65536.0) * 120.0 + 30.0;
        const angle = normalize(fround(offset * sign + direction));
        const range = this.#missileTypes[raw[10]].range;
        output[0] = clamp(fround(Math.cos(angle) * range + current[0]), 1.0, 255.0);
        output[1] = clamp(fround(Math.sin(angle) * range + current[1]), 1.0, 255.0);
        return true;
    }

    // 00482190: [0,99) 확률·현재 후보 선택 후 새 Missile 을 요청한다. 빈 경계를 성공 처리하지 않는다. 원본은 표적이 없어도 target 0 의 반사 Missile 을 생성한다. (2026-11-08 WB04-E124)
    generate(fields) {
        const hooks = this.#hooks;
        if (this.#random.next(99) <= 19) return;
        this.#slot(fields.parent);
        // 표시 슬롯 주소를 잡아 두므로 순회 중 바뀐 바이트를 그대로 읽는 live 뷰이다.
        const display = this.#pool.physical(fields.parent);
        let at = [123.25, 124.5];
        // 원문 00482190 은 변위 0 에서도 point 가 보존한 실제 지역 8바이트로 valid 를 계속한다. 같은 호출의 외부 바이트 입력으로 옮긴 뒤 판정한다.
        if (!this.point(fields, at)) {
            if (typeof hooks.initialPointBytes !== 'function') throw new Error('아이스 반사 현재 지역점 8BYTE 입력 미연결');
            const bytes = new Uint8Array(8);
            hooks.initialPointBytes(bytes);
            at = point(bytes, 0);
        }
        if (!valid(at)) return;
        if (typeof hooks.begin !== 'function' || typeof hooks.next !== 'function' || typeof hooks.hasHitPoints !== 'function') throw new Error('아이스 반사 실제 Finder/HP 미연결');
        const x = cell(at[0]);
        const y = cell(at[1]);
        let bestDistance = 10000.0;
        let best = 0;
        // 현재 flags2 를 먼저 캡처하고 HP 호출 뒤 나머지 raw/표적을 다시 읽는다. 동률은 최초 후보를 유지한다.
        for (let sid = hooks.begin({ left: x - 1, top: y - 1, right: x + 1, bottom: y + 1 }, 0); sid; sid = hooks.next()) {
            this.#slot(sid);
            const raw = this.#pool.physical(sid);
            const flags2 = this.#types[raw[10]].flags2;
            if (!hooks.hasHitPoints(sid) || (sid >= 5 && sid <= 14999) || (raw[40] & 1) || (flags2 & AIRBORNE) || sid === fields.target) continue;
            const candidate = point(raw, 14);
            const dx = candidate[0] - at[0];
            const dy = candidate[1] - at[1];
            const distance = Math.sqrt(dx * dx + dy * dy);
            if (distance < bestDistance) {
                best = sid;
                bestDistance = fround(distance);
            }
        }
        if (typeof hooks.createProjectile !== 'function') throw new Error('아이스 반사 실제 후속 Missile 생성 미연결');
        hooks.createProjectile({
            firing: fields.parent,
            target: best,
            speed: float(fields.bytes, 48),
            displayType: display[10],
            effectType: 0,
            targetPosition: at,
            targetCenter: [-10000, -10000],
            ricochet: 1,
            firingPosition: point(display, 14)
        });
    }
}

// 각 시작은 원본처럼 전체 Finder 상태를 초기화하며 flags 0 의 일반 공간 체인을 사용한다. 같은 풀의 실제 해시/HP(SquidReward)를 반사 훅에 묶고 후속 Missile 생성 경계는 그대로 보존한다. 사용: makeIceRicochetWorldHooks(pool, hash, types, reward, hooks). (2026-11-08 WB04-E124)
export function makeIceRicochetWorldHooks(pool, hash, types, reward, hooks) {
    if (pool !== reward.pool) throw new Error('아이스 반사 Finder/HP 풀 불일치');
    const finder = new RawSquidFinder(pool, hash, types);
    return {
        ...hooks,
        begin: (area, flags) => finder.begin(area, flags),
        next: () => finder.next(),
        hasHitPoints: (sid) => reward.hasHitPoints(sid)
    };
}
