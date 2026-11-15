// 생산 Deck 재계산 연결 raw-campaign-production-bindings.js(RawCampaignProductionBindings)를 원본 10.78 관측 242행과 cpppj 계약으로 대조한다(4-D C 구획, WB04-E522).
//  - 원본 관측 production-refresh-x86.tsv 242행: Refresh/Factory(mode 0)의 입력(모드·owner·미션·Fort/Battle·fullDeck·설정·금지 비트·레벨·변형)과 입력 Deck 704 hex 에서 반환 토큰/개수·352 BYTE Deck 전체·
//    요청 순서(설정 질의 Q, 모드 진단 A:331)를 같은 Player/Factory/내용물/허용표로 대조하고 Player 9기록·풀 전체·금지 표가 변하지 않음을 확인한다. 매 행 새 월드를 만든다.
//  - cpppj 계약(E215 에서 이월한 시험 3건): ProductionDeck_ReplaysFactoryRefreshNative242 ·ProductionDeck_CurrentRestrictionsAndFactoriesAreShared(허용표/Factory 목록/설정/templateOwner 의 현재 값 재조회) ·
//    ProductionDeck_SharedFullDeckReadsAfterModeDiagnostic(모드 진단 뒤 장기 fullDeck DWORD 를 다시 읽어 자연 반환을 고름).
//  - 웹 전용 거부: 다른 풀/Player/Deck 연결, 미복원 Factory 가상 표, 범위 밖 Factory SID/TYPE, deckToken 0 의 Deck 불변.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E522)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat } from './raw-support.mjs';
import { realPlayerAssets } from './player-scene-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { PlayerWorldState, RawPlayerWorld } from '../www/core/raw-player-world.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { RawCarrierPreDestroy } from '../www/core/raw-carrier-pre-destroy.js';
import { INDEPENDENT_CONTAINED_STATE, RawContainedCreate } from '../www/core/contained-create.js';
import { ContentFormHost } from '../www/core/content-form-host.js';
import { RawPlayerContentNotice } from '../www/core/raw-player-content-notice.js';
import { bindSpStoreNotice } from '../www/core/raw-sp-store-notice-bindings.js';
import { RawTotalMade } from '../www/core/raw-total-made.js';
import { PRODUCTION_DECK_BYTES, RawProductionDeck } from '../www/core/raw-production-deck.js';
import { RawCampaignProductionBindings } from '../www/core/raw-campaign-production-bindings.js';

// 원본 격리 관찰기의 opaque 주소와 원본 관측 242행이다. 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E522)
const DECK_TOKEN = 0x11100000;
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'production-refresh-x86.tsv'));

// 선택되지 않은 생성/장치/삭제 경계는 빈 성공으로 넘기지 않는다. (2026-11-15 WB04-E522)
function productionMissing() { throw new Error('생산 검사에서 선택하지 않은 실제 하위 경계'); }

// 바이트 배열을 소문자 hex 로 만들고 hex 문자열을 Deck 바이트로 복원한다. (2026-11-15 WB04-E522)
const hex = (bytes) => Buffer.from(bytes).toString('hex');
function deckBytes(text) {
    assert.equal(text.length, PRODUCTION_DECK_BYTES * 2);
    return Uint8Array.from(text.match(/../g), (pair) => Number.parseInt(pair, 16));
}

// 같은 보호 188 TYPE/Pool/Player9/Factory/ContentHost 를 보관하는 검사 월드이다(cpppj ProductionScene 의 Initialize 경로). (2026-11-15 WB04-E522)
class ProductionScene {
    // owner 는 templateOwner 이자 battleOwner 이다. 같은 풀에 Player/Deck/SP 통지/내용물 사슬을 조립한다.
    constructor(owner) {
        this.types = realPlayerAssets().types.map((record) => ({ ...record }));
        this.pool = new SidPool(32768, { server: true });
        this.book = new SquidPostPopState();
        this.post = new SquidPostPop(this.pool, this.types, this.book);
        this.factory = new SquidFactory(this.pool, this.types);
        this.records = new Uint8Array(9 * 180);
        this.state = new PlayerWorldState();
        this.trace = [];
        this.state.templateOwner = owner;
        this.state.battleOwner = owner;
        this.state.playerKnown = 1;
        put(this.records, owner * 180 + 0x9c, 50);
        this.player = new RawPlayerWorld(this.pool, this.post, this.records, this.state, {});
        this.carrierState = new ContainedFinderState();
        this.carrier = new RawCarrierPreDestroy(this.pool, this.carrierState, { validateDamageable: productionMissing, damageablePre: productionMissing, dependentFound: productionMissing });
        this.random = new GameRandom(0);
        this.sp = new ScrambledSpStore(this.random, () => 1234);
        this.tracking = { value: 0 };
        this.problem = 0;
        this.fallback = 0;
        this.moneyChecks = 0;
        this.deck = new RawProductionDeck(this.player, (raw) => { raw.fill(167); this.trace.push('N:352'); }, productionMissing, DECK_TOKEN);
        const token = this.deck.get(owner);
        assert.equal(token, DECK_TOKEN);
        this.trace = [];
        this.sp.bindTrackingOffset(this.tracking);
        this.sp.initialize();
        const reads = bindSpStoreNotice(this.sp);
        const scene = this;
        this.notice = new RawPlayerContentNotice(this.pool, {
            cursorA: reads.cursorA,
            cursorB: reads.cursorB,
            get problemCode() { return scene.problem; },
            set problemCode(value) { scene.problem = value; },
            fallbackCode: () => this.fallback,
            get moneyChecks() { return scene.moneyChecks; },
            set moneyChecks(value) { scene.moneyChecks = value; },
            battleShown: () => this.state.playerKnown,
            localPlayer: () => this.state.battleOwner,
            firstSpPool: reads.firstSpPool,
            firstPermutation: reads.firstPermutation,
            gumpFormType: () => 3,
            contentEvent: () => 0x02000031,
        }, { money: (who) => bitsToFloat(this.sp.get(who)), gump: productionMissing, handle: productionMissing });
        this.forms = new ContentFormHost(this.pool, this.types, (parent, form, reason) => this.notice.notify(parent, form, reason));
        this.create = new RawContainedCreate(this.pool, INDEPENDENT_CONTAINED_STATE, {
            create: (type, flags) => { assert.equal(type, 6); return this.factory.createContained(flags); },
            notifySurface: productionMissing,
            attach: (form, parent, flags) => {
                const raw = this.pool.slot(form);
                this.trace.push(`C:${parent},${get(raw, 18, 2)},${get(raw, 12, 2)},${get(raw, 20, 2)},${flags},${raw[30]}`);
                this.forms.attach(form, parent, flags);
            },
        });
        // 월드 Core 뷰의 필요한 부분이다(같은 풀/TYPE/공통 장부/소유자 모드).
        this.core = { pool: this.pool, types: this.types, bookkeeping: this.book, ownerMode: { fort: false, battle: false } };
    }

    // 관찰 요청 순서를 원본 fixture 와 동일하게 직렬화한다. (2026-11-15 WB04-E522)
    events() { return this.trace.length === 0 ? '-' : this.trace.join(';'); }

    // 같은 입력으로 연결을 만든다. state 는 사용 때마다 현재 값을 읽는 객체이다.
    bindings(allowed, state, effects) {
        return new RawCampaignProductionBindings(this.core, this.player, this.carrier, this.create, this.deck, allowed, this.carrierState, state, effects);
    }

    // native 재계산 도구와 동일한 실제 Factory/내용물 입력만 준비한다. 반환값이나 포함 여부는 계산하지 않는다. (cpppj RefreshNodes)
    refreshNodes(variant, level) {
        assert.equal(this.factory.create(85, 2), 5);
        assert.equal(this.factory.create(124, 2), 6);
        assert.equal(this.factory.create(86, 2), 7);
        assert.equal(this.pool.allocate(2), 8);
        assert.equal(this.pool.allocate(2), 9);
        // 모든 종속은 기존 실제 타입 6 생성자로 먼저 확보한다.
        for (let sid = 10; sid < 14; sid += 1) assert.equal(this.factory.createContained(2), sid);
        const nodes = [
            [5, 85, (level << 5) | [0, 1, 2, 4][variant % 4], 1, 10, 0, 0, 0],
            [6, 124, 0, variant % 5 === 1 ? 2 : 1, 0, 0, 0, 0],
            [7, 86, 0, variant % 5 === 2 ? 2 : 1, 13, 0, 0, 0],
            [10, 6, 8, 0, 0, 11, 71, 0x10],
            [11, 6, 1, 0, 0, 12, variant % 4 === 2 ? 83 : 71, variant % 3 ? 0x10 : 1],
            [12, 6, 2, 0, 0, 0, 158, 0x10],
            [13, 6, 0, 0, 0, 0, 83, 0x10],
        ];
        // 실제 VT/TYPE 를 보존하고 원문 관찰의 현재 owner/head/next/kind/mask/state 만 명시 입력한다.
        for (const input of nodes) {
            const raw = this.pool.allocated(input[0]);
            assert.equal(raw[10], input[1]);
            raw[34] = input[3];
            put(raw, 6, input[4], 2);
            put(raw, 4, input[5], 2);
            if (input[1] === 6) {
                put(raw, 18, input[6], 2);
                put(raw, 20, input[7], 2);
            }
            raw[11] = input[2];
        }
        this.book.factories.entries = [5, 6, 7];
        this.book.factories.count = 3;
        assert.ok((this.types[85].flags1 & 0x04000000) !== 0 && (this.types[85].flags2 & 0x4000) !== 0 && (this.types[124].flags2 & 0x200) !== 0);
    }
}

test('생산 재계산: 원본 10.78 242입력의 Deck 전체·반환·요청 순서가 같고 Player/풀/금지 표는 변하지 않는다', { skip: !rows && '원본 fixture 없음' }, () => {
    assert.equal(rows.length, 242);
    let checked = 0;
    // 같은 입력을 실제 월드/Factory/Player 수명에 매 행 새로 설치한다.
    for (const row of rows) {
        assert.equal(row.length, 5);
        const values = row[0].split(',');
        assert.equal(values.length, 10);
        const owner = Number(values[1]);
        const variant = Number(values[9]);
        const level = Number(values[8]);
        const scene = new ProductionScene(owner);
        scene.refreshNodes(variant, level);
        scene.deck.bytes(DECK_TOKEN).set(deckBytes(row[1]));
        scene.core.ownerMode.fort = Number(values[3]) !== 0;
        scene.core.ownerMode.battle = Number(values[4]) !== 0;
        const restrictions = new Uint32Array(256);
        const blocked = Number(values[7]);
        restrictions[71] = blocked & 1;
        restrictions[133] = blocked & 2;
        const allowed = new RawTotalMade(scene.types, scene.book, restrictions, productionMissing);
        const state = { mission: Number(values[2]) !== 0, fullDeck: Number(values[5]), freeTechnologyType: 153, templeProduct: 133 };
        const effects = {
            configInt: (key) => { assert.equal(key, 'getTeleportTechFree'); scene.trace.push(`Q:${key}`); return Number(values[6]) | 0; },
            modeAssertion: () => { scene.trace.push('A:331'); },
        };
        const production = scene.bindings(allowed, state, effects);
        const records = Uint8Array.from(scene.records);
        const before = Uint8Array.from(scene.pool.bytes());
        let result;
        if (values[0] === 'Refresh') result = production.refresh(owner);
        else {
            assert.equal(values[0], 'Factory');
            result = production.appendFactoryProducts(5, DECK_TOKEN) >>> 0;
        }
        const label = `행 ${checked} (${row[0]})`;
        assert.equal(result >>> 0, Number(row[2]) >>> 0, label);
        assert.equal(hex(scene.deck.bytes(DECK_TOKEN)), row[3], label);
        assert.equal(scene.events(), row[4], label);
        assert.deepEqual(scene.records, records, label);
        assert.deepEqual(Uint8Array.from(scene.pool.bytes()), before, label);
        assert.equal(restrictions[71], blocked & 1, label);
        assert.equal(restrictions[133], blocked & 2, label);
        assert.equal(production.pool, scene.pool);
        assert.equal(production.deck, scene.deck);
        checked += 1;
    }
    assert.equal(checked, 242);
});

test('생산 재계산 계약: 허용표·Factory 목록·설정·templateOwner 의 현재 값을 호출 때마다 다시 읽는다', () => {
    const scene = new ProductionScene(1);
    scene.refreshNodes(0, 0);
    scene.deck.clearEntries(DECK_TOKEN);
    scene.core.ownerMode.battle = true;
    const restrictions = new Uint32Array(256);
    const allowed = new RawTotalMade(scene.types, scene.book, restrictions, productionMissing);
    const state = { mission: true, fullDeck: 0, freeTechnologyType: 153, templeProduct: 133 };
    let config = 0;
    const production = scene.bindings(allowed, state, { configInt: (key) => { assert.equal(key, 'getTeleportTechFree'); return config; }, modeAssertion: productionMissing });
    production.refreshCurrent();
    assert.ok(production.contains(71) && production.contains(133) && !production.contains(153));
    allowed.allow(71, 0);
    allowed.allow(133, 0);
    production.refreshCurrent();
    // 일반 Factory 는 허용 조회 뒤에도 포함하고 신전 기본 생산만 금지 조건으로 제외한다.
    assert.ok(production.contains(71) && !production.allowed(71) && !production.contains(133) && !production.allowed(133));
    // Factory 목록을 비우면 기존 Deck 은 유지되지만 다음 재계산에서 사라진다.
    scene.book.factories.count = 0;
    assert.ok(production.contains(71));
    production.refreshCurrent();
    assert.ok(!production.contains(71));
    config = 1;
    production.refreshCurrent();
    assert.ok(production.contains(153));
    // 현재 templateOwner 가 바뀌면 다른 Player 의 Deck 을 읽고 재계산한다.
    scene.state.templateOwner = 8;
    assert.ok(!production.contains(153));
    production.refreshCurrent();
    assert.ok(production.contains(153));
});

test('생산 재계산 계약: 모드 진단 뒤 장기 fullDeck DWORD 를 다시 읽어 자연 반환을 고른다', () => {
    const scene = new ProductionScene(1);
    scene.deck.clearEntries(DECK_TOKEN);
    scene.deck.append(DECK_TOKEN, 71, 1);
    scene.deck.bytes(DECK_TOKEN)[2] = 0x7e;
    scene.core.ownerMode.fort = false;
    scene.core.ownerMode.battle = false;
    const allowed = new RawTotalMade(scene.types, scene.book, new Uint32Array(256), productionMissing);
    // 장기 소유자의 현재 DWORD 를 속성 getter 로 빌린다.
    const globals = { fullDeck: 0 };
    const state = { mission: false, get fullDeck() { return globals.fullDeck; }, freeTechnologyType: 153, templeProduct: 133 };
    let queries = 0;
    let diagnostics = 0;
    const production = scene.bindings(allowed, state, {
        configInt: () => { queries += 1; return 0; },
        modeAssertion: () => { diagnostics += 1; globals.fullDeck = 0x80000000; },
    });
    const before = hex(scene.deck.bytes(DECK_TOKEN));
    // Fort/Battle 가 모두 꺼져 진단을 실행하고, 진단이 쓴 fullDeck 이 현재 값이라 자연 반환한다.
    assert.equal(production.refreshCurrent(), DECK_TOKEN);
    assert.equal(diagnostics, 1);
    assert.equal(queries, 0);
    assert.equal(globals.fullDeck, 0x80000000);
    assert.equal(hex(scene.deck.bytes(DECK_TOKEN)), before);
    // Battle 모드에서는 진단 없이 fullDeck 0xffffffff 가 자연 반환한다.
    scene.core.ownerMode.battle = true;
    globals.fullDeck = 0xffffffff;
    production.refreshCurrent();
    assert.equal(queries, 0);
    assert.equal(hex(scene.deck.bytes(DECK_TOKEN)), before);
    assert.equal(globals.fullDeck, 0xffffffff);
    // fullDeck 0 이면 재계산을 실행해 설정을 질의하고 항목이 모두 사라진다.
    globals.fullDeck = 0;
    production.refreshCurrent();
    assert.equal(queries, 1);
    assert.equal(get(scene.deck.bytes(DECK_TOKEN), 0x14c), 0);
    assert.equal(globals.fullDeck, 0);
});

test('생산 재계산 계약: 다른 풀/Player/Deck 연결과 미복원 Factory·범위 밖 SID/TYPE·진단/설정 미연결을 숨기지 않는다', () => {
    const scene = new ProductionScene(1);
    const other = new ProductionScene(1);
    const allowed = new RawTotalMade(scene.types, scene.book, new Uint32Array(256), productionMissing);
    const state = { mission: true, fullDeck: 0, freeTechnologyType: 153, templeProduct: 133 };
    const good = { configInt: () => 0, modeAssertion: () => {} };
    // 다른 풀의 Player/Deck 은 연결 전에 거부한다.
    assert.throws(() => new RawCampaignProductionBindings(other.core, scene.player, scene.carrier, scene.create, scene.deck, allowed, scene.carrierState, state, good), TypeError);
    assert.throws(() => new RawCampaignProductionBindings(scene.core, other.player, scene.carrier, scene.create, scene.deck, allowed, scene.carrierState, state, good), TypeError);
    assert.throws(() => new RawCampaignProductionBindings(scene.core, scene.player, scene.carrier, scene.create, other.deck, allowed, scene.carrierState, state, good), TypeError);
    const production = scene.bindings(allowed, state, good);
    scene.refreshNodes(0, 0);
    // 실제 Factory 두 가상 표만 용량 몸체가 복원돼 있다(미복원 가상 표 거부).
    assert.equal(production.factoryCapacity(5), 2 + (scene.pool.slot(5)[11] >> 5));
    assert.throws(() => production.factoryCapacity(10), /미복원/);
    // deckToken 0 은 Deck 을 바꾸지 않고 남은 자리만 센다.
    const before = hex(scene.deck.bytes(DECK_TOKEN));
    assert.equal(typeof production.appendFactoryProducts(5, 0), 'number');
    assert.equal(hex(scene.deck.bytes(DECK_TOKEN)), before);
    // 손상된 Factory 목록의 SID 는 호스트 주소를 만들기 전에 거부한다.
    scene.book.factories.entries = [0, 6, 7];
    assert.throws(() => production.refresh(1), RangeError);
    scene.book.factories.entries = [65535, 6, 7];
    assert.throws(() => production.refresh(1), RangeError);
    // 모드 진단/설정 소비자가 없으면 성공으로 대신하지 않는다.
    const noEffects = scene.bindings(allowed, state, {});
    scene.core.ownerMode.battle = false;
    assert.throws(() => noEffects.refresh(1), /진단미연결/);
    scene.core.ownerMode.battle = true;
    scene.book.factories.entries = [5, 6, 7];
    assert.throws(() => noEffects.refresh(1), /설정미연결/);
});
