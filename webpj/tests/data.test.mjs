// 데이터 조회 계층(TAFF 아카이브·가상 파일 조회·게임 데이터/Tell 섹션)을 검사한다.
// TAFF 해독은 Python 도구(tools/taff.py)가 만든 엔트리별 크기·SHA-256 기대값(tests/fixtures/taff-entries.tsv)과 246개 전부를 대조한다.
// (2026-10-28 WB02-D: TAFF·설정·Tell 섹션 데이터층 검증)
import test from 'node:test';
import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { WEBPJ_ROOT, readData } from './helpers.mjs';
import { TaffArchive, archiveNameMatches, normalizeArchiveName, xorDecode } from '../www/assets/taff.js';
import { GameFiles, normalizeFilePath } from '../www/assets/game-files.js';
import { ARCHIVE_PATH, GameData, SETUP_PATH } from '../www/assets/game-data.js';
import { decodeOriginalText, decodeWindows1252, encodeWindows1252 } from '../www/assets/text-encoding.js';

// Python 독립 구현이 만든 TAFF 엔트리 기대값 파일이다. (2026-10-28 WB02-D)
const ENTRY_FIXTURE = path.join(WEBPJ_ROOT, 'tests', 'fixtures', 'taff-entries.tsv');

// 바이트 배열의 SHA-256 을 16진 문자열로 계산한다. (2026-10-28 WB02-D)
function sha256(bytes) {
    return crypto.createHash('sha256').update(bytes).digest('hex');
}

// 데이터 폴더의 TAFF 아카이브를 연다(검사마다 공유). (2026-10-28 WB02-D)
const archive = new TaffArchive(readData(ARCHIVE_PATH));

// 아카이브의 246개 엔트리 이름·크기·복호화 결과가 Python 도구의 기대값과 같은지 확인한다.
test('TAFF: 246개 엔트리 이름 순서·크기·복호화 SHA-256 이 Python 독립 구현과 같다', () => {
    const rows = fs.readFileSync(ENTRY_FIXTURE, 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && line[0] !== '#').map((line) => line.split('\t'));
    assert.equal(rows.length, 246);
    assert.equal(archive.count, 246);
    assert.deepEqual(archive.names(), rows.map((row) => row[0]));
    const mismatches = [];
    // 엔트리마다 복호화한 크기와 해시를 비교한다.
    for (const [name, size, hash] of rows) {
        const bytes = archive.read(name);
        if (!bytes || bytes.length !== Number(size) || sha256(bytes) !== hash) mismatches.push(name);
    }
    assert.deepEqual(mismatches, []);
});

// 이름 비교 규칙(대소문자 무시·".\" 접두·슬래시)과 없는 엔트리 처리, 와일드카드 목록을 확인한다.
test('TAFF: 이름 비교·없는 엔트리·와일드카드 조회', () => {
    assert.equal(archive.stat('\\d\\tell.english').size, 68635);
    assert.equal(archive.has('\\D\\TELL.ENGLISH'), true);
    assert.equal(archive.has('.\\d\\tell.english'), true);
    assert.equal(archive.has('/d/tell.english'), true);
    assert.equal(archive.has('\\d\\nothing.english'), false);
    assert.equal(archive.read('\\d\\nothing.english'), null);
    assert.equal(decodeOriginalText(archive.read('\\d\\tell.english')).startsWith('[COMMENTS]'), true);
    // offical2sep·offical5sep 은 캠페인 목록의 그룹 구분선 행이다(스크린샷 mainMenu - Campaign.png 의 구분선 2개, 6 섹션 + 구분선 2 = 8행).
    assert.deepEqual(archive.find('\\d\\offical*.english'), [
        '\\d\\offical1.english', '\\d\\offical2.english', '\\d\\offical2sep.english', '\\d\\offical3.english',
        '\\d\\offical4.english', '\\d\\offical5.english', '\\d\\offical5sep.english', '\\d\\offical6.english',
    ]);
    assert.deepEqual(archive.find('\\d\\config.????ch'), ['\\d\\config.french']);
    assert.deepEqual(archive.find('\\d\\offical*.german'), []);
    assert.equal(archiveNameMatches('\\d\\offical*.english', '\\d\\sub\\offical1.english'), false);
    assert.equal(normalizeArchiveName('.\\D\\Tell.English'), '\\d\\tell.english');
});

// 잘못된 아카이브(짧음·매직 불일치·범위 밖)를 구분해 오류를 던지는지, XOR 이 대칭인지 확인한다.
test('TAFF: 손상 입력 거부와 XOR 대칭', () => {
    const valid = readData(ARCHIVE_PATH);
    assert.throws(() => new TaffArchive(new Uint8Array(8)), /너무 짧습니다/);
    const badMagic = valid.slice();
    badMagic[0] = 0x58;
    assert.throws(() => new TaffArchive(badMagic), /매직/);
    assert.throws(() => new TaffArchive(valid.subarray(0, 4000)), /밖/);
    const sample = Uint8Array.from([0, 1, 2, 250, 251, 252, 253, 254, 255, 9, 8, 7, 6, 5, 4, 3]);
    assert.deepEqual(xorDecode(xorDecode(sample)), sample);
});

// 가상 파일 조회가 느슨한 파일을 아카이브보다 먼저 보고, 경로 표기와 대소문자 차이를 같은 이름으로 취급하는지 확인한다.
test('가상 파일 조회: 느슨한 파일 우선·경로 표기 통일·중복 제거', () => {
    const override = Uint8Array.from([1, 2, 3]);
    const files = new GameFiles({ loose: new Map([['d/Tell.english', override], ['d/Offical1.ENGLISH', Uint8Array.from([9])], ['d/other.txt', Uint8Array.from([7])]]) });
    files.addArchive(readData(ARCHIVE_PATH));
    assert.equal(files.read('\\D\\tell.english'), override);
    assert.equal(files.read('d/tell.english'), override);
    assert.equal(files.read('\\d\\offical2.english').length > 100, true);
    assert.equal(files.read('\\d\\nothing.english'), null);
    assert.equal(files.has('./D/OTHER.TXT'), true);
    // 대소문자만 다른 offical1 은 느슨한 파일 이름(Offical1.ENGLISH → \offical1.english 형식)이 먼저이고 아카이브의 같은 이름은 빠진다.
    const listed = files.find('\\D\\offical*.english');
    assert.equal(listed.length, 8);
    assert.equal(listed[0], '\\d\\Offical1.ENGLISH');
    assert.deepEqual(listed.slice(1).map((name) => name.toLowerCase()), [
        '\\d\\offical2.english', '\\d\\offical2sep.english', '\\d\\offical3.english', '\\d\\offical4.english', '\\d\\offical5.english', '\\d\\offical5sep.english', '\\d\\offical6.english',
    ]);
    assert.equal(normalizeFilePath('.\\D\\Tell.English'), 'd/tell.english');
});

// 원본 Windows-1252 해독(0x80~0x9F 표 포함)과 UTF-8 판별을 확인한다.
test('텍스트 해독: Windows-1252 와 UTF-8', () => {
    const cp1252 = Uint8Array.from([0x41, 0x80, 0x93, 0x94, 0x99, 0xe9, 0x81]);
    assert.equal(decodeWindows1252(cp1252), 'A€“”™é\u0081');
    assert.deepEqual(encodeWindows1252(decodeWindows1252(Uint8Array.from([0x41, 0x80, 0x93, 0x94, 0x99, 0xe9]))), Uint8Array.from([0x41, 0x80, 0x93, 0x94, 0x99, 0xe9]));
    assert.equal(decodeOriginalText(Uint8Array.from([0xe9])), 'é');
    assert.equal(decodeOriginalText(Uint8Array.from([0xea, 0xb0, 0x80])), '가');
    assert.equal(decodeOriginalText(Uint8Array.from([0xef, 0xbb, 0xbf, 0xea, 0xb0, 0x80])), '가');
    assert.equal(decodeWindows1252(new Uint8Array(100000).fill(0x41)).length, 100000);
});

// 데이터 폴더의 실제 자산으로 만든 GameData 가 원본 설정 규칙대로 용어·경로·Tell 섹션을 돌려주는지 확인한다.
test('게임 데이터: 경로 지정값·언어 대체·Tell 섹션 조회', () => {
    const files = new Map([[ARCHIVE_PATH, readData(ARCHIVE_PATH)], [SETUP_PATH, readData(SETUP_PATH)]]);
    const data = GameData.build({ files, language: 'english', options: { soundVolume: 3, musicVolume: 2 } });
    assert.equal(data.get('version'), 'v10.78');
    assert.equal(data.get('soundVolume'), '3');
    assert.equal(data.get('currentLanguage'), 'english');
    assert.equal(data.pathSpec('missionSpec', 'tell'), '\\D\\tell.english');
    assert.equal(data.pathSpec('languageSpec', 'german'), '\\D\\config.german');
    assert.equal(data.get('local.1'), null, '임시 local 객체는 조회 뒤 내려가야 한다');
    assert.equal(data.substitute('{progName} {factory}'), 'NetStorm Workshop');
    // Credits 섹션: 배경·제목·메뉴 두 줄과 취소 버튼 줄로 이루어진다.
    const credits = data.tellSection('Credits').split(/\r?\n/).filter((line) => line.length > 0);
    assert.deepEqual(credits, [
        '<background="nsstart">',
        '<h2>{H2}NetStorm Credits</h2>',
        '$Menu=Netstorm 10.72 Patch Credits,Tell,CreditsNew',
        '$Menu=Netstorm Original Credits,Tell,Creditsold',
        '$Button=Cancel,Tell,Blank',
    ]);
    // 대소문자를 무시하고, 다음 섹션 직전에서 끝난다.
    assert.equal(data.tellSection('credits'), data.tellSection('Credits'));
    assert.equal(data.tellSection('Credits').includes('[CreditsNew]'), false);
    const campaign = data.tellSection('UCampaign').split(/\r?\n/).filter((line) => line.length > 0);
    assert.deepEqual(campaign, [
        '<background="nsstart">',
        '<h2>{H2}Campaign</h2>',
        '<p>{Text}',
        'Which section would you like to play?<br>',
        '$Timeout=120,Tell,Blank',
        '$Menu=@{guideSpec},Tell,{cur.file}.overview',
        '$Button=Back,Tell,Blank',
    ]);
    assert.equal(data.tellSection('NoSuchSection'), null);
    assert.equal(data.substitute('{guideSpec}'), '\\D\\offical*.english');
    assert.equal(data.files.find(data.get('guideSpec')).length, 8, '캠페인 섹션 6개 + 구분선 2개');
    // 아직 없는 한국어 자료는 영어 파일로 대체한다(웹 적응).
    const korean = GameData.build({ files, language: 'korean' });
    assert.equal(korean.get('currentLanguage'), 'korean');
    assert.equal(korean.pathSpec('missionSpec', 'tell'), '\\D\\tell.korean');
    assert.equal(korean.tellSection('Credits'), data.tellSection('Credits'));
    assert.equal(korean.substitute('{factory}'), 'Workshop');
    // 파일 목록 패턴(`\D\offical*.korean`)도 한국어 파일이 없으면 영어 파일로 대체한다: 한국어 브라우저에서 Campaign 목록이 빈 채로 나오던 결함의 검사.
    // 영어 데이터에서는 대체 없이 그대로이고, 영어로도 맞는 파일이 없는 패턴은 빈 목록이다. (2026-10-30 WB03-H)
    assert.equal(korean.get('guideSpec') ?? korean.substitute('{guideSpec}'), '\\D\\offical*.korean');
    assert.equal(korean.files.find('\\D\\offical*.korean').length, 0);
    assert.deepEqual(korean.findFiles('\\D\\offical*.korean'), data.findFiles('\\D\\offical*.english'));
    assert.equal(korean.findFiles('\\D\\offical*.korean').length, 8);
    assert.equal(data.findFiles('\\D\\offical*.korean').length, 0);
    assert.equal(korean.findFiles('\\D\\nosuchfile*.korean').length, 0);
    // 필수 자산이 빠지면 어떤 파일인지 알린다.
    assert.throws(() => GameData.build({ files: new Map() }), /netstorm\.tarc/);
    assert.throws(() => GameData.build({ files: new Map([[ARCHIVE_PATH, readData(ARCHIVE_PATH)]]) }), /setup\.cfg/);
});
