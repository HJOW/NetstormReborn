// Fort 객체 로더 시험이 공유하는 장면 도우미이다: fortobjectload-x86.tsv 의 입력 열 이름(F), 원본 50바이트 슬롯 쓰기(put), float 비트(bits), 프레임·내용물 입력 프로필, 레코드 모양(record),
// 그리고 원본 관측기에 공급했던 명시적 장치/저장소(harness: 풀 · finder · 타입 188 · 프레임 188 · 난수 · 공유 상태 · 효과 훅 · 이벤트 기록)이다.
// 값형 경로(fort-object-load.test.mjs)와 stream 경로(fort-object-stream.test.mjs)가 같은 입력과 같은 효과 기록을 쓰도록 분리했다. 기대값은 만들지 않는다. (2026-11-10 WB04-E276: 시험 도우미 분리)
import assert from 'node:assert/strict';
import { createFortObjectLoadState, createFortObjectLoadStateFromGlobals } from '../www/core/fort-object-load.js';
import { LocalRandom } from '../www/assets/terrain-mask.js';

// 원본 관측의 입력 열 이름이다. 출력 계산에는 사용하지 않는다.
export const F = Object.freeze(Object.fromEntries(['stored', 'type', 'position', 'flags1', 'flags2', 'version', 'defaultOwner', 'owner',
    'special', 'mission', 'fort', 'blocked', 'quarter', 'seed', 'frames', 'terrain', 'mutation', 'originX', 'originY',
    'contents', 'frame', 'qa', 'qb', 'bridgeFrame', 'legacy', 'factory', 'chaffMode', 'slot'].map((name, index) => [name, index])));
// 실제 원본 기록의50바이트 슬롯과 동일한 little endian 쓰기/float 비트 관측이다.
export function put(raw, offset, value, width = 4) {
    for (let byte = 0; byte < width; byte += 1) raw[offset + byte] = (value >>> (byte * 8)) & 255;
}
export function bits(value) { const data = Buffer.alloc(4); data.writeFloatLE(value); return data.readUInt32LE(); }

// fixture 입력의 P/P, P/A, P/모든 변형, 없는 후보,256개 상한을 시험하는 프레임 표이다.
export function frameProfile(profile) {
    const pairs = { 1: [[80, 80], [80, 80], [65, 65]], 2: [[80, 65], [80, 65]], 3: [[80, 67], [80, 68]], 4: [[65, 65]] };
    const codes = profile === 5 ? Array.from({ length: 300 }, (_, index) => ({ side: 80, variant: 80, number: index & 255, flags: index & 255 }))
        : (pairs[profile] ?? []).map(([side, variant], index) => ({ side, variant, number: index, flags: index === 1 ? 255 : 0 }));
    return { codes };
}

// native 입력 목록이다. 중첩 목록의 자식은 실제 createContent 경계가 반환한 Form6을 부모로 쓴다.
export function contentsProfile(profile) {
    const a = { type: 110, quantity: 9, listFlags: 0, listFlagsStored: false, contents: [], contentsComplete: true };
    const b = { type: 111, quantity: -3, listFlags: 0x20, listFlagsStored: true, contents: [], contentsComplete: true };
    if (profile === 1) return { items: [a, b], complete: true };
    if (profile === 2) return { items: [{ type: 112, quantity: 0, contents: [b], contentsComplete: true }], complete: true };
    return { items: [], complete: profile !== 3 };
}

// parseFort와 같은 레코드 모양을 입력 열에서 만든다. 출력 owner/frame은 계산하지 않는다.
export function record(v) {
    return { position: v[F.position] & 255, storedType: v[F.stored] & 255, type: v[F.type], frame: v[F.frame] & 255,
        quantityByte: v[F.qa] & 255, quantityWord: (v[F.qb] << 16) >> 16, bridgeFrame: v[F.bridgeFrame] & 255,
        legacyState: v[F.legacy] & 255, factoryState: v[F.factory] & 255, owner: v[F.owner] & 255, contents: contentsProfile(v[F.contents]) };
}

// 원본 관측기에 공급했던 명시적 장치/저장소만 준비한다. 완성 게임 월드의 대체 장치가 아니다.
// options.globals(FortSerializationGlobals)를 주면 다섯 저장 전역을 빌린 로더 상태로, options.depthOwner({value})를 주면 깊이만 빌린 상태로 만든다(기본은 독립 상태). (2026-11-11 WB04-E279: 공유 전역 대여 생성자 검사용 옵션 추가)
export function harness(v, options = {}) {
    const slots = new Map();
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, containerListFlags: 0, contentListFlags: 0, maxHitPoints: 0 }));
    const frames = Array.from({ length: 188 }, () => ({ codes: [] }));
    const random = new LocalRandom(v[F.seed]);
    const versionOwner = { value: v[F.version] };
    let state;
    if (options.globals) {
        options.globals.version = v[F.version];
        state = createFortObjectLoadStateFromGlobals(options.globals);
    } else state = createFortObjectLoadState(versionOwner, options.depthOwner ?? null);
    Object.assign(state, { islandType: 97, largeIslandType: 98, loadingDepth: 3, chunksX: 2, chunksY: 2,
        special: !!v[F.special], mission: !!v[F.mission], fort: !!v[F.fort], fortDestroyBlocked: !!v[F.blocked], chaffMode: !!v[F.chaffMode] });
    // 원본 fixture가 지정한 두 타입만 같은 flags/HP/프레임 프로필을 갖는다.
    for (const type of [v[F.type], 101]) {
        Object.assign(types[type], { flags1: v[F.flags1], flags2: v[F.flags2], maxHitPoints: 803 });
        frames[type] = frameProfile(v[F.frames]);
    }
    Object.assign(types[164], { flags1: 0x10, maxHitPoints: 803 });
    Object.assign(types[110], { flags1: 0x1000, contentListFlags: 0x110 });
    types[111].flags1 = 0x2000;
    Object.assign(types[112], { flags1: 0x20000, contentListFlags: 0x20 });
    // 같은 풀의 할당 슬롯 조회다. 없는 슬롯을 읽으면 실패한다.
    const slot = (sid) => { if (!slots.has(sid)) throw new RangeError(`slot ${sid}`); return slots.get(sid); };
    const pool = { edition: 'patch1078', slot, allocated: slot };
    const finder = { pool, begin() { return v[F.terrain] ? 50 : 0; }, next() { return 0; } };
    if (v[F.terrain]) {
        const raw = new Uint8Array(50);
        raw[10] = 97;
        put(raw, 36, v[F.terrain] === 2 ? 0xffffffff : 7);
        slots.set(50, raw);
        types[97].flags2 = 2;
    }
    const events = [];
    let born = 0, children = 0;
    const event = (name, ...args) => events.push([name, ...args.map((value) => value >>> 0)].join(':'));
    const hooks = {
        // fixture의 가상 factory는 지정 패턴을 쓰고 TYPE/void/연결 WORD만 덮는다.
        create(type, flags) {
            const sid = 100 + born++;
            const raw = Uint8Array.from({ length: 50 }, (_, index) => index * 37 + 11);
            put(raw, 0, 0x13000000);
            raw[10] = type; raw[11] = 4; raw[6] = raw[7] = 0;
            slots.set(sid, raw);
            event('C', type, flags, sid);
            return sid;
        },
        // 원본 관측의 출력 장치이며 setOwner/Pop의 의도된 현재 상태 변경도 그대로 적용한다.
        notifySurface(sid) { event('N', sid); },
        setState(sid, value) { event('S', sid, value); },
        setOwner(sid, value) {
            event('O', sid, value);
            slot(sid)[34] = value;
            if (v[F.mutation] === 1) slot(sid)[10] = 101;
            if (v[F.mutation] === 2) state.fortDestroyBlocked = true;
        },
        // 별도로 검증된 SquidReward의 HP 조회 경계를 원본 관측기의 입력 정책으로 제공한다.
        hasHitPoints(sid) { const raw = slot(sid), type = types[raw[10]]; return !!(type.flags2 & 0x400000 ? raw[40] & 1 : type.flags1 & 0x10); },
        maxHitPoints(sid) {
            const raw = slot(sid), type = types[raw[10]];
            if (type.flags2 & 0x400000) return raw[40] & 1 ? types[164].maxHitPoints : 0;
            return v[F.quarter] && raw[10] === 158 ? Math.trunc(type.maxHitPoints / 4) : type.maxHitPoints;
        },
        virtualCount(sid) { event('Q', sid); return v[F.qa] | 0; },
        pop(sid, x, y, flags) { event('P', sid, bits(x), bits(y), flags); if (v[F.mutation] === 3) types[slot(sid)[10]].flags1 = 0; },
        destroy(sid, flags) { event('D', sid, flags); },
        createContent(parent, type, quantity, listFlags, flags, quality) {
            const sid = 200 + children++;
            const raw = new Uint8Array(50);
            put(raw, 0, 0x13000000); raw[10] = 6; raw[11] = 8;
            slots.set(sid, raw);
            event('I', parent, type, quantity, listFlags, flags, quality, sid);
            return sid;
        },
        chunkRegion(x, y) { event('G', x, y); return (x + y * 2) % 3; },
        territoryAllowed(region, owner, territory) { event('A', region, owner, territory); return (region + territory + v[F.qa]) % 2 === 0; },
        loadingDepthChanged(depth) { assert.equal(depth, state.loadingDepth); },
        finishOwner(owner) { event('F', owner); },
    };
    return { pool, finder, types, frames, random, state, hooks, versionOwner, events, v,
        raws: () => Array.from({ length: born }, (_, i) => Buffer.from(slot(100 + i)).toString('hex')).join(';') || '-' };
}

