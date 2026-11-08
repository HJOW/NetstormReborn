// Player 기술 목록 raw-player-technology.js(RawPlayerTechnology: 조회·전체 삭제·보장 생성·40개 목록 사슬)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: player-technology-x86.tsv 158행(find 56·ensure 56·clear 16·add 18·parse 9·stalled 3, 10.78 PE 전용 관측이라 판본 열이 없다).
//    cpppj PlayerTechnology_Native158 과 같은 입력으로 실제 RawPlayerWorld(Technology getter)·RawCarrierPreDestroy(내용물 조회)·RawContainedCreate(SquidFactory 내장 contained 생성자)를
//    연결하고 반환·외부 경계 순서·목록·관찰 다섯 슬롯·생성 슬롯·기술 부모 DWORD 를 비교한다.
//  - 계약: PlayerTechnology_ActualTypeBindingsAndStalledBoundary(실제 TYPE 이름 조회 연결과 무진행 입력의 생성 전 거부).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E56: Player 기술 목록 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get } from './raw-support.mjs';
import { realPlayerAssets, hex } from './player-scene-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { INDEPENDENT_CONTAINED_STATE, RawContainedCreate } from '../www/core/contained-create.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { RawCarrierPreDestroy } from '../www/core/raw-carrier-pre-destroy.js';
import { PLAYER_RECORD_STRIDE, PLAYER_TECHNOLOGY_OFFSET, PLAYER_WORLD_ASSERTION, PlayerWorldState, RawPlayerWorld } from '../www/core/raw-player-world.js';
import { PLAYER_TECHNOLOGY_ASSERTION, PLAYER_TECHNOLOGY_PARSE_STEP, PlayerTechnologyList, RawPlayerTechnology, bindPlayerTechnologyTypes } from '../www/core/raw-player-technology.js';

// 원본 관측 행이다(첫 행은 열 이름). 파일이 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E56)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const technologyRows = readRows(path.join(FIXTURES, 'player-technology-x86.tsv'))?.filter((row) => row[0] !== 'kind') ?? null;

// 원본 격리 입력과 같은 가상 삭제/부착 경계 표식 VT 이다. (2026-11-08 WB04-E56)
const TECHNOLOGY_VTABLE = 0x12010000;

// 선택되지 않은 Player 생성/삭제 부모 경계에 빈 성공을 공급하지 않는다. (2026-11-08 WB04-E56)
const missing = () => { throw new Error('기술 검사에서 선택되지 않은 실제 하위 경계입니다'); };

// 기술 부모 DWORD 의 9기록 위치이다. (2026-11-08 WB04-E56)
const technologyAt = (owner) => owner * PLAYER_RECORD_STRIDE + PLAYER_TECHNOLOGY_OFFSET;

// 공통 Post/Factory 는 188칸 표를 받고 기술 조회 선택 입력은 앞 48개 이름/group 을 쓴다(cpppj TechnologyTypes). (2026-11-08 WB04-E56)
function technologyTypes() {
    return Array.from({ length: 188 }, (_, index) => ({
        cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, name: `T${index}`, group: index % 7 === 0 ? 10 : index % 10,
        ...([1, 2, 3, 6].includes(index) ? { constructorAddress: typeConstructorAddress(index) } : {}),
    }));
}

// TSV 의 목록 문자열 16진을 원래 바이트 문자열로 되돌린다. (2026-11-08 WB04-E56)
const textFromHex = (text) => Buffer.from(text, 'hex').toString('latin1');

// cpppj TechnologyScene: 같은 풀·기술 부모·contained 조회/생성·기술 몸체를 만들고 fixture 입력만으로 raw 체인을 준비한다. (2026-11-08 WB04-E56)
function technologyScene(input, typesOverride = null, hooksOverride = null) {
    const pool = new SidPool(32768, { server: true });
    const types = technologyTypes();
    const book = new SquidPostPopState();
    const post = new SquidPostPop(pool, types, book);
    const records = new Uint8Array(9 * PLAYER_RECORD_STRIDE);
    const state = new PlayerWorldState();
    const factory = new SquidFactory(pool, types);
    const fields = input.split('|');
    const numbers = fields[0].split(',').map((item) => Number(item));
    const owner = numbers[0] >>> 0;
    state.playerKnown = numbers[2] >>> 0;
    const pattern = numbers[3] >>> 0;
    const mutate = numbers[4] >>> 0;
    put(records, technologyAt(owner), 50);
    const scene = { pool, types, records, owner, trace: [], born: [] };
    const add = (value) => scene.trace.push(value);
    // 부모와 종속 네 개의 주변 바이트를 원본과 같게 초기화한다.
    for (const sid of [50, 51, 60, 61, 62, 63]) pool.physical(sid).fill(0);
    put(pool.physical(50), 0, TECHNOLOGY_VTABLE);
    pool.physical(50)[10] = 152;
    if (pattern) {
        put(pool.physical(50), 6, 60, 2);
        const kinds = [1, 2, 2, 7];
        const flags = [4, 4, 1, 4];
        // 비 contained 타입/flags 1/flags 4 를 같은 raw 체인에 섞는다.
        for (let index = 0; index < 4; index += 1) {
            const raw = pool.physical(60 + index);
            put(raw, 0, TECHNOLOGY_VTABLE);
            raw[10] = index === 1 ? 46 : 6;
            put(raw, 4, index < 3 ? 61 + index : 0, 2);
            put(raw, 18, kinds[index], 2);
            put(raw, 20, flags[index], 2);
        }
    }
    const player = new RawPlayerWorld(pool, post, records, state, {
        assertion: (value) => { assert.equal(value, PLAYER_WORLD_ASSERTION.playerIdKnown); add('A,669'); },
        find: missing, create: missing, setOwner: missing, pop: missing, reserveRegular: missing, addRegular: missing,
    });
    const finder = new RawCarrierPreDestroy(pool, new ContainedFinderState(), { validateDamageable: missing, damageablePre: missing, dependentFound: missing });
    const create = new RawContainedCreate(pool, INDEPENDENT_CONTAINED_STATE, {
        create: (type, flags) => {
            add(`C,${type},${flags}`);
            assert.equal(type, 6);
            const sid = factory.createContained(flags);
            scene.born.push(sid);
            return sid;
        },
        notifySurface: missing,
        attach: (sid, parent, flags) => {
            add(`B,${sid},${parent},${flags}`);
            const head = get(pool.slot(parent), 6, 2);
            put(pool.allocated(sid), 4, head, 2);
            put(pool.physical(parent), 6, sid, 2);
        },
    });
    const hooks = hooksOverride ?? {
        findType: (name) => {
            // 실제 이름 조회 요청 바이트를 기록한다.
            add(`F,${Buffer.from(name, 'latin1').toString('hex')}`);
            const wanted = name.replace(/[A-Z]/g, (letter) => letter.toLowerCase());
            // 명시 fixture 이름 입력의 첫 일치 조회이며 제품의 기대 목록을 계산하지 않는다.
            for (let index = 0; index < types.length; index += 1) if (types[index].name.toLowerCase() === wanted) return index;
            return -1;
        },
        assertion: (value) => add(`A,${value === PLAYER_TECHNOLOGY_ASSERTION.findTypeRange ? 867 : 886}`),
        destroy: (sid, flags) => {
            add(`D,${sid},${flags}`);
            let previous = 50;
            let field = 6;
            let current = get(pool.slot(previous), field, 2);
            // 외부 삭제 응답은 같은 현재 raw 체인에서 해당 SID 만 제거한다.
            while (current && current !== sid) {
                previous = current;
                field = 4;
                current = get(pool.slot(previous), field, 2);
            }
            assert.equal(current, sid);
            put(pool.physical(previous), field, get(pool.slot(sid), 4, 2), 2);
            if (mutate) put(records, technologyAt(owner), 51);
        },
    };
    scene.technology = new RawPlayerTechnology(player, finder, create, typesOverride ?? types.slice(0, 48), hooks);
    scene.player = player;
    scene.finder = finder;
    scene.create = create;
    // 같은 관찰 슬롯 전체를 메모리 순서대로 직렬화한다.
    scene.raw = () => [50, 60, 61, 62, 63].map((sid) => hex(pool.slot(sid))).join(':');
    // 실제 생성된 모든 슬롯의 전체 50 바이트이다.
    scene.bornText = () => (scene.born.length ? scene.born.map((sid) => hex(pool.slot(sid))).join(':') : '-');
    return scene;
}

test('PlayerTechnology: 원본 158 입력의 반환·두 반복 무진행·전체 raw/생성 슬롯·모든 효과가 같다', { skip: !technologyRows && '원본 fixture 없음' }, () => {
    let count = 0;
    const unique = new Set();
    // fixture 출력은 실행 뒤에만 읽어 기대 모델로 쓰지 않는다.
    for (const row of technologyRows) {
        assert.equal(row.length, 8);
        const key = `${row[0]}\t${row[1]}`;
        assert.ok(!unique.has(key));
        unique.add(key);
        const scene = technologyScene(row[1]);
        const fields = row[1].split('|');
        const type = Number(fields[0].split(',')[1]) | 0;
        const text = textFromHex(fields[1]);
        const owner = scene.owner | 0;
        let result = '-';
        const values = [];
        if (row[0] === 'find') result = String(scene.technology.find(owner, type));
        else if (row[0] === 'clear') scene.technology.clear(owner);
        else if (row[0] === 'ensure') scene.technology.ensure(owner, type);
        else if (row[0] === 'add') scene.technology.addList(owner, text);
        else if (row[0] === 'parse' || row[0] === 'stalled') {
            const list = new PlayerTechnologyList();
            // 원본 정상 종료 또는 같은 미등록 항목의 실제 두 반복까지만 진행한다.
            for (;;) {
                const step = scene.technology.parseStep(text, list);
                if (step === PLAYER_TECHNOLOGY_PARSE_STEP.complete) break;
                if (step === PLAYER_TECHNOLOGY_PARSE_STEP.stalled) {
                    assert.equal(row[0], 'stalled');
                    assert.equal(scene.technology.parseStep(text, list), PLAYER_TECHNOLOGY_PARSE_STEP.stalled);
                    result = String(list.index);
                    break;
                }
            }
            // 만든 목록을 순서대로 기록한다.
            for (let index = 0; index < list.count; index += 1) values.push(list.types[index]);
        } else assert.fail(`알 수 없는 기술 연산 ${row[0]}`);
        const output = [result, scene.trace.length ? scene.trace.join(';') : '-', values.length ? values.join(',') : '-', scene.raw(), scene.bornText(), String(get(scene.records, technologyAt(scene.owner)))];
        assert.deepEqual(output, row.slice(2), `${row[0]} 행 ${count} 입력 ${row[1]}`);
        count += 1;
    }
    assert.equal(count, 158);
});

test('PlayerTechnology: 실제 보호 TYPE 이름을 이름 조회에 직접 연결하고 무진행 입력은 기술 생성 전에 보고한다', () => {
    const { types } = realPlayerAssets();
    const hooks = bindPlayerTechnologyTypes(types, { destroy: missing, assertion: missing });
    const scene = technologyScene('1,0,1,0,0|', types, hooks);
    const technology = scene.technology;
    const list = new PlayerTechnologyList();
    const text = 'sunWalker;windBattery;sunCannon';
    assert.equal(technology.parseStep(text, list), PLAYER_TECHNOLOGY_PARSE_STEP.progress);
    assert.ok(list.count === 1 && list.types[0] === hooks.findType('sunWalker') && list.types[0] > 0);
    assert.equal(technology.parseStep(text, list), PLAYER_TECHNOLOGY_PARSE_STEP.progress);
    assert.equal(technology.parseStep(text, list), PLAYER_TECHNOLOGY_PARSE_STEP.progress);
    assert.equal(technology.parseStep(text, list), PLAYER_TECHNOLOGY_PARSE_STEP.complete);
    assert.equal(list.count, 3);
    assert.throws(() => technology.addList(1, 'sunWalker;missing;sunCannon'), /무한 반복/);
    assert.deepEqual(scene.born, []);
});

test('PlayerTechnology: 같은 풀·이름 조회·TYPE 표 계약과 40개 용량 상한(웹 보강)', () => {
    const scene = technologyScene('1,0,1,0,0|');
    const other = technologyScene('1,0,1,0,0|');
    assert.throws(() => new RawPlayerTechnology(scene.player, other.finder, scene.create, scene.types, { findType: () => -1 }), TypeError);
    assert.throws(() => new RawPlayerTechnology(scene.player, scene.finder, scene.create, scene.types, {}), TypeError);
    assert.throws(() => new RawPlayerTechnology(scene.player, scene.finder, scene.create, [], { findType: () => -1 }), RangeError);
    // all 은 group 10 을 뺀 전체를 넣되 40개에서 멈춘다.
    const list = new PlayerTechnologyList();
    const technology = new RawPlayerTechnology(scene.player, scene.finder, scene.create, scene.types, { findType: () => -1 });
    assert.equal(technology.parseStep('ALL', list), PLAYER_TECHNOLOGY_PARSE_STEP.progress);
    assert.equal(list.count, 40);
    assert.ok(Array.from(list.types).every((type) => type % 7 !== 0));
    assert.equal(technology.parseStep('ALL', list), PLAYER_TECHNOLOGY_PARSE_STEP.complete);
});
