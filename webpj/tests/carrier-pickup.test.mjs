// Carrier 포획 raw-carrier-pickup.js(RawCarrierPickup·CarrierPickupState)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: carrier-pickup-x86.tsv(10.78 전용; x87 53/64 두 관측이 같은 열) 2657행 — 모드(0 요청 / 1 수신 / 2 기존 내용물 제거)·권한·미션 모드·후보·타입·genus·검색 결과·특정 타입·명령·헤더·변이 단계 등 29 DWORD 입력 →
//    호출 순서(P·S·I·V·Q·F·B·C·D·U·A), 최종 현재 전역 8칸(권한·미션·특정 타입·명령·내용물 타입·헤더 셋), 아홉 물리 슬롯 raw 전체, 실제 송신 패킷.
//  - 계약: CarrierPickup_LazyBoundariesAndPacketBounds(선택하지 않은 효과는 요구하지 않고 짧은 패킷과 도달한 누락만 거부).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E73: Carrier 포획 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { RawCarrierPickup, CarrierPickupState } from '../www/core/raw-carrier-pickup.js';

// 원본 관측 행이다(단일 판본 파일). 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E73)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'carrier-pickup-x86.tsv'));

// 독립 입력 수와 아홉 슬롯 SID 목록이다. (2026-11-08 WB04-E73)
const ROWS = 2657;
const SLOTS = [50, 51, 52, 60, 61, 70, 80, 81, 82];

// 예외가 나는지 본다. (2026-11-08 WB04-E73)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E73)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 원본과 같은 큰 외부 효과 입력만 공급하고 분기/임시 dead/종속 선택은 제품이 수행하는 장면이다. (2026-11-08 WB04-E73)
class CarrierPickupScene {
    constructor() {
        this.pool = new SidPool(24000, { server: true });
        this.types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));
        this.contained = new ContainedFinderState();
        this.single = false;
        this.state = new CarrierPickupState();
        this.row = [];
        this.trace = [];
        this.packet = '-';
        this.target = 60;
        this.currentPacket = null;
    }

    // 입력은 할당 여부와 별개인 원본 물리 raw 이며 제품의 공개 쓰기 API 를 추가하지 않는다.
    raw(sid) {
        return this.pool.physical(sid);
    }

    // 관측한 인자를 DWORD 로 기록한다.
    event(name, values = []) {
        this.trace.push([name, ...values].join(':'));
    }

    // 원문에 명시한 외부 변경만 재생하며 예상 반환/순서는 계산하지 않는다.
    mutate(where, sid = 0) {
        const m = this.row[17];
        if (m === 1 && where === 'candidate') this.contained.boss = false;
        if (m === 2 && where === 'candidate') { this.single = false; this.raw(60)[10] = 155; }
        if (m === 3 && where === 'assertion') this.contained.boss = true;
        if (m === 4 && where === 'reveal') { put(this.currentPacket, 4, 51, 2); this.state.command = 0xffffffff; }
        if (m === 5 && where === 'queue') { this.raw(this.target)[10] = 155; put(this.raw(this.target), 12, 0x2222, 2); }
        if (m === 6 && where === 'find') this.state.silentType = 169;
        if (m === 7 && where === 'destroy-old') this.state.silentType = this.row[13];
        if (m === 8 && where === 'after-discard') { this.raw(this.target)[10] = 133; put(this.raw(this.target), 12, 0x3333, 2); }
        if (m === 9 && where === 'create') this.raw(this.target)[11] = 0x0f;
        if (m === 10 && where === 'create') put(this.raw(this.target), 12, 0x4444, 2);
        if (m === 11 && where === 'detach' && sid === 80) put(this.raw(82), 4, 0, 2);
        if (m === 12 && where === 'detach' && sid === 80) this.raw(82)[10] = 7;
        if (m === 13 && where === 'attach') this.raw(this.target)[10] = 70;
        if (m === 14 && where === 'create') this.contained.containedType = 3;
        if (m === 15 && where === 'destroy-old') this.types[this.raw(this.target)[10]].flags2 = 0x100;
        if (m === 16 && where === 'reveal') this.contained.boss = false;
        if (m === 17 && where === 'queue') put(this.currentPacket, 4, 51, 2);
        if (m === 18 && where === 'create') this.raw(this.target)[10] = 70;
        if (m === 19 && where === 'send') this.state.request = { message: 0x55, protocol: 0xff, length: 3 };
    }

    // 실제 입력 raw/현재 TYPE/현재 전역만 설치한다.
    prepare(input) {
        assert.equal(input.length, 29);
        this.row = input;
        this.contained.boss = input[1] !== 0;
        this.single = input[2] !== 0;
        this.contained.containedType = input[22];
        this.contained.checkingDead = input[23] !== 0 ? 1 : 0;
        this.state = new CarrierPickupState();
        this.state.request = { message: input[19] & 0xff, protocol: input[18] & 0xff, length: input[20] & 0xffff };
        this.state.command = input[15];
        this.state.silentType = input[14];
        this.trace = [];
        this.packet = '-';
        this.target = 60;
        // 원문과 같은 아홉 슬롯과 미사용 바이트 패턴이다.
        for (const sid of SLOTS) {
            const raw = this.raw(sid);
            raw.fill(0xa5);
            put(raw, 0, 0x12200000);
            raw[10] = sid === 60 ? input[4] : sid === 61 ? input[10] : (sid === 50 || sid === 51) ? 133 : sid === 81 ? 3 : 6;
            raw[11] = sid === 50 ? input[6] : sid === 60 ? input[7] : sid >= 80 ? 8 : 0;
            raw[40] = 0;
            put(raw, 14, input[26]);
            put(raw, 18, input[27]);
            put(raw, 4, 0, 2);
            put(raw, 6, 0, 2);
        }
        put(this.raw(52), 18, input[13], 2);
        put(this.raw(50), 6, input[12], 2);
        const chain = input[16] === 0 ? [] : input[16] === 1 ? [80, 82] : input[16] === 2 ? [80, 81, 82] : [81, 80, 82];
        // 오벨리스크 변환은 큰 명시 효과이며 두 대상은 같은 종속 입력을 가진다.
        for (const sid of [60, 61]) {
            put(this.raw(sid), 6, chain.length === 0 ? 0 : chain[0], 2);
            put(this.raw(sid), 12, input[21], 2);
        }
        // 현재 next 를 바꾸는 효과보다 앞서 기본 finder 가 선택한다.
        chain.forEach((sid, index) => put(this.raw(sid), 4, index + 1 < chain.length ? chain[index + 1] : 0, 2));
        // 현재 genus 만 입력하며 선택 결과를 별도 공급하지 않는다.
        for (const type of this.types) type.flags2 = 0;
        this.types[input[4]].flags2 = input[8];
        this.types[input[10]].flags2 = input[11];
        this.types[155].flags2 = 0x2100;
        this.types[133].flags2 = 0x100;
    }

    // 현재 외부 효과를 원래 인자로만 관찰한다.
    hooks() {
        return {
            candidate: (sid, x, y) => { this.event('P', [sid, floatToBits(x), floatToBits(y)]); this.mutate('candidate'); return [this.row[3], this.row[25] !== 0]; },
            send: (owner, bytes) => { this.packet = hex(bytes); this.trace.push(`S:${owner}:${this.packet}`); this.mutate('send'); },
            assertion: (line) => { this.event('I', [line]); this.mutate('assertion'); },
            reveal: (sid) => { this.event('V', [sid]); this.target = this.row[9]; this.mutate('reveal'); return this.target; },
            queue: (command, sid, parent) => { this.target = sid; this.event('Q', [command, sid >>> 0, parent >>> 0]); this.mutate('queue'); },
            find: (sid, mask, kind, index) => { this.event('F', [sid, mask, kind, index]); this.mutate('find'); return this.row[12]; },
            afterDiscard: () => { this.event('B'); this.mutate('after-discard'); },
            create: (parent, kind, argument, flags, attachFlags, quality) => {
                this.event('C', [parent, kind, argument, flags, attachFlags, quality, this.raw(this.target)[11]]);
                this.mutate('create');
                return this.row[28];
            },
            destroy: (sid, flags) => { this.event('D', [sid, flags, this.raw(sid)[11], this.raw(sid)[10]]); this.mutate(sid === 52 ? 'destroy-old' : 'destroy-target', sid); },
            detach: (sid, flags) => { this.event('U', [sid, flags]); this.mutate('detach', sid); },
            attach: (sid, parent, flags) => { this.event('A', [sid, parent, flags]); this.mutate('attach', sid); }
        };
    }

    // 실제 최종 현재 전역을 원문과 같은 순서로 읽는다.
    stateText() {
        return [this.contained.boss ? 1 : 0, this.single ? 1 : 0, this.state.silentType >>> 0, this.state.command >>> 0, this.contained.containedType >>> 0, this.state.request.protocol, this.state.request.message, this.state.request.length].join(',');
    }

    // 현재 전역 getter 로 만든 제품 몸체이다. (2026-11-08 WB04-E73)
    body(hooks = this.hooks()) {
        return new RawCarrierPickup(this.pool, this.types, this.contained, () => this.contained.boss, () => this.single, this.state, hooks);
    }
}

test('CarrierPickup: 원본 10.78 2657행의 요청/서버 수신/기존 내용물 제거 효과 순서·최종 전역·아홉 슬롯 raw·송신 패킷이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new CarrierPickupScene();
    let count = 0;
    // 원본 입력은 재생하지만 원본 결과는 계산하지 않는다.
    for (const fields of rows) {
        assert.equal(fields.length, 9);
        scene.prepare(fields[0].split(',').map(Number));
        const pickup = scene.body();
        if (scene.row[0] === 0) pickup.request(50);
        else if (scene.row[0] === 2) pickup.discard(50);
        else {
            const packet = new Uint8Array(9);
            packet[0] = scene.row[18] & 0x7f;
            packet[1] = scene.row[19] & 0xff;
            put(packet, 2, scene.row[20], 2);
            put(packet, 4, 50, 2);
            put(packet, 6, 60, 2);
            packet[8] = scene.row[5] & 0xff;
            scene.currentPacket = packet;
            pickup.receive(scene.row[24], 0, packet);
        }
        const raws = SLOTS.map((sid) => hex(scene.raw(sid))).join(',');
        assert.equal(scene.trace.length === 0 ? '-' : scene.trace.join(';'), fields[1], `행 ${count}: 입력 ${fields[0]}`);
        assert.equal(scene.stateText(), fields[2], `행 ${count}: 입력 ${fields[0]}`);
        assert.equal(raws, fields[3], `행 ${count}: 입력 ${fields[0]}`);
        assert.equal(scene.packet, fields[4], `행 ${count}: 입력 ${fields[0]}`);
        assert.ok(fields[1] === fields[5] && fields[2] === fields[6] && fields[3] === fields[7] && fields[4] === fields[8]);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('CarrierPickup: 선택하지 않은 효과는 요구하지 않으며 짧은 패킷과 실제 도달한 경계 누락만 거부한다', () => {
    const scene = new CarrierPickupScene();
    const pickup = scene.body({ candidate: () => [0, false] });
    pickup.request(50);
    assert.equal(pickup.pool, scene.pool);
    assert.ok(throws(() => pickup.receive(0xffffffff, 0xffffffff, new Uint8Array(8))));
    const missing = scene.body({});
    assert.ok(throws(() => missing.discard(50)));
    assert.ok(throws(() => new RawCarrierPickup(scene.pool, scene.types.slice(1), scene.contained, () => true, () => false, scene.state, {})));
    assert.ok(throws(() => new RawCarrierPickup({ edition: 'cd1072' }, scene.types, scene.contained, () => true, () => false, scene.state, {})));
});
