// 신전 PreDestroy raw-vortex-pre-destroy.js(RawVortexPreDestroy)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: vortex-pre-x86.tsv 102행 — 입력 3열(변이 조합 variant·외부 효과 변경 종류 mutation·flags) → 아홉 Player 전체 raw, 슬롯 50/60/61 raw 전체, 두 등록 목록과 추가 목록의 count+물리 꼬리,
//    로딩 깊이, 외부 효과 호출 순서(Island/Exists/Temple/Clear/Owner/Region/Base/Refresh/Assert). owner/extra/로딩 변경 뒤 현재 재읽기를 검증한다.
//  - 계약: VortexPreDestroy_RequiredDamageableAndStorage(필수 Damageable 이 없으면 목록/지역의 앞선 실제 효과 뒤 원본 Player 정리를 실행하지 않고, 잘못된 Player 저장소를 거부).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E80: 신전 PreDestroy 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { RawVortexPreDestroy } from '../www/core/raw-vortex-pre-destroy.js';

// 원본 관측 행이다(머리 행 제외). 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E80)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'vortex-pre-x86.tsv'))?.filter((row) => row[0] !== 'input') ?? null;

// 원본 관찰의 부모와 두 outpost SID, 독립 입력 수이다. (2026-11-08 WB04-E80)
const IDS = Object.freeze([50, 60, 61]);
const ROWS = 102;

// 예외가 나는지 본다. (2026-11-08 WB04-E80)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E80)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 실제 풀과 서로 다른 등록 목록을 같은 신전 Pre 객체에 공급하는 장면이다. (2026-11-08 WB04-E80)
class VortexScene {
    // 판단 결과를 계산하지 않고 native 와 같은 raw 입력을 실제 할당 슬롯에 설치한다.
    constructor(input, change) {
        this.variant = input;
        this.mutation = change;
        this.loading = (input & 256) ? 1 : 0;
        this.pool = new SidPool(24000, { server: false });
        this.players = new Uint8Array(1620).fill(0xa5);
        this.temples = new SquidPostPopList();
        this.residences = new SquidPostPopList();
        this.outposts = new SquidPostPopList();
        this.trace = [];
        // 마지막 검사 대상 61 까지 실제 클라이언트 슬롯을 차례로 확보한다.
        for (;;) {
            const sid = this.pool.allocate(2);
            if (!sid || sid > 61) throw new Error('신전 검사 실제 SID 준비 오류');
            if (sid === 61) break;
        }
        const owner = (input >> 2) & 15;
        const match = (input >> 6) & 3;
        // 네 신전 참조를 owner0 까지 독립 입력으로 준비한다.
        for (let player = 0; player < 9; player += 1) {
            put(this.players, player * 180 + 0x7c, (match & 1) ? 50 : 99);
            put(this.players, player * 180 + 0x80, 99);
            put(this.players, player * 180 + 0x84, (match & 2) ? 50 : 99);
            put(this.players, player * 180 + 0x88, 99);
        }
        // 각 실제 할당 슬롯에 native 와 같은 전체 50 BYTE 를 복사한다.
        for (const sid of IDS) {
            const bytes = new Uint8Array(50);
            bytes[10] = sid === 50 ? 124 : 126;
            bytes[11] = sid === 50 ? 2 : 0;
            bytes[34] = sid === 61 ? (owner + 1) % 9 : owner;
            bytes[40] = sid === 50 ? [0, 1, 8, 9][input & 3] : 0;
            put(bytes, 14, floatToBits(sid + 0.25));
            put(bytes, 18, floatToBits(sid + 0.5));
            this.pool.allocated(sid).set(bytes);
        }
        this.temples.entries = [50, 60, 50, 61];
        this.temples.count = 4;
        this.residences.entries = [62, 50, 63, 50];
        this.residences.count = 4;
        this.outposts.entries = [60, 61];
        this.outposts.count = 2;
        this.body = new RawVortexPreDestroy(this.pool, this.players, this.temples, this.residences, this.outposts, () => this.loading, this.hooks());
    }

    // 원본이 외부로 선택한 작은 경계의 현재 반환/변경 입력만 공급한다.
    hooks() {
        const event = (text) => this.trace.push(text);
        return {
            islandNumber: (x, y) => { event(`Island,${floatToBits(x)},${floatToBits(y)}`); return 2; },
            islandExists: (island) => { event(`Exists,${island}`); return !(this.variant & 512); },
            islandTemple: (island) => { event(`Temple,${island}`); return (this.variant & 1024) ? 99 : 50; },
            clearIslandTemple: (island) => { event(`Clear,${island}`); },
            islandOwner: (island, owner) => { event(`Owner,${island},${owner}`); },
            regionOwnership: (x, y) => { event(`Region,${floatToBits(x)},${floatToBits(y)}`); },
            damageablePre: (sid, flags) => {
                event(`Base,${flags}`);
                const raw = this.pool.allocated(sid);
                if (this.mutation === 1) raw[34] = 2;
                if (this.mutation === 2) raw[40] = 9;
                if (this.mutation === 3) this.loading = 1;
            },
            refreshProduction: () => { event('Refresh'); if (this.mutation === 4) this.pool.allocated(50)[34] = 8; },
            assertion: (line) => { event(`Assert,${line}`); }
        };
    }

    // 출력은 실제 비교 대상 세 슬롯의 모든 150 BYTE 이다.
    objects() { return IDS.map((sid) => hex(this.pool.slot(sid))).join(''); }

    // 논리 count 와 사용하지 않는 물리 꼬리까지 원문 순서로 직렬화한다.
    lists() {
        const bytes = new Uint8Array(48);
        put(bytes, 0, this.temples.count);
        put(bytes, 4, this.residences.count);
        let offset = 8;
        // 각 목록은 같은 초기 물리 capacity 를 그대로 유지한다.
        for (const list of [this.temples, this.residences, this.outposts]) for (const value of list.entries) { put(bytes, offset, value); offset += 4; }
        return hex(bytes);
    }
}

test('VortexPreDestroy: 원본 10.78 102입력의 현재 Player·세 객체·물리 목록·호출 순서가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 헤더 이외의 102개를 모두 새로운 실제 입력 저장소에서 재생한다.
    for (const row of rows) {
        const input = row[0].split(',').map(Number);
        assert.ok(row.length === 6 && input.length === 3);
        const scene = new VortexScene(input[0], input[1]);
        scene.body.preDestroy(50, input[2]);
        const label = `행 ${count}: 입력 ${row[0]}`;
        assert.equal(hex(scene.players), row[1], label);
        assert.equal(scene.objects(), row[2], label);
        assert.equal(scene.lists(), row[3], label);
        assert.equal(String(scene.loading), row[4], label);
        assert.equal(scene.trace.join(';'), row[5], label);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('VortexPreDestroy: 필수 Damageable 이 없으면 앞선 실제 효과를 남기고 Player 정리를 실행하지 않으며 잘못된 저장소와 풀 밖 outpost 를 거부한다', () => {
    const scene = new VortexScene(196, 0);
    const before = Uint8Array.from(scene.players);
    const hooks = scene.hooks();
    delete hooks.damageablePre;
    const missing = new RawVortexPreDestroy(scene.pool, scene.players, scene.temples, scene.residences, scene.outposts, () => scene.loading, hooks);
    assert.ok(throws(() => missing.preDestroy(50, 0x400000)));
    assert.ok(scene.temples.count === 2 && scene.residences.count === 2 && Buffer.from(scene.players).equals(Buffer.from(before)));
    assert.ok(throws(() => new RawVortexPreDestroy(scene.pool, scene.players.subarray(0, 180), scene.temples, scene.residences, scene.outposts, () => scene.loading, scene.hooks())));
    assert.ok(throws(() => new RawVortexPreDestroy({ edition: 'cd1072' }, scene.players, scene.temples, scene.residences, scene.outposts, () => scene.loading, scene.hooks())));
    assert.ok(throws(() => new RawVortexPreDestroy(scene.pool, scene.players, scene.temples, scene.residences, scene.outposts, 1, scene.hooks())));
    // 추가 목록의 항목이 풀 밖이면 지역 효과 뒤 거부한다.
    const wide = new VortexScene(196, 0);
    wide.outposts.entries = [30000, 61];
    assert.ok(throws(() => wide.body.preDestroy(50, 0)));
});

// 원문 outpost 반복은 index 와 count 를 signed 로 비교한다: count 의 상위 비트가 서 있으면(음수) 한 번도 반복하지 않아 풀 밖 항목에도 도달하지 않는다(WB04-E260, cpppj 기준선 뒤 VortexPreDestroy 재반영).
test('VortexPreDestroy: 음수로 읽히는 outpost count 는 반복하지 않고 양수 count 의 풀 밖 항목만 거부한다', () => {
    const negative = new VortexScene(196, 0);
    negative.outposts.entries = [30000, 61];
    negative.outposts.count = 0x80000001;
    negative.body.preDestroy(50, 0);
    // count 는 읽기만 하므로 그대로 남는다.
    assert.equal(negative.outposts.count, 0x80000001);
    const positive = new VortexScene(196, 0);
    positive.outposts.entries = [30000, 61];
    positive.outposts.count = 2;
    assert.ok(throws(() => positive.body.preDestroy(50, 0)));
});
