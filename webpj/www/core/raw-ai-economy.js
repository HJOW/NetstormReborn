// AI 전략 말미의 같은 지갑·SP 정리(004138c0)를 옮긴 모듈이다(cpppj `o/RawAiEconomy.cpp/.h`, 10.78).
//  - new RawAiEconomy(memory, sp, now, hooks): memory 는 E164 `RawAiBrainMemory`(본체 소유자), sp 는 같은 분산 SP 저장소(`ScrambledSpStore`), now() 는 현재 QWORD 시각(0055b4d0 의 double)을 읽는 함수이다.
//    hooks = { demo()(00463b50 현재 데모 여부), captured(owner)(00492340 의 현재 참여 플래그와 사제 ownerState 1) } 이며 도달한 위치에서만 요구하고 없으면 명시 오류이다(생성은 지갑/SP/타이머에 효과를 주지 않는다).
//  - reconcile(token): 본체 +0x5d8/+0x5dc 의 QWORD 기한(double)이 현재 시각 이하이면 다음 기한(= 현재 + 1.0 초)을 쓰고, 지갑(+0x2d8) signed 값이 `+0x290 + 2200`(DWORD 감김 뒤 signed) 미만이면 `+0x24c` 를 더한 뒤 데모 하한을 적용한다
//    (단일 지원이며 큰 시간 점프를 누적 보충 반복으로 바꾸지 않는다). 기한이 안 되어도 `+0x264`, `+0x260` 두 owner 의 포획 판정과 최종 SP 쓰기는 항상 수행한다. 포획이면 `+0x290 + 2200` 미만의 지갑을 그 값으로 올리고 데모 하한을 적용한다.
//    데모 하한은 데모 조회 뒤 현재 signed 지갑을 다시 읽어 1190 이하이면 1190 으로 둔다. 마지막은 지갑 signed 정수를 float 으로 저장해 `+0x260` owner 의 SP 에 쓴다(본체 정수 지갑은 바꾸지 않는다).
//    기한이 NaN(unordered)이면 지원만 생략하고 나머지는 계속한다. 각 하위 뒤에 본체의 현재 DWORD 를 다시 읽는다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E166: AI 경제 이식으로 추가)
import { AI_BRAIN_BYTES } from './raw-ai-brain.js';

// 004138c0 의 0x898 예산, 0x4a6 데모 하한(signed), 보호 PE 00501580 의 QWORD 지원 간격(초)이다. (2026-11-09 WB04-E166)
const BUDGET = 2200;
const FLOOR = 1190;
const PERIOD = 1.0;

// 범위 안의 비정렬 little endian DWORD 를 읽는다. (2026-11-09 WB04-E166)
function read(raw, offset) {
    if (offset < 0 || offset > raw.length || raw.length - offset < 4) throw new RangeError('AI 경제 DWORD 읽기 범위');
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 지정 DWORD 만 쓰고 나머지 본체를 유지한다. (2026-11-09 WB04-E166)
function put(raw, offset, value) {
    if (offset < 0 || offset > raw.length || raw.length - offset < 4) throw new RangeError('AI 경제 DWORD 쓰기 범위');
    for (let index = 0; index < 4; index += 1) raw[offset + index] = (value >>> (index * 8)) & 0xff;
}

// 원본 signed 비교를 위한 DWORD → int32 변환이다. (2026-11-09 WB04-E166)
const signed = (value) => value | 0;

// 선택되지 않은 하위는 요구하지 않고 선택된 누락은 명시 오류로 남긴다. (2026-11-09 WB04-E166)
function need(hook, message) {
    if (typeof hook !== 'function') throw new Error(message);
    return hook;
}

// QWORD 비트를 double 로, double 을 QWORD 비트(BigInt)로 바꾼다. (2026-11-09 WB04-E166)
function bitsToDouble(low, high) {
    const view = new DataView(new ArrayBuffer(8));
    view.setUint32(0, low, true);
    view.setUint32(4, high, true);
    return view.getFloat64(0, true);
}

// 같은 지갑의 DWORD 감김·signed 비교·QWORD 기한·최종 float 저장을 유지하는 AI 경제 처리기이다. (2026-11-09 WB04-E166)
export class RawAiEconomy {
    #memory;
    #sp;
    #now;
    #hooks;

    // 같은 실제 저장소와 현재 시각 조회기를 빌린다. 생성은 원본 지갑/SP/타이머에 효과를 주지 않는다. (2026-11-09 WB04-E166)
    constructor(memory, sp, now, hooks) {
        if (!memory || typeof memory.bytes !== 'function') throw new TypeError('AI 경제 본체 소유자 누락');
        if (!sp || typeof sp.set !== 'function') throw new TypeError('AI 경제 분산 SP 저장소 누락');
        if (typeof now !== 'function') throw new TypeError('AI 경제 현재 시각 조회기 누락');
        this.#memory = memory;
        this.#sp = sp;
        this.#now = now;
        this.#hooks = hooks ?? {};
    }

    // 가짜 핸들이나 작은 본체를 다른 저장소로 대신하지 않고 현재 살아 있는 0xb08 본체를 빌린다. (2026-11-09 WB04-E166)
    #body(token) {
        const raw = this.#memory.bytes(token);
        if (raw.length !== AI_BRAIN_BYTES) throw new RangeError('AI 경제 실제0xb08 본체 누락');
        return raw;
    }

    // 하위 효과 뒤에도 같은 본체의 지정 DWORD 를 현재 값으로 읽는다. (2026-11-09 WB04-E166)
    #field(token, offset) { return read(this.#body(token), offset); }

    // 현재 본체의 지정 DWORD 만 쓴다. (2026-11-09 WB04-E166)
    #set(token, offset, value) { put(this.#body(token), offset, value >>> 0); }

    // 데모 조회가 바꾼 현재 지갑을 원본 저장 포인터에서 다시 읽고 signed 1190 이하이면 1190 으로 유지한다. (2026-11-09 WB04-E166)
    #demoFloor(token) {
        if (need(this.#hooks.demo, 'AI 경제 현재 데모 조회 미연결')()) {
            const wallet = this.#field(token, 0x2d8);
            this.#set(token, 0x2d8, signed(wallet) > FLOOR ? wallet : FLOOR);
        }
    }

    // 포획 플래그만 먼저 판정하고 반환 뒤 비용/지갑을 현재 다시 읽는다(최대 비용 + 2200 미만이면 보정). (2026-11-09 WB04-E166)
    #captureBudget(token, owner) {
        if (need(this.#hooks.captured, 'AI 경제 현재 사제 포획 조회 미연결')(owner)) {
            const target = (this.#field(token, 0x290) + BUDGET) >>> 0;
            if (signed(this.#field(token, 0x2d8)) < signed(target)) {
                this.#set(token, 0x2d8, target);
                this.#demoFloor(token);
            }
        }
    }

    // 004138c0: 원본은 기한이 안 되어도 양쪽 포획 판정과 최종 분산 SP 쓰기를 항상 수행한다. (2026-11-09 WB04-E166)
    reconcile(token) {
        const due = bitsToDouble(this.#field(token, 0x5d8), this.#field(token, 0x5dc));
        const now = this.#now();
        if (now >= due) {
            const view = new DataView(new ArrayBuffer(8));
            view.setFloat64(0, now + PERIOD, true);
            this.#set(token, 0x5d8, view.getUint32(0, true));
            this.#set(token, 0x5dc, view.getUint32(4, true));
            if (signed(this.#field(token, 0x2d8)) < signed((this.#field(token, 0x290) + BUDGET) >>> 0)) {
                this.#set(token, 0x2d8, (this.#field(token, 0x24c) + this.#field(token, 0x2d8)) >>> 0);
                this.#demoFloor(token);
            }
        }
        this.#captureBudget(token, this.#field(token, 0x264));
        this.#captureBudget(token, this.#field(token, 0x260));
        const wallet = Math.fround(signed(this.#field(token, 0x2d8)));
        const bits = new DataView(new ArrayBuffer(4));
        bits.setFloat32(0, wallet, true);
        this.#sp.set(this.#field(token, 0x260), bits.getUint32(0, true));
    }
}
