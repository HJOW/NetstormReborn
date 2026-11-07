// 바깥 링크 새 탭/창 열기(2026-11-05 사용자 결정, docs/webpj-external-access-20261105.md 7절)를 검사한다.
//  1) 판정 모듈(ui/external-link.js): 열 수 있는 주소(http/https/mailto)와 열지 않는 주소(file:·미치환 자리표시·호스트/받는 사람 없음·다른 접두)의 경계
//  2) 열기 장치(platform/external-opener.js): 가짜 문서로 앵커 요소의 주소·target·rel·click 호출과 실패 처리 (실제 브라우저·네트워크는 쓰지 않는다)
//  3) 실제 자료: 도움말 모든 주제와 Tell 섹션의 바깥 링크를 게임 치환 뒤 판정한 결과 목록
// 기대값의 바깥 링크 11종은 문서 3.1절의 목록이다. 원본 주소의 도메인이 지금 살아 있는지는 확인하지 않는다.
// (2026-11-07 WB03-AD: 바깥 링크 새 탭/창 열기 구현과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { DATA_ROOT, readDataMap } from './helpers.mjs';
import { GameData } from '../www/assets/game-data.js';
import { EXTERNAL_LINK_KIND, EXTERNAL_LINK_MAX_LENGTH, resolveExternalLink } from '../www/ui/external-link.js';
import { classifyHelpLink } from '../www/ui/help-link.js';
import { renderHelpTopic } from '../www/ui/help-topic.js';
import { openExternalLink } from '../www/platform/external-opener.js';

// 메뉴 묶음 파일 전부로 영어 게임 데이터를 만든다(링크 자리표시 치환에 쓴다). (2026-11-07 WB03-AD)
const manifest = JSON.parse(fs.readFileSync(path.join(DATA_ROOT, 'manifest.json'), 'utf8'));
const files = readDataMap(manifest.files.filter((entry) => entry.bundle === 'menu').map((entry) => entry.path));
const data = GameData.build({ files, language: 'english', options: { soundVolume: 3, musicVolume: 2 } });

// 판정 경계: 열 수 있는 주소는 종류와 정규화된 주소를 돌려주고, 열 수 없는 주소는 이유를 돌려준다.
test('바깥 링크 판정: http/https/mailto 만 열고 나머지는 이유와 함께 거른다', () => {
    const open = (input) => {
        const link = resolveExternalLink(input);
        assert.equal(link.open, true, `${input}: ${link.reason}`);
        assert.equal(link.reason, null, input);
        return [link.kind, link.url];
    };
    const closed = (input) => {
        const link = resolveExternalLink(input);
        assert.equal(link.open, false, input);
        assert.equal(link.kind, null, input);
        assert.equal(link.url, null, input);
        assert.ok(typeof link.reason === 'string' && link.reason.length > 0, input);
        return link.reason;
    };
    // 열 수 있는 주소: 주소는 URL 해석으로 정규화되고, 접두의 대소문자는 구분하지 않으며, 앞뒤 공백과 첫 NUL 뒤는 버린다.
    assert.deepEqual(open('http://www.netstormhq.com'), [EXTERNAL_LINK_KIND.WEB, 'http://www.netstormhq.com/']);
    assert.deepEqual(open('HTTPS://Example.ORG/a%20b'), [EXTERNAL_LINK_KIND.WEB, 'https://example.org/a%20b']);
    assert.deepEqual(open('http://www.netstormhq.com/content.php?content.27'), [EXTERNAL_LINK_KIND.WEB, 'http://www.netstormhq.com/content.php?content.27']);
    assert.deepEqual(open('  http://te.netstormhq.com\0뒤는 버림'), [EXTERNAL_LINK_KIND.WEB, 'http://te.netstormhq.com/']);
    assert.deepEqual(open('mailto:bugs@titanic.com'), [EXTERNAL_LINK_KIND.MAIL, 'mailto:bugs@titanic.com']);
    assert.deepEqual(open('MAILTO:a@b.c?subject=x'), [EXTERNAL_LINK_KIND.MAIL, 'MAILTO:a@b.c?subject=x']);
    // 원본 자료의 {EMAIL1} 은 치환 뒤 `www.netstormhq.com` 이라 `@` 가 없지만 원본도 그대로 메일 처리기에 넘겼으므로 연다.
    assert.deepEqual(open('mailto:www.netstormhq.com'), [EXTERNAL_LINK_KIND.MAIL, 'mailto:www.netstormhq.com']);
    // 열지 않는 주소: 접두 없음·다른 접두·치환 안 된 자리표시·호스트/받는 사람 없음·공백/제어 문자·너무 긴 주소·빈 값.
    assert.match(closed(''), /빈 주소/);
    assert.match(closed('   '), /빈 주소/);
    assert.match(closed('\0abc'), /빈 주소/);
    assert.match(closed('www.netstormhq.com'), /접두가 없는/);
    for (const scheme of ['file:NetStormPieceList.html', 'telnet:host', 'javascript:alert(1)', 'data:text/html,x', 'ftp://host/a', 'FILE:///c:/a', 'blob:http://a/b', 'vbscript:x']) {
        assert.match(closed(scheme), /웹에서 열지 않습니다/, scheme);
    }
    for (const placeholder of ['http://{UPDATEURL}', 'mailto:{EMAIL1}', 'http://{{param.1}.WebSite}', 'https://a.b/{x}']) {
        assert.match(closed(placeholder), /치환되지 않은/, placeholder);
    }
    for (const noHost of ['http://', 'http:///x', 'http:/x', 'https:www.x.com', 'http://?a', 'http://#a']) {
        assert.match(closed(noHost), /호스트가 없는|해석할 수 없는/, noHost);
    }
    assert.match(closed('mailto:'), /받는 사람/);
    assert.match(closed('mailto:?subject=x'), /받는 사람/);
    assert.match(closed('http://a b.c'), /치환되지 않은 자리표시나 공백/);
    assert.match(closed('http://a.b/\u0001'), /치환되지 않은 자리표시나 공백/);
    assert.match(closed(`http://a.b/${'x'.repeat(EXTERNAL_LINK_MAX_LENGTH)}`), /너무 깁니다/);
    // 판정 결과는 바뀌지 않는 객체이다.
    assert.throws(() => { resolveExternalLink('http://a.b').url = 'x'; }, TypeError);
});

// 가짜 문서를 만든다. 만든 앵커 요소마다 설정된 속성과 click 호출 수를 created 에 쌓는다. 실제 DOM 은 쓰지 않는다. (2026-11-07 WB03-AD)
function fakeDocument({ clickError = null } = {}) {
    const created = [];
    return {
        created,
        createElement(tag) {
            const element = { tag, clicks: 0, click() { if (clickError) throw clickError; this.clicks += 1; } };
            created.push(element);
            return element;
        },
    };
}

// 열기 장치: 웹 주소는 새 탭 속성(target=_blank, rel=noopener noreferrer)으로, 메일 주소는 target 없이 click 한다. 열 수 없는 판정이나 문서 없음은 아무것도 만들지 않는다.
test('바깥 링크 열기 장치: 웹 주소는 새 탭 속성으로, 메일 주소는 target 없이 연다', (t) => {
    const warn = t.mock.method(console, 'warn', () => {});
    const document = fakeDocument();
    // 웹 주소: 앵커 하나를 만들어 새 탭 속성을 주고 click 한다.
    assert.equal(openExternalLink(document, resolveExternalLink('http://www.netstormhq.com')), true);
    assert.equal(document.created.length, 1);
    const [web] = document.created;
    assert.deepEqual({ tag: web.tag, href: web.href, target: web.target, rel: web.rel, clicks: web.clicks }, { tag: 'a', href: 'http://www.netstormhq.com/', target: '_blank', rel: 'noopener noreferrer', clicks: 1 });
    // 메일 주소: target·rel 없이 click 한다(현재 문서를 떠나지 않는다).
    assert.equal(openExternalLink(document, resolveExternalLink('mailto:bugs@titanic.com')), true);
    const [, mail] = document.created;
    assert.deepEqual({ href: mail.href, target: mail.target, rel: mail.rel, clicks: mail.clicks }, { href: 'mailto:bugs@titanic.com', target: undefined, rel: undefined, clicks: 1 });
    // 열 수 없는 판정·잘못된 입력·문서 없음은 앵커를 만들지 않고 false 이다.
    assert.equal(openExternalLink(document, resolveExternalLink('file:NetStormPieceList.html')), false);
    assert.equal(openExternalLink(document, null), false);
    assert.equal(openExternalLink(document, { open: true, kind: 'web', url: 5 }), false);
    assert.equal(openExternalLink(null, resolveExternalLink('http://www.netstormhq.com')), false);
    assert.equal(openExternalLink({}, resolveExternalLink('http://www.netstormhq.com')), false);
    assert.equal(document.created.length, 2);
    assert.equal(warn.mock.callCount(), 0);
    // click 가 예외를 던지면 경고만 하고 false 를 돌려준다(게임 진행은 계속).
    const broken = fakeDocument({ clickError: new Error('차단됨') });
    assert.equal(openExternalLink(broken, resolveExternalLink('https://example.org/')), false);
    assert.equal(warn.mock.callCount(), 1);
});

// 실제 자료: 도움말 모든 주제의 링크와 Tell 섹션(Tips·TechSupport·PieceDump)의 바깥 링크를 게임이 쓰는 치환 뒤 판정한다.
// 문서 3.1절의 11종(치환 전 모습)과, 게임 치환(substitute) 뒤에 열리는/열리지 않는 주소를 고정한다.
test('바깥 링크 판정: 실제 자료의 바깥 링크를 치환 뒤 판정한 목록', () => {
    // 문서 3.1절 11종의 치환 전 모습과 치환 뒤 판정(열림 여부, 열린다면 최종 주소)이다.
    const documented = [
        ['http://www.netstormhq.com', true, 'http://www.netstormhq.com/'],
        ['http://te.netstormhq.com', true, 'http://te.netstormhq.com/'],
        ['http://www.activision.com', true, 'http://www.activision.com/'],
        ['http://www.netstormhq.com/content.php?content.27', true, 'http://www.netstormhq.com/content.php?content.27'],
        ['http://www.microsoft.com', true, 'http://www.microsoft.com/'],
        ['http://www.titanic.com/netstorm/buynetstormdirect.html', true, 'http://www.titanic.com/netstorm/buynetstormdirect.html'],
        ['http://www.titanic.com/netstorm/demo.html', true, 'http://www.titanic.com/netstorm/demo.html'],
        // {UPDATEURL} 은 게임 설정 값으로 치환되어 열린다.
        ['http://{UPDATEURL}', true, 'http://www.netstormhq.com/'],
        // {EMAIL1} 도 설정 값(`www.netstormhq.com`)으로 치환된다. 원본과 같이 메일 처리기에 그대로 넘긴다.
        ['mailto:{EMAIL1}', true, 'mailto:www.netstormhq.com'],
        ['mailto:bugs@titanic.com', true, 'mailto:bugs@titanic.com'],
        // 로컬 파일은 열지 않는다.
        ['file:NetStormPieceList.html', false, null],
    ];
    for (const [raw, opens, url] of documented) {
        assert.equal(classifyHelpLink(raw), 'external', raw);
        const link = resolveExternalLink(data.substitute(raw));
        assert.equal(link.open, opens, `${raw}: ${link.reason}`);
        assert.equal(link.url, url, raw);
    }
    // 도움말 모든 주제의 바깥 링크는 모두 판정을 거치며, 열리지 않는 것은 없다(도움말에는 http 링크만 있다).
    const helpText = data.readText(data.get('Quick Help Spec'));
    const topics = [...new Set([...helpText.matchAll(/<a\s+name\s*=\s*"([^"]+)"/gi)].map((match) => match[1].toLowerCase()))];
    const helpExternal = new Set();
    for (const topic of topics) {
        for (const destination of renderHelpTopic({ helpText, topic, substitute: (text) => data.substitute(text) }).links.destinations) {
            if (classifyHelpLink(destination) !== 'external') continue;
            const link = resolveExternalLink(destination);
            assert.equal(link.open, true, `${topic}: ${destination}: ${link.reason}`);
            helpExternal.add(link.url);
        }
    }
    assert.ok(helpExternal.size >= 1, '도움말에 바깥 링크가 있다');
    assert.ok([...helpExternal].every((url) => url.startsWith('http://') || url.startsWith('https://')));
});
