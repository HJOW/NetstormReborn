// 실제 표면 지도와 다리 J/K 주변 후보에서 AI 배치 스캔/30개 선택 목록을 옮긴 모듈이다(cpppj `o/RawAiDropScan.cpp/.h`, 10.78: 패치 00412ce0 다리 보정·00412e10 스캔·0040f390 제거).
//  - new RawAiDropScan(memory, pool, types, frames, spots, surfaces, alliances, random, globals, hooks): memory 는 E164 `RawAiBrainMemory`, pool 은 같은 PATCH SidPool, types/frames 는 현재 188 TYPE({ flags2 })/프레임 코드 표(FrameCodeTable 188개),
//    spots 는 월드 65536 BYTE spot(행 stride 256), surfaces 는 65536 WORD 표면 지도, alliances 는 방향별 81 DWORD(owner*9 + 상대), random 은 원본 공용 `GameRandom`, globals = { bridgeType()(005411a0 현재 다리 TYPE), bounds()(00544d14..20 현재 { left, top, right, bottom },
//    CRT 004fc530 초기값 10, 10, 246, 246) }, hooks = { free(type, x, y)(0040f130 같은 현재 TYPE/전체 spot 발자국), consider(token, type, point, direction, best, target)(00412b90 실제 후보 승인, best = `{ x, y }`·target = `{ value }`, 점수는 같은 본체 +0x3e8),
//    assertion(text, line)(0040f390 현재 원본 진단, 반환하면 현재 count 로 계속) } 이며 도달한 위치에서만 요구한다.
//  - adjustBridge(surface, cell): 00412ce0 — 표면이 현재 다리 TYPE 이면 프레임 코드 side 가 J 는 상/하, K 는 좌/우를 전체 BYTE 0 세 칸으로 검사해 기준 축을 먼저 쓰고(양쪽 가능하면 J 의 위쪽/K 의 왼쪽 우선) 다른 축에 공용 난수 3 을 더한다
//    (실패에서는 난수를 소비하지 않음). cell = `{ x, y }` 를 제자리에서 바꾼다. 다른 TYPE 는 프레임을 읽지 않는다.
//  - scan(token, type, direction): 00412e10 — x 를 먼저 증가(inclusive 우측/하단 소진 뒤 좌상단으로 되돌리고 false)하며 flags2 0x800 은 예산 500, 그 밖은 1000 으로 칸마다 −1, 표면이 있으면 −5, 다리 후보는 승인 전 −125,
//    일반 후보(flags2 0x4040800·(spot&3f)==2·본체 +0x2c8)는 동맹 행(표면 owner*9 + owner)이 켜지면 −10 이고 같은 owner 일 때 전체 발자국을 확인한다. 승인(consider)이 점수(+0x3e8)를 strict 로 올리면 30개 목록을 다시 시작하고 동률이면 추가한다
//    (30개가 가득하면 공용 난수로 한 칸만 교체하고 count 는 그대로). true 는 예산 소진(미완료)이다. 호출의 flags/경계는 snapshot 이고 본체 진행/owner/지도는 각 원문 위치에서 새로 읽는다.
//  - remove(token, index): 0040f390 — count 가 1 미만이면 진단 뒤에도 계속해 count 를 먼저 줄이고 좌표 → 표적 순으로 안정 압축하며 미사용 꼬리/점수/진행 위치는 보존한다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E173: AI 배치 스캔 이식으로 추가)
import { AI_BRAIN_BYTES } from './raw-ai-brain.js';
import { pathSide } from './raw-path-animation.js';

// 원본 최대 후보 수와 flags2 0x800 별 호출 예산, 지도 행 stride, 후보 좌표/표적 배열 위치, 다리 방향 글자이다. (2026-11-09 WB04-E173)
const CHOICES = 30;
const NORMAL_BUDGET = 1000;
const SUPPLY_BUDGET = 500;
const WIDTH = 256;
const POINT_AT = 0x408;
const TARGET_AT = 0x4f8;
const SIDE_J = 'J'.charCodeAt(0);
const SIDE_K = 'K'.charCodeAt(0);
const TWO_63 = 9223372036854775808;

// 비정렬 DWORD 의 실제 물리 범위를 확인해 읽는다. (2026-11-09 WB04-E173)
function read(raw, at) {
    if (at < 0 || at > raw.length || raw.length - at < 4) throw new RangeError('AI 배치 스캔 DWORD 범위');
    return (raw[at] | (raw[at + 1] << 8) | (raw[at + 2] << 16) | (raw[at + 3] << 24)) >>> 0;
}

// 원본 DWORD 의 signed 해석과 덧셈/뺄셈 감김이다. (2026-11-09 WB04-E173)
const signed = (value) => value | 0;
const add = (value, delta) => (value + delta) | 0;

// ftol 의 signed64 절삭 하위 DWORD(signed)이며 무효 FISTP 의 하위값은 0 이다. (2026-11-09 WB04-E173)
function ftol(value) {
    if (!Number.isFinite(value) || value < -TWO_63 || value >= TWO_63) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 선택된 실제 하위가 없으면 앞선 상태/효과를 보존하며 거부한다. (2026-11-09 WB04-E173)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('AI 배치 스캔 실제 하위 미연결');
    return hook;
}

// 배치 스캔 처리기이다. 같은 본체/월드 지도/원본 공용 난수와 실제 후보 승인만 사용한다. (2026-11-09 WB04-E173)
export class RawAiDropScan {
    #memory;
    #pool;
    #types;
    #frames;
    #spots;
    #surfaces;
    #alliances;
    #random;
    #globals;
    #hooks;

    // 생성 중 스캔/난수/장치를 실행하지 않는다. 같은 PATCH 월드의 크기를 확인한다. (2026-11-09 WB04-E173)
    constructor(memory, pool, types, frames, spots, surfaces, alliances, random, globals, hooks) {
        if (!memory || typeof memory.bytes !== 'function') throw new TypeError('AI 배치 스캔 본체 소유자 누락');
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== 188 || !Array.isArray(frames) || frames.length !== types.length
            || !(spots instanceof Uint8Array) || spots.length !== 65536 || !(surfaces instanceof Uint16Array) || surfaces.length !== 65536 || alliances?.length !== 81
            || typeof random?.next !== 'function' || typeof globals?.bridgeType !== 'function' || typeof globals.bounds !== 'function') {
            throw new TypeError('AI 배치 스캔 실제PATCH월드누락');
        }
        this.#memory = memory;
        this.#pool = pool;
        this.#types = types;
        this.#frames = frames;
        this.#spots = spots;
        this.#surfaces = surfaces;
        this.#alliances = alliances;
        this.#random = random;
        this.#globals = globals;
        this.#hooks = hooks ?? {};
    }

    // 같은 실제 월드의 풀인지 확인할 수 있도록 제공한다. (2026-11-09 WB04-E173)
    get pool() { return this.#pool; }

    // 해제/작은 가짜 본체를 거부한다. (2026-11-09 WB04-E173)
    #body(token) {
        const raw = this.#memory.bytes(token);
        if (raw.length !== AI_BRAIN_BYTES) throw new RangeError('AI 배치 스캔 실제0xb08본체 누락');
        return raw;
    }

    // 각 하위 효과 뒤 현재 본체를 다시 읽는다. (2026-11-09 WB04-E173)
    #field(token, at) { return read(this.#body(token), at); }

    // 지정 필드만 쓰고 다른 상태를 동기화하지 않는다. (2026-11-09 WB04-E173)
    #set(token, at, value) {
        const raw = this.#body(token);
        if (at < 0 || at > raw.length || raw.length - at < 4) throw new RangeError('AI 배치 스캔 쓰기 범위');
        // 지정 네 바이트만 쓴다.
        for (let index = 0; index < 4; index += 1) raw[at + index] = ((value >>> 0) >>> (index * 8)) & 0xff;
    }

    // 행 경계를 clamp 하면 원본 세 칸 검사의 주소가 달라지므로 전체 선형 범위만 확인한다. (2026-11-09 WB04-E173)
    #spot(x, y) {
        const at = y * WIDTH + x;
        if (at < 0 || at >= this.#spots.length) throw new RangeError('AI 다리 후보 spot 물리 범위');
        return this.#spots[at];
    }

    // 정상 후보 배열의 범위를 넘는 손상 count/index 는 명시 거부한다. (2026-11-09 WB04-E173)
    #point(index) {
        if (index < 0 || index >= CHOICES) throw new RangeError('AI 배치 후보 좌표 인덱스');
        return POINT_AT + index * 8;
    }

    // 같은 후보의 표적은 별도 30 DWORD 배열이다. (2026-11-09 WB04-E173)
    #target(index) {
        if (index < 0 || index >= CHOICES) throw new RangeError('AI 배치 후보 표적 인덱스');
        return TARGET_AT + index * 4;
    }

    // 각 쓰기 순서와 현재 count 재조회를 스캔 호출자가 정한다(좌표 x → y → 표적). (2026-11-09 WB04-E173)
    #choice(token, index, point, target) {
        const at = this.#point(index);
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat32(0, point.x, true);
        view.setFloat32(4, point.y, true);
        this.#set(token, at, view.getUint32(0, true));
        this.#set(token, at + 4, view.getUint32(4, true));
        this.#set(token, this.#target(index), target.value);
    }

    // 00412ce0: 양쪽 모두 가능하면 J 의 위쪽/K 의 왼쪽이 우선이며 실패 때 난수를 소비하지 않는다. (2026-11-09 WB04-E173)
    adjustBridge(surface, cell) {
        const raw = this.#pool.slot(surface);
        if (raw[10] !== (this.#globals.bridgeType() >>> 0)) return false;
        if (raw[10] >= this.#frames.length) throw new RangeError('AI 다리 후보 현재TYPE 범위');
        const side = pathSide(this.#frames[raw[10]], signed(read(raw, 36)));
        if (side === SIDE_J) {
            if (!this.#spot(cell.x, add(cell.y, -1)) && !this.#spot(cell.x, add(cell.y, -2)) && !this.#spot(cell.x, add(cell.y, -3))) cell.y = add(cell.y, -1);
            else {
                if (this.#spot(cell.x, add(cell.y, 1)) || this.#spot(cell.x, add(cell.y, 2)) || this.#spot(cell.x, add(cell.y, 3))) return false;
                cell.y = add(cell.y, 3);
            }
            cell.x = add(cell.x, this.#random.next(3));
            return true;
        }
        if (side === SIDE_K) {
            if (!this.#spot(add(cell.x, -1), cell.y) && !this.#spot(add(cell.x, -2), cell.y) && !this.#spot(add(cell.x, -3), cell.y)) cell.x = add(cell.x, -1);
            else {
                if (this.#spot(add(cell.x, 1), cell.y) || this.#spot(add(cell.x, 2), cell.y) || this.#spot(add(cell.x, 3), cell.y)) return false;
                cell.x = add(cell.x, 3);
            }
            cell.y = add(cell.y, this.#random.next(3));
            return true;
        }
        return false;
    }

    // 00412e10: 원본 유일 호출과 같은 +0x3e8 의 현재 점수를 그 자리에서 평가·비교한다. (2026-11-09 WB04-E173)
    scan(token, type, direction) {
        const hooks = this.#hooks;
        if (type >= this.#types.length) throw new RangeError('AI 배치 스캔 현재TYPE 범위');
        const flags = this.#types[type].flags2 >>> 0;
        const bounds = this.#globals.bounds();
        const left = ftol(bounds.left);
        const top = ftol(bounds.top);
        const right = ftol(bounds.right);
        const bottom = ftol(bounds.bottom);
        let budget = (flags & 0x800) ? SUPPLY_BUDGET : NORMAL_BUDGET;
        // 각 호출의 flags/경계는 snapshot 이고 본체 진행/소유자/지도를 각 원문 위치에서 새로 읽는다.
        do {
            let x = add(signed(this.#field(token, 0xaf8)), 1);
            this.#set(token, 0xaf8, x >>> 0);
            if (x > right) {
                this.#set(token, 0xafc, (this.#field(token, 0xafc) + 1) >>> 0);
                this.#set(token, 0xaf8, left >>> 0);
                if (signed(this.#field(token, 0xafc)) > bottom) {
                    this.#set(token, 0xafc, top >>> 0);
                    return false;
                }
            }
            x = signed(this.#field(token, 0xaf8));
            let y = signed(this.#field(token, 0xafc));
            if (x >= 0 && y >= 0 && x < 256 && y < 256) {
                const surface = this.#surfaces[y * WIDTH + x];
                if (surface) {
                    const cell = { x, y };
                    const bridge = this.adjustBridge(surface, cell);
                    x = cell.x;
                    y = cell.y;
                    budget -= 5;
                    let eligible = bridge;
                    if (!bridge && (flags & 0x4040800) && (this.#spot(x, y) & 0x3f) === 2 && this.#field(token, 0x2c8)) {
                        const surfaceOwner = this.#pool.slot(surface)[34];
                        const owner = this.#field(token, 0x260);
                        const at = surfaceOwner * 9 + owner;
                        if (at >= this.#alliances.length) throw new RangeError('AI 배치 스캔 현재 동맹 인덱스');
                        if (this.#alliances[at]) {
                            eligible = surfaceOwner === owner && Boolean(need(hooks.free)(type, x, y));
                            budget -= 10;
                        }
                    }
                    if (eligible) {
                        const previous = signed(this.#field(token, 0x3e8));
                        const point = { x: 0, y: 0 };
                        const target = { value: 0 };
                        if (need(hooks.consider)(token, type, { x: Math.fround(x), y: Math.fround(y) }, direction, point, target)) {
                            if (signed(this.#field(token, 0x3e8)) > previous) {
                                this.#set(token, 0x570, 0);
                                this.#choice(token, 0, point, target);
                                this.#set(token, 0x570, (this.#field(token, 0x570) + 1) >>> 0);
                            } else {
                                const count = signed(this.#field(token, 0x570));
                                if (count >= CHOICES) {
                                    const index = signed(this.#random.next(count >>> 0));
                                    this.#choice(token, index, point, target);
                                } else {
                                    this.#choice(token, count, point, target);
                                    this.#set(token, 0x570, (this.#field(token, 0x570) + 1) >>> 0);
                                }
                            }
                        }
                        budget -= 125;
                    }
                }
            }
            budget -= 1;
        } while (budget > 0);
        return true;
    }

    // 진단 뒤 현재 count 를 먼저 줄이고 복사하지 않는 꼬리/점수/진행 위치는 보존한다. (2026-11-09 WB04-E173)
    remove(token, index) {
        index |= 0;
        if (signed(this.#field(token, 0x570)) < 1) need(this.#hooks.assertion)('totalChoices > 0', 0x1ac);
        this.#set(token, 0x570, (this.#field(token, 0x570) - 1) >>> 0);
        // 현재 count 까지 XY → 표적 순서로 안정 압축하고 꼬리 바이트는 그대로 둔다.
        while (index < signed(this.#field(token, 0x570))) {
            const at = this.#point(index);
            const next = this.#point(add(index, 1));
            this.#set(token, at, this.#field(token, next));
            this.#set(token, at + 4, this.#field(token, next + 4));
            this.#set(token, this.#target(index), this.#field(token, this.#target(add(index, 1))));
            index = add(index, 1);
        }
    }
}
