// 대응표 `port` 행을 `session` 으로 올리는 판정(tools/port-map-session.mjs 의 planSession)을 작은 표로 확인한다. (2026-11-23 WB04-E1282)
import test from 'node:test';
import assert from 'node:assert/strict';
import { planSession, SESSION_MARK } from '../tools/port-map-session.mjs';

// 한 행(cpp·class·web·base·status·note)을 탭으로 이은 줄이다.
const row = (cpp, web, status, note = '[규칙] 메모') => [cpp, 'port', web, 'aaaaaaaa', status, note].join('\t');

// 커버리지 JSON 의 모양: 모듈 경로마다 실행한 함수 수와 전체 함수 수이다.
const coverage = { modules: {
    'www/core/a.js': { executed: 3, total: 5 },
    'www/core/b.js': { executed: 1, total: 1 },
    'www/core/idle.js': { executed: 0, total: 4 },
} };

test('port 행은 모든 웹 모듈이 함수를 실행했을 때만 session 으로 오른다', () => {
    const text = ['# 머리 행', 'cpp\tclass\tweb\tbase\tstatus\tnote',
        row('o/One.cpp', 'www/core/a.js', 'port'),
        row('o/Two.cpp', 'www/core/a.js;www/core/b.js', 'port'),
        row('o/Idle.cpp', 'www/core/a.js;www/core/idle.js', 'port'),
        row('o/Gone.cpp', 'www/core/none.js', 'port'),
        row('o/Part.cpp', 'www/core/a.js', 'partial'),
        row('o/Skip.cpp', 'www/core/a.js', '-'),
        row('o/Done.cpp', 'www/core/a.js', 'oracle')].join('\n');
    const plan = planSession(text, coverage);
    const lines = plan.text.split('\n');
    assert.equal(plan.promoted, 2); assert.equal(plan.kept, 2);
    assert.deepEqual(plan.reasons, { noModule: 0, notExecuted: 1, missingFromCoverage: 1 });
    // 올라간 행은 상태가 바뀌고 표식이 붙으며 다른 칸은 그대로다.
    assert.equal(lines[2].split('\t')[4], 'session'); assert.ok(lines[2].endsWith(SESSION_MARK)); assert.equal(lines[2].split('\t')[2], 'www/core/a.js');
    assert.equal(lines[3].split('\t')[4], 'session');
    // 함수 미실행·커버리지에 없음·partial·제외·이미 oracle 인 행과 머리 행은 그대로다.
    for (const index of [0, 1, 4, 5, 6, 7, 8]) assert.equal(lines[index], text.split('\n')[index]);
});

test('줄바꿈(CRLF)을 보존하고 두 번 적용해도 표식이 겹치지 않는다', () => {
    const text = [row('o/One.cpp', 'www/core/a.js', 'port'), ''].join('\r\n');
    const first = planSession(text, coverage);
    assert.ok(first.text.includes('\r\n')); assert.ok(!first.text.replace(/\r\n/g, '').includes('\n'));
    const second = planSession(first.text, coverage);
    // 이미 session 인 행은 다시 건드리지 않는다.
    assert.equal(second.promoted, 0); assert.equal(second.text, first.text);
    assert.equal(first.text.split(SESSION_MARK).length - 1, 1);
});
