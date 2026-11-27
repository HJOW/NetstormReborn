// 작은 실제 webpack 프로젝트에서 단일 자산/소스 맵/ESLint 관문/오류시 이전 파일 보존을 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import vm from 'node:vm';
import { createRequire } from 'node:module';
import { decodeModWorkerPayload } from '../www/modding/mod-worker-source.js';
// 같은 운영 플러그인을 사용한다. 모드 SDK 대신 작은 신뢰 Worker로 컴파일 경계만 검사한다.
const require = createRequire(import.meta.url), webpack = require('webpack');
const { ModWorkerBundlePlugin, PAYLOAD_KEY } = require('../tools/mod-worker-bundle.js');
const { ESLintBeforeBuildPlugin } = require('../tools/eslint-before-build.js');
// 검사 소유의 임시 파일만 만들고 원래 번들은 건드리지 않는다.
function fixture() {
    const root = fs.mkdtempSync(path.join(os.tmpdir(), 'netstorm-worker-'));
    fs.mkdirSync(path.join(root, 'src')); fs.mkdirSync(path.join(root, 'dist'));
    fs.writeFileSync(path.join(root, 'src/main.js'), 'globalThis.mainRan = true;\n');
    fs.writeFileSync(path.join(root, 'src/worker.js'), 'globalThis.workerRan = true;\n');
    fs.writeFileSync(path.join(root, 'eslint.config.cjs'), "module.exports=[{ignores:['dist/**']},{files:['src/**/*.js'],languageOptions:{globals:{globalThis:'readonly'}},rules:{'no-undef':'error'}}];\n");
    return root;
}
// 컴파일러 종료까지 기다려 임시 경로에 대한 쓰기를 끝낸다.
async function build(root, mode = 'production') {
    const compiler = webpack({ mode, context: root, entry: './src/main.js', devtool: mode === 'development' ? 'source-map' : false, output: { path: path.join(root, 'dist'), filename: 'bundle.js', clean: true }, optimization: { minimize: false }, plugins: [new ESLintBeforeBuildPlugin(root), new ModWorkerBundlePlugin({ entry: path.join(root, 'src/worker.js'), bundle: 'bundle.js' })] });
    try { return await new Promise((resolve, reject) => compiler.run((error, stats) => error ? reject(error) : resolve(stats))); }
    finally { await new Promise((resolve, reject) => compiler.close((error) => error ? reject(error) : resolve())); }
}
test('실행기 원문은 단일 JS에 데이터로만 들어가고 개발 소스 맵은 한 행 이동한다', async () => {
    const root = fixture();
    try {
        // 개발/운영 양쪽에서 별도 Worker 스크립트를 내보내지 않는다.
        for (const mode of ['production', 'development']) {
            const stats = await build(root, mode); assert.equal(stats.hasErrors(), false, stats.toString());
            const files = fs.readdirSync(path.join(root, 'dist')); assert.deepEqual(files.filter((name) => name.endsWith('.js')), ['bundle.js']);
            const source = fs.readFileSync(path.join(root, 'dist/bundle.js'), 'utf8'), scope = {}; vm.runInNewContext(source, scope);
            assert.equal(scope.mainRan, true); assert.equal(scope.workerRan, undefined); assert.match(decodeModWorkerPayload(scope[PAYLOAD_KEY]), /workerRan/u);
            if (mode === 'development') { const map = JSON.parse(fs.readFileSync(path.join(root, 'dist/bundle.js.map'), 'utf8')); assert.ok(map.mappings.startsWith(';')); }
        }
    } finally { fs.rmSync(root, { recursive: true, force: true }); }
});
test('메인에서 import하지 않는 Worker lint 오류와 컴파일 오류도 이전 출력/시각을 보존한다', async () => {
    const root = fixture();
    try {
        assert.equal((await build(root)).hasErrors(), false); const file = path.join(root, 'dist/bundle.js'), before = fs.readFileSync(file), mtime = fs.statSync(file).mtimeMs;
        fs.writeFileSync(path.join(root, 'src/worker.js'), 'missingAuthority();\n'); await assert.rejects(build(root), /ESLint/u);
        assert.deepEqual(fs.readFileSync(file), before); assert.equal(fs.statSync(file).mtimeMs, mtime);
        fs.writeFileSync(path.join(root, 'src/worker.js'), "import './missing.js';\n");
        await assert.rejects(build(root), /missing\.js/u); assert.deepEqual(fs.readFileSync(file), before); assert.equal(fs.statSync(file).mtimeMs, mtime);
    } finally { fs.rmSync(root, { recursive: true, force: true }); }
});
