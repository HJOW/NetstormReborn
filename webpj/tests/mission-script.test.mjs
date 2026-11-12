// 미션 스크립트와 요새 파일 선택(mission-script.js, cpppj `client/Mission`)을 cpppj 시험 8건 이식과 웹 계약으로 확인한다.
//  - cpppj 시험 이식: MissionFortSelectionTests 3건(선택/읽기 분리와 경로 고정 · 스크립트·설정 반납 뒤 source 소유 · 부재/빈 파일/예외 지연), MissionFileSourceTests 의 명시 reader 시험(부재·빈 파일·예외 구별, 추가 설정 파일도 같은 source),
//    사용자 시나리오 별칭(loadFort)과 Fort 이미지 고정, 완료된 부재가 원본 script/Fort 를 되살리지 않음, 현재 언어 script 의 제목, 안내 해제의 통지 순서. Editor 작업 폴더(EditorFiles/EditorCatalogWorkspace)는 웹 제외라 같은 계약을
//    메모리 source 로 확인한다. 같은 파일의 Player 머리글 부분(MissionPlayers.Load)은 E321 시험이 맡는다.
//  - 웹 계약: 기준선 뒤 +12줄(현재 언어 파일의 title/Section 우선과 부재 시 처음 스크립트), 생성 예외 시 등록부 반납, dispose 멱등, 생성 예외 시 모드 유지, 읽지 못한 스크립트의 조회, 빈 loadFort, sectionNames, set 의 접두어 없는 기록.
//  - Config 확장(isValid·appendSection·appendArguments·sectionNames)과 GameData 어댑터(reader·loadFile·appendConfigArguments)도 함께 확인한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-12 WB04-E342)
import test from 'node:test';
import assert from 'node:assert/strict';
import { readData } from './helpers.mjs';
import { ARCHIVE_PATH, GameData, SETUP_PATH } from '../www/assets/game-data.js';
import { Config, ConfigRegistry } from '../www/assets/config.js';
import { MissionScript, createGuideMissionScript, releaseGuideMissionScript } from '../www/core/mission-script.js';

// 큰 아카이브는 한 번만 읽고 GameData 는 시험마다 새로 만든다. (2026-11-12 WB04-E342)
let cachedFiles = null;
// 실제 아카이브와 setup.cfg 로 GameData 를 만들고, 경로 지정값만 시험용으로 바꾼다. readerStub 가 있으면 전역 파일 읽기를 그것으로 바꾼다. (2026-11-12 WB04-E342)
function makeData(configText, readerStub = null) {
    cachedFiles ??= new Map([[ARCHIVE_PATH, readData(ARCHIVE_PATH)], [SETUP_PATH, readData(SETUP_PATH)]]);
    const data = GameData.build({ files: cachedFiles, language: 'english' });
    if (readerStub) data.files = { read: readerStub, find: () => [] };
    if (configText !== null) data.configuration.assign(configText);
    return data;
}

// 문자열을 원문 바이트로 바꾼다. (2026-11-12 WB04-E342)
const bytesOf = (text) => Uint8Array.from(text, (ch) => ch.charCodeAt(0));
// 바이트를 문자열로 바꾼다. (2026-11-12 WB04-E342)
const textOf = (bytes) => String.fromCharCode(...bytes);
// 던진 오류 메시지를 돌려준다(던지지 않으면 빈 문자열). (2026-11-12 WB04-E342)
function failureOf(action) {
    try { action(); } catch (error) { return error.message; }
    return '';
}
// 등록부에 `mission` 객체가 남았는지이다. (2026-11-12 WB04-E342)
const missionObjects = (data) => data.registry.objects.filter((object) => object.prefix === 'mission.').length;

// 시험 공통 경로 설정이다. (2026-11-12 WB04-E342)
const SPECS = 'missionSpec="d/{local.1}.english"\nfortSpec="d/{local.1}.fort"\n[END]\n';

test('cpppj MissionFortSelection: 선택은 I/O 없이 이름을 고정하고 읽기는 선택한 경로만 읽는다', () => {
    const reads = [];
    const mapped = [];
    const data = makeData('missionSpec="d/{local.1}.english"\nfortSpec="first/{local.1}.fort"\n[END]\n', () => { throw new Error('전역 reader 우회 호출'); });
    const mission = new MissionScript(data, 'Choice', '', (path) => { reads.push(path); return bytesOf(path); });
    // 생성자의 script 읽기는 선택기 검사 시작 전에 분리한다.
    reads.length = 0;
    const select = mission.fortFileSelector((path) => { mapped.push(path); return `physical/${path}`; });
    const first = select('Picked');
    assert.deepEqual(reads, []);
    assert.deepEqual(mapped, ['first/Picked.fort']);
    assert.equal(first.fileName, 'physical/first/Picked.fort');
    // 선택 뒤 설정이 바뀌어도 보관한 read 는 앞선 경로를 읽는다.
    data.set('fortSpec', 'second/{local.1}.fort');
    const bytes = first.read();
    assert.deepEqual(reads, ['first/Picked.fort']);
    assert.equal(textOf(bytes), 'first/Picked.fort');
    // 새 선택만 현재 경로를 쓴다.
    const second = select('Picked');
    assert.equal(reads.length, 1);
    assert.equal(mapped.at(-1), 'second/Picked.fort');
    assert.equal(second.fileName, 'physical/second/Picked.fort');
    assert.equal(textOf(second.read()), 'second/Picked.fort');
    assert.deepEqual(reads, ['first/Picked.fort', 'second/Picked.fort']);
    mission.dispose();
});

test('cpppj MissionFortSelection: 이미 선택한 읽기는 스크립트와 설정이 사라져도 source 를 계속 쓴다', () => {
    const reads = [];
    const data = makeData('missionSpec="d/{local.1}.english"\nfortSpec="chosen/{local.1}.fort"\n[END]\n', () => { throw new Error('전역 reader 우회 호출'); });
    const mission = new MissionScript(data, 'Choice', '', (path) => { reads.push(path); return Uint8Array.of(7, 8, 9); });
    const selected = mission.fortFileSelector()('Held');
    assert.equal(selected.fileName, 'chosen/Held.fort');
    // 스크립트를 내리고 설정 줄을 바꿔도 선택한 읽기는 그대로 동작한다.
    mission.dispose();
    data.configuration.assign('');
    assert.deepEqual(Array.from(selected.read()), [7, 8, 9]);
    assert.equal(reads.at(-1), 'chosen/Held.fort');
    // fortReader()와 fortFileReader()도 같은 source 를 닫아 두었다.
    const again = new MissionScript(makeData(SPECS), 'Choice', '', (path) => bytesOf(path));
    const reader = again.fortReader();
    const image = again.fortFileReader((path) => `x/${path}`)('Pic');
    again.dispose();
    assert.equal(textOf(reader('Late')), 'd/Late.fort');
    assert.equal(textOf(image.image), 'd/Pic.fort');
    assert.equal(image.fileName, 'x/d/Pic.fort');
});

test('cpppj MissionFortSelection: 부재·빈 파일·읽기 예외는 read 를 부른 순간에 구별되고 전역 fallback 이 없다', () => {
    let reads = 0;
    const data = makeData(SPECS, () => { throw new Error('선택 source의 전역 fallback'); });
    const mission = new MissionScript(data, 'Choice', '', (path) => {
        reads += 1;
        if (path === 'd/empty.fort') return new Uint8Array(0);
        if (path === 'd/throw.fort') throw new Error('selected reader failure');
        return null;
    });
    reads = 0;
    const select = mission.fortFileSelector();
    const missing = select('missing');
    const empty = select('empty');
    const failure = select('throw');
    assert.equal(reads, 0);
    assert.equal(failureOf(() => missing.read()), 'Game file not found: d/missing.fort');
    assert.equal(reads, 1);
    assert.equal(empty.read().length, 0);
    assert.equal(reads, 2);
    assert.equal(failureOf(() => failure.read()), 'selected reader failure');
    assert.equal(reads, 3);
    // undefined 도 부재이고 source 가 없는 경우의 선택도 부재로 거부한다.
    const undef = new MissionScript(makeData(SPECS), 'U', '', () => undefined);
    assert.equal(failureOf(() => undef.readFortImage('x')), 'Game file not found: d/x.fort');
    undef.dispose();
    mission.dispose();
});

test('cpppj MissionFileSource: 명시 source 는 추가 설정 파일과 스크립트와 Fort 에 같은 장치로 전달되고 실패를 성공으로 바꾸지 않는다', () => {
    const data = makeData('missionSpec="d/{local.1}.english"\nfortSpec="d/{local.1}.fort"\n[END]\n', () => { throw new Error('전역 reader 우회 호출'); });
    const reads = [];
    const source = (path) => {
        reads.push(path);
        if (path === '@d/extra.english') return bytesOf('[Header]\nextraOnly=17\n[END]\n');
        if (path === 'd/Only.english') return bytesOf('[Header]\nmissionType="Tutorial"\n[END]\n');
        if (path === 'd/empty.fort') return new Uint8Array(0);
        if (path === 'd/throw.fort') throw new Error('explicit I/O failure');
        return null;
    };
    const mission = new MissionScript(data, 'Only', '@d/extra.english', source);
    assert.ok(mission.loaded);
    assert.ok(reads.length >= 2 && reads[0] === '@d/extra.english' && reads.at(-1) === 'd/Only.english');
    assert.equal(mission.readFortImage('empty').length, 0);
    assert.equal(failureOf(() => mission.readFortImage('missing')), 'Game file not found: d/missing.fort');
    assert.equal(failureOf(() => mission.readFortImage('throw')), 'explicit I/O failure');
    assert.equal(mission.missionType(), 'Tutorial');
    mission.dispose();
    // 읽기 예외로 만들기가 실패하면 모드와 global.inMission 은 그대로이고 등록부에 객체가 남지 않는다.
    const mode = { value: false };
    const before = missionObjects(data);
    assert.notEqual(failureOf(() => createGuideMissionScript(data, 'Only', mode, () => { throw new Error('reader failure'); })), '');
    assert.equal(mode.value, false);
    assert.equal(data.getInt('global.inMission'), 0);
    assert.equal(missionObjects(data), before);
});

test('cpppj MissionFileSource: 사용자 시나리오 별칭(loadFort)과 이미 고른 Fort 이미지는 이후 loadFort 변경에 흔들리지 않는다', () => {
    const data = makeData('missionSpec="d/{local.1}.english"\nfortSpec="d/{local.1}.fort"\n[END]\n', () => { throw new Error('전역 reader 우회 호출'); });
    const files = new Map([
        ['d/Fresh482.english', bytesOf('[Header]\nmissionType="Tutorial"\nloadFort="TEST01"\nmyStartMoney=12345\nmyTech="all"\n[A.]\n<h2>User scenario</h2>\n[END]\n')],
        ['d/TEST01.fort', Uint8Array.of(1, 2, 3)],
    ]);
    const mode = { value: false };
    const box = { value: createGuideMissionScript(data, 'Fresh482', mode, (path) => files.get(path) ?? null) };
    const mission = box.value;
    assert.ok(mission.loaded && mode.value);
    assert.equal(data.getInt('global.inMission'), 1);
    assert.equal(mission.missionType(), 'Tutorial');
    assert.equal(mission.fortName(), 'TEST01');
    assert.equal(mission.fortPath(), 'd/TEST01.fort');
    assert.equal(mission.get('myStartMoney'), '12345');
    assert.ok(mission.section('A.').includes('User scenario'));
    assert.deepEqual(Array.from(mission.readFortImage(mission.fortName())), [1, 2, 3]);
    // 이미 선택한 Init 요청을 이후 loadFort 변경으로 바꾸지 않는다.
    assert.equal(mission.set('loadFort', 'NotTheCapturedRequest'), true);
    assert.deepEqual(Array.from(mission.readFortImage('TEST01')), [1, 2, 3]);
    assert.equal(mission.fortName(), 'NotTheCapturedRequest');
    // fileName 은 스크립트를 읽었을 때 미션 이름으로 쓰인다.
    assert.equal(mission.get('fileName'), 'Fresh482');
    // 안내 해제: 스크립트 반납 → 모드 false → global.inMission 0.
    releaseGuideMissionScript(box, mode, data);
    assert.equal(box.value, null);
    assert.equal(mode.value, false);
    assert.equal(data.getInt('global.inMission'), 0);
    assert.equal(missionObjects(data), 0);
});

test('cpppj MissionFileSource: 작업본이 삭제되면 원본 스크립트/Fort 를 되살리지 않고 현재 재작성은 다시 읽는다', () => {
    const originalFiles = new Map([['d/TEST01.english', bytesOf('[Header]\nmissionType="Tutorial"\n[END]\n')], ['d/TEST01.fort', Uint8Array.of(9, 9)]]);
    const working = new Map(originalFiles);
    const data = makeData(SPECS, (path) => originalFiles.get(path) ?? null);
    const reader = (path) => working.get(path) ?? null;
    const mission = new MissionScript(data, 'TEST01', '', reader);
    assert.ok(mission.loaded);
    working.delete('d/TEST01.fort');
    assert.equal(failureOf(() => mission.readFortImage('TEST01')), 'Game file not found: d/TEST01.fort');
    assert.ok(originalFiles.has('d/TEST01.fort'));
    working.set('d/TEST01.fort', Uint8Array.of(9, 9));
    assert.deepEqual(Array.from(mission.readFortImage('TEST01')), [9, 9]);
    mission.dispose();
    // 스크립트가 삭제된 상태의 새 미션은 읽지 못한 것이며 전역 reader 로 되살리지 않는다.
    working.delete('d/TEST01.english');
    const missing = new MissionScript(data, 'TEST01', '', reader);
    assert.equal(missing.loaded, false);
    assert.equal(data.reader()('d/TEST01.english') !== null, true);
    assert.equal(missing.get('missionType'), null);
    assert.equal(missing.fortName(), 'TEST01');
    assert.equal(missing.missionType(), '');
    assert.equal(missing.section('Header'), null);
    assert.equal(missing.sectionNames('A'), '');
    missing.dispose();
});

test('cpppj MissionFileSource: 안내 해제는 스크립트를 먼저 내린 뒤 모드와 global.inMission 0 을 알린다', () => {
    const notified = [];
    const data = makeData(SPECS, () => null);
    data.onChange = (key, value) => notified.push([key, value, mode.value, missionObjects(data)]);
    const mode = { value: false };
    const box = { value: createGuideMissionScript(data, 'TEST01', mode, () => bytesOf('[Header]\nmissionType="Tutorial"\n[END]\n')) };
    assert.deepEqual(notified, [['global.inMission', '1', true, 1]]);
    releaseGuideMissionScript(box, mode, data);
    // 해제 통지 시점에는 객체가 내려갔고 모드도 이미 꺼져 있다.
    assert.deepEqual(notified.at(-1), ['global.inMission', '0', false, 0]);
    // 현재 미션이 없으면 효과가 없다.
    releaseGuideMissionScript(box, mode, data);
    assert.equal(notified.length, 2);
});

test('cpppj MissionFileSource: 현재 언어 스크립트의 제목은 언어 파일을, 일반 미션은 원래 파일을 읽는다', () => {
    const files = new Map([
        ['d/TEST01.english', bytesOf('[Header]\nmissionType="Tutorial"\ntitle="EnglishScenario"\nmyStartMoney=50000\n[A]\nEnglish briefing\n[END]\n')],
        ['d/TEST01.korean', bytesOf('[Header]\nmissionType="Tutorial"\ntitle="KoreanScenario"\nmyStartMoney=22\n[A]\nKorean scenario briefing\n[END]\n')],
    ]);
    const source = (path) => files.get(path) ?? null;
    const data = makeData('currentLanguage="english"\nmissionSpec="d/{local.1}.{currentLanguage}"\nfortSpec="d/{local.1}.fort"\n[END]\n', source);
    // 한국어로 읽은 미션: 경로가 .korean 이고 제목이 한국어 파일에서 온다.
    data.set('currentLanguage', 'korean');
    const korean = new MissionScript(data, 'TEST01', '', source);
    assert.ok(korean.loaded);
    assert.equal(korean.scriptPath, 'd/TEST01.korean');
    assert.equal(korean.get('title'), 'KoreanScenario');
    // 표시 언어가 바뀌어도 실행 대본(myStartMoney)은 처음 읽은 스크립트를 유지하고, 제목과 섹션만 현재 언어 파일을 먼저 본다.
    data.set('currentLanguage', 'english');
    assert.equal(korean.get('myStartMoney'), '22');
    assert.equal(korean.get('title'), 'EnglishScenario');
    assert.ok(korean.section('A').includes('English briefing'));
    // 현재 언어 파일에 없는 섹션은 처음 읽은 스크립트의 섹션이다.
    files.set('d/TEST01.english', bytesOf('[Header]\ntitle="OnlyHeader"\n[END]\n'));
    assert.ok(korean.section('A').includes('Korean scenario briefing'));
    assert.equal(korean.get('title'), 'OnlyHeader');
    // 현재 언어 파일이 사라지면 처음 스크립트의 제목으로 돌아간다.
    files.delete('d/TEST01.english');
    assert.equal(korean.get('title'), 'KoreanScenario');
    // 임시 표시 객체는 조회마다 등록부에서 내려간다.
    assert.equal(data.registry.objects.filter((object) => object.prefix === '').length, 2);
    korean.dispose();
    // 같은 경로를 읽은 일반 미션은 임시 객체 없이 원래 스크립트를 쓴다.
    files.set('d/TEST01.english', bytesOf('[Header]\nmissionType="Tutorial"\ntitle="EnglishScenario"\nmyStartMoney=50000\n[END]\n'));
    const english = new MissionScript(data, 'TEST01', '', source);
    assert.equal(english.scriptPath, 'd/TEST01.english');
    assert.equal(english.get('myStartMoney'), '50000');
    assert.equal(english.get('title'), 'EnglishScenario');
    english.dispose();
});

test('현재 언어 파일의 빈 제목/빈 섹션도 있는 값으로 취급한다(없을 때만 처음 스크립트로 돌아감)', () => {
    const files = new Map([
        ['d/T.english', bytesOf('[Header]\ntitle=""\n[A]\n[B]\nbody\n[END]\n')],
        ['d/T.korean', bytesOf('[Header]\ntitle="First"\n[A]\nfirst A\n[END]\n')],
    ]);
    const source = (path) => files.get(path) ?? null;
    const data = makeData('currentLanguage="korean"\nmissionSpec="d/{local.1}.{currentLanguage}"\nfortSpec="d/{local.1}.fort"\n[END]\n', source);
    const mission = new MissionScript(data, 'T', '', source);
    data.set('currentLanguage', 'english');
    // 빈 제목은 있는 값이라 처음 스크립트의 "First" 로 돌아가지 않는다.
    assert.equal(mission.get('title'), '');
    // 본문이 비어 있는 섹션 A 는 현재 언어 파일에서 찾은 것으로 본다(내용이 비어 있어도).
    assert.equal(mission.section('A'), '');
    assert.equal(mission.section('B'), 'body\n');
    mission.dispose();
});

test('기본 reader 경로: 명시 source 가 없으면 설정 인터페이스의 reader/loadFile/appendConfigArguments 를 쓴다', () => {
    const data = makeData(null);
    const mission = new MissionScript(data, 'Tutorial1');
    assert.ok(mission.loaded);
    assert.equal(mission.scriptPath, '\\D\\Tutorial1.english');
    assert.equal(mission.name, 'Tutorial1');
    assert.equal(mission.fileSource(), null);
    assert.ok(mission.missionType().length > 0);
    assert.ok(mission.sectionNames('').length > 0);
    // 기본 경로의 추가 설정은 설정 인터페이스의 appendConfigArguments 로 들어간다.
    const extra = new MissionScript(data, 'Tutorial1', 'extraKey=7;otherKey="x"');
    assert.equal(extra.get('extraKey'), '7');
    assert.equal(extra.get('otherKey'), 'x');
    assert.equal(extra.loaded, true);
    extra.dispose();
    // 실제 Fort 경로와 이미지(파일은 전역 reader 로 읽는다).
    assert.equal(mission.fortName(), mission.get('loadFort') || 'Tutorial1');
    assert.ok(mission.readFortImage(mission.fortName()).length > 0);
    mission.dispose();
    mission.dispose();
    // 읽을 수 없는 미션: 스크립트를 못 읽어 loaded 가 false 이고 조회는 null/빈 값이다.
    const none = new MissionScript(data, 'NoSuchMissionFile');
    assert.equal(none.loaded, false);
    assert.equal(none.get('missionType'), null);
    assert.equal(none.missionType(), '');
    assert.equal(none.fortName(), 'NoSuchMissionFile');
    assert.equal(failureOf(() => none.readFortImage('NoSuchMissionFile')).startsWith('Game file not found:'), true);
    none.dispose();
    assert.equal(missionObjects(data), 0);
});

test('빈 loadFort 는 미션 이름을 쓰고 명시 source 의 fileSource 는 소유 복사를 돌려준다', () => {
    const source = (path) => (path === 'd/E.english' ? bytesOf('[Header]\nloadFort=""\nmissionType="X"\n[A1]\none\n[A2]\ntwo\n[B]\nthree\n[END]\n') : null);
    const data = makeData(SPECS, () => null);
    const mission = new MissionScript(data, 'E', '', source);
    assert.equal(mission.fortName(), 'E');
    assert.equal(mission.fileSource(), source);
    // Config 확장: 이름이 접두어로 시작하는 섹션의 이름 목록이다.
    assert.equal(mission.sectionNames('A'), 'A1;A2;');
    assert.equal(mission.sectionNames('B'), 'B;');
    assert.equal(mission.sectionNames('Z'), '');
    // set 은 객체 접두어 없이 현재 mission 객체에 쓴다(같은 키가 있으면 true).
    assert.equal(mission.set('newKey', '5'), false);
    assert.equal(mission.get('newKey'), '5');
    assert.equal(mission.set('newKey', '6'), true);
    mission.dispose();
});

test('Config 확장: isValid·appendSection·appendArguments·sectionNames 의 원본 규칙', () => {
    const registry = new ConfigRegistry();
    const config = new Config(registry, 'x');
    assert.equal(config.isValid(), false);
    assert.equal(config.sectionNames('A'), '');
    config.appendSection('First');
    assert.equal(config.isValid(), true);
    assert.equal(config.text, '[First]\r\n');
    assert.throws(() => config.appendSection('[Bad]'), /sectionName\[0\] != startSection/u);
    config.clear();
    assert.equal(config.isValid(), false);
    // 인자: `;` 는 줄바꿈이 되고 `@낱말` 은 먼저 파일로 읽는다(낱말은 공백·탭·`;` 에서 끝남, 글 끝이어도 읽음).
    const reads = [];
    const readFile = (path) => { reads.push(path); return path === '@one' ? bytesOf('[F1]\r\na=1\r\n') : path === '@end' ? new Uint8Array(0) : null; };
    config.appendArguments('k=1;@one\tj=2;@missing;m=3;@end', readFile);
    assert.deepEqual(reads, ['@one', '@missing', '@end']);
    // 파일 낱말은 읽는 즉시(빈 파일은 빈 줄 하나) 들어가고 인라인 글은 마지막에 한 번 덧붙는다.
    assert.equal(config.text, '[F1]\r\na=1\r\n\r\nk=1\r\n\tj=2\r\n\r\nm=3\r\n');
    // 인라인 글이 없고 파일 낱말만 있으면 파일 내용만 들어간다(빈 줄을 더하지 않음).
    const only = new Config(registry);
    only.appendArguments('@one', readFile);
    assert.equal(only.text, '[F1]\r\na=1\r\n');
    only.dispose();
    // 빈 글·NUL 앞이 빈 글은 아무것도 더하지 않는다.
    const before = config.text;
    config.appendArguments('', readFile);
    config.appendArguments('\0abc', readFile);
    assert.equal(config.text, before);
    // loadFile 이 있으면 그것만 쓴다.
    const loaded = [];
    const second = new Config(registry);
    second.appendArguments('a;@f;b', () => { throw new Error('호출되면 안 됨'); }, (path) => loaded.push(path));
    assert.deepEqual(loaded, ['@f']);
    assert.equal(second.text, 'a\r\n\r\nb\r\n');
    // 섹션 이름 목록은 등장 순서이고 접두어는 대소문자를 구별하지 않는다.
    const sections = new Config(registry);
    sections.assign('[Alpha]\r\nx=1\r\n[alpha2]\r\ny=2\r\n[Beta]\r\nz=3\r\n[END]\r\n');
    assert.equal(sections.sectionNames('al'), 'Alpha;alpha2;');
    assert.equal(sections.sectionNames(''), 'Alpha;alpha2;Beta;'); // 본문이 비어 버퍼 끝인 [END] 는 찾지 못한 것으로 다룬다
});

test('GameData 어댑터: reader 는 언어 대체 없이 파일을 읽고 loadFile/appendConfigArguments 는 언어 대체를 쓴다', () => {
    const data = makeData(null);
    const reader = data.reader();
    assert.ok(reader('\\D\\Tutorial1.english').length > 0);
    assert.equal(reader('\\D\\NoSuchMissionFile.english'), null);
    const target = new Config(data.registry);
    data.loadFile(target, '\\D\\NoSuchMissionFile.english');
    assert.equal(target.isValid(), false);
    data.loadFile(target, data.pathSpec('missionSpec', 'Tutorial1'));
    assert.equal(target.isValid(), true);
    const args = new Config(data.registry);
    data.appendConfigArguments(args, 'extra=1;@\\D\\NoSuchMissionFile.english');
    assert.equal(args.text, 'extra=1\r\n');
    target.dispose();
    args.dispose();
});
