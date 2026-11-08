// 객체 선택 raw-squid-selection.js(RawSquidSelection·squidSelectionEntry·selectionUsesRange·startSelectionContentPreview)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: squid-selection-x86.tsv 5600행(Mark 선택 비트 쓰기·사정거리 종류·내용물 표시 입구). 입력 10개 DWORD(종류·VT·extra·value·표시 변이·group·flags2·TYPE·로딩 깊이·regular 인자 기록)에서
//    결과·효과 순서·현재 [VT, extra, TYPE] 를 비교한다. 기대값을 코드로 만들지 않는다.
//  - 계약: SquidSelection_LazyDisplayAndUnknownVtable(기본 +7c 는 표시를 요구하지 않고 파생·미지의 표는 비트 쓰기 전에 거부).
//  - 표 대조: squid-selection-vtables.js 83칸을 cpppj `SelectionVtables.inc` 와 행 단위로 비교한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E58: 객체 선택 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawSquidSelection, squidSelectionEntry, selectionUsesRange, startSelectionContentPreview } from '../www/core/raw-squid-selection.js';
import { SQUID_SELECTION_VTABLES } from '../www/core/squid-selection-vtables.js';

// 원본 관측 파일이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E58)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const rows = readRows(path.join(FIXTURES, 'squid-selection-x86.tsv'));

// 쉼표 십진수를 부호 없는 DWORD 목록으로 읽는다(부호 있던 group 도 상위 비트를 유지한다). (2026-11-08 WB04-E58)
const numbers = (text) => text.split(',').map((part) => Number(part) >>> 0);

// 예외가 나는지 본다. (2026-11-08 WB04-E58)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 상태 필터 없이 쓰는 물리 슬롯 SID 50 의 명시 입력과 표시 효과를 담는 장면이다. (2026-11-08 WB04-E58)
function selectionScene() {
    const pool = new SidPool(24000, { server: true });
    const scene = { pool, row: [], trace: [] };
    // 물리 슬롯의 쓰기 대여이다.
    scene.raw = () => pool.physical(50);
    // 원문 boundary 인자를 부호 없는 십진수로 기록한다.
    scene.event = (name, values = []) => {
        scene.trace.push([name, ...values.map((value) => String(value >>> 0))].join(':'));
    };
    // 경계가 수정하는 현재 입력만 공급한다(비트 변경·종류·Regular 관문은 제품이 한다).
    scene.refresh = (sid) => {
        assert.equal(sid, 50);
        const raw = scene.raw();
        scene.event('R', [sid, raw[40], get(raw, 0)]);
        if (scene.row[4] === 1) raw[40] = 0xa5;
        else if (scene.row[4] === 2) { raw[40] = 0x12; put(raw, 0, 0xdeadbeef); }
        else if (scene.row[4] === 3) raw[40] = 0xff;
    };
    // 관측과 같은 물리 raw 입력만 설치하고 이전 효과를 지운다.
    scene.prepare = (values) => {
        scene.row = values;
        assert.equal(values.length, 10);
        scene.trace = [];
        const raw = scene.raw();
        raw.fill(0);
        put(raw, 0, values[1]);
        raw[10] = values[7] & 0xff;
        raw[11] = 0xff;
        raw[40] = values[2] & 0xff;
    };
    return scene;
}

test('SquidSelection: 원본 5600행의 선택 비트 쓰기·사정거리 종류·내용물 표시 입구가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = selectionScene();
    let count = 0;
    // 모든 관측 행을 순서대로 재생한다.
    for (const fields of rows) {
        assert.equal(fields.length, 4);
        scene.prepare(numbers(fields[0]));
        const row = scene.row;
        let result = 0;
        if (row[0] === 0) {
            new RawSquidSelection(scene.pool, (sid) => scene.refresh(sid)).mark(50, row[3]);
        } else if (row[0] === 1) {
            result = selectionUsesRange({ group: row[5] | 0, flags2: row[6] }) ? 1 : 0;
        } else {
            startSelectionContentPreview(scene.pool, 50, row[8], (sid, event, payload) => {
                scene.event('A', [40]);
                if (row[9]) scene.event('C', [event, sid, floatToBits(payload)]);
            }, () => {
                scene.event('!');
                scene.trace[scene.trace.length - 1] += ':!s.isForm():144';
                if (row[4] === 4) scene.raw()[10] = 158;
            });
        }
        assert.equal(result, Number(fields[1]));
        assert.equal(scene.trace.length === 0 ? '-' : scene.trace.join(';'), fields[2]);
        const raw = scene.raw();
        assert.deepEqual([get(raw, 0), raw[40], raw[10]], numbers(fields[3]));
        count += 1;
    }
    assert.equal(count, 5600);
});

test('SquidSelection: 기본 +7c 는 표시를 요구하지 않고 파생·미지의 표시는 원본 비트 쓰기 전에 거부한다', () => {
    const scene = selectionScene();
    scene.prepare([0, 0x501dd0, 0xa1, 1, 0, 0, 0, 158, 0, 1]);
    const basic = new RawSquidSelection(scene.pool, undefined);
    basic.mark(50, 1);
    assert.equal(scene.raw()[40], 0xa5);
    put(scene.raw(), 0, 0x501ea0);
    scene.raw()[40] = 0xa1;
    assert.ok(throws(() => basic.mark(50, 1)), '파생 +7c 는 표시 갱신 없이 거부');
    assert.equal(scene.raw()[40], 0xa1);
    put(scene.raw(), 0, 0xdeadbeef);
    assert.ok(throws(() => basic.mark(50, 1)), '미지의 표는 거부');
    assert.equal(scene.raw()[40], 0xa1);
    assert.ok(throws(() => basic.mark(0, 1)));
    assert.ok(throws(() => basic.mark(24000, 1)));
    assert.equal(squidSelectionEntry(0xdeadbeef), 0);
});

test('SquidSelection: 가상 표 83칸이 cpppj SelectionVtables.inc 와 행 단위로 같다', () => {
    const file = path.join(REPO_ROOT, 'cpppj', 'src', 'o', 'SelectionVtables.inc');
    if (!fs.existsSync(file)) return;
    // 한 줄 `{0x…U,0x…U,0x…U},` 형태의 행만 읽는다.
    const parsed = fs.readFileSync(file, 'utf8').split(/\r?\n/)
        .map((line) => /^\s*\{(0x[0-9a-fA-F]+)U,(0x[0-9a-fA-F]+)U,(0x[0-9a-fA-F]+)U\},?\s*$/.exec(line))
        .filter(Boolean)
        .map((match) => [1, 2, 3].map((index) => Number.parseInt(match[index], 16)));
    assert.equal(parsed.length, 83);
    assert.deepEqual(SQUID_SELECTION_VTABLES.map((row) => [...row]), parsed);
});
