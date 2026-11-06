// cpppj 오라클 비교 검사이다. 저장된 오라클만 읽고 cpppj 를 실행하지 않는다(수집은 tools/cpp-oracle.mjs).
//  1) 메타: tests/fixtures/cpppj/m1/oracle.json 의 형식·cpppj 기준선 커밋·시나리오 파일 해시·그림/보고서 SHA-256 이 현재 파일과 같다.
//  2) M1 비교: m1-scenarios.json 의 시나리오마다 웹을 무브라우저(tools/headless.mjs)로 실행해 오라클 그림·보고서와 비교하고,
//     기록 영역 밖 픽셀 차이 0·영역 안 차이 = 기록 픽셀 수·보고서 칸 일치(또는 기록한 원인)를 확인한다.
//  3) 도구: BMP/PNG 왕복, 영역별 차이 집계, cpppj 보고서 구간 해석.
// (2026-11-07 WB02-H: LEFT_JOBS.webpj.md 3.2절 순서2의 M1 화면 cpppj 대조를 검사로 고정하기 위해 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { BASELINE } from '../tools/port-map.mjs';
import { collectionInputs, readScenarios } from '../tools/headless.mjs';
import { backgroundClassifier, compareImages, compareScenario, judge, parseCppReport } from '../tools/screen-compare.mjs';
import { encodeCppBitmap, encodeIndexedPng, readBmp, readPng } from '../tools/image-files.mjs';

// 오라클 폴더와 두 기록 파일의 절대 경로이다. (2026-11-07 WB02-H)
const ORACLE_DIR = path.join(REPO_ROOT, 'webpj', 'tests', 'fixtures', 'cpppj', 'm1');
const META_PATH = path.join(ORACLE_DIR, 'oracle.json');
const SCENARIOS_PATH = path.join(REPO_ROOT, 'webpj', 'tests', 'fixtures', 'cpppj', 'm1-scenarios.json');

// 파일의 SHA-256 이다. (2026-11-07 WB02-H)
function fileSha256(file) {
    return crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
}

// 텍스트 파일의 CRLF 를 LF 로 바꾼 내용의 SHA-256 이다. 저장소가 core.autocrlf=true 라 Windows 체크아웃에서는 보고서가 CRLF 가 되므로,
// cpp-oracle 이 LF 원문으로 기록한 해시와 같은 값을 얻기 위해 정규화한다(PNG 는 이진이라 변환되지 않는다). (2026-11-07 WB02-H)
function textSha256(file) {
    return crypto.createHash('sha256').update(Buffer.from(fs.readFileSync(file, 'utf8').replace(/\r\n/g, '\n'), 'utf8')).digest('hex');
}

// 메타: 수집 조건과 파일 해시가 기록과 같다.
test('cpppj 오라클: 메타·기준선 커밋·파일 해시', () => {
    const meta = JSON.parse(fs.readFileSync(META_PATH, 'utf8'));
    const data = readScenarios(SCENARIOS_PATH);
    assert.equal(meta.format, 'netstorm-webpj-cpppj-oracle');
    assert.equal(meta.cpppjCommit, BASELINE, '오라클은 이식 기준선 실행 파일로 수집한다');
    assert.match(meta.exe.sha256, /^[0-9a-f]{64}$/);
    const inputs = crypto.createHash('sha256').update(Buffer.from(collectionInputs(data), 'utf8')).digest('hex');
    assert.equal(meta.collectionInputsSha256, inputs, '시나리오의 수집 입력(언어·화면·설정·단계표)이 바뀌었으면 tools/cpp-oracle.mjs 로 다시 수집한다');
    assert.deepEqual(meta.data.mismatched, [], 'cpppj 자료 폴더와 웹 자료의 같은 파일이 달라졌다');
    const problems = [];
    // 시나리오마다 결과 기록과 파일 해시를 확인한다.
    for (const scenario of data.scenarios) {
        const result = meta.results[scenario.id];
        if (!result) { problems.push(`${scenario.id}: 수집 결과 없음`); continue; }
        const names = scenario.steps.filter(([, operation]) => operation === 'snapshot').map(([, , name]) => name);
        assert.deepEqual(Object.keys(result.snapshots).sort(), [...names].sort(), `${scenario.id}: 스냅샷 목록`);
        for (const [name, entry] of Object.entries(result.snapshots)) if (fileSha256(path.join(ORACLE_DIR, entry.png)) !== entry.pngSha256) problems.push(`${scenario.id}/${name}: 그림 해시`);
        if (textSha256(path.join(ORACLE_DIR, result.report.file)) !== result.report.sha256) problems.push(`${scenario.id}: 보고서 해시`);
    }
    assert.deepEqual(problems, []);
});

// M1 비교: 기록한 원인 밖의 차이가 없다.
test('cpppj 오라클: M1 메인 메뉴 화면·보고서가 기록과 일치', async () => {
    const data = readScenarios(SCENARIOS_PATH);
    const problems = [];
    // 시나리오마다 웹을 실행해 비교하고 판정한다.
    for (const scenario of data.scenarios) problems.push(...judge(await compareScenario(ORACLE_DIR, data, scenario)));
    assert.deepEqual(problems, [], '차이가 고쳐졌거나 새로 생겼다: node webpj/tools/screen-compare.mjs --diff <폴더> 로 확인한 뒤 원인을 기록하거나 고친다');
    // 원인을 기록한 차이는 모두 사유 문장을 가진다.
    for (const scenario of data.scenarios) {
        for (const entry of Object.values(scenario.expected ?? {})) {
            for (const region of entry.regions ?? []) assert.ok(region.reason && region.rect.length === 4 && Number.isInteger(region.pixels), `${scenario.id}: 영역 기록`);
            for (const row of Object.values(entry.report ?? {})) assert.ok(row.reason, `${scenario.id}: 보고서 기록`);
        }
    }
});

// 도구: 그림 왕복과 차이 집계, 보고서 해석.
test('cpppj 오라클: 그림 파일·차이 집계·보고서 해석 도구', () => {
    const image = { width: 3, height: 2, rgb: Uint8Array.from([1, 2, 3, 4, 5, 6, 7, 8, 9, 1, 2, 3, 0, 0, 0, 255, 255, 255]) };
    // cpppj BMP 형식: 122바이트 머리, 32비트, 위→아래.
    const bmp = encodeCppBitmap(image);
    assert.deepEqual([bmp.length, bmp.readUInt32LE(10), bmp.readInt32LE(22), bmp.readUInt16LE(28)], [122 + 24, 122, -2, 32]);
    assert.deepEqual(readBmp(bmp), image);
    // 팔레트 PNG 왕복과 결정적 바이트.
    const png = encodeIndexedPng(image);
    assert.deepEqual(readPng(png), image);
    assert.ok(encodeIndexedPng(image).equals(png));
    // 영역 안 차이는 영역에, 밖 차이는 밖에 센다.
    const changed = { ...image, rgb: Uint8Array.from(image.rgb) };
    changed.rgb[0] = 99;
    changed.rgb[15] = 0;
    assert.deepEqual(compareImages(image, changed, [{ rect: [0, 0, 1, 1] }]), { sizeMatch: true, total: 2, outside: 1, outsideBox: [2, 1, 3, 2], regions: [1], background: 0 });
    // 구름 분류: 웹 메인 메뉴가 구름 그대로인 위치에서 웹=웹 구름, cpppj=cpppj 메인 메뉴이면 구름 차이다. 웹이 그림을 빠뜨린 경우(cpppj≠cpppj 메인 메뉴)는 구름이 아니다.
    const webBackground = { ...image, rgb: Uint8Array.from(image.rgb) };
    const webMain = { ...image, rgb: Uint8Array.from(image.rgb) };
    webMain.rgb.set([200, 200, 200], 3); // 픽셀 1 은 웹 메인 메뉴에서 그림이 덮는 위치다.
    const cppMain = { ...image, rgb: Uint8Array.from(image.rgb) };
    cppMain.rgb.set([9, 9, 9], 0); // 픽셀 0: cpppj 구름은 웹 구름과 다르다.
    const classifier = backgroundClassifier(webMain, webBackground, cppMain);
    const cppScene = { ...cppMain, rgb: Uint8Array.from(cppMain.rgb) };
    cppScene.rgb.set([50, 50, 50], 6); // 픽셀 2: cpppj 에만 그림이 있다(웹은 구름).
    assert.deepEqual(Array.from(classifier.mask), [1, 0, 1, 1, 1, 1]);
    const classified = compareImages(cppScene, webBackground, [], classifier);
    assert.deepEqual([classified.total, classified.background, classified.outside, classified.outsideBox], [2, 1, 1, [2, 0, 3, 1]]);
    // cpppj 보고서 구간: 같은 칸이 여러 번 나오면 첫 값을 쓴다.
    const report = parseCppReport('frame\t20\tmain\npage\tmain\npopup\t\npage\tlater\nframe\t30\tnext\nsize\t1024\t768\n');
    assert.deepEqual([report.get('main').get('page'), report.get('main').get('popup'), report.get('next').get('size')], ['main', '', '1024\t768']);
});
