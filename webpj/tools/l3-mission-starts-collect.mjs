// 미션 시작 상태 오라클 수집 도구: 지정한 미션들을 cpppj 기준선(225553df)으로 5·30·120프레임까지 돌려 Session 덤프의 정규화 해시를 fixture 로 쓴다.
// 사용(저장소 루트 또는 webpj 에서): `node webpj/tools/l3-mission-starts-collect.mjs [미션 이름...]` (이름이 없으면 전체 목록)
//  - 실제 게임을 구동하므로 AGENTS.md 의 실행 확인 규칙을 따른다(HJOW-Athlon 은 확인 없이 가능). 소리는 끄고 고정 25ms 프레임으로 돌린다.
//  - 해시 정의(`dumpHash`): 덤프를 LF 로 정규화하고 환경 의존 행(`sp`: 소유자 0 의 미초기화 난수, `player:` 계열: 주소 잔재)을 뺀 뒤 SHA-256 을 낸다. 웹 e2e(`e2e/l3-mission-starts.spec.js`)가 같은 정의로 웹 덤프를 해시해 비교한다.
//  - 결과 `tests/fixtures/cpppj/candidate-225553df/mission-starts/hashes.json` 은 { cpppjCommit, frames, missions: { 이름: { 프레임: 해시 } } } 이며 이름은 `_beginCampaignFromDialog` 에 넘기는 미션 이름이다.
// (2026-11-24 WB04-E1313: 전 미션 시작 상태 L3 대조에서 Tutorial2 의 슬롯 순서 차이 같은 시작 시점 차이를 미리 잡도록 추가)
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import crypto from 'node:crypto';
import { fileURLToPath } from 'node:url';
import { runCppScenario } from './cpp-oracle.mjs';

// 저장소 루트·기준선 오라클 루트·기준선 실행 파일이다. (2026-11-24 WB04-E1313)
const REPO = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');
const BASELINE = 'webpj/tests/fixtures/cpppj/candidate-225553df';
const EXE = path.join(REPO, 'extracted', 'webpj-oracle', '225553df', 'Netstorm.exe');

// 비교하는 프레임 번호(미션 시작 뒤 주 루프 프레임)이다. (2026-11-24 WB04-E1313)
export const START_FRAMES = Object.freeze([5, 30, 120]);

// 시작 상태를 대조하는 미션 이름 전체이다(Tutorial1~6·캠페인 세 묶음 16개·Priest Training 9개). Tutorial1·2·3 은 L3 단계표가 따로 있지만 시작 상태도 같은 방식으로 고정한다. (2026-11-24 WB04-E1313)
export const START_MISSIONS = Object.freeze([
    'Tutorial1', 'Tutorial2', 'Tutorial3', 'Tutorial4', 'Tutorial5', 'Tutorial6',
    'thewarbegins', 'masterofwhirligigs', 'savetheisland', 'fragilefortune', 'thunderingpower', 'dissolvedalliance',
    'thenoose', 'rainvsrain', 'runforit', 'guardmyback', 'surrounded', 'breakingthrough', 'totherescue', 'vicious', 'enemyterritory', 'finalconfrontation',
    'bc1menu76', 'bc2bridging76', 'bc3acid_bar76', 'bc4bow_rush76', 'bc5walkers76', 'bc6mows76', 'bc7sun_bars76', 'bc8fury76', 'bc9ssun_bars76',
]);

// 덤프 문자열의 정규화 해시를 돌려준다(`sp`·`player:` 행 제외). (2026-11-24 WB04-E1313)
export function dumpHash(source) {
    const text = typeof source === 'string' ? source : Buffer.from(source).toString('utf8'); // cpp 수집 결과는 바이트이고 웹 덤프는 문자열이다.
    const kept = text.split(/\r?\n/).filter((line) => !(line.startsWith('sp\t') || line.startsWith('player\t')));
    return crypto.createHash('sha256').update(kept.join('\n'), 'utf8').digest('hex');
}

// 직접 실행했을 때만 수집한다(스펙이 이 모듈의 상수·함수를 가져다 쓸 수 있게 한다). (2026-11-24 WB04-E1313)
if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
    const requested = process.argv.slice(2), names = requested.length ? requested : START_MISSIONS;
    const outPath = path.join(REPO, BASELINE, 'mission-starts', 'hashes.json');
    const plan = JSON.parse(fs.readFileSync(path.join(REPO, BASELINE, 'tutorial1-l3', 'steps.json'), 'utf8'));
    const previous = fs.existsSync(outPath) ? JSON.parse(fs.readFileSync(outPath, 'utf8')) : { missions: {} };
    const missions = { ...previous.missions };
    // 미션마다 같은 설정으로 돌려 덤프 해시를 모은다.
    for (const name of names) {
        const steps = START_FRAMES.map((frame) => [frame, 'session-dump', `s${frame}`]);
        const result = runCppScenario({ exe: EXE, gameDir: path.join(REPO, 'assets', 'game-data'), shared: { settings: plan.settings, fixedFrameMs: plan.fixedFrameMs, mission: name }, scenario: { id: 'start', steps, frames: Math.max(...START_FRAMES) + 1 } });
        missions[name] = Object.fromEntries(START_FRAMES.map((frame) => [frame, dumpHash(result.dumps[`s${frame}`].tsv)]));
        console.log(name, Object.values(missions[name]).map((hash) => hash.slice(0, 8)).join(' '));
    }
    fs.mkdirSync(path.dirname(outPath), { recursive: true });
    fs.writeFileSync(outPath, `${JSON.stringify({ format: 'netstorm-webpj-mission-start-hashes', version: 1, cpppjCommit: '225553df', frames: START_FRAMES, settings: plan.settings, collected: { host: os.hostname(), at: new Date().toISOString(), node: process.version }, missions }, null, 2)}\n`);
    console.log('쓴 파일', path.relative(REPO, outPath));
}
