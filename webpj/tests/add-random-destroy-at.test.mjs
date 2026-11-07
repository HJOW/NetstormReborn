// 대본 AddRandom 부모(고정 범위·시각 seed·현재 소유자 섬 검색)와 DestroyAt(위치 해석·점 검색·삭제 루프)를 원본 10.78 관측으로 대조한다(WB04-E30, cpppj AddRandomCommandTests(부모)·DestroyAtTests 이식).
//  - add-random-parent-x86.tsv 720행: 실제 seed/기본 범위/섬 검색의 전체 인자. destroy-at-parse-x86.tsv 16행: 토큰 구분자·따옴표·길이 제한·NUL·atol과 다섯 출력. destroy-at-parent-x86.tsv 804행: 권한/점 정수화/필터 루프와 장치 효과 뒤 늦은 읽기·전체 raw.
//  - 계약: 선택되지 않은 하위와 이미 실행된 벽시각 효과, 권한 진단 복귀, 도달한 장치만 요구.
// 대본 큐(QueueScriptCommand)·일반 입력 분배(E21 이식 완료)와 실제 월드 삭제/배치 몸체 연결은 해당 단위에서 대조한다. (2026-11-08 WB04-E30: AddRandom·DestroyAt 부모 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { floatToBits, readRows } from './raw-support.mjs';
import { addRandomObjects, findAnyFortTerritoryIsland, findFortTerritoryIsland, timeSeed } from '../www/core/raw-add-random.js';
import { destroyObjectsAt, parseDestroyAtLocation } from '../www/core/raw-destroy-at.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E30)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const addRows = fixture('add-random-parent-x86.tsv');
const parseRows = fixture('destroy-at-parse-x86.tsv');
const parentRows = fixture('destroy-at-parent-x86.tsv');

// 원문 DWORD 읽기(감김), 부호 있는 십진 읽기, 원본 바이트 16진 복원이다. '-' 는 빈 문자열이다. (2026-11-08 WB04-E30)
const number = (text) => Number(BigInt(text) & 0xffffffffn);
const signedInt = (text) => Number(BigInt.asIntN(32, BigInt(text)));
const hexToText = (hex) => (hex === '-' ? '' : Buffer.from(hex, 'hex').toString('latin1'));
const textToHex = (text) => Buffer.from(text, 'latin1').toString('hex');
const bits = (value) => value >>> 0;

// 원문 입력의 세 섬 기록을 정확한 바이트/DWORD 위치에 설치한다(개수 3, 항목 32바이트: +4 존재, +8 현재 owner, +12 지역, +13 원래 owner). (2026-11-08 WB04-E30)
function islandsImage() {
    const bytes = new Uint8Array(3204);
    const put = (at, value) => { for (let index = 0; index < 4; index += 1) bytes[at + index] = (value >>> (8 * index)) & 0xff; };
    put(0, 3);
    const owners = [2, 1, 0];
    const originals = [1, 1, 9];
    // 지역 BYTE 는 0 이고 현재/원래 owner 는 다르다.
    for (let index = 0; index < 3; index += 1) {
        const at = 4 + index * 32;
        put(at + 4, 1);
        put(at + 8, owners[index]);
        bytes[at + 13] = originals[index];
    }
    return bytes;
}

// 기대 seed/범위/지역을 다시 계산하지 않고 호출 결과와 비교한다.
test('AddRandom 부모: 원본10.78 720행(시각 seed·기본 범위 16..240·현재 소유자 섬)', { skip: !addRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of addRows) {
        if (row[0] === 'type') continue;
        assert.equal(row.length, 9);
        const type = number(row[0]);
        const quantity = number(row[1]);
        const low = number(row[2]);
        const high = number(row[3]);
        let owner = number(row[4]);
        let fort = number(row[5]) !== 0;
        const view = new DataView(new ArrayBuffer(8));
        view.setUint32(0, low, true);
        view.setUint32(4, high, true);
        const time = view.getFloat64(0, true);
        const islands = islandsImage();
        const trace = [];
        const hooks = {
            wallSeconds: () => { trace.push(`T:${low}:${high}`); if (number(row[6])) { owner = 2; fort = true; } return time; },
            local: () => owner,
            island: (current, territory) => { trace.push(`R:${current}:${territory}`); return findFortTerritoryIsland(islands, fort, current | 0, territory); },
            place: (random, area, region, amount, selected, force) => {
                trace.push(`B:${random.state}:${floatToBits(area.left)}:${floatToBits(area.top)}:${floatToBits(area.right)}:${floatToBits(area.bottom)}:${bits(region)}:${amount}:${selected}:${force}`);
                return 7;
            },
        };
        addRandomObjects(type, quantity, hooks);
        assert.equal(trace.join(';'), row[7], `행 ${count + 1}`);
        assert.equal(row[8], '4');
        count += 1;
    }
    assert.equal(count, 720);
});

// 시각 seed 계산과 섬 검색 함수의 직접 계약이다.
test('AddRandom: 시각 seed·섬 검색 규칙', () => {
    // 0 초는 seed 0, 음수 초는 SAR 반올림과 DWORD 감김, 비유한 값은 하위 DWORD 0 이다.
    assert.equal(timeSeed(0), 0);
    assert.equal(timeSeed(Number.NaN), 0);
    assert.equal(timeSeed(Number.POSITIVE_INFINITY), 0);
    assert.equal(timeSeed(1), ((1013 >> 4) + 1013 * 33) >>> 0);
    assert.equal(timeSeed(-1), (((-1013) >> 4) + Math.imul(-1013, 33)) >>> 0);
    const islands = islandsImage();
    // 일반은 원래 owner 와 현재 owner 가 모두 요청 owner 와 같은 첫 항목, Fort 는 현재 owner 가 0 이거나 요청 owner 인 첫 항목이다.
    assert.equal(findFortTerritoryIsland(islands, false, 1, 0), 1);
    assert.equal(findFortTerritoryIsland(islands, false, 5, 0), 127);
    assert.equal(findFortTerritoryIsland(islands, true, 2, 0), 0);
    assert.equal(findFortTerritoryIsland(islands, true, 7, 0), 2);
    assert.equal(findAnyFortTerritoryIsland(islands, 0), 0);
    assert.equal(findAnyFortTerritoryIsland(islands, 3), 127);
    // 손상된 큰 count 는 실제 접근할 때만 거부한다.
    const broken = new Uint8Array(8);
    broken[0] = 0xff; broken[1] = 0xff; broken[2] = 0xff; broken[3] = 0x7f;
    assert.throws(() => findFortTerritoryIsland(broken, true, 1, 0), RangeError);
    // 선택되지 않은 하위와 이미 실행된 벽시각 효과를 잘못 처리하지 않는다.
    let clocks = 0;
    assert.throws(() => addRandomObjects(89, 1, { wallSeconds: () => { clocks += 1; return 0; } }), /미연결/);
    assert.equal(clocks, 1);
    assert.throws(() => addRandomObjects(89, 1, {}), /미연결/);
});

// 문자열만 제품에 주고 뒤의 정수/호출은 독립 기대값으로 읽는다.
test('DestroyAt 위치 해석: 원본10.78 16행(토큰 구분자·따옴표·길이·NUL·atol 다섯 출력)', { skip: !parseRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of parseRows) {
        if (row[0] === 'text') continue;
        assert.equal(row.length, 7);
        const text = hexToText(row[0]);
        const trace = [];
        const result = parseDestroyAtLocation(text, (name) => {
            trace.push(`type:${name === '' ? '-' : textToHex(name)}`);
            if (name === '') return 0;
            // 원문 명시 타입 조회 경계의 ASCII 이름만 해석한다.
            const key = name.replace(/[A-Z]/g, (character) => character.toLowerCase());
            return key === 'suncannon' ? 128 : key === 'buildmana' ? 134 : -1;
        });
        const label = `행 ${count + 1}`;
        assert.deepEqual([bits(result.x), bits(result.y), result.type], [number(row[1]), number(row[2]), number(row[3])], `${label} x/y/type`);
        assert.deepEqual([bits(result.radius), result.owner, trace.join(';')], [number(row[4]), number(row[5]), row[6]], `${label} 반경/소유자/타입 조회`);
        count += 1;
    }
    assert.equal(count, 16);
});

// 입력의 네 물리 슬롯은 명시 가상 표와 타입/상태/소유자만 보유한다. (2026-11-08 WB04-E30)
function destroyPool() {
    const bytes = new Uint8Array(200);
    // 원문 장치의 같은 50바이트 raw 입력을 설치한다(가상 표는 호스트 포인터가 아닌 격리 장치 식별자).
    for (let sid = 1; sid < 4; sid += 1) {
        const table = 0x12001000;
        for (let byte = 0; byte < 4; byte += 1) bytes[sid * 50 + byte] = (table >>> (byte * 8)) & 0xff;
        bytes[sid * 50 + 10] = 99 + sid;
        bytes[sid * 50 + 11] = sid;
        bytes[sid * 50 + 34] = sid === 2 ? 2 : 1;
    }
    return bytes;
}

// 앞 8열은 입력이며 trace/pool 은 원문 기대값이다.
test('DestroyAt 부모: 원본10.78 804행(권한·점 정수화·필터 루프·늦은 읽기·전체 raw)', { skip: !parentRows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 명시 finder 의 입력 순서이며 실제 필터 정답을 계산하지 않는다.
    const orders = [[1, 2, 3, 0], [0], [4, 1, 0], [1, 4, 2, 0]];
    for (const row of parentRows) {
        if (row[0] === 'x') continue;
        assert.equal(row.length, 10);
        const location = { x: signedInt(row[0]), y: signedInt(row[1]), type: number(row[2]), radius: signedInt(row[3]), owner: number(row[4]) };
        const bossMode = number(row[5]);
        const profile = number(row[6]);
        const mutation = number(row[7]);
        let boss = bossMode === 1;
        let capacity = 4;
        let cursor = 0;
        const pool = destroyPool();
        const trace = [];
        const order = orders[profile];
        const hooks = {
            boss: () => boss,
            assertion: (where) => { trace.push(`assert:${where}`); if (bossMode === 2) boss = true; },
            parse: () => { trace.push('parse'); return location; },
            begin: (area) => { trace.push(`begin:${bits(area.left)}:${bits(area.top)}:${bits(area.right)}:${bits(area.bottom)}`); return order[0]; },
            next: () => { cursor += 1; assert.ok(cursor < order.length, 'finder 순서 밖'); const next = order[cursor]; trace.push(`next:${next}`); return next; },
            capacity: () => capacity,
            slot: (sid) => pool.subarray(sid * 50, sid * 50 + 50),
            genus: (sid) => {
                const type = pool[sid * 50 + 10];
                const flags = type === 100 ? 0x100 : type === 101 ? 0x2000 : 0x80;
                trace.push(`genus:${sid}:${flags}`);
                if (mutation === 2) pool[sid * 50 + 34] = 2;
                return flags;
            },
            destroy: (sid, flags) => {
                trace.push(`delete:${sid}:${flags}`);
                pool[sid * 50 + 11] = 1;
                if (mutation === 1) { pool[2 * 50 + 10] = 100; pool[2 * 50 + 34] = 1; }
                if (mutation === 3) capacity = 2;
            },
        };
        destroyObjectsAt('explicit parser boundary', hooks);
        assert.equal(trace.length === 0 ? '-' : trace.join(';'), row[8], `행 ${count + 1} 장치 순서`);
        assert.equal(Buffer.from(pool).toString('hex'), row[9], `행 ${count + 1} 전체 raw`);
        count += 1;
    }
    assert.equal(count, 804);
});

// 실제로 도달하지 않은 하위는 요구하지 않으며 선택된 부모의 누락은 거부한다.
test('DestroyAt: 도달한 장치만 요구하고 권한 진단은 한 번이다', () => {
    let diagnostics = 0;
    const hooks = { boss: () => false, assertion: (line) => { assert.equal(line, 839); diagnostics += 1; } };
    destroyObjectsAt('', hooks);
    assert.equal(diagnostics, 1);
    // 권한이 생기면 위치 해석부터 선택된 부모를 요구한다.
    hooks.boss = () => true;
    assert.throws(() => destroyObjectsAt('', hooks), /미연결/);
    // 타입 조회가 없으면 토큰을 읽은 뒤 거부한다.
    assert.throws(() => parseDestroyAtLocation('(1x2)3', undefined), /미연결/);
});
