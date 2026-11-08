// 원본 AI 의 생산 조각 소비·대기 예약·실제 배치(004106e0)와 비용(00410280 확인·004102b0 지불)을 옮긴 모듈이다(cpppj `o/RawAiDeployment.cpp/.h`, 10.78).
//  - new RawAiDeployment(memory, pool, types, create, globals, hooks): memory 는 E164 `RawAiBrainMemory`, pool 은 같은 PATCH SidPool, types 는 현재 188 TYPE({ cost, flags2 })의 같은 배열(매 원문 위치에서 다시 읽는다), create 는 같은 풀의 `RawContainedCreate`(실제 생산 내용물/예측 조각 생성과 부착),
//    globals = { authority()(00540bc4: 서버 확정과 로컬 예측 분기), moveLocalBuilder(`{ value }` 상자, 00544d10: 실제 배치 요청 동안 true 를 쓰고 정상 반환 뒤 false), now()(0055b4d0: 배치 뒤 시설 종류별 QWORD 기한에 더하는 현재 시각) },
//    hooks = { product(token, type)→SID(004103c0 같은 본체 owner → Player 기술 부모 → mask 8 조회), technology(owner)→SID(0048fc90 현재 Player 기술 부모), destroy(sid, flags)(가상 +10 실제 조각 삭제),
//    reserveRegular()→bool(원본 new(0x28) 성공 여부, 실패해도 나머지 배치는 계속), addRegular(sid, rate)(예약 메모리에 event 0/현재 rate 를 구성하고 같은 Kernel 에 부착), request(request)(00444760 10인자 실제 서버 요청, 반환 무시),
//    localPlace(request)(004433b0 실제 로컬 예측/서버 송신), demo()(00463b50 현재 데모 여부), assertion(text)(원본 진단 뒤 계속) } 이며 도달한 위치에서만 요구한다.
//  - canAfford(token, type): 00410280 — 원본 ftol(cost)의 signed 하위 DWORD 와 현재 지갑(+0x2d8)을 비교한다. charge(token, type): 004102b0 — ftol(−cost) 감김 가산 뒤 현재 데모이면 signed 1190 이하를 1190 으로 올린다(SP 는 이 위치에서 쓰지 않는다).
//  - place(token, type, x, y, argument, direction): 004106e0 전체 — 생산 조각이 있으면 삭제 → 같은 기술 부모에 대기 내용물(type, flags 0x108, 부착 0x10) 생성 → Regular 예약(성공 시 현재 +0x224 rate 로 부착) → 비용 지불. 생산 조각이 없으면 이동/운반(0x70000) TYPE 는 진단 뒤 현재 flags2 를 다시 읽고
//    시설(0x4200)만 비용을 지불한다. 이어서 이동 플래그를 켜고 권한이면 10인자 서버 요청(소유자 = 현재 +0x260), 아니면 두 번째 내용물을 만들어 로컬 예측으로 보낸다. 반환 뒤 이동 플래그를 끄고 flags2 0x800/0x4200 시설은 현재 시각 + 20초 QWORD 기한(+0x5b8/+0x5c0)을 쓴다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E175: AI 배치 실행 이식으로 추가)
import { AI_BRAIN_BYTES } from './raw-ai-brain.js';

// 원본 signed 지갑의 0x4a6 하한과 보호 PE 005012d8 의 QWORD 시설 재판단 기한(초)이다. (2026-11-09 WB04-E175)
const DEMO_FLOOR = 1190;
const DEPLOYMENT_DELAY = 20;
const TWO_63 = 9223372036854775808;

// 비정렬 원문 DWORD 를 물리 범위 안에서 읽는다. (2026-11-09 WB04-E175)
function read(raw, offset) {
    if (offset < 0 || offset > raw.length || raw.length - offset < 4) throw new RangeError('AI 배치 DWORD 읽기 범위');
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 지정 필드만 쓰고 나머지 본체는 유지한다. (2026-11-09 WB04-E175)
function put(raw, offset, value) {
    if (offset < 0 || offset > raw.length || raw.length - offset < 4) throw new RangeError('AI 배치 DWORD 쓰기 범위');
    for (let index = 0; index < 4; index += 1) raw[offset + index] = (value >>> (index * 8)) & 0xff;
}

// 원본 ftol 은 signed64 절삭의 하위 DWORD 를 사용하며 무효 FISTP 의 하위 DWORD 는 0 이다. (2026-11-09 WB04-E175)
function truncate(value) {
    if (!Number.isFinite(value) || value < -TWO_63 || value >= TWO_63) return 0;
    return Number(BigInt.asUintN(32, BigInt(Math.trunc(value))));
}

// 단정도 float 비트를 수로 바꾼다. (2026-11-09 WB04-E175)
function bitsFloat(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 선택된 하위의 누락을 빈 성공으로 처리하지 않는다. (2026-11-09 WB04-E175)
function need(hook, message) {
    if (typeof hook !== 'function') throw new Error(message);
    return hook;
}

// AI 배치 실행기이다. 실제 내용물과 본체 지갑만 변경하며 별도 SP 차감이나 임시 전략을 만들지 않는다. (2026-11-09 WB04-E175)
export class RawAiDeployment {
    #memory;
    #pool;
    #types;
    #create;
    #globals;
    #hooks;

    // 실제 게임 계산을 시작하지 않고 공유 저장소의 물리 계약만 확인한다. (2026-11-09 WB04-E175)
    constructor(memory, pool, types, create, globals, hooks) {
        if (!memory || typeof memory.bytes !== 'function') throw new TypeError('AI 배치 본체 소유자 누락');
        if (pool?.edition !== 'patch1078' || create?.pool !== pool || !Array.isArray(types) || types.length === 0) throw new TypeError('AI 배치 같은 PATCH Pool/TYPE/내용물 생성 계약 누락');
        if (typeof globals?.authority !== 'function' || typeof globals.now !== 'function' || typeof globals.moveLocalBuilder !== 'object' || globals.moveLocalBuilder === null) throw new TypeError('AI 배치 현재 권한/이동 플래그/시각 누락');
        this.#memory = memory;
        this.#pool = pool;
        this.#types = types;
        this.#create = create;
        this.#globals = globals;
        this.#hooks = hooks ?? {};
    }

    // 실제 월드 조립 시 같은 풀인지 확인할 수 있도록 제공한다. (2026-11-09 WB04-E175)
    get pool() { return this.#pool; }

    // 해제 토큰이나 작은 본체를 거부하고 현재 0xb08 본체를 다시 빌린다. (2026-11-09 WB04-E175)
    #body(token) {
        const raw = this.#memory.bytes(token);
        if (raw.length !== AI_BRAIN_BYTES) throw new RangeError('AI 배치 실제0xb08 본체 누락');
        return raw;
    }

    // 하위 뒤 같은 본체의 현재 필드를 다시 읽는다. (2026-11-09 WB04-E175)
    #field(token, offset) { return read(this.#body(token), offset); }

    // 선택된 필드만 현재 본체에 기록한다. (2026-11-09 WB04-E175)
    #set(token, offset, value) { put(this.#body(token), offset, value >>> 0); }

    // 원본 TYPE getter 의 물리 범위만 검사한다. (2026-11-09 WB04-E175)
    #type(type) {
        if (type >= this.#types.length) throw new RangeError('AI 배치 현재 TYPE 범위');
        return this.#types[type];
    }

    // 실제 getter 반환을 WORD 로 미리 잘라 잘못된 부모로 바꾸지 않고 현재 owner 로 기술 부모를 다시 조회한다. (2026-11-09 WB04-E175)
    #technology(token) {
        const parent = need(this.#hooks.technology, 'AI 배치 현재 기술부모 조회 미연결')(this.#field(token, 0x260)) >>> 0;
        if (parent >= this.#pool.capacity) throw new RangeError('AI 배치 현재 기술부모 SID 범위');
        return parent;
    }

    // 00410280: 비용은 반올림이 아닌 원본 signed 절삭 뒤 현재 지갑과 비교한다. (2026-11-09 WB04-E175)
    canAfford(token, type) { return (truncate(Math.fround(this.#type(type).cost)) | 0) <= (this.#field(token, 0x2d8) | 0); }

    // 004102b0: 원본은 비용을 먼저 음수로 만든 뒤 절삭하며 SP 자체는 이 위치에서 쓰지 않는다. (2026-11-09 WB04-E175)
    charge(token, type) {
        const delta = truncate(-Math.fround(this.#type(type).cost));
        this.#set(token, 0x2d8, (this.#field(token, 0x2d8) + delta) >>> 0);
        if (need(this.#hooks.demo, 'AI 배치 비용의 현재 데모 조회 미연결')()) {
            const wallet = this.#field(token, 0x2d8);
            this.#set(token, 0x2d8, (wallet | 0) > DEMO_FLOOR ? wallet : DEMO_FLOOR);
        }
    }

    // 시설 종류별 기한은 실제 배치 하위가 반환한 뒤의 현재 시각 + 20초 QWORD 이다. (2026-11-09 WB04-E175)
    #deadline(token, offset) {
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat64(0, this.#globals.now() + DEPLOYMENT_DELAY, true);
        this.#set(token, offset, view.getUint32(0, true));
        this.#set(token, offset + 4, view.getUint32(4, true));
    }

    // 성공을 가정한 임의 유닛 생성 대신 기존 생산/내용물/건설 몸체를 실행한다. (2026-11-09 WB04-E175)
    place(token, type, x, y, argument, direction) {
        const hooks = this.#hooks;
        const globals = this.#globals;
        const product = need(hooks.product, 'AI 배치 현재 생산 조회 미연결')(token, type);
        let charge = false;
        if (product) {
            need(hooks.destroy, 'AI 배치 실제 생산 조각 삭제 미연결')(product, 0);
            const replacement = this.#create.create(this.#technology(token), type, 0, 0x108, 0x10, 0);
            if (need(hooks.reserveRegular, 'AI 배치 Regular 예약 미연결')()) need(hooks.addRegular, 'AI 배치 Regular 실제 부착 미연결')(replacement, bitsFloat(this.#field(token, 0x224)));
            charge = true;
        } else {
            if (this.#type(type).flags2 & 0x70000) need(hooks.assertion, 'AI 배치 생산 조각 진단 미연결')('!( ::getType(type)->genus & (gEMPLACEMENT|gCARRIER) )');
            charge = (this.#type(type).flags2 & 0x4200) !== 0;
        }
        if (charge) this.charge(token, type);
        const authority = globals.authority();
        globals.moveLocalBuilder.value = true;
        if (authority) {
            const request = { type, x, y, player: this.#field(token, 0x260), argument, direction, flags: 0, timeLow: 0, timeHigh: 0, quality: 0 };
            need(hooks.request, 'AI 배치 실제 서버 건설 요청 미연결')(request);
        } else {
            const preview = this.#create.create(this.#technology(token), type, 0, 0x108, 0x10, 0);
            need(hooks.localPlace, 'AI 배치 실제 로컬 예측 미연결')({ preview, x, y, argument, direction });
        }
        globals.moveLocalBuilder.value = false;
        if (this.#type(type).flags2 & 0x800) this.#deadline(token, 0x5b8);
        if (this.#type(type).flags2 & 0x4200) this.#deadline(token, 0x5c0);
    }
}
