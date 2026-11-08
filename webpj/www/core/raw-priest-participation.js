// 사제의 살아 있는 목록과 포획 원소유자를 같은 실제 월드에서 찾아 참여 상태를 갱신하는 조회(00494f10)·갱신(00495060)을 옮긴 모듈이다(cpppj `o/RawPriestParticipation.cpp/.h`, 10.78 전용).
//  - find(소유자): 살아 있는 사제 목록(RawPriestLookup)의 첫 SID → 풀을 SID 2 부터 free 비트(1)만 건너뛰며 순회: genus 0x30000(Carrier)의 포획 내용물(mask 0x20, 현재 사제 타입)이 있고 그 원래 소유자 WORD 하위 7비트가 소유자와 같으면 즉시 그 SID,
//    현재 타입이 제단 타입이고 그 제단의 다이스(DaisAt)가 있으며 dead 가 아니고 원래 소유자 WORD 전체가 소유자와 같으면 마지막 일치 제단을 기억 → 끝까지 가면 기억한 제단(없으면 0). dead/void/contained 와 예측 슬롯도 후보로 검사한다.
//  - refresh(소유자): 소유자가 유효하지 않아도 조회는 수행하고, 결과가 없으면 2, 현재 타입이 사제 타입이면 0, 아니면 1 을 소유자 1..8 일 때만 현재 공유 소유자 상태 표에 쓴다.
// 세 타입 전역은 매 조회마다 읽는다(state 객체를 공유하고 값을 바꿔 가며 쓴다). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E68: 사제 참여 이식으로 추가)

import { RawSquidFinder } from './raw-squid-finder.js';
import { RawPriestLookup } from './raw-priest-lookup.js';

// 원본 풀 순회에서 읽는 상태/타입/원래 소유자/좌표 필드 위치, Carrier genus 비트, 포획된 사제 종속의 mask, 소유자 하위 7비트, 반복 시작 SID 이다. (2026-11-08 WB04-E68)
const TYPE_OFFSET = 10;
const STATUS_OFFSET = 11;
const ORIGINAL_OWNER_OFFSET = 12;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const CARRIER_GENUS = 0x30000;
const CAPTURED_MASK = 0x20;
const OWNER_MASK = 0x7f;
const FIRST_CANDIDATE = 2;
const TYPE_COUNT = 188;
const INT_LOW = -2147483648;
const INT_HIGH = 2147483648;

// 정렬되지 않은 슬롯 좌표를 단정도로 읽는다. (2026-11-08 WB04-E68)
function floatAt(raw, offset) {
    const view = new DataView(new ArrayBuffer(4));
    // 네 바이트를 리틀엔디언으로 옮긴다.
    for (let index = 0; index < 4; index += 1) view.setUint8(index, raw[offset + index]);
    return view.getFloat32(0, true);
}

// 실제 0041d770/CRT __ftol 처럼 0 방향 절삭하며 호스트의 범위 밖 변환은 거부한다. (2026-11-08 WB04-E68)
function cell(value) {
    if (!Number.isFinite(value) || value < INT_LOW || value >= INT_HIGH) throw new RangeError('사제 참여 typed 좌표 절삭 범위');
    return Math.trunc(value) | 0;
}

// pool 은 10.78 SidPool, hash 는 같은 공간 해시, types 는 188칸 TYPE 표(참조), priests 는 PriestPostPopState(목록과 소유자 상태 공유), carrier 는 RawCarrierPreDestroy(findContained),
//   state = { priestType, altarType, daisType } 는 현재 타입 DWORD 를 읽는 객체(값을 바꿔도 다음 조회에 반영), altar 는 선택 인자로 설치된 제단 몸체(daisAt, pool)이다. (2026-11-08 WB04-E68)
export class RawPriestParticipation {
    // 후보/종속/다이스의 현재 raw 를 공급하는 풀, 현재 4단계 버킷, 타입 표, 사제 목록 상태, 종속 조회, 타입 전역, 선택 제단 몸체이다.
    #pool;
    #hash;
    #types;
    #priests;
    #carrier;
    #state;
    #altar;

    // 같은 풀/현재 타입 표 연결 누락은 구성 때 거부한다. altar 는 있으면 같은 풀이어야 한다. (2026-11-08 WB04-E68)
    constructor(pool, hash, types, priests, carrier, state, altar = null) {
        if (pool?.edition !== 'patch1078' || carrier?.pool !== pool || !Array.isArray(types) || types.length !== TYPE_COUNT) throw new TypeError('사제 참여의 같은 풀/현재 타입 표 연결 누락');
        if (altar && altar.pool !== pool) throw new TypeError('사제 참여/제단의 SID 풀이 다릅니다');
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#priests = priests;
        this.#carrier = carrier;
        this.#state = state;
        this.#altar = altar;
    }

    // 로그인 종료 등 실제 소비자가 같은 월드인지 확인할 읽기 전용 접근이다. (2026-11-08 WB04-E68)
    get pool() {
        return this.#pool;
    }

    // 같은 사제 목록/소유자 상태 저장소인지 확인할 읽기 전용 접근이다. (2026-11-08 WB04-E68)
    get priests() {
        return this.#priests;
    }

    // 00448bb0 의 실제 좌표 절삭·현재 다이스 타입·일반 4단계 finder 를 재사용한다. 설치된 제단 몸체가 있으면 그 DaisAt 을 쓴다. (2026-11-08 WB04-E68)
    #daisAt(altar) {
        if (this.#altar) return this.#altar.daisAt(altar);
        const raw = this.#pool.slot(altar);
        const type = this.#state.daisType;
        if (type >= this.#types.length) throw new RangeError('사제 참여의 현재 다이스 타입 범위');
        const x = cell(floatAt(raw, X_OFFSET));
        const y = cell(floatAt(raw, Y_OFFSET));
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        // 현재 타입 flags2 와 실제 버킷·체인을 사용하며 owner/dead/free 추가 필터는 적용하지 않는다.
        for (let current = finder.begin({ left: x, top: y, right: x, bottom: y }, (this.#types[type].flags2 & 6) === 0 ? 1 : 0); current; current = finder.next()) {
            if (this.#pool.slot(current)[TYPE_OFFSET] === type) return current;
        }
        return 0;
    }

    // 원본 iterator 는 첫 증가로 SID 2 부터 시작하고 live 조회가 성공하면 풀을 읽지 않는다. 없으면 0 이다. (2026-11-08 WB04-E68)
    find(player) {
        const live = new RawPriestLookup(this.#pool, this.#priests).find(player);
        if (live) return live;
        const iteratorEnd = this.#pool.capacity;
        let candidate = FIRST_CANDIDATE;
        let savedAltar = 0;
        // free 인 슬롯을 건너뛴다. 최초 iterator 의 end 는 고정하고 각 바깥 반복의 유효 범위는 현재 capacity 로 다시 확인한다.
        while (candidate < iteratorEnd && (this.#pool.physical(candidate)[STATUS_OFFSET] & 1)) candidate += 1;
        // free 만 제외한다. dead/void/contained 및 예측 슬롯도 실제 후보로 검사한다.
        for (;;) {
            if (candidate < 1 || candidate >= this.#pool.capacity) return savedAltar;
            const type = this.#pool.physical(candidate)[TYPE_OFFSET];
            if (type >= this.#types.length) throw new RangeError('사제 참여의 현재 후보 타입 범위');
            if (this.#types[type].flags2 & CARRIER_GENUS) {
                const captured = this.#carrier.findContained(candidate, CAPTURED_MASK, this.#state.priestType, 0);
                if (captured && (this.#pool.slot(captured)[ORIGINAL_OWNER_OFFSET] & OWNER_MASK) === (player >>> 0)) return candidate;
            }
            // Carrier 조회 뒤의 현재 타입/현재 원래 소유자를 읽으며 제단 원래 소유자는 WORD 전체다.
            if (this.#pool.physical(candidate)[TYPE_OFFSET] === this.#state.altarType) {
                const dais = this.#daisAt(candidate);
                const raw = this.#pool.physical(candidate);
                if (dais && !(this.#pool.slot(dais)[STATUS_OFFSET] & 2) && (raw[ORIGINAL_OWNER_OFFSET] | (raw[ORIGINAL_OWNER_OFFSET + 1] << 8)) === (player >>> 0)) savedAltar = candidate;
            }
            candidate += 1;
            // 원본의 snapshot end 까지 free 를 건너뛰고 다음 반복에서 현재 capacity 를 다시 검사한다.
            while (candidate < iteratorEnd && (this.#pool.physical(candidate)[STATUS_OFFSET] & 1)) candidate += 1;
        }
    }

    // 유효하지 않은 소유자도 원본 조회 순서를 유지한 뒤 쓰기만 생략한다. 소유자 1..8 만 현재 공유 소유자 상태에 0/1/2 를 쓴다. (2026-11-08 WB04-E68)
    refresh(player) {
        const sid = this.find(player);
        const value = !sid ? 2 : this.#pool.slot(sid)[TYPE_OFFSET] === this.#state.priestType ? 0 : 1;
        const states = this.#priests.currentOwnerState();
        if (player > 0 && player < states.length) states[player] = value;
    }
}
