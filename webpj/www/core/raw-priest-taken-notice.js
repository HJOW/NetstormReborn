// 사제가 포획되어 소유자가 바뀐 뒤의 참여 갱신·음향·제한 안내(004927f0)를 옮긴 모듈이다(cpppj `o/RawPriestTakenNotice.cpp/.h`, 10.78 전용).
// 순서: 이전 소유자 기준으로 참여 상태를 먼저 갱신 → 현재 소유자가 로컬 플레이어이면 끝(음향·안내 없음) → 이전 소유자 기한 칸 범위 확인 → 안내 키와 음향 선택(이전 소유자가 로컬이면 `WarnYourPriestTaken`+ourPriestCaptured.wav,
//   중립이면 `WarnNeutralPriestTaken`(음향 없음), 그 밖은 `WarnPriestTaken`+enemyPriestCaptured.wav) → 벽시각을 읽어 이전 소유자의 기한 이하일 때만 이름 둘(이전 소유자, 현재 소유자 순)을 읽어 256바이트 안의 안내 문자열을
//   `키(이전,현재 이름,이전 이름,현재 색,이전 색)` 로 만들어 전달 → 기한을 안내 뒤 다시 읽은 벽시각 + 20초로 저장한다(억제 중에도 음향과 기한 재설정은 수행). 색 문자열은 소유자 0..8 순의 고정 표이며 범위 밖 현재 소유자는 0번 색이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E68: 사제 포획 안내 이식으로 추가)

// 슬롯의 현재 소유자 BYTE 위치, 안내 문자열 최대 바이트(원문 지역 버퍼 256), 기한 유예(초)이다. (2026-11-08 WB04-E68)
const OWNER_OFFSET = 34;
const BUFFER_BYTES = 256;
const SUPPRESS_SECONDS = 20.0;

// 원문 004cef50 의 고정 색 문자열이며 사용자 색 표와 다르다. 소유자 0..8 순이다. (2026-11-08 WB04-E68)
const OWNER_COLORS = Object.freeze(['~q', '~l', '~r', '~w', '~u', '~s', '~y', '~d', '~a']);

// 도달한 하위 누락만 진단하며 앞선 효과는 되돌리지 않는다. (2026-11-08 WB04-E68)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('사제 포획 안내의 실제 하위 미연결');
    return hook;
}

// pool 은 10.78 SidPool, localOwner 는 현재 로컬 소유자를 읽는 getter(호출 전 사본으로 고정하지 않음), deadlines 는 소유자별 안내 기한 9칸 배열(세션 교체에서 지우지 않음; 참조),
//   hooks = { refresh(이전 소유자), wallSeconds()→초, name(소유자)→문자열, sound(파일), tell(문자열) } 이며 도달한 것만 필요하다. (2026-11-08 WB04-E68)
export class RawPriestTakenNotice {
    // 음향/참여 콜백 뒤에도 같은 부모의 현재 소유자를 읽는 풀, 로컬 소유자 getter, 이전 소유자별 기한 표, 하위 효과이다.
    #pool;
    #localOwner;
    #deadlines;
    #hooks;

    // 실제 판본과 참조 수명만 확인한다. 생성은 어떤 게임 효과도 내지 않는다. (2026-11-08 WB04-E68)
    constructor(pool, localOwner, deadlines, hooks) {
        if (pool?.edition !== 'patch1078' || typeof localOwner !== 'function' || !deadlines || deadlines.length !== 9) throw new TypeError('사제 포획 안내는 10.78 실제 연결 전용');
        this.#pool = pool;
        this.#localOwner = localOwner;
        this.#deadlines = deadlines;
        this.#hooks = hooks ?? {};
    }

    // 같은 월드 연결을 검사하는 풀이다. (2026-11-08 WB04-E68)
    get pool() {
        return this.#pool;
    }

    // 기한 억제는 음향을 막지 않으며 최종 시각은 안내 뒤 다시 읽는다. (2026-11-08 WB04-E68)
    notify(parent, previousOwner) {
        const hooks = this.#hooks;
        need(hooks.refresh)(previousOwner);
        const owner = this.#pool.slot(parent)[OWNER_OFFSET];
        if (owner === (this.#localOwner() >>> 0)) return;
        if (previousOwner >= this.#deadlines.length) throw new RangeError('사제 포획 안내의 이전 owner 기한 범위');
        const color = owner < OWNER_COLORS.length ? OWNER_COLORS[owner] : OWNER_COLORS[0];
        const previousColor = OWNER_COLORS[previousOwner];
        let key;
        if (previousOwner === (this.#localOwner() >>> 0)) {
            key = 'WarnYourPriestTaken';
            need(hooks.sound)('ourPriestCaptured.wav');
        } else if (!previousOwner) {
            key = 'WarnNeutralPriestTaken';
        } else {
            key = 'WarnPriestTaken';
            need(hooks.sound)('enemyPriestCaptured.wav');
        }
        // 원문은 시계 호출이 끝난 뒤 현재 기한을 읽어 비교한다.
        const now = need(hooks.wallSeconds)();
        if (this.#deadlines[previousOwner] <= now) {
            // 원문은 이름 getter 둘보다 먼저 현재 포획자를 보관한다.
            const currentOwner = this.#pool.slot(parent)[OWNER_OFFSET];
            const previousName = need(hooks.name)(previousOwner);
            const currentName = need(hooks.name)(currentOwner);
            const output = `${key}(${previousOwner},${currentName},${previousName},${color},${previousColor})`;
            // 원문 지역 버퍼의 크기를 유지하고 범위 밖 쓰기만 거부한다.
            if (new TextEncoder().encode(output).length >= BUFFER_BYTES) throw new RangeError('사제 포획 안내의 원문256BYTE 서식 범위');
            need(hooks.tell)(output);
        }
        this.#deadlines[previousOwner] = need(hooks.wallSeconds)() + SUPPRESS_SECONDS;
    }
}
