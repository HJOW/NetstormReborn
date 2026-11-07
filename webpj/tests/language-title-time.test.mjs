// 언어 변경 요청·창 제목 선택·게임 시간 표시 토글/시계를 원본 10.78 관측으로 대조한다(WB04-E34, cpppj LanguageRequestTests·WindowTitleTests·GameTimeMessageTests(T/t 키)·GameTimeDisplay 이식).
//  - language-request-x86.tsv 337행: 설정 콜백 효과 뒤의 현재 명령/카운터와 큐 writer 12바이트. window-title-x86.tsv 중 10.78(originals) 576행: 편집/요새/미션 우선순위·readonly·NULL/빈 인자·설정 조회 순서와 최종 문자열.
//  - game-time-key-x86.tsv 168행: T/t 의 뗌/모달/정지/수정키와 DWORD 토글. 계약: 선택되지 않은 장치 비요구, 예외 앞의 부분 효과 보존, CRT clock 호출 뒤 원점 재조회와 signed 차이의 0 쪽 절삭.
// 전투 시간 메시지 90행(MessageGump 192바이트·서식/기한/Idle 순서)은 MessageGump 단위에서 대조한다. (2026-11-08 WB04-E34: 언어 요청·창 제목·게임 시간 표시 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { InputCode, InputQueue } from '../www/core/input-queue.js';
import {
    LANGUAGE_CHANGED_COMMAND, SET_LANGUAGE_COMMAND, dispatchLanguageChanged, dispatchSetLanguage, originalLanguageName, requestOriginalLanguage,
} from '../www/core/language-request.js';
import { refreshWindowTitle } from '../www/core/window-title.js';
import { dispatchGameTimeKey, readGameTimeSeconds } from '../www/core/game-time-display.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E34)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const languageRows = fixture('language-request-x86.tsv');
const titleRows = fixture('window-title-x86.tsv');
const keyRows = fixture('game-time-key-x86.tsv');

// 원문 DWORD 읽기(감김)와 16진 변환이다. (2026-11-08 WB04-E34)
const number = (text) => Number(BigInt(text) & 0xffffffffn);
const hexOf = (text) => Buffer.from(text, 'latin1').toString('hex');

// 큐 사건 12바이트(코드·x·y)를 리틀엔디언 16진으로 쓴다. (2026-11-08 WB04-E34)
function eventHex(event) {
    const bytes = Buffer.alloc(12);
    bytes.writeUInt32LE(event.code >>> 0, 0);
    bytes.writeUInt32LE(event.x >>> 0, 4);
    bytes.writeUInt32LE(event.y >>> 0, 8);
    return bytes.toString('hex');
}

// 기대값은 제품 모델이 아닌 최초 원문 관측에서 읽는다.
test('LanguageRequest: 원본10.78 337행(설정 콜백 뒤 현재 명령·카운터·큐 writer)', { skip: !languageRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of languageRows) {
        assert.equal(row.length, 8);
        const requested = number(row[0]);
        const current = number(row[1]) | 0;
        const refresh = { value: number(row[2]) };
        const mutation = number(row[3]);
        let currentNow = current;
        let changed = LANGUAGE_CHANGED_COMMAND;
        let trace = '';
        const queue = new InputQueue();
        const hooks = {
            set: (key, name) => {
                assert.equal(key, 'currentLanguage');
                trace = `S,${hexOf(name)},${refresh.value}`;
                if (mutation & 1) { currentNow = requested | 0; refresh.value = 100; changed = 0xa2001234; }
            },
            changed: () => changed,
            queue: (code, first, second) => {
                trace += `;Q,${code},${first},${second},${refresh.value}`;
                if (mutation & 2) refresh.value = 0xffffffff;
                queue.pushCommand(code, first | 0, second | 0);
            },
        };
        requestOriginalLanguage(requested, current, refresh, hooks);
        const label = `행 ${count + 1}`;
        assert.deepEqual([currentNow >>> 0, refresh.value], [number(row[4]), number(row[5])], `${label} 전역`);
        assert.equal(trace === '' ? '-' : trace, row[7], `${label} 호출 순서`);
        const actual = queue.empty ? '-' : eventHex(queue.pop(false));
        assert.equal(actual, row[6], `${label} 큐`);
        assert.ok(queue.empty);
        count += 1;
    }
    assert.equal(count, 337);
});

// 선택되지 않은 장치를 요구하지 않고 예외 때 앞선 효과를 취소하거나 뒤 카운터를 증가시키지 않는다.
test('LanguageRequest: 같은 번호는 자연 반환, 부분 실패와 입력 분배·이름표', () => {
    let current = 3;
    const refresh = { value: 7 };
    requestOriginalLanguage(3, current, refresh, {});
    assert.equal(refresh.value, 7);
    const hooks = { set: () => { current = 6; throw new Error('set'); } };
    assert.throws(() => requestOriginalLanguage(6, current, refresh, hooks), /set/);
    assert.deepEqual([current, refresh.value], [6, 7]);
    current = 0;
    hooks.set = () => { current = 3; };
    hooks.changed = () => LANGUAGE_CHANGED_COMMAND;
    hooks.queue = () => { refresh.value = 91; throw new Error('queue'); };
    assert.throws(() => requestOriginalLanguage(3, current, refresh, hooks), /queue/);
    assert.deepEqual([current, refresh.value], [3, 91]);
    // 이름표: 0/1 은 별칭을 합치지 않고 표 밖은 거부한다.
    assert.deepEqual([0, 1, 2, 3, 4, 5, 6, 7].map(originalLanguageName), ['english', 'english', 'french', 'german', 'spanish', 'japanese', 'portuguese', 'korean']);
    assert.throws(() => originalLanguageName(8), RangeError);
    // 일반 입력: 뗌은 소비만, 두 번째 DWORD 를 전달하고 수정키는 막지 않는다.
    const requests = [];
    assert.equal(dispatchSetLanguage({ code: SET_LANGUAGE_COMMAND, x: 99, y: -1 }, (value) => requests.push(value)), true);
    assert.equal(dispatchSetLanguage({ code: (SET_LANGUAGE_COMMAND | InputCode.release) >>> 0, x: 0, y: 2 }, undefined), true);
    assert.equal(dispatchSetLanguage({ code: (SET_LANGUAGE_COMMAND | InputCode.control) >>> 0, x: 0, y: 4 }, (value) => requests.push(value)), true);
    assert.equal(dispatchSetLanguage({ code: 0x58, x: 0, y: 4 }, undefined), false);
    assert.deepEqual(requests, [0xffffffff, 4]);
    // 알림은 이미 원문 경로로 전달됐으면 중복 없이 소비한다.
    const routed = [];
    assert.equal(dispatchLanguageChanged({ code: LANGUAGE_CHANGED_COMMAND, x: 0, y: 0 }, true, undefined), true);
    assert.equal(dispatchLanguageChanged({ code: (LANGUAGE_CHANGED_COMMAND | InputCode.release) >>> 0, x: 1, y: 2 }, false, (event) => routed.push(event.x)), true);
    assert.deepEqual(routed, [1]);
    assert.equal(dispatchLanguageChanged({ code: 0x58, x: 0, y: 0 }, false, undefined), false);
});

// 현재 판본의 독립 native fixture 를 전체 재생한다. 기대 분기는 계산하지 않는다.
test('WindowTitle: 원본10.78 576행(편집/요새/미션 우선순위·readonly·NULL/빈 인자·설정 조회 순서)', { skip: !titleRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of titleRows) {
        // 원본 10.78 PE 행만 쓴다. CD 10.72(originalCD)는 범위 밖이다.
        if (row[0] !== 'originals') continue;
        assert.equal(row.length, 11);
        const state = {
            fortName: row[1] === '#NULL' ? null : row[1],
            editor: Number(row[2]) !== 0, mission: Number(row[3]) !== 0,
            templatePresent: Number(row[4]) !== 0, templateReadOnly: Number(row[5]) !== 0,
            demo: Number(row[6]) !== 0, testBattle: Number(row[7]) !== 0,
        };
        const trace = [];
        let result = '';
        refreshWindowTitle(state, {
            pathSpec: (key, arg) => {
                const value = arg === null ? '<null>' : arg;
                trace.push(`${key}=${value}`);
                return `<${key}:${value}>`;
            },
            missionTitle: () => { trace.push('title'); return row[8]; },
            setTitle: (title) => { result = title; },
        });
        assert.equal(trace.join('|'), row[9], `행 ${count + 1} 조회 순서`);
        assert.equal(result, row[10], `행 ${count + 1} 최종 제목`);
        count += 1;
    }
    assert.equal(count, 576);
});

// 소비자 누락과 256바이트/NUL 범위 초과를 거부한다.
test('WindowTitle: 소비자 누락·길이 초과·NUL 거부', () => {
    const state = { fortName: 'fort', editor: false, mission: false, templatePresent: false, templateReadOnly: false, demo: false, testBattle: false };
    assert.throws(() => refreshWindowTitle(state, {}), TypeError);
    const make = (title) => ({ pathSpec: () => title, missionTitle: () => '', setTitle: () => assert.fail('적용되면 안 됨') });
    assert.throws(() => refreshWindowTitle(state, make('x'.repeat(256))), RangeError);
    assert.throws(() => refreshWindowTitle(state, make('a\0b')), RangeError);
    let applied = '';
    refreshWindowTitle(state, { pathSpec: () => 'x'.repeat(255), missionTitle: () => '', setTitle: (title) => { applied = title; } });
    assert.equal(applied.length, 255);
});

// 선행 모달은 실제 Uber 부모와 같은 호출 거부로 구별한다.
test('GameTimeDisplay: T/t 키 원본10.78 168행(뗌·정지·수정키·DWORD 토글)', { skip: !keyRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of keyRows) {
        assert.equal(row.length, 7);
        const holder = { value: number(row[3]) };
        const modal = number(row[2]) !== 0;
        let trace = '';
        if (!modal) dispatchGameTimeKey({ code: number(row[0]), x: 0, y: 0 }, holder, () => { trace = `P:${row[1]}`; return number(row[1]) !== 0; });
        assert.equal(holder.value, number(row[4]), `행 ${count + 1} 표시 값`);
        assert.equal(trace === '' ? '-' : trace, row[5], `행 ${count + 1} 정지 조회`);
        count += 1;
    }
    assert.equal(count, 168);
});

// clock 콜백 뒤 현재 원점을 읽고 예외 전후의 부분 쓰기를 원문 위치에 유지한다.
test('GameTimeDisplay: 현재 원점 재조회·signed 차이 절삭·선택된 실패', () => {
    const stamp = { value: 9 };
    let origin = 0;
    assert.equal(readGameTimeSeconds(stamp, () => origin, () => { origin = 2000; return 3001; }), 1);
    assert.equal(stamp.value, 3001);
    assert.throws(() => readGameTimeSeconds(stamp, () => origin, () => { throw new Error('clock stop'); }), /clock stop/);
    assert.deepEqual([stamp.value, origin], [3001, 2000]);
    // 32비트 감김: 원점보다 작은 now 는 음수 차이가 0 쪽으로 절삭된다(-0 이 아닌 0).
    assert.equal(readGameTimeSeconds(stamp, () => 1500, () => 1000), 0);
    assert.equal(readGameTimeSeconds(stamp, () => 5000, () => 1000), -4);
    assert.equal(readGameTimeSeconds(stamp, () => 0xfffffc18, () => 2000), 3);
    assert.throws(() => readGameTimeSeconds(stamp, () => 0, undefined), TypeError);
    // T/t 만 소비하며 정지 조회가 없으면 거부한다.
    assert.equal(dispatchGameTimeKey({ code: 0x58, x: 0, y: 0 }, { value: 0 }, undefined), false);
    assert.equal(dispatchGameTimeKey({ code: (0x54 | InputCode.release) >>> 0, x: 0, y: 0 }, { value: 0 }, undefined), false);
    assert.throws(() => dispatchGameTimeKey({ code: 0x74, x: 0, y: 0 }, { value: 0 }, undefined), TypeError);
});
