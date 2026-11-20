// 옵션 메뉴 Difficulty(난이도)의 선택지·Tell 대상 해석·메뉴 항목과 설정 연결을 검사한다. 계산은 ai-difficulty.test.mjs 가 맡는다. 게임을 실행하지 않는다. (2026-11-20 WB04-E1096)
import test from 'node:test';
import assert from 'node:assert/strict';
import { difficultyChoices, parseDifficultyTarget } from '../www/core/difficulty-option.js';
import { buildOptionsItems } from '../www/ui/option-menus.js';
import { Settings } from '../www/platform/settings.js';

test('난이도 선택지: 설정이 없거나 범위 밖이면 보통에 체크하고 1·3 은 쉬움·어려움에 체크한다', () => {
    const checked = (value) => difficultyChoices(value).filter((choice) => choice.checked).map((choice) => choice.label);
    for (const value of [0, undefined, null, 2, 4, 99, -1]) assert.deepEqual(checked(value), ['Normal'], String(value));
    assert.deepEqual(checked(1), ['Easy']); assert.deepEqual(checked(3), ['Hard']);
    assert.deepEqual(difficultyChoices(0).map((choice) => [choice.label, choice.number]), [['Easy', 1], ['Normal', 2], ['Hard', 3]]);
});

test('Tell 대상: 1~3 의 10진 숫자만 받고 대소문자는 무시하며 그 밖은 null 이다', () => {
    assert.equal(parseDifficultyTarget('Patch1101.SetDifficulty(1)'), 1); assert.equal(parseDifficultyTarget('patch1101.setdifficulty(3)'), 3); assert.equal(parseDifficultyTarget('PATCH1101.SETDIFFICULTY(2)'), 2);
    for (const bad of ['Patch1101.SetDifficulty(0)', 'Patch1101.SetDifficulty(4)', 'Patch1101.SetDifficulty(-1)', 'Patch1101.SetDifficulty(2', 'Patch1101.SetDifficulty( 2)', 'Patch1101.SetLanguage(1)', '', null, undefined]) assert.equal(parseDifficultyTarget(bad), null, String(bad));
});

test('메인 메뉴 Options: Difficulty 하위 목록이 Frame Rate 다음에 있고 설정을 따르며 원본 비교 항목에는 없다', () => {
    const settings = new Settings(), items = buildOptionsItems(settings);
    assert.deepEqual(items.slice(0, 3).map((item) => item.label), ['Language', 'Frame Rate', 'Difficulty']);
    const level = items[2];
    assert.deepEqual(level.items.map((item) => [item.label, item.checked, item.command.name, item.command.argument]),
        [['Easy', false, 'Tell', 'Patch1101.SetDifficulty(1)'], ['Normal', true, 'Tell', 'Patch1101.SetDifficulty(2)'], ['Hard', false, 'Tell', 'Patch1101.SetDifficulty(3)']]);
    settings.set('difficulty', 3); assert.deepEqual(buildOptionsItems(settings)[2].items.map((item) => item.checked), [false, false, true]);
    assert.equal(buildOptionsItems(settings, { includeAll: true }).some((item) => item.label === 'Difficulty'), false);
});
