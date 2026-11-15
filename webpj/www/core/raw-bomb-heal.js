// 힐 주문의 대상 가상 함수 두 개(원문 41ebb0 자격·41ebd0 현재 대상 상태 복원)와 힐/그라비테이션 버스트 정기(41f9d0·41f760)를 같은 raw 풀에 적용하는 모듈이다(cpppj `o/RawBombHeal.cpp/.h`, 10.78 전용).
// 사용:
//   const body = new RawBombHeal(pool, types, hooks);
//     pool = SidPool(10.78), types = 188칸 타입 표(`flags2`·`footX/footY`), hooks = { hasHitPoints(sid) → bool, maximumHitPoints(sid) → int32, fenceType() → DWORD, burstType() → DWORD,
//       fence(sid), unpop(sid, 가상표, 0), repop(sid, 가상표, 0x40), refresh(sid, 가상표), create(타입, 2) → sid, pop(sid, 가상표, x, y, 0) } (도달한 효과만 요구한다).
//   body.mayTarget(target) → 대상의 현재 genus & 0x81574204 를 0/1 로 바꾸지 않고 그대로 돌려준다(HP·소유자·동맹·free/dead 관문 없음).
//   body.apply(parent, target) → 운반체 대상이면 끝. HP 가 있으면 최초 HP 와 최대 HP 의 DWORD 합을 signed 로 0 쪽 절삭 평균해 쓰고, 은신/마비 두 비트(0x30)만 해제하며, 펜스면 재연결,
//     보행 genus(0x200000)는 Unpop→Repop(0x40), 아니면 표시 갱신을 하고, 버스트 금지 genus(0x1000004)가 아니면 현재 버스트 객체를 만들어 대상 발자국 중심에 놓는다.
//   handleHealBurstRegular(pool, frames, setter, host, refresh, destroy, sid, count, payload) → 41f9d0: signed count 가 현재 'A' 구간 길이−1 보다 작으면 현재 프레임+1 을 지정하고 payload 를 돌려준다.
//     아니면 실제 일반 Regular(이벤트 2, 0.05)를 붙인 뒤 destroy(sid, 0x100048)로 삭제하고 −1 을 돌려준다.
//   handleGravityBurstRegular(…같은 인자) → 41f760: 위와 같되 count 가 한계 이상이면 현재 프레임이 0 일 때만 종료하고, 아니면 32비트 감김의 역방향 프레임(total+total−count−2)을 지정한다.
// 선택한 효과가 빠졌으면 앞선 변경을 되돌리지 않고 명시 오류로 알린다. 원본 관측: cpppj `bomb-gravity-regular-x86.tsv` 122행(그라비테이션 정기의 signed count·프레임 감김/역방향·종료·NaN payload)과 힐/버스트 계약을
//   tests/raw-bomb-heal.test.mjs 가 대조한다. 기준선 뒤 cpppj 변경 없음(2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-15 WB04-E553: 힐 주문 대상 본문과 버스트 정기 RawBombHeal 이식)
import { setRawSquidHitPoints } from './raw-squid-word.js';
import { RegularProcess } from './squid-process.js';

// 슬롯의 가상 표(+0)·TYPE(+10)·운반체 WORD(+12)·좌표 float(+14/+18)·HP(+26)·extra(+40)·프레임(+36) 위치이다. (2026-11-15 WB04-E553)
const TABLE_OFFSET = 0;
const TYPE_OFFSET = 10;
const CARRIER_OFFSET = 12;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const HIT_POINTS_OFFSET = 26;
const FRAME_OFFSET = 36;
const EXTRA_OFFSET = 40;
// 41ebb0/41ebd0 의 genus 마스크(자격·보행·버스트 금지)와 extra 에서 해제하는 은신/마비 비트 보존 마스크이다. (2026-11-15 WB04-E553)
const ALLOWED_GENUS = 0x81574204;
const WALKING_GENUS = 0x200000;
const NO_BURST_GENUS = 0x1000004;
const CLEAR_STATUS_MASK = 0xcf;
// 보행 대상의 Repop flags 와 버스트 종료 삭제 flags, 종료 직전 붙이는 일반 Regular 의 이벤트/주기이다. (2026-11-15 WB04-E553)
const REPOP_FLAGS = 0x40;
const BURST_DESTROY_FLAGS = 0x100048;
const NEXT_EVENT = 2;
const NEXT_DELAY = 0.05;
const REGULAR_FLAGS = 0x50;

// 선택된 실제 효과만 요구하며 빈 경계로 동작을 생략하지 않는다. (2026-11-15 WB04-E553)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('힐 주문의 선택된 실제 하위 미연결');
    return hook;
}

// 슬롯 DWORD 를 비정렬 little endian 으로 읽는다. (2026-11-15 WB04-E553)
function readDword(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 힐 대상 본문이다. 같은 풀/현재 TYPE 를 읽으며 독립 회복 수치나 임시 월드를 만들지 않는다. (2026-11-15 WB04-E553)
export class RawBombHeal {
    // PATCH 의 실제 188칸 TYPE 표와 같은 물리 풀을 빌린다.
    constructor(pool, types, hooks) {
        if (pool?.edition !== 'patch1078' || types.length !== 188) throw new TypeError('힐 주문 PATCH/188TYPE 계약 오류');
        this.pool = pool;
        this.types = types;
        this.hooks = hooks;
    }

    // free/dead 나 추가 자격 필터 없이 원문 폭으로 읽는다.
    read(sid, offset, width = 4) {
        const raw = this.pool.slot(sid);
        let value = 0;
        for (let byte = 0; byte < width; byte += 1) value += raw[offset + byte] * 2 ** (8 * byte);
        return value >>> 0;
    }

    // 외부 효과가 바꾼 현재 종류를 호출 위치마다 확인한다.
    type(sid) {
        const type = this.read(sid, TYPE_OFFSET, 1);
        if (type >= this.types.length) throw new RangeError('힐 주문 현재TYPE 범위 오류');
        return this.types[type];
    }

    // 생성 뒤 실제 4adea0 발자국 중심을 단정도로 저장한다(정수 발자국의 절반을 double 로 빼고 한 번만 float 로 좁힘).
    center(sid) {
        const type = this.type(sid);
        const raw = this.pool.slot(sid);
        const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
        return [Math.fround(view.getFloat32(X_OFFSET, true) - type.footX * 0.5), Math.fround(view.getFloat32(Y_OFFSET, true) - type.footY * 0.5)];
    }

    // 41ebb0: 현재 genus 의 전체 마스크 값을 0/1 로 바꾸지 않고 돌려준다.
    mayTarget(target) { return (this.type(target).flags2 & ALLOWED_GENUS) >>> 0; }

    // 41ebd0: 현재 대상의 각 효과가 끝난 뒤 필요한 필드를 다시 읽는다.
    apply(parent, target) {
        const h = this.hooks;
        if (target === this.read(parent, CARRIER_OFFSET, 2)) return;
        if (need(h.hasHitPoints)(target)) {
            const health = this.read(target, HIT_POINTS_OFFSET);
            const maximum = need(h.maximumHitPoints)(target);
            // 원문 ADD 의 DWORD 감김을 유지하고 signed 평균만 0 쪽으로 절삭한다.
            const sum = (health + (maximum >>> 0)) | 0;
            setRawSquidHitPoints(this.pool, target, Math.trunc(sum / 2) >>> 0);
        }
        this.pool.physical(target)[EXTRA_OFFSET] &= CLEAR_STATUS_MASK;
        const kind = this.read(target, TYPE_OFFSET, 1);
        if (kind === need(h.fenceType)()) need(h.fence)(target);
        const walking = (this.type(target).flags2 & WALKING_GENUS) !== 0;
        const table = this.read(target, TABLE_OFFSET);
        if (walking) {
            need(h.unpop)(target, table, 0);
            need(h.repop)(target, this.read(target, TABLE_OFFSET), REPOP_FLAGS);
        } else need(h.refresh)(target, table);
        if (this.type(target).flags2 & NO_BURST_GENUS) return;
        const burst = need(h.burstType)();
        const created = need(h.create)(burst, 2);
        const center = this.center(target);
        need(h.pop)(created, this.read(created, TABLE_OFFSET), center[0], center[1], 0);
    }
}

// 41f9d0: 힐/미티어의 현재 VT 가 같은 Pool·Frame·Host 본문을 공유한다. (2026-11-15 WB04-E553)
export function handleHealBurstRegular(pool, frames, setter, host, refresh, destroy, sid, count, payload) {
    if (pool?.edition !== 'patch1078' || frames.length !== 188 || host?.pool !== pool) throw new TypeError('힐 버스트 실제 풀/프레임/Host 계약 오류');
    const raw = pool.slot(sid);
    const type = raw[TYPE_OFFSET];
    if (type >= frames.length) throw new RangeError('힐 버스트 현재TYPE 범위 오류');
    const limit = ((frames[type].run('A').count >>> 0) - 1) | 0;
    if ((count | 0) < limit) {
        // 원문은 현재 프레임에 1 을 더하며 첫 프레임으로 다시 시작하지 않는다.
        setter.set(sid, (readDword(raw, FRAME_OFFSET) + 1) | 0, 0);
        need(refresh)(sid);
        return payload;
    }
    host.attachRegular(new RegularProcess(NEXT_EVENT, NEXT_DELAY), sid, false, REGULAR_FLAGS);
    need(destroy)(sid, BURST_DESTROY_FLAGS);
    return -1.0;
}

// 41f760: 그라비테이션 정기이다. 진행 뒤 역방향 프레임과 0 프레임 종료·일반 Regular 2 부착을 보존한다. (2026-11-15 WB04-E553)
export function handleGravityBurstRegular(pool, frames, setter, host, refresh, destroy, sid, count, payload) {
    if (pool?.edition !== 'patch1078' || frames.length !== 188 || host?.pool !== pool || setter?.pool !== pool) throw new TypeError('그라비테이션 실제 풀/프레임/Host 계약 오류');
    const raw = pool.slot(sid);
    const type = raw[TYPE_OFFSET];
    if (type >= frames.length) throw new RangeError('그라비테이션 현재 TYPE 범위 오류');
    const total = frames[type].run('A').count >>> 0;
    const limit = (total - 1) | 0;
    const frame = readDword(raw, FRAME_OFFSET);
    // 진행 중에는 현재 프레임에 1 을 더하며 첫 프레임으로 초기화하지 않는다.
    let next = (frame + 1) >>> 0;
    if ((count | 0) >= limit) {
        if (frame === 0) {
            host.attachRegular(new RegularProcess(NEXT_EVENT, NEXT_DELAY), sid, false, REGULAR_FLAGS);
            need(destroy)(sid, BURST_DESTROY_FLAGS);
            return -1.0;
        }
        // 원문 SUB/LEA 의 32비트 감김과 역방향 프레임 계산을 유지한다.
        next = (total + total - (count >>> 0) - 2) >>> 0;
    }
    setter.set(sid, next | 0, 0);
    need(refresh)(sid);
    return payload;
}
