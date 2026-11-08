// 주변음 raw-ambient-sound.js(stepAmbientSound·resetAmbientSoundDeadline)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: ambient-sound-hardware-x86.tsv 10.78(originals) 636행 — 초기 기한·종류·강제 값·모달·난수 시드·벽시각 증가·반복 수·재예약/바람 실행 모드·동작(모달/강제/기한 변이) → 최종 기한 비트·종류·강제 값·모달·난수 상태·시계 호출 수·호출 기록(W 시계/S 음향/F 섬광),
//    결합된 공유 바람(X·Y·각도 float 비트). 바람은 기존 `raw-ambient-wind.js` 가 소비자 순서(소리 부모 바로 뒤)대로 같은 난수를 소비한다. CD 10.72 행은 웹 범위(10.78) 밖이다.
//  - 계약: 기한 미래/비순서에서는 첫 시계 조회만 하고 난수를 소비하지 않으며, 재예약은 난수보다 먼저 저장되고 기한 공유(State/Edit 와 같은 속성) 뒤 현재 시계로 읽는다. 선택하지 않은 장치는 요구하지 않는다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E77: 주변음 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { GameRandom } from '../www/core/random.js';
import { createAmbientWindState, stepAmbientWind } from '../www/core/raw-ambient-wind.js';
import { stepAmbientSound, resetAmbientSoundDeadline } from '../www/core/raw-ambient-sound.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E77)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'ambient-sound-hardware-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 판본당 독립 행 수이다. (2026-11-08 WB04-E77)
const ROWS = 636;

// 예외가 나는지 본다. (2026-11-08 WB04-E77)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 64비트/32비트 부동소수 패턴을 읽고 쓰는 공용 DataView 이다(NaN payload 보존). (2026-11-08 WB04-E77)
const view = new DataView(new ArrayBuffer(8));
const bitsToDouble = (text) => { view.setBigUint64(0, BigInt(text), true); return view.getFloat64(0, true); };
const doubleBits = (value) => { view.setFloat64(0, value, true); return view.getBigUint64(0, true).toString(); };
const floatBits = (value) => { view.setFloat32(0, value, true); return String(view.getUint32(0, true)); };

// 한 행을 재생해 실제 Client 순서(소리 부모 → 같은 난수의 바람)로 실행한 최종 상태를 직렬화한다. 기대값은 계산하지 않는다. (2026-11-08 WB04-E77)
function replay(row) {
    let deadline = bitsToDouble(row[3]);
    let kind = Number(row[4]);
    let forced = Number(row[5]);
    let dialog = Number(row[6]);
    let wall = bitsToDouble(row[7]);
    const increment = bitsToDouble(row[8]);
    const steps = Number(row[9]);
    const mode = Number(row[10]);
    const action = Number(row[11]);
    const random = new GameRandom(Number(row[2]));
    const wind = createAmbientWindState();
    let calls = 0;
    let trace = '';
    // 현재 값을 읽고 쓰는 전역 뷰이다.
    const globals = {
        get deadline() { return deadline; },
        set deadline(value) { deadline = value; },
        get kind() { return kind; },
        set kind(value) { kind = value; },
        get forced() { return forced; },
        set forced(value) { forced = value; }
    };
    const wallSeconds = () => {
        const now = wall;
        wall += increment;
        calls += 1;
        trace += `W,${doubleBits(now)};`;
        if ((action & 4) && calls === 2) { forced = 1; kind = 2; }
        return now;
    };
    const hooks = {
        wallSeconds,
        sound: (name, loop, volume, pan, priority, limit) => {
            trace += `S,${name},${loop},${volume},${pan},${priority},${limit};`;
            if (action & 1) dialog = 0;
            if (action & 2) dialog = 0x80000000;
            if (action & 8) deadline = -4.25;
        },
        currentDialog: () => dialog,
        flash: (brightness) => { trace += `F,${brightness};`; }
    };
    if (mode & 1) resetAmbientSoundDeadline(globals, wallSeconds);
    // 원문 Client 처럼 소리 부모 바로 뒤에서 같은 난수를 소비하는 바람을 실행한다.
    for (let step = 0; step < steps; step += 1) {
        stepAmbientSound(globals, random, hooks);
        if (mode & 2) stepAmbientWind(wind, random);
    }
    return [doubleBits(deadline), String(kind), String(forced), String(dialog), String(random.state), String(calls), trace === '' ? '-' : trace, floatBits(wind.x), floatBits(wind.y), floatBits(wind.angle)];
}

test('AmbientSound: 원본 10.78 636입력의 시간·난수·종류·강제 값·모달·요청 순서와 결합 바람이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 첫 불일치의 입력 번호와 필드를 알려 주며 전체를 대조한다.
    for (const row of rows) {
        assert.equal(row.length, 22);
        assert.deepEqual(replay(row), row.slice(12), `행 ${row[1]}: 입력 ${row.slice(2, 12).join(' ')}`);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('AmbientSound: 공유 기한과 현재 시계 — 미래 기한은 첫 조회만 하고 재예약은 난수보다 먼저 저장하며 강제 값은 종류를 유지한다', () => {
    const globals = { deadline: 110.25, kind: 0, forced: 0 };
    const random = new GameRandom(55);
    let calls = 0;
    const hooks = { wallSeconds: () => { calls += 1; return 110.0; } };
    stepAmbientSound(globals, random, hooks);
    assert.ok(calls === 1 && random.state === 55 && globals.deadline === 110.25);
    // 두 번째 조회로 기한을 지난 시각을 읽으면 120 초 뒤로 재예약하고 강제 값(1)이 양수라 종류와 난수를 보존한다(종류 3 은 아무것도 재생하지 않음).
    hooks.wallSeconds = () => { calls += 1; return calls === 2 ? 110.25 : 110.5; };
    globals.forced = 1;
    globals.kind = 3;
    stepAmbientSound(globals, random, hooks);
    assert.ok(calls === 3 && globals.deadline === 230.5 && random.state === 55 && globals.kind === 3);
    // 시계가 NaN 이면 기한 비교가 거짓이라 첫 조회만 하고 돌아간다.
    hooks.wallSeconds = () => { calls += 1; return Number.NaN; };
    stepAmbientSound(globals, random, hooks);
    assert.ok(calls === 4 && globals.deadline === 230.5);
    resetAmbientSoundDeadline(globals, () => 100.25);
    assert.equal(globals.deadline, 110.25);
});

test('AmbientSound: 선택하지 않은 장치는 요구하지 않고 도달한 장치의 누락은 선행 효과를 남긴 채 거부한다', () => {
    const globals = { deadline: 0, kind: 5, forced: 0 };
    const random = new GameRandom(55);
    assert.ok(throws(() => stepAmbientSound(globals, random, {})));
    assert.ok(throws(() => resetAmbientSoundDeadline(globals, undefined)));
    // 미래 기한은 시계만 요구한다.
    globals.deadline = 1e300;
    stepAmbientSound(globals, random, { wallSeconds: () => 1 });
    // 범위 밖 종류(강제 값이 양수)는 소리/모달 장치 없이 기한만 재예약한다.
    globals.deadline = 0;
    globals.forced = 1;
    stepAmbientSound(globals, random, { wallSeconds: () => 5 });
    assert.ok(globals.deadline === 125 && globals.kind === 5 && random.state === 55);
    // 천둥은 소리 요청을 먼저 하고 그 뒤 모달 장치가 없으면 거부한다(소리 효과는 남음).
    globals.deadline = 0;
    globals.kind = 0;
    const played = [];
    assert.ok(throws(() => stepAmbientSound(globals, random, { wallSeconds: () => 5, sound: (name) => played.push(name) })));
    assert.deepEqual(played, ['thunderQuietDistant.wav']);
});
