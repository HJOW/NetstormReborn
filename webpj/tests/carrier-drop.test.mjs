// 골렘 Cast/Drop raw-carrier-drop.js(RawCarrierDrop)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: carrier-drop-x86.tsv 413행(두 x87 정밀도 동일) — 평탄한 정수 JSON 입력(요청 op 0/메뉴 op 1/서버 수신 op 2·소유자·서버·checking·TYPE 수·헤더·내용물 종류/플래그/표시·연결 체인 길이·genus·비용·돈·승인·정지·생성·외부 변이 종류 등)
//    → 외부 효과 순서(immobile/afford/subtract/project/popup/regular/attached/process·notify/select/send/pickup/base-menu/assert/point/queue/bad-owner/money/refund/movement/remove-path/detach/unpop/create/owner/pop/local-send/broadcast/place/destroy/standing/escape),
//    송신 패킷 hex, 일곱 슬롯(50·60·61·62·70·80·81) 전체 raw.
//  - 계약: CarrierDrop_LazyBoundariesAndPacketBounds(거부 관문에서는 도달하지 않은 경계를 요구하지 않고 짧은 패킷은 사전에 거부).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E93: 골렘 Cast/Drop 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { RawCarrierDrop } from '../www/core/raw-carrier-drop.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E93)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'carrier-drop-x86.tsv'));
const ROWS = 413;
const SLOTS = Object.freeze([50, 60, 61, 62, 70, 80, 81]);

// 예외가 나는지 본다. (2026-11-08 WB04-E93)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E93)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 원문과 같은 외부 경계 입력만 공급하며 요청/수신 분기는 제품 몸체가 실행하는 장면이다. (2026-11-08 WB04-E93)
class DropScene {
    constructor() {
        this.pool = new SidPool(24000);
        this.types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));
        this.contained = new ContainedFinderState();
        this.state = this.freshState();
        this.input = {};
        this.costs = new Map();
        this.trace = [];
        this.packet = '-';
        this.receivePacket = null;
    }

    // CarrierDropState 의 기본값이다.
    freshState() {
        return {
            request: { message: 0x50, protocol: 1, length: 9 }, activation: { message: 0x5c, protocol: 1, length: 6 },
            localOwner: 1, server: 1, checking: 0, numTypes: 188, contentType: 6, notifyProcessType: 51, queueCommand: 0x030000b2,
            selectCommand: 0x03000073, castCommand: 0x0300011a, dropCommand: 0x03000076, pickupCommand: 0x03000075, mapLimit: 256
        };
    }

    // 검증 입력을 물리 슬롯에만 설치한다.
    raw(sid) { return this.pool.physical(sid); }

    // 관측된 인자들을 원문 형식으로 직렬화한다(각 인자의 실제 DWORD 비트).
    event(name, values = []) { this.trace.push(values.reduce((text, value) => `${text}:${value >>> 0}`, name)); }

    // 실제 패킷 바이트만 원문 효과 뒤에 붙인다.
    blob(name, target, bytes, includeTarget) { this.trace.push(`${name}${includeTarget ? `:${target >>> 0}` : ''}:${hex(bytes)}`); }

    // 원본 입력 raw/전역만 준비하고 예상 반환/효과는 공급하지 않는다.
    prepare(text) {
        const input = JSON.parse(text);
        this.input = input;
        this.trace = [];
        this.packet = '-';
        this.costs = new Map();
        this.state = this.freshState();
        this.state.localOwner = input.local;
        this.state.server = input.server;
        this.state.checking = input.checking;
        this.state.numTypes = input.count;
        this.state.request = { message: input.message & 0xff, protocol: input.protocol & 0xff, length: input.length & 0xffff };
        // 원문과 같은 7개 슬롯의 미사용 바이트도 매 입력 초기화한다.
        for (const sid of SLOTS) {
            const raw = this.raw(sid);
            raw.fill(0);
            put(raw, 0, 0x12200000);
            raw[10] = sid === 50 ? 133 : (sid >= 60 && sid <= 62) ? input.contentType : sid === 80 ? 51 : 52;
            raw[11] = sid === 50 ? input.parentFlags : sid === 60 ? input.contentFlags : 0;
            raw[34] = input.owner;
            raw[40] = input.extra;
            put(raw, 14, input.x);
            put(raw, 18, input.y);
            if (sid >= 60 && sid <= 62) {
                put(raw, 18, sid === 60 ? input.kind : 169, 2);
                raw[22] = input.flag;
                raw[20] = sid === 60 ? input.mark : 1;
            }
        }
        this.raw(51).set(this.raw(50));
        const count = input.chain;
        put(this.raw(50), 6, count ? 60 : 0, 2);
        // 현재 raw 의 next 를 실제 제품 finder 가 따라가도록 구성한다.
        for (let index = 0; index < count; index += 1) put(this.raw(60 + index), 4, index + 1 < count ? 61 + index : 0, 2);
        // 원문의 TYPE/비용 입력을 현재 값으로 설치한다.
        for (const type of this.types) type.flags2 = 0;
        this.types[133].flags2 = input.carrierGenus;
        this.types[168].flags2 = input.genus;
        // 동일한 기본 비용 네 항목은 외부 효과가 나중에 바꿀 수 있다.
        for (const kind of [0, 168, 169, 188]) this.costs.set(kind, input.cost | 0);
    }

    // 원본에 선언된 큰 외부 효과만 동일 입력과 인자로 재생한다.
    hooks() {
        const input = () => this.input;
        return {
            immobile: (parent) => { this.event('immobile', [parent]); return input().immobile !== 0; },
            cost: (type) => { if (!this.costs.has(type)) throw new Error('비용 키 없음'); return this.costs.get(type); },
            afford: (value) => { this.event('afford', [floatToBits(value)]); if (input().mutation === 1) this.costs.set(input().kind, 77); return input().afford !== 0; },
            subtract: (owner, value) => { this.event('subtract', [owner, floatToBits(value)]); if (input().mutation === 2) put(this.raw(60), 18, 169, 2); },
            project: (point) => { this.event('project', [floatToBits(point[0]), floatToBits(point[1])]); return [321, 234]; },
            popup: (cost, point, flags) => { this.event('popup', [cost, point[0], point[1], flags]); },
            regular: (parent, delay) => { this.event('regular', [parent, floatToBits(delay)]); },
            attached: (parent, visitor) => { this.event('attached', [parent]); if (input().attached) { visitor(80); visitor(81); } },
            notify: (process, message, first, second) => { this.event('process', [process]); this.event('notify', [message, first, second]); },
            select: (sid, force, active) => { this.event('select', [sid, force, active]); },
            send: (target, bytes) => { this.packet = hex(bytes); this.blob('send', target, bytes, true); },
            pickup: (parent) => { this.event('pickup', [parent]); },
            baseMenu: (parent) => { this.event('base-menu', [parent]); return 7; },
            assertion: (line) => { this.event('assert', [line]); },
            dropPoint: (parent, kind) => { this.event('point', [parent, kind]); return input().point ? [bitsToFloat(input().x), bitsToFloat(input().y)] : [-10000, -10000]; },
            queue: (command, kind, zero) => {
                this.event('queue', [command, kind, zero]);
                if (input().mutation === 3) this.raw(50)[34] = 2;
                if (input().mutation === 4) put(this.receivePacket, 4, 51, 2);
            },
            badOwner: (kind, sender) => { this.event('bad-owner', [kind, sender]); },
            money: (owner) => { this.event('money', [owner]); return bitsToFloat(input().money); },
            refund: (kind, owner, point, argument, flags) => { this.event('refund', [kind, owner, floatToBits(point[0]), floatToBits(point[1]), argument, flags]); },
            movement: (type) => { this.event('movement', [type]); return 10; },
            removeMovement: (parent, type, first, second) => { this.event('remove-path', [parent, type, first, second]); if (input().mutation === 5) this.raw(50)[34] = 2; },
            detach: (parent, flags) => { this.event('detach', [parent, flags]); },
            unpop: (parent, flags) => { this.event('unpop', [parent, flags]); },
            create: (kind, flags) => {
                this.event('create', [kind, flags]);
                this.raw(70)[10] = kind & 0xff;
                if (input().mutation === 6) put(this.receivePacket, 4, 51, 2);
                return 70;
            },
            owner: (sid, owner) => { this.event('owner', [sid, owner]); this.raw(sid)[34] = owner & 0xff; },
            argument: (sid, argument) => { put(this.raw(sid), 12, argument, 2); },
            pop: (sid, point, flags) => { this.event('pop', [sid, floatToBits(point[0]), floatToBits(point[1]), flags]); },
            activate: (owner, flags, bytes) => { assert.equal(flags, 0); this.blob('local-send', owner, bytes, true); },
            broadcast: (bytes) => { this.blob('broadcast', 0, bytes, false); },
            place: (kind, point, owner, argument, first, second, amount, flags) => {
                const view = new DataView(new ArrayBuffer(8));
                view.setFloat64(0, amount, true);
                this.event('place', [kind, floatToBits(point[0]), floatToBits(point[1]), owner, argument, first, second, view.getUint32(0, true), view.getUint32(4, true), flags]);
                return input().create !== 0;
            },
            destroy: (content, flags) => { this.event('destroy', [content, flags, this.raw(content)[22]]); },
            standing: (parent) => { this.event('standing', [parent]); return input().standing !== 0; },
            escape: (parent, delay, direction) => { this.event('escape', [parent, floatToBits(delay), direction]); }
        };
    }

    // 원문과 같은 7슬롯의 전체 바이트를 직렬화한다.
    raws() { return SLOTS.map((sid) => hex(this.raw(sid))).join(','); }

    // 현재 장면으로 모듈을 만든다.
    drop(hooks = this.hooks()) { return new RawCarrierDrop(this.pool, this.types, this.contained, this.state, hooks); }
}

test('CarrierDrop: 원본 10.78 413입력의 Cast/Drop 요청·메뉴·서버 수신 효과 순서·송신 패킷·일곱 슬롯 raw 가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new DropScene();
    let count = 0;
    // 각 입력은 독립 관측의 기대값을 읽기만 한다.
    for (const row of rows) {
        assert.equal(row.length, 7);
        scene.prepare(row[0]);
        const drop = scene.drop();
        const op = scene.input.op;
        if (op === 0) drop.request(50, scene.input.chosen);
        else if (op === 1) assert.equal(drop.command(50, 0x0300011a, 50), 1);
        else {
            const packet = Uint8Array.of(1, 0x50, 9, 0, 50, 0, 0, 0, 0);
            put(packet, 6, scene.input.chosen, 2);
            packet[8] = scene.input.flag & 0xff;
            scene.receivePacket = packet;
            drop.receive(scene.input.sender, packet);
        }
        const label = `입력 ${row[0]}`;
        assert.equal(scene.trace.length === 0 ? '-' : scene.trace.join(';'), row[1], label);
        assert.equal(scene.packet, row[2], label);
        assert.equal(scene.raws(), row[3], label);
        assert.ok(row[1] === row[4] && row[2] === row[5] && row[3] === row[6], label);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('CarrierDrop: 거부 관문에서는 도달하지 않은 경계를 요구하지 않고 짧은 패킷을 사전에 거부한다', () => {
    const scene = new DropScene();
    const drop = scene.drop({});
    assert.equal(drop.pool, scene.pool);
    assert.ok(throws(() => drop.receive(1, new Uint8Array(8))));
    const dead = new Uint8Array(9);
    put(dead, 4, 50, 2);
    scene.raw(50)[11] = 2;
    drop.receive(1, dead);
    scene.raw(50)[11] = 0;
    assert.ok(throws(() => drop.command(50, 0x0300011a, 50)));
    assert.ok(throws(() => new RawCarrierDrop({ edition: 'cd1072' }, scene.types, scene.contained, scene.state, {})));
    assert.ok(throws(() => new RawCarrierDrop(scene.pool, scene.types.slice(1), scene.contained, scene.state, {})));
});
