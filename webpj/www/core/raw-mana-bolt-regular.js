// 마나 볼트의 실제 주기 프레임과 건설 대상 옆 화면 위치를 옮긴다(cpppj o/RawManaBoltRegular.cpp/.h, 원문 45a8e0, 10.78 전용). 기준선 1f00a25a 와 기준선 뒤 dfde4ad3(20261110-49) 반영분을 함께 쓴다.
// 현재 프레임(글자 구간 안 count 의 signed 나머지)을 적용하고, event 가 0 이면(count 0 일 때만) 위치 소리를 낸 뒤, event 1 이면 같은 finder 의 현재 대상(마나 볼트 도착 Target)을 찾아
// 대상이 없거나 extra 비트 1 이 없으면 자기 삭제 뒤 -1, 있으면 대상 옆 화면 위치(DWORD 감김 산술)를 현재 카메라 역투영으로 월드 float 로 되돌려 Unpop→Pop 한다. 반환은 다음 실행 간격(0.05) 또는 -1 이다.
// 기준선 뒤 변경 판정(3.6절 ②): Handles 는 TYPE 바이트를 선검사하지 않고 가상 표 0x509810 만 보며, 자기 TYPE frame0 은 선택 장치 frameZero(type)가 있으면 그 결과를 쓰고 없으면 기준선의 shapes 검사를 쓴다.
// 사용: const regular = new RawManaBoltRegular(pool, types, frames, shapes, arrival, state, hooks); regular.run(sid, event, count) → 간격|-1. (2026-11-15 WB04-E468)
import { pathDirection } from './raw-path-animation.js';

// 가상 표 0x509810(마나 볼트)과 아이콘 소리 이름, 슬롯 필드 위치(TYPE +10, 프레임 +36, WORD +12, extra +40, 좌표 +14/+18)이다.
const BOLT_VTABLE = 0x509810; const SOUND = 'influenceIcon.wav'; const TYPE_AT = 10; const WORD_AT = 12; const X_AT = 14; const Y_AT = 18; const FRAME_AT = 36; const EXTRA_AT = 40;
// 건설/표면 SHP 헤더의 미적재 표식 두 값이다(maxHitPoints 에 남은 sentinel).
const BAD_HIT_POINTS = [-0x22222223, -0x32323233];

// 주기 함수가 읽는 실제 간격 전역이다(00532750 다음 실행 간격, 005325e0 아이콘 가로/세로 간격). 카메라는 별도 현재 투영 경계에서 읽는다.
export class ManaBoltRegularState {
    // 단정도 0.05 간격과 정수 16 간격이다.
    interval = Math.fround(0.05); spacing = 16;
}

// 정렬되지 않은 raw 필드의 지정 폭만 읽는다(낮은 바이트부터).
function read(raw, offset, width = 4) { let value = 0; for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index); return value; }
// 슬롯의 비정렬 float DWORD 를 읽는다.
function float(raw, offset) { return new DataView(raw.buffer, raw.byteOffset + offset, 4).getFloat32(0, true); }
// 선택된 실제 효과가 없으면 생략하지 않는다(C++ std::logic_error → Error).
function need(hook) { if (typeof hook !== 'function') throw new Error('마나 볼트 주기 효과 미연결'); return hook; }

// 마나 볼트 주기 본체이다. 모든 참조는 이 모듈보다 오래 살아야 하고 hooks = { setFrame(sid, frame, flags), sound(x, y, name), project(x, y)→[x, y], inverse(x, y)→[x, y], unpop(sid, flags), destroy(sid, flags), pop(sid, x, y, flags), frameZero?(type)→SHP 헤더 }.
export class RawManaBoltRegular {
    // 프레임/좌표/WORD/현재 대상 raw 의 같은 풀, 현재 TYPE 의 해제 흔적과 플래그, 현재 물리 프레임의 글자/구간 표, 자기 TYPE frame0 의 pixel 폭/핫스폿, 같은 finder 의 도착 검색, 간격/spacing 과 효과다.
    #pool; #types; #frames; #shapes; #arrival; #state; #hooks;
    // 10.78 판본·188 TYPE 표 크기·같은 월드의 도착 검색만 허용한다(C++ invalid_argument → TypeError).
    constructor(pool, types, frames, shapes, arrival, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || types?.length !== 188 || frames?.length !== types.length || shapes?.length !== types.length || arrival?.pool !== pool) throw new TypeError('마나 볼트 주기 판본/같은 월드 오류');
        this.#pool = pool; this.#types = types; this.#frames = frames; this.#shapes = shapes; this.#arrival = arrival; this.#state = state; this.#hooks = hooks;
    }

    // 다른 월드의 분배기에 연결하지 않게 하는 같은 풀 참조다.
    get pool() { return this.#pool; }

    // 현재 가상 표 0x509810 만 Regular 분배에서 받는다(기준선 뒤 변경: TYPE 바이트를 별도로 선검사하지 않는다).
    handles(sid) { return read(this.#pool.slot(sid), 0) === BOLT_VTABLE; }

    // 현재 프레임 → 대상/위치 → 0.05 또는 삭제 후 -1 이다. 가상 자기 삭제 뒤에는 raw 를 읽지 않는다.
    run(sid, event, count) {
        if (!this.handles(sid)) throw new Error('다른 마나 볼트 주기 가상 몸체');
        const pool = this.#pool; const state = this.#state; const hooks = this.#hooks; const raw = pool.slot(sid); const type = raw[TYPE_AT];
        const direction = pathDirection(this.#frames[type], read(raw, FRAME_AT) | 0);
        if (direction < 0 || direction >= 16) throw new RangeError('마나 볼트 주기 글자 구간');
        const run = this.#frames[type].run(0x41 + direction); if (run.count <= 0) throw new Error('마나 볼트 주기 frame 나머지0');
        if (!event && !count) need(hooks.sound)(float(raw, X_AT), float(raw, Y_AT), SOUND);
        need(hooks.setFrame)(sid, (((count | 0) % run.count) + run.first) | 0, 0);
        if (event !== 1) return state.interval;
        const target = this.#arrival.target(sid);
        if (!target || !(pool.slot(target)[EXTRA_AT] & 1)) { need(hooks.destroy)(sid, 0); return -1; }
        const currentType = pool.slot(sid)[TYPE_AT]; let frame;
        if (hooks.frameZero) frame = need(hooks.frameZero)(currentType); // 실제 Native 는 같은 상세/단언 장치를 거쳐 저장한 TYPE 의 현재 SHP 를 읽는다.
        else {
            const shape = this.#shapes[currentType]; const record = this.#types[currentType];
            if (!shape.loaded || shape.frameCount <= 0 || !shape.frames?.length || BAD_HIT_POINTS.includes(record.maxHitPoints)) throw new Error('마나 볼트 자기 TYPE frame0 SHP 오류');
            frame = shape.frames[0];
        }
        const targetRaw = pool.slot(target); const point = Array.from(need(hooks.project)(float(targetRaw, X_AT), float(targetRaw, Y_AT)));
        // 화면 정수 산술은 DWORD 로 감긴다: X 는 폭+핫스폿+간격, Y 는 (WORD+1)×간격 을 뺀다.
        point[0] = (point[0] - (((frame.width + frame.hotspotX) >>> 0) + (state.spacing >>> 0))) | 0;
        point[1] = (point[1] - Math.imul((read(pool.slot(sid), WORD_AT, 2) + 1) >>> 0, state.spacing >>> 0)) | 0;
        need(hooks.unpop)(sid, 0); const position = need(hooks.inverse)(point[0], point[1]); need(hooks.pop)(sid, position[0], position[1], 0); return state.interval;
    }
}
