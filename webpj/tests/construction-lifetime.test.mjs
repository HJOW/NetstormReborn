// 표시 목록 등록/제거와 건설 표시 버퍼 소멸 순서 raw-construction-lifetime.js(RawRenderRegistry·destroyConstructionVisual·makeConstructionRegistryHooks)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: constructionlifetime-x86.tsv 의 `originals` 459행(Construct 138·Sequence 1·Render 64·Build 64·DeleteBase 64·DeleteInstant 64·DeleteNormal 64) — 목록 용량(0~)·빈 칸 위치·부모·깊이 입력을 주었을 때
//    외부 요청 순서(A 확보/F:T 배열 반납/B 부착/F:S·O·P·B 버퍼·객체 반납), 슬롯 배열 전체, 필드 8개(form·parent·사각형·slot·zOrder), 삭제→재등록 연속 기록, 반환값이 같다. `originalCD`/`original1037` 행은 다른 판본이라 사용하지 않는다.
//  - 계약: 잘못된 등록·0 토큰·경계 누락은 정상 목록을 손상시키지 않고 거부, 확보 실패는 기존 배열/토큰을 보존하고 부착을 요청하지 않음, 건설 표시 생성에 목록 부착을 연결한 뒤 버퍼·목록을 함께 해제.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E129: 표시 목록 수명 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { RawConstructionVisual, createConstructionVisualFields, createConstructionVisualState } from '../www/core/raw-construction-visual.js';
import { ConstructionRelease, RawRenderRegistry, createRenderProcessFields, destroyConstructionVisual, makeConstructionRegistryHooks } from '../www/core/raw-construction-lifetime.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E129)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'constructionlifetime-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 독립 관찰의 입력 열 순서와 호스트 정규화 객체/부모 토큰이다. (2026-11-08 WB04-E129)
const CAPACITY = 0, PATTERN = 1, PARENT = 2, Z0 = 3, Z = 4, MUTATE = 5, BUFFERS = 6, FLAGS = 7, SLOT = 8;
const TOKEN = 1;
const PARENT_SID = 50;

// 예외가 나는지 본다. (2026-11-08 WB04-E129)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 빈 값은 실행기와 같은 '-' 로 표현해 순서를 유지하며 잇는다. (2026-11-08 WB04-E129)
function join(values, delimiter) {
    return values.length ? values.join(delimiter) : '-';
}

// 원본 포인터 목록을 토큰으로 정규화한 입력이다. 빈 칸 검색/확장을 미리 계산하지 않는다. (2026-11-08 WB04-E129)
function initial(input) {
    const slots = Array.from({ length: input[CAPACITY] }, (_, index) => 100 + index);
    if (slots.length && input[PATTERN]) slots[input[PATTERN] === 1 ? 0 : input[PATTERN] === 2 ? Math.floor(slots.length / 2) : slots.length - 1] = 0;
    return slots;
}

// 원본 필드/전체 목록을 관찰 순서로 직렬화한다. 기대 상태는 계산하지 않는다. (2026-11-08 WB04-E129)
function snapshot(registry, fields) {
    const slots = registry.slots.map(String);
    const values = [fields.form >>> 0, fields.parent >>> 0, floatToBits(fields.left), floatToBits(fields.top), floatToBits(fields.right), floatToBits(fields.bottom), fields.slot >>> 0, fields.zOrder >>> 0].map(String);
    return [join(slots, ':'), join(values, ':')];
}

// 실제 목록·입력 raw 자료와 외부 사건을 소유하는 장면이다. (2026-11-08 WB04-E129)
class LifetimeScene {
    constructor() {
        this.pool = new SidPool(32768, { server: false });
        this.input = [];
        this.events = [];
        this.registry = null;
    }

    // 호출 인자와 등록이 이미 끝난 부착 시점을 관찰한다.
    hooks() {
        return {
            // 원본 32비트 포인터 배열의 확보 바이트이다.
            reserve: (amount) => { this.events.push(`A:${amount}`); return true; },
            // 원본의 기존 배열 반납은 확장했을 때만 발생한다.
            releaseArray: () => { this.events.push('F:T'); },
            // 실제 BaseProcess 대신 명시 입력으로 부모/form 을 공급한다. 원본과 같은 슬롯/초기 깊이를 기록한다.
            attach: (fields, type, parent, flags) => {
                assert.ok(this.registry.slots[fields.slot] === TOKEN && type === 17 && flags === 16);
                this.events.push(`B:${type}:${parent}:0:${flags}:${fields.slot}:${fields.zOrder >>> 0}`);
                fields.form = 7;
                fields.parent = this.input[MUTATE] === 2 ? 51 : parent;
                if (this.input[MUTATE] === 1) this.pool.physical(parent)[35] = 128;
            },
        };
    }

    // 기대 결과 없이 합성 슬롯/부모 깊이와 포인터가 있는 버퍼만 준비한다.
    prepare(operation, values) {
        this.events = [];
        this.input = values.split(',').map(Number);
        this.pool.physical(0)[35] = this.input[Z0] & 0xff;
        this.pool.physical(PARENT_SID)[35] = this.input[Z] & 0xff;
        this.pool.physical(51)[35] = 127;
        const slots = initial(this.input);
        const fields = createRenderProcessFields();
        if (operation !== 'Construct' && operation !== 'Sequence') {
            slots[this.input[SLOT]] = TOKEN;
            fields.token = TOKEN;
            fields.slot = this.input[SLOT];
            fields.form = 7;
            fields.parent = 50;
            fields.zOrder = -128;
            fields.left = fields.top = fields.right = fields.bottom = bitsToFloat(0xa5a5a5a5);
        }
        this.registry = new RawRenderRegistry(this.pool, this.hooks(), slots);
        return fields;
    }

    // 반납 경계에서 아직 등록이 살아 있는지, 객체 반납 때 이미 제거되었는지도 확인한다.
    release(kind, fields) {
        const label = kind === ConstructionRelease.SOURCE ? 'S' : kind === ConstructionRelease.OUTPUT ? 'O' : kind === ConstructionRelease.PARTICLES ? 'P' : 'B';
        assert.equal(this.registry.slots[fields.slot], kind === ConstructionRelease.OBJECT ? 0 : TOKEN);
        this.events.push(`F:${label}`);
    }
}

test('ConstructionLifetime: 원본 10.78 표시 등록/삭제/연속 재사용 459행의 외부 요청·슬롯 배열·필드·기록·반환값이 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    const scene = new LifetimeScene();
    let count = 0;
    // 기대 필드/목록/호출 순서는 독립 PE fixture 에서 읽는다.
    for (const row of rows) {
        assert.equal(row.length, 8);
        const fields = scene.prepare(row[1], row[2]);
        const history = [];
        let result = 0;
        if (row[1] === 'Construct' || row[1] === 'Sequence') {
            scene.registry.construct(fields, TOKEN, scene.input[PARENT], 17);
            result = 1;
            if (row[1] === 'Sequence') {
                history.push(join(snapshot(scene.registry, fields), '|'));
                scene.registry.destroy(fields);
                history.push(join(snapshot(scene.registry, fields), '|'));
                scene.registry.construct(fields, TOKEN, scene.input[PARENT], 17);
                history.push(join(snapshot(scene.registry, fields), '|'));
            }
        } else if (row[1] === 'Render') scene.registry.destroy(fields);
        else {
            const visual = createConstructionVisualFields();
            if (scene.input[BUFFERS] & 1) visual.image.pixels = Uint8Array.of(17);
            if (scene.input[BUFFERS] & 2) visual.output = Uint8Array.of(31);
            if (scene.input[BUFFERS] & 4) visual.particles = [{ active: 0, color: 0, life: 0, pixel: 0 }];
            destroyConstructionVisual(visual, fields, scene.registry, row[1] === 'Build' ? 0 : scene.input[FLAGS], (kind) => scene.release(kind, fields));
            assert.ok(visual.image.pixels.length === 0 && visual.output.length === 0 && visual.particles.length === 0);
            if (row[1] !== 'Build') result = 1;
        }
        const observed = snapshot(scene.registry, fields);
        assert.deepEqual([join(scene.events, ';'), observed[0], observed[1], join(history, '~'), result], [row[3], row[4], row[5], row[6], Number(row[7])], `건설 표시 수명 행 ${count} ${row[1]}`);
        count += 1;
    }
    assert.equal(count, 459);
});

test('ConstructionLifetime: 잘못된 등록·0 토큰·경계 누락은 정상 목록을 손상시키지 않고 거부한다', () => {
    const scene = new LifetimeScene();
    const fields = scene.prepare('Construct', '40,1,50,128,255,0,7,0,19');
    assert.ok(throws(() => scene.registry.construct(fields, 0, PARENT_SID, 17)));
    scene.registry.construct(fields, TOKEN, PARENT_SID, 17);
    const before = snapshot(scene.registry, fields);
    assert.ok(throws(() => scene.registry.construct(fields, TOKEN, PARENT_SID, 17)));
    assert.deepEqual(snapshot(scene.registry, fields), before);
    assert.ok(throws(() => scene.registry.construct(fields, 999, PARENT_SID, 17)));
    assert.deepEqual(snapshot(scene.registry, fields), before);
    let invalid = { ...fields, slot: -1 };
    assert.ok(throws(() => scene.registry.destroy(invalid)));
    invalid = { ...fields, token: 999 };
    assert.ok(throws(() => scene.registry.destroy(invalid)));
    assert.deepEqual(snapshot(scene.registry, fields), before);
    scene.registry.destroy(fields);
    assert.ok(throws(() => scene.registry.destroy(fields)));
    const hooks = scene.hooks();
    delete hooks.attach;
    assert.ok(throws(() => new RawRenderRegistry(scene.pool, hooks)));
    assert.ok(throws(() => makeConstructionRegistryHooks(scene.registry, fields, 0, {})));
    assert.ok(throws(() => destroyConstructionVisual(createConstructionVisualFields(), fields, scene.registry, 0, null)));
});

test('ConstructionLifetime: 확보가 실패하면 기존 배열/토큰을 보존하고 Kernel 부착을 요청하지 않는다', () => {
    const scene = new LifetimeScene();
    scene.input = [1, 0, 50, 128, 255, 0, 7, 0, 0];
    const hooks = scene.hooks();
    hooks.reserve = () => false;
    const registry = new RawRenderRegistry(scene.pool, hooks, [99]);
    const fields = createRenderProcessFields();
    assert.ok(throws(() => registry.construct(fields, TOKEN, PARENT_SID, 17)));
    assert.deepEqual(registry.slots, [99]);
    assert.equal(scene.events.length, 0);
});

test('ConstructionLifetime: 건설 표시 생성에 목록 부착을 연결하고 버퍼·목록을 함께 해제한 뒤 같은 토큰을 재사용한다', () => {
    const scene = new LifetimeScene();
    scene.input = [0, 0, 50, 128, 255, 0, 7, 0, 0];
    scene.registry = new RawRenderRegistry(scene.pool, scene.hooks());
    const raw = scene.pool.physical(PARENT_SID);
    put(raw, 0, 0x501dd0);
    raw[10] = 100;
    raw[11] = 0;
    raw[34] = 1;
    put(raw, 14, floatToBits(10));
    put(raw, 18, floatToBits(20));
    put(raw, 26, 0);
    const random = new GameRandom(0);
    const visualState = createConstructionVisualState();
    visualState.palette = 71;
    // 빈 저장소를 처음 만들 때도 최소 40칸을 확보한다. 같은 토큰은 삭제 후 다시 쓸 수 있다.
    for (const instant of [false, true]) {
        const registration = createRenderProcessFields();
        const hooks = {
            reserve: () => true,
            createPalette: () => 71,
            gradient: () => {},
            buildPalette: () => {},
            capture: () => ({ width: 5, height: 7, hotspotX: 2, hotspotY: 3, pixels: new Uint8Array(35).fill(17) }),
            releaseParticles: () => {},
        };
        const visual = new RawConstructionVisual(scene.pool, visualState, random, makeConstructionRegistryHooks(scene.registry, registration, TOKEN, hooks));
        const fields = visual.create(PARENT_SID, instant);
        assert.ok(registration.slot === 0 && scene.registry.slots[0] === TOKEN && fields.parent === PARENT_SID);
        assert.equal(fields.zOrder, 0);
        scene.events = [];
        destroyConstructionVisual(fields, registration, scene.registry, 0, (kind) => scene.release(kind, registration));
        assert.ok(scene.registry.slots[0] === 0 && registration.form === -12345 && registration.parent === -12345);
        assert.equal(scene.events.join(';'), 'F:S;F:O;F:P');
        assert.ok(fields.image.pixels.length === 0 && fields.output.length === 0 && fields.particles.length === 0);
        assert.ok(visualState.palette === 71 && fields.palette === 71 && scene.registry.slots.length === 40);
    }
});
