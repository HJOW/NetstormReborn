// 한국어 모드에서 미션 화면에 나오는 커서 경고·생산 힌트·객체 정보·Players 메뉴 Send 줄·도움말 없음 안내가 번역표로 번역되는지 고정한다. (2026-11-23 WB04-E1249)
// 이 문구들은 웹 코드가 번역 호출로 받지만(소스 배열·상수라 E1118·E1166 의 리터럴 호출 수집에 잡히지 않았다) 웹 번역표에 항목이 없어 영어로 남아 있었다.
// 소스 파일에서 원문 리터럴을 직접 읽어 번역표와 대조하므로, 새 문구를 더하고 번역표를 빠뜨리면 이 시험이 실패한다. 게임·브라우저를 실행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { XlatTable } from '../www/assets/xlat.js';
import { formatPlayerMenuText } from '../www/core/raw-player-menu.js';
import { formatFortPriestCount } from '../www/core/raw-campaign-session-install.js';
import { NO_HELP_TEXT, renderHelpTopic } from '../www/ui/help-topic.js';

// 웹·cpppj 한국어 번역표이다. (2026-11-23 WB04-E1249)
const korean = new XlatTable(fs.readFileSync(path.join(REPO_ROOT, 'webpj', 'translations', 'xlat.korean'), 'utf8'));
const cpppj = new XlatTable(fs.readFileSync(path.join(REPO_ROOT, 'assets', 'lang', 'korean', 'xlat.korean'), 'utf8'));
// 소스 파일 본문을 읽는다. 줄바꿈 형식(LF/CRLF)에 영향받지 않게 CR 을 지운다.
const source = (relative) => fs.readFileSync(path.join(REPO_ROOT, 'webpj', relative), 'utf8').replaceAll('\r', '');

// 사제 수 번역은 단수/복수 모두 한국어로 표시하고 영어 복수 접미사를 출력하지 않는다.
test('미션 사제 수와 채팅 도우미의 키워드·번호 범위가 한국어이다', () => {
    const format = new Uint8Array(Buffer.from(korean.translate('%d Priest%s') + '\0', 'utf8'));
    for (const count of [0, 1, 2, 8]) assert.equal(Buffer.from(formatFortPriestCount(format, count)).toString('utf8').split('\0', 1)[0], `사제 ${count}명`);
    for (const key of ['Mimic', 'Sound', 'Icon', 'Emboss Offset', 'Colors %d to %d', 'BGColors %d to %d', 'Icons %d to %d', 'Mods']) assert.match(korean.translate(key), /[가-힣]/u, key);
});
// JS 문자열 리터럴 본문의 \n·\t·\\·\' 이스케이프를 푼다.
const unescape = (body) => body.replace(/\\(.)/gu, (_, letter) => (letter === 'n' ? '\n' : letter === 't' ? '\t' : letter));
// 소스에서 `이름 = [ '…', '…' ]` 형태 배열의 문자열 원소를 모두 읽는다(작은따옴표·큰따옴표).
const arrayLiterals = (text, name) => {
    // 배열은 `];` 또는 `]);` 로 끝난다(원소 리터럴 안에 `]` 가 있어도 처음 `];` 까지 읽는다).
    const match = new RegExp(`${name} = (?:Object\\.freeze\\()?\\[(.*?)\\]\\)?;`, 'su').exec(text); assert.ok(match, `${name} 배열`);
    return [...match[1].matchAll(/'((?:\\.|[^'\\])*)'|"((?:\\.|[^"\\])*)"/gu)].map((item) => unescape(item[1] ?? item[2]));
};

test('커서 배치 경고 다섯 문구가 모두 한국어 번역표에 있다', () => {
    const warnings = arrayLiterals(source('www/core/raw-cursor-lifecycle.js'), 'const WARNINGS');
    assert.equal(warnings.length, 5);
    for (const key of warnings) { assert.equal(korean.has(key), true, key); assert.equal(korean.translate(key), cpppj.translate(key), `cpppj 와 같은 번역 ${key}`); }
    assert.equal(korean.translate("Priest can't get here."), '사제가 이곳에 올 수 없습니다.');
});

test('생산 힌트 두 문구가 한국어 번역표에 있고 색 서식을 보존한다', () => {
    const text = source('www/ui/production-hint-gump.js');
    const keys = [/const EMPTY_TEXT = '((?:\\.|[^'\\])*)'/u, /const POOR_TEXT = '((?:\\.|[^'\\])*)'/u].map((pattern) => unescape(pattern.exec(text)[1]));
    assert.deepEqual(keys, ['~0~w~ENone in inventory', '~0~w~ENot enough Storm Power to build this']);
    for (const key of keys) { assert.equal(korean.has(key), true, key); assert.match(korean.translate(key), /^~0~w~E[가-힣]/u, key); assert.equal(korean.translate(key), cpppj.translate(key)); }
});

test('객체 정보의 포획 가능·연결되지 않음 줄이 줄바꿈까지 번역된다', () => {
    const text = source('www/core/raw-object-menu-info.js');
    assert.ok(text.includes("'~yNot Connected!~.\\n' : '~yCapturable!~.\\n'"));
    assert.equal(korean.translate('~yCapturable!~.\n'), '~y포획 가능!~.\n');
    assert.equal(korean.translate('~yNot Connected!~.\n'), '~y연결되지 않음!~.\n');
});

test('Players 메뉴의 Send 다섯 줄이 모두 한국어이고 색 서식과 금액이 서식 함수로 이어진다', () => {
    const labels = arrayLiterals(source('www/core/raw-player-menu.js'), 'const labels');
    assert.equal(labels.length, 5);
    for (const key of labels) assert.equal(korean.has(key), true, key);
    // 실제 서식 함수로 만든 줄이다: 살 수 있으면 색 없이, 못 사면 빨강(~r)이 금액 앞에 붙는다.
    const rows = labels.map((key) => formatPlayerMenuText(korean.translate(key), ['~r']));
    assert.deepEqual(rows, ['~r300~[IfortGump.3] 보내기', '~r700~[IfortGump.3] 보내기', '~r1000~[IfortGump.3] 보내기', '~r X~[IfortGump.3] 보내기', '~r 전부~[IfortGump.3] 보내기']);
    assert.equal(formatPlayerMenuText(korean.translate(labels[0]), ['']), '300~[IfortGump.3] 보내기');
});

test('도움말 본문이 비면 번역 장치로 바꾼 안내를 쓰고 번역 장치가 없으면 영어 그대로이다', () => {
    assert.equal(NO_HELP_TEXT, 'No help information available');
    // 치환 결과가 비는 본문이다(원본: 설정 치환 뒤 본문 길이가 0 이면 안내 문구).
    const base = { helpText: 'x', topic: 'noSuchTopic', substitute: () => '' };
    assert.equal(renderHelpTopic(base).styleText, NO_HELP_TEXT, '번역 장치 없음');
    const requested = [], result = renderHelpTopic({ ...base, translate: (text) => { requested.push(text); return korean.translate(text); } });
    assert.deepEqual(requested, [NO_HELP_TEXT]);
    assert.equal(result.styleText, '사용할 수 있는 도움말 정보가 없습니다');
    // 본문이 있으면 번역 장치를 부르지 않는다(원본: 치환 뒤 비었을 때만 Translate).
    const found = renderHelpTopic({ helpText: '<a name="t1">\n본문\n', topic: 't1', substitute: (text) => text, translate: () => { throw new Error('호출되면 안 됨'); } });
    assert.equal(found.found, true);
});

test('채팅 도우미 메뉴가 번역 호출로 받는 고정 라벨이 모두 한국어 번역표에 있다', () => {
    // `#helperMenu` 본문에서 tr/translatedItem/sub/toggle/grouped 로 번역을 거치는 리터럴을 읽는다(item() 으로 바로 넣는 Mimic·Sound 등은 원본도 번역하지 않는다).
    const text = source('www/ui/chat-window.js'), body = text.slice(text.indexOf('#helperMenu() {'));
    const keys = new Set();
    // 이 라벨들에는 이스케이프된 따옴표가 없어 `[^']*` 로 충분하다.
    for (const pattern of [/\btr\('([^']*)'\)/gu, /translatedItem\('([^']*)'/gu, /\bsub\('([^']*)'\)/gu, /toggle\('[^']*', '([^']*)'\)/gu, /grouped\([^,]+, \d+, '[^']*', '([^']*)'\)/gu, /\['(~[A-Za-z*.][^']*)', '~[^']*'\]/gu]) {
        for (const match of body.matchAll(pattern)) keys.add(unescape(match[1]));
    }
    // 명령 이름(.front 등)은 번역 호출을 거치지만 영어 그대로 둔다(번역표에 넣지 않는다). 범위 라벨은 코드가 문자열을 이어 붙여 만들어 키가 될 수 없다.
    for (const command of ['.front', '.back', '.format']) keys.delete(command);
    assert.ok(keys.size >= 55, `고정 라벨 ${keys.size}개`);
    for (const key of keys) assert.equal(korean.has(key), true, `번역표에 없음: ${JSON.stringify(key)}`);
    // 서식 접두(~B·~r 등)와 끝 공백이 번역문에도 그대로 이어진다.
    assert.equal(korean.translate('~BBold'), '~B굵게'); assert.equal(korean.translate('~rRed'), '~r빨강'); assert.equal(korean.translate('Chat Effects '), '채팅 효과 ');
    // 용어는 기존 번역표와 맞춘다(Back/Front 는 영어 유지, Emboss 는 엠보스).
    assert.match(korean.translate('Edit Back Format'), /^Back /u); assert.match(korean.translate('Edit Front Format'), /^Front /u); assert.match(korean.translate('~EEmboss'), /엠보스/u);
});
