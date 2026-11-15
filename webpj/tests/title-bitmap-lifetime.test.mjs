// 시작 title 그림 수명 title-bitmap-lifetime.js(runTitleBitmaps·redrawTitleBitmap·waitTitleBitmap·requestInitialTitleBitmap)를 원본 10.78 관측 33행과 cpppj `TitleBitmapLifetimeTests` 계약으로 대조한다(WB04-E533).
//  - 원본 관측 title-bitmap-lifetime-x86.tsv 33행(모드 0 실행·1 재그리기·2 요청 × 치수·건너뜀·실패 지점·현재 번호·기존 슬롯): 종료 종류·현재 번호·paint 깊이·게임 시각·8칸 슬롯·장치 호출 전체 순서
//    (P pump·I 입력·N 파일명·G 적재·A 팔레트·L/U lock·B 복사·V present·F flip·W 시각·R 해제·Z 채움·C clip·FL/FU/FB 앞면)와 요청 뒤 144BYTE next 블록 전체.
//  - 계약(`State42Requests41AndRestoresPaletteAfterSplash`): State 42 는 `HTRPS` 순서이고 스플래시가 파일 오류/WM_QUIT 로 중단되면 앞서 숨긴 커서/요청을 되돌리지 않으며 TitleBitmapQuit 는 State 41 성공으로 바꾸지 않고 전달된다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E533)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { LocalMissionParentResult } from '../www/core/raw-local-mission-state.js';
import { installNativeIntroState } from '../www/core/native-intro-state.js';
import { TitleBitmapQuit, redrawTitleBitmap, requestInitialTitleBitmap, runTitleBitmaps, waitTitleBitmap } from '../www/core/title-bitmap-lifetime.js';

// 원본 관측 33행이다. 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E533)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'title-bitmap-lifetime-x86.tsv'), { keepHeader: false });
// 독립 CPU 의 8칸 bitmap 불투명 토큰 시작이다. (2026-11-15 WB04-E533)
const BASE = 0x12003000;
// 원본 관측의 이분 정확 표기(소수점이 없으면 ".0")이다. (2026-11-15 WB04-E533)
const decimal = (value) => { const text = String(value); return text.includes('.') ? text : `${text}.0`; };
// 현재 DWORD 표를 쉼표로 이은 관측 형식이다. (2026-11-15 WB04-E533)
const words = (values) => Array.from(values, (value) => value >>> 0).join(',');

// 한 행을 새 상태에서 재생한다(cpppj 시험과 같은 장치 배선). (2026-11-15 WB04-E533)
function replay(row) {
    const p = row[0].split(',').map(Number);
    const [mode, , , skip, fail, , seed] = p;
    const g = { bitmaps: new Uint32Array(8), current: p[5] | 0, paintDepth: 3, width: p[1] | 0, height: p[2] | 0, gameSeconds: 7.25 };
    const next = new Uint32Array(36).fill(0xa5a5a5a5);
    for (let index = 0; index < 8; index += 1) g.bitmaps[index] = seed || mode === 1 ? (BASE + index * 4) >>> 0 : 0;
    let clock = 0;
    let pops = 0;
    let outcome = 'ok';
    let events = '';
    const event = (text) => { events = events ? `${events};${text}` : text; };
    const hooks = {
        pump: () => event('P'),
        input: () => {
            let code = 0;
            if (skip) { pops += 1; if (pops === skip) code = 65; } else pops += 1;
            event(`I:${code}`);
            return code;
        },
        game: () => { event('Gtime:123.5'); return 123.5; },
        wall: () => { const value = clock; event(`W:${decimal(value)}`); clock += 1.25; return value; },
        load: (name, size) => {
            event(`N:${name}`);
            const index = name === 'title0.gif' ? 0 : name === 'title6.gif' ? 6 : 7;
            event(`G:${index}`);
            if (fail === index + 1) throw new Error(`load${index}`);
            // 6/7 은 서로 다른 치수여서 같은 마지막 출력쌍을 쓰는 원문을 확인한다.
            size.width = index === 0 ? 100 : index === 6 ? 120 : 111;
            size.height = index === 0 ? 80 : index === 6 ? 90 : 81;
            return (BASE + index * 4) >>> 0;
        },
        palette: (name) => { event(`N:${name}`); event(`A:${name}`); if (fail === 20) throw new Error('palette'); },
        clip: (r) => event(`C:${words([r.left, r.top, r.right, r.bottom])}`),
        lock: () => event('L'),
        unlock: () => event('U'),
        present: () => event('V'),
        flip: () => event('F'),
        frontLock: () => event('FL'),
        frontUnlock: () => event('FU'),
        copy: (c) => event(`B:${words([c.x, c.y, c.bitmap, c.stride, c.source.left, c.source.top, c.source.right, c.source.bottom])}`),
        frontCopy: (c) => event(`FB:${words([c.x, c.y, c.bitmap, c.stride, c.source.left, c.source.top, c.source.right, c.source.bottom])}`),
        fill: (r, color) => event(`Z:${words([r.left, r.top, r.right, r.bottom, color])}`),
        release: (token) => event(`R:${token >>> 0}`),
    };
    try {
        if (mode === 0) runTitleBitmaps(g, hooks);
        else if (mode === 1) redrawTitleBitmap(g, hooks);
        else requestInitialTitleBitmap(next);
    } catch (failure) {
        if (!(failure instanceof Error)) throw failure;
        outcome = failure.message;
    }
    return { outcome, current: g.current >>> 0, depth: g.paintDepth >>> 0, game: g.gameSeconds, slots: words(g.bitmaps), events: events === '' ? '-' : events, next: Buffer.from(next.buffer).toString('hex') };
}

test('시작 title 그림: 원본 10.78 33입력의 장치 열·8칸·현재 번호·깊이·게임 시각·요청 블록이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const data = rows.filter((row) => row.length === 8);
    assert.equal(data.length, 33);
    let count = 0;
    for (const row of data) {
        const actual = replay(row);
        const label = `행 ${count}: ${row[0]}`;
        assert.equal(actual.outcome, row[1], label);
        assert.equal(actual.current, Number(row[2]) >>> 0, label);
        assert.equal(actual.depth, Number(row[3]), label);
        assert.equal(actual.game, Number(row[4]), label);
        assert.equal(actual.slots, row[5], label);
        assert.equal(actual.events, row[6], label);
        assert.equal(actual.next, row[7], label);
        count += 1;
    }
    assert.equal(count, 33);
});

test('시작 title 그림 계약: State 42 는 HTRPS 이고 스플래시 중단은 앞선 효과를 되돌리지 않으며 WM_QUIT 은 그대로 전달된다', () => {
    const blocks = Array.from({ length: 5 }, () => new Uint32Array(36));
    const state = { old: blocks[0], phase: blocks[1], immediate: blocks[2], current: blocks[3], next: blocks[4] };
    state.current[0] = 42;
    state.immediate[35] = 0x12345678;
    const modes = { fort: false, server: false, boss: false };
    const box = (name) => ({ get value() { return modes[name]; }, set value(next) { modes[name] = Boolean(next); } });
    const pending = Uint8Array.from([75, 101, 101, 112]);
    let trace = '';
    const hooks = {
        titleCursor: (show) => { trace += show ? 'S' : 'H'; },
        titleSplash: () => { trace += 'T'; assert.equal(state.immediate[35], 0x12345678); },
        request: (selected) => { trace += 'R'; state.immediate[0] = selected; },
        restoreTitlePalette: () => { trace += 'P'; assert.ok(state.immediate[0] === 41 && state.immediate[35] === 0); },
    };
    const install = () => installNativeIntroState({ state, fort: box('fort'), server: box('server'), boss: box('boss'), pendingName: pending }, hooks);
    assert.equal(install(), LocalMissionParentResult.Commit);
    assert.equal(trace, 'HTRPS');
    assert.ok(!modes.fort && !modes.server && !modes.boss && pending[0] === 75);
    // 스플래시가 파일 오류로 중단되면 앞선 숨김만 남고 요청/팔레트/보임은 일어나지 않는다.
    state.immediate[0] = 0;
    state.immediate[35] = 0xa5a5a5a5;
    trace = '';
    hooks.titleSplash = () => { trace += 'T'; throw new Error('명시title파일중단'); };
    assert.throws(install, /명시title파일중단/);
    assert.equal(trace, 'HT');
    assert.ok(!state.immediate[0] && state.immediate[35] === 0xa5a5a5a5);
    // WM_QUIT 도 State 41 성공으로 만들지 않고 Run 전용 종료 경계까지 전달한다.
    trace = '';
    hooks.titleSplash = () => { trace += 'T'; throw new TitleBitmapQuit(); };
    assert.throws(install, TitleBitmapQuit);
    assert.equal(trace, 'HT');
    assert.ok(!state.immediate[0] && state.immediate[35] === 0xa5a5a5a5);
});

test('시작 title 그림 계약: Wait 는 기한 도달에서 pump/input 을 더 부르지 않고 재그리기는 범위 밖 현재 번호를 거부한다', () => {
    let clock = 0;
    const calls = [];
    const hooks = { wall: () => { const value = clock; clock += 1; calls.push('W'); return value; }, pump: () => calls.push('P'), input: () => { calls.push('I'); return 0; } };
    // 기한 = 2×0.001... 여기서는 1000ms → 1초. 시계가 호출마다 1초 오르므로 첫 확인에서 기한 도달이다.
    assert.equal(waitTitleBitmap(1000, hooks), false);
    assert.deepEqual(calls, ['W', 'W']);
    const bad = { bitmaps: new Uint32Array(8), current: 9, paintDepth: 0, width: 640, height: 480, gameSeconds: 0 };
    assert.throws(() => redrawTitleBitmap(bad, { frontLock: () => {}, frontUnlock: () => {} }), RangeError);
    const next = new Uint32Array(36).fill(7);
    requestInitialTitleBitmap(next);
    assert.deepEqual([next[0], next[1], next[34], next[35]], [42, 7, 7, 0]);
});
