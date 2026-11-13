// 원본 일반 자산/그림자 수집기(10.78 00433660 ClientBoard 의 일반 자산 수집)를 옮긴 모듈이다: 해시 1~3단계의 칸을 y 바깥/x 안쪽으로 훑어 가시 객체의 본체와 그림자를 공통 표시 목록의 kind 0 항목으로 쓴다(cpppj `client/ObjectCollection.cpp/.h`, 10.78 전용. CD 004887a0 경로는 범위 밖).
//  - collect(view, output, limit): 해시 단계 1·2·3 마다 `select(단계)` 하고 월드 가시 사각형을 단계 크기로 나눠(좌표 + 0.5 의 CRT 절삭 하위 DWORD 를 signed 로 나눔, 상단 칸으로 제한) 양 끝을 한 칸씩 넓힌 뒤(DWORD 감김 + 각 끝의 독립 clamp) 칸마다 체인을 끝까지 방문한다.
//    칸 안 SID 마다: 타입 0/표 밖이면 거부하고 프레임(10.78 은 DWORD +36)·월드 위치(+14/+18 float)를 읽어 본체(프레임)가 가시 영역과 교차하면 기록하고, 타입 flags1 의 그림자 비트(0x40000)가 있으면 `프레임 + 논리 프레임 수` 의 그림자 프레임도 판정해 기록한다.
//    교차 판정: 프레임 상자를 `view.left − cellWidth − 1`·`view.top − cellHeight − 1` 부터 `view.right/bottom` 까지의 닫힌 구간에 객체 위치에서 프레임 기준점을 뺀 값이 들어갈 때(x 차이는 x87 정밀도, y 차이는 float 로 저장)이다.
//    기록 항목: x/y 는 현재 슬롯의 float 위치(그림자는 y − 0.1, walker(flags2 0x10000) 본체는 y + 0.5, float 로 저장), 토큰 `tokenBase + SID × 50`, 깊이는 +35 의 signed BYTE, kind 0, tail 은 그림자이거나 abstract(flags1 0x400000) 타입의 프레임이 논리 프레임 수 이상이면 4 아니면 0.
//    프레임 검사(`Frame`): 로드된 타입이고 프레임이 물리 프레임 수 안이며(아니면 'physical frame'), 논리 프레임 수 ×(abstract/그림자 타입이면 2)를 넘지 않고 HP 쓰레기 표식(0xdddddddd/0xcdcdcdcd)이 아니어야 한다(아니면 'frameCheck').
//    같은 체인의 순환은 거부하지만 다른 칸에서 같은 SID 가 반복되는 것은 전역 중복 제거하지 않고 그대로 낸다. 원본 strict limit 은 **기록 후** 검사한다(`count >= limit` 이면 오류, 실패 전 기록/선택 상태는 남을 수 있음). 정상 반환 때 해시 선택은 2단계가 된다.
//    다음 SID 는 판정이 끝난 뒤 현재 슬롯(+4 WORD)에서 다시 읽는다. 풀 범위/타입/프레임은 기록 전에 검사하고 실제 출력 칸 넘침은 쓰기 전에 거부한다.
//  - object(token): 수집된 대리 토큰을 같은 풀의 SID 로 되돌린다(기준 미만·0·stride 배수 아님·용량 밖은 거부). bindObjectCollection(hooks, collector): 공통 수집기 묶음의 `objects` 슬롯을 이 수집기로 연결한다.
// 인자: hash = `SquidHash`(select/at 사용), pool = `SidPool`, types = 타입별 { flags1, flags2, maxHitPoints }, shapes = 타입별 { loaded, frameCount, frames:[{ cellWidth, cellHeight, cellHotspotX, cellHotspotY }] }, view = { left, top, right, bottom }(월드 float),
//   output = 16바이트 항목 `Uint8Array`. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E389: 5-A 타입별 Draw·미리보기의 일반 객체 수집기로 추가, 기준선 뒤 cpppj 변경 없음)
import { RENDER_ENTRY_BYTES } from '../core/raw-render-collection.js';
import { SQUID_HASH_SCALES, SquidHash } from '../core/squid-hash.js';

// 타입 flags1 의 abstract/그림자 비트, flags2 의 walker 비트, 월드 경계 여유, walker/그림자 y 보정(float 상수)이다. (2026-11-13 WB04-E389)
const ABSTRACT = 0x400000;
const SHADOW = 0x40000;
const WALKER = 0x10000;
const PADDING = 1.0;
const WALKER_BIAS = 0.5;
const SHADOW_BIAS = Math.fround(0.1);
// 풀 레이아웃(10.78)과 토큰 공간, 항목 안의 필드 위치이다. (2026-11-13 WB04-E389)
const STRIDE = 50;
const TOKEN_SPACE = 0x100000000;
const X = 0;
const Y = 4;
const TOKEN = 8;
const DEPTH = 12;
const KIND = 14;
const TAIL = 15;

// 슬롯의 비정렬 little endian 필드를 읽는다(요청한 바이트 폭만). (2026-11-13 WB04-E389)
function readField(raw, offset, width = 4) {
    let value = 0;
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// 슬롯의 float32 를 읽는다. (2026-11-13 WB04-E389)
const readFloat = (raw, offset) => new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getFloat32(offset, true);

// 일반 자산/그림자 수집기이다. 모든 입력은 이 객체보다 오래 살아야 한다. 사용: new ObjectCollection(hash, pool, types, shapes, tokenBase). (2026-11-13 WB04-E389)
export class ObjectCollection {
    // 공간 해시, 객체 슬롯 풀, 타입 표, 셰이프(프레임 상자) 표, 토큰 기준이다.
    #hash;
    #pool;
    #types;
    #shapes;
    #tokenBase;

    // 표 길이/토큰 범위를 검증하고 기존 raw 풀과 타입/프레임 저장소를 참조한다. (2026-11-13 WB04-E389)
    constructor(hash, pool, types, shapes, tokenBase = 0) {
        if (types.length === 0 || types.length !== shapes.length) throw new TypeError('Object collection type/shape tables');
        if (tokenBase + pool.capacity * STRIDE > TOKEN_SPACE) throw new RangeError('Object collection token range');
        this.#hash = hash;
        this.#pool = pool;
        this.#types = types;
        this.#shapes = shapes;
        this.#tokenBase = tokenBase >>> 0;
    }

    // 패치 frameCheck 와 물리 범위를 검사하고 프레임 상자를 돌려준다. (2026-11-13 WB04-E389)
    #frame(type, frame) {
        const record = this.#types[type];
        const shape = this.#shapes[type];
        if (!shape.loaded || frame < 0 || frame >= shape.frames.length) throw new RangeError('Object collection physical frame');
        const count = shape.frameCount * ((record.flags1 & (ABSTRACT | SHADOW)) ? 2 : 1);
        if (frame >= count || record.maxHitPoints === -0x22222223 || record.maxHitPoints === -0x32323233) throw new RangeError('Object collection frameCheck');
        return shape.frames[frame];
    }

    // 좌표 + 0.5 의 CRT 절삭 하위 DWORD 를 signed 로 읽어 단계 크기로 나눈 칸(0 방향 절삭)을 현재 단계의 상단 칸으로 제한한다. (2026-11-13 WB04-E389)
    static #cell(coordinate, level) {
        const value = Math.fround(coordinate) + 0.5;
        const integer = Number.isFinite(value) && value >= -(2 ** 63) && value < 2 ** 63 ? BigInt(Math.trunc(value)) : 0n;
        const cell = Math.trunc(Number(BigInt.asIntN(32, integer)) / SQUID_HASH_SCALES[level]) | 0;
        return Math.min(cell, SquidHash.side(level) - 1);
    }

    // 패치의 x 차이는 x87 정밀도에 남고 y 차이는 float 로 저장된다. 닫힌 구간 교차 판정이다. (2026-11-13 WB04-E389)
    static #visible(view, x, y, frame) {
        const dx = x - frame.cellHotspotX;
        const dy = Math.fround(y - frame.cellHotspotY);
        const left = view.left - frame.cellWidth - PADDING;
        const top = view.top - frame.cellHeight - PADDING;
        return left <= dx && dx <= view.right && top <= dy && dy <= view.bottom;
    }

    // 현재 SID 를 본체/그림자 순서로 제출하며 next 는 판정이 끝난 뒤 현재 raw 슬롯에서 다시 읽는다. 반환은 쓴 항목 수이다. (2026-11-13 WB04-E389)
    collect(view, output, limit) {
        const pool = this.#pool;
        const hash = this.#hash;
        const capacity = Math.floor(output.length / RENDER_ENTRY_BYTES);
        const data = new DataView(output.buffer, output.byteOffset, output.byteLength);
        let count = 0;
        // 화면/해시 단계 범위만 방문한다. 다른 칸의 반복 SID 를 전역 중복 제거하지 않는다.
        for (let level = 1; level < 4; level += 1) {
            hash.select(level);
            const side = SquidHash.side(level);
            // 원본 DWORD 한 칸 확장과 각 끝의 독립 클램프를 signed overflow 없이 재현한다.
            const expand = (cell, step) => Math.min(Math.max(((cell + step) | 0), 0), side - 1);
            const left = expand(ObjectCollection.#cell(view.left, level), -1);
            const right = expand(ObjectCollection.#cell(view.right, level), 1);
            const top = expand(ObjectCollection.#cell(view.top, level), -1);
            const bottom = expand(ObjectCollection.#cell(view.bottom, level), 1);
            // y 바깥, x 안쪽 순서이며 빈 칸은 체인 조회를 생략한다.
            for (let y = top; y <= bottom; y += 1) {
                for (let x = left; x <= right; x += 1) {
                    let sid = hash.at(x, y);
                    const chain = new Set();
                    // 같은 체인 안의 순환만 거부한다. 풀 범위/type/frame 은 기록 전에 검사한다.
                    while (sid) {
                        if (chain.has(sid)) throw new Error('Object collection SID cycle');
                        chain.add(sid);
                        const raw = pool.slot(sid);
                        const type = raw[10];
                        if (!type || type >= this.#types.length) throw new RangeError('Object collection nonzero type');
                        const record = this.#types[type];
                        const frame = readField(raw, 36) | 0;
                        const positionX = readFloat(raw, 14);
                        const positionY = readFloat(raw, 18);
                        // 수집되는 레이어만 기록하고 원본 strict limit 은 기록 후 검사한다. 실제 span 넘침은 쓰기 전에 거부한다.
                        const append = (shadow) => {
                            if (count >= capacity) throw new RangeError('Object collection output capacity');
                            const current = pool.slot(sid);
                            const base = count * RENDER_ENTRY_BYTES;
                            let entryY = readFloat(current, 18);
                            if (shadow) entryY = Math.fround(entryY - SHADOW_BIAS);
                            else if (record.flags2 & WALKER) entryY = Math.fround(entryY + WALKER_BIAS);
                            data.setFloat32(base + X, readFloat(current, 14), true);
                            data.setFloat32(base + Y, entryY, true);
                            data.setUint32(base + TOKEN, (this.#tokenBase + sid * STRIDE) >>> 0, true);
                            data.setInt16(base + DEPTH, (current[35] << 24) >> 24, true);
                            output[base + KIND] = 0;
                            output[base + TAIL] = shadow || ((record.flags1 & ABSTRACT) && frame >= this.#shapes[type].frameCount) ? 4 : 0;
                            count += 1;
                            if (count >= limit) throw new RangeError('Object collection count < limit');
                        };
                        if (ObjectCollection.#visible(view, positionX, positionY, this.#frame(type, frame))) append(false);
                        if (record.flags1 & SHADOW) {
                            const shadowFrame = (frame + this.#shapes[type].frameCount) | 0;
                            if (ObjectCollection.#visible(view, positionX, positionY, this.#frame(type, shadowFrame))) append(true);
                        }
                        sid = readField(pool.slot(sid), 4, 2);
                    }
                }
            }
        }
        hash.select(2);
        return count;
    }

    // 수집된 대리 토큰을 같은 풀의 SID 로 되돌린다. 원본 tokenBase/stride 와 슬롯 경계를 검사한다. (2026-11-13 WB04-E389)
    object(token) {
        token >>>= 0;
        if (token < this.#tokenBase) throw new RangeError('Object collection token');
        const offset = token - this.#tokenBase;
        if (offset % STRIDE || !offset || offset / STRIDE >= this.#pool.capacity) throw new RangeError('Object collection token SID');
        return offset / STRIDE;
    }
}

// 공통 일반 자산/효과 슬롯을 실제 수집기에 연결한다. collector 수명은 호출자가 유지해야 한다. (2026-11-13 WB04-E389)
export function bindObjectCollection(hooks, collector) {
    hooks.objects = (view, output, limit) => collector.collect(view, output, limit);
}
