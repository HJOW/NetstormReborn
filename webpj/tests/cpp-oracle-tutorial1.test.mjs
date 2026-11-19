// 기준선 cpppj의 고정 시각 Tutorial1 덤프·화면·보고서가 보존됐는지 검사한다. 수집 실행은 하지 않는다. (2026-11-19 WB04-E869)
import test from 'node:test';
import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { BASELINE } from '../tools/port-map.mjs';
import { collectionInputs, readScenarios } from '../tools/headless.mjs';
import { readPng } from '../tools/image-files.mjs';

// 수집된 파일과 시나리오가 있는 고정 기준선 경로다.
const root = path.join(REPO_ROOT, 'webpj', 'tests', 'fixtures', 'cpppj', 'candidate-225553df', 'tutorial1');
const oracle = path.join(root, 'oracle');

// 텍스트는 체크아웃의 줄바꿈 차이를 없애고 원래 LF 바이트 해시를 비교한다.
function normalizedHash(file) {
    const text = fs.readFileSync(file, 'utf8').replace(/\r\n/g, '\n');
    return crypto.createHash('sha256').update(text, 'utf8').digest('hex');
}

// 시나리오·실행 파일·파일 해시와 두 프레임의 실제 월드/Graph를 확인한다.
test('cpppj Tutorial1 오라클: 고정 시각 덤프와 논리 화면 보존', () => {
    const scenario = readScenarios(path.join(root, 'm1-scenarios.json'));
    const meta = JSON.parse(fs.readFileSync(path.join(oracle, 'oracle.json'), 'utf8'));
    assert.equal(meta.cpppjCommit, BASELINE);
    assert.equal(meta.exe.sha256, '5b52d96d2e53f4674a40991b78ff64c3c7e2ad8275a0a99b22d18d532b77eb46');
    assert.equal(meta.collectionInputsSha256, crypto.createHash('sha256').update(collectionInputs(scenario), 'utf8').digest('hex'));
    assert.equal(scenario.fixedFrameMs, 25); assert.equal(scenario.scenarios[0].mission, 'Tutorial1');
    const result = meta.results.tutorial1;
    assert.ok(result.args.includes('--mission') && result.args.includes('--fixed-frame-ms'));
    assert.equal(path.basename(result.dumps.load.file), 'load.tsv');
    assert.equal(path.basename(result.dumps.later.file), 'later.tsv');
    assert.equal(path.basename(result.snapshots.load.png), 'load.png');
    assert.equal(path.basename(result.report.file), 'report.txt');
    // 덤프마다 원문 해시와 실제 Session·255개 Graph 행을 확인한다.
    for (const [name, frame] of [['load', 90], ['later', 130]]) {
        const entry = result.dumps[name], file = path.join(oracle, entry.file);
        assert.equal(normalizedHash(file), entry.sha256);
        const rows = fs.readFileSync(file, 'utf8').replace(/\r\n/g, '\n').trimEnd().split('\n');
        assert.equal(rows[0], 'netstorm_session_dump\t1'); assert.equal(rows[1], 'edition\toriginals');
        assert.ok(rows[2].startsWith(`frame\t${frame}\t`)); assert.ok(rows.includes('world\t1'));
        assert.ok(rows.some((row) => row.startsWith('kernel\t6\t39999')));
        assert.equal(rows.filter((row) => row.startsWith('graph\t')).length, 255);
        assert.ok(rows.includes('graph\t254\t32253\t32253\t0')); assert.equal(rows.at(-1), 'end');
    }
    const screenshot = result.snapshots.load, bytes = fs.readFileSync(path.join(oracle, screenshot.png));
    assert.equal(crypto.createHash('sha256').update(bytes).digest('hex'), screenshot.pngSha256);
    const image = readPng(bytes); assert.deepEqual([image.width, image.height], [1024, 768]);
    const reportFile = path.join(oracle, result.report.file);
    assert.equal(normalizedHash(reportFile), result.report.sha256);
    const report = fs.readFileSync(reportFile, 'utf8');
    assert.match(report, /mission\tTutorial1\tTutorial\t826/); assert.match(report, /mission_start\t1\t4\t0/);
});
