// 건설 배치 실행(Construction.cpp 00442c80)과 비용 부족 처리(00442b50)·환불(00442a40)·취소 통지(00442af0)·SP 조회(0040ef30)·SP 가산(0041dff0)을 옮긴 모듈이다(cpppj `o/RawConstructionPlace.cpp/.h`, 10.78).
// 로컬 배치(004433b0)와 서버 확정(00444590)이 미리 확보한 SID 목록을 받아, 타입의 조각마다 abstract 비트·HP·프레임·단어·소유자를 정하고 Pop 을 요청한다. SID 확보·배치 가능 판정·메시지 수신 처리는 호출자 쪽이다.
//  - place(request): 요청 {type, x, y, argument, direction, player, sids, abstract, quality}. 타입 번호가 표 밖이거나 플레이어가 서버 번호(9)이거나 SID 개수가 디코더 조각 수와 다르면 효과 없이 거부한다
//    (조각 수는 디코더 사본으로 끝까지 세어 본다). 비용 판정(다른 플레이어의 배치만, 모자라면 chargeShortfall 아니면 차감) 뒤 조각마다 [서버가 아니고 번호가 5 미만이거나 서버 영역이면 take 로 수신 →
//    void 비트 확인 → abstract 비트(+40) → HP 가 있으면(방금 쓴 abstract 비트를 읽음) abstract 인자가 0 이 아니면 0, 아니면 최대 HP → 프레임 DWORD(+36) → +0xc 단어(noIsland 시작 칸 0x5d4 또는 인자 하위 단어, 다리 타입이면 표면 알림)
//    → 다리 타입이면 품질 프레임(금 간 0x20·단단한 0x40)·수명 비트(3~6)·표면 알림 → 소유자 지정 → Pop(abstract 인자가 0 이 아니면 2, 아니면 0x41)]을 한다.
//  - actualPlace(request, declaredCount, sidReader): 같은 순서의 실제 wire 경로이다. 선행 전수 검사 없이 고정 scalar/count 와 현재 SID 읽기 함수로 비용→조각→후행 진단을 실행하며, 예약/free 슬롯도 물리 범위 안이면 접근한다.
//    진단(player 9·void 아님·다리 품질 프레임 없음·SID 개수 불일치)은 Client 진단 장치(hooks.assertion)에 보고하고 복귀하면 같은 순서로 계속한다.
//  - chargeShortfall(type, player, x, y, argument, direction): 표시 비트(0x400)를 먼저 읽어 두고 켠 뒤, 권한이 있으면 [처음이면 다른 플레이어에게 안내, 당사자 안내, 좌표가 유효하면 reject], 없으면 [비용 강제 차감, 처음이면 로컬 안내]를 한다.
//  - refund(type, player): 너겟이 아니면 비용을 정수로 절삭해 SP 를 돌려주고(전투 표시 중의 로컬 플레이어는 건너뜀) 금액이 양수이면 SP 통지(11바이트)를 보낸다. reject(…): 환불 뒤 취소 통지(15바이트)를 보낸다.
//  - money(player): 전투 표시 중의 로컬 플레이어는 번호 검사 없이, 그 밖에는 1~8 일 때만 저장소를 읽는다. addMoney(player, delta): 로컬 플레이어이면 로컬 가산, 아니면 플레이어 가산이다.
//  - bindPlayerRecords(players): 실제 9×0xb4 Player 바이트를 연결한다(표시 비트 +0x54·이름 +0xc 를 매 호출 직접 읽고 쓴다).
//  - makeConstructionPlaceHooks(pool, factory, owner, reward, store, state, hooks): 수신(factory.take)·소유자 지정(owner.set)·SP 읽기(store.get)/가산(reward.addSp)을 같은 풀의 실제 모듈로 바꾼다.
// 10.72(CD) 경로와 10.37 은 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E138: 건설 배치 이식으로 추가)
import { decodeCanonType } from './raw-canon-type.js';

// 두 판본 공통 raw 필드: 타입(+10)·상태(+11)·+0xc 단어·HP(+26 DWORD)·abstract 비트 바이트(+40)·프레임 DWORD(+36)·PATCH 소유자(+34) 위치이다. (2026-11-09 WB04-E138)
const TYPE = 10;
const STATE = 11;
const WORD = 12;
const HP = 26;
const EXTRA = 40;
const FRAME = 36;
// 상태 바이트의 void 비트, 일반 객체의 첫 번호, 서버 영역 첫 번호, 서버 플레이어 번호(GAME_SERVER_ID), 플레이어 수이다. (2026-11-09 WB04-E138)
const VOID = 4;
const FIRST_SID = 5;
const SERVER_FIRST = 15000;
const SERVER_PLAYER = 9;
const PLAYER_COUNT = 8;
// Player +0x54 의 비용 부족 표시 비트, Player 구조체 크기, noIsland 시작 칸 단어, 이름 필드 범위(+0xc..+0x2c)이다. (2026-11-09 WB04-E138)
const SHORTFALL_FLAG = 0x400;
const PLAYER_STRIDE = 0xb4;
const ORIGIN_WORD = 0x5d4;
const NAME_AT = 0xc;
const NAME_END = 0x2c;
// Pop 플래그: 확정 배치 0x41, abstract 인자가 0 이 아닌 배치 2 이다. (2026-11-09 WB04-E138)
const POP_PLACED = 0x41;
const POP_ABSTRACT = 2;
// 다리 품질 인자와 프레임 플래그, 단어의 수명 비트(3~6), 끝 칸 판정의 경계 글자 'K' 이다. (2026-11-09 WB04-E138)
const QUALITY_CRACKED = 1;
const QUALITY_HARD = 3;
const CRACKED_FLAG = 0x20;
const HARD_FLAG = 0x40;
const LIFE_MASK = 0x78;
const LAST_PLANK_SIDE = 0x4b;
// 0040e800 의 좌표 상한(256)이다. 0 초과 256 미만만 유효하다. (2026-11-09 WB04-E138)
const MAP_LIMIT = 256;
// 비용 부족 안내의 서식 key 와 고정 문구이다(패치 005071a0, 00507174, 00507188). (2026-11-09 WB04-E138)
const CLIENT_CHEATED = 'ClientCheated(%s)';
const SERVER_CHEATED = 'ServerCheated(%s)';
const SERVER_THINKS = 'ServerThinksYouCheated';
// 필수 하위 효과 이름이다(생성 때 모두 확인한다). assertion 은 실제 wire 경로에서만 필요하다. (2026-11-09 WB04-E138)
const HOOK_NAMES = Object.freeze(['take', 'notifySurface', 'setOwner', 'pop', 'sendMoney', 'storedSp', 'addLocalSp', 'addPlayerSp', 'format', 'tellOthers', 'tellPlayer', 'tell', 'sendReject']);
const TYPE_COUNT = 188;

// 건설 배치가 읽고 쓰는 전역과 Player 필드이다. 주소는 PATCH 기준이다. 값형 필드(독립 입력)와 shared* 함수(실제 Session 이 같은 전역을 빌려줌)를 함께 두고,
// typed place 는 값형 필드를, 실제 wire 경로와 SP 조회/환불/비용 부족 처리는 *Now() 접근자를 소비 위치마다 부른다(원본 C++ 와 같은 구분). (2026-11-09 WB04-E138)
export class ConstructionPlaceState {
    // 00540c70: 로컬 플레이어 번호이다.
    localPlayer = 0;
    // 00540bc0: 꺼져 있으면 번호 5 미만·서버 영역 SID 를 수신(take)해서 쓴다.
    server = true;
    // 00540bc4: 비용 부족 처리가 권한 쪽 안내/환불과 비권한 쪽 강제 차감 가운데 하나를 고른다.
    authority = true;
    // 00594fa4: 켜져 있으면 배치가 비용을 판정하지 않는다.
    chargeBlocked = false;
    // 00595344: 로컬 플레이어의 환불 가산을 건너뛰고 SP 조회의 번호 범위 검사를 건너뛴다.
    battleShown = false;
    // 005412cc: 시작 좌표의 칸에 단어 0x5d4 를 쓰는 타입, 005411a0: 품질 프레임과 수명 비트를 정하는 타입, 005412c0: 비용 판정/환불에서 빠지는 타입이다.
    noIslandType = 157;
    bridgeType = 82;
    nuggetType = 154;
    // 0052f960: 다리 수명 상수. 금 간 품질과 끝 칸은 여기서 1 을 뺀 값으로 시작한다.
    bridgeLife = 5;
    // Canon 디코더가 패턴으로 분기하는 여덟 타입 전역의 현재 번호(puzzlePiece, bridge, island, noIsland, 전투 넷)이다.
    patternTypes = [107, 82, 94, 157, 131, 129, 140, 142];
    // Player +0x54 의 표시 필드(0x400 은 비용 부족이 한 번 보고됐다는 표시)와 Player +0xc 의 이름(독립 값형 입력)이다.
    playerFlags = new Array(PLAYER_COUNT + 1).fill(0);
    playerNames = new Array(PLAYER_COUNT + 1).fill('');
    // 실제 00594fa4 의 현재 값을 돌려주는 함수(로컬 Player 자연 생략 뒤에만 읽음). 없으면 위 bool 을 쓴다.
    chargeBlockedRead = null;
    // 실제 Client server/boss/너겟/로컬 플레이어/PlayerKnown/다리/noIsland/다리 수명/패턴 타입 전역을 빌리는 함수들(없으면 null)이다.
    sharedServer = null;
    sharedAuthority = null;
    sharedNuggetType = null;
    sharedLocalPlayer = null;
    sharedPlayerKnown = null;
    sharedBridgeType = null;
    sharedNoIslandType = null;
    sharedBridgeLife = null;
    patternTypesRead = null;
    // 디코더가 쓰는 현재 checking·진단 장치(없으면 null)이다.
    decoderDiagnostics = null;

    // 각 조각의 take 관문에서 현재 서버 역할을 읽는다. (2026-11-09 WB04-E138)
    serverNow() { return this.sharedServer ? Boolean(this.sharedServer()) : this.server; }
    // 비용 부족 안내/환불 또는 차감 직전에 현재 권한을 읽는다. (2026-11-09 WB04-E138)
    authorityNow() { return this.sharedAuthority ? Boolean(this.sharedAuthority()) : this.authority; }
    // 비용 판정과 환불의 너겟 예외를 고르는 현재 번호이다. (2026-11-09 WB04-E138)
    nuggetTypeNow() { return this.sharedNuggetType ? this.sharedNuggetType() >>> 0 : this.nuggetType; }
    // 비용/환불/로컬 가산의 소비 위치에서 읽는 현재 로컬 플레이어이다. (2026-11-09 WB04-E138)
    localPlayerNow() { return this.sharedLocalPlayer ? this.sharedLocalPlayer() >>> 0 : this.localPlayer; }
    // 원문 현재 PlayerKnown 의 비영 전체를 사용한다. (2026-11-09 WB04-E138)
    battleShownNow() { return this.sharedPlayerKnown ? this.sharedPlayerKnown() !== 0 : this.battleShown; }
    // 표면 통지 뒤 품질 선택도 현재 번호이다. (2026-11-09 WB04-E138)
    bridgeTypeNow() { return this.sharedBridgeType ? this.sharedBridgeType() >>> 0 : this.bridgeType; }
    // 각 조각의 시작 칸 비교 위치에서 읽는 현재 번호이다. (2026-11-09 WB04-E138)
    noIslandTypeNow() { return this.sharedNoIslandType ? this.sharedNoIslandType() >>> 0 : this.noIslandType; }
    // 금 간 품질과 끝 칸 각각의 원문 소비 위치에서 읽는 현재 수명 상수이다. (2026-11-09 WB04-E138)
    bridgeLifeNow() { return this.sharedBridgeLife ? this.sharedBridgeLife() | 0 : this.bridgeLife | 0; }
    // 진단 복귀 뒤 디코더 생성 위치에서 읽는 현재 패턴 타입들이다. (2026-11-09 WB04-E138)
    currentPatternTypes() { return this.patternTypesRead ? this.patternTypesRead() : this.patternTypes; }
}

// 원본 __ftol 의 하위 signed DWORD 이다. NaN·무한대·64비트 범위 밖은 x87 정수 불확정값이라 하위 DWORD 가 0 이다. (2026-11-09 WB04-E138)
function ftol(value) {
    if (!Number.isFinite(value) || Math.abs(value) >= 9223372036854775808) return 0;
    const whole = Math.trunc(value);
    if (Math.abs(whole) <= 2147483647) return whole | 0;
    return Number(BigInt.asIntN(32, BigInt(whole)));
}

// 정렬되지 않은 리틀엔디언 값을 폭만큼 쓴다. (2026-11-09 WB04-E138)
function put(raw, offset, value, width) {
    for (let index = 0; index < width; index += 1) raw[offset + index] = Math.floor(value / 2 ** (8 * index)) & 0xff;
}

// 0040e800: 두 좌표가 모두 0 초과 지도 크기 미만인지 본다. NaN 은 유효하지 않다. (2026-11-09 WB04-E138)
function validPosition(x, y) {
    return x > 0 && x < MAP_LIMIT && y > 0 && y < MAP_LIMIT;
}

// 0049aa30: 정확한 플래그 값(signed byte)까지 일치하는 첫 프레임 코드를 찾는다. 번호는 이 검색의 조건이 아니다. 없으면 -1 이다. (2026-11-09 WB04-E138)
function findFlags(codes, side, variant, flags) {
    for (let index = 0; index < codes.length; index += 1) {
        const code = codes[index];
        if (code.side === side && code.variant === variant && ((code.flags << 24) >> 24) === flags) return index;
    }
    return -1;
}

// 건설 배치의 실행 계층이다. pool(10.78 SidPool)·types(현재 타입 표 188칸)·frames(타입별 { frames: 프레임 코드 배열, defaultFrame })·state·reward(같은 풀의 SquidReward)·hooks 는
// 이 객체보다 오래 살아야 하며 복사하지 않는다. 사용 순서: 호출자가 SID 를 확보하고 place 를 한 번 부른다. (2026-11-09 WB04-E138)
export class RawConstructionPlace {
    #pool;
    #types;
    #frames;
    #state;
    #reward;
    #hooks;
    #players = null;

    // 풀 일치와 필수 경계는 첫 효과 전에 검사한다. (2026-11-09 WB04-E138)
    constructor(pool, types, frames, state, reward, hooks) {
        if (pool?.edition !== 'patch1078' || pool !== reward?.pool) throw new TypeError('건설 배치 풀/필수 효과 연결 오류');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT || !Array.isArray(frames) || !state) throw new TypeError('건설 배치 풀/필수 효과 연결 오류');
        for (const name of HOOK_NAMES) {
            if (typeof hooks?.[name] !== 'function') throw new TypeError('건설 배치 풀/필수 효과 연결 오류');
        }
        this.#pool = pool;
        this.#types = types;
        this.#frames = frames;
        this.#state = state;
        this.#reward = reward;
        this.#hooks = hooks;
    }

    // 어댑터가 같은 실제 풀을 연결하는지 확인하는 데 쓰는 참조이다. (2026-11-09 WB04-E138)
    get pool() { return this.#pool; }

    // 타입 번호가 타입 표·프레임 표·타입 바이트 범위 안인지 검사하고 레코드를 돌려준다. (2026-11-09 WB04-E138)
    #type(type) {
        if (type >= this.#types.length || type >= this.#frames.length || type > 0xff) throw new RangeError('건설 배치 타입 번호 오류');
        return this.#types[type];
    }

    // 실제 Player 기록은 Session 보다 오래 살아야 하며 PATCH 의 9개 구조체 크기를 요구한다. 연결하면 표시 비트(+0x54)와 이름(+0xc)을 매 호출 직접 읽고 쓴다. (2026-11-09 WB04-E138)
    bindPlayerRecords(players) {
        if (!(players instanceof Uint8Array) || players.length !== (PLAYER_COUNT + 1) * PLAYER_STRIDE) throw new RangeError('건설 배치의 실제 Player 기록 크기 오류');
        this.#players = players;
    }

    // 004ac220: +0xc 단어를 쓰고 객체의 타입 바이트가 다리 타입이면 표면 알림을 부른다. (2026-11-09 WB04-E138)
    #setWord(sid, word) {
        const raw = this.#pool.allocated(sid);
        put(raw, WORD, word, 2);
        if (raw[TYPE] === this.#state.bridgeTypeNow()) this.#hooks.notifySurface(sid);
    }

    // 다리 조각의 품질 프레임 번호를 프레임 DWORD 에 쓰고, +0xc 단어의 수명 비트(3~6)만 life(signed)로 바꾼다. (2026-11-09 WB04-E138)
    #mixLife(raw, quality, life) {
        put(raw, FRAME, quality >>> 0, 4);
        const word = raw[WORD] | (raw[WORD + 1] << 8);
        const mixed = (word ^ ((((life >>> 0) << 3) & 0xffff ^ word) & LIFE_MASK)) & 0xffff;
        put(raw, WORD, mixed, 2);
    }

    // 조각 수를 먼저 세어 개수 불일치를 효과 전에 거부한 뒤 원본 순서(비용 판정 → 조각 순회)로 실행한다. (2026-11-09 WB04-E138)
    place(request) {
        const state = this.#state;
        this.#type(request.type);
        if (request.player === SERVER_PLAYER) throw new Error('건설 배치 플레이어가 서버 번호다(playerId != GAME_SERVER_ID assert)');
        const meta = this.#frames[request.type];
        const decoder = decodeCanonType(meta.frames, meta.defaultFrame | 0, state.patternTypes,
            { type: request.type, argument: request.argument | 0, direction: request.direction | 0, x: request.x, y: request.y, explicitFrame: false }, state.decoderDiagnostics);
        let pieces = 0;
        // 사본으로 유효 칸을 끝까지 세어 본다. 원본 반복자는 건드리지 않는다.
        for (const probe = decoder.clone(); probe.valid; probe.advance()) pieces += 1;
        if (pieces !== request.sids.length) throw new RangeError('건설 배치 SID 개수가 조각 수와 다르다(i == sidCount assert)');
        this.#chargeForPlacement(request);
        const abstract = request.abstract >>> 0;
        const popFlags = abstract ? POP_ABSTRACT : POP_PLACED;
        const codes = meta.frames;
        let index = 0;
        // 유효 칸마다 조각 하나를 놓는다. 한 조각의 쓰기 순서는 abstract 비트 → HP → 프레임 → 단어 → (다리 품질) → 소유자 → Pop 이다.
        for (; decoder.valid; decoder.advance()) {
            let sid = request.sids[index];
            index += 1;
            if (!state.serverNow() && (sid < FIRST_SID || sid >= SERVER_FIRST)) sid = this.#hooks.take(request.type, sid);
            const raw = this.#pool.allocated(sid);
            if ((raw[STATE] & VOID) === 0) throw new Error('건설 배치 조각이 void가 아니다(isVoid assert)');
            raw[EXTRA] = (raw[EXTRA] & 0xfe) | (abstract & 1);
            // HP 유무는 방금 쓴 abstract 비트를 읽는다(dais 부류). abstract 인자가 0 이 아니면 HP 0 으로 시작한다.
            if (this.#reward.hasHitPoints(sid)) put(raw, HP, abstract ? 0 : this.#reward.maxHitPoints(sid) >>> 0, 4);
            const frame = decoder.frame;
            put(raw, FRAME, frame >>> 0, 4);
            // noIsland 의 시작 좌표 칸만 단어 0x5d4 를 받는다.
            const x = decoder.x;
            const y = decoder.y;
            const origin = request.type === state.noIslandType && x === request.x && y === request.y;
            this.#setWord(sid, origin ? ORIGIN_WORD : request.argument & 0xffff);
            if (request.type === state.bridgeType) {
                if (frame < 0 || frame >= codes.length) throw new RangeError('다리 조각 프레임 범위 오류');
                let qualityFlags = 0;
                let life = 0;
                if (request.quality === QUALITY_CRACKED) { qualityFlags = CRACKED_FLAG; life = state.bridgeLife - 1; }
                else if (request.quality === QUALITY_HARD) qualityFlags = HARD_FLAG;
                // 방향 글자가 K 보다 뒤(끝 칸 L~O 등)이면 품질과 무관하게 수명이 하나 준다. 글자는 signed char 로 비교한다.
                const code = codes[frame];
                if (((code.side << 24) >> 24) > LAST_PLANK_SIDE) life = state.bridgeLife - 1;
                const quality = findFlags(codes, code.side, code.variant, (((code.flags << 24) >> 24) | qualityFlags) | 0);
                if (quality < 0) throw new Error('다리 품질 프레임이 타입에 없다(Rifttype.cpp assert)');
                this.#mixLife(raw, quality, life);
                this.#hooks.notifySurface(sid);
            }
            this.#hooks.setOwner(sid, request.player);
            this.#hooks.pop(sid, x, y, popFlags);
        }
    }

    // 실제 wire 배치는 예약/free 슬롯도 원문처럼 접근하며 풀 바깥의 호스트 메모리만 거부한다. (2026-11-09 WB04-E138)
    #actualBytes(sid) { return this.#pool.physical(sid); }

    // 진단 장치가 선택한 현재 바이트/전역을 바꿔도 정상 복귀 뒤 같은 배치 순서를 계속한다. 장치가 없으면 성공을 합성하지 않는다. (2026-11-09 WB04-E138)
    #assertActual(expression, file, line) {
        if (typeof this.#hooks.assertion !== 'function') throw new Error('실제 건설 배치의 Client 진단 장치 미연결');
        this.#hooks.assertion(expression, file, line);
    }

    // 고정 scalar/count 와 lazy SID 읽기의 실제 실행 경로이며 typed place 의 선행 전수 검사는 사용하지 않는다. (2026-11-09 WB04-E138)
    actualPlace(request, declaredCount, sidReader) {
        const state = this.#state;
        if (typeof sidReader !== 'function') throw new TypeError('실제 건설 배치의 현재 SID reader 없음');
        if (request.type >= this.#types.length) throw new RangeError('실제 건설 배치 TYPE 물리 범위');
        // 49a840 은 player 진단/디코더보다 먼저 고른 같은 TYPE 레코드 주소다.
        const selectedType = this.#types[request.type];
        if (request.player === SERVER_PLAYER) this.#assertActual('playerId != GAME_SERVER_ID', '\\Ns\\O\\Construction.cpp', 0x273);
        if (request.type >= this.#frames.length) throw new RangeError('실제 건설 배치 frame TYPE 물리 범위');
        const meta = this.#frames[request.type];
        // 진단 복귀 뒤 현재 패턴 번호로 생성하되 반복자는 한 번만 만든다.
        const decoder = decodeCanonType(meta.frames, meta.defaultFrame | 0, state.currentPatternTypes(),
            { type: request.type, argument: request.argument | 0, direction: request.direction | 0, x: request.x, y: request.y, explicitFrame: false }, state.decoderDiagnostics);
        const player = request.player | 0;
        if (request.player !== state.localPlayerNow() && !this.#chargeBlocked() && player > 0 && player < SERVER_PLAYER && selectedType.cost !== 0 && request.type !== state.nuggetTypeNow()) {
            // SP 조회 뒤 비용은 진입 때 선택한 같은 TYPE 에서 다시 읽는다.
            const money = this.money(request.player);
            if (!(money >= selectedType.cost)) this.chargeShortfall(request.type, request.player, request.x, request.y, request.argument, request.direction);
            else this.addMoney(request.player, -selectedType.cost);
        }
        const abstract = request.abstract >>> 0;
        const popFlags = abstract ? POP_ABSTRACT : POP_PLACED;
        // 패킷의 선언 개수와 별도로 디코더가 실제 소비한 SID 수다.
        let index = 0;
        // 개수 선검사 없이 비용/앞 조각의 부분 효과 뒤 다음 현재 SID WORD 를 읽는다.
        for (; decoder.valid; decoder.advance()) {
            let sid = sidReader(index);
            index += 1;
            if (!state.serverNow() && (sid < FIRST_SID || sid >= SERVER_FIRST)) sid = this.#hooks.take(request.type, sid);
            let raw = this.#actualBytes(sid);
            if ((raw[STATE] & VOID) === 0) {
                this.#assertActual('isVoid()', 'c:\\ns\\o\\squid.h', 0x282);
                raw = this.#actualBytes(sid);
            }
            raw[EXTRA] = (raw[EXTRA] & 0xfe) | (abstract & 1);
            if (this.#reward.hasHitPoints(sid)) put(this.#actualBytes(sid), HP, abstract ? 0 : this.#reward.maxHitPoints(sid) >>> 0, 4);
            // 디코더가 보유한 현재 frame 을 쓴다.
            const frame = decoder.frame;
            put(this.#actualBytes(sid), FRAME, frame >>> 0, 4);
            const x = decoder.x;
            const y = decoder.y;
            // 원문의 ordered 좌표 같음이다.
            const origin = request.type === state.noIslandTypeNow() && x === request.x && y === request.y;
            put(this.#actualBytes(sid), WORD, origin ? ORIGIN_WORD : request.argument & 0xffff, 2);
            // 4ac220 은 방금 쓴 조각의 현재 TYPE 와 현재 다리 번호를 비교한다.
            if (this.#actualBytes(sid)[TYPE] === state.bridgeTypeNow()) this.#hooks.notifySurface(sid);
            if (request.type === state.bridgeTypeNow()) {
                let qualityFlags = 0;
                let life = 0;
                if (request.quality === QUALITY_CRACKED) { qualityFlags = CRACKED_FLAG; life = ((state.bridgeLifeNow() - 1) << 24) >> 24; }
                else if (request.quality === QUALITY_HARD) qualityFlags = HARD_FLAG;
                const bridge = state.bridgeTypeNow();
                if (bridge >= this.#frames.length) throw new RangeError('실제 건설 품질의 현재 다리 TYPE 물리 범위');
                // 표면 통지 뒤 현재 다리 TYPE 의 코드 표를 선택한다.
                const codes = this.#frames[bridge].frames;
                if (frame < 0 || frame >= codes.length) throw new RangeError('실제 건설 품질의 코드 물리 범위');
                const code = codes[frame];
                if (((code.side << 24) >> 24) > LAST_PLANK_SIDE) life = ((state.bridgeLifeNow() - 1) << 24) >> 24;
                // 같은 current code/품질로 원문 49aa30 을 찾는다.
                const quality = findFlags(codes, code.side, code.variant, (((code.flags << 24) >> 24) | qualityFlags) | 0);
                // 원문 진단 복귀 뒤 반환값 -1 도 실제 frame 에 쓴다.
                if (quality < 0) this.#assertActual('0', '\\Ns\\O\\Rifttype.cpp', 0x16a);
                raw = this.#actualBytes(sid);
                this.#mixLife(raw, quality, life);
                this.#hooks.notifySurface(sid);
            }
            this.#hooks.setOwner(sid, request.player);
            this.#hooks.pop(sid, x, y, popFlags);
        }
        // 모든 디코더 칸의 부분 효과가 끝난 뒤 진단한다.
        if (index !== declaredCount) this.#assertActual('i == sidCount', '\\Ns\\O\\Construction.cpp', 0x2f2);
    }

    // 현재 비용 판정 차단 전역(00594fa4)이다. 읽는 함수가 있으면 그 현재 값, 없으면 값형 bool 이다. (2026-11-09 WB04-E138)
    #chargeBlocked() {
        const state = this.#state;
        return state.chargeBlockedRead ? Boolean(state.chargeBlockedRead()) : state.chargeBlocked;
    }

    // 로컬 플레이어·차단 전역·범위 밖 번호·비용 0·nugget 은 판정하지 않는다. 잔액 비교는 x87 과 같이 NaN 을 "모자람"으로 본다. (2026-11-09 WB04-E138)
    #chargeForPlacement(request) {
        const state = this.#state;
        const player = request.player | 0;
        // 같은 원문의 short-circuit 순서에서만 현재 전역을 조회한다.
        if (request.player === state.localPlayer || this.#chargeBlocked() || player <= 0 || player >= SERVER_PLAYER) return;
        const cost = this.#types[request.type].cost;
        if (cost === 0 || request.type === state.nuggetTypeNow()) return;
        if (!(this.money(request.player) >= cost)) this.chargeShortfall(request.type, request.player, request.x, request.y, request.argument, request.direction);
        else this.addMoney(request.player, -cost);
    }

    // Player 이름을 현재 위치에서 읽는다(연결된 실제 기록이면 +0xc 부터 NUL 까지, 아니면 값형 입력). (2026-11-09 WB04-E138)
    #playerName(player, offset) {
        if (this.#players === null) return this.#state.playerNames[player];
        const begin = offset + NAME_AT;
        const end = offset + NAME_END;
        let terminator = begin;
        while (terminator < end && this.#players[terminator] !== 0) terminator += 1;
        // 정상 Player 이름의 종료를 인접 필드까지 넘겨 찾지 않는다.
        if (terminator === end) throw new RangeError('건설 비용 안내 Player 이름의 NUL 누락');
        return String.fromCharCode(...this.#players.subarray(begin, terminator));
    }

    // 00442b50: 표시 비트를 먼저 읽어 두고 켠 뒤 권한 여부로 갈린다. 권한 쪽은 SP 를 빼지 않고 환불/취소 통지를 보낸다. (2026-11-09 WB04-E138)
    chargeShortfall(type, player, x, y, argument, direction) {
        const state = this.#state;
        const cost = this.#type(type).cost;
        if (player > PLAYER_COUNT) throw new RangeError('건설 비용 처리 플레이어 번호 오류');
        const offset = player * PLAYER_STRIDE;
        let flags = state.playerFlags[player];
        if (this.#players !== null) {
            flags = 0;
            // 현재 비정렬 DWORD 를 리틀엔디언으로 읽는다.
            for (let index = 0; index < 4; index += 1) flags = (flags + this.#players[offset + 0x54 + index] * 2 ** (8 * index)) >>> 0;
        }
        const reported = (flags & SHORTFALL_FLAG) !== 0;
        flags = (flags | SHORTFALL_FLAG) >>> 0;
        if (this.#players === null) state.playerFlags[player] = flags;
        else put(this.#players, offset + 0x54, flags, 4);
        if (state.authorityNow()) {
            // 권한 쪽: 처음이면 다른 플레이어에게 알리고, 당사자에게 알린 뒤 좌표가 유효하면 환불과 취소 통지를 보낸다.
            if (!reported) this.#hooks.tellOthers(player, this.#hooks.format(CLIENT_CHEATED, this.#playerName(player, offset)));
            this.#hooks.tellPlayer(player, SERVER_THINKS);
            if (validPosition(x, y)) this.reject(type, player, x, y, argument, direction);
            return;
        }
        // 비권한 쪽: 잔액과 무관하게 차감하고 처음이면 로컬 안내를 띄운다.
        this.addMoney(player, -cost);
        if (!reported) this.#hooks.tell(this.#hooks.format(SERVER_CHEATED, this.#playerName(player, offset)));
    }

    // 00442a40: 비용을 정수로 절삭해 돌려주고 양수이면 SP 통지를 보낸다. (2026-11-09 WB04-E138)
    refund(type, player) {
        const state = this.#state;
        const record = this.#type(type);
        if (type === state.nuggetTypeNow()) return;
        const amount = ftol(record.cost);
        // SP 를 돌려준다. 전투 표시 중의 로컬 플레이어는 가산을 건너뛴다.
        if (!(state.battleShownNow() && player === state.localPlayerNow())) this.addMoney(player, Math.fround(amount));
        if (amount > 0) this.#hooks.sendMoney(player, { player: player & 0xff, source: player & 0xff, kind: 1, amount });
    }

    // 00442af0: 환불 뒤 좌표·타입·두 인자의 하위 바이트를 실은 취소 통지를 보낸다. (2026-11-09 WB04-E138)
    reject(type, player, x, y, argument, direction) {
        this.refund(type, player);
        this.#hooks.sendReject(player, { x, y, type: type & 0xff, argument: argument & 0xff, direction: direction & 0xff });
    }

    // 0040ef30: 전투 표시 중의 로컬 플레이어는 번호 검사 없이, 그 밖에는 1~8 일 때만 저장소를 읽는다. (2026-11-09 WB04-E138)
    money(player) {
        const state = this.#state;
        if (state.battleShownNow() && player === state.localPlayerNow()) return this.#hooks.storedSp(state.localPlayerNow());
        const number = player | 0;
        return number > 0 && number < SERVER_PLAYER ? this.#hooks.storedSp(player) : 0;
    }

    // 0041dff0: 로컬 플레이어이면 로컬 가산, 아니면 플레이어 가산으로 넘긴다. (2026-11-09 WB04-E138)
    addMoney(player, delta) {
        if (player === this.#state.localPlayerNow()) this.#hooks.addLocalSp(delta);
        else this.#hooks.addPlayerSp(player, delta);
    }
}

// 같은 풀의 실제 수신/소유자/SP 모듈만 바꿔 끼운다. Pop·표면 알림·통지 경계는 호출자가 준 것을 유지한다. 풀이 다르거나 SP 저장소가 없으면 거부한다. (2026-11-09 WB04-E138)
export function makeConstructionPlaceHooks(pool, factory, owner, reward, store, state, hooks) {
    if (pool !== factory?.pool || pool !== owner?.pool || pool !== reward?.pool || !store) throw new TypeError('건설 배치/factory/소유자/보상 SID 풀 또는 SP 저장소 연결 오류');
    const view = new DataView(new ArrayBuffer(4));
    return {
        ...hooks,
        take: (type, sid) => factory.take(type, sid),
        setOwner: (sid, player) => { owner.set(sid, player); },
        storedSp: (player) => { view.setUint32(0, store.get(player) >>> 0, true); return view.getFloat32(0, true); },
        // 실제 공유 로컬 번호는 가산 선택 뒤에도 같은 소비 위치에서 읽는다.
        addLocalSp: (delta) => { reward.addSp(state.localPlayerNow(), delta); },
        addPlayerSp: (player, delta) => { reward.addSp(player, delta); },
    };
}
