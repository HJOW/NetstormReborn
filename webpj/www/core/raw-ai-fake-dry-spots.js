// 공급 시설/현재 부족 거리와 원본 양방향 탐색으로 AI 가짜 부족 좌표를 갱신하는 모듈이다(cpppj `o/RawAiFakeDrySpots.cpp/.h`, 10.78: 패치 00415fe0).
//  - AI_FAKE_DRY_LOG: 004105b0 의 두 원래 알림(Discarded 폐기·Added 추가)이다. 종류/owner 는 실제 출력 시점의 값을 쓴다.
//  - new RawAiFakeDrySpots(memory, pool, globals, hooks): memory 는 E164 `RawAiBrainMemory`, pool 은 같은 PATCH SidPool, globals = { manaRadius()(0052f488 현재 공급 반경, 제곱 반경이 아님), mapLimit()(00531928 을 각 좌표 유효성 검사에서 float 로 바꿈), debug()(0052f3dc, 29 이면 채택 뒤 실제 표식) },
//    hooks = { score(token, point, box)→float(00412640 실제 부족 거리, box.value 에 선택 index 를 쓴다), center(sid)→점(004adea0 현재 실제 시설 중심), candidate(source, target)→점(00415bb0 실제 양방향 탐색, E163 `RawAiDryGeometry.candidate`),
//    add(token, point, type)(0040f1c0 같은 Dry Spot Add, 반환 무시), remove(token, index)(0040f260 같은 index 0 안정 압축), seed(token)(004133e0 같은 실제 공용 난수/경계 좌표 선택), mark(point)(00410540 현재 TYPE/실제 표식 수명),
//    log(token, action, x, y)(004105b0, 현재 +0xb00 이 비영인 위치에서만 요청) } 이며 도달한 위치에서만 요구한다.
//  - extend(token): 00415fe0 전체 — 현재 가짜 TYPE(+0x280)가 0 이면 아무것도 하지 않고, 개수가 용량 이상이면 다른 모든 하위보다 먼저 index 0 을 제거한다. 공급 시설(+0x2fc)이 없으면 신전(+0x294)이 없을 때 끝, 있으면 신전의 raw 좌표를 같은 목록에 추가하고(중복 실패도 무시)
//    Added 로그를 남긴다. 공급 시설이 있으면 진입 시점의 count/포인터를 유지한 채 각 시설 raw 좌표의 부족 거리 점수가 최초 strict 최고가 되는 시설과 선택 index 를 고르고, 양수이면 선택 시설 중심과 선택 Dry Spot 좌표 − 1 을 두 점으로(출발은 중심 Y 를 X/Y 에 두 번 쓴 원본 인자) 탐색한다.
//    후보가 유효하고 중심과의 거리가 공급 반경의 0.7 배(float)보다 크면 같은 목록에 추가하고 현재 전략(+0x2d4 == 4)이면 전략 4 상태를 미정으로 되돌리며 +0xaf0/+0xaf4 를 갱신하고 디버그 29 이면 표식을 만든다. 아니면 Discarded 를 로그한다.
//    마지막으로 현재 가짜 좌표가 유효하지 않으면 실제 Seed 로 보충하고, 추가된 경우에만 원래 후보 좌표로 Added 를 로그한다. 각 하위 뒤에는 본체 필드를 다시 조회한다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E172: AI 가짜 부족 좌표 이식으로 추가)
import { AI_BRAIN_BYTES } from './raw-ai-brain.js';
import { aiPointDistance } from './raw-ai-dry-geometry.js';
import { aiPriestPointValid } from './raw-ai-priest-actions.js';

// 004105b0 의 두 원래 알림 종류이다. (2026-11-09 WB04-E172)
export const AI_FAKE_DRY_LOG = Object.freeze({ discarded: 'Discarded', added: 'Added' });

// 00501784 의 float 0.7(문자열 조각이 아님), 전략 4 의 미정 좌표 −10000 비트, 원본 표식 활성 디버그 종류, 부족 좌표 기록 폭(x/y float + TYPE DWORD)이다. (2026-11-09 WB04-E172)
const RADIUS_FACTOR = Math.fround(0.7);
const UNSET = 0xc61c4000;
const DEBUG = 29;
const DRY_STRIDE = 12;
const TWO_63 = 9223372036854775808;

// 실제 물리 블록 안의 비정렬 little endian DWORD 를 읽는다. (2026-11-09 WB04-E172)
function read(raw, at) {
    if (at < 0 || at > raw.length || raw.length - at < 4) throw new RangeError('AI 가짜좌표 DWORD 읽기 범위');
    return (raw[at] | (raw[at + 1] << 8) | (raw[at + 2] << 16) | (raw[at + 3] << 24)) >>> 0;
}

// 지정 DWORD 의 네 바이트만 기록한다(주변 미정 바이트 보존). (2026-11-09 WB04-E172)
function put(raw, at, value) {
    if (at < 0 || at > raw.length || raw.length - at < 4) throw new RangeError('AI 가짜좌표 DWORD 쓰기 범위');
    for (let index = 0; index < 4; index += 1) raw[at + index] = (value >>> (index * 8)) & 0xff;
}

// 원본 DWORD 의 signed 비교를 보존한다. (2026-11-09 WB04-E172)
const signed = (value) => value | 0;

// CRT signed64 절삭의 하위 DWORD(signed)이며 무효 FISTP 의 하위값은 0 이다. (2026-11-09 WB04-E172)
function ftol(value) {
    if (!Number.isFinite(value) || value < -TWO_63 || value >= TWO_63) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 단정도 float 와 비트의 상호 변환이다. (2026-11-09 WB04-E172)
function floatBits(value) {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
}
function bitsFloat(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 도달한 실제 하위가 없으면 앞선 쓰기를 유지하고 그 위치에서 거부한다. (2026-11-09 WB04-E172)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('AI 가짜좌표 실제 하위 미연결');
    return hook;
}

// 가짜 부족 좌표 갱신기이다. 실제 본체/목록만 읽고 선택된 하위 뒤 현재 필드를 다시 조회한다. (2026-11-09 WB04-E172)
export class RawAiFakeDrySpots {
    #memory;
    #pool;
    #globals;
    #hooks;

    // 생성 중 지도 탐색이나 목록 효과를 수행하지 않는다. (2026-11-09 WB04-E172)
    constructor(memory, pool, globals, hooks) {
        if (!memory || typeof memory.bytes !== 'function') throw new TypeError('AI 가짜좌표 본체 소유자 누락');
        if (pool?.edition !== 'patch1078') throw new TypeError('AI 가짜좌표의 같은 PATCH 풀 누락');
        for (const name of ['manaRadius', 'mapLimit', 'debug']) {
            if (typeof globals?.[name] !== 'function') throw new TypeError(`AI 가짜좌표 현재 전역 조회기 누락: ${name}`);
        }
        this.#memory = memory;
        this.#pool = pool;
        this.#globals = globals;
        this.#hooks = hooks ?? {};
    }

    // 같은 실제 월드의 풀인지 확인할 수 있도록 제공한다. (2026-11-09 WB04-E172)
    get pool() { return this.#pool; }

    // 작은 가짜 본체/해제 토큰을 성공으로 취급하지 않고 현재 살아 있는 0xb08 본체를 빌린다. (2026-11-09 WB04-E172)
    #body(token) {
        const raw = this.#memory.bytes(token);
        if (raw.length !== AI_BRAIN_BYTES) throw new RangeError('AI 가짜좌표 실제0xb08 본체 누락');
        return raw;
    }

    // 현재 본체를 하위 효과 뒤 다시 빌려 DWORD 를 읽는다. (2026-11-09 WB04-E172)
    #field(token, at) { return read(this.#body(token), at); }

    // 현재 지정 필드의 네 바이트만 기록한다. (2026-11-09 WB04-E172)
    #set(token, at, value) { put(this.#body(token), at, value >>> 0); }

    // 풀 밖 SID 를 잘린 WORD 의 다른 객체로 바꾸지 않는다(SID 0 도 원문처럼 허용). (2026-11-09 WB04-E172)
    #object(number) {
        if (number > 65535 || number >= this.#pool.capacity) throw new RangeError('AI 가짜좌표 현재 SID 물리 범위');
        return number;
    }

    // 중심 계산과 raw 위치 조회의 원래 차이를 유지해 현재 슬롯의 raw X/Y float 를 그대로 읽는다. (2026-11-09 WB04-E172)
    #position(sid) {
        const raw = this.#pool.slot(sid);
        return { x: bitsFloat(read(raw, 14)), y: bitsFloat(read(raw, 18)) };
    }

    // 원본은 로그가 꺼져도 두 ftol 인자(y → x)를 먼저 평가한다. (2026-11-09 WB04-E172)
    #log(token, action, point) {
        const y = ftol(point.y);
        const x = ftol(point.x);
        if (this.#field(token, 0xb00)) need(this.#hooks.log)(token, action, x, y);
    }

    // 현재 TYPE 가 없으면 full 검사조차 하지 않으며, full 제거는 다른 모든 하위보다 먼저다. (2026-11-09 WB04-E172)
    extend(token) {
        const hooks = this.#hooks;
        const globals = this.#globals;
        if (!this.#field(token, 0x280)) return;
        if (signed(this.#field(token, 0xae0)) >= signed(this.#field(token, 0xadc))) need(hooks.remove)(token, 0);
        let point = { x: 0, y: 0 };
        if (!this.#field(token, 0x2fc)) {
            const temple = this.#field(token, 0x294);
            if (!temple) return;
            point = this.#position(this.#object(temple));
            need(hooks.add)(token, point, this.#field(token, 0x280));
        } else {
            let added = false;
            let best = 0;
            const dryIndex = { value: 0 };
            let selectedIndex = 0;
            let selected = 0;
            if (signed(this.#field(token, 0x2fc)) > 0) {
                const count = signed(this.#field(token, 0x2fc));
                const pointer = this.#field(token, 0x2f4);
                // 진입 count/포인터를 유지하며 raw 위치/부족 거리는 각 항목에서 다시 조회한다.
                for (let index = 0; index < count; index += 1) {
                    const sid = read(this.#memory.bytes(pointer), index * 4);
                    const score = Math.fround(need(hooks.score)(token, this.#position(this.#object(sid)), dryIndex));
                    if (score > best) {
                        best = score;
                        selectedIndex = dryIndex.value;
                        selected = sid;
                    }
                }
                if (best > 0) {
                    const center = need(hooks.center)(this.#object(selected));
                    const records = this.#memory.bytes(this.#field(token, 0xae4));
                    if (selectedIndex < 0) throw new RangeError('AI 가짜좌표 선택 Dry Spot 음수 index');
                    const at = selectedIndex * DRY_STRIDE;
                    const target = { x: Math.fround(bitsFloat(read(records, at)) - 1), y: Math.fround(bitsFloat(read(records, at + 4)) - 1) };
                    point = need(hooks.candidate)({ x: center.y, y: center.y }, target);
                    if (aiPriestPointValid(point, globals.mapLimit())) {
                        if (aiPointDistance(point, center) > globals.manaRadius() * RADIUS_FACTOR) {
                            need(hooks.add)(token, point, this.#field(token, 0x280));
                            const wandering = this.#field(token, 0x2d4) === 4;
                            this.#set(token, 0xaf0, floatBits(point.x));
                            this.#set(token, 0xaf4, floatBits(point.y));
                            if (wandering) {
                                this.#set(token, 0x3cc, 0);
                                this.#set(token, 0x3d4, UNSET);
                                this.#set(token, 0x3d0, UNSET);
                                this.#set(token, 0x3d8, 1);
                            }
                            if (globals.debug() === DEBUG) need(hooks.mark)(point);
                            added = true;
                        } else this.#log(token, AI_FAKE_DRY_LOG.discarded, point);
                    }
                }
            }
            const current = { x: bitsFloat(this.#field(token, 0xaf0)), y: bitsFloat(this.#field(token, 0xaf4)) };
            if (!aiPriestPointValid(current, globals.mapLimit())) need(hooks.seed)(token);
            if (!added) return;
        }
        this.#log(token, AI_FAKE_DRY_LOG.added, point);
    }
}
