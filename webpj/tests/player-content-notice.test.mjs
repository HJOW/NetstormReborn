// Player 내용물 통지(RawPlayerContentNotice)를 원본 10.78 관측으로 대조한다(WB04-E9, cpppj CampaignLifetimeTests 의 PlayerNoticeIndependentPeFixture 이식).
//  - player-content-notice-x86.tsv 88행: 진단 접두 11개 전역의 최종 값, 현재 SP 조회·가상 handle 호출 순서/인자, 부모 포함 네 슬롯 전체 50 BYTE 를 대조한다.
//  - 계약: 풀/전역 조회기/소비자 누락 거부, 순환 체인 거부, 전역은 호출 위치에서 현재 값을 읽음.
// cpppj 의 RawCampaignLifetimeBindings 통합(World 삭제 장부와 내용물 수명)은 해당 단위를 옮길 때 대조한다. (2026-11-08 WB04-E9: Player 내용물 통지 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawPlayerContentNotice } from '../www/core/raw-player-content-notice.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E9)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'player-content-notice-x86.tsv'));

// 입력 DWORD 를 32비트로 감아 읽고 슬롯을 hex 로 표시한다. (2026-11-08 WB04-E9)
const number = (text) => Number(BigInt(text) & 0xffffffffn);
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 원본 입력과 같은 부모(50)와 세 종속(60·61·62)을 실제 풀에 구성한다(전체 50 BYTE 를 명시 입력으로 초기화). (2026-11-08 WB04-E9)
function noticeSlots(pool, layout) {
    for (const sid of [50, 60, 61, 62]) pool.physical(sid).fill(0);
    const parent = pool.physical(50);
    put(parent, 0, 0x50ee08);
    parent[10] = 152;
    if (!layout) return;
    put(parent, 6, 60, 2);
    // 세 내용물의 type/state/등록 ID 입력은 원본과 동일하다.
    for (const sid of [60, 61, 62]) {
        const raw = pool.physical(sid);
        put(raw, 4, sid === 62 ? 0 : sid + 1, 2);
        raw[10] = (sid === 61 || (layout === 1 && sid === 60)) ? 6 : (layout === 5 ? 2 : 3);
        raw[11] = layout === 4 ? 3 : 0;
        put(raw, 18, sid === 60 ? 7 : sid === 61 ? 8 : 9);
    }
}

// 실제 wrapper/접두/등록표 88행의 전체 200 BYTE/전역 11필드/가상 알림 순서를 대조한다.
test('Player 내용물 통지: 원본10.78 88행(전역 11·호출 순서·네 슬롯 200 BYTE)', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of rows) {
        assert.equal(row.length, 20);
        const pool = new SidPool(32768);
        noticeSlots(pool, number(row[12]));
        const current = [];
        // 현재 각 전역의 명시 입력을 손실 없이 기록한다.
        for (let index = 0; index < 11; index += 1) current.push(number(row[index]));
        const first = (current[8] << 16) >> 16;
        const mutation = number(row[13]);
        const registry = new Array(64).fill(0);
        // 현재 등록된 세 가상 노드를 원본과 동일하게 공급한다.
        for (const token of [7, 8, 9]) registry[token] = number(row[12]) === 3 ? 0 : token;
        const trace = [];
        const state = {
            cursorA: () => current[0],
            cursorB: () => current[1],
            get problemCode() { return current[2]; },
            set problemCode(value) { current[2] = value; },
            fallbackCode: () => current[3],
            get moneyChecks() { return current[4]; },
            set moneyChecks(value) { current[4] = value; },
            battleShown: () => current[5],
            localPlayer: () => current[6],
            firstSpPool: () => current[7],
            firstPermutation: () => first,
            gumpFormType: () => current[9],
            contentEvent: () => current[10],
        };
        const hooks = {
            money: (owner) => {
                trace.push(`M:${owner}`);
                if (mutation === 1) { current[3] = 91; current[4] = 0; }
                return bitsToFloat(number(row[11]));
            },
            gump: (id) => {
                assert.ok(id < registry.length, '현재 Gump 등록 ID 물리 범위');
                return registry[id];
            },
            handle: (token, event, form, reason) => {
                trace.push(`H:${token}:${event}:${form}:${reason}`);
                if (mutation === 2) { put(pool.physical(60), 4, 0, 2); current[10] = 0x11223344; registry[9] = 9; }
                else if (mutation === 3) current[9] = 6;
            },
        };
        const notice = new RawPlayerContentNotice(pool, state, hooks);
        notice.notify(50, number(row[14]), number(row[15]));
        current[8] = first & 0xffff;
        assert.equal(notice.pool, pool);
        const slots = [50, 60, 61, 62].map((sid) => hex(pool.slot(sid))).join(',');
        const label = `행 ${count} ${row.slice(0, 16).join(',')}`;
        assert.equal(current.join(','), row[17], `${label} 전역`);
        assert.equal(trace.length ? trace.join('|') : '-', row[18], `${label} 호출 순서`);
        assert.equal(slots, row[19], `${label} 슬롯`);
        count += 1;
    }
    assert.equal(count, 88);
});

// 풀/전역 조회기/소비자 누락은 통지를 성공으로 대신하지 않고, 순환 체인은 거부하며, 전역은 호출 위치에서 현재 값을 읽는다.
test('Player 내용물 통지: 누락 거부·순환 거부·현재 전역 재조회', () => {
    const pool = new SidPool(32768);
    noticeSlots(pool, 2);
    const live = { cursorA: 1, cursorB: 1, problem: 0, fallback: 5, checks: 0 };
    const state = {
        cursorA: () => live.cursorA, cursorB: () => live.cursorB,
        get problemCode() { return live.problem; }, set problemCode(value) { live.problem = value; },
        fallbackCode: () => live.fallback,
        get moneyChecks() { return live.checks; }, set moneyChecks(value) { live.checks = value; },
        battleShown: () => 0, localPlayer: () => 1, firstSpPool: () => 1, firstPermutation: () => 0, gumpFormType: () => 3, contentEvent: () => 7,
    };
    const handled = [];
    const hooks = { money: () => 0, gump: (id) => id, handle: (...args) => handled.push(args) };
    assert.throws(() => new RawPlayerContentNotice(null, state, hooks), TypeError);
    assert.throws(() => new RawPlayerContentNotice(pool, { ...state, cursorA: undefined }, hooks), /cursorA/);
    assert.throws(() => new RawPlayerContentNotice(pool, state, { money: () => 0 }), TypeError);
    const notice = new RawPlayerContentNotice(pool, state, hooks);
    // 접두 조건이 모두 정상이면 problemCode 는 그대로이고 타입 3 인 Form 두 개(60·62)만 처리한다.
    notice.notify(50, 1234, 2);
    assert.equal(live.problem, 0);
    assert.deepEqual(handled.map((args) => args[0]), [7, 9]);
    // 현재 전역 변경은 다음 호출에서 바로 읽힌다.
    live.cursorB = 2;
    notice.notify(50, 1234, 2);
    assert.equal(live.problem, 5);
    // 종속 체인이 자기 자신으로 돌아오면 거부한다.
    put(pool.physical(62), 4, 60, 2);
    assert.throws(() => notice.notify(50, 1234, 2), /순환/);
});
