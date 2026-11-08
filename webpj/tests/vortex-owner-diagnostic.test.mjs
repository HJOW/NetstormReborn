// 신전 소유자 진단 raw-vortex-owner-diagnostic.js(reportVortexOwnerMissingIsland)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: vortex-owner-diagnostic-local-x86.tsv 10.78(originals) 48행 — 입력 5열(슬롯 +14/+18 float 비트·옛 소유자·새 소유자 DWORD·섬 목록 개수 DWORD, 섬 번호는 7) → 상세 문자열 hex(상세 저장 장치에 한 번 기록),
//    진단 식·파일·줄. 원문 부모(RawVortexOwner)의 슬롯 필드 제거와 Player 18필드는 부모 이식 단위에서 대조하므로 여기서는 진단 접두(상세 → 진단)만 확인한다. CD 10.72 행(48)은 웹 범위 밖이다.
//  - 계약: VortexOwnerDiagnostic_ResumesAndKeepsDetailBeforeMissingAssertion(진단이 정상 복귀하면 복귀하고, 누락된 진단은 앞선 상세를 보존한 채 거부).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E78: 신전 소유자 진단 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { reportVortexOwnerMissingIsland } from '../www/core/raw-vortex-owner-diagnostic.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E78)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'vortex-owner-diagnostic-local-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 독립 입력 수(10.78)와 관찰 SID, 부모가 섬 번호로 돌려주는 값이다. (2026-11-08 WB04-E78)
const ROWS = 48;
const SOURCE = 5;
const ISLAND = 7;

// 예외가 나는지 본다. (2026-11-08 WB04-E78)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

test('VortexOwnerDiagnostic: 원본 10.78 48입력의 상세 문자열(ftol 좌표·signed 정수)과 진단 위치가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // MP/포획/희생/공격 없이 진단 진입까지만 확인한다.
    for (const row of rows) {
        assert.equal(row.length, 8);
        const args = row[1].split(',').map(Number);
        assert.equal(args.length, 5);
        const pool = new SidPool(32768);
        const raw = pool.physical(SOURCE);
        raw.fill(0);
        put(raw, 0, 0x519860);
        raw[10] = 123;
        raw[34] = args[2];
        put(raw, 14, args[0]);
        put(raw, 18, args[1]);
        let detail = '';
        let details = 0;
        let assertions = 0;
        let stopped = false;
        try {
            reportVortexOwnerMissingIsland(pool, SOURCE, ISLAND, args[3], args[4], {
                detail: (text) => { details += 1; detail = text; },
                assertion: (expression, file, line) => {
                    assertions += 1;
                    assert.ok(details === 1 && expression === row[3] && file === row[4] && String(line) === row[5], `행 ${count}`);
                    throw new Error('diagnostic');
                }
            });
        } catch (error) {
            stopped = error.message === 'diagnostic';
        }
        assert.ok(stopped && details === 1 && assertions === 1, `행 ${count}: 입력 ${row[1]}`);
        assert.equal(Buffer.from(detail, 'latin1').toString('hex'), row[2], `행 ${count}: 입력 ${row[1]}`);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('VortexOwnerDiagnostic: 진단이 정상 복귀하면 복귀하고 누락된 진단·상세는 앞선 상세를 보존한 채 거부하며 판본을 구별한다', () => {
    const pool = new SidPool(32768);
    let detail = '';
    let calls = 0;
    reportVortexOwnerMissingIsland(pool, SOURCE, 7, 2, 5, { detail: (text) => { detail = text; }, assertion: () => { calls += 1; } });
    assert.ok(detail !== '' && calls === 1);
    detail = '';
    assert.ok(throws(() => reportVortexOwnerMissingIsland(pool, SOURCE, 7, 2, 5, { detail: (text) => { detail = text; } })));
    assert.ok(detail !== '' && calls === 1);
    assert.ok(throws(() => reportVortexOwnerMissingIsland(pool, SOURCE, 7, 2, 5, {})));
    assert.ok(throws(() => reportVortexOwnerMissingIsland({ edition: 'cd1072' }, SOURCE, 7, 2, 5, {})));
    // 비유한 좌표는 무효 FISTP 의 하위 0 으로, 음수는 signed64 절삭의 하위 DWORD 로 쓴다.
    const raw = pool.physical(SOURCE);
    put(raw, 14, 0x7fc00000);
    put(raw, 18, 0xc2f60000);
    reportVortexOwnerMissingIsland(pool, SOURCE, -1, 0xffffffff, 0xfffffffe, { detail: (text) => { detail = text; }, assertion: () => {} });
    assert.equal(detail, 'islandList.totalIslandIds=-2\nislandId=-1\npos=(0,-123)\noldPlayerId=0\nnewPlayerId=-1\n');
});
