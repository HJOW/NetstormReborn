// Burning 표시 생성자/입자 추가 raw-burning-visual.js(RawBurningVisual)를 실제 PE 관측 fixture 와 cpppj 계약으로 대조한다.
//  - burningprocess-constructor-x86.tsv 의 10.78(originals) 128행: 열 = 판본, 피해 SID, 모드, 초기 표시 목록 패턴, 피해 50/0 의 깊이 BYTE, (예약), 기대 값 27개(registration form/parent/사각형/slot/zOrder·capacity/total·시각 QWORD 두 개·palette/sound/stage·image·victim/mode/rows/selected/off),
//    기대 표시 슬롯, 메모리/부착 호출 순서, 150입자 24바이트 Adler-32 와 합. 입력(깊이/초기 목록/피해/모드)만 설치하고 warm-palette 생성자를 한 번 호출한다. CD 행은 웹 범위(10.78) 밖이다.
//  - burningparticle-x86.tsv 의 10.78(originals) 282행(WB04-E186): 입자 용량/현재 수/기존 입자 수명 비트·새 입자 수명 비트/source 표식 입력의 `append` 결과(성공 여부·현재 수·전체 입자 24바이트) — 여유 칸 확장, ordered float 수명 ≤ 0 의 첫 만료 칸 재사용, NaN/무한대/음수 0 수명 경계.
//  - cpppj 계약: 첫 색 표가 없는 객체를 이름 없는 빈 색 표로 완료 처리하지 않고 거부(앞선 입자 초기화는 보존), 입자 추가의 여유 칸/첫 만료 칸/가득 참/NaN 수명 비재사용.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E183: Burning 표시 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits, adler32 } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawRenderRegistry } from '../www/core/raw-construction-lifetime.js';
import { RawBurningVisual, createBurningParticle, createBurningVisualFields } from '../www/core/raw-burning-visual.js';

// 실제 PE 관측 fixture 의 10.78 행이다. (2026-11-09 WB04-E183)
const rows = (readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'burningprocess-constructor-x86.tsv')) ?? []).filter((row) => row[0] === 'originals');
// 원본 포인터 대신 같은 실제 표시 목록에 넣는 명시 호스트 토큰과 실제 풀 크기이다. (2026-11-09 WB04-E183)
const TOKEN = 777;
const CAPACITY = 24000;

// 예외가 나는지 본다. (2026-11-09 WB04-E183)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 등록/메모리의 명시 입력 경계를 원본과 같은 인자·순서로 기록하고 현재 의미 DWORD 들을 원본 관찰 열 순서로 직렬화한다. (2026-11-09 WB04-E183)
function runBurning(row) {
    const pool = new SidPool(CAPACITY, { server: true });
    pool.physical(0)[35] = Number(row[5]) & 0xff;
    pool.physical(50)[35] = Number(row[4]) & 0xff;
    const initial = [[], [0], [111, 0, 222], [111, 222]][Number(row[3])];
    const events = [];
    const registry = new RawRenderRegistry(pool, {
        reserve: (amount) => { events.push(`N:${amount}`); return true; },
        releaseArray: () => { events.push('L:registry'); },
        attach: (fields, type, parent, flags) => { events.push(`B:${type}:${parent}:0:${flags}`); fields.form = 83; fields.parent = parent; }
    }, initial);
    const state = { processType: 14, palette: 1 };
    const visual = new RawBurningVisual(pool, registry, state, { reserve: (amount) => { events.push(`N:${amount}`); return true; } });
    const fields = createBurningVisualFields();
    visual.initialize(fields, TOKEN, Number(row[1]) & 0xffff, Number(row[2]));
    const r = fields.registration;
    const clock = new DataView(new ArrayBuffer(16));
    clock.setFloat64(0, fields.lastTime, true);
    clock.setFloat64(8, fields.elapsed, true);
    const values = [
        r.form >>> 0, r.parent >>> 0, floatToBits(r.left), floatToBits(r.top), floatToBits(r.right), floatToBits(r.bottom), r.slot >>> 0, r.zOrder >>> 0, fields.capacity, fields.total,
        clock.getUint32(0, true), clock.getUint32(4, true), clock.getUint32(8, true), clock.getUint32(12, true), fields.palette, fields.sound, fields.stage, fields.image.pixels.length ? 1 : 0,
        fields.image.width >>> 0, fields.image.height >>> 0, fields.image.hotspotX >>> 0, fields.image.hotspotY >>> 0, fields.victim, fields.mode, fields.rows.length ? 1 : 0, fields.selected >>> 0, fields.off ? 1 : 0
    ];
    const bytes = new Uint8Array(fields.particles.length * 24);
    // host padding 은 읽지 않고 현재 150개 입자의 원본 6필드만 24바이트로 만든다.
    fields.particles.forEach((particle, index) => {
        const at = index * 24;
        put(bytes, at, floatToBits(particle.x));
        put(bytes, at + 4, floatToBits(particle.y));
        put(bytes, at + 8, floatToBits(particle.velocityX));
        put(bytes, at + 12, floatToBits(particle.velocityY));
        put(bytes, at + 16, floatToBits(particle.life));
        put(bytes, at + 20, particle.source ? 1 : 0);
    });
    let sum = 0;
    for (const byte of bytes) sum += byte;
    return [values.join(','), registry.slots.join(','), events.join(';'), String(adler32(bytes)), String(sum)];
}

test('Burning 표시: 10.78 warm-palette 생성자 128행의 등록 필드·표시 슬롯·호출 순서·150입자 Adler 를 실제 PE 와 대조한다', () => {
    assert.equal(rows.length, 128);
    rows.forEach((row, index) => {
        assert.equal(row.length, 12);
        const actual = runBurning(row);
        const expected = row.slice(7);
        actual.forEach((value, column) => assert.equal(value, expected[column], `Burning 생성자 행 ${index} 열 ${column}`));
    });
});

test('Burning 표시: 첫 색 표가 없는 객체를 이름 없는 빈 색 표로 완료 처리하지 않고 거부한다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const registry = new RawRenderRegistry(pool, { reserve: () => true, releaseArray: () => {}, attach: () => { throw new Error('부모0입력의미허용부착'); } });
    const state = { processType: 14, palette: 0 };
    const visual = new RawBurningVisual(pool, registry, state, { reserve: () => true });
    const fields = createBurningVisualFields();
    assert.ok(throws(() => visual.initialize(fields, TOKEN, 0, 0)));
    assert.ok(fields.capacity === 150 && fields.particles.length === 150 && state.palette === 0);
    // 색 표 생성이 연결되면 같은 공유 표에 한 번만 만든다.
    const events = [];
    const second = new RawBurningVisual(pool, registry, state, {
        reserve: (bytes) => { events.push(`N:${bytes}`); return true; },
        createPalette: () => { events.push('create'); return 9; },
        gradient: (first, last, colors, noise) => { events.push(`gradient:${first[0]}:${last[0]}:${colors}:${noise}`); },
        buildPalette: (token) => { events.push(`build:${token}`); }
    });
    second.initialize(createBurningVisualFields(), TOKEN + 1, 0, 1);
    assert.ok(state.palette === 9 && events.join(';') === 'N:3600;N:8;create;gradient:1:1:8:2.4;build:9');
});

test('Burning 표시: 입자 추가는 여유 칸 → 가득 찬 뒤 첫 만료(life ≤ 0) 칸을 쓰고 NaN 수명은 재사용하지 않는다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const registry = new RawRenderRegistry(pool, { reserve: () => true, releaseArray: () => {}, attach: () => {} });
    const visual = new RawBurningVisual(pool, registry, { processType: 14, palette: 1 }, { reserve: () => true });
    const fields = createBurningVisualFields();
    fields.capacity = 3;
    fields.particles = Array.from({ length: 3 }, createBurningParticle);
    const make = (x, life) => ({ x, y: 0, velocityX: 0, velocityY: 0, life, source: false });
    assert.ok(visual.append(fields, make(1, 5)) && visual.append(fields, make(2, 5)) && visual.append(fields, make(3, 5)));
    assert.ok(fields.total === 3 && fields.particles[0].x === 1 && fields.particles[2].x === 3);
    // 가득 찼고 모두 살아 있으면 실패한다.
    assert.ok(!visual.append(fields, make(4, 5)) && fields.particles[0].x === 1);
    // 첫 만료 칸(life ≤ 0)을 재사용하며 NaN 은 ordered 비교가 거짓이라 재사용하지 않는다.
    fields.particles[1].life = Number.NaN;
    fields.particles[2].life = -1;
    assert.ok(visual.append(fields, make(5, 5)) && fields.particles[2].x === 5 && fields.particles[1].x === 2);
    // 저장소/개수 불일치는 거부한다.
    fields.total = 4;
    assert.ok(throws(() => visual.append(fields, make(6, 5))));
    assert.ok(throws(() => new RawBurningVisual(pool, { pool: {} }, { processType: 14, palette: 1 }, { reserve: () => true })));
});

// 실제 PE 의 외부 경계 없는 24바이트 입자 추가와 현재 count/전체 입자를 대조할 문자열이다. (2026-11-09 WB04-E186)
function particleText(fields) {
    const bytes = new Uint8Array(fields.capacity * 24);
    fields.particles.forEach((particle, index) => {
        const at = index * 24;
        put(bytes, at, floatToBits(particle.x));
        put(bytes, at + 4, floatToBits(particle.y));
        put(bytes, at + 8, floatToBits(particle.velocityX));
        put(bytes, at + 12, floatToBits(particle.velocityY));
        put(bytes, at + 16, floatToBits(particle.life));
        put(bytes, at + 20, particle.source ? 1 : 0);
    });
    return bytes.length ? Buffer.from(bytes).toString('hex') : '-';
}

test('Burning 입자 추가: 10.78 여유 칸/ordered float life ≤ 0 첫 만료 칸/NaN·무한대·음수 0 수명 경계를 실제 PE 282행과 대조한다', () => {
    const particleRows = (readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'burningparticle-x86.tsv')) ?? []).filter((row) => row[0] === 'originals');
    assert.equal(particleRows.length, 282);
    const pool = new SidPool(CAPACITY, { server: true });
    const refuse = (name) => () => { throw new Error(name); };
    const registry = new RawRenderRegistry(pool, { reserve: refuse('입자추가의미허용표시확보'), releaseArray: refuse('입자추가의미허용표시반납'), attach: refuse('입자추가의미허용부착') });
    const visual = new RawBurningVisual(pool, registry, { processType: 14, palette: 0 }, { reserve: refuse('입자추가의미허용추가확보') });
    // 입력 숫자/old 입자만 공급하며 첫 만료 칸이나 결과 count 를 미리 계산하지 않는다.
    particleRows.forEach((row, index) => {
        assert.equal(row.length, 9);
        const fields = createBurningVisualFields();
        fields.capacity = Number(row[1]);
        fields.total = Number(row[2]);
        const lifes = row[3] === '-' ? [] : row[3].split(',');
        assert.equal(lifes.length, fields.capacity);
        // 각 입자의 좌표/속도/수명과 두 허용 source 표식은 native 와 같은 입력이다.
        fields.particles = Array.from({ length: fields.capacity }, (_, at) => ({ x: Math.fround(10.5 + at), y: Math.fround(20.0 + at), velocityX: Math.fround(at - 1.0), velocityY: Math.fround(at + 1.0), life: bitsToFloat(Number(lifes[at])), source: (at & 1) !== 0 }));
        const made = visual.append(fields, { x: 100.5, y: 50.75, velocityX: -0.25, velocityY: 1.5, life: bitsToFloat(Number(row[4])), source: Number(row[5]) !== 0 });
        assert.equal(made ? '1' : '0', row[6], `입자 추가 반환 행 ${index}`);
        assert.equal(String(fields.total), row[7], `입자 추가 현재 수 행 ${index}`);
        assert.equal(particleText(fields), row[8], `입자 추가 전체 입자 행 ${index}`);
    });
});
