// 애시드 포획 명령 작성·수신 raw-acid-zap.js(RawAcidZap)를 독립 원본 관측 acid-zap-x86.tsv 1017행과 대조한다.
// 행 종류: 헤더 작성 90행(현재 프로토콜 상위비트·메시지·보정하지 않는 선언 길이와 모든 페이로드)과 수신 927행(free/dead·두 제외 TYPE/사제·입력 무시·효과 뒤 현재 owner/가상 표).
// 입력은 원본과 같은 6개 raw 슬롯(대상 50·51, 폴 60·61·62·63)이며 소리/분해/삭제 효과는 원본 기록과 같은 문자열로 남긴다. 콜백이 바꾸는 현재 슬롯/패킷만 수행하고 기대값은 계산하지 않는다.
// (2026-11-08 WB04-E48: RawAcidZap 이식과 함께 추가)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { RawAcidZap } from '../www/core/raw-acid-zap.js';
import { SidPool } from '../www/core/sid-pool.js';
import { REPO_ROOT } from './helpers.mjs';
import { floatToBits, put, readRows } from './raw-support.mjs';

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E48)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');

// 입력의 signed/unsigned DWORD 를 원본 하위 비트로 읽는다. 음수 방향도 비트로 보존한다. (2026-11-08 WB04-E48)
const numbers = (text) => text.split(',').map((word) => Number(BigInt.asUintN(32, BigInt(word))));

// 원본 다섯 몸체의 고유 입력을 코드가 만드는 기대값과 섞지 않고 한 쪽(헤더/수신)을 재생한다. 비교한 행 수를 돌려준다. (2026-11-08 WB04-E48)
function replay(headers) {
    const pool = new SidPool(32768, { server: false });
    // 같은 여섯 raw 슬롯을 확보한다.
    for (let value = 5; value <= 63; value += 1) assert.equal(pool.allocate(2), value);
    // free 상태도 포함한 독립 원문 입력을 같은 예약 슬롯에 다시 설치한다.
    const writable = (sid) => pool.physical(sid);
    let count = 0;
    const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'acid-zap-x86.tsv'));
    // 1017행의 최초 원문이다.
    rows.forEach((row, line) => {
        const c = numbers(row[0]);
        assert.equal(row.length, 4);
        assert.equal(c.length, 15);
        if ((c[0] !== 0) !== headers) return;
        // 현재 대상 두 개와 폴 네 개를 같은 원본 입력으로 설치한다.
        for (const sid of [50, 51, 60, 61, 62, 63]) {
            const raw = writable(sid);
            raw.fill(0);
            put(raw, 0, 0x12200000, 4);
            raw[10] = sid < 60 ? c[3] : 145;
            raw[11] = sid === 50 ? c[1] : sid === 60 ? c[2] : 0;
            raw[34] = sid === 60 ? c[7] : 3;
            put(raw, 14, floatToBits(20.75), 4);
            put(raw, 18, floatToBits(21.5), 4);
        }
        const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 1, footY: 1 }));
        types[c[3]].flags2 = c[4];
        const state = { altar: c[5], growingIceTower: c[6] };
        const header = { message: c[13] & 0xff, protocol: c[12] & 0xff, length: c[14] & 0xffff };
        let packet = new Uint8Array(11);
        packet[0] = c[12] & 0x7f;
        packet[1] = c[13] & 0xff;
        put(packet, 2, c[14], 2);
        put(packet, 4, 50, 2);
        put(packet, 6, 60, 2);
        put(packet, 8, 62, 2);
        packet[10] = c[8] & 0xff;
        let effects = '';
        // 원문 요청 인수를 DWORD 로 출력한다.
        const event = (kind, values) => {
            if (effects) effects += ';';
            effects += kind + values.map((value) => `:${value}`).join('');
        };
        // 콜백이 공급한 현재 raw/패킷 변경만 수행한다.
        const mutate = (sound) => {
            if (sound) {
                if (c[11] === 1) writable(60)[34] = 7;
                if (c[11] === 2) put(packet, 6, 61, 2);
                if (c[11] === 3) put(packet, 4, 51, 2);
                if (c[11] === 4) { put(packet, 8, 63, 2); packet[10] = 255; }
                if (c[11] === 5) put(writable(50), 0, 0x12200300, 4);
            } else {
                if (c[11] === 6) writable(60)[34] = 9;
                if (c[11] === 7) put(writable(50), 0, 0x12200300, 4);
                if (c[11] === 8) writable(50)[11] = 3;
                if (c[11] === 9) put(packet, 6, 61, 2);
                if (c[11] === 10) { writable(60)[34] = 0; put(writable(50), 0, 0x12200300, 4); }
            }
        };
        const hooks = {
            sound: (x, y, name, loop) => {
                event('S', [floatToBits(x), floatToBits(y)]);
                effects += `:${name}:${loop}`;
                mutate(true);
            },
            dissolve: (victim, first, second, orientation) => {
                event('F', [victim, first, second, orientation >>> 0]);
                mutate(false);
            },
            destroy: (victim, flags) => {
                const vtable = pool.slot(victim);
                const original = (vtable[0] | (vtable[1] << 8) | (vtable[2] << 16) | (vtable[3] << 24)) >>> 0 === 0x12200000;
                event(original ? 'D' : 'd', [victim, flags]);
            },
        };
        const zap = new RawAcidZap(pool, types, state, header, hooks);
        if (headers) packet = zap.build(50, 60, 62, (c[8] << 24) >> 24);
        else zap.receive(c[9], c[10], packet);
        // 실제 콜백 이후의 여섯 슬롯이다.
        const raws = [50, 51, 60, 61, 62, 63].map((sid) => hex(pool.slot(sid))).join(',');
        assert.equal(effects || '-', row[1], `애시드 원문 행 ${line} 효과`);
        assert.equal(raws, row[2], `애시드 원문 행 ${line} 슬롯`);
        assert.equal(hex(packet), row[3], `애시드 원문 행 ${line} 패킷`);
        count += 1;
    });
    return count;
}

test('acid-zap: free/dead·두 제외 TYPE/사제·입력 무시·효과 뒤 현재 owner/가상 표를 원본 927행과 대조한다', () => {
    assert.equal(replay(false), 927);
});

test('acid-zap: 현재 프로토콜 상위 비트/메시지/보정하지 않는 선언 길이와 모든 페이로드를 원본 90행과 대조한다', () => {
    assert.equal(replay(true), 90);
});

test('acid-zap: 구성 오류와 짧은 패킷은 효과 전에 거부한다', () => {
    const pool = new SidPool(32768, { server: false });
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 1, footY: 1 }));
    const hooks = { sound() {}, dissolve() {}, destroy() {} };
    assert.throws(() => new RawAcidZap(pool, types.slice(1), { altar: 164, growingIceTower: 150 }, { message: 0, protocol: 1, length: 11 }, hooks), TypeError);
    assert.throws(() => new RawAcidZap(pool, types, { altar: 164, growingIceTower: 150 }, { message: 0, protocol: 1, length: 11 }, { sound() {}, dissolve() {} }), TypeError);
    const zap = new RawAcidZap(pool, types, { altar: 164, growingIceTower: 150 }, { message: 7, protocol: 0x81, length: 3 }, hooks);
    assert.throws(() => zap.receive(0, 0, new Uint8Array(10)), RangeError);
    // 선언 길이는 보정하지 않고 프로토콜 상위 비트는 지운다.
    assert.deepEqual(Array.from(zap.build(0x0102, 0x0304, 0x0506, -2)), [0x01, 7, 3, 0, 0x02, 0x01, 0x04, 0x03, 0x06, 0x05, 0xfe]);
});
