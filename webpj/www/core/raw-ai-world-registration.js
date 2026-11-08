// AI 가 기존 월드와 간헐천을 같은 본체 목록에 등록하는 모듈이다(cpppj `o/RawAiWorldRegistration.cpp/.h`, 10.78: 패치 00415720 초기 전체 스캔·004153c0 간헐천 등록/제거·004152b0 동맹보다 가까운지·00411000 표면 분류).
//  - new RawAiWorldRegistration(memory, pool, types, players, allies, surface, objects, hooks): memory 는 E164 `RawAiBrainMemory`(본체 0xb08 + 목록 블록), pool 은 같은 PATCH SidPool, types 는 현재 188 TYPE 의 `{ flags2 }` 표,
//    players 는 현재 Player 9기록(9×180 BYTE), allies 는 방향별 81 DWORD(owner×9+candidate), surface 는 005c84bc 의 같은 hash0 65536 WORD(Uint16Array), objects 는 같은 풀의 E180 `RawAiObjectNotice`(지연한 방어 자산도 같은 00414de0 으로 통지),
//    hooks = { center(sid)→[x, y](004adea0 현재 신전 발 중심, 자신의 신전이 0 이면 본체 +0x268/+0x26c 를 씀), fartherGeyser(token, sid)(004105b0 "Not closer to geyser" 로그, 후보를 추가하지 않은 뒤 호출), missingSurface(sid)(00411000 좌표/표면 0 진단, 반환하면 원본처럼 SID 0 분류를 계속) } 는 도달한 위치에서만 요구한다.
//  - fullScan(token): 00415720 — SID 2 부터(최초 호출도 1 을 먼저 증가시켜 SID 1 은 방문하지 않음) free 가 아닌 항목만(dead 는 제외하지 않음) 방문해 TYPE 70 이상이고 genus 2 가 아닌 것 중 간헐천(0x10000000)은 geyser(…, 1)로 등록하고,
//    본체 owner 와 같은 owner 의 비방어 자산은 즉시 통지하며 방어 자산(0x4000000)은 임시 300개 목록(1200 바이트 할당, 실패해도 반납)에 지연했다가 마지막에 SID 순서로 통지한 뒤 +0x3c8 을 3 으로 쓴다(+0x25c 는 caller 가 씀).
//  - geyser(token, sid, state): 004153c0 — state 0 이면 좌표/TYPE 를 읽지 않고 다섯 목록(+0x324/+0x330/+0x33c/+0x348/+0x354)의 모든 중복을 제거하고, 그 밖이면 동맹이 더 가까우면 `fartherGeyser` 로그만, 아니면 +0x324 에 추가한 뒤 현재 표면(좌표 y → x 절삭, 0..255 밖은 표면 0)으로 분류한다:
//    자기 소유(또는 owner 0)의 지면(genus 0x1000002 중 2 만)이면 +0x330, 그 밖(섬/기타)은 +0x348 이다.
//  - closerToAlly(token, sid): 004152b0 — Player 0 은 방문하지 않고 현재 owner 행의 방향 동맹과 현재 Player+0x7c 신전만 선택해 동맹 신전 거리가 자신의 거리보다 엄격히 작으면 true 이다. 자신의 거리만 float 로 저장하며 동률이면 자신의 후보를 유지한다.
//  - 목록 추가/제거는 원본 네이티브 0040e9b0(full 이면 포인터를 읽지 않고 생략, 중복 유지)/0040ea00(모든 중복 안정 압축, inactive 꼬리 보존)과 같다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E180: AI 월드 등록 이식으로 추가)
import { AI_BRAIN_BYTES } from './raw-ai-brain.js';

// 00415720 의 실제 0x4b0 임시 목록 용량(300개)과 간헐천 genus/지면 분류 마스크, 정수 변환 상한이다. (2026-11-09 WB04-E180)
const DELAYED_LIMIT = 300;
const TWO_63 = 9223372036854775808;

// 원본 비정렬 DWORD 를 실제 backing 범위에서 읽는다. (2026-11-09 WB04-E180)
function read(bytes, at) {
    if (at < 0 || at > bytes.length || bytes.length - at < 4) throw new RangeError('AI 월드 등록 DWORD 범위');
    return (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
}

// 현재 DWORD 의 네 바이트만 바꾸며 inactive/padding 을 유지한다. (2026-11-09 WB04-E180)
function write(bytes, at, value) {
    if (at < 0 || at > bytes.length || bytes.length - at < 4) throw new RangeError('AI 월드 등록 쓰기 범위');
    for (let index = 0; index < 4; index += 1) bytes[at + index] = (value >>> (index * 8)) & 0xff;
}

// 단정도 float 비트를 수로 바꾼다. (2026-11-09 WB04-E180)
function bitsFloat(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 원본 x87 의 두 float 좌표 차이는 중간 float 저장 없이 배정도로 계산한다. (2026-11-09 WB04-E180)
function distance(first, second) {
    const dx = first[0] - second[0];
    const dy = first[1] - second[1];
    return dx * dx + dy * dy;
}

// 원본 CRT signed64 절삭의 낮은 DWORD(signed)이며 주소 계산이 불가능한 비유한 입력은 호스트 계약 오류이다. (2026-11-09 WB04-E180)
function truncate(value) {
    if (!Number.isFinite(value) || value < -TWO_63 || value >= TWO_63) throw new RangeError('AI 간헐천 CRT 좌표 범위');
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 선택되지 않은 하위를 미리 요구하지 않고 도달한 경계에서만 거부한다. (2026-11-09 WB04-E180)
function need(effect) {
    if (typeof effect !== 'function') throw new Error('AI 월드 등록 실제 하위 미연결');
    return effect;
}

// 월드 등록 처리기이다. 별도 관찰용 목록을 만들지 않는다. (2026-11-09 WB04-E180)
export class RawAiWorldRegistration {
    #memory;
    #pool;
    #types;
    #players;
    #allies;
    #surface;
    #objects;
    #hooks;

    // 같은 Pool/본체/현재 65536 WORD 를 빌린다. (2026-11-09 WB04-E180)
    constructor(memory, pool, types, players, allies, surface, objects, hooks) {
        if (!memory || typeof memory.bytes !== 'function') throw new TypeError('AI 월드 등록 본체 소유자 누락');
        if (pool?.edition !== 'patch1078' || objects?.pool !== pool || !Array.isArray(types) || types.length !== 188 || !(players instanceof Uint8Array) || players.length !== 1620
            || allies?.length !== 81 || !(surface instanceof Uint16Array) || surface.length !== 65536) {
            throw new TypeError('AI 월드 등록의 같은 PATCH188/Player9/동맹81/표면65536 누락');
        }
        this.#memory = memory;
        this.#pool = pool;
        this.#types = types;
        this.#players = players;
        this.#allies = allies;
        this.#surface = surface;
        this.#objects = objects;
        this.#hooks = hooks ?? {};
    }

    // 해제 포인터나 크기 없는 가짜 본체는 성공으로 대신하지 않는다. (2026-11-09 WB04-E180)
    #body(token) {
        const raw = this.#memory.bytes(token);
        if (raw.length !== AI_BRAIN_BYTES) throw new RangeError('AI 월드 등록 실제0xb08 본체 누락');
        return raw;
    }

    // 슬롯 0 은 진단 뒤의 원본 분류에도 사용되므로 임의 금지하지 않으며 풀 밖은 거부한다. (2026-11-09 WB04-E180)
    #object(sid) {
        if (sid >= this.#pool.capacity || sid > 65535) throw new RangeError('AI 월드 등록 SID 물리 범위');
        return sid;
    }

    // free/dead 판정은 각 caller 의 원문 위치에만 두며 현재 TYPE 의 순수 flags2 이다. (2026-11-09 WB04-E180)
    #genus(sid) {
        const type = this.#pool.slot(sid)[10];
        if (type >= this.#types.length) throw new RangeError('AI 월드 등록 현재 TYPE 범위');
        return this.#types[type].flags2 >>> 0;
    }

    // 비양수 count 의 자연 루프를 유지한다. (2026-11-09 WB04-E180)
    #count(token, at) { return read(this.#body(token), at + 8) | 0; }

    // 용량이 남을 때만 실제 현재 포인터로 쓰며 full 이면 포인터를 읽지 않고 반환한다. (2026-11-09 WB04-E180)
    #append(token, at, sid) {
        const raw = this.#body(token);
        const count = this.#count(token, at);
        if (count >= (read(raw, at + 4) | 0)) return;
        if (count < 0) throw new RangeError('AI 월드 등록 음수 목록 인덱스');
        write(this.#memory.bytes(read(raw, at)), count * 4, sid);
        write(raw, at + 8, count + 1);
    }

    // count 밖의 꼬리나 여유 메모리를 0 으로 정리하지 않으며 현재 count 와 포인터를 원본처럼 재조회한다. (2026-11-09 WB04-E180)
    #remove(token, at, sid) {
        let removed = 0;
        for (let index = 0; index < this.#count(token, at); index += 1) {
            const list = this.#memory.bytes(read(this.#body(token), at));
            const value = read(list, index * 4);
            if (value === sid) removed += 1;
            else write(list, (index - removed) * 4, value);
        }
        write(this.#body(token), at + 8, (this.#count(token, at) - removed) >>> 0);
    }

    // 004152b0: 자신과 동맹의 현재 신전이 같은 거리이면 자신의 후보를 유지한다. 자신의 거리만 float 로 저장한다(00415312 의 fstp DWORD). (2026-11-09 WB04-E180)
    closerToAlly(token, number) {
        const sid = this.#object(number);
        const object = this.#pool.slot(sid);
        const point = [bitsFloat(read(object, 14)), bitsFloat(read(object, 18))];
        const temple = read(this.#body(token), 0x294);
        const own = temple
            ? need(this.#hooks.center)(this.#object(temple)).map(Math.fround)
            : [bitsFloat(read(this.#body(token), 0x268)), bitsFloat(read(this.#body(token), 0x26c))];
        const ownDistance = Math.fround(distance(point, own));
        // Player 0 은 방문하지 않고 현재 owner 행의 방향 동맹과 현재 Player+0x7c 만 선택한다.
        for (let candidate = 1; candidate < 9; candidate += 1) {
            const owner = read(this.#body(token), 0x260);
            if (owner >= 9) throw new RangeError('AI 간헐천 현재 동맹 owner 범위');
            if (candidate === owner || !this.#allies[owner * 9 + candidate]) continue;
            const other = read(this.#players, candidate * 180 + 0x7c);
            if (other && distance(point, need(this.#hooks.center)(this.#object(other)).map(Math.fround)) < ownDistance) return true;
        }
        return false;
    }

    // 00411000: 표면 0 진단 뒤 원본처럼 현재 0 슬롯을 분류한다(자기 소유 지면만 +0x330, 섬/기타는 +0x348). (2026-11-09 WB04-E180)
    #classifySurface(token, number) {
        const object = this.#pool.slot(this.#object(number));
        const x = truncate(bitsFloat(read(object, 14)));
        const y = truncate(bitsFloat(read(object, 18)));
        let surface = 0;
        if (x >= 0 && y >= 0 && x < 256 && y < 256) surface = this.#surface[y * 256 + x];
        if (!surface) need(this.#hooks.missingSurface)(number);
        const sid = this.#object(surface);
        if ((this.#genus(sid) & 0x1000002) === 2) {
            const owner = this.#pool.slot(sid)[34];
            if (!owner || owner === read(this.#body(token), 0x260)) this.#append(token, 0x330, number);
        } else this.#append(token, 0x348, number);
    }

    // 004153c0: 제거는 좌표/TYPE 를 읽지 않고, 등록은 가까움 판정 뒤 현재 표면을 읽는다. (2026-11-09 WB04-E180)
    geyser(token, sid, state) {
        this.#body(token);
        if (!state) {
            // 원본 다섯 목록의 순서대로 모든 중복을 제거한다.
            for (const at of [0x324, 0x330, 0x33c, 0x348, 0x354]) this.#remove(token, at, sid);
            return;
        }
        if (this.closerToAlly(token, sid)) need(this.#hooks.fartherGeyser)(token, sid);
        else {
            this.#append(token, 0x324, sid);
            this.#classifySurface(token, sid);
        }
    }

    // 00415720: 기존 수도·시설·수집자를 먼저 등록한 뒤 방어 자산을 한 번씩 지연 통지한다. (2026-11-09 WB04-E180)
    fullScan(token) {
        this.#body(token);
        const temporary = this.#memory.allocate(DELAYED_LIMIT * 4);
        let count = 0;
        const end = this.#pool.capacity;
        // 0040eb40 의 최초 호출도 1 을 먼저 증가시키므로 SID 1 은 방문하지 않는다.
        let cursor = 1;
        // 원본의 free bit 만 건너뛴다(dead 는 제외하지 않음).
        const next = () => {
            cursor += 1;
            while (cursor < end && (this.#pool.slot(this.#object(cursor))[11] & 1)) cursor += 1;
        };
        try {
            next();
            // 하위 로그 뒤 현재 pool/owner/type 을 다시 읽되 iterator 끝은 최초 cap 을 유지한다.
            while (cursor >= 1 && cursor < this.#pool.capacity) {
                const sid = this.#object(cursor);
                if (this.#pool.slot(sid)[10] >= 70 && !(this.#genus(sid) & 2)) {
                    const genus = this.#genus(sid);
                    if (genus & 0x10000000) this.geyser(token, cursor, 1);
                    else if (this.#pool.slot(sid)[34] === read(this.#body(token), 0x260)) {
                        if (!(genus & 0x4000000)) this.#objects.update(token, sid, true);
                        else if (count < DELAYED_LIMIT) {
                            write(this.#memory.bytes(temporary), count * 4, cursor);
                            count += 1;
                        }
                    }
                }
                next();
            }
            // 방어용 300개 제한을 넘은 항목은 통지하지 않으며 SID 순서를 유지한다.
            for (let index = 0; index < count; index += 1) this.#objects.update(token, this.#object(read(this.#memory.bytes(temporary), index * 4)), true);
            write(this.#body(token), 0x3c8, 3);
        } finally {
            this.#memory.free(temporary);
        }
    }
}
