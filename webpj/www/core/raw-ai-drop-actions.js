// AI 의 현재 배치 TYPE/예산 스캔과 생산·기한·사제 관문을 실제 후보 배치 행동으로 연결하는 모듈이다(cpppj `o/RawAiDropActions.cpp/.h`, 10.78: 패치 004162e0 전체 배치 행동·00410580 생산 조각).
//  - AI_DROP_ACTION_LOG: 004105b0 의 일곱 원래 알림(lostAbility·scanComplete·partialScan·cannotAfford·badChoice·laying·target)이다. 알림 객체는 `{ action, type, direction, x, y }` 이며 TYPE/방향/좌표는 각 원문 위치의 snapshot,
//    좌표는 y → x ftol 로 구한 값이고 Partial 외에는 알림을 내기 전에 원래 TYPE getter 의 범위 검사를 먼저 수행한다.
//  - new RawAiDropActions(memory, pool, types, random, globals, hooks): memory 는 E164 `RawAiBrainMemory`, pool 은 같은 PATCH SidPool(생산 조각/표적 raw 슬롯), types 는 현재 188 TYPE({ flags2 })의 같은 배열, random 은 같은 게임 공용 `GameRandom`(후보 count 로 한 번씩 소비),
//    globals = { now()(0055b4d0 현재 QWORD 시각, 각 기한 비교에서 재조회), mapLimit()(00531928 현재 strict 좌표 유효성 상한), checking()(005e4794 의 현재 DWORD, 빈 후보/점수 모순 진단 관문) },
//    hooks = { product(token, type)→SID(004103c0 현재 owner 기술 부모 mask 8 TYPE 조회), select(token)(00413100), scan(token, type, direction)→미완료(00412e10), canAfford(token, type)(00410280), mayPlace(query)(00411a10→0049b510 전체),
//    deploy(token, type, point, argument, direction)(004108f0, 성공값을 만들지 않음), fake(token)(00415fe0), seed(token)(004133e0), remove(token, index)(0040f390), assertion(text, line)(Ai.cpp 복귀 가능한 진단), log(token, notice)(+0xb00 이 비영인 위치에서만) } 이다.
//  - run(token): 004162e0 전체 — 현재 TYPE 의 생산 조회 실패는 알림만(선택을 지우지 않음) → TYPE 가 없거나 재선택 표식이면 선택(여전히 0 이면 끝) → 스캔 미완료면 스캔(완료 시 완료 표식 + 현재 TYPE/방향으로 알림) → 기한(+0x5a8/+0x5e8, unordered 미도달)·지갑(signed 1200)·최초 시도(+0x400)·
//    생산 조각(pending bit 0x100 만 제외)·사제 탐색 진행(signed)·유효한 목표 좌표(있으면 배치 안 함) 관문 → 스캔 미완료 알림 → 후보가 없으면(checking 진단 뒤 현재 TYPE flags2 0x800 이면 fake + 재선택, 아니면 seed) 있으면 비용 확인 후
//    성공/표적(+0x3f4/+0x3fc)을 지우고 공용 난수로 후보 하나씩 승인(거부는 BadChoice, 승인은 Laying/Target 알림 뒤 실제 배치와 성공·표적·집중(+0x248 하위 BYTE 0x40) 쓰기)하며 실패 후보도 실제 압축으로 제거하고 압축 뒤 현재 성공 표식으로만 중단한다. 각 하위 뒤 현재 필드를 다시 읽는다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E174: AI 배치 행동 이식으로 추가)
import { AI_BRAIN_BYTES } from './raw-ai-brain.js';
import { aiPriestPointValid } from './raw-ai-priest-actions.js';

// 004105b0 의 일곱 원래 알림 종류이다. (2026-11-09 WB04-E174)
export const AI_DROP_ACTION_LOG = Object.freeze({
    lostAbility: 'LostAbility', scanComplete: 'ScanComplete', partialScan: 'PartialScan', cannotAfford: 'CannotAfford', badChoice: 'BadChoice', laying: 'Laying', target: 'Target'
});

// 원본 signed 지갑 하한과 물리 후보 배열 크기, 후보 XY/표적 배열 위치이다. (2026-11-09 WB04-E174)
const WALLET_MINIMUM = 1200;
const CHOICES = 30;
const POINT_AT = 0x408;
const TARGET_AT = 0x4f8;
const TWO_63 = 9223372036854775808;

// 비정렬 little endian DWORD 를 실제 블록 안에서 읽는다. (2026-11-09 WB04-E174)
function read(raw, at) {
    if (at < 0 || at > raw.length || raw.length - at < 4) throw new RangeError('AI 배치 행동 DWORD 읽기 범위');
    return (raw[at] | (raw[at + 1] << 8) | (raw[at + 2] << 16) | (raw[at + 3] << 24)) >>> 0;
}

// 원본 DWORD 의 signed 비교/방향을 보존한다. (2026-11-09 WB04-E174)
const signed = (value) => value | 0;

// CRT signed64 절삭의 하위 DWORD(signed)이며 무효 FISTP 의 하위값은 0 이다. (2026-11-09 WB04-E174)
function ftol(value) {
    if (!Number.isFinite(value) || value < -TWO_63 || value >= TWO_63) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 단정도 float 비트를 수로 바꾼다. (2026-11-09 WB04-E174)
function bitsFloat(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 도달한 실제 하위가 없으면 앞선 상태 쓰기를 유지하고 그 위치에서 거부한다. (2026-11-09 WB04-E174)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('AI 배치 행동 실제 하위 미연결');
    return hook;
}

// 배치 행동 처리기이다. 별도 구매/좌표/성공 사본 없이 같은 실제 본체/원본 공용 난수로 행동한다. (2026-11-09 WB04-E174)
export class RawAiDropActions {
    #memory;
    #pool;
    #types;
    #random;
    #globals;
    #hooks;

    // 생성 중 선택/난수/게임 장치를 실행하지 않는다. (2026-11-09 WB04-E174)
    constructor(memory, pool, types, random, globals, hooks) {
        if (!memory || typeof memory.bytes !== 'function') throw new TypeError('AI 배치 행동 본체 소유자 누락');
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== 188 || typeof random?.next !== 'function') throw new TypeError('AI 배치 행동 같은 PATCH188 TYPE 누락');
        for (const name of ['now', 'mapLimit', 'checking']) {
            if (typeof globals?.[name] !== 'function') throw new TypeError(`AI 배치 행동 현재 전역 조회기 누락: ${name}`);
        }
        this.#memory = memory;
        this.#pool = pool;
        this.#types = types;
        this.#random = random;
        this.#globals = globals;
        this.#hooks = hooks ?? {};
    }

    // 같은 실제 월드의 풀인지 확인할 수 있도록 제공한다. (2026-11-09 WB04-E174)
    get pool() { return this.#pool; }

    // 해제 토큰/작은 가짜 본체를 성공으로 취급하지 않는다. (2026-11-09 WB04-E174)
    #body(token) {
        const raw = this.#memory.bytes(token);
        if (raw.length !== AI_BRAIN_BYTES) throw new RangeError('AI 배치 행동 실제0xb08 본체 누락');
        return raw;
    }

    // 하위 뒤 현재 본체를 다시 빌려 DWORD 를 읽는다. (2026-11-09 WB04-E174)
    #field(token, at) { return read(this.#body(token), at); }

    // 주변 필드를 보존하며 지정한 네 바이트만 기록한다. (2026-11-09 WB04-E174)
    #set(token, at, value) {
        const raw = this.#body(token);
        if (at < 0 || at > raw.length || raw.length - at < 4) throw new RangeError('AI 배치 행동 DWORD 쓰기 범위');
        // 지정 폭만 쓴다.
        for (let index = 0; index < 4; index += 1) raw[at + index] = ((value >>> 0) >>> (index * 8)) & 0xff;
    }

    // TYPE 0 도 원래 getter 처럼 조회하고 상위 비트가 있는 번호를 BYTE 로 자르지 않는다. (2026-11-09 WB04-E174)
    #type(type) {
        if (type >= this.#types.length) throw new RangeError('AI 배치 행동 현재 TYPE 범위');
        return this.#types[type];
    }

    // 두 기한은 각 비교 위치의 현재 시각이며 unordered 도 거부한다. (2026-11-09 WB04-E174)
    #ready(token, at) {
        const view = new DataView(new ArrayBuffer(8));
        view.setUint32(0, this.#field(token, at), true);
        view.setUint32(4, this.#field(token, at + 4), true);
        return this.#globals.now() >= view.getFloat64(0, true);
    }

    // 00410580: 생산 조각은 free/dead/새 소유자 필터 없이 원래 pending bit 0x100 만 제외한다. (2026-11-09 WB04-E174)
    #availableProduct(token, type) {
        const product = need(this.#hooks.product)(token, type);
        return product && (this.#pool.slot(product)[21] & 1) ? 0 : product;
    }

    // 회전을 배치 쿼리 flags 로 넣지 않으며 owner 의 하위 BYTE 를 사용한다. (2026-11-09 WB04-E174)
    #mayPlace(token, type, point) {
        const query = { type, argument: 0, x: point.x, y: point.y, flags: 0, owner: this.#field(token, 0x260) & 255, mode: 1 };
        this.#type(type);
        return need(this.#hooks.mayPlace)(query);
    }

    // 손상 count/index 를 다른 유효 후보로 clamp 하지 않는다. (2026-11-09 WB04-E174)
    #pointOffset(index) {
        if (index < 0 || index >= CHOICES) throw new RangeError('AI 배치 행동30후보 물리 범위');
        return POINT_AT + index * 8;
    }

    // 현재 후보 X/Y 를 하위/로그 뒤 다시 조회한다. (2026-11-09 WB04-E174)
    #point(token, index) {
        const at = this.#pointOffset(index);
        return { x: bitsFloat(this.#field(token, at)), y: bitsFloat(this.#field(token, at + 4)) };
    }

    // 같은 후보의 현재 전체 표적 번호이다. (2026-11-09 WB04-E174)
    #target(token, index) {
        this.#pointOffset(index);
        return this.#field(token, TARGET_AT + index * 4);
    }

    // 원본 0 슬롯 조회는 허용하며 물리 풀 밖을 잘라 다른 객체로 바꾸지 않는다. (2026-11-09 WB04-E174)
    #object(number) {
        if (number > 65535 || number >= this.#pool.capacity) throw new RangeError('AI 배치 행동 현재 표적 SID 물리 범위');
        return number;
    }

    // 출력이 꺼져도 원래 TYPE getter 의 범위 검사는 Partial 외에 앞서 수행한다. (2026-11-09 WB04-E174)
    #log(token, notice) {
        const full = { action: notice.action, type: notice.type ?? 0, direction: notice.direction ?? 0, x: notice.x ?? 0, y: notice.y ?? 0 };
        if (full.action !== AI_DROP_ACTION_LOG.partialScan) this.#type(full.type);
        if (this.#field(token, 0xb00)) need(this.#hooks.log)(token, full);
    }

    // 후보 좌표는 MayPlace 인자의 사본이 아니라 그 이후 현재 값이다(y → x ftol 뒤 현재 시설 TYPE 의 알림). (2026-11-09 WB04-E174)
    #pointLog(token, action, index) {
        const at = this.#pointOffset(index);
        const y = ftol(bitsFloat(this.#field(token, at + 4)));
        const x = ftol(bitsFloat(this.#field(token, at)));
        this.#log(token, { action, type: this.#field(token, 0x3e0), direction: 0, x, y });
    }

    // 모든 관문/출력·선택 효과 뒤 현재 필드를 원본 위치에서 재조회한다. (2026-11-09 WB04-E174)
    run(token) {
        const hooks = this.#hooks;
        let type = this.#field(token, 0x3e0);
        if (type && !need(hooks.product)(token, type)) this.#log(token, { action: AI_DROP_ACTION_LOG.lostAbility, type: this.#field(token, 0x3e0) });
        type = this.#field(token, 0x3e0);
        if (!type || this.#field(token, 0x3f0)) {
            need(hooks.select)(token);
            type = this.#field(token, 0x3e0);
            if (!type) return;
        }
        if (!this.#field(token, 0x3ec) && !need(hooks.scan)(token, type, signed(this.#field(token, 0x3e4)))) {
            const direction = signed(this.#field(token, 0x3e4));
            type = this.#field(token, 0x3e0);
            this.#set(token, 0x3ec, 1);
            this.#log(token, { action: AI_DROP_ACTION_LOG.scanComplete, type, direction });
        }
        if (!this.#ready(token, 0x5a8) || !this.#ready(token, 0x5e8) || signed(this.#field(token, 0x2d8)) < WALLET_MINIMUM || (this.#field(token, 0x400) && !this.#field(token, 0x3ec))) return;
        if (!this.#availableProduct(token, this.#field(token, 0x3e0)) || signed(this.#field(token, 0x5a4)) < signed(this.#field(token, 0x5a0))) return;
        if (aiPriestPointValid({ x: bitsFloat(this.#field(token, 0xad0)), y: bitsFloat(this.#field(token, 0xad4)) }, this.#globals.mapLimit())) return;
        if (!this.#field(token, 0x3ec)) this.#log(token, { action: AI_DROP_ACTION_LOG.partialScan });
        if (signed(this.#field(token, 0x570)) < 1) {
            if (this.#globals.checking() && this.#field(token, 0x3e8)) need(hooks.assertion)('drop.value == 0', 0xf8b);
            if (this.#type(this.#field(token, 0x3e0)).flags2 & 0x800) {
                need(hooks.fake)(token);
                this.#set(token, 0x3f0, 1);
                return;
            }
            need(hooks.seed)(token);
        } else {
            if (!need(hooks.canAfford)(token, this.#field(token, 0x3e0))) {
                this.#log(token, { action: AI_DROP_ACTION_LOG.cannotAfford, type: this.#field(token, 0x3e0) });
                this.#set(token, 0x3f0, 1);
                return;
            }
            this.#set(token, 0x3f4, 0);
            this.#set(token, 0x3fc, 0);
            // 실패 후보도 실제 압축하고 압축 뒤 현재 성공 플래그로만 중단한다.
            while (signed(this.#field(token, 0x570)) > 0) {
                const index = signed(this.#random.next(this.#field(token, 0x570)));
                const selectedType = this.#field(token, 0x3e0);
                const point = this.#point(token, index);
                if (!this.#mayPlace(token, selectedType, point)) this.#pointLog(token, AI_DROP_ACTION_LOG.badChoice, index);
                else {
                    this.#pointLog(token, AI_DROP_ACTION_LOG.laying, index);
                    const target = this.#pool.slot(this.#object(this.#target(token, index)));
                    const targetType = target[10];
                    const y = ftol(bitsFloat(read(target, 18)));
                    const x = ftol(bitsFloat(read(target, 14)));
                    this.#log(token, { action: AI_DROP_ACTION_LOG.target, type: targetType, direction: 0, x, y });
                    const direction = this.#field(token, 0x3e4);
                    const currentPoint = this.#point(token, index);
                    const currentType = this.#field(token, 0x3e0);
                    need(hooks.deploy)(token, currentType, currentPoint, 0, direction);
                    const focused = (this.#field(token, 0x248) & 0x40) !== 0;
                    this.#set(token, 0x3f4, 1);
                    this.#set(token, 0x3fc, this.#target(token, index));
                    if (focused) this.#set(token, 0x3f8, this.#target(token, index));
                }
                need(hooks.remove)(token, index);
                if (this.#field(token, 0x3f4)) {
                    this.#set(token, 0x3f0, 1);
                    return;
                }
            }
        }
        this.#set(token, 0x3f0, 1);
    }
}
