// Git 에 추적하는 webpack 번들(`www/dist/netstorm.bundle.js`)이 현재 소스로 다시 만든 번들과 바이트까지 같은지 확인한다.
// 번들은 사용자 결정(2026-11-24)으로 Git 에 넣으므로 `www` 소스를 고치고 번들을 다시 만들지 않은 채 커밋하면 배포본이 소스와 어긋난다. 이 시험이 그 실수를 잡는다.
// 같은 설정(webpack.config.js 의 운영 모드)으로 임시 폴더에 다시 빌드해(약 20초) 비교하며 `www/dist` 는 건드리지 않는다. 실패하면 `npm --prefix webpj run build` 로 번들을 다시 만들어 함께 커밋한다.
// 크기 한도(9.9MB)도 같이 확인한다. (2026-11-24 WB04-E1297: 번들 Git 추적 결정에 맞춰 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { createRequire } from 'node:module';
import { WEBPJ_ROOT } from './helpers.mjs';

// CommonJS 인 webpack 과 설정을 불러오는 require 이다. (2026-11-24 WB04-E1297)
const require = createRequire(import.meta.url);

test('커밋된 번들은 현재 소스로 다시 만든 운영 번들과 같고 크기 한도 안에 있다', { timeout: 300000 }, async () => {
    const webpack = require('webpack'), makeConfig = require('../webpack.config.js'), { LIMIT_BYTES } = require('../tools/check-bundle-size.js');
    const committedPath = path.join(WEBPJ_ROOT, 'www', 'dist', 'netstorm.bundle.js');
    assert.ok(fs.existsSync(committedPath), '번들이 없습니다: npm --prefix webpj run build');
    const committed = fs.readFileSync(committedPath);
    // 출력만 임시 폴더로 바꿔 같은 설정으로 다시 만든다(clean 을 끄고 www/dist 를 지우지 않는다).
    const output = fs.mkdtempSync(path.join(os.tmpdir(), 'netstorm-bundle-'));
    // 저장소 루트에서 호출해도 npm의 webpj 작업 디렉터리와 같은 모듈 ID를 만들어야 한다.
    const callerDirectory = process.cwd();
    process.chdir(path.dirname(WEBPJ_ROOT));
    try {
        const base = makeConfig({}, { mode: 'production' });
        const stats = await new Promise((resolve, reject) => webpack({ ...base, output: { ...base.output, path: output, clean: false } }, (error, result) => (error ? reject(error) : resolve(result))));
        assert.equal(stats.hasErrors(), false, stats.toString({ all: false, errors: true }));
        const rebuilt = fs.readFileSync(path.join(output, 'netstorm.bundle.js'));
        assert.ok(committed.equals(rebuilt), `커밋된 번들(${committed.length}바이트)이 현재 소스의 번들(${rebuilt.length}바이트)과 다릅니다: npm --prefix webpj run build 후 번들을 함께 커밋하세요.`);
        assert.ok(rebuilt.length <= LIMIT_BYTES, `번들이 크기 한도(${LIMIT_BYTES}바이트)를 넘었습니다: ${rebuilt.length}`);
    } finally { process.chdir(callerDirectory); fs.rmSync(output, { recursive: true, force: true }); }
});
