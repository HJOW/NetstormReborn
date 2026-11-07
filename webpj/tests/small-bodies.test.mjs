// 소형 몸체 네 개(플라이어 삭제 전처리·펜스 타입 부모·사제 희생 요청·캐리어 채취 통지)를 원본 10.78 관측으로 대조한다(WB04-E14,
// cpppj FlyerPreDestroyTests·FenceTypeParentTests·PriestSacrificeRequestTests·CarrierMiningTests 이식).
//  - flyer-predestroy-x86.tsv 96행: +8 WORD 대상 삭제 요청·flags·callback 뒤 네 슬롯 200 BYTE. fence-type-parent-x86.tsv 432행: 두 부모의 진단/복귀 뒤 raw·전역 재판정.
//  - priest-sacrifice-request-x86.tsv 128행: 호출 순서·현재 owner 재독·반환. carrier-mining-x86.tsv 864행: 헤더 길이 비보정·bit7 제거·송신 전후 같은 버퍼·현재 지면 상태(두 정밀도).
//  - 계약: 선택된 경계만 필수, 판본/물리 슬롯 오류는 효과 전에 거부.
// cpppj 의 실제 Fence 자식(RawFenceRebuild·RawFenceLifetime) 연결 검사는 해당 단위를 옮길 때 대조한다. (2026-11-08 WB04-E14: 소형 몸체 네 개 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawFlyerPreDestroy } from '../www/core/raw-flyer-pre-destroy.js';
import { runFenceTypeParent, FENCE_TYPE_OPERATION } from '../www/core/raw-fence-type-parent.js';
import { RawPriestSacrificeRequest } from '../www/core/raw-priest-sacrifice-request.js';
import { RawCarrierMining } from '../www/core/raw-carrier-mining.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E14)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const flyerRows = fixture('flyer-predestroy-x86.tsv');
const fenceRows = fixture('fence-type-parent-x86.tsv');
const priestRows = fixture('priest-sacrifice-request-x86.tsv');
const miningRows = fixture('carrier-mining-x86.tsv');

// 입력 DWORD 를 32비트로 감아 읽고 바이트를 소문자 hex 로 표시한다. (2026-11-08 WB04-E14)
const number = (text) => Number(BigInt(text) & 0xffffffffn);
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 대상 주소를 DWORD 로 읽지 않고 현재 WORD 만 가상 삭제한 뒤 같은 flags 로 하위를 호출한다.
test('플라이어 삭제 전처리: 원본10.78 96행(WORD 대상·재진입·네 슬롯 200 BYTE)', { skip: !flyerRows && '원본 fixture 없음' }, () => {
    const pool = new SidPool(32768, { server: false });
    const ids = [50, 60, 61, 32767];
    let count = 0;
    for (const row of flyerRows) {
        assert.equal(row.length, 5);
        const child = number(row[0]) & 0xffff;
        const flags = number(row[1]);
        const mutation = number(row[2]);
        // 실제 TYPE/상태가 +8 WORD 위에 있으므로 넓게 읽으면 다른 대상이 된다(이전 행의 dead/free 도 재설치).
        for (const sid of ids) {
            const bytes = pool.physical(sid);
            bytes.fill(0);
            put(bytes, 0, 0x12400000);
            bytes[10] = 144;
            bytes[11] = 2;
            put(bytes, 14, floatToBits(2.125));
            put(bytes, 18, floatToBits(3.25));
            if (sid === 50) put(bytes, 8, child, 2);
        }
        const events = [];
        const logic = new RawFlyerPreDestroy(pool, {
            destroy: (sid, value) => {
                events.push(`D:${sid}:${value}`);
                assert.deepEqual([sid, value], [child, 0]);
                if (mutation === 1) { put(pool.physical(50), 8, 61, 2); pool.physical(50)[40] = 9; }
                if (mutation === 2) { put(pool.physical(50), 8, 0, 2); pool.physical(sid)[11] = 1; }
            },
            damageable: (sid, value) => {
                events.push(`P:${sid}:${value}`);
                if (mutation === 3) put(pool.physical(50), 14, floatToBits(9.75));
            },
        });
        logic.preDestroy(50, flags);
        assert.equal(events.join(';'), row[3], `행 ${count} 호출`);
        const slots = Buffer.concat(ids.map((sid) => Buffer.from(pool.slot(sid))));
        assert.equal(slots.toString('hex'), row[4], `행 ${count} 슬롯`);
        count += 1;
    }
    assert.equal(count, 96);
});

// 두 부모의 정상 반환/진단 중단과 복귀 뒤 raw/전역 변경을 432행 전체와 비교한다.
test('펜스 타입 부모: 원본10.78 432행(진단·복귀 뒤 raw/전역 재판정)', { skip: !fenceRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of fenceRows) {
        assert.equal(row.length, 6);
        const input = row[0].split(',').map(number);
        assert.equal(input.length, 6);
        let checking = input[1];
        let type = input[3];
        let trace = '';
        let outcome = 'return';
        const raw = new Uint8Array(50);
        // 무관한 49 BYTE 쓰기까지 검출하는 표식이다.
        for (let index = 0; index < raw.length; index += 1) raw[index] = (index * 17 + 3) & 0xff;
        raw[10] = input[2];
        const hooks = {
            assertion: (expression, file, line) => {
                assert.deepEqual([expression, file], ['fence.getType() == sunFenceType', '\\Ns\\O\\Fence.cpp']);
                trace = `D:${Buffer.from(expression).toString('hex')}:${Buffer.from(file).toString('hex')}:${line}`;
                if (!input[5]) throw new Error('명시 진단 중단');
                if (input[4] === 1) { raw[10] = type & 0xff; checking = 0; }
                if (input[4] === 2) { type = ((type & 0xffffff00) | raw[10]) >>> 0; checking = 0; }
            },
            body: (sid) => { assert.equal(sid, 50); trace += `${trace ? ';' : ''}${input[0] === 0 ? 'B:50' : 'F:50'}`; },
        };
        try {
            runFenceTypeParent(raw, 50, () => type, () => checking, input[0] === 0 ? FENCE_TYPE_OPERATION.rebuild : FENCE_TYPE_OPERATION.refresh, hooks);
        } catch (error) {
            if (error.message !== '명시 진단 중단') throw error;
            outcome = 'assertion';
        }
        const label = `행 ${count} ${row[0]}`;
        assert.equal(trace || '-', row[1], `${label} 호출`);
        assert.equal(hex(raw), row[2], `${label} raw`);
        assert.equal(checking >>> 0, number(row[3]), `${label} checking`);
        assert.equal(type >>> 0, number(row[4]), `${label} 전역`);
        assert.equal(outcome, row[5], `${label} 결과`);
        count += 1;
    }
    assert.equal(count, 432);
});

// 진단/자식은 선택될 때만 필수이며 물리 슬롯 오류는 역참조 전에 중단한다.
test('펜스 타입 부모: 선택된 경계와 DWORD 타입 폭', () => {
    const raw = new Uint8Array(50);
    let type = 136;
    let checking = 0;
    const kind = FENCE_TYPE_OPERATION.rebuild;
    runFenceTypeParent(raw, 0, () => type, () => checking, kind, {});
    checking = 1;
    assert.throws(() => runFenceTypeParent(raw, 0, () => type, () => checking, kind, {}), /진단 장치 미연결/);
    raw[10] = 136;
    assert.throws(() => runFenceTypeParent(raw, 0, () => type, () => checking, FENCE_TYPE_OPERATION.refresh, {}), /하위 미연결/);
    assert.throws(() => runFenceTypeParent(raw.subarray(0, 10), 0, () => type, () => checking, kind, {}), TypeError);
    // 슬롯 BYTE 는 zero-extend 하며 전역 DWORD 전체와 비교한다.
    type = 0xffffff88;
    checking = 0;
    let calls = 0;
    const hooks = { body: (sid) => { assert.equal(sid, 0); calls += 1; } };
    runFenceTypeParent(raw, 0, () => type, () => checking, FENCE_TYPE_OPERATION.refresh, hooks);
    assert.equal(calls, 0);
    type = 136;
    runFenceTypeParent(raw, 0, () => type, () => checking, FENCE_TYPE_OPERATION.refresh, hooks);
    assert.equal(calls, 1);
});

// 제품 전에 동결한 128입력으로 wrapper 의 호출 순서/현재 owner 재독/반환을 대조한다.
test('사제 희생 요청: 원본10.78 128행(호출 순서·현재 owner 재독·반환)', { skip: !priestRows && '원본 fixture 없음' }, () => {
    const pool = new SidPool(24000, { server: true });
    let count = 0;
    for (const row of priestRows) {
        assert.equal(row.length, 6);
        const values = row[0].split(',').map(number);
        assert.equal(values.length, 6);
        const [op, owner, , allowed, mutation] = values;
        let local = values[2];
        let kind = values[5];
        const raw = pool.physical(50);
        raw[34] = owner & 0xff;
        const trace = [];
        const request = new RawPriestSacrificeRequest(pool, () => kind, () => local, {
            allowed: (dais, priest) => { trace.push(`A:${dais}:${priest}`); return allowed !== 0; },
            remove: (priest, type, index, removal) => {
                trace.push(`R:${priest}:${type}:${index}:${removal}`);
                if (mutation === 1) raw[34] = 2;
                if (mutation === 2) kind = 77;
            },
            command: (dais, priest, current) => {
                trace.push(`C:${dais}:${priest}:${current}`);
                if (mutation === 3) raw[34] = 2;
                if (mutation === 4) local = 2;
            },
            music: (name) => trace.push(`M:${name}`),
        });
        // 시작은 반환값이 없으므로 원문 직렬화 표기인 0 을 쓴다.
        let result = false;
        if (op === 0) result = request.allowed(60, 50); else request.begin(50, 60);
        const label = `행 ${count} ${row[0]}`;
        assert.equal(trace.join(';'), row[1], `${label} 호출`);
        assert.equal(String(result ? 1 : 0), row[2], `${label} 반환`);
        assert.deepEqual([String(raw[34]), String(local), String(kind)], [row[3], row[4], row[5]], `${label} 현재 값`);
        count += 1;
    }
    assert.equal(count, 128);
});

// 헤더 길이 비보정/bit7 제거/송신 전후 동일 버퍼/현재 지면 상태를 두 정밀도의 원문과 대조한다.
test('캐리어 채취 통지: 원본10.78 864행(6 BYTE 작성·송신 전후 같은 버퍼·현재 지면)', { skip: !miningRows && '원본 fixture 없음' }, () => {
    const pool = new SidPool(24000);
    let count = 0;
    for (const row of miningRows) {
        assert.equal(row.length, 7);
        const c = row[0].split(',').map(number);
        assert.equal(c.length, 5);
        pool.physical(c[3])[11] = 0;
        const header = { message: c[1] & 0xff, protocol: c[0] & 0xff, length: c[2] & 0xffff };
        let events = '';
        let packet = '';
        const mining = new RawCarrierMining(pool, header, {
            broadcast: (bytes) => {
                events = `S:${hex(bytes)}`;
                assert.equal(bytes.length, 6);
                if (c[4] === 1) Object.assign(header, { message: 0x23, protocol: 0x81, length: 7 });
                if (c[4] === 2) put(bytes, 4, 61, 2);
                if (c[4] === 3) pool.physical(c[3])[11] = 1;
            },
            receive: (bytes) => { packet = hex(bytes); events += `;L:${packet}`; },
        });
        assert.equal(mining.pool, pool);
        mining.notify(c[3]);
        const current = `${header.protocol},${header.message},${header.length},${pool.slot(c[3])[11]}`;
        const label = `행 ${count} ${row[0]}`;
        assert.equal(events, row[1], `${label} 효과`);
        assert.equal(packet, row[2], `${label} 패킷`);
        assert.equal(current, row[3], `${label} 현재`);
        assert.deepEqual(row.slice(1, 4), row.slice(4, 7), `${label} 두 정밀도 동일`);
        count += 1;
    }
    assert.equal(count, 864);
});

// 송신 뒤의 미연결 수신 경계와 지면 SID 의 전체 WORD 폭을 확인한다.
test('캐리어 채취 통지: 늦은 경계와 판본', () => {
    const pool = new SidPool(24000);
    const header = { message: 1, protocol: 0x5b, length: 6 };
    let sends = 0;
    const mining = new RawCarrierMining(pool, header, { broadcast: (bytes) => { sends += 1; assert.deepEqual([bytes[4], bytes[5]], [0xff, 0xff]); } });
    assert.throws(() => mining.notify(0xffff), /실제 하위 미연결/);
    assert.equal(sends, 1);
    assert.throws(() => new RawCarrierMining({ edition: 'cd1072' }, header, {}), TypeError);
});
