// 간헐천 AI 등록 raw-geyser-ai-registration.js(RawGeyserAiRegistration)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: geyser-ai-registration-x86.tsv 10.78(originals) 302행(정상 84·하위 거부 218) — 입력 6열(sid·state·무시되는 Player0 AI·다른 Player 번호·AI 포인터·다중 여부) → 분류(normal/boundary)·첫 요청 `Player,AI 포인터,SID,state`·
//    전체 Player 저장소 Adler-32(등록은 Player 저장소를 바꾸지 않음). 원본 PATCH Player 0xb4/AI +b0. CD 10.72 행(302)은 웹 범위 밖이다.
//  - 계약: GeyserAiRegistration_ReadsSharedCurrentPlayersAndRejectsMissingStorage(같은 객체가 현재 Player AI 변경을 다시 읽고 Player0 은 무시, 부족한 저장소 거부) + 연결한 실제 하위 호출 순서/인자.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E78: 간헐천 AI 등록 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, adler32 } from './raw-support.mjs';
import { GeyserAiRegistrationBoundary, RawGeyserAiRegistration } from '../www/core/raw-geyser-ai-registration.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E78)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'geyser-ai-registration-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 10.78 Player 한 기록 폭·AI 위치와 분류별 독립 행 수이다. (2026-11-08 WB04-E78)
const STRIDE = 0xb4;
const AI = 0xb0;
const NORMAL_ROWS = 84;
const BOUNDARY_ROWS = 218;

// 예외가 나는지 본다. (2026-11-08 WB04-E78)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 같은 저장소를 참조하는 등록 객체에 현재 입력 AI 포인터들을 배치한다(원본 Player0..8 의 실제 AI 필드만 초기화, padding 은 A5). (2026-11-08 WB04-E78)
function preparePlayers(players, input) {
    players.fill(0xa5);
    for (let owner = 0; owner < 9; owner += 1) put(players, owner * STRIDE + AI, 0);
    put(players, AI, input[2]);
    if (input[3]) {
        put(players, input[3] * STRIDE + AI, input[4]);
        if (input[5]) put(players, (input[3] === 3 ? 7 : 2) * STRIDE + AI, 0xfedcba98);
    }
}

test('GeyserAiRegistration: 원본 10.78 302입력(정상 84·하위 거부 218)의 분류·첫 요청·전체 Player 저장소가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const players = new Uint8Array(9 * STRIDE);
    const registration = new RawGeyserAiRegistration(players);
    let normal = 0;
    let boundary = 0;
    // fixture 모든 입력의 분류·첫 요청 DWORD·전체 바이트를 대조한다.
    for (const row of rows) {
        assert.equal(row.length, 6);
        const input = row[1].split(',').map(Number);
        assert.equal(input.length, 6);
        preparePlayers(players, input);
        const before = Uint8Array.from(players);
        let kind = 'normal';
        let request = '-';
        try {
            registration.register(input[0], input[1]);
        } catch (error) {
            if (!(error instanceof GeyserAiRegistrationBoundary)) throw error;
            kind = 'boundary';
            request = `${error.owner},${error.aiPointer},${error.sid},${error.state}`;
        }
        const label = `입력 ${row[1]}`;
        assert.equal(kind, row[2], label);
        assert.equal(request, row[4], label);
        assert.deepEqual(players, before, label);
        assert.equal(adler32(players), Number(row[5]), label);
        if (kind === 'normal') normal += 1; else boundary += 1;
    }
    assert.ok(normal === NORMAL_ROWS && boundary === BOUNDARY_ROWS);
});

test('GeyserAiRegistration: 같은 객체가 현재 Player AI 변경을 다시 읽고 Player0 은 무시하며 부족한 저장소를 거부한다', () => {
    const players = new Uint8Array(9 * STRIDE);
    const registration = new RawGeyserAiRegistration(players);
    put(players, AI, 123);
    registration.register(0xffffffff, 0xffffffff);
    put(players, 8 * STRIDE + AI, 0x12345678);
    let rejected = false;
    try { registration.register(50, 1); } catch (error) { rejected = error instanceof GeyserAiRegistrationBoundary && error.owner === 8 && error.aiPointer === 0x12345678 && error.sid === 50 && error.state === 1; }
    assert.ok(rejected);
    put(players, 8 * STRIDE + AI, 0);
    registration.register(50, 1);
    put(players, STRIDE + AI, 77);
    rejected = false;
    try { registration.register(50, 0); } catch (error) { rejected = error instanceof GeyserAiRegistrationBoundary && error.owner === 1 && error.aiPointer === 77 && error.sid === 50 && error.state === 0; }
    assert.ok(rejected);
    assert.ok(throws(() => new RawGeyserAiRegistration(players.subarray(0, 8 * STRIDE))));
    assert.ok(throws(() => new RawGeyserAiRegistration([])));
});

test('GeyserAiRegistration: 연결한 실제 하위는 번호순으로 현재 포인터·SID·state 를 받고 각 호출 뒤 다음 Player 를 다시 읽는다', () => {
    const players = new Uint8Array(9 * STRIDE);
    const registration = new RawGeyserAiRegistration(players);
    assert.ok(throws(() => registration.bindPatch(null)));
    put(players, 2 * STRIDE + AI, 0x100);
    put(players, 5 * STRIDE + AI, 0x200);
    const seen = [];
    registration.bindPatch((ai, sid, state) => {
        seen.push([ai, sid, state]);
        // 첫 하위가 뒤 Player 의 포인터를 바꾸면 다음 방문이 현재 값을 읽는다.
        if (seen.length === 1) put(players, 5 * STRIDE + AI, 0x300);
    });
    registration.register(77, 1);
    assert.deepEqual(seen, [[0x100, 77, 1], [0x300, 77, 1]]);
});
