// 상태 주문(117/118)의 대상 목록·기한과 만료 처리(420a40/41e100)를 옮긴 모듈이다(cpppj `o/RawBombEffectTimer.cpp/.h`, 10.78 전용).
//  - BombEffectTimerFields: 원본 +0c/+14 목록(members), +18 double 기한(endTime / 정확한 비트 endBits), +20 flags(1 은닉 효과 extra 0x10, 2 마비 효과 extra 0x20), +10 고정 한도(capacity 100)의 소유 자료이다.
//    기한은 NaN 페이로드까지 보존하려고 endBits(BigInt)를 같이 둔다(endTime 을 대입하면 비트는 그 수에서 다시 만든다).
//  - RawBombEffectTimer: constructEmpty(초기 flags)은 빈 100칸·기한 0, construct(delay, members, flags)는 `delay(signed) + 현재 시각` 기한과 최대 100개 목록 복사,
//    frame(fields, finish)는 ① 현재 목록의 live 대상(상태 7 비트 없음)마다 unpop(0) → repop(0x40) ② 기한이 지났으면(`기한 <= 지금` 이 거짓이면 종료) 목록을 다시 읽으며 은닉(flags 1·extra 0x10)/마비(flags 2·extra 0x20)를 풀고
//    표시 갱신(마비 대상이 genus 0x200000 이 아니면 refresh, 맞으면 unpop/repop, 현재 TYPE 136 이면 fence) ③ finish() 순서이다. 효과 뒤 현재 목록/기한/상태/TYPE 을 원본 위치에서 다시 읽는다.
//  - BombEffectTimerProcess(SquidProcess): Kernel 슬롯 프로세스이다. runFrame 은 frame 을 부르고 finish 에서 부착한 호스트로 자기 자신을 kill(0) 한다. 통지(notify)는 빈 함수이다.
// 훅 { unpop, repop, refresh, fence } 는 도달한 지점에서만 요구한다(없으면 TypeError). now 는 호출마다 현재 게임 시각을 돌려주는 함수이다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E52: RawBombEffectTimer 이식으로 추가)
import { readBombTimerPacket, writeBombTimerPacket } from './raw-bomb-timer-packet.js';
import { SquidProcess } from './squid-process.js';

// 슬롯 필드 위치와 개수, 효과 비트이다. (2026-11-08 WB04-E52)
const STATE_OFFSET = 11;
const EXTRA_OFFSET = 40;
const TYPE_OFFSET = 10;
const TYPE_COUNT = 188;
const LIST_CAPACITY = 100;
const INACTIVE_MASK = 7;
const HIDE_EXTRA = 0x10;
const PARALYZE_EXTRA = 0x20;
const PRIEST_GENUS = 0x200000;
const FENCE_TYPE = 136;
const REPOP_FLAGS = 0x40;

// double 과 그 비트(BigInt) 사이의 변환 버퍼이다. (2026-11-08 WB04-E52)
const doubleView = new DataView(new ArrayBuffer(8));

// 선택된 큰 호출만 요구하고 누락은 명시 오류로 남긴다. (2026-11-08 WB04-E52)
function need(hook) {
    if (typeof hook !== 'function') throw new TypeError('상태 효과 타이머의 실제 하위 미연결');
    return hook;
}

// 원본 +0c/+14 목록과 +18 기한·+20 flags 를 소유하는 자료이다. 사용: const fields = new BombEffectTimerFields(); fields.endBits = 0x7ff8000000001234n. (2026-11-08 WB04-E52)
export class BombEffectTimerFields {
    // 순서와 중복을 보존하는 원본 DWORD SID 목록이다.
    members = [];
    // 값 1 은닉(extra 0x10), 값 2 마비(extra 0x20) 효과 비트이다(상위 비트도 보존).
    flags = 0;
    // 원본 +10 의 고정 저장 한도이다.
    capacity = LIST_CAPACITY;
    // 기한 double 과 그 정확한 비트(미설정이면 null)이다.
    #time = 0;
    #bits = null;

    // 현재 기한 수치이다. (2026-11-08 WB04-E52)
    get endTime() {
        return this.#time;
    }

    // 기한 수치를 바꾸면 이전 비트 기록은 버린다. (2026-11-08 WB04-E52)
    set endTime(value) {
        this.#time = value;
        this.#bits = null;
    }

    // 기한의 정확한 64비트 표현(BigInt)이다. 비트를 설정한 적이 없으면 수치에서 만든다. (2026-11-08 WB04-E52)
    get endBits() {
        if (this.#bits !== null) return this.#bits;
        doubleView.setFloat64(0, this.#time, true);
        return doubleView.getBigUint64(0, true);
    }

    // 정확한 비트로 기한을 정한다(NaN 페이로드 보존). (2026-11-08 WB04-E52)
    set endBits(bits) {
        this.#bits = BigInt.asUintN(64, BigInt(bits));
        doubleView.setBigUint64(0, this.#bits, true);
        this.#time = doubleView.getFloat64(0, true);
    }
}

// 420a40/41e100 의 전체 Frame 몸체이다. 사용: new RawBombEffectTimer(pool, types, () => now, { unpop, repop, refresh, fence }). (2026-11-08 WB04-E52)
export class RawBombEffectTimer {
    #pool;
    #types;
    #now;
    #hooks;

    // 같은 PATCH 풀/188TYPE/게임 시계를 빌린다. (2026-11-08 WB04-E52)
    constructor(pool, types, now, hooks) {
        if (pool?.edition !== 'patch1078' || types?.length !== TYPE_COUNT || typeof now !== 'function') throw new TypeError('상태 효과 타이머 PATCH 풀/188TYPE/시계 필요');
        this.#pool = pool;
        this.#types = types;
        this.#now = now;
        this.#hooks = hooks ?? {};
    }

    // 00420960 의 빈 100칸/기한 0 이다. 쓰지 않는 +20 flags 는 호출자 heap 입력으로 보존한다(실제 생성 입력은 0). (2026-11-08 WB04-E52)
    constructEmpty(initialFlags = 0) {
        const fields = new BombEffectTimerFields();
        fields.flags = initialFlags >>> 0;
        return fields;
    }

    // signed 지연을 현재 double 시각에 더하고 목록을 최대 100개까지 복사한다(중복·비활성 대상을 미리 거르지 않음). (2026-11-08 WB04-E52)
    construct(delay, members, flags) {
        const fields = new BombEffectTimerFields();
        fields.endTime = (delay | 0) + this.#now();
        fields.members = Array.from(members).slice(0, LIST_CAPACITY);
        fields.flags = flags >>> 0;
        return fields;
    }

    // 손상된 목록을 WORD 로 축소하지 않고 물리 SID 범위를 확인한다. (2026-11-08 WB04-E52)
    #target(member) {
        if (member >= this.#pool.capacity || member > 65535) throw new RangeError('상태 효과 목록의 물리 SID 범위');
        return member;
    }

    // 갱신 → 현재 기한 → 효과 해제 → 현재 가상 갱신 → 종료 순서이다. 첫 순회와 만료 순회는 독립적으로 목록을 읽으며 콜백이 바꾼 기한/flags/대상 상태를 보존한다. (2026-11-08 WB04-E52)
    frame(fields, finish) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        // 첫 갱신은 만료 여부와 관계없이 모든 현재 live 대상을 방문한다.
        for (let index = 0; index < fields.members.length; index += 1) {
            const target = this.#target(fields.members[index]);
            if (pool.slot(target)[STATE_OFFSET] & INACTIVE_MASK) continue;
            need(hooks.unpop)(target, 0);
            need(hooks.repop)(target, REPOP_FLAGS);
        }
        if (!(fields.endTime <= this.#now())) return;
        // 만료 뒤 목록을 다시 읽으며 같은 대상을 중복 방문할 수 있다.
        for (let index = 0; index < fields.members.length; index += 1) {
            const target = this.#target(fields.members[index]);
            if (pool.slot(target)[STATE_OFFSET] & INACTIVE_MASK) continue;
            if ((fields.flags & 1) && (pool.slot(target)[EXTRA_OFFSET] & HIDE_EXTRA)) {
                // 검증한 물리 슬롯에 원본처럼 직접 쓰며 콜백 뒤 상태를 재필터하지 않는다.
                pool.physical(target)[EXTRA_OFFSET] &= ~HIDE_EXTRA & 0xff;
                need(hooks.refresh)(target);
            }
            if ((fields.flags & 2) && (pool.slot(target)[EXTRA_OFFSET] & PARALYZE_EXTRA)) {
                pool.physical(target)[EXTRA_OFFSET] &= ~PARALYZE_EXTRA & 0xff;
                // 첫 refresh 뒤 바뀐 현재 종류를 읽는다.
                const type = pool.slot(target)[TYPE_OFFSET];
                if (type >= this.#types.length) throw new RangeError('상태 효과 만료의 현재 TYPE 범위');
                if (!(this.#types[type].flags2 & PRIEST_GENUS)) {
                    need(hooks.refresh)(target);
                } else {
                    need(hooks.unpop)(target, 0);
                    need(hooks.repop)(target, REPOP_FLAGS);
                }
                if (pool.slot(target)[TYPE_OFFSET] === FENCE_TYPE) need(hooks.fence)(target);
            }
        }
        need(finish)();
    }
}

// Kernel 57 의 실제 현재 필드와 ProcessForm 수명을 함께 소유하는 프로세스이다. (2026-11-08 WB04-E52)
export class BombEffectTimerProcess extends SquidProcess {
    #logic;
    #fields;

    // 00420ad0/00420960 의 Init 없는 cdecl 생성이다. 현재 시각/부모를 읽거나 호스트에 부착하지 않는다. 사용: new BombEffectTimerProcess(pool, types, now, hooks, initialFlags). (2026-11-08 WB04-E52)
    constructor(pool, types, now, hooks, initialFlags = 0) {
        super();
        this.#logic = new RawBombEffectTimer(pool, types, now, hooks);
        this.#fields = this.#logic.constructEmpty(initialFlags);
    }

    // 단일 목록/기한을 생성하는 두 번째 생성자 대신 쓰는 정적 팩토리이다(원본 420a40). (2026-11-08 WB04-E52)
    static withTimer(pool, types, now, delay, members, flags, hooks) {
        const process = new BombEffectTimerProcess(pool, types, now, hooks);
        process.#fields = process.#logic.construct(delay, members, flags);
        return process;
    }

    // 기존 호스트의 실제 삭제에 원본 Frame 을 연결한다. 삭제 뒤 필드나 this 를 다시 읽지 않는다. (2026-11-08 WB04-E52)
    runFrame() {
        this.#logic.frame(this.#fields, () => {
            if (!this.host) throw new Error('상태 효과 타이머 호스트 미부착');
            this.host.kill(this, 0);
        });
    }

    // VT 50330c+1c 의 480260 은 ret 12 인 빈 통지이다. (2026-11-08 WB04-E52)
    notify() {}

    // 선택 표시가 같은 현재 목록·기한·flags 를 읽는다. (2026-11-08 WB04-E52)
    get fields() {
        return this.#fields;
    }

    // 41e230 을 현재 유일한 목록에 누적 적용한다. (2026-11-08 WB04-E52)
    readPacket(packet) {
        readBombTimerPacket(this.#fields, packet);
    }

    // 현재 호스트 Form/부모를 실제 41e280 에 전달하고 반환 길이를 돌려준다. (2026-11-08 WB04-E52)
    writePacket(packet, declaredLength) {
        return writeBombTimerPacket(this.#fields, this.form, this.parent, packet, declaredLength);
    }
}
