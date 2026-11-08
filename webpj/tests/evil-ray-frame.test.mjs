// EvilRay 프레임/출력 raw-evil-ray-frame.js(RawEvilRayFrame)를 실제 PE 관측 fixture 로 대조한다(cpppj `tests/EvilRayFrameTests.cpp` 를 그대로 옮김).
//  - evilray-frame-x86.tsv 기본 027f 18행(프레임 10·출력 8, 비교 037f 18행은 중복 가산하지 않고 건너뜀): 열 = 이름, 종류(frame/draw), 정밀도, 호출 전 객체 raw, 용량 전체 입자, SID 0/50/51 의 물리 150바이트, 전역 28바이트,
//    매개변수(SHP 폭·source/부모 51/끝 음향 번호·진단이 source 를 51 로 바꾸는지), 호출 뒤 80바이트 의미 필드, 입자, 요청 순서(M 폭 조회·T 음향 번호·S 음향·D dirty·K 삭제·C clip·R 타원·A 진단)를 대조한다.
//    프레임 행은 프레임 뒤 삭제되지 않았으면 출력까지 실행한다(cpppj 시험과 같음).
//  - cpppj 계약: 부모 head 진단 뒤 저장한 부모 번호/현재 TYPE 사용, 출력의 진단 → clip → 용량 밖 오류 순서, 정확히 0 이 된 수명의 끝 음향이 다음 호출의 빈 중단 kill 보다 앞섬. 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E190: EvilRay 프레임 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import fs from 'node:fs';
import { REPO_ROOT } from './helpers.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { createFieryTailParticle } from '../www/core/raw-fiery-tail-frame.js';
import { createEvilRayFields } from '../www/core/raw-evil-ray-lifetime.js';
import { RawEvilRayFrame } from '../www/core/raw-evil-ray-frame.js';

// 실제 PE 관측 fixture 이다(헤더 행은 `#` 로 시작해 걸러진다). (2026-11-09 WB04-E190)
const rows = fs.readFileSync(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'evilray-frame-x86.tsv'), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && line[0] !== '#').map((line) => line.split('\t'));

// 원본 hex 를 바꾸지 않고 바이트로 읽는다('-' 는 빈 입자 배열). (2026-11-09 WB04-E190)
const decode = (text) => (text === '-' ? new Uint8Array(0) : Uint8Array.from(text.match(/../g) ?? [], (pair) => parseInt(pair, 16)));
// 소문자 hex 로 다시 쓴다(외부 요청 DWORD 표기). (2026-11-09 WB04-E190)
const encode = (bytes) => Array.from(bytes, (value) => value.toString(16).padStart(2, '0')).join('');

// 리틀엔디언 판독과 float/double 비트 변환이다. (2026-11-09 WB04-E190)
const read = (bytes, offset) => (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
function single(bytes, offset) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, read(bytes, offset), true);
    return view.getFloat32(0, true);
}
function double(bytes, offset) {
    const view = new DataView(new ArrayBuffer(8));
    view.setUint32(0, read(bytes, offset), true);
    view.setUint32(4, read(bytes, offset + 4), true);
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
// 외부 요청 DWORD 배열의 실제 바이트 hex 이다. (2026-11-09 WB04-E190)
const blob = (values) => {
    const bytes = [];
    for (const value of values) append(bytes, value >>> 0);
    return encode(bytes);
};

// 호스트 포인터/패딩을 뺀 원문 80바이트 의미 필드이다(cpppj 시험 Fields). (2026-11-09 WB04-E190)
function fieldBytes(fields) {
    const reg = fields.registration;
    const bytes = [];
    append(bytes, reg.form >>> 0);
    append(bytes, reg.parent >>> 0);
    // 네 영역 float 이다.
    for (const value of [reg.left, reg.top, reg.right, reg.bottom]) append(bytes, bits(value));
    append(bytes, reg.slot >>> 0);
    append(bytes, reg.zOrder >>> 0);
    append(bytes, fields.particles.length);
    append(bytes, fields.total >>> 0);
    appendDouble(bytes, fields.lastTime);
    appendDouble(bytes, fields.elapsed);
    append(bytes, bits(fields.delta));
    append(bytes, bits(fields.scaledDelta));
    append(bytes, fields.first ? 1 : 0);
    append(bytes, bits(fields.nextEmission));
    append(bytes, fields.source >>> 0);
    append(bytes, fields.stopping >>> 0);
    return encode(bytes);
}

// 사용 범위 밖을 포함한 전체 24바이트 입자 배열이다(미작성 속도/마지막 DWORD 포함). (2026-11-09 WB04-E190)
function particleBytes(particles) {
    const bytes = [];
    for (const particle of particles) {
        for (const value of [particle.x, particle.y, particle.velocityX, particle.velocityY, particle.life]) append(bytes, bits(value));
        append(bytes, particle.spare >>> 0);
    }
    return encode(bytes);
}

test('EvilRay 프레임/출력: 기본 027f 18행의 상태·모든 입자·요청 순서를 실제 PE 와 비트 단위로 대조한다', () => {
    let count = 0;
    let extended = 0;
    // 독립 원문 입력으로만 각 몸체를 실행한다.
    rows.forEach((row, current) => {
        assert.equal(row.length, 11);
        if (Number(row[2]) === 0x037f) { extended += 1; return; }
        const raw = decode(row[3]);
        const particles = decode(row[4]);
        const parents = decode(row[5]);
        const stateBytes = decode(row[6]);
        const parameters = row[7].split(',');
        const pool = new SidPool(32768, { server: false });
        // 세 현재 물리 슬롯만 원문 입력으로 덮는다.
        [0, 50, 51].forEach((sid, index) => pool.physical(sid).set(parents.subarray(index * 50, index * 50 + 50)));
        const fields = createEvilRayFields();
        const reg = fields.registration;
        reg.form = read(raw, 4) | 0;
        reg.parent = read(raw, 8) | 0;
        reg.left = single(raw, 12);
        reg.top = single(raw, 16);
        reg.right = single(raw, 20);
        reg.bottom = single(raw, 24);
        reg.slot = read(raw, 28) | 0;
        reg.zOrder = read(raw, 32) | 0;
        fields.total = read(raw, 40) | 0;
        fields.lastTime = double(raw, 48);
        fields.elapsed = double(raw, 56);
        fields.delta = single(raw, 64);
        fields.scaledDelta = single(raw, 68);
        fields.first = read(raw, 72) !== 0;
        fields.nextEmission = single(raw, 80);
        fields.source = read(raw, 84);
        fields.stopping = read(raw, 88);
        // 입력 전체 용량과 미사용 비트이다.
        for (let at = 0; at < particles.length; at += 24) {
            fields.particles.push({ x: single(particles, at), y: single(particles, at + 4), velocityX: single(particles, at + 8), velocityY: single(particles, at + 12), life: single(particles, at + 16), spare: read(particles, at + 20) });
        }
        const state = { now: double(stateBytes, 0), pixelScaleX: single(stateBytes, 8), pixelScaleY: single(stateBytes, 12), rate: single(stateBytes, 16), cameraX: single(stateBytes, 20), cameraY: single(stateBytes, 24), diagnostic: 0 };
        const colors = [17, 0x12345678, 0xabcdef01];
        const events = [];
        let killed = false;
        // 실제 외부 경계의 인자/순서만 관찰한다.
        const body = new RawEvilRayFrame(pool, state, colors, {
            shapeWidth: (sid) => {
                const slot = pool.slot(sid);
                events.push(`M:${slot[10]}:${slot[40] & 1}:${read(slot, 36)}`);
                return (Number.parseInt(parameters[0], 10) << 16) >> 16;
            },
            soundId: (sid, end) => {
                events.push(`T:${sid}:${end ? 1 : 0}`);
                return Number(parameters[end ? 3 : sid === 51 ? 2 : 1]) >>> 0;
            },
            sound: (x, y, id) => events.push(`S:${blob([bits(x), bits(y), id, 0, 0])}`),
            invalidate: (rect) => events.push(`D:${blob([rect.left, rect.top, rect.right, rect.bottom, 0])}`),
            kill: (flags) => { events.push(`K:${flags}`); killed = true; },
            clip: (rect) => events.push(`C:${rect.join(',')}`),
            circle: (circle) => events.push(`R:${blob([circle.x, circle.y, circle.radius, circle.color])}`),
            assertion: (expression, file, number) => {
                assert.equal(file, '.\\Evilray.cpp');
                events.push(`A:${number}:${expression}`);
                if (Number(parameters[4])) fields.source = 51;
            }
        });
        if (row[1] === 'frame') body.step(fields);
        if (!killed) body.draw(fields);
        assert.equal(fieldBytes(fields), row[8], `EvilRay 프레임 행 ${current} 의미 필드`);
        // 빈 입자 배열은 fixture 에서 '-' 로 적혀 있다(cpppj Hex("-") = 빈 배열).
        assert.equal(particleBytes(fields.particles), row[9] === '-' ? '' : row[9], `EvilRay 프레임 행 ${current} 입자`);
        assert.equal(events.length ? events.join(';') : '-', row[10], `EvilRay 프레임 행 ${current} 요청 순서`);
        count += 1;
    });
    assert.equal(count, 18);
    assert.equal(extended, 18);
});

test('EvilRay 부모 진단: 저장한 부모 번호와 현재 TYPE 를 쓰고 출력 장치는 도달 순서(진단 → clip → 용량 밖 오류)로만 요구한다', () => {
    const pool = new SidPool(32768, { server: false });
    const parent = pool.physical(51);
    parent[6] = 0;
    parent[7] = 0;
    const state = { now: 1, pixelScaleX: 16, pixelScaleY: 11, rate: 1, cameraX: 0, cameraY: 0, diagnostic: 0 };
    const fields = createEvilRayFields();
    fields.source = 50;
    fields.registration.parent = 51;
    fields.first = true;
    fields.lastTime = 1;
    let diagnoses = 0;
    let dirty = 0;
    // 실제 게임/음향/Host 없이 선택 경계만 설치한다.
    const body = new RawEvilRayFrame(pool, state, [], {
        shapeWidth: () => 9,
        assertion: (expression, file, line) => {
            assert.ok(expression === 'parent->getHead() != 0' && file === '\\Ns\\O\\Baseprocess.cpp' && line === 0xa4);
            diagnoses += 1;
            parent[10] = 74;
            fields.registration.parent = 52;
        },
        soundId: (sid, end) => {
            assert.equal(end, false);
            if (sid === 50) return 0;
            assert.ok(sid === 51 && pool.slot(sid)[10] === 74);
            return 17;
        },
        sound: (x, y, id) => assert.equal(id, 17),
        invalidate: () => { dirty += 1; }
    });
    body.step(fields);
    assert.ok(diagnoses === 1 && dirty === 2 && fields.nextEmission === 15 && fields.total === 0);
    const invalid = createEvilRayFields();
    invalid.total = 1;
    const order = [];
    const draw = new RawEvilRayFrame(pool, state, [], {
        assertion: (expression, file, line) => { assert.ok(expression === 'totalParticles <= maxParticles' && line === 0x8c); order.push(1); },
        clip: (rect) => { assert.deepEqual(rect, [0, 0, 639, 479]); order.push(2); }
    });
    assert.throws(() => draw.draw(invalid), RangeError);
    assert.deepEqual(order, [1, 2]);
    // 미연결 장치는 도달했을 때만 오류이며 다른 판본은 생성 때 거부한다.
    assert.throws(() => new RawEvilRayFrame({}, state, [], {}));
    assert.throws(() => new RawEvilRayFrame(pool, state, [], {}).draw(createEvilRayFields()));
});

test('EvilRay 정확히 0 인 수명: 끝 음향이 이번 호출에 나고 활성으로 센 뒤 다음 호출에서 빈 중단 kill 이 난다', () => {
    const pool = new SidPool(32768, { server: false });
    const state = { now: 1.1, pixelScaleX: 16, pixelScaleY: 11, rate: 1, cameraX: 0, cameraY: 0, diagnostic: 0 };
    const fields = createEvilRayFields();
    fields.source = 50;
    fields.registration.parent = 51;
    fields.first = true;
    fields.lastTime = 1;
    fields.stopping = 1;
    fields.total = 1;
    fields.particles.push({ ...createFieryTailParticle(), x: 1, y: 2, life: 1 });
    let sounds = 0;
    let dirty = 0;
    let killed = 0;
    // 순수 시계/끝 음향과 종료 요청만 관찰한다.
    const body = new RawEvilRayFrame(pool, state, [], {
        shapeWidth: () => 9,
        soundId: (sid, end) => { assert.ok(sid === 51 && end); return 31; },
        sound: (x, y, id) => { assert.equal(id, 31); sounds += 1; },
        invalidate: () => { dirty += 1; },
        kill: (flags) => { assert.equal(flags, 0); killed += 1; }
    });
    body.step(fields);
    assert.ok(fields.particles[0].life === 0 && sounds === 1 && dirty === 2 && killed === 0);
    body.step(fields);
    assert.ok(sounds === 1 && dirty === 2 && killed === 1);
});
