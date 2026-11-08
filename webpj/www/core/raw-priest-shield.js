// 사제 보호막의 조회→생성/소유자/Pop→소리→로컬 안내 요청을 옮긴 모듈이다(cpppj `o/RawPriestShield.cpp/.h`, 10.78 전용: 00493d30).
// 순서: 기존 보호막이 있으면(조회) 모든 생성/소리/안내를 생략한다. 없으면 현재 보호막 TYPE 로 로컬 풀(flags 2)에 자산을 생성하고(유효한 자산 SID 가 아니면 거부), 사제의 좌표를 캡처해 소유자를 지정하고 가상 Pop(flags 0)을 호출한다.
//   그 뒤 소리 예약이 성공하면 "priestForceField.wav" 를 조회해 보호막에 재생(0, 1)한다. 마지막으로 소유자 지정/Pop/소리 뒤의 현재 사제 소유자가 로컬 플레이어이고 로딩 깊이 0, 알림 시각이 +0/−0 이면(NaN 제외)
//   "ourPriestImmobile.wav" 소리와 번역 없이 포맷한 "PriestImmobile" 안내 문구(Tell)를 요청한다. 좌표는 산술 변환 없이 읽어 생성 훅이 공급한 NaN/부호 있는 0도 전달한다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E63: 사제 보호막 이식으로 추가)

// 슬롯의 좌표/소유자 위치, 보호막 소리·안내 이름, 첫 자산 SID 이다. (2026-11-08 WB04-E63)
const X_OFFSET = 14;
const Y_OFFSET = 18;
const OWNER_OFFSET = 34;
const SHIELD_SOUND = 'priestForceField.wav';
const NOTICE_KEY = 'PriestImmobile';
const FIRST_ASSET_SID = 5;
const INVALID_STATE = 9;

// 사제 보호막 처리 중 조회하는 상태이다: 로컬 플레이어 DWORD, 요새 로딩 중첩 전역, 안내 시각(double; 원본은 +0/−0 일 때만 로컬 알림을 요청하며 NaN 은 제외)이다. (2026-11-08 WB04-E63)
export class PriestShieldState {
    localPlayer = 0;
    loadingDepth = 0;
    noticeClock = 0;

    // 초기값 객체로 만든다(생략한 칸은 0). (2026-11-08 WB04-E63)
    constructor({ localPlayer = 0, loadingDepth = 0, noticeClock = 0 } = {}) {
        this.localPlayer = localPlayer;
        this.loadingDepth = loadingDepth;
        this.noticeClock = noticeClock;
    }
}

// 안내 소리의 원본 전역 소리 함수 인자(파일 이름과 다섯 DWORD)이다. (2026-11-08 WB04-E63)
export function priestShieldNotice() {
    return { file: 'ourPriestImmobile.wav', arguments: [0, 0, 0, 1, 0] };
}

// 비트 패턴을 단정도 float 로 해석한다(NaN 페이로드를 그대로 전달). (2026-11-08 WB04-E63)
function coordinate(raw, offset) {
    const view = new DataView(new ArrayBuffer(4));
    for (let index = 0; index < 4; index += 1) view.setUint8(index, raw[offset + index]);
    return view.getFloat32(0, true);
}

// pool 은 10.78 SidPool, lookup 은 RawPriestForcefield, state 는 PriestShieldState, hooks = { create(type, flags) → sid, setOwner(sid, player), pop(sid, x, y, flags), reserveSound() → bool,
//   loadSound(파일) → 소리 번호, attachSound(sid, 소리, 첫째, 둘째), noticeSound(notice), translate(키) → 문구, tell(문구) } 이며 모두 필요하다. (2026-11-08 WB04-E63)
export class RawPriestShield {
    // 객체 슬롯 풀, 조회, 상태, 외부 효과이다.
    #pool;
    #lookup;
    #state;
    #hooks;

    // 실제 조회의 풀 및 필수 경계는 첫 효과 전에 검사한다. (2026-11-08 WB04-E63)
    constructor(pool, lookup, state, hooks) {
        const required = ['create', 'setOwner', 'pop', 'reserveSound', 'loadSound', 'attachSound', 'noticeSound', 'translate', 'tell'];
        if (pool?.edition !== 'patch1078' || lookup?.pool !== pool || required.some((name) => typeof hooks?.[name] !== 'function')) throw new TypeError('사제 보호막 생성 풀/필수 효과 연결 오류');
        this.#pool = pool;
        this.#lookup = lookup;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 사제 후처리의 어댑터가 월드를 혼합하지 않도록 실제 풀을 제공한다. (2026-11-08 WB04-E63)
    get pool() {
        return this.#pool;
    }

    // 00493d30: 기존 보호막이 있으면 모든 생성/소리/안내를 생략한다. 생성 뒤 좌표를 캡처하고 소유자 지정/Pop/소리 뒤에는 로컬 조건을 다시 읽는다. (2026-11-08 WB04-E63)
    ensure(priest) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        if (this.#lookup.find(priest)) return;
        const shield = hooks.create(this.#lookup.type(), 2);
        if (shield < FIRST_ASSET_SID || shield >= pool.capacity || (pool.slot(shield)[11] & INVALID_STATE)) throw new TypeError('사제 보호막 생성이 유효한 자산 SID를 반환하지 않았습니다');
        const raw = pool.slot(priest);
        const x = coordinate(raw, X_OFFSET);
        const y = coordinate(raw, Y_OFFSET);
        hooks.setOwner(shield, raw[OWNER_OFFSET]);
        hooks.pop(shield, x, y, 0);
        if (hooks.reserveSound()) {
            const sound = hooks.loadSound(SHIELD_SOUND);
            hooks.attachSound(shield, sound, 0, 1);
        }
        if (pool.slot(priest)[OWNER_OFFSET] === this.#state.localPlayer && this.#state.loadingDepth === 0 && this.#state.noticeClock === 0) {
            hooks.noticeSound(priestShieldNotice());
            const text = hooks.translate(NOTICE_KEY);
            hooks.tell(text);
        }
    }
}

// 생성·소유자 지정만 실제 모듈로 바꾸며 Pop/소리/안내의 미복원 효과는 유지한다. factory 는 SquidFactory, owner 는 SquidOwner(둘 다 같은 풀). (2026-11-08 WB04-E63)
export function makePriestShieldCreationHooks(pool, factory, owner, hooks) {
    if (factory.pool !== pool || owner.pool !== pool) throw new TypeError('보호막/factory/소유자 SID 풀이 다릅니다');
    return {
        ...hooks,
        create: (type, flags) => factory.create(type, flags),
        setOwner: (sid, player) => { owner.set(sid, player); }
    };
}
