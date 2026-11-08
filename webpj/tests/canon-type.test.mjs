// 타입별 Canon 디코더 raw-canon-type.js(decodeCanonType·canonSpecialPatterns·selectBridgePattern)와 territory.js 의 패턴 없는 한 칸 경로를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측 1: canontype-x86.tsv 의 `originals` 2079행 — 타입 전역 9종(영역·다리·섬·받침·전투 넷·일반 사제)×인자×방향(홀수·signed −1 포함)×프레임 표 3종(정상·역순·빈 표)×전역 별칭 입력에서
//    생성자 직후 발자국 범위(Bounds)·남은 칸 전체 순서(프레임·x·y 비트·번호·방향 글자)·끝 상태(프레임·유효 여부·마지막 좌표 비트·번호·끝 범위)가 같다. `originalCD`·`original1037` 행은 사용하지 않는다.
//  - 원본 관측 2: canon-decoder-checking-x86.tsv 156행 — 패턴 없는 한 칸 경로의 명시/기본 프레임·checking·금 간 프레임 코드 비트·진단 복귀/중단·진단이 checking 을 끄는 경우에서
//    진단 사건(식·파일·줄의 바이트)·결과·첫/마지막 공개 상태 4필드·마지막 checking 이 같다.
//  - 계약: 생성 도구(make-canon-type-patterns.py)가 만든 패턴 표가 cpppj 표 선언(.inc)과 같은 글자·가중치(합 287)임, 특수 패턴 그룹·전역 선행 비교·범위 밖 거부, 다리 추첨 경계, 같은 owner 의
//    현재 checking·교체한 진단 콜백을 다음 진행에서 읽기, 명시 프레임 진단 복귀 뒤 물리 표 밖 방향 조회 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E136: 타입별 Canon 디코더 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, bitsToFloat, floatToBits } from './raw-support.mjs';
import { CanonDecoder } from '../www/assets/territory.js';
import { BATTLE_PATTERNS, BRIDGE_PATTERNS, ISLAND_PATTERNS, SUPPORT_PATTERNS } from '../www/assets/canon-type-patterns.js';
import { canonSpecialPatterns, decodeCanonType, selectBridgePattern } from '../www/core/raw-canon-type.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E136)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const typeRows = (readRows(path.join(FIXTURES, 'canontype-x86.tsv')) ?? []).filter((row) => row[0] === 'originals');
const checkingRows = readRows(path.join(FIXTURES, 'canon-decoder-checking-x86.tsv')) ?? [];

// PE 초기 전역의 타입 번호 아홉 개(앞 여덟이 타입 전역, 마지막이 일반 사제)와 판본 행 수이다. (2026-11-08 WB04-E136)
const TYPES = [107, 82, 94, 157, 131, 129, 140, 142, 158];
const ROWS = 2079;
const CHECKING_ROWS = 156;

// 예외가 나는지 본다. (2026-11-08 WB04-E136)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 기계어 분석 입력과 같은 프레임 코드 표이다. profile 0 은 정상(A~P × 번호 0~10, 끝에 첫 항목 중복), 1 은 역순, 2 는 빈 표이다. (2026-11-08 WB04-E136)
function inputFrames(profile) {
    const codes = [];
    if (profile !== 2) {
        // 방향 글자 A~P 마다 번호 0~10 의 프레임 코드를 만든다.
        for (let side = 65; side <= 80; side += 1) {
            for (let number = 0; number <= 10; number += 1) codes.push({ side, variant: 80, number, flags: 0 });
        }
    }
    if (profile === 1) codes.reverse();
    if (profile !== 2) codes.push(codes[0]);
    return codes;
}

test('타입 디코더: 원본 10.78 관측 2079행의 범위·전체 순회·끝 상태가 같다', { skip: typeRows.length === 0 }, () => {
    assert.equal(typeRows.length, ROWS);
    // 모든 회전과 타입 전역의 별칭 입력을 독립적으로 구성한다.
    for (const row of typeRows) {
        assert.equal(row.length, 21);
        const patterns = TYPES.slice(0, 8);
        if (row[10] === '1') patterns[0] = patterns[1];
        if (row[10] === '2') patterns[2] = patterns[4];
        const decoder = decodeCanonType(inputFrames(Number(row[4])), Number(row[11]), patterns, {
            type: TYPES[Number(row[1])], argument: Number(row[2]) | 0, direction: Number(row[3]) | 0, x: bitsToFloat(Number(row[5])), y: bitsToFloat(Number(row[6])),
            explicitFrame: Number(row[9]) !== 0,
        });
        const where = row.slice(0, 12).join(' ');
        assert.deepEqual(decoder.bounds(Number(row[7]), Number(row[8])), row[13].split(',').map(Number), where);
        const expected = row[14] === '-' ? [] : row[14].split(';');
        // 생성자의 첫 진행과 남은 칸 전체 순서·번호·방향을 비교한다.
        for (const entry of expected) {
            assert.ok(decoder.valid, where);
            const fields = entry.split(',').map(Number);
            assert.equal(decoder.frame, fields[0], where);
            assert.equal(floatToBits(decoder.x), fields[1], where);
            assert.equal(floatToBits(decoder.y), fields[2], where);
            assert.equal(decoder.label, fields[3], where);
            assert.equal(decoder.side, fields[4], where);
            decoder.advance();
        }
        assert.ok(!decoder.valid, where);
        assert.equal(decoder.frame, Number(row[15]), where);
        assert.equal(Number(row[16]), 0);
        assert.equal(floatToBits(decoder.x), Number(row[17]), where);
        assert.equal(floatToBits(decoder.y), Number(row[18]), where);
        assert.equal(decoder.label, Number(row[19]), where);
        assert.deepEqual(decoder.bounds(Number(row[7]), Number(row[8])), row[20].split(',').map(Number), where);
    }
});

test('타입 디코더: 생성된 패턴 표가 cpppj 표 선언과 같은 글자·가중치이다', () => {
    // .inc 선언에서 배열 하나의 패턴 줄을 읽어 [첫 값, 폭, 높이, 방향, 변형, 번호] 로 바꾼다.
    const read = (file, marker) => {
        const lines = fs.readFileSync(path.join(REPO_ROOT, 'cpppj', 'src', 'o', file), 'utf8').split(/\r?\n/);
        const start = lines.findIndex((line) => line.startsWith(marker));
        assert.ok(start >= 0, marker);
        const patterns = [];
        // 선언 바로 뒤부터 닫는 `}};` 전까지 읽는다.
        for (const line of lines.slice(start + 1)) {
            if (line.startsWith('}};')) break;
            const matched = /^\s*\{(\d+),\s*(\d+),\s*(\d+),\s*\{\{(.*)\}\}\},/.exec(line);
            if (!matched) continue;
            const cells = [...matched[4].matchAll(/\{0x([0-9a-f]{2}),0x([0-9a-f]{2}),0x([0-9a-f]{2}),0x([0-9a-f]{2})\}/g)];
            const count = Number(matched[2]) * Number(matched[3]);
            patterns.push({
                first: Number(matched[1]), width: Number(matched[2]), height: Number(matched[3]),
                variations: cells.slice(0, count).map((cell) => String.fromCharCode(parseInt(cell[1], 16))).join(''),
                sides: cells.slice(0, count).map((cell) => String.fromCharCode(parseInt(cell[2], 16))).join(''),
                labels: cells.slice(0, count).map((cell) => (cell[4] === '00' ? '-' : String.fromCharCode(parseInt(cell[4], 16)))).join(''),
            });
        }
        return patterns;
    };
    const cases = [
        [BRIDGE_PATTERNS, read('CanonDecoderTables.inc', 'constexpr std::array<CanonPattern, 26> kBridgePatterns'), 26],
        [ISLAND_PATTERNS, read('CanonTypePatterns.inc', 'constexpr std::array<CanonPattern,2> kIslandPatterns'), 2],
        [SUPPORT_PATTERNS, read('CanonTypePatterns.inc', 'constexpr std::array<CanonPattern,1> kSupportPatterns'), 1],
        [BATTLE_PATTERNS, read('CanonTypePatterns.inc', 'constexpr std::array<CanonPattern,1> kBattlePatterns'), 1],
    ];
    for (const [actual, source, count] of cases) {
        assert.equal(actual.length, count);
        assert.deepEqual(actual.map((pattern) => ({ ...pattern })), source);
    }
    assert.equal(BRIDGE_PATTERNS.reduce((sum, pattern) => sum + pattern.first, 0), 287);
});

test('타입 디코더: 특수 패턴 그룹·전역 선행 비교·범위 밖 거부·명시 프레임 무시를 확인한다', () => {
    assert.equal(canonSpecialPatterns(0).length, 2);
    assert.equal(canonSpecialPatterns(1).length, 1);
    assert.equal(canonSpecialPatterns(2).length, 1);
    assert.equal(canonSpecialPatterns(0)[1].variations[0], '3');
    assert.equal(canonSpecialPatterns(1)[0].width, 3);
    assert.equal(canonSpecialPatterns(2)[0].sides[0], 'L');
    assert.ok(throws(() => canonSpecialPatterns(3)));
    assert.ok(throws(() => canonSpecialPatterns(-1)));
    const frames = inputFrames(0);
    const patterns = TYPES.slice(0, 8);
    // 패턴 표 밖 번호·잘못된 회전·비유한 좌표·잘못된 전역 개수를 거부한다.
    assert.ok(throws(() => decodeCanonType(frames, 3, patterns, { type: 94, argument: 2, direction: 0, x: 0, y: 0 })));
    assert.ok(throws(() => decodeCanonType(frames, 3, patterns, { type: 157, argument: -1, direction: 0, x: 0, y: 0 })));
    assert.ok(throws(() => decodeCanonType(frames, 3, patterns, { type: 131, argument: 1, direction: 0, x: 0, y: 0, explicitFrame: true })));
    assert.ok(throws(() => decodeCanonType(frames, 3, patterns, { type: 94, argument: 0, direction: 8, x: 0, y: 0 })));
    assert.ok(throws(() => decodeCanonType(frames, 3, patterns, { type: 94, argument: 0, direction: 0, x: Infinity, y: 0 })));
    assert.ok(throws(() => decodeCanonType(frames, 3, patterns.slice(0, 7), { type: 94, argument: 0, direction: 0, x: 0, y: 0 })));
    assert.ok(throws(() => decodeCanonType(frames, 3, patterns, { type: 158, argument: 0, direction: 0, x: NaN, y: 0 })));
    // 빈 프레임 표에서는 패턴 칸의 프레임을 못 찾아 유효 칸이 없다.
    assert.ok(!decodeCanonType(inputFrames(2), 3, patterns, { type: 157, argument: 0, direction: 0, x: 0, y: 0 }).valid);
    // 패턴 타입에서는 explicitFrame 이 무시된다(타입 선택이 앞선다).
    assert.equal(decodeCanonType(frames, 3, patterns, { type: 94, argument: 1, direction: 0, x: 0, y: 0, explicitFrame: true }).frame, 168);
    // 앞 네 전역이 먼저 비교된다: puzzlePiece 와 같은 번호의 bridge 전역은 영역 패턴을 고른다.
    const alias = patterns.slice();
    alias[1] = alias[0];
    assert.equal(decodeCanonType(frames, 3, alias, { type: 107, argument: 7, direction: 0, x: 0, y: 0 }).valid, true);
});

test('다리 추첨: 누적 가중치와 signed 나머지를 `<=` 로 비교한다', () => {
    // 가중치는 0, 30, 10, 4, 50 … 이라 누적은 0, 30, 40, 44, 94 … 이다.
    const cases = [[0, 0], [1, 1], [30, 1], [31, 2], [40, 2], [41, 3], [44, 3], [45, 4], [287, 0], [288, 1], [-1, 0], [-5, 0], [-287, 0], [-288, 0]];
    for (const [random, expected] of cases) assert.equal(selectBridgePattern(random), expected, `random ${random}`);
    // 나머지의 최댓값 286 은 마지막 항목에서 끝난다.
    assert.equal(selectBridgePattern(286), 25);
    assert.equal(selectBridgePattern(-0x80000000), 0);
});

test('패턴 없는 한 칸 경로: 원본 관측 156행의 진단 사건·결과·공개 상태가 같다', { skip: checkingRows.length === 0 }, () => {
    assert.equal(checkingRows.length, CHECKING_ROWS);
    // 반복자 구성에서 이미 호출하는 첫 진행과 유효 칸 뒤 끝 상태를 함께 비교한다.
    for (const row of checkingRows) {
        assert.equal(row.length, 6);
        const input = row[0].split(',').map(Number);
        assert.equal(input.length, 6);
        const frame = input[1];
        let checking = input[2] >>> 0;
        const events = [];
        let outcome = 'return';
        let first = '-';
        let last = '-';
        const codes = [{ side: 65, variant: 80, number: 0, flags: input[3] }, { side: 65, variant: 80, number: 1, flags: input[3] }];
        const hex = (text) => Buffer.from(text, 'utf8').toString('hex');
        const diagnostics = {
            checking: () => checking,
            assertion: (expression, file, line) => {
                events.push(`X:${hex(expression)}:${hex(file)}:${line}`);
                if (!input[4]) throw new Error('명시 Canon 진단 중단');
                // 진단이 checking 을 꺼도 원문처럼 좌표/순회 효과는 계속한다.
                if (input[5]) checking = 0;
            },
        };
        // 공개 네 필드를 원문 비트 형식으로 읽는다.
        const snapshot = (decoder) => `${decoder.frame},${decoder.valid ? 1 : 0},${floatToBits(decoder.x)},${floatToBits(decoder.y)}`;
        try {
            const decoder = new CanonDecoder(codes, null, 0, 20.5, -3.25, null, diagnostics, { defaultFrame: frame, argument: frame, explicitFrame: input[0] !== 0 });
            first = snapshot(decoder);
            if (decoder.valid) decoder.advance();
            last = snapshot(decoder);
        } catch (error) {
            if (error.message !== '명시 Canon 진단 중단') throw error;
            outcome = 'assertion';
        }
        assert.equal(events.join(';'), row[1], row[0]);
        assert.equal(outcome, row[2], row[0]);
        assert.equal(first, row[3], row[0]);
        assert.equal(last, row[4], row[0]);
        assert.equal(checking >>> 0, Number(row[5]) >>> 0, row[0]);
    }
});

test('Canon 진단: 같은 owner 의 현재 checking·교체한 진단 콜백을 다음 진행에서 읽는다', () => {
    const codes = [{ side: 65, variant: 80, number: 1, flags: 0x20 }];
    const pattern = { first: 1, width: 2, height: 1, sides: 'AA', variations: '11', labels: 'ab' };
    let checking = 0;
    let called = 0;
    const diagnostics = { checking: () => checking, assertion: null };
    const decoder = new CanonDecoder(codes, pattern, 0, 20, 30, null, diagnostics);
    assert.ok(decoder.valid);
    assert.equal(decoder.x, 20);
    assert.equal(called, 0);
    diagnostics.assertion = (expression, file, line) => {
        assert.equal(expression, 'frame == EMPTY_FRAME || !(typeStruct->frameInfo[frame].flags & FF_CRACKED)');
        assert.equal(file, '\\Ns\\O\\Canondecoder.cpp');
        assert.equal(line, 0x325);
        called += 1;
        checking = 0;
    };
    checking = 0x80000000;
    decoder.advance();
    assert.ok(decoder.valid);
    assert.equal(decoder.x, 21);
    assert.equal(called, 1);
    assert.equal(checking, 0);
    decoder.advance();
    assert.ok(!decoder.valid);
    // 실제 선택된 진단 장치가 없으면 성공을 합성하지 않는다.
    checking = 1;
    diagnostics.assertion = null;
    assert.ok(throws(() => new CanonDecoder(codes, pattern, 0, 0, 0, null, diagnostics)));
    // 잘못된 명시값 진단이 복귀해도 물리 메모리 안전 경계는 유지한다.
    checking = 0;
    diagnostics.assertion = (expression, file, line) => { assert.equal(line, 0x305); };
    const invalid = new CanonDecoder(codes, null, 0, 0, 0, null, diagnostics, { defaultFrame: 0, argument: 2, explicitFrame: true });
    assert.ok(invalid.valid);
    assert.equal(invalid.frame, 2);
    assert.ok(throws(() => invalid.side));
});
