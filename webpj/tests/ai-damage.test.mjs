// AI 피해 반응 raw-ai-damage.js(RawAiDamage)를 실제 PE 관측 fixture 와 cpppj 계약으로 대조한다.
//  - ai-damage-x86.tsv 149행(원본 00415460): 열 = 독립 입력 17개(피해 대상 genus·164 여부·목록 count/용량·능력 비트·홈 SID·공격자 등), 호출 뒤 전체 본체 0xb08 hex, 네 목록 backing 블록 전체(미사용 꼬리 포함) hex, 홈/로그 요청 순서, 입력 슬롯 전체 hex.
//    각 행은 실제 memory/Pool 에 입력 바이트만 설치하고 `notice` 를 한 번 호출해 본체·목록·효과 순서·슬롯 보존·할당(5개)/해제(없음)을 모두 대조한다. 결과 필드는 fixture 에서만 읽는다.
//  - cpppj 계약: 미연결 실제 효과를 빈 성공으로 바꾸지 않고 요청 시점에 거부하며 앞선 목록 효과는 보존하고 가짜 토큰을 거부한다, 생성자 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E180: AI 피해 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { RawAiDamage } from '../www/core/raw-ai-damage.js';

// 실제 PE 관측 fixture 이다. (2026-11-09 WB04-E180)
const FIXTURE = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'ai-damage-x86.tsv');
const rows = (readRows(FIXTURE) ?? []).filter((row) => row[0] !== 'input');

// native 와 같은 실제 등록부의 첫 opaque32 핸들과 네 실제 목록 offset 이다. (2026-11-09 WB04-E180)
const BRAIN = 0x13000000;
const LISTS = [0x360, 0x36c, 0x378, 0x384];

// 예외가 나는지 본다. (2026-11-09 WB04-E180)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 16진 문자열로 바꾼다. (2026-11-09 WB04-E180)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 독립 입력으로 실제 memory/Pool 을 준비한다. 결과 필드는 fixture 에서만 읽는다. (2026-11-09 WB04-E180)
class DamageScene {
    // 입력 준비 뒤 같은 실제 등록부에 모듈을 연결한다.
    constructor(values) {
        this.input = values.split(',').map(Number);
        assert.equal(this.input.length, 17);
        this.pool = new SidPool(24000, { server: true });
        this.types = Array.from({ length: 188 }, () => ({ flags2: 0, group: 0 }));
        this.blocks = [];
        this.slots = [];
        this.events = [];
        this.allocation = 0;
        // 실제 할당 직후의 독립 입력이다(본체는 i×13, 이후 블록은 i×7 + 17×번호).
        this.memory = new RawAiBrainMemory(BRAIN, (bytes) => {
            for (let index = 0; index < bytes.length; index += 1) {
                bytes[index] = (this.allocation ? index * 7 + this.input[15] + (this.allocation - 1) * 17 : index * 13 + this.input[15]) & 0xff;
            }
            this.allocation += 1;
        });
        assert.equal(this.memory.allocate(AI_BRAIN_BYTES), BRAIN);
        for (let index = 0; index < LISTS.length; index += 1) {
            const block = this.memory.allocate(32);
            this.blocks.push(block);
            const offset = LISTS[index];
            const capacity = index === 1 ? this.input[5] : index === 2 ? this.input[8] : 4;
            const count = index === 1 ? this.input[6] : index === 2 ? this.input[9] : index === 0 ? this.input[3] : this.input[2];
            const raw = this.memory.bytes(BRAIN);
            put(raw, offset, block);
            put(raw, offset + 4, capacity);
            put(raw, offset + 8, count);
            // native 와 같은 초기 목록 내용이다.
            for (let item = 0; item < count; item += 1) {
                const value = index === 0 || index === 3 ? 50 : item === 0 && this.input[index === 1 ? 7 : 10] ? 60 : 200 + index * 10 + item;
                put(this.memory.bytes(block), item * 4, value);
            }
        }
        put(this.memory.bytes(BRAIN), 0x248, this.input[4]);
        put(this.memory.bytes(BRAIN), 0x404, 0x11223344);
        const victimType = this.input[1] ? 164 : 71;
        this.types[victimType].flags2 = this.input[0];
        this.types[140].flags2 = this.input[11] ? 0x100000 : 0;
        this.types[73].group = this.input[14];
        this.slot(50, victimType, 0, 5.5, 7.5);
        this.slot(60, 140, 0, -4.75, 9.25);
        if (this.input[12] > 0 && this.input[12] < 24000) this.slot(this.input[12], 73, this.input[13], 12.75, -3.5);
        this.slots.sort((left, right) => left - right);
        this.hooks = {
            home: (attacker) => { this.events.push(`H:${attacker}`); return this.input[12] & 0xffff; },
            log: (token, log) => {
                assert.equal(token, BRAIN);
                this.events.push(`L:${log.more ? 1 : 0}:${log.attacker}:${log.type}:${log.x}:${log.y}`);
            }
        };
        this.damage = new RawAiDamage(this.memory, this.pool, this.types, this.hooks);
    }

    // 실제 raw 슬롯의 입력 바이트만 준비한다. 할당/생성 성공을 합성하지 않는다. (2026-11-09 WB04-E180)
    slot(sid, type, state, x, y) {
        const raw = this.pool.physical(sid);
        // 전체 입력 padding 패턴이다.
        for (let index = 0; index < raw.length; index += 1) raw[index] = (index * 3 + sid) & 0xff;
        raw[10] = type;
        raw[11] = state;
        put(raw, 14, floatToBits(x));
        put(raw, 18, floatToBits(y));
        this.slots.push(sid);
    }

    // 네 실제 backing 블록의 보존 바이트도 모두 직렬화한다. (2026-11-09 WB04-E180)
    listsHex() { return this.blocks.map((token) => hex(this.memory.bytes(token))).join(''); }

    // 입력 슬롯이 피해 반응으로 변하지 않았는지 전체 폭을 비교한다. (2026-11-09 WB04-E180)
    slotsHex() { return this.slots.map((sid) => hex(this.pool.slot(sid))).join(''); }
}

test('AI 피해: 실제 PE 149입력의 전체 본체·네 목록·효과 순서·슬롯 보존을 대조한다', () => {
    assert.equal(rows.length, 149);
    const unique = new Set();
    // 모든 기대값은 두 정밀도로 일치한 실제 10.78 기계어 관찰이다.
    rows.forEach((row, index) => {
        assert.ok(row.length === 5 && !unique.has(row[0]), `입력 행 형식/중복 ${index}`);
        unique.add(row[0]);
        const scene = new DamageScene(row[0]);
        scene.damage.notice(BRAIN, 50, scene.input[16] & 0xffff);
        assert.equal(hex(scene.memory.bytes(BRAIN)), row[1], `피해 본체 행 ${index}`);
        assert.equal(scene.listsHex(), row[2], `피해 목록 행 ${index}`);
        assert.equal(scene.events.length === 0 ? '-' : scene.events.join(';'), row[3], `피해 효과 순서 행 ${index}`);
        assert.equal(scene.slotsHex(), row[4], `피해 슬롯 행 ${index}`);
        assert.ok(scene.memory.allocations.length === 5 && scene.memory.frees.length === 0, `피해 할당/해제 행 ${index}`);
    });
});

test('AI 피해: 미연결 실제 효과를 빈 성공으로 바꾸지 않고 요청 시점에 거부한다', () => {
    const scene = new DamageScene('512,0,0,0,2,4,0,0,4,0,0,1,100,0,3,19,60');
    const missing = new RawAiDamage(scene.memory, scene.pool, scene.types, {});
    assert.ok(throws(() => missing.notice(BRAIN, 50, 60)));
    assert.ok(get(scene.memory.bytes(BRAIN), 0x374) === 1 && get(scene.memory.bytes(scene.blocks[1]), 0) === 60 && get(scene.memory.bytes(BRAIN), 0x380) === 0);
    assert.ok(throws(() => missing.notice(BRAIN + 1, 50, 60)));
});

test('AI 피해: 공격자 0 은 아무 효과 없이 끝나고 생성자는 PATCH188/본체 소유자 누락을 거부한다', () => {
    const scene = new DamageScene('512,0,0,0,2,4,0,0,4,0,0,1,100,0,3,19,60');
    const seed = hex(scene.memory.bytes(BRAIN));
    scene.damage.notice(BRAIN, 50, 0);
    assert.ok(hex(scene.memory.bytes(BRAIN)) === seed && scene.events.length === 0);
    assert.ok(throws(() => new RawAiDamage(undefined, scene.pool, scene.types, scene.hooks)));
    assert.ok(throws(() => new RawAiDamage(scene.memory, {}, scene.types, scene.hooks)));
    assert.ok(throws(() => new RawAiDamage(scene.memory, scene.pool, scene.types.slice(1), scene.hooks)));
    assert.equal(scene.damage.pool, scene.pool);
});
