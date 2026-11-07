// 대본 CreateAt 생성 부모(권한 진단·위치 해석·세 양수 관문·지면 소유자·직접 HP/프레임 쓰기·float32 Pop)를 원본 10.78 관측으로 대조한다(WB04-E31, cpppj CreateAtTests 이식).
//  - create-at-parent-x86.tsv 580행: 실제 parser/권한/지면/HP/메타 부모의 결과·호출 순서·현재 50바이트 전체.
//  - 계약: 선택하지 않은 하위 비요구, 첫 NUL/256바이트 지역 버퍼, 도달한 하위 실패 뒤에도 앞선 생성/소유자/HP 효과 보존, TYPE+15c 이웃 필드(첫 프레임/개수/종류)와 포인터 거부.
// TYPE 프레임 표(RiftTypeFrames)의 실제 연결은 자산 적재 단위에서 대조한다(여기서는 같은 인터페이스를 가진 검사용 표를 쓴다). (2026-11-08 WB04-E31: CreateAt 부모 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { floatToBits, readRows } from './raw-support.mjs';
import { CreateAtNullObject, createAtFrameField, createObjectAt } from '../www/core/raw-create-at.js';
import { parseDestroyAtLocation } from '../www/core/raw-destroy-at.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E31)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'create-at-parent-x86.tsv'));

// 원본 바이트 16진을 쓴다. 비정렬 리틀엔디언 DWORD 를 작은 합성 객체에 저장한다. (2026-11-08 WB04-E31)
const hex = (bytes) => Buffer.from(bytes).toString('hex');
const word = (raw, offset, value) => { for (let byte = 0; byte < 4; byte += 1) raw[offset + byte] = (value >>> (byte * 8)) & 0xff; };
const bits = (value) => value >>> 0;

// 기계어 0049b060 이 받는 것과 같은 독립 프레임 코드 입력에서 만든 글자별 첫 프레임/개수 표이다(방향 순서/개수는 명시 입력이며 첫 프레임 기대값이 아니다). (2026-11-08 WB04-E31)
function createAtFrames(reverse) {
    const first = new Array(16).fill(-1);
    const count = new Array(16).fill(0);
    let index = 0;
    // 16개 방향 글자마다 1 + 글자%3 개의 프레임을 순서대로 놓는다.
    for (let order = 0; order < 16; order += 1) {
        const side = reverse ? 15 - order : order;
        for (let number = 0; number < 1 + (side % 3); number += 1) {
            if (first[side] === -1) first[side] = index;
            count[side] += 1;
            index += 1;
        }
    }
    return {
        letterKinds: first.filter((value) => value !== -1).length,
        run: (letter) => {
            const kind = letter - 0x41;
            if (kind < 0 || kind >= 16) throw new RangeError('프레임 글자 범위');
            return { first: first[kind], count: count[kind] };
        },
    };
}

// 첫 열만 제품 입력으로 쓰며 나머지는 최초 원문 관측이다.
test('CreateAt 부모: 원본10.78 580행(권한·세 양수 관문·지면 소유자·HP·프레임·float32 Pop·전체 50바이트)', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of rows) {
        assert.equal(row.length, 4);
        // 입력 JSON: [원래 문자열(NUL 포함), Boss, HP, 지면, owner 효과, NULL].
        const [source, bossMode, hasHp, terrain, mutation, nullObject] = JSON.parse(row[0]);
        let boss = bossMode === 1;
        const raw = new Uint8Array(50).fill(0xcd);
        const surface = new Uint8Array(50);
        surface[34] = 3;
        let maximum = 0;
        const trace = [];
        let outcome = 'ok';
        const forward = createAtFrames(false);
        const reverse = createAtFrames(true);
        const hooks = {
            boss: () => boss,
            assertion: (where) => { trace.push(`assert:${where}`); if (bossMode === 2) boss = true; },
            // 실제 기존 parser 와 명시 타입 조회만 사용한다.
            parse: (text) => parseDestroyAtLocation(text, (name) => {
                trace.push(`type:${name === '' ? '-' : Buffer.from(name, 'latin1').toString('hex')}`);
                const key = name.replace(/[A-Z]/g, (character) => character.toLowerCase());
                return key === '' ? 0 : key === 'provider' ? 76 : key === 'suncannon' ? 131 : key === 'special2' ? 129 : key === 'special3' ? 140 : key === 'special4' ? 142 : -1;
            }),
            create: (type, flags) => {
                trace.push(`create:${type}:${flags}`);
                if (nullObject) return 0;
                raw.fill(0);
                word(raw, 0, 0x12001000);
                raw[10] = type;
                raw[11] = 4;
                word(raw, 36, 0xdeadbeef);
                maximum = type === 76 ? 203 : 503;
                return 1;
            },
            surface: (x, y) => { trace.push(`surface:${bits(x)}:${bits(y)}`); return terrain && x === 20 && y === 21 ? 2 : 0; },
            slot: (sid) => { assert.equal(sid, 2); return surface; },
            owner: (sid, owner) => {
                assert.equal(sid, 1);
                trace.push(`owner:${owner}`);
                raw[34] = owner & 0xff;
                if (mutation === 1) { raw[10] = 76; maximum = 203; }
                if (mutation === 2) maximum = 701;
            },
            hasHitPoints: (sid) => { assert.equal(sid, 1); trace.push('hasHp'); return hasHp !== 0; },
            maximumHitPoints: (sid) => { assert.equal(sid, 1); trace.push('maxHp'); return maximum; },
            hitPoints: (sid, value) => { assert.equal(sid, 1); trace.push(`hp:${value}`); word(raw, 26, bits(value)); },
            directional: (type) => { trace.push(`directional:${type}`); return type === 129 || type === 131 || type === 140 || type === 142; },
            frameField: (sid, offset) => { assert.equal(sid, 1); trace.push(`frameField:${offset}`); return createAtFrameField(raw[10] === 76 ? reverse : forward, offset); },
            frame: (sid, value) => { assert.equal(sid, 1); word(raw, 36, value); },
            pop: (sid, x, y, flags) => {
                assert.equal(sid, 1);
                const xb = floatToBits(x);
                const yb = floatToBits(y);
                trace.push(`pop:${xb}:${yb}:${flags}`);
                word(raw, 14, xb);
                word(raw, 18, yb);
            },
        };
        try { createObjectAt(source, hooks); } catch (error) { if (!(error instanceof CreateAtNullObject)) throw error; outcome = 'null'; trace.push('NULL'); }
        const label = `행 ${count + 1} ${row[0].slice(0, 60)}`;
        assert.equal(outcome, row[1], `${label} 결과`);
        assert.equal(trace.length === 0 ? '-' : trace.join(';'), row[2], `${label} 호출 순서`);
        assert.equal(hex(raw), row[3], `${label} 현재 50바이트`);
        count += 1;
    }
    assert.equal(count, 580);
});

// 선택하지 않은 하위를 요구하지 않고 첫 NUL/지역 버퍼 범위와 부분 효과를 구별한다.
test('CreateAt: 도달한 부모 실패 뒤에도 앞선 효과 보존', () => {
    let diagnostics = 0;
    const hooks = { boss: () => false, assertion: (line) => { assert.equal(line, 803); diagnostics += 1; } };
    createObjectAt('', hooks);
    assert.equal(diagnostics, 1);
    hooks.boss = () => true;
    // 첫 NUL 앞만 위치 해석에 넘기고 x 가 양수가 아니면 생성 하위를 요구하지 않는다.
    hooks.parse = (text) => { assert.equal(text, 'x'); return { x: 0, y: 1, type: 76, radius: 0, owner: 1 }; };
    createObjectAt('x\0ignored', hooks);
    assert.throws(() => createObjectAt('a'.repeat(256), hooks), RangeError);
    hooks.parse = () => ({ x: 20, y: 21, type: 131, radius: 44, owner: 1 });
    const effects = [];
    let expectedType = 131;
    hooks.create = (type, flags) => { assert.equal(type, expectedType); assert.ok(!flags); effects.push('create'); return 1; };
    hooks.owner = (_, owner) => { assert.equal(owner, 1); effects.push('owner'); };
    hooks.hasHitPoints = () => true;
    hooks.maximumHitPoints = () => 503;
    hooks.hitPoints = (_, value) => { assert.equal(value, 503); effects.push('hp'); };
    hooks.directional = () => true;
    // 반경 44 의 절반 22 → 0x15c + 88 = 0x1b4 는 프레임 표 구간(0x1b0 미만) 밖이라 이웃 필드로 거부한다.
    hooks.frameField = (_, offset) => { assert.equal(offset, 0x1b4); return createAtFrameField(createAtFrames(false), offset); };
    hooks.frame = () => effects.push('frame');
    hooks.pop = () => effects.push('pop');
    assert.throws(() => createObjectAt('explicit parser input', hooks), RangeError);
    assert.deepEqual(effects, ['create', 'owner', 'hp']);
    // owner 장치가 없으면 생성 뒤 그 위치에서 거부한다.
    expectedType = 76;
    hooks.parse = () => ({ x: 20, y: 21, type: 76, radius: 0, owner: 1 });
    delete hooks.owner;
    assert.throws(() => createObjectAt('explicit parser input', hooks), /미연결/);
    assert.equal(effects.at(-1), 'create');
});

// TYPE+15c 가 L 방향 첫 프레임에서 개수/종류 필드까지 넘어가는 물리표를 보존한다.
test('CreateAt: frameField 는 첫 프레임·개수·종류를 읽고 포인터 필드는 거부한다', () => {
    const frames = createAtFrames(true);
    assert.equal(createAtFrameField(frames, 0x15c), frames.run(0x4c).first);
    assert.equal(createAtFrameField(frames, 0x12c), 16);
    assert.equal(createAtFrameField(frames, 0x1ac), frames.run(0x50).count);
    assert.throws(() => createAtFrameField(frames, 0x124), RangeError);
    assert.throws(() => createAtFrameField(frames, 0x15d), RangeError);
    // 없는 글자의 첫 프레임 -1 은 DWORD 비트로 읽는다.
    const sparse = { letterKinds: 1, run: () => ({ first: -1, count: 0 }) };
    assert.equal(createAtFrameField(sparse, 0x130), 0xffffffff);
});
