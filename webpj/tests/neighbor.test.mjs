// 일반 해시 flag 0 첫 이웃(RawSquidNeighbors.first)과 그 이웃을 쓰는 다리 끝 칸 변환(RawBridgeEvents)을 원본 10.78 관측 neighbor-x86.tsv 로 대조한다
// (cpppj `NeighborTests.cpp` 이식, WB04-E243: RawSquidNeighbors 마감).
//  - First 행(originals 288): 합성 노드 열(번호·타입·상태·extra·x·y 비트·발자국·flags1/2·프레임·해시 단계)을 풀과 해시 버킷에 놓고 첫 이웃을 대조한다.
//  - Event 행(originals 288): 같은 장면에서 다리 이벤트 처리기를 호출해 반환 float 비트·모든 외부 효과 기록(F/I/N/C/D/O/P)의 순서와 인자·두 관찰 슬롯 50바이트 전체를 대조한다.
//  - 계약: 첫 이웃 뒤의 손상된 체인/다른 풀 슬롯을 읽지 않고, 조회는 읽기 전용이며, 범위 밖 SID(0)·빈 지도는 거부한다.
// 울타리 이웃(fence-neighbors 1904행)과 표면 지도 순회(flag 8)는 각각 fence-neighbors.test.mjs·squid-neighbors.test.mjs 가 대조한다. CD 10.72(originalCD)·10.37 행은 범위 밖이다. (2026-11-10 WB04-E243)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { makeBridgeNeighborHooks, RawBridgeEvents } from '../www/core/raw-bridge-events.js';
import { RawSquidNeighbors } from '../www/core/raw-squid-neighbors.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { REPO_ROOT } from './helpers.mjs';
import { bitsToFloat, floatToBits, put, readRows } from './raw-support.mjs';

// 기계어 실행기의 입력 번호·가상 표 기록값·풀 크기이다. 호스트 함수 주소가 아니며 실제 월드 번호 배치도 아니다. (2026-11-10 WB04-E243)
const SOURCE = 50;
const BORN = 60;
const VTABLE = 0x11010000;
const CAPACITY = 24000;

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. originals(10.78) 행만 쓴다. (2026-11-10 WB04-E243)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'neighbor-x86.tsv'))?.filter((row) => row[1] === 'originals') ?? null;

// 10진 문자열 DWORD 를 부호 없는 수로 읽는다. (2026-11-10 WB04-E243)
const number = (text) => Number(BigInt(text) & 0xffffffffn);

// 합성 프레임 코드 표 3변형이다(cpppj `Frames(variant)`): 글자열 순서대로 번호 1부터, hard 면 0x40, L·M 은 0x20, 그 밖은 0. 판단 결과는 계산하지 않는다. (2026-11-10 WB04-E243)
function frameCodes(variant) {
    const letters = 'JKALMNOJKLBJPPCD';
    return [...letters].map((letter, index) => {
        const hard = variant === 1 ? [0, 1, 7, 8, 11].includes(index) : variant === 2 && (index === 0 || index === 8);
        return { side: letter.charCodeAt(0), variant: 0x50, number: index + 1, flags: hard ? 0x40 : letter === 'L' || letter === 'M' ? 0x20 : 0 };
    });
}

// 슬롯 전체 바이트를 hex 관찰과 바이트마다 비교한다. 빈 배열 관찰은 없다(50바이트 고정). (2026-11-10 WB04-E243)
const sameBytes = (bytes, hex) => bytes.length * 2 === hex.length && bytes.every((byte, index) => byte === parseInt(hex.slice(index * 2, index * 2 + 2), 16));

// cpppj `Scene`: 같은 풀·해시·spot·타입·프레임·이웃 탐색기를 조립해 입력과 결과를 대조하는 장면이다. 풀 번호 5~74 를 확보한다. (2026-11-10 WB04-E243)
class Scene {
    constructor() {
        this.pool = new SidPool(CAPACITY, { server: false });
        this.hash = new SquidHash();
        this.spots = new Uint8Array(65536);
        this.types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
        this.frames = Array.from({ length: 188 }, () => []);
        this.state = { authority: true, debugKeep: false, bridgeType: 0 };
        this.events = [];
        this.neighbors = null;
        // 예측/서버 목록과 무관한 client 번호를 입력 슬롯으로 확보한다.
        for (let id = 5; id <= 74; id += 1) assert.equal(this.pool.allocate(2), id);
    }

    // 각 입력의 raw 슬롯·타입·실제 네 단계 체인을 준비한다. nodes = `;` 구분 노드, 노드 = 12개 정수 필드, initialSpots = `-` 또는 `x,y,값;…`.
    prepare(nodes, initialSpots, variant) {
        this.hash.reset();
        this.spots.fill(0);
        this.events = [];
        // 두 관찰 슬롯의 stale 바이트도 제거한다.
        for (const sid of [SOURCE, BORN]) this.pool.allocated(sid).fill(0);
        // 각 노드는 분석 도구에 넣은 12개 정수 필드이다.
        for (const entry of nodes.split(';')) {
            const n = entry.split(',');
            assert.equal(n.length, 12);
            const sid = Number(n[0]);
            const type = number(n[1]);
            const raw = this.pool.allocated(sid);
            raw.fill(0);
            put(raw, 0, VTABLE, 4);
            raw[10] = type;
            raw[11] = number(n[2]);
            raw[40] = number(n[3]);
            put(raw, 14, number(n[4]), 4);
            put(raw, 18, number(n[5]), 4);
            put(raw, 36, number(n[10]), 4);
            this.types[type] = { footX: Number(n[6]), footY: Number(n[7]), flags1: number(n[8]), flags2: number(n[9]) };
            this.frames[type] = frameCodes(variant);
            const level = Number(n[11]);
            const x = bitsToFloat(number(n[4]));
            const y = bitsToFloat(number(n[5]));
            // 해시 버킷의 이전 머리를 노드의 다음 연결(+4 WORD)에 쓰고 노드가 새 머리가 된다.
            put(raw, 4, this.hash.bucket(level, x, y), 2);
            this.hash.setBucket(level, x, y, sid);
        }
        if (initialSpots !== '-') {
            // 기준점/발자국의 내부 비트 입력만 적용한다.
            for (const entry of initialSpots.split(';')) {
                const n = entry.split(',');
                this.spots[Number(n[1]) * 256 + Number(n[0])] = number(n[2]) & 0xff;
            }
        }
        this.neighbors = new RawSquidNeighbors(this.pool, this.hash, this.spots, this.types, this.frames);
    }

    // 실제 첫 이웃과 프레임만 쓰고 후속 외부 효과는 원본 실행기의 대체와 같은 사건 문자열로 기록한다.
    hooks() {
        const hooks = makeBridgeNeighborHooks(this.neighbors, {
            // 새 객체의 헤더(vtable·타입·상태 0)만 만든다. 실제 할당/postCreate 는 대체 경계이다.
            create: (type) => {
                this.events.push(`C:${type}:0`);
                const raw = this.pool.allocated(BORN);
                put(raw, 0, VTABLE, 4);
                raw[10] = type;
                raw[11] = 0;
                return BORN;
            },
            notifySurface: (sid) => this.events.push(`N:${sid}`),
            destroy: (sid, flags) => this.events.push(`D:${sid}:${flags}`),
            setOwner: (sid, owner) => this.events.push(`O:${sid}:${owner}`),
            pop: (sid, x, y, flags) => this.events.push(`P:${sid}:${floatToBits(x)}:${floatToBits(y)}:${flags}`),
        });
        const real = hooks.firstNeighbor;
        // 임시 프레임이 쓰인 채로 호출된다. 호출 전 프레임 낮은 바이트를 기록하고 실제 첫 이웃을 돌려준다.
        hooks.firstNeighbor = (sid) => {
            this.events.push(`F:${sid}:${this.pool.slot(sid)[36]}:0`);
            const first = real(sid);
            this.events.push(`I:${first}`);
            return first;
        };
        return hooks;
    }
}

test('neighbor: 원본10.78 flag 0 첫 이웃 288장면', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new Scene();
    let count = 0;
    for (const row of rows) {
        if (row[0] !== 'First') continue;
        assert.equal(row.length, 6);
        scene.prepare(row[2], row[3], Number(row[4]));
        assert.equal(scene.neighbors.first(SOURCE), number(row[5]), `First ${count + 1}`);
        count += 1;
    }
    assert.equal(count, 288);
});

test('neighbor: 원본10.78 다리 끝 칸 변환(실제 첫 이웃)의 반환·효과 순서·슬롯 전체 288장면', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new Scene();
    let count = 0;
    for (const row of rows) {
        if (row[0] === 'First') continue;
        assert.equal(row.length, 25);
        scene.prepare(row[19], row[20], Number(row[6]));
        scene.state.authority = row[3] === '1';
        scene.state.debugKeep = row[4] === '1';
        scene.state.bridgeType = number(row[5]);
        const raw = scene.pool.allocated(SOURCE);
        put(raw, 8, number(row[11]), 2);
        put(raw, 12, number(row[10]), 2);
        raw[34] = number(row[12]);
        raw[40] = number(row[13]);
        const bridge = new RawBridgeEvents(scene.pool, scene.state, scene.hooks());
        const result = bridge.handle(SOURCE, number(row[15]), 0, bitsToFloat(number(row[16])));
        const events = scene.events.join(';');
        // 반환 float 의 NaN 은 정규화로 비트가 달라질 수 있으므로 NaN 이면 관찰 비트가 NaN 인지로 비교한다.
        const expected = Number(row[21]);
        if (Number.isNaN(result)) assert.ok(Number.isNaN(bitsToFloat(expected)), `반환 NaN ${count + 1}`);
        else assert.equal(floatToBits(result), expected, `반환 ${count + 1} 프레임 ${row[7]}`);
        assert.equal(events === '' ? '-' : events, row[22], `효과 ${count + 1} 프레임 ${row[7]}`);
        assert.ok(sameBytes(scene.pool.slot(SOURCE), row[23]), `다리 슬롯 ${count + 1}`);
        assert.ok(sameBytes(scene.pool.slot(BORN), row[24]), `새 슬롯 ${count + 1}`);
        count += 1;
    }
    assert.equal(count, 288);
});

// fixture 가 구별하지 못하는 경계 계약이다(변형 시험에서 살아남은 입력): 후보 기준점의 spot 칸은 좌표에 float 0.9999 를 더해 0 쪽으로 자른 칸이고, 프레임 번호가 코드 수 밖이면 지연 읽기가 아닌 호출은 거부한다.
// 방향 계산의 가로 차이 부호(`dx > 0`)는 `|세로| < |가로|` 가 참일 때만 도달해 dx 가 0 일 수 없으므로 `dx >= 0` 과 동치이다(동등 변형). (2026-11-10 WB04-E243)
test('neighbor: 후보 spot 칸의 0.9999 보정과 프레임 번호 범위 계약', () => {
    const scene = new Scene();
    const cases = [
        // [후보 x, 내부 비트를 놓은 칸의 x, 후보 수용 여부]: 20.00015 + 0.9999 는 21 칸을 넘고 20.00005 + 0.9999 는 20 칸에 남는다.
        [20.00015, 21, false],
        [20.00015, 20, true],
        [20.00005, 20, false],
        [20.00005, 21, true],
    ];
    for (const [candidateX, interiorX, accepted] of cases) {
        scene.prepare(`50,82,0,0,${floatToBits(40)},${floatToBits(40)},1,1,2048,4,0,0;70,83,0,0,${floatToBits(candidateX)},${floatToBits(30)},1,1,2048,4,0,0`, '-', 0);
        scene.spots[30 * 256 + interiorX] = 8;
        const source = scene.neighbors.readObject(scene.neighbors.source(SOURCE), false);
        // flag 8(표면 지도)+1(접합 생략)은 교차 XOR·접합 검사를 건너뛰어 spot 검사만 남긴다.
        assert.equal(scene.neighbors.accept(70, source, 9), accepted, `x=${candidateX} 내부 칸=${interiorX}`);
    }
    // 프레임 번호가 코드 표 밖이면 거부하고, 지연 읽기이면 기본 프레임 코드로 둔다.
    assert.throws(() => scene.neighbors.readObject({ x: 20, y: 20, type: 82, frame: 99 }, true), RangeError);
    assert.deepEqual(scene.neighbors.readObject({ x: 20, y: 20, type: 82, frame: 99 }, true, true).frame, { side: 0, variant: 0, number: 0, flags: 0 });
});

// 첫 결과 뒤의 손상된 체인/다른 풀 슬롯을 미리 읽지 않고, 조회 전체는 읽기 전용이다. 범위 밖 SID 와 빈 지도는 거부한다.
test('neighbor: 첫 이웃은 방문하지 않은 손상을 읽기 전에 멈추고 풀을 바꾸지 않는다', () => {
    const scene = new Scene();
    scene.prepare('50,82,0,0,1101004800,1101529088,1,1,2048,4,2,0;70,83,0,0,1101529088,1101529088,1,1,2048,4,2,0', '-', 0);
    put(scene.pool.allocated(70), 4, 65535, 2);
    // 기준점 20/21 의 오른쪽 이웃은 단계 0에서 반환된다. next 는 저장되지만 이후 슬롯을 역참조하지 않는다.
    scene.hash.setCell(3, 1, 1, 65535);
    const before = scene.pool.bytes();
    assert.equal(scene.neighbors.first(SOURCE), 70);
    assert.deepEqual(scene.pool.bytes(), before);
    assert.throws(() => scene.neighbors.first(0), RangeError);
    assert.throws(() => new RawSquidNeighbors(scene.pool, scene.hash, new Uint8Array(0), scene.types, scene.frames), TypeError);
});
