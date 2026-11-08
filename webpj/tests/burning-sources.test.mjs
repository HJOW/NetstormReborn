// Burning 연소점 선택 raw-burning-sources.js(RawBurningSources)를 실제 PE 관측 fixture 와 cpppj 계약으로 대조한다.
//  - burningsources-x86.tsv 의 10.78(originals) 326행: 열 = 판본, 너비, 높이, HP DWORD, TYPE 최대 HP, 종류(0 일반 89/1 flag 0x400 TYPE 70/2 사제 158), 현재 선택 수, 불투명 입력 여부, 난수 상태, 입자 용량, 기존 입자 패턴(0 없음/1 고정 source 3개/2 가득 참),
//    위크 마나 옵션, 피해 SID, 기대 열 6개(반환·선택 수·total·행 표식 hex·150입자 24바이트 hex·난수 상태). 현재 raw HP/타입·캡처·기존 입자만 입력으로 만들고 목표 행을 계산하지 않는다. 독립 값과 반대 fallback 으로 연결한 공유 현재 옵션도 같은 행 전체로 재생한다.
//  - burning-sources-corner-x86.tsv 의 10.78 18행: 작은 투명 캡처에서 10.78 이 투명 픽셀에서 논리 행 끝 후보를 읽지 않는 순서(실제 난수/입자/행 결과)와, 불투명 픽셀 뒤 실제 선택된 잘못된 행은 계속 보호하는 계약.
//  - 연소점 개수/현재 TYPE/HP 부모 쪽 5,652행 관측은 `burning-source-count.test.mjs` 가 검사한다. CD 행은 웹 범위(10.78) 밖이다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E183: Burning 연소점 선택 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { RawRenderRegistry } from '../www/core/raw-construction-lifetime.js';
import { RawBurningVisual, createBurningParticle, createBurningVisualFields } from '../www/core/raw-burning-visual.js';
import { RawBurningSources } from '../www/core/raw-burning-sources.js';

// 실제 PE 관측 fixture 의 10.78 행이다. (2026-11-09 WB04-E183)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const sourceRows = (readRows(path.join(FIXTURES, 'burningsources-x86.tsv')) ?? []).filter((row) => row[0] === 'originals');
const cornerRows = (readRows(path.join(FIXTURES, 'burning-sources-corner-x86.tsv')) ?? []).filter((row) => row[0] === 'originals');
const CAPACITY = 24000;

// 바이트 배열을 소문자 16진 문자열로 바꾼다. (2026-11-09 WB04-E183)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 입자 한 개를 원본 24바이트(다섯 float + source 포인터 0/1)로 직렬화한다(호스트 padding 은 읽지 않음). (2026-11-09 WB04-E183)
function particleBytes(fields) {
    const bytes = new Uint8Array(fields.capacity * 24);
    fields.particles.forEach((p, index) => {
        const at = index * 24;
        put(bytes, at, floatToBits(p.x));
        put(bytes, at + 4, floatToBits(p.y));
        put(bytes, at + 8, floatToBits(p.velocityX));
        put(bytes, at + 12, floatToBits(p.velocityY));
        put(bytes, at + 16, floatToBits(p.life));
        put(bytes, at + 20, p.source ? 1 : 0);
    });
    return bytes;
}

// 실제 source/Append 만 소비하며 생성/부착/첫 팔레트 호출은 명시 거부하는 장면이다. (2026-11-09 WB04-E183)
class SourceScene {
    // 같은 풀/난수/타입 표/표시 등록/Append 몸체/선택기를 만든다. (2026-11-09 WB04-E183)
    constructor() {
        this.pool = new SidPool(CAPACITY, { server: true });
        this.types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, maxHitPoints: 0 }));
        this.random = new GameRandom();
        this.state = { weakenedMana: false, manaType: 158, daisType: 165, victim: { checking: () => 0 } };
        // 현재 피해 50 까지 실제 client SID 로 입력 번호를 확보한다.
        for (let id = 5; id <= 50; id += 1) assert.equal(this.pool.allocate(2), id);
        const refuse = (name) => () => { throw new Error(name); };
        this.registry = new RawRenderRegistry(this.pool, { reserve: refuse('source선택의미허용표시확보'), releaseArray: refuse('source선택의미허용표시반납'), attach: refuse('source선택의미허용부착') });
        this.visual = new RawBurningVisual(this.pool, this.registry, { processType: 14, palette: 0 }, { reserve: refuse('source선택의미허용추가확보') });
        this.sources = new RawBurningSources(this.pool, this.types, this.visual, this.random, this.state);
    }
}

// 한 판본의 전체 독립 입력을 같은 선택기에 재생한다(shared 이면 장기 설정을 모사하는 getter 와 반대 fallback 을 연결). (2026-11-09 WB04-E183)
function replaySources(shared) {
    const scene = new SourceScene();
    let currentWeakened = false;
    if (shared) scene.state.sharedWeakenedMana = () => currentWeakened;
    let count = 0;
    // 현재 raw HP/타입·캡처·기존 입자만 입력에서 만들며 목표 행을 계산하지 않는다.
    for (const row of sourceRows) {
        assert.equal(row.length, 19);
        const kind = Number(row[5]);
        const type = kind === 2 ? 158 : kind === 1 ? 70 : 89;
        scene.types[type].flags1 = 16;
        scene.types[type].flags2 = kind === 1 ? 0x400 : 0;
        scene.types[type].maxHitPoints = Number(row[4]) | 0;
        const raw = scene.pool.allocated(50);
        raw.fill(0);
        raw[10] = type;
        put(raw, 26, Number(row[3]), 4);
        scene.random.setState(Number(row[8]));
        scene.state.weakenedMana = Number(row[11]) !== 0;
        currentWeakened = scene.state.weakenedMana;
        // 별도 값형 설정으로 성공하는 오연결을 막는다.
        if (shared) scene.state.weakenedMana = !currentWeakened;
        const fields = createBurningVisualFields();
        fields.victim = Number(row[12]) & 0xffff;
        fields.capacity = Number(row[9]);
        fields.particles = Array.from({ length: fields.capacity }, createBurningParticle);
        fields.image.width = Number(row[1]) | 0;
        fields.image.height = Number(row[2]) | 0;
        fields.image.pixels = new Uint8Array(fields.image.width * fields.image.height).fill(Number(row[7]) ? 255 : 0);
        fields.rows = new Uint8Array(Math.trunc((fields.image.width + fields.image.height) / 5));
        fields.selected = Number(row[6]) | 0;
        const pattern = Number(row[10]);
        fields.total = pattern === 2 ? fields.capacity : pattern === 1 ? Math.min(3, fields.capacity) : 0;
        const points = fields.image.width >= 8 && fields.image.height >= 8 ? [[2, 3], [7, 3], [0, 0]] : [[0, 0], [0, 0], [0, 0]];
        // 기존 source 좌표와 rowTaken 표식은 같은 명시 입력이다.
        fields.particles.forEach((particle, index) => {
            const source = pattern === 1 && index < 3;
            const x = source ? points[index][0] : 10 + index;
            const y = source ? points[index][1] : 20 + index;
            fields.particles[index] = { x, y, velocityX: 0, velocityY: 0, life: source ? 9999 : pattern === 2 ? 1.25 : 0, source };
            if (source) {
                const group = Math.trunc((Math.trunc(x) + Math.trunc(y)) / 5);
                if (group < fields.rows.length) fields.rows[group] = 1;
            }
        });
        scene.sources.select(fields);
        const output = ['return', String(fields.selected), String(fields.total), fields.rows.length ? hex(fields.rows) : '-', fields.capacity ? hex(particleBytes(fields)) : '-', String(scene.random.state)];
        const expected = row.slice(13);
        output.forEach((value, column) => assert.equal(value, expected[column], `BurningSources 행 ${count} 열 ${column}`));
        count += 1;
    }
    assert.equal(count, sourceRows.length);
}

test('Burning 연소점: 10.78 현재 HP/고정 source/300시도/폴백/제거와 정수 난수를 실제 PE 326행 전체로 대조한다', () => {
    assert.equal(sourceRows.length, 326);
    replaySources(false);
});

test('Burning 연소점: 같은 326행을 반대 fallback 과 공유 현재 위크 마나 getter 로 다시 대조한다', () => {
    replaySources(true);
});

// corner 한 행을 새 필드에 공급해 선택기를 한 번 호출한다(모든 픽셀이 투명). (2026-11-09 WB04-E183)
function cornerFields(scene, row) {
    scene.types[89].flags1 = 16;
    scene.types[89].flags2 = 0;
    scene.types[89].maxHitPoints = Number(row[4]) | 0;
    const raw = scene.pool.allocated(50);
    raw.fill(0);
    raw[10] = 89;
    put(raw, 26, Number(row[3]), 4);
    scene.random.setState(Number(row[8]));
    const fields = createBurningVisualFields();
    fields.victim = Number(row[12]) & 0xffff;
    fields.capacity = Number(row[9]);
    fields.particles = Array.from({ length: fields.capacity }, createBurningParticle);
    fields.total = 0;
    fields.selected = Number(row[6]) | 0;
    fields.image.width = Number(row[1]) | 0;
    fields.image.height = Number(row[2]) | 0;
    fields.image.pixels = new Uint8Array(fields.image.width * fields.image.height).fill(255);
    // 추가 읽기 가능 0 BYTE 도 fixture 가 명시한 현재 입력이며 caller 할당을 바꾸지 않는다.
    fields.rows = new Uint8Array(Number(row[13]));
    fields.particles.forEach((particle, index) => { fields.particles[index] = { x: 10 + index, y: 20 + index, velocityX: 0, velocityY: 0, life: 0, source: false }; });
    return fields;
}

test('Burning 연소점: 10.78 은 투명 픽셀에서 논리 행 끝 후보를 읽지 않는다(corner 18행)', () => {
    assert.equal(cornerRows.length, 18);
    const scene = new SourceScene();
    cornerRows.forEach((row, index) => {
        assert.ok(row.length === 20 && Number(row[5]) === 0 && Number(row[10]) === 0 && Number(row[11]) === 0);
        const fields = cornerFields(scene, row);
        scene.sources.select(fields);
        const actual = ['return', String(fields.selected), String(fields.total), hex(fields.rows), hex(particleBytes(fields)), String(scene.random.state)];
        const expected = row.slice(14);
        actual.forEach((value, column) => assert.equal(value, expected[column], `corner 행 ${index} 열 ${column}`));
    });
});

test('Burning 연소점: 불투명 픽셀 뒤 실제 선택된 잘못된 행은 계속 보호한다(guard 삭제/배열 임의 확장 금지)', () => {
    const scene = new SourceScene();
    const row = cornerRows[0];
    const fields = cornerFields(scene, row);
    fields.image.pixels.fill(0);
    let rejected = false;
    try { scene.sources.select(fields); } catch (error) { rejected = error instanceof RangeError && error.message === 'Burning sources 새 행 인덱스 오류'; }
    assert.ok(rejected);
});

test('Burning 연소점 생성자: PATCH 풀/같은 풀의 Append 몸체/난수/상태가 없으면 거부한다', () => {
    const scene = new SourceScene();
    const make = (pool, types, visual, random, state) => () => new RawBurningSources(pool, types, visual, random, state);
    const throwsOf = (action) => { try { action(); } catch { return true; } return false; };
    assert.ok(throwsOf(make({}, scene.types, scene.visual, scene.random, scene.state)));
    assert.ok(throwsOf(make(scene.pool, scene.types, { pool: new SidPool(CAPACITY, { server: true }) }, scene.random, scene.state)));
    assert.ok(throwsOf(make(scene.pool, undefined, scene.visual, scene.random, scene.state)));
    assert.ok(throwsOf(make(scene.pool, scene.types, scene.visual, {}, scene.state)));
    assert.ok(throwsOf(make(scene.pool, scene.types, scene.visual, scene.random, {})));
    assert.equal(scene.sources.pool, scene.pool);
});
