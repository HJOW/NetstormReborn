// AI 전체 행동 1회·개별 Tick·Player 1..8 자동 방문/삭제의 수명 순서를 옮긴 모듈이다(cpppj `o/RawAiRuntime.cpp/.h`, 10.78: 패치 00417c70 Actions·00417d30 Tick·00417de0 Frame).
//  - new RawAiRuntime(memory, players, globals, hooks): memory 는 E164 `RawAiBrainMemory`(실제 0xb08 본체 소유자), players 는 실제 9×180 BYTE Player 기록(+0xb0 이 현재 brain 핸들)을 빌린다(복사하지 않는다).
//    globals = { battle()(00594fbc 현재 전투 모드, server/boss 와 합치지 않는다), timing()(0052f3c4 현재 실행 시간 로그 DWORD), dryLog()(0052f3dc 현재 부족 좌표 로그 종류) }는 각 원본 읽기 위치에서 현재 값을 다시 읽는 getter 이다.
//    hooks = { initialize·prepare·selectGoal(token)(00414af0/00416eb0/004139f0), collectors·factories(token)(00415a10→004136b0), dryScan(token)→제거 TYPE(0040f2b0, 0 이면 제거 없음), dryNotice(token, kind, fake)(004105b0),
//    priest(token, dword)(00411cd0, 현재 +0x29c 전체 DWORD), drops·bridges·carriers·economy(token)(004162e0→00417bd0→00412130→004138c0), money(owner)→float(0040ef30), wall()→double(00460d70, 매 호출 현재 벽시각),
//    took(token, elapsed)(0.1초를 엄격히 넘을 때 로그 장치), assertion(text, line)(004e0620, 반환하면 원본처럼 계속), destruct(token)·free(token)(Player+b0 본체의 00410100 → CRT free) } 이며 도달한 위치에서만 요구하고 없으면 명시 오류이다.
//  - actions(token): 살아 있는 정확한 0xb08 본체 확인 → 수집자 → 시설 → 부족 좌표 → (제거 TYPE 이 있으면 현재 로그 종류와 `제거 TYPE === 현재 +0x280` 비교를 알림) → 사제 → 배치 → 다리 → 운반자 → 경제 → 지갑 정수를 float 으로 먼저 저장해
//    현재 SP float 와 비교(다르거나 NaN 이면 `money == getMoney( playerId )`, 줄 0x1048 진단 뒤 계속)한다. 각 위치에서 본체 필드는 다시 읽는다.
//  - tick(token): +0x254 가 비제로면 false 만 반환한다. +0x258 이 0 이면 초기화 후 다시 읽어 0 이면 본체를 유지한 채 true 이다. 시작 벽시각 → 준비 → 목표 선택 → 행동 뒤 현재 timing 이 비제로일 때만 끝 벽시각을 읽어 0.1 초 초과분을 알린다.
//  - frame(): 현재 전투일 때 Player 1..8 의 현재 brain 을 방문해 tick 이 false 이면 그 시점에 현재 +0xb0 을 다시 읽어 있으면 소멸 → 반납하고 +0xb0 을 0 으로 쓴다(Player 의 다른 필드는 보존).
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E165: AI 런타임 이식으로 추가)
import { AI_BRAIN_BYTES } from './raw-ai-brain.js';

// 원본 Player 기록 폭/개수, 현재 brain 필드 위치, 00501960 의 느린 프레임 기준 시간(초, double)이다. (2026-11-09 WB04-E165)
const PLAYER_BYTES = 180;
const PLAYERS = 9;
const BRAIN = 0xb0;
const SLOW_FRAME = 0.1;
// 지갑 불일치 진단 문자열과 줄 번호이다. (2026-11-09 WB04-E165)
const MONEY_ASSERT = 'money == getMoney( playerId )';
const MONEY_LINE = 0x1048;

// 비정렬 DWORD 네 바이트만 little endian 으로 조회하며 범위 밖은 거부한다(주변 바이트를 읽지 않는다). (2026-11-09 WB04-E165)
function read(raw, at) {
    if (at < 0 || at > raw.length || raw.length - at < 4) throw new RangeError('AI 프레임 DWORD 범위');
    return (raw[at] | (raw[at + 1] << 8) | (raw[at + 2] << 16) | (raw[at + 3] << 24)) >>> 0;
}

// 현재 Player 의 지정 네 바이트만 쓴다(나머지 Player 상태는 보존). (2026-11-09 WB04-E165)
function write(raw, at, value) {
    for (let index = 0; index < 4; index += 1) raw[at + index] = (value >>> (index * 8)) & 0xff;
}

// 도달한 하위가 없으면 빈 성공으로 대신하지 않고 그 위치에서 오류로 보고한다. (2026-11-09 WB04-E165)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('AI 전체 프레임 실제 하위 미연결');
    return hook;
}

// AI 런타임이다. 본체/Player/게임 계산을 생성자에서 바꾸지 않으며 모든 효과는 명시 hook 으로 위임한다. (2026-11-09 WB04-E165)
export class RawAiRuntime {
    #memory;
    #players;
    #globals;
    #hooks;

    // 실제 9 Player 기록과 필수 현재 전역 getter 를 확인한다. (2026-11-09 WB04-E165)
    constructor(memory, players, globals, hooks) {
        if (!memory || typeof memory.bytes !== 'function') throw new TypeError('AI 프레임 실제 본체 소유자 누락');
        if (!(players instanceof Uint8Array) || players.length !== PLAYER_BYTES * PLAYERS) throw new RangeError('AI 프레임 실제9Player 누락');
        if (typeof globals?.battle !== 'function' || typeof globals.timing !== 'function' || typeof globals.dryLog !== 'function') throw new TypeError('AI 프레임 현재 전역 조회기 누락');
        this.#memory = memory;
        this.#players = players;
        this.#globals = globals;
        this.#hooks = hooks ?? {};
    }

    // 해제된 본체/잘못된 크기의 가짜 본체를 거부하고 현재 살아 있는 바이트를 빌린다. (2026-11-09 WB04-E165)
    #body(token) {
        const raw = this.#memory.bytes(token);
        if (raw.length !== AI_BRAIN_BYTES) throw new RangeError('AI 프레임 실제0xb08 본문 누락');
        return raw;
    }

    // 각 원본 위치에서 현재 본체 DWORD 를 다시 조회한다. (2026-11-09 WB04-E165)
    #field(token, at) {
        return read(this.#body(token), at);
    }

    // 00417c70: 지갑 정수를 float 으로 먼저 저장해 현재 SP float 와 비교한다(NaN 은 다름). (2026-11-09 WB04-E165)
    actions(token) {
        const hooks = this.#hooks;
        this.#body(token);
        need(hooks.collectors)(token);
        need(hooks.factories)(token);
        const removed = need(hooks.dryScan)(token) >>> 0;
        if (removed) need(hooks.dryNotice)(token, this.#globals.dryLog() >>> 0, removed === this.#field(token, 0x280));
        need(hooks.priest)(token, this.#field(token, 0x29c));
        need(hooks.drops)(token);
        need(hooks.bridges)(token);
        need(hooks.carriers)(token);
        need(hooks.economy)(token);
        const wallet = Math.fround(this.#field(token, 0x2d8) | 0);
        const money = Math.fround(need(hooks.money)(this.#field(token, 0x260)));
        if (!(wallet === money)) need(hooks.assertion)(MONEY_ASSERT, MONEY_LINE);
    }

    // 00417d30: 초기화 효과 뒤 +0x258 을 다시 읽고 타이밍 전역은 전체 행동 뒤 현재 값을 읽는다. (2026-11-09 WB04-E165)
    tick(token) {
        const hooks = this.#hooks;
        if (this.#field(token, 0x254)) return false;
        if (!this.#field(token, 0x258)) need(hooks.initialize)(token);
        if (!this.#field(token, 0x258)) return true;
        const started = need(hooks.wall)();
        need(hooks.prepare)(token);
        need(hooks.selectGoal)(token);
        this.actions(token);
        if (this.#globals.timing() >>> 0) {
            const elapsed = need(hooks.wall)() - started;
            if (elapsed > SLOW_FRAME) need(hooks.took)(token, elapsed);
        }
        return true;
    }

    // 00417de0: +0x254 종료 때 현재 Player 본체를 재조회하고 소멸 → 반납 뒤 같은 +0xb0 을 0 으로 쓴다. (2026-11-09 WB04-E165)
    frame() {
        const hooks = this.#hooks;
        if (!need(this.#globals.battle)()) return;
        // 현재 Player 1..8 을 같은 stride 로 차례로 방문한다(0 번 중립 Player 는 대상이 아니다).
        for (let owner = 1; owner < PLAYERS; owner += 1) {
            const at = owner * PLAYER_BYTES + BRAIN;
            const token = read(this.#players, at);
            if (!token || this.tick(token)) continue;
            const current = read(this.#players, at);
            if (current) {
                need(hooks.destruct)(current);
                need(hooks.free)(current);
            }
            write(this.#players, at, 0);
        }
    }
}
