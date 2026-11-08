// 실제 AI 본체(0xb08 바이트)/목록의 소유 메모리와 패치 00416860(생성자)/00410100(소멸자)을 옮긴 모듈이다(cpppj `o/RawAiBrain.cpp/.h`, 10.78).
//  - RawAiBrainMemory(firstToken, fill): 원본 32비트 pointer 필드를 호스트 64비트 포인터로 역참조하지 않는 실제 소유 메모리 등록부이다. allocate(size)는 8바이트 정렬 크기만큼 다음 opaque32 핸들(비제로)을 올리며
//    할당 직후의 미초기화 바이트는 fill(bytes)가 공급한다(생성자는 이후 실제로 쓰는 바이트만 바꾼다). free(0)은 CRT free 처럼 무동작이고 없는 핸들 해제는 거부하며, 해제한 블록은 다시 조회할 수 없다.
//    bytes(token)/contains(token)/allocations/frees 는 살아 있는 블록·할당 순서/크기·해제 순서이다.
//  - RawAiBrainConstructor(memory, player, participation, islands, allies, types, globals, hooks): player(RawPlayerWorld: 9기록 1620바이트·같은 풀)·participation(RawPlayerParticipation)·islands(RawIslandRuntime)·allies(방향별 81 DWORD)·
//    types(현재 TYPE 번호순 flags2 표)는 같은 실제 세계의 저장소이다. globals = { localOwner()(00540c70), noRegion()(0050c038, 첫 지역 없음 DWORD), invalidGraph()(본체 DWORD 에 쓰는 현재 00540410 값) }(모두 필수 getter),
//    hooks = { priest(owner)→SID(004921f0), regionAt(x, y)→지역(0046f530), requiredEnergy(type)→문자열(TYPE+a0) } 이며 도달한 경계가 없으면 명시 오류이다.
//  - construct(owner, settings, bytes = 0xb08): 0xb08 본체 할당 → 00416860 이며 목록 생성 실패 때도 소유자가 정리한다. initialize(token, owner, settings): 이미 할당한 0xb08 블록에 원본 생성자만 적용한다 — 600바이트 설정 복사,
//    고정 필드/double 0/17개 목록(0x284 포함 16개의 포인터·capacity·count 와 0xae4 의 120바이트) 할당과 필드 쓰기, 현재 Player 첫 지역이 '없음'이면 사제 위치의 지역을 찾아 섬 owner 와 Player 첫 지역에 쓰기, flags2 0x40800 인 첫 'ss' 요구 에너지 TYPE 저장,
//    방향별 동맹(SetAlliance/AddAlly)이다. 실패하면 이미 만든 목록을 역순으로 해제한다. destruct(token): 00410100 — 0xae4 가 있으면 free 후 16개 목록 포인터를 역순 free(본체는 아직 살아 있음). freeBody(token): 소멸한 본체만 해제한다.
//  - createAi(...): 아래 RawAiCreate 가 PATCH 00417e50 의 색 변경 → 기존 brain → NoTemple → 실제 생성 → 부착 → 초기 갱신 순서를 감싼다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E164: AI 본체 생성/소멸 이식으로 추가)
import { MISSION_AI_SETTINGS_SIZE } from './raw-mission-ai-settings.js';

// 원본 −10000.0f 비트, 실제 본체 크기, Player 한 기록 폭과 첫 지역 위치, 9기록 크기, 방향별 동맹표 칸 수이다. (2026-11-09 WB04-E164)
const UNSET = 0xc61c4000;
export const AI_BRAIN_BYTES = 0xb08;
const PLAYER_STRIDE = 180;
const FIRST_REGION = 0x78;
const PLAYER_BYTES = 1620;
const ALLY_CELLS = 81;
// 실제 16개 pointer/capacity/count 필드 순서이다: [pointer 위치, capacity]. (2026-11-09 WB04-E164)
const LISTS = Object.freeze([[0x284, 10], [0x2dc, 500], [0x2e8, 500], [0x2f4, 500], [0x300, 4], [0x30c, 5], [0x318, 5], [0x324, 40], [0x330, 500], [0x33c, 500], [0x348, 500], [0x354, 500], [0x360, 500], [0x36c, 500], [0x378, 40], [0x384, 2]]);
// 0xae4 할당 전에 원본이 쓰는 고정 필드와 첫 지역/metadata 조회 전의 고정 필드, 00500460 의 실제 double 0 위치이다. (2026-11-09 WB04-E164)
const FIRST_FIELDS = Object.freeze([[0x3cc, 0], [0x3d4, UNSET], [0x3d0, UNSET], [0x3d8, 1], [0x3dc, 0], [0x3f8, 0], [0x404, 0], [0x3e0, 0], [0x3e4, 6], [0x3f0, 1], [0x3f4, 0], [0x3fc, 0], [0x400, 0], [0x3ec, 0], [0x574, 0], [0x5a4, 0], [0x58c, 0], [0x590, 0], [0x5a0, 0], [0x59c, 0], [0x598, 0], [0x594, 0], [0x628, 0], [0x5f4, 0], [0x5f0, 0], [0xadc, 10], [0xae0, 0]]);
const DOUBLES = Object.freeze([0x5a8, 0x5b0, 0x5b8, 0x5c0, 0x5c8, 0x5d0, 0x5d8, 0x5e0, 0x5e8, 0x630]);
const LAST_FIELDS = Object.freeze([[0x2d4, 0], [0x264, 0], [0x2a4, 0], [0x258, 0], [0x25c, 0], [0x6d0, 0], [0xad8, 2], [0x390, 3], [0xad4, UNSET], [0xad0, UNSET], [0xaf4, UNSET], [0xaf0, UNSET], [0x3e0, 0], [0x3e4, 6], [0x3f0, 1], [0x3f4, 0], [0x3fc, 0], [0x400, 0], [0x3ec, 0], [0x3b0, 0], [0x394, 0], [0x2cc, 0], [0x2c8, 0], [0x2d0, 0], [0x290, 0], [0x3c8, 3], [0xaf8, 10], [0xafc, 10], [0xb00, 0], [0x294, 0]]);

// 비정렬 원본 DWORD 를 소유 메모리에서 읽는다. (2026-11-09 WB04-E164)
function read(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 원본 DWORD 를 같은 소유 메모리에 쓴다(주변 미초기화 byte 는 보존한다). (2026-11-09 WB04-E164)
function put(raw, offset, value) {
    for (let index = 0; index < 4; index += 1) raw[offset + index] = (value >>> (index * 8)) & 0xff;
}

// 선택된 실제 지면/metadata 효과가 없으면 명시 오류로 보고한다. (2026-11-09 WB04-E164)
function need(hook, name) {
    if (typeof hook !== 'function') throw new Error(name);
    return hook;
}

// 원본 TYPE 요구 에너지 문자열이 'ss'(ASCII 대소문자 무시, 종료 문자 앞)인지 본다. (2026-11-09 WB04-E164)
function doubleSun(value) {
    const text = String(value).split('\0')[0];
    return text.length === 2 && (text[0] === 's' || text[0] === 'S') && (text[1] === 's' || text[1] === 'S');
}

// 실제 소유 블록의 opaque32 핸들 등록부이다. 원본 가상 주소와 같음을 주장하지 않는다. (2026-11-09 WB04-E164)
export class RawAiBrainMemory {
    #next;
    #fill;
    #blocks = new Map();
    #allocations = [];
    #frees = [];

    // 첫 핸들은 비제로이며 각 블록 크기를 8바이트 경계로 정렬해 다음 핸들을 정한다. 미초기화 byte 입력 공급자는 필수이다. (2026-11-09 WB04-E164)
    constructor(firstToken, fill) {
        if (!firstToken || typeof fill !== 'function') throw new TypeError('AI 실제 메모리 핸들/미초기화 byte 입력 누락');
        this.#next = firstToken >>> 0;
        this.#fill = fill;
    }

    // 실제 host 소유 블록을 만들고 등록된 핸들을 돌려준다. 주소 공간 소진은 예외이며 NULL 성공을 합성하지 않는다. (2026-11-09 WB04-E164)
    allocate(size) {
        const step = Math.floor((size + 7) / 8) * 8;
        if (!size || step > 0xffffffff - this.#next) throw new RangeError('AI 실제 소유 메모리 주소 공간 소진');
        const block = new Uint8Array(size);
        this.#fill(block);
        const token = this.#next;
        if (this.#blocks.has(token)) throw new Error('AI 메모리 핸들 중복');
        this.#blocks.set(token, block);
        this.#next += step;
        this.#allocations.push({ token, size });
        return token;
    }

    // NULL 은 CRT free 처럼 무동작이며 유효 블록은 해제 뒤 다시 조회할 수 없다. (2026-11-09 WB04-E164)
    free(token) {
        if (!token) return;
        if (!this.#blocks.delete(token)) throw new Error('AI 실제 소유자 없는 메모리 해제');
        this.#frees.push(token);
    }

    // 현재 살아 있는 실제 블록 전체를 빌린다. 가짜 표식/해제 토큰은 거부한다. (2026-11-09 WB04-E164)
    bytes(token) {
        const block = this.#blocks.get(token);
        if (!block) throw new RangeError('AI 살아 있는 소유 메모리 누락');
        return block;
    }

    // 실제 할당 수명 조회이다. (2026-11-09 WB04-E164)
    contains(token) { return this.#blocks.has(token); }

    // 관찰된 실제 할당 순서/크기와 해제 순서의 사본이다. (2026-11-09 WB04-E164)
    get allocations() { return this.#allocations.map((entry) => ({ ...entry })); }
    get frees() { return this.#frees.slice(); }
}

// 실제 AI 본체 생성자/소멸자이다. 전략 전체/초기 update 를 대체하지 않는다. (2026-11-09 WB04-E164)
export class RawAiBrainConstructor {
    #memory;
    #player;
    #participation;
    #islands;
    #allies;
    #types;
    #globals;
    #hooks;
    #live = new Set();
    #destructed = new Set();

    // 현재 Player 9/섬 기록과 같은 풀/방향별 81 DWORD 만 연결한다. (2026-11-09 WB04-E164)
    constructor(memory, player, participation, islands, allies, types, globals, hooks) {
        if (player?.pool?.edition !== 'patch1078' || player.pool !== islands?.pool || player.records?.length !== PLAYER_BYTES || allies?.length !== ALLY_CELLS || !Array.isArray(types) || types.length === 0
            || typeof globals?.invalidGraph !== 'function' || typeof globals.localOwner !== 'function' || typeof globals.noRegion !== 'function' || !memory || !participation) {
            throw new TypeError('AI 생성자 같은 PATCH 세계/Player9/동맹표/현재 그래프 조회 누락');
        }
        this.#memory = memory;
        this.#player = player;
        this.#participation = participation;
        this.#islands = islands;
        this.#allies = allies;
        this.#types = types;
        this.#globals = globals;
        this.#hooks = hooks ?? {};
    }

    // 상위 세계 바인딩이 같은 풀인지 검증할 수 있도록 제공한다. (2026-11-09 WB04-E164)
    get pool() { return this.#player.pool; }

    // 후속 초기 update/전략이 동일 실제 원문을 읽고 쓰게 한다. (2026-11-09 WB04-E164)
    bytes(token) { return this.#memory.bytes(token); }

    // 실제 0xb08 본체 할당 → 00416860 을 수행한다. 목록 생성 실패 때도 소유자가 본체를 정리한다. (2026-11-09 WB04-E164)
    construct(owner, settings, bytes = AI_BRAIN_BYTES) {
        if (bytes !== AI_BRAIN_BYTES) throw new RangeError('AI 실제 본체 크기는 0xb08 입니다');
        const token = this.#memory.allocate(AI_BRAIN_BYTES);
        try { this.initialize(token, owner, settings); } catch (error) { this.#memory.free(token); throw error; }
        return token;
    }

    // 미초기화 영역 전체를 지우지 않고 원본이 실제로 쓰는 field/목록만 구성한다. 생성 중 예외면 이미 만든 목록을 역순으로 정리한다. (2026-11-09 WB04-E164)
    initialize(token, owner, settings) {
        if (settings?.length !== MISSION_AI_SETTINGS_SIZE || owner >= 9 || this.#live.has(token) || this.#destructed.has(token)) throw new TypeError('AI 생성자 실제 설정/owner/새 본체 계약 오류');
        const memory = this.#memory;
        const raw = memory.bytes(token);
        if (raw.length !== AI_BRAIN_BYTES) throw new TypeError('AI 생성자 실제 0xb08 소유 메모리 누락');
        const made = [];
        // 생성 중 예외 시 실제 구성된 목록만 역순 정리하도록 기록하며 할당한다.
        const allocate = (bytes) => { const block = memory.allocate(bytes); made.push(block); return block; };
        const globals = this.#globals;
        try {
            raw.set(settings, 0);
            put(raw, 0x274, UNSET);
            put(raw, 0x270, UNSET);
            put(raw, 0x27c, UNSET);
            put(raw, 0x278, UNSET);
            put(raw, 0x284, allocate(40));
            put(raw, 0x288, 10);
            put(raw, 0x28c, 0);
            put(raw, 0x2bc, UNSET);
            put(raw, 0x2b8, UNSET);
            put(raw, 0x2c4, UNSET);
            put(raw, 0x2c0, UNSET);
            // 첫 목록 뒤 15개는 원본 capacity×DWORD 크기와 포인터/capacity/count 순서다.
            for (let index = 1; index < LISTS.length; index += 1) {
                const [offset, capacity] = LISTS[index];
                put(raw, offset, allocate(capacity * 4));
                put(raw, offset + 4, capacity);
                put(raw, offset + 8, 0);
            }
            // 원본 상수와 zero 필드를 같은 실제 storage 에 쓴다.
            for (const [offset, value] of FIRST_FIELDS) put(raw, offset, value);
            // 원본 double 0 의 두 DWORD 만 변경한다.
            for (const offset of DOUBLES) { put(raw, offset, 0); put(raw, offset + 4, 0); }
            put(raw, 0xaec, owner);
            put(raw, 0xae8, 0);
            put(raw, 0xae4, allocate(120));
            put(raw, 0x2d8, read(raw, 0x22c));
            put(raw, 0x260, owner);
            // 원본 고정 배열 255 DWORD 만 0 으로 만든다.
            for (let index = 0; index < 255; index += 1) put(raw, 0x6d4 + index * 4, 0);
            for (const [offset, value] of LAST_FIELDS) put(raw, offset, value);
            put(raw, 0x298, globals.invalidGraph() >>> 0);
            put(raw, 0x2a8, 0);
            put(raw, 0x2ac, globals.invalidGraph() >>> 0);
            const records = this.#player.records;
            // callback 뒤에도 현재 brain owner 를 다시 읽는다.
            const playerOffset = () => {
                const current = read(raw, 0x260);
                if (current >= 9) throw new RangeError('AI 현재 owner 실제 9기록 범위 오류');
                return current * PLAYER_STRIDE;
            };
            if ((read(records, playerOffset() + FIRST_REGION) | 0) === (globals.noRegion() | 0)) {
                const priest = need(this.#hooks.priest, 'AI 생성자 실제 사제 조회 누락')(read(raw, 0x260));
                if (priest > 0 && priest < this.#player.pool.capacity) {
                    const piece = this.#player.pool.slot(priest);
                    const view = new DataView(new ArrayBuffer(8));
                    view.setUint32(0, read(piece, 14), true);
                    view.setUint32(4, read(piece, 18), true);
                    const region = need(this.#hooks.regionAt, 'AI 생성자 실제 좌표 지역 조회 누락')(view.getFloat32(0, true), view.getFloat32(4, true)) | 0;
                    if (this.#islands.exists(region) !== 0) {
                        this.#islands.setOwner(region, read(raw, 0x260));
                        put(records, playerOffset() + FIRST_REGION, region >>> 0);
                    }
                }
            }
            put(raw, 0x280, 0);
            // 실제 flags 0x40800 후 요구 에너지 문자열 ss 에 맞는 첫 타입만 저장한다.
            for (let type = 0; type < this.#types.length; type += 1) {
                if ((this.#types[type].flags2 & 0x40800) !== 0 && doubleSun(need(this.#hooks.requiredEnergy, 'AI 생성자 실제 TYPE 에너지 문자열 누락')(type))) { put(raw, 0x280, type); break; }
            }
            if ((raw[0x248] & 1) === 0) { put(raw, 0x370, 0); put(raw, 0x37c, 0); }
            this.#participation.setAlliance(read(raw, 0x260), read(raw, 0x23c));
            const current = read(raw, 0x260);
            const local = globals.localOwner() >>> 0;
            if (current >= 9 || local >= 9) throw new RangeError('AI 현재 방향별 동맹 범위 오류');
            if (this.#allies[current * 9 + local]) this.#participation.addAlly(local, read(raw, 0x260));
            this.#live.add(token);
        } catch (error) {
            for (let index = made.length - 1; index >= 0; index -= 1) memory.free(made[index]);
            throw error;
        }
    }

    // 소멸자는 본체의 현재 pointer field 를 읽는다: 0xae4 가 있으면 free 한 뒤 원본은 capacity 가 0 으로 바뀌어도 16개 목록 포인터를 역순으로 해제한다(본체는 아직 살아 있다). (2026-11-09 WB04-E164)
    destruct(token) {
        if (!this.#live.has(token)) throw new Error('AI 실제 생성자 완료 없는 소멸 요청');
        const raw = this.#memory.bytes(token);
        if (read(raw, 0xae4)) this.#memory.free(read(raw, 0xae4));
        for (let index = LISTS.length - 1; index >= 0; index -= 1) this.#memory.free(read(raw, LISTS[index][0]));
        this.#live.delete(token);
        this.#destructed.add(token);
    }

    // Participation 의 destructBrain 뒤 freeBrain 에 분리 연결한다: 이미 소멸한 본체만 해제한다. (2026-11-09 WB04-E164)
    freeBody(token) {
        if (!this.#destructed.delete(token)) throw new Error('AI 실제 목록 소멸 전 본체 해제 요청');
        this.#memory.free(token);
    }
}

// 도달한 큰 생성자/초기 갱신이 없으면 명시 오류로 보고하는 필수 경계 확인이다. (2026-11-09 WB04-E164)
function require(hook, name) {
    if (typeof hook !== 'function') throw new Error(name);
    return hook;
}

// PATCH 00417e50 의 실제 AI 생성/기존 소유자/NoTemple 순서를 엄격한 생성자 경계로 감싸는 wrapper 이다. brain 을 합성하지 않으며 생성자와 initialUpdate 가 없으면 flags 20 을 켜지 않는다. (2026-11-09 WB04-E164)
export class RawAiCreate {
    #players;
    #hooks;

    // 같은 현재 Player 기록의 참여/수명 처리기와 hooks = { changeColor(owner, color), firstRegion(owner), islandTemple(region), destroyTemple(temple, flags), construct(owner, settings, bytes)→token, initialUpdate(token, settings) } 를 빌린다. (2026-11-09 WB04-E164)
    constructor(players, hooks) {
        if (!players) throw new TypeError('AiCreate 참여 처리기 누락');
        this.#players = players;
        this.#hooks = hooks ?? {};
    }

    // Color → 기존 brain → NoTemple → 실제 생성 → 부착 → 초기 갱신 순이다. 기존 brain 에는 갱신을 재호출하지 않는다. (2026-11-09 WB04-E164)
    create(owner, settings) {
        if (settings?.length !== MISSION_AI_SETTINGS_SIZE) throw new TypeError('AiCreate 실제 600바이트 설정 누락');
        // 색/신전/실제 본체 할당보다 먼저 닫힌 소비자 수명을 확인한다.
        if (this.#players.brainReturnedForState(owner)) throw new Error('Native 반납된 AI owner의 새 생성 금지');
        const hooks = this.#hooks;
        const color = read(settings, 0x250);
        if (color !== 0) require(hooks.changeColor, 'AiCreate 실제 색 변경 누락')(owner, color);
        const old = this.#players.brain(owner);
        if (old !== 0) return old;
        if (read(settings, 0x244) !== 0) {
            const region = require(hooks.firstRegion, 'AiCreate 실제 첫 지역 조회 누락')(owner);
            const temple = require(hooks.islandTemple, 'AiCreate 실제 지역 신전 조회 누락')(region);
            if (temple !== 0) require(hooks.destroyTemple, 'AiCreate 실제 신전 삭제 누락')(temple, 0x400000);
            return 0;
        }
        // 공급 누락으로 부착만 성공하지 않게 두 큰 몸체를 생성 전에 확인한다.
        require(hooks.construct, 'AI 실제 생성자 00416860 미복원');
        require(hooks.initialUpdate, 'AI 실제 초기 갱신 00414710 미복원');
        const brain = hooks.construct(owner, settings, AI_BRAIN_BYTES);
        if (!brain) throw new Error('AI 실제 생성 소유자가 NULL 을 반환했습니다');
        this.#players.attachBrain(owner, brain);
        hooks.initialUpdate(brain, settings);
        return brain;
    }
}
