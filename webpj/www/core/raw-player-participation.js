// PATCH 10.78 의 실제 로그인·방향별 동맹·AI 부착/해제에서 Player 기록의 쓰기와 효과 순서를 옮긴 모듈이다(cpppj `o/RawPlayerParticipation.cpp/.h`).
// 저장소: players 는 9×0xb4(180) BYTE Player 기록, allies 는 원본 9×9 DWORD 방향표(한쪽 방향만 바뀔 수 있음), hidden 은 0052f81e 의 현재 9개 숨김 표시이다.
// 배열 사본/대칭 동맹/추정 active 입력을 만들지 않는다. Ready 와 사람 연결/AI 참여를 같은 bool 로 합치지 않는다.
//  - checkLogin(004a6dc0): 유효 owner 1..8 의 flags(+0x50) 값 1 비트 부재에서만 flags=(flags&~0x100)|3 뒤 연결을 수락(로컬 전송에서 pending==owner 가 아니면 assignConnection)하고,
//    이미 로그인/무효 owner 는 거부 응답→연결 해제를 수행한다.
//  - setAlliance(00490ea0): 자기 bit 를 포함한 +0x54 mask 와 현재 한 행 9칸만 쓰고, playerKnown 이면 채널 0x40/0x80 을 지우고 양방향 동맹이 모두 참인 Player 를 0x40, 숨김이면 0x80 에 다시 등록한다.
//  - addAlly(0048f6a0): 지정 방향의 한 bit/한 cell 만 켜며 battle 이면 현재 mask 를 서버(9)에 요청한다. receiveAlliance(00491220): 같은 수신 packet 의 자기 bit→Player mask→방향표/Chat→broadcast 순서.
//  - attachBrain(004911a0)/removeBrain(00413ae0/00413b20): 실제 생성 소유자가 전달한 opaque32 brain 을 +0xb0 에 기록하고 flags 0x20 을 켜며(기존 brain 은 assert 만 보고 덮어씀),
//    제거는 파괴→반납→+0xb0=0 이고 flags 0x20 은 원본대로 지우지 않는다. 가짜 brain 생성/등록은 제공하지 않는다.
//  - removeBrainForState/returnBrainForState/brainReturnedForState: State cleanup 의 owner 1..8 위치에서 정상 복귀한 뒤에만 발급되는 **이동 전용 증빙(BrainResetPass)**을 같은 모듈이 한 번만 소비하며
//    소비한 owner 의 새 brain 설치/반복 reset 을 닫는다(C++ 수명 증빙의 JS 대응: 발급 모듈 동일성 + 1회 소비 + 파괴/반납 효과 진행 중 소비 금지).
// 근거: cpppj 기준선 1f00a25a, 원본 관측 player-participation-x86.tsv(608행)·player-alliance-packet-x86.tsv(288행, tests/player-participation.test.mjs). 10.78 전용이다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E5: Player 참여/동맹/AI 부착 이식으로 추가)

// 원본의 진단 후 계속 진행하는 두 assertion 경계이다. (2026-11-08 WB04-E5)
export const PLAYER_PARTICIPATION_ASSERTION = Object.freeze({ brainAlreadyAssigned: 'brainAlreadyAssigned', metamapMode: 'metamapMode' });

// 각 함수가 실제 도달 시 읽는 값형 전역이다(독립 대조용). 실제 세션은 bindCurrentInputs 로 현재 조회를 공급한다. (2026-11-08 WB04-E5)
export class PlayerParticipationState {
    // 00540c70/00595344 의 현재 DWORD 와 00594fbc/00594fc8/0054db50 의 서로 다른 모드이다.
    localOwner = 1;
    playerKnown = 0;
    battle = false;
    metamap = false;
    localTransport = false;
}

// Client 수명의 동맹 요청 제한 상태다. 입력 처리기/협상 수신 연결은 기존 3.6③ 6단계에 남긴다. (2026-11-14 WB04-E453)
export class AllianceRequestState {
    // 5caf44의 counter9·5caf68의 sample/start9쌍·5cafb0 initialized를 같은 112BYTE BSS로 소유한다.
    #memory = new ArrayBuffer(112);
    // 같은 물리의 DWORD/행 뷰이며 미션/창 교체 때 재할당하지 않는다.
    #counters = new Uint32Array(this.#memory, 0, 9);
    #clocks = Array.from({ length: 9 }, (_, index) => new Uint32Array(this.#memory, 36 + index * 8, 2));
    // 마지막 initialized DWORD를 읽고 쓰는 리틀엔디언 뷰다.
    #view = new DataView(this.#memory);
    // PE 540bc8/bcc의 signed 초기값이다. BSS 시간 표와 별개인 현재 설정이다.
    windowSeconds = 60; limit = 3;
    // 원문 요청 counter9를 값 사본 없이 빌린다.
    get counters() { return this.#counters; }
    // 각 행의 sampled/start가 같은 72BYTE에 이어지는 실제 뷰다.
    get clocks() { return this.#clocks; }
    // 첫 요청의 아홉 clock store 뒤 쓰는 최초 BSS 0 DWORD다.
    get initialized() { return this.#view.getUint32(108, true); }
    // 입력 값은 DWORD로 감겨 같은 실제 BSS에 기록된다.
    set initialized(value) { this.#view.setUint32(108, value, true); }
}

// Player 한 기록 폭·개수와 통신/동맹/brain/구독자 필드 위치(10.78)이다. (2026-11-08 WB04-E5)
const STRIDE = 0xb4, COUNT = 9, FLAGS = 0x50, ALLIES = 0x54, BRAIN = 0xb0, SUBSCRIBER = 0x2c;

// 증빙 발급/소비를 모듈 안에서만 하도록 정적 블록에서 채우는 함수이다. (2026-11-08 WB04-E5)
let issuePass;
let readPass;
let consumePass;

// 정상 복귀한 State AI 반납의 이동 전용 증빙이다. 외부에서 만들 수 없고(발급은 RawPlayerParticipation 만) 한 번만 소비된다.
export class BrainResetPass {
    #source;
    #lifetime;
    #owner;
    #spent = false;

    static {
        issuePass = (source, lifetime, owner) => new BrainResetPass(source, lifetime, owner);
        readPass = (pass) => (pass instanceof BrainResetPass ? { source: pass.#source, lifetime: pass.#lifetime, owner: pass.#owner, spent: pass.#spent } : null);
        consumePass = (pass) => { pass.#spent = true; };
    }

    constructor(source, lifetime, owner) {
        this.#source = source;
        this.#lifetime = lifetime;
        this.#owner = owner;
    }

    // 원문 resetAi 가 정상 복귀한 1..8 번호만 순수 조회한다.
    get owner() { return this.#owner; }
}

// 선택된 효과 장치를 꺼낸다. 없으면 그 위치에서 오류이다. (2026-11-08 WB04-E5)
function require(device, name) {
    if (typeof device !== 'function') throw new Error(`Player 참여 초기화의 실제 외부효과(${name}) 미연결`);
    return device;
}

// Player 참여 처리기이다. hooks: { assertion(kind), assignConnection(pending, owner), rejectLogin(pending), disconnect(pending), clearChat(channel), addChat(subscriber, name, channel),
// sendAlliance(server, player, mask), destructBrain(token), freeBrain(token) }. 사용: new RawPlayerParticipation(players, allies, hidden, state, hooks). (2026-11-08 WB04-E5)
export class RawPlayerParticipation {
    #players;
    #allies;
    #hidden;
    #state;
    #hooks;
    // 실제 세션이 바인딩한 현재 전역 getter(값형 fixture 와 구분)이다.
    #reads = null;
    // 증빙보다 모듈이 먼저 소멸한 경우를 식별하는 모듈 정체성과, 소비한 정상 복귀 증빙(owner 0 은 사용하지 않음), 파괴/반납 효과가 진행 중인 호출 수이다.
    #lifetime = {};
    #returned = new Array(COUNT).fill(null);
    #activeBrainCalls = 0;

    // 실제 한 저장소의 물리 범위를 변경 전에 확인한다.
    constructor(players, allies, hidden, state, hooks) {
        if (players.length !== STRIDE * COUNT || allies.length !== COUNT * COUNT || hidden.length !== COUNT) throw new RangeError('Player 참여 초기화의 PATCH 공유 저장소 크기 오류');
        this.#players = players;
        this.#allies = allies;
        this.#hidden = hidden;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 다섯 현재 조회를 모두 공급한다({ localOwner, playerKnown, battle, metamap, localTransport } 함수). 부분 바인딩은 첫 변경 전에 거부한다.
    bindCurrentInputs(reads) {
        for (const name of ['localOwner', 'playerKnown', 'battle', 'metamap', 'localTransport']) {
            if (typeof reads?.[name] !== 'function') throw new TypeError('Player 참여 현재 전역 조회 다섯 개 누락');
        }
        this.#reads = reads;
    }

    // 값형 원본 fixture 를 보존하며 실제 세션에서는 현재 값을 직접 읽는다.
    #localOwner() { return (this.#reads ? this.#reads.localOwner() : this.#state.localOwner) >>> 0; }

    #playerKnown() { return (this.#reads ? this.#reads.playerKnown() : this.#state.playerKnown) >>> 0; }

    #battle() { return Boolean(this.#reads ? this.#reads.battle() : this.#state.battle); }

    #metamap() { return Boolean(this.#reads ? this.#reads.metamap() : this.#state.metamap); }

    #localTransport() { return Boolean(this.#reads ? this.#reads.localTransport() : this.#state.localTransport); }

    // 0번 중립 기록도 동맹/초기화의 실제 입력이므로 허용한다. 범위 밖은 RangeError.
    #player(owner) {
        if (!Number.isInteger(owner) || owner < 0 || owner >= COUNT) throw new RangeError('Player 참여 번호 범위 오류');
        return this.#players.subarray(owner * STRIDE, (owner + 1) * STRIDE);
    }

    // 정렬되지 않은 원본 DWORD 를 읽고 쓴다.
    #read(owner, offset) {
        const raw = this.#player(owner);
        return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
    }

    #put(owner, offset, value) {
        const raw = this.#player(owner);
        for (let index = 0; index < 4; index += 1) raw[offset + index] = (value >>> (8 * index)) & 0xff;
    }

    // Player+0xc 의 NUL 종료 이름(32 BYTE 안)을 이진 문자열로 읽는다. NUL 이 없으면 RangeError.
    #name(owner) {
        const raw = this.#player(owner);
        let length = 0;
        while (length < 32 && raw[12 + length]) length += 1;
        if (length === 32) throw new RangeError('Player 참여 이름의 NUL 종료 없음');
        return String.fromCharCode(...raw.subarray(12, 12 + length));
    }

    // 004a6dc0: 유효 owner 1..8 의 값 1 비트 부재에서만 flags=(flags&~0x100)|3 뒤 연결을 수락한다(값 1·2 두 비트를 켜고 0x100 만 지우며 다른 flag 는 보존).
    // 이미 로그인/무효 owner 는 거부 응답→연결 해제를 수행한다. pending/owner 는 signed 32비트이다.
    checkLogin(pending, owner) {
        const target = owner | 0;
        if (target > 0 && target < COUNT && !(this.#read(target, FLAGS) & 1)) {
            this.#put(target, FLAGS, ((this.#read(target, FLAGS) & ~0x100) | 3) >>> 0);
            if (!(this.#localTransport() && (pending | 0) === target)) require(this.#hooks.assignConnection, 'assignConnection')(pending | 0, target);
            return true;
        }
        require(this.#hooks.rejectLogin, 'rejectLogin')(pending | 0);
        require(this.#hooks.disconnect, 'disconnect')(pending | 0);
        return false;
    }

    // 00490ea0: 서로 다른 방향은 원본처럼 독립이다. Chat 은 양방향 동맹이 모두 참인 때만 0x40 채널에 등록한다.
    setAlliance(owner, mask) {
        this.#player(owner);
        const value = (mask | (1 << (owner & 31))) >>> 0;
        this.#put(owner, ALLIES, value);
        // 원본은 한 행의 9개 DWORD 를 각각 0/1 로 갱신한다.
        for (let target = 0; target < COUNT; target += 1) this.#allies[owner * COUNT + target] = (value >>> target) & 1;
        if (!this.#playerKnown()) return;
        require(this.#hooks.clearChat, 'clearChat')(0x40);
        require(this.#hooks.clearChat, 'clearChat')(0x80);
        // 채널 효과 뒤 현재 로컬 번호/표/Player 를 다시 읽어 다음 owner 를 처리한다.
        for (let player = 0; player < COUNT; player += 1) {
            if (this.#localOwner() >= COUNT) throw new RangeError('Player 참여 Chat의 로컬 번호 오류');
            if (this.#allies[this.#localOwner() * COUNT + player] && this.#allies[player * COUNT + this.#localOwner()]) require(this.#hooks.addChat, 'addChat')(this.#read(player, SUBSCRIBER), this.#name(player), 0x40);
            if (this.#metamap()) require(this.#hooks.assertion, 'assertion')(PLAYER_PARTICIPATION_ASSERTION.metamapMode);
            if (this.#hidden[player]) require(this.#hooks.addChat, 'addChat')(this.#read(player, SUBSCRIBER), this.#name(player), 0x80);
        }
    }

    // 0048f6a0: 지정 방향의 한 bit/한 cell 만 켜며 battle 이면 현재 mask 를 서버 9 에 요청한다. metamap 의 진단은 원본 getter 호출마다 유지한다.
    addAlly(owner, target) {
        this.#player(owner);
        this.#player(target);
        this.#put(owner, ALLIES, (this.#read(owner, ALLIES) | (1 << (target & 31))) >>> 0);
        if (this.#metamap()) require(this.#hooks.assertion, 'assertion')(PLAYER_PARTICIPATION_ASSERTION.metamapMode);
        const battle = this.#battle();
        this.#allies[owner * COUNT + target] = 1;
        if (battle) {
            if (this.#metamap()) require(this.#hooks.assertion, 'assertion')(PLAYER_PARTICIPATION_ASSERTION.metamapMode);
            require(this.#hooks.sendAlliance, 'sendAlliance')(9, owner, this.#read(owner, ALLIES));
        }
    }

    // 00491220: 원본의 수신 버퍼 직접 쓰기와 같은 Player/방향별 동맹/현재 Chat 재구성을 실행한다. packet 은 9 BYTE 이상이어야 하며 broadcast(packet)는 변경된 같은 바이트를 읽는다.
    receiveAlliance(packet, broadcast) {
        if (packet.length < 9) throw new RangeError('동맹 수신의 실제 9BYTE 미만 입력');
        // 원본 유효 0..8 owner 계약을 변경 전에 확인한다.
        const owner = packet[4];
        this.#player(owner);
        let mask = (1 << (owner & 31)) >>> 0;
        // 비정렬 packet+5 의 현재 DWORD 를 원본 순서로 조립한다.
        for (let index = 0; index < 4; index += 1) mask = (mask | (packet[5 + index] << (index * 8))) >>> 0;
        // 원본은 입력 packet 에도 self bit 를 기록하므로 broadcast 가 같은 변경 바이트를 읽는다.
        for (let index = 0; index < 4; index += 1) packet[5 + index] = (mask >>> (index * 8)) & 0xff;
        this.#put(owner, ALLIES, mask);
        this.setAlliance(owner, mask);
        require(broadcast, 'broadcast')(packet);
    }

    // 004911a0: 실제 생성 소유자가 전달한 opaque32 brain 을 +b0 에 기록하고 flags 20 을 켠다. 기존 brain 은 assert 만 보고 덮어쓰며 임의로 해제하지 않는다. 0 입력도 원본 필드 쓰기와 같다.
    attachBrain(owner, brain) {
        // 실제 할당/필드 쓰기로 수명 증빙을 무효화하지 않는다.
        if (this.brainReturnedForState(owner)) throw new Error('Native 반납된 AI owner의 새 brain 설치 금지');
        if (this.#read(owner, BRAIN)) require(this.#hooks.assertion, 'assertion')(PLAYER_PARTICIPATION_ASSERTION.brainAlreadyAssigned);
        this.#put(owner, BRAIN, brain >>> 0);
        this.#put(owner, FLAGS, (this.#read(owner, FLAGS) | 0x20) >>> 0);
        if ((this.#read(owner, SUBSCRIBER) | 0) < 1) {
            if (this.#metamap()) require(this.#hooks.assertion, 'assertion')(PLAYER_PARTICIPATION_ASSERTION.metamapMode);
            this.#put(owner, SUBSCRIBER, owner + 10000);
            // 원본은 DWORD 7개와 마지막 WORD 를 써 +30..4d 의 30 BYTE 만 지운다.
            this.#player(owner).fill(0, 0x30, 0x4e);
        }
    }

    // 현재 실제 소유자의 토큰만 읽는다. flags 20 이나 연결 상태로 포인터를 추정하지 않는다.
    brain(owner) { return this.#read(owner, BRAIN); }

    // 00413ae0/00413b20: 파괴/반납 callback 시점에는 원본처럼 이전 포인터를 Player 에서 계속 읽을 수 있다. 실제 파괴→반납→+b0=0, flags 20 은 지우지 않는다.
    removeBrain(owner) {
        if (this.brainReturnedForState(owner)) throw new Error('Native 반납된 AI owner의 반복 reset 금지');
        const token = this.brain(owner);
        if (!token) return;
        this.#activeBrainCalls += 1;
        try {
            require(this.#hooks.destructBrain, 'destructBrain')(token);
            require(this.#hooks.freeBrain, 'freeBrain')(token);
            this.#put(owner, BRAIN, 0);
        } finally {
            this.#activeBrainCalls -= 1;
        }
    }

    // 원문 cleanup 의 owner 1..8 위치에서 같은 실제 부모를 끝낸 뒤에만 증빙을 만든다(0 번 중립 AI 는 대상이 아님).
    removeBrainForState(owner) {
        if (!Number.isInteger(owner) || owner < 1 || owner >= COUNT) throw new RangeError('State AI 반납의 실제 owner1..8 범위');
        this.removeBrain(owner);
        return issuePass(this, this.#lifetime, owner);
    }

    // 정상 복귀와 현재 모듈의 같은 대여만 소비한다. 실패한 앞선 효과나 다른 owner 상태는 복구하지 않는다.
    returnBrainForState(pass) {
        const data = readPass(pass);
        if (!data || data.source !== this || data.lifetime !== this.#lifetime || data.owner < 1 || data.owner >= COUNT) throw new TypeError('같은 실제 AI 모듈의 정상 reset 증빙 필요');
        if (data.spent) throw new Error('이미 소비한 reset 증빙');
        if (this.#activeBrainCalls) throw new Error('진행 중 AI 파괴/반납 효과의 증빙 소비 금지');
        if (this.#returned[data.owner] || this.brain(data.owner)) throw new Error('현재 brain0인 미반납 AI owner 증빙 필요');
        consumePass(pass);
        this.#returned[data.owner] = pass;
    }

    // 실제 증빙이 없는 일반 경로는 종전대로 동작하며 raw 필드 외부 변조도 새 brain 을 숨기지 않는다.
    brainReturnedForState(owner) {
        this.#player(owner);
        if (!this.#returned[owner]) return false;
        if (this.brain(owner)) throw new Error('반납된 AI owner의 현재 brain 정체성 변경');
        return true;
    }

    // 0 번 중립 AI 는 이 원본 전체 삭제 루프의 대상이 아니다. owner 1..8 의 현재 포인터를 각 방문 시점에 새로 읽는다.
    removeAllBrains() {
        for (let owner = 1; owner < COUNT; owner += 1) {
            // 이미 State 가 반납한 NULL 도 원문처럼 자연 반환한다. 증빙이 있어도 현재 brain 변경은 실제 조회가 거부하며 전체 루프를 생략하지 않는다.
            if (!this.brainReturnedForState(owner)) this.removeBrain(owner);
        }
    }
}
