// 냉각 연기 raw-smoke-effect.js(RawSmokeLifetime·RawSmokeFrame)를 실제 PE 관측 fixture 로 대조한다(cpppj `tests/SmokeEffectTests.cpp` 의 SmokeScene/ReplaySmoke 를 그대로 옮김).
//  - smoke-effect-x86.tsv(10.78) 893행: 종류별 프레임/출력 841·방출 실패/재사용 28·냉각 생성/소멸 8·현재 SHP 냉각 wrapper 8·기본 생성자 8행.
//    입력 11열(종류, 128바이트 객체, 용량 전체 입자, 부모 50 raw, 전역 28바이트, 색수+인접 64 DWORD, 난수 상태, 기존 공유 색 여부, 저장소 없음 표시, 포구 x/y) 뒤 원문 출력 8열
//    (112바이트 의미 필드, 전체 입자, 색 표 메모리, 난수 상태, dirty 요청, 선 요청, 생성 행의 반납 뒤 112바이트, 외부 요청 순서)을 허용오차 없이 비교한다.
//  - 생성 행은 새 표시 목록에서 40칸 확장(N:160)·부착(B)·입자 4800·첫 공유 색 8/48·반납(F:P/F:O)의 순서를 대조하고, wrapper 행은 현재 SHP 조회 인자(H)를 앞에 둔다.
//  - cpppj 계약(추가): 경계 미연결/다른 판본 거부, 저장소 없는 효과의 무동작, 색 표 인접 DWORD 미공급 거부. 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E191: 냉각 연기 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import fs from 'node:fs';
import { REPO_ROOT } from './helpers.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { RawRenderRegistry } from '../www/core/raw-construction-lifetime.js';
import { RawSmokeFrame, RawSmokeLifetime, SmokeRelease, buildSmokePalette, createSmokeFields, createSmokeRequest } from '../www/core/raw-smoke-effect.js';

// 실제 PE 관측 fixture 이다(헤더 행은 `#` 로 시작해 걸러진다). (2026-11-09 WB04-E191)
const rows = fs.readFileSync(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'smoke-effect-x86.tsv'), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && line[0] !== '#').map((line) => line.split('\t'));
// 도구의 입력 11열 뒤 원문 출력 8열이 이어진다(cpppj SmokeColumn). (2026-11-09 WB04-E191)
const COLUMN = Object.freeze({ kind: 0, fields: 1, particles: 2, parent: 3, state: 4, palette: 5, seed: 6, warm: 7, nullStorage: 8, hotX: 9, hotY: 10, output: 11 });
// cpppj RawSceneSupport 의 장면 풀 용량이다. (2026-11-09 WB04-E191)
const CAPACITY = 24000;

// 빈 배열 표시까지 보존하는 원본 hex 판독과 소문자 hex 출력이다. (2026-11-09 WB04-E191)
const decode = (text) => (text === '-' ? new Uint8Array(0) : Uint8Array.from(text.match(/../g) ?? [], (pair) => parseInt(pair, 16)));
const encode = (bytes) => Array.from(bytes, (value) => value.toString(16).padStart(2, '0')).join('');

// 리틀엔디언 DWORD 와 float/double 입력 비트를 계산 없이 읽는다. (2026-11-09 WB04-E191)
const get = (bytes, offset) => (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
function float(bytes, offset) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, get(bytes, offset), true);
    return view.getFloat32(0, true);
}
function double(bytes, offset) {
    const view = new DataView(new ArrayBuffer(8));
    view.setUint32(0, get(bytes, offset), true);
    view.setUint32(4, get(bytes, offset + 4), true);
    return view.getFloat64(0, true);
}
const bits = (value) => {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
};
function append(bytes, value) {
    for (let index = 0; index < 4; index += 1) bytes.push((value >>> (index * 8)) & 0xff);
}
function appendDouble(bytes, value) {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    append(bytes, view.getUint32(0, true));
    append(bytes, view.getUint32(4, true));
}

// 원문의 실제 112바이트 순서로 의미 필드만 직렬화한다(cpppj SmokeFieldsHex). (2026-11-09 WB04-E191)
function fieldsHex(fields) {
    const r = fields.registration;
    const bytes = [];
    for (const value of [r.form, r.parent]) append(bytes, value >>> 0);
    for (const value of [r.left, r.top, r.right, r.bottom]) append(bytes, bits(value));
    for (const value of [r.slot, r.zOrder, fields.capacity, fields.total]) append(bytes, value >>> 0);
    appendDouble(bytes, fields.lastTime);
    appendDouble(bytes, fields.elapsed);
    append(bytes, bits(fields.delta));
    append(bytes, bits(fields.scaledDelta));
    append(bytes, fields.first ? 1 : 0);
    for (const value of [fields.due, fields.windX, fields.windY]) append(bytes, bits(value));
    for (const value of [fields.hotspotX, fields.hotspotY, fields.remaining]) append(bytes, value >>> 0);
    for (const value of [fields.maximumLife, fields.velocity, fields.spread, fields.period, fields.noise]) append(bytes, bits(value));
    assert.equal(bytes.length, 112);
    return encode(bytes);
}

// 실제 원문 입출력 형식의 전체 입자 배열이다(비어 있으면 '-'). (2026-11-09 WB04-E191)
function particleHex(particles) {
    const bytes = [];
    // 실제 남은 배열 전체를 비교한다.
    for (const particle of particles) {
        for (const value of [particle.x, particle.y, particle.velocityX, particle.velocityY, particle.life]) append(bytes, bits(value));
        append(bytes, particle.spare >>> 0);
    }
    return bytes.length ? encode(bytes) : '-';
}

// 입력 원문을 현재 의미 필드/배열·부모·전역·색 표·난수에 설치하고 실제 제품의 수명 또는 프레임/출력을 실행해 출력 8열을 돌려준다(cpppj SmokeScene). (2026-11-09 WB04-E191)
function run(pool, row) {
    assert.equal(row.length, COLUMN.output + 8);
    const raw = decode(row[COLUMN.fields]);
    assert.equal(raw.length, 128);
    const fields = createSmokeFields();
    const r = fields.registration;
    r.token = 1;
    r.form = get(raw, 4) | 0;
    r.parent = get(raw, 8) | 0;
    r.left = float(raw, 12);
    r.top = float(raw, 16);
    r.right = float(raw, 20);
    r.bottom = float(raw, 24);
    r.slot = get(raw, 28) | 0;
    r.zOrder = get(raw, 32) | 0;
    fields.capacity = get(raw, 36) | 0;
    fields.total = get(raw, 40) | 0;
    fields.hasStorage = get(raw, 44) !== 0;
    fields.lastTime = double(raw, 48);
    fields.elapsed = double(raw, 56);
    fields.delta = float(raw, 64);
    fields.scaledDelta = float(raw, 68);
    fields.first = get(raw, 72) !== 0;
    fields.due = float(raw, 80);
    fields.windX = float(raw, 84);
    fields.windY = float(raw, 88);
    fields.hotspotX = get(raw, 92) | 0;
    fields.hotspotY = get(raw, 96) | 0;
    fields.remaining = get(raw, 100) | 0;
    fields.maximumLife = float(raw, 104);
    fields.velocity = float(raw, 108);
    fields.spread = float(raw, 112);
    fields.period = float(raw, 116);
    fields.noise = float(raw, 120);
    const particles = decode(row[COLUMN.particles]);
    assert.equal(particles.length, fields.capacity * 24);
    // 사용 꼬리 밖의 속도/spare 도 같은 비트로 보존한다.
    fields.particles = Array.from({ length: fields.capacity }, (_, index) => {
        const at = index * 24;
        return { x: float(particles, at), y: float(particles, at + 4), velocityX: float(particles, at + 8), velocityY: float(particles, at + 12), life: float(particles, at + 16), spare: get(particles, at + 20) };
    });
    pool.physical(50).set(decode(row[COLUMN.parent]));
    const globals = decode(row[COLUMN.state]);
    const state = { now: double(globals, 0), pixelScaleX: float(globals, 8), pixelScaleY: float(globals, 12), rate: float(globals, 16), cameraX: float(globals, 20), cameraY: float(globals, 24), diagnostic: 0 };
    const random = new GameRandom(0);
    random.setState(Number(row[COLUMN.seed]) >>> 0);
    const color = decode(row[COLUMN.palette]);
    // 실제 공급한 인접 DWORD 까지 옮기며 새 색으로 해석하지 않는다.
    const colors = Array.from({ length: 64 }, (_, index) => get(color, 4 + index * 4));
    const palette = { count: get(color, 0) | 0, readable: colors };
    const events = [];
    const dirty = [];
    const lines = [];
    const registry = new RawRenderRegistry(pool, {
        reserve: (bytes) => { events.push(`N:${bytes}`); return true; },
        releaseArray: () => events.push('F:R'),
        attach: (target, type, parent, flags) => { events.push(`B:${type}:${parent}:0:${flags}`); target.form = 81; target.parent = parent; }
    });
    const lifetimeState = { palette: Number(row[COLUMN.warm]) ? 1 : 0 };
    const lifetime = new RawSmokeLifetime(pool, registry, lifetimeState, {
        reserve: (bytes) => { events.push(`N:${bytes}`); return true; },
        createPalette: () => 1,
        buildPalette: (token) => {
            assert.equal(token, 1);
            events.push('N:48');
            // 실제 48바이트 색 배열을 같은 메모리 앞에 만든다.
            buildSmokePalette().forEach((value, index) => { colors[index] = value; });
            palette.count = 12;
        }
    });
    const frame = new RawSmokeFrame(pool, random, state, palette, {
        invalidate: (rect) => { for (const value of [rect.left, rect.top, rect.right, rect.bottom, 0]) append(dirty, value >>> 0); },
        line: (rect) => { for (const value of [rect.x1, rect.y1, rect.x2, rect.y2, rect.color]) append(lines, value >>> 0); },
        kill: (flags) => events.push(`K:${flags}`)
    });
    const kind = row[COLUMN.kind];
    const constructed = kind === 'ctor' || kind === 'cold' || kind === 'wrapper';
    if (constructed) {
        if (kind === 'ctor') {
            const request = createSmokeRequest();
            request.parent = 50;
            request.hotspotX = Number.parseInt(row[COLUMN.hotX], 10);
            request.hotspotY = Number.parseInt(row[COLUMN.hotY], 10);
            request.windX = fields.windX;
            request.windY = fields.windY;
            request.maximumLife = fields.maximumLife;
            request.velocity = fields.velocity;
            request.spread = fields.spread;
            request.period = fields.period;
            request.noise = fields.noise;
            lifetime.initialize(fields, 1, request);
        } else {
            if (kind === 'wrapper') {
                const parent = pool.slot(50);
                events.push(`H:${parent[40] & 1}:${get(parent, 36)}`);
            }
            lifetime.initializeCooling(fields, 1, 50, [Number.parseInt(row[COLUMN.hotX], 10), Number.parseInt(row[COLUMN.hotY], 10)]);
        }
    } else {
        if (kind === 'spawn') frame.emit(fields, -7.75, 31.125, 100);
        else frame.step(fields);
        frame.draw(fields);
    }
    // 원문 공유 표의 읽을 수 있는 전체 메모리를 비교한다.
    const colorBytes = [];
    append(colorBytes, palette.count >>> 0);
    for (const value of colors) append(colorBytes, value >>> 0);
    const result = [fieldsHex(fields), particleHex(fields.particles), encode(colorBytes), String(random.state), dirty.length ? encode(dirty) : '-', lines.length ? encode(lines) : '-'];
    if (constructed) {
        lifetime.destroy(fields, 1, (what) => events.push(what === SmokeRelease.Particles ? 'F:P' : 'F:O'));
        result.push(fieldsHex(fields));
        assert.ok(registry.slots.length === 40 && registry.slots[0] === 0);
        assert.equal(lifetimeState.palette, 1);
    } else result.push('-');
    result.push(events.join(';'));
    return result;
}

// 같은 종류의 모든 독립 입력을 대조하고 처리한 행 수를 돌려준다(cpppj ReplaySmoke, 같은 장면 풀을 행 사이에 재사용). (2026-11-09 WB04-E191)
function replay(kind) {
    const pool = new SidPool(CAPACITY, { server: false });
    let count = 0;
    rows.forEach((row, index) => {
        if (row[COLUMN.kind] !== kind) return;
        const actual = run(pool, row);
        count += 1;
        // 전체 8개 출력 열을 허용오차 없이 비교한다.
        actual.forEach((value, column) => assert.equal(value, row[COLUMN.output + column], `연기 원문 차이 kind=${kind} row=${index} column=${column}`));
    });
    return count;
}

test('냉각 연기 프레임/출력: 전체 delta·동적 분열·가득 찬 꼬리·dirty/선·색 841입력을 실제 PE 와 비트 단위로 대조한다', () => {
    assert.equal(replay('frame'), 841);
});

test('냉각 연기 방출: 슬롯 확보 실패의 난수 보존과 죽은 슬롯 재사용 28입력을 실제 PE 와 대조한다', () => {
    assert.equal(replay('spawn'), 28);
});

test('냉각 연기 냉각 생성: 200입자·공유 색·10회/3간격과 실제 소멸 8입력을 실제 PE 와 대조한다', () => {
    assert.equal(replay('cold'), 8);
});

test('냉각 연기 wrapper: 현재 SHP 조회 인자/signed 포구 오프셋 8입력을 실제 PE 와 대조한다', () => {
    assert.equal(replay('wrapper'), 8);
});

test('냉각 연기 기본 생성자: 8회 방출 기본값과 쓰지 않는 delta 입력 보존 8입력을 실제 PE 와 대조한다', () => {
    assert.equal(replay('ctor'), 8);
});

test('냉각 연기 경계: 실제 출력/삭제·확보 경계 누락과 다른 판본을 거부하고 저장소 없는 효과는 아무것도 하지 않는다', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    const random = new GameRandom(5);
    const state = { now: 1, pixelScaleX: 16, pixelScaleY: 11, rate: 1, cameraX: 0, cameraY: 0, diagnostic: 0 };
    const palette = { count: 12, readable: buildSmokePalette() };
    const hooks = { invalidate: () => {}, line: () => {}, kill: () => {} };
    assert.throws(() => new RawSmokeFrame({}, random, state, palette, hooks));
    // 출력 경계 하나라도 없으면 빈 성공으로 바꾸지 않고 거부한다.
    for (const name of Object.keys(hooks)) assert.throws(() => new RawSmokeFrame(pool, random, state, palette, { ...hooks, [name]: undefined }));
    const registry = new RawRenderRegistry(pool, { reserve: () => true, releaseArray: () => {}, attach: () => {} });
    assert.throws(() => new RawSmokeLifetime(pool, registry, { palette: 0 }, {}));
    assert.throws(() => new RawSmokeLifetime(new SidPool(CAPACITY, { server: false }), registry, { palette: 0 }, { reserve: () => true }));
    // 공유 색이 없는데 생성 장치가 없으면 입자 확보 뒤 멈춘다.
    const lifetime = new RawSmokeLifetime(pool, registry, { palette: 0 }, { reserve: () => true });
    assert.throws(() => lifetime.initialize(createSmokeFields(), 1, createSmokeRequest()));
    let calls = 0;
    const frame = new RawSmokeFrame(pool, random, state, palette, { invalidate: () => { calls += 1; }, line: () => { calls += 1; }, kill: () => { calls += 1; } });
    const empty = createSmokeFields();
    frame.step(empty);
    frame.draw(empty);
    frame.emit(empty, 0, 0, 1);
    assert.equal(calls, 0);
    assert.equal(random.state, 5);
    // 색수 다음 DWORD 가 공급되지 않으면 가짜 색 없이 거부한다. 역수가 정확한 최대 수명 4 에서 수명 4 는 12×4×0.25 = 12 번째(인접) DWORD 를 읽는다
    // (기본 최대 수명 100 은 float(1/100) 이 내림이라 11 번째를 읽으므로 이 확인에 쓰지 않는다).
    const fields = createSmokeFields();
    fields.hasStorage = true;
    fields.capacity = 1;
    fields.total = 1;
    fields.maximumLife = 4;
    fields.particles = [{ x: 1, y: 1, velocityX: 0, velocityY: 0, life: 4, spare: 0 }];
    assert.throws(() => frame.draw(fields), RangeError);
    palette.readable = [...buildSmokePalette(), 0x55];
    frame.draw(fields);
    assert.equal(calls, 1);
});
