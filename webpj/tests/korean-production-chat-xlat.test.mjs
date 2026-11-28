// 한국어 모드에서 생산 창·스톰 파워 표시의 우클릭 도움말 문구와 채팅 채널 이름이 번역표로 번역되는지 고정한다. (2026-11-23 WB04-E1246)
// 원본(cpppj RawProductionControls.cpp:179~186)은 생산 패널을 만들 때 "Storm Power Available"·"Production Window" 를 번역 호출로 받아 노드 도움말 문구로 쓰고,
// 채팅 입력의 채널 이름(ChatRecipients.cpp·RawChatSubmit.cpp 의 여덟 키)은 입력을 현재 언어의 번역 이름과 비교한다. 웹은 이 호출들을 이미 하고 있었지만 웹 번역표(xlat.korean)에 항목이 없어 영어로 남았다.
// 미니맵은 원본 본체의 영어 제목을 보존하며 웹 조립부에서 표시 제목을 번역한다.
// 게임·브라우저를 실행하지 않고 번역표와 소스 파일만 읽는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { XlatTable } from '../www/assets/xlat.js';

// 웹 한국어 번역표(번역 파일 원본)이다. (2026-11-23 WB04-E1246)
const korean = new XlatTable(fs.readFileSync(path.join(REPO_ROOT, 'webpj', 'translations', 'xlat.korean'), 'utf8'));
// cpppj 한국어 번역표이다. 같은 원문의 번역이 두 프로젝트에서 어긋나지 않는지 대조하는 기준이다. (2026-11-23 WB04-E1246)
const cpppj = new XlatTable(fs.readFileSync(path.join(REPO_ROOT, 'assets', 'lang', 'korean', 'xlat.korean'), 'utf8'));
// 소스 파일 본문을 읽는다. 줄바꿈 형식(LF/CRLF)에 영향받지 않게 CR 을 지운다. (2026-11-23 WB04-E1246)
const source = (relative) => fs.readFileSync(path.join(REPO_ROOT, 'webpj', relative), 'utf8').replaceAll('\r', '');

test('생산 창·스톰 파워 도움말과 채팅 채널 이름이 한국어로 번역된다', () => {
    const expected = new Map([['Storm Power Available', '사용 가능한 스톰 파워'], ['Production Window', '생산 창'], ['EVERYONE', '모두'], ['ALLIES', '동맹'], ['BATTLE', '전투']]);
    for (const [key, text] of expected) {
        assert.equal(korean.has(key), true, `${key} 항목`);
        assert.equal(korean.translate(key), text, key);
        // cpppj 한국어 번역표와 같은 번역이어야 두 프로젝트의 한국어가 어긋나지 않는다.
        assert.equal(cpppj.translate(key), text, `cpppj ${key}`);
    }
});

test('생산 패널이 도움말 문구로 쓰는 영어 원문은 모두 한국어 번역표에 있다', () => {
    // 생산 패널 소스의 `parts.help(토큰, '원문', '주제')` 호출에서 원문 리터럴을 읽는다(원본 hooks.translate 대상). 새 도움말을 더하면 번역표 항목이 없을 때 이 시험이 실패한다.
    const text = source('www/ui/client-production-panel.js'), keys = [...text.matchAll(/parts\.help\(\s*[^,()]+,\s*'([^']+)'\s*,/gu)].map((match) => match[1]);
    assert.deepEqual([...new Set(keys)].sort(), ['Production Window', 'Storm Power Available']);
    for (const key of keys) assert.equal(korean.has(key), true, `${key} 번역 항목`);
    // 조립부는 이 원문을 번역 장치를 거쳐 노드에 넣는다(원문을 그대로 넣지 않는다).
    assert.match(source('www/ui/client-production-assembly.js'), /devices\.setNodeHelp\(token, devices\.translate\(text\), false\)/u);
});

test('채팅 채널 키 가운데 번역 항목이 있는 것과 없는 것이 cpppj 번역표와 같다', () => {
    // 채널 키(BATTLE·EVERYONE·ALLIES·OBSERVER)는 세 소스가 같은 여덟 칸 배열을 쓴다.
    for (const file of ['www/ui/chat-recipients.js', 'www/ui/raw-chat-submit.js', 'www/ui/chat-name-completion.js']) {
        const match = /const CHANNELS = \[([^\]]+)\]/u.exec(source(file)); assert.ok(match, file);
        assert.deepEqual([...match[1].matchAll(/'([^']*)'/gu)].map((item) => item[1]), ['', '', '', 'BATTLE', '', 'EVERYONE', 'ALLIES', 'OBSERVER'], file);
    }
    // 번역 항목이 있는 키는 웹과 cpppj 표에서 모두 번역되고, OBSERVER 는 두 표 모두 항목이 없다(관전은 단독 미션에 없다).
    for (const key of ['BATTLE', 'EVERYONE', 'ALLIES']) assert.equal(korean.has(key) && cpppj.has(key), true, key);
    assert.equal(korean.has('OBSERVER'), false); assert.equal(cpppj.has('OBSERVER'), false);
});

test('미니맵 도움말 Sky Overview 는 웹 조립부에서 번역하고 주제 키는 보존한다', () => {
    // 원본 본체는 영어 리터럴을 직접 넣고 웹 ClientMinimap 조립부가 표시 번역을 맡는다.
    const text = source('www/ui/raw-minimap-controls.js');
    assert.match(text, /this\.#menus\.setHelpText\(this\.#token, 'Sky Overview'\)/u);
    assert.equal(korean.translate('Sky Overview'), '하늘 전체 보기');
    assert.match(source('www/ui/client-minimap.js'), /devices\.translate\?\.\(text\)/u);
    assert.match(source('www/netstorm.js'), /setNodeHelp: devices\.setNodeHelp, translate: devices\.translate/u);
});
