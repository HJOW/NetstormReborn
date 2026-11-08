// Carrier 너겟 전달 raw-carrier-transfer.js(RawCarrierTransfer·CarrierTransferState)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: carrier-transfer-x86.tsv(10.78 전용; x87 53/64 두 관측 중 53비트 열) 1110행 — 모드(0 전달 / 1 주기 Halt / 2 수신)·권한·편집·검색 결과·cooldown·카운터·체인·내용물·소유자·genus·변이·헤더 등 24 DWORD 입력 →
//    호출 순서(F·D·A·P·S·M·X·G·V·H·C), 최종 상태 7칸(반환·카운터·cooldown·마지막 기증자·권한·편집·너겟 타입), 네 물리 슬롯 raw 전체, 송출 패킷. 실제 일반 finder 와 공간 해시를 사용한다.
//  - 계약: CarrierTransfer_SharedCounterAndLazyBoundaries(주기 카운터는 부모별로 분리하지 않고 signed 감김을 보존하며 도달하지 않은 효과는 요구하지 않음).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E74: Carrier 너겟 전달 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { RawCarrierTransfer, CarrierTransferState } from '../www/core/raw-carrier-transfer.js';

// 원본 관측 행이다(단일 판본 파일). 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E74)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'carrier-transfer-x86.tsv'));

// 독립 입력 수와 네 슬롯 SID 이다. (2026-11-08 WB04-E74)
const ROWS = 1110;
const SLOTS = [50, 52, 60, 61];

// 예외가 나는지 본다. (2026-11-08 WB04-E74)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E74)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 실제 일반 finder 를 사용하며 큰 외부 효과만 원문과 같은 입력으로 공급하는 장면이다. (2026-11-08 WB04-E74)
class CarrierTransferScene {
    constructor() {
        this.pool = new SidPool(24000, { server: true });
        this.hash = new SquidHash();
        this.types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));
        this.state = new CarrierTransferState();
        this.boss = false;
        this.editor = 0;
        this.owner = 0;
        this.row = [];
        this.trace = [];
        this.packet = '-';
    }

    // 할당 여부를 새 필터로 넣지 않는 원본 물리 raw 입력이다.
    raw(sid) {
        return this.pool.physical(sid);
    }

    // 원문과 같은 효과/현재 raw 변이만 공급한다.
    mutate(where) {
        const m = this.row[16];
        if (m === 1 && where === 'find-parent') this.editor = 1;
        if (m === 2 && where === 'find-parent') this.state.cooldown = 1;
        if (m === 3 && where === 'find-donor') { put(this.raw(52), 18, 155, 2); this.state.nuggetType = 155; }
        if (m === 4 && where === 'detach') { this.raw(52)[11] = 1; this.raw(50)[11] = 1; }
        if (m === 5 && where === 'attach') this.raw(60)[10] = 133;
        if (m === 6 && where === 'preview') put(this.raw(52), 18, 155, 2);
        if (m === 7 && where === 'broadcast') { this.raw(60)[10] = 133; put(this.raw(52), 18, 155, 2); }
        if (m === 8 && where === 'remove') put(this.raw(52), 18, 155, 2);
        if (m === 9 && where === 'choose') { this.boss = false; this.state.cooldown = 9; }
        if (m === 10 && where === 'go') put(this.raw(52), 18, 155, 2);
    }

    // 호출 인자의 DWORD 순서/폭을 그대로 기록한다.
    event(name, values) {
        this.trace.push([name, ...values].join(':'));
    }

    // 입력만 설치하며 예상 카운터/성공 여부/명령은 계산하지 않는다.
    prepare(input) {
        assert.equal(input.length, 24);
        this.row = input;
        this.boss = input[1] !== 0;
        this.editor = input[2];
        this.owner = input[21];
        this.state = new CarrierTransferState();
        this.state.haltCounter = input[5];
        this.state.cooldown = input[4];
        this.state.lastDonor = 77;
        this.state.emptyType = input[22];
        this.state.nuggetType = input[23];
        this.state.packet = { message: input[19] & 0xff, protocol: input[18] & 0xff, length: input[20] & 0xffff };
        this.trace = [];
        this.packet = '-';
        this.hash.reset();
        // 실제 단일 칸 발자국 입력이며 생성 결과는 공급하지 않는다.
        for (const type of this.types) { type.footX = 1; type.footY = 1; type.flags2 = 0; }
        this.types[154].flags2 = input[14];
        this.types[155].flags2 = (input[14] ^ 0x8000) >>> 0;
        // 원문과 같은 네 슬롯의 입력과 미사용 바이트 패턴이다.
        for (const sid of SLOTS) {
            const raw = this.raw(sid);
            raw.fill(0xa5);
            put(raw, 0, 0x12200000);
            raw[10] = sid === 50 ? 187 : sid === 52 ? 6 : sid === 60 ? input[13] : 133;
            raw[11] = sid === 52 ? input[9] : sid === 50 ? input[10] : 0;
            raw[34] = sid === 50 ? input[12] : sid === 60 ? input[11] : sid === 52 ? 0 : 2;
            raw[40] = 0;
            put(raw, 14, floatToBits(20.75));
            put(raw, 18, floatToBits(21.5));
        }
        put(this.raw(52), 18, input[8], 2);
        const chain = input[6] === 0 ? [50] : input[6] === 1 ? [60, 50] : [61, 60, 50];
        // 원문 finder 경계의 물리 체인을 같은 실제 버킷에 설치한다.
        chain.forEach((sid, index) => put(this.raw(sid), 4, index + 1 < chain.length ? chain[index + 1] : 0, 2));
        this.hash.setBucket(1, 20.75, 21.5, chain[0]);
    }

    // 반환을 대신 계산하지 않고 실제 큰 하위 효과만 관찰한다.
    hooks() {
        return {
            find: (sid, mask, kind, index) => {
                this.event('F', [sid, mask, kind, index]);
                this.mutate(sid === 50 ? 'find-parent' : 'find-donor');
                return sid === 50 ? this.row[3] : (sid === 60 && this.row[7] ? 52 : 0);
            },
            detach: (sid, flags) => { this.event('D', [sid, flags]); this.mutate('detach'); },
            attach: (sid, parent, flags) => { this.event('A', [sid, parent, flags]); this.mutate('attach'); },
            preview: (sid) => { this.event('P', [sid]); this.mutate('preview'); },
            broadcast: (bytes) => { this.packet = hex(bytes); this.trace.push(`S:${this.packet}`); this.mutate('broadcast'); },
            movementType: (type) => { this.event('M', [type, 0, 1]); return 40; },
            removeMovement: (sid, type) => { this.event('X', [sid, type, 0, 1]); this.mutate('remove'); },
            chooseGeyser: (sid) => { this.event('G', [sid]); this.mutate('choose'); return this.row[15]; },
            goGeyser: (sid, target) => { this.event('V', [sid, target]); this.mutate('go'); },
            goHome: (sid) => { this.event('H', [sid]); },
            commonHalt: (sid) => { this.event('C', [sid]); return this.row[17]; }
        };
    }

    // 현재 전역 getter 로 만든 제품 몸체이다. (2026-11-08 WB04-E74)
    body(hooks = this.hooks()) {
        return new RawCarrierTransfer(this.pool, this.hash, this.types, () => this.boss, () => this.editor, () => this.owner, this.state, hooks);
    }

    // 실제 결과와 현재 전역을 원문과 같은 순서로 기록한다.
    stateText(result) {
        return [result, this.state.haltCounter, this.state.cooldown, this.state.lastDonor, this.boss ? 1 : 0, this.editor, this.state.nuggetType].join(',');
    }
}

test('CarrierTransfer: 원본 10.78 1110행의 전달/주기 Halt/수신 효과 순서·최종 상태·네 슬롯 raw·송출 패킷이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new CarrierTransferScene();
    let count = 0;
    // 성공/조기 종료/로컬 수신/주기 분기를 모두 제품 본문으로 재생한다.
    for (const fields of rows) {
        assert.equal(fields.length, 9);
        scene.prepare(fields[0].split(',').map(Number));
        const transfer = scene.body();
        let result = 0;
        if (scene.row[0] === 0) result = transfer.transfer(50);
        else if (scene.row[0] === 1) result = transfer.halt(50);
        else {
            const packet = new Uint8Array(10);
            packet[0] = scene.row[18] & 0x7f;
            packet[1] = scene.row[19] & 0xff;
            put(packet, 2, scene.row[20], 2);
            put(packet, 4, 50, 2);
            put(packet, 6, 52, 2);
            put(packet, 8, 60, 2);
            transfer.receive(scene.owner, 0, packet);
        }
        const raws = SLOTS.map((sid) => hex(scene.raw(sid))).join(',');
        assert.equal(scene.trace.length === 0 ? '-' : scene.trace.join(';'), fields[1], `행 ${count}: 입력 ${fields[0]}`);
        assert.equal(scene.stateText(result), fields[2], `행 ${count}: 입력 ${fields[0]}`);
        assert.equal(raws, fields[3], `행 ${count}: 입력 ${fields[0]}`);
        assert.equal(scene.packet, fields[4], `행 ${count}: 입력 ${fields[0]}`);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('CarrierTransfer: 주기 카운터는 부모별로 분리하지 않으며 signed 감김을 보존하고 도달하지 않은 효과는 요구하지 않는다', () => {
    const scene = new CarrierTransferScene();
    scene.boss = false;
    scene.state.haltCounter = 0;
    const calls = [];
    const transfer = scene.body({ commonHalt: (sid) => { calls.push(sid); return 11; } });
    assert.equal(transfer.halt(50), 11);
    assert.equal(transfer.halt(60), 11);
    assert.equal(transfer.halt(50), 11);
    assert.equal(transfer.halt(60), 10);
    assert.equal(scene.state.haltCounter, 0);
    assert.deepEqual(calls, [50, 60, 50]);
    assert.equal(transfer.pool, scene.pool);
    scene.state.haltCounter = 0x7fffffff;
    assert.ok(transfer.halt(50) === 10 && scene.state.haltCounter === 0);
    const missing = scene.body({});
    assert.ok(throws(() => missing.halt(50)));
    assert.equal(scene.state.haltCounter, 1);
    assert.ok(throws(() => transfer.receive(999, 0xffffffff, new Uint8Array(7))));
    assert.ok(throws(() => new RawCarrierTransfer(scene.pool, scene.hash, scene.types.slice(1), () => true, () => 0, () => 1, scene.state, {})));
});
