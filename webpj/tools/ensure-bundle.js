// webpack 번들이 최신인지 확인하고 없거나 소스보다 오래됐으면 운영 번들을 다시 만든다.
// 번들(`www/dist/netstorm.bundle.js`)은 소스에서 만드는 산출물이지만 Git 에 추적한다(사용자 결정 2026-11-24, 시험 tests/bundle-fresh.test.mjs 가 최신 여부를 확인). 서버 시작(`npm start`)·e2e 시험 전에 이 도구가 최신 번들을 보장한다.
// 판정: `www` 아래 .js(data·dist 제외), webpack/lint 설정·선행 관문·패키지의 수정 시각이 번들보다 늦으면 다시 만든다.
// 사용(webpj 폴더에서): `node tools/ensure-bundle.js`. 다시 만든 뒤에는 크기 한도(tools/check-bundle-size.js)도 검사한다.
// (2026-11-24 WB04-E1293: 번들 도입 뒤 낡은 번들로 시험·실행하는 일을 막으려고 추가)
'use strict';

const fs = require('fs');
const path = require('path');
const { spawnSync } = require('child_process');
const { BUNDLE_PATH, checkBundleSize } = require('./check-bundle-size.js');

// webpj 폴더의 절대 경로이다. (2026-11-24 WB04-E1293)
const ROOT = path.join(__dirname, '..');

// 폴더 아래 .js 파일의 가장 늦은 수정 시각(ms)을 구한다. data(자료 복사본)·dist(산출물)·node_modules 는 소스가 아니므로 건너뛴다.
function newestSource(directory, newest = 0) {
    for (const entry of fs.readdirSync(directory, { withFileTypes: true })) {
        const full = path.join(directory, entry.name);
        if (entry.isDirectory()) { if (!['data', 'dist', 'node_modules'].includes(entry.name)) newest = newestSource(full, newest); }
        else if (entry.name.endsWith('.js')) newest = Math.max(newest, fs.statSync(full).mtimeMs);
    }
    return newest;
}

// 번들이 최신이면 true, 없거나 오래됐으면 false 이다.
function bundleIsFresh() {
    if (!fs.existsSync(BUNDLE_PATH)) return false;
    const newest = Math.max(newestSource(path.join(ROOT, 'www')), ...['webpack.config.js', 'eslint.config.cjs', 'eslint-suppressions.json', 'tools/eslint-before-build.js', 'package.json', 'package-lock.json'].map((name) => fs.statSync(path.join(ROOT, name)).mtimeMs));
    return fs.statSync(BUNDLE_PATH).mtimeMs >= newest;
}

if (require.main === module) {
    if (bundleIsFresh()) { console.log('[번들] 최신입니다.'); process.exit(checkBundleSize() ? 0 : 1); }
    console.log('[번들] 없거나 소스보다 오래되어 webpack 으로 다시 만듭니다.');
    const run = spawnSync(process.execPath, [path.join(ROOT, 'node_modules', 'webpack', 'bin', 'webpack.js'), '--mode', 'production'], { cwd: ROOT, stdio: 'inherit' });
    if (run.status !== 0) process.exit(run.status ?? 1);
    process.exit(checkBundleSize() ? 0 : 1);
}
module.exports = { bundleIsFresh };
