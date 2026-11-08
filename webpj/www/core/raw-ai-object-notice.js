// AI 의 생성/삭제 통지를 같은 Player→AI 본체/목록에 반영하는 모듈이다(cpppj `o/RawAiObjectNotice.cpp/.h`, 10.78: 패치 004166e0 Player 통지 바깥 몸체·00414de0 타입 수·목록·목표·타이머 갱신·00410f80 간헐천 제외·00410620 건설 통지).
//  - new RawAiObjectNotice(memory, pool, types, players, now, outpostType, disabled, hooks): memory 는 E164 `RawAiBrainMemory`(본체 0xb08 + 17개 목록 블록), pool 은 같은 PATCH SidPool, types 는 현재 188 TYPE 의 `{ flags2 }` 표,
//    players 는 현재 Player 9기록(9×180 BYTE, +0xb0 이 AI 본체 토큰), now()=0055b4d0 현재 게임 시각, outpostType()=00541250 현재 분류 제외 번호, disabled()=00594fc8 현재 모드(Player 조회 전에 읽음),
//    hooks = { carrierTargets(sid)→[TargetA, TargetB](00426150 현재 이동 프로세스 두 대상, 없으면 0/0), distanceSquared(sid, other)→거리 제곱(00411120 두 현재 발 중심 사이), log(token, { kind, object, type, x, y })(004105b0 표시 경계, 게임 타이머는 본문이 갱신) } 는 도달한 위치에서만 요구한다.
//    로그 종류는 `AI_OBJECT_LOG { built, madeGeyserIneligible, cannotExcludeGeyser }` 이다.
//  - notice(owner, sid, added): 004166e0 — disabled 이거나 owner 가 signed 비양수이면 아무것도 읽지 않고 끝내고, owner 가 9 이상이면 거부하며, 현재 Player+0xb0 이 0 이 아닐 때만 update 를 호출한다(호출마다 현재 포인터를 다시 읽음).
//  - update(token, sid, added): 00414de0 — 자산 TYPE 별 개수(+0x6d4 + TYPE×4)를 갱신하고 TYPE genus 를 한 번 읽어 삭제 시 수집/신전 포인터와 추적/미움 목록 정리·간헐천 제외·잃은 수집자 30초 지연(+0x5e0)을 처리하고, 다이스(+0x2a4)·방어(+0x360, 근접 거리 180.5 미만 전체 방문 뒤 추가)·수집자(+0x2dc, 건설 통지)·워크샵(+0x2f4)·
//    시설(+0x300/+0x30c/+0x318, 삭제 시 상태 초기화와 대기 목록 재예약)·공략 목록(+0x2e8, 용량이 차면 첫 항목 제거 뒤 추가, +0x3c8 에 3 가산)·타이머(+0x5b8/+0x5c0)를 원본 순서대로 갱신한다. 각 하위 효과 뒤에는 현재 본체/타입/목록을 다시 읽는다.
//  - 목록은 원본 네이티브 0040e9b0(추가, full 이면 포인터를 읽지 않고 생략)/00414450(unique, full 이면 검색도 생략)/0040ea00(모든 중복 안정 압축, inactive 꼬리 보존)과 같다. 타이머는 float 로 축소하지 않고 QWORD 로 쓴다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E180: AI 생성/삭제 통지 이식으로 추가)
import { AI_BRAIN_BYTES } from './raw-ai-brain.js';

// 방어 근접 거리 제곱(00501700)과 잃은 수집자 지연 초(00501708), 로그 종류, 정수 변환 상한이다. (2026-11-09 WB04-E180)
const GUARD_DISTANCE_SQUARED = 180.5;
const LOST_CARRIER_DELAY = 30.0;
const TWO_63 = 9223372036854775808;
export const AI_OBJECT_LOG = Object.freeze({ built: 0, madeGeyserIneligible: 1, cannotExcludeGeyser: 2 });

// 원본 비정렬 DWORD 를 현재 바이트에서 읽는다. (2026-11-09 WB04-E180)
function read(bytes, at) {
    if (at < 0 || at > bytes.length || bytes.length - at < 4) throw new RangeError('AI 통지 DWORD 물리 범위');
    return (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
}

// 주변 BYTE 를 보존하고 지정 DWORD 만 쓴다. (2026-11-09 WB04-E180)
function write(bytes, at, value) {
    if (at < 0 || at > bytes.length || bytes.length - at < 4) throw new RangeError('AI 통지 DWORD 쓰기 물리 범위');
    for (let index = 0; index < 4; index += 1) bytes[at + index] = (value >>> (8 * index)) & 0xff;
}

// 목록 비교의 signed DWORD 를 보존한다. (2026-11-09 WB04-E180)
const signedAt = (bytes, at) => read(bytes, at) | 0;

// 타이머는 float 로 축소하지 않고 같은 QWORD(낮은 DWORD 먼저)에 쓴다. (2026-11-09 WB04-E180)
function writeTime(bytes, at, value) {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    write(bytes, at, view.getUint32(0, true));
    write(bytes, at + 4, view.getUint32(4, true));
}

// 로그 좌표의 CRT signed64 절삭 후 낮은 DWORD(signed)이며 비유한/범위 밖은 호스트 계약 오류이다. (2026-11-09 WB04-E180)
function truncate(value) {
    if (!Number.isFinite(value) || value < -TWO_63 || value >= TWO_63) throw new RangeError('AI 통지 로그 좌표 변환 범위');
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 단정도 float 비트를 수로 바꾼다. (2026-11-09 WB04-E180)
function bitsFloat(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 도달한 실제 하위가 없으면 그 위치에서 명시 오류로 남긴다. (2026-11-09 WB04-E180)
function need(effect) {
    if (typeof effect !== 'function') throw new Error('AI 생성/삭제 통지의 실제 하위 미연결');
    return effect;
}

// 생성/삭제 통지 처리기이다. 생성은 실제 효과를 일으키지 않는다. (2026-11-09 WB04-E180)
export class RawAiObjectNotice {
    #memory;
    #pool;
    #types;
    #players;
    #now;
    #outpostType;
    #disabled;
    #hooks;

    // 원본의 실제 저장소와 현재 조회기만 빌린다. (2026-11-09 WB04-E180)
    constructor(memory, pool, types, players, now, outpostType, disabled, hooks) {
        if (!memory || typeof memory.bytes !== 'function') throw new TypeError('AI 통지 본체 소유자 누락');
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== 188 || !(players instanceof Uint8Array) || players.length !== 1620 || typeof disabled !== 'function') {
            throw new TypeError('AI 통지의 실제 PATCH188/Player9/현재 모드 누락');
        }
        if (typeof now !== 'function' || typeof outpostType !== 'function') throw new TypeError('AI 통지 현재 시각/제외 TYPE 조회기 누락');
        this.#memory = memory;
        this.#pool = pool;
        this.#types = types;
        this.#players = players;
        this.#now = now;
        this.#outpostType = outpostType;
        this.#disabled = disabled;
        this.#hooks = hooks ?? {};
    }

    // 공통 Pop/삭제와 같은 풀인지 확인하기 위한 풀이다. (2026-11-09 WB04-E180)
    get pool() { return this.#pool; }

    // 실제 본체의 수명/크기를 검사한다. (2026-11-09 WB04-E180)
    #body(token) {
        const bytes = this.#memory.bytes(token);
        if (bytes.length !== AI_BRAIN_BYTES) throw new RangeError('AI 통지의 실제0xb08 본체 누락');
        return bytes;
    }

    // signed count 는 원본의 비양수 자연 루프를 보존한다. (2026-11-09 WB04-E180)
    #count(token, offset) { return signedAt(this.#body(token), offset + 8); }

    // 논리 count 와 별도로 현재 실제 할당 안의 인덱스를 확인한다. (2026-11-09 WB04-E180)
    #item(token, offset, index) {
        if (index < 0) throw new RangeError('AI 통지 음수 목록 인덱스');
        return read(this.#memory.bytes(read(this.#body(token), offset)), index * 4);
    }

    // 실제 목록의 첫 일치에서만 검색을 끝낸다(외부에서 공유하는 count 와 포인터를 매 방문 다시 읽음). (2026-11-09 WB04-E180)
    #contains(token, offset, value) {
        for (let index = 0; index < this.#count(token, offset); index += 1) if (this.#item(token, offset, index) === value) return true;
        return false;
    }

    // 일반 추가는 중복을 허용하고 용량이 찼으면 포인터를 조회하지 않는다. (2026-11-09 WB04-E180)
    #append(token, offset, value) {
        const raw = this.#body(token);
        const count = this.#count(token, offset);
        if (count >= signedAt(raw, offset + 4)) return;
        if (count < 0) throw new RangeError('AI 통지 음수 추가 인덱스');
        write(this.#memory.bytes(read(raw, offset)), count * 4, value);
        write(raw, offset + 8, count + 1);
    }

    // full 판정은 중복 검색보다 먼저 수행한다. (2026-11-09 WB04-E180)
    #appendUnique(token, offset, value) {
        if (this.#count(token, offset) >= signedAt(this.#body(token), offset + 4) || this.#contains(token, offset, value)) return;
        this.#append(token, offset, value);
    }

    // 모든 중복만 압축하며 count 밖의 이전 꼬리를 0 으로 지우지 않는다(생존 항목의 상대 순서와 현재 count 재조회 유지). (2026-11-09 WB04-E180)
    #remove(token, offset, value) {
        let removed = 0;
        for (let index = 0; index < this.#count(token, offset); index += 1) {
            const item = this.#item(token, offset, index);
            if (item === value) removed += 1;
            else write(this.#memory.bytes(read(this.#body(token), offset)), (index - removed) * 4, item);
        }
        write(this.#body(token), offset + 8, (this.#count(token, offset) - removed) >>> 0);
    }

    // 00410f80: 제외 능력(+0x248 의 bit 3)이 없으면 어떤 로그/목록도 선택하지 않는다. (2026-11-09 WB04-E180)
    #excludeGeyser(token, geyser) {
        if (!(read(this.#body(token), 0x248) & 8)) return;
        if (this.#count(token, 0x330) > 0) {
            this.#append(token, 0x33c, geyser);
            this.#remove(token, 0x330, geyser);
            need(this.#hooks.log)(token, { kind: AI_OBJECT_LOG.madeGeyserIneligible, object: geyser, type: 0, x: 0, y: 0 });
        } else {
            need(this.#hooks.log)(token, { kind: AI_OBJECT_LOG.cannotExcludeGeyser, object: geyser, type: 0, x: 0, y: 0 });
        }
    }

    // 00410620: fadd QWORD now 뒤 QWORD 에 저장한다(디컴파일의 float now 표기를 따르지 않음). 로그 좌표는 y → x 절삭이다. (2026-11-09 WB04-E180)
    #built(token, sid) {
        const object = this.#pool.slot(sid);
        const x = bitsFloat(read(object, 14));
        const y = bitsFloat(read(object, 18));
        const iy = truncate(y);
        const ix = truncate(x);
        const type = this.#pool.slot(sid)[10];
        const raw = this.#body(token);
        const rate = bitsFloat(read(raw, 0x228));
        writeTime(raw, 0x5a8, rate + this.#now());
        writeTime(raw, 0x5b0, (rate + 1.0) + this.#now());
        need(this.#hooks.log)(token, { kind: AI_OBJECT_LOG.built, object: sid, type, x: ix, y: iy });
    }

    // 004166e0: 현재 모드/Player 포인터를 읽는 바깥 원본 몸체이다. (2026-11-09 WB04-E180)
    notice(owner, sid, added) {
        if (this.#disabled() || ((owner >>> 0) | 0) < 1) return;
        if (owner >= 9) throw new RangeError('AI 통지 Player9 범위');
        const token = read(this.#players, owner * 180 + 0xb0);
        if (token) this.update(token, sid, added);
    }

    // 00414de0: 본문은 원본 genus 한 번 읽기를 보존하고 각 하위 효과 뒤의 실제 본체/타입/목록을 다시 읽는다. (2026-11-09 WB04-E180)
    update(token, sid, added) {
        let raw = this.#body(token);
        const type = this.#pool.slot(sid)[10];
        const counter = 0x6d4 + type * 4;
        write(raw, counter, (read(raw, counter) + (added ? 1 : 0xffffffff)) >>> 0);
        if (type >= this.#types.length) throw new RangeError('AI 통지 현재 TYPE 범위');
        const genus = this.#types[type].flags2 >>> 0;
        if (!added) {
            if (genus & 0x200000) {
                if (read(raw, 0x2a0) === sid) write(raw, 0x2a0, 0);
                if (read(raw, 0x29c) === sid) write(raw, 0x29c, 0);
            }
            if (genus & 0x30000) {
                const targets = need(this.#hooks.carrierTargets)(sid);
                if (targets[0]) this.#excludeGeyser(token, targets[0]);
                if (this.#contains(token, 0x384, sid)) {
                    this.#remove(token, 0x384, sid);
                    writeTime(this.#body(token), 0x5e0, this.#now() + LOST_CARRIER_DELAY);
                }
            }
            raw = this.#body(token);
            if ((genus & 0x200) && read(raw, 0x294) === sid) write(raw, 0x294, 0);
            this.#remove(token, 0x36c, sid);
            this.#remove(token, 0x378, sid);
            raw = this.#body(token);
            if (read(raw, 0x3f8) === sid) write(raw, 0x3f8, 0);
            if (read(raw, 0x3fc) === sid) write(raw, 0x3fc, 0);
        }
        if (genus & 0x400000) {
            raw = this.#body(token);
            if (added) write(raw, 0x2a4, sid);
            else if (read(raw, 0x2a4) === sid) write(raw, 0x2a4, 0);
        }
        if (genus & 0x4000000) {
            if (!added) this.#remove(token, 0x360, sid);
            else {
                let nearby = false;
                const temple = read(this.#body(token), 0x294);
                if (temple && need(this.#hooks.distanceSquared)(sid, temple) < GUARD_DISTANCE_SQUARED) nearby = true;
                // 첫 근접 판정 뒤에도 원본처럼 현재 목록의 모든 거리 조회를 진행한다.
                for (let index = 0; index < this.#count(token, 0x300); index += 1) {
                    if (need(this.#hooks.distanceSquared)(sid, this.#item(token, 0x300, index)) < GUARD_DISTANCE_SQUARED) nearby = true;
                }
                if (nearby) this.#appendUnique(token, 0x360, sid);
            }
        }
        if ((genus & 0x30000) && !(genus & 0x200000)) {
            if (added) this.#append(token, 0x2dc, sid);
            else this.#remove(token, 0x2dc, sid);
            this.#built(token, sid);
        }
        if (genus & 0x800) {
            if (added) this.#append(token, 0x2f4, sid);
            else this.#remove(token, 0x2f4, sid);
            writeTime(this.#body(token), 0x5b8, 0.0);
        }
        if ((genus & 0x4000) && this.#pool.slot(sid)[10] !== (this.#outpostType() >>> 0)) {
            if (added) this.#append(token, 0x300, sid);
            else this.#remove(token, 0x300, sid);
            if (!added) {
                this.#remove(token, 0x30c, sid);
                raw = this.#body(token);
                write(raw, 0x3e0, 0);
                write(raw, 0x3e4, 6);
                write(raw, 0x3f0, 1);
                write(raw, 0x3f4, 0);
                write(raw, 0x3fc, 0);
                write(raw, 0x400, 0);
                write(raw, 0x3ec, 0);
                this.#remove(token, 0x318, sid);
                // 남은 원소 시설을 원래 순서로 방문하며 대기 목록의 중복만 막는다.
                for (let index = 0; index < this.#count(token, 0x300); index += 1) {
                    const item = this.#item(token, 0x300, index);
                    if (!this.#contains(token, 0x30c, item)) this.#append(token, 0x30c, item);
                }
            } else this.#append(token, 0x30c, sid);
            write(this.#body(token), 0x2c8, signedAt(this.#body(token), 0x3b0) <= this.#count(token, 0x300) ? 1 : 0);
        }
        if (genus & 0x40800) {
            if (!added) {
                if (this.#contains(token, 0x2e8, sid)) this.#remove(token, 0x2e8, sid);
                return;
            }
            if (signedAt(this.#body(token), 0x2ec) <= this.#count(token, 0x2e8)) this.#remove(token, 0x2e8, this.#item(token, 0x2e8, 0));
            this.#append(token, 0x2e8, sid);
            raw = this.#body(token);
            write(raw, 0x3c8, (read(raw, 0x3c8) + 3) >>> 0);
        }
        if (added && (genus & 0x4200)) writeTime(this.#body(token), 0x5c0, 0.0);
    }
}
