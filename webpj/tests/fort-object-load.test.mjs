// 원본 x86이 저장한 효과 순서·50바이트 슬롯·난수·전역을 그대로 대조한다.
// fixture의 생성/가상 장치만 재현하며 기대값을 JS로 다시 만들지 않는다. (2026-11-02 WB03-AB)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { FortObjectLoad, createFortObjectLoadState } from '../www/core/fort-object-load.js';
import { LocalRandom } from '../www/assets/terrain-mask.js';
import { parseFort } from '../www/assets/fort.js';
import { nameHash } from '../www/assets/type-flags.js';

// 원본 관측의 입력 열 이름이다. 출력 계산에는 사용하지 않는다.
const F = Object.freeze(Object.fromEntries(['stored', 'type', 'position', 'flags1', 'flags2', 'version', 'defaultOwner', 'owner',
    'special', 'mission', 'fort', 'blocked', 'quarter', 'seed', 'frames', 'terrain', 'mutation', 'originX', 'originY',
    'contents', 'frame', 'qa', 'qb', 'bridgeFrame', 'legacy', 'factory', 'chaffMode', 'slot'].map((name, index) => [name, index])));
// 실제 원본 기록의50바이트 슬롯과 동일한 little endian 쓰기/float 비트 관측이다.
function put(raw, offset, value, width = 4) {
    for (let byte = 0; byte < width; byte += 1) raw[offset + byte] = (value >>> (byte * 8)) & 255;
}
function bits(value) { const data = Buffer.alloc(4); data.writeFloatLE(value); return data.readUInt32LE(); }

// fixture 입력의 P/P, P/A, P/모든 변형, 없는 후보,256개 상한을 시험하는 프레임 표이다.
function frameProfile(profile) {
    const pairs = { 1: [[80, 80], [80, 80], [65, 65]], 2: [[80, 65], [80, 65]], 3: [[80, 67], [80, 68]], 4: [[65, 65]] };
    const codes = profile === 5 ? Array.from({ length: 300 }, (_, index) => ({ side: 80, variant: 80, number: index & 255, flags: index & 255 }))
        : (pairs[profile] ?? []).map(([side, variant], index) => ({ side, variant, number: index, flags: index === 1 ? 255 : 0 }));
    return { codes };
}

// native 입력 목록이다. 중첩 목록의 자식은 실제 createContent 경계가 반환한 Form6을 부모로 쓴다.
function contentsProfile(profile) {
    const a = { type: 110, quantity: 9, listFlags: 0, listFlagsStored: false, contents: [], contentsComplete: true };
    const b = { type: 111, quantity: -3, listFlags: 0x20, listFlagsStored: true, contents: [], contentsComplete: true };
    if (profile === 1) return { items: [a, b], complete: true };
    if (profile === 2) return { items: [{ type: 112, quantity: 0, contents: [b], contentsComplete: true }], complete: true };
    return { items: [], complete: profile !== 3 };
}

// parseFort와 같은 레코드 모양을 입력 열에서 만든다. 출력 owner/frame은 계산하지 않는다.
function record(v) {
    return { position: v[F.position] & 255, storedType: v[F.stored] & 255, type: v[F.type], frame: v[F.frame] & 255,
        quantityByte: v[F.qa] & 255, quantityWord: (v[F.qb] << 16) >> 16, bridgeFrame: v[F.bridgeFrame] & 255,
        legacyState: v[F.legacy] & 255, factoryState: v[F.factory] & 255, owner: v[F.owner] & 255, contents: contentsProfile(v[F.contents]) };
}

// 원본 관측기에 공급했던 명시적 장치/저장소만 준비한다. 완성 게임 월드의 대체 장치가 아니다.
function harness(v) {
    const slots = new Map();
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, containerListFlags: 0, contentListFlags: 0, maxHitPoints: 0 }));
    const frames = Array.from({ length: 188 }, () => ({ codes: [] }));
    const random = new LocalRandom(v[F.seed]);
    const versionOwner = { value: v[F.version] };
    const state = createFortObjectLoadState(versionOwner);
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

// 원본 fixture만 기대값으로 사용한다. 없는 환경은 원본 대조만 건너뛴다.
const fixturePath = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'fortobjectload-x86.tsv');
const rows = fs.existsSync(fixturePath) ? fs.readFileSync(fixturePath, 'utf8').split(/\r?\n/)
    .filter((line) => line.startsWith('originals\t')).map((line) => line.split('\t')) : null;

// 첫 fixture 입력의 변형은 후속 합성 경계 검사에만 사용한다.
function basic(overrides = {}) {
    const v = [100, 100, 227, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 32, 0, 23, 129, 65533, 17, 3, 4, 1, 1];
    for (const [name, value] of Object.entries(overrides)) v[F[name]] = value;
    return harness(v);
}

// 전체 관찰609줄: 객체573개·전체 순회36개이다. 기대 효과/슬롯/난수/전역을 어느 것도 재생성하지 않는다.
test('Fort 객체 로더: 원본10.78의609개 효과·전체 슬롯·난수·전역과 일치', (t) => {
    if (!rows) { t.skip('원본 fixture 없음'); return; }
    assert.equal(rows.length, 609);
    // 관찰마다 현재 자료/장치를 새로 만든다.
    for (const row of rows) {
        const v = row[2].split(',').map(Number);
        const scene = harness(v);
        const loader = new FortObjectLoad(scene);
        if (row[1] === 'O') loader.object(record(v), v[F.originX] | 0, v[F.originY] | 0, v[F.defaultOwner]);
        else {
            const fort = { territories: Array.from({ length: 20 }, () => ({ present: false, version: 0, chunks: [] })) };
            const section = () => ({ present: true, version: 2, chunks: Array.from({ length: 4 }, () => ({ objects: [{ ...record(v), position: 0x21 }] })) });
            fort.territories[0] = section(); fort.territories[1] = section(); fort.chaff = section();
            loader.load(fort, v[F.slot], 0, 20);
        }
        const state = [scene.state.version, scene.state.loadingDepth, scene.state.scope, Number(scene.state.fortDestroyBlocked)].join(',');
        assert.deepEqual([scene.events.join(';') || '-', scene.raws(), String(scene.random.state), state], row.slice(4), `${row[1]} ${row[2]}`);
    }
});

// 원본 packet을 Chaff에 넣고 입력 변환표를 TypeNames로 기록한다. 실제 해석은 기존 parseFort가 맡는다.
function packetFort(scene, packet) {
    const v = scene.v;
    // 입력 타입 이름은 변환표용 합성 식별자이며 게임 타입 이름을 바꾸는 구현은 아니다.
    scene.types.forEach((type, index) => { type.name = `load${index}`; });
    const count = Math.max(188, v[F.stored] + 1);
    const names = Buffer.alloc(1 + count * 4);
    names[0] = count;
    // 기본 타입 번호와 내용물 번호를 항등으로 연결한다.
    for (let index = 0; index < count; index += 1) names.writeUInt32LE(index < 188 ? nameHash(scene.types[index].name) : 0, 1 + index * 4);
    if (v[F.stored] < 246 && v[F.stored] !== v[F.type]) names.writeUInt32LE(v[F.type] ? nameHash(scene.types[v[F.type]].name) : 0, 1 + v[F.stored] * 4);
    const chunks = Buffer.concat([Buffer.from([v[F.version], 99, 1, 0]), Buffer.from(packet, 'hex')]);
    const sections = [Buffer.alloc(0), Buffer.alloc(0), Buffer.alloc(0), Buffer.alloc(0), chunks, Buffer.alloc(0), Buffer.alloc(0), names];
    const encoded = [Buffer.from([0x46, 0])];
    // 빈 섹션도 길이2로 기록해 원본 섹션 위치를 유지한다.
    for (const section of sections) {
        const length = Buffer.alloc(2);
        length.writeUInt16LE(section.length + 2);
        encoded.push(length, section);
    }
    return parseFort(new Uint8Array(Buffer.concat(encoded)), { records: scene.types, bridgeType: 82, islandType: 97 });
}

// 저장 packet을 읽은 레코드가 실제 로더에 들어가도 전체 원본 관측이 같아야 한다.
test('판독기→로더: 원본 저장 packet573개와 타입 변환·내용물 연결', (t) => {
    if (!rows) { t.skip('원본 fixture 없음'); return; }
    let count = 0;
    // 상위 순회L은 원본 packet 하나가 아니라 별도 template 입력이므로 객체O만 대조한다.
    for (const row of rows) {
        if (row[1] !== 'O') continue;
        const scene = harness(row[2].split(',').map(Number));
        const fort = packetFort(scene, row[3]);
        assert.equal(fort.trailingBytes, 0);
        const object = fort.chaff.chunks[0].objects[0];
        new FortObjectLoad(scene).object(object, scene.v[F.originX] | 0, scene.v[F.originY] | 0, scene.v[F.defaultOwner]);
        assert.deepEqual([scene.events.join(';') || '-', scene.raws(), String(scene.random.state),
            [scene.state.version, scene.state.loadingDepth, scene.state.scope, Number(scene.state.fortDestroyBlocked)].join(',')], row.slice(4), row[2]);
        count += 1;
    }
    assert.equal(count, 573);
});

// 필수 의존과 선택 필드 누락은 해당 첫 분기에서 명시적으로 실패한다.
test('로더 계약: 필수 효과·다른 풀·판본·표 크기·선택 필드 누락', () => {
    const scene = basic({ flags1: 0x20, version: 2 });
    assert.throws(() => new FortObjectLoad({ ...scene, hooks: { ...scene.hooks, pop: null } }), TypeError);
    assert.throws(() => new FortObjectLoad({ ...scene, finder: { ...scene.finder, pool: {} } }), TypeError);
    assert.throws(() => new FortObjectLoad({ ...scene, frames: [] }), TypeError);
    assert.throws(() => new FortObjectLoad({ ...scene, pool: { ...scene.pool, edition: 'cd1072' } }), TypeError);
    assert.throws(() => createFortObjectLoadState(), TypeError);
    const loader = new FortObjectLoad(scene);
    assert.throws(() => loader.object({ ...record(scene.v), frame: null }, 16, 32, 1), /frame/);
    assert.equal(scene.events.length, 1); // 누락이 발견되기 전의 원본 create 효과는 남는다.
    assert.throws(() => loader.load({}, 1, 0, 0), RangeError);
    assert.equal(scene.state.loadingDepth, 3);
});

// 공유 판정은 Pop/Contents 뒤 현재 값을 읽으며 독립 fallback에 가려지지 않는다.
test('공유 전역: Pop/내용물 뒤 FullDeck·fortKeep 변경과 판본의 같은 소유자', () => {
    // 같은 입력에서 공유0과비영DWORD를 각각 현재 삭제 관문에 전달한다.
    for (const afterContents of [false, true]) for (const value of [0, 0x80000000, 0xffffffff]) {
        const scene = basic({ flags1: afterContents ? 0x20000 : 0, version: 2, fort: 1, blocked: 1, contents: afterContents ? 1 : 0 });
        let fullDeck = value ? 0 : 0xffffffff;
        scene.state.readFullDeck = () => fullDeck;
        const name = afterContents ? 'createContent' : 'pop';
        const original = scene.hooks[name];
        scene.hooks[name] = (...args) => { const result = original(...args); fullDeck = value; return result; };
        new FortObjectLoad(scene).object(record(scene.v), 16, 32, 1);
        assert.equal(scene.events.filter((event) => event.startsWith('D:')).length, value ? 0 : 1);
    }
    const scene = basic({ flags2: 2, version: 2, fort: 1 });
    let keep = 0;
    scene.state.readFortKeepGenus = () => keep;
    const original = scene.hooks.pop;
    scene.hooks.pop = (...args) => { original(...args); keep = 2; };
    new FortObjectLoad(scene).object(record(scene.v), 16, 32, 1);
    assert.ok(!scene.events.some((event) => event.startsWith('D:')));
    scene.versionOwner.value = 9;
    assert.equal(scene.state.version, 9);
    scene.state.version = 0xffffffff;
    assert.equal(scene.versionOwner.value, 0xffffffff);
});

// 로더의 실제 부모 예외 경계는 앞선 정상 효과를 남기고 loadingDepth를 복구한다.
test('상위 순회: 청크 소진·소유자 마감 오류 뒤 깊이 복구와 scope/판본 보존', () => {
    const scene = basic({ version: 7 });
    const loader = new FortObjectLoad(scene);
    const fort = { territories: [{ present: true, version: 2, chunks: [] }], chaff: { present: false, version: 0 } };
    assert.throws(() => loader.load(fort, 1, 0, 1), /소진/);
    assert.equal(scene.state.loadingDepth, 3);
    assert.equal(scene.state.scope, 1);
    assert.equal(scene.versionOwner.value, 2);
    assert.ok(scene.events.length > 0);
    scene.hooks.finishOwner = () => { throw new Error('finish'); };
    fort.territories[0] = { present: false, version: 3 };
    assert.throws(() => loader.load(fort, 1, 0, 1), /finish/);
    assert.equal(scene.state.loadingDepth, 3);
});

// 가상 조회가 풀을 재할당해도 먼저 빌려 둔 오래된 바이트에 HP를 쓰지 않는다.
test('현재 슬롯: 가상 HP 조회 뒤 교체된 저장소에 좁은 HP 쓰기', () => {
    const scene = basic({ flags1: 0x10, version: 2 });
    const slot = scene.pool.slot;
    let previous, replacement;
    scene.hooks.maxHitPoints = (sid) => {
        previous = slot(sid);
        replacement = previous.slice();
        scene.pool.slot = scene.pool.allocated = (number) => number === sid ? replacement : slot(number);
        return 123;
    };
    new FortObjectLoad(scene).object(record(scene.v), 16, 32, 1);
    assert.equal(Buffer.from(replacement).readInt32LE(26), 123);
    assert.notEqual(Buffer.from(previous).readInt32LE(26), 123);
    assert.deepEqual(replacement.slice(0, 26), previous.slice(0, 26));
    assert.deepEqual(replacement.slice(30), previous.slice(30));
});
