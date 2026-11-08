// 사제 포획 안내 raw-priest-taken-notice.js(RawPriestTakenNotice)를 cpppj 소스(004927f0 안내 부모)의 순서 계약으로 대조한다.
//  - cpppj 는 이 파일의 독립 원본 관측 fixture 를 갖지 않고(RawCampaignSession 이 소비) 소스 주석의 순서 계약만 둔다. 이 검사는 그 계약을 하위 효과 호출 기록으로 확인한다:
//    참여 갱신 먼저 → 현재 소유자가 로컬이면 끝 → 안내 키/음향 선택 → 벽시각 읽기 → 기한 이하일 때만 이름 둘(이전, 현재 순)과 256바이트 서식 → 안내 뒤 다시 읽은 벽시각 + 20초 기한 저장.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E68: 사제 포획 안내 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { SidPool } from '../www/core/sid-pool.js';
import { RawPriestTakenNotice } from '../www/core/raw-priest-taken-notice.js';

// 안내 대상 사제 SID 이다. (2026-11-08 WB04-E68)
const PARENT = 50;

// 예외가 나는지 본다. (2026-11-08 WB04-E68)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 호출 순서를 기록하는 장면이다: 현재 소유자(raw +34)와 로컬 소유자·기한 표·시계를 입력으로 받는다. (2026-11-08 WB04-E68)
function noticeScene({ owner, local, deadlines = new Array(9).fill(0), clocks = [100, 200] }) {
    const pool = new SidPool(24000, { server: false });
    for (let id = 5; id <= PARENT; id += 1) pool.allocate(2);
    pool.allocated(PARENT)[34] = owner;
    const scene = { pool, local, deadlines, events: [], clocks: [...clocks] };
    scene.hooks = {
        refresh: (previous) => scene.events.push(`R:${previous}`),
        wallSeconds: () => { const value = scene.clocks.length > 1 ? scene.clocks.shift() : scene.clocks[0]; scene.events.push(`W:${value}`); return value; },
        name: (who) => { scene.events.push(`N:${who}`); return `P${who}`; },
        sound: (file) => scene.events.push(`S:${file}`),
        tell: (text) => scene.events.push(`T:${text}`)
    };
    scene.notice = new RawPriestTakenNotice(pool, () => scene.local, deadlines, scene.hooks);
    return scene;
}

test('PriestTakenNotice: 내 사제가 포획되면 참여 갱신→음향→시계→이름(이전, 현재)→안내→시계 순서로 안내하고 기한을 +20초로 저장한다', () => {
    const scene = noticeScene({ owner: 2, local: 1 });
    scene.notice.notify(PARENT, 1);
    assert.deepEqual(scene.events, ['R:1', 'S:ourPriestCaptured.wav', 'W:100', 'N:1', 'N:2', 'T:WarnYourPriestTaken(1,P2,P1,~r,~l)', 'W:200']);
    assert.equal(scene.deadlines[1], 220);
});

test('PriestTakenNotice: 중립 사제는 음향 없이 중립 키로, 적 사제는 적 음향과 일반 키로 안내한다', () => {
    const neutral = noticeScene({ owner: 3, local: 1 });
    neutral.notice.notify(PARENT, 0);
    assert.deepEqual(neutral.events, ['R:0', 'W:100', 'N:0', 'N:3', 'T:WarnNeutralPriestTaken(0,P3,P0,~w,~q)', 'W:200']);
    const enemy = noticeScene({ owner: 3, local: 1 });
    enemy.notice.notify(PARENT, 2);
    assert.deepEqual(enemy.events, ['R:2', 'S:enemyPriestCaptured.wav', 'W:100', 'N:2', 'N:3', 'T:WarnPriestTaken(2,P3,P2,~w,~r)', 'W:200']);
});

test('PriestTakenNotice: 현재 소유자가 로컬이면 참여 갱신만 하고 끝나며 범위 밖 현재 소유자는 0번 색을 쓴다', () => {
    const own = noticeScene({ owner: 1, local: 1 });
    own.notice.notify(PARENT, 2);
    assert.deepEqual(own.events, ['R:2']);
    assert.equal(own.deadlines[2], 0);
    const far = noticeScene({ owner: 200, local: 1 });
    far.notice.notify(PARENT, 2);
    assert.ok(far.events.includes('T:WarnPriestTaken(2,P200,P2,~q,~r)'));
});

test('PriestTakenNotice: 기한 안이면 음향은 내고 안내는 억제하며 기한은 다시 읽은 시계 + 20초로 재설정한다', () => {
    const deadlines = new Array(9).fill(0);
    deadlines[2] = 150;
    const scene = noticeScene({ owner: 3, local: 1, deadlines });
    scene.notice.notify(PARENT, 2);
    assert.deepEqual(scene.events, ['R:2', 'S:enemyPriestCaptured.wav', 'W:100', 'W:200']);
    assert.equal(scene.deadlines[2], 220);
});

test('PriestTakenNotice: 이전 소유자 기한 칸 밖·256바이트 서식 초과·판본 불일치·누락한 하위 효과는 거부한다', () => {
    const range = noticeScene({ owner: 3, local: 1 });
    assert.ok(throws(() => range.notice.notify(PARENT, 9)));
    assert.deepEqual(range.events, ['R:9']);
    const long = noticeScene({ owner: 3, local: 1 });
    long.hooks.name = () => 'x'.repeat(300);
    assert.ok(throws(() => long.notice.notify(PARENT, 2)));
    assert.ok(!long.events.some((event) => event.startsWith('T:')));
    assert.ok(throws(() => new RawPriestTakenNotice({ edition: 'cd1072' }, () => 1, new Array(9).fill(0), {})));
    assert.ok(throws(() => new RawPriestTakenNotice(new SidPool(24000, { server: false }), () => 1, new Array(8).fill(0), {})));
    const missing = noticeScene({ owner: 3, local: 1 });
    delete missing.hooks.sound;
    assert.ok(throws(() => missing.notice.notify(PARENT, 2)));
    assert.deepEqual(missing.events, ['R:2']);
});
