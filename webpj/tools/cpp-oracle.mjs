// cpppj 기준선 실행 파일로 오라클(논리 화면·관측 보고서)을 수집해 webpj/tests/fixtures/cpppj/ 아래에 저장하는 도구이다.
// 시나리오 묶음(tests/fixtures/cpppj/m1-scenarios.json)의 단계표를 cpppj `--run <자료> --window --mute-audio --frames N --set ... --ui-script ... --ui-report ...` 로 실행한다.
//  - 시나리오마다 새 프로세스와 빈 사용자 루트(NETSTORM_USER_DATA, 임시 폴더)를 써서 설정·진행 파일이 결과에 섞이지 않게 한다.
//  - 소리는 `--mute-audio`(장치는 열고 소리만 끔)로 끈다. `--no-audio`·NETSTORM_CPP_NO_AUDIO 는 소리 장치가 없는 상태라 Options 의 소리 항목이 비활성으로 그려져
//    원본(소리 장치 있음)과 다른 화면이 되므로 쓰지 않는다(2026-11-07 첫 수집에서 확인). 환경에 NETSTORM_CPP_NO_AUDIO 가 있으면 지운다.
//  - snapshot 의 BMP(122바이트 머리 32비트)는 색이 256개 이하이므로 무손실 팔레트 PNG 로 바꿔 <출력>/<id>/<이름>.png 에 두고, 원래 BMP 의 SHA-256 은 메타에 남긴다.
//  - 보고서는 <출력>/<id>/report.txt 에 그대로 둔다.
//  - 메타 <출력>/oracle.json: cpppj 커밋·EXE SHA-256/크기·명령줄·자료 폴더 해시·웹 자료와의 같은 파일 대조·수집 PC/시각·파일별 SHA-256.
// 창이 뜨는 실행이므로 AGENTS.md 의 실행 확인 규칙을 따른다(예외 PC: HJOW-Athlon·VM-W11-Codex·vm-debian-codex). npm test 는 이 도구를 부르지 않고 저장된 결과만 읽는다.
// 사용(저장소 루트):
//   node webpj/tools/cpp-oracle.mjs --exe extracted/webpj-oracle/1f00a25a/Netstorm.exe [--commit 1f00a25a] [--game assets/game-data] [--id campaign] [--out webpj/tests/fixtures/cpppj/m1]
// (2026-11-07 WB02-H: LEFT_JOBS.webpj.md 3.2절 순서2 ②의 cpppj 오라클 수집 요구로 추가)
import crypto from 'node:crypto';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import { BASELINE } from './port-map.mjs';
import { collectionInputs, readScenarios } from './headless.mjs';
import { encodeIndexedPng, readBmp } from './image-files.mjs';
import { DEFAULT_ORACLE_DIR, ORACLE_META_FILE, SCENARIOS_FILE } from './screen-compare.mjs';

// 저장소 루트의 절대 경로이다. (2026-11-07 WB02-H)
const REPO_ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');

// cpppj 실행 파일이 쓰는 기본 자료 폴더(저장소 상대)이다. webpj/www/data 의 원본 자산은 이 폴더의 복사본이다. (2026-11-07 WB02-H)
export const DEFAULT_GAME_DIR = 'assets/game-data';

// 웹 자료 폴더(저장소 상대)이다. 같은 파일 대조에 쓴다. (2026-11-07 WB02-H)
const WEB_DATA_DIR = 'webpj/www/data';

// 시나리오 한 번의 실행 시간 한도(밀리초)이다. (2026-11-07 WB02-H)
const RUN_TIMEOUT_MS = 120_000;

// 메타 형식 이름이다. (2026-11-07 WB02-H)
export const ORACLE_FORMAT = 'netstorm-webpj-cpppj-oracle';

// 바이트의 SHA-256 16진 문자열이다. (2026-11-07 WB02-H)
function sha256(bytes) {
    return crypto.createHash('sha256').update(bytes).digest('hex');
}

// 폴더 아래 모든 파일의 상대 경로(/) → SHA-256 표를 만든다. 정렬된 배열로 돌려준다. (2026-11-07 WB02-H)
function hashTree(root) {
    const out = [];
    // 깊이 우선으로 파일을 모은다.
    const walk = (directory) => {
        for (const entry of fs.readdirSync(directory, { withFileTypes: true })) {
            const full = path.join(directory, entry.name);
            if (entry.isDirectory()) walk(full);
            else out.push([path.relative(root, full).replaceAll('\\', '/'), sha256(fs.readFileSync(full))]);
        }
    };
    walk(root);
    return out.sort(([a], [b]) => a.localeCompare(b, 'en'));
}

// 설정 표를 cpppj --set 문자열(`키=값;키=값`)로 만든다. 값에 ; 나 줄바꿈이 있으면 Error 이다. (2026-11-07 WB02-H)
export function settingsArgument(settings) {
    return Object.entries(settings).map(([key, value]) => {
        if (/[;=\r\n]/.test(key) || /[;\r\n]/.test(String(value))) throw new Error(`--set 에 넣을 수 없는 설정: ${key}`);
        return `${key}=${value}`;
    }).join(';');
}

// 시나리오 하나를 cpppj 로 실행한다. 반환: { args, snapshots: { 이름: { bmp(바이트) } }, report(문자열), stdout, stderr }. 실패하면 Error 이다. (2026-11-07 WB02-H)
export function runCppScenario({ exe, gameDir, shared, scenario }) {
    const work = fs.mkdtempSync(path.join(os.tmpdir(), `netstorm-oracle-${scenario.id}-`));
    const userData = path.join(work, 'user');
    fs.mkdirSync(userData);
    const stepsFile = path.join(work, 'steps.tsv');
    const reportFile = path.join(work, 'report.tsv');
    const captures = {};
    // 단계표를 cpppj 형식(프레임\t동작\t인자)으로 쓴다. snapshot 인자는 임시 BMP 절대 경로로 바꾼다.
    const lines = scenario.steps.map(([frame, operation, argument = '']) => {
        if (operation === 'snapshot') { captures[argument] = path.join(work, `${argument}.bmp`); return `${frame}\t${operation}\t${captures[argument]}`; }
        return `${frame}\t${operation}\t${argument}`;
    });
    fs.writeFileSync(stepsFile, `${lines.join('\n')}\n`, 'utf8');
    const frames = Math.max(scenario.frames ?? 0, ...scenario.steps.map(([frame]) => frame)) + 1;
    const args = ['--run', gameDir, '--window', '--mute-audio', '--frames', String(frames), '--set', settingsArgument({ ...(shared.settings ?? {}), ...(scenario.settings ?? {}) }), '--ui-script', stepsFile, '--ui-report', reportFile];
    const env = { ...process.env, NETSTORM_USER_DATA: userData };
    delete env.NETSTORM_CPP_NO_AUDIO;
    const run = spawnSync(exe, args, { cwd: path.dirname(exe), env, timeout: RUN_TIMEOUT_MS, encoding: 'utf8', windowsHide: false });
    if (run.error) throw run.error;
    if (run.status !== 0) throw new Error(`cpppj 종료 코드 ${run.status} (${scenario.id})\n${run.stderr}`);
    const snapshots = {};
    // 캡처마다 BMP 바이트를 읽는다.
    for (const [name, file] of Object.entries(captures)) {
        if (!fs.existsSync(file)) throw new Error(`cpppj 캡처 없음: ${name}`);
        snapshots[name] = { bmp: fs.readFileSync(file) };
    }
    const report = fs.existsSync(reportFile) ? fs.readFileSync(reportFile, 'utf8') : '';
    fs.rmSync(work, { recursive: true, force: true });
    // 명령줄 기록에는 임시 경로 대신 자리 표시를 쓴다(PC 마다 다른 경로를 남기지 않기 위해).
    const recorded = args.map((value) => (value === stepsFile ? '<steps.tsv>' : value === reportFile ? '<report.tsv>' : value === gameDir ? path.relative(REPO_ROOT, gameDir).replaceAll('\\', '/') : value));
    return { args: recorded, snapshots, report, stdout: run.stdout, stderr: run.stderr };
}

// 자료 폴더와 웹 자료 폴더의 같은 이름 파일을 대조한다. 반환: { gameFiles, gameTreeSha256, compared, mismatched[], missingInWeb(웹이 복사하지 않은 파일 수) }.
// 웹은 메뉴·한국어 묶음에 필요한 파일만 복사하므로 웹에 없는 파일은 차이로 세지 않고 개수만 남긴다. (2026-11-07 WB02-H)
export function compareDataTrees(gameDir) {
    const game = hashTree(gameDir);
    const web = new Map(hashTree(path.join(REPO_ROOT, WEB_DATA_DIR)));
    const mismatched = [];
    let compared = 0;
    // 자료 폴더의 파일마다 웹 복사본과 비교한다(웹에 없는 파일은 세지 않는다).
    for (const [file, hash] of game) {
        if (!web.has(file)) continue;
        compared += 1;
        if (web.get(file) !== hash) mismatched.push(file);
    }
    return { gameFiles: game.length, gameTreeSha256: sha256(game.map(([file, hash]) => `${file}\t${hash}\n`).join('')), compared, mismatched, missingInWeb: game.filter(([file]) => !web.has(file)).length };
}

// 명령줄 진입점이다. 고른 시나리오를 수집해 PNG·보고서·메타를 쓴다. 기존 메타가 있으면 다른 시나리오 기록은 보존한다. (2026-11-07 WB02-H)
function main(argv) {
    const option = (name) => { const at = argv.indexOf(name); return at >= 0 ? argv[at + 1] : null; };
    const exe = option('--exe') ? path.resolve(option('--exe')) : null;
    if (!exe || !fs.existsSync(exe)) { console.error('사용: node webpj/tools/cpp-oracle.mjs --exe <Netstorm.exe> [--commit 커밋] [--game 자료폴더] [--id 시나리오] [--out 출력폴더]'); return 1; }
    const commit = option('--commit') ?? BASELINE;
    const gameDir = path.resolve(REPO_ROOT, option('--game') ?? DEFAULT_GAME_DIR);
    const outDir = path.resolve(REPO_ROOT, option('--out') ?? DEFAULT_ORACLE_DIR);
    const scenariosPath = path.join(outDir, '..', SCENARIOS_FILE);
    const data = readScenarios(scenariosPath);
    const only = option('--id');
    const exeBytes = fs.readFileSync(exe);
    const metaPath = path.join(outDir, ORACLE_META_FILE);
    const previous = fs.existsSync(metaPath) ? JSON.parse(fs.readFileSync(metaPath, 'utf8')) : null;
    const meta = {
        format: ORACLE_FORMAT,
        version: 1,
        description: 'cpppj 기준선 실행 파일로 수집한 M1 메인 메뉴 오라클. 그림은 cpppj 논리 화면 캡처(BMP)를 무손실 팔레트 PNG 로 바꾼 것이다. tools/cpp-oracle.mjs 가 쓴다.',
        cpppjCommit: commit,
        exe: { file: path.basename(exe), sha256: sha256(exeBytes), bytes: exeBytes.length },
        scenarios: path.relative(REPO_ROOT, scenariosPath).replaceAll('\\', '/'),
        // 수집 입력(언어·화면·설정·단계표)만의 해시이다. 비교 기록(expected)을 고쳐도 바뀌지 않는다.
        collectionInputsSha256: sha256(Buffer.from(collectionInputs(data), 'utf8')),
        data: compareDataTrees(gameDir),
        collected: { host: os.hostname(), at: new Date().toISOString(), node: process.version },
        results: previous && previous.exe?.sha256 === sha256(exeBytes) ? previous.results : {},
    };
    if (meta.data.mismatched.length > 0) console.warn(`경고: 자료 폴더와 웹 자료가 다른 파일 ${meta.data.mismatched.length}개: ${meta.data.mismatched.join(' ')}`);
    // 고른 시나리오마다 실행해 결과를 저장한다.
    for (const scenario of data.scenarios.filter((entry) => !only || entry.id === only)) {
        const result = runCppScenario({ exe, gameDir, shared: data, scenario });
        const folder = path.join(outDir, scenario.id);
        fs.mkdirSync(folder, { recursive: true });
        const snapshots = {};
        // 캡처마다 PNG 로 바꿔 쓴다.
        for (const [name, { bmp }] of Object.entries(result.snapshots)) {
            const image = readBmp(bmp);
            const png = encodeIndexedPng(image);
            fs.writeFileSync(path.join(folder, `${name}.png`), png);
            snapshots[name] = { png: `${scenario.id}/${name}.png`, pngSha256: sha256(png), bmpSha256: sha256(bmp), width: image.width, height: image.height };
        }
        fs.writeFileSync(path.join(folder, 'report.txt'), result.report, 'utf8');
        meta.results[scenario.id] = { args: result.args, snapshots, report: { file: `${scenario.id}/report.txt`, sha256: sha256(Buffer.from(result.report, 'utf8')) } };
        console.log(`${scenario.id}: 캡처 ${Object.keys(snapshots).length}개`);
    }
    fs.writeFileSync(metaPath, `${JSON.stringify(meta, null, 2)}\n`, 'utf8');
    return 0;
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) process.exitCode = main(process.argv.slice(2));
