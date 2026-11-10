// 미션 자료(타입 표·미션 스크립트·`.fort`·Tell)를 조회하는 순서를 실제 공개 자료 위에서 고정한다(4-A 마지막 항목, WB04-E273).
//  - 타입 표: 웹 `TYPE_LOAD_ORDER` 가 cpppj `o/TypeLoadOrder.cpp`(원본 00540dd0 의 116개, CD 판은 앞 101개)와 같고, `.type` 을 그 순서로 한 번씩만 읽는다.
//    cpppj `SquidFactory_TypeLoader_ConnectsHpDepthAndConstructorMetadata` 대응 검사는 type-record-complete.test.mjs(E265)이다.
//  - 시작: 설정 버퍼는 `d/setup.cfg` 를 읽은 다음 언어 용어표(`config.<언어>`)를 읽는다(원본 00441790 → 00440e00).
//  - 미션: cpppj `MissionScript` 와 같이 미션 스크립트(`missionSpec`)를 읽은 뒤 `mission.loadFort`(없으면 미션 이름)로 `fortSpec` 경로를 정해 `.fort` 를 읽는다.
//    타입 표는 미션보다 앞서 읽혀 있고(자산 초기화), `.fort` 해석은 그 타입 표를 쓴다.
//  - Tell: 등록 객체 접두어 섹션 → 현재 언어 tell → (웹 적응) 영어 tell 순이며, 영어 tell 은 현재 언어 tell 에 섹션이 없을 때만 한 번 읽는다.
// 파일 조회를 `GameFiles.read/find` 위에서 기록해 비교하며 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E273)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT, readDataMap } from './helpers.mjs';
import { ARCHIVE_PATH, GameData, SETUP_PATH } from '../www/assets/game-data.js';
import { GameFiles } from '../www/assets/game-files.js';
import { Config, sectionRange } from '../www/assets/config.js';
import { FIRST_ASSET_TYPE_NUMBER, TYPE_LOAD_ORDER, TypeTable } from '../www/assets/type-table.js';
import { createFortTypes } from '../www/assets/type-flags.js';
import { parseFort } from '../www/assets/fort.js';
import { decodeOriginalText } from '../www/assets/text-encoding.js';

// 공개 자료 폴더에서 읽은 파일 Map 이다(아카이브·설정·한국어 tell·한국어 도움말). (2026-11-10 WB04-E273)
const DATA_FILES = readDataMap([ARCHIVE_PATH, SETUP_PATH, 'd/tell.korean', 'd/help.korean']);

// 조회를 기록하는 GameFiles 와 GameData 를 만든다. 생성 중의 조회까지 log 에 담고, 필요하면 log.length = 0 으로 비운다.
// 항목은 'read 경로' 또는 'find 패턴' 이다. (2026-11-10 WB04-E273)
function openData(language) {
    const loose = new Map(DATA_FILES);
    const archive = loose.get(ARCHIVE_PATH);
    loose.delete(ARCHIVE_PATH);
    const files = new GameFiles({ loose });
    files.addArchive(archive);
    const log = [];
    const read = files.read.bind(files);
    const find = files.find.bind(files);
    files.read = (name) => {
        log.push(`read ${name}`);
        return read(name);
    };
    files.find = (pattern) => {
        log.push(`find ${pattern}`);
        return find(pattern);
    };
    const data = new GameData({ files, language });
    return { data, files, log };
}

test('타입 로딩 순서: cpppj TypeLoadOrder 116개와 같고 .type 을 그 순서로 한 번씩 읽는다', () => {
    const source = fs.readFileSync(path.join(REPO_ROOT, 'cpppj/src/o/TypeLoadOrder.cpp'), 'utf8');
    const start = source.indexOf('kAssetTypes{');
    const names = [...source.slice(start, source.indexOf('};', start)).matchAll(/"([^"]+)"/g)].map((match) => match[1]);
    // CD 판은 같은 배열의 앞 101개이다.
    assert.equal(Number(/kCdAssetTypeCount = (\d+)/.exec(source)[1]), 101);
    assert.equal(names.length, 116);
    assert.deepEqual([...TYPE_LOAD_ORDER], names);
    const { files, log } = openData('english');
    log.length = 0;
    const errors = [];
    const table = TypeTable.fromArchive(files, decodeOriginalText, (name, error) => errors.push(`${name}: ${error.message}`));
    assert.deepEqual(errors, []);
    // 한 번씩, 원본 순서대로, `\d\이름.type` 만 읽는다.
    assert.deepEqual(log, names.map((name) => `read \\d\\${name}.type`));
    // 번호는 70 부터 로딩 순서대로 붙는다.
    assert.equal(table.find('dude'), FIRST_ASSET_TYPE_NUMBER);
    assert.equal(table.find('Monument'), FIRST_ASSET_TYPE_NUMBER + 115);
    assert.equal(table.find('SUNARCHER'), FIRST_ASSET_TYPE_NUMBER + 1);
});

test('시작 조회 순서: setup.cfg 를 읽은 다음 언어 용어표를 읽고 Tell 은 처음 조회 때 읽는다', () => {
    const english = openData('english');
    // 영어는 설정과 영어 용어표 두 파일뿐이고 Tell 은 읽지 않는다.
    assert.deepEqual(english.log, ['read d/setup.cfg', 'read \\D\\config.english']);
    english.log.length = 0;
    english.data.findTellSection('WELCOME');
    english.data.findTellSection('About');
    // Tell 스크립트는 처음 조회에서 한 번만 읽고 캐시한다.
    assert.deepEqual(english.log, ['read \\D\\tell.english']);
    const korean = openData('korean');
    // 한국어 용어표가 없으면 영어 용어표로 대체하고(웹 적응), 제품 표기 언어를 정하려고 한국어 tell 을 확인한다.
    assert.deepEqual(korean.log.slice(0, 3), ['read d/setup.cfg', 'read \\D\\config.korean', 'read \\D\\config.english']);
    assert.ok(korean.log.includes('read \\D\\tell.korean'));
});

test('Tell 조회 순서: 등록 객체 → 현재 언어 tell → 영어 tell 이며 영어 tell 은 필요할 때 한 번만 읽는다', () => {
    const { data, files, log } = openData('korean');
    const koreanPath = data.pathSpec('missionSpec', 'tell');
    const englishPath = koreanPath.replace(/korean$/i, 'english');
    assert.equal(koreanPath, '\\D\\tell.korean');
    const korean = decodeOriginalText(files.read(koreanPath));
    const english = decodeOriginalText(files.read(englishPath));
    // 한국어 tell 은 웹에서 닿는 섹션만 번역한 부분 파일이라 영어에만 있는 섹션이 있다.
    assert.ok(sectionRange(korean, 'WELCOME') && sectionRange(english, 'WELCOME'));
    assert.ok(!sectionRange(korean, 'ADMINZONE') && sectionRange(english, 'ADMINZONE') && sectionRange(english, 'ADMINZONE2'));
    log.length = 0;
    // 현재 언어 tell 에 섹션이 있으면 영어 tell 은 읽지 않는다.
    const welcome = data.findTellSection('WELCOME');
    assert.equal(welcome, korean.slice(sectionRange(korean, 'WELCOME').start, sectionRange(korean, 'WELCOME').start + sectionRange(korean, 'WELCOME').length));
    assert.deepEqual(log, ['read \\D\\tell.korean']);
    log.length = 0;
    // 없으면 영어 tell 을 한 번 읽어 찾고, 이어지는 영어 섹션은 캐시에서 찾는다.
    const range = sectionRange(english, 'ADMINZONE');
    assert.equal(data.findTellSection('ADMINZONE'), english.slice(range.start, range.start + range.length));
    assert.deepEqual(log, ['read \\D\\tell.english']);
    log.length = 0;
    assert.notEqual(data.findTellSection('ADMINZONE2'), null);
    data.findTellSection('WELCOME');
    assert.deepEqual(log, []);
    // 어느 쪽에도 없으면 null 이고 더 읽지 않는다.
    assert.equal(data.findTellSection('NoSuchTellSection'), null);
    assert.deepEqual(log, []);
    // 등록 객체의 접두어 섹션이 먼저이고 파일을 읽지 않는다. 객체에 없으면 tell 스크립트를 찾지만 점 있는 이름 그대로는 없다.
    const quest = new Config(data.registry, 'quest');
    quest.append('[Intro]\nobject body\n[END]\n');
    assert.equal(data.findTellSection('quest.Intro'), quest.section('Intro'));
    assert.deepEqual(log, []);
    assert.equal(data.findTellSection('quest.WELCOME'), null);
    quest.dispose();
});

test('미션 자료 조회 순서: 타입 표 → 미션 스크립트(missionSpec) → loadFort 의 .fort(fortSpec) 이며 .fort 해석은 앞서 읽은 타입 표를 쓴다', () => {
    const { data, files, log } = openData('english');
    // 자산 초기화: 타입 정의 116개가 미션보다 먼저 읽힌다.
    log.length = 0;
    const table = TypeTable.fromArchive(files, decodeOriginalText);
    assert.equal(log.length, TYPE_LOAD_ORDER.length);
    log.length = 0;
    // 미션 스크립트 경로는 missionSpec 이다. `{@미션.키}` 는 그 스크립트를 조회마다 한 번 읽어 임시 객체에서 값을 찾는다(원본 00440760, 미션 이름은 대문자로 바뀌며 파일 조회는 대소문자를 무시한다).
    assert.equal(data.pathSpec('missionSpec', 'Tutorial2'), '\\D\\Tutorial2.english');
    const loadFort = data.substitute('{@Tutorial2.loadFort}');
    assert.equal(loadFort, 'SecretWorkshop');
    assert.equal(data.substitute('{@Tutorial2.missionType}'), 'Tutorial');
    assert.deepEqual(log, ['read \\D\\TUTORIAL2.english', 'read \\D\\TUTORIAL2.english']);
    log.length = 0;
    // 요새 경로는 loadFort 값을 fortSpec 에 넣어 정하며 스크립트를 다시 읽지 않는다. 그 파일을 한 번 읽는다.
    const fortPath = data.pathSpec('fortSpec', loadFort);
    assert.equal(fortPath, '\\D\\SecretWorkshop.fort');
    assert.deepEqual(log, []);
    const bytes = files.read(fortPath);
    assert.deepEqual(log, ['read \\D\\SecretWorkshop.fort']);
    assert.ok(bytes && bytes.length > 0);
    // .fort 해석은 앞서 읽은 타입 표로 만든 타입 정보를 쓰며 파일을 더 읽지 않는다.
    const fort = parseFort(bytes, createFortTypes(table));
    assert.ok(fort.sections.length > 0 && fort.subscriber !== null);
    assert.equal(log.length, 1);
    // 미션 이름과 loadFort 가 달라도 미션 이름의 .fort 를 대신 읽지 않는다(없는 이름은 null).
    assert.equal(files.read(data.pathSpec('fortSpec', 'Tutorial2')) === null, true);
});

test('언어 파일 조회: 현재 언어 파일을 먼저, 없을 때만 영어 파일을 읽는다(웹 적응)', () => {
    const english = openData('english');
    english.log.length = 0;
    assert.ok(english.data.readText('\\D\\help.english'));
    assert.deepEqual(english.log, ['read \\D\\help.english']);
    const korean = openData('korean');
    korean.log.length = 0;
    // 한국어 도움말 파일이 있으면 영어 파일은 읽지 않고, 없는 파일이면 영어 대응 파일을 한 번 읽는다.
    assert.ok(korean.data.readText('\\D\\help.korean'));
    assert.deepEqual(korean.log, ['read \\D\\help.korean']);
    korean.log.length = 0;
    assert.ok(korean.data.readText('\\D\\offical1.korean'));
    assert.deepEqual(korean.log, ['read \\D\\offical1.korean', 'read \\D\\offical1.english']);
});
