// 실제 미션 새 login 의 저장 월드 부모(cpppj `o/RawCampaignWorldLoad`, 원본 4b5300)의 웹 대응 `core/raw-campaign-world-load.js` 를 검사한다(4-B, WB04-E287).
//  - 고정 관측 `campaign-worldload-x86.tsv` originals 65행: 실제 Player152 생성(보호 Factory·Owner)·Builder·Territory(보호 WarBeginsTerritory 120BYTE 포함)·3204BYTE 섬 목록·16×16 청크·Template 9 header·mySubHandle·Player 9기록을 같은 입력에서 재생해
//    실행 뒤 전체 현재 BYTE·두 owner(template/battle)·editor·pos/end/depth·하위 요청 순서(Y 번역 · S 설정 · V 제목 · L Player pop · G 생성 · O Objects · R 그래프 · K 카메라 · A:E 진단)와 진단 수가 같다.
//    변이 10종(각 단계가 template/battle owner·editor·handle 을 바꿈)이 매 단계 직전 현재 owner 재조회를 구별한다. CD 행은 웹 목표(10.78) 밖이다.
//  - 계약: CampaignWorldGlobals(빌린 subscriber 상자·독립 값), 구성 거부(다른 풀·필수 하위·header/handle 크기), 9기록 밖 owner 거부, missionPost 0 의 사제 준비 부모 요구, 이름 복사의 NUL/물리 범위.
// 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E287)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT, readData, loadOriginalForts } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { realPlayerAssets } from './player-scene-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { SquidPostPopList, SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidOwner, SquidOwnerMode } from '../www/core/squid-owner.js';
import { ChunkMap } from '../www/assets/territory.js';
import { parseFort } from '../www/assets/fort.js';
import { ARCHIVE_PATH } from '../www/assets/game-data.js';
import { TaffArchive } from '../www/assets/taff.js';
import { decodeOriginalText } from '../www/assets/text-encoding.js';
import { TypeTable } from '../www/assets/type-table.js';
import { createFortTypes } from '../www/assets/type-flags.js';
import { RawIslandRuntime, createFortFinishOwnerState } from '../www/core/raw-island-runtime.js';
import { PlayerWorldState, RawPlayerWorld } from '../www/core/raw-player-world.js';
import { RawFortWorldLoad } from '../www/core/raw-fort-world-load.js';
import { CampaignWorldGlobals, RawCampaignWorldLoad } from '../www/core/raw-campaign-world-load.js';

// 원본 관측 파일(originals 행만)이다. 파일이 없으면 해당 검사를 건너뛴다. (2026-11-11 WB04-E287)
const ROWS = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'campaign-worldload-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 16진 문자열과 `원본:횟수;…` 반복 블록을 BYTE 배열로 복원한다(padding 도 생략하지 않음). (2026-11-11 WB04-E287)
const hexBytes = (hex) => Uint8Array.from(hex.match(/../g) ?? [], (pair) => Number.parseInt(pair, 16));
const bytesHex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');
function blocks(text) {
    const out = [];
    // 관찰 run 의 실제 BYTE 와 반복 수만 사용한다.
    for (const run of text.split(';')) {
        const [raw, count] = run.split(':');
        const bytes = hexBytes(raw);
        for (let index = 0; index < Number(count); index += 1) out.push(...bytes);
    }
    return Uint8Array.from(out);
}

// 원본과 같은 176개 P 변형 및 중복 첫 코드의 명시 입력이다. (2026-11-11 WB04-E287)
function pieceFrames() {
    const codes = [];
    for (let side = 65; side <= 80; side += 1) for (let number = 0; number < 11; number += 1) codes.push({ side, variant: 80, number, flags: 0 });
    codes.push({ ...codes[0] });
    return codes;
}

// 현재 청크의 모든 BYTE 를 원본 X 열 우선 순서로 직렬화한다. (2026-11-11 WB04-E287)
function chunkBytes(chunks) {
    const bytes = new Uint8Array(256 * 12);
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

// 예외가 나는지 본다. (2026-11-11 WB04-E287)
function throws(action) {
    try {
        action();
        return false;
    } catch {
        return true;
    }
}

// 보호 WarBegins Fort 의 Territory 120BYTE 이다(입력 profile 3). 처음 필요할 때 한 번만 읽는다. (2026-11-11 WB04-E287)
let warBegins = null;
function warBeginsTerritory() {
    if (warBegins) return warBegins;
    const archive = new TaffArchive(readData(ARCHIVE_PATH));
    const entry = loadOriginalForts(archive).find((candidate) => /thewarbegins.fort$/i.test(candidate.source));
    warBegins = parseFort(entry.bytes, createFortTypes(TypeTable.fromArchive(archive, decodeOriginalText))).territory;
    return warBegins;
}

// 관찰 범위 밖의 하위 효과는 기본 성공으로 완료 처리하지 않는다. (2026-11-11 WB04-E287)
const missing = () => { throw new Error('actual 캠페인 부모 관찰 범위 밖 하위 효과'); };

// 입력 한 행의 실제 풀/Factory/Player/Runtime/지도를 소유한다(기대 출력 없이 입력과 원본 전체 저장소만 준비). (2026-11-11 WB04-E287)
class CampaignScene {
    constructor(fields) {
        this.input = fields.split(',').map(Number);
        assert.equal(this.input.length, 10);
        const assets = realPlayerAssets();
        this.assets = assets;
        const { types } = assets;
        this.pool = new SidPool(32768, { server: true });
        this.hash = new SquidHash();
        const spots = new Uint8Array(65536);
        const book = new SquidPostPopState();
        const unpop = new SquidUnpop(this.pool, this.hash, spots);
        const post = new SquidPostPop(this.pool, types, book);
        this.factory = new SquidFactory(this.pool, types, { unpop });
        const mode = new SquidOwnerMode();
        const owner = new SquidOwner(this.pool, types, book, mode);
        this.players = new Uint8Array(9 * 0xb4).fill(0xa5);
        this.playerState = new PlayerWorldState();
        this.playerState.playerKnown = 1;
        this.playerState.battleOwner = this.input[1];
        this.playerState.templateOwner = this.input[0];
        this.chunks = new ChunkMap();
        this.islandBytes = new Uint8Array(3204).fill(0xa5);
        put(this.islandBytes, 0, 0);
        this.piece = { value: 107 };
        this.noRegion = { value: 127 };
        this.depth = this.input[6];
        this.frames = Array.from({ length: 188 }, () => []);
        this.frames[this.piece.value] = pieceFrames();
        this.events = [];
        this.assertions = 0;
        this.headers = new Uint8Array(180).fill(0xa5);
        this.handle = new Uint8Array(64).fill(0x7e);
        this.globals = new CampaignWorldGlobals({ my: 0xabcdef01, login: this.input[8], editor: this.input[2], missionPost: true });
        // 실제 Template manager pointer 비트와 관찰용 identity 를 원래 9칸에 입력한다.
        for (let slot = 0; slot < 9; slot += 1) put(this.headers, slot * 20, 0x11a00100);
        this.templates = Array.from({ length: 9 }, (_, slot) => ({ flag: slot }));
        // 같은 지도 전체의 stale record/ordinal 을 원본과 동일하게 준비한다.
        for (let x = 0; x < 16; x += 1) {
            for (let y = 0; y < 16; y += 1) {
                const entry = this.chunks.at(x, y);
                entry.island = 127;
                entry.side = 46;
                entry.record.fill(0xa5);
                entry.ordinal = 0x12345678;
            }
        }
        this.data = new Uint8Array(this.input[4] + 120);
        if (this.input[3] === 3) {
            const territory = warBeginsTerritory();
            assert.equal(territory.length, 120);
            this.data.set(territory, this.input[4]);
        } else {
            if (this.input[3]) this.data.set([3, 1, 0x11, 7, 0xaa, 0xbb], this.input[4]);
            if (this.input[3] === 2) this.data.set([3, 1, 0x55, 31, 0xcc, 0xdd], this.input[4] + 18);
        }
        this.stream = { bytes: this.data, position: this.input[4], end: this.input[5] };
        const name = this.input[9] === 0 ? '' : this.input[9] === 1 ? 'You' : 'new-handle';
        this.translated = Uint8Array.from([...Buffer.from(name, 'latin1'), 0]);
        this.islands = new RawIslandRuntime(this.pool, this.hash, this.islandBytes, createFortFinishOwnerState(), new SquidPostPopList(), { value: 0 }, { value: 0 }, { setResidenceOwner: missing, assertion: missing });
        const state = this.playerState;
        const hooks = {
            assertion: missing,
            find: () => 0,
            create: (type, flags) => this.factory.create(type, flags),
            setOwner: (sid, value) => owner.set(sid, value),
            pop: (sid, x, y, flags) => {
                assert.ok(this.pool.slot(sid)[10] === 152 && x === Math.fround(state.battleOwner + 10) && y === 1 && flags === 0);
                this.events.push(`L:${state.battleOwner}`);
                if (this.input[7] === 3) state.battleOwner = 3;
                if (this.input[7] === 4) this.globals.editor = 0;
                if (this.input[7] === 5) state.templateOwner = 5;
            },
            reserveRegular: missing,
            addRegular: missing,
        };
        this.player = new RawPlayerWorld(this.pool, post, this.players, state, hooks);
    }

    // templateOwner 를 가리키는 현재 DWORD 상자이다(Territory 부모가 매번 읽는다). (2026-11-11 WB04-E287)
    templateBox() {
        const state = this.playerState;
        return { get value() { return state.templateOwner; }, set value(value) { state.templateOwner = value >>> 0; } };
    }

    // 기존 실제 120 getter 만 재사용하며 generic 부모의 다른 child 는 실행하지 않는다. (2026-11-11 WB04-E287)
    territoryHooks() {
        const slot = (index) => {
            if (!(index >= 0 && index < 9)) throw new RangeError('Template 칸 범위');
            return index;
        };
        return {
            templatePresent: () => true,
            territory: () => {
                if (this.input[7] === 6) this.playerState.templateOwner = 6;
                return this.stream;
            },
            currentTemplate: (index) => this.templates[slot(index)],
            assertion: () => {
                this.events.push('A:E');
                this.assertions += 1;
            },
            builderAssertion: missing,
            generate: missing,
            objects: missing,
            money: missing,
            badges: missing,
            technology: missing,
            populateIsland: missing,
            rebuildGraph: missing,
        };
    }

    // actual parent 의 상위 body 만 대조하며 하위 전체 완료로 확대하지 않는다. 출력 14열을 원본 순서로 돌려준다. (2026-11-11 WB04-E287)
    run(territoryOverride = {}, campaignOverride = {}) {
        const territory = new RawFortWorldLoad(this.pool, this.chunks, this.islands, this.frames, this.piece, this.templateBox(), this.noRegion, { ...this.territoryHooks(), ...territoryOverride });
        const state = this.playerState;
        const mutation = this.input[7];
        const hooks = {
            translateYou: () => {
                this.events.push('Y');
                return this.translated;
            },
            setConfig: (key, bytes) => {
                assert.equal(key, 'fort.mySubHandle');
                assert.equal(bytes.buffer, this.handle.buffer);
                assert.equal(bytes.byteOffset, this.handle.byteOffset);
                assert.ok(bytes.length >= 1 && bytes[bytes.length - 1] === 0 && bytes.indexOf(0) === bytes.length - 1, 'NUL 포함 현재 handle');
                this.events.push('S');
                if (mutation === 1) state.templateOwner = 2;
            },
            refreshWindowTitle: () => {
                this.events.push('V');
                if (mutation === 2) state.battleOwner = 2;
            },
            builderAssertion: missing,
            generate: (current, region) => {
                assert.ok(current.owner === 1 && current.bounds.left === 1 && current.bounds.top === 1 && current.bounds.right === 15 && current.bounds.bottom === 15);
                this.events.push(`G:${region}`);
                if (mutation === 8) state.templateOwner = 8;
            },
            currentTemplate: (slot) => this.templates[slot],
            objects: (current, slot) => {
                assert.equal(current.flag, slot);
                this.events.push(`O:${slot}`);
                if (mutation === 9) state.battleOwner = 8;
            },
            rebuildGraph: (reset) => {
                assert.equal(reset, false);
                this.events.push('R:0');
            },
            resetCamera: () => {
                this.events.push('K');
                if (mutation === 7) state.battleOwner = 7;
                if (mutation === 10) this.handle.set(Buffer.from('camera-name\0', 'latin1'));
            },
        };
        new RawCampaignWorldLoad(this.chunks, this.islands, this.frames, this.piece, this.noRegion, territory, this.player, this.globals, this.headers, this.handle, { ...hooks, ...campaignOverride }).load();
        return [String(this.globals.mySubscriber), bytesHex(this.headers), bytesHex(this.handle), bytesHex(this.players), String(state.templateOwner), String(state.battleOwner),
            String(this.globals.editor), String(this.stream.position), String(this.stream.end), String(this.depth), bytesHex(chunkBytes(this.chunks)), bytesHex(this.islandBytes),
            this.events.join(';'), String(this.assertions)];
    }
}

// 같은 판본의 원본 65행을 같은 입력에서 재생하고 상위 전체 BYTE/하위 요청을 대조한다. (2026-11-11 WB04-E287)
test('캠페인 월드 적재: 원본 10.78 65입력의 전체 현재 BYTE·두 owner·editor·stream·청크/섬 목록·하위 요청 순서가 같다', { skip: ROWS ? false : 'campaign-worldload-x86.tsv 없음' }, () => {
    let count = 0;
    // 원본 기대 출력은 현재 body 의 실행 뒤에만 읽는다.
    for (const row of ROWS) {
        assert.equal(row.length, 16);
        const scene = new CampaignScene(row[1]);
        const output = scene.run();
        const expected = row.slice(2);
        expected[10] = bytesHex(blocks(expected[10]));
        // 원본 Native 로그의 내부 진입 계수(C·A·P:0:20·F:1: ctor/attach/PlaceAll/선택 Finish)는 공개 하위 요청이 아니므로 제외하고 외부 요청만 순서대로 비교한다.
        expected[12] = expected[12].split(';').filter((event) => !['C', 'A', 'P:0:20', 'F:1'].includes(event)).join(';');
        const island = expected[11].split(',');
        const full = new Uint8Array(4);
        put(full, 0, Number(island[0]));
        expected[11] = bytesHex(Uint8Array.from([...full, ...blocks(island[1])]));
        const names = ['mySubscriber', 'Template180', 'Handle64', 'Player9', 'templateOwner', 'battleOwner', 'editor', 'position', 'end', 'depth', 'chunks', 'islands', 'events', 'asserts'];
        for (let index = 0; index < output.length; index += 1) assert.equal(output[index], expected[index], `행 ${count + 1} ${names[index]}: ${row[1]}`);
        count += 1;
    }
    assert.equal(count, 65);
});

// 계약: 전역 묶음과 구성 거부, 9기록 밖 owner, missionPost 0 의 사제 준비 부모, 이름 복사의 NUL/물리 범위. (2026-11-11 WB04-E287)
test('캠페인 월드 적재 계약: 전역 묶음·구성 거부·9기록 밖 owner·사제 준비 부모·이름 복사 범위', () => {
    // 전역: 독립 값, 빌린 상자(초기화 쓰기 없음)와 읽기/쓰기 전파.
    const own = new CampaignWorldGlobals({ my: 7, login: 9 });
    assert.deepEqual([own.mySubscriber, own.loginSubscriber, own.editor, own.missionPost], [7, 9, 0, true]);
    own.mySubscriber = -1;
    assert.equal(own.mySubscriber, 0xffffffff);
    const my = { value: 11 };
    const login = { value: 22 };
    const borrowed = CampaignWorldGlobals.borrow({ my, login });
    assert.deepEqual([my.value, login.value, borrowed.mySubscriber, borrowed.loginSubscriber], [11, 22, 11, 22]);
    borrowed.mySubscriber = borrowed.loginSubscriber;
    assert.equal(my.value, 22);
    login.value = 33;
    assert.equal(borrowed.loginSubscriber, 33);
    borrowed.loginSubscriber = 5;
    assert.equal(login.value, 5);
    const half = CampaignWorldGlobals.borrow({ my });
    assert.equal(half.loginSubscriber, 0);
    assert.ok(throws(() => CampaignWorldGlobals.borrow({ my: {} })));
    // 장면 한 개로 구성 거부를 본다.
    const scene = new CampaignScene('0,0,0,0,0,120,0,0,5,1');
    const territory = new RawFortWorldLoad(scene.pool, scene.chunks, scene.islands, scene.frames, scene.piece, scene.templateBox(), scene.noRegion, scene.territoryHooks());
    const base = {
        translateYou: () => scene.translated, setConfig: () => {}, refreshWindowTitle: () => {}, builderAssertion: missing, generate: () => {},
        currentTemplate: (slot) => scene.templates[slot], objects: () => {}, rebuildGraph: () => {}, resetCamera: () => {},
    };
    const build = (overrides = {}, args = {}) => new RawCampaignWorldLoad(scene.chunks, scene.islands, args.frames ?? scene.frames, scene.piece, scene.noRegion, territory, scene.player,
        args.globals ?? scene.globals, args.headers ?? scene.headers, args.handle ?? scene.handle, { ...base, ...overrides });
    assert.doesNotThrow(() => build());
    for (const name of Object.keys(base)) assert.ok(throws(() => build({ [name]: undefined })), name);
    assert.ok(throws(() => build({}, { headers: new Uint8Array(179) })));
    assert.ok(throws(() => build({}, { handle: new Uint8Array(0) })));
    assert.ok(throws(() => build({}, { frames: scene.frames.slice(0, 187) })));
    assert.ok(throws(() => build({}, { globals: {} })));
    const other = new CampaignScene('0,0,0,0,0,120,0,0,5,1');
    assert.ok(throws(() => new RawCampaignWorldLoad(scene.chunks, other.islands, scene.frames, scene.piece, scene.noRegion, territory, scene.player, scene.globals, scene.headers, scene.handle, base)));
    const otherTerritory = new RawFortWorldLoad(other.pool, other.chunks, other.islands, other.frames, other.piece, other.templateBox(), other.noRegion, other.territoryHooks());
    assert.ok(throws(() => new RawCampaignWorldLoad(scene.chunks, scene.islands, scene.frames, scene.piece, scene.noRegion, otherTerritory, scene.player, scene.globals, scene.headers, scene.handle, base)));
    // missionPost 0 이면 사제 준비 부모를 요구하고 mission 1 은 자연 반환한다.
    let prepared = 0;
    scene.globals.missionPost = false;
    assert.ok(throws(() => build().finishMissionPriests()));
    build({ preparePriests: () => { prepared += 1; } }).finishMissionPriests();
    assert.equal(prepared, 1);
    scene.globals.missionPost = true;
    build().finishMissionPriests();
    assert.equal(prepared, 1);
    // 현재 owner 가 9기록 밖이면 쓰기 전에 거부한다(template owner).
    const bad = new CampaignScene('9,0,0,0,0,120,0,0,5,1');
    const badTerritory = new RawFortWorldLoad(bad.pool, bad.chunks, bad.islands, bad.frames, bad.piece, bad.templateBox(), bad.noRegion, bad.territoryHooks());
    const beforeHeaders = bad.headers.slice();
    assert.ok(throws(() => new RawCampaignWorldLoad(bad.chunks, bad.islands, bad.frames, bad.piece, bad.noRegion, badTerritory, bad.player, bad.globals, bad.headers, bad.handle, { ...base, currentTemplate: () => ({}) }).load()));
    assert.deepEqual(bad.headers, beforeHeaders);
    // 이름 복사: 번역 결과에 NUL 이 없거나 handle 저장소(64BYTE)를 넘으면 거부한다.
    const noNul = new CampaignScene('1,1,0,0,0,120,0,0,5,1');
    const noNulTerritory = new RawFortWorldLoad(noNul.pool, noNul.chunks, noNul.islands, noNul.frames, noNul.piece, noNul.templateBox(), noNul.noRegion, noNul.territoryHooks());
    const make = (translate, handle = noNul.handle) => new RawCampaignWorldLoad(noNul.chunks, noNul.islands, noNul.frames, noNul.piece, noNul.noRegion, noNulTerritory, noNul.player, noNul.globals, noNul.headers, handle,
        { ...base, translateYou: () => translate, objects: () => {} });
    assert.throws(() => make(Uint8Array.from([0x59, 0x6f, 0x75])).load(), /NUL 문자열/);
    assert.throws(() => make(Uint8Array.from([...new Uint8Array(70).fill(0x41), 0])).load(), /대상 물리 바이트 범위/);
});

// 계약 2: Territory 조회는 setConfig 가 바꾼 현재 template owner 를 쓰고, Generate 는 현재 noRegion, PlaceAll 은 20번째 영역까지, 사제 준비 부모는 Graph 뒤·카메라 앞, 카메라가 바꾼 battleOwner 는 마지막 이름 복사에서 거부한다. (2026-11-11 WB04-E287)
test('캠페인 월드 적재 계약 2: 현재 owner/noRegion 재조회·20번째 영역·사제 준비 순서·battleOwner 범위', () => {
    // setConfig(변이 1)가 template owner 를 2 로 바꾸면 Territory 조회 칸은 2 이다.
    const slots = [];
    const first = new CampaignScene('0,0,0,0,0,120,0,1,5,1');
    first.run({ territory: (slot) => { slots.push(slot); return first.stream; } });
    assert.deepEqual(slots, [2]);
    // Territory 조회가 noRegion 을 22 로 바꾸면 Generate 가 22 를 받는다.
    const second = new CampaignScene('0,0,0,0,0,120,0,0,5,1');
    second.run({ territory: () => { second.noRegion.value = 22; return second.stream; } });
    assert.ok(second.events.includes('G:22'));
    // 20번째 영역 레코드(오프셋 114)까지 놓는다(마지막을 빠뜨리면 섬 수 0).
    const third = new CampaignScene('0,0,0,0,0,120,0,0,5,1');
    third.data.set([3, 1, 0x11, 7, 0xaa, 0xbb], 114);
    third.run();
    assert.equal(third.islands.count, 1);
    // missionPost 0: 사제 준비 부모는 Graph 뒤·카메라 앞에서 한 번 불린다.
    const fourth = new CampaignScene('0,0,0,0,0,120,0,0,5,1');
    fourth.globals.missionPost = false;
    fourth.run({}, { preparePriests: () => fourth.events.push('P') });
    assert.ok(fourth.events.join(';').endsWith('R:0;P;K'));
    // 카메라가 battleOwner 를 9기록 밖(9)으로 바꾸면 마지막 이름 복사 직전에 거부한다.
    const fifth = new CampaignScene('0,0,0,0,0,120,0,0,5,1');
    assert.throws(() => fifth.run({}, { resetCamera: () => { fifth.playerState.battleOwner = 9; } }), /battleOwner/);
});
