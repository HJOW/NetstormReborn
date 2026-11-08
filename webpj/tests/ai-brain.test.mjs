// AI 본체(0xb08) 생성/소멸 raw-ai-brain.js(RawAiBrainMemory·RawAiBrainConstructor·RawAiCreate)를 실제 PE 관측 160행과 cpppj 계약 시험으로 검사한다.
//  - fixture ai-brain-x86.tsv: 열 = 입력 9개, 본체 0xb08 전체, Player 9기록 1620바이트, 방향별 동맹 81 DWORD(324바이트), 섬 3204바이트, 실제 할당 "핸들,크기,Adler" 순서, 해제 순서, 사제/지역/요구 에너지 호출 trace.
//    각 행마다 미초기화 입력(할당 순서별 byte 패턴)과 600바이트 설정 원문을 같은 식으로 준비해 initialize 를 호출하고 모든 출력을 비교한 뒤, destruct 의 해제 순서/본체의 계속 생존/freeBody 의 해제를 확인한다.
//  - cpppj 계약 시험 3건(참여 수명과 초기 갱신 누락의 정리, 두 번의 현재 그래프 BYTE 재조회, 현재 그래프 조회기 누락 거부)과 메모리/생성자/AiCreate 순서의 추가 계약을 옮겼다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E164: AI 본체 생성/소멸 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, adler32, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidPostPopList, SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { PlayerWorldState, RawPlayerWorld } from '../www/core/raw-player-world.js';
import { PlayerParticipationState, RawPlayerParticipation } from '../www/core/raw-player-participation.js';
import { ISLAND_STORAGE_BYTES, RawIslandRuntime, createFortFinishOwnerState } from '../www/core/raw-island-runtime.js';
import { AI_BRAIN_BYTES, RawAiBrainConstructor, RawAiBrainMemory, RawAiCreate } from '../www/core/raw-ai-brain.js';
import { MISSION_AI_SETTINGS_SIZE } from '../www/core/raw-mission-ai-settings.js';

// 원본 실행기 첫 핸들, 행 수, 열 수이다. (2026-11-09 WB04-E164)
const BRAIN_BASE = 0x13000000;
const ROWS = 160;
const COLUMNS = 8;
const rows = (readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'ai-brain-x86.tsv')) ?? []).filter((row) => row[0] !== 'input');

// 예외가 나는지 본다. (2026-11-09 WB04-E164)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 16진 문자열로 바꾼다. (2026-11-09 WB04-E164)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 선택하지 않은 Player 생성/삭제 부모 서비스는 빈 성공을 공급하지 않고 즉시 거부한다. (2026-11-09 WB04-E164)
function missing() { throw new Error('AI 생성자 검사에서 선택되지 않은 실제 하위입니다'); }

// 같은 풀/Player 9기록/동맹표/섬 목록/참여 처리기/소유 메모리를 한 장면에 소유한다. (2026-11-09 WB04-E164)
class BrainScene {
    // 기대 필드 계산 없이 원본 실행기와 같은 입력 패턴만 준비한다.
    constructor(values) {
        this.input = values.split(',').map(Number);
        assert.equal(this.input.length, 9);
        this.pool = new SidPool(32768, { server: true });
        this.types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));
        this.energy = new Array(188).fill('');
        this.book = new SquidPostPopState();
        this.post = new SquidPostPop(this.pool, this.types, this.book);
        this.records = new Uint8Array(1620);
        this.playerState = new PlayerWorldState();
        this.allies = new Uint32Array(81);
        this.hidden = new Uint8Array(9);
        this.participationState = new PlayerParticipationState();
        this.hash = new SquidHash();
        this.islandBytes = new Uint8Array(ISLAND_STORAGE_BYTES);
        this.modes = createFortFinishOwnerState();
        this.residences = new SquidPostPopList();
        this.revision = { value: 0 };
        this.ownerChanged = { value: 0 };
        this.invalidGraph = 254;
        this.noRegion = 127;
        this.allocationIndex = 0;
        this.trace = '';
        const input = this.input;
        this.playerState.battleOwner = input[1] >>> 0;
        this.participationState.localOwner = this.playerState.battleOwner;
        put(this.records, input[0] * 180 + 0x78, input[2] >>> 0);
        // 원본과 같은 미초기화 600바이트 설정 입력이다.
        this.profile = new Uint8Array(MISSION_AI_SETTINGS_SIZE);
        for (let index = 0; index < this.profile.length; index += 1) this.profile[index] = (index * 37 + input[8]) & 0xff;
        put(this.profile, 0x23c, input[4] >>> 0);
        put(this.profile, 0x240, 0);
        put(this.profile, 0x244, 0);
        put(this.profile, 0x248, input[3] >>> 0);
        // 초기 방향별 표의 각 칸은 독립 7/0 값이다(대칭으로 새로 만들지 않는다).
        for (let owner = 0; owner < 9; owner += 1) for (let target = 0; target < 9; target += 1) this.allies[owner * 9 + target] = owner === target ? 1 : (owner + target) % 3 === 0 ? 7 : 0;
        this.player = new RawPlayerWorld(this.pool, this.post, this.records, this.playerState, {
            assertion: missing, find: missing, create: missing, setOwner: missing, pop: missing, reserveRegular: missing, addRegular: missing
        });
        this.participation = new RawPlayerParticipation(this.records, this.allies, this.hidden, this.participationState, {
            destructBrain: (token) => this.runtime.destruct(token),
            freeBrain: (token) => this.runtime.freeBody(token)
        });
        put(this.islandBytes, 0, 4);
        // 존재 3개/미존재 1개의 네 기록과 유효 경계 입력만 준비한다.
        for (let index = 0; index < 4; index += 1) {
            const record = 4 + index * 32;
            put(this.islandBytes, record + 4, index < 3 ? 1 : 0);
            put(this.islandBytes, record + 8, 4);
            put(this.islandBytes, record + 16, floatToBits(1));
            put(this.islandBytes, record + 20, floatToBits(1));
            put(this.islandBytes, record + 24, floatToBits(10));
            put(this.islandBytes, record + 28, floatToBits(10));
        }
        this.islands = new RawIslandRuntime(this.pool, this.hash, this.islandBytes, this.modes, this.residences, this.revision, this.ownerChanged, { setResidenceOwner: missing, assertion: missing });
        this.islands.bindPlayerRecords(this.records);
        if (input[6] > 0 && input[6] < 32768) {
            const raw = this.pool.physical(input[6]);
            put(raw, 14, floatToBits(5.75));
            put(raw, 18, floatToBits(6.25));
        }
        if (input[5] === 1) {
            this.types[3].flags2 = 0x40800;
            this.energy[3] = 'ws';
            this.types[4].flags2 = 0x800;
            this.energy[4] = 'Ss';
            this.types[5].flags2 = 0x40000;
            this.energy[5] = 'ss';
        } else if (input[5] === 2) {
            this.types[0].flags2 = 0x40800;
            this.energy[0] = 'ss';
        }
        // 할당 직후의 미초기화 byte 입력이며 기대 초기화 결과가 아니다.
        this.memory = new RawAiBrainMemory(BRAIN_BASE, (bytes) => {
            for (let index = 0; index < bytes.length; index += 1) bytes[index] = (index * 29 + input[8] + this.allocationIndex * 17) & 0xff;
            this.allocationIndex += 1;
        });
        this.runtime = new RawAiBrainConstructor(this.memory, this.player, this.participation, this.islands, this.allies, this.types.slice(0, 48), this.globals(), this.hooks());
    }

    // 현재 BYTE 를 필요할 때 읽는 전역 조회기이다(같은 backing 을 계속 빌린다). (2026-11-09 WB04-E164)
    globals() {
        return { localOwner: () => this.playerState.battleOwner, noRegion: () => this.noRegion, invalidGraph: () => this.invalidGraph };
    }

    // 현재 경계가 실제로 요청한 순서만 기록한다. (2026-11-09 WB04-E164)
    add(value) {
        this.trace += (this.trace ? ';' : '') + value;
    }

    // 원본의 외부 몸체(사제/좌표 지역/요구 에너지)를 명시 입력에 연결한다. (2026-11-09 WB04-E164)
    hooks() {
        return {
            priest: (owner) => { this.add(`P,${owner}`); return this.input[6]; },
            regionAt: (x, y) => { this.add(`R,${floatToBits(x)},${floatToBits(y)}`); return this.input[7]; },
            requiredEnergy: (type) => { this.add(`E,${type}`); return this.energy[type]; }
        };
    }

    // 방향별 전체 DWORD 를 실제 byte 순서로 보존한다. (2026-11-09 WB04-E164)
    alliesHex() {
        const raw = new Uint8Array(324);
        for (let index = 0; index < this.allies.length; index += 1) put(raw, index * 4, this.allies[index]);
        return hex(raw);
    }

    // 소유자의 실제 할당 순서/크기/전체 byte Adler 를 직렬화한다. (2026-11-09 WB04-E164)
    blocks() {
        return this.memory.allocations.map((block) => `${block.token},${block.size},${adler32(this.memory.bytes(block.token))}`).join(';');
    }

    // 해제 순서를 원본 CRT 관찰과 같은 표현으로 쓴다. (2026-11-09 WB04-E164)
    frees() {
        return this.memory.frees.join(',');
    }
}

test('AI 본체 fixture: 실제 PE 160행의 본체/Player9/동맹/섬/할당·역순 해제를 모두 대조한다', () => {
    assert.equal(rows.length, ROWS);
    const unique = new Set();
    // 각 독립 native 입력을 복원 몸체에 전달한다(기대 출력은 실행 뒤에만 읽는다).
    for (const row of rows) {
        assert.equal(row.length, COLUMNS);
        assert.ok(!unique.has(row[0]));
        unique.add(row[0]);
        const scene = new BrainScene(row[0]);
        const token = scene.memory.allocate(AI_BRAIN_BYTES);
        scene.runtime.initialize(token, scene.input[0], scene.profile);
        assert.equal(hex(scene.runtime.bytes(token)), row[1], `본체 ${row[0]}`);
        assert.equal(hex(scene.records), row[2], `Player ${row[0]}`);
        assert.equal(scene.alliesHex(), row[3], `동맹 ${row[0]}`);
        assert.equal(hex(scene.islandBytes), row[4], `섬 ${row[0]}`);
        assert.equal(scene.blocks(), row[5], `할당 ${row[0]}`);
        assert.equal(scene.trace || '-', row[7], `trace ${row[0]}`);
        scene.runtime.destruct(token);
        assert.equal(scene.frees(), row[6], `해제 ${row[0]}`);
        // 본체는 소멸 뒤에도 같은 바이트로 살아 있고 freeBody 가 마지막에 해제한다.
        assert.ok(scene.memory.contains(token));
        assert.equal(hex(scene.runtime.bytes(token)), row[1]);
        scene.runtime.freeBody(token);
        assert.ok(!scene.memory.contains(token));
    }
});

test('AI 본체: 생성 소유자가 참여에 전달되고 초기 갱신 누락은 먼저 중단하며 예외 시 모든 소유 메모리를 정리한다', () => {
    const scene = new BrainScene('1,8,5,0,256,1,70,2,19');
    // 초기 갱신을 공급하지 않은 wrapper 는 실제 생성 전에 거부한다(색 변경 효과가 없어도 먼저 막힌다).
    const wrapper = new RawAiCreate(scene.participation, { construct: (owner, settings, bytes) => scene.runtime.construct(owner, settings, bytes) });
    assert.ok(throws(() => wrapper.create(1, scene.profile)));
    assert.ok(scene.memory.allocations.length === 0 && scene.participation.brain(1) === 0);
    // 색 변경을 공급해도 초기 갱신이 없으면 실제 본체를 할당하기 전에 거부한다.
    const noUpdate = new RawAiCreate(scene.participation, { changeColor: () => {}, construct: (owner, settings, bytes) => scene.runtime.construct(owner, settings, bytes) });
    assert.ok(throws(() => noUpdate.create(1, scene.profile)) && scene.memory.allocations.length === 0 && scene.participation.brain(1) === 0);
    const token = scene.runtime.construct(1, scene.profile);
    assert.equal(scene.memory.allocations.length, 18);
    assert.ok(scene.participation.brain(1) === 0 && (scene.records[180 + 0x50] & 0x20) === 0);
    scene.participation.attachBrain(1, token);
    scene.participation.removeBrain(1);
    assert.ok(!scene.memory.contains(token) && scene.participation.brain(1) === 0 && (scene.records[180 + 0x50] & 0x20) !== 0);
    assert.equal(scene.memory.frees.length, 18);
    // 요구 에너지 조회가 없으면 생성 중간에 실패하고 실제 구성한 17개 목록과 본체를 모두 해제한다.
    const lacking = new BrainScene('1,8,5,1,256,1,70,2,19');
    const lackingHooks = lacking.hooks();
    lackingHooks.requiredEnergy = undefined;
    const incomplete = new RawAiBrainConstructor(lacking.memory, lacking.player, lacking.participation, lacking.islands, lacking.allies, lacking.types.slice(0, 48), lacking.globals(), lackingHooks);
    assert.ok(throws(() => incomplete.construct(1, lacking.profile)));
    assert.ok(lacking.memory.allocations.length === 18 && lacking.memory.frees.length === 18);
    assert.ok(lacking.participation.brain(1) === 0 && (lacking.records[180 + 0x50] & 0x20) === 0);
});

test('AI 본체: 두 원본 쓰기 위치에서 현재 그래프 BYTE 를 각각 다시 읽고 그 사이의 초기화 순서를 지킨다', () => {
    const scene = new BrainScene('1,8,5,0,256,1,70,2,19');
    const globals = scene.globals();
    const token = scene.memory.allocate(AI_BRAIN_BYTES);
    let calls = 0;
    globals.invalidGraph = () => {
        calls += 1;
        const value = scene.invalidGraph;
        if (calls === 1) scene.invalidGraph = 17;
        else {
            assert.equal(get(scene.memory.bytes(token), 0x298), 254);
            assert.equal(get(scene.memory.bytes(token), 0x2a8), 0);
        }
        return value;
    };
    const runtime = new RawAiBrainConstructor(scene.memory, scene.player, scene.participation, scene.islands, scene.allies, scene.types.slice(0, 48), globals, scene.hooks());
    runtime.initialize(token, 1, scene.profile);
    assert.equal(calls, 2);
    assert.ok(get(runtime.bytes(token), 0x298) === 254 && get(runtime.bytes(token), 0x2ac) === 17);
    runtime.destruct(token);
    runtime.freeBody(token);
    assert.ok(!scene.memory.contains(token));
});

test('AI 본체: 현재 그래프 조회기가 없으면 실제 본체를 확보하기 전에 연결 오류를 보고한다', () => {
    const scene = new BrainScene('1,8,5,0,256,1,70,2,19');
    const globals = scene.globals();
    globals.invalidGraph = undefined;
    assert.ok(throws(() => new RawAiBrainConstructor(scene.memory, scene.player, scene.participation, scene.islands, scene.allies, scene.types.slice(0, 48), globals, scene.hooks())));
    assert.equal(scene.memory.allocations.length, 0);
    // 다른 필수 연결(로컬 owner/첫 지역 없음 전역/같은 풀/크기)도 같은 위치에서 거부한다.
    for (const broken of [{ localOwner: undefined }, { noRegion: undefined }]) {
        assert.ok(throws(() => new RawAiBrainConstructor(scene.memory, scene.player, scene.participation, scene.islands, scene.allies, scene.types.slice(0, 48), { ...scene.globals(), ...broken }, scene.hooks())));
    }
    const other = new BrainScene('1,8,5,0,256,1,70,2,19');
    assert.ok(throws(() => new RawAiBrainConstructor(scene.memory, scene.player, scene.participation, other.islands, scene.allies, scene.types.slice(0, 48), scene.globals(), scene.hooks())));
    assert.ok(throws(() => new RawAiBrainConstructor(scene.memory, scene.player, scene.participation, scene.islands, new Uint32Array(80), scene.types.slice(0, 48), scene.globals(), scene.hooks())));
    assert.ok(throws(() => new RawAiBrainConstructor(scene.memory, scene.player, scene.participation, scene.islands, scene.allies, [], scene.globals(), scene.hooks())));
});

test('AI 본체 메모리: 핸들 정렬/해제 계약/주소 공간 소진/미초기화 입력 공급자를 검사한다', () => {
    const memory = new RawAiBrainMemory(0x1000, (bytes) => bytes.fill(0xa5));
    const first = memory.allocate(1);
    const second = memory.allocate(9);
    const third = memory.allocate(16);
    assert.deepEqual([first, second, third], [0x1000, 0x1008, 0x1018]);
    assert.ok(memory.bytes(first).every((value) => value === 0xa5) && memory.bytes(second).length === 9);
    assert.deepEqual(memory.allocations, [{ token: 0x1000, size: 1 }, { token: 0x1008, size: 9 }, { token: 0x1018, size: 16 }]);
    // NULL 해제는 CRT free 처럼 무동작이며 기록되지 않는다.
    memory.free(0);
    assert.deepEqual(memory.frees, []);
    memory.free(second);
    assert.deepEqual(memory.frees, [0x1008]);
    assert.ok(!memory.contains(second) && throws(() => memory.bytes(second)) && throws(() => memory.free(second)) && throws(() => memory.free(0x1234)));
    // 크기 0, 첫 핸들 0, 공급자 없음, 주소 공간 소진은 거부한다.
    assert.ok(throws(() => memory.allocate(0)) && throws(() => new RawAiBrainMemory(0, () => {})) && throws(() => new RawAiBrainMemory(0x1000, undefined)));
    const end = new RawAiBrainMemory(0xfffffff0, () => {});
    assert.ok(throws(() => end.allocate(64)) && end.allocations.length === 0);
    assert.equal(end.allocate(8), 0xfffffff0);
});

test('AI 본체 생성자: 수명 계약(새 본체만 초기화·소멸 전 해제 금지·크기/owner/설정 거부)을 지킨다', () => {
    const scene = new BrainScene('1,8,5,0,256,1,70,2,19');
    const runtime = scene.runtime;
    assert.ok(throws(() => runtime.construct(1, scene.profile, 0xb00)) && scene.memory.allocations.length === 0);
    assert.ok(throws(() => runtime.construct(9, scene.profile)) && scene.memory.allocations.length === 1 && scene.memory.frees.length === 1);
    assert.ok(throws(() => runtime.construct(1, new Uint8Array(599))));
    const token = scene.memory.allocate(AI_BRAIN_BYTES);
    // 초기화 전 소멸/해제는 거부된다.
    assert.ok(throws(() => runtime.destruct(token)) && throws(() => runtime.freeBody(token)));
    runtime.initialize(token, 1, scene.profile);
    // 같은 본체의 중복 초기화와 소멸 전 해제는 거부된다.
    assert.ok(throws(() => runtime.initialize(token, 1, scene.profile)) && throws(() => runtime.freeBody(token)));
    runtime.destruct(token);
    assert.ok(throws(() => runtime.destruct(token)) && throws(() => runtime.initialize(token, 1, scene.profile)));
    runtime.freeBody(token);
    assert.ok(throws(() => runtime.freeBody(token)));
    // 같은 풀을 확인할 수 있다.
    assert.equal(runtime.pool, scene.pool);
});

test('AiCreate: 색 변경 → 기존 brain → NoTemple → 실제 생성/부착/초기 갱신 순서를 지킨다', () => {
    const scene = new BrainScene('1,8,5,0,256,1,70,2,19');
    const events = [];
    const hooks = {
        changeColor: (owner, color) => events.push(`C:${owner}:${color}`),
        firstRegion: (owner) => { events.push(`F:${owner}`); return 3; },
        islandTemple: (region) => { events.push(`T:${region}`); return 77; },
        destroyTemple: (temple, flags) => events.push(`D:${temple}:${flags}`),
        construct: (owner, settings, bytes) => { events.push(`K:${owner}:${bytes}`); return scene.runtime.construct(owner, settings, bytes); },
        initialUpdate: (brain) => events.push(`U:${scene.participation.brain(1) === brain}`)
    };
    const wrapper = new RawAiCreate(scene.participation, hooks);
    const color = get(scene.profile, 0x250);
    assert.ok(color !== 0);
    // NoTemple(+0x244 != 0)이면 색 변경 뒤 첫 지역 신전만 삭제하고 생성하지 않는다.
    const noTemple = scene.profile.slice();
    put(noTemple, 0x244, 1);
    assert.equal(wrapper.create(1, noTemple), 0);
    assert.deepEqual(events, [`C:1:${color}`, 'F:1', 'T:3', 'D:77:4194304']);
    assert.equal(scene.memory.allocations.length, 0);
    // 정상 경로는 생성 → 부착 → 초기 갱신(부착 뒤)이다.
    events.length = 0;
    const brain = wrapper.create(1, scene.profile);
    assert.ok(brain !== 0 && scene.participation.brain(1) === brain);
    assert.deepEqual(events, [`C:1:${color}`, `K:1:${AI_BRAIN_BYTES}`, 'U:true']);
    // 이미 brain 이 있으면 색만 바꾸고 기존 brain 을 돌려주며 다시 생성하거나 갱신하지 않는다.
    events.length = 0;
    assert.equal(wrapper.create(1, scene.profile), brain);
    assert.deepEqual(events, [`C:1:${color}`]);
    // 색이 0 이면 색 변경 효과를 요구하지 않는다.
    const plain = scene.profile.slice();
    put(plain, 0x250, 0);
    const other = new BrainScene('2,8,5,0,256,1,70,2,19');
    const direct = new RawAiCreate(other.participation, { construct: (owner, settings, bytes) => other.runtime.construct(owner, settings, bytes), initialUpdate: () => {} });
    const made = direct.create(2, plain);
    assert.ok(made !== 0 && other.participation.brain(2) === made);
    // 설정 크기, NULL 소유자는 거부된다.
    assert.ok(throws(() => wrapper.create(1, new Uint8Array(599))));
    assert.ok(throws(() => new RawAiCreate(other.participation, { construct: () => 0, initialUpdate: () => {} }).create(3, plain)));
});

test('AiCreate: Native 반납된 owner 의 새 생성은 색 변경이나 생성보다 먼저 거부한다', () => {
    const scene = new BrainScene('1,8,5,0,256,1,70,2,19');
    const events = [];
    const wrapper = new RawAiCreate(scene.participation, {
        changeColor: () => events.push('C'),
        construct: (owner, settings, bytes) => { events.push('K'); return scene.runtime.construct(owner, settings, bytes); },
        initialUpdate: () => {}
    });
    scene.participation.returnBrainForState(scene.participation.removeBrainForState(1));
    assert.ok(throws(() => wrapper.create(1, scene.profile)));
    assert.deepEqual(events, []);
    assert.equal(scene.memory.allocations.length, 0);
});
