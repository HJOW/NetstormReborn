// 자산 정기 처리 분배 raw-asset-regular.js(RawAssetRegular·AssetRegularState·runVortexRegular·runNativeGeyserRegular)와 그 아래 본문 raw-residence-regular.js·raw-factory-regular.js 를
// 원본 10.78 관측과 cpppj `AssetRegularTests.cpp`·`ResidenceRegularTests.cpp`·`FactoryRegularCurrentTests.cpp` 계약으로 대조한다(WB04-E255).
//  - assetregular-x86.tsv `originals` 1,054행: 거주 구역(kind 0)·간헐천(1)·신전(2)·Damageable 단독(3)의 사건별 반환 float 비트·게임 난수 상태·최대 장식 조사 수·하위 효과 호출 순서·슬롯 50 전체·이웃 프레임 세 개.
//    같은 행을 독립 호출·공유 권한(반대 fallback)·공유 신전 목록으로 다시 재생하고, 거주 구역 kind 0 행과 신전 프레임 사건(1/2) 99행을 따로 한 번 더 재생한다.
//  - residence-coordinates-x86.tsv `originals` 864행: 별도 원본 관찰의 축 교차/경계 좌표를 같은 finder/난수/해시에서 대조한다.
//  - factory-regular-current-x86.tsv 656행(원문 4549c0 의 현재 두 조회·난수·0범위 예외, 오류 128행)과 cpppj 의 게으른 선택 계약.
//  - 계약(cpppj): Factory 업그레이드 분배/상위 플레어, 실제 Host(SquidProcessHost·Kernel)에서의 첫 Regular 시각, 미연결 효과/다른 풀 거부, 거주 구역 Dude 생성 부모의 관문과 조건부 작업.
//  - 경계 계약: fixture 가 구별하지 못해 변형 시험에서 살아남은 입력을 닫는다(마지막 검사들의 주석).
// CD 10.72(originalCD) 행과 CD 전용 assert 는 웹 범위 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E255)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { Kernel } from '../www/core/kernel.js';
import { AssetPostPopState } from '../www/core/raw-asset-post-pop-state.js';
import { AssetRegularState, RawAssetRegular, runNativeGeyserRegular, runVortexRegular } from '../www/core/raw-asset-regular.js';
import { RawAuthorityState } from '../www/core/raw-authority-state.js';
import { RawFrameAdvance } from '../www/core/raw-frame-advance.js';
import { runFactoryRegular } from '../www/core/raw-factory-regular.js';
import { runResidenceDudeSpawn, runResidenceRegular } from '../www/core/raw-residence-regular.js';
import { GameRandom } from '../www/core/random.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidFrame } from '../www/core/squid-frame.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { REPO_ROOT } from './helpers.mjs';
import { bitsToFloat, floatToBits, get, put, readRows } from './raw-support.mjs';

// 합성 풀 크기, 대상 SID, 10.78 가상 표 기록값(거주 구역·간헐천·신전·Factory), 관측 행 수이다. (2026-11-10 WB04-E255)
const CAPACITY = 24000;
const SOURCE = 50;
const PATCH_TABLES = [0x50f540, 0x50da88, 0x5196f0];
const FACTORY_TABLE = 0x509148;
const ASSET_ROWS = 1054;
const COORDINATE_ROWS = 864;
const FACTORY_ROWS = 656;
const FACTORY_FAULTS = 128;

// fixture 폴더와 10.78 행 읽기이다. 파일이 없으면 null 이다. (2026-11-10 WB04-E255)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const fixture = (name) => readRows(path.join(FIXTURES, name));
const rows = fixture('assetregular-x86.tsv')?.filter((row) => row[0] === 'originals') ?? null;
const coordinateRows = fixture('residence-coordinates-x86.tsv')?.filter((row) => row[0] === 'originals') ?? null;
const factoryRows = fixture('factory-regular-current-x86.tsv');
const skip = { skip: !rows && '원본 fixture 없음' };

// 10진 문자열 DWORD 를 부호 없는 수로 읽고, 바이트를 hex 로 바꾼다. (2026-11-10 WB04-E255)
const number = (text) => Number(BigInt(text) & 0xffffffffn);
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 예외 종류와 문구를 함께 확인한다(문구에는 기대 정규식과 겹치는 낱말을 넣지 않는다). (2026-11-10 WB04-E255)
const throwsAs = (action, type, pattern) => assert.throws(action, (error) => error instanceof type && pattern.test(error.message));

// 원본 기계어와 같은 6개 방향의 8프레임 코드 표이다(cpppj `Codes`). flags 는 모든 프레임의 코드 플래그이다. (2026-11-10 WB04-E255)
function codes(flags = 0) {
    return new FrameCodeTable(Array.from({ length: 48 }, (_, index) => ({ side: 0x41 + Math.floor(index / 8), variant: 0x41, flags })));
}

// 188칸 TYPE 입력이다(cpppj 기본 RiftTypeRecord: 모든 칸 0). (2026-11-10 WB04-E255)
const blankTypes = () => {
    const types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0 }));
    // 공통 Form 생성이 읽는 원문 내장 생성자 주소를 Form 네 종류에 명시한다(cpppj MakeFormTypeInputs).
    for (const form of [1, 2, 3, 6]) types[form].constructorAddress = typeConstructorAddress(form);
    return types;
};

// cpppj `AssetRegularScene`: 원본 입력의 단일 pool/hash/random 과 외부 효과를 구성한다. 판단은 fixture 의 native 결과가 정한다. (2026-11-10 WB04-E255)
class AssetRegularScene {
    constructor() {
        this.pool = new SidPool(CAPACITY, { server: false });
        this.hash = new SquidHash();
        this.types = blankTypes();
        this.frames = this.types.map(() => codes());
        this.elements = new Array(188).fill(0);
        this.random = new GameRandom(0);
        this.assets = new AssetPostPopState();
        this.state = new AssetRegularState();
        this.events = [];
        this.input = new Array(25).fill(0);
        // native fixture 의 단일 풀 번호를 확보한다.
        for (let sid = 5; sid <= 62; sid += 1) assert.equal(this.pool.allocate(2), sid);
        this.#build();
    }

    // 현재 types 로 프레임 지정기/진행기를 다시 만든다(지정기는 구성 때 TYPE 표를 복사하므로 행마다 입력을 바꾼 뒤 다시 만든다). (2026-11-10 WB04-E255)
    #build() {
        this.setter = new SquidFrame(this.pool, this.types, {
            frameSize: () => [1, 1],
            update: (sid, flags) => {
                this.update(sid, flags);
                if (this.input[24] === 4) put(this.pool.allocated(SOURCE), 36, 7, 4);
            },
            unpop: () => { throw new Error('합성 입력은 해시 단계1을 유지해야 합니다'); },
            pop: () => { throw new Error('합성 입력은 재Pop 경계에 도달하지 않습니다'); },
        });
        this.advance = new RawFrameAdvance(this.pool, this.frames, this.setter);
        this.regular = null;
    }

    // 현재 프레임 DWORD 이다. (2026-11-10 WB04-E255)
    frame(sid) { return get(this.pool.slot(sid), 36, 4); }

    // 원본 표시 경계에서 호출 당시의 프레임을 기록한다. (2026-11-10 WB04-E255)
    update(sid, flags) { this.events.push(`U:${sid}:${flags}:${this.frame(sid)}`); }

    // 입력만 raw 슬롯/타입/해시에 쓰며 Handle 판단을 미리 계산하지 않는다(cpppj `Prepare`). (2026-11-10 WB04-E255)
    prepare(row) {
        const { input, pool, hash } = this;
        this.input = row[1].split(',').map(number);
        assert.equal(this.input.length, 25);
        this.events = [];
        hash.reset();
        this.random.setState(this.input[7]);
        const state = new AssetRegularState();
        this.state = state;
        this.assets = new AssetPostPopState();
        state.fortMode = this.input[8] !== 0;
        state.boss = this.input[9] !== 0;
        state.suppressLeaveRequest = this.input[10] !== 0;
        state.checkingForms = this.input[11] !== 0;
        state.themeMode = this.input[13] !== 0;
        state.decorationType = 91;
        state.maxDecorationScan = this.input[16];
        state.regionGeneration = 3;
        state.animationType = 17;
        this.assets.temples.entries = [62, 0];
        this.assets.temples.count = 1;
        const x = this.input[17] === 4 ? 5.25 : 50.75;
        const y = this.input[17] === 4 ? 4.75 : 52.5;
        // 원본 입력과 같은 역순 머리 삽입으로 실제 finder 의 반환 순서를 비교한다.
        for (const sid of [62, 61, 60, SOURCE]) {
            // 앞 관찰의 이탈 효과가 raw free 를 켰어도 다음 독립 입력은 원본처럼 슬롯 자체를 다시 초기화한다.
            const raw = pool.physical(sid);
            raw.fill(0);
            const xx = sid === SOURCE ? x : sid === 60 ? 49.75 : sid === 61 ? 50.25 : 51.75;
            const yy = sid === SOURCE ? y : 50.25;
            raw[10] = sid === SOURCE ? 89 : sid === 62 ? 90 : 91;
            raw[40] = sid === SOURCE ? this.input[22] : this.input[17] === 3 ? 8 : 0;
            put(raw, 0, 0x11018000);
            put(raw, 14, floatToBits(xx));
            put(raw, 18, floatToBits(yy));
            put(raw, 4, hash.bucket(1, xx, yy), 2);
            hash.setBucket(1, xx, yy, sid);
        }
        const kind = this.input[0];
        const typeNumber = kind === 0 ? 89 : kind === 1 ? 122 : 124;
        const raw = pool.allocated(SOURCE);
        raw[10] = typeNumber;
        raw[34] = this.input[12];
        put(raw, 12, this.input[4], 2);
        put(raw, 36, this.input[5], 4);
        raw[33] = 1;
        put(raw, 0, PATCH_TABLES[Math.min(kind, 2)]);
        put(raw, 42, 5);
        pool.allocated(60)[34] = this.input[15];
        pool.allocated(62)[34] = this.input[12];
        // 같은 원본 입력의 타입/코드를 구성하고 각 몸체가 후속 프레임을 결정하게 한다.
        for (const typ of [89, 90, 91, 122, 124]) {
            this.types[typ].cost = bitsToFloat(this.input[6]);
            this.types[typ].flags2 = this.input[21];
            this.types[typ].footX = 1;
            this.types[typ].footY = 1;
            this.elements[typ] = this.input[14] | 0;
            this.frames[typ] = codes(this.input[17] === 1 ? 8 : this.input[17] === 2 ? 16 : 0);
        }
        this.#build();
        this.regular = new RawAssetRegular(pool, hash, this.types, this.frames, this.elements, this.setter, this.advance, this.random, this.assets, this.state, this.hooks());
        void input;
    }

    // 원본이 관찰한 하위 효과의 인자/후속 raw 변화만 동일하게 공급한다(cpppj `Hooks`). (2026-11-10 WB04-E255)
    hooks() {
        const { pool, types } = this;
        const push = (text) => this.events.push(text);
        return {
            notifySurface: (sid) => {
                push(`N:${sid}`);
                if (this.input[24] === 1) put(pool.allocated(sid), 36, 23, 4);
            },
            surface: () => (this.input[15] ? 60 : 0),
            setOwner: (sid, owner) => {
                push(`O:${sid}:${owner}`);
                pool.allocated(sid)[34] = owner & 255;
                if (this.input[24] === 2) put(pool.allocated(sid), 42, 7);
            },
            residenceEvent2: (sid) => push(`E:${sid}`),
            displayUpdate: (sid, flags) => {
                this.update(sid, flags);
                if (this.input[24] === 3) put(pool.allocated(sid), 12, 1000, 2);
            },
            ascendTick: (sid, count) => {
                push(`P:${sid}:${count}`);
                return this.input[18] !== 0;
            },
            playerLeavingNotice: (owner) => push(`L:${owner}`),
            countForeignOwnedIslands: (owner) => {
                push(`R:${owner}`);
                return 0;
            },
            islandNumber: () => {
                push('I');
                return 9;
            },
            requestLeave: (sid, island) => {
                push(`Q:9:0:${island}:${sid}:1`);
                if (this.input[19] || this.input[24] === 5) pool.allocated(sid)[11] = 1;
            },
            regionNumber: () => {
                push('Z');
                return 11;
            },
            removeAnimation: (type, token) => {
                push(`A:${type}:${token}:${token}`);
                if (this.input[24] === 6) put(pool.allocated(SOURCE), 12, 99, 2);
            },
            damageGround: (x, y) => push(`G:${floatToBits(x)}:${floatToBits(y)}`),
            hitPoints: (sid, value, source, flags) => {
                push(`H:${sid}:${value}:${source >>> 0}:${flags}`);
                if (this.input[24] === 7) types[pool.slot(sid)[10]].flags2 = 0x200000;
                return this.input[20] | 0;
            },
            findAttached: (sid, type) => {
                push(`F:${sid}:${type}:0:0`);
                return this.input[23];
            },
            startConstruction: (sid, duration) => push(`X:${sid}:${floatToBits(duration)}`),
        };
    }

    // 한 행의 입력에 맞는 진입(kind 3 은 Damageable 직접, 그 밖은 가상 표 분배)을 실행하고 결과 float 를 돌려준다. (2026-11-10 WB04-E255)
    run() {
        const { input } = this;
        return input[0] === 3 ? this.regular.damageableHandle(SOURCE, input[1], input[2], bitsToFloat(input[3]))
            : this.regular.handle(SOURCE, input[1], input[2], bitsToFloat(input[3]));
    }

    // 관찰 문자열(호출 순서·이웃 프레임·슬롯 50)을 fixture 형식으로 만든다. (2026-11-10 WB04-E255)
    observed() {
        return {
            events: this.events.length === 0 ? '-' : this.events.join(';'),
            neighbors: `${this.frame(60)},${this.frame(61)},${this.frame(62)}`,
            raw: hex(this.pool.slot(SOURCE)),
        };
    }
}

// 첫 불일치를 설명 문자열로 돌려준다(없으면 null). 결과/난수/최대 조사/호출 순서/슬롯/이웃 프레임을 모두 비교한다. (2026-11-10 WB04-E255)
function compareRow(scene, row, result, offset = 2) {
    const seen = scene.observed();
    const want = { result: number(row[offset]), rng: number(row[offset + 1]), scan: number(row[offset + 2]), events: row[offset + 3], raw: row[offset + 4], neighbors: row[offset + 5] };
    if (floatToBits(result) === want.result && scene.random.state === want.rng && scene.state.maxDecorationScan === want.scan &&
        seen.events === want.events && seen.raw === want.raw && seen.neighbors === want.neighbors) return null;
    return `입력 ${row[1]}\n기대 반환 ${want.result} / 실제 ${floatToBits(result)}\n기대 효과 ${want.events} / 실제 ${seen.events}\n기대 raw ${want.raw} / 실제 ${seen.raw}`;
}

// 한 모드(독립 호출·공유 권한·공유 신전 목록)로 10.78 행을 모두 재생한다. 공유 모드도 원본 기대값은 바꾸지 않는다. (2026-11-10 WB04-E255)
function replay(selector, { sharedAuthority = false, sharedTemples = false } = {}) {
    const scene = new AssetRegularScene();
    let count = 0;
    // 각 행의 결과는 원본이 정했으며 테스트가 재계산하지 않는다.
    for (const row of rows) {
        if (!selector(row)) continue;
        assert.equal(row.length, 8);
        scene.prepare(row);
        if (sharedAuthority) {
            // 이 행의 Native 권한값만 제공하고 독립 boss 는 반대로 둔다(이미 구성한 모듈도 다음 실제 Handle 에서 현재 owner 를 읽는다).
            const authority = new RawAuthorityState();
            authority.boss = scene.state.boss;
            scene.state.sharedAuthority = authority;
            scene.state.boss = !authority.boss;
        }
        if (sharedTemples) {
            // 실제 Session 처럼 신전은 공유 목록에만 존재한다.
            const shared = new SquidPostPopList();
            shared.entries = [...scene.assets.temples.entries];
            shared.count = scene.assets.temples.count;
            scene.assets.sharedTemples = shared;
            scene.assets.temples = new SquidPostPopList();
        }
        const mismatch = compareRow(scene, row, scene.run());
        assert.equal(mismatch, null, `Handle 행 ${count + 1}: ${mismatch}`);
        count += 1;
    }
    return count;
}

test('AssetRegular: 원본 10.78 1,054행의 반환·난수·최대 조사·호출 순서·슬롯 50·이웃 프레임이 같다', skip, () => {
    assert.equal(replay(() => true), ASSET_ROWS);
});

test('AssetRegular: 공유 권한(반대 fallback)에서도 원본 기대값을 바꾸지 않고 현재 권한을 소비한다', skip, () => {
    assert.equal(replay(() => true, { sharedAuthority: true }), ASSET_ROWS);
});

test('AssetRegular: 공유 신전 목록에서 실제 Session 의 원소별 거주 구역 그림 선택이 같다', skip, () => {
    assert.equal(replay(() => true, { sharedTemples: true }), ASSET_ROWS);
});

test('AssetRegular: 거주 구역(kind 0) 행만 따로 재생해도 같다', skip, () => {
    assert.equal(replay((row) => row[1].startsWith('0,')), 261);
});

test('AssetRegular: 이탈/상승/Descend 가 없는 신전 프레임 사건 1/2 의 99행이 같다', skip, () => {
    assert.equal(replay((row) => row[1].startsWith('2,1,') || row[1].startsWith('2,2,')), 99);
});

// 좌표 관찰 행의 입력을 장면에 배치한다: 부모의 x/y 비트를 바꾸고 네 슬롯을 해시 1단계에 다시 등록한다(cpppj ReplayResidenceCoordinates). (2026-11-10 WB04-E255)
function placeCoordinates(scene, row) {
    scene.prepare(row);
    put(scene.pool.allocated(SOURCE), 14, number(row[2]));
    put(scene.pool.allocated(SOURCE), 18, number(row[3]));
    scene.hash.reset();
    // 바뀐 입력 위치에 실제 네 단계 해시 중 단계1의 체인을 다시 등록한다.
    for (const sid of [62, 61, 60, SOURCE]) {
        const raw = scene.pool.allocated(sid);
        const x = bitsToFloat(get(raw, 14));
        const y = bitsToFloat(get(raw, 18));
        raw[33] = 1;
        put(raw, 4, scene.hash.bucket(1, x, y), 2);
        scene.hash.setBucket(1, x, y, sid);
    }
}

test('AssetRegular: 거주 구역 축 교차/경계 좌표 864행이 같은 finder/난수/해시에서 같다', { skip: !coordinateRows && '원본 fixture 없음' }, () => {
    const scene = new AssetRegularScene();
    let count = 0;
    // 좌표는 입력 그대로 넣으며 탐색 범위/기대값을 재계산하지 않는다.
    for (const row of coordinateRows) {
        assert.equal(row.length, 11);
        placeCoordinates(scene, row);
        const result = scene.regular.handle(SOURCE, scene.input[1], scene.input[2], bitsToFloat(scene.input[3]));
        const mismatch = compareRow(scene, row, result, 4);
        assert.equal(mismatch, null, `거주 구역 좌표 행${count + 1} x${row[2]} y${row[3]} 호출${row[10]}: ${mismatch}`);
        count += 1;
    }
    assert.equal(count, COORDINATE_ROWS);
});

// ---- Factory 정기 본문(raw-factory-regular.js) ----

// cpppj `FactoryRegularScene`: 사건/조회 순서와 분기는 제품 본문이 실행하고 장치는 원본 관측과 같은 명시 경계다. (2026-11-10 WB04-E255)
class FactoryScene {
    constructor() {
        this.pool = new SidPool(CAPACITY, { server: true });
        this.random = new GameRandom(1);
        this.row = [];
        this.trace = [];
    }

    // 부모 50 의 물리 슬롯이다(fixture 입력 전용 직접 쓰기). (2026-11-10 WB04-E255)
    raw() { return this.pool.physical(SOURCE); }

    // 8 DWORD 입력과 같은 바이트/시드를 재배치한다. (2026-11-10 WB04-E255)
    prepare(input) {
        this.row = input.split(',').map(number);
        const raw = this.raw();
        raw.fill(0xa5);
        put(raw, 0, 0x11010000);
        raw[10] = 100;
        raw[11] = ((this.row[3] << 5) | 4) & 0xff;
        this.random.setState(this.row[6]);
        this.trace = [];
    }

    // 원문이 선택한 효과만 서식화한다. (2026-11-10 WB04-E255)
    event(text) { this.trace.push(text); }

    // 큰 업그레이드/표시·진행은 원본 관측과 같은 명시 경계이다. (2026-11-10 WB04-E255)
    hooks() {
        const { row } = this;
        const check = (parent) => assert.equal(parent, SOURCE);
        return {
            level: (parent) => {
                check(parent);
                const level = this.raw()[11] >> 5;
                this.event(`L:${level}`);
                return level;
            },
            firstFrame: (parent, level) => {
                check(parent);
                const type = this.raw()[10];
                this.event(`T:${type}`);
                const first = (row[4] + (type === 101 ? 100 : 0) + level * 10) | 0;
                if (row[7]) {
                    this.raw()[10] = 101;
                    this.raw()[11] = ((((row[3] + 1) & 7) << 5) | 4) & 0xff;
                }
                return first;
            },
            frameCount: (parent) => {
                check(parent);
                const type = this.raw()[10];
                this.event(`T:${type}`);
                return (row[5] + (type === 101 ? 1 : 0)) | 0;
            },
            random: (limit) => {
                this.event(`R:${limit}`);
                if (!limit) {
                    this.random.next(1);
                    throw new RangeError('원문 난수0범위');
                }
                return this.random.next(limit);
            },
            frame: (parent, value, flags) => {
                check(parent);
                this.event(`S:${value >>> 0}:${flags}`);
            },
            advance: (parent, value, flags) => {
                check(parent);
                this.event(`A:${value}:${flags}`);
            },
            begin: (parent, payload) => {
                check(parent);
                this.event(`U:${floatToBits(payload)}`);
            },
            finish: (parent) => {
                check(parent);
                this.event('F');
                return -1;
            },
            flare: (parent, count, code) => this.event(`P:${parent}:${count}:${code}`),
            damageable: (parent, event, count, payload) => {
                check(parent);
                this.event(`B:${event}:${count}:${floatToBits(payload)}`);
                return 0.375;
            },
        };
    }
}

test('FactoryRegular: 원문 4549c0 의 독립 원본 656입력(현재 두 조회·실제 난수·0범위 예외)이 같다', { skip: !factoryRows && '원본 fixture 없음' }, () => {
    const scene = new FactoryScene();
    let count = 0;
    let faults = 0;
    // 원래 기대 반환/순서/시드/50바이트는 수정하지 않는다.
    for (const fields of factoryRows) {
        assert.equal(fields.length, 5);
        scene.prepare(fields[0]);
        let result;
        try {
            result = String(floatToBits(runFactoryRegular(scene.pool, SOURCE, scene.row[0], scene.row[1], bitsToFloat(scene.row[2]), scene.hooks())));
        } catch (error) {
            if (!(error instanceof RangeError)) throw error;
            result = 'fault';
            faults += 1;
        }
        const trace = scene.trace.length === 0 ? '-' : scene.trace.join(';');
        assert.equal(result, fields[1], `입력 ${fields[0]}`);
        assert.equal(trace, fields[2], `입력 ${fields[0]}`);
        assert.equal(scene.random.state, number(fields[3]));
        assert.equal(hex(scene.raw()), fields[4]);
        count += 1;
    }
    assert.equal(count, FACTORY_ROWS);
    assert.equal(faults, FACTORY_FAULTS);
});

test('FactoryRegular: 조기 반환은 관련 없는 조회/기본/표시 장치를 요구하지 않고 선택된 장치는 없으면 거부한다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const hooks = {};
    assert.equal(runFactoryRegular(pool, 0, 4, 1, 9, hooks), 0);
    hooks.finish = () => 0.125;
    assert.equal(runFactoryRegular(pool, 0, 3, 1, 9, hooks), 0.125);
    throwsAs(() => runFactoryRegular(pool, 0, 3, 0, 9, hooks), Error, /Factory Regular/);
    // 10.78 이 아닌 풀은 상태를 읽기 전에 거부한다.
    throwsAs(() => runFactoryRegular({ edition: 'cd1072' }, 0, 4, 1, 9, hooks), TypeError, /PATCH10\.78/);
});

// ---- 거주 구역 정기 본문(raw-residence-regular.js) ----

test('ResidenceRegular: 비선택 사건은 슬롯/난수/설정을 읽지 않고 선택 사건은 장치가 없으면 거부한다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const hash = new SquidHash();
    const temples = new SquidPostPopList();
    const payload = bitsToFloat(0x7fc01234);
    assert.equal(floatToBits(runResidenceRegular(pool, hash, [], [], temples, 0, 99, 0, payload, {})), 0x7fc01234);
    throwsAs(() => runResidenceRegular(pool, hash, [], [], temples, 0, 2, 0, payload, {}), Error, /거주 구역/);
    throwsAs(() => runResidenceRegular({ edition: 'cd1072' }, hash, [], [], temples, 0, 99, 0, payload, {}), TypeError, /PATCH 본문/);
});

test('ResidenceDudeSpawn: Fort/현재 두 목록의 개수 관문과 진입점 뒤 소유자/생성 뒤 현재 상태로 작업을 예약한다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const source = 50;
    const born = 60;
    const slot = (sid) => pool.physical(sid);
    const population = new SquidPostPopList();
    population.entries = new Array(30).fill(0);
    population.count = 0;
    const providers = new SquidPostPopList();
    providers.entries = new Array(30).fill(0);
    providers.count = 1;
    let entrances = 0;
    let spawns = 0;
    let tasks = 0;
    const hooks = {
        entrance: (selected) => {
            assert.equal(selected, source);
            entrances += 1;
            slot(source)[34] = 7;
            return [12, 24];
        },
        spawn: (x, y, owner) => {
            assert.ok(x === 12 && y === 24 && owner === 7);
            spawns += 1;
            return born;
        },
        task: (selected, code, delay) => {
            assert.ok(selected === born && code === 0x5102 && floatToBits(delay) === 0);
            tasks += 1;
        },
    };
    // source 의 free 비트만 차단하며 dead 는 원문의 자연 통과이다.
    for (const free of [0, 1, 2]) {
        // 생성된 Dude 의 free/dead 두 비트가 작업 예약을 결정한다.
        for (const state of [0, 1, 2, 4]) {
            entrances = 0;
            spawns = 0;
            tasks = 0;
            slot(source)[11] = free;
            slot(born)[11] = state;
            runResidenceDudeSpawn(pool, false, population, providers, source, hooks);
            assert.ok(entrances === 0 && spawns === 0 && tasks === 0);
            runResidenceDudeSpawn(pool, true, population, providers, source, hooks);
            assert.equal(entrances, free === 1 ? 0 : 1);
            assert.equal(spawns, entrances);
            assert.equal(tasks, free !== 1 && !(state & 3) ? 1 : 0);
        }
    }
    // 현재 Dude 수가 공급원 수 이상이면 source 0 진단이나 장치를 선택하지 않는다.
    population.count = 1;
    runResidenceDudeSpawn(pool, true, population, providers, 0, {});
    population.count = 2;
    runResidenceDudeSpawn(pool, true, population, providers, 0, {});
    population.count = 0;
    providers.count = 0;
    runResidenceDudeSpawn(pool, true, population, providers, 0, {});
    providers.count = 1;
    slot(source)[11] = 0;
    tasks = 0;
    runResidenceDudeSpawn(pool, true, population, providers, source, { ...hooks, spawn: () => 0 });
    assert.equal(tasks, 0);
});

// ---- 분배 계약 ----

test('AssetRegular: PATCH Factory 의 event3 은 count0 에서 그림→프레임→부모, 양수에서 완료 반환으로 분기하고 상위 레벨 플레어를 부른다', () => {
    const scene = new AssetRegularScene();
    const raw = scene.pool.allocated(SOURCE);
    raw.fill(0);
    put(raw, 0, FACTORY_TABLE);
    raw[10] = 85;
    raw[11] = 4;
    raw[33] = 1;
    const trace = [];
    const hooks = {
        beginFactoryUpgrade: (parent, duration) => {
            assert.ok(parent === SOURCE && duration === 15);
            trace.push('begin');
        },
        finishFactoryUpgrade: (parent) => {
            assert.equal(parent, SOURCE);
            trace.push('finish');
            return -1;
        },
        factoryFlare: (parent, count, event) => {
            assert.ok(parent === SOURCE && count === 2 && event === 1);
            trace.push('flare');
        },
    };
    const regular = new RawAssetRegular(scene.pool, scene.hash, scene.types, scene.frames, scene.elements, scene.setter, scene.advance, scene.random, scene.assets, scene.state, hooks);
    assert.ok(regular.handle(SOURCE, 3, 0, 15) === 15 && scene.frame(SOURCE) === 1);
    assert.equal(regular.handle(SOURCE, 3, 1, 15), -1);
    raw[11] = 100;
    assert.ok(regular.handle(SOURCE, 5, 0, 2) === 2);
    assert.deepEqual(trace, ['begin', 'finish', 'flare']);
    assert.equal(regular.handle(SOURCE, 4, 1, 20), 0);
});

test('AssetRegular: postPop 에 등록된 첫 Regular 가 시각/float payload 를 혼동하지 않고 현재 Kernel 에서 바로 호출된다', skip, () => {
    const scene = new AssetRegularScene();
    const found = rows.find((row) => row[1].startsWith('2,1,'));
    assert.ok(found);
    scene.prepare(found);
    const spots = new Uint8Array(65536);
    const destroy = new RawSquidDestroy(scene.pool, new SquidUnpop(scene.pool, scene.hash, spots), scene.types);
    const kernel = new Kernel();
    const processState = new SquidProcessState();
    processState.now = 17.25;
    const host = new SquidProcessHost(scene.pool, scene.types, kernel, destroy, processState, { regular: scene.regular.handler() });
    const scheduled = host.addRegular(SOURCE, 1, Math.fround(0.08));
    assert.ok(scheduled);
    assert.ok(scheduled.time === 17.25 && scheduled.count === 0 && scheduled.payload === Math.fround(0.08));
    kernel.runFrame();
    assert.ok(scheduled.count === 1 && scene.frame(SOURCE) === 1);
    assert.equal(scheduled.time, 17.25 + Math.fround(0.08));
    processState.now = 17.26;
    kernel.runFrame();
    assert.ok(scheduled.count === 1 && scene.frame(SOURCE) === 1);
    processState.now = scheduled.time;
    kernel.runFrame();
    assert.ok(scheduled.count === 2 && scene.frame(SOURCE) === 2);
});

test('AssetRegular: 미복원 효과/부모 및 다른 풀 지정기는 성공 처리 전에 진단한다', skip, () => {
    const scene = new AssetRegularScene();
    scene.prepare(rows[0]);
    const missing = new RawAssetRegular(scene.pool, scene.hash, scene.types, scene.frames, scene.elements, scene.setter, scene.advance, scene.random, scene.assets, scene.state, {});
    throwsAs(() => missing.handle(SOURCE, 0, 0, 0), Error, /선택된 실제 본문 미연결/);
    put(scene.pool.allocated(SOURCE), 0, 0x11018000);
    throwsAs(() => missing.handler()(SOURCE, 0, 0, 0), TypeError, /Regular 부모 Handle 미연결/);
    const fallback = missing.handler(() => 0.5);
    assert.equal(fallback(SOURCE, 0, 0, 0), 0.5);
    const other = new AssetRegularScene();
    throwsAs(() => new RawAssetRegular(scene.pool, scene.hash, scene.types, scene.frames, scene.elements, other.setter, other.advance, scene.random, scene.assets, scene.state, {}), TypeError, /불일치/);
});

// ---- 기준선 뒤 Native 진입 동치 (2026-11-10 WB04-E255) ----

test('AssetRegular: Native 간헐천 본문(runNativeGeyserRegular)이 typed 와 같은 fixture 511행을 그대로 재현한다', skip, () => {
    const scene = new AssetRegularScene();
    let count = 0;
    // 간헐천(kind 1) 행만 골라 현재 표시 접두/비용·프레임/후행 Damageable 을 분리 본문으로 실행한다.
    for (const row of rows) {
        if (!row[1].startsWith('1,')) continue;
        scene.prepare(row);
        const { input } = scene;
        const result = runNativeGeyserRegular(scene.pool, scene.types, SOURCE, input[1], input[2], bitsToFloat(input[3]),
            scene.hooks().displayUpdate, (sid, event, times, payload) => scene.regular.damageableHandle(sid, event, times, payload));
        const mismatch = compareRow(scene, row, result);
        assert.equal(mismatch, null, `Native 간헐천 행 ${count + 1}: ${mismatch}`);
        count += 1;
    }
    assert.equal(count, 511);
});

test('AssetRegular: Native 신전 진입(공유 Fort 모드·상태 통지·억제 getter·Descend)이 typed 와 같은 fixture 결과를 낸다', skip, () => {
    const scene = new AssetRegularScene();
    const states = [];
    let descends = 0;
    let count = 0;
    // 신전(kind 2) 행만 골라 독립 입력 대신 장기 owner 가 빌려주는 현재 값/통지 경로로 다시 실행한다.
    for (const row of rows) {
        if (!row[1].startsWith('2,')) continue;
        scene.prepare(row);
        const { input, state } = scene;
        const base = scene.hooks();
        // 독립 입력과 반대 값을 넣어 두고 공유 getter 만이 현재 값을 정하게 한다.
        state.fortMode = input[8] === 0;
        state.sharedFortMode = () => input[8] !== 0;
        state.suppressLeaveRequest = input[10] === 0;
        const hooks = {
            ...base,
            // 004ac220 통지: 상태 WORD 를 쓰는 실제 부모 경로를 흉내 낸다.
            state: (sid, word) => {
                states.push(word);
                put(scene.pool.allocated(sid), 12, word, 2);
            },
            suppressLeaveRequest: () => input[10] !== 0,
            // 지역 조회 뒤 애니메이션 통지는 Descend 본문 안에서 일어난다. 독립 입력용 두 훅은 도달하면 안 된다.
            descend: (x, y) => {
                descends += 1;
                const region = base.regionNumber(x, y);
                base.removeAnimation(17, ((3 << 16) | region) >>> 0);
            },
            regionNumber: undefined,
            removeAnimation: undefined,
        };
        const native = new RawAssetRegular(scene.pool, scene.hash, scene.types, scene.frames, scene.elements, scene.setter, scene.advance, scene.random, scene.assets, state,
            { ...hooks, regionNumber: undefined, removeAnimation: undefined });
        // typed 에서 쓰던 두 훅은 Descend 안에서만 base 로 부른다.
        const result = native.handle(SOURCE, input[1], input[2], bitsToFloat(input[3]));
        const mismatch = compareRow(scene, row, result);
        assert.equal(mismatch, null, `Native 신전 행 ${count + 1}: ${mismatch}`);
        count += 1;
    }
    assert.equal(count, 257);
    assert.ok(states.length > 0 && descends > 0);
    // 상태 통지는 이탈(2)·Descend 시작(4)·Descend 끝(0)만 쓴다.
    assert.ok(states.every((word) => word === 0 || word === 2 || word === 4));
});

test('AssetRegular: Factory 현재 레벨 getter(virtualCount)와 DamageRegular 의 native 선검사 순서', () => {
    const scene = new AssetRegularScene();
    const raw = scene.pool.allocated(SOURCE);
    raw.fill(0);
    put(raw, 0, FACTORY_TABLE);
    raw[10] = 85;
    raw[11] = 0;
    raw[33] = 1;
    const trace = [];
    let level = 3;
    const hooks = {
        virtualCount: () => level,
        factoryFlare: (parent, count, code) => trace.push(`flare:${parent}:${count}:${code}`),
        damageGround: (x, y) => trace.push(`ground:${floatToBits(x)}:${floatToBits(y)}`),
    };
    const regular = new RawAssetRegular(scene.pool, scene.hash, scene.types, scene.frames, scene.elements, scene.setter, scene.advance, scene.random, scene.assets, scene.state, hooks);
    // 슬롯의 레벨 비트(0)가 아니라 현재 가상 +80 getter(3)가 상위 레벨 플레어를 정한다.
    assert.equal(regular.handle(SOURCE, 5, 0, 7), 7);
    level = 2;
    assert.equal(regular.handle(SOURCE, 5, 0, 7), 7);
    assert.deepEqual(trace, [`flare:${SOURCE}:2:1`]);
    trace.length = 0;
    // typed 는 지면 피해 전에 HP 장치를 요구하고, native 는 지면 피해를 먼저 실행한 뒤 HP 장치 부재를 보고한다.
    throwsAs(() => regular.damageRegular(SOURCE, 0), Error, /가상 HP 몸체/);
    assert.deepEqual(trace, []);
    throwsAs(() => regular.damageRegular(SOURCE, 0, true), Error, /현재HP종단/);
    assert.equal(trace.length, 1);
});

// ---- 경계 계약: fixture 가 구별하지 못해 변형 시험에서 살아남은 입력 (2026-11-10 WB04-E255) ----

// 행 하나를 준비한 장면에서 같은 월드의 모듈을 지정한 훅/상태로 다시 만든다. (2026-11-10 WB04-E255)
function sceneWith(prefix, override = {}) {
    const scene = new AssetRegularScene();
    const row = rows.find((candidate) => candidate[1].startsWith(prefix));
    assert.ok(row, `fixture 행 ${prefix}`);
    scene.prepare(row);
    const rebuild = (extra = {}) => new RawAssetRegular(scene.pool, scene.hash, scene.types, scene.frames, scene.elements, scene.setter, scene.advance,
        scene.random, scene.assets, scene.state, { ...scene.hooks(), ...override, ...extra });
    scene.regular = rebuild();
    scene.rebuild = rebuild;
    return scene;
}

test('AssetRegular: 가상 표 기록값은 거주 1·간헐천 1·신전 4·Factory 2 가지만 복원된 부모로 인식한다', skip, () => {
    const scene = sceneWith('0,');
    const recognized = [0x50f540, 0x50da88, 0x5196f0, 0x5197a8, 0x519860, 0x519918, 0x509148, 0x509210];
    // 각 기록값은 인식하고 바로 이웃한 값과 임의 합성 표는 거부한다.
    for (const table of recognized) {
        put(scene.pool.allocated(SOURCE), 0, table);
        assert.equal(scene.regular.handles(SOURCE), true, `표 ${table.toString(16)}`);
        put(scene.pool.allocated(SOURCE), 0, table + 1);
        assert.equal(scene.regular.handles(SOURCE), false, `표+1 ${table.toString(16)}`);
        put(scene.pool.allocated(SOURCE), 0, table - 1);
        assert.equal(scene.regular.handles(SOURCE), false, `표-1 ${table.toString(16)}`);
    }
    put(scene.pool.allocated(SOURCE), 0, 0x11018000);
    assert.equal(scene.regular.handles(SOURCE), false);
    // 신전 네 부류와 Factory 두 부류는 모두 해당 몸체로 분배된다(event1 은 프레임 진행을 일으킨다).
    for (const table of [0x5196f0, 0x5197a8, 0x519860, 0x519918, 0x509148, 0x509210]) {
        put(scene.pool.allocated(SOURCE), 0, table);
        scene.pool.allocated(SOURCE)[10] = 124;
        put(scene.pool.allocated(SOURCE), 36, 0, 4);
        scene.events.length = 0;
        assert.equal(scene.regular.handle(SOURCE, 1, 0, 0.5), 0.5);
        assert.ok(scene.events.length > 0 && scene.frame(SOURCE) === 1, `표 ${table.toString(16)} 분배`);
    }
});

test('AssetRegular: 부모 TYPE 이 표 크기 이상이면 외부 효과 전에 거부하고 그 아래 값은 가상 표 판정으로 넘어간다', skip, () => {
    const scene = sceneWith('0,');
    const raw = scene.pool.allocated(SOURCE);
    raw[10] = 188;
    throwsAs(() => scene.regular.handle(SOURCE, 0, 0, 0), RangeError, /자산 Handle 타입 범위/);
    raw[10] = 187;
    put(raw, 0, 0x11018000);
    throwsAs(() => scene.regular.handle(SOURCE, 0, 0, 0), TypeError, /가상 표 미복원/);
});

test('AssetRegular: 프레임 진행기가 다른 지정기에 연결돼 있으면 같은 풀이어도 거부하고 표 크기도 188칸이어야 한다', skip, () => {
    const scene = sceneWith('0,');
    const base = [scene.pool, scene.hash, scene.types, scene.frames, scene.elements, scene.setter, scene.advance, scene.random, scene.assets, scene.state, {}];
    const sameSetter = new SquidFrame(scene.pool, scene.types, { frameSize: () => [1, 1], update() {}, unpop() {}, pop() {} });
    const strangeAdvance = new RawFrameAdvance(scene.pool, scene.frames, sameSetter);
    const attempt = (index, value) => () => new RawAssetRegular(...base.map((item, at) => (at === index ? value : item)));
    throwsAs(attempt(6, strangeAdvance), TypeError, /불일치/);
    throwsAs(attempt(2, scene.types.slice(0, 187)), TypeError, /불일치/);
    throwsAs(attempt(3, scene.frames.slice(0, 187)), TypeError, /불일치/);
    throwsAs(attempt(4, scene.elements.slice(0, 187)), TypeError, /불일치/);
    throwsAs(attempt(2, [...scene.types, scene.types[0]]), TypeError, /불일치/);
    attempt(5, scene.setter)();
});

test('AssetRegular: 신전 event2 는 count 하위 5비트가 모두 1 일 때만 0프레임으로 되돌리고 2를 반환한다', skip, () => {
    const scene = sceneWith('2,2,');
    scene.state.fortMode = false;
    const raw = scene.pool.allocated(SOURCE);
    // 하위 5비트 31 과 그 위 비트가 섞인 값은 되돌림, 15/16/30/32 는 일반 진행(0.1)이다.
    for (const [count, wrap] of [[31, true], [63, true], [0x80000000 + 31, true], [15, false], [16, false], [30, false], [32, false], [0, false]]) {
        put(raw, 36, 5, 4);
        scene.events.length = 0;
        const result = scene.regular.handle(SOURCE, 2, count, 0.5);
        assert.equal(result, wrap ? 2 : Math.fround(0.1), `count ${count}`);
        if (wrap) assert.equal(scene.frame(SOURCE), 0, `count ${count} 프레임`);
    }
});

test('AssetRegular: 신전 event1 편집 모드는 현재 방향 글자 구간 길이-1 칸을 진행하고 길이가 다른 구간에서 한 바퀴 돈다', skip, () => {
    const scene = sceneWith('2,1,');
    // A 구간 3칸, B 구간 5칸, C 구간 2칸의 불균등 코드 표이다.
    scene.frames[124] = new FrameCodeTable([...'AAABBBBBCC'].map((letter) => ({ side: letter.charCodeAt(0), variant: 0x41, flags: 0 })));
    scene.state.sharedFortMode = () => true;
    scene.state.fortMode = false;
    const raw = scene.pool.allocated(SOURCE);
    put(raw, 36, 4, 4);
    assert.equal(scene.regular.handle(SOURCE, 1, 0, 0.25), 0.25);
    // B 구간(3..7)에서 현재 4 + (5-1) = 8 → 구간 길이 5 로 한 바퀴 돌아 3.
    assert.equal(scene.frame(SOURCE), 3);
    // 편집 모드가 아니면 한 칸만 진행한다.
    scene.state.sharedFortMode = () => false;
    scene.state.fortMode = true;
    put(raw, 36, 4, 4);
    assert.equal(scene.regular.handle(SOURCE, 1, 0, 0.25), 0.25);
    assert.equal(scene.frame(SOURCE), 5);
});

test('AssetRegular: 신전 이탈 event3 은 섬 번호를 BYTE 로 줄여 요청하고 상태 bit0 가 있을 때만 -1 을 반환한다', skip, () => {
    const scene = sceneWith('2,3,');
    const islands = [];
    let stateValue = 2;
    const regular = scene.rebuild({
        islandNumber: () => 300,
        requestLeave: (sid, island) => {
            islands.push(island);
            scene.pool.allocated(sid)[11] = stateValue;
        },
    });
    scene.state.boss = true;
    scene.state.suppressLeaveRequest = false;
    scene.state.sharedAuthority = null;
    put(scene.pool.allocated(SOURCE), 12, 0, 2);
    const reset = () => { scene.pool.physical(SOURCE)[11] = 0; };
    // bit1 만 있는 상태는 삭제 요청이 아니다. bit0 이 있으면 -1 이다.
    reset();
    assert.equal(regular.handle(SOURCE, 3, 3, 0.5), 0);
    stateValue = 3;
    reset();
    assert.equal(regular.handle(SOURCE, 3, 3, 0.5), -1);
    stateValue = 1;
    reset();
    assert.equal(regular.handle(SOURCE, 3, 3, 0.5), -1);
    assert.deepEqual(islands, [44, 44, 44]);
    // 임계(boss 3, 아님 23)보다 하나 적은 count 는 요청 없이 1 을 반환한다.
    assert.equal(regular.handle(SOURCE, 3, 2, 0.5), 1);
    scene.state.boss = false;
    reset();
    assert.equal(regular.handle(SOURCE, 3, 22, 0.5), 1);
    assert.equal(regular.handle(SOURCE, 3, 23, 0.5), -1);
    assert.equal(islands.length, 4);
});

test('AssetRegular: 신전 event3/4 의 상태 통지 훅은 직접 쓰기 대신 쓰이며 순서는 이탈 2 · Descend 시작 4 · 끝 0 이다', skip, () => {
    const scene = sceneWith('2,3,');
    const calls = [];
    const regular = scene.rebuild({
        state: (sid, word) => calls.push(['state', sid, word]),
        descend: (x, y) => calls.push(['descend', floatToBits(x), floatToBits(y)]),
    });
    scene.state.checkingForms = 1;
    scene.state.boss = true;
    const raw = scene.pool.allocated(SOURCE);
    put(raw, 12, 0x1234, 2);
    // 이탈 시작(count 0)은 통지 훅으로 상태 2 를 알리며 WORD 를 직접 바꾸지 않는다.
    assert.equal(regular.handle(SOURCE, 3, 0, 0.5), 1);
    assert.deepEqual(calls, [['state', SOURCE, 2]]);
    assert.equal(get(scene.pool.slot(SOURCE), 12, 2), 0x1234);
    // Descend(count 0)는 시작 4 → 현재 좌표의 Descend → 끝 0 이다.
    calls.length = 0;
    assert.equal(regular.handle(SOURCE, 4, 0, 0.5), 0);
    assert.deepEqual(calls, [['state', SOURCE, 4], ['descend', get(raw, 14), get(raw, 18)], ['state', SOURCE, 0]]);
    assert.equal(get(scene.pool.slot(SOURCE), 12, 2), 0x1234);
    // checking 이 꺼져 있으면 count 4 이하는 Descend 없이 1 이다. 시작 통지(count 0)만 일어난다.
    calls.length = 0;
    scene.state.checkingForms = 0;
    assert.equal(regular.handle(SOURCE, 4, 0, 0.5), 1);
    assert.equal(regular.handle(SOURCE, 4, 4, 0.5), 1);
    assert.deepEqual(calls, [['state', SOURCE, 4]]);
    // count 5 부터는 checking 과 무관하게 Descend 를 진행한다.
    calls.length = 0;
    assert.equal(regular.handle(SOURCE, 4, 5, 0.5), 0);
    assert.deepEqual(calls.map((call) => call[0]), ['descend', 'state']);
});

test('AssetRegular: 신전 event4 의 checking 은 현재 DWORD 의 어느 비트든 비영이면 켜진 것이고 함수 값은 소비 위치에서 읽는다', skip, () => {
    const scene = sceneWith('2,4,');
    const tokens = [];
    const regular = scene.rebuild({ removeAnimation: (type, token) => tokens.push([type, token]) });
    scene.state.regionGeneration = 0x8001;
    scene.state.animationType = 0x2000013;
    // 값 2(bit1 만)와 함수 형태도 비영이다.
    for (const checking of [2, 0x80000000, () => 2, true]) {
        tokens.length = 0;
        scene.state.checkingForms = checking;
        put(scene.pool.allocated(SOURCE), 12, 0, 2);
        assert.equal(regular.handle(SOURCE, 4, 1, 0.5), 0, `checking ${String(checking)}`);
        assert.equal(tokens.length, 1);
        // 서버 선택(상위 WORD)이 0x8000 이상이어도 토큰은 부호 없는 DWORD 이다.
        assert.deepEqual(tokens[0], [0x2000013, 0x8001000b]);
    }
    for (const checking of [0, false, () => 0]) {
        tokens.length = 0;
        scene.state.checkingForms = checking;
        assert.equal(regular.handle(SOURCE, 4, 1, 0.5), 1, `checking ${String(checking)}`);
        assert.equal(tokens.length, 0);
    }
});

test('AssetRegular: Damageable 건설 재시도는 extra bit0 일 때만 부착 프로세스를 조회하고 없을 때만 0 길이로 시작한다', skip, () => {
    const scene = sceneWith('3,');
    const calls = [];
    let attached = 0;
    const regular = scene.rebuild({
        findAttached: (sid, type) => { calls.push(`find:${sid}:${type}`); return attached; },
        startConstruction: (sid, duration) => calls.push(`start:${sid}:${floatToBits(duration)}`),
    });
    // 2(bit1 만)와 0 은 조회조차 하지 않고, 1·3 은 조회한다.
    for (const [extra, expected] of [[0, []], [2, []], [1, ['find:50:30', 'start:50:0']], [3, ['find:50:30', 'start:50:0']], [8, []]]) {
        calls.length = 0;
        scene.pool.allocated(SOURCE)[40] = extra;
        assert.equal(regular.damageableHandle(SOURCE, 6700, 0, 9), 0);
        assert.deepEqual(calls, expected, `extra ${extra}`);
    }
    // 이미 부착된 프로세스가 있으면 시작하지 않는다. 건설 프로세스 번호도 상태에서 읽는다.
    attached = 77;
    scene.state.constructionProcessType = 31;
    calls.length = 0;
    scene.pool.allocated(SOURCE)[40] = 1;
    regular.damageableHandle(SOURCE, 6700, 0, 9);
    assert.deepEqual(calls, ['find:50:31']);
    // 다른 사건은 payload 를 그대로 돌려준다.
    assert.equal(regular.damageableHandle(SOURCE, 1, 0, 9), 9);
    assert.equal(regular.damageableHandle(SOURCE, 6701, 0, 9), 9);
});

test('AssetRegular: Factory 현재 레벨은 슬롯 상태 상위 3비트이거나 getter 이며 0..15 밖이면 거부하고 0 프레임 범위는 난수를 한 번 소비한다', skip, () => {
    const scene = new AssetRegularScene();
    const raw = scene.pool.allocated(SOURCE);
    raw.fill(0);
    put(raw, 0, FACTORY_TABLE);
    raw[10] = 85;
    raw[33] = 1;
    const build = (hooks) => new RawAssetRegular(scene.pool, scene.hash, scene.types, scene.frames, scene.elements, scene.setter, scene.advance, scene.random, scene.assets, scene.state, hooks);
    // 슬롯의 상태 BYTE 상위 3비트가 현재 레벨이다: 64 = 2(플레어 없음), 96 = 3, 224 = 7.
    for (const [state, flare] of [[64, false], [96, true], [224, true], [31, false], [0, false]]) {
        const flares = [];
        raw[11] = state;
        const regular = build({ factoryFlare: (parent, count, code) => flares.push([parent, count, code]) });
        assert.equal(regular.handle(SOURCE, 5, 0, 0.5), 0.5);
        assert.equal(flares.length, flare ? 1 : 0, `상태 ${state}`);
    }
    // getter 레벨 15 는 허용하지만(구간 'P' 는 비어 있어 0 범위 난수 오류), 16 과 음수는 레벨 범위 오류이다.
    raw[11] = 0;
    let level = 0;
    const regular = build({ virtualCount: () => level });
    for (const bad of [16, -1]) {
        level = bad;
        throwsAs(() => regular.handle(SOURCE, 2, 0, 0.5), RangeError, /레벨 범위/);
    }
    level = 15;
    const before = scene.random.state;
    const probe = new GameRandom(before);
    probe.next(1);
    throwsAs(() => regular.handle(SOURCE, 2, 0, 0.5), RangeError, /난수0범위/);
    assert.equal(scene.random.state, probe.state);
    assert.notEqual(before, probe.state);
    // 같은 레벨 범위 검사는 프레임 수 조회에도 적용된다(첫 프레임 조회는 정상 레벨, 두 번째 조회가 16).
    let calls = 0;
    const flip = build({ virtualCount: () => { calls += 1; return calls === 1 ? 0 : 16; } });
    throwsAs(() => flip.handle(SOURCE, 2, 0, 0.5), RangeError, /레벨 범위/);
    assert.equal(calls, 2);
});

test('AssetRegular: 거주 구역 지면 테마는 설정 훅이 있으면 훅 값을 쓰고, 원소 표 밖 신전 TYPE 은 거부하며, 최대 조사 수는 signed 로 비교한다', skip, () => {
    const scene = sceneWith('0,0,');
    const theme = rows.find((row) => row[1].startsWith('0,3,') && row[1].split(',')[14] === '2');
    assert.ok(theme);
    // 같은 요새에서 설정 값(state.themeMode)은 꺼두고 현재 설정 훅만 켜면 켠 것과 같고, 훅이 끄면 꺼진 것과 같다.
    const run = (themeMode, hook) => {
        scene.prepare(theme);
        scene.state.themeMode = themeMode;
        scene.regular = hook === undefined ? scene.rebuild() : scene.rebuild({ themeMode: hook });
        scene.regular.handle(SOURCE, 3, 0, 0.5);
        return hex(scene.pool.slot(SOURCE));
    };
    const stateOn = run(true);
    assert.notEqual(stateOn, run(false));
    assert.equal(run(false, () => 1), stateOn);
    assert.notEqual(run(true, () => 0), stateOn);
    // 일치한 신전의 TYPE 이 원소 표(188칸) 밖이면 거부한다.
    scene.prepare(theme);
    scene.pool.allocated(62)[10] = 200;
    throwsAs(() => scene.regular.handle(SOURCE, 3, 0, 0.5), RangeError, /원소 범위/);
    // 최대 조사 수는 signed 비교이다: 현재 값이 signed 음수면 새 조사 수가 이긴다(좌표 관찰 중 조사 수가 1 이상이고 초기 최대가 0 인 행 사용).
    const scan = coordinateRows.find((row) => number(row[6]) >= 1 && row[1].split(',')[16] === '0');
    assert.ok(scan);
    placeCoordinates(scene, scan);
    scene.state.maxDecorationScan = 0x80000000;
    scene.regular.handle(SOURCE, scene.input[1], scene.input[2], bitsToFloat(scene.input[3]));
    assert.equal(scene.state.maxDecorationScan, number(scan[6]));
    // 양수 현재 값이 더 크면 유지하고, 같은 값이면 그대로이다.
    placeCoordinates(scene, scan);
    scene.state.maxDecorationScan = 0x7fffffff;
    scene.regular.handle(SOURCE, scene.input[1], scene.input[2], bitsToFloat(scene.input[3]));
    assert.equal(scene.state.maxDecorationScan, 0x7fffffff);
});

// ---- 거주 구역 본문 경계 계약 (2026-11-10 WB04-E255) ----

// 거주 구역 사건 1(장식 조사)을 통제된 난수와 장식 배치로 실행하는 작은 장면이다. 난수 스텁은 요청한 범위 크기를 기록하고 미리 정한 값을 차례로 돌려준다.
// decorations 는 [x, y, frame, flags] 목록이고 변경된 장식은 [SID, 새 프레임] 으로 기록된다. (2026-11-10 WB04-E255)
function miniResidence({ x, y, randoms = [0, 0], decorations = [], extraHooks = {}, frameFlags = 8, runEvent = 1, rawX = null }) {
    const pool = new SidPool(CAPACITY, { server: false });
    for (let sid = 5; sid <= 70; sid += 1) assert.equal(pool.allocate(2), sid);
    const hash = new SquidHash();
    const types = blankTypes();
    types[89].footX = 1;
    types[89].footY = 1;
    types[91].footX = 1;
    types[91].footY = 1;
    const frames = types.map(() => codes(frameFlags));
    const place = (sid, typeNumber, px, py, frame) => {
        const raw = pool.physical(sid);
        raw.fill(0);
        raw[10] = typeNumber;
        put(raw, 0, 0x11018000);
        put(raw, 14, floatToBits(px));
        put(raw, 18, floatToBits(py));
        put(raw, 36, frame, 4);
        raw[33] = 1;
        put(raw, 4, hash.bucket(1, px, py), 2);
        hash.setBucket(1, px, py, sid);
    };
    place(50, 89, x, y, 0);
    // 해시 등록이 가능한 좌표로 배치한 뒤 실제로 읽힐 x 비트만 바꾸는 입력(비유한 등)이다.
    if (rawX !== null) put(pool.physical(50), 14, floatToBits(rawX));
    decorations.forEach(([px, py, frame = 0], index) => place(51 + index, 91, px, py, frame));
    const asked = [];
    const queue = [...randoms];
    const changed = [];
    const scans = [];
    const hooks = {
        random: () => ({ next: (limit) => { asked.push(limit); return queue.shift() ?? 0; } }),
        frame: (sid, value, flags) => changed.push([sid, value, flags]),
        decorationType: () => 91,
        recordScan: (value) => scans.push(value),
        ...extraHooks,
    };
    const result = runResidenceRegular(pool, hash, types, frames, new SquidPostPopList(), 50, runEvent, 0, 0.5, hooks);
    return { result, asked, changed, scans, pool };
}

test('ResidenceRegular: 조사 중심은 X/Y 모두 10..246 으로 자르고 영역은 좌우 ±1·위 3·아래 min(y+32 float, 246) 이다', skip, () => {
    // 난수 한계는 (x+32)-(x-48)=80, (y+32)-(y-32)=64 이다. 중심이 10 미만이면 10 으로 올려 영역 x 9..11 을 조사한다.
    const low = miniResidence({ x: 20, y: 100, randoms: [0, 32], decorations: [[9.5, 100.5], [8.5, 100.5], [12.5, 100.5]] });
    assert.deepEqual(low.asked, [80, 64]);
    assert.deepEqual(low.changed.map((entry) => entry[0]), [51]);
    // 중심 raw X = 20-48 = -28 → 10, 중심 raw Y = 100-32+32 = 100.
    // 중심이 246 초과면 246 으로 내려 영역 x 245..247 을 조사한다.
    const high = miniResidence({ x: 240, y: 100, randoms: [79, 32], decorations: [[247.5, 100.5], [248.5, 100.5], [244.5, 100.5]] });
    assert.deepEqual(high.changed.map((entry) => entry[0]), [51]);
    // Y 도 같다: 아래로는 3 줄 위까지, 10 미만이면 10 으로 올린다(영역 y 7..아래).
    const top = miniResidence({ x: 100, y: 5, randoms: [48, 0], decorations: [[100.5, 7.5], [100.5, 6.5]] });
    assert.deepEqual(top.changed.map((entry) => entry[0]), [51]);
    // 246 초과면 246 으로 내려 영역 y 243..246 을 조사한다(아래 끝도 246 으로 제한).
    const bottom = miniResidence({ x: 100, y: 240, randoms: [48, 63], decorations: [[100.5, 243.5], [100.5, 242.5], [100.5, 246.5]] });
    assert.deepEqual(bottom.changed.map((entry) => entry[0]), [51]);
});

test('ResidenceRegular: 영역 아래 끝은 float 로 저장한 y+32 를 절삭하고 중심 좌표 절삭은 CRT 범위를 벗어나면 거부한다', skip, () => {
    // y = 128 - 2^-17 에서 y+32 = 160 - 2^-17 는 double 이면 159.99999 이고 float 로 저장하면 160 이 된다(아래 끝 160).
    const y = bitsToFloat(0x42ffffff);
    assert.ok(y < 128 && Math.fround(y + 32) === 160 && y + 32 < 160);
    const edge = miniResidence({ x: 100, y, randoms: [48, 0], decorations: [[100.5, 160.5]] });
    assert.deepEqual(edge.changed.map((entry) => entry[0]), [51]);
    // 중심 좌표가 비유한/±2^63 이상이면 난수를 소비하기 전에(좌표 계산 단계에서) CRT 범위 오류이다.
    for (const bad of [Infinity, -Infinity, NaN, bitsToFloat(0x5f000000), -bitsToFloat(0x5f000000) * 4]) {
        const state = { calls: 0 };
        throwsAs(() => miniResidence({ x: 100, rawX: bad, y: 100, extraHooks: { random: () => ({ next: () => { state.calls += 1; return 0; } }) } }), RangeError, /CRT 물리 범위/);
        assert.equal(state.calls, 0);
    }
});

test('ResidenceRegular: 조사 값은 float 로 저장하고 주민 nibble 은 4비트 전체로 판정한다', skip, () => {
    // 사건 0 의 예약값은 float32 로 저장한 값이다(double 그대로 반환하지 않는다).
    const pool = new SidPool(CAPACITY, { server: false });
    for (let sid = 5; sid <= 60; sid += 1) pool.allocate(2);
    const raw = pool.allocated(50);
    raw.fill(0);
    raw[10] = 89;
    raw[12] = 0xf8;
    put(raw, 36, 5, 4);
    const frames = [];
    const events = [];
    const hooks = {
        random: () => ({ next: () => 12345 }),
        notify: (sid) => events.push(['notify', sid, pool.slot(sid)[12]]),
        frame: (sid, value) => events.push(['frame', sid, value]),
    };
    const result = runResidenceRegular(pool, new SquidHash(), [], frames, new SquidPostPopList(), 50, 0, 0, 0, hooks);
    assert.equal(result, Math.fround(result));
    assert.equal(result, Math.fround(12345 * (20.0 / 65536.0) + 15.0));
    // nibble 8 은 0 이 아니므로 0 으로 바뀌고(상위 nibble 보존) 프레임은 짝수 바닥 + 새 nibble 이다.
    assert.deepEqual(events, [['notify', 50, 0xf0], ['frame', 50, 4]]);
    // nibble 0 은 1 이 된다.
    raw[12] = 0xa0;
    events.length = 0;
    runResidenceRegular(pool, new SquidHash(), [], frames, new SquidPostPopList(), 50, 0, 0, 0, hooks);
    assert.deepEqual(events, [['notify', 50, 0xa1], ['frame', 50, 5]]);
    // 음수 프레임은 0 쪽 나눗셈이다: -3 → -1*2 = -2 에 nibble 을 더한다.
    put(raw, 36, 0xfffffffd, 4);
    raw[12] = 0xa0;
    events.length = 0;
    runResidenceRegular(pool, new SquidHash(), [], frames, new SquidPostPopList(), 50, 0, 0, 0, hooks);
    assert.deepEqual(events[1], ['frame', 50, -1]);
});

test('ResidenceRegular: 장식 조사는 현재 장식 TYPE/프레임 범위를 읽기 전에 검사하고 첫 변경 뒤에는 더 바꾸지 않는다', skip, () => {
    // 범위 밖 프레임(코드 표 길이 48 과 같은 값, 음수)은 범위 오류이다.
    for (const frame of [48, 0xffffffff]) {
        throwsAs(() => miniResidence({ x: 100, y: 100, randoms: [0, 32], decorations: [[52.5, 100.5, frame]] }), RangeError, /현재 장식 frame 범위/);
    }
    // 장식 TYPE 이 프레임 표 길이 밖이면 거부한다. 표 길이 91 이면 TYPE 91 은 밖이다.
    const pool = new SidPool(CAPACITY, { server: false });
    for (let sid = 5; sid <= 60; sid += 1) pool.allocate(2);
    const hash = new SquidHash();
    const types = blankTypes();
    types[91].footX = 1;
    types[91].footY = 1;
    types[89].footX = 1;
    types[89].footY = 1;
    const place = (sid, typeNumber, px, py) => {
        const raw = pool.physical(sid);
        raw.fill(0);
        raw[10] = typeNumber;
        put(raw, 14, floatToBits(px));
        put(raw, 18, floatToBits(py));
        raw[33] = 1;
        put(raw, 4, hash.bucket(1, px, py), 2);
        hash.setBucket(1, px, py, sid);
    };
    place(50, 89, 100, 100);
    place(51, 91, 52.5, 100.5);
    const queue = [0, 32];
    const hooks = { random: () => ({ next: () => queue.shift() ?? 0 }), decorationType: () => 91, frame() {}, recordScan() {} };
    const shortFrames = Array.from({ length: 91 }, () => codes(8));
    throwsAs(() => runResidenceRegular(pool, hash, types, shortFrames, new SquidPostPopList(), 50, 1, 0, 0, hooks), RangeError, /현재 장식 TYPE 범위/);
    // 장식 둘이 같은 영역에 있어도 처음 바뀐 것 하나만 변경한다(변경 뒤 Next 는 한 번 실행하지만 조사 수에는 세지 않는다).
    const two = miniResidence({ x: 100, y: 100, randoms: [0, 32], decorations: [[52.5, 100.5], [52.5, 100.5]] });
    assert.equal(two.changed.length, 1);
    assert.deepEqual(two.scans, [1]);
});

test('ResidenceRegular: 지면 테마는 목록의 첫 소유자 일치 신전만 읽고 설정/원소 장치는 그 한 번만 부른다', skip, () => {
    const scene = sceneWith('0,3,');
    const calls = [];
    scene.assets.temples.entries = [62, 61, 0];
    scene.assets.temples.count = 2;
    // 두 신전의 소유자가 현재 거주 구역과 같고 두 번째는 다른 원소이다. 목록 순서의 첫 신전의 원소(2 → +6 간격 1)만 쓴다.
    scene.pool.allocated(61)[34] = scene.pool.slot(SOURCE)[34];
    scene.pool.allocated(62)[34] = scene.pool.slot(SOURCE)[34];
    scene.pool.allocated(61)[10] = 124;
    scene.pool.allocated(62)[10] = 90;
    scene.elements[90] = 2;
    scene.elements[124] = 0;
    const regular = scene.rebuild({ themeMode: () => { calls.push('theme'); return 1; } });
    regular.handle(SOURCE, 3, 0, 0.5);
    assert.deepEqual(calls, ['theme']);
    const first = get(scene.pool.slot(SOURCE), 36, 4);
    // 같은 장면에서 목록 순서를 뒤집으면 첫 신전이 바뀌어 결과가 달라진다.
    scene.pool.allocated(SOURCE).fill(0, 36, 40);
    scene.assets.temples.entries = [61, 62, 0];
    regular.handle(SOURCE, 3, 0, 0.5);
    assert.notEqual(get(scene.pool.slot(SOURCE), 36, 4), first);
    assert.equal(calls.length, 2);
});

test('ResidenceDudeSpawn: 개수는 signed 로 비교하고 Fort 는 호출 시점의 현재 값이며 source 0 진단은 정상 복귀한다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const population = new SquidPostPopList();
    population.entries = new Array(30).fill(0);
    const providers = new SquidPostPopList();
    providers.entries = new Array(30).fill(0);
    const calls = [];
    const hooks = {
        assertion: () => calls.push('assert'),
        entrance: (sid) => { calls.push(`entrance:${sid}`); return [1, 2]; },
        spawn: () => 0,
        task: () => calls.push('task'),
    };
    // 음수(0xffffffff = -1) Dude 수는 공급원 수 1 보다 작아 진행하고, 부호 없이 읽으면 막힌다.
    population.count = 0xffffffff;
    providers.count = 1;
    runResidenceDudeSpawn(pool, true, population, providers, 0, hooks);
    assert.deepEqual(calls, ['assert', 'entrance:0']);
    // 공급원 수가 음수면 Dude 수 0 이 더 크므로 진행하지 않는다.
    calls.length = 0;
    population.count = 0;
    providers.count = 0xffffffff;
    runResidenceDudeSpawn(pool, true, population, providers, 0, hooks);
    assert.deepEqual(calls, []);
    // Fort 가 getter 이면 호출 시점의 값을 읽는다.
    providers.count = 1;
    let fort = false;
    runResidenceDudeSpawn(pool, () => fort, population, providers, 0, hooks);
    assert.deepEqual(calls, []);
    fort = true;
    runResidenceDudeSpawn(pool, () => fort, population, providers, 0, hooks);
    assert.deepEqual(calls, ['assert', 'entrance:0']);
});
