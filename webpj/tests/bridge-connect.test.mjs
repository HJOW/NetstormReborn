// 다리/섬 연결 raw-bridge-connect.js(RawBridgeConnect)와 E39 의 일반 해시 이웃 순회(flag 0~7)를 원본 10.78 관측 bridgeconnect-x86.tsv 3,600행과 cpppj 계약으로 대조한다.
// 행 종류: Walk(232 장면 × flags 8개의 이웃 번호 목록)·Connect(등록/미리보기)·Link(연결 객체 생성)·Owner(소유자 전파)·FindAt(한 칸 조회)·Color(색 프레임)·IslandOwner/StalagOwner(소유자 지정 재정의)·IslandPost(섬 받침 postPop).
// original1037·originalCD 행은 범위 밖이다. 생성·소유자·Pop·프레임·표면 알림은 원본 실행기와 같은 기록 훅이며 소유자는 소유자 바이트만, 프레임은 프레임 필드만 쓴다. (2026-11-08 WB04-E42: RawBridgeConnect 이식과 함께 추가)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { BridgeConnectState, RawBridgeConnect, makeOwnerDispatch, ownerColorFrame, setIslandOwner } from '../www/core/raw-bridge-connect.js';
import { NEIGHBOR_FLAG, RawSquidNeighborWalk, RawSquidNeighbors } from '../www/core/raw-squid-neighbors.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { REPO_ROOT } from './helpers.mjs';
import { adler32, bitsToFloat, floatToBits, get, put, readRows } from './raw-support.mjs';

// 원본 실행기의 입력 번호·가상 표 기록값·풀 크기이다. (2026-11-08 WB04-E42)
const SOURCE = 50;
const FIRST_BORN = 100;
const BORN_COUNT = 12;
const LAST_SID = 127;
const VTABLE = 0x11010000;
const CAPACITY = 24000;
const FRAME_OFFSET = 36;
const OWNER_OFFSET = 34;
const EXTRA_OFFSET = 40;

// 합성 타입 번호: bridge 82, bridgeConnector 90, island 91, islandStalag 92, noIsland 93 와 실제 10.78 플래그·발자국이다(bridge, isle, isleBig, priest, connector, island, stalag, noIsland 순). (2026-11-08 WB04-E42)
const TYPE_INPUTS = Object.freeze([
    [82, 0x00000802, 0x00000004, 1, 1], [83, 0x28000803, 0x00000002, 1, 1], [84, 0x08000803, 0x01000002, 3, 3], [85, 0x00069012, 0x00210000, 1, 1],
    [90, 0x28000002, 0x02000000, 1, 1], [91, 0x28000003, 0x01000000, 3, 3], [92, 0x28000002, 0x00000000, 3, 3], [93, 0x08000803, 0x01000002, 1, 1],
]);

// fixture 파일 경로이다. (2026-11-08 WB04-E42)
const fixture = (name) => path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name);

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E42)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');

// bridgeevent 도구의 세 변형과 같은 합성 프레임 코드 표를 만든다(letters JKALMNOJKLBJPPCD, variant 별 hard 비트). (2026-11-08 WB04-E42)
function framesFor(variant) {
    const letters = 'JKALMNOJKLBJPPCD';
    // 문자마다 프레임 코드 하나를 만든다.
    return Array.from(letters, (letter, index) => {
        const hard = variant === 1 ? [0, 1, 7, 8, 11].includes(index) : variant === 2 && (index === 0 || index === 8);
        return { side: letter.charCodeAt(0), variant: 80, number: index + 1, flags: hard ? 0x40 : (letter === 'L' || letter === 'M' ? 0x20 : 0) };
    });
}

// 장면: 실행기와 같은 풀 번호 5~127 을 확보하고 장면 입력(객체·spot·프레임 변형·소유자·전역)으로 raw 슬롯·타입·해시·전역을 준비한다. (2026-11-08 WB04-E42)
class Scene {
    constructor() {
        this.pool = new SidPool(CAPACITY, { server: false });
        // 예측/서버 목록과 무관한 client 번호를 입력 슬롯으로 확보한다.
        for (let id = 5; id <= LAST_SID; id += 1) assert.equal(this.pool.allocate(2), id);
        this.hash = new SquidHash();
        this.spots = new Uint8Array(65536);
        this.types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 1, footY: 1 }));
        this.frames = Array.from({ length: 188 }, () => []);
        this.state = new BridgeConnectState();
        this.neighbors = null;
        this.events = [];
        this.born = 0;
    }

    // 장면 입력 5칸으로 raw 슬롯·타입·네 단계 체인·전역을 준비한다.
    prepare(columns) {
        assert.equal(columns.length, 5);
        const variant = Number(columns[2]);
        this.hash.reset();
        this.spots.fill(0);
        this.events = [];
        this.born = 0;
        // 앞선 행의 슬롯이 남지 않도록 실행기 풀 전체를 지운다.
        for (let id = 5; id <= LAST_SID; id += 1) this.pool.physical(id).fill(0);
        // 객체가 없는 타입도 조회 대상이므로 합성 타입 표 전체를 넣는다.
        for (const [number, flags1, flags2, footX, footY] of TYPE_INPUTS) {
            Object.assign(this.types[number], { flags1, flags2, footX, footY });
            this.frames[number] = framesFor(variant);
        }
        // 각 node 는 분석 도구에 넣은 12개 정수 필드: 번호, 타입, 상태, extra, x, y, 폭, 높이, flags1, flags2, 프레임, 해시 단계.
        for (const entry of columns[0].split(';')) {
            const n = entry.split(',');
            assert.equal(n.length, 12);
            const sid = Number(n[0]);
            const type = Number(n[1]);
            const raw = this.pool.physical(sid);
            raw.fill(0);
            put(raw, 0, VTABLE);
            raw[10] = type;
            raw[11] = Number(n[2]);
            raw[EXTRA_OFFSET] = Number(n[3]);
            put(raw, 14, Number(n[4]));
            put(raw, 18, Number(n[5]));
            put(raw, FRAME_OFFSET, Number(n[10]), 4);
            Object.assign(this.types[type], { footX: Number(n[6]), footY: Number(n[7]), flags1: Number(n[8]) >>> 0, flags2: Number(n[9]) >>> 0 });
            this.frames[type] = framesFor(variant);
            const level = Number(n[11]);
            const x = bitsToFloat(n[4]);
            const y = bitsToFloat(n[5]);
            put(raw, 4, this.hash.bucket(level, x, y), 2);
            this.hash.setBucket(level, x, y, sid);
        }
        // 기준 객체의 두 단어는 실행기의 고정 입력(+8 = 0x2a2b, +12 = 0x1234)이다.
        put(this.pool.physical(SOURCE), 8, 0x2a2b, 2);
        put(this.pool.physical(SOURCE), 12, 0x1234, 2);
        if (columns[1] !== '-') {
            // spot 입력은 x, y, 값이다.
            for (const entry of columns[1].split(';')) {
                const [x, y, value] = entry.split(',').map(Number);
                this.spots[y * 256 + x] = value;
            }
        }
        if (columns[3] !== '-') {
            // 소유자 입력은 번호, 소유자이다.
            for (const entry of columns[3].split(';')) {
                const [sid, owner] = entry.split(',').map(Number);
                this.pool.physical(sid)[OWNER_OFFSET] = owner;
            }
        }
        // 전역: 다리 타입, 전투, 미션, 색 표 10칸(표는 앞의 9칸이다).
        const globals = columns[4].split(',');
        assert.equal(globals.length, 13);
        this.state = new BridgeConnectState();
        Object.assign(this.state, { connectorType: 90, islandType: 91, stalagType: 92, noIslandType: 93, bridgeType: Number(globals[0]), battle: globals[1] === '1', mission: globals[2] === '1' });
        this.state.ownerColors = Array.from({ length: 9 }, (_, index) => Number(globals[3 + index]) | 0);
        this.neighbors = new RawSquidNeighbors(this.pool, this.hash, this.spots, this.types, this.frames);
    }

    // 원본 실행기의 대체와 같은 경계: 호출 사실과 인자를 기록하고, 소유자 지정은 소유자 바이트만, 프레임 지정은 프레임 필드만 쓴다.
    hooks() {
        return {
            create: (type, flags) => {
                if (this.born >= BORN_COUNT) throw new Error('새 객체 번호 소진');
                const sid = FIRST_BORN + this.born;
                this.born += 1;
                const raw = this.pool.physical(sid);
                raw.fill(0);
                put(raw, 0, VTABLE);
                raw[10] = type;
                raw[11] = 0;
                this.events.push(`C:${type}:${flags}`);
                return sid;
            },
            setOwner: (sid, owner) => {
                this.pool.physical(sid)[OWNER_OFFSET] = owner & 0xff;
                this.events.push(`O:${sid}:${owner}`);
            },
            pop: (sid, x, y, flags) => this.events.push(`P:${sid}:${floatToBits(x)}:${floatToBits(y)}:${flags}`),
            setFrame: (sid, frame, flags) => {
                put(this.pool.physical(sid), FRAME_OFFSET, frame >>> 0, 4);
                this.events.push(`S:${sid}:${frame >>> 0}:${flags}`);
            },
            notifySurface: (sid) => this.events.push(`N:${sid}`),
        };
    }

    // 모든 원본 효과의 순서와 인자를 한 문자열로 만든다.
    eventText() { return this.events.length ? this.events.join(';') : '-'; }

    // 만든 객체 슬롯 전체의 16진수 목록이다.
    bornText() {
        const parts = [];
        // 만든 순서대로 이어 붙인다.
        for (let index = 0; index < this.born; index += 1) parts.push(hex(this.pool.slot(FIRST_BORN + index)));
        return parts.length ? parts.join(',') : '-';
    }

    // 실행기 풀(번호 0~127)의 Adler-32 이다. 번호 0~4 는 실행기에서 항상 0 이다.
    poolAdler() {
        const stride = this.pool.layout.stride;
        const image = new Uint8Array((LAST_SID + 1) * stride);
        // 확보한 슬롯만 실제 바이트를 복사한다.
        for (let id = 5; id <= LAST_SID; id += 1) image.set(this.pool.slot(id), id * stride);
        return adler32(image);
    }
}

// 저장된 독립 기계어 출력(장면 입력과 관찰 행)을 한 번만 읽는다. (2026-11-08 WB04-E42)
const rows = readRows(fixture('bridgeconnect-x86.tsv'));
const scenes = new Map(rows.filter((row) => row[0] === 'Scene').map((row) => [row[1], row.slice(2)]));
const observations = rows.filter((row) => row[0] !== 'Scene' && row[1] === 'originals');

// 미리보기 목록을 실행기의 표기로 만든다. (2026-11-08 WB04-E42)
const listed = (preview, used) => (used ? (preview.length ? preview.join(',') : 'empty') : '-');

test('bridge-connect: 일반 해시 이웃 순회(flag 0~7)가 232 장면 1856행의 이웃 번호 목록과 일치한다', () => {
    const scene = new Scene();
    let count = 0;
    // 장면마다 플래그 8개를 같은 입력으로 순회해 원본 관찰과 비교한다.
    for (const row of observations.filter((item) => item[0] === 'Walk')) {
        scene.prepare(scenes.get(row[2]));
        const walk = new RawSquidNeighborWalk(scene.neighbors, SOURCE, Number(row[3]));
        const found = [];
        // 0 이 나올 때까지 이웃 번호를 모은다.
        for (let current = walk.current; current !== 0; current = walk.next()) found.push(current);
        assert.equal(found.length ? found.join(',') : '-', row[4], `Walk ${row[2]} flags=${row[3]}`);
        count += 1;
    }
    assert.equal(count, 1856);
});

test('bridge-connect: 색 프레임·섬 받침/종유석 소유자 재정의·한 칸 조회를 원본 관찰과 대조한다', () => {
    const scene = new Scene();
    const counts = { Color: 0, IslandOwner: 0, StalagOwner: 0, FindAt: 0 };
    // 행 종류마다 같은 입력으로 실행해 원본 관찰과 비교한다.
    for (const row of observations) {
        if (row[0] === 'Color') {
            const state = new BridgeConnectState();
            state.battle = row[2] === '1';
            state.ownerColors = row[3].split(',').slice(0, 9).map((value) => Number(value) | 0);
            const owner = Number(row[4]) | 0;
            // 전투 중의 표 밖 번호는 원본이 표 뒤의 메모리를 읽는다. 여기서는 거부한다. 전투가 아니면 표를 읽지 않는다.
            if (state.battle && owner >= 9) assert.throws(() => ownerColorFrame(state, owner), RangeError);
            else assert.equal(String(ownerColorFrame(state, owner)), row[5], `Color ${row[3]} owner=${owner}`);
        } else if (row[0] === 'IslandOwner' || row[0] === 'StalagOwner') {
            scene.prepare(scenes.get(row[2]));
            setIslandOwner(scene.pool, scene.state, SOURCE, Number(row[3]), (sid, owner) => {
                scene.pool.physical(sid)[OWNER_OFFSET] = owner & 0xff;
                scene.events.push(`B:${sid}:${owner}`);
            });
            assert.equal(`${scene.eventText()}|${get(scene.pool.slot(SOURCE), FRAME_OFFSET, 4)}`, `${row[4]}|${row[5]}`, `${row[0]} ${row[2]} owner=${row[3]}`);
        } else if (row[0] === 'FindAt') {
            scene.prepare(scenes.get(row[2]));
            const connect = new RawBridgeConnect(scene.pool, scene.neighbors, scene.state, scene.hooks());
            assert.equal(String(connect.findTypeAt(bitsToFloat(row[3]), bitsToFloat(row[4]), Number(row[5]))), row[6], `FindAt ${row[2]}`);
        } else continue;
        counts[row[0]] += 1;
    }
    assert.deepEqual(counts, { Color: 42, IslandOwner: 96, StalagOwner: 96, FindAt: 270 });
});

test('bridge-connect: 연결 순회(Connect)·연결 객체 생성(Link)·소유자 전파(Owner)·섬 받침 postPop 을 원본 관찰과 대조한다', () => {
    const scene = new Scene();
    const counts = { Connect: 0, Link: 0, Owner: 0, IslandPost: 0 };
    // 행 종류마다 새 장면에서 같은 입력으로 실행한다.
    for (const row of observations) {
        if (!(row[0] in counts)) continue;
        scene.prepare(scenes.get(row[2]));
        const connect = new RawBridgeConnect(scene.pool, scene.neighbors, scene.state, scene.hooks());
        const preview = [];
        let actual;
        let expected;
        if (row[0] === 'Connect') {
            const used = row[3] === '1';
            if (used) connect.connectPreview(SOURCE, (born) => preview.push(born));
            else connect.connect(SOURCE);
            actual = `${scene.eventText()}|${scene.bornText()}|${scene.poolAdler()}|${listed(preview, used)}`;
            expected = `${row[4]}|${row[5]}|${row[6]}|${row[7]}`;
        } else if (row[0] === 'Link') {
            const used = row[5] === '1';
            connect.link(Number(row[3]), Number(row[4]), Number(row[6]), used ? preview : null);
            actual = `${scene.eventText()}|${scene.bornText()}|${scene.poolAdler()}|${listed(preview, used)}`;
            expected = `${row[7]}|${row[8]}|${row[9]}|${row[10]}`;
        } else if (row[0] === 'Owner') {
            connect.propagateOwner(Number(row[3]), Number(row[4]));
            actual = `${scene.eventText()}|${scene.poolAdler()}`;
            expected = `${row[5]}|${row[6]}`;
        } else {
            connect.islandPostPop(SOURCE, Number(row[3]), (sid, flags) => scene.events.push(`T:${sid}:${flags}`));
            actual = `${scene.eventText()}|${scene.bornText()}|${scene.poolAdler()}`;
            expected = `${row[4]}|${row[5]}|${row[6]}`;
        }
        assert.equal(actual, expected, `${row[0]} ${row[2]}`);
        counts[row[0]] += 1;
    }
    assert.deepEqual(counts, { Connect: 464, Link: 392, Owner: 330, IslandPost: 54 });
});

test('원본 4210f0 의 목록 쓰기가 다음 연결 객체 생성 전에 보이고, 소비자가 없으면 거부한다', () => {
    const scene = new Scene();
    let found = false;
    // 독립 원문의 다중 연결 장면을 고른다. 기대 SID 순서도 원문에서 읽는다.
    for (const row of observations) {
        if (row[0] !== 'Connect' || row[3] !== '1' || !row[7].includes(',')) continue;
        scene.prepare(scenes.get(row[2]));
        const hooks = scene.hooks();
        const create = hooks.create;
        const written = [];
        let calls = 0;
        hooks.create = (type, flags) => {
            assert.equal(written.length, calls);
            calls += 1;
            return create(type, flags);
        };
        const connect = new RawBridgeConnect(scene.pool, scene.neighbors, scene.state, hooks);
        assert.throws(() => connect.connectPreview(SOURCE, null), TypeError);
        connect.connectPreview(SOURCE, (sid) => written.push(sid));
        assert.ok(calls > 1);
        assert.equal(listed(written, true), row[7]);
        assert.equal(connect.pool, scene.pool);
        found = true;
        break;
    }
    assert.ok(found);
});

test('훅 누락·다른 풀·미지원 flag·색 표 밖 번호·소유자 없는 다리의 전파는 상태를 바꾸기 전에 거부한다', () => {
    const scene = new Scene();
    // 다리(50, 소유자 0) 서쪽에 표면 칸(70), 그 칸을 덮는 주인 없는 섬 받침(71)을 둔다.
    scene.prepare(['50,82,0,0,1101004800,1101529088,1,1,2050,4,2,0;70,83,0,0,1100480512,1101529088,1,1,671090691,2,2,0;71,91,0,0,1100480512,1101529088,3,3,671088643,16777216,8,3',
        '-', '0', '50,0;71,0', '82,1,0,0,1,2,3,4,5,6,7,8,1']);
    const hooks = scene.hooks();
    assert.throws(() => new RawBridgeConnect(scene.pool, scene.neighbors, scene.state, { ...hooks, setFrame: undefined }), TypeError);
    assert.throws(() => new RawBridgeConnect(scene.pool, scene.neighbors, scene.state, { ...hooks, create: undefined }), TypeError);
    const other = new Scene();
    assert.throws(() => new RawBridgeConnect(other.pool, scene.neighbors, scene.state, hooks), TypeError);
    // 원문 지도 순회의 8 은 지원하며 이 단언은 미지원 다섯째 비트의 호스트 계약이다.
    assert.throws(() => new RawSquidNeighborWalk(scene.neighbors, SOURCE, 0x10), RangeError);
    assert.throws(() => ownerColorFrame(scene.state, 9), RangeError);
    assert.throws(() => setIslandOwner(scene.pool, scene.state, SOURCE, 1, null), TypeError);
    assert.throws(() => makeOwnerDispatch(scene.pool, scene.state, null), TypeError);
    const connect = new RawBridgeConnect(scene.pool, scene.neighbors, scene.state, hooks);
    const before = scene.pool.bytes();
    // 원본 assert("newPlayerId != INVALID_PLAYER_ID") 경로: 훅을 부르거나 풀을 바꾸기 전에 예외이다.
    assert.throws(() => connect.propagateOwner(SOURCE, 70), /newPlayerId/);
    assert.equal(scene.events.length, 0);
    assert.deepEqual(scene.pool.bytes(), before);
    // 비최초의 공통 장치 실패만 이 무효과 장면에서 확인한다.
    assert.throws(() => connect.islandPostPop(SOURCE, 0, null), TypeError);
    assert.throws(() => connect.findTypeAt(20, 21, 5000), RangeError);
    // 소유자가 있으면 같은 장면에서 받침의 소유자와 색 프레임(전투 모드, 소유자 4 → 색 표 4 - 1)이 바뀐다.
    scene.pool.physical(SOURCE)[OWNER_OFFSET] = 4;
    connect.propagateOwner(SOURCE, 70);
    assert.equal(scene.eventText(), 'O:71:4;S:71:3:0');
});

test('섬 받침 접두: 공통 장치 누락 앞에 종유석 생성/소유자/Pop 효과가 남고, 소유자 분배는 가상 표로 재정의를 고른다', () => {
    const scene = new Scene();
    scene.prepare(['50,91,0,0,1106247680,1106247680,3,3,671088643,16777216,0,0', '-', '0', '50,3', '82,1,0,0,1,2,3,4,5,6,7,8,1']);
    const connect = new RawBridgeConnect(scene.pool, scene.neighbors, scene.state, scene.hooks());
    assert.throws(() => connect.islandPostPop(SOURCE, 1, null), TypeError);
    assert.equal(scene.born, 1);
    assert.equal(scene.eventText(), 'C:92:2;O:100:3;P:100:1106247680:1106247680:0');
    // 소유자 분배: 일반 가상 표는 base 만, 섬 받침(0x506d98)과 종유석(0x506e50)은 색 프레임까지 쓴다.
    const base = [];
    const dispatch = makeOwnerDispatch(scene.pool, scene.state, (sid, owner) => base.push(`${sid}:${owner}`));
    dispatch(SOURCE, 2);
    assert.deepEqual(base, ['50:2']);
    assert.equal(get(scene.pool.slot(SOURCE), FRAME_OFFSET, 4), 0);
    put(scene.pool.physical(SOURCE), 0, 0x506d98);
    scene.state.battle = true;
    dispatch(SOURCE, 2);
    assert.equal(get(scene.pool.slot(SOURCE), FRAME_OFFSET, 4), 1);
    put(scene.pool.physical(SOURCE), 0, 0x506e50);
    dispatch(SOURCE, 3);
    assert.equal(get(scene.pool.slot(SOURCE), FRAME_OFFSET, 4), 2);
    // 미션 중이고 base 뒤의 프레임이 중립이면 색칠하지 않는다.
    scene.state.mission = true;
    put(scene.pool.physical(SOURCE), FRAME_OFFSET, 8, 4);
    dispatch(SOURCE, 4);
    assert.equal(get(scene.pool.slot(SOURCE), FRAME_OFFSET, 4), 8);
});

test('이웃 순회는 호출 사이에 등록된 뒤쪽 버킷 객체를 읽고 이미 지나간 버킷은 읽지 않는다', () => {
    const scene = new Scene();
    // 다리(50) 서쪽 (19,21)과 동쪽 (21,21)에 표면 칸이 있다. 0단계에서 y, x 순서라 서쪽이 먼저다.
    scene.prepare(['50,82,0,0,1101004800,1101529088,1,1,2050,4,2,0;70,83,0,0,1100480512,1101529088,1,1,671090691,2,2,0;71,83,0,0,1101529088,1101529088,1,1,671090691,2,2,0',
        '-', '0', '-', '82,1,0,0,1,2,3,4,5,6,7,8,1']);
    const walk = new RawSquidNeighborWalk(scene.neighbors, SOURCE, NEIGHBOR_FLAG.skipAbstract);
    assert.equal(walk.current, 70);
    // 첫 이웃을 받은 뒤 남쪽 (20,22)과 이미 지나간 북쪽 (20,20)에 표면 칸을 직접 등록한다.
    const add = (id, x, y) => {
        const raw = scene.pool.physical(id);
        put(raw, 0, VTABLE);
        raw[10] = 83;
        put(raw, 14, floatToBits(x));
        put(raw, 18, floatToBits(y));
        put(raw, 36, 2, 4);
        put(raw, 4, scene.hash.bucket(0, x, y), 2);
        scene.hash.setBucket(0, x, y, id);
    };
    add(72, 20, 22);
    add(73, 20, 20);
    assert.equal(walk.next(), 71);
    assert.equal(walk.next(), 72);
    assert.equal(walk.next(), 0);
    assert.equal(walk.next(), 0);
});
