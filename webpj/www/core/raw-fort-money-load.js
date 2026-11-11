// Fort `Money` 섹션 읽기와 현재 Player 의 분산 SP 지정 정책을 옮긴 모듈이다(cpppj `o/RawFortMoneyLoad.cpp/.h`, 원본 패치 004bf340 → 00483300 · 00447810 · 0040ef30, 10.78 전용).
// 이 모듈은 저장 금액(float)을 SP 로 대입하지 않는다. 섹션에서는 실제 4바이트만 소비하고, 현재 모드(fort 고정 SP·해체 절반 HP 모드·경제 차단·난이도)에 따라 분산 SP 저장소에 쓴다.
//  - ReadMoney(00483300): position 을 4 더해(32비트 감김) signed 비교로 끝을 넘으면 진단 → fort 고정 SP 면 현재 로컬 owner 에 194162.0f 를 쓰고 →
//    절반 HP 모드이고 경제 차단이 아니면 난이도(0~2)로 1000/2500/3000 에 4000 을 더한 값을 owner 1..8 중 flags(+0x50)의 0x20 비트가 없는 Player 에 지정한다.
//  - SetPlayerSp(00447810): 활성 로컬 Client 이고 owner 가 현재 로컬 owner 와 같으면 편집기(005c85a4) 값에 따라 194162.0f 또는 amount 를 float 로 바꿔 쓰고 SP 표시를 갱신,
//    그 밖에는 signed owner 1~8 일 때만 같은 값을 쓴다(표시 갱신 없음). 이 지정 몸체는 AI 지갑 가산이나 SP 차감용 AddSp 를 쓰지 않는다.
//  - LoadSelected(004bf340): 패치에서 절반 HP 모드이고 경제 차단이 아니면 SP 검사 잔여 횟수를 99 로 쓰고, 깊이를 올려 알린 뒤 Template 없음 진단 → 실제 섹션 선택 → ReadMoney,
//    예외가 나도 같은 깊이로 되돌려 알린 뒤 다시 던진다. 실제 manager 의 섹션 선택(위치 0 초기화)은 selector 가 맡는다.
//  - readPlayerSpAmount(0040ef30): 활성 로컬이고 owner 가 현재 로컬 owner 와 같으면 로컬 owner 를 다시 조회해 그 잔액을, 그 밖에는 signed 1~8 일 때만 분산 잔액을 float 로 읽고 아니면 0.
// 현재 모드/owner/편집기/활성 여부는 모두 호출 위치에서 다시 읽는다(수명 사본을 만들지 않음). CD 10.72 분기(전역 float cdLocalSp·cdStartingSp)는 웹 목표가 10.78 이라 옮기지 않는다.
// 근거: cpppj `o/RawFortMoneyLoad.*`(기준선 1f00a25a, 기준선 뒤 2b048419 의 FixedSpA/FixedSpB/HalfHealthMode 접근자 동기화는 웹의 SquidRewardState.current*() 와 같은 의미). 원본 관측: `cpppj/tests/fixtures/fortmoney-x86.tsv` 중 originals 244행.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E278: 4-B Fort Money 읽기와 Player SP 지정 정책 이식)

// 원본 진단이 알리는 두 종류이다(진단 뒤 계속). (2026-11-11 WB04-E278)
export const FORT_MONEY_ASSERTION = Object.freeze({ missingTemplate: 'MissingTemplate', streamPastEnd: 'StreamPastEnd' });

// 패치 0041c7d0 의 난이도 0..2 별 시작 SP 배열이다. (2026-11-11 WB04-E278)
const INITIAL_SP = Object.freeze([1000, 2500, 3000]);

// Money 시작 SP 지정에서 건너뛰는 현재 Player+0x50 의 비트이다. (2026-11-11 WB04-E278)
const EXCLUDED_PLAYER = 0x20;

// fort 고정 SP·편집기(special) 로컬 SP 의 float 값(둘 다 194162.0f)과 시작 SP 의 가산값·SP 검사 잔여 횟수이다. (2026-11-11 WB04-E278)
const PATCH_FORT_SP = 194162;
const SPECIAL_LOCAL_SP = 194162;
const BONUS_SP = 4000;
const SP_CHECK_COUNT = 99;

// 실제 Player 기록: 9개, 패치 stride 0xb4, flags BYTE 위치 +0x50 이다. (2026-11-11 WB04-E278)
const PLAYER_COUNT = 9;
const PLAYER_STRIDE = 0xb4;
const PLAYER_FLAGS_OFFSET = 0x50;

// float 와 비트 변환용 공유 버퍼이다. 값은 모두 정확히 표현되는 정수라 NaN 페이로드 문제가 없다. (2026-11-11 WB04-E278)
const FLOAT_VIEW = new Float32Array(1);
const BITS_VIEW = new Uint32Array(FLOAT_VIEW.buffer);

// 수를 단정도 float 비트로 바꾼다. (2026-11-11 WB04-E278)
function floatBits(value) {
    FLOAT_VIEW[0] = value;
    return BITS_VIEW[0];
}

// float 비트를 수로 바꾼다. (2026-11-11 WB04-E278)
function bitsFloat(bits) {
    BITS_VIEW[0] = bits >>> 0;
    return FLOAT_VIEW[0];
}

// 원문이 선택한 실제 조회/표시 장치만 그 호출 위치에서 요구한다. 없으면 빈 성공으로 넘기지 않고 거부한다. (2026-11-11 WB04-E278)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('현재 Player SP 지정 장치 누락');
    return hook;
}

// 0040ef30: 현재 known/owner 를 원문 위치에서 조회하고 선택된 같은 분산 SP 잔액만 읽는다.
// reads = { clientActive(), localOwner(), editor() } (각각 00595344/00540c70/005c85a4 의 현재 DWORD), 사용: readPlayerSpAmount(1, store, reads) → float 잔액. (2026-11-11 WB04-E278)
export function readPlayerSpAmount(owner, store, reads) {
    if (need(reads?.clientActive)() && (owner >>> 0) === (need(reads.localOwner)() >>> 0)) return bitsFloat(store.get(need(reads.localOwner)() >>> 0));
    const number = owner | 0;
    return number > 0 && number < 9 ? bitsFloat(store.get(owner >>> 0)) : 0;
}

// 00447810: 현재 known/owner/편집기를 다시 조회하는 정책 setter 이다. getter 와 상태 사본을 공유하지 않는다.
// owner·amount 는 signed DWORD, refresh 는 활성 로컬 분기에서만 호출하는 SP 표시 갱신(0043dad0)이다. (2026-11-11 WB04-E278)
export function setPlayerSpAmount(owner, amount, store, reads, refresh) {
    const number = owner | 0;
    if (need(reads?.clientActive)() && (number >>> 0) === (need(reads.localOwner)() >>> 0)) {
        const value = need(reads.editor)() ? SPECIAL_LOCAL_SP : Math.fround(amount | 0);
        store.set(need(reads.localOwner)() >>> 0, floatBits(value));
        need(refresh)();
    } else if (number > 0 && number < 9) {
        const value = need(reads.editor)() ? SPECIAL_LOCAL_SP : Math.fround(amount | 0);
        store.set(number >>> 0, floatBits(value));
    }
}

// Money wrapper 와 현재 Player SP 지정이 읽는 나머지 전역이다(cpppj FortMoneyState). 필드는 독립 값형 입력이며 소비 위치마다 다시 읽는다.
// 사용: const state = new FortMoneyState(); state.difficulty = 1. (2026-11-11 WB04-E278)
export class FortMoneyState {
    // 005c85d4 가 nonzero 이면 시작 SP 적용을 생략한다.
    economyBlocked = false;
    // 00595344: 로컬 플레이어 SP/표시 경로를 선택한다.
    clientActive = false;
    // 실제 Template 의 첫 DWORD 가 nonzero 인지이다. 없으면 진단 뒤 계속한다.
    templatePresent = true;
    // 0052f7ee 의 BYTE 난이도(0..2 인덱스)이다.
    difficulty = 0;
    // 005c89c4 의 SP 검사 잔여 횟수이다. 조건 성립 시 99 를 쓴다(지형 CRT 캐시와 별개).
    spCheckCount = 0;
}

// 패치 004bf340 → 00483300 의 현재 Money 실행 사슬이다.
// 사용: new RawFortMoneyLoad(reward, players, playerRecords, depth, state, store, { assertion, loadingDepthChanged, refreshSp }).load(cursor).
//  reward: SquidRewardState(currentFixedSpA/B·currentHalfHealthMode), players: { localOwner }(SquidPostPopState), playerRecords: 9×0xb4 바이트 Uint8Array(살아 있는 뷰),
//  depth: { value }(현재 로딩 깊이 DWORD 소유자), state: FortMoneyState, store: ScrambledSpStore(같은 게임 난수). 모두 이 객체보다 오래 살아야 한다. (2026-11-11 WB04-E278)
export class RawFortMoneyLoad {
    // 필수 효과·실제 Player 기록 크기·분산 저장소를 구성 전에 확인한다. 빠진 장치를 기본값으로 만들지 않는다.
    constructor(reward, players, playerRecords, depth, state, patchStore, hooks) {
        if (!(playerRecords instanceof Uint8Array) || playerRecords.length !== PLAYER_COUNT * PLAYER_STRIDE || !patchStore
            || typeof hooks?.assertion !== 'function' || typeof hooks.loadingDepthChanged !== 'function' || typeof hooks.refreshSp !== 'function'
            || typeof reward?.currentFixedSpA !== 'function' || typeof reward.currentFixedSpB !== 'function' || typeof reward.currentHalfHealthMode !== 'function'
            || !players || typeof depth?.value !== 'number' || !state) throw new TypeError('FortMoney 실제 Player/SP/필수 효과 누락');
        this.reward = reward;
        this.players = players;
        this.playerRecords = playerRecords;
        this.depth = depth;
        this.state = state;
        this.patchStore = patchStore;
        this.hooks = hooks;
    }

    // 실제 physical Player 0..8 의 현재 flags BYTE 만 읽는다. 범위 밖은 거부한다. (2026-11-11 WB04-E278)
    playerFlags(owner) {
        if (!Number.isInteger(owner) || owner < 0 || owner >= PLAYER_COUNT) throw new RangeError('FortMoney 실제 Player 범위 오류');
        return this.playerRecords[owner * PLAYER_STRIDE + PLAYER_FLAGS_OFFSET];
    }

    // 00447810: AI 지갑 가산이나 SP 차감용 AddSp 를 쓰지 않고 실제 지정 몸체의 분기를 따른다. 활성 여부·로컬 owner·편집기는 호출마다 현재 값이다. (2026-11-11 WB04-E278)
    setPlayerSp(owner, amount) {
        setPlayerSpAmount(owner, amount, this.patchStore, {
            clientActive: () => (this.state.clientActive ? 1 : 0),
            localOwner: () => this.players.localOwner & 255,
            editor: () => (this.reward.currentFixedSpB() ? 1 : 0),
        }, this.hooks.refreshSp);
    }

    // 독립 입력도 원본 getter 시점에서 위치를 0 으로 초기화한 같은 cursor 를 소비한다. cursor = { position, end }(DWORD). (2026-11-11 WB04-E278)
    load(stream) {
        this.loadSelected(() => {
            stream.position = 0;
            return stream;
        });
    }

    // 004bf340: wrapper 의 현재 깊이/Template 진단 뒤에 실제 getter(select)를 실행하고, 실패하면 같은 원래 깊이를 복구한 뒤 다시 던진다. (2026-11-11 WB04-E278)
    loadSelected(select) {
        if (typeof select !== 'function') throw new TypeError('Fort Money 실제 섹션 선택 누락');
        if (this.reward.currentHalfHealthMode() && !this.state.economyBlocked) this.state.spCheckCount = SP_CHECK_COUNT;
        this.depth.value = (this.depth.value + 1) >>> 0;
        this.hooks.loadingDepthChanged(this.depth.value);
        try {
            if (!this.state.templatePresent) this.hooks.assertion(FORT_MONEY_ASSERTION.missingTemplate);
            this.readMoney(select()); // 실제 DataManager getter 가 같은 현재 노드를 선택하고 위치를 초기화한다.
        } catch (error) {
            this.depth.value = (this.depth.value - 1) >>> 0;
            this.hooks.loadingDepthChanged(this.depth.value);
            throw error;
        }
        this.depth.value = (this.depth.value - 1) >>> 0;
        this.hooks.loadingDepthChanged(this.depth.value);
    }

    // 00483300: 섹션 4바이트 소비와 현재 모드별 SP 설정을 그대로 실행한다. 저장 float 비트를 SP 로 대입하지 않는다. wrapper/조회/깊이/99회 설정은 loadSelected 몫이다. (2026-11-11 WB04-E278)
    readMoney(stream) {
        stream.position = (stream.position + 4) >>> 0;
        if ((stream.end | 0) < (stream.position | 0)) this.hooks.assertion(FORT_MONEY_ASSERTION.streamPastEnd);
        if (this.reward.currentFixedSpA()) this.patchStore.set(this.players.localOwner & 255, floatBits(PATCH_FORT_SP));
        if (this.reward.currentHalfHealthMode() && !this.state.economyBlocked) {
            const difficulty = this.state.difficulty & 255;
            if (difficulty >= INITIAL_SP.length) throw new RangeError('FortMoney 실제 difficulty 배열 범위 오류');
            const amount = (INITIAL_SP[difficulty] + BONUS_SP) | 0;
            // 원본은 현재 Player 1..8 의 flags 를 각 반복에서 읽고 같은 게임 난수를 소비하는 SP 지정 몸체를 호출한다.
            for (let owner = 1; owner < PLAYER_COUNT; owner += 1) {
                if (!(this.playerFlags(owner) & EXCLUDED_PLAYER)) this.setPlayerSp(owner, amount);
            }
        }
    }
}
