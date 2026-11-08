// AI 초기 갱신 raw-ai-initial-update.js(RawAiBrainInitialUpdate)를 실제 PE 관측 133행(+ theme5 16행)과 cpppj 계약 시험으로 검사한다.
//  - fixture ai-initial-update-x86.tsv: 열 = 입력 7개(패턴·mission·이름·기술·부모·빌더·시드), 본체 0xb08 전체, 빌더 40바이트, Player 9기록 1620바이트, 새 Form 의 전체 raw 50바이트(할당 순서, ':' 구분), 이름/타입/부착 요청 순서.
//    fixture ai-initial-update-theme5-x86.tsv: theme5 의 실제 stack 칸 공유와 후속 battery 중립화·원소 목록·전체 본체를 별도 16입력으로 대조한다(열 앞에 첫 TYPE 의 theme 입력).
//    실제 RawPlayerWorld(Technology getter/Create)·RawPlayerTechnology(40개 목록/ensure)·RawContainedCreate(내장 contained 생성자)·RawCarrierPreDestroy(내용물 조회)를 같은 풀로 연결해 입력만 설치하고 모든 출력을 비교한다.
//  - cpppj 계약 시험(선택되지 않은 경계는 요구하지 않고 요구 에너지 누락은 일부 효과 뒤에도 오류, 지갑/SP 불변)과 생성자 거부를 옮겼다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E168: AI 초기 갱신 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { INDEPENDENT_CONTAINED_STATE, RawContainedCreate } from '../www/core/contained-create.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { RawCarrierPreDestroy } from '../www/core/raw-carrier-pre-destroy.js';
import { PlayerWorldState, RawPlayerWorld } from '../www/core/raw-player-world.js';
import { RawPlayerTechnology } from '../www/core/raw-player-technology.js';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { RawAiBrainInitialUpdate } from '../www/core/raw-ai-initial-update.js';
import { MISSION_AI_SETTINGS_SIZE } from '../www/core/raw-mission-ai-settings.js';

// 실제 PE 관측 행 수와 열 수이다(첫 fixture 는 첫 행이 열 이름, 둘째 fixture 는 # 주석 뒤 16행). (2026-11-09 WB04-E168)
const MAIN_ROWS = 133;
const MAIN_COLUMNS = 6;
const THEME_ROWS = 16;
const THEME_COLUMNS = 7;
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const mainRows = (readRows(path.join(FIXTURES, 'ai-initial-update-x86.tsv')) ?? []).filter((row) => row[0] !== 'input');
const themeRows = readRows(path.join(FIXTURES, 'ai-initial-update-theme5-x86.tsv')) ?? [];

// 예외가 나는지 본다. (2026-11-09 WB04-E168)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 16진 문자열로 바꾼다. (2026-11-09 WB04-E168)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 선택되지 않은 삭제/가상 후속은 성공으로 채우지 않는다. (2026-11-09 WB04-E168)
function missing() { throw new Error('AI초기갱신선택범위밖실제하위입니다'); }

// 내용물 type 배열은 기대 분류가 아닌 독립 입력이다. (2026-11-09 WB04-E168)
function nodesOf(pattern) {
    if (pattern === 0) return [];
    if (pattern === 1) return [1, 2, 3, 4, 5, 133];
    if (pattern === 2) return [2];
    if (pattern === 3) return [2, 3];
    if (pattern === 4) return new Array(6).fill(1);
    if (pattern === 5) return [...new Array(16).fill(1), 4];
    if (pattern === 6) return [1, 2, 3, 4];
    return [5, 1];
}

// 실제 getter/기술/Factory/contained/소유 메모리를 한 장면으로 조립한다. (2026-11-09 WB04-E168)
class InitialScene {
    // 원본의 입력만 같은 raw 저장소에 준비한다.
    constructor(text) {
        this.input = text.split(',').map(Number);
        assert.equal(this.input.length, 7);
        const input = this.input;
        this.pool = new SidPool(32768, { server: true });
        // 공통 Post/Factory 는 188칸 표를 받으며 내장 Form 의 생성자 주소(1/2/3/6)만 원문 표에서 명시한다.
        this.types = Array.from({ length: 188 }, (_, index) => ({
            cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, name: `T${index}`, theme: 0, group: 0,
            ...([1, 2, 3, 6].includes(index) ? { constructorAddress: typeConstructorAddress(index) } : {})
        }));
        this.energy = new Array(188).fill('');
        this.book = new SquidPostPopState();
        this.post = new SquidPostPop(this.pool, this.types, this.book);
        this.factory = new SquidFactory(this.pool, this.types);
        this.records = new Uint8Array(1620);
        this.state = new PlayerWorldState();
        this.finderState = new ContainedFinderState();
        this.mission = input[1];
        this.excluded = 133;
        this.workshops = [86, 87, 88, 85];
        this.trace = '';
        this.born = [];
        this.nodeKinds = nodesOf(input[0]);
        this.state.playerKnown = 1;
        this.state.battleOwner = 1;
        put(this.records, 180 + 0x9c, input[4] ? 50 : 0);
        this.records[180 + 12] = 'O'.charCodeAt(0);
        this.records[180 + 13] = 'l'.charCodeAt(0);
        this.records[180 + 14] = 'd'.charCodeAt(0);
        // 부모/내용물만 명시 입력 패턴으로 준비한다.
        for (const sid of [50, ...Array.from({ length: 17 }, (_, index) => 60 + index)]) this.pool.physical(sid).fill(0);
        const parent = input[4] ? 50 : 5;
        if (input[4]) put(this.pool.physical(parent), 6, this.nodeKinds.length === 0 ? 0 : 60, 2);
        // free/dead/비 contained 를 원본과 같은 입력으로 섞는다.
        this.nodeKinds.forEach((kind, index) => {
            const raw = this.pool.physical(60 + index);
            put(raw, 0, 0x12010000);
            raw[10] = input[0] === 6 && index === 1 ? 46 : 6;
            raw[11] = index % 3 === 0 ? 0 : index % 3 === 1 ? 8 : 9;
            put(raw, 4, index + 1 < this.nodeKinds.length ? 61 + index : 0, 2);
            put(raw, 18, kind, 2);
            put(raw, 20, input[0] === 6 && index === 2 ? 0 : 4, 2);
        });
        // 명시 TYPE 표의 이름/비용/원소/flags 를 준비한다.
        const metadata = (type, theme, flags, cost, textValue) => {
            this.types[type].theme = theme;
            this.types[type].flags2 = flags;
            this.types[type].cost = cost;
            this.energy[type] = textValue;
        };
        metadata(1, 0, 0x10000, Math.fround(150.5), 's');
        metadata(2, 0, 0x800, 100, 'ss');
        metadata(3, 1, 0x40000, Math.fround(200.75), 'wz');
        metadata(4, 2, 0x800, Math.fround(800.25), 'rrrr');
        metadata(5, 3, 0, input[0] === 7 ? Number.NaN : -1, '');
        metadata(133, 5, 0x10000, 99, 's');
        this.types[85].cost = Math.fround(200.25);
        this.types[86].cost = Math.fround(150.75);
        this.types[87].cost = Math.fround(500.5);
        this.types[88].cost = 300;
        this.player = new RawPlayerWorld(this.pool, this.post, this.records, this.state, {
            assertion: missing,
            find: () => 0,
            create: (type, flags) => {
                const sid = this.factory.create(type, flags);
                put(this.pool.allocated(sid), 6, this.nodeKinds.length === 0 ? 0 : 60, 2);
                return sid;
            },
            setOwner: (sid, owner) => { this.pool.allocated(sid)[34] = owner; },
            pop: (sid) => {
                const owner = this.pool.slot(sid)[34];
                put(this.records, owner * 180 + 0x9c, sid);
                this.add(`J,${owner}`);
            },
            reserveRegular: missing,
            addRegular: missing
        });
        this.finder = new RawCarrierPreDestroy(this.pool, this.finderState, { validateDamageable: missing, damageablePre: missing, dependentFound: missing });
        this.create = new RawContainedCreate(this.pool, INDEPENDENT_CONTAINED_STATE, {
            create: (type, flags) => {
                this.add(`C,${type},${flags}`);
                assert.equal(type, 6);
                const sid = this.factory.createContained(flags);
                this.born.push(sid);
                return sid;
            },
            notifySurface: missing,
            attach: (sid, target, flags) => {
                this.add(`B,${sid},${target},${flags}`);
                put(this.pool.allocated(sid), 4, get(this.pool.slot(target), 6, 2), 2);
                put(this.pool.physical(target), 6, sid, 2);
            }
        });
        this.technology = new RawPlayerTechnology(this.player, this.finder, this.create, this.types, {
            findType: (name) => {
                this.add(`F,${Buffer.from(name, 'latin1').toString('hex')}`);
                // 실제 이름 조회는 앞 48개 TYPE 이름과 정확히 같은 첫 번호이다.
                for (let index = 0; index < 48; index += 1) if (name === this.types[index].name) return index;
                return -1;
            },
            assertion: missing,
            destroy: missing
        });
        let allocation = 0;
        this.memory = new RawAiBrainMemory(0x13000000, (bytes) => {
            // 첫 할당(본체)만 독립 시드 패턴이고 이후 할당(빌더 목록)은 0 이다.
            for (let index = 0; index < bytes.length; index += 1) bytes[index] = allocation ? 0 : (index * 29 + input[6]) & 0xff;
            allocation += 1;
        });
        this.token = this.memory.allocate(AI_BRAIN_BYTES);
        this.builders = this.memory.allocate(40);
        const raw = this.memory.bytes(this.token);
        for (const [offset, value] of [[0x260, 1], [0x284, this.builders], [0x288, 10], [0x28c, input[5]], [0x3b0, input[5]], [0x394, input[5] ? 85 : 0], [0x390, 3], [0xad8, input[5] ? 7 : 2], [0x2d8, 999], [0x290, 0]]) put(raw, offset, value);
        if (input[5]) {
            put(this.memory.bytes(this.builders), 0, 99);
            put(this.memory.bytes(this.builders), 4, 98);
            put(raw, 0x398, 1);
            put(raw, 0x39c, 3);
        }
        // 같은 600바이트 미초기화 입력에 이름/기술/새 StartMoney 만 쓴다.
        this.profile = new Uint8Array(MISSION_AI_SETTINGS_SIZE);
        for (let index = 0; index < this.profile.length; index += 1) this.profile[index] = (index * 37 + input[6]) & 0xff;
        if (input[2]) {
            this.profile[0] = 'A'.charCodeAt(0);
            this.profile[1] = 'I'.charCodeAt(0);
            this.profile[2] = 0;
        } else this.profile[0] = 0;
        const tech = input[3] ? 'T1;T2' : '';
        Buffer.from(tech, 'latin1').forEach((value, index) => { this.profile[30 + index] = value; });
        this.profile[30 + tech.length] = 0;
        put(this.profile, 0x22c, 777);
        this.runtime = this.makeRuntime(this.hooks());
    }

    // 현재 전역 조회기/TYPE 표/워크샾을 같은 장면에서 빌려 초기 갱신기를 만든다. (2026-11-09 WB04-E168)
    makeRuntime(hooks, memory = this.memory) {
        return new RawAiBrainInitialUpdate(memory, this.player, this.technology, this.finderState, this.types, {
            missionMode: () => this.mission, excludedType: () => this.excluded, workshops: this.workshops
        }, hooks);
    }

    // 실제 호출만 순서대로 기록한다. (2026-11-09 WB04-E168)
    add(value) { this.trace += (this.trace ? ';' : '') + value; }

    // 원본 외부 언어/TYPE 문자열 요청만 기록한다. (2026-11-09 WB04-E168)
    hooks() {
        return {
            defaultName: () => { this.add('D'); return 'Default AI'; },
            requiredEnergy: (type) => { this.add(`E,${type}`); return this.energy[type]; }
        };
    }

    // 실제 새 Form 의 전체 raw 50바이트를 원본 할당 순서로 읽는다. (2026-11-09 WB04-E168)
    bornHex() { return this.born.map((sid) => hex(this.pool.slot(sid))).join(':') || '-'; }
}

// 한 장면의 결과를 fixture 열(본체/빌더/Player/새 Form/순서)과 비교한다. (2026-11-09 WB04-E168)
function expectRow(scene, columns, label) {
    assert.equal(hex(scene.memory.bytes(scene.token)), columns.body, `${label} 본체`);
    assert.equal(hex(scene.memory.bytes(scene.builders)), columns.builders, `${label} 빌더`);
    assert.equal(hex(scene.records), columns.players, `${label} Player`);
    assert.equal(scene.bornHex(), columns.born, `${label} 새 Form`);
    assert.equal(scene.trace || '-', columns.trace, `${label} 순서`);
}

test('AI 초기 갱신 fixture: 실제 PE 133행의 본체/빌더/Player9/새 Form/모든 조회 효과를 비교한다', () => {
    assert.equal(mainRows.length, MAIN_ROWS);
    const unique = new Set();
    // 원본 기대 출력은 실행 이후에만 읽는다.
    for (const row of mainRows) {
        assert.equal(row.length, MAIN_COLUMNS);
        assert.ok(!unique.has(row[0]));
        unique.add(row[0]);
        const scene = new InitialScene(row[0]);
        scene.runtime.update(scene.token, scene.profile);
        expectRow(scene, { body: row[1], builders: row[2], players: row[3], born: row[4], trace: row[5] }, row[0]);
        assert.equal(get(scene.memory.bytes(scene.token), 0x2d8), 999);
    }
});

test('AI 초기 갱신 fixture: theme5 의 실제 stack 칸 공유와 후속 battery 중립화·원소 목록·전체 본체를 별도 16입력으로 대조한다', () => {
    assert.equal(themeRows.length, THEME_ROWS);
    // 기존 133 입력 fixture 는 보존하고 추가 원문만 읽는다.
    for (const row of themeRows) {
        assert.equal(row.length, THEME_COLUMNS);
        const scene = new InitialScene(row[1]);
        scene.types[1].theme = Number(row[0]) | 0;
        scene.runtime.update(scene.token, scene.profile);
        expectRow(scene, { body: row[2], builders: row[3], players: row[4], born: row[5], trace: row[6] }, `theme ${row[0]} ${row[1]}`);
    }
});

test('AI 초기 갱신: 선택되지 않은 경계는 요구하지 않고 실제 에너지 누락은 일부 효과 뒤에도 오류로 남으며 SP 지갑을 재지정하지 않는다', () => {
    const empty = new InitialScene('0,0,0,0,1,0,19');
    empty.makeRuntime({}).update(empty.token, empty.profile);
    assert.ok(get(empty.memory.bytes(empty.token), 0x2d8) === 999 && get(empty.memory.bytes(empty.token), 0x22c) === 777);
    const lacking = new InitialScene('1,1,1,0,1,0,19');
    const hooks = lacking.hooks();
    hooks.requiredEnergy = undefined;
    assert.ok(throws(() => lacking.makeRuntime(hooks).update(lacking.token, lacking.profile)));
    assert.ok(get(lacking.memory.bytes(lacking.token), 0x28c) === 1 && get(lacking.memory.bytes(lacking.token), 0x2d8) === 999);
});

test('AI 초기 갱신: 기본 이름 언어 조회 누락·이름 owner 범위·본체/설정 크기·생성자 연결을 도달 위치에서 거부한다', () => {
    const scene = new InitialScene('0,1,0,0,1,0,19');
    // 이름이 없고 mission 표식이 켜져 있으면 기본 언어 이름 조회가 필수이다(복사된 설정은 이미 본체에 남는다).
    assert.ok(throws(() => scene.makeRuntime({}).update(scene.token, scene.profile)));
    assert.equal(hex(scene.memory.bytes(scene.token).subarray(0, 600)), hex(scene.profile));
    const other = new InitialScene('0,0,0,0,1,0,19');
    assert.ok(throws(() => other.runtime.update(other.token, new Uint8Array(599))));
    put(other.memory.bytes(other.token), 0x260, 9);
    assert.ok(throws(() => other.runtime.update(other.token, other.profile)));
    const small = other.memory.allocate(16);
    assert.ok(throws(() => other.runtime.update(small, other.profile)));
    // 같은 풀이 아니거나 워크샾 4개/조회기가 없으면 생성 때 거부한다.
    const foreign = new InitialScene('0,0,0,0,1,0,19');
    assert.ok(throws(() => new RawAiBrainInitialUpdate(other.memory, other.player, foreign.technology, other.finderState, other.types, { missionMode: () => 0, excludedType: () => 133, workshops: other.workshops }, {})));
    assert.ok(throws(() => new RawAiBrainInitialUpdate(other.memory, other.player, other.technology, other.finderState, other.types, { missionMode: () => 0, excludedType: () => 133, workshops: [1, 2, 3] }, {})));
    assert.ok(throws(() => new RawAiBrainInitialUpdate(other.memory, other.player, other.technology, other.finderState, other.types, { excludedType: () => 133, workshops: other.workshops }, {})));
    assert.ok(throws(() => new RawAiBrainInitialUpdate(other.memory, other.player, other.technology, other.finderState, [], { missionMode: () => 0, excludedType: () => 133, workshops: other.workshops }, {})));
    // NaN 비용(TYPE 5)은 최대 비용을 0 으로 쓰는 원본 ftol 규칙이므로 fixture 입력 7 과 같은 장면이 오류 없이 끝난다.
    const nan = new InitialScene('7,0,0,0,1,0,19');
    nan.runtime.update(nan.token, nan.profile);
    assert.equal(get(nan.memory.bytes(nan.token), 0x2d8), 999);
});
