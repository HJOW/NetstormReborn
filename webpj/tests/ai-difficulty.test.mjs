// 11차 난이도 계산 ai-difficulty.js(aiDifficultyFromValue·applyAiDifficulty·sanitizeAiForcedEnemy)를 cpppj `AiDifficultyTests.cpp` 의 다섯 시험으로 대조한다(WB04-E554).
//  - 난이도 값 해석(1 쉬움·3 어려움·그 밖 보통)과 왕복, NB10 강제 적 정리, 보통의 비트 보존, 쉬움의 주기 4배·채취 절반(내림·최소 1·0 유지), 어려움의 주기 절반과 크기 거부.
//  - 원본에 난이도가 없어 원본 관측 fixture 는 없다. 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E554)
import test from 'node:test';
import assert from 'node:assert/strict';
import { RawAiCreate } from '../www/core/raw-ai-brain.js';
import { AiDifficulty, DIFFICULTY_CONFIG_KEY, EASY_CYCLE_FACTOR, HARD_CYCLE_FACTOR, aiDifficultyFromValue, aiDifficultyToValue, applyAiDifficulty, sanitizeAiForcedEnemy } from '../www/core/ai-difficulty.js';

// 지정 위치의 단정도/정수 값을 읽고 쓴다(비정렬 little endian). (2026-11-15 WB04-E554)
const view = (bytes) => new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
const putFloat = (bytes, offset, value) => view(bytes).setFloat32(offset, value, true);
const putInt = (bytes, offset, value) => view(bytes).setInt32(offset, value, true);
const getFloat = (bytes, offset) => view(bytes).getFloat32(offset, true);
const getInt = (bytes, offset) => view(bytes).getInt32(offset, true);

// 원본 AI 기본 설정과 비슷한 600바이트(다리 0, 갱신 1.0, 이동 2.0, 지질 3, 채취 6)이며 다른 바이트는 구별 가능한 무늬로 채운다. (2026-11-15 WB04-E554)
function sampleSettings() {
    const bytes = new Uint8Array(600);
    // 모든 칸을 위치 기반 무늬로 채워 배율이 다른 필드를 건드리는지 비교할 수 있게 한다.
    for (let index = 0; index < bytes.length; index += 1) bytes[index] = (index * 7 + 1) & 0xff;
    putFloat(bytes, 0x220, 0);
    putFloat(bytes, 0x224, 1);
    putFloat(bytes, 0x228, 2);
    putInt(bytes, 0x230, 3);
    putInt(bytes, 0x234, 6);
    return bytes;
}

test('AiDifficulty: 저장값 해석(1 쉬움·3 어려움·그 밖 보통)과 왕복·상수', () => {
    // 값 없음/보통/범위 밖은 보통이다.
    assert.deepEqual([0, 2, 99, 4294967295, -1].map(aiDifficultyFromValue), new Array(5).fill(AiDifficulty.Normal));
    assert.deepEqual([aiDifficultyFromValue(1), aiDifficultyFromValue(3)], [AiDifficulty.Easy, AiDifficulty.Hard]);
    for (const level of [AiDifficulty.Easy, AiDifficulty.Normal, AiDifficulty.Hard]) assert.equal(aiDifficultyFromValue(aiDifficultyToValue(level)), level);
    assert.deepEqual([DIFFICULTY_CONFIG_KEY, EASY_CYCLE_FACTOR, HARD_CYCLE_FACTOR, AiDifficulty.Easy, AiDifficulty.Normal, AiDifficulty.Hard], ['difficulty', 4, 0.5, 1, 2, 3]);
});

test('sanitizeAiForcedEnemy: 소유자 범위(1..8) 밖이거나 자기 자신이면 0 으로 정리한다', () => {
    const settings = sampleSettings();
    putInt(settings, 0x240, 0);
    // 0(자동 선택)은 그대로다.
    assert.equal(sanitizeAiForcedEnemy(settings, 2), null);
    putInt(settings, 0x240, 3);
    // 자기 자신(3)은 정리하고 다른 소유자(2)에게는 그대로다.
    assert.equal(sanitizeAiForcedEnemy(settings, 2), null);
    assert.equal(getInt(sanitizeAiForcedEnemy(settings, 3), 0x240), 0);
    // 소유자 범위(1..8) 밖이다(음수는 부호 없는 큰 값).
    for (const bad of [9, 100, -1, 0x7fffffff]) {
        putInt(settings, 0x240, bad);
        assert.equal(getInt(sanitizeAiForcedEnemy(settings, 2), 0x240), 0);
    }
    // 경계 8 은 정상이며 자기 자신이면 정리한다. 다른 바이트는 그대로다.
    putInt(settings, 0x240, 8);
    assert.equal(sanitizeAiForcedEnemy(settings, 2), null);
    const cleaned = sanitizeAiForcedEnemy(settings, 8);
    for (let index = 0; index < settings.length; index += 1) if (index < 0x240 || index >= 0x244) assert.equal(cleaned[index], settings[index], `바이트 ${index}`);
    // 입력은 바꾸지 않고 600바이트가 아니면 거부한다.
    assert.equal(getInt(settings, 0x240), 8);
    assert.throws(() => sanitizeAiForcedEnemy(settings.subarray(0, 100), 2), RangeError);
});

test('applyAiDifficulty: 보통은 한 비트도 바꾸지 않는다(사본)', () => {
    const settings = sampleSettings();
    const normal = applyAiDifficulty(settings, AiDifficulty.Normal);
    assert.deepEqual(normal, settings);
    assert.notEqual(normal, settings);
    normal[0] ^= 0xff;
    assert.notEqual(normal[0], settings[0]);
});

test('applyAiDifficulty: 쉬움은 세 주기 4배·채취 절반(내림·최소 1)이고 0 이하 주기와 0 채취는 그대로다', () => {
    const settings = sampleSettings();
    const easy = applyAiDifficulty(settings, AiDifficulty.Easy);
    // 다리 주기 0 은 배율로 바뀌지 않고 나머지 두 주기는 4배이다.
    assert.deepEqual([getFloat(easy, 0x220), getFloat(easy, 0x224), getFloat(easy, 0x228)], [0, 4, 8]);
    // 채취 6→3 이고 지질 연결 수는 그대로이다.
    assert.deepEqual([getInt(easy, 0x234), getInt(easy, 0x230)], [3, 3]);
    // 주기·채취 필드만 달라야 한다.
    for (let index = 0; index < settings.length; index += 1) {
        const changed = (index >= 0x224 && index < 0x22c) || (index >= 0x234 && index < 0x238);
        if (!changed) assert.equal(easy[index], settings[index], `바이트 ${index}`);
    }
    const bridged = sampleSettings();
    putFloat(bridged, 0x220, 6);
    putInt(bridged, 0x234, 1);
    const easyBridged = applyAiDifficulty(bridged, AiDifficulty.Easy);
    // 다리 6→24, 채취 1은 최소 1 을 보장한다.
    assert.deepEqual([getFloat(easyBridged, 0x220), getInt(easyBridged, 0x234)], [24, 1]);
    const none = sampleSettings();
    putInt(none, 0x234, 0);
    assert.equal(getInt(applyAiDifficulty(none, AiDifficulty.Easy), 0x234), 0);
    // 5→2(내림), 음수 채취(원래 설계 밖)는 건드리지 않는다.
    const odd = sampleSettings();
    putInt(odd, 0x234, 5);
    assert.equal(getInt(applyAiDifficulty(odd, AiDifficulty.Easy), 0x234), 2);
    putInt(odd, 0x234, -4);
    assert.equal(getInt(applyAiDifficulty(odd, AiDifficulty.Easy), 0x234), -4);
    // 0 이하·비유한 주기는 배율을 곱하지 않는다.
    const odd2 = sampleSettings();
    putFloat(odd2, 0x220, -3);
    putFloat(odd2, 0x224, Number.NaN);
    putFloat(odd2, 0x228, Number.POSITIVE_INFINITY);
    const kept = applyAiDifficulty(odd2, AiDifficulty.Easy);
    assert.deepEqual([getFloat(kept, 0x220), Number.isNaN(getFloat(kept, 0x224)), getFloat(kept, 0x228)], [-3, true, Number.POSITIVE_INFINITY]);
});

test('AiDifficulty: 600바이트보다 큰 설정도 거부하고 owner 0 의 자동 선택은 그대로이며 알 수 없는 단계는 어려움처럼 주기만 줄인다', () => {
    const big = new Uint8Array(601);
    assert.throws(() => applyAiDifficulty(big, AiDifficulty.Easy), RangeError);
    assert.throws(() => sanitizeAiForcedEnemy(big, 2), RangeError);
    // owner 가 0 이어도 강제 적 0(자동 선택)은 바꿀 것이 없다.
    const settings = sampleSettings();
    putInt(settings, 0x240, 0);
    assert.equal(sanitizeAiForcedEnemy(settings, 0), null);
    // 알 수 없는 단계(열거 밖)는 쉬움이 아니어서 배율 0.5 와 채취 유지의 어려움 경로를 탄다.
    const odd = applyAiDifficulty(sampleSettings(), 99);
    assert.deepEqual([getFloat(odd, 0x224), getFloat(odd, 0x228), getInt(odd, 0x234)], [0.5, 1, 6]);
});

test('applyAiDifficulty: 어려움은 세 주기만 절반이고 설정 크기가 600바이트가 아니면 거부한다', () => {
    const settings = sampleSettings();
    putFloat(settings, 0x220, 6);
    const hard = applyAiDifficulty(settings, AiDifficulty.Hard);
    assert.deepEqual([getFloat(hard, 0x220), getFloat(hard, 0x224), getFloat(hard, 0x228)], [3, 0.5, 1]);
    assert.equal(getInt(hard, 0x234), 6);
    assert.throws(() => applyAiDifficulty(settings.subarray(0, 100), AiDifficulty.Hard), RangeError);
    // 단정도 곱셈의 저장 위치: 0.1×4 는 단정도로 좁힌 값이다.
    const odd = sampleSettings();
    putFloat(odd, 0x224, 0.1);
    assert.equal(getFloat(applyAiDifficulty(odd, AiDifficulty.Easy), 0x224), Math.fround(Math.fround(0.1) * 4));
});

// 실제 참여 처리기 없이 AiCreate 의 호출 순서만 관찰하는 작은 장면이다: 생성자/초기 갱신이 받은 설정 바이트와 난이도 hook 의 인자를 기록한다. (2026-11-15 WB04-E555)
function createScene(hooksOverride = {}) {
    const events = [];
    const players = {
        brainReturnedForState: () => false,
        brain: () => 0,
        attachBrain: (owner, brain) => events.push(['attach', owner, brain])
    };
    const hooks = {
        construct: (owner, settings, bytes) => { events.push(['construct', owner, Uint8Array.from(settings), bytes]); return 77; },
        initialUpdate: (brain, settings) => events.push(['update', brain, Uint8Array.from(settings)]),
        ...hooksOverride
    };
    return { events, wrapper: new RawAiCreate(players, hooks) };
}

test('RawAiCreate: 강제 적 정리(NB10)와 난이도 배율이 생성자와 초기 갱신에 같은 설정을 넘기고 입력은 바꾸지 않는다', () => {
    // 난이도 hook 이 없으면 설정을 그대로 쓴다(사본 전달이어도 같은 바이트).
    let scene = createScene();
    const settings = sampleSettings();
    putInt(settings, 0x240, 0);
    putInt(settings, 0x244, 0);
    putInt(settings, 0x250, 0);
    assert.equal(scene.wrapper.create(3, settings), 77);
    assert.deepEqual(scene.events[0][2], settings);
    // 쉬움이면 주기 4배·채취 절반을 생성자와 초기 갱신에 같은 바이트로 넘기고 입력 설정은 바꾸지 않는다. 난이도 hook 은 owner 와 정리된 설정을 받는다.
    const seen = [];
    scene = createScene({ difficulty: (owner, bytes) => { seen.push([owner, getInt(bytes, 0x240)]); return 1; } });
    const bad = sampleSettings();
    putInt(bad, 0x240, 3);
    putInt(bad, 0x244, 0);
    putInt(bad, 0x250, 0);
    const before = Uint8Array.from(bad);
    scene.wrapper.create(3, bad);
    assert.deepEqual(seen, [[3, 0]]);
    const [construct, update] = [scene.events[0], scene.events[2]];
    assert.deepEqual(construct[2], update[2]);
    assert.deepEqual([getInt(construct[2], 0x240), getFloat(construct[2], 0x224), getFloat(construct[2], 0x228), getInt(construct[2], 0x234)], [0, 4, 8, 3]);
    assert.deepEqual(bad, before);
    // 보통(2)·0·범위 밖은 설정을 배율 없이 쓰되 강제 적 정리는 이미 반영된다.
    for (const value of [2, 0, 99]) {
        scene = createScene({ difficulty: () => value });
        scene.wrapper.create(3, bad);
        assert.deepEqual([getFloat(scene.events[0][2], 0x224), getInt(scene.events[0][2], 0x234), getInt(scene.events[0][2], 0x240)], [1, 6, 0]);
    }
    // 어려움은 주기 절반이다. 동맹(+23c) 같은 다른 설정 읽기는 hook 이 받은 정리된 600바이트에서 한다.
    scene = createScene({ difficulty: (owner, bytes) => { assert.equal(bytes.length, 600); return 3; } });
    const hard = sampleSettings();
    putInt(hard, 0x244, 0);
    putInt(hard, 0x250, 0);
    scene.wrapper.create(2, hard);
    assert.deepEqual([getFloat(scene.events[0][2], 0x224), getFloat(scene.events[0][2], 0x228)], [0.5, 1]);
});
