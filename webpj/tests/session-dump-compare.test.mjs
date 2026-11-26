// 판번호 1 덤프 비교의 누락·추가·값·순서와 잘못된 입력을 검사한다. (2026-11-19 WB04-E870)
import test from 'node:test';
import assert from 'node:assert/strict';
import { compareSessionDumps, parseSessionDump } from '../tools/session-dump-compare.mjs';

// 실제 덤프와 같은 월드 없는 최소 머리와 두 프로세스 행이다.
const base = ['netstorm_session_dump\t1', 'edition\toriginals', 'frame\t2\t0', 'random\t1\t2', 'world\t0', 'kernel\t2\t3',
    'process\t1\t0\t0', 'process\t2\t0\t0', 'end', ''].join('\n');

// 값·추가·누락·순서 차이를 각각 구분한다.
test('Session 덤프 비교: 현재 행과 누락·추가·순서 차이', () => {
    assert.deepEqual(compareSessionDumps(base, base), { differences: [], ignored: [] });
    assert.equal(compareSessionDumps(base, base.replace('random\t1\t2', 'random\t1\t3')).differences[0].kind, 'changed');
    assert.equal(compareSessionDumps(base, base.replace('process\t2\t0\t0\n', '')).differences[0].kind, 'missing');
    assert.equal(compareSessionDumps(base, base.replace('end\n', 'process\t3\t0\t0\nend\n')).differences[0].kind, 'extra');
    const reordered = base.replace('process\t1\t0\t0\nprocess\t2\t0\t0', 'process\t2\t0\t0\nprocess\t1\t0\t0');
    assert.deepEqual(compareSessionDumps(base, reordered).differences.map((entry) => entry.kind), ['order']);
});

// 누락된 머리·중복·열 수·잘못된 번호를 같은 빈 결과로 오인하지 않는다.
test('Session 덤프 비교: 잘못된 형식은 거부한다', () => {
    assert.throws(() => parseSessionDump(base.replace('netstorm_session_dump\t1', 'netstorm_session_dump\t2')), /판번호/);
    assert.throws(() => parseSessionDump(base.replace('end\n', 'process\t1\t0\t0\nend\n')), /중복/);
    assert.throws(() => parseSessionDump(base.replace('random\t1\t2', 'random\t1')), /형식/);
    assert.throws(() => parseSessionDump(base.replace('process\t1\t0\t0', 'process\t-1\t0\t0')), /번호/);
    assert.throws(() => parseSessionDump(base.replace('world\t0', 'world\t1')), /월드 행 누락/);
    assert.throws(() => parseSessionDump(base.replace('end\n', 'slot\t1\t00\nend\n')), /월드 없는 행/);
    // AI도 월드 전용이며 고정37칸 이외의 확장은 조용히 버리지 않는다.
    const ai = ['ai', 1, 536892288, ...Array(34).fill(0)].join('\t');
    assert.throws(() => parseSessionDump(base.replace('end\n', ai + '\nend\n')), /월드 없는 행/);
    assert.throws(() => parseSessionDump(base.replace('end\n', ai + '\t0\nend\n')), /형식/);
});
