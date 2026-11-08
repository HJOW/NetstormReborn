// 실제 Player152 의 Regular 처리기(가상 +5c, 00490810)가 다루는 두 "소어 루저" 검사 사건을 현재 두 owner·실제 Regular 호스트에 연결하는 모듈이다(cpppj `o/RawPlayerRegular.cpp/.h`, 10.78 전용).
// 순서(handle): Player VT 확인 → 물리 슬롯 owner BYTE 가 현재 battleOwner DWORD 와 다르거나 호출 횟수 0 이면 효과 없이 원래 payload 를 돌려준다.
//  - event 1: 로그 그룹 29 "SORE LOSER TEST: " 뒤 전투 모드(로그 효과 뒤 다시 읽음)면 "STARTING.\n" 로그와 예약(new 40) 성공 시 event 2/payload 60.0 Regular 를 만들고 0 을, 아니면 "NOT STARTING YET.\n" 로그 뒤 원래 payload 를 돌려준다.
//  - event 2: 전투 모드면 현재 templateOwner 의 Template State(4bd550) 첫 BYTE 에 4 를 OR 하고, getter 뒤 다시 읽은 templateOwner 로 Notify(4bd540)를 부른다. 0 을 돌려준다.
//  - 그 밖의 사건: 진단(Player.cpp 0x41d) 뒤 원래 payload 를 돌려준다.
// event 1/count 0 의 자연 반환은 최초 payload 5 를 현재+5 로 재예약하는 Regular 실행 몸체에 맡긴다.
// hooks(모두 필수): assertion(kind), log(group, text), reserveRegular() → boolean, addRegular(sid, event, payload), templateState(slot) → Uint8Array, templateChanged(slot).
// CD 10.72 판본 분기(owner +32·진단 0x3c9)는 옮기지 않았다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E55: Player Regular 처리기 이식으로 추가)

// 원본 assert 위치 이름이다: local/count 조건을 통과했는데 사건이 1/2 가 아님(10.78 Player.cpp 0x41d). 진단 뒤 payload 를 돌려준다. (2026-11-08 WB04-E55)
export const PLAYER_REGULAR_ASSERTION = Object.freeze({ unknownEvent: 'UnknownEvent' });

// 원본 로그 그룹과 두 사건 번호, 새 Regular 2 의 payload(비트 0x42700000)이다. payload 60 은 최초 시각 지연이 아니다. (2026-11-08 WB04-E55)
export const PLAYER_REGULAR_LOG_GROUP = 29;
const FIRST_EVENT = 1;
const SECOND_EVENT = 2;
const SECOND_PAYLOAD = 60.0;

// 슬롯의 소유자 BYTE 위치(+34)와 Template State 첫 BYTE 의 OR 비트이다. (2026-11-08 WB04-E55)
const OWNER_OFFSET = 34;
const SORE_LOSER_BIT = 4;

// 원본 로그 문자열 세 개이다(그대로 같은 순서로 보낸다). (2026-11-08 WB04-E55)
export const PLAYER_REGULAR_MESSAGES = Object.freeze({ test: 'SORE LOSER TEST: ', starting: 'STARTING.\n', waiting: 'NOT STARTING YET.\n' });

// 필수 하위 장치 이름이다. (2026-11-08 WB04-E55)
const REQUIRED_HOOKS = Object.freeze(['assertion', 'log', 'reserveRegular', 'addRegular', 'templateState', 'templateChanged']);

// Player Regular 처리기이다. 같은 RawPlayerWorld(9기록·상태·VT 판정)와 살아 있는 SquidOwnerMode(battle)를 빌린다.
// 사용: const regular = new RawPlayerRegular(player, mode, hooks); host.setRegularHandler((sid, e, c, p) => regular.handle(sid, e, c, p)). (2026-11-08 WB04-E55)
export class RawPlayerRegular {
    // 같은 현재 Player 기록·상태와 VT 판별 몸체(RawPlayerWorld)이다.
    #player;
    // 로그 효과 뒤에도 다시 읽는 살아 있는 battle 모드 소유자(SquidOwnerMode)이다.
    #mode;
    // 로그/예약/Template 효과의 실제 연결(생성 때 모두 확인)이다.
    #hooks;

    // 최초 처리기 호출 전에 실제 Player 와 빠짐없는 하위 효과를 확인한다(빈 함수로 덮지 않는다).
    constructor(player, mode, hooks = {}) {
        if (typeof player?.handles !== 'function' || !player.state || player.pool?.edition !== 'patch1078') throw new TypeError('Player Regular 는 10.78 RawPlayerWorld 가 필요합니다.');
        if (typeof mode !== 'object' || mode === null || !('battle' in mode)) throw new TypeError('Player Regular 의 현재 battle 모드 소유자가 필요합니다.');
        // 여섯 하위 효과가 모두 함수인지 확인한다.
        for (const name of REQUIRED_HOOKS) {
            if (typeof hooks?.[name] !== 'function') throw new TypeError(`Player Regular 실제 로그/예약/현재 Template 효과 누락: ${name}`);
        }
        this.#player = player;
        this.#mode = mode;
        this.#hooks = { ...hooks };
    }

    // 같은 Player/공통 후처리/호스트의 풀이다.
    get pool() { return this.#player.pool; }

    // 실제 Player VT 만 판정하며 다른 타입의 처리기를 기본 본문으로 덮지 않는다.
    handles(sid) { return this.#player.handles(sid); }

    // 00490810: 사건/횟수/payload(float32)를 원본 폭으로 읽는다. 사건 1/횟수 0 은 원래 payload 를 그대로 돌려준다.
    handle(sid, event, count, payload) {
        if (!this.handles(sid)) throw new Error('Player Regular 미복원 가상 처리기 표');
        const hooks = this.#hooks;
        const state = this.#player.state;
        const owner = this.pool.slot(sid)[OWNER_OFFSET];
        if (owner !== state.battleOwner || !(count >>> 0)) return payload;
        const kind = event >>> 0;
        if (kind === FIRST_EVENT) {
            hooks.log(PLAYER_REGULAR_LOG_GROUP, PLAYER_REGULAR_MESSAGES.test);
            // 로그 효과 뒤의 현재 전투 모드를 다시 읽는다.
            if (this.#mode.battle) {
                hooks.log(PLAYER_REGULAR_LOG_GROUP, PLAYER_REGULAR_MESSAGES.starting);
                if (hooks.reserveRegular()) hooks.addRegular(sid, SECOND_EVENT, SECOND_PAYLOAD);
                return 0.0;
            }
            hooks.log(PLAYER_REGULAR_LOG_GROUP, PLAYER_REGULAR_MESSAGES.waiting);
            return payload;
        }
        if (kind === SECOND_EVENT) {
            if (this.#mode.battle) {
                const bytes = hooks.templateState(state.templateOwner);
                if (!bytes || bytes.length === 0) throw new RangeError('Player 실제 Template State BYTE 누락');
                bytes[0] |= SORE_LOSER_BIT;
                // getter 효과 뒤 다시 읽은 현재 templateOwner 로 통지한다.
                hooks.templateChanged(state.templateOwner);
            }
            return 0.0;
        }
        hooks.assertion(PLAYER_REGULAR_ASSERTION.unknownEvent);
        return payload;
    }
}
