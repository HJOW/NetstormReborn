// Fort 사제 준비 raw-fort-priest-preparation.js(RawFortPriestPreparation)를 원본 10.78 기계어 관측 13입력과 cpppj 계약으로 대조한다.
//  - 원본 관측: fort-priest-preparation-x86.tsv(10.78 전용 단일 판본) 13행 — 라벨(mission/editor/state17/no-record/marked/fallback/duplicates/populate/missing-island/two-regions/shorten-list/post-change/full-capacity) → 하위 호출 trace 와 마지막 missionPost.
//    기술 네 가지 보장·State+17·Territory 기록·섬 조회·목록 수집/표시 선택/안정 제거·소유자 지정·초기 배치·안내의 순서와 인자를 모두 대조한다.
//  - 계약: FortPriestPreparation_SelectedFailureKeepsEarlierTechnologyEffects(선택된 기술 생성의 예외는 앞선 효과를 보존하고 뒤 장치 누락을 미리 요구하지 않음).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E70: Fort 사제 준비 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { PriestPostPopState } from '../www/core/raw-priest-post-pop.js';
import { RawFortPriestPreparation } from '../www/core/raw-fort-priest-preparation.js';

// 원본 관측 행이다(단일 판본 파일; 빈 trace 는 빈 문자열). 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E70)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'fort-priest-preparation-x86.tsv'));

// 관측 입력 수이다. (2026-11-08 WB04-E70)
const ROWS = 13;

// 예외가 나는지 본다. (2026-11-08 WB04-E70)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

test('FortPriestPreparation: 원본 기계어 관측 13입력의 하위 호출 trace 와 마지막 missionPost 가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let checked = 0;
    // 주석 외 모든 최초 관측 행을 한 번씩 독립 물리에서 재현한다.
    for (const row of rows) {
        const [label, expected, post] = [row[0], row[1] ?? '', row[2] ?? ''];
        const pool = new SidPool(30000, { server: true });
        const priests = new PriestPostPopState();
        let mission = label === 'mission';
        const editor = label === 'editor' ? 1 : 0;
        const subscriber = 5;
        const templateOwner = 1;
        const playerOwner = 7;
        let finds = 0;
        const state = new Uint8Array(28);
        state[17] = label === 'state17' ? 1 : 0;
        const events = [];
        let entries;
        if (label === 'duplicates') entries = [1, 1, 2];
        else if (label === 'full-capacity') entries = Array.from({ length: 22 }, (_, index) => index + 1);
        else if (label === 'populate') entries = [];
        else entries = [1, 2, 3];
        priests.priests.entries = entries;
        priests.priests.count = entries.length;
        // 검사 소유의 실제 풀에 raw 입력만 설치한다. 게임 생성/Pop 을 실행한 것으로 세지 않는다.
        for (let sid = 1; sid <= 22; sid += 1) {
            const raw = pool.physical(sid);
            put(raw, 14, floatToBits(sid));
            put(raw, 18, floatToBits(1.0));
            raw[12] = (label === 'marked' && sid === 2) || (label === 'duplicates' && sid === 1) ? 0x80 : 0;
        }
        const hooks = {
            baseType: (index) => (index === 0 ? 133 : index === 1 ? 71 : 128),
            themeType: (theme) => { assert.equal(theme, 2); return 77; },
            technology: () => { events.push('tech'); return 4; },
            contains: (parent, type) => {
                assert.equal(parent, 4);
                events.push(`find:${type}`);
                finds += 1;
                if (label === 'post-change' && finds === 1) mission = true;
                return finds % 2 === 0 ? type : 0;
            },
            createTechnology: (parent, type) => { assert.equal(parent, 4); events.push(`create:${type}`); },
            templatePresent: (owner) => { assert.equal(owner, 1); return true; },
            state: (owner) => { assert.equal(owner, 1); return state; },
            territory: (owner, output) => {
                assert.equal(owner, 1);
                events.push('territory');
                output.fill(0);
                if (label !== 'no-record') output[1] = 1;
                if (label === 'two-regions') output[7] = 1;
            },
            island: (owner, index) => { assert.equal(owner, 7); events.push(`island:${index}`); return label === 'missing-island' ? 127 : index + 1; },
            region: (x, y) => {
                assert.equal(y, 1.0);
                if (label === 'shorten-list') priests.priests.count = 1;
                return label === 'two-regions' && x === 3.0 ? 2 : 1;
            },
            setOwner: (sid, owner) => { events.push(`owner:${sid}:${owner}`); pool.physical(sid)[34] = owner; },
            populate: (region) => { events.push(`populate:${region}`); },
            tellPriests: (count) => { events.push(`tell:${count}`); }
        };
        const globals = { get missionPost() { return mission; }, get editor() { return editor; }, subscriber, templateOwner, playerOwner };
        new RawFortPriestPreparation(pool, priests, globals, hooks).run();
        assert.equal(events.join(';'), expected, `라벨 ${label}`);
        assert.equal(String(mission ? 1 : 0), post, `라벨 ${label}`);
        checked += 1;
    }
    assert.equal(checked, ROWS);
});

test('FortPriestPreparation: 선택된 기술 생성의 예외는 앞선 효과를 보존하고 뒤 장치 누락을 미리 요구하지 않는다', () => {
    const pool = new SidPool(30000, { server: true });
    const priests = new PriestPostPopState();
    let reads = 0;
    let created = 0;
    const hooks = {
        baseType: (index) => { reads += 1; return 100 + index; },
        technology: () => 1,
        contains: () => 0,
        createTechnology: (parent, type) => { created += 1; if (type === 101) throw new Error('선택한 두 번째 기술 생성 중단'); }
    };
    const globals = { missionPost: false, editor: 1, subscriber: 0, templateOwner: 1, playerOwner: 1 };
    let stopped = false;
    try {
        new RawFortPriestPreparation(pool, priests, globals, hooks).run();
    } catch (error) {
        stopped = error.message === '선택한 두 번째 기술 생성 중단';
    }
    assert.ok(stopped && reads === 2 && created === 2 && globals.missionPost === false && globals.editor === 1);
    assert.ok(throws(() => new RawFortPriestPreparation({ edition: 'cd1072' }, priests, globals, hooks)));
    // 손상된 목록 SID 는 접근할 때 거부한다.
    priests.priests.entries = [0x10032];
    priests.priests.count = 1;
    const preparation = new RawFortPriestPreparation(pool, priests, globals, { region: () => 1 });
    assert.ok(throws(() => preparation.collect(1, new Array(20).fill(0))));
    assert.ok(throws(() => preparation.marked([30000])));
});
