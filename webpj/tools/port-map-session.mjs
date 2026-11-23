// 이식 대응표(webpj/PORT_MAP.tsv)의 `port` 행을 `session`(앱 수명 연결)으로 올리는 도구이다.
// 기준: `e2e/session-coverage.spec.js` 가 고정 시각 Tutorial1(4:3) 130프레임 실행 중 V8 함수 커버리지로 모은 JSON 에서, 행의 웹 모듈이 **모두** 함수를 하나 이상 실제로 실행한 행만 올린다.
// 자료만 들고 있어 함수가 없는 모듈이나 이 흐름에 쓰이지 않는 모듈이 하나라도 있으면 `port` 로 둔다(보수적 판정). `partial`·제외(`-`)·이미 `session`/`oracle` 인 행은 건드리지 않는다.
// 사용(저장소 루트 또는 webpj 에서):
//   NETSTORM_COVERAGE_OUT=extracted/coverage.json npx playwright test e2e/session-coverage.spec.js   → 커버리지 JSON 수집(옵트인 시험)
//   node webpj/tools/port-map-session.mjs --coverage extracted/coverage.json            → 올릴 행 보고(바꾸지 않음)
//   node webpj/tools/port-map-session.mjs --coverage extracted/coverage.json --write    → 대응표 상태 칸을 저장한다(줄바꿈·머리 행·다른 칸은 그대로 둔다).
// 이 도구는 로컬 파일만 읽고 쓰며 네트워크를 쓰지 않는다. (2026-11-23 WB04-E1282)
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

// 저장소 루트와 대응표 경로이다.
const REPO_ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');
const MAP_FILE = path.join(REPO_ROOT, 'webpj', 'PORT_MAP.tsv');

// 대응표 note 칸 끝에 붙이는 승격 근거 표식이다.
export const SESSION_MARK = '[session WB04-E1282 Tutorial1 130프레임 실행]';

// 커버리지 JSON 과 대응표 텍스트로 승격할 행을 계산한다. 반환: { lines(새 텍스트의 줄 배열), promoted(행 수), kept(port 로 남은 행 수), reasons(남은 이유별 개수) }.
// 사용: planSession(tsvText, coverage). 입력은 바꾸지 않는다.
export function planSession(tsvText, coverage) {
    const eol = tsvText.includes('\r\n') ? '\r\n' : '\n';
    const lines = tsvText.split(/\r?\n/);
    let promoted = 0, kept = 0;
    const reasons = { noModule: 0, notExecuted: 0, missingFromCoverage: 0 };
    // 줄마다 자료 행이면 상태가 port 인 행을 판정한다.
    const out = lines.map((line) => {
        if (!line || line.startsWith('#')) return line;
        const cells = line.split('\t');
        if (cells.length < 6 || cells[4] !== 'port') return line;
        const modules = cells[2].split(';').map((name) => name.trim()).filter(Boolean);
        if (modules.length === 0) { kept += 1; reasons.noModule += 1; return line; }
        const missing = modules.filter((name) => !coverage.modules[name]);
        if (missing.length > 0) { kept += 1; reasons.missingFromCoverage += 1; return line; }
        if (modules.some((name) => !(coverage.modules[name].executed > 0))) { kept += 1; reasons.notExecuted += 1; return line; }
        promoted += 1;
        cells[4] = 'session';
        cells[5] = cells[5].includes(SESSION_MARK) ? cells[5] : `${cells[5]} ${SESSION_MARK}`;
        return cells.join('\t');
    });
    return { text: out.join(eol), promoted, kept, reasons };
}

// 명령줄 진입점이다.
function main(argv) {
    const option = (name) => { const at = argv.indexOf(name); return at >= 0 ? argv[at + 1] : null; };
    const coveragePath = option('--coverage');
    if (!coveragePath) { console.error('사용: node webpj/tools/port-map-session.mjs --coverage <json> [--write]'); return 1; }
    const coverage = JSON.parse(fs.readFileSync(path.resolve(coveragePath), 'utf8'));
    const text = fs.readFileSync(MAP_FILE, 'utf8');
    const plan = planSession(text, coverage);
    console.log(`승격 ${plan.promoted}행, port 로 남음 ${plan.kept}행(웹 모듈 없음 ${plan.reasons.noModule}·커버리지에 없음 ${plan.reasons.missingFromCoverage}·함수 미실행 ${plan.reasons.notExecuted})`);
    if (argv.includes('--write')) { fs.writeFileSync(MAP_FILE, plan.text, 'utf8'); console.log('대응표를 저장했습니다.'); }
    return 0;
}

// 직접 실행했을 때만 진입점을 부른다(시험에서 planSession 을 가져다 쓸 수 있게).
if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) process.exitCode = main(process.argv.slice(2));
