// 초기 terrain frame 선택과 stamp(cpppj `o/RawTerrainStamp`, 패치 004c04b0/004c0640/004bfe50)의 웹 대응 `core/raw-terrain-stamp.js` 를 검사한다(4-B, WB04-E295).
//  - 고정 관측 `terrainstamp-x86.tsv` originals 780행(Frame 600·Stamp 180): 같은 raw 풀(client 슬롯 5..127)·두 난수·99 캐시·현재 지도/타입 표/프레임 코드 표 입력에서 프레임 번호·게임 난수·CRT 상태·99 캐시 전체·
//    외부 효과(factory/owner/Pop/지역/chunkRegion/진단)의 인자와 순서·생성한 모든 raw 슬롯의 type/frame/owner/region/좌표·마지막 SID·mapping 세 항목·진단 수가 같다. CD 행은 웹 목표(10.78) 밖이다.
//    모든 기대 값은 원본 PE 관측에서 읽고 이 시험이 결과를 새로 계산하지 않는다(입력 지도/코드 표/이웃 mask 만 만든다).
//  - 계약: 캐시 보존(같은 월드 안의 두 번째 호출과 새 실행기/새 지도/새 풀에서도 CRT 를 다시 소비하지 않음), 필수 하위 효과/크기/종류 거부, stamp code 종류 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E295)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { FrameCodeTable, chooseTerrainFrame, stampCoreFrame } from '../www/assets/terrain-tiles.js';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { setSquidRegion } from '../www/core/raw-squid-region.js';
import { RawTerrainStamp, TerrainStampAssertion, TerrainStampState } from '../www/core/raw-terrain-stamp.js';

// 원본 관측 행(originals 만)이다. 파일이 없으면 null 이라 해당 검사를 건너뛴다. (2026-11-11 WB04-E295)
const ROWS = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'terrainstamp-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 원본 함수에 넣은 실제 타입 번호와 factory 가 반환하기 시작하는 공간 번호다. (2026-11-11 WB04-E295)
const TERRAIN_ISLE = 97;
const TERRAIN_EDGE = 98;
const TERRAIN_FRINGE = 102;
const TERRAIN_OTHER = 94;
const TERRAIN_BORN = 40;
const TERRAIN_END = 127;

// 원본이 읽는 코드 저장 순서이며 판단/선택 결과는 계산하지 않는다. dataset 0: 256쌍 전체, 1: 빈 표, 2: 최대 256 후보를 넘는 동일 코드 300개, 3: 희소 코드, 4: 본섬은 전체·fringe 는 빈 표. (2026-11-11 WB04-E295)
function terrainCodes(dataset, fringe) {
    if (dataset === 1 || (dataset === 4 && fringe)) return new FrameCodeTable([]);
    const codes = [];
    if (dataset === 4) dataset = 0;
    if (dataset === 2) {
        // 최대 256 후보를 넘는 동일 code 300개를 원본과 같은 순서로 준다.
        for (let index = 0; index < 300; index += 1) codes.push({ side: 0x41, variant: 0x41, number: index & 0xff, flags: 0 });
        return new FrameCodeTable(codes);
    }
    // 정확 검색/정규화/side,A/side,wildcard 를 서로 다른 sparse 표에서도 검사한다.
    const append = (side, variant) => {
        const flags = [0, 2, 0x80, 1]; // 실제 fringe signed flags 입력이다.
        // 같은 code 의 8개 후보는 patch quartile 의 차이를 가진다.
        for (let index = 0; index < 8; index += 1) codes.push({ side, variant, number: index, flags: fringe ? flags[index % 4] : 1 });
    };
    if (dataset === 0) {
        // 물리 코드의 side 외부/variant 내부 순서도 실제 테이블과 동일하다.
        for (let side = 0x41; side <= 0x50; side += 1) for (let variant = 0x41; variant <= 0x50; variant += 1) append(side, variant);
    } else {
        // sparse 코드의 원본 입력 순서다.
        for (const side of [0x41, 0x43, 0x46, 0x4a, 0x50]) append(side, 0x41);
    }
    return new FrameCodeTable(codes);
}

// 원본의 현재 지도·두 난수·전역 cache 와 단일 raw 슬롯을 조립한 장면이다. (2026-11-11 WB04-E295)
class TerrainStampScene {
    constructor() {
        // 같은 factory/owner/Pop/region 의 raw 저장소이며 fixture 가 생성할 모든 SID 는 행별로 재사용한다.
        this.pool = new SidPool(32768);
        for (let sid = 5; sid <= TERRAIN_END; sid += 1) assert.equal(this.pool.allocate(2), sid);
        this.types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
        this.frames = Array.from({ length: 188 }, () => new FrameCodeTable([]));
        this.codes = new Uint8Array(65536);
        this.regions = new Uint8Array(65536);
        this.mapping = new Int32Array(256);
        this.aliases = new Int32Array(256);
        this.map = { codes: this.codes, regions: this.regions, typeMapping: this.mapping, aliases: this.aliases, bounds: [0, 0, 0, 0], makePredictable: 0, lastSid: { value: 0 }, excludedFlags: 0 };
        this.state = new TerrainStampState();
        this.random = new GameRandom(0);
        this.crt = { value: 0 };
        this.input = [];
        this.events = [];
        this.born = 0;
        this.assertions = 0;
        this.kind = '';
        this.stamp = null;
    }

    // 지도 밖 이웃은 쓰지 않고 원본이 bounds 검사로 배제하게 둔다. (2026-11-11 WB04-E295)
    cell(x, y, code, region = 5) {
        if (x < 0 || x >= 256 || y < 0 || y >= 256) return;
        this.codes[x * 256 + y] = code;
        this.regions[x * 256 + y] = region;
    }

    // 필수 하위 경계의 동일 입력/생성 raw 효과를 설치한다. 전체 파생 factory/Pop 을 복원한 검사로 세지 않는다. (2026-11-11 WB04-E295)
    hooks() {
        return {
            create: (type, predictable) => {
                const sid = this.born;
                this.born += 1;
                const raw = this.pool.allocated(sid);
                raw.fill(0);
                raw[10] = type & 0xff;
                this.events.push(`C:${sid}:${type}:${predictable}`);
                return sid;
            },
            setOwner: (sid, owner) => {
                this.pool.allocated(sid)[34] = owner & 0xff;
                this.events.push(`O:${sid}:${owner}`);
            },
            pop: (sid, x, y, flags) => {
                const raw = this.pool.allocated(sid);
                const bx = floatToBits(x);
                const by = floatToBits(y);
                put(raw, 14, bx);
                put(raw, 18, by);
                this.events.push(`P:${sid}:${bx}:${by}:${flags}`);
            },
            setRegion: (sid, region) => {
                this.events.push(`R:${sid}:${region >>> 0}`);
                setSquidRegion(this.pool, this.types, sid, region >>> 0, this.state.noRegion >>> 0, () => { this.assertions += 1; this.events.push('A'); });
            },
            chunkRegion: (x, y) => { this.events.push(`H:${x}:${y}`); return 6; },
            assertion: () => { this.assertions += 1; this.events.push('A'); },
        };
    }

    // 모든 입력을 원본과 같은 raw 지도/타입/캐시/두 난수로 구성한다. (2026-11-11 WB04-E295)
    prepare(row) {
        this.kind = row[1];
        this.input = row[2].split(',').map(Number);
        this.events = [];
        this.assertions = 0;
        this.born = TERRAIN_BORN;
        this.codes.fill(0);
        this.regions.fill(0);
        this.mapping.fill(0);
        this.aliases.fill(0);
        this.state = new TerrainStampState();
        this.map.lastSid = { value: 0 };
        const frame = this.kind === 'Frame';
        // 원본의 case 별 raw 초기화와 같은, 사전에 확보한 슬롯을 비운다.
        for (let sid = 5; sid <= TERRAIN_END; sid += 1) this.pool.allocated(sid).fill(0);
        const dataset = this.input[frame ? 7 : 1];
        const cache = this.input[frame ? 8 : 2];
        this.crt.value = this.input[frame ? 5 : 3];
        this.random.setState(this.input[frame ? 6 : 4]);
        if (cache) {
            this.state.variants = new Int32Array(99);
            // 이미 존재하는 전역 cache 는 명시 입력이며 새 CRT 소비가 아니다.
            for (let index = 0; index < 99; index += 1) this.state.variants[index] = (index * 17 + 3) % 32768;
        }
        // 독립 원본에 넣은 네 타입의 코드/genus 를 같은 생산 표에 전달한다.
        for (const type of [TERRAIN_ISLE, TERRAIN_EDGE, TERRAIN_FRINGE, TERRAIN_OTHER]) {
            this.types[type] = { flags2: type === TERRAIN_ISLE ? 2 : 0 };
            this.frames[type] = terrainCodes(dataset, type === TERRAIN_FRINGE);
        }
        this.mapping[0x78] = TERRAIN_ISLE;
        this.mapping[0x3d] = TERRAIN_EDGE;
        this.mapping[0x3f] = TERRAIN_OTHER;
        this.mapping[0x68] = TERRAIN_ISLE;
        this.aliases[0x78] = 0x78;
        this.aliases[0x3d] = 0x3d;
        this.aliases[0x3f] = 0x3f;
        this.aliases[0x68] = 0x78;
        if (frame) {
            assert.equal(this.input.length, 10);
            const x = this.input[3] | 0;
            const y = this.input[4] | 0;
            this.map.bounds = [0, 0, 255, 255];
            this.map.makePredictable = 1;
            this.map.excludedFlags = 0;
            this.cell(x, y, this.input[0]);
            // native 입력과 같은 여덟 이웃 mask/alias/지역 불일치만 넣는다.
            [[0, -1], [1, -1], [1, 0], [1, 1], [0, 1], [-1, 1], [-1, 0], [-1, -1]].forEach(([dx, dy], index) => {
                const mask = this.input[index % 2 ? 2 : 1];
                this.cell(x + dx, y + dy, (mask & (1 << Math.trunc(index / 2))) ? 0x68 : 0x3f, this.input[9] ? 6 : 5);
            });
        } else {
            assert.equal(this.input.length, 8);
            this.map.bounds = [50, 53, 52, 55];
            this.map.makePredictable = this.input[6];
            this.map.excludedFlags = this.input[5];
            this.cell(50, 53, 0x78);
            this.cell(51, 53, 0x78);
            this.cell(50, 54, 0x78);
            this.cell(51, 54, 0x78);
            if (this.input[0] === 1) this.cell(51, 53, 0x3f);
            if (this.input[0] === 2) { this.cell(50, 53, 0x68); this.mapping[0x68] = 0; }
            if (this.input[0] === 3) this.cell(51, 54, 0x68, 6);
            if (this.input[0] === 4) this.map.bounds[2] = 50;
        }
        this.stamp = new RawTerrainStamp(this.pool, this.types, this.frames, this.map, this.random, this.crt, this.state, this.hooks());
    }
}

// 패치의 theme 0 초기 검색·직접 DWORD frame·지역/fringe 생성을 독립 원본과 비교한다(780행). (2026-11-11 WB04-E295)
test('초기 terrain stamp 는 관측한 프레임·두 난수·99 캐시·외부 효과·raw 슬롯을 그대로 재생한다', { skip: ROWS === null }, () => {
    const scene = new TerrainStampScene();
    let count = 0;
    // 저장된 native 결과와 생산 모듈의 현재 결과만 비교한다.
    for (const row of ROWS) {
        assert.equal(row.length, 12);
        scene.prepare(row);
        let result = 0;
        if (scene.kind === 'Frame') result = scene.stamp.frameAt(scene.input[3] | 0, scene.input[4] | 0) >>> 0;
        else scene.stamp.stamp(0x78);
        const cache = scene.state.variants ? Array.from(scene.state.variants).join(',') : '-';
        const effects = scene.events.length ? scene.events.join(';') : '-';
        const slots = [];
        // 생성한 모든 raw 슬롯의 type/frame/owner/region/좌표를 실제 폭으로 읽는다.
        for (let sid = TERRAIN_BORN; sid < scene.born; sid += 1) {
            const slot = scene.pool.slot(sid);
            slots.push(`${sid}:${slot[10]}:${get(slot, 36, 4)}:${slot[34]}:${get(slot, 8, 2)}:${get(slot, 14)}:${get(slot, 18)}`);
        }
        const raw = slots.length ? slots.join(';') : '-';
        const mapping = `${scene.mapping[0x78]},${scene.mapping[0x58]},${scene.mapping[0x3d]}`;
        count += 1;
        const label = `행 ${count} ${row[1]} ${row[2]}`;
        assert.equal(result, Number(row[3]), `${label} 프레임`);
        assert.equal(scene.random.state, Number(row[4]), `${label} 게임 난수`);
        assert.equal(scene.crt.value, Number(row[5]), `${label} CRT`);
        assert.equal(cache, row[6], `${label} 99 캐시`);
        assert.equal(effects, row[7], `${label} 외부 효과`);
        assert.equal(raw, row[8], `${label} raw 슬롯`);
        assert.equal(scene.map.lastSid.value, Number(row[9]), `${label} 마지막 SID`);
        assert.equal(mapping, row[10], `${label} mapping`);
        assert.equal(scene.assertions, Number(row[11]), `${label} 진단 수`);
    }
    assert.equal(count, 780);
});

// 같은 모듈에서 cache 가 재사용될 때 CRT 가 다시 소비되지 않으며 미연결 하위 효과를 거부한다. (2026-11-11 WB04-E295)
test('초기 terrain stamp 는 캐시를 재사용하고 미연결 효과/입력 종류를 거부한다', { skip: ROWS === null }, () => {
    const scene = new TerrainStampScene();
    scene.prepare(ROWS[0]);
    scene.stamp.frameAt(50, 53);
    const crt = scene.crt.value;
    const cache = Int32Array.from(scene.state.variants);
    scene.stamp.frameAt(50, 53);
    assert.equal(scene.crt.value, crt);
    assert.deepEqual([...scene.state.variants], [...cache]);
    const make = (changes = {}) => new RawTerrainStamp(changes.pool ?? scene.pool, changes.types ?? scene.types, changes.frames ?? scene.frames, changes.map ?? scene.map,
        changes.random ?? scene.random, changes.crt ?? scene.crt, changes.state ?? scene.state, changes.hooks ?? scene.hooks());
    make();
    // 필수 하위 효과 여섯 개 중 하나라도 없으면 거부한다.
    for (const name of ['create', 'setOwner', 'pop', 'setRegion', 'chunkRegion', 'assertion']) {
        const hooks = scene.hooks();
        delete hooks[name];
        assert.throws(() => make({ hooks }), TypeError, name);
    }
    assert.throws(() => make({ hooks: {} }), TypeError);
    assert.throws(() => make({ hooks: undefined, pool: {} }), TypeError);
    assert.throws(() => make({ types: scene.types.slice(1) }), TypeError);
    assert.throws(() => make({ frames: scene.frames.slice(1) }), TypeError);
    assert.throws(() => make({ map: { ...scene.map, codes: new Uint8Array(100) } }), TypeError);
    assert.throws(() => make({ map: { ...scene.map, regions: new Uint8Array(100) } }), TypeError);
    assert.throws(() => make({ map: { ...scene.map, typeMapping: new Int32Array(10) } }), TypeError);
    assert.throws(() => make({ map: { ...scene.map, aliases: new Int32Array(10) } }), TypeError);
    assert.throws(() => make({ map: { ...scene.map, bounds: null } }), TypeError);
    assert.throws(() => make({ random: {} }), TypeError);
    assert.throws(() => make({ crt: 5 }), TypeError);
    assert.throws(() => make({ state: {} }), TypeError);
    assert.throws(() => scene.stamp.stamp(1.5), TypeError);
    assert.throws(() => scene.stamp.stamp(200), TypeError);
    assert.equal(scene.stamp.pool, scene.pool);
});

// Client 의 99 캐시는 새 실행기/지도/풀로 바뀌어도 재사용되어 두 번째 99회 소비를 막는다(원본 x86 AA 프레임 행으로 확인). (2026-11-11 WB04-E295)
test('초기 terrain stamp 의 전역 캐시는 새 실행기와 새 장면에서도 다시 만들지 않는다', { skip: ROWS === null }, () => {
    const found = ROWS.find((row) => row[1] === 'Frame' && row[2] === '120,15,15,50,53,0,0,0,0,0');
    assert.ok(found);
    const fresh = new TerrainStampState();
    assert.equal(fresh.variants, null);
    const first = new TerrainStampScene();
    first.prepare(found);
    const state = first.state;
    // 첫 장면의 실행기가 만든 캐시가 상태 객체에 남는다.
    assert.equal(first.stamp.frameAt(50, 53), Number(found[3]));
    assert.equal(first.crt.value, Number(found[5]));
    assert.equal(Array.from(state.variants).join(','), found[6]);
    const cache = Int32Array.from(state.variants);
    // 새 장면(새 지도/풀/실행기)이 같은 상태 객체와 다른 CRT 를 쓰면 CRT 를 소비하지 않고 같은 프레임을 낸다.
    const second = new TerrainStampScene();
    second.prepare(found);
    second.crt.value = 0x87654321;
    const stamp = new RawTerrainStamp(second.pool, second.types, second.frames, second.map, second.random, second.crt, state, second.hooks());
    assert.equal(stamp.frameAt(50, 53), Number(found[3]));
    assert.equal(second.crt.value, 0x87654321);
    assert.deepEqual([...state.variants], [...cache]);
    assert.equal(TerrainStampAssertion.MissingRegion, 'MissingRegion');
    assert.equal(TerrainStampAssertion.UnpredictableFringe, 'UnpredictableFringe');
});

// ───────────── 변형 확인으로 보강한 계약 ─────────────

// 합성 행으로 장면을 만든다: Frame 입력은 [중심 문자, 직선 mask, 대각 mask, x, y, CRT, 게임 난수, dataset, 캐시, 지역 불일치], Stamp 입력은 [profile, dataset, 캐시, CRT, 게임 난수, 제외 flags, makePredictable, 0]. (2026-11-11 WB04-E295)
function sceneOf(kind, input) {
    const scene = new TerrainStampScene();
    scene.prepare(['originals', kind, input.join(',')]);
    return scene;
}

// 안쪽 칸('x', 두 방향 글자 모두 'A')의 프레임은 첫 'J' 구간 뒤 3×3 묶음 계산이며, 좌표 x/y 의 역할이 바뀌지 않는다(독립 계산: 타입 모델 stampCoreFrame). (2026-11-11 WB04-E295)
test('초기 terrain stamp 의 안쪽 칸 프레임은 3×3 묶음 계산을 따른다', () => {
    for (const [x, y] of [[50, 53], [51, 53], [52, 55], [53, 54], [100, 101], [17, 200], [3, 254], [200, 2], [66, 67]]) {
        const scene = sceneOf('Frame', [0x78, 15, 15, x, y, 12345, 7, 0, 0, 0]);
        const frame = scene.stamp.frameAt(x, y);
        assert.equal(frame, stampCoreFrame(scene.frames[TERRAIN_ISLE], Array.from(scene.state.variants), x, y), `${x},${y}`);
    }
});

// 안쪽 판정은 'x' 문자에만 쓴다: 같은 이웃을 가진 '=' 칸은 방향 'A','A' 후보 선택(theme 0)이다. 원문의 alias 는 signed char 로 읽어 DWORD alias 가 0x80 이상이면 이웃과 같다고 보지 않는다. (2026-11-11 WB04-E295)
test('초기 terrain stamp 는 안쪽 판정을 x 로 한정하고 alias 를 signed 로 읽는다', () => {
    // '=' 칸(타입 98)과 여덟 이웃이 모두 '=': 방향 A/A 이지만 안쪽 묶음이 아니라 theme 0 후보 선택이다.
    const scene = sceneOf('Frame', [0x3d, 0, 0, 120, 130, 99, 1234, 0, 0, 0]);
    for (const [dx, dy] of [[0, -1], [1, -1], [1, 0], [1, 1], [0, 1], [-1, 1], [-1, 0], [-1, -1]]) scene.cell(120 + dx, 130 + dy, 0x3d);
    const oracle = new GameRandom(1234);
    const expected = chooseTerrainFrame(scene.frames[TERRAIN_EDGE], 'A', 'A', 0, oracle);
    assert.equal(scene.stamp.frameAt(120, 130), expected);
    assert.equal(scene.random.state, oracle.state);
    // alias DWORD 가 0xe9 이면 중심의 signed alias(-23)가 이웃의 alias(233)와 달라 이웃이 하나도 맞지 않는다: 방향 P/P 후보 선택이다.
    const quirk = sceneOf('Frame', [0x68, 0, 0, 120, 130, 99, 1234, 0, 0, 0]);
    quirk.aliases[0x68] = 0xe9;
    for (const [dx, dy] of [[0, -1], [1, -1], [1, 0], [1, 1], [0, 1], [-1, 1], [-1, 0], [-1, -1]]) quirk.cell(120 + dx, 130 + dy, 0x68);
    const quirkOracle = new GameRandom(1234);
    const quirkExpected = chooseTerrainFrame(quirk.frames[TERRAIN_ISLE], 'P', 'P', 0, quirkOracle);
    assert.equal(quirk.stamp.frameAt(120, 130), quirkExpected);
    assert.equal(quirk.random.state, quirkOracle.state);
});

// 지도 범위와 문자 범위를 가장자리로 끌어당기지 않고 거부한다. (2026-11-11 WB04-E295)
test('초기 terrain stamp 는 지도 밖 좌표/signed 문자/타입 범위를 거부한다', () => {
    const scene = sceneOf('Frame', [0x78, 0, 0, 120, 130, 99, 1234, 0, 0, 0]);
    assert.throws(() => scene.stamp.frameAt(256, 10), /지도 범위/);
    assert.throws(() => scene.stamp.frameAt(10, 256), /지도 범위/);
    assert.throws(() => scene.stamp.frameAt(-1, 10), /지도 범위/);
    assert.throws(() => scene.stamp.frameAt(10, -1), /지도 범위/);
    assert.throws(() => scene.stamp.frameAt(1.5, 10), /지도 범위/);
    // 가장자리 칸(255)은 정상이다: 문자 0 은 mapping 0(타입 0)이라 프레임 0.
    assert.equal(scene.stamp.frameAt(255, 255), 0);
    scene.codes[120 * 256 + 130] = 0x80;
    assert.throws(() => scene.stamp.frameAt(120, 130), /signed 문자/);
    scene.codes[120 * 256 + 130] = 0x7f;
    scene.stamp.frameAt(120, 130);
    // mapping 타입이 프레임 표 밖이면 거부한다(-1, 188).
    scene.codes[120 * 256 + 130] = 0x78;
    scene.mapping[0x78] = -1;
    assert.throws(() => scene.stamp.frameAt(120, 130), /frame 타입 범위/);
    scene.mapping[0x78] = 188;
    assert.throws(() => scene.stamp.frameAt(120, 130), /frame 타입 범위/);
    scene.mapping[0x78] = 187;
    scene.stamp.frameAt(120, 130);
});

// stamp 순회는 반열린 Y 외부/X 내부 bounds 이다: 위/아래/오른쪽 경계 밖의 'x' 칸은 만들지 않고, 지역 BYTE 는 signed 로 setRegion 에 전달한다. (2026-11-11 WB04-E295)
test('초기 terrain stamp 는 bounds 안쪽의 칸만 만들고 지역을 signed 로 전달한다', () => {
    const scene = sceneOf('Stamp', [0, 0, 0, 1, 1, 0, 1, 0]);
    // 경계 밖의 'x' 칸: 위(y 52), 아래(y 55 = bottom), 오른쪽(x 52 = right), 왼쪽(x 49).
    for (const [x, y] of [[50, 52], [50, 55], [52, 53], [52, 54], [49, 53]]) scene.cell(x, y, 0x78);
    // 안쪽 네 칸의 지역을 0xff(signed -1)로 둔다.
    for (const [x, y] of [[50, 53], [51, 53], [50, 54], [51, 54]]) scene.cell(x, y, 0x78, 0xff);
    scene.stamp.stamp(0x78);
    assert.equal(scene.events.filter((event) => event.startsWith(`C:`) && event.endsWith(`:${TERRAIN_ISLE}:1`)).length, 4);
    const regionEvents = scene.events.filter((event) => event.startsWith('R:'));
    assert.ok(regionEvents.length >= 4);
    assert.ok(regionEvents.every((event) => event.endsWith(`:${0xffffffff}`)), regionEvents.join(' '));
    // 처음 만든 칸은 (50,53), 이어서 (51,53), (50,54), (51,54) 순서의 좌표 Pop 이다(첫 Pop 은 x 50 → float bits).
    const pops = scene.events.filter((event) => event.startsWith('P:'));
    assert.equal(pops[0].split(':')[2], String(floatToBits(50)));
    // 시작 행은 top(bounds[1])이며 left 값에 의존하지 않는다: left(60)가 bottom(55)보다 커도 안쪽 네 칸을 모두 만든다.
    const shifted = sceneOf('Stamp', [0, 0, 0, 1, 1, 0, 1, 0]);
    shifted.codes.fill(0);
    shifted.map.bounds[0] = 60;
    shifted.map.bounds[1] = 53;
    shifted.map.bounds[2] = 62;
    shifted.map.bounds[3] = 55;
    for (const [x, y] of [[60, 53], [61, 53], [60, 54], [61, 54]]) shifted.cell(x, y, 0x78);
    shifted.stamp.stamp(0x78);
    assert.equal(shifted.events.filter((event) => event.startsWith('C:') && event.endsWith(`:${TERRAIN_ISLE}:1`)).length, 4);
});

// 지역이 noRegion 이면 패치는 진단 뒤 현재 ChunkMap 의 x/16, y/16 칸 지역을 조회해 쓴다. (2026-11-11 WB04-E295)
test('초기 terrain stamp 는 지역 없음이면 진단 뒤 청크 지역을 조회해 설정한다', () => {
    const scene = sceneOf('Stamp', [0, 0, 0, 1, 1, 0, 1, 0]);
    scene.map.bounds[0] = 50;
    scene.map.bounds[1] = 70;
    scene.map.bounds[2] = 52;
    scene.map.bounds[3] = 72;
    scene.codes.fill(0);
    for (const [x, y] of [[50, 70], [51, 70], [50, 71], [51, 71]]) scene.cell(x, y, 0x78, 127);
    scene.stamp.stamp(0x78);
    assert.equal(scene.events.filter((event) => event === 'H:3:4').length, 4);
    assert.equal(scene.events.filter((event) => event.startsWith('H:')).length, 4);
    // 칸마다 진단 → 청크 조회 → 지역 설정(청크 지역 6) 순서이다.
    const first = scene.events.indexOf('H:3:4');
    assert.equal(scene.events[first - 1], 'A');
    assert.match(scene.events[first + 1], /^R:\d+:6$/);
    assert.ok(scene.assertions >= 4);
});

// makePredictable 는 매 생성 호출에 그대로 전달되고 0 이면 fringe 직전에 진단한다. (2026-11-11 WB04-E295)
test('초기 terrain stamp 는 makePredictable 을 factory 마다 전달한다', () => {
    const scene = sceneOf('Stamp', [0, 0, 0, 1, 1, 0, 7, 0]);
    scene.stamp.stamp(0x78);
    const creates = scene.events.filter((event) => event.startsWith('C:'));
    assert.ok(creates.some((event) => event.endsWith(`:${TERRAIN_ISLE}:7`)));
    assert.ok(creates.some((event) => event.endsWith(`:${TERRAIN_FRINGE}:7`)));
    assert.ok(creates.every((event) => event.endsWith(':7')));
    // 0 이면 fringe 직전에 진단하고 계속 만든다.
    const zero = sceneOf('Stamp', [0, 0, 0, 1, 1, 0, 0, 0]);
    zero.stamp.stamp(0x78);
    assert.ok(zero.assertions > 0);
    assert.ok(zero.events.some((event) => event.startsWith('C:') && event.endsWith(`:${TERRAIN_FRINGE}:0`)));
});

// 범위 방어: 표 밖 생성 타입, 빈 프레임 표, 표 밖 fringe 타입은 거부한다. (2026-11-11 WB04-E295)
test('초기 terrain stamp 는 생성 타입/프레임/fringe 타입 범위를 거부한다', () => {
    // factory 가 표 밖 타입(188)의 슬롯을 돌려주면 거부한다.
    const bad = sceneOf('Stamp', [0, 0, 0, 1, 1, 0, 1, 0]);
    const hooks = bad.hooks();
    const create = hooks.create;
    hooks.create = (type, predictable) => { const sid = create(type, predictable); bad.pool.allocated(sid)[10] = 188; return sid; };
    const stamp = new RawTerrainStamp(bad.pool, bad.types, bad.frames, bad.map, bad.random, bad.crt, bad.state, hooks);
    assert.throws(() => stamp.stamp(0x78), /생성 타입 범위/);
    // 프레임 표가 비어 있으면(프레임 0) 지면 genus 의 코드 조회가 범위 오류이다.
    const empty = sceneOf('Stamp', [0, 1, 0, 1, 1, 0, 1, 0]);
    assert.throws(() => empty.stamp.stamp(0x78), /생성 프레임 범위/);
    // fringe 타입이 표 크기(188) 이상이면 거부한다.
    const fringe = sceneOf('Stamp', [0, 0, 0, 1, 1, 0, 1, 0]);
    fringe.state.fringeType = 188;
    assert.throws(() => fringe.stamp.stamp(0x78), /fringe 타입 범위/);
    fringe.state.fringeType = 187;
    fringe.frames[187] = terrainCodes(0, true);
    fringe.stamp.stamp(0x78);
});

// 프레임 DWORD 는 4바이트 전체를 쓴다: 16비트를 넘는 프레임 번호도 보존한다(첫 'J' 구간이 뒤쪽에 있는 큰 표). (2026-11-11 WB04-E295)
test('초기 terrain stamp 는 프레임 번호를 DWORD 전체로 쓴다', () => {
    const scene = sceneOf('Stamp', [0, 0, 0, 1, 1, 0, 1, 0]);
    const codes = [];
    // 앞쪽 65,600개 채움 코드, 그 뒤 'J' 구간과 3×3 묶음 자리.
    for (let index = 0; index < 65600; index += 1) codes.push({ side: 0x41, variant: 0x41, number: index & 0xff, flags: 0 });
    codes.push({ side: 0x4a, variant: 0x4a, number: 0, flags: 0 });
    for (let index = 0; index < 144; index += 1) codes.push({ side: 0x41, variant: 0x41, number: index, flags: 0 });
    scene.frames[TERRAIN_ISLE] = new FrameCodeTable(codes);
    scene.codes.fill(0);
    scene.map.bounds[0] = 99;
    scene.map.bounds[1] = 99;
    scene.map.bounds[2] = 102;
    scene.map.bounds[3] = 102;
    for (let x = 99; x <= 101; x += 1) for (let y = 99; y <= 101; y += 1) scene.cell(x, y, 0x78);
    scene.stamp.stamp(0x78);
    // 중심 (100,100)은 y 외부/x 내부의 다섯 번째 칸이다.
    const slot = scene.pool.slot(TERRAIN_BORN + 4);
    const frame = get(slot, 36, 4);
    assert.equal(frame, stampCoreFrame(scene.frames[TERRAIN_ISLE], Array.from(scene.state.variants), 100, 100));
    assert.ok(frame > 0xffff);
});
