// 크리스탈 크랩 포획 raw-carrier-steal.js(RawCarrierSteal)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: carrier-steal-x86.tsv 1105행(두 x87 정밀도 동일) — 입력 30 DWORD(종류 0 포획/1 수신/2 정지/3 PostPop·boss·편집·내용물 존재·체인 길이·기증자 flags2·동맹 방향·할당 성공·외부 변이 종류·헤더 등)
//    → 외부 효과 순서(B 공통 정지/N 할당/R 예약/K 검사/Z Damageable/F 조회/D 분리/A 부착/P 미리보기/S 송신 패킷/M·X 이동 제거/H 귀환/W 음향), 결과·boss·편집·사제 타입·정지 카운터, 네 슬롯(50·52·60·61) 전체 raw, 송신 패킷 hex.
//  - 계약: CarrierSteal_LazyBoundariesAndPacketBounds(비권한 조기 종료와 실제로 도달한 누락 경계 구분, 공유 카운터, 두 가상 표 분배, 짧은 명령 거부).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E92: 크리스탈 크랩 포획 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { RawCarrierSteal } from '../www/core/raw-carrier-steal.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E92)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'carrier-steal-x86.tsv'));
const ROWS = 1105;

// 예외가 나는지 본다. (2026-11-08 WB04-E92)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E92)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 단일 칸 좌표의 float 비트(20.75, 21.5)이다. (2026-11-08 WB04-E92)
const floatBits = (value) => { const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true); return view.getUint32(0, true); };

// 실제 일반 finder 를 사용하며 큰 외부 효과만 원문과 같은 입력으로 공급하는 장면이다. (2026-11-08 WB04-E92)
class CarrierStealScene {
    constructor() {
        this.pool = new SidPool(24000);
        this.hash = new SquidHash();
        this.types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));
        this.alliances = new Array(81).fill(0);
        this.haltCounter = { value: 0 };
        this.carrierType = 156;
        this.priestType = 158;
        this.header = { message: 1, protocol: 0x60, length: 10 };
        this.currentPacket = null;
        this.boss = false;
        this.editor = 0;
        this.owner = 0;
        this.row = [];
        this.trace = [];
        this.packet = '-';
    }

    // 할당 여부를 새 필터로 넣지 않는 원본 물리 raw 입력이다.
    raw(sid) { return this.pool.physical(sid); }

    // 원문과 같은 효과/현재 raw 변이만 공급한다.
    mutate(where) {
        const m = this.row[17];
        if (m === 1 && where === 'find-parent') this.editor = 1;
        if (m === 2 && where === 'find-donor') { put(this.raw(52), 18, 158, 2); this.raw(52)[12] = 1; }
        if (m === 3 && where === 'find-donor') this.priestType = 154;
        if (m === 4 && where === 'detach') { this.raw(52)[11] = 1; this.raw(50)[11] = 1; }
        if (m === 5 && where === 'attach') this.raw(60)[10] = 71;
        if (m === 6 && where === 'preview') put(this.raw(52), 18, 155, 2);
        if (m === 7 && where === 'broadcast') { this.raw(60)[10] = 71; put(this.raw(52), 18, 155, 2); }
        if (m === 8 && where === 'remove') put(this.raw(52), 18, 155, 2);
        if (m === 9 && where === 'sound') { put(this.raw(52), 18, 155, 2); this.boss = false; }
        if (m === 10 && where === 'detach') put(this.currentPacket, 4, 61, 2);
        if (m === 11 && where === 'attach') put(this.currentPacket, 4, 60, 2);
        if (m === 12 && where === 'allocation') { this.boss = false; this.raw(50)[40] = 9; }
        if (m === 13 && where === 'check') { this.boss = true; this.raw(50)[40] = 4; }
    }

    // 호출 인자의 DWORD 순서/폭을 그대로 기록한다.
    event(name, values) { this.trace.push(values.reduce((text, value) => `${text}:${value >>> 0}`, name)); }

    // 입력만 설치하며 예상 성공 여부/명령은 계산하지 않는다.
    prepare(input) {
        assert.equal(input.length, 30);
        const row = input;
        this.row = row;
        this.boss = row[1] !== 0;
        this.editor = row[2];
        this.owner = row[23];
        this.haltCounter.value = row[24];
        this.carrierType = row[21];
        this.priestType = row[22];
        this.header = { message: row[19] & 0xff, protocol: row[18] & 0xff, length: row[20] & 0xffff };
        this.trace = [];
        this.packet = '-';
        this.hash.reset();
        this.alliances.fill(0);
        this.alliances[row[6] * 9 + row[7]] = row[8];
        this.alliances[row[7] * 9 + row[6]] = row[9];
        // 실제 단일 칸 발자국/genus 입력이다. 예상 후보/분기 결과는 공급하지 않는다.
        for (const type of this.types) { type.footX = 1; type.footY = 1; type.flags2 = 0; }
        this.types[row[10]].flags2 = row[5];
        this.types[154].flags2 = row[16];
        this.types[158].flags2 = row[16];
        this.types[155].flags2 = (row[16] ^ 0x8000) >>> 0;
        // 원문과 같은 네 슬롯의 입력과 미사용 바이트 패턴이다.
        for (const sid of [50, 52, 60, 61]) {
            const raw = this.raw(sid);
            raw.fill(0xa5);
            put(raw, 0, 0x12200000);
            raw[10] = sid === 50 ? 156 : sid === 52 ? 6 : sid === 60 ? row[10] : 71;
            raw[11] = sid === 52 ? row[15] : sid === 50 ? row[14] : 0;
            raw[34] = sid === 50 ? row[6] : sid === 60 ? row[7] : sid === 52 ? 0 : 2;
            raw[40] = sid === 50 ? row[26] : 0;
            put(raw, 14, floatBits(20.75));
            put(raw, 18, floatBits(21.5));
        }
        put(this.raw(52), 18, row[12], 2);
        this.raw(52)[12] = row[13] & 0xff;
        const chain = row[4] === 0 ? [50] : row[4] === 1 ? [60, 50] : [61, 60, 50];
        // 원문 finder 경계의 물리 체인을 실제 같은 버킷에 설치한다.
        chain.forEach((sid, index) => put(this.raw(sid), 4, index + 1 < chain.length ? chain[index + 1] : 0, 2));
        this.hash.setBucket(1, 20.75, 21.5, chain[0]);
    }

    // 반환을 대신 계산하지 않고 실제 큰 하위 효과만 관찰한다.
    hooks() {
        const row = () => this.row;
        return {
            commonHalt: (sid) => { this.event('B', [sid]); return row()[29]; },
            allocateRegular: () => { this.event('N', [0x28]); this.mutate('allocation'); return row()[27] !== 0; },
            regular: (sid, event, payload) => { this.event('R', [sid, event, floatBits(payload)]); },
            check: (sid) => { this.event('K', [sid]); this.mutate('check'); },
            damageablePostPop: (sid, flags) => { this.event('Z', [sid, flags]); },
            find: (sid, mask, kind, index) => {
                this.event('F', [sid, mask, kind, index]);
                this.mutate(sid === 50 ? 'find-parent' : 'find-donor');
                return sid === 50 ? row()[3] : sid === 60 && row()[11] ? 52 : 0;
            },
            detach: (sid, flags) => { this.event('D', [sid, flags]); this.mutate('detach'); },
            attach: (sid, parent, flags) => { this.event('A', [sid, parent, flags]); this.mutate('attach'); },
            preview: (sid) => { this.event('P', [sid]); this.mutate('preview'); },
            broadcast: (bytes) => { this.packet = hex(bytes); this.trace.push(`S:${this.packet}`); this.mutate('broadcast'); },
            movementType: (type) => { this.event('M', [type, 0, 1]); return 40; },
            removeMovement: (sid, type) => { this.event('X', [sid, type, 0, 1]); this.mutate('remove'); },
            goHome: (sid) => { this.event('H', [sid]); },
            // 전역 음향의 다섯 인자를 낮은 폭으로 줄이지 않고 기록한다.
            sound: (name, loop, volume, pan, priority, limit) => {
                this.trace.push(`W:${name}${[loop, volume, pan, priority, limit].reduce((text, value) => `${text}:${value >>> 0}`, '')}`);
                this.mutate('sound');
            }
        };
    }

    // 현재 장면으로 포획 모듈을 만든다.
    steal(hooks = this.hooks()) {
        return new RawCarrierSteal(this.pool, this.hash, this.types, () => this.boss, () => this.editor, () => this.owner, this.alliances, () => this.carrierType, () => this.priestType, this.header, this.haltCounter, hooks);
    }

    // 실제 결과와 현재 전역을 원문과 같은 순서로 기록한다.
    state(result) { return [result, this.boss ? 1 : 0, this.editor, this.priestType, this.haltCounter.value].map((value) => value >>> 0).join(','); }
}

test('CarrierSteal: 원본 10.78 1105입력의 포획/수신/정지/PostPop 효과 순서·결과·전역·네 슬롯 raw·송신 패킷이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new CarrierStealScene();
    let count = 0;
    // 실제 검색/수신 결과를 두 정밀도 원문과 별도로 비교한다.
    for (const fields of rows) {
        assert.equal(fields.length, 9);
        scene.prepare(fields[0].split(',').map(Number));
        const steal = scene.steal();
        let result = 0;
        if (scene.row[0] === 0) result = steal.try(50);
        else if (scene.row[0] === 2) result = steal.halt(50);
        else if (scene.row[0] === 3) steal.postPop(50, scene.row[25]);
        else {
            const packet = new Uint8Array(10);
            packet[0] = scene.row[18] & 0x7f;
            packet[1] = scene.row[19] & 0xff;
            put(packet, 2, scene.row[20], 2);
            put(packet, 4, 50, 2);
            put(packet, 6, 52, 2);
            put(packet, 8, 60, 2);
            scene.currentPacket = packet;
            steal.receive(scene.owner, 0, packet);
        }
        const label = `입력 ${fields[0]}`;
        assert.equal(scene.trace.length === 0 ? '-' : scene.trace.join(';'), fields[1], label);
        assert.equal(scene.state(result), fields[2], label);
        assert.equal([50, 52, 60, 61].map((sid) => hex(scene.raw(sid))).join(','), fields[3], label);
        assert.equal(scene.packet, fields[4], label);
        assert.deepEqual(fields.slice(1, 5), fields.slice(5), label);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('CarrierSteal: 비권한 조기 종료와 실제로 도달한 누락 경계를 구분하고 공유 정지 카운터·두 가상 표 분배·짧은 명령 거부를 확인한다', () => {
    const scene = new CarrierStealScene();
    const steal = scene.steal({});
    assert.equal(steal.try(50), 10);
    assert.equal(steal.pool, scene.pool);
    const parents = [];
    const shared = scene.steal({ commonHalt: (parent) => { parents.push(parent); return 11; } });
    assert.ok(shared.halt(50) === 11 && shared.halt(60) === 11 && shared.halt(50) === 11 && shared.halt(60) === 10);
    assert.ok(scene.haltCounter.value === 0 && JSON.stringify(parents) === '[50,60,50]');
    // PE 의 두 +20 은 같은 42abb0 이며 Halt 는 서로 다르다.
    put(scene.raw(50), 0, 0x503f20);
    assert.ok(shared.handles(50));
    put(scene.raw(50), 0, 0x503ff0);
    assert.ok(shared.handles(50));
    scene.boss = true;
    assert.ok(throws(() => steal.try(50)));
    assert.ok(throws(() => steal.receive(0, 0, new Uint8Array(7))));
    assert.ok(throws(() => new RawCarrierSteal({ edition: 'cd1072' }, scene.hash, scene.types, () => false, () => 0, () => 0, scene.alliances, () => 0, () => 0, scene.header, scene.haltCounter, {})));
    assert.ok(throws(() => new RawCarrierSteal(scene.pool, scene.hash, scene.types, false, () => 0, () => 0, scene.alliances, () => 0, () => 0, scene.header, scene.haltCounter, {})));
});
