// 작성한 게임·서버·검사·도구에 ESLint 권장 규칙을 적용하는 flat 설정이다.
'use strict';

// ESLint의 공식 권장 규칙과 브라우저/Node 실행 환경의 전역 이름표다.
const js = require('@eslint/js');
const globals = require('globals');

module.exports = [
    // 생성 번들·복사 자료·오라클 원문·로컬 조사/검사 산출물은 작성한 코드에서 제외한다.
    { ignores: ['**/node_modules/**', 'www/dist/**', 'www/data/**', 'tests/fixtures/**', 'extracted/**', 'playwright-report/**', 'test-results*/**'] },
    js.configs.recommended,
    {
        // 원본과 같은 콜백 인수/예외 이름을 보존한다. 그 밖의 미사용 선언 규칙은 계속 활성화한다.
        rules: { 'no-unused-vars': ['error', { args: 'none', caughtErrors: 'none' }] },
        linterOptions: { reportUnusedDisableDirectives: 'error' },
    },
    {
        // package.json의 commonjs 범위에 있는 설정·서버·도구와 Playwright 검사다.
        files: ['*.{js,cjs}', 'tools/**/*.{js,cjs}', 'tests/**/*.{js,cjs}', 'e2e/**/*.{js,cjs}'],
        languageOptions: { sourceType: 'commonjs', globals: globals.node },
    },
    {
        // Node에서 실행하는 ES 모듈 검사와 도구다.
        files: ['*.mjs', 'tools/**/*.mjs', 'tests/**/*.mjs', 'e2e/**/*.mjs'],
        languageOptions: { sourceType: 'module', globals: globals.node },
    },
    {
        // 게임 스크립트는 브라우저 ES 모듈이며 Node 전역을 허용하지 않는다.
        files: ['www/**/*.{js,mjs}'],
        languageOptions: { sourceType: 'module', globals: globals.browser },
    },
    {
        // 이 파일들의 evaluate 콜백은 실제 브라우저에서 실행하므로 두 환경을 함께 허용한다.
        files: ['e2e/**/*.{js,cjs,mjs}', 'tools/ally-ai-campaign-flow.cjs', 'tools/observe-ally-ai.cjs', 'tools/menu-frame-profile.cjs', 'tools/mission-memory-profile.cjs'],
        languageOptions: { globals: globals.browser },
    },
];
