// 제네레이터 Meltdown raw-meltdown.js(RawMeltdown)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: meltdown-x86.tsv 442행(두 정밀도 열 동일) — 입력 23 DWORD(연산 0 명령/1 자격/2 피해/3 요청 수신/4 효과 수신/5 패킷 확정·대상 TYPE genus·HP·프레임 플래그·상태·extra·발자국·좌표·반경·owner·동맹·권한·헤더·변이 0~3 …)
//    → 반환, 외부 효과 순서(hp/assert/find/send/broadcast/receive/destroy/damage/ground/edge/visual/sound/base 문자열), 대상 슬롯(60)과 220바이트 효과 패킷 전체.
//  - 계약: 실제 검색의 엄격한 거리/예약 SID/abstract/dead/한도와 같은 해시 순서.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E111: 제네레이터 Meltdown 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { RawMeltdown } from '../www/core/raw-meltdown.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E111)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'meltdown-x86.tsv'));
const hex = (bytes) => Buffer.from(bytes).toString('hex');
const bits = floatToBits;

// 명시 입력과 큰 외부 효과만 공급하는 장면이다. (2026-11-08 WB04-E111)
function makeScene() {
    const scene = {
        pool: new SidPool(24000),
        hash: new SquidHash(),
        types: [],
        frames: [],
        alliances: new Array(81).fill(0),
        state: null,
        packet: new Uint8Array(220),
        c: [],
        boss: 0,
        local: 513,
        visuals: 0,
        trace: []
    };
    const raw = (sid) => scene.pool.physical(sid);
    // 원문 효과 순서를 같은 DWORD 표기로 기록한다.
    const event = (name, args = []) => scene.trace.push(args.reduce((text, value) => `${text}:${value >>> 0}`, name));
    scene.event = event;
    // 패킷/음향의 원래 문자열이다.
    const text = (value) => scene.trace.push(value);
    // 명시 입력만 설치하며 판단/기대값을 계산하지 않는다.
    scene.prepare = (input) => {
        scene.c = input.split(',').map((value) => Number(BigInt.asUintN(32, BigInt(value))));
        const c = scene.c;
        assert.equal(c.length, 23);
        scene.trace = [];
        scene.visuals = 0;
        scene.boss = c[16];
        scene.local = 513;
        scene.types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
        scene.frames = Array.from({ length: 188 }, () => ({ codes: [] }));
        scene.alliances.fill(0);
        scene.state = {
            boss: () => scene.boss,
            localOwner: () => scene.local,
            alliances: scene.alliances,
            request: { message: 0x76, protocol: 1, length: 6 },
            effect: { message: 0x75, protocol: 1, length: 0 }
        };
        scene.state.request.protocol = scene.state.effect.protocol = c[20] & 0xff;
        scene.state.effect.length = c[21] & 0xffff;
        scene.alliances[c[13] * 9 + c[14]] = c[15];
        // 전체 입력 후보를 같은 바이트로 초기화한다.
        for (let sid = 50; sid < 160; sid += 1) {
            const slot = raw(sid);
            slot.fill(0);
            put(slot, 0, 0x12200000);
            slot[10] = sid === 50 ? 75 : 70;
            slot[11] = sid === 50 ? 0 : c[4];
            slot[34] = sid === 50 ? 2 : c[14];
            slot[40] = c[5];
            put(slot, 14, c[8]);
            put(slot, 18, c[9]);
        }
        if (c[0] === 3) raw(50)[11] = c[4];
        scene.types[75].flags2 = 0x200;
        scene.types[75].footX = scene.types[75].footY = 3;
        scene.types[70].flags2 = c[1];
        scene.types[70].footX = c[6] | 0;
        scene.types[70].footY = c[7] | 0;
        scene.frames[70] = { codes: [{ side: 74, variant: 0, number: 1, flags: c[3] & 0xff }] };
        scene.packet.fill(255);
        put(scene.packet, 0, 1, 1);
        put(scene.packet, 1, 0x75, 1);
        put(scene.packet, 2, 4, 2);
        put(scene.packet, 4, c[10]);
        put(scene.packet, 8, c[11]);
        put(scene.packet, 12, c[12], 1);
        put(scene.packet, 13, c[13], 1);
        put(scene.packet, 14, 50, 2);
        put(scene.packet, 16, c[17]);
        // 원문 WORD 목록이다.
        for (let index = 0; index < 100; index += 1) put(scene.packet, 20 + index * 2, 60 + index, 2);
        if (c[0] === 3) put(scene.packet, 4, 50, 2);
    };
    // 원문과 같은 명시 큰 효과만 기록한다.
    scene.hooks = () => ({
        hasHitPoints: (sid) => { event('hp', [sid]); return scene.c[2] !== 0; },
        assertion: (line) => event('assert', [line]),
        send: (target, bytes) => text(`send:${target}:${hex(bytes)}`),
        find: (x, y, radius, list) => {
            event('find', [bits(x), bits(y), radius]);
            assert.ok(list.capacity === 100 && list.count === 0);
            // 독립 검색 경계가 제공한 번호이다.
            for (let index = 0; index < scene.c[17]; index += 1) list.storage[index] = 15000 + index;
            list.count = scene.c[17] | 0;
            if (scene.c[18] === 1) { put(raw(50), 14, bits(80.25)); raw(50)[34] = 7; }
            if (scene.c[18] === 3) { scene.state.effect.protocol = 0x83; scene.state.effect.length = 6; }
        },
        broadcast: (bytes) => { text(`broadcast:${hex(bytes)}`); if (scene.c[18] === 2) { scene.local = 769; put(bytes, 14, 99, 2); } },
        receive: (owner, connection, bytes) => text(`receive:${owner}:${connection}:${hex(bytes)}`),
        destroy: (sid, flags) => event('destroy', [sid, flags]),
        damage: (sid, amount, owner, parent) => {
            event('damage', [sid, amount, owner, parent]);
            if (scene.c[18] === 1) put(raw(60), 14, bits(200));
            if (scene.c[18] === 2) raw(60)[11] = 2;
            if (scene.c[0] === 4 && scene.c[18] === 3) { put(scene.packet, 16, 1); put(scene.packet, 13, 7, 1); }
        },
        ground: (x, y) => event('ground', [bits(x), bits(y)]),
        edge: (sid, x, y) => event('edge', [sid, bits(x), bits(y)]),
        visual: (x, y, variant, interval) => {
            event('visual', [bits(x), bits(y), variant, bits(interval)]);
            scene.visuals += 1;
            if (scene.c[18] === 1 && scene.visuals === 1) { put(scene.packet, 12, 2, 1); put(scene.packet, 4, bits(100)); }
        },
        sound: (name) => { text(`sound:${name}:0:0:0:0:0`); if (scene.c[18] === 2) put(scene.packet, 16, 1); }
    });
    scene.raw = raw;
    return scene;
}

test('Meltdown: 원본 10.78 442입력의 명령/자격/피해/요청 수신/효과 수신/패킷 확정 반환·효과 순서·대상 슬롯·220바이트 패킷이 두 정밀도 열과 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = makeScene();
    let count = 0;
    // 기존 기대값을 재계산하거나 바꾸지 않는다.
    for (const row of rows) {
        assert.equal(row.length, 9);
        scene.prepare(row[0]);
        const body = new RawMeltdown(scene.pool, scene.hash, scene.types, scene.frames, scene.state, scene.hooks());
        let result = 0;
        const c = scene.c;
        const op = c[0];
        if (op === 0) body.command(50, c[19], (sid, code) => scene.event('base', [sid, code]));
        else if (op === 1) result = body.mayTarget(60) ? 1 : 0;
        else if (op === 2) body.apply(bitsToFloat(c[10]), bitsToFloat(c[11]), c[12] | 0, c[13], 60, 50);
        else if (op === 3) body.receiveRequest(scene.packet);
        else if (op === 4) body.receiveEffect(scene.packet);
        else RawMeltdown.finalizePacket(scene.packet, c[22] !== 0);
        const actual = [String(result), scene.trace.length === 0 ? '-' : scene.trace.join(';'), hex(scene.pool.slot(60)), hex(scene.packet)];
        // 기대값 두 정밀도와 제품 전체 출력을 비교한다.
        for (let field = 0; field < actual.length; field += 1) {
            assert.equal(actual[field], row[1 + field], `Meltdown 입력 ${row[0]} 필드 ${field}`);
            assert.equal(row[1 + field], row[5 + field]);
        }
        count += 1;
    }
    assert.equal(count, 442);
});

test('Meltdown: 실제 검색의 엄격한 거리/예약 SID/abstract/dead/한도와 같은 해시 순서를 지킨다', () => {
    const scene = makeScene();
    scene.prepare('0,512,1,0,0,0,3,3,1112014848,1112014848,1111621632,1111621632,4,2,1,0,1,3,0,50331896,1,0,1');
    // 예약/서버 경계와 거리의 명시 입력이다.
    const ids = [1, 4, 5, 14999, 15000, 15001, 15002, 15003];
    scene.types[70].footX = scene.types[70].footY = 2;
    // 거리 5 의 정확한 경계와 안쪽 입력이다.
    const x = [50, 50, 50, 50, 53, Math.fround(52.999996), 50, 50];
    const y = [50, 50, 50, 50, 54, 54, 50, 50];
    // 실제 해시에 정해진 후보만 등록한다.
    for (let index = 0; index < ids.length; index += 1) {
        const slot = scene.raw(ids[index]);
        slot.fill(0);
        slot[10] = 70;
        put(slot, 14, bits(Math.fround(x[index] + 1)));
        put(slot, 18, bits(Math.fround(y[index] + 1)));
        if (index === 6) slot[40] = 1;
        if (index === 7) slot[11] = 2;
        const head = scene.hash.bucket(1, Math.fround(x[index] + 1), Math.fround(y[index] + 1));
        put(slot, 4, head, 2);
        scene.hash.setBucket(1, Math.fround(x[index] + 1), Math.fround(y[index] + 1), ids[index]);
    }
    const body = new RawMeltdown(scene.pool, scene.hash, scene.types, scene.frames, scene.state, scene.hooks());
    const storage = new Array(100).fill(0);
    const result = { storage, capacity: 100, count: 0 };
    body.find(50, 50, 4, result);
    assert.deepEqual(storage.slice(0, result.count).sort((a, b) => a - b), [1, 4, 15001, 15003]);
    result.capacity = 1;
    result.count = 0;
    body.find(50, 50, 4, result);
    assert.equal(result.count, 1);
});
