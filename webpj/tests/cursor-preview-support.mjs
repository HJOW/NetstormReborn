// 원본 미리보기/이동 관측의 독립 입력과 경계 기록을 공유한다. 기대값은 fixture에서만 읽는다. (2026-11-14 WB04-E403)
import assert from 'node:assert/strict';
import { put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { CursorDropState } from '../www/core/raw-cursor-drop.js';
import { CursorLifecycleState } from '../www/core/raw-cursor-lifecycle.js';
import { CursorPlacementState } from '../www/core/raw-cursor-placement.js';
import { BridgeConnectState } from '../www/core/raw-bridge-connect.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';

// fixture 입력14DWORD의 열 순서다.
export const [PROFILE, QUALITY, FLAGS1, FLAGS2, EXISTING, CONNECTORS, HP, CODE_PROFILE, X, Y, MUTATION, REPEAT, PREFIX, CHAIN] = Array.from({ length: 14 }, (_, index) => index);

// 현재 프레임 표의 명시 코드 입력이다. signed flags 판정은 제품 검색기가 수행한다.
function frames(profile) {
    const plain = [[65, 49, 0, 0], [66, 50, 1, 0], [67, 51, 2, 0]]; const special = [[193, 192, 0, 128], [127, 49, 1, 255], [65, 255, 2, 16]]; const result = [];
    // 원본의 기본/cracked/hard 코드 묶음 순서를 유지한다.
    for (const mask of [0, 32, 64]) {
        // 입력 코드의 BYTE만 결합한다.
        for (const [side, variant, number, flags] of profile ? special : plain) result.push({ side, variant, number, flags: (flags | mask) & 255 });
    }
    return new FrameCodeTable(result);
}

// Canon 공급자에 주는 상대 칸 입력이다.
function cells(profile) { return !profile ? [] : profile === 1 ? [{ frame: 0, x: 0, y: 0 }] : [{ frame: 0, x: 0, y: 0 }, { frame: 1, x: 1.25, y: -0.5 }, { frame: 2, x: -2.75, y: 3.5 }]; }

// 실제 풀/상태는 한 입력의 반복 호출 동안 유지한다. 가상/decoder 경계만 명시 입력을 제공한다.
export class CursorPreviewScene {
    // 단일 raw 저장소와 현재 TYPE 표다.
    pool = new SidPool(24000, { server: false }); types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
    // 같은 커서 전역과 표면 TYPE·프레임 입력이다.
    drop; life; state; surfaces; frames; input; touched = new Set(); trace = []; nextMain; nextConnector; ownerCalls;

    // 기존/생성/연결 입력에 필요한 실제 SID를 순서대로 확보한다.
    constructor() { for (let sid = 5; sid < 230; sid += 1) assert.equal(this.pool.allocate(2), sid); }

    // 원래 인자의 DWORD 비트와 호출 순서만 직렬화한다.
    event(letter, ...values) { this.trace.push([letter, ...values.map((value) => value >>> 0)].join(':')); }

    // Factory/연결 경계의 독립50바이트 입력이다. 실제 생성자 기대값은 아니다.
    raw(sid, type = 100) { const raw = this.pool.allocated(sid); raw.fill(0xa5); put(raw, 0, 1); raw[10] = type & 255; raw[11] = 0; raw[34] = 7; put(raw, 26, 99999); this.touched.add(sid); return raw; }

    // 입력만 새로 주며 반복 호출의 결과는 여기서 초기화하지 않는다.
    prepare(v = [1, 1, 0, 4, 0, 2, 1, 0, 0, 0, 0, 1, 0, 0]) {
        this.input = v; this.drop = new CursorDropState(); this.life = new CursorLifecycleState(); this.state = new CursorPlacementState(); this.surfaces = new BridgeConnectState();
        this.trace = []; this.touched.clear(); this.nextMain = 100; this.nextConnector = 200; this.ownerCalls = 0;
        // 이전 관측의 출력 바이트를 입력으로 재사용하지 않는다.
        for (const sid of [50, 51, 100, 101, 102, 103, 104]) this.raw(sid);
        this.pool.allocated(50)[30] = v[QUALITY] & 255; this.pool.allocated(51)[30] = 3;
        // 생성 여부와 현재 개수는 별도 원본 전역이다.
        for (let index = 0; index < 5; index += 1) this.life.previews[index] = v[EXISTING] ? 100 + index : 0;
        Object.assign(this.life, { previewCount: v[EXISTING] ? 5 : v[PREFIX] | 0, abstractType: 100, warningWidth: 35, warningHeight: 17, costHeight: 23 });
        Object.assign(this.drop, { type: 100, argument: 7, direction: 3, preview: 50, localPlayer: 1, x: bitsToFloat(v[X]), y: bitsToFloat(v[Y]), costFlags: v[CHAIN] ? 0x40 : 0, screenX: -3, screenY: 47, cursorWidth: 49 });
        this.state.screenOrigin = { x: -17, y: 31 }; Object.assign(this.surfaces, { bridgeType: 99, islandType: 91, stalagType: 92 });
        // 재조회/Factory 변경에 쓰는 세 TYPE에 같은 입력을 준다.
        for (const type of [100, 101, 102]) Object.assign(this.types[type], { flags1: v[FLAGS1], flags2: v[FLAGS2], footX: 2, footY: 3 });
        this.frames = frames(v[CODE_PROFILE]); return this;
    }

    // Query 재진입으로 바뀌는 전역의 명시 입력이다.
    queryMutation() {
        if (this.input[MUTATION] === 1) { this.life.abstractType = 101; Object.assign(this.drop, { type: 101, argument: 99, localPlayer: 2, x: -3.25, y: 4.75 }); }
        if (this.input[MUTATION] === 4) {
            // 생성 전 Query가 공급한 기존 목록이다.
            for (let index = 0; index < 5; index += 1) this.life.previews[index] = 100 + index;
            this.life.previewCount = 5;
        }
    }

    // 원본 검사와 같은 가상/프레임/연결 경계이며 복원 계산을 대신하지 않는다.
    hooks() {
        const event = (letter, ...values) => this.event(letter, ...values); const v = this.input;
        return {
            queryCursor: () => { event('Q'); this.queryMutation(); }, prepareCursor: () => event('P'),
            walk: (query, visit) => {
                event('W', query.type, query.argument, query.direction, floatToBits(query.x), floatToBits(query.y), query.explicitFrame ? 1 : 0);
                // false이면 원본처럼 다음 칸 진행 사건을 생략한다.
                for (const cell of cells(v[PROFILE])) { cell.x = Math.fround(cell.x + query.x); cell.y = Math.fround(cell.y + query.y); if (!visit(cell)) return; event('N'); }
            },
            create: (type, flags) => { const sid = this.nextMain++; event('C', type, flags, sid); this.raw(sid, type); if (v[MUTATION] === 2 && this.nextMain === 101) Object.assign(this.drop, { type: 102, argument: 0xface9999, localPlayer: 2, x: -7.75, y: 6.5 }); return sid; },
            setWord: (sid, value) => { event('S', sid, value); put(this.pool.allocated(sid), 12, value, 2); },
            setOwner: (sid, owner) => { event('O', sid, owner); this.pool.allocated(sid)[34] = owner & 255; this.ownerCalls += 1; if (v[MUTATION] === 3 && this.ownerCalls === 1) { this.types[100].flags2 = 4; this.drop.preview = 51; } },
            hasHitPoints: (sid) => { event('H', sid); return v[HP] !== 0; }, setHitPoints: (sid, value) => { event('Z', sid, value); put(this.pool.allocated(sid), 26, value); },
            frames: (type) => { event('F', type); return this.frames; },
            connect: (sid, output) => {
                const raw = this.pool.slot(sid); const x = bitsToFloat(get(raw, 14)); const y = bitsToFloat(get(raw, 18)); event('B', sid, get(raw, 14), get(raw, 18), get(raw, 22, 2), get(raw, 24, 2), get(raw, 36), this.life.previewCount);
                // 실제 RawBridgeConnect는 별도 관측 범위다. 여기서는 SID와 위치 입력만 공급한다.
                for (let index = 0; index < v[CONNECTORS]; index += 1) { const target = this.nextConnector++; const data = this.raw(target, 90); put(data, 14, floatToBits(Math.fround(x + (index % 2) * 2 - 1))); put(data, 18, floatToBits(Math.fround(y + index % 3 - 1))); output[index] = target; }
                return v[CONNECTORS] | 0;
            },
        };
    }

    // 실제 Query/Prepare 사슬에 필요한 외부 공급자다.
    placementHooks(previews) {
        const event = (letter, ...values) => this.event(letter, ...values);
        return { walk: previews.walk,
            describe: (text, x, y, type, owner) => { event('D', floatToBits(x), floatToBits(y), type, owner); text.value = 'Cursor'; return 1; },
            image: (type, frame) => { event('I', type, 0, frame); return { token: 1, width: 31, height: 23, hotspotX: 8, hotspotY: 17 }; },
            gumpAt: (x, y) => { event('G', x, y); return 0; }, mayPlace: (q) => { event('M', q.type, q.argument, floatToBits(q.x), floatToBits(q.y), q.flags, q.owner, q.mode); return 1; },
            project: (x, y) => { event('J', floatToBits(x), floatToBits(y)); return { x: 501, y: 603 }; }, invalidate: (r, flags) => event('V', r.left, r.top, r.right, r.bottom, flags) };
    }

    // 원본52DWORD 전체를 원래 순서로 직렬화한다.
    snapshot() {
        const d = this.drop; const l = this.life; const s = this.state;
        return [l.previewCount, d.type, d.argument, d.localPlayer, floatToBits(d.x), floatToBits(d.y), d.legal, s.placementAllowed, s.descriptionAllowed, s.spaceAllowed, s.sourceAllowed, ...s.bounds,
            s.hotspotX, s.hotspotY, s.imageWidth, s.imageHeight, ...l.previews, l.abstractType, this.types[100].flags2, d.preview].map((value) => value >>> 0).join(',');
    }

    // 접근한 슬롯의 미수정 바이트까지 번호 순서로 비교한다.
    raws() { return [...this.touched].sort((a, b) => a - b).map((sid) => `${sid}:${Buffer.from(this.pool.slot(sid)).toString('hex')}`).join(';'); }
}
