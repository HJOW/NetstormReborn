// Factory 연구 raw-factory-research.js(RawFactoryResearch·writeFactoryResearchCommand)를 원본 10.78 관측 216입력과 cpppj `FactoryResearchTests` 계약으로 대조한다.
//  - 원본 관측 factory-research-x86.tsv 216행(Learn 32·Input 72·Receive 16·Notice 96): 입력(검색 결과·남은 슬롯·attach flags·kind·수정키·sender·부모 상태/TYPE flags2·권한·내용물 flags/reason/loading)에서
//    반환·productionDirty 카운터·전체 호출 순서(find·slots·destroy·create·send 16바이트·help 메시지 원문 hex·flare·refresh·assert 메시지 원문 hex)를 대조한다. 게임 하위는 명시 경계로 둔다.
//  - 계약: 실제 VT 판정(Factory/아웃포스트/그 밖)·짧은 패킷/미공급 업그레이드 수신의 거부·플레어 누락 시 dirty 불변·입력 로그가 슬롯과 시각을 바꿔도 처음 양수 판단 유지.
//  - 패킷 0x58 분배기 등록(`RegisterFactoryResearchCommand`)과 그 시험(FactoryResearch_PacketDispatchAndMissingEffects 의 분배 부분)은 RawPacketDispatch 이식 뒤로 미룬다(PORT_MAP 메모).
// 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E217: Factory 연구 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawFactoryResearch, writeFactoryResearchCommand } from '../www/core/raw-factory-research.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-10 WB04-E217)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'factory-research-x86.tsv'));

// 예외가 나는지 본다. (2026-11-10 WB04-E217)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-10 WB04-E217)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 문자열의 원본 바이트를 관측 TSV 와 같은 hex 로 나타낸다. (2026-11-10 WB04-E217)
const textHex = (text) => Buffer.from(text, 'utf8').toString('hex');

// 원본 실행기의 검색/슬롯/생성/송신 경계에 공급한 명시 입력이다(cpppj ResearchScene). (2026-11-10 WB04-E217)
class ResearchScene {
    constructor() {
        this.pool = new SidPool(24000, { server: true });
        this.types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
        this.loading = 0;
        this.dirty = { value: 3 };
        this.outpost = 126;
        this.boss = true;
        this.now = 12.5;
        this.found = 0;
        this.slots = 2;
        this.trace = '';
        const parent = this.pool.physical(50);
        parent.fill(0);
        put(parent, 0, 0x509148);
        parent[10] = 85;
        parent[34] = 1;
        const form = this.pool.physical(100);
        form.fill(0);
        put(form, 0, 0x512208);
        put(form, 18, 128, 2);
        put(form, 20, 16, 2);
        this.types[85].flags2 = 0x4000;
        this.hooks = {
            find: (parentSid, mask, _kind, index) => { assert.ok(parentSid === 50 && mask === 16 && index === 0); this.add(`find,${this.found}`); return this.found; },
            slots: (parentSid) => { assert.equal(parentSid, 50); this.add(`slots,${this.slots}`); return this.slots; },
            destroy: (sid, flags) => { this.add(`destroy,${sid},${flags}`); this.found = 0; },
            create: (parentSid, kind, quantity, flags, attach, quality) => { this.add(`create,${parentSid},${kind},${quantity},${flags},${attach},${quality}`); return 20000; },
            send: (channel, packet) => this.add(`send,${channel},${hex(packet)}`),
            help: (text, flags) => this.add(`help,${textHex(text)},${flags}`),
            flare: (parentSid, count, event) => this.add(`flare,${parentSid},${count},${event}`),
            refresh: (owner) => this.add(`refresh,${owner}`),
            assertion: (text) => this.add(`assert,${textHex(text)}`),
        };
    }

    // 호출 목록 이어 붙이기이다(빈 목록은 '-' 로 바꿀 때만 처리). (2026-11-10 WB04-E217)
    add(value) { this.trace = this.trace === '' ? value : `${this.trace};${value}`; }

    // 같은 현재 상태 참조를 매 새 몸체에 연결한다. (2026-11-10 WB04-E217)
    state() {
        return { loadingDepth: () => this.loading, productionDirty: this.dirty, boss: () => this.boss, now: () => this.now, outpostType: () => this.outpost };
    }

    // 현재 장면의 연구 처리기이다. (2026-11-10 WB04-E217)
    research() { return new RawFactoryResearch(this.pool, this.types, this.state(), this.hooks); }
}

test('Factory 연구: 원본 10.78 216입력의 반환·dirty·호출 순서와 16바이트 송신이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const counts = { Learn: 0, Input: 0, Receive: 0, Notice: 0 };
    for (const row of rows) {
        const scene = new ResearchScene();
        let result = 0;
        if (row[0] === 'Learn') {
            scene.found = Number(row[1]);
            scene.slots = Number(row[2]) | 0;
            scene.research().learn(50, 128, Number(row[3]));
        } else if (row[0] === 'Input') {
            scene.found = Number(row[2]);
            scene.slots = Number(row[3]) | 0;
            result = scene.research().input(50, (0x03000080 | Number(row[4])) >>> 0, (((Number(row[1]) << 16) | 50) >>> 0));
        } else if (row[0] === 'Receive') {
            const parent = scene.pool.physical(50);
            parent[11] = Number(row[2]) & 0xff;
            parent[10] = Number(row[4]) & 0xff;
            scene.types[parent[10]].flags2 = Number(row[3]);
            scene.boss = Number(row[5]) !== 0;
            const packet = new Uint8Array(16);
            writeFactoryResearchCommand(50, Number(row[6]), scene.now, packet);
            packet[7] = Number(row[7]) & 0xff;
            scene.research().receive(Number(row[1]), packet);
        } else {
            assert.equal(row[0], 'Notice');
            const form = Number(row[1]);
            put(scene.pool.physical(form), 20, Number(row[3]), 2);
            scene.loading = Number(row[4]);
            scene.found = Number(row[5]);
            scene.research().notice(50, form, Number(row[2]));
        }
        counts[row[0]] += 1;
        const trace = scene.trace === '' ? '-' : scene.trace;
        const label = `${row[0]} #${counts[row[0]]}`;
        assert.equal(result, Number(row[row.length - 3]), `${label} 반환`);
        assert.equal(scene.dirty.value, Number(row[row.length - 2]), `${label} dirty`);
        assert.equal(trace, row[row.length - 1], `${label} 호출 순서`);
    }
    assert.deepEqual(counts, { Learn: 32, Input: 72, Receive: 16, Notice: 96 });
});

test('Factory 연구 계약: 패킷 기록은 16바이트만 쓰고 짧은 저장소/수신 본문은 거부하며 실제 VT 로 통지 대상을 고른다', () => {
    const scene = new ResearchScene();
    const research = scene.research();
    const packet = new Uint8Array(18).fill(0xa5);
    assert.equal(writeFactoryResearchCommand(50, 128, 12.5, packet), 16);
    assert.ok(packet[16] === 0xa5 && packet[17] === 0xa5);
    assert.deepEqual(Array.from(packet.slice(0, 8)), [1, 0x58, 16, 0, 50, 0, 128, 0]);
    // 종류는 최종 BYTE 만 쓴다.
    writeFactoryResearchCommand(50, 0x1234, 12.5, packet);
    assert.equal(packet[6], 0x34);
    // 업그레이드 mode(kind 0, mode 1) 수신은 미공급 하위를 가짜 성공으로 만들지 않는다.
    packet[2] = 16;
    packet[6] = 0;
    packet[7] = 1;
    assert.ok(throws(() => research.receive(1, packet)));
    assert.ok(throws(() => writeFactoryResearchCommand(50, 128, 12.5, packet.subarray(0, 15))));
    assert.ok(throws(() => research.receive(1, packet.subarray(0, 7))));
    // 실제 VT 로만 통지 대상을 고른다.
    assert.ok(research.handlesNotice(50));
    put(scene.pool.physical(50), 0, 0x509210);
    assert.ok(research.handlesNotice(50));
    put(scene.pool.physical(50), 0, 0x5096a0);
    assert.ok(!research.handlesNotice(50));
    // 플레어 경계가 없으면 변경 전에 거부하며 dirty 는 그대로이다.
    delete scene.hooks.flare;
    assert.ok(throws(() => scene.research().notice(50, 100, 1)));
    assert.equal(scene.dirty.value, 3);
    // 판본/타입 표 크기.
    assert.ok(throws(() => new RawFactoryResearch(scene.pool, scene.types.slice(0, 171), scene.state(), scene.hooks)));
    assert.ok(throws(() => scene.research().input(50, 0x03000081, 128 << 16)));
});

test('Factory 연구 계약: 입력 로그가 슬롯과 시각을 바꿔도 처음 양수 판단을 유지하고 패킷은 로그 뒤 현재 시각을 쓴다', () => {
    const scene = new ResearchScene();
    let sent = null;
    scene.hooks.inputLog = (parent, slots) => {
        assert.ok(parent === 50 && slots === 2);
        scene.add('inputLog');
        scene.now = 33.75;
        scene.slots = 0;
    };
    scene.hooks.send = (channel, packet) => { assert.equal(channel, 9); scene.add('send'); sent = Uint8Array.from(packet); };
    assert.equal(scene.research().input(50, 0x03000080, ((128 << 16) | 50) >>> 0), 1);
    assert.equal(scene.trace, 'slots,2;inputLog;find,0;slots,0;send');
    assert.equal(sent.length, 16);
    const view = new DataView(sent.buffer);
    assert.equal(view.getFloat64(8, true), 33.75);
    assert.equal(sent[6], 128);
});
