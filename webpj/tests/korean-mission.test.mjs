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

// 번역 파일이 아직 없는 다음 튜토리얼은 기존 영어 대체 경로를 유지한다.
test('한국어 미션: 번역하지 않은 Tutorial2는 영어 원문으로 대체한다', () => {
    const manifest = JSON.parse(fs.readFileSync(path.join(DATA_ROOT, 'manifest.json'), 'utf8'));
    const files = readDataMap(manifest.files.filter((entry) => entry.bundle === 'menu' || entry.bundle === 'korean').map((entry) => entry.path));
    const data = GameData.build({ files, language: 'korean' }), mission = new MissionScript(data, 'Tutorial2');
    try { assert.ok(mission.loaded); assert.match(mission.section('A.'), /[A-Za-z]{4}/); assert.doesNotMatch(mission.section('A.'), /[가-힣]/u); }
    finally { mission.dispose(); }
});
