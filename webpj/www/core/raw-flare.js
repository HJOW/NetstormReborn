// 연구·업그레이드 효과가 사용하는 원본 플레어(106) 생성(45a6c0→45a480)과 Regular 수명(45a250)을 옮긴 모듈이다(cpppj `o/RawFlare.cpp/.h`, 10.78 전용).
// 실제 기준점 난수 분포·signed 깊이·주기·후속 플레어와 종료 순서를 보존한다. 같은 raw 슬롯/TYPE 프레임/게임 난수로 실행하고 생성·표시·수명·예약은 훅으로 받는다.
//  - handles(sid): 가상 표가 플레어 vtable 0x5096a0 인 객체만 처리한다.
//  - spawn(parent, count, event) / spawnDirection(parent, count, event, effectType, direction): 억제 시각(005c85f0)이 0 이 아니거나 로딩 깊이가 있으면 아무 것도 하지 않는다. 아니면 부모 깊이(+35)와 Kernel 개수를 한 번 읽고
//    count(signed)번 반복한다(Kernel 개수가 30000 초과면 중단). 매번 클라이언트 효과 생성(create(type, 2))→부모 현재 좌표와 SHP 핫스팟(sourceHotspot, 폭/높이가 아님)으로 기준점 난수 범위(부모가 이동/비행 genus 0x30000 면 오프셋 −0.7/−0.1)를 만들어
//    X·Y 난수 두 개를 뽑고, 효과의 프레임을 방향 글자(A + direction)의 첫 프레임으로 직접 쓰고 소유자 0·extra bit3·깊이−1 로 설정한 뒤 [1, 255] 로 보정한 `부모 − 난수` 좌표에 배치(pop)한다. 예약(reserveRegular)이 성공하면 `난수(0..0.07)+0.05`초
//    주기의 Regular 를 붙인다. spawn 은 기본 A 구간(현재 효과 타입), spawnDirection 은 주문의 C/D 방향이다.
//  - handle(sid, event, count, payload): 현재 프레임 방향 글자 구간(first/count)을 구한다. 단계 count==0 이면 주기를 난수(0.05..0.13)로 새로 정한다. event>1 이고 마지막 전 단계이면 Kernel 개수가 30000 초과가 아닐 때 후속 플레어를 부모 좌표(+0.5 절삭, ±0.4 난수)에
//    만들고 구간 첫 프레임으로 배치해 `event−1` 의 Regular 를 붙인다. 단계가 구간 끝이면 마지막 그림 갱신과 삭제 뒤 −1(종료)을 반환한다. 아니면 프레임을 `구간 첫 번호 + 단계`로 갱신하고, 구간 절반 단계에서 방향이 B 이면 위치 소리(powerup.wav)를 낸 뒤 주기를 반환한다.
//  - handler(fallback): 플레어 부모만 처리하고 나머지 부모는 기존 실제 처리기로 전달하는 Regular 분배기를 돌려준다.
// state = { suppressedTime(double), loadingDepth, type(106) } 의 live 속성, frames[type] 은 FrameCodeTable(codes·run), hooks = { processCount, create, sourceHotspot, owner, pop, reserveRegular, addRegular, frame, update, destroy, sound }(도달한 것만 필수).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E110: 플레어 이식으로 추가)
import { pathDirection } from './raw-path-animation.js';

// 플레어 vtable, 고도→칸 배율(ROM 504194)과 Kernel 개수 상한, 슬롯의 TYPE/상태/소유자/깊이/extra/프레임/좌표 위치, 판본의 TYPE 수이다. (2026-11-08 WB04-E110)
const FLARE_VTABLE = 0x005096a0;
const Y_SCALE = Math.fround(0.09090909361839294);
const PROCESS_LIMIT = 30000;
const TYPE_AT = 10;
const OWNER_AT = 34;
const DEPTH_AT = 35;
const EXTRA_AT = 40;
const FRAME_AT = 36;
const X_AT = 14;
const Y_AT = 18;
const TYPE_COUNT = 188;
const fround = Math.fround;

// 필수 효과를 실제 호출할 때만 검사한다. (2026-11-08 WB04-E110)
function need(callback) {
    if (typeof callback !== 'function') throw new Error('플레어 실제 하위 미연결');
    return callback;
}

// 실제 fadd 0.5 → CRT ftol → 하위 signed DWORD → float 저장 순서이다. 불확정(invalid)의 하위 DWORD 는 0 이다. (2026-11-08 WB04-E110)
function integer(input) {
    const value = input + 0.5;
    if (!Number.isFinite(value) || value < -(2 ** 63) || value >= 2 ** 63) return 0;
    return fround(Number(BigInt.asIntN(32, BigInt(Math.trunc(value)))));
}

// 실제 x87 unordered 분기는 NaN 을 유지하며 유한 좌표에만 1..255 를 적용한다. (2026-11-08 WB04-E110)
function clamp(value) {
    if (value < 1) value = 1;
    if (255 < value) value = 255;
    return value;
}

// 플레어 생성/수명 처리기이다. (2026-11-08 WB04-E110)
export class RawFlare {
    // 모든 현재 효과와 부모가 사용하는 원본 풀이다. (2026-11-08 WB04-E110)
    #pool;
    // 원본 부모 genus 를 읽는 현재 188 TYPE 이다. (2026-11-08 WB04-E110)
    #types;
    // 현재 frame code 와 first/count 를 제공하는 프레임 표 188칸이다. (2026-11-08 WB04-E110)
    #frames;
    // 다른 게임 효과와 공유하는 00532710 난수이다. (2026-11-08 WB04-E110)
    #random;
    // 원본 현재 전역(억제 시각·로딩 깊이·효과 타입)이다. (2026-11-08 WB04-E110)
    #state;
    // 선택한 실제 외부 몸체의 연결이다. (2026-11-08 WB04-E110)
    #hooks;

    // 같은 PATCH 자료의 188개 타입/프레임만 허용하며 생성 시 게임 효과는 실행하지 않는다. (2026-11-08 WB04-E110)
    constructor(pool, types, frames, random, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || !types || types.length !== TYPE_COUNT || !frames || frames.length !== TYPE_COUNT) throw new Error('플레어 PATCH 풀/TYPE/프레임 오류');
        this.#pool = pool;
        this.#types = types;
        this.#frames = frames;
        this.#random = random;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 단일 월드 조립 검사용 참조이다. (2026-11-08 WB04-E110)
    get pool() { return this.#pool; }

    // 가상 표를 확인해 이름만 같은 다른 파생에 처리기를 적용하지 않는다. (2026-11-08 WB04-E110)
    handles(sid) { return this.#read(sid, 0) === FLARE_VTABLE; }

    // 원본 DWORD 만 낮은 바이트부터 조합한다(인접 필드로 읽기 폭을 넓히지 않음). (2026-11-08 WB04-E110)
    #read(sid, offset) {
        const raw = this.#pool.slot(sid);
        return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
    }

    // 현재 float 좌표를 읽는다. (2026-11-08 WB04-E110)
    #coordinate(sid, offset) {
        const raw = this.#pool.slot(sid);
        return new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getFloat32(offset, true);
    }

    // 원본의 직접 frame 쓰기는 SquidFrame 통지를 호출하지 않으며 네 바이트만 덮어쓴다(깊이/owner/extra 유지). (2026-11-08 WB04-E110)
    #put(sid, offset, value) {
        const raw = this.#pool.allocated(sid);
        for (let index = 0; index < 4; index += 1) raw[offset + index] = (value >>> (index * 8)) & 0xff;
    }

    // 실제 455930 의 같은 전역 난수와 53비트 계산이다: 난수(65536)/65536 × |high−low| + low. (2026-11-08 WB04-E110)
    #randomRange(low, high) { return this.#random.next(65536) * (1.0 / 65536.0) * Math.abs(high - low) + low; }

    // TYPE 의 실제 코드 방향으로부터 first/count 를 얻고 잘못된 물리 프레임은 거부한다. (2026-11-08 WB04-E110)
    #run(sid) {
        const type = this.#pool.slot(sid)[TYPE_AT];
        if (type >= this.#frames.length) throw new RangeError('플레어 현재 TYPE 범위');
        const direction = pathDirection(this.#frames[type], this.#read(sid, FRAME_AT) | 0);
        if (direction < 0 || direction >= 16) throw new RangeError('플레어 현재 프레임 방향 범위');
        return this.#frames[type].run(0x41 + direction);
    }

    // 각 생성 후 현재 부모 좌표/기준점을 다시 읽고 로컬 Kernel 개수만 하나씩 증가시킨다. 기본 wrapper 는 A 방향이다. (2026-11-08 WB04-E110)
    spawn(parent, count, event) { this.spawnDirection(parent, count, event, this.#state.type, 0); }

    // 주문은 같은 생성 몸체에 C/D 방향을 전달하며 기본 wrapper 의 A 방향과 구별한다. (2026-11-08 WB04-E110)
    spawnDirection(parent, count, event, effectType, direction) {
        const hooks = this.#hooks;
        const state = this.#state;
        const pool = this.#pool;
        if (state.suppressedTime !== 0 || state.loadingDepth) return;
        if (direction >= 16) throw new RangeError('플레어 생성 방향 범위');
        const depth = pool.slot(parent)[DEPTH_AT];
        let size = need(hooks.processCount)();
        // 음수/0개는 생성하지 않으며 30000개일 때는 마지막 하나만 더 만든다.
        for (let index = 0; index < (count | 0); index += 1) {
            if (size > PROCESS_LIMIT) return;
            const child = need(hooks.create)(effectType, 2);
            const x = this.#coordinate(parent, X_AT);
            const y = this.#coordinate(parent, Y_AT);
            const hotspot = need(hooks.sourceHotspot)(parent);
            const type = pool.slot(parent)[TYPE_AT];
            if (type >= this.#types.length) throw new RangeError('플레어 부모 현재 TYPE 범위');
            const shifted = (this.#types[type].flags2 & 0x30000) !== 0;
            const offsetX = shifted ? fround(-0.7) : 0;
            const offsetY = shifted ? fround(-0.1) : 0;
            const highX = fround(hotspot[0] * 0.0625 + offsetX);
            const highY = fround(hotspot[1] * Y_SCALE + offsetY);
            const randomX = this.#randomRange(offsetX, highX);
            const randomY = this.#randomRange(offsetY, highY);
            const currentType = pool.slot(child)[TYPE_AT];
            if (currentType >= this.#frames.length) throw new RangeError('플레어 생성 현재 TYPE 범위');
            this.#put(child, FRAME_AT, this.#frames[currentType].run(0x41 + direction).first >>> 0);
            need(hooks.owner)(child, 0);
            const raw = pool.allocated(child);
            raw[EXTRA_AT] |= 8;
            raw[DEPTH_AT] = (depth - 1) & 0xff;
            const spawnX = clamp(fround(x - randomX));
            const spawnY = clamp(fround(y - randomY));
            need(hooks.pop)(child, spawnX, spawnY, 0);
            if (need(hooks.reserveRegular)()) need(hooks.addRegular)(child, event, fround(this.#randomRange(0, fround(0.07)) + 0.05));
            size += 1;
        }
    }

    // count/event 은 원본 signed int 비교이며 실제 Kernel 의 uint32 비트 입력을 유지한다. (2026-11-08 WB04-E110)
    handle(sid, event, count, payload) {
        const hooks = this.#hooks;
        const pool = this.#pool;
        if (!this.handles(sid)) throw new Error('플레어 현재 가상 표 오류');
        const run = this.#run(sid);
        const step = count | 0;
        const kind = event | 0;
        if (!step) payload = fround(this.#randomRange(fround(0.05), fround(0.13)));
        if (kind > 1 && step === run.count - 1) {
            if (need(hooks.processCount)() > PROCESS_LIMIT) return 0;
            const child = need(hooks.create)(pool.slot(sid)[TYPE_AT], 2);
            const x = integer(this.#coordinate(sid, X_AT));
            const y = integer(this.#coordinate(sid, Y_AT));
            const randomX = this.#randomRange(fround(-0.4), fround(0.4));
            const randomY = this.#randomRange(fround(-0.4), fround(0.4));
            const childY = fround(randomY + y);
            this.#put(child, FRAME_AT, run.first >>> 0);
            need(hooks.pop)(child, fround(randomX + x), childY, 0);
            if (need(hooks.reserveRegular)()) need(hooks.addRegular)(child, (event - 1) >>> 0, 0);
        }
        if (step >= run.count) {
            need(hooks.update)(sid);
            need(hooks.destroy)(sid, 0);
            return -1;
        }
        need(hooks.frame)(sid, ((run.first >>> 0) + (count >>> 0)) | 0, 0);
        if (step === Math.trunc(run.count / 2) && pathDirection(this.#frames[pool.slot(sid)[TYPE_AT]], this.#read(sid, FRAME_AT) | 0) === 1) {
            need(hooks.sound)(this.#coordinate(sid, X_AT), this.#coordinate(sid, Y_AT), 'powerup.wav', 0);
        }
        return payload;
    }

    // 나머지 부모는 기존 실제 처리기로 전달한다. (2026-11-08 WB04-E110)
    handler(fallback = null) {
        return (sid, event, count, payload) => {
            if (this.handles(sid)) return this.handle(sid, event, count, payload);
            if (typeof fallback !== 'function') throw new Error('플레어 외 Regular 미연결');
            return fallback(sid, event, count, payload);
        };
    }
}
