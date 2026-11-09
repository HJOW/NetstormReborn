// 생산 Deck raw-production-deck.js(RawProductionDeckStorage·RawProductionDeck·PlayerLinksPass·RawIndicateTechnology)와 SP 통지 바인딩 raw-sp-store-notice-bindings.js 를
// 원본 10.78 관측과 cpppj 계약(ProductionDeckTests·ProductionDeckReleaseTests)으로 대조한다.
//  - 원본 관측 production-deck-x86.tsv 248행: Get/Contains/Initialize/Clear/Append/Activate/Compact/Indicate 의 입력(모드·templateOwner·type·chance·개수·채움 바이트·Player 포인터 유무·PlayerKnown·battleOwner)에서
//    반환·352 BYTE Deck 전체·Player 9기록 전체·요청한 하위 경계 순서(할당 바이트 N, 진단 A, 종속 부착 C)를 대조한다. 실제 Player/Factory/ContentHost/SP 통지/Carrier 조건 순회로 매 행 새 월드를 만든다.
//  - 계약: 현재 templateOwner·서로 다른 두 owner 의 블록 수명, 물리 범위(누락 할당·모르는 토큰·66 초과 개수)의 쓰기 전 거부, Player 링크 정리 증빙(실제 부모 36번 조회·세 DWORD 밖 보존·실패 접두 보존·
//    다른 모듈/소비/만료 증빙 거부·부모 복귀 전 소비 금지), 개별 반납(한 블록만·Player 쓰기 없음·NULL 무효과·모르는/중복 토큰 거부·소멸자의 별도 정리).
//  - 웹 범위 밖: 재계산/Factory 상품 추가(`RawCampaignProductionBindings`, client 단위)와 장기 FullDeck 전역을 쓰는 cpppj 시험 3건(ProductionDeck_ReplaysFactoryRefreshNative242·CurrentRestrictionsAndFactoriesAreShared·SharedFullDeckReadsAfterModeDiagnostic)은
//    그 client 모듈을 옮길 때 대조한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E215: 생산 Deck 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat } from './raw-support.mjs';
import { realPlayerAssets } from './player-scene-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
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
import { cleanupPlayerLinks } from '../www/core/raw-player-links-cleanup.js';
import { bindSpStoreNotice } from '../www/core/raw-sp-store-notice-bindings.js';
import {
    PRODUCTION_DECK_ASSERTION, PRODUCTION_DECK_BYTES, PRODUCTION_ENTRY_BYTES, PRODUCTION_ENTRY_CAPACITY, PlayerLinksPass, RawIndicateTechnology, RawProductionDeck, RawProductionDeckStorage,
} from '../www/core/raw-production-deck.js';

// 원본 격리 관찰기의 opaque 주소이다. 호스트 포인터로 해석하지 않는다. (2026-11-10 WB04-E215)
const DECK_TOKEN = 0x11100000;

// 원본 관측 248행이다. 없으면 해당 검사를 건너뛴다. (2026-11-10 WB04-E215)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'production-deck-x86.tsv'));

// 예외가 지정 종류로 나는지 본다(종류를 주지 않으면 아무 예외). (2026-11-10 WB04-E215)
function throws(action, kind = null) {
    try { action(); } catch (error) { return kind === null || error instanceof kind; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-10 WB04-E215)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 선택되지 않은 생성/장치/삭제 경계는 빈 성공으로 넘기지 않는다. (2026-11-10 WB04-E215)
function productionMissing() { throw new Error('생산 검사에서 선택하지 않은 실제 하위 경계'); }

// 원본 count/미초기화 바이트/모든 물리 항목을 입력으로만 준비한다(기대 출력이 아님). 활성 개수 밖 꼬리도 보존한다. (2026-11-10 WB04-E215)
function deckInput(values) {
    const raw = new Uint8Array(PRODUCTION_DECK_BYTES).fill(Number(values[5]) & 0xff);
    for (let index = 0; index < PRODUCTION_ENTRY_CAPACITY; index += 1) {
        raw[index * 5] = [0, 71, 158, 255][index % 4];
        raw[index * 5 + 1] = index % 7 + 1;
        raw[index * 5 + 2] = [255, 0, 1, 128, 127][index % 5];
        raw[index * 5 + 3] = 37;
        raw[index * 5 + 4] = index;
    }
    put(raw, 0x14c, Number(values[4]));
    put(raw, 0x150, 0x12345678);
    put(raw, 0x154, 0x23456789);
    put(raw, 0x158, 0x3456789a);
    put(raw, 0x15c, 0x456789ab);
    return raw;
}

// 실제 함수가 보고한 진단 위치를 원본 Deck.cpp line 으로 보존한다. (2026-11-10 WB04-E215)
const ASSERTION_LINE = new Map([
    [PRODUCTION_DECK_ASSERTION.DuplicateType, 100], [PRODUCTION_DECK_ASSERTION.Capacity, 102], [PRODUCTION_DECK_ASSERTION.TypeTruncated, 45],
    [PRODUCTION_DECK_ASSERTION.ChanceTruncated, 46], [PRODUCTION_DECK_ASSERTION.PowerTruncated, 47], [PRODUCTION_DECK_ASSERTION.RemainingTruncated, 48],
]);

// 같은 보호 188 TYPE/Pool/Player9/Factory/ContentHost 를 보관하는 검사 월드이다(cpppj ProductionScene). 월드 전체는 별도 단위이므로 필요한 부품만 같은 풀로 조립한다. (2026-11-10 WB04-E215)
class ProductionScene {
    // values 는 행 입력 9칸: 모드·templateOwner·type·chance·개수·채움 바이트·Player 포인터 유무·PlayerKnown·battleOwner.
    constructor(values) {
        this.types = realPlayerAssets().types.map((record) => ({ ...record }));
        this.pool = new SidPool(32768, { server: true });
        this.hash = new SquidHash();
        this.book = new SquidPostPopState();
        this.post = new SquidPostPop(this.pool, this.types, this.book);
        this.factory = new SquidFactory(this.pool, this.types);
        this.records = new Uint8Array(9 * 180);
        this.state = new PlayerWorldState();
        this.trace = [];
        this.state.templateOwner = Number(values[1]);
        this.state.battleOwner = Number(values[8]);
        this.state.playerKnown = Number(values[7]);
        put(this.records, this.state.battleOwner * 180 + 0x9c, 50);
        this.player = new RawPlayerWorld(this.pool, this.post, this.records, this.state, {});
        this.carrierState = new ContainedFinderState();
        this.carrier = new RawCarrierPreDestroy(this.pool, this.carrierState, { validateDamageable: productionMissing, damageablePre: productionMissing, dependentFound: productionMissing });
        this.random = new GameRandom(0);
        this.sp = new ScrambledSpStore(this.random, () => 1234);
        this.tracking = { value: 0 };
        this.problem = 0;
        this.fallback = 0;
        this.moneyChecks = 0;
        this.gumpType = 3;
        this.event = 0x02000031;
        this.deck = new RawProductionDeck(this.player, (raw) => { raw.fill(Number(values[5]) & 0xff); this.trace.push('N:352'); },
            (where) => this.trace.push(`A:${ASSERTION_LINE.get(where)}`), DECK_TOKEN);
        const mode = values[0];
        if ((mode !== 'Get' && mode !== 'Contains') || Number(values[6])) {
            const token = this.deck.get(this.state.templateOwner);
            assert.equal(token, DECK_TOKEN);
            this.deck.bytes(token).set(deckInput(values));
            if (!Number(values[6])) put(this.records, this.state.templateOwner * 180 + 0x68, 0);
        }
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
            gumpFormType: () => this.gumpType,
            contentEvent: () => this.event,
        }, { money: (owner) => bitsToFloat(this.sp.get(owner)), gump: productionMissing, handle: productionMissing });
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
        this.indicate = new RawIndicateTechnology(this.player, this.carrier, this.create);
        if (mode === 'Indicate') {
            // 실제 할당/생성자로 원본 관찰의 Technology SID 50 을 준비한다.
            for (let sid = 5; sid < 50; sid += 1) assert.equal(this.pool.allocate(2), sid);
            assert.equal(this.factory.create(152, 2), 50);
            // 원본 입력의 종속 SID 60 도 실제 타입 6 생성자로 확보한다.
            for (let sid = 51; sid < 60; sid += 1) assert.equal(this.pool.allocate(2), sid);
            assert.equal(this.factory.createContained(2), 60);
            const raw = this.pool.allocated(60);
            put(raw, 18, Number(values[4]), 2);
            put(raw, 20, Number(values[5]), 2);
            put(raw, 16, 50, 2);
            raw[11] = 8;
            if (Number(values[6])) put(this.pool.allocated(50), 6, 60, 2);
        }
    }

    // 관찰 요청 순서를 원본 fixture 와 동일하게 직렬화한다. (2026-11-10 WB04-E215)
    events() { return this.trace.length === 0 ? '-' : this.trace.join(';'); }
}

test('생산 Deck: 원본 10.78 248입력의 Deck 전체·현재 Player 기록·요청 순서가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    assert.equal(rows.length, 248);
    let checked = 0;
    for (const row of rows) {
        assert.equal(row.length, 5);
        const values = row[0].split(',');
        assert.equal(values.length, 9);
        const scene = new ProductionScene(values);
        const type = Number(values[2]);
        const chance = Number(values[3]);
        let result = '-';
        const mode = values[0];
        if (mode === 'Get') result = String(scene.deck.get(Number(values[1])));
        else if (mode === 'Contains') result = scene.deck.contains(type) ? '1' : '0';
        else if (mode === 'Initialize') scene.deck.initialize(DECK_TOKEN);
        else if (mode === 'Clear') scene.deck.clearEntries(DECK_TOKEN);
        else if (mode === 'Append') scene.deck.append(DECK_TOKEN, type, chance | 0);
        else if (mode === 'Activate') scene.deck.activate(DECK_TOKEN, type, chance | 0);
        else if (mode === 'Compact') scene.deck.compact(DECK_TOKEN);
        else if (mode === 'Indicate') {
            // 원본 생성 관찰 경계 뒤의 호스트 TYPE 범위 계약은 명시 예외이다.
            assert.equal(throws(() => scene.indicate.indicate(type)), type >= scene.types.length, `Indicate ${row[0]}`);
        } else assert.fail(`모르는 모드 ${mode}`);
        const label = `${row[0]} (#${checked})`;
        assert.equal(result, row[1], `${label} 반환`);
        assert.equal(hex(scene.deck.bytes(DECK_TOKEN)), row[2], `${label} Deck`);
        assert.equal(hex(scene.records), row[3], `${label} Player 기록`);
        assert.equal(scene.events(), row[4], `${label} 요청 순서`);
        checked += 1;
    }
    assert.equal(checked, 248);
});

test('생산 Deck 계약: 현재 templateOwner 와 서로 다른 두 owner 의 실제 블록 수명을 보존한다', () => {
    const scene = new ProductionScene('Get,1,0,0,4,167,0,1,1'.split(','));
    const one = scene.deck.get(1);
    const eight = scene.deck.get(8);
    scene.deck.append(one, 71, 1);
    scene.deck.append(eight, 158, 1);
    scene.state.templateOwner = 1;
    assert.ok(scene.deck.contains(71) && !scene.deck.contains(158));
    scene.state.templateOwner = 8;
    assert.ok(scene.deck.contains(158) && !scene.deck.contains(71));
    assert.equal(get(scene.records, 180 + 0x68), one);
    assert.equal(get(scene.records, 8 * 180 + 0x68), eight);
    // 다른 모듈이 교체한 포인터는 지우지 않고 자기 블록만 반납한다.
    put(scene.records, 180 + 0x68, 0x7fffffff);
    scene.deck.dispose();
    assert.equal(get(scene.records, 180 + 0x68), 0x7fffffff);
    assert.equal(get(scene.records, 8 * 180 + 0x68), 0);
    assert.equal(scene.indicate.pool, scene.pool);
});

test('생산 Deck 계약: 누락 할당/모르는 현재 포인터/물리 capacity 를 생산 성공이나 빈 목록으로 숨기지 않는다', () => {
    const scene = new ProductionScene('Initialize,1,0,0,4,167,1,1,1'.split(','));
    assert.ok(throws(() => new RawProductionDeck(scene.player, null)));
    assert.ok(throws(() => scene.deck.get(9)));
    assert.ok(throws(() => scene.deck.bytes(0xffffffff)));
    put(scene.records, 180 + 0x68, 0x7fffffff);
    assert.ok(throws(() => scene.deck.contains(71)));
    put(scene.records, 180 + 0x68, DECK_TOKEN);
    const raw = scene.deck.bytes(DECK_TOKEN);
    put(raw, 0x14c, 66);
    const before = hex(raw);
    assert.ok(throws(() => scene.deck.append(DECK_TOKEN, 83, 1)));
    assert.equal(hex(raw), before);
    put(raw, 0x14c, 67);
    assert.ok(throws(() => scene.deck.contains(71)));
});

// 첫 동작을 할 수 있는 모든 Player 에 Deck 을 붙이고 링크 값을 채운 장면이다. (2026-11-10 WB04-E215)
function linkedScene() {
    const scene = new ProductionScene('Get,1,0,0,4,167,0,1,1'.split(','));
    const tokens = [];
    // 원문이 방문하는 9행의 실제 Deck 만 할당한다.
    for (let owner = 0; owner < 9; owner += 1) {
        tokens[owner] = scene.deck.get(owner);
        put(scene.records, owner * 180 + 0x9c, owner + 31);
        put(scene.records, owner * 180 + 8, owner + 1);
    }
    return { scene, tokens };
}

test('생산 Deck 계약: Player 링크 증빙은 실제 9Player 현재 getter 와 자기 Deck free 가 끝난 뒤에만 소비되어 소비자를 닫는다', () => {
    const { scene, tokens } = linkedScene();
    const before = scene.records.slice();
    let reads = 0;
    const pass = scene.deck.cleanupPlayerLinksForState(() => { reads += 1; return scene.records; });
    assert.equal(reads, 36);
    assert.ok(!scene.deck.playerLinksReturnedForState());
    // 세 DWORD 밖 전체 BYTE 는 실제 부모의 보존 계약이다.
    for (let index = 0; index < scene.records.length; index += 1) {
        const field = index % 180;
        const zero = (field >= 8 && field < 12) || (field >= 0x68 && field < 0x6c) || (field >= 0x9c && field < 0xa0);
        assert.equal(scene.records[index], zero ? 0 : before[index], `byte ${index}`);
    }
    scene.deck.returnPlayerLinksForState(pass);
    assert.ok(scene.deck.playerLinksReturnedForState());
    const allocations = scene.trace.length;
    assert.ok(throws(() => scene.deck.get(1)) && throws(() => scene.deck.release(0)) && throws(() => scene.deck.bytes(tokens[1])));
    assert.equal(scene.trace.length, allocations);
    // 실제 증빙 소비 뒤 dispose 는 old Player 를 재조회하지 않는다.
    scene.deck.dispose();
});

test('생산 Deck 계약: 선택된 잘못된 Deck free 의 앞선 필드 쓰기만 남고 정상 복귀 증빙이나 반납 완료를 만들지 않는다', () => {
    const { scene, tokens } = linkedScene();
    put(scene.records, 3 * 180 + 0x68, 0xdeadcaf0);
    assert.ok(throws(() => scene.deck.cleanupPlayerLinksForState(() => scene.records)));
    assert.ok(!scene.deck.playerLinksReturnedForState());
    for (let owner = 0; owner < 9; owner += 1) {
        assert.equal(get(scene.records, owner * 180 + 0x9c), owner <= 3 ? 0 : owner + 31);
        assert.equal(get(scene.records, owner * 180 + 8), owner <= 3 ? 0 : owner + 1);
        if (owner < 3) assert.equal(get(scene.records, owner * 180 + 0x68), 0);
    }
    // 실패 행 뒤 원문 미방문 필드를 지우지 않는다.
    assert.equal(get(scene.records, 3 * 180 + 0x68), 0xdeadcaf0);
    put(scene.records, 3 * 180 + 0x68, tokens[3]);
    const pass = scene.deck.cleanupPlayerLinksForState(() => scene.records);
    put(scene.records, 8 * 180 + 8, 7);
    assert.ok(throws(() => scene.deck.returnPlayerLinksForState(pass)));
    assert.ok(!scene.deck.playerLinksReturnedForState());
    put(scene.records, 8 * 180 + 8, 0);
    scene.deck.returnPlayerLinksForState(pass);
    assert.ok(scene.deck.playerLinksReturnedForState());
});

test('생산 Deck 계약: 다른 모듈/소비한/만료된 증빙은 정상 복귀 증빙으로 받지 않는다', () => {
    const first = new ProductionScene('Get,1,0,0,4,167,0,1,1'.split(','));
    const second = new ProductionScene('Get,1,0,0,4,167,0,1,1'.split(','));
    const pass = first.deck.cleanupPlayerLinksForState(() => first.records);
    assert.ok(throws(() => second.deck.returnPlayerLinksForState(pass)));
    first.deck.returnPlayerLinksForState(pass);
    assert.ok(first.deck.playerLinksReturnedForState() && !second.deck.playerLinksReturnedForState());
    assert.ok(throws(() => first.deck.returnPlayerLinksForState(pass)));
    // 임의 증빙은 만들 수 없다.
    assert.ok(throws(() => new PlayerLinksPass(Symbol('x'), second.deck, {})));
    // 같은 월드의 새 Deck 은 이전 Deck 이 발급한 증빙을 소비할 수 없다(수명 표식이 다르다).
    let reused = new RawProductionDeck(second.player, (bytes) => bytes.fill(0x71));
    const old = reused.cleanupPlayerLinksForState(() => second.records);
    reused = new RawProductionDeck(second.player, (bytes) => bytes.fill(0x71));
    assert.ok(throws(() => reused.returnPlayerLinksForState(old)) && !reused.playerLinksReturnedForState());
});

test('생산 Deck 계약: 실제 부모의 현재 getter 가 재진입해도 전체 정리 호출이 복귀하기 전에는 이전 증빙을 소비하지 못한다', () => {
    // Deck 을 하나도 할당하지 않은 장면(Player+68 이 모두 0)이라 현재 getter 는 Player 마다 세 번(27번) 읽힌다.
    const empty = new ProductionScene('Get,1,0,0,4,167,0,1,1'.split(','));
    const previous = empty.deck.cleanupPlayerLinksForState(() => empty.records);
    let reads = 0;
    let rejected = false;
    const current = empty.deck.cleanupPlayerLinksForState(() => {
        if (!reads++) {
            try { empty.deck.returnPlayerLinksForState(previous); } catch (error) { rejected = String(error.message).includes('진행 중'); }
        }
        return empty.records;
    });
    assert.equal(reads, 27);
    assert.ok(rejected);
    assert.ok(!empty.deck.playerLinksReturnedForState());
    empty.deck.returnPlayerLinksForState(current);
    assert.ok(empty.deck.playerLinksReturnedForState());
});

// 개별 반납 검사 장면: 같은 풀·Player·Deck 단일 소유자만 구성한다(cpppj DeckReleaseScene). (2026-11-10 WB04-E215)
function releaseScene() {
    const pool = new SidPool(24000, { server: true });
    const types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0, maxHitPoints: 0, zOrder: 0 }));
    const post = new SquidPostPop(pool, types, new SquidPostPopState());
    const records = new Uint8Array(9 * 180);
    const player = new RawPlayerWorld(pool, post, records, new PlayerWorldState(), {});
    const scene = { pool, records, player, fills: 0, deck: null };
    // 각 새 352 BYTE 블록을 다른 입력으로 채워 다른 할당 보존을 확인한다.
    scene.deck = new RawProductionDeck(player, (bytes) => { scene.fills += 1; bytes.fill(0x40 + scene.fills); });
    return scene;
}

test('생산 Deck 반납: 선택한 한 블록만 제거하고 Player 와 다른 블록/할당 순서는 보존한다', () => {
    const scene = releaseScene();
    const first = scene.deck.get(1);
    const other = scene.deck.get(8);
    const records = scene.records.slice();
    const otherBytes = scene.deck.bytes(other).slice();
    scene.deck.release(first);
    assert.deepEqual(scene.records, records);
    assert.equal(scene.fills, 2);
    assert.ok(throws(() => scene.deck.bytes(first), RangeError));
    assert.deepEqual(scene.deck.bytes(other), otherBytes);
    // 부모의 0 쓰기 전 stale Player 포인터를 새 할당으로 숨기지 않는다.
    assert.ok(throws(() => scene.deck.get(1), RangeError));
    // 실제 부모의 후속 0 쓰기를 별도 명시한다.
    put(scene.records, 180 + 0x68, 0);
    const replacement = scene.deck.get(1);
    assert.equal(replacement, other + PRODUCTION_DECK_BYTES);
    assert.notEqual(replacement, first);
    assert.equal(scene.fills, 3);
    assert.equal(scene.deck.bytes(replacement)[0], 0x43);
    assert.equal(scene.deck.bytes(other)[0], 0x42);
});

test('생산 Deck 반납: 원문 Player 링크 부모가 같은 할당 owner 를 호출한 뒤 현재 Player 슬롯을 0 으로 만든다', () => {
    const scene = releaseScene();
    const tokens = [];
    // 모든 실제 Player 기록에 서로 다른 블록을 부착한다.
    for (let owner = 0; owner < 9; owner += 1) {
        tokens[owner] = scene.deck.get(owner);
        put(scene.records, owner * 180 + 0x9c, owner + 20);
        put(scene.records, owner * 180 + 8, owner + 30);
    }
    let calls = 0;
    const hooks = {
        players: () => scene.records,
        release: (token) => {
            assert.ok(calls < tokens.length && token === tokens[calls]);
            const offset = calls * 180;
            assert.ok(get(scene.records, offset + 0x9c) === 0 && get(scene.records, offset + 8) === 0);
            // free 전후 포인터 0 쓰기는 이 API 의 효과가 아니다.
            assert.equal(get(scene.records, offset + 0x68), token);
            const before = scene.records.slice();
            scene.deck.release(token);
            assert.deepEqual(scene.records, before);
            assert.ok(throws(() => scene.deck.bytes(token), RangeError));
            calls += 1;
        },
    };
    cleanupPlayerLinks(hooks);
    assert.equal(calls, 9);
    assert.equal(scene.fills, 9);
    // 실제 부모가 후속 0 쓰기를 끝냈다.
    for (let owner = 0; owner < 9; owner += 1) assert.equal(get(scene.records, owner * 180 + 0x68), 0);
    // 모두 0 이면 실제 free 장치를 선택하지 않는다.
    cleanupPlayerLinks({ players: hooks.players });
    assert.equal(calls, 9);
});

test('생산 Deck 반납: NULL 무효과·모르는/중복 토큰 거부와 소멸자의 별도 Player 정리를 구별한다', () => {
    const scene = releaseScene();
    const first = scene.deck.get(1);
    const other = scene.deck.get(8);
    const before = scene.records.slice();
    scene.deck.release(0);
    assert.ok(throws(() => scene.deck.release(0xffffffff), RangeError));
    assert.deepEqual(scene.records, before);
    scene.deck.release(first);
    assert.ok(throws(() => scene.deck.release(first), RangeError));
    assert.deepEqual(scene.records, before);
    assert.equal(scene.fills, 2);
    assert.equal(scene.deck.bytes(other).length, PRODUCTION_DECK_BYTES);
    // 살아 있는 map 항목에 매인 현재 Player 포인터만 소멸자가 0 으로 만든다.
    scene.deck.dispose();
    assert.equal(get(scene.records, 180 + 0x68), first);
    assert.equal(get(scene.records, 8 * 180 + 0x68), 0);
});

test('생산 Deck 저장소: 첫 주소/Player 범위/할당 바이트 입력을 쓰기 전에 거부하고 장기 저장소를 빌려 쓸 수 있다', () => {
    assert.ok(throws(() => new RawProductionDeckStorage(0)));
    assert.ok(throws(() => new RawProductionDeckStorage(0xfffffff0)));
    const storage = new RawProductionDeckStorage(0x30000000);
    const records = new Uint8Array(9 * 180);
    assert.ok(throws(() => storage.get(records, 9, () => {}), RangeError));
    assert.ok(throws(() => storage.get(new Uint8Array(10), 0, () => {}), RangeError));
    assert.ok(throws(() => storage.get(records, 0, null)));
    assert.equal(storage.get(records, 0, (bytes) => bytes.fill(7)), 0x30000000);
    assert.equal(storage.get(records, 1, (bytes) => bytes.fill(8)), 0x30000160);
    // 이미 붙은 포인터는 그대로 돌려주고 다시 할당하지 않는다.
    assert.equal(storage.get(records, 0, () => assert.fail('이미 부착됨')), 0x30000000);
    assert.ok(storage.has(0x30000000) && !storage.isEmpty());
    // 같은 저장소를 빌린 두 번째 Deck 은 같은 블록을 본다.
    const pool = new SidPool(24000, { server: true });
    const types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0 }));
    const player = new RawPlayerWorld(pool, new SquidPostPop(pool, types, new SquidPostPopState()), records, new PlayerWorldState(), {});
    const borrowed = new RawProductionDeck(player, () => assert.fail('이미 할당됨'), null, 0x30000000, storage);
    assert.equal(borrowed.get(0), 0x30000000);
    assert.equal(borrowed.bytes(0x30000000)[0], 7);
    assert.equal(PRODUCTION_ENTRY_BYTES * PRODUCTION_ENTRY_CAPACITY + 22, PRODUCTION_DECK_BYTES);
});

test('생산 Deck 계약: 기술 표시는 저장 kind WORD 의 높은 BYTE 만 있어도 이미 표시된 것으로 보고 생성하지 않는다(WORD 폭)', () => {
    // 종속 SID 60 의 kind WORD +18 이 0x0100 이고 같은 값으로 조회하면 일치하며 낮은 BYTE 만 읽으면 0 으로 보여 생성으로 잘못 진행한다.
    const scene = new ProductionScene('Indicate,1,256,0,256,4,1,1,1'.split(','));
    assert.doesNotThrow(() => scene.indicate.indicate(256));
    assert.ok(!scene.trace.some((item) => item.startsWith('C:')));
});
