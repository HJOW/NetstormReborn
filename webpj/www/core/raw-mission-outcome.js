// 원본 미션의 플레이어/팀 승패 조건과 한 번만 발생하는 스크립트 사건(004c36c0 Check·004c3940 Event·004922e0 사제 사망/포획)을 옮긴 모듈이다(cpppj `o/RawMissionOutcome.cpp/.h`, 10.78 전용).
//  - check(): 플레이어 1~8 중 활성(Player+0x50 & 0x22)인 플레이어마다 번호 순서로 신전 절반 파괴(HP < 최대 HP/2)·신전 파괴(없거나 dead 비트)·사제 사망·사제 포획·사제 구출(소유 사제의 영역 소유자 == 로컬 번호) 사건을
//    `ai<번호>TempleHalfDead` 등의 이름으로 검사하고(각 사건은 DWORD 잠금이 0 일 때만), 사제가 죽지 않았으면 로컬 번호와 두 방향 동맹이 모두 비영인지로 우리 팀/적 팀 생존을 센다.
//    끝으로 GoodTeamDead(우리 팀 전멸)·BadTeamDead(적 팀 전멸)·Failed(로컬 사제 사망) 사건을 검사한다. 신전 파괴만으로 승리하지 않는다.
//  - event(target, key, condition, name): 조건이 참이고 잠금이 0 이면, 데모면 `Demo` 접두 이름을 만든 뒤 Tell 차단 DWORD 가 0 일 때만 사건 섹션을 실행하고(발견되면 임시 Gump 정리 → 미션 가상 통지)
//    **콜백이 끝난 뒤** 잠금에 1 을 쓴다. 섹션이 없어도 차단이 없으면 잠금을 확정한다. 재진입은 막지 않는다.
//  - priestDead(player)/priestCaptured(player): 범위 밖 번호는 진단 뒤 죽음(true)/포획(true)으로 본다. 비활성은 죽음 true·포획 false, 활성 소유자는 ownerState 가 2/1 일 때만 true 이다.
//  - bindCurrentInputs(inputs): 실제 9 Player 원문(180 BYTE 기록)·방향별 9×9 동맹 DWORD·로컬 번호/데모/Tell 차단 getter 를 조회마다 읽도록 연결한다(생략하면 독립 입력 상태 `state` 를 쓴다).
// 외부 효과(최대 HP·영역 소유자·사건 섹션·임시 Gump 정리·미션 통지·범위 진단)는 호출자가 연결한다(진단 외 모두 필수). 콜백이 바꾼 값은 원본의 다음 조회 시점에 다시 읽는다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-10 WB04-E233: 미션 승패 조건 이식으로 추가)

import { RawPriestLookup } from './raw-priest-lookup.js';

// 플레이어 수와 활성 마스크(Player+0x50), 사망 비트, 슬롯 필드 위치(상태 +11·좌표 float +14/+18·HP +26), Player 기록 폭과 활성/신전 위치이다. (2026-11-10 WB04-E233)
const PLAYER_COUNT = 8;
const ACTIVE = 0x22;
const DEAD = 2;
const STATE_AT = 11;
const X_AT = 14;
const Y_AT = 18;
const HP_AT = 26;
const PLAYER_STRIDE = 0xb4;
const FLAGS_AT = 0x50;
const TEMPLE_AT = 0x7c;

// 원본 Player 한 명의 활성 플래그와 현재 신전 SID 이다. 독립 입력 상태용 값형이다. (2026-11-10 WB04-E233)
function createPlayer() { return { flags: 0, temple: 0 }; }

// 플레이어마다 유지하는 원본 DWORD 사건 잠금이다. 참이 된 조건을 처리하면 1 을 쓰며 자동 초기화하지 않는다. (2026-11-10 WB04-E233)
function createLatches() { return { templeDead: 0, templeHalfDead: 0, priestDead: 0, priestCaptured: 0, priestSaved: 0 }; }

// 미션의 현재 입력과 사건 잠금을 값형 독립 입력으로 만든다(플레이어 0~8 기록, 두 방향 9×9 동맹, 플레이어별 잠금, 팀 전멸/실패 잠금 +0x180/+0x184/+0x188, 로컬 번호 00540c70, 데모 +0x68, Tell 차단 00565dd4).
// 사용: const state = createMissionOutcomeState(); state.players[1].flags = 0x22. (2026-11-10 WB04-E233)
export function createMissionOutcomeState() {
    return {
        players: Array.from({ length: PLAYER_COUNT + 1 }, createPlayer),
        allies: Array.from({ length: PLAYER_COUNT + 1 }, () => new Array(PLAYER_COUNT + 1).fill(0)), // 두 방향이 모두 비제로여야 같은 팀이다.
        latches: Array.from({ length: PLAYER_COUNT + 1 }, createLatches),
        goodTeamDead: 0, badTeamDead: 0, failed: 0, localPlayer: 1, demo: 0, tellBlocked: 0,
    };
}

// 범위 밖 첨자는 원본 메모리를 읽는 대신 거부한다(C++ `std::array::at`). (2026-11-10 WB04-E233)
function at(array, index) {
    if (!Number.isInteger(index) || index < 0 || index >= array.length) throw new RangeError('미션 승패 입력 첨자 범위 오류');
    return array[index];
}

// 정렬되지 않은 raw 값을 지정 폭만큼 little endian 으로 읽는다. (2026-11-10 WB04-E233)
function read(raw, offset, width = 4) {
    let value = 0;
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// raw 의 float 비트를 단정도 수로 읽는다. (2026-11-10 WB04-E233)
const floatView = new DataView(new ArrayBuffer(4));
function floatAt(raw, offset) {
    floatView.setUint32(0, read(raw, offset), true);
    return floatView.getFloat32(0, true);
}

// 이 모듈 밖의 실제 게임 효과 이름이다(범위 진단 invalidPlayerAssert 만 선택). 누락은 동작 전에 거부한다. (2026-11-10 WB04-E233)
const REQUIRED_HOOKS = Object.freeze(['maxHitPoints', 'regionOwner', 'section', 'closeTransientGumps', 'onSection']);

// 004c36c0/004c3940/004922e0 의 실행 계층이다.
//  pool: 10.78 SidPool, priests: PriestPostPopState(현재 사제 목록 `priests`·ownerState), state: createMissionOutcomeState() 형태,
//  hooks = { maxHitPoints(sid) → 정수(004adcb0, 같은 풀 SquidReward), regionOwner(x, y) → 정수(0046f770), section(name) → boolean(00482de0 사건 섹션 실행/발견), closeTransientGumps()(00466710),
//            onSection()(미션 가상 +0x28), invalidPlayerAssert(player)(선택, 범위 assert 기록) }. (2026-11-10 WB04-E233)
export class RawMissionOutcome {
    // 사제 목록/ownerState 는 복사하지 않고 공유하며 입력 상태·효과·사제 조회기를 소유한다.
    #pool;
    #priests;
    #state;
    #hooks;
    #lookup;
    #current = null; // 연결 후에도 span/콜백만 소유하며 Player·동맹·미션 입력을 복사하지 않는다.

    // 판본과 필수 효과를 동작 전에 검사한다. (2026-11-10 WB04-E233)
    constructor(pool, priests, state, hooks = {}) {
        if (pool?.edition !== 'patch1078') throw new TypeError('미션 승패는 10.78 SidPool 만 받습니다.');
        if (REQUIRED_HOOKS.some((name) => typeof hooks[name] !== 'function')) throw new TypeError('미션 승패의 필수 효과 연결 누락');
        this.#pool = pool;
        this.#priests = priests;
        this.#state = state;
        this.#hooks = hooks;
        this.#lookup = new RawPriestLookup(pool, priests);
    }

    // 후속 연결에서 같은 풀의 객체/최대 HP 몸체를 쓰는지 검증하는 풀이다. (2026-11-10 WB04-E233)
    get pool() { return this.#pool; }

    // 패치만 범위 assert 효과를 요청한 뒤 정상 범위 여부를 돌려준다. (2026-11-10 WB04-E233)
    #validPlayer(player) {
        const valid = player >= 1 && player <= PLAYER_COUNT;
        if (!valid && typeof this.#hooks.invalidPlayerAssert === 'function') this.#hooks.invalidPlayerAssert(player >>> 0);
        return valid;
    }

    // 현재 사제 ownerState 표(공유 표를 빌렸으면 그 표)이다. (2026-11-10 WB04-E233)
    #ownerState() { return this.#priests.currentOwnerState(); }

    // 활성 플래그를 BYTE 그대로 읽는다(주변 필드나 후속 callback 의 입력을 미리 저장하지 않는다). (2026-11-10 WB04-E233)
    #playerFlags(player) {
        if (this.#current === null) return at(this.#state.players, player).flags;
        return this.#current.players[player * PLAYER_STRIDE + FLAGS_AT];
    }

    // 신전은 각 조건이 도달한 시점의 전체 DWORD 이며 높은 비트를 WORD 로 먼저 줄이지 않는다. (2026-11-10 WB04-E233)
    #playerTemple(player) {
        if (this.#current === null) return at(this.#state.players, player).temple >>> 0;
        return read(this.#current.players, player * PLAYER_STRIDE + TEMPLE_AT);
    }

    // 방향별 표의 현재 DWORD 이다. 두 방향의 단락 평가는 호출자가 유지한다. (2026-11-10 WB04-E233)
    #ally(from, to) {
        if (this.#current === null) return at(at(this.#state.allies, from), to) >>> 0;
        return this.#current.allies[from * 9 + to] >>> 0;
    }

    // callback 뒤의 다음 조회도 현재 로컬 번호/데모/Tell 차단을 읽는다. (2026-11-10 WB04-E233)
    #localPlayer() { return (this.#current === null ? this.#state.localPlayer : this.#current.localPlayer()) >>> 0; }
    #demo() { return (this.#current === null ? this.#state.demo : this.#current.demo()) >>> 0; }
    #tellBlocked() { return (this.#current === null ? this.#state.tellBlocked : this.#current.tellBlocked()) >>> 0; }

    // DWORD SID 를 범위 검사한 뒤 해당 슬롯 사본을 읽는다(0 번 슬롯은 원본처럼 유효). (2026-11-10 WB04-E233)
    #raw(sid) {
        if (sid >= this.#pool.capacity) throw new RangeError('미션 승패 raw SID 범위 오류');
        return this.#pool.slot(sid);
    }

    // 004922e0: 비활성/범위 밖은 사망으로 본다. 활성 소유자의 상태가 2 일 때만 true 이며 raw dead/void 검사를 덧붙이지 않는다. (2026-11-10 WB04-E233)
    priestDead(player) {
        return !this.#validPlayer(player) || !(this.#playerFlags(player) & ACTIVE) || this.#ownerState()[player] === 2;
    }

    // 범위 밖은 true, 비활성은 false, 활성 소유자는 상태가 1 일 때만 true 이다. (2026-11-10 WB04-E233)
    priestCaptured(player) {
        if (!this.#validPlayer(player)) return true;
        return Boolean(this.#playerFlags(player) & ACTIVE) && this.#ownerState()[player] === 1;
    }

    // 004c3940: 조건 참/잠금 0/gate 열림일 때만 section → 임시 UI 정리 → 가상 통지 → 잠금 1 순서이다. 성공 여부는 section 발견 여부와 별개이다.
    // 원본의 DWORD 잠금 참조는 (target, key) 쌍이며 콜백이 끝난 뒤 같은 참조에 쓴다. target 은 함수일 수 있어(잠금 객체가 바뀌어도 현재 것을 쓰도록) 매번 호출해 얻는다. (2026-11-10 WB04-E233)
    event(target, key, condition, name) {
        const holder = typeof target === 'function' ? target : () => target;
        if (!condition || holder()[key]) return false;
        const section = (this.#demo() ? 'Demo' : '') + name;
        if (this.#tellBlocked()) return false;
        if (this.#hooks.section(section)) { this.#hooks.closeTransientGumps(); this.#hooks.onSection(); }
        holder()[key] = 1;
        return true;
    }

    // 현재 신전 없음 또는 HP < 최대 HP/2(0 방향 절삭)이다. HP 는 최대 HP 조회 전에 읽고 정확히 절반이면 아니다. (2026-11-10 WB04-E233)
    #templeHalfDead(player) {
        const sid = this.#playerTemple(player);
        if (!sid) return true;
        const hp = read(this.#raw(sid), HP_AT) | 0;
        return hp < Math.trunc(this.#hooks.maxHitPoints(sid) / 2);
    }

    // 앞선 halfDead 사건 콜백이 바꾼 현재 신전 SID 와 상태를 새로 읽는다. 없음 또는 raw dead bit2 만 보고 free/void 는 보지 않는다. (2026-11-10 WB04-E233)
    #templeDead(player) {
        const sid = this.#playerTemple(player);
        return !sid || Boolean(this.#raw(sid)[STATE_AT] & DEAD);
    }

    // 현재 사제 목록에서 찾은 좌표의 영역 소유자와 callback 직전에 저장한 로컬 번호를 비교한다. (2026-11-10 WB04-E233)
    #priestSaved(player) {
        const sid = this.#lookup.find(player);
        if (!sid) return false;
        const local = this.#localPlayer();
        const raw = this.#raw(sid);
        return this.#hooks.regionOwner(floatAt(raw, X_AT), floatAt(raw, Y_AT)) >>> 0 === local;
    }

    // 004c36c0: 플레이어 1~8 의 halfTemple/deadTemple/deadPriest/captured/saved 순서 뒤 팀/로컬 실패를 검사한다. 신전 파괴만으로 승리하지 않으며 사제 ownerState 와 두 방향 동맹으로 적 팀의 생존을 집계한다.
    // 상태가 재진입 중 바뀌어도 현재 플레이어의 진입 시 활성 조건을 다시 검사하지 않는다. (2026-11-10 WB04-E233)
    check() {
        const state = this.#state;
        let good = 0;
        let bad = 0;
        // 원본 번호 순서이며 사건마다 현재 입력을 다시 조회한다. 두 팀 집계는 그 플레이어의 saved 사건 뒤이다.
        for (let player = 1; player <= PLAYER_COUNT; player += 1) {
            if (!(this.#playerFlags(player) & ACTIVE)) continue;
            const latch = () => state.latches[player]; // 콜백이 잠금 표를 갈아 끼워도 현재 것에 쓴다.
            const prefix = `ai${player}`;
            this.event(latch, 'templeHalfDead', this.#templeHalfDead(player), `${prefix}TempleHalfDead`);
            this.event(latch, 'templeDead', this.#templeDead(player), `${prefix}TempleDead`);
            this.event(latch, 'priestDead', this.priestDead(player), `${prefix}PriestDead`);
            this.event(latch, 'priestCaptured', this.priestCaptured(player), `${prefix}PriestCaptured`);
            this.event(latch, 'priestSaved', this.#priestSaved(player), `${prefix}PriestSaved`);
            if (!this.priestDead(player)) {
                const local = this.#localPlayer();
                if (local > PLAYER_COUNT) throw new RangeError('미션 승패 동맹의 로컬 번호 범위 오류');
                if (this.#ally(local, player) && this.#ally(player, local)) good += 1;
                else bad += 1;
            }
        }
        this.event(state, 'goodTeamDead', good === 0, 'GoodTeamDead');
        this.event(state, 'badTeamDead', bad === 0, 'BadTeamDead');
        this.event(state, 'failed', this.priestDead(this.#localPlayer()), 'Failed');
    }

    // 모든 필수 현재 입력을 확인한 뒤 한 번에 연결한다. 오류 때는 이전 연결을 유지한다.
    //  inputs = { players: Uint8Array(9 × 180 BYTE 현재 Player 원문), allies: 81 DWORD(방향별 9×9), localPlayer(), demo(), tellBlocked() }. (2026-11-10 WB04-E233)
    bindCurrentInputs(inputs) {
        if (inputs?.players?.length !== 9 * PLAYER_STRIDE || inputs.allies?.length !== 81 || typeof inputs.localPlayer !== 'function' ||
            typeof inputs.demo !== 'function' || typeof inputs.tellBlocked !== 'function') throw new TypeError('미션 승패의 실제 Player9/방향별 동맹/현재 전역 누락');
        this.#current = inputs;
    }
}

// 최대 HP 만 같은 풀의 실제 SquidReward 몸체로 연결한다. UI/스크립트/영역 효과는 호출자가 채운 그대로 둔다. 다른 풀이면 TypeError 이다. 반환은 maxHitPoints 만 바꾼 새 hooks 객체이다. (2026-11-10 WB04-E233)
export function makeMissionOutcomeHooks(pool, reward, hooks) {
    if (reward?.pool !== pool) throw new TypeError('미션 승패와 최대 HP 의 SID 풀이 다릅니다');
    return { ...hooks, maxHitPoints: (sid) => reward.maxHitPoints(sid) };
}
