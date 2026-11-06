// 이식 추적 목록(tests/fixtures/manifest.json)을 검사한다.
//  1) 형식: id·경로가 겹치지 않고, 종류·판본·소비 검사·웹 모듈·단위 문서·cpppj 근거 소스가 실제로 있으며, 소비 검사가 그 자료 이름을 실제로 쓴다.
//  2) 해시: 기록한 정규화 SHA-256·크기·줄 수가 현재 파일과 같다(원본 대조 자료가 바뀌면 이식을 다시 검토하도록 실패한다). 필수 자료가 없으면 실패, 선택 자료는 건너뛴다.
//  3) 적용 범위: 검사 파일이 쓰는 fixture(.tsv)·스크린샷(.png)·cpppj/dotnetpj 교차 확인 소스가 모두 목록에 있고 그 검사가 소비자로 적혀 있다.
// git 기록이 필요한 칸(fixtureCommit·webCommit·cpppjBasis)은 tools/fixture-manifest.mjs 가 채우며 이 검사는 git 을 쓰지 않는다.
// (2026-11-05 WB02-F: LEFT_JOBS.webpj.md 3.1절 이식 추적 요구로 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT, WEBPJ_ROOT } from './helpers.mjs';
import { UNHASHED_KINDS, check, measure, readManifest } from '../tools/fixture-manifest.mjs';

// 이식 추적 목록이다. (2026-11-05 WB02-F)
const manifest = readManifest();

// 저장소 상대 경로의 파일이 있는지 본다. (2026-11-05 WB02-F)
function exists(relative) {
    return fs.existsSync(path.join(REPO_ROOT, relative));
}

// 형식: 고유성·필수 칸·참조 파일 존재·소비 검사의 실제 사용.
test('이식 추적 목록: 형식과 참조 파일', () => {
    assert.equal(manifest.format, 'netstorm-webpj-fixture-manifest');
    assert.equal(manifest.version, 1);
    assert.equal(manifest.originalEdition, '10.78');
    const ids = manifest.entries.map((entry) => entry.id);
    const paths = manifest.entries.map((entry) => entry.path);
    assert.equal(new Set(ids).size, ids.length, 'id 중복');
    assert.equal(new Set(paths).size, paths.length, '경로 중복');
    const problems = [];
    // 항목마다 칸과 참조를 확인한다.
    for (const entry of manifest.entries) {
        const where = entry.id;
        if (!Object.hasOwn(manifest.kinds, entry.kind)) problems.push(`${where}: 모르는 종류 ${entry.kind}`);
        if (entry.edition !== '10.78') problems.push(`${where}: 판본 ${entry.edition}`);
        if (typeof entry.required !== 'boolean' || !entry.unit || !entry.scope || !entry.producer) problems.push(`${where}: 필수 칸 누락`);
        if (entry.consumers.length === 0) problems.push(`${where}: 소비 검사 없음`);
        for (const file of [...entry.consumers, ...entry.modules, entry.doc]) if (!exists(file)) problems.push(`${where}: 없는 파일 ${file}`);
        for (const source of entry.cpppjSources) if (!source.startsWith('cpppj/') || !exists(source)) problems.push(`${where}: cpppj 근거 소스 ${source}`);
        // 계산 칸: 해시 기록 종류는 sha256 이 있어야 하고, 교차 확인 소스는 해시가 없어야 한다.
        if (UNHASHED_KINDS.includes(entry.kind) ? entry.sha256 !== null : !/^[0-9a-f]{64}$/.test(entry.sha256 ?? '')) problems.push(`${where}: sha256 칸`);
        if (!entry.webCommit || Object.keys(entry.cpppjBasis ?? {}).join() !== entry.cpppjSources.join()) problems.push(`${where}: 계산 칸(webCommit/cpppjBasis)을 도구로 채워야 함`);
        // 소비 검사가 그 자료 이름을 실제로 쓴다.
        const name = path.posix.basename(entry.path);
        for (const consumer of entry.consumers) if (exists(consumer) && !fs.readFileSync(path.join(REPO_ROOT, consumer), 'utf8').includes(name)) problems.push(`${where}: ${consumer} 가 ${name} 을 쓰지 않음`);
    }
    assert.deepEqual(problems, []);
});

// 해시: 현재 파일이 기록과 같다. 정규화 규칙(텍스트 CRLF→LF)도 확인한다.
test('이식 추적 목록: 원본 대조 자료의 정규화 해시가 기록과 같다', (t) => {
    // 정규화: CRLF 와 LF 텍스트는 같은 해시, 이진 파일은 바이트 그대로이다.
    assert.equal(measure(Buffer.from('a\r\nb\r\n')).sha256, measure(Buffer.from('a\nb\n')).sha256);
    assert.deepEqual([measure(Buffer.from('a\r\n\r\nb')).lines, measure(Buffer.from([0, 13, 10])).text], [2, false]);
    assert.equal(measure(Buffer.from([0, 13, 10])).bytes, 3);
    const result = check(manifest, { withDrift: false });
    assert.deepEqual(result.mismatches, [], '원본 대조 자료가 바뀌었다: 이식을 다시 검토한 뒤 node webpj/tools/fixture-manifest.mjs --write 로 갱신');
    const required = new Set(manifest.entries.filter((entry) => entry.required).map((entry) => entry.path));
    assert.deepEqual(result.missing.filter((file) => required.has(file)), [], '필수 자료 누락');
    for (const file of result.missing) t.diagnostic(`선택 자료가 없는 환경이라 해시 비교를 건너뜀: ${file}`);
});

// 적용 범위: 검사 파일이 쓰는 원본 대조 자료가 모두 목록에 있다.
test('이식 추적 목록: 검사가 쓰는 fixture·스크린샷·교차 확인 소스가 모두 등록되어 있다', () => {
    const consumersByName = new Map();
    // 목록의 자료 이름 → 소비 검사 집합을 만든다.
    for (const entry of manifest.entries) {
        const name = path.posix.basename(entry.path);
        consumersByName.set(name, new Set([...(consumersByName.get(name) ?? []), ...entry.consumers]));
    }
    const missing = [];
    const testsDirectory = path.join(WEBPJ_ROOT, 'tests');
    // 검사 파일마다 자료 이름을 찾는다(이 파일 자신은 뺀다).
    for (const file of fs.readdirSync(testsDirectory).filter((name) => name.endsWith('.test.mjs') && name !== 'fixture-manifest.test.mjs')) {
        const text = fs.readFileSync(path.join(testsDirectory, file), 'utf8');
        const consumer = `webpj/tests/${file}`;
        const names = new Set([
            ...[...text.matchAll(/([A-Za-z0-9_.-]+\.tsv)\b/g)].map((match) => match[1]),
            ...[...text.matchAll(/'([^'/\\]+\.png)'/g)].map((match) => match[1]),
            // cpppj/dotnetpj 소스를 path.join 조각으로 읽는 교차 확인(fixtures 폴더 제외).
            ...[...text.matchAll(/path\.join\(REPO_ROOT,\s*'(?:cpppj|dotnetpj)'((?:,\s*'[^']+')+)\)/g)]
                .map((match) => [...match[1].matchAll(/'([^']+)'/g)].map((part) => part[1]))
                .filter((parts) => !parts.includes('fixtures'))
                .map((parts) => parts[parts.length - 1]),
        ]);
        // 이식 대응표(PORT_MAP.tsv)는 원본 대조 자료가 아니라 tools/port-map.mjs 가 검사하는 대응표이므로 뺀다. (2026-11-06 WB02-G)
        for (const name of names) if (name !== 'PORT_MAP.tsv' && !consumersByName.get(name)?.has(consumer)) missing.push(`${consumer}: ${name}`);
    }
    assert.deepEqual(missing, []);
});
