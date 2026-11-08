// 서버의 건설 확정(Construction.cpp 00444590)을 옮긴 모듈이다(cpppj `o/RawConstructionConfirm.cpp/.h`, 10.78).
// 배치 가능 판정을 통과한 요청에 대해 조각마다 서버 SID 를 확보하고, 배치 통지(메시지 0x3f)를 만들어 보낸 뒤 같은 통지를 자기 쪽 처리기에 직접 넘긴다. 통지를 받은 쪽이 RawConstructionPlace 로 조각을 놓는다.
//  - confirm(request): 값형 요청 {type, x, y, player, argument, direction, flags, timeLow, timeHigh, quality}. 타입 번호 범위(255 이하·표 안)를 효과 전에 확인하고, 타입 flags1 에 "사제가 지어야 함"(0x8000)이 있고
//    요청 flags 비트0 이 없고 기준점 조회 생략 전역이 꺼져 있으면 먼저 그래프 기준점(findBuilder)을 찾아 0 이면 아무 효과 없이 거짓을 돌려준다. 그 밖에는 타입별 Canon 디코더로 조각 수를 세어(20개 이상이면
//    SID 생성 전에 거부) 조각마다 서버 SID 의 void 객체를 만들고(create(type, 0), 번호가 5 미만이거나 풀 밖이면 거부) 통지(좌표·시각은 비트 그대로, 나머지는 하위 바이트)를 담아 broadcast → handle(서버 번호 9 가 보낸 것처럼
//    직접 처리) → (기록기가 켜져 있고 활성이면) record(첫 SID, 조각이 없으면 0)를 부르고 참을 돌려준다.
//  - actualConfirm(request, header): 실제 경로이다. 호출마다 독립 512바이트 지역 패킷을 헤더로 초기화(0xff)한 뒤 디코더 생성(FindBuilder 정상 복귀 뒤 현재 패턴 타입)→조각마다 create → 현재 SID WORD 저장 → 개수 증가 →
//    20개 이상이면 원문 진단(sidCount < MAX_SIDS, 0x32d) 복귀 뒤 계속 → 필드 쓰기 → transmitAndHandle(같은 패킷 바이트) → (기록기) record(지역 첫 SID)이다. 타입 255 초과 검사는 typed 경로에만 있다.
//  - makeConstructionConfirmHooks(pool, factory, hooks): SID 생성(create)만 같은 풀의 실제 factory 로 바꾼다. 다른 풀은 거부한다.
// 10.72(CD) 경로는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E139: 건설 확정 이식으로 추가)
import { decodeCanonType } from './raw-canon-type.js';
import { initializeConstructionPacket } from './raw-construction-packet.js';

// 타입 flags1 의 "사제가 지어야 함" 비트와 확정 플래그의 "기준점 조회 생략" 비트, 통지에 담을 수 있는 SID 수 상한(20번째를 넣은 뒤 assert), 일반 객체의 첫 번호, 타입 표 크기이다. (2026-11-09 WB04-E139)
const NEEDS_BUILDER = 0x8000;
const SKIP_BUILDER_FLAG = 1;
const MAX_SIDS = 20;
const FIRST_SID = 5;
const TYPE_COUNT = 188;
// 실제 wire 의 SID 목록 시작 위치(+0x1b)와 지역 패킷 크기이다. (2026-11-09 WB04-E139)
const PAYLOAD_START = 27;
const PACKET_SIZE = 512;

// 확정이 읽는 전역이다. skipBuilderCheck(0054db14 의 독립 fallback)·recorderEnabled(0059a7e4)·patternTypes(Canon 디코더의 여덟 타입 전역)와 실제 owner 를 빌리는 함수들
// (sharedFullDeck: 0054db14 의 현재 DWORD, sharedRecorderEnabled: 59a7e4 의 현재 DWORD, patternTypesRead: 실제 경로의 현재 패턴 타입, decoderDiagnostics: 디코더 진단)이다.
// 사용: 월드마다 하나를 두고 RawConstructionConfirm 에 넘긴다. 값은 원문 분기마다 다시 읽는다. (2026-11-09 WB04-E139)
export class ConstructionConfirmState {
    skipBuilderCheck = false;
    recorderEnabled = false;
    patternTypes = [107, 82, 94, 157, 131, 129, 140, 142];
    sharedFullDeck = null;
    sharedRecorderEnabled = null;
    patternTypesRead = null;
    decoderDiagnostics = null;

    // 각 확정의 원문 분기에서 현재 비영 여부를 다시 읽는다(최상위 비트도 참). (2026-11-09 WB04-E139)
    skipBuilderCheckNow() { return this.sharedFullDeck ? (this.sharedFullDeck() >>> 0) !== 0 : this.skipBuilderCheck; }
    // broadcast/자기 수신 뒤 현재 재생 플래그를 읽는다. (2026-11-09 WB04-E139)
    recorderEnabledNow() { return this.sharedRecorderEnabled ? (this.sharedRecorderEnabled() >>> 0) !== 0 : this.recorderEnabled; }
}

// 실제 wire 의 소비 위치에서 지정 폭만 쓴다. 이미 끝난 Create/앞 SID 의 부분 효과는 되돌리지 않는다. (2026-11-09 WB04-E139)
function writeField(packet, offset, value, width) {
    if (offset > packet.length || width > packet.length - offset) throw new RangeError('실제 건설 writer의 현재 필드 물리 부족');
    // 비정렬 WORD/DWORD 의 해당 BYTE 만 기록한다.
    for (let byte = 0; byte < width; byte += 1) packet[offset + byte] = Math.floor(value / 2 ** (8 * byte)) & 0xff;
}

// 단정도 float 의 비트(uint32)이다. (2026-11-09 WB04-E139)
const view = new DataView(new ArrayBuffer(4));
function bitsOf(value) {
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
}

// 건설 확정의 실행 계층이다. pool(10.78 SidPool)·types(현재 타입 표 188칸)·frames(타입별 { frames: 프레임 코드, defaultFrame })·state·hooks 는 이 객체보다 오래 살아야 한다.
// hooks: findBuilder(player, type, x, y)→SID, create(type, flags)→SID, recorderActive()→bool, record(sid) 와 [broadcast(notice), handle(notice)] 또는 transmitAndHandle(packet) 중 하나 이상,
// actualConfirm 에는 transmitAndHandle 과 assertion(식, 파일, 줄)이 필요하다. 사용 순서: 배치 가능 판정을 통과한 요청마다 confirm 을 한 번 부른다. (2026-11-09 WB04-E139)
export class RawConstructionConfirm {
    #pool;
    #types;
    #frames;
    #state;
    #hooks;

    // 풀과 필수 경계는 첫 효과 전에 검사한다. (2026-11-09 WB04-E139)
    constructor(pool, types, frames, state, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('건설 확정은 10.78 SidPool 만 받습니다.');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT || !Array.isArray(frames) || !state) throw new TypeError('건설 확정 필수 효과 연결 오류');
        const typed = typeof hooks?.broadcast === 'function' && typeof hooks?.handle === 'function';
        if (typeof hooks?.findBuilder !== 'function' || typeof hooks?.create !== 'function' || (!typed && typeof hooks?.transmitAndHandle !== 'function')
            || typeof hooks?.recorderActive !== 'function' || typeof hooks?.record !== 'function') throw new TypeError('건설 확정 필수 효과 연결 오류');
        this.#pool = pool;
        this.#types = types;
        this.#frames = frames;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 어댑터가 같은 실제 풀을 연결하는지 확인하는 데 쓰는 참조이다. (2026-11-09 WB04-E139)
    get pool() { return this.#pool; }

    // 사제가 지어야 하는 타입이면 먼저 그래프 기준점을 찾고, 없으면(0) 거짓이다. (2026-11-09 WB04-E139)
    #builderMissing(request) {
        const record = this.#types[request.type];
        return (record.flags1 & NEEDS_BUILDER) !== 0 && (request.flags & SKIP_BUILDER_FLAG) === 0 && !this.#state.skipBuilderCheckNow()
            && this.#hooks.findBuilder(request.player, request.type, request.x, request.y) === 0;
    }

    // 원본 순서: 기준점 조회 → 디코더 생성 → 조각마다 SID 생성 → 통지 필드 → 전송 → 직접 처리 → 기록기. (2026-11-09 WB04-E139)
    confirm(request) {
        const hooks = this.#hooks;
        const state = this.#state;
        // 실제 wire 전용 구성을 typed 빈 성공으로 호출하지 않는다.
        if (typeof hooks.broadcast !== 'function' || typeof hooks.handle !== 'function') throw new Error('typed 건설 확정의 통지 효과 미연결');
        if (request.type >= this.#types.length || request.type >= this.#frames.length || request.type > 0xff) throw new RangeError('건설 확정 타입 번호 오류');
        if (this.#builderMissing(request)) return false;
        const meta = this.#frames[request.type];
        const decoder = decodeCanonType(meta.frames, meta.defaultFrame | 0, state.patternTypes,
            { type: request.type, argument: request.argument | 0, direction: request.direction | 0, x: request.x, y: request.y, explicitFrame: false }, state.decoderDiagnostics);
        let pieces = 0;
        // 사본으로 유효 칸을 세어 상한을 넘는 요청을 SID 생성 전에 거부한다.
        for (const probe = decoder.clone(); probe.valid; probe.advance()) pieces += 1;
        if (pieces >= MAX_SIDS) throw new RangeError('건설 확정 조각 수가 통지 상한을 넘는다(sidCount < MAX_SIDS assert)');
        const sids = [];
        // 유효 칸마다 서버 SID 의 void 객체를 하나 만들고 번호를 통지에 순서대로 담는다.
        for (; decoder.valid; decoder.advance()) {
            const sid = hooks.create(request.type, 0);
            if (sid < FIRST_SID || sid >= this.#pool.capacity) throw new RangeError('건설 확정 SID 생성이 유효한 번호를 돌려주지 않았습니다');
            sids.push(sid);
        }
        // 좌표와 시각은 비트 그대로, 나머지 인자는 하위 바이트만 싣는다.
        const notice = {
            type: request.type & 0xff, x: request.x, y: request.y, argument: request.argument & 0xff, direction: request.direction & 0xff, player: request.player & 0xff,
            flags: request.flags & 0xff, quality: request.quality & 0xff, timeLow: request.timeLow >>> 0, timeHigh: request.timeHigh >>> 0, sids,
        };
        hooks.broadcast(notice);
        hooks.handle(notice);
        // 조각이 없을 때 원본은 초기화되지 않은 지역 값을 기록기에 넘긴다. 여기서는 0 을 넘기며 원본 값이라고 주장하지 않는다.
        if (state.recorderEnabledNow() && hooks.recorderActive()) hooks.record(sids.length === 0 ? 0 : sids[0]);
        return true;
    }

    // 444590 의 실제 순서를 따른다. typed 독립 계약의 선행 조각 수 검사는 이 경로에 적용하지 않는다. (2026-11-09 WB04-E139)
    actualConfirm(request, header) {
        const hooks = this.#hooks;
        const state = this.#state;
        if (typeof hooks.transmitAndHandle !== 'function' || typeof hooks.assertion !== 'function') throw new Error('실제 건설 확정의 mutable 통지/진단 미연결');
        if (request.type >= this.#types.length || request.type >= this.#frames.length) throw new RangeError('실제 건설 확정의 현재 TYPE/프레임 물리 범위');
        if (this.#builderMissing(request)) return false;
        // 호출마다 독립 512BYTE 를 소유하여 중첩 확정이 앞 호출의 wire 를 덮지 않는다.
        const packet = new Uint8Array(PACKET_SIZE);
        // 원문의 df590 은 디코더/Create 보다 앞서 미작성 본문을 0xff 로 만든다.
        initializeConstructionPacket(header, packet);
        // 디코더 생성 시 선택한 같은 TYPE 프레임을 유지한다. FindBuilder 의 효과 뒤 현재 8 owner 를 읽는다.
        const meta = this.#frames[request.type];
        const patterns = state.patternTypesRead ? state.patternTypesRead() : state.patternTypes;
        const decoder = decodeCanonType(meta.frames, meta.defaultFrame | 0, patterns,
            { type: request.type, argument: request.argument | 0, direction: request.direction | 0, x: request.x, y: request.y, explicitFrame: false }, state.decoderDiagnostics);
        let count = 0;
        let first = 0;
        // Create → 현재 WORD store → 개수 증가 → 20개 진단 정상 복귀 뒤에만 advance 한다.
        for (; decoder.valid; decoder.advance()) {
            const sid = hooks.create(request.type, 0);
            if (count === 0) first = sid;
            writeField(packet, PAYLOAD_START + 2 * count, sid, 2);
            count += 1;
            if (count >= MAX_SIDS) hooks.assertion('sidCount < MAX_SIDS', '\\Ns\\O\\Construction.cpp', 0x32d);
        }
        packet[4] = request.type & 0xff;
        writeField(packet, 5, bitsOf(request.x), 4);
        writeField(packet, 9, bitsOf(request.y), 4);
        packet[14] = request.direction & 0xff;
        packet[13] = request.argument & 0xff;
        packet[15] = request.player & 0xff;
        writeField(packet, 18, request.timeLow >>> 0, 4);
        packet[26] = count & 0xff;
        writeField(packet, 22, request.timeHigh >>> 0, 4);
        packet[16] = request.flags & 0xff;
        packet[17] = request.quality & 0xff;
        // 각 실제 가상 송신의 관찰 결과를 같은 지역 예약의 직접 수신에 이어 준다.
        hooks.transmitAndHandle(packet);
        // wire 가 바뀌어도 원본 지역 첫 SID 를 현재 기록기에 전달한다.
        if (state.recorderEnabledNow() && hooks.recorderActive()) hooks.record(first);
        return true;
    }
}

// SID 생성만 같은 풀의 실제 factory 로 바꾼다. 기준점 조회·통지·기록기 경계는 받은 그대로 둔다. 다른 풀의 factory 는 거부한다. (2026-11-09 WB04-E139)
export function makeConstructionConfirmHooks(pool, factory, hooks) {
    if (pool !== factory?.pool) throw new TypeError('건설 확정/factory SID 풀이 다릅니다');
    return { ...hooks, create: (type, flags) => factory.create(type, flags) };
}
