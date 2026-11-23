// webpack 번들 크기 한도 검사 도구이다. `www/dist/netstorm.bundle.js` 의 바이트 수를 한도(9.9MB)와 견주어 넘으면 종료 코드 1 로 끝낸다.
// 한도는 사용자가 정한 롤백 기준(번들이 "9.9MB 정도"를 넘으면 도입을 되돌릴지 검토)이며 10진 MB(9,900,000바이트)로 엄격하게 잡는다.
// 사용(webpj 폴더에서): `node tools/check-bundle-size.js` 또는 `npm run check-bundle`. 번들이 없으면 먼저 `npm run build` 가 필요하다.
// (2026-11-24 WB04-E1293: webpack 도입의 크기 한도를 자동으로 확인하려고 추가)
'use strict';

const fs = require('fs');
const path = require('path');

// 번들 크기 한도(바이트)이다. 사용자가 정한 기준 9.9MB 를 10진 단위로 쓴다. (2026-11-24 WB04-E1293)
const LIMIT_BYTES = 9_900_000;

// 검사할 번들 파일 경로이다. (2026-11-24 WB04-E1293)
const BUNDLE_PATH = path.join(__dirname, '..', 'www', 'dist', 'netstorm.bundle.js');

// 번들 크기를 검사하고 결과를 한 줄로 출력한다. 한도 이하이면 true, 넘거나 번들이 없으면 false 이다.
function checkBundleSize() {
    if (!fs.existsSync(BUNDLE_PATH)) { console.error(`[번들 크기] 번들이 없습니다: ${BUNDLE_PATH} (npm run build 필요)`); return false; }
    const bytes = fs.statSync(BUNDLE_PATH).size;
    const mb = (bytes / 1_000_000).toFixed(2), mib = (bytes / 1_048_576).toFixed(2), ok = bytes <= LIMIT_BYTES;
    console.log(`[번들 크기] ${bytes.toLocaleString('en-US')}바이트 (${mb}MB / ${mib}MiB), 한도 ${LIMIT_BYTES.toLocaleString('en-US')}바이트(9.9MB): ${ok ? '통과' : '초과 — 롤백을 검토하세요'}`);
    return ok;
}

if (require.main === module) process.exit(checkBundleSize() ? 0 : 1);
module.exports = { LIMIT_BYTES, BUNDLE_PATH, checkBundleSize };
