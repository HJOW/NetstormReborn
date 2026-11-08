// 타입 정의의 방향 수/물리 프레임 수 읽기 raw-archer-type-data.js(readRawArcherTypeData)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: archertype-x86.tsv 의 `originals` 22행 — 타입 초기화 접두(prefix)와 보호 35클러스터 AST 의 전체 callback/후처리 정상 반환에서 원본 현재 TYPE 의미 필드
//    (방향 수·프레임 수·기본/gump/help/base 프레임·글자 종류 수·SHP 프레임 수), 4바이트 프레임 코드 배열 전체(side/variant/number/flags), 글자 A~P 의 첫 번호 16개와 개수 16개가 같다.
//    prefix 행은 정의를 만들지 않은 기본값(방향 0, 프레임 0, 기본 0, gump 0, help −1, base −1)이다. 보호 자산의 SHP 프레임 수는 sunArcher 의 35 이다.
//  - 계약: 숫자가 아닌 orientations 와 범위 밖 숫자를 누락 속성 0 으로 바꾸지 않고 거부하며, 속성이 없으면 0 이다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E127: 아처 TYPE 자료 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { parseTypeDefinition } from '../www/assets/type-table.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { readRawArcherTypeData } from '../www/core/raw-archer-type-data.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E127)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'archertype-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 보호 sunArcher 의 SHP 프레임 수이다. (2026-11-08 WB04-E127)
const SHAPE_COUNT = 35;

// 원본 AST 에 공급한 ASCII 투영문을 16진수 입력에서 복원한다. (2026-11-08 WB04-E127)
function archerTypeText(value) {
    return Buffer.from(value, 'hex').toString('latin1');
}

// 원본 현재 TYPE 의미 필드와 기존 실제 4바이트 프레임/16문자 Run 조회를 직렬화한다. (2026-11-08 WB04-E127)
function projection(definition, prefix) {
    // prefix 행은 정의를 만들지 않은 기본값이다.
    const data = definition ? readRawArcherTypeData(definition) : { directions: 0, frameCount: 0 };
    const table = definition ? FrameCodeTable.fromDefinition(definition) : new FrameCodeTable([]);
    const letterKinds = [...Array(16).keys()].filter((index) => table.run(0x41 + index).first !== -1).length;
    const values = definition
        ? [data.directions, data.frameCount, definition.defaultFrame, definition.gumpFrame, definition.helpFrame, definition.baseFrame, prefix ? 0 : letterKinds, SHAPE_COUNT]
        : [0, 0, 0, 0, -1, -1, 0, 0];
    const metadata = values.join(',');
    if (prefix) return [metadata, '-', '-'];
    // 실제 4바이트 코드 배열 전체를 그대로 비교한다.
    let codes = '';
    for (const code of table.codes) codes += Buffer.from([code.side, code.variant, definition.clusters[table.codes.indexOf(code)].code.number, code.flags]).toString('hex');
    // first 배열 16개 뒤 count 배열 16개를 원본 순서로 비교한다.
    const runs = [];
    for (const first of [true, false]) {
        for (let letter = 0x41; letter <= 0x50; letter += 1) {
            const run = table.run(letter);
            runs.push(first ? run.first >>> 0 : run.count >>> 0);
        }
    }
    return [metadata, codes, runs.join(',')];
}

test('ArcherTypeData: 원본 10.78 초기화 접두와 35클러스터 callback 후처리의 의미 필드·프레임 코드·글자 구간이 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    let count = 0;
    // 각 행의 input 투영문과 실제 PE 출력만 사용한다.
    for (const row of rows) {
        assert.equal(row.length, 8);
        const prefix = row[1] === 'prefix';
        const definition = prefix ? null : parseTypeDefinition(archerTypeText(row[4]));
        assert.deepEqual(projection(definition, prefix), row.slice(5), `ArcherType 행 ${count}/${row[1]}`);
        count += 1;
    }
    assert.equal(count, 22);
});

test('ArcherTypeData: 숫자가 아닌 방향 속성과 범위 밖 숫자를 누락 속성 0 으로 바꾸지 않고 거부하며 속성이 없으면 0 이다', () => {
    const wrong = parseTypeDefinition('typename sunArcher constructor {orientations="many";} P00::"sarcher.gif" #0;');
    assert.throws(() => readRawArcherTypeData(wrong), TypeError);
    const large = parseTypeDefinition('typename sunArcher constructor {orientations=1e30;} P00::"sarcher.gif" #0;');
    assert.throws(() => readRawArcherTypeData(large), RangeError);
    const plain = parseTypeDefinition('typename sunArcher constructor {group="archer";} P00::"sarcher.gif" #0;');
    assert.deepEqual(readRawArcherTypeData(plain), { directions: 0, frameCount: 1 });
    // float 폭으로 줄여 0 쪽으로 절삭한다(마지막 속성이 이긴다).
    const fractional = parseTypeDefinition('typename sunArcher constructor {orientations=6; orientations=7.9;} P00::"sarcher.gif" #0;P01::"sarcher.gif" #1;');
    assert.deepEqual(readRawArcherTypeData(fractional), { directions: 7, frameCount: 2 });
});
