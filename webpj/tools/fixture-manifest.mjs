// webpj 이식 추적 목록(tests/fixtures/manifest.json)을 계산하고 검사하는 도구이다.
// 목록의 사람이 쓰는 칸(id·종류·판본·소비 검사·웹 모듈·단위·문서·cpppj 근거 소스·반영 범위)은 그대로 두고,
// 계산 칸(sha256·bytes·lines·text·fixtureCommit·webCommit·cpppjBasis)만 이 도구가 채운다.
//  - 해시: 텍스트 파일(앞 8000바이트에 NUL 없음)은 CRLF 를 LF 로 바꾼 내용의 SHA-256 이다. Windows(core.autocrlf=true)와 다른 체크아웃에서 같은 값을 얻기 위해서다.
//    이진 파일(PNG 등)은 바이트 그대로이다. kind 가 'source'(cpppj/dotnetpj 교차 확인용 소스)인 항목은 계속 바뀌므로 해시를 기록하지 않는다.
//  - lines: LF 로 나눈 비어 있지 않은 줄 수(CR 제외)이다. 이진 파일은 null 이다.
//  - fixtureCommit: 그 파일을 마지막으로 바꾼 커밋, webCommit: 웹 모듈(modules)을 마지막으로 바꾼 커밋(실제 반영 커밋),
//    cpppjBasis: webCommit 시점에 각 cpppj 근거 소스를 마지막으로 바꾼 커밋(웹이 옮길 때 볼 수 있던 판)이다. 단위 문서의 '검토 트리'와는 따로 기록한다.
// 사용(저장소 루트 또는 webpj 에서):
//   node webpj/tools/fixture-manifest.mjs            → 검사만 한다(해시 불일치 시 종료 코드 1). cpppj 근거 소스가 basis 뒤에 바뀐 커밋 수도 보고한다.
//   node webpj/tools/fixture-manifest.mjs --write    → 계산 칸을 다시 채워 저장한다(파일 변경을 검토한 뒤에만 쓴다).
// 이 도구는 로컬 파일과 git 기록만 읽고 네트워크를 쓰지 않는다.
// (2026-11-05 WB02-F: LEFT_JOBS.webpj.md 3.1절 이식 추적 요구로 추가)
import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

// 저장소 루트의 절대 경로이다. (2026-11-05 WB02-F)
const REPO_ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');

// 이식 추적 목록 파일의 저장소 상대 경로이다. (2026-11-05 WB02-F)
export const MANIFEST_PATH = 'webpj/tests/fixtures/manifest.json';

// 해시를 기록하지 않는 항목 종류(계속 바뀌는 교차 확인용 소스)이다. (2026-11-05 WB02-F)
export const UNHASHED_KINDS = Object.freeze(['source']);

// 파일 바이트가 텍스트인지 판정한다(git 과 같은 규칙: 앞 8000바이트에 NUL 이 없으면 텍스트). (2026-11-05 WB02-F)
export function isText(bytes) {
    return !bytes.subarray(0, 8000).includes(0);
}

// 텍스트 바이트의 CRLF 를 LF 로 바꾼다. 이진 파일은 그대로 돌려준다. (2026-11-05 WB02-F)
export function normalize(bytes) {
    if (!isText(bytes)) return bytes;
    const out = Buffer.alloc(bytes.length);
    let length = 0;
    // CR 다음이 LF 인 CR 만 뺀다.
    for (let index = 0; index < bytes.length; index += 1) {
        if (bytes[index] === 0x0d && bytes[index + 1] === 0x0a) continue;
        out[length] = bytes[index];
        length += 1;
    }
    return out.subarray(0, length);
}

// 파일 하나의 정규화 해시·크기·줄 수를 계산한다. 반환: { text, sha256, bytes, lines }. (2026-11-05 WB02-F)
export function measure(bytes) {
    const text = isText(bytes);
    const normalized = normalize(bytes);
    const lines = text ? normalized.toString('utf8').split('\n').filter((line) => line.length > 0).length : null;
    return { text, sha256: crypto.createHash('sha256').update(normalized).digest('hex'), bytes: normalized.length, lines };
}

// git 명령을 저장소 루트에서 실행해 앞뒤 공백을 뗀 출력을 돌려준다. 실패하면 null 이다. (2026-11-05 WB02-F)
function git(...args) {
    try {
        return execFileSync('git', args, { cwd: REPO_ROOT, encoding: 'utf8', stdio: ['ignore', 'pipe', 'ignore'] }).trim() || null;
    } catch {
        return null;
    }
}

// 경로들을 마지막으로 바꾼 커밋(짧은 해시)이다. revision 을 주면 그 커밋까지의 기록에서 찾는다. (2026-11-05 WB02-F)
function lastCommit(paths, revision = null) {
    return git('log', '-1', '--format=%h', ...(revision ? [revision] : []), '--', ...paths);
}

// 목록을 읽는다. (2026-11-05 WB02-F)
export function readManifest() {
    return JSON.parse(fs.readFileSync(path.join(REPO_ROOT, MANIFEST_PATH), 'utf8'));
}

// 작업 사본 표시이다. 웹 모듈에 커밋하지 않은 변경(새 파일 포함)이 있으면 webCommit 을 이 값으로 두고 cpppj 근거는 현재 HEAD 기준으로 기록한다.
// 그 단위를 커밋한 뒤 다음 --write 에서 실제 커밋 해시로 바뀐다. (2026-11-05 WB04-B: 새 모듈을 같은 단위에서 등록하기 위해 추가)
export const WORKTREE = 'worktree';

// 경로들에 커밋하지 않은 변경이 있는지 본다. (2026-11-05 WB04-B)
function dirty(paths) {
    return Boolean(git('status', '--porcelain', '--', ...paths));
}

// 항목 하나의 계산 칸을 채운 새 항목을 만든다. 파일이 없으면 해시 칸은 null 이다. (2026-11-05 WB02-F, WB04-B: 작업 사본 표시)
function compute(entry) {
    const full = path.join(REPO_ROOT, entry.path);
    const exists = fs.existsSync(full);
    const hashed = exists && !UNHASHED_KINDS.includes(entry.kind) ? measure(fs.readFileSync(full)) : { text: null, sha256: null, bytes: null, lines: null };
    const worktree = entry.modules.length > 0 && dirty(entry.modules);
    const webCommit = entry.modules.length === 0 ? null : worktree ? WORKTREE : lastCommit(entry.modules);
    const basisRevision = worktree ? 'HEAD' : webCommit;
    return {
        ...entry,
        ...hashed,
        fixtureCommit: lastCommit([entry.path]),
        webCommit,
        cpppjBasis: Object.fromEntries((entry.cpppjSources ?? []).map((source) => [source, basisRevision ? lastCommit([source], basisRevision) : null])),
    };
}

// 검사: 해시 칸이 있는 항목의 현재 파일 값이 기록과 같은지 본다. 반환: { mismatches[], missing[], drift[] }.
// drift 는 cpppj 근거 소스가 basis 커밋 뒤 HEAD 까지 바뀐 커밋 수이다(재검토 후보 알림이며 실패가 아니다). (2026-11-05 WB02-F)
export function check(manifest = readManifest(), { withDrift = true } = {}) {
    const mismatches = [];
    const missing = [];
    const drift = [];
    // 항목마다 파일과 기록을 비교한다.
    for (const entry of manifest.entries) {
        const full = path.join(REPO_ROOT, entry.path);
        if (!fs.existsSync(full)) {
            missing.push(entry.path);
            continue;
        }
        if (entry.sha256) {
            const current = measure(fs.readFileSync(full));
            if (current.sha256 !== entry.sha256 || current.bytes !== entry.bytes || current.lines !== entry.lines) mismatches.push({ path: entry.path, recorded: entry.sha256, current: current.sha256 });
        }
        if (!withDrift) continue;
        // 작업 사본으로 기록한 항목의 모듈이 이제 커밋되었으면 갱신 알림을 남긴다.
        if (entry.webCommit === WORKTREE && !dirty(entry.modules)) drift.push({ id: entry.id, source: '(webCommit)', basis: WORKTREE, commits: 0 });
        // cpppj 근거 소스별로 basis 이후 변경 커밋 수를 센다.
        for (const [source, basis] of Object.entries(entry.cpppjBasis ?? {})) {
            if (!basis) continue;
            const count = Number(git('rev-list', '--count', `${basis}..HEAD`, '--', source) ?? 0);
            if (count > 0) drift.push({ id: entry.id, source, basis, commits: count });
        }
    }
    return { mismatches, missing, drift };
}

// 명령줄 진입점이다. --write 면 계산 칸을 다시 채워 저장하고, 아니면 검사 결과를 출력한다. (2026-11-05 WB02-F)
function main(argv) {
    const manifest = readManifest();
    if (argv.includes('--write')) {
        const updated = { ...manifest, entries: manifest.entries.map(compute) };
        fs.writeFileSync(path.join(REPO_ROOT, MANIFEST_PATH), `${JSON.stringify(updated, null, 2)}\n`, 'utf8');
        console.log(`이식 추적 목록 갱신: ${updated.entries.length}개 항목`);
        return 0;
    }
    const result = check(manifest);
    console.log(`항목 ${manifest.entries.length}개, 해시 불일치 ${result.mismatches.length}개, 없는 파일 ${result.missing.length}개`);
    for (const item of result.mismatches) console.log(`불일치\t${item.path}\t기록 ${item.recorded}\t현재 ${item.current}`);
    for (const item of result.missing) console.log(`없음\t${item}`);
    // 재검토 후보(같은 소스가 여러 항목에 있으면 한 번만)를 출력한다.
    const seen = new Set();
    for (const item of result.drift) {
        const key = `${item.source}@${item.basis}`;
        if (seen.has(key)) continue;
        seen.add(key);
        if (item.basis === WORKTREE) console.log(`갱신 필요\t${item.id}\twebCommit 이 작업 사본 표시이나 모듈이 커밋됨(--write 로 실제 해시 기록)`);
        else console.log(`cpppj 변경\t${item.source}\t${item.basis} 이후 ${item.commits}커밋\t(${item.id})`);
    }
    return result.mismatches.length > 0 ? 1 : 0;
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) process.exitCode = main(process.argv.slice(2));
