// 실제 webpack으로 선행 검사 실패 시 기존 산출물을 보존하는지 임시 프로젝트에서 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { createRequire } from 'node:module';

// webpack과 검사 플러그인의 CommonJS 진입점이다.
const require = createRequire(import.meta.url);
const webpack = require('webpack');
const { ESLintBeforeBuildPlugin } = require('../tools/eslint-before-build.js');

// 각 검사만 소유하는 임시 경로와 외부에서 import하지 않는 오류 파일을 만든다.
function fixture() {
    // 이 검사만 생성하고 반납하는 프로젝트 루트다.
    const root = fs.mkdtempSync(path.join(os.tmpdir(), 'netstorm-eslint-'));
    fs.mkdirSync(path.join(root, 'src'));
    fs.mkdirSync(path.join(root, 'dist'));
    fs.writeFileSync(path.join(root, 'eslint.config.cjs'), "module.exports = [{ ignores: ['dist/**'] }, { files: ['src/**/*.js'], rules: { 'no-undef': 'error' } }];\n");
    fs.writeFileSync(path.join(root, 'src/main.js'), 'export default 1;\n');
    fs.writeFileSync(path.join(root, 'src/unimported.js'), 'missingLintProbe();\n');
    fs.writeFileSync(path.join(root, 'dist/bundle.js'), 'previous bundle');
    return root;
}

// 실제 compiler.run을 실행하고 실패 시에도 webpack 자원을 반납한다.
async function compile(config) {
    // 실제 실행과 종료를 모두 기다릴 같은 compiler다.
    const compiler = webpack(config);
    try { return await new Promise((resolve, reject) => compiler.run((error, stats) => error ? reject(error) : resolve(stats))); }
    finally { await new Promise((resolve, reject) => compiler.close((error) => error ? reject(error) : resolve())); }
}

// 모드별 실제 출력을 생성하며 lint 실패 이전에 clean이 실행되지 않도록 같은 플러그인을 설치한다.
function config(root, mode) {
    return { mode, context: root, entry: './src/main.js', output: { path: path.join(root, 'dist'), filename: 'bundle.js', clean: true }, plugins: [new ESLintBeforeBuildPlugin(root)] };
}

// 두 webpack 모드 모두 모듈 그래프 밖의 오류도 검사하고 이전 파일/수정 시각을 보존해야 한다.
for (const mode of ['production', 'development']) {
    test(`${mode}: 미사용 소스 오류가 생성을 막고 수정 뒤 같은 compiler 설정이 통과한다`, async () => {
        // 이 모드 검사만 소유하는 프로젝트다.
        const root = fixture();
        try {
            // 오류를 넣기 전의 실제 산출물과 수정 시각이다.
            const bundle = path.join(root, 'dist/bundle.js');
            const before = fs.statSync(bundle).mtimeMs;
            await assert.rejects(compile(config(root, mode)), /missingLintProbe/);
            assert.equal(fs.readFileSync(bundle, 'utf8'), 'previous bundle');
            assert.equal(fs.statSync(bundle).mtimeMs, before);
            // 권장 규칙의 기존 발생 건수 예외가 새 오류를 허용하지 않는지 실제 CLI로 검사한다.
            fs.writeFileSync(path.join(root, 'eslint-suppressions.json'), JSON.stringify({ 'src/unimported.js': { 'no-undef': { count: 1 } } }));
            fs.writeFileSync(path.join(root, 'src/unimported.js'), 'missingLintProbe(); anotherLintProbe();\n');
            await assert.rejects(compile(config(root, mode)), /anotherLintProbe/);
            assert.equal(fs.readFileSync(bundle, 'utf8'), 'previous bundle');
            assert.equal(fs.statSync(bundle).mtimeMs, before);
            // 고쳐진 오류의 낡은 예외도 npm lint와 동일하게 거부한다.
            fs.writeFileSync(path.join(root, 'src/unimported.js'), 'export default 2;\n');
            await assert.rejects(compile(config(root, mode)), /suppressions left/);
            assert.equal(fs.readFileSync(bundle, 'utf8'), 'previous bundle');
            fs.unlinkSync(path.join(root, 'eslint-suppressions.json'));
            // 같은 설정의 정상 입력은 실제 번들을 다시 만든다.
            const stats = await compile(config(root, mode));
            assert.equal(stats.hasErrors(), false, stats.toString());
            assert.notEqual(fs.readFileSync(bundle, 'utf8'), 'previous bundle');
        } finally {
            // 재귀 삭제 전 절대 경로가 이 검사에서 만든 임시 하위 폴더인지 확인한다.
            assert.equal(path.dirname(path.resolve(root)), path.resolve(os.tmpdir()));
            assert.match(path.basename(root), /^netstorm-eslint-/);
            fs.rmSync(root, { recursive: true, force: true });
        }
    });
}
