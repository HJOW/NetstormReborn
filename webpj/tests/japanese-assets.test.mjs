// 일본어 배포 자산의 원본 보존과 실제 메뉴/미션 데이터 소비·영어 대체를 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { DATA_ROOT, REPO_ROOT, readDataMap } from './helpers.mjs';
import { JAPANESE_BUNDLE, JAPANESE_LANGUAGE_PATHS, JAPANESE_TRANSLATIONS_PATH } from '../www/assets/japanese-asset-config.js';
import { buildJapaneseSupport } from '../www/assets/japanese-assets.js';
import { GameData } from '../www/assets/game-data.js';
import { buildMenuResources } from '../www/assets/menu-assets.js';
import { MissionScript } from '../www/core/mission-script.js';
import { TUTORIAL_OBJECTIVES, TUTORIAL_WAITING } from '../www/core/tutorial-objective.js';

// 실제 공개 자산을 각 언어 묶음으로 읽는다. UI 연결 전에도 같은 배포 파일을 소비한다.
const manifest = JSON.parse(fs.readFileSync(path.join(DATA_ROOT, 'manifest.json'), 'utf8'));
const entries = manifest.files.filter((entry) => entry.bundle === JAPANESE_BUNDLE);
const files = readDataMap(entries.map((entry) => entry.path));
const menuFiles = readDataMap(manifest.files.filter((entry) => entry.bundle === 'menu').map((entry) => entry.path));
// 실제 글꼴 검증은 브라우저 단위가 맡고 여기서는 데이터·글리프 연결만 확인한다.
const support = await buildJapaneseSupport(files, { createRasterizer: () => ({ glyph: () => ({ advance: 12, left: 0, top: 0, width: 1, height: 1, coverage: Uint8Array.of(255) }) }) });

// 공용 번역의 누락/별도 사본 분기를 막고 원본 바이트를 그대로 배포하는지 확인한다.
test('일본어 묶음: 공용43파일과 번역표를 동일 바이트로 복사하고 글꼴TTF를 넣지 않는다', () => {
    // 공용 디렉터리의 전체 파일 목록과 공개 경로를 독립적으로 대조한다.
    const expected = fs.readdirSync(path.join(REPO_ROOT, 'assets/lang/japanese/d')).filter((name) => name.endsWith('.japanese')).map((name) => `d/${name}`).sort();
    assert.equal(expected.length, 43);
    assert.deepEqual([...JAPANESE_LANGUAGE_PATHS].sort(), expected);
    assert.deepEqual(entries.map((entry) => entry.path).sort(), [...expected, JAPANESE_TRANSLATIONS_PATH].sort());
    // 모든 배포 파일의 원본과 엄격한UTF-8 해독을 확인한다.
    for (const entry of entries) {
        const source = fs.readFileSync(path.join(REPO_ROOT, entry.source));
        assert.deepEqual(Buffer.from(files.get(entry.path)), source, entry.path);
        assert.doesNotThrow(() => new TextDecoder('utf-8', { fatal: true }).decode(source));
    }
    assert.ok(entries.every((entry) => entry.path.endsWith('.japanese')));
});

// 실제 번역표/UnicodeFontProvider를 메뉴 자원에 연결하고39개 튜토리얼 안내를 빠짐없이 확인한다.
test('일본어 번역: 메뉴와 튜토리얼 안내39키를 실제 자원으로 소비한다', () => {
    const resources = buildMenuResources(menuFiles, support);
    assert.equal(resources.translate('Campaign'), 'キャンペーン');
    assert.equal(resources.translate('알 수 없는 키'), '알 수 없는 키');
    // 중복 단계의 공유 문구는 한 번만 세며 대기 문구도 포함한다.
    const keys = new Set([...TUTORIAL_OBJECTIVES.map((row) => row.english), TUTORIAL_WAITING]);
    assert.equal(keys.size, 39);
    for (const key of keys) {
        assert.ok(support.translations.has(key), key);
        assert.notEqual(resources.translate(key), key);
    }
    assert.equal(support.languageFiles.size, 44);
    assert.ok(resources.fonts);
});

// 원래 MissionScript 경로로 일본어를 읽고 언어 파일 누락 때 영어 대체를 확인한다.
test('일본어 미션:32개 실행 헤더를 보존하고 없는 파일은 영어로 대체한다', () => {
    // 실행에 영향을 주는 헤더만 비교하며 표시 제목/설명은 번역을 허용한다.
    const japanese = GameData.build({ files: new Map([...menuFiles, ...support.languageFiles]), language: 'japanese', product: false });
    const english = GameData.build({ files: menuFiles, language: 'english', product: false });
    const missions = JAPANESE_LANGUAGE_PATHS.filter((name) => !/^d\/(?:offical|tell\.|config\.|help\.)/u.test(name));
    assert.equal(missions.length, 32);
    // 원본과 같은32개 대본을 실제 파서로 구성해 설정 값을 비교한다. 미션 플레이는 하지 않는다.
    for (const file of missions) {
        const name = path.basename(file, '.japanese'), translated = new MissionScript(japanese, name), original = new MissionScript(english, name);
        try {
            // 각 실행 헤더를 원본 영어 파서의 같은 조회와 대조한다.
            for (const key of ['missionType', 'loadFort', 'tutorialNumber', 'myStartMoney', 'techAllowed', 'myTech', 'aiOff']) assert.equal(translated.get(key), original.get(key), `${name}.${key}`);
        } finally { translated.dispose(); original.dispose(); }
    }
    const partial = new Map([...menuFiles, ...support.languageFiles]); partial.delete('d/tutorial1.japanese');
    const fallback = GameData.build({ files: partial, language: 'japanese', product: false });
    assert.equal(fallback.readText('\\d\\tutorial1.japanese'), english.readText('\\d\\tutorial1.english'));
    assert.match(japanese.readText('\\d\\tutorial1.japanese'), /隙間/u);
});

// 필수 번역표나 실제 글꼴 실패를 조용히 삼키지 않아 앱의 언어 대체/진단 경로가 처리하게 한다.
test('일본어 자원 실패: 필수 번역표와 글꼴 공급자의 오류를 전달한다', async () => {
    let called = false;
    await assert.rejects(buildJapaneseSupport(new Map(), { createRasterizer: () => { called = true; } }), /xlat\.japanese/u);
    assert.equal(called, false);
    const failure = new Error('글꼴 계측 실패');
    await assert.rejects(buildJapaneseSupport(files, { createRasterizer: () => { throw failure; } }), (error) => error === failure);
});
