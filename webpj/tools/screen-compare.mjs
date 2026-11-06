// cpppj 오라클 그림·보고서와 webpj 무브라우저 실행 결과를 비교하는 도구이다(L4a 논리 화면, 보고서 일부 칸).
// 그림은 같은 크기의 RGB 끼리 픽셀 단위로 비교한다(허용 오차 없음). 원인을 기록한 차이는 시나리오 파일의 expected[스냅샷].regions
// ({ rect: [왼, 위, 오른, 아래), pixels, reason })로 적고, 영역 밖 차이는 0이어야 하며 영역 안 차이는 기록한 픽셀 수와 같아야 한다.
// 메인 메뉴 구름 배경은 cpppj(원본 Kencloud 출력)와 웹(GIF 단순 타일)이 행마다 1픽셀씩 어긋나는 알려진 차이라서, 시나리오 묶음의 background 칸이 가리키는
// 메인 메뉴 스냅샷으로 '구름 차이'를 정밀 분류해 expected[스냅샷].background({ pixels, reason })와 맞춘다(backgroundClassifier 설명 참조).
// 보고서는 cpppj `--ui-report` 의 같은 라벨 구간에서 COMPARED_REPORT_KEYS 칸만 웹 보고서와 비교한다. 원인을 기록한 차이는 expected[라벨].report 에 적는다.
// 사용:
//   node webpj/tools/screen-compare.mjs [--oracle webpj/tests/fixtures/cpppj/m1] [--id campaign] [--diff <폴더>]
//     → 시나리오마다 웹을 무브라우저로 실행해 저장된 오라클과 비교하고 결과를 출력한다. --diff 를 주면 차이 그림(같은 픽셀은 어둡게, 다른 픽셀은 자홍)을 PNG 로 쓴다.
// 이 도구는 cpppj 를 실행하지 않는다. 오라클 수집은 tools/cpp-oracle.mjs 가 한다.
// (2026-11-07 WB02-H: LEFT_JOBS.webpj.md 3.2절 순서2 ③의 화면/보고서 비교 요구로 추가)
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { readScenarios, runScenario } from './headless.mjs';
import { encodeIndexedPng, readPng } from './image-files.mjs';

// 저장소 루트의 절대 경로이다. (2026-11-07 WB02-H)
const REPO_ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');

// 기본 오라클 폴더(저장소 상대)이다. (2026-11-07 WB02-H)
export const DEFAULT_ORACLE_DIR = 'webpj/tests/fixtures/cpppj/m1';

// 오라클 폴더 안의 시나리오 파일 이름과 메타 파일 이름이다. (2026-11-07 WB02-H)
export const SCENARIOS_FILE = 'm1-scenarios.json';
export const ORACLE_META_FILE = 'oracle.json';

// 보고서에서 비교하는 칸 이름이다. 웹 보고서(tools/headless.mjs report)가 같은 이름으로 쓰는 칸만 둔다.
// cpppj 의 popup 칸은 옛 호스트 보조 메뉴 이름이라 10.78 메인 Help/Options 펼침 메뉴가 열려도 비어 있으므로 비교하지 않는다(웹은 web_popup 으로 따로 기록). (2026-11-07 WB02-H)
export const COMPARED_REPORT_KEYS = Object.freeze(['size', 'page']);

// 두 그림의 같은 위치 픽셀이 같은지 본다(at 은 RGB 배열 위치). (2026-11-07 WB02-H)
function samePixel(a, b, at) {
    return a.rgb[at] === b.rgb[at] && a.rgb[at + 1] === b.rgb[at + 1] && a.rgb[at + 2] === b.rgb[at + 2];
}

// 구름 배경 분류 자료를 만든다. 웹 메인 메뉴 화면에서 구름이 그대로 보이는 위치(webMain == webBackground)를 배경 위치로 본다.
// 웹 원본 비교 모드의 메인 메뉴 제목·8버튼은 cpppj 와 픽셀까지 같으므로(2026-11-07 대조) 이 위치들은 cpppj 메인 메뉴에서도 구름이 보이는 곳이다.
// 반환: { mask: Uint8Array(픽셀마다 1=배경 위치), webBackground, cppBackground(cpppj 메인 메뉴 오라클) }. (2026-11-07 WB02-H)
export function backgroundClassifier(webMain, webBackground, cppMain) {
    const mask = new Uint8Array(webMain.width * webMain.height);
    // 픽셀마다 웹 메인 메뉴가 구름 그대로인지 기록한다.
    for (let index = 0; index < mask.length; index += 1) mask[index] = samePixel(webMain, webBackground, index * 3) ? 1 : 0;
    return { mask, webBackground, cppBackground: cppMain };
}

// 두 RGB 그림을 비교한다. regions 는 [{ rect: [l, t, r, b] }] 이며 한 픽셀은 처음 포함하는 영역에 센다.
// background(backgroundClassifier 결과)를 주면, 배경 위치에서 웹 픽셀이 웹 구름과 같고 cpppj 픽셀이 cpppj 메인 메뉴 구름과 같은 차이는 '구름 차이'로 따로 센다.
// 웹이 그려야 할 그림을 빠뜨리거나(웹=구름, cpppj=그림) cpppj 에만 그림이 있으면 cpppj 쪽 조건이 깨져 구름 차이로 세지 않는다.
// 반환: { sizeMatch, total, outside, outsideBox([l,t,r,b] 또는 null), regions: [그 영역의 차이 수], background(구름 차이 수) }. 크기가 다르면 sizeMatch false 이다. (2026-11-07 WB02-H)
export function compareImages(expected, actual, regions = [], background = null) {
    if (expected.width !== actual.width || expected.height !== actual.height) return { sizeMatch: false, total: -1, outside: -1, outsideBox: null, regions: regions.map(() => -1), background: -1 };
    const counts = regions.map(() => 0);
    let total = 0;
    let outside = 0;
    let clouds = 0;
    let box = null;
    // 픽셀마다 RGB 세 값을 비교한다.
    for (let y = 0; y < expected.height; y += 1) {
        // 한 행의 픽셀을 비교한다.
        for (let x = 0; x < expected.width; x += 1) {
            const index = y * expected.width + x;
            const at = index * 3;
            if (samePixel(expected, actual, at)) continue;
            total += 1;
            if (background && background.mask[index] && samePixel(actual, background.webBackground, at) && samePixel(expected, background.cppBackground, at)) { clouds += 1; continue; }
            const region = regions.findIndex(({ rect }) => x >= rect[0] && x < rect[2] && y >= rect[1] && y < rect[3]);
            if (region >= 0) { counts[region] += 1; continue; }
            outside += 1;
            box = box ? [Math.min(box[0], x), Math.min(box[1], y), Math.max(box[2], x + 1), Math.max(box[3], y + 1)] : [x, y, x + 1, y + 1];
        }
    }
    return { sizeMatch: true, total, outside, outsideBox: box, regions: counts, background: clouds };
}

// 차이 그림을 만든다. 같은 픽셀은 오라클 색을 1/4 밝기로, 다른 픽셀은 자홍(255,0,255)으로 칠한다. 사람 확인용이다. (2026-11-07 WB02-H)
export function diffImage(expected, actual) {
    const rgb = new Uint8Array(expected.rgb.length);
    // 픽셀마다 같음/다름을 칠한다.
    for (let index = 0; index < expected.width * expected.height; index += 1) {
        const at = index * 3;
        const same = expected.rgb[at] === actual.rgb[at] && expected.rgb[at + 1] === actual.rgb[at + 1] && expected.rgb[at + 2] === actual.rgb[at + 2];
        if (same) rgb.set([expected.rgb[at] >> 2, expected.rgb[at + 1] >> 2, expected.rgb[at + 2] >> 2], at);
        else rgb.set([255, 0, 255], at);
    }
    return { width: expected.width, height: expected.height, rgb };
}

// cpppj `--ui-report` 글을 라벨별 칸 표로 나눈다. 구간은 `frame\t프레임\t라벨` 줄로 시작한다. 반환: Map(라벨 → Map(칸 → 탭 뒤 값)). (2026-11-07 WB02-H)
export function parseCppReport(text) {
    const sections = new Map();
    let current = null;
    // 줄마다 구간 머리 또는 칸으로 읽는다.
    for (const line of text.split(/\r?\n/)) {
        const head = /^frame\t\d+\t(.*)$/.exec(line);
        if (head) { current = new Map(); sections.set(head[1], current); continue; }
        if (!current || !line) continue;
        const tab = line.indexOf('\t');
        const key = tab < 0 ? line : line.slice(0, tab);
        if (!current.has(key)) current.set(key, tab < 0 ? '' : line.slice(tab + 1));
    }
    return sections;
}

// 웹 무브라우저 보고서 글(칸 줄 목록)을 칸 표로 읽는다. (2026-11-07 WB02-H)
export function parseWebReport(text) {
    const fields = new Map();
    // 줄마다 첫 탭 앞을 칸 이름으로 읽는다.
    for (const line of text.split(/\r?\n/)) {
        if (!line) continue;
        const tab = line.indexOf('\t');
        fields.set(tab < 0 ? line : line.slice(0, tab), tab < 0 ? '' : line.slice(tab + 1));
    }
    return fields;
}

// 오라클 폴더별 구름 배경 분류 자료의 캐시이다(시나리오마다 다시 계산하지 않기 위해서). (2026-11-07 WB02-H)
const backgroundCache = new Map();

// 시나리오 묶음의 background 칸({ scenario, snapshot }: 기준 메인 메뉴 스냅샷)으로 구름 배경 분류 자료를 만든다. 칸이 없으면 null 이다. (2026-11-07 WB02-H)
export async function loadBackground(oracleDir, data) {
    if (!data.background) return null;
    const key = `${oracleDir}|${data.background.scenario}|${data.background.snapshot}`;
    if (!backgroundCache.has(key)) {
        const reference = data.scenarios.find((entry) => entry.id === data.background.scenario);
        if (!reference) throw new Error(`구름 기준 시나리오 없음: ${data.background.scenario}`);
        const web = await runScenario(reference, data);
        const cppMain = readPng(fs.readFileSync(path.join(oracleDir, reference.id, `${data.background.snapshot}.png`)));
        backgroundCache.set(key, backgroundClassifier(web.snapshots[data.background.snapshot], web.background, cppMain));
    }
    return backgroundCache.get(key);
}

// 오라클 폴더의 시나리오 하나를 웹으로 실행해 비교한다. 반환: { id, snapshots: [{ name, result, regions, background }], reports: [{ label, key, cpp, web, expected }] }.
// background 는 expected[스냅샷].background({ pixels, reason }) 기록이다. diffDir 를 주면 차이가 있는 스냅샷의 차이 그림을 쓴다. (2026-11-07 WB02-H)
export async function compareScenario(oracleDir, data, scenario, { diffDir = null } = {}) {
    const classifier = await loadBackground(oracleDir, data);
    const web = await runScenario(scenario, data);
    const expected = scenario.expected ?? {};
    const snapshots = [];
    // 스냅샷마다 오라클 PNG 와 비교한다.
    for (const [name, image] of Object.entries(web.snapshots)) {
        const oracle = readPng(fs.readFileSync(path.join(oracleDir, scenario.id, `${name}.png`)));
        const regions = expected[name]?.regions ?? [];
        const result = compareImages(oracle, image, regions, classifier);
        snapshots.push({ name, result, regions, background: expected[name]?.background ?? null });
        if (diffDir && result.total !== 0 && result.sizeMatch) {
            fs.mkdirSync(diffDir, { recursive: true });
            fs.writeFileSync(path.join(diffDir, `${scenario.id}-${name}.diff.png`), encodeIndexedPng(diffImage(oracle, image)));
            fs.writeFileSync(path.join(diffDir, `${scenario.id}-${name}.web.png`), encodeIndexedPng(image));
        }
    }
    const cppReport = parseCppReport(fs.readFileSync(path.join(oracleDir, scenario.id, 'report.txt'), 'utf8'));
    const reports = [];
    // 보고 라벨마다 비교 칸을 맞춘다.
    for (const [label, text] of Object.entries(web.reports)) {
        const cpp = cppReport.get(label) ?? new Map();
        const webFields = parseWebReport(text);
        for (const key of COMPARED_REPORT_KEYS) reports.push({ label, key, cpp: cpp.get(key) ?? null, web: webFields.get(key) ?? null, expected: expected[label]?.report?.[key] ?? null });
    }
    return { id: scenario.id, snapshots, reports };
}

// 비교 결과가 기록과 맞는지 판정한다. 반환: 문제 문장 배열(비면 통과). (2026-11-07 WB02-H)
export function judge(comparison) {
    const problems = [];
    // 스냅샷 조건: 크기 같음, 영역 밖 0, 영역 안과 구름 차이는 기록한 픽셀 수와 같음.
    for (const { name, result, regions, background } of comparison.snapshots) {
        if (!result.sizeMatch) { problems.push(`${comparison.id}/${name}: 크기 다름`); continue; }
        if (result.outside !== 0) problems.push(`${comparison.id}/${name}: 기록 영역 밖 차이 ${result.outside}픽셀 ${JSON.stringify(result.outsideBox)}`);
        if (result.background !== (background?.pixels ?? 0)) problems.push(`${comparison.id}/${name}: 구름 차이 ${result.background} (기록 ${background?.pixels ?? 0})`);
        regions.forEach((region, index) => {
            if (result.regions[index] !== region.pixels) problems.push(`${comparison.id}/${name}: 영역 ${JSON.stringify(region.rect)} 차이 ${result.regions[index]} (기록 ${region.pixels})`);
        });
    }
    // 보고서 조건: 같은 값이거나, 다르면 그 칸의 원인이 기록되어 있고 기록한 웹 값과 같음.
    for (const row of comparison.reports) {
        if (row.cpp === row.web) { if (row.expected) problems.push(`${comparison.id}/${row.label}: ${row.key} 가 이제 같음(기록 정리 필요)`); continue; }
        if (!row.expected || row.expected.cpp !== row.cpp || row.expected.web !== row.web) problems.push(`${comparison.id}/${row.label}: ${row.key} cpp=${JSON.stringify(row.cpp)} web=${JSON.stringify(row.web)}`);
    }
    return problems;
}

// 명령줄 진입점이다. (2026-11-07 WB02-H)
async function main(argv) {
    const option = (name) => { const at = argv.indexOf(name); return at >= 0 ? argv[at + 1] : null; };
    const oracleDir = path.resolve(REPO_ROOT, option('--oracle') ?? DEFAULT_ORACLE_DIR);
    const data = readScenarios(path.join(oracleDir, '..', SCENARIOS_FILE));
    const only = option('--id');
    const diffDir = option('--diff') ? path.resolve(option('--diff')) : null;
    let failed = 0;
    // 고른 시나리오를 차례로 비교한다.
    for (const scenario of data.scenarios.filter((entry) => !only || entry.id === only)) {
        const comparison = await compareScenario(oracleDir, data, scenario, { diffDir });
        for (const { name, result } of comparison.snapshots) console.log(`${scenario.id}/${name}\t전체 ${result.total}\t구름 ${result.background}\t영역 밖 ${result.outside}\t${JSON.stringify(result.outsideBox)}\t영역 ${JSON.stringify(result.regions)}`);
        for (const row of comparison.reports) if (row.cpp !== row.web) console.log(`${scenario.id}/${row.label}\t보고 ${row.key}\tcpp=${JSON.stringify(row.cpp)}\tweb=${JSON.stringify(row.web)}`);
        const problems = judge(comparison);
        failed += problems.length;
        for (const problem of problems) console.log(`판정 실패\t${problem}`);
    }
    console.log(failed === 0 ? '모든 비교가 기록과 일치' : `기록과 다른 비교 ${failed}건`);
    return failed === 0 ? 0 : 1;
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) process.exitCode = await main(process.argv.slice(2));
