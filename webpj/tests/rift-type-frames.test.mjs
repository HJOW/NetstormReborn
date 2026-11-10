// 타입 프레임 코드 표 검색(cpppj RiftType.cpp RiftTypeFrames::FindNumber/FindMasked/FindFlags/LetterKinds, 원본 0049a9a0/0049a9e0/0049aa30)을 원본 x86 관측으로 대조한다.
//  - original-x86.tsv 의 FrameFindNumber·FrameFindMasked·FrameFindFlags 각 512행(모두 1536행): 원본 4바이트 코드 배열(방향·변형·번호·플래그)을 그대로 넣고 반환 위치를 비교한다.
//    같은 파일의 Config 270행은 4-A Config 항목이 읽는다.
//  - AssetTests 의 정의 → 프레임 표 확인과 letterKinds(archertype 관측의 글자 종류 수는 type-table 의 letterKinds 와 같은 의미).
// 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E266)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { parseTypeDefinition } from '../www/assets/type-table.js';

const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'original-x86.tsv'));
const frameRows = rows ? rows.filter((row) => row[0].startsWith('FrameFind')) : null;

// 16진수 문자열의 4바이트 코드 배열을 FrameCodeTable 입력으로 만든다. (2026-11-10 WB04-E266)
function tableOf(hex) {
    const bytes = Buffer.from(hex, 'hex');
    const codes = [];
    // 원본 4바이트 코드 순서를 그대로 가져온다.
    for (let pos = 0; pos + 3 < bytes.length; pos += 4) codes.push({ side: bytes[pos], variant: bytes[pos + 1], number: bytes[pos + 2], flags: bytes[pos + 3] });
    return new FrameCodeTable(codes);
}

test('FrameCodeTable: 원본 x86 FrameFindNumber/Masked/Flags 1536행의 반환 위치가 같다', { skip: !frameRows && '원본 fixture 없음' }, () => {
    const counts = { FrameFindNumber: 0, FrameFindMasked: 0, FrameFindFlags: 0 };
    // 행마다 종류에 맞는 검색을 실행한다.
    for (const row of frameRows) {
        const table = tableOf(row[1]);
        const [kind] = row;
        const rest = row.slice(2).map(Number);
        let actual;
        if (kind === 'FrameFindFlags') {
            const [side, variant, flags, expected] = rest;
            actual = [table.findFlags(side, variant, flags), expected];
        } else if (kind === 'FrameFindMasked') {
            const [side, variant, number, mask, expected] = rest;
            actual = [table.findMasked(side, variant, number, mask), expected];
        } else {
            const [side, variant, number, expected] = rest;
            actual = [table.findNumber(side, variant, number), expected];
        }
        assert.equal(actual[0], actual[1], row.join(' ').slice(0, 160));
        counts[kind] += 1;
    }
    assert.deepEqual(counts, { FrameFindNumber: 512, FrameFindMasked: 512, FrameFindFlags: 512 });
});

test('FrameCodeTable: 정의에서 만든 표는 번호를 보관하고 중복은 첫 위치를 돌려준다', () => {
    const type = parseTypeDefinition('typename Sample { cost=2; } P01:default:"a.gif"#0; AA04:default:"b.gif"#1; AA04:default:"c.gif"#2;');
    const frames = FrameCodeTable.fromDefinition(type);
    assert.equal(frames.findNumber('A'.charCodeAt(0), 'A'.charCodeAt(0), 4), 1);
    assert.equal(frames.findNumber('A'.charCodeAt(0), 'A'.charCodeAt(0), 5), -1);
    // 방향·변형·번호가 모두 같은 코드가 둘이어도 먼저 선언된 위치가 이긴다.
    assert.equal(frames.findNumber('A'.charCodeAt(0), 'A'.charCodeAt(0), 4), 1);
});

test('FrameCodeTable: 플래그는 signed 8비트로 부호 확장해 마스크/정확 비교한다', () => {
    const table = new FrameCodeTable([{ side: 65, variant: 65, number: 1, flags: 0x81 }, { side: 65, variant: 65, number: 1, flags: 0x01 }]);
    // 0x81 은 -127 이고 DWORD 마스크 0xffffff00 과의 AND 가 비영이다. 0x01 은 부호 확장해도 0xffffff00 과 겹치지 않는다.
    assert.equal(table.findMasked(65, 65, 1, 0xffffff00), 0);
    assert.equal(table.findMasked(65, 65, 1, 0x00000001), 0);
    // 부호 확장은 상위 비트도 세우므로 0x81 은 0x100 마스크에도 걸리고, 어느 코드도 가지지 않은 비트 0x2 만 -1 이다.
    assert.equal(table.findMasked(65, 65, 1, 0x00000100), 0);
    assert.equal(table.findMasked(65, 65, 1, 0x00000002), -1);
    assert.equal(table.findFlags(65, 65, -127), 0);
    assert.equal(table.findFlags(65, 65, 0x81), -1);
    assert.equal(table.findFlags(65, 65, 1), 1);
    // 인자는 BYTE 로 줄여 비교한다(원본이 push 한 8비트).
    assert.equal(table.findNumber(65 + 256, 65, 1), 0);
});

test('FrameCodeTable: letterKinds 는 구간이 하나라도 있는 글자의 수이다', () => {
    const codes = ['A', 'A', 'C', 'P', 'Q'].map((letter, index) => ({ side: letter.charCodeAt(0), variant: 0x50, number: index, flags: 0 }));
    // 'Q' 는 A~P 밖이라 세지 않는다(run 은 오류이므로 letterKinds 가 직접 훑는 A~P 만 센다).
    assert.equal(new FrameCodeTable(codes).letterKinds(), 3);
    assert.equal(new FrameCodeTable([]).letterKinds(), 0);
});

// 방향이 같고 변형만 다른 코드는 FindNumber 의 일치가 아니다(변형 확인으로 추가: 원본 관측 512행은 이 차이를 구별하지 못했다).
test('FrameCodeTable: findNumber 는 변형 글자까지 비교한다', () => {
    const table = new FrameCodeTable([{ side: 65, variant: 66, number: 1, flags: 0 }, { side: 65, variant: 65, number: 1, flags: 0 }]);
    assert.equal(table.findNumber(65, 65, 1), 1);
    assert.equal(table.findNumber(65, 66, 1), 0);
    assert.equal(table.findNumber(65, 67, 1), -1);
});
