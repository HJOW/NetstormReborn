// 운반물 메뉴 raw-carrier-menu.js(RawCarrierMenu)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: carrier-menu-x86.tsv 의 `originals` 314행 — 입력 22 DWORD(소유자·로컬·부모 TYPE·flags1/flags2·extra·지상 고정·경로·활성·내용물 수/flags/종류·위치 x/y·지도 한계·비용·SP·편집·차단·건설·콜백 변이 비트)
//    → 번역/조회/설명/이동/경로/위치/SP/이벤트/헤더·항목/기본 메뉴의 효과 순서와 각 인자(번역 키·메뉴 문자열 hex·명령·매개변수·활성). `originalCD` 314행은 10.72 라 사용하지 않는다.
//  - 계약: 도달한 누락 하위의 명시 실패, 메뉴 생략 TYPE 의 기본 메뉴 직행, 사제 기본 메뉴 연결, 위치 계산기 연결의 풀 검사.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E96: 운반물 메뉴 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { RawCarrierMenu, bindCarrierMenuDropPoint, bindPriestMenuCarrier } from '../www/core/raw-carrier-menu.js';
import { RawCarrierDropPoint } from '../www/core/raw-carrier-drop-point.js';

// 원본 관측 행이다. 첫 행은 열 이름이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E96)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'carrier-menu-x86.tsv'));
const patchRows = rows ? rows.filter((row) => row[0] === 'originals') : [];

// 예외가 나는지 본다. (2026-11-08 WB04-E96)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 입력 열 위치이다(원문 22 DWORD). (2026-11-08 WB04-E96)
const I = { Owner: 0, Local: 1, RawType: 2, Flags1: 3, Flags2: 4, Extra: 5, Ground: 6, Path: 7, Active: 8, Count: 9, ContentFlags: 10, ContentType: 11, X: 12, Y: 13, Limit: 14, Cost: 15, CdCost: 16, Sp: 17, Editor: 18, Blocked: 19, Building: 20, Mutation: 21 };
const hex = (text) => Buffer.from(text, 'utf8').toString('hex');
const numbers = (value) => Number(BigInt.asUintN(32, BigInt(value)));

// 기대 메뉴를 계산하지 않고 실제 외부 효과와 콜백 변경만 제공하는 장면이다. (2026-11-08 WB04-E96)
function makeScene(row) {
    const scene = {
        pool: new SidPool(24000, { server: false }),
        types: Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, cost: 0 })),
        contained: new ContainedFinderState(),
        common: { localOwner: 0, editor: false, helpCommand: 0x02000017 },
        input: [],
        trace: [],
        points: 0
    };
    for (let index = 1; index <= 22; index += 1) scene.input.push(numbers(row[index]));
    const input = scene.input;
    // 실제 클라이언트 번호 5..70 을 확보한다.
    for (let number = 5; number <= 70; number += 1) assert.equal(scene.pool.allocate(2), number);
    const raw = scene.pool.allocated(50);
    raw[10] = input[I.RawType];
    raw[34] = input[I.Owner];
    raw[40] = input[I.Extra];
    put(raw, 6, input[I.Count] ? 60 : 0, 2);
    // 타입 변경 콜백이 가리키는 다른 타입도 같은 독립 플래그/비용 입력을 갖는다.
    for (const type of scene.types) { type.flags1 = input[I.Flags1]; type.flags2 = input[I.Flags2]; type.cost = bitsToFloat(input[I.CdCost]); }
    // 기존 실제 finder 가 읽는 혼합 내용물 체인이다.
    for (let index = 0; index < input[I.Count]; index += 1) {
        const content = scene.pool.allocated(60 + index);
        content[10] = 6;
        put(content, 18, input[I.ContentType] + index, 2);
        put(content, 20, input[I.ContentFlags] ^ (index & 1), 2);
        put(content, 4, index + 1 < input[I.Count] ? 61 + index : 0, 2);
    }
    const common = scene.common;
    common.localOwner = input[I.Local];
    common.editor = input[I.Editor] !== 0;
    scene.limit = input[I.Limit] | 0;
    const add = (text) => scene.trace.push(text);
    const owner = () => scene.pool.allocated(50);
    scene.state = {
        get localOwner() { return common.localOwner; },
        get editor() { return common.editor; },
        get mapSize() { return scene.limit; },
        get helpCommand() { return common.helpCommand; },
        priestType: 158,
        dropCommand: 0x03000076,
        castCommand: 0x0300011a
    };
    scene.hooks = {
        groundImmobile: (sid) => { add(`G,${sid}`); return input[I.Ground] !== 0; },
        description: (type) => { add(`D,${type}`); if (input[I.Mutation] & 2) owner()[34] = 2; return `Type${type}`; },
        movementType: (type) => { add(`M,${type}`); if (input[I.Mutation] & 4) owner()[10] = 133; return 54; },
        findPath: (parent, type) => { add(`L,${parent},${type}`); return input[I.Path] ? 70 : 0; },
        pathActive: (sid) => { add(`A,${sid}`); return input[I.Active] !== 0; },
        dropPoint: (sid, type) => {
            add(`P,${sid},${type}`);
            const content = scene.pool.allocated(60 + scene.points);
            scene.points += 1;
            if (input[I.Mutation] & 1) put(content, 20, get(content, 20, 2) ^ 1, 2);
            if (input[I.Mutation] & 16) common.localOwner = 2;
            return [bitsToFloat(input[I.X]), bitsToFloat(input[I.Y])];
        },
        patchCost: () => input[I.Cost] | 0,
        money: (ownerNumber) => { add(`S,${ownerNumber}`); if (input[I.Mutation] & 8) common.editor = true; return bitsToFloat(input[I.Sp]); },
        hasEvent: (sid, event) => {
            add(`E,${sid},${event}`);
            if ((input[I.Mutation] & 32) && event === 0x493) scene.pool.allocated(sid)[10] = 133;
            return (event === 0x493 ? input[I.Blocked] : input[I.Building]) !== 0;
        },
        // 번역된 원본 고정 서식의 %s/%d 를 인자 순서대로 처리한다.
        format: (key, args) => {
            add(`T,${hex(key)}`);
            let result = '';
            let argument = 0;
            // 서식 문자를 앞에서부터 처리한다.
            for (let index = 0; index < key.length; index += 1) {
                if (key[index] !== '%') { result += key[index]; continue; }
                index += 1;
                assert.ok(index < key.length && argument < args.length);
                if (key[index] === 's') result += String(args[argument]);
                else { assert.equal(key[index], 'd'); result += String(args[argument]); }
                argument += 1;
            }
            assert.equal(argument, args.length);
            return result;
        },
        header: (text) => add(`H,${hex(text)}`),
        item: (text, command, parameter, enabled) => {
            add(`I,${hex(text)},${command},${parameter},${enabled ? 1 : 0}`);
            if ((input[I.Mutation] & 64) && scene.points < input[I.Count]) {
                const next = scene.pool.allocated(60 + scene.points);
                put(next, 20, get(next, 20, 2) ^ 1, 2);
            }
        },
        damageableMenu: (sid) => add(`B,${sid}`)
    };
    scene.menu = () => new RawCarrierMenu(scene.pool, scene.types, scene.contained, scene.state, scene.hooks);
    return scene;
}

test('CarrierMenu: 원본 10.78 314입력의 번역/조회/설명/이동/경로/위치/SP/이벤트/헤더·항목/기본 메뉴 효과 순서와 인자가 같다', { skip: patchRows.length === 0 && '원본 fixture 없음' }, () => {
    let count = 0;
    // 결과는 실제 PE 의 전체 메뉴 몸체에서 수집한 독립 기대값이다.
    for (const row of patchRows) {
        assert.equal(row.length, 24);
        const scene = makeScene(row);
        scene.menu().open(50);
        assert.equal(scene.trace.join(';'), row[row.length - 1], `행 ${count}: owner${scene.input[I.Owner]} flags${scene.input[I.ContentFlags]} count${scene.input[I.Count]} x${scene.input[I.X]} mut${scene.input[I.Mutation]}`);
        count += 1;
    }
    assert.equal(count, 314);
});

test('CarrierMenu: 도달한 누락 하위는 명시 실패하고 메뉴 생략 TYPE 은 기본 메뉴로 직행하며 사제/위치 연결은 같은 풀만 받는다', { skip: patchRows.length === 0 && '원본 fixture 없음' }, () => {
    const scene = makeScene(patchRows[0]);
    scene.hooks.groundImmobile = null;
    assert.ok(throws(() => scene.menu().open(50)));
    scene.types[158].flags1 = 0x1000000;
    scene.trace = [];
    const carrier = scene.menu();
    const priest = {};
    bindPriestMenuCarrier(priest, carrier);
    priest.baseMenu(50);
    assert.equal(scene.trace.join(';'), 'B,50');
    scene.hooks.damageableMenu = null;
    assert.ok(throws(() => scene.menu().open(50)));
    // 잘못된 판본/TYPE 표 크기/SID 범위는 거부한다.
    assert.ok(throws(() => new RawCarrierMenu({ edition: 'cd1072' }, scene.types, scene.contained, scene.state, {})));
    assert.ok(throws(() => new RawCarrierMenu(scene.pool, scene.types.slice(0, 171), scene.contained, scene.state, {})));
    scene.hooks.damageableMenu = () => {};
    assert.ok(throws(() => scene.menu().open(0)));
    assert.ok(throws(() => scene.menu().open(24000)));
    // 위치 계산기 연결은 같은 풀의 계산기만 받는다.
    const point = new RawCarrierDropPoint(scene.pool, scene.types, { mapLimit: 256, invalidPoint: [0, 0] }, {});
    const hooks = {};
    bindCarrierMenuDropPoint(hooks, scene.pool, point);
    assert.equal(typeof hooks.dropPoint, 'function');
    assert.ok(throws(() => bindCarrierMenuDropPoint(hooks, new SidPool(24000, { server: false }), point)));
});
