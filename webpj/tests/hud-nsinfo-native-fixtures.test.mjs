// NSInfo(준비 창, 0xd8 바이트 파생)의 일반 미션 부착(HudNsInfo, 원본 0047e9d0)의 원본 10.78 독립 관측 310행 대조: 8개 노드 0x11c 바이트(읽지 않는 확장 영역 포함)·24개 현재 전역·부착 인자와
// 기본 부착/Push/배경 부착/메타맵 배치 호출 순서·부분 중단 8행을 cpppj `HudNsInfoTests` 와 같은 입력 구성으로 비교한다. 메타맵 본문의 완료 검증으로 확장하지 않는다.
// 앞 6열(입력)만 장치에 공급하고 뒤 8열(관측)은 비교에만 쓴다. 원본 게임을 실행하지 않는다. (2026-11-15 WB04-E488)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { GumpCreateNode } from '../www/ui/gump-create.js';
import { HudNsInfo, HudNsInfoNode } from '../www/ui/hud-ns-info.js';
import { decodeGumpNode, encodeGumpNode } from './gump-node-support.mjs';

// 격리 원문 포인터(노드 주소·간격)와 노드 수·노드 원본 바이트 폭이다.
const NODE_ADDRESS = 0x01000100; const NODE_STRIDE = 0x200; const NODE_COUNT = 8; const RAW_SIZE = 0x11c;
// 명시 callback 중단을 정상 복귀와 구별하는 예외이다.
class NsExplicitStop extends Error {}
// float32 와 비트 사이의 공용 보기이다.
const f32 = new Float32Array(1); const u32 = new Uint32Array(f32.buffer);
const bitsOf = (value) => { f32[0] = value; return u32[0]; }; const floatOf = (bits) => { u32[0] = bits >>> 0; return f32[0]; };
const nsNumber = (text) => Number(BigInt.asUintN(32, BigInt(text)));
// 원문 주소와 1 기반 안정 토큰 사이의 변환이다(0 은 0).
function nsToken(address) {
    if (!address) return 0;
    if (address < NODE_ADDRESS || (address - NODE_ADDRESS) % NODE_STRIDE || (address - NODE_ADDRESS) / NODE_STRIDE >= NODE_COUNT) throw new Error('NSInfo 원문 주소');
    return (address - NODE_ADDRESS) / NODE_STRIDE + 1;
}
const nsAddress = (token) => (token ? NODE_ADDRESS + (token - 1) * NODE_STRIDE : 0);
// 원문 입력 필드 이름과 raw offset 이다(입력 효과에서 허용한 필드만).
const EFFECT_OFFSETS = { flags: 4, baseWidth: 0x3c, baseHeight: 0x40, scale: 0x38, background: 0xa0 };

// 앞 6열만 읽어 원문 실행과 같은 장치/현재 전역 입력을 만든다.
class NsScene {
    constructor(row) {
        const bytes = Uint8Array.from(Buffer.from(row[2], 'hex')); assert.equal(bytes.length, NODE_COUNT * RAW_SIZE);
        this.ns = new HudNsInfoNode(); this.bases = Array.from({ length: NODE_COUNT - 1 }, () => new GumpCreateNode()); this.preserved = new Array(NODE_COUNT); this.calls = new Map(); this.trace = '';
        for (let i = 0; i < NODE_COUNT; i += 1) this.decode(i, bytes.subarray(i * RAW_SIZE, (i + 1) * RAW_SIZE));
        const fields = row[3].split(':'); assert.equal(fields.length, 24); this.globals = fields.map(nsNumber);
        this.effects = row[5] === '-' ? [] : row[5].split(';').map((field) => field.split('/'));
        this.handler = new HudNsInfo({ metamap: () => this.globals[22], playerKnown: () => this.globals[23] }, this.hooks());
    }
    // 토큰 1 은 NSInfo 자신(상속 주소 공유), 그 밖은 나머지 노드의 안정된 기본 주소이다.
    base(token) { if (!token || token > NODE_COUNT) throw new Error('NSInfo 검사 토큰'); return token === 1 ? this.ns : this.bases[token - 2]; }
    // 원문 전체 입력을 같은 기본 필드와 현재 배경 포인터로 읽는다.
    decode(index, bytes) {
        this.preserved[index] = Uint8Array.from(bytes); const base = decodeGumpNode(Buffer.from(bytes.subarray(0, 160)).toString('hex')); base.parent = nsToken(base.parent);
        Object.assign(this.base(index + 1), base);
        if (index === 0) this.ns.background = nsToken(new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength).getUint32(0xa0, true));
    }
    // 현재 필드만 원문 offset 으로 쓰고 읽지 않는 확장은 그대로 보존한다(vtable 은 0, 부모는 원문 주소).
    encode(index) {
        const output = Uint8Array.from(this.preserved[index]); const source = this.base(index + 1); const copy = Object.assign(Object.create(Object.getPrototypeOf(source)), source);
        copy.vtable = 0; copy.parent = nsAddress(copy.parent); output.set(Buffer.from(encodeGumpNode(copy), 'hex'));
        if (index === 0) new DataView(output.buffer).setUint32(0xa0, nsAddress(this.ns.background), true);
        return output;
    }
    // 입력의 명시 변경(노드 raw 필드·전역·중단)을 현재 저장소에 적용한다.
    effect(effect) {
        const value = nsNumber(effect[3]);
        if (effect[1].startsWith('node')) {
            const index = Number(effect[1].slice(4)); const bytes = this.encode(index); const at = EFFECT_OFFSETS[effect[2]]; assert.notEqual(at, undefined, effect[2]);
            new DataView(bytes.buffer).setUint32(at, value, true); this.decode(index, bytes);
        } else if (effect[1] === 'global') {
            if (effect[2] === 'metamap') this.globals[22] = value; else if (effect[2] === 'playerKnown') this.globals[23] = value; else throw new Error('NSInfo 전역 효과');
        } else if (effect[1] !== 'control') throw new Error('NSInfo 효과 대상');
    }
    // 기본 장치 효과 뒤 같은 호출 번호의 입력 변경과 부분 중단을 공급한다.
    event(name, values = []) {
        this.trace += `${this.trace ? ';' : ''}${name}${values.map((value) => `:${value >>> 0}`).join('')}`;
        const count = (this.calls.get(name) ?? 0) + 1; this.calls.set(name, count); const trigger = `${name}${count}`; let stop = false;
        for (const effect of this.effects) { if (effect[0] !== trigger) continue; this.effect(effect); if (effect[1] === 'control' && effect[2] === 'stop' && effect[3] === '1') stop = true; }
        if (stop) throw new NsExplicitStop();
    }
    // 장치의 기본 Attach 상태 쓰기(표시 해제 비트·위치·부모)와 같은 최소 외부 계약이다.
    attach(name, token, x, y, parent, mode) { const node = this.base(token); node.flags = (node.flags & ~8) >>> 0; node.x = x; node.y = y; if (parent) node.parent = parent; this.event(name, [token, bitsOf(x), bitsOf(y), parent, mode]); }
    hooks() {
        return {
            node: (token) => (token === 1 ? this.ns : null), base: (token) => this.base(token),
            baseAttach: (token, x, y, parent, mode) => this.attach('baseAttach', token, x, y, parent, mode),
            push: (token) => { const node = this.base(token); node.flags = (node.flags | 8) >>> 0; this.event('push', [token]); },
            attach: (token, x, y, parent, mode) => this.attach('attach', token, x, y, parent, mode), metamapLayout: (token) => this.event('metamapLayout', [token]),
        };
    }
    // 읽지 않은 영역을 포함한 모든 현재 노드를 원문 주소 순서로 관측한다.
    nodes() { let output = ''; for (let i = 0; i < NODE_COUNT; i += 1) output += Buffer.from(this.encode(i)).toString('hex'); return output; }
    globalsText() { return this.globals.map((value) => value >>> 0).join(':'); }
}

test('HudNsInfo 원본 10.78 310행: 일반 부착의 8노드 raw·전역·호출 순서·부분 중단(8행)이 일치한다', () => {
    const table = fs.readFileSync(new URL('../../cpppj/tests/fixtures/hud-nsinfo-attach-x86.tsv', import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('name\t')).map((line) => line.split('\t'));
    assert.equal(table.length, 310);
    const failures = []; let stops = 0;
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 14, `행 ${index}`);
        const scene = new NsScene(row); const parts = row[4].split(':'); assert.equal(parts.length, 4); let completed = true;
        try { scene.handler.attach(1, floatOf(nsNumber(parts[0])), floatOf(nsNumber(parts[1])), nsNumber(parts[2]), nsNumber(parts[3])); } catch (error) { if (!(error instanceof NsExplicitStop)) throw error; completed = false; stops += 1; }
        const problems = [];
        if (completed !== (row[6] === '1')) problems.push('completed');
        if (scene.nodes() !== row[7]) problems.push('nodes');
        if (scene.globalsText() !== row[8]) problems.push('globals');
        if ((scene.trace === '' ? '-' : scene.trace) !== row[9]) problems.push('trace');
        if (completed && row[11] !== '20') problems.push('stack');
        if (!(nsNumber(row[12]) > 0) || row[13] !== '0') problems.push('instructions');
        if (problems.length) failures.push(`${index}/${row[0]}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
    assert.equal(stops, 8);
});
