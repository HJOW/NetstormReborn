// Fort 월드 적재 부모(cpppj `o/RawFortWorldLoad`, 원본 004b5f70 Load · 004bd660 ReadTerritory)의 웹 대응 `core/raw-fort-world-load.js` 를 검사한다(4-B, WB04-E286).
//  - 고정 관측 `fortworldload-x86.tsv` originals 행(전체 534 중 patch 267): 새 저장소에서 시작해 현재 Builder·실제 3204BYTE 섬 목록·16×16 청크(12BYTE)·localOwner/noRegion/깊이·Territory stream 의 pos/end 가
//    실행 뒤 모두 같고, 단계 순서(진단 A:T/A:E · 생성 G · 후처리 O/M/B/T · 섬 채움 I · 그래프 R)와 진단 횟수가 같다. 변이 9종(Territory 조회·Generate·각 child 가 owner/noRegion/섬 존재를 바꿈)은 매 단계 직전 현재 owner 재조회를 구별한다.
//    CD 행은 웹 목표(10.78) 밖이다(깊이 증감 hook 도 옮기지 않음).
//  - 손 시험(cpppj RejectsForeignPoolMissingChildAndTruncatedTerritory · CanonConsumers_FortLoaderBeforeWorldGeneration): 다른 풀·필수 child 누락 거부, 물리 120BYTE 미만 거부(pos 불변), Canon 진단이 생성/Objects 이전에 중단.
//  - 계약: child 어댑터(`makeFortWorldChildrenHooks`)의 풀 불일치 거부와 선택→읽기 순서.
// 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E286)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { ChunkMap } from '../www/assets/territory.js';
import { RawIslandRuntime, createFortFinishOwnerState } from '../www/core/raw-island-runtime.js';
import { FORT_WORLD_LOAD_ASSERTION, RawFortWorldLoad, makeFortWorldChildrenHooks } from '../www/core/raw-fort-world-load.js';

// 원본 관측 파일(originals 행만)이다. 파일이 없으면 해당 검사를 건너뛴다. (2026-11-11 WB04-E286)
const ROWS = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'fortworldload-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 원본 입력의 고정 열 순서이다. 기대 후처리/섬 수/owner 는 포함하지 않는다. (2026-11-11 WB04-E286)
const F = Object.freeze({ kind: 0, alternate: 1, owner: 2, count: 3, exists: 4, profile: 5, position: 6, end: 7, depth: 8, mutation: 9, templatePresent: 10 });

// 16진 문자열과 `원본:횟수;…` 반복 블록을 BYTE 배열로 복원한다(비활성 기록/padding 도 생략하지 않음). (2026-11-11 WB04-E286)
const hexBytes = (hex) => Uint8Array.from(hex.match(/../g) ?? [], (pair) => Number.parseInt(pair, 16));
const bytesHex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');
function blocks(text) {
    const out = [];
    // 저장 run 의 그대로인 바이트/횟수를 사용한다.
    for (const run of text.split(';')) {
        const [raw, count] = run.split(':');
        const bytes = hexBytes(raw);
        for (let index = 0; index < Number(count); index += 1) out.push(...bytes);
    }
    return Uint8Array.from(out);
}

// 실제 기존 Builder 의 16방향·P 변형·번호 0..10 합성 코드 입력이다(마지막에 첫 코드 하나 더). (2026-11-11 WB04-E286)
function pieceFrames() {
    const codes = [];
    for (let side = 65; side <= 80; side += 1) for (let number = 0; number < 11; number += 1) codes.push({ side, variant: 80, number, flags: 0 });
    codes.push({ ...codes[0] });
    return codes;
}

// 현재 소비자 청크를 원본 X 열 우선 12BYTE 로 직렬화한다(섬·방향 글자·record 6·순번). (2026-11-11 WB04-E286)
function chunkBytes(chunks) {
    const bytes = new Uint8Array(256 * 12);
    // X 열이 바깥이며 각 열의 Y 16칸을 저장한다.
    for (let x = 0; x < 16; x += 1) {
        for (let y = 0; y < 16; y += 1) {
            const entry = chunks.at(x, y);
            const offset = (x * 16 + y) * 12;
            bytes[offset] = entry.island;
            bytes[offset + 1] = typeof entry.side === 'string' ? entry.side.charCodeAt(0) : entry.side & 0xff;
            bytes.set(entry.record, offset + 2);
            put(bytes, offset + 8, entry.ordinal >>> 0);
        }
    }
    return bytes;
}

// 예외가 나는지 본다. (2026-11-11 WB04-E286)
function throws(action) {
    try {
        action();
        return false;
    } catch {
        return true;
    }
}

// 단일 원본 입력의 실제 풀/현재 3204BYTE/후처리 경계를 소유한다. (2026-11-11 WB04-E286)
class WorldScene {
    constructor(fields) {
        this.input = fields.split(',').map(Number);
        assert.equal(this.input.length, 11);
        this.pool = new SidPool(32768, { server: false });
        this.hash = new SquidHash();
        this.chunks = new ChunkMap();
        this.raw = new Uint8Array(3204);
        this.modes = createFortFinishOwnerState();
        this.residences = new SquidPostPopList();
        this.piece = { value: 107 };
        this.owner = { value: this.input[F.owner] };
        this.noRegion = { value: 127 };
        this.depth = this.input[F.depth];
        this.frames = Array.from({ length: 188 }, () => []);
        this.events = [];
        this.assertions = 0;
        this.raw.fill(0xa5);
        put(this.raw, 0, this.input[F.count]);
        put(this.raw, 8, this.input[F.exists]);
        this.frames[this.piece.value] = pieceFrames();
        // 초기 원본 청크의 모든 12BYTE 를 준비한다(다음 input 에서 이전 순회 결과를 재사용하지 않음).
        for (let x = 0; x < 16; x += 1) {
            for (let y = 0; y < 16; y += 1) {
                const entry = this.chunks.at(x, y);
                entry.island = 127;
                entry.side = 46;
                entry.record.fill(0xa5);
                entry.ordinal = 0x12345678;
            }
        }
        this.data = new Uint8Array(this.input[F.position] + 120);
        if (this.input[F.profile]) this.data.set([3, 1, 0x11, 7, 0xaa, 0xbb], this.input[F.position]);
        if (this.input[F.profile] === 2) this.data.set([3, 1, 0x55, 31, 0xcc, 0xdd], this.input[F.position] + 18);
        this.stream = { bytes: this.data, position: this.input[F.position], end: this.input[F.end] };
        this.present = Array(9).fill(this.input[F.templatePresent] !== 0);
        // 각 Template 의 flag 는 관찰용 identity 이며 월드 효과를 대신 계산하지 않는다.
        this.templates = Array.from({ length: 9 }, (_, index) => ({ flag: index }));
        const missing = () => { throw new Error('선택 Builder 는 Residence owner/Runtime 진단 경계를 요청하지 않는다'); };
        this.islands = new RawIslandRuntime(this.pool, this.hash, this.raw, this.modes, this.residences, { value: 0 }, { value: 0 }, { setResidenceOwner: missing, assertion: missing });
    }

    // 기존 Builder 는 실제 실행하고 아직 복원하지 않은 상위 child 는 명시 관찰 경계로 남긴다. (2026-11-11 WB04-E286)
    hooks() {
        const mutation = this.input[F.mutation];
        const child = (kind, change) => (fort, slot) => {
            assert.equal(fort.flag, slot);
            this.events.push(`${kind}:${slot}`);
            if (mutation === change) this.owner.value = change + 1;
        };
        return {
            templatePresent: (slot) => {
                if (!(slot >= 0 && slot < 9)) throw new RangeError('Template 칸 범위');
                return this.present[slot];
            },
            territory: () => {
                if (mutation === 1) this.owner.value = 2;
                if (mutation === 8) this.noRegion.value = 22;
                if (mutation === 9) this.owner.value = 5;
                return this.stream;
            },
            currentTemplate: (slot) => {
                if (!(slot >= 0 && slot < 9)) throw new RangeError('Template 칸 범위');
                return this.templates[slot];
            },
            assertion: (reason) => {
                this.assertions += 1;
                this.events.push(reason === FORT_WORLD_LOAD_ASSERTION.missingTerritoryTemplate ? 'A:T' : 'A:E');
                if (reason === FORT_WORLD_LOAD_ASSERTION.missingTerritoryTemplate) this.present[this.input[F.owner]] = true;
            },
            builderAssertion: () => { throw new Error('유효 방향/프레임의 선택 Builder 진단 불일치'); },
            generate: (state, region) => {
                assert.ok(state.owner === 1 && state.bounds.left === 1 && state.bounds.right === 15);
                this.events.push(`G:${region}`);
                if (mutation === 2) this.owner.value = 3;
                if (mutation === 7) put(this.raw, 8, 1);
            },
            objects: child('O', 3),
            money: child('M', 4),
            badges: child('B', 5),
            technology: child('T', 6),
            populateIsland: (state, region) => {
                assert.ok(state.owner === 1 && region === 0);
                this.events.push('I:0');
            },
            rebuildGraph: (reset) => {
                assert.equal(reset, false);
                this.events.push('R:0');
            },
        };
    }

    // 실제 기록한 경계 요청만 원본 순서로 연결한다. (2026-11-11 WB04-E286)
    eventText() {
        return this.events.length === 0 ? '-' : this.events.join(';');
    }

    // 부모를 만든다. (2026-11-11 WB04-E286)
    loader(hooks = this.hooks()) {
        return new RawFortWorldLoad(this.pool, this.chunks, this.islands, this.frames, this.piece, this.owner, this.noRegion, hooks);
    }
}

// 원본 patch 267입력(전체 534 중)의 전체 청크/3204BYTE/현재 owner/depth 와 stage 순서를 전수 대조한다. (2026-11-11 WB04-E286)
test('Fort 월드 적재: 원본 10.78 patch 입력의 전체 청크·3204BYTE 섬 목록·owner/noRegion/depth·stage 순서·진단이 같다', { skip: ROWS ? false : 'fortworldload-x86.tsv 없음' }, () => {
    let checked = 0;
    // 각 원본 행은 새 저장소에서 시작하며 기대 Builder 효과는 계산하지 않는다.
    for (const row of ROWS) {
        assert.equal(row.length, 12);
        const scene = new WorldScene(row[1]);
        const loader = scene.loader();
        const output = new Uint8Array(120);
        if (scene.input[F.kind]) loader.readTerritory(scene.owner.value, output);
        else loader.load(scene.input[F.alternate] | 0);
        const islandFields = row[9].split(',');
        const where = `행 ${checked + 1}: ${row[1]}`;
        assert.equal(String(scene.stream.position), row[2], `pos ${where}`);
        assert.equal(String(scene.stream.end), row[3], `end ${where}`);
        assert.equal(String(scene.depth), row[4], `depth ${where}`);
        assert.equal(String(scene.owner.value), row[5], `owner ${where}`);
        assert.equal(String(scene.noRegion.value | 0), row[6], `noRegion ${where}`);
        if (scene.input[F.kind]) assert.equal(bytesHex(output), row[7], `Territory ${where}`);
        assert.ok(Buffer.from(chunkBytes(scene.chunks)).equals(blocks(row[8])), `청크 ${where}`);
        assert.equal(String(scene.islands.count), islandFields[0], `섬 수 ${where}`);
        assert.ok(Buffer.from(scene.raw.subarray(4)).equals(blocks(islandFields[1])), `섬 기록 ${where}`);
        assert.equal(scene.eventText(), row[10], `단계 ${where}`);
        assert.equal(String(scene.assertions), row[11], `진단 ${where}`);
        checked += 1;
    }
    assert.equal(checked, 267);
});

// 동일 풀/필수 효과/물리 120BYTE 경계를 보존하며 없는 child 를 성공 처리하지 않는다(cpppj RejectsForeignPoolMissingChildAndTruncatedTerritory). (2026-11-11 WB04-E286)
test('Fort 월드 적재 계약: 다른 풀·필수 child 누락·물리 120BYTE 미만 Territory 를 거부한다', () => {
    const scene = new WorldScene('0,0,1,0,0,1,0,120,0,0,1');
    const hooks = scene.hooks();
    hooks.populateIsland = null;
    assert.ok(throws(() => scene.loader(hooks)));
    for (const name of ['templatePresent', 'territory', 'currentTemplate', 'assertion', 'builderAssertion', 'generate', 'objects', 'money', 'badges', 'technology', 'rebuildGraph']) {
        assert.ok(throws(() => scene.loader({ ...scene.hooks(), [name]: undefined })), name);
    }
    const foreign = new SidPool(32768, { server: false });
    assert.ok(throws(() => new RawFortWorldLoad(foreign, scene.chunks, scene.islands, scene.frames, scene.piece, scene.owner, scene.noRegion, scene.hooks())));
    assert.ok(throws(() => new RawFortWorldLoad(scene.pool, scene.chunks, scene.islands, scene.frames.slice(0, 187), scene.piece, scene.owner, scene.noRegion, scene.hooks())));
    assert.ok(throws(() => new RawFortWorldLoad(scene.pool, scene.chunks, scene.islands, scene.frames, { value: undefined }, scene.owner, scene.noRegion, scene.hooks())));
    const loader = scene.loader();
    scene.stream.bytes = scene.data.subarray(0, 119);
    assert.ok(throws(() => loader.readTerritory(1, new Uint8Array(120))));
    assert.equal(scene.stream.position, 0);
    // 출력이 120BYTE 가 아니면 stream 이 충분해도 거부하고 pos 를 바꾸지 않는다.
    scene.stream.bytes = scene.data;
    assert.ok(throws(() => loader.readTerritory(1, new Uint8Array(119))));
    assert.equal(scene.stream.position, 0);
});

// 계약: Territory 조회는 현재 localOwner 의 Template 칸을 쓰고, PlaceAll 은 20번째(마지막) 영역 레코드까지 놓는다. fixture 의 레코드는 앞쪽 영역에만 있다. (2026-11-11 WB04-E286)
test('Fort 월드 적재 계약: Territory 조회 칸은 현재 localOwner 이고 20번째 영역 레코드까지 배치한다', () => {
    const scene = new WorldScene('0,0,4,0,0,0,0,120,0,0,1');
    const slots = [];
    const hooks = scene.hooks();
    hooks.templatePresent = (slot) => { slots.push(['present', slot]); return true; };
    hooks.territory = (slot) => { slots.push(['territory', slot]); return scene.stream; };
    scene.loader(hooks).load(0);
    assert.deepEqual(slots, [['present', 4], ['territory', 4]]);
    assert.equal(scene.islands.count, 0); // Territory 가 모두 0 이면 놓인 영역이 없다.
    // 20번째 영역(인덱스 19, 오프셋 114)에만 레코드를 두면 한 영역이 놓인다(마지막 레코드를 빠뜨리면 0).
    const last = new WorldScene('0,0,1,0,0,0,0,120,0,0,1');
    last.data.set([3, 1, 0x11, 7, 0xaa, 0xbb], 114);
    last.loader().load(0);
    assert.equal(last.islands.count, 1);
});

// Fort 부모의 선택 진단 owner 가 지역 Builder 까지 전달되어 생성/Objects 후처리 전에 중단한다(cpppj CanonConsumers_FortLoaderBeforeWorldGeneration). (2026-11-11 WB04-E286)
test('Fort 월드 적재 계약: Canon 진단이 지역 Builder 에서 중단하면 생성/후처리를 하지 않는다', () => {
    const scene = new WorldScene('0,0,1,0,0,1,0,120,0,0,1');
    scene.frames[scene.piece.value] = scene.frames[scene.piece.value].map((code) => ({ ...code, flags: code.flags | 0x20 }));
    let reported = 0;
    const hooks = scene.hooks();
    hooks.decoderDiagnostics = { checking: () => 1, assertion: (_expression, _file, line) => { assert.equal(line, 0x325); reported += 1; throw new Error('Fort Canon 진단 중단'); } };
    assert.ok(throws(() => scene.loader(hooks).load(0)));
    assert.equal(reported, 1);
    assert.deepEqual(scene.events, []);
});

// child 어댑터: 다른 풀 거부, 선택(selectMeta) → 읽기 순서, Objects 는 signed owner 로 영역 0..20 적재. (2026-11-11 WB04-E286)
test('Fort 월드 적재 child 어댑터: 풀 불일치 거부와 Objects/Badges/Technology 연결 순서', () => {
    const pool = new SidPool(32768, { server: false });
    const calls = [];
    const objects = { pool, load: (...args) => calls.push(['load', ...args]) };
    const meta = { pool, loadBadgesSelected: (select) => { calls.push(['badges']); select(); }, loadTechnologySelected: (select) => { calls.push(['technology']); select(); } };
    const selectMeta = (fort, owner, technology) => calls.push(['select', fort.flag, owner, technology]);
    const metaStream = (fort, technology) => { calls.push(['stream', fort.flag, technology]); return {}; };
    const base = { sentinel: 1 };
    const hooks = makeFortWorldChildrenHooks(pool, objects, meta, selectMeta, metaStream, base);
    assert.equal(hooks.sentinel, 1);
    hooks.objects({ flag: 3 }, 0xffffffff);
    hooks.badges({ flag: 4 }, 2);
    hooks.technology({ flag: 5 }, 3);
    assert.deepEqual(calls, [
        ['load', { flag: 3 }, -1, 0, 20],
        ['select', 4, 2, false], ['badges'], ['stream', 4, false],
        ['select', 5, 3, true], ['technology'], ['stream', 5, true],
    ]);
    assert.ok(throws(() => makeFortWorldChildrenHooks(new SidPool(32768, { server: false }), objects, meta, selectMeta, metaStream, base)));
    assert.ok(throws(() => makeFortWorldChildrenHooks(pool, { pool }, { pool: {} }, selectMeta, metaStream, base)));
    assert.ok(throws(() => makeFortWorldChildrenHooks(pool, objects, meta, null, metaStream, base)));
    assert.ok(throws(() => makeFortWorldChildrenHooks(pool, objects, meta, selectMeta, null, base)));
});
