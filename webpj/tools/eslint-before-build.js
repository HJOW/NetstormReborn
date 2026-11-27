// webpack의 생성/출력 정리 전에 전체 JavaScript 검사를 완료하는 빌드 관문이다.
'use strict';

// 현재 Node 실행 파일로 ESLint CLI를 호출해 npm lint와 같은 판정을 받는 실행 장치다.
const path = require('node:path');
const { execFile } = require('node:child_process');
const { promisify } = require('node:util');
// 셸을 거치지 않는 비동기 실행기와 현재 설치한 ESLint의 CLI 절대 경로다.
const execute = promisify(execFile);
const ESLINT_CLI = path.join(path.dirname(require.resolve('eslint/package.json')), 'bin', 'eslint.js');

// npm 명령과 같은 전체 범위/설정을 검사하며 오류 또는 경고가 있으면 원문과 함께 실패한다.
async function lintProject(root) {
    // 자동 수정/캐시 없이 전체를 읽고 기존 미사용 선언의 고정 예외와 남은 예외 정리까지 동일하게 검사한다.
    try { await execute(process.execPath, [ESLINT_CLI, '.', '--max-warnings', '0'], { cwd: root, maxBuffer: 16 * 1024 * 1024 }); }
    catch (error) {
        throw new Error(`${error.stdout ?? ''}${error.stderr ?? ''}\n[ESLint] 검사 실패로 번들 생성을 중단합니다.`, { cause: error });
    }
}

// 운영·개발·직접 webpack API 호출에 같은 선행 검사를 설치한다.
class ESLintBeforeBuildPlugin {
    // 실행한 셸의 cwd 대신 검사 대상 프로젝트의 절대 경로를 보관한다.
    #root;

    // 별도 임시 프로젝트 검사에도 같은 플러그인을 적용할 수 있다.
    constructor(root = path.resolve(__dirname, '..')) { this.#root = root; }

    // 단발 실행과 watch의 각 실행 모두 컴파일 전에 실패를 전달해 기존 산출물을 보존한다.
    apply(compiler) {
        compiler.hooks.beforeRun.tapPromise('ESLintBeforeBuildPlugin', () => lintProject(this.#root));
        compiler.hooks.watchRun.tapPromise('ESLintBeforeBuildPlugin', () => lintProject(this.#root));
    }
}

module.exports = { ESLintBeforeBuildPlugin };
