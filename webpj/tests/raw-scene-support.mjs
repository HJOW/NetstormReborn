// 다리 연결(bridgeconnect)·섬 표면 후처리(islandpostpop) 관측이 함께 쓰는 장면 준비 도구이다(cpppj `tests/RawSceneSupport.h` 의 Scene 에 대응).
// 장면 형식: 객체 | spot | 프레임 변형 | 소유자 | 전역(다리 타입, 전투, 미션, 색 표 10칸). 실행기와 같은 풀 번호 5~127 을 확보하고 raw 슬롯·타입·네 단계 체인·전역을 준비하며 판단 결과는 계산하지 않는다.
// 훅은 원본 실행기의 대체와 같은 기록 경계이다: 호출 사실과 인자를 기록하고, 소유자 지정은 소유자 바이트만, 프레임 지정은 프레임 필드만 쓴다. (2026-11-08 WB04-E47: 두 시험이 공유하도록 bridge-connect.test.mjs 에서 분리)
import assert from 'node:assert/strict';
import { BridgeConnectState } from '../www/core/raw-bridge-connect.js';
import { RawSquidNeighbors } from '../www/core/raw-squid-neighbors.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { adler32, bitsToFloat, floatToBits, put } from './raw-support.mjs';

// 원본 실행기의 입력 번호·가상 표 기록값·풀 크기이다. (2026-11-08 WB04-E47)
export const SOURCE = 50;
export const FIRST_BORN = 100;
export const BORN_COUNT = 12;
export const LAST_SID = 127;
export const VTABLE = 0x11010000;
export const CAPACITY = 24000;
export const FRAME_OFFSET = 36;
export const OWNER_OFFSET = 34;
export const EXTRA_OFFSET = 40;

// 합성 타입 번호: bridge 82, bridgeConnector 90, island 91, islandStalag 92, noIsland 93 와 실제 10.78 플래그·발자국이다(bridge, isle, isleBig, priest, connector, island, stalag, noIsland 순). (2026-11-08 WB04-E47)
export const TYPE_INPUTS = Object.freeze([
    [82, 0x00000802, 0x00000004, 1, 1], [83, 0x28000803, 0x00000002, 1, 1], [84, 0x08000803, 0x01000002, 3, 3], [85, 0x00069012, 0x00210000, 1, 1],
    [90, 0x28000002, 0x02000000, 1, 1], [91, 0x28000003, 0x01000000, 3, 3], [92, 0x28000002, 0x00000000, 3, 3], [93, 0x08000803, 0x01000002, 1, 1],
]);

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E47)
export const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');

// bridgeevent 도구의 세 변형과 같은 합성 프레임 코드 표를 만든다(letters JKALMNOJKLBJPPCD, variant 별 hard 비트). (2026-11-08 WB04-E47)
export function framesFor(variant) {
    const letters = 'JKALMNOJKLBJPPCD';
    // 문자마다 프레임 코드 하나를 만든다.
    return Array.from(letters, (letter, index) => {
        const hard = variant === 1 ? [0, 1, 7, 8, 11].includes(index) : variant === 2 && (index === 0 || index === 8);
        return { side: letter.charCodeAt(0), variant: 80, number: index + 1, flags: hard ? 0x40 : (letter === 'L' || letter === 'M' ? 0x20 : 0) };
    });
}

// 장면: 실행기와 같은 풀 번호 5~127 을 확보하고 장면 입력으로 raw 슬롯·타입·해시·전역을 준비한다. (2026-11-08 WB04-E47)
export class Scene {
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

    // 이웃 탐색기는 생성 때의 타입/프레임 표를 복사하므로 장면 준비 뒤 표를 바꾸면 다시 만든다.
    refreshNeighbors() { this.neighbors = new RawSquidNeighbors(this.pool, this.hash, this.spots, this.types, this.frames); }

    // 프레임 필드를 DWORD 로 쓴다.
    putFrame(sid, frame) { put(this.pool.physical(sid), FRAME_OFFSET, frame >>> 0, 4); }

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
                this.putFrame(sid, frame);
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
