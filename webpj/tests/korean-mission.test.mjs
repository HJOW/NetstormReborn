// 한국어 Tutorial1 대본의 실행 설정·명령과 영어 대체를 원본과 대조한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { DATA_ROOT, REPO_ROOT, readData, readDataMap } from './helpers.mjs';
import { TaffArchive } from '../www/assets/taff.js';
import { decodeOriginalText } from '../www/assets/text-encoding.js';
import { GameData } from '../www/assets/game-data.js';
import { MissionScript } from '../www/core/mission-script.js';
import { XlatTable } from '../www/assets/xlat.js';

// 원본과 동일한 실행 키·단계·명령을 유지하고 안내만 한국어로 출력한다.
test('한국어 Tutorial1: 실행 설정과 단계별 명령은 원본과 같다', () => {
    const manifest = JSON.parse(fs.readFileSync(path.join(DATA_ROOT, 'manifest.json'), 'utf8'));
    const files = readDataMap(manifest.files.filter((entry) => entry.bundle === 'menu' || entry.bundle === 'korean').map((entry) => entry.path));
    const source = fs.readFileSync(path.join(REPO_ROOT, 'webpj/translations/Tutorial1.korean'), 'utf8');
    assert.equal(decodeOriginalText(files.get('d/Tutorial1.korean')), source);
    const archive = new TaffArchive(readData('netstorm.tarc')), original = decodeOriginalText(archive.read('\\d\\Tutorial1.english'));
    const header = (text) => text.split('[Options]')[0].split(/\r?\n/).filter((line) => line.trim() && !/^title\s*=/i.test(line)).join('\n');
    assert.equal(header(source), header(original));
    const sections = (text) => [...text.matchAll(/^\[([^\]]+)\]/gm)].map((match) => match[1]); assert.deepEqual(sections(source), sections(original));
    const commands = (text) => [...text.matchAll(/^\$Button=[^,]*,([^\r\n]+)|(<\$[^>]+>)/gm)].map((match) => match[1] ?? match[2]);
    assert.deepEqual(commands(source), commands(original));
    const pictures = (text) => [...text.matchAll(/<!"[^">]+">/g)].map((match) => match[0]); assert.deepEqual(pictures(source), pictures(original));
    const data = GameData.build({ files, language: 'korean' }), mission = new MissionScript(data, 'Tutorial1');
    try { assert.equal(mission.get('loadFort'), 'BridgeTheGap'); assert.equal(mission.get('tutorialNumber'), '1'); assert.match(mission.get('title'), /간극/u); assert.match(mission.section('A.'), /적 사제/u); }
    finally { mission.dispose(); }
});

// 두 번째 튜토리얼도 단계·명령·그림의 원본 순서와 실행 설정을 보존한다.
test('한국어 Tutorial2: 실행 설정과 단계별 명령은 원본과 같다', () => {
    const manifest = JSON.parse(fs.readFileSync(path.join(DATA_ROOT, 'manifest.json'), 'utf8'));
    const files = readDataMap(manifest.files.filter((entry) => entry.bundle === 'menu' || entry.bundle === 'korean').map((entry) => entry.path));
    const source = fs.readFileSync(path.join(REPO_ROOT, 'webpj/translations/Tutorial2.korean'), 'utf8');
    assert.equal(decodeOriginalText(files.get('d/Tutorial2.korean')), source);
    const archive = new TaffArchive(readData('netstorm.tarc')), original = decodeOriginalText(archive.read('\\d\\Tutorial2.english'));
    const header = (text) => text.split('[Options]')[0].split(/\r?\n/).filter((line) => line.trim() && !/^title\s*=/i.test(line)).join('\n');
    const sections = (text) => [...text.matchAll(/^\[([^\]]+)\]/gm)].map((match) => match[1]);
    const commands = (text) => [...text.matchAll(/^\$Button=[^,]*,([^\r\n]+)|(<\$[^>]+>)/gm)].map((match) => match[1] ?? match[2]);
    const pictures = (text) => [...text.matchAll(/<!"[^>]+>/g)].map((match) => match[0]);
    assert.equal(header(source), header(original)); assert.deepEqual(sections(source), sections(original));
    assert.deepEqual(commands(source), commands(original)); assert.deepEqual(pictures(source), pictures(original));
    const data = GameData.build({ files, language: 'korean' }), mission = new MissionScript(data, 'Tutorial2');
    try { assert.equal(mission.get('loadFort'), 'SecretWorkshop'); assert.match(mission.get('title'), /워크샾/u); assert.match(mission.section('A.'), /사제/u); }
    finally { mission.dispose(); }
});

// 번역 파일이 아직 없는 다음 튜토리얼은 기존 영어 대체 경로를 유지한다.
test('한국어 미션: 번역하지 않은 Tutorial3은 영어 원문으로 대체한다', () => {
    const manifest = JSON.parse(fs.readFileSync(path.join(DATA_ROOT, 'manifest.json'), 'utf8'));
    const files = readDataMap(manifest.files.filter((entry) => entry.bundle === 'menu' || entry.bundle === 'korean').map((entry) => entry.path));
    const data = GameData.build({ files, language: 'korean' }), mission = new MissionScript(data, 'Tutorial3');
    try { assert.ok(mission.loaded); assert.match(mission.section('A.'), /[A-Za-z]{4}/); assert.doesNotMatch(mission.section('A.'), /[가-힣]/u); }
    finally { mission.dispose(); }
});

// 공통 명칭과 서식은 사용자 용어표 및 원본 삽입 토큰을 보존한다.
test('한국어 미션 정보: 유닛 이름과 서식 토큰을 보존한다', () => {
    const table = new XlatTable(fs.readFileSync(path.join(REPO_ROOT, 'webpj/translations/xlat.korean'), 'utf8'));
    const names = JSON.parse(fs.readFileSync(path.join(REPO_ROOT, 'assets/unit-names.korean.json'), 'utf8'));
    for (const [english, korean] of Object.entries(names)) assert.equal(table.translate(english), korean, english);
    assert.equal(table.translate('Production'), '생산'); assert.equal(table.translate('Owner'), '소유자'); assert.equal(table.translate('\nClass: ~i%s~.'), '\n분류: ~i%s~.');
    assert.equal(table.translate('Cost: ~i%d~[IfortGump.3]~.\n'), '비용: ~i%d~[IfortGump.3]~.\n');
    // 사제 건설 메뉴의 번역은 원본 포맷 인자와 그림 명령 순서를 보존한다.
    const priestPatterns = ['Construct Building\n', '%sConstruct~.', '%sTemple', 'Build %s%s', '~[Imana.%d] Build %s%s', ' for %s%d~[IfortGump.3]', 'Player %s\n'];
    for (const pattern of priestPatterns) {
        const translated = table.translate(pattern); assert.notEqual(translated, pattern, pattern);
        assert.deepEqual(translated.match(/%[sd]/g) ?? [], pattern.match(/%[sd]/g) ?? [], pattern);
        assert.deepEqual(translated.match(/~\[I[^\]]+\]/g) ?? [], pattern.match(/~\[I[^\]]+\]/g) ?? [], pattern);
    }
    assert.equal(table.translate('%sConstruct~.'), '%s건설~.'); assert.equal(table.translate('%sTemple'), '%s신전');
});
