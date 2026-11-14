// UI 현재 상태 계열의 원본 10.78 독립 관측 대조: 사용자 강조 모드/마우스 TYPE 초기화(RawUserHighlight, 178행)와 현재 TYPE HP getter(CurrentTypeHitPoints, 10.78 행)를
// cpppj `UserHighlightTests`·`PriestRegenHitPointCurrentTests`(CurrentTypeHitPointReader) 와 같은 입력 구성으로 비교한다. 입력 열만 장치/전역에 공급하고 관측 열은 비교에만 쓴다.
// 원본 게임을 실행하지 않는다. (2026-11-15 WB04-E482)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { RawUserHighlight } from '../www/core/raw-user-highlight.js';
import { makeCurrentTypeHitPoints } from '../www/core/current-type-hit-points.js';

// 탭 구분 fixture 의 머리글/주석 줄을 건너뛴 행 목록이다(빈 열 보존).
function rows(name) { return fs.readFileSync(new URL(`../../cpppj/tests/fixtures/${name}`, import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('#')).map((line) => line.split('\t')); }

test('RawUserHighlight 원본 10.78 178행: 강조 모드·마우스 TYPE 초기화의 전체 전역과 커서/역투영/이동 경계의 인자·순서가 일치한다', () => {
    const table = rows('user-highlight-x86.tsv'); const failures = [];
    assert.equal(table.length, 178);
    // 입력/출력 열은 쉼표로 나눈 부호 없는 십진수이다(부호 있는 입력의 상위 비트를 버리지 않는다).
    const numbers = (text) => text.split(',').map(Number);
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 3, `행 ${index}`);
        const input = numbers(row[0]); assert.equal(input.length, 10);
        // 각 행마다 새 전역 저장소와 기록을 쓴다(이전 행의 callback 변경을 넘기지 않는다).
        const state = { mode: input[1], selectedSid: input[3], mouseType: input[4], mouseX: input[5] | 0, mouseY: input[6] | 0 }; const trace = [];
        // 경계가 도달했을 때만 입력이 지정한 변경(모드 7·선택 202·TYPE 77·좌표 -33/65)을 적용한다.
        const mutate = (boundary) => { if (input[7] !== boundary) return; state.mode = 7; state.selectedSid = 202; state.mouseType = 77; state.mouseX = -33; state.mouseY = 65; };
        const hooks = {
            selectCursor: (cursor) => { trace.push(`K:${cursor >>> 0}:${state.mouseType >>> 0}`); mutate(1); },
            project: (x, y) => { trace.push(`P:${x >>> 0}:${y >>> 0}:${state.mouseType >>> 0}`); mutate(2); return [input[8], input[9]]; },
            move: (sid, x, y, force) => { trace.push(`M:${sid >>> 0}:${x >>> 0}:${y >>> 0}:${force >>> 0}`); mutate(3); },
        };
        const highlight = new RawUserHighlight(state, hooks);
        if (input[0] === 0) highlight.resetMouse(); else highlight.highlight(input[2]);
        const problems = [];
        if ((trace.join(';') || '-') !== row[1]) problems.push('trace');
        if ([state.mode >>> 0, state.selectedSid >>> 0, state.mouseType >>> 0, state.mouseX >>> 0, state.mouseY >>> 0].join(',') !== row[2]) problems.push('globals');
        if (problems.length) failures.push(`${index}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});

test('CurrentTypeHitPoints 원본 10.78 행: 현재 전역·저장된 물리 TYPE 의 HP 진단/사제 1/4 읽기와 진단 복귀 뒤 표 이동이 일치한다', () => {
    const all = rows('rifttype-hit-points-x86.tsv'); const table = all.filter((row) => row[0] === 'originals'); const failures = []; let count = 0;
    assert.ok(table.length > 500 && table.length < all.length);
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 6, `행 ${index}`);
        const input = row[1].split(','); assert.equal(input.length, 9);
        // 초기 번호가 음수인 getNum 행은 직접 getter 검사의 몫이라 이 소비자에서는 건너뛴다(cpppj 와 같은 선택).
        if (Number(input[7])) continue;
        const number = Number(input[0]); const profile = Number(input[8]);
        // 190칸 물리 표에서 선택 레코드 하나(번호 + 1)만 입력 HP/플래그를 갖는다.
        const physical = Array.from({ length: 190 }, () => ({ flags1: 0, maxHitPoints: 0 })); const selected = physical[1 + number];
        selected.maxHitPoints = Number(input[1]) | 0; selected.flags1 = Number(input[2]) >>> 0;
        let checking = Number(input[3]); let quarter = Number(input[4]) !== 0; let priest = Number(input[5]); const dais = Number(input[6]); let shift = 0; const diagnostics = [];
        const reader = makeCurrentTypeHitPoints(() => physical.slice(1 + shift, 1 + shift + 188), { checking: () => checking, quarter: () => quarter, priest: () => priest, dais: () => dais }, (expression, file, line) => {
            diagnostics.push(`${expression}|${file}|${line}`);
            // 진단 UI 복귀가 현재 전역/표를 바꾸는 입력이다: checking 끄기·1/4 켜기·선택 HP 교체·표 이동·사제 번호.
            if (profile) { checking = 0; quarter = true; selected.maxHitPoints = [-13, -37, 41][profile - 1]; shift = [0, 1, -1][profile - 1]; priest = (74 - shift) >>> 0; }
        });
        const problems = [];
        if (reader(number) !== (Number(row[2]) | 0)) problems.push('value');
        if ((diagnostics.join(';') || '-') !== row[3]) problems.push('diagnostics');
        if (problems.length) failures.push(`${index}: ${problems.join(',')}`);
        count += 1;
    }
    assert.deepEqual(failures.slice(0, 20), []);
    assert.equal(count, 579);
});

// 원본 관측 fixture 는 유효한 번호(0~187)만 담으므로 번호 범위/미연결 경계를 기준선 소스(CurrentTypeHitPoints.cpp)의 계약으로 독립 확인한다: 번호 188 이상(음수는 부호 없는 큰 수)은 범위 오류(out_of_range→RangeError),
// 표 조회 장치가 없으면 읽기 위치에서 거부한다. (2026-11-15 WB04-E482)
test('CurrentTypeHitPoints 번호 범위와 표 조회 미연결: 188 이상·음수 번호는 RangeError, 조회 장치 없음은 호출 시점의 Error 다(독립 확인)', () => {
    const physical = Array.from({ length: 190 }, () => ({ flags1: 0, maxHitPoints: 7 }));
    const reader = makeCurrentTypeHitPoints(() => physical.slice(1, 189), { checking: () => 0, quarter: () => false, priest: () => 74, dais: () => 165 }, () => assert.fail('진단 금지'));
    assert.equal(reader(187), 7); assert.equal(reader(0), 7);
    for (const bad of [188, 189, 0xffffffff, -1]) assert.throws(() => reader(bad), { name: 'RangeError' }, String(bad));
    // 장치가 없는 getter 는 만들 수 있지만 읽을 때 거부한다(현재 표는 생성 때 캐시하지 않는다).
    const missing = makeCurrentTypeHitPoints(undefined, { checking: () => 0, quarter: () => false, priest: () => 74, dais: () => 165 }, () => {});
    assert.throws(() => missing(0), { name: 'Error', message: /미연결/ });
});
