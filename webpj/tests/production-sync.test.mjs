// 생산 내용물 동기화 raw-production-sync.js(RawProductionSync)를 원본 10.78 관측 98입력과 cpppj `ProductionSyncTests` 계약으로 대조한다.
//  - 원본 관측 production-sync-x86.tsv 98행: 현재 기술 부모의 내용물(kind:flags 목록, BYTE seen 의 255/256/257개 중복 포함)과 Deck 항목(kind:power)·TYPE 프로필(신전 flags2 비트 4)·신전 유무·fullDeck·
//    기술 부모 변경·기존 종속 조회 반환에서 호출 순서와 인자 전체(Deck 조회 K·기술 부모 T·조회 Q·취소 X·삭제 D·생성 C·진단 A)와 풀 불변을 대조한다.
//  - 계약: 장기 FullDeck 전역을 Deck getter 효과 뒤 현재 값으로 읽음, 기술 부모 SID 0 은 예약 슬롯을 읽고 빈 Deck 에서는 조회/생성하지 않음, 선택된 실제 하위 누락·범위 밖 kind/부모/Deck 개수의 거부와 signed 음수 개수의 빈 순회.
// 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E216: 생산 내용물 동기화 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { PRODUCTION_DECK_BYTES } from '../www/core/raw-production-deck.js';
import { RawProductionSync } from '../www/core/raw-production-sync.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-10 WB04-E216)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'production-sync-x86.tsv'));

// 예외가 나는지 본다. (2026-11-10 WB04-E216)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 'kind:값;kind:값' 목록을 두 정수 쌍으로 읽는다('-' 는 빈 목록). 기대 동작은 만들지 않는다. (2026-11-10 WB04-E216)
function pairs(text) {
    if (text === '-') return [];
    return text.split(';').map((item) => {
        const fields = item.split(':');
        assert.equal(fields.length, 2);
        return [Number(fields[0]), Number(fields[1])];
    });
}

// 메시지의 원본 바이트를 그대로 기록한다. (2026-11-10 WB04-E216)
const textBytes = (text) => Buffer.from(text, 'utf8').toString('hex');

// 원본 실행기에 공급한 동일 풀/TYPE/Deck/조회 입력이다(cpppj SyncScene). 실제 새 계산은 하지 않고 원본과 같은 경계 입력만 만든다. (2026-11-10 WB04-E216)
class SyncScene {
    constructor(row) {
        this.pool = new SidPool(24000, { server: true });
        this.types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
        this.finder = new ContainedFinderState();
        this.deck = new Uint8Array(PRODUCTION_DECK_BYTES);
        this.owner = 1;
        this.getters = 0;
        this.query = 0;
        this.trace = '';
        const forms = pairs(row[0]);
        const products = pairs(row[1]);
        const profile = Number(row[2]);
        this.temple = Number(row[3]) !== 0;
        this.full = Number(row[4]);
        this.change = Number(row[5]);
        this.queries = row[6].split(',').map(Number);
        const parent = this.pool.physical(50);
        put(parent, 6, forms.length === 0 ? 0 : 100, 2);
        // 중복도 각각의 실제 raw Form 과 next 를 공급한다.
        for (let index = 0; index < forms.length; index += 1) {
            const raw = this.pool.physical(100 + index);
            raw[10] = 6;
            put(raw, 4, index + 1 < forms.length ? 101 + index : 0, 2);
            put(raw, 18, forms[index][0], 2);
            put(raw, 20, forms[index][1], 2);
        }
        // 명시 TYPE 프로필의 같은 bit4 만 바꾸며 다른 분기 결과는 공급하지 않는다.
        for (let kind = 0; kind < this.types.length; kind += 1) this.types[kind].flags2 = profile === 1 || (!profile && kind === 133) ? 4 : 0;
        put(this.deck, 0x14c, products.length);
        // 물리 순서와 signed power 를 보존한다.
        for (let index = 0; index < products.length; index += 1) {
            this.deck[index * 5] = products[index][0] & 0xff;
            this.deck[index * 5 + 2] = products[index][1] & 0xff;
        }
        this.hooks = {
            deck: () => { this.event('K'); return this.deck; },
            technology: () => {
                const value = 50 + (this.getters ? this.change : 0);
                this.getters += 1;
                this.event(`T,${value}`);
                return value;
            },
            temple: (current) => { assert.equal(current, this.owner); return this.temple; },
            cancelCursor: () => this.event('X'),
            destroy: (sid, flags) => this.event(`D,${sid},${flags}`),
            findKind: (parentSid, mask, kind, index) => {
                const value = this.queries[Math.min(this.query, this.queries.length - 1)];
                this.query += 1;
                this.event(`Q,${parentSid},${mask},${kind},${index},${value}`);
                return value;
            },
            create: (parentSid, kind, amount, flags, attach, quality) => {
                this.event(`C,${parentSid},${kind},${amount},${flags},${attach},${quality}`);
                return 20000;
            },
            assertion: (text) => this.event(`A,${textBytes(text)}`),
        };
    }

    // 이벤트 사이의 구분자만 추가한다. (2026-11-10 WB04-E216)
    event(text) { this.trace = this.trace === '' ? text : `${this.trace};${text}`; }

    // 이 장면의 현재 owner/fullDeck 전역으로 만든 동기화기이다. (2026-11-10 WB04-E216)
    sync(hooks = this.hooks) {
        return new RawProductionSync(this.pool, this.types, this.finder, { localOwner: () => this.owner, fullDeck: () => this.full }, hooks);
    }
}

test('생산 동기화: 원본 10.78 98입력의 호출 순서·인자와 풀 불변이 같다(BYTE seen 255/256/257 중복 포함)', { skip: !rows && '원본 fixture 없음' }, () => {
    assert.equal(rows.length, 98);
    let count = 0;
    for (const row of rows) {
        assert.equal(row.length, 8);
        const scene = new SyncScene(row);
        const before = scene.pool.bytes().slice();
        scene.sync().synchronize();
        assert.equal(scene.trace, row[7], `생산 동기화 원문 ${count}`);
        assert.deepEqual(scene.pool.bytes(), before);
        count += 1;
    }
    assert.equal(count, 98);
});

test('생산 동기화 계약: 장기 FullDeck 전역은 Deck getter 효과 뒤 현재 값으로 읽고 비영을 보존한다', () => {
    const scene = new SyncScene(['71:8', '-', '2', '0', '0', '0', '0', '']);
    const globals = { fullDeck: 0 };
    let destroyed = 0;
    let getterValue = 0x80000000;
    const deck = scene.hooks.deck;
    const hooks = {
        ...scene.hooks,
        deck: () => { globals.fullDeck = getterValue; return deck(); },
        destroy: (sid, flags) => { assert.deepEqual([sid, flags], [100, 0]); destroyed += 1; },
    };
    const sync = new RawProductionSync(scene.pool, scene.types, scene.finder, { localOwner: () => scene.owner, fullDeck: () => globals.fullDeck }, hooks);
    // 부모는 같은 raw 내용물을 유지하므로 삭제 관문의 변화만 관측한다.
    for (const value of [0x80000000, 0xffffffff, 0]) {
        getterValue = value;
        sync.synchronize();
        assert.equal(globals.fullDeck, value);
        assert.equal(scene.full, 0);
        assert.equal(destroyed, value ? 0 : 1);
    }
});

test('생산 동기화 계약: 기술 부모 SID 0 은 실제 예약 슬롯을 조회하고 빈 Deck 에서는 조회/생성하지 않는다', () => {
    const scene = new SyncScene(['-', '-', '0', '0', '0', '0', '0', '']);
    scene.hooks.technology = () => { scene.getters += 1; scene.event('T,0'); return 0; };
    scene.hooks.findKind = () => { throw new Error('빈 Deck 의 예상 밖 조회'); };
    scene.hooks.create = () => { throw new Error('빈 Deck 의 예상 밖 생성'); };
    const before = scene.pool.bytes().slice();
    scene.sync().synchronize();
    assert.equal(scene.getters, 2);
    assert.equal(scene.trace, 'K;T,0;T,0');
    assert.deepEqual(scene.pool.bytes(), before);
});

test('생산 동기화 계약: 선택된 실제 하위 누락과 범위 밖 kind/부모/Deck 개수는 그 위치에서 거부하고 음수 개수는 빈 순회이다', () => {
    // 하위가 없으면 첫 소비 위치에서 오류이다.
    const scene = new SyncScene(['-', '-', '0', '0', '0', '0', '0', '']);
    assert.ok(throws(() => scene.sync({}).synchronize()));
    assert.ok(throws(() => scene.sync({ deck: scene.hooks.deck }).synchronize()));
    // 판본/타입 표 크기 오류.
    assert.ok(throws(() => new RawProductionSync(scene.pool, scene.types.slice(0, 171), scene.finder, { localOwner: () => 1, fullDeck: () => 0 }, scene.hooks)));
    // 현재 기술 부모가 풀 밖이거나 WORD 를 넘으면 거부한다.
    const farParent = new SyncScene(['-', '-', '0', '0', '0', '0', '0', '']);
    farParent.hooks.technology = () => 30000;
    assert.ok(throws(() => farParent.sync().synchronize()));
    farParent.hooks.technology = () => 0x10000;
    assert.ok(throws(() => farParent.sync().synchronize()));
    // 내용물 kind 가 TYPE 표/BYTE seen 밖이면 거부한다.
    const badKind = new SyncScene(['255:8', '-', '0', '0', '0', '0', '0', '']);
    assert.ok(throws(() => badKind.sync().synchronize()));
    // 물리 66개를 넘는 Deck 개수는 거부하고 signed 음수 개수는 빈 순회이다.
    const bigDeck = new SyncScene(['-', '-', '0', '0', '0', '0', '0', '']);
    put(bigDeck.deck, 0x14c, 67);
    assert.ok(throws(() => bigDeck.sync().synchronize()));
    const negative = new SyncScene(['-', '-', '0', '0', '0', '0', '0', '']);
    put(negative.deck, 0x14c, 0xffffffff);
    negative.hooks.findKind = () => { throw new Error('음수 개수는 빈 순회'); };
    negative.sync().synchronize();
    assert.equal(negative.trace, 'K;T,50;T,50');
    // Deck 의 type BYTE 가 seen 표 밖(255)이면 거부한다.
    const wideDeck = new SyncScene(['-', '255:1', '0', '0', '0', '0', '0', '']);
    assert.ok(throws(() => wideDeck.sync().synchronize()));
});
