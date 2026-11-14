// 내장 Tell 조회와 HtmlBody 생성/serial 반환(EmbeddedTell)을 기준선 소스 계약과 실제 공개 tell 자료로 검증한다. (2026-11-14 WB04-E437)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 순서/경계는 기준선 소스(EmbeddedTell.cpp)에서 옮겼다. 실제 경로는 공개 자료의 GameData/Config 위에서 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { readDataMap } from './helpers.mjs';
import { ARCHIVE_PATH, GameData, SETUP_PATH } from '../www/assets/game-data.js';
import { Config, ConfigRegistry } from '../www/assets/config.js';
import { showEmbeddedTell, showEmbeddedTellInConfig } from '../www/ui/embedded-tell.js';

// 모든 효과를 기록하는 장치 묶음이다. sources 의 mission/fallback 본문 표를 쓰고 없는 이름은 null 이다.
function rig({ mission = {}, fallback = {}, token = 0x77, serial = 0x1234 } = {}) {
    const log = [];
    const hooks = {
        mission: (name) => { log.push(['mission', name]); return name in mission ? mission[name] : null; },
        fallback: (name) => { log.push(['fallback', name]); return name in fallback ? fallback[name] : null; },
        missing: (name) => log.push(['missing', name]),
        construct: () => { log.push(['construct']); return token; },
        setHtml: (value, body) => log.push(['setHtml', value, body]),
        serial: (value) => { log.push(['serial', value]); return serial; },
    };
    return { log, hooks };
}

test('showEmbeddedTell: 현재 Mission 본문이 있으면 fallback 없이 생성→설치→serial 순이고 본문 문자열을 그대로 설치한다', () => {
    const r = rig({ mission: { Tips: '<p>hi</p>' } }); assert.equal(showEmbeddedTell('Tips', r.hooks), 0x1234);
    assert.deepEqual(r.log, [['mission', 'Tips'], ['construct'], ['setHtml', 0x77, '<p>hi</p>'], ['serial', 0x77]]);
});

test('showEmbeddedTell: 존재하는 빈 섹션도 HtmlBody 를 만들며 Mission 이 없으면 fallback, 둘 다 없을 때만 missing 뒤 0 이다', () => {
    const empty = rig({ mission: { A: '' } }); assert.equal(showEmbeddedTell('A', empty.hooks), 0x1234); assert.deepEqual(empty.log.map((entry) => entry[0]), ['mission', 'construct', 'setHtml', 'serial']); assert.equal(empty.log[2][2], '');
    const emptyFallback = rig({ fallback: { A: '' } }); assert.equal(showEmbeddedTell('A', emptyFallback.hooks), 0x1234); assert.deepEqual(emptyFallback.log.map((entry) => entry[0]), ['mission', 'fallback', 'construct', 'setHtml', 'serial']);
    const second = rig({ fallback: { B: 'body' } }); assert.equal(showEmbeddedTell('B', second.hooks), 0x1234); assert.deepEqual(second.log, [['mission', 'B'], ['fallback', 'B'], ['construct'], ['setHtml', 0x77, 'body'], ['serial', 0x77]]);
    const none = rig(); assert.equal(showEmbeddedTell('Z', none.hooks), 0); assert.deepEqual(none.log, [['mission', 'Z'], ['fallback', 'Z'], ['missing', 'Z']]);
    // undefined 도 없음이다(optional 반환).
    const undef = rig(); undef.hooks.mission = () => undefined; undef.hooks.fallback = () => undefined; assert.equal(showEmbeddedTell('Z', undef.hooks), 0); assert.deepEqual(undef.log, [['missing', 'Z']]);
});

test('showEmbeddedTell: 이름은 첫 NUL 앞까지만 쓰고 mission 은 선택 장치이며 serial 은 DWORD 로 돌려준다', () => {
    const r = rig({ mission: { ab: 'x' } }); assert.equal(showEmbeddedTell('ab\0cd', r.hooks), 0x1234); assert.equal(r.log[0][1], 'ab');
    // 선두 NUL 은 빈 이름이다.
    const leading = rig({ mission: { '': 'e' } }); assert.equal(showEmbeddedTell('\0ab', leading.hooks), 0x1234); assert.deepEqual(leading.log[0], ['mission', '']);
    const noMission = rig({ fallback: { q: 'y' } }); delete noMission.hooks.mission; assert.equal(showEmbeddedTell('q', noMission.hooks), 0x1234); assert.deepEqual(noMission.log[0], ['fallback', 'q']);
    const wide = rig({ mission: { s: 'x' }, serial: -1 }); assert.equal(showEmbeddedTell('s', wide.hooks), 0xffffffff);
    const huge = rig({ mission: { s: 'x' }, serial: 0x100000005 }); assert.equal(showEmbeddedTell('s', huge.hooks), 5);
});

test('showEmbeddedTell: 장치 누락은 도달한 위치에서만 Error 이고 생성 실패(0 토큰)는 설치 전에 Error 이다', () => {
    assert.throws(() => showEmbeddedTell('x', {}), { name: 'Error', message: /선택된 장치/ });
    // Mission 이 있으면 fallback/missing 은 요구하지 않는다.
    const r = rig({ mission: { m: 'b' } }); delete r.hooks.fallback; delete r.hooks.missing; assert.equal(showEmbeddedTell('m', r.hooks), 0x1234);
    // 둘 다 없는데 missing 이 없으면 Error 이며 construct 는 부르지 않는다.
    const noMissing = rig(); delete noMissing.hooks.missing; assert.throws(() => showEmbeddedTell('n', noMissing.hooks), /선택된 장치/); assert.deepEqual(noMissing.log, [['mission', 'n'], ['fallback', 'n']]);
    // 본문이 있는데 construct/setHtml/serial 이 없으면 각 위치에서 Error 다.
    for (const name of ['construct', 'setHtml', 'serial']) { const x = rig({ mission: { m: 'b' } }); delete x.hooks[name]; assert.throws(() => showEmbeddedTell('m', x.hooks), /선택된 장치/, name); }
    const noSet = rig({ mission: { m: 'b' } }); delete noSet.hooks.setHtml; assert.throws(() => showEmbeddedTell('m', noSet.hooks)); assert.deepEqual(noSet.log.map((entry) => entry[0]), ['mission', 'construct']);
    const failed = rig({ mission: { m: 'b' }, token: 0 }); assert.throws(() => showEmbeddedTell('m', failed.hooks), { name: 'Error', message: /확보 실패/ }); assert.deepEqual(failed.log.map((entry) => entry[0]), ['mission', 'construct']);
    const fallbackless = rig(); delete fallbackless.hooks.fallback; assert.throws(() => showEmbeddedTell('n', fallbackless.hooks), /선택된 장치/);
});

// 공개 자료의 실제 GameData 를 연다(영어 아카이브 + 한국어 tell).
const FILES = readDataMap([ARCHIVE_PATH, SETUP_PATH, 'd/tell.korean']);
const open = (language) => GameData.build({ files: FILES, language });

test('showEmbeddedTellInConfig: 실제 영어 tell 파일을 임시 Config("tell") 로 매번 읽어 섹션을 설치하고 임시 Config 는 UI 생성 전에 등록부에서 내린다', () => {
    const data = open('english'); const baseline = data.registry.objects.length; const seen = [];
    const hooks = { missing: (name) => seen.push(['missing', name]), construct: () => { seen.push(['construct', data.registry.objects.length]); return 9; }, setHtml: (token, body) => seen.push(['setHtml', token, body]), serial: (token) => { seen.push(['serial', token]); return 42; } };
    const loadFile = data.loadFile.bind(data); let loads = 0; data.loadFile = (config, path) => { loads += 1; seen.push(['load', data.registry.objects.length, path]); loadFile(config, path); };
    assert.equal(showEmbeddedTellInConfig('ADMINZONE', data, null, hooks), 42);
    // 임시 Config 는 로드 중 등록(+1)되고 생성 시점에는 이미 내려갔다(기준 개수).
    assert.deepEqual(seen.slice(0, 2), [['load', baseline + 1, '\\D\\tell.english'], ['construct', baseline]]); assert.equal(data.registry.objects.length, baseline);
    const expected = data.tellSection('ADMINZONE'); assert.ok(expected.length > 0); assert.equal(seen[2][2], expected); assert.deepEqual(seen.slice(3), [['serial', 9]]);
    // 매번 새로 읽는다(캐시 재사용 없음).
    seen.length = 0; showEmbeddedTellInConfig('ADMINZONE', data, null, hooks); assert.equal(loads, 2);
    // 대소문자 무시로 같은 섹션을 찾고, 없는 이름은 missing 이며 0 이다.
    seen.length = 0; assert.equal(showEmbeddedTellInConfig('adminzone', data, null, hooks), 42); assert.equal(seen.find((entry) => entry[0] === 'setHtml')[2], expected);
    seen.length = 0; assert.equal(showEmbeddedTellInConfig('NO_SUCH_SECTION', data, null, hooks), 0); assert.deepEqual(seen.map((entry) => entry[0]), ['load', 'missing']); assert.equal(data.registry.objects.length, baseline);
});

test('showEmbeddedTellInConfig: mission 이 있으면 먼저 해독한 키로 조회하며 본문이 있으면 파일을 읽지 않고, 한국어 tell 도 같은 방식으로 읽는다', () => {
    const data = open('english'); let loads = 0; const loadFile = data.loadFile.bind(data); data.loadFile = (config, path) => { loads += 1; loadFile(config, path); };
    const calls = []; const hooks = { missing() {}, construct: () => 3, setHtml: (token, body) => calls.push(['set', body]), serial: () => 8 };
    const mission = (key) => { calls.push(['mission', key]); return key === 'Café' ? '<m/>' : null; };
    assert.equal(showEmbeddedTellInConfig('Caf\xe9', data, mission, hooks), 8); assert.deepEqual(calls, [['mission', 'Café'], ['set', '<m/>']]); assert.equal(loads, 0);
    // UTF-8 바이트 열은 UTF-8 로 해독하고, 이미 해독된 문자열(255 초과 글자 포함)은 그대로 넘긴다.
    calls.length = 0; showEmbeddedTellInConfig('\xc3\xa9', data, mission, hooks); assert.equal(calls[0][1], 'é'); calls.length = 0; showEmbeddedTellInConfig('캠페인', data, mission, hooks); assert.equal(calls[0][1], '캠페인');
    // mission 이 null 을 주면 파일 fallback 으로 넘어간다.
    calls.length = 0; loads = 0; assert.equal(showEmbeddedTellInConfig('ADMINZONE', data, mission, hooks), 8); assert.equal(loads, 1); assert.equal(calls[0][0], 'mission'); assert.equal(calls.at(-1)[1], data.tellSection('ADMINZONE'));
    // 한국어 tell: 한국어 파일의 섹션을 읽는다.
    const korean = open('korean'); const set = []; assert.equal(showEmbeddedTellInConfig('TechSupport', korean, null, { missing() {}, construct: () => 1, setHtml: (token, body) => set.push(body), serial: () => 2 }), 2);
    assert.equal(set[0], korean.tellSection('TechSupport')); assert.ok(/[가-힣]/.test(set[0]));
});

test('showEmbeddedTellInConfig: 호출자의 hooks 는 바꾸지 않고, 읽기 중 예외도 임시 Config 를 내리며 장치 누락은 실제 선택 위치에서 Error 다', () => {
    const data = open('english'); const baseline = data.registry.objects.length; const hooks = { missing() {}, construct: () => 1, setHtml() {}, serial: () => 2, mission: () => 'keep', fallback: () => 'keep' };
    showEmbeddedTellInConfig('ADMINZONE', data, null, hooks); assert.equal(typeof hooks.mission, 'function'); assert.equal(hooks.mission('x'), 'keep'); assert.equal(hooks.fallback('x'), 'keep');
    // 호출자가 준 mission/fallback 은 이 함수가 덮어쓴다(mission 인자가 null 이면 mission 은 비어 파일을 읽는다).
    const failing = open('english'); const base = failing.registry.objects.length; failing.loadFile = () => { throw new Error('load fail'); };
    assert.throws(() => showEmbeddedTellInConfig('ADMINZONE', failing, null, { missing() {}, construct: () => 1, setHtml() {}, serial: () => 2 }), /load fail/); assert.equal(failing.registry.objects.length, base);
    assert.throws(() => showEmbeddedTellInConfig('NO_SUCH_SECTION', data, null, { construct: () => 1, setHtml() {}, serial: () => 2 }), { name: 'Error', message: /선택된 장치/ }); assert.equal(data.registry.objects.length, baseline);
    // 빈 섹션은 있는 것이다: 직접 만든 설정 인터페이스로 확인한다.
    const registry = new ConfigRegistry(); const config = { registry, pathSpec: (key, name) => `${key}/${name}`, loadFile: (target) => { target.append('[Empty]'); target.append('[Next]'); target.append('x'); } };
    const bodies = []; assert.equal(showEmbeddedTellInConfig('Empty', config, null, { missing() {}, construct: () => 5, setHtml: (token, body) => bodies.push(body), serial: () => 6 }), 6); assert.deepEqual(bodies, ['']); assert.equal(registry.objects.length, 0);
    assert.equal(showEmbeddedTellInConfig('Next', config, null, { missing() {}, construct: () => 5, setHtml: (token, body) => bodies.push(body), serial: () => 6 }), 6); assert.deepEqual(bodies, ['', 'x\r\n']);
});
