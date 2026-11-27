// 옵션 메뉴 Language(언어 선택)의 선택지·Tell 대상 해석·저장된 선택 읽기와 메뉴 항목을 검사한다. 게임을 실행하지 않는다. (2026-11-20 WB04-E1095)
import test from 'node:test';
import assert from 'node:assert/strict';
import { languageChoices, languageOfChoice, parseLanguageTarget, savedLanguage } from '../www/platform/language-choice.js';
import { buildOptionsItems } from '../www/ui/option-menus.js';
import { Settings, SETTINGS_STORAGE_KEY } from '../www/platform/settings.js';

// getItem 만 가진 가짜 저장소이다.
const storage = (value, fail = false) => ({ getItem: (key) => { if (fail) throw new Error('blocked'); return key === SETTINGS_STORAGE_KEY ? value : null; } });

test('언어 선택지: 각 언어 자신의 표기이며 현재 언어에 체크가 붙는다', () => {
    // 영어 화면에는 한글 글꼴이 없어 한국어 항목은 'Korean' 이고 한국어 화면에서만 '한국어' 이다.
    assert.deepEqual(languageChoices('english'), [{ label: 'English', number: 1, checked: true }, { label: 'Korean', number: 2, checked: false }, { label: 'Japanese', number: 3, checked: false }]);
    assert.deepEqual(languageChoices('korean'), [{ label: 'English', number: 1, checked: false }, { label: '한국어', number: 2, checked: true }, { label: 'Japanese', number: 3, checked: false }]);
    assert.equal(languageOfChoice(1), 'english'); assert.equal(languageOfChoice(2), 'korean'); assert.equal(languageOfChoice(3), 'japanese');
    for (const value of [0, 4, -1, null, undefined, '1']) assert.equal(languageOfChoice(value), null, String(value));
});

test('Tell 대상: 선택지 번호만 받고 대소문자는 무시하며 그 밖은 null 이다', () => {
    assert.equal(parseLanguageTarget('Patch1101.SetLanguage(1)'), 1); assert.equal(parseLanguageTarget('patch1101.setlanguage(2)'), 2);
    for (const bad of ['Patch1101.SetLanguage(0)', 'Patch1101.SetLanguage(4)', 'Patch1101.SetLanguage(-1)', 'Patch1101.SetLanguage(1', 'Patch1101.SetLanguage( 1)', 'Patch1101.SetFrameRate(60)', '', null, undefined]) assert.equal(parseLanguageTarget(bad), null, String(bad));
});

test('저장된 선택: 설정 JSON 의 language 1·2 만 언어가 되고 자동(0)·손상·저장소 오류는 null 이다', () => {
    assert.equal(savedLanguage(storage(JSON.stringify({ version: 1, values: { language: 2 } }))), 'korean');
    assert.equal(savedLanguage(storage(JSON.stringify({ version: 1, values: { language: 1 } }))), 'english');
    assert.equal(savedLanguage(storage(JSON.stringify({ version: 1, values: { language: 0 } }))), null);
    assert.equal(savedLanguage(storage(JSON.stringify({ version: 1, values: {} }))), null);
    for (const value of ['{', 'null', '[]', '', null]) assert.equal(savedLanguage(storage(value)), null, String(value));
    assert.equal(savedLanguage(storage('{}', true)), null); assert.equal(savedLanguage(null), null); assert.equal(savedLanguage(undefined), null);
});

test('메인 메뉴 Options: Language 하위 목록이 Frame Rate 앞에 있고 현재 언어에 체크하며 원본 비교 항목에는 없다', () => {
    const settings = new Settings(), items = buildOptionsItems(settings, { language: 'korean' });
    assert.deepEqual(items.slice(0, 2).map((item) => item.label), ['Language', 'Frame Rate']);
    assert.deepEqual(items[0].items.map((item) => [item.label, item.checked, item.command.name, item.command.argument]), [['English', false, 'Tell', 'Patch1101.SetLanguage(1)'], ['한국어', true, 'Tell', 'Patch1101.SetLanguage(2)'], ['Japanese', false, 'Tell', 'Patch1101.SetLanguage(3)']]);
    assert.deepEqual(buildOptionsItems(settings)[0].items.map((item) => item.label), ['English', 'Korean', 'Japanese']);
    assert.deepEqual(buildOptionsItems(settings)[0].items.map((item) => item.checked), [true, false, false]);
    assert.equal(buildOptionsItems(settings, { includeAll: true }).some((item) => item.label === 'Language'), false);
});
