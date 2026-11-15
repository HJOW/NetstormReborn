// 재기준선 후보의 저장된 8장면과 메타를 검사한다. 게임을 실행하거나 현행 기준선을 바꾸지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { collectionInputs, readScenarios } from '../tools/headless.mjs';
import { compareScenario, judge } from '../tools/screen-compare.mjs';

// 후보 오라클과 시나리오의 저장 경계이다.
const ROOT = path.join(REPO_ROOT, 'webpj/tests/fixtures/cpppj/candidate-225553df');
const ORACLE = path.join(ROOT, 'm1');
const SCENARIOS = path.join(ROOT, 'm1-scenarios.json');

// 텍스트 보고서는 체크아웃 줄바꿈을 정규화한 뒤 해시한다.
function digest(file, text = false) {
    const bytes = text ? Buffer.from(fs.readFileSync(file, 'utf8').replace(/\r\n/g, '\n'), 'utf8') : fs.readFileSync(file);
    return crypto.createHash('sha256').update(bytes).digest('hex');
}

test('제품 1313 후보 오라클의 입력·자료·8장면 해시와 기록된 화면 차이가 일치한다', async () => {
    const meta = JSON.parse(fs.readFileSync(path.join(ORACLE, 'oracle.json'), 'utf8'));
    const scenarios = readScenarios(SCENARIOS);
    assert.equal(meta.cpppjCommit, '225553df');
    assert.equal(meta.exe.sha256, '5b52d96d2e53f4674a40991b78ff64c3c7e2ad8275a0a99b22d18d532b77eb46');
    assert.equal(meta.collectionInputsSha256, crypto.createHash('sha256').update(collectionInputs(scenarios), 'utf8').digest('hex'));
    assert.deepEqual(meta.data.mismatched, []);
    assert.equal(scenarios.scenarios.length, 8);
    assert.deepEqual(Object.keys(meta.results).sort(), scenarios.scenarios.map((scenario) => scenario.id).sort());
    const problems = [];
    // 각 수집 결과의 바이트와 같은 입력의 웹 화면을 함께 대조한다.
    for (const scenario of scenarios.scenarios) {
        const result = meta.results[scenario.id];
        assert.ok(result, `${scenario.id}: 수집 기록`);
        assert.deepEqual(Object.keys(result.snapshots).sort(), scenario.steps.filter(([, operation]) => operation === 'snapshot').map(([, , name]) => name).sort());
        for (const snapshot of Object.values(result.snapshots)) assert.equal(digest(path.join(ORACLE, snapshot.png)), snapshot.pngSha256);
        assert.equal(digest(path.join(ORACLE, result.report.file), true), result.report.sha256);
        for (const expected of Object.values(scenario.expected ?? {})) {
            for (const region of expected.regions ?? []) assert.ok(region.reason && region.rect.length === 4 && Number.isInteger(region.pixels));
        }
        problems.push(...judge(await compareScenario(ORACLE, scenarios, scenario)));
    }
    assert.deepEqual(problems, []);
});
