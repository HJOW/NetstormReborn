// Carrier 후처리 raw-carrier-post-pop.js(RawCarrierPostPop·CarrierPostPopState·makeCarrierPostPopHooks)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: carrierpostpop-x86.tsv 의 10.78(originals) 2448행(Carrier 본체 00427720 / Damageable 본체 0044bf80: flags·boss·로딩·TYPE·간헐천 TYPE·extra·flags1·상태·소유자·좌표 → +0xcc/지면 소유자/공통 postPop 호출 기록과 슬롯 50바이트).
//    CD 10.72·10.37 행은 웹 범위(10.78) 밖이다.
//  - 계약: carrier_postpop_direct_base_preserves_virtual_guards_and_bookkeeping(직접 공통 호출의 장부·AI 실패 앞 지면/비용·억제 깊이)·
//    CarrierPostPop_SharedBossChangesBetweenCallsAndKeepsFirstFlag(현재 boss 재조회)의 10.78 부분이다. 사제 접두·Process·Reward 와 조합하는 carrier_postpop_composes_priest_prefix_*
//    는 사제 후처리(RawPriestPostPop) 이식 단위의 몫이다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E60: Carrier 후처리 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawCarrierPostPop, CarrierPostPopState, makeCarrierPostPopHooks } from '../www/core/raw-carrier-post-pop.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E60)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'carrierpostpop-x86.tsv'))?.filter((row) => row[1] === 'originals') ?? null;

// 관찰 SID, 사제 TYPE 번호와 10.78 사제 가상 표, 판본당 관찰 행 수이다. (2026-11-08 WB04-E60)
const SOURCE = 50;
const PRIEST_TYPE = 158;
const PATCH_PRIEST_VTABLE = 0x50f210;
const ROWS = 2448;

// 예외가 나는지 본다. (2026-11-08 WB04-E60)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E60)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 188칸 TYPE 표이다(flags1·flags2 와 사제 비용). (2026-11-08 WB04-E60)
const carrierTypes = () => Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, cost: 0 }));

// 관찰 SID 50 까지 client 번호를 확보한 장면이다. (2026-11-08 WB04-E60)
function carrierScene() {
    const pool = new SidPool(24000, { server: false });
    // 최초 client 번호부터 대상까지 확보한다.
    for (let id = 5; id <= SOURCE; id += 1) assert.equal(pool.allocate(2), id);
    const scene = { pool, types: carrierTypes(), state: new CarrierPostPopState(), events: [], afterType: 0, afterExtra: 0, afterOwner: 0, groundOwner: 0, validations: 0 };
    // 입력만 준비하며 기대 분기/호출 순서는 계산하지 않는다.
    scene.prepare = (row) => {
        assert.equal(row.length, 19);
        scene.events = [];
        scene.validations = 0;
        scene.state.boss = Number(row[3]) !== 0;
        scene.state.loadingDepth = Number(row[4]);
        scene.state.geyserType = Number(row[6]);
        // 가상 효과 뒤 타입 재조회에서도 같은 입력 flags 를 사용한다.
        for (const number of [82, 122, 158]) {
            scene.types[number].flags1 = Number(row[8]) >>> 0;
            scene.types[number].flags2 = 0x00210000;
        }
        scene.afterType = Number(row[13]);
        scene.afterExtra = Number(row[14]);
        scene.afterOwner = Number(row[15]);
        scene.groundOwner = Number(row[16]);
        const raw = pool.allocated(SOURCE);
        raw.fill(0);
        put(raw, 0, PATCH_PRIEST_VTABLE);
        raw[10] = Number(row[5]);
        raw[11] = Number(row[9]);
        raw[40] = Number(row[7]);
        raw[34] = Number(row[10]);
        put(raw, 12, 0xab84, 2);
        put(raw, 14, Number(row[11]) >>> 0);
        put(raw, 18, Number(row[12]) >>> 0);
    };
    // 원본의 외부 경계와 같은 위치에서 사건/필드 쓰기를 주입한다. 보호 검사는 사건으로 세지 않는다.
    scene.hooks = () => ({
        carrierCheck: (sid) => {
            assert.equal(scene.validations, 0, '실제 PATCH 는 공통 전체 사전 검사를 호출하지 않는다');
            scene.events.push(`C:${sid}`);
            if (scene.afterType >= 0) {
                const raw = pool.allocated(sid);
                raw[10] = scene.afterType;
                raw[40] = scene.afterExtra;
                raw[34] = scene.afterOwner;
                put(raw, 14, 0x41f20000);
                put(raw, 18, 0x41fc0000);
            }
        },
        claimGround: (x, y, owner) => {
            assert.equal(scene.validations, 0);
            scene.events.push(`G:${floatToBits(x)}:${floatToBits(y)}:${owner}`);
            if (scene.groundOwner >= 0) pool.allocated(SOURCE)[34] = scene.groundOwner;
        },
        validateBase: (sid) => {
            assert.equal(sid, SOURCE);
            scene.validations += 1;
        },
        base: (sid, flags) => {
            assert.equal(scene.validations, 0);
            const raw = pool.slot(sid);
            scene.events.push(`B:${sid}:${flags}:${raw[10]}:${raw[40]}:${raw[34]}`);
        }
    });
    scene.eventText = () => scene.events.join(';');
    return scene;
}

test('CarrierPostPop: 원본 10.78 2448행의 사건·좌표 비트·효과 뒤 슬롯 전체가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = carrierScene();
    let count = 0;
    // fixture 가 정한 직접 호출 종류를 그대로 실행한다.
    for (const row of rows) {
        scene.prepare(row);
        const carrier = new RawCarrierPostPop(scene.pool, scene.types, scene.state, scene.hooks());
        if (row[0] === 'Carrier') carrier.postPop(SOURCE, Number(row[2]));
        else carrier.damageablePostPop(SOURCE, Number(row[2]));
        assert.equal(scene.eventText(), row[17], `행 ${count}`);
        assert.equal(hex(scene.pool.slot(SOURCE)), row[18], `행 ${count}`);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('CarrierPostPop: 직접 공통 호출은 기존 장부를 쓰고 AI 실패 앞의 지면/비용과 억제 깊이를 보존한다', () => {
    const scene = carrierScene();
    const first = rows?.[0] ?? ['Carrier', 'originals', '0', '0', '0', '158', '122', '0', '430098', '0', '0', '1101398016', '1102000947', '-1', '-1', '-1', '-1', '', ''];
    const row = [...first];
    row[3] = '1';
    row[4] = '1';
    row[7] = '0';
    row[8] = '431122';
    row[10] = '1';
    scene.prepare(row);
    scene.types[PRIEST_TYPE].cost = 123.75;
    const book = new SquidPostPopState();
    book.localOwner = 1;
    book.totalCost = 10;
    book.depth = 1;
    const base = new SquidPostPop(scene.pool, scene.types, book);
    assert.ok(throws(() => base.validate(SOURCE, 1)));
    assert.ok(throws(() => base.activate(SOURCE, 1)));
    // 직접 공통 호출과 가상/명시 Activate 지원 범위를 구별한다.
    const direct = new SquidPostPopState();
    direct.localOwner = 1;
    direct.totalCost = 10;
    direct.depth = 1;
    new SquidPostPop(scene.pool, scene.types, direct).postPop(SOURCE, 1);
    assert.ok(direct.depth === 0 && direct.totalCost === 133 && direct.globalCounts()[PRIEST_TYPE] === 1);
    assert.ok(book.depth === 1 && book.totalCost === 10);
    let claimed = 0;
    let xbits = 0;
    let ybits = 0;
    const hooks = makeCarrierPostPopHooks(scene.pool, base, () => {}, (x, y, owner) => {
        claimed += 1;
        xbits = floatToBits(x);
        ybits = floatToBits(y);
        assert.equal(owner, 1);
    });
    const carrier = new RawCarrierPostPop(scene.pool, scene.types, scene.state, hooks);
    carrier.postPop(SOURCE, 1);
    assert.ok(claimed === 1 && xbits === Number(row[11]) && ybits === Number(row[12]));
    assert.ok(book.depth === 0 && book.totalCost === 133 && book.localCounts()[PRIEST_TYPE] === 1 && book.localSecondaryCounts()[PRIEST_TYPE] === 1);
    assert.ok(book.globalCounts()[PRIEST_TYPE] === 1 && book.providers.count === 0 && book.factories.count === 0);
    // 실제 PATCH 의 AI 실패 앞에는 지면/비용을 이미 기록한다.
    book.aiAttached[1] = true;
    book.depth = 1;
    assert.ok(throws(() => carrier.postPop(SOURCE, 1)));
    assert.ok(claimed === 2 && book.depth === 1 && book.totalCost === 256 && book.globalCounts()[PRIEST_TYPE] === 1);
    // 억제된 공통 몸체도 한 번만 깊이를 줄인다. 억제는 damageable 지면 효과를 끄지 않는다.
    book.suppressed = true;
    carrier.damageablePostPop(SOURCE, 1);
    assert.ok(claimed === 3 && book.depth === 0 && book.totalCost === 256);
    assert.ok(throws(() => makeCarrierPostPopHooks(new SidPool(24000, { server: false }), base, () => {}, () => {})));
});

test('CarrierPostPop: 비최초 검사만 현재 boss 를 읽으며 첫 Pop flag 는 실제 전역을 덮지 않는다', () => {
    const scene = carrierScene();
    const raw = scene.pool.allocated(SOURCE);
    raw.fill(0);
    raw[10] = 158;
    let boss = false;
    scene.state.sharedBoss = () => boss;
    scene.state.boss = true;
    let checks = 0;
    let bases = 0;
    const hooks = {
        carrierCheck: (sid) => { assert.equal(sid, SOURCE); checks += 1; },
        claimGround: () => { throw new Error('로딩0에서 지면 claim 미도달'); },
        validateBase: () => { scene.validations += 1; },
        base: () => { bases += 1; }
    };
    const post = new RawCarrierPostPop(scene.pool, scene.types, scene.state, hooks);
    post.postPop(SOURCE, 0);
    assert.ok(checks === 1 && bases === 1);
    boss = true;
    post.postPop(SOURCE, 0);
    assert.ok(checks === 1 && bases === 2);
    boss = false;
    post.postPop(SOURCE, 1);
    assert.ok(checks === 1 && bases === 3 && scene.validations === 0);
});

test('CarrierPostPop: 현재 TYPE 대여와 공유 로딩/간헐천 getter, 장치 누락 거부', () => {
    const scene = carrierScene();
    assert.ok(throws(() => new RawCarrierPostPop(scene.pool, scene.types.slice(0, 100), scene.state, {})));
    const carrier = new RawCarrierPostPop(scene.pool, scene.types, scene.state, { base: () => {} });
    assert.ok(throws(() => carrier.bindCurrentTypes(carrierTypes().slice(0, 10))), '잘못된 대여는 기존 연결을 바꾸기 전에 거부');
    // 실제 World 의 현재 표를 대여하면 최초 flag 의 genus 조회가 그 표의 범위를 따른다.
    const current = carrierTypes();
    carrier.bindCurrentTypes(current);
    const raw = scene.pool.allocated(SOURCE);
    raw.fill(0);
    raw[10] = 187;
    carrier.damageablePostPop(SOURCE, 1);
    carrier.bindCurrentTypes(null);
    raw[10] = 255;
    assert.ok(throws(() => carrier.damageablePostPop(SOURCE, 1)), '낮은 프로세스 TYPE 도 원문처럼 물리 범위 안만 허용');
    // 로딩 중 claimGround 장치가 없으면 도달한 위치에서 거부한다.
    scene.state.sharedLoadingDepth = () => 1;
    scene.state.sharedGeyserType = () => 122;
    raw[10] = 158;
    raw[40] = 0;
    scene.types[158].flags1 = 0x400;
    carrier.bindCurrentTypes(scene.types);
    assert.ok(throws(() => carrier.damageablePostPop(SOURCE, 0)));
    assert.ok(scene.state.readLoadingDepth() === 1 && scene.state.readGeyserType() === 122 && scene.state.isBoss() === true);
});
