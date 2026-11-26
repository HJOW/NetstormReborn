// L3 오라클 수집 도구: 지정한 오라클 폴더의 입력 단계표(steps.json)를 cpppj 기준선 실행 파일(225553df)로 돌려 체크포인트의 논리 화면(.png)과 Session 덤프(.tsv)를 같은 폴더에 쓴다.
// 사용(저장소 루트 또는 webpj 에서): `node webpj/tools/l3-collect.mjs tutorial2-l3`  (인자는 `tests/fixtures/cpppj/candidate-225553df/` 아래 폴더 이름)
//  - 실제 게임을 구동하므로 AGENTS.md 의 실행 확인 규칙을 따른다(HJOW-Athlon 은 확인 없이 가능). 소리는 끄고 고정 25ms 프레임으로 돌린다.
//  - 폴더에 oracle.json 이 없으면 새로 만들고, 있으면 실행 파일 해시·인자·수집 정보만 갱신한다.
//  - 이미 있는 덤프를 다시 수집하면 `sp 1` 행(소유자 0 의 미초기화 난수)만 달라지므로 바뀐 기존 덤프는 되돌려 두는 것이 좋다(웹 비교는 `sp`·`player:*` 행을 허용 차이로 둔다).
// (2026-11-24 WB04-E1305: 이전에는 임시 스크립트였던 수집 절차를 저장소 도구로 옮김)
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import crypto from 'node:crypto';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import { runCppScenario } from './cpp-oracle.mjs';
import { readBmp, encodeIndexedPng } from './image-files.mjs';

// 저장소 루트 절대 경로와 기준선 오라클 루트, 기준선 실행 파일이다. (2026-11-24 WB04-E1305)
const REPO = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');
const BASELINE = 'webpj/tests/fixtures/cpppj/candidate-225553df';
const EXE = path.join(REPO, 'extracted', 'webpj-oracle', '225553df', 'Netstorm.exe');

const name = process.argv[2];
if (!name) { console.error('사용: node webpj/tools/l3-collect.mjs <오라클 폴더 이름(예: tutorial2-l3)>'); process.exit(2); }
const dir = path.join(REPO, BASELINE, name);
const plan = JSON.parse(fs.readFileSync(path.join(dir, 'steps.json'), 'utf8'));
// 마지막 단계 프레임까지 돌린다(실행기는 프레임 수를 1 크게 받는다).
const frames = Math.max(...plan.steps.map(([frame]) => frame)) + 1;
// 전체화면 가장자리 폴링은 창 테두리가 없어야 하므로 명시한 startInFullScreen=1에서는 강제 --window만 제거한다.
const fullScreen = Object.entries(plan.settings ?? {}).some(([key, value]) => key.toLowerCase() === 'startinfullscreen' && Number(value) === 1);
const result = runCppScenario({ exe: EXE, gameDir: path.join(REPO, 'assets', 'game-data'), shared: { settings: plan.settings, fixedFrameMs: plan.fixedFrameMs, mission: plan.mission }, scenario: { id: 'l3', steps: plan.steps, frames: frames - 1 },
    execute: fullScreen ? (file, args, options) => spawnSync(file, args.filter((value) => value !== '--window'), options) : undefined });
// 체크포인트 화면(BMP → 인덱스 PNG)과 덤프(TSV)를 쓴다.
for (const [label, { bmp }] of Object.entries(result.snapshots)) fs.writeFileSync(path.join(dir, `${label}.png`), encodeIndexedPng(readBmp(bmp)));
for (const [label, { tsv }] of Object.entries(result.dumps)) fs.writeFileSync(path.join(dir, `${label}.tsv`), tsv);
// 요청한 실제 보고서는 카메라 좌표 등 Session 밖 관측의 원시 근거로도 보존한다.
if (plan.steps.some(([, operation]) => operation === 'report' || operation === 'report-presented')) fs.writeFileSync(path.join(dir, 'report.tsv'), result.report);
// 출처 파일을 만들거나 갱신한다.
const metaPath = path.join(dir, 'oracle.json');
const meta = fs.existsSync(metaPath) ? JSON.parse(fs.readFileSync(metaPath, 'utf8')) : {
    format: 'netstorm-webpj-cpppj-oracle', version: 1,
    description: `cpppj 기준선 실행 파일(225553df)로 ${plan.mission} 입력 단계표(steps.json)를 수집한 L3 오라클. 체크포인트마다 Session 덤프(.tsv)와 논리 화면(.png)이다. tools/l3-collect.mjs 로 수집했다.`,
    cpppjCommit: '225553df', exe: { file: 'Netstorm.exe' }, steps: `${BASELINE}/${name}/steps.json`,
};
const exeBytes = fs.readFileSync(EXE);
meta.exe = { file: 'Netstorm.exe', sha256: crypto.createHash('sha256').update(exeBytes).digest('hex'), bytes: exeBytes.length };
meta.args = result.args.filter((value) => !(fullScreen && value === '--window')).map((value) => (/\.tsv$/.test(value) && /steps/.test(value) ? '<steps.tsv>' : /report/.test(value) && /\.tsv$/.test(value) ? '<report.tsv>' : value));
meta.collected = { host: os.hostname(), at: new Date().toISOString(), node: process.version };
fs.writeFileSync(metaPath, `${JSON.stringify(meta, null, 2)}\n`);
console.log('frames', frames, 'snapshots', Object.keys(result.snapshots).join(','), 'dumps', Object.keys(result.dumps).join(','));
