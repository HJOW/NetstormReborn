// 시간 알림 raw-monitor-time.js(stepMonitorTime·formatMonitorTimeLine·monitorCalendarText·submitTemporaryChatFormat)를 원본 관측과 cpppj `MonitorTimeTests` 계약으로 대조한다(WB04-E526).
//  - 원본 관측 monitor-time-hardware-x86.tsv 1176행: guard/기한/옵션 두 DWORD/벽시각 증분/단계 수/변경 입력에서 최종 initialized·deadline 비트·monitor·alert·벽시각 호출 수와
//    요청 순서(W 벽시각·T/L/A 달력·P 서식 줄·Q 설정 서식·E 제출)를 대조한다. originals 와 originalCD 행을 같은 단계 로직으로 모두 재생한다(CD 의 무효과 atexit 등록 열은 CD 전용이라 대조하지 않는다).
//  - 원본 관측 temporary-chat-format-hardware-x86.tsv 12행: 판본별 60/16BYTE 절삭·패딩·제출 뒤 현재 manager 재조회와 두 manager 전체 256BYTE.
//  - 계약: submit 예외에서는 앞선 임시 서식을 남기고, 달력은 25글자·끝 개행·32비트 범위, 줄은 256BYTE 미만, 미도달 장치는 요구하지 않고 도달한 장치는 명시 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E526)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { MonitorTimeState, formatMonitorTimeLine, monitorCalendarText, stepMonitorTime, submitTemporaryChatFormat } from '../www/core/raw-monitor-time.js';

// 원본 관측 두 파일이다. 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E526)
const timerRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'monitor-time-hardware-x86.tsv'));
const temporaryRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'temporary-chat-format-hardware-x86.tsv'));

// 64비트 double 비트(BigInt)와 숫자 사이의 변환이다. (2026-11-15 WB04-E526)
const view = new DataView(new ArrayBuffer(8));
const bitsToDouble = (bits) => { view.setBigUint64(0, BigInt(bits), true); return view.getFloat64(0, true); };
const doubleToBits = (value) => { view.setFloat64(0, value, true); return view.getBigUint64(0, true).toString(); };
// 바이트(문자 하나 = 원본 BYTE) 문자열을 소문자 16진으로 바꾸고 바이트 배열을 16진으로 바꾼다. (2026-11-15 WB04-E526)
const textHex = (text) => Buffer.from(text, 'latin1').toString('hex');
const bytesHex = (bytes) => Buffer.from(bytes).toString('hex');

// Native 와 같은 명시 외부 시계/달력/Config/제출 입력만 조립해 한 행을 재생한다. (2026-11-15 WB04-E526)
function replayTimer(row) {
    const state = new MonitorTimeState();
    state.initialized = Number(row[2]);
    state.deadline = bitsToDouble(row[3]);
    state.monitor = Number(row[4]);
    state.alert = Number(row[5]);
    let wall = bitsToDouble(row[6]);
    const increment = bitsToDouble(row[7]);
    const steps = Number(row[8]);
    const changes = Number(row[9]);
    let calls = 0;
    let trace = '';
    let name = 'Alpha';
    const hooks = {
        wallSeconds: () => {
            const now = wall;
            wall += increment;
            calls += 1;
            trace += `W,${doubleToBits(now)};`;
            if ((changes & 1) && calls === 1) state.deadline = 10000;
            return now;
        },
        calendar: () => {
            trace += 'T;L;A;';
            if (changes & 2) name = 'Bravo';
            return 'Wed Nov  4 01:00:00 2026\n';
        },
        handle: () => name,
        line: (handle, calendar) => {
            trace += `P,${textHex('%s: !%s\n')},${textHex(handle)},${textHex(calendar)};`;
            return formatMonitorTimeLine(handle, calendar);
        },
        format: () => {
            trace += `Q,${textHex('monitorFormat')},0,0,0;`;
            if (changes & 4) name = 'Bravo';
            return '~9~0';
        },
        submit: (format, line) => {
            trace += `E,${textHex(format)},${textHex(line)};`;
            if (changes & 8) {
                state.monitor = 0;
                state.alert = 0;
            }
            if (changes & 16) state.deadline = -4.25;
        },
    };
    // 행 안에서는 같은 현재 상태를 재시드하지 않는다.
    for (let step = 0; step < steps; step += 1) stepMonitorTime(state, hooks);
    return [String(state.initialized), doubleToBits(state.deadline), String(state.monitor), String(state.alert), String(calls), trace === '' ? '-' : trace];
}

test('시간 알림: 원본 1176행의 guard·기한·옵션·시각·본문·서식·요청 순서가 같다', { skip: !timerRows && '원본 fixture 없음' }, () => {
    assert.equal(timerRows.length, 1176);
    let count = 0;
    // 첫 불일치의 전체 필드를 남기며 모든 행을 비교한다.
    for (const row of timerRows) {
        assert.equal(row.length, 17);
        assert.deepEqual(replayTimer(row), [row[10], row[11], row[12], row[13], row[14], row[16]], `${row[0]} 행 ${row[1]}`);
        count += 1;
    }
    assert.equal(count, 1176);
});

// 실제 임시서식 부모의 두 manager 전체 256BYTE 를 비교하는 장면이다. (2026-11-15 WB04-E526)
class TemporaryScene {
    // patch 는 10.78, action 의 bit0 은 제출 중 현재 manager 가 바뀌는 변경 입력이다.
    constructor(patch, action) {
        this.current = { value: 0x601000 };
        this.changes = action;
        this.patch = patch;
        this.trace = '';
        this.memory = new Map([[0x601000, new Uint8Array(256).fill(0xab)], [0x602000, new Uint8Array(256).fill(0xcd)]]);
        const offset = patch ? 0x14 : 0x10;
        const put = (token, text) => {
            const bytes = this.memory.get(token);
            for (let index = 0; index < text.length; index += 1) bytes[offset + index] = text.charCodeAt(index);
            bytes[offset + text.length] = 0;
        };
        put(0x601000, 'saved-prefix');
        put(0x602000, 'second-prefix');
        this.hooks = {
            memory: (token) => {
                const bytes = this.memory.get(token);
                if (!bytes) throw new RangeError('임시 Chat manager 토큰 없음');
                return bytes;
            },
            submit: (manager, line) => {
                const bytes = this.memory.get(manager);
                const end = bytes.indexOf(0, offset);
                const text = Array.from(bytes.subarray(offset, end), (byte) => String.fromCharCode(byte)).join('');
                this.trace += `S,${textHex(text)},${textHex(line)};`;
                if (this.changes & 1) this.current.value = 0x602000;
            },
        };
    }
}

test('시간 알림 임시 서식: 원본 12행의 판본별 60/16BYTE 절삭·패딩과 현재 manager 재조회·전체 256BYTE 가 같다', { skip: !temporaryRows && '원본 fixture 없음' }, () => {
    assert.equal(temporaryRows.length, 12);
    let count = 0;
    // 두 실제 원문 wrapper 의 같은 입력을 독립 실행한다.
    for (const row of temporaryRows) {
        assert.equal(row.length, 8);
        const scene = new TemporaryScene(row[0] === 'originals', Number(row[3]));
        const pattern = Number(row[2]);
        const format = pattern === 0 ? '~9~0' : pattern === 1 ? 'Z'.repeat(75) : '';
        submitTemporaryChatFormat(scene.current, format, 'Alpha: !calendar\n\n', scene.patch ? 'patch1078' : 'cd1072', scene.hooks);
        const label = `${row[0]} 행 ${count}`;
        assert.equal(String(scene.current.value === 0x602000 ? 1 : 0), row[4], label);
        assert.equal(bytesHex(scene.memory.get(0x601000)), row[5], label);
        assert.equal(bytesHex(scene.memory.get(0x602000)), row[6], label);
        assert.equal(scene.trace, row[7], label);
        count += 1;
    }
    assert.equal(count, 12);
});

test('시간 알림 임시 서식: 정상 submit 복귀에서만 복원하며 실패하면 앞선 임시 서식 쓰기를 남긴다', () => {
    const scene = new TemporaryScene(true, 0);
    scene.hooks.submit = () => { throw new Error('명시 제출 장치 실패'); };
    assert.throws(() => submitTemporaryChatFormat(scene.current, 'Z'.repeat(75), 'x', 'patch1078', scene.hooks), /명시 제출 장치 실패/);
    // 60BYTE 로 절삭된 임시 서식과 마지막 BYTE 0 이 manager 에 남는다(원본 관측 행 `originals 1 1` 의 manager 와 같은 상태).
    const row = temporaryRows?.find((item) => item[0] === 'originals' && item[2] === '1' && item[3] === '1');
    const bytes = scene.memory.get(0x601000);
    assert.equal(bytes[0x14], 0x5a);
    assert.equal(bytes[0x14 + 59], 0);
    if (row) assert.equal(bytesHex(bytes), row[5]);
    // 물리가 부족하거나 front 가 지역 80BYTE 를 넘으면 명시 거부한다.
    assert.throws(() => submitTemporaryChatFormat({ value: 1 }, 'a', 'b', 'patch1078', { memory: () => new Uint8Array(8), submit: () => {} }), RangeError);
    const long = new Uint8Array(256).fill(0x41);
    assert.throws(() => submitTemporaryChatFormat({ value: 1 }, 'a', 'b', 'patch1078', { memory: () => long, submit: () => {} }), RangeError);
    // 미도달 장치는 요구하지 않고 도달한 장치는 명시 거부한다.
    assert.throws(() => submitTemporaryChatFormat({ value: 1 }, 'a', 'b', 'patch1078', {}), /실제 장치 미연결/);
});

test('시간 알림 계약: 줄은 NUL 앞만 읽고 256BYTE 미만이며 달력은 25글자 표준 asctime 이고 32비트 범위를 지킨다', () => {
    assert.equal(formatMonitorTimeLine('Alpha\0hidden', 'Wed Nov  4 01:00:00 2026\n\0x'), 'Alpha: !Wed Nov  4 01:00:00 2026\n\n');
    assert.throws(() => formatMonitorTimeLine('A'.repeat(240), 'B'.repeat(20)), RangeError);
    const calendar = monitorCalendarText(1_700_000_000);
    assert.equal(calendar.length, 25);
    assert.match(calendar, /^(Sun|Mon|Tue|Wed|Thu|Fri|Sat) (Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec) [ 0-3]\d \d\d:\d\d:\d\d \d{4}\n$/);
    assert.throws(() => monitorCalendarText(-1), RangeError);
    assert.throws(() => monitorCalendarText(0x80000000), RangeError);
    // 장치는 도달한 위치에서만 요구한다: 두 옵션이 모두 0 이면 어떤 장치도 요구하지 않는다.
    const state = new MonitorTimeState();
    stepMonitorTime(state, {});
    assert.equal(state.initialized, 1);
    state.monitor = 1;
    assert.throws(() => stepMonitorTime(state, {}), /실제 장치 미연결/);
});
