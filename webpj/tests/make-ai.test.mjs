// 일반 MakeAi(기본 설정 600바이트·SP 절삭·owner 0 이전/삭제 꼬리)와 전체 객체 이전·삭제 순회를 원본 10.78 관측으로 대조한다(WB04-E32, cpppj MakeAiTests 이식).
//  - make-ai-parent-x86.tsv 770행: 원문 600바이트 전체와 초기화/현재 조회/두 인자 생성 순서. make-ai-objects-x86.tsv 2592행: 현재 슬롯 50바이트·Player 첫 region·깊이·두 카운터·삭제 mask.
//  - 계약: 전투가 아니면 설정 비접근, 초기화 중단 뒤 앞선 쓰기 보존, 삭제가 슬롯을 바꿔도 선행 선택한 다음 후보 유지, 깊이 증가 뒤 예외는 감소를 합성하지 않음. AI 설정(RawMissionAiSettings)의 Apply/SetAbility 규칙도 직접 확인한다.
// 실제 미션 시작 override 흐름(MissionStart)·AI 생성(417e50)·Player/월드 카운터의 연결은 해당 단위를 옮길 때 대조한다. (2026-11-08 WB04-E32: MakeAi·AI 설정 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { bitsToFloat, readRows } from './raw-support.mjs';
import { RawMissionAiSettings, configListItem } from '../www/core/raw-mission-ai-settings.js';
import { clearMakeAiObjects, makeAi, transferMakeAiObjects } from '../www/core/raw-make-ai.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E32)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const parentRows = fixture('make-ai-parent-x86.tsv');
const objectRows = fixture('make-ai-objects-x86.tsv');

// 원문 DWORD 읽기(감김)와 signed 변환, 바이트 16진, 비정렬 DWORD 쓰기/읽기이다. (2026-11-08 WB04-E32)
const number = (text) => Number(BigInt(text) & 0xffffffffn);
const signed = (value) => value | 0;
const hex = (bytes) => Buffer.from(bytes).toString('hex');
const word = (bytes, offset, value) => { for (let index = 0; index < 4; index += 1) bytes[offset + index] = (value >>> (8 * index)) & 0xff; };
const wordAt = (bytes, offset) => (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
const floatBits = (value) => { const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true); return view.getUint32(0, true); };

// 제품 계산에서 기대 설정을 만들지 않는다.
test('MakeAi: 원본10.78 770행(600바이트 기본 설정·SP 절삭·owner 0 봉투)', { skip: !parentRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of parentRows) {
        assert.equal(row.length, 13);
        const bytes = new Uint8Array(600).fill(205);
        let post = number(row[2]);
        let created = '';
        const trace = [];
        const state = { stuffRefreshRate: signed(number(row[8])), startingSp: signed(number(row[9])), checkingAsserts: false };
        // 일반 MakeAi 는 미션 override 를 쓰지 않는다.
        const settings = new RawMissionAiSettings(state, { get: () => { throw new Error('일반 MakeAi 의 미션 override 금지'); } });
        const reads = {
            battle: () => number(row[1]),
            missionPost: () => post,
            playerKnown: () => number(row[3]),
            localOwner: () => number(row[4]),
            brain: (owner) => { assert.equal(owner, number(row[4]) + 1); return number(row[6]); },
        };
        const hooks = {
            initialize: (raw) => { trace.push('Init'); settings.initialize(raw); if (row[7] !== '-1') post = number(row[7]); },
            stormPower: (owner) => { trace.push(`Sp:${owner}`); return bitsToFloat(number(row[5])); },
            create: (owner, raw) => { trace.push(`Create:${owner}`); created = hex(raw); return 0x12345678; },
            transferObjects: (owner) => trace.push(`Transfer:${owner}`),
            clearObjects: (mask) => trace.push(`Clear:${mask}`),
        };
        makeAi(number(row[0]), bytes, reads, hooks);
        const label = `행 ${count + 1}`;
        assert.equal(hex(bytes), row[10], `${label} 설정 600바이트`);
        assert.equal(trace.length === 0 ? '-' : trace.join(';'), row[11], `${label} 호출 순서`);
        assert.equal(created === '' ? '-' : created, row[12], `${label} 생성 인자`);
        count += 1;
    }
    assert.equal(count, 770);
});

// 원문 입력의 각 현재 물리는 별개로 소유한다.
test('MakeAi: 원본10.78 2592행(현재 슬롯·Player 첫 region·깊이·두 카운터·삭제 mask)', { skip: !objectRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of objectRows) {
        assert.equal(row.length, 14);
        const local = number(row[1]);
        const kind = number(row[6]);
        const slots = Array.from({ length: 6 }, () => new Uint8Array(50));
        const players = new Uint8Array(1620);
        const localCounts = new Uint32Array(256);
        const builtCounts = new Uint32Array(256);
        const flags = new Array(8).fill(0);
        const depth = { value: 7 };
        const trace = [];
        // Native 에 명시한 같은 pointer 식별값/좌표/free 입력이다.
        slots.forEach((slot, index) => { word(slot, 0, 0x10015000); slot[11] = 1; word(slot, 14, floatBits(index)); word(slot, 18, floatBits(index)); });
        slots[2][10] = kind;
        slots[2][11] = number(row[3]) | number(row[7]);
        slots[2][34] = number(row[2]);
        flags[kind] = number(row[4]);
        word(players, local * 180 + 0x78, 41);
        word(players, (local + 1) * 180 + 0x78, 99);
        const at = (list, index) => { if (index >= list.length) throw new RangeError('범위 밖'); return list[index]; };
        const hooks = {
            capacity: () => 6,
            localOwner: () => local,
            playerType: () => 4,
            bridgeType: () => 5,
            slot: (index) => at(slots, index),
            players: () => players,
            loadingDepth: depth,
            flags: (index) => { const value = at(flags, slots[index][10]); trace.push(`Flags:${index}:${value}`); return value; },
            region: (x, y) => { assert.ok(x === y && x === 2); const region = number(row[5]) ? 41 : 42; trace.push(`Region:2:${region}`); return region; },
            setOwner: (index, owner) => { trace.push(`Owner:${index}:${owner}:${depth.value}`); slots[index][34] = owner & 0xff; },
            destroy: (index, value) => trace.push(`Destroy:${index}:${value}`),
            decrementLocal: (type) => { trace.push(`Local:${type}`); localCounts[type] -= 1; },
            decrementBuilt: (type) => { trace.push(`Built:${type}`); builtCounts[type] -= 1; },
        };
        if (row[0] === 'Transfer') transferMakeAiObjects(local + 1, hooks);
        else clearMakeAiObjects(row[0] === 'MakeClear' ? 0x2000 : number(row[4]), hooks);
        const label = `행 ${count + 1} ${row[0]}`;
        assert.equal(hex(slots[2]), row[8], `${label} 슬롯 2`);
        assert.equal(wordAt(players, (local + 1) * 180 + 0x78), number(row[9]), `${label} 새 소유자 region`);
        assert.deepEqual([depth.value, localCounts[kind], builtCounts[kind]], [number(row[10]), number(row[11]), number(row[12])], `${label} 깊이/카운터`);
        assert.equal(trace.length === 0 ? '-' : trace.join(';'), row[13], `${label} 호출 순서`);
        count += 1;
    }
    assert.equal(count, 2592);
});

// 실제 effect 실패를 성공으로 바꾸지 않고 원문 쓰기/iterator 의 선행 선택을 유지한다.
test('MakeAi: 부분 실패와 삭제 전 다음 후보 선택', () => {
    const reads = { battle: () => 0 };
    makeAi(1, new Uint8Array(0), reads, {});
    reads.battle = () => 1;
    const settings = new Uint8Array(600);
    assert.throws(() => makeAi(1, settings, reads, { initialize: (raw) => { raw[30] = 77; throw new Error('명시 초기화 실패'); } }), /명시 초기화 실패/);
    assert.equal(settings[30], 77);
    assert.throws(() => makeAi(1, new Uint8Array(599), reads, {}), TypeError);
    // 삭제 순회: 다음 후보는 flags 조회/가상 삭제보다 먼저 정하므로 선택 뒤 free 가 돼도 새 free 관문을 더하지 않는다.
    const slots = Array.from({ length: 5 }, () => new Uint8Array(50));
    slots[0][11] = 1;
    slots[1][11] = 1;
    const depth = { value: 0 };
    const destroyed = [];
    const objects = {
        capacity: () => 5,
        slot: (index) => slots[index],
        flags: (index) => { if (index === 2) slots[3][11] = 1; return 0x2000; },
        destroy: (index, value) => { assert.equal(value, 0); destroyed.push(index); },
        loadingDepth: depth,
    };
    clearMakeAiObjects(0x2000, objects);
    assert.deepEqual(destroyed, [2, 3, 4]);
    // 이전: 두 번째 setOwner 에서 중단하면 증가한 깊이는 감소를 합성하지 않는다.
    const players = new Uint8Array(1620);
    let owners = 0;
    Object.assign(objects, {
        capacity: () => 3,
        localOwner: () => 1,
        playerType: () => 4,
        bridgeType: () => 5,
        players: () => players,
        region: () => 1,
        flags: () => 0x200000,
        setOwner: () => { owners += 1; if (owners === 2) throw new Error('명시 두 번째 Owner 실패'); },
    });
    slots[2][34] = 0;
    assert.throws(() => transferMakeAiObjects(2, objects), /두 번째 Owner/);
    assert.deepEqual([owners, depth.value], [2, 1]);
});

// 호스트 측 설정 조회가 만든 현재 키로 공통(-1)→owner 순서로 누적한다. 원문 관측이 없는 Apply/SetAbility 의 규칙은 소스 계약으로 확인한다.
test('AI 설정: Apply/SetAbility/Prepare 규칙(키 접두·빈 값 유지·가산 동맹·느낌표 능력)', () => {
    const values = new Map();
    const asked = [];
    const unknown = [];
    const state = { stuffRefreshRate: 3, startingSp: 2500, checkingAsserts: true };
    const hooks = {
        get: (key) => { asked.push(key); return values.get(key) ?? ''; },
        parseFloat: (text) => Number(text),
        unknownAbilityAssertion: (name) => unknown.push(`assert:${name}`),
        unknownAbilityLog: (name) => unknown.push(`log:${name}`),
    };
    const settings = new RawMissionAiSettings(state, hooks);
    const raw = new Uint8Array(600);
    settings.initialize(raw);
    assert.deepEqual([wordAt(raw, 0x22c), wordAt(raw, 0x230), wordAt(raw, 0x238), wordAt(raw, 0x248)], [6500, 3, 4, 0xffff]);
    assert.equal(Buffer.from(raw.subarray(0x1e, 0x1e + 80)).toString('latin1').split('\0')[0], 'sunWalker;windBattery;sunCannon;sunBalloon;sunBlocker;sunArcher;windArcher');
    // 공통(-1)은 'ai' 접두, owner>0 은 'ai<owner>' 접두이며 공통의 동맹/능력이 owner 값에 누적된다.
    values.set('aiName', 'Common');
    values.set('aiAllyList', '2;3');
    values.set('ai3Name', 'Third');
    values.set('ai3Color', 'RED');
    values.set('ai3Ability', '!BUILD_DAIS;collect_storm_power;MADE_UP');
    values.set('ai3StartMoney', '777');
    values.set('ai3TimeBetweenMoves', '1.5');
    settings.prepare(raw, 3);
    assert.equal(Buffer.from(raw.subarray(0, 8)).toString('latin1').split('\0')[0], 'Third');
    assert.equal(wordAt(raw, 0x23c), (1 << 2) | (1 << 3));
    assert.equal(wordAt(raw, 0x250), 2);
    assert.equal(wordAt(raw, 0x22c), 777);
    assert.equal(wordAt(raw, 0x228), floatBits(1.5));
    // BUILD_DAIS(9) 끔, COLLECT_STORM_POWER(4) 켬, 미등록 이름은 진단 뒤 index 13 비트를 켠다.
    const abilities = wordAt(raw, 0x248);
    assert.deepEqual([(abilities >> 9) & 1, (abilities >> 4) & 1, (abilities >> 13) & 1], [0, 1, 1]);
    assert.deepEqual(unknown, ['assert:MADE_UP', 'log:MADE_UP']);
    assert.ok(asked.includes('aiAllyList') && asked.includes('ai3Color'));
    // 목록 조각 읽기와 계약 거부.
    assert.deepEqual([configListItem('a;b;c', 0), configListItem('a;b;c', 2), configListItem('a;b;c', 3), configListItem('a;b;c', -1)], ['a', 'c', '', '']);
    assert.throws(() => new RawMissionAiSettings(state, {}), TypeError);
    assert.throws(() => settings.initialize(new Uint8Array(599)), TypeError);
    // 값이 비어 있으면 이전 필드를 유지한다.
    const before = Uint8Array.from(raw);
    values.clear();
    settings.apply(raw, 3);
    assert.equal(hex(raw), hex(before));
});
