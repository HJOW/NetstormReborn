// 하드너 주문의 대상 가상 함수 두 개(원문 0041fe90 자격·0041feb0 다리 강화 효과)를 같은 raw 풀에 적용하는 모듈이다(cpppj `o/RawBombHardener.cpp/.h`, 10.78 전용).
// 사용:
//   const body = new RawBombHardener(pool, types, frames, random, setter, flare);
//     pool = SidPool(10.78), types = 188칸 타입 표(`flags2`), frames = 188칸 프레임 코드 표({ codes: [{ side, variant, number, flags }] }), random = 공유 GameRandom,
//     setter = 같은 풀의 SquidFrame(실제 4acee0 프레임 지정), flare(target, 1, 2) = 실제 45a6c0 의 기본 방향 플레어.
//   body.mayTarget(target) → 대상의 현재 genus 중 다리 비트(4)를 그대로 돌려준다(HP·동맹·이미 강화됨을 자격에 더하지 않음).
//   body.apply(parent, target) → 대상이 부모의 운반체 WORD(+12)이면 끝낸다. 아니면 현재 방향의 강화 프레임('P' 변형·flags 0x40 이고 side 가 현재와 같음) 후보를
//     프레임 코드 순서로 최대 256개 모아 하나라도 있으면 난수를 한 번 쓴 뒤 고른 프레임을 지정하고, 후보가 없으면 난수/프레임 없이, 어느 쪽이든 플레어(target, 1, 2)를 호출한다.
// 원본 관측: cpppj `bomb-hardener-x86.tsv` 21행(자격 E 5행·적용 A 16행: 자격 마스크·후보 상한·난수 상태·QA 제외·실패 플레어)을 tests/raw-bomb-hardener.test.mjs 가 대조한다.
// 플레어가 연결되지 않았으면 이미 수행한 프레임 지정과 난수 소비를 되돌리지 않고 명시 오류로 알린다. 기준선 뒤 cpppj 변경 없음(2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-15 WB04-E551: 하드너 주문 대상 본문 RawBombHardener 이식)
import { pathSide } from './raw-path-animation.js';

// 슬롯의 TYPE(+10)·운반체 WORD(+12)·프레임 DWORD(+36) 위치이다. (2026-11-15 WB04-E551)
const TYPE_OFFSET = 10;
const CARRIER_OFFSET = 12;
const FRAME_OFFSET = 36;
// 하드너 genus 의 다리 비트와 강화 프레임 후보의 변형 글자('P')·flags 비트, 후보 보관 상한(원본 1024바이트 후보 공간)이다. (2026-11-15 WB04-E551)
const BRIDGE_GENUS = 4;
const HARDENED_VARIANT = 0x50;
const HARDENED_FLAG = 0x40;
const CANDIDATE_LIMIT = 256;

// 하드너 대상 본문이다. 모든 참조는 이 본문보다 오래 살아야 하며 별도 월드/난수 상태를 만들지 않는다. (2026-11-15 WB04-E551)
export class RawBombHardener {
    // 같은 PATCH 풀·현재 188칸 TYPE/프레임 표와 프레임 지정기를 연결한다.
    constructor(pool, types, frames, random, setter, flare) {
        if (pool?.edition !== 'patch1078' || types.length !== 188 || frames.length !== 188 || setter?.pool !== pool) throw new TypeError('하드너 실제 PATCH 풀/현재 TYPE/프레임 계약 오류');
        this.pool = pool;
        this.types = types;
        this.frames = frames;
        this.random = random;
        this.setter = setter;
        this.flare = flare;
    }

    // 물리 슬롯의 지정 폭만 little endian 으로 조합한다(이웃 필드·free/dead 상태를 읽지 않음).
    read(sid, offset, width = 4) {
        const raw = this.pool.slot(sid);
        let value = 0;
        for (let byte = 0; byte < width; byte += 1) value += raw[offset + byte] * 2 ** (8 * byte);
        return value >>> 0;
    }

    // 호출 위치의 현재 raw TYPE 번호이다(효과 뒤 다시 읽을 수 있게 번호를 고정하지 않음).
    type(sid) {
        const type = this.read(sid, TYPE_OFFSET, 1);
        if (type >= this.types.length) throw new RangeError('하드너 현재 TYPE 범위 오류');
        return type;
    }

    // 41fe90: 현재 genus 의 다리 비트(4)를 그대로 돌려준다.
    mayTarget(target) { return (this.types[this.type(target)].flags2 & BRIDGE_GENUS) >>> 0; }

    // 49aa90: 현재 방향의 강화 프레임 후보를 첫 256개만 모아 하나를 난수로 고른다(없으면 -1, 난수 미사용).
    chooseFrame(target) {
        const table = this.frames[this.type(target)];
        const side = pathSide(table, this.read(target, FRAME_OFFSET) | 0);
        const codes = this.frames[this.type(target)].codes;
        const candidates = [];
        // 후보는 원문 표 전체를 순서대로 훑되 처음 256개만 보관한다.
        for (let index = 0; index < codes.length; index += 1) {
            if (codes[index].side === (side & 0xff) && codes[index].variant === HARDENED_VARIANT && (codes[index].flags & HARDENED_FLAG) && candidates.length < CANDIDATE_LIMIT) candidates.push(index);
        }
        return candidates.length === 0 ? -1 : candidates[this.random.next(candidates.length)];
    }

    // 41feb0: 선택된 실제 가상 c0 이며 자격 검사를 반복하지 않는다.
    apply(parent, target) {
        if (target === this.read(parent, CARRIER_OFFSET, 2)) return;
        const selected = this.chooseFrame(target);
        if (selected !== -1) this.setter.set(target, selected, 0);
        if (typeof this.flare !== 'function') throw new Error('하드너 실제 플레어 미연결');
        const call = this.flare;
        call(target, 1, 2);
    }
}
