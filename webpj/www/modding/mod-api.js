// 공통 v0.1.0의 순수 공개 API다. 격리 실행기는 이 객체만 모드에 주고 실제 게임 참조를 전달하지 않는다.

// 공개 이름·사건·입력·오류의 정확한 목록이다. shared/modding/v0/contract.json과 검사로 대조한다.
export const MOD_API_VERSION = '0.1.0';
export const MOD_ROOT_NAMES = Object.freeze(['apiVersion', 'modId', 'on', 'off', 'getState', 'getPlayers', 'getSelection', 'randomInt', 'log', 'commands']);
export const MOD_COMMAND_NAMES = Object.freeze(['click', 'pressKey', 'selectPriest', 'nextUnit', 'produceBridge', 'repeatProduction', 'rotatePiece']);
export const MOD_EVENT_NAMES = Object.freeze(['sessionStart', 'tick', 'sessionEnd', 'commandResult']);
export const MOD_KEY_NAMES = Object.freeze(['p', 'r', 'n', 'h', 'q', 'w', 'a', 's', 'z', 'x', 'e', 'd', 'c', 'ArrowLeft', 'ArrowRight', 'ArrowUp', 'ArrowDown']);
export const MOD_ERROR_CODES = Object.freeze(['invalidArgument', 'unsupportedVersion', 'invalidPackage', 'moduleNotFound', 'permissionDenied', 'quotaExceeded', 'noSession', 'staleSession', 'inputBlocked', 'queueFull', 'scriptError', 'timeLimit', 'disposed']);
// 패키지/메시지 한도는 호스트도 재검사한다. 웹 VM의 힙·스택 바이트 상한을 뜻하지 않는다.
export const MOD_LIMITS = Object.freeze({ maxInstalledMods: 8, maxPackageBytes: 1048576, maxModuleBytes: 262144, maxModules: 32, maxHandlers: 32, maxCommandsPerEvent: 8, maxCommandResultsPerEvent: 16, maxLogsPerEvent: 8, maxLogCharacters: 1024, maxMessageBytes: 65536, timeLimitMilliseconds: 1000, quickJsMemoryBytes: 16777216, quickJsStackBytes: 262144, tickGameSeconds: 1 });

// 호스트와 모드가 함께 판별할 오류다. 내부 예외 객체를 그대로 메시지 경계에 보내지 않는다.
export class ModError extends Error {
    // 계약에 정의된 코드와 사람이 읽을 설명을 묶는다.
    constructor(code, message) {
        super(message);
        if (!MOD_ERROR_CODES.includes(code)) throw new TypeError('모드 오류 코드가 계약에 없습니다.');
        // 이름과 코드는 JSON 오류 봉투를 만들 때 읽는다.
        this.name = 'ModError'; this.code = code;
    }
}

// 인수 오류를 첫 효과 전에 계약 오류로 통일한다.
function requireValue(condition, message) { if (!condition) throw new ModError('invalidArgument', message); }
// 정수 범위와 NaN/Infinity를 한 번에 검사한다.
function integer(value, minimum, maximum, name) { requireValue(Number.isInteger(value) && value >= minimum && value <= maximum, `${name} 범위 오류`); return value; }
// 허용한 키만 있는 일반 데이터 객체를 받으며 접근자 실행을 차단한다.
function record(value, keys, name) {
    requireValue(value !== null && typeof value === 'object' && !Array.isArray(value) && [Object.prototype, null].includes(Object.getPrototypeOf(value)), `${name} 객체가 필요합니다.`);
    // getter·symbol·숨은 속성도 호스트 경계에 들이지 않는다.
    for (const key of Reflect.ownKeys(value)) {
        const descriptor = Object.getOwnPropertyDescriptor(value, key);
        requireValue(typeof key === 'string' && keys.includes(key) && descriptor.enumerable && 'value' in descriptor, `${name}의 허용되지 않은 필드`);
    }
    return value;
}
// 경계의 JSON 값을 사본으로 만들며 비정상 깊이·개수·접근자·비유한 수를 거부한다.
export function copyModJson(value) {
    // 검사 중인 총 항목 수를 공유해 큰 중첩 입력의 비용을 제한한다.
    let count = 0;
    // 각 재귀에서 평범한 값만 복사한다. 순환 참조도 깊이 한도로 종료한다.
    function copy(item, depth) {
        requireValue(depth <= 16 && ++count <= 4096, '모드 메시지 구조 한도 초과');
        if (item === null || typeof item === 'boolean') return item;
        if (typeof item === 'number') { requireValue(Number.isFinite(item), '모드 숫자는 유한해야 합니다.'); return item; }
        if (typeof item === 'string') { requireValue(item.length <= MOD_LIMITS.maxMessageBytes, '모드 문자열 한도 초과'); return item; }
        requireValue(typeof item === 'object', '모드 메시지는 JSON 값이어야 합니다.');
        if (Array.isArray(item)) {
            requireValue(item.length <= 4096 && Object.keys(item).length === item.length && Reflect.ownKeys(item).length === item.length + 1, '모드 배열 형식 오류');
            // 배열 접근자도 호출하지 않고 데이터 descriptor만 복사한다.
            return Array.from({ length: item.length }, (_, index) => {
                const descriptor = Object.getOwnPropertyDescriptor(item, String(index));
                requireValue(descriptor && 'value' in descriptor && descriptor.enumerable, '모드 배열 접근자/빈 칸 금지');
                return copy(descriptor.value, depth + 1);
            });
        }
        requireValue([Object.prototype, null].includes(Object.getPrototypeOf(item)), '모드 메시지의 객체 형식 오류');
        // null prototype 사본으로 __proto__ 같은 키의 대입 효과를 없앤다.
        const output = Object.create(null);
        // 모든 필드를 데이터 descriptor로만 읽는다.
        for (const key of Reflect.ownKeys(item)) {
            const descriptor = Object.getOwnPropertyDescriptor(item, key);
            requireValue(typeof key === 'string' && descriptor.enumerable && 'value' in descriptor, '모드 메시지 접근자/심볼 금지');
            output[key] = copy(descriptor.value, depth + 1);
        }
        return output;
    }
    const result = copy(value, 0);
    requireValue(new TextEncoder().encode(JSON.stringify(result)).length <= MOD_LIMITS.maxMessageBytes, '모드 메시지 바이트 한도 초과');
    return result;
}

// 등록 시의 미션 없는 상태다. 각 호출은 독립한 객체를 만든다.
export function emptyModSnapshot() {
    return { state: { sessionId: null, mission: null, gameSeconds: 0, localOwner: null, paused: false, logicalSize: { width: 1, height: 1 }, mapRect: { left: 0, top: 0, right: 1, bottom: 1 } }, players: [], selection: null };
}

// 원본 시계/RNG를 소비하지 않고 모드별 초기 난수 상태를 정한다.
export function modRandomSeed(sessionSeed, modId) {
    integer(sessionSeed, 0, 0xffffffff, '미션 seed');
    requireValue(typeof modId === 'string' && modId.length <= 64 && /^[a-z][a-z0-9]*(?:[.-][a-z0-9]+)*$/u.exec(modId)?.[0] === modId, '모드 id 형식 오류');
    let hash = 2166136261;
    // FNV-1a의 각 곱에서 하위 32비트만 보존한다.
    for (const byte of new TextEncoder().encode(`${sessionSeed}\n${modId}`)) hash = Math.imul(hash ^ byte, 16777619) >>> 0;
    return hash || 0x6d2b79f5;
}

// 조회 사본을 만들기 전에 공개 상태의 필수 필드와 범위를 검증한다.
export function validateModSnapshot(input) {
    const snapshot = copyModJson(input);
    record(snapshot, ['state', 'players', 'selection'], '모드 상태 묶음');
    const state = record(snapshot.state, ['sessionId', 'mission', 'gameSeconds', 'localOwner', 'paused', 'logicalSize', 'mapRect'], '게임 상태');
    requireValue(state.sessionId === null || (typeof state.sessionId === 'string' && state.sessionId.length <= 32 && /^session-[1-9][0-9]*$/u.exec(state.sessionId)?.[0] === state.sessionId), '모드 미션 식별자 오류');
    requireValue(state.mission === null || (typeof state.mission === 'string' && state.mission.length <= 128), '모드 미션 이름 오류');
    requireValue(Number.isFinite(state.gameSeconds) && state.gameSeconds >= 0 && typeof state.paused === 'boolean', '모드 게임 시계/정지 오류');
    if (state.localOwner !== null) integer(state.localOwner, 1, 8, '현재 플레이어');
    if (state.sessionId === null) requireValue(state.mission === null && state.gameSeconds === 0 && state.localOwner === null, '미션 없는 상태의 필드 오류');
    else requireValue(typeof state.mission === 'string' && state.mission.length > 0, '미션 이름 누락');
    const size = record(state.logicalSize, ['width', 'height'], '논리 크기'), rect = record(state.mapRect, ['left', 'top', 'right', 'bottom'], '지도 영역');
    integer(size.width, 1, 16384, '논리 폭'); integer(size.height, 1, 16384, '논리 높이');
    integer(rect.left, 0, size.width - 1, '지도 왼쪽'); integer(rect.top, 0, size.height - 1, '지도 위쪽');
    integer(rect.right, rect.left + 1, size.width, '지도 오른쪽'); integer(rect.bottom, rect.top + 1, size.height, '지도 아래쪽');
    requireValue(Array.isArray(snapshot.players) && snapshot.players.length <= 8, '플레이어 목록 오류');
    let previousOwner = 0;
    // 원래 플레이어 번호 순서와 동맹의 정렬/중복 금지까지 검사한다.
    for (const player of snapshot.players) {
        record(player, ['owner', 'stormPower', 'allies'], '플레이어');
        integer(player.owner, previousOwner + 1, 8, '플레이어 번호'); previousOwner = player.owner;
        integer(player.stormPower, 0, 0xffffffff, '스톰 파워');
        requireValue(Array.isArray(player.allies) && player.allies.length <= 7, '동맹 목록 오류');
        let previousAlly = 0;
        // 자신을 제외하고 증가하는 플레이어 번호만 받는다.
        for (const ally of player.allies) { integer(ally, previousAlly + 1, 8, '동맹 번호'); requireValue(ally !== player.owner, '자기 동맹 번호 금지'); previousAlly = ally; }
    }
    if (snapshot.selection !== null) {
        record(snapshot.selection, ['typeId', 'owner', 'health'], '선택 자산');
        integer(snapshot.selection.typeId, 0, 255, '선택 TYPE'); integer(snapshot.selection.owner, 0, 8, '선택 소유자'); integer(snapshot.selection.health, 0, 0xffffffff, '선택 HP');
    }
    if (state.sessionId === null) requireValue(snapshot.players.length === 0 && snapshot.selection === null, '미션 없는 조회 목록 오류');
    return snapshot;
}

// 등록·사건 동안에만 쓸 수 있는 얇은 API와 독립 난수를 소유한다. 게임 객체/큐/DOM은 소유하지 않는다.
export class ModRuntime {
    // 모드 식별자·난수·공개 객체와 현재 경계의 값 사본이다.
    #random; #api; #snapshot = emptyModSnapshot();
    // 구독 목록과 번호는 런타임 하나의 수명에서만 유효하다.
    #handlers = new Map(); #nextSubscription = 1;
    // 등록 여부·콜백 실행 여부·종료 여부와 현재 사건의 결과를 관리한다.
    #registered = false; #active = false; #disabled = false; #ending = false; #sequence = 0; #sessionId = null;
    #commands = []; #logs = [];

    // 호스트가 확보한 모드 id와 원래 미션 초기 seed의 사본만 받는다.
    constructor({ modId, sessionSeed = 0 }) {
        this.#random = modRandomSeed(sessionSeed, modId);
        // 별칭은 동일한 정규화 입력 명령을 만들어 호스트의 별도 우회 경로를 없앤다.
        const commands = Object.freeze({
            click: (x, y, button = 'left', modifiers = {}) => this.#command('click', { x, y, button, modifiers }),
            pressKey: (key, modifiers = {}) => this.#command('pressKey', { key, modifiers }),
            selectPriest: () => this.#command('pressKey', { key: 'p', modifiers: {} }),
            nextUnit: (add = false) => { this.#live(); requireValue(typeof add === 'boolean', '추가 선택은 불리언이어야 합니다.'); return this.#command('pressKey', { key: 'n', modifiers: { shift: add } }); },
            produceBridge: (slot) => { this.#live(); integer(slot, 0, 5, '다리 슬롯'); return this.#command('pressKey', { key: ['q', 'w', 'a', 's', 'z', 'x'][slot], modifiers: {} }); },
            repeatProduction: () => this.#command('pressKey', { key: 'd', modifiers: {} }),
            rotatePiece: () => this.#command('pressKey', { key: 'c', modifiers: {} }),
        });
        // 함수가 private 상태를 닫아 두고 조회는 매번 새 값으로 반환한다.
        this.#api = Object.freeze({
            apiVersion: MOD_API_VERSION, modId,
            on: (name, handler) => this.#on(name, handler),
            off: (subscription) => { this.#live(); integer(subscription, 1, Number.MAX_SAFE_INTEGER, '구독 번호'); return this.#handlers.delete(subscription); },
            getState: () => { this.#live(); return copyModJson(this.#snapshot.state); },
            getPlayers: () => { this.#live(); return copyModJson(this.#snapshot.players); },
            getSelection: () => { this.#live(); return copyModJson(this.#snapshot.selection); },
            randomInt: (limit) => this.#randomInt(limit),
            log: (level, text) => this.#log(level, text), commands,
        });
    }

    // 실패·종료·명시 반납으로 비활성화됐는지 호스트가 읽는다.
    get disabled() { return this.#disabled; }
    // 실행 콜백 바깥에서는 저장한 API 함수도 사용할 수 없다.
    #live() {
        if (this.#disabled) throw new ModError('disposed', '모드 실행 수명이 끝났습니다.');
        if (!this.#active) throw new ModError('permissionDenied', '모드 API는 동기 콜백 안에서만 사용할 수 있습니다.');
    }
    // 알려진 사건의 콜백을 구독 순서로 등록한다.
    #on(name, handler) {
        this.#live(); requireValue(MOD_EVENT_NAMES.includes(name) && typeof handler === 'function', '모드 사건/핸들러 오류');
        if (this.#handlers.size >= MOD_LIMITS.maxHandlers) throw new ModError('quotaExceeded', '모드 구독 한도 초과');
        const id = this.#nextSubscription++; this.#handlers.set(id, { name, handler }); return id;
    }
    // 모드 전용 xorshift32만 전진시킨다. 게임의 GameRandom에는 접근하지 않는다.
    #randomInt(limit) {
        this.#live(); integer(limit, 1, 0xffffffff, '난수 상한');
        let state = this.#random; state ^= state << 13; state ^= state >>> 17; state ^= state << 5; this.#random = state >>> 0;
        return this.#random % limit;
    }
    // 한 사건의 제한된 로그를 값으로 보관한다. 실제 console 출력은 호스트가 맡는다.
    #log(level, text) {
        this.#live(); requireValue(['info', 'warn', 'error'].includes(level) && typeof text === 'string' && text.length <= MOD_LIMITS.maxLogCharacters, '모드 로그 형식/길이 오류');
        if (this.#logs.length >= MOD_LIMITS.maxLogsPerEvent) throw new ModError('quotaExceeded', '모드 로그 한도 초과');
        this.#logs.push({ level, text });
    }
    // 정규화 명령을 검증해 현재 사건의 요청만 만든다. 실제 큐는 다음 호스트 경계에서 받는다.
    #command(name, input) {
        this.#live();
        if (!this.#snapshot.state.sessionId || this.#ending) throw new ModError('noSession', '입력을 보낼 활성 미션이 없습니다.');
        if (this.#commands.length >= MOD_LIMITS.maxCommandsPerEvent) throw new ModError('quotaExceeded', '모드 사건의 명령 한도 초과');
        const modifiers = record(input.modifiers, ['shift', 'control'], '보조 키');
        // 생략된 보조 키는 false로 정규화한다.
        for (const value of Object.values(modifiers)) requireValue(typeof value === 'boolean', '보조 키는 불리언이어야 합니다.');
        const args = { modifiers: { shift: modifiers.shift ?? false, control: modifiers.control ?? false } };
        if (name === 'click') {
            integer(input.x, 0, this.#snapshot.state.logicalSize.width - 1, '클릭 x'); integer(input.y, 0, this.#snapshot.state.logicalSize.height - 1, '클릭 y');
            requireValue(['left', 'right'].includes(input.button), '클릭 버튼 오류'); Object.assign(args, { x: input.x, y: input.y, button: input.button });
        } else { requireValue(MOD_KEY_NAMES.includes(input.key), '허용되지 않은 게임 키'); args.key = input.key; }
        const commandId = this.#commands.length + 1;
        this.#commands.push({ commandId, sessionId: this.#snapshot.state.sessionId, name, args }); return commandId;
    }
    // 콜백 예외가 나면 그 사건의 명령/로그를 버리고 이 모드만 반납한다.
    #invoke(callback) {
        this.#commands = []; this.#logs = []; this.#active = true;
        try {
            callback();
            return copyModJson({ eventSequence: this.#sequence, commands: this.#commands, logs: this.#logs });
        } catch (error) {
            this.dispose();
            if (error instanceof ModError) throw error;
            throw new ModError('scriptError', String(error?.message ?? error).slice(0, 2048));
        } finally { this.#active = false; this.#commands = []; this.#logs = []; }
    }
    // 반환값이 Promise/thenable이면 비동기 계약 위반으로 즉시 실패한다.
    #synchronous(result) { if (result && (typeof result === 'object' || typeof result === 'function') && typeof result.then === 'function') throw new ModError('scriptError', '모드 콜백은 동기로 종료해야 합니다.'); }
    // 엔트리 default 함수를 한 번만 등록한다. 실제 import/시간 제한은 격리 호스트의 책임이다.
    register(registerMod) {
        if (this.#disabled) throw new ModError('disposed', '모드 실행 수명이 끝났습니다.');
        requireValue(!this.#registered && typeof registerMod === 'function', '모드 엔트리는 한 번만 등록할 수 있습니다.');
        this.#registered = true; return this.#invoke(() => this.#synchronous(registerMod(this.#api)));
    }
    // 호스트 사건과 상태 사본을 검증한 뒤 사건 시작 때의 구독 목록에 한 번 전달한다.
    dispatch(input, snapshot) {
        if (this.#disabled) throw new ModError('disposed', '모드 실행 수명이 끝났습니다.');
        requireValue(this.#registered && !this.#active, '모드 등록/재진입 순서 오류');
        const event = copyModJson(input), state = validateModSnapshot(snapshot);
        requireValue(MOD_EVENT_NAMES.includes(event.name), '알 수 없는 모드 사건');
        integer(event.sequence, this.#sequence + 1, this.#sequence + 1, '사건 번호');
        requireValue(typeof event.sessionId === 'string' && event.sessionId === state.state.sessionId, '사건/상태의 미션 식별자 불일치');
        if (event.name === 'sessionStart') requireValue(this.#sessionId === null && event.mission === state.state.mission, '미션 시작 순서/이름 오류');
        else requireValue(this.#sessionId === event.sessionId, '현재 모드 미션과 다른 사건');
        requireValue(state.state.gameSeconds >= this.#snapshot.state.gameSeconds, '모드 게임 시계가 이전 경계보다 작습니다.');
        const keys = ['name', 'sequence', 'sessionId'];
        if (event.name === 'commandResult') {
            keys.push('results'); requireValue(Array.isArray(event.results) && event.results.length <= MOD_LIMITS.maxCommandResultsPerEvent, '명령 결과 목록 오류');
            // 같은 요청의 중복 통지를 거절하고 원래 요청 사건/번호 순서를 보존한다.
            let previousSequence = 0, previousCommand = 0;
            // 각 입력 수용 결과도 미지정 코드를 통과시키지 않는다.
            for (const result of event.results) {
                record(result, ['eventSequence', 'commandId', 'status', 'code'], '명령 결과'); integer(result.eventSequence, 1, this.#sequence, '요청 사건 번호'); integer(result.commandId, 1, MOD_LIMITS.maxCommandsPerEvent, '명령 번호');
                requireValue(result.eventSequence > previousSequence || (result.eventSequence === previousSequence && result.commandId > previousCommand), '명령 결과 중복/순서 오류');
                previousSequence = result.eventSequence; previousCommand = result.commandId;
                requireValue((result.status === 'queued' && result.code === null) || (result.status === 'rejected' && MOD_ERROR_CODES.includes(result.code)), '명령 결과 상태/코드 오류');
            }
        } else {
            keys.push('gameSeconds'); requireValue(event.gameSeconds === state.state.gameSeconds, '사건/상태의 게임 시각 불일치');
            if (event.name !== 'tick') { keys.push('mission'); requireValue(event.mission === state.state.mission, '사건/상태의 미션 이름 불일치'); }
            if (event.name === 'sessionEnd') { keys.push('reason'); requireValue(['leave', 'restart', 'replace', 'destroy'].includes(event.reason), '미션 종료 사유 오류'); }
        }
        record(event, keys, '모드 사건'); this.#snapshot = state; this.#sequence = event.sequence; this.#sessionId = event.sessionId; this.#ending = event.name === 'sessionEnd';
        const handlers = [...this.#handlers.values()].filter((subscription) => subscription.name === event.name);
        const output = this.#invoke(() => {
            // 콜백별 사건 사본도 분리해 앞 콜백의 수정을 다음 콜백에 전파하지 않는다.
            for (const { handler } of handlers) this.#synchronous(handler(copyModJson(event)));
        });
        if (this.#ending) this.dispose(); return output;
    }
    // 반납 이후 이전 콜백·조회·사건은 모두 거부하고 강한 상태 참조를 놓는다.
    dispose() { this.#disabled = true; this.#handlers.clear(); this.#snapshot = emptyModSnapshot(); this.#commands = []; this.#logs = []; }
}
