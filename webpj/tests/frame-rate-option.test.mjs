// 옵션 메뉴 Frame Rate(프레임 상한)의 값 해석·선택지·Tell 대상·프레임 처리 판단을 검사한다. 근거: cpppj `FrameRateOption.cpp`(후순위 F1). 게임을 실행하지 않는다. (2026-11-20 WB04-E1094)
import test from 'node:test';
import assert from 'node:assert/strict';
import { FRAME_RATE_CHOICES, ORIGINAL_FRAME_RATE, frameDue, frameRateChoices, normalizeFrameRate, parseFrameRateTarget } from '../www/core/frame-rate-option.js';
import { buildOptionsItems } from '../www/ui/option-menus.js';
import { Settings } from '../www/platform/settings.js';

test('프레임 상한 값: 0·범위 밖·정수가 아닌 값은 원본 기본 75 이고 1~1000 은 그대로 인정한다', () => {
    assert.deepEqual([...FRAME_RATE_CHOICES], [60, 75, 120]); assert.equal(ORIGINAL_FRAME_RATE, 75);
    for (const value of [0, -1, 1001, 2000, 1.5, NaN, undefined, null, '60']) assert.equal(normalizeFrameRate(value), 75, String(value));
    for (const value of [1, 30, 60, 75, 90, 120, 1000]) assert.equal(normalizeFrameRate(value), value);
});

test('Tell 대상: 접두·닫는 괄호가 정확하고 선택지 안의 10진 정수만 값이며 대소문자는 무시한다', () => {
    assert.equal(parseFrameRateTarget('Patch1101.SetFrameRate(60)'), 60); assert.equal(parseFrameRateTarget('patch1101.setframerate(120)'), 120); assert.equal(parseFrameRateTarget('PATCH1101.SETFRAMERATE(75)'), 75);
    for (const bad of ['Patch1101.SetFrameRate(90)', 'Patch1101.SetFrameRate(060x)', 'Patch1101.SetFrameRate(-60)', 'Patch1101.SetFrameRate( 60)', 'Patch1101.SetFrameRate(60', 'Patch1101.SetFrameRate()', 'Patch1101.SetDifficulty(1)', 'SetFrameRate(60)', '', null, undefined]) assert.equal(parseFrameRateTarget(bad), null, String(bad));
});

test('옵션 선택지: 원본 기본에만 Original 표시가 붙고 현재 상한에 체크가 붙는다', () => {
    assert.deepEqual(frameRateChoices(0), [{ label: '60 FPS', number: 60, checked: false }, { label: '75 FPS (Original)', number: 75, checked: true }, { label: '120 FPS', number: 120, checked: false }]);
    assert.deepEqual(frameRateChoices(120).map((choice) => choice.checked), [false, false, true]); assert.deepEqual(frameRateChoices(60).map((choice) => choice.checked), [true, false, false]);
    // 선택지 밖의 수동 값(예: 90)은 체크가 없다(원본 비교 그대로).
    assert.deepEqual(frameRateChoices(90).map((choice) => choice.checked), [false, false, false]);
});

test('프레임 처리 판단: 첫 프레임은 항상 처리하고 상한 간격(허용 1.5ms)보다 빨리 온 프레임은 건너뛴다', () => {
    assert.equal(frameDue(0, null, 75), true); assert.equal(frameDue(5, undefined, 120), true);
    // 75 FPS 간격은 13.33ms 이므로 11.83ms 미만은 건너뛴다. 60Hz 표시(16.7ms)는 매 프레임 처리된다.
    assert.equal(frameDue(11.8, 0, 75), false); assert.equal(frameDue(11.9, 0, 75), true); assert.equal(frameDue(16.7, 0, 75), true);
    // 144Hz 표시(6.94ms)에서는 한 프레임 걸러 처리해 약 72 FPS 로 돈다.
    let last = 0, handled = 0; for (let step = 1; step <= 144; step += 1) { const now = step * 1000 / 144; if (frameDue(now, last, 75)) { handled += 1; last = now; } }
    assert.ok(handled >= 70 && handled <= 76, `처리 ${handled}`);
    // 상한 60 이면 120Hz 표시를 절반으로, 상한 120 이면 120Hz 표시를 모두 처리한다.
    last = 0; handled = 0; for (let step = 1; step <= 120; step += 1) { const now = step * 1000 / 120; if (frameDue(now, last, 60)) { handled += 1; last = now; } } assert.equal(handled, 60);
    last = 0; handled = 0; for (let step = 1; step <= 120; step += 1) { const now = step * 1000 / 120; if (frameDue(now, last, 120)) { handled += 1; last = now; } } assert.equal(handled, 120);
});

test('메인 메뉴 Options: 웹 항목에는 Frame Rate 하위 목록이 맨 앞에 있고 원본 비교 항목에는 없다', () => {
    const settings = new Settings();
    const web = buildOptionsItems(settings), first = web[0];
    assert.equal(first.kind, 'submenu'); assert.equal(first.label, 'Frame Rate');
    assert.deepEqual(first.items.map((item) => [item.label, item.checked, item.command.name, item.command.argument]),
        [['60 FPS', false, 'Tell', 'Patch1101.SetFrameRate(60)'], ['75 FPS (Original)', true, 'Tell', 'Patch1101.SetFrameRate(75)'], ['120 FPS', false, 'Tell', 'Patch1101.SetFrameRate(120)']]);
    // 설정을 바꾸면 체크가 따라가고, 원본 스크린샷 비교용 includeAll 항목에는 만들지 않는다.
    settings.set('maxFPS', 120); assert.deepEqual(buildOptionsItems(settings)[0].items.map((item) => item.checked), [false, false, true]);
    assert.equal(buildOptionsItems(settings, { includeAll: true }).some((item) => item.label === 'Frame Rate'), false);
});
