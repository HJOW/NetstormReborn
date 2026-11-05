// 공통 공간 해제(WB04-D1)를 원본 10.78 관측과 대조한다.
//  - cpppj/tests/fixtures/hash-x86.tsv 의 Genus 3216행: effectiveGenus(004afd30)의 두 결과 열.
//  - cpppj/tests/fixtures/squid-unpop-parent-hardware-x86.tsv 1300행: 공통 해제 부모(004afe50)의 진단 복귀·부분 변경 뒤 checking·슬롯 필드·타입 70 발자국,
//    풀 슬롯 0~128(6450바이트)·네 단계 해시(172544바이트)·spot 지도(65536바이트)의 Adler-32 와 외부 요청 순서(A/M/N/D/Q/U).
//    장면 구성과 진단·통지 장치의 입력 변경은 cpppj tests/SquidUnpopCheckingTests.cpp 와 같다(기대값은 웹이 만들지 않는다).
// (2026-11-05 WB04-D1: 공통 공간 해제 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { RawBuildingSpace, effectiveGenus } from '../www/core/squid-space.js';
import { runSquidUnpopParent } from '../www/core/squid-unpop.js';

// 원본 관측 파일 경로이다. (2026-11-05 WB04-D1)
const HASH_FIXTURE = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'hash-x86.tsv');
const UNPOP_FIXTURE = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'squid-unpop-parent-hardware-x86.tsv');

// 탭 구분 행을 읽는다(주석·빈 줄 제외). 파일이 없으면 null 이다. (2026-11-05 WB04-D1)
function readRows(file) {
    if (!fs.existsSync(file)) return null;
    return fs.readFileSync(file, 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && line[0] !== '#').map((line) => line.split('\t'));
}

const genusRows = readRows(HASH_FIXTURE)?.filter((row) => row[0] === 'Genus') ?? null;
const unpopRows = readRows(UNPOP_FIXTURE);

// 단정도 비트를 수로 되돌린다. (2026-11-05 WB04-D1)
function float(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, Number(bits) >>> 0, true);
    return view.getFloat32(0, true);
}

// 바이트 배열에 리틀엔디언 값을 쓴다(cpppj Put). (2026-11-05 WB04-D1)
function put(raw, offset, value, width = 4) {
    // 필요한 바이트만 쓴다.
    for (let index = 0; index < width; index += 1) raw[offset + index] = (value >>> (8 * index)) & 0xff;
}

// 바이트 배열의 리틀엔디언 값을 읽는다(cpppj Get). (2026-11-05 WB04-D1)
function get(raw, offset, width = 4) {
    let value = 0;
    // 낮은 바이트부터 조합한다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// zlib 과 같은 Adler-32 이다. 결과는 매 바이트 나머지 계산과 같고, zlib 처럼 5552바이트마다 한 번 나머지를 구해 빠르게 계산한다. (2026-11-05 WB04-D1)
function adler(bytes) {
    let a = 1;
    let b = 0;
    // 5552바이트 묶음마다 두 합의 나머지를 구한다(합이 2^53 을 넘지 않는 zlib NMAX).
    for (let start = 0; start < bytes.length; start += 5552) {
        const end = Math.min(start + 5552, bytes.length);
        // 묶음 안에서는 나머지 없이 더한다.
        for (let index = start; index < end; index += 1) {
            a += bytes[index];
            b += a;
        }
        a %= 65521;
        b %= 65521;
    }
    return ((b << 16) | a) >>> 0;
}

// effectiveGenus: 원본 Genus 3216행의 두 결과 열과 같고, 일부 행은 실제로 비트 8 이 붙는다.
test('effectiveGenus: 원본10.78 Genus 3216행과 같다', { skip: !genusRows && '원본 fixture 없음' }, () => {
    let changed = 0;
    // 행마다 발자국·좌표·칸으로 계산한다.
    for (const row of genusRows) {
        const flags = Number(row[1]) >>> 0;
        const value = effectiveGenus(flags, float(row[4]), float(row[5]), Number(row[2]), Number(row[3]), Number(row[6]), Number(row[7]));
        assert.equal(value, Number(row[8]), row.join(' '));
        assert.equal(value, Number(row[9]), row.join(' '));
        if (value !== flags) changed += 1;
    }
    assert.equal(genusRows.length, 3216);
    assert.ok(changed > 0);
});

// effectiveGenus 계약: 해시의 단순 절삭과 다른 +0.9999 보정, 범위 밖 거부.
test('effectiveGenus: +0.9999 보정과 범위 밖 거부', () => {
    assert.equal(SquidHash.bucketIndex(0, 20.25, 20.25), 20 * 256 + 20);
    // 20.25 + 0.9999 → 21 이 오른쪽 아래이므로 3×3 의 안쪽은 20,20 이다.
    assert.equal(effectiveGenus(0x200, 20.25, 20.25, 3, 3, 20, 20), 0x208);
    assert.equal(effectiveGenus(0x200, 20.0001, 20.0001, 3, 3, 19, 19), 0x208);
    assert.equal(effectiveGenus(0x200, 20.25, 20.25, 3, 3, 21, 20), 0x200, '가장자리는 보정하지 않는다');
    assert.throws(() => effectiveGenus(0, 0, 0, 0, 1, 0, 0), RangeError);
    assert.throws(() => effectiveGenus(0, 0, 0, 1, 1, -1, 0), RangeError);
    assert.throws(() => effectiveGenus(0, 256, 0, 1, 1, 0, 0), RangeError);
});

// 원본 관측 장면이다. 같은 실제 풀(24000)·해시·점유 지도와 188칸 타입 표, 건물 옆면 장치(섬 83·다리 82·진단 0)를 만든다. (2026-11-05 WB04-D1)
class UnpopScene {
    // fixture 의 명시 입력만 읽어 장면을 만든다.
    constructor(row) {
        // 현재 진단 DWORD 이다(진단·통지 장치가 바꾼다).
        this.checking = Number(row[1]);
        // 해제할 SID 이다.
        this.source = Number(row[2]);
        // 표시 갱신 플래그이다.
        this.flags = Number(row[13]);
        // 진단·통지 장치가 할 입력 변경 번호이다.
        this.action = Number(row[14]);
        // 외부 요청 기록이다.
        this.events = '';
        // 첫 옆면 통지만 입력을 바꾸기 위한 표시이다.
        this.notified = false;
        // 실제 풀·해시·점유 지도·타입 표·건물 옆면 장치이다.
        this.pool = new SidPool(24000);
        this.hash = new SquidHash();
        this.spots = new Uint8Array(65536).fill(Number(row[15]));
        this.types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
        this.building = new RawBuildingSpace(this.pool, this.hash, this.types, { islandType: 83, bridgeType: 82, checking: () => 0 });
        // 관측 구간 슬롯 0~128 을 0 으로 채운다.
        for (let sid = 0; sid <= 128; sid += 1) this.raw(sid).fill(0);
        this.type(70, Number(row[11]), Number(row[10]), Number(row[8]), Number(row[9]));
        this.type(6, Number(row[11]), Number(row[10]), Number(row[8]), Number(row[9]));
        this.type(71, 0x800, 1, 3, 3);
        this.type(72, 0, 6, 2, 2);
        this.type(73, 0, 0, 1, 1);
        const raw = this.raw(this.source);
        put(raw, 0, 0x61f000);
        raw[10] = Number(row[3]);
        raw[11] = Number(row[4]);
        raw[40] = Number(row[5]);
        put(raw, 14, Number(row[6]));
        put(raw, 18, Number(row[7]));
        put(raw, 4, 51, 2);
        put(this.raw(49), 4, this.source, 2);
        this.raw(70)[10] = 83;
        this.raw(70)[12] = 0xff;
        // 주변 표면 입력만 배치한다.
        if (Number(row[16])) for (let y = 17; y <= 22; y += 1) for (let x = 13; x <= 18; x += 1) this.hash.setCell(0, x, y, 70);
        const layout = Number(row[12]);
        if (layout === 1 || layout === 4) this.hash.setBucket(0, this.x(), this.y(), this.source);
        if (layout === 2) this.hash.setBucket(1, this.x(), this.y(), 49);
        if (layout === 3 || layout === 4) this.hash.setBucket(3, this.x(), this.y(), this.source);
        // 원본 관측의 진단·통지 장치이다.
        this.hooks = {
            type: (number) => {
                if (number >= this.types.length) throw new RangeError('타입 범위');
                return this.types[number];
            },
            assertion: (expression, file, line) => this.assertion(expression, file, line),
            miniMap: (sid, genus) => this.miniMap(sid, genus),
            side: (x, y) => this.side(x, y),
            contained: (sid) => { this.events += `D,${sid};Q,${sid};`; },
            update: (sid, value) => { this.events += `U,${(value & 0x2000) ? 0x8c : 0x88},${sid},${this.raw(sid)[10]},${this.raw(sid)[11]};`; },
        };
    }

    // 물리 슬롯을 빌린다.
    raw(sid) { return this.pool.physical(sid); }

    // 타입 레코드 입력을 쓴다.
    type(number, flags1, flags2, width, height) { Object.assign(this.types[number], { flags1, flags2, footX: width, footY: height }); }

    // 해제 대상의 현재 x/y 이다.
    x() { return float(get(this.raw(this.source), 14)); }
    y() { return float(get(this.raw(this.source), 18)); }

    // 원본과 같은 진단 정상 복귀 입력이다.
    assertion(expression, file, line) {
        this.events += `A,${line}:${expression}:${file};`;
        const raw = this.raw(this.source);
        if (this.action === 1 && line === 0x617) {
            this.checking = 0;
            this.spots.fill(0xf0);
        }
        if (this.action === 2) {
            this.checking = 0x80000000;
            raw[10] = 71;
        }
        if (this.action === 3) {
            this.checking = 0;
            raw[10] = 70;
            raw[11] = 0;
        }
        if (this.action === 8) {
            this.checking = 0;
            raw[10] = 73;
        }
        if (this.action === 10 && line === 0x65d) this.hash.setBucket(0, this.x(), this.y(), this.source);
        if (this.action === 11) {
            this.checking = 0x80000000;
            raw[10] = 70;
            raw[11] &= 0xf7;
        }
    }

    // 미니맵 통지와 그 입력 변경이다.
    miniMap(sid, genus) {
        const raw = this.raw(this.source);
        this.events += `M,${sid},${get(raw, 14)},${get(raw, 18)},${genus};`;
        if (this.action === 4) {
            this.checking = 0x80000000;
            raw[10] = 71;
            put(raw, 14, 0x41880000);
            put(raw, 18, 0x41a80000);
            this.types[70].footX = 2;
            this.types[70].footY = 2;
        }
        if (this.action === 5) raw[40] |= 8;
        if (this.action === 6) raw[11] |= 8;
        if (this.action === 12) this.checking = 0;
    }

    // 실제 건물 옆면 해제와 첫 통지의 입력 변경이다.
    side(x, y) {
        const sid = this.building.surface(x, y);
        this.building.unpopSide(x, y);
        this.events += `N,${sid};`;
        if (this.notified) return;
        this.notified = true;
        const raw = this.raw(this.source);
        if (this.action === 7) {
            this.checking = 0x80000000;
            raw[10] = 71;
            put(raw, 14, 0x41880000);
            put(raw, 18, 0x41a80000);
        }
        if (this.action === 9) raw[11] |= 8;
    }

    // 해제를 실행하고 원본 관측과 같은 필드를 문자열로 내보낸다.
    run() {
        runSquidUnpopParent(this.pool, this.hash, this.spots, this.source, this.flags, () => this.checking, this.hooks);
        const poolBytes = new Uint8Array(129 * 50);
        // 관측 구간의 모든 raw BYTE 를 합친다.
        for (let sid = 0; sid <= 128; sid += 1) poolBytes.set(this.raw(sid), sid * 50);
        const hashBytes = new Uint8Array(2 * (65536 + 16384 + 4096 + 256));
        let offset = 0;
        // WORD 를 낮은 바이트부터 단계 순서로 합친다.
        for (let level = 0; level < 4; level += 1) {
            // 단계 배열의 각 WORD 를 리틀엔디언으로 쓴다.
            for (const value of this.hash.entries(level)) {
                hashBytes[offset] = value & 0xff;
                hashBytes[offset + 1] = value >>> 8;
                offset += 2;
            }
        }
        const raw = this.raw(this.source);
        return [this.checking, raw[10], raw[11], raw[40], get(raw, 14), get(raw, 18), this.types[70].footX, this.types[70].footY, adler(poolBytes), adler(hashBytes), adler(this.spots), this.events || '-'].map(String);
    }
}

// 공통 해제 부모: 원본 1300행의 진단 복귀·부분 변경·체크섬·요청 순서가 같다.
test('runSquidUnpopParent: 원본10.78 1300행과 같다', { skip: !unpopRows && '원본 fixture 없음' }, () => {
    let count = 0;
    const failures = [];
    // 행마다 장면을 새로 만들어 비교한다.
    for (const row of unpopRows) {
        assert.equal(row.length, 29);
        const actual = new UnpopScene(row).run();
        const expected = row.slice(17);
        if (JSON.stringify(actual) !== JSON.stringify(expected) && failures.length < 5) failures.push({ row: row[0], fields: actual.map((value, index) => (value === expected[index] ? null : `${index}:${expected[index]}≠${value}`)).filter(Boolean) });
        count += 1;
    }
    assert.deepEqual(failures, []);
    assert.equal(count, 1300);
});

// 선택된 진단·종속 장치가 없으면 오류이지만 이미 실행한 void·spot 쓰기와 미니맵 통지는 남는다(되돌리지 않는다).
test('runSquidUnpopParent: 미연결 장치 오류 때 부분 변경 보존', { skip: !unpopRows && '원본 fixture 없음' }, () => {
    const row = [...unpopRows[0]];
    row[10] = '4';
    row[8] = '1';
    row[9] = '1';
    row[12] = '0';
    row[16] = '0';
    const scene = new UnpopScene(row);
    delete scene.hooks.assertion;
    assert.throws(() => scene.run(), /assertion/);
    assert.equal(scene.raw(50)[11], 4);
    assert.equal(scene.spots[20 * 256 + 16], 0x31);
    assert.ok(scene.events.startsWith('M,50,'));
    assert.equal(scene.events.includes('U,'), false);
    const contained = new UnpopScene(row);
    contained.raw(50)[11] = 8;
    delete contained.hooks.contained;
    contained.hooks.assertion = () => {};
    assert.throws(() => contained.run(), /contained/);
    assert.equal(contained.raw(50)[11], 12);
});

// 건물 옆면 장치: 표면 WORD 의 비트 1/2 만 지우고 다리 표면이면 번호 진단을 고른다. 현재 다리 TYPE 공급자를 빌린다.
test('RawBuildingSpace: 옆면 해제와 다리 표면 통지 진단', () => {
    const pool = new SidPool(24000);
    const hash = new SquidHash();
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
    let checking = 0;
    const building = new RawBuildingSpace(pool, hash, types, { islandType: 83, bridgeType: 82, checking: () => checking });
    const sid = pool.allocate();
    const raw = pool.allocated(sid);
    raw[10] = 82;
    raw[12] = 0xff;
    raw[13] = 0x77;
    hash.setCell(0, 5, 6, sid);
    const calls = [];
    building.bindNotifyDiagnostics(() => checking, (...args) => calls.push(args));
    building.unpopSide(5, 6);
    assert.deepEqual([raw[12], raw[13], calls.length], [0xf9, 0x77, 0], '진단 0 이면 번호를 보지 않는다');
    checking = 0x80000000;
    building.unpopSide(5, 6);
    assert.deepEqual(calls, [['num >= 0 && num <= 7', '\\Ns\\O\\Bridge.cpp', 0xff]]);
    // 현재 다리 TYPE 이 바뀌면 같은 표면도 통지 대상이 아니다.
    let bridge = 82;
    building.bindCurrentBridgeType(() => bridge);
    bridge = 90;
    building.unpopSide(5, 6);
    assert.equal(calls.length, 1);
    assert.equal(building.surface(-1, 0), 0);
    assert.equal(building.surface(5, 6), sid);
    assert.throws(() => building.validateSpace(pool, new SquidHash()), TypeError);
    assert.throws(() => new RawBuildingSpace(pool, hash, types.slice(1), { islandType: 83, bridgeType: 82, checking: () => 0 }), TypeError);
});
