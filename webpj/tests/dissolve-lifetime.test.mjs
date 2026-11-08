// Dissolve(분해) 생성/소멸 raw-dissolve-lifetime.js(RawDissolveLifetime)를 실제 PE 관측 fixture 로 대조한다(cpppj `tests/DissolveLifetimeTests.cpp` 를 그대로 옮김).
//  - dissolve-lifetime-x86.tsv 288입력(방향 × 표시 목록 용량/구멍 × source 깊이 × callback 중 변경 4종 × flags × 불투명/완전 투명 캡처): 열 = 방향, 목록 용량, 빈 칸 위치, source 깊이, 변경 종류, flags, 불투명 여부,
//    생성 직후 100바이트 의미 상태, 용량 전체 입자, 게임 난수, CRT 상태(항상 1, 이 효과는 CRT 를 쓰지 않음), 등록 목록, 생성 요청 순서(N 확보·C 생성·P Pop·A 부착·D 상호 감시·I 캡처·F 반납),
//    삭제 뒤 100바이트, 삭제 뒤 목록, 반납 요청 순서를 대조한다. 변경 1 은 더미 생성 중 source 좌표/깊이, 변경 2 는 부착이 다른 부모(61), 변경 3 은 캡처 중 source 깊이를 바꾼다.
//    준비 경계는 같은 `RawDissolveFrame.initialize` 를 폴 61/62 로 연결한다. 완전 투명 캡처는 용량 0 이어도 입자 반납 요청이 나온다(new 0 의 비영 포인터).
//    삭제 뒤 원본은 해제한 포인터/용량을 그대로 두지만 JS 는 실제 배열을 반납하므로 그 두 저장소 표현(입자 수·그림 존재)만 비교에서 뺀다(cpppj 시험과 같음).
//  - cpppj 계약: 다른 풀/판본·경계 누락 거부, 확보 실패는 등록 전에 멈춤, 반납 경계 누락 거부. 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E189: Dissolve 수명 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import fs from 'node:fs';
import { REPO_ROOT } from './helpers.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { RawRenderRegistry } from '../www/core/raw-construction-lifetime.js';
import { shrapnelFloatFromBits } from '../www/core/raw-shrapnel-frame.js';
import { ShrapnelRelease } from '../www/core/raw-shrapnel-lifetime.js';
import { RawDissolveFrame, createDissolveFrameFields } from '../www/core/raw-dissolve-frame.js';
import { RawDissolveLifetime } from '../www/core/raw-dissolve-lifetime.js';

// 실제 PE 관측 fixture 이다(헤더 행은 `#` 로 시작해 걸러진다). (2026-11-09 WB04-E189)
const rows = fs.readFileSync(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'dissolve-lifetime-x86.tsv'), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && line[0] !== '#').map((line) => line.split('\t'));

// fixture 의 hex 를 원문 바이트로 복원한다('-' 는 길이 0). (2026-11-09 WB04-E189)
const decode = (text) => (text === '-' ? new Uint8Array(0) : Uint8Array.from(text.match(/../g) ?? [], (pair) => parseInt(pair, 16)));

// float 비트 변환과 리틀엔디언 DWORD/QWORD 기록·raw 쓰기이다. (2026-11-09 WB04-E189)
const floatBits = (value) => {
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
function put(raw, offset, value) {
    for (let index = 0; index < 4; index += 1) raw[offset + index] = (value >>> (index * 8)) & 0xff;
}

// 원문 100바이트 의미 상태이다. 그림 주소만 비영 토큰(존재 1)으로 정규화한다(cpppj 시험 Fields). (2026-11-09 WB04-E189)
function fieldBytes(fields) {
    const value = fields.common;
    const rect = value.registration;
    const bytes = [];
    append(bytes, rect.form >>> 0);
    append(bytes, rect.parent >>> 0);
    // 실제 표시 영역 네 float 를 먼저 저장한다.
    for (const part of [rect.left, rect.top, rect.right, rect.bottom]) append(bytes, floatBits(part));
    append(bytes, rect.slot >>> 0);
    append(bytes, rect.zOrder >>> 0);
    append(bytes, value.particles.length);
    append(bytes, value.total >>> 0);
    appendDouble(bytes, value.lastTime);
    appendDouble(bytes, value.elapsed);
    append(bytes, floatBits(value.delta));
    append(bytes, floatBits(value.scaledDelta));
    append(bytes, value.first ? 1 : 0);
    append(bytes, value.image.pixels.length ? 1 : 0);
    // 캡처 치수/기준점은 signed DWORD 비트 그대로 보존한다.
    for (const part of [value.image.width, value.image.height, value.image.hotspotX, value.image.hotspotY]) append(bytes, part >>> 0);
    append(bytes, 0);
    append(bytes, fields.opaqueCount >>> 0);
    append(bytes, fields.clockStarted ? 1 : 0);
    return Uint8Array.from(bytes);
}

// 전체 용량의 입자를 원문 24바이트 순서로 비교한다. (2026-11-09 WB04-E189)
function particleBytes(particles) {
    const bytes = [];
    // 다섯 float 와 색 offset+1 토큰을 유지한다.
    for (const particle of particles) {
        for (const part of [particle.x, particle.y, particle.velocityX, particle.velocityY, particle.life]) append(bytes, floatBits(part));
        append(bytes, particle.pixel >>> 0);
    }
    return Uint8Array.from(bytes);
}

// 현재 원본 목록의 빈칸/토큰을 그대로 직렬화한다. (2026-11-09 WB04-E189)
const slotText = (registry) => (registry.slots.length ? registry.slots.join(',') : '-');

// 첫 차이를 행/항목/바이트로 보고한다. (2026-11-09 WB04-E189)
function compare(actual, expected, row, field) {
    assert.equal(actual.length, expected.length, `분해 수명 행 ${row} ${field} 폭`);
    for (let index = 0; index < actual.length; index += 1) {
        if (actual[index] !== expected[index]) assert.fail(`분해 수명 행 ${row} ${field} 바이트 ${index} 실제 ${actual[index]} 원본 ${expected[index]}`);
    }
}

// 예외가 나는지 본다. (2026-11-09 WB04-E189)
function throwsOf(action) {
    try { action(); } catch { return true; }
    return false;
}

// cpppj 시험의 source 50·dummy 60·대체 부모 61·폴 62 raw 입력을 원본과 똑같이 설치한다(폴은 61/62). (2026-11-09 WB04-E189)
function installSources(pool, depth) {
    // 네 SID 의 head·TYPE BYTE·깊이·좌표를 같은 값으로 다시 쓴다.
    for (const sid of [50, 60, 61, 62]) {
        const raw = pool.allocated(sid);
        raw.fill(0);
        put(raw, 0, 0x12900000);
        raw[10] = sid === 50 ? 144 : sid === 62 ? 145 : 110;
        raw[35] = sid === 50 ? depth & 0xff : sid === 61 ? 127 : 0;
        put(raw, 14, floatBits(sid === 50 ? 2.125 : sid === 60 ? 9.75 : sid === 61 ? -2.75 : 20.5));
        put(raw, 18, floatBits(sid === 50 ? 3.25 : sid === 60 ? 8.5 : sid === 61 ? 7.125 : 30.25));
    }
}

test('분해 수명: 실제 wrapper/분해 생성자/같은 준비와 반납 288입력을 실제 PE 와 비트 단위로 대조한다', () => {
    const pool = new SidPool(32768, { server: false });
    // 원본 입력의 source 50/dummy 60/대체 부모 61/폴 62 까지 확보한다.
    for (let sid = 5; sid <= 62; sid += 1) assert.equal(pool.allocate(2), sid);
    let count = 0;
    // 기대 결과를 공급하지 않고 각 명시 입력으로만 실행한다.
    for (const row of rows) {
        assert.equal(row.length, 16, '분해 수명 fixture 열 수');
        const orientation = Number(row[0]) | 0;
        const capacity = Number(row[1]);
        const hole = Number.parseInt(row[2], 10);
        const mutation = Number(row[4]);
        const flags = Number(row[5]);
        const opaque = Number(row[6]) !== 0;
        pool.physical(0)[35] = 255;
        installSources(pool, Number(row[3]));
        // 토큰 1 은 생성할 객체이며 기존 100+ 항목은 그대로 유지한다.
        const slots = Array.from({ length: capacity }, (_, index) => (index === hole ? 0 : 100 + index));
        let events = [];
        const event = (name, values) => {
            events.push(name.charCodeAt(0));
            // 원문 요청의 인수를 DWORD 폭으로 저장한다.
            for (const value of values) append(events, value >>> 0);
        };
        const registry = new RawRenderRegistry(pool, {
            reserve: (bytes) => { event('N', [bytes]); return true; },
            releaseArray: () => event('F', [1]),
            attach: () => { throw new Error('parent0 등록의 예상 밖 부착'); }
        }, slots);
        const game = new GameRandom(0x12345678);
        const state = { now: 1.125, pixelScaleX: 16, pixelScaleY: 11, rate: 1, cameraX: 0, cameraY: 0, diagnostic: 0 };
        const fields = createDissolveFrameFields();
        fields.common.delta = shrapnelFloatFromBits(0xa5a5a5a5);
        fields.common.scaledDelta = fields.common.delta;
        const frame = new RawDissolveFrame(pool, game, state, {
            reserve: (bytes) => { event('N', [bytes]); return true; },
            release: (scratch) => event('F', [scratch ? 5 : 3]),
            invalidate: () => { throw new Error('준비의 예상 밖 dirty'); },
            pixel: () => { throw new Error('준비의 예상 밖 픽셀'); },
            removeForm: () => { throw new Error('준비의 예상 밖 소진'); }
        });
        const lifetime = new RawDissolveLifetime(pool, registry, {
            reserve: (bytes) => { event('N', [bytes]); return true; },
            create: (type, value) => {
                event('C', [type, value]);
                assert.ok(type === 110 && value === 2);
                if (mutation === 1) {
                    const raw = pool.allocated(50);
                    put(raw, 14, floatBits(-7.75));
                    put(raw, 18, floatBits(8.25));
                    raw[35] = 128;
                }
                return 60;
            },
            pop: (sid, x, y, value) => {
                event('P', [sid, floatBits(x), floatBits(y), value]);
                const raw = pool.allocated(sid);
                put(raw, 14, floatBits(x));
                put(raw, 18, floatBits(y));
            },
            attach: (rect, type, dummy, value) => {
                event('A', [type, dummy, 0, value, rect.slot, rect.zOrder]);
                rect.form = 80;
                rect.parent = mutation === 2 ? 61 : dummy;
            },
            depend: (form, dummy) => event('D', [form, dummy]),
            capture: (source) => {
                event('I', [source]);
                // 실제 캡처가 반환한 명시 bitmap 입력이며 결과 입자는 계산하지 않는다(투명 행은 전부 255).
                const pixels = Uint8Array.from({ length: 12 }, (_, index) => (opaque ? (index * 13 + 7) % 255 : 255));
                if (mutation === 3) pool.allocated(50)[35] = 255;
                return { width: 3, height: 4, hotspotX: 1, hotspotY: 3, pixels };
            },
            prepare: (current, first, second, xOriented) => frame.initialize(current, first, second, xOriented)
        });
        assert.equal(lifetime.reserveObject(), true);
        const dummy = { value: 0 };
        lifetime.initialize(fields, 1, 50, 61, 62, orientation, dummy);
        assert.equal(dummy.value, 60);
        compare(fieldBytes(fields), decode(row[7]), count, '생성100');
        compare(particleBytes(fields.common.particles), decode(row[8]), count, '용량 전체');
        assert.equal(game.state, Number(row[9]), `분해 수명 행 ${count} 게임 난수`);
        assert.equal(Number(row[10]), 1, `분해 수명 행 ${count} CRT 상태`);
        assert.equal(slotText(registry), row[11], `분해 수명 행 ${count} 등록 목록`);
        compare(Uint8Array.from(events), decode(row[12]), count, '생성 요청');
        events = [];
        lifetime.destroy(fields, flags, (value) => {
            event('F', [value === ShrapnelRelease.Source ? 2 : value === ShrapnelRelease.Particles ? 3 : 4]);
            // 객체 반납 전에는 아직 등록이 살아 있고 객체 반납 때는 이미 빠져 있다.
            assert.equal(registry.slots[fields.common.registration.slot], value === ShrapnelRelease.Object ? 0 : 1);
        });
        const actual = fieldBytes(fields);
        const expected = decode(row[13]);
        // 삭제 뒤 원본은 포인터/용량을 그대로 두지만 JS 배열은 실제 반납하므로 그 두 저장소 표현만 제외한다.
        compare(actual.subarray(0, 32), expected.subarray(0, 32), count, '삭제 등록');
        compare(actual.subarray(36, 68), expected.subarray(36, 68), count, '삭제 시간/꼬리');
        compare(actual.subarray(72), expected.subarray(72), count, '삭제 메타데이터');
        assert.equal(slotText(registry), row[14], `분해 수명 행 ${count} 삭제 목록`);
        compare(Uint8Array.from(events), decode(row[15]), count, '반납 요청');
        assert.ok(fields.common.particles.length === 0 && fields.common.image.pixels.length === 0 && !fields.particlesAllocated);
        count += 1;
    }
    assert.equal(count, 288);
});

test('분해 수명 경계: 다른 풀/판본·경계 누락·확보 실패·출력 상자 누락·반납 경계 누락을 등록 전에 거부한다', () => {
    const pool = new SidPool(32768, { server: false });
    for (let sid = 5; sid <= 62; sid += 1) pool.allocate(2);
    const other = new SidPool(32768, { server: false });
    const registry = new RawRenderRegistry(pool, { reserve: () => true, releaseArray: () => {}, attach: () => {} }, []);
    const hooks = { reserve: () => false, create: () => 60, pop: () => {}, attach: () => {}, depend: () => {}, capture: () => ({}), prepare: () => {} };
    assert.ok(throwsOf(() => new RawDissolveLifetime({}, registry, hooks)));
    assert.ok(throwsOf(() => new RawDissolveLifetime(other, registry, hooks)), '표시 목록과 다른 풀');
    // 실제 경계 하나라도 없으면 빈 성공으로 바꾸지 않고 거부한다.
    for (const name of Object.keys(hooks)) assert.ok(throwsOf(() => new RawDissolveLifetime(pool, registry, { ...hooks, [name]: undefined })));
    let created = 0;
    const lifetime = new RawDissolveLifetime(pool, registry, { ...hooks, create: () => { created += 1; return 60; } });
    // 확보 실패는 false 를 돌려줄 뿐 등록/더미 생성에 들어가지 않는다.
    assert.equal(lifetime.reserveObject(), false);
    const fields = createDissolveFrameFields();
    assert.ok(throwsOf(() => lifetime.initialize(fields, 1, 50, 61, 62, 1, null)), '더미 출력 상자 누락');
    assert.ok(throwsOf(() => lifetime.initialize(fields, 1, 40000, 61, 62, 1, { value: 0 })), 'source 풀 밖');
    assert.equal(registry.slots.length, 0);
    assert.equal(created, 0);
    assert.ok(throwsOf(() => lifetime.destroy(fields, 1, null)));
});
