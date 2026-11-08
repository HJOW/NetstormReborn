// 크로스보우 발사 raw-crossbow-animation.js(RawCrossbowAnimation)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: crossbow-animation-x86.tsv 2638행 — 몸체(0 아처 공유 회전/1 내림 발사/2 발사 생성자/3 회전 생성자/4 launch)·부모 프레임·시각 도달·표적 상태/HP·방향 자료·표적 중심·aim 바이트·표적 WORD·재진입 입력으로
//    반환, 외부 효과 순서(C 중심/U·P 공간/T 표적점/A·J Missile 인자/M 통지/K 삭제/N·F·S 회전 소리/B 부착), 필드 이미지(회전 88/발사 64바이트), 네 슬롯 200바이트가 같다.
//  - 계약: 실제 Missile 경계 누락을 성공으로 우회하지 않음, 판본/표 크기, 프로세스 보조.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E116: 크로스보우 발사 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GunAnimationFields } from '../www/core/raw-gun-animation.js';
import { ArcherAnimationFields, ArcherAnimationState, RawArcherAnimation } from '../www/core/raw-archer-animation.js';
import { CrossbowAnimationProcess, CrossbowAnimationState, RawCrossbowAnimation } from '../www/core/raw-crossbow-animation.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E116)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'crossbow-animation-x86.tsv')) ?? [];
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 예외가 나는지 본다. (2026-11-08 WB04-E116)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 동일 현재 풀·TYPE 과 선언한 외부 경계를 가진 독립 입력 장면이다. (2026-11-08 WB04-E116)
function makeScene() {
    const scene = {
        pool: new SidPool(24000, { server: false }),
        types: Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, group: 0 })),
        data: Array.from({ length: 188 }, () => ({ directions: 0, frameCount: 0 })),
        rotateState: new ArcherAnimationState(),
        state: new CrossbowAnimationState(),
        fields: new ArcherAnimationFields(),
        // 훅이 바꾸는 현재 필드 이미지이다(발사 몸체에서는 64바이트 이미지로 바뀐다).
        active: null,
        row: null,
        events: []
    };
    scene.active = scene.fields;
    // 실제 부모 50/51·표적 60/61·GunForm 70 을 먼저 할당한다.
    for (let id = 5; id <= 70; id += 1) assert.equal(scene.pool.allocate(2), id);
    scene.n = (index) => Number(scene.row[index]);
    scene.event = (text) => scene.events.push(text);
    // 프레임 실행 전 원본과 같은 패턴·raw 슬롯을 작성한다.
    scene.prepare = (row) => {
        assert.equal(row.length, 16);
        scene.row = row;
        scene.events = [];
        scene.state = new CrossbowAnimationState({ now: 10 });
        scene.rotateState = new ArcherAnimationState({ type: 140, now: 10 });
        scene.data[140] = { directions: scene.n(5) | 0, frameCount: scene.n(6) | 0 };
        scene.types[158].flags1 = scene.n(4) ? 0x10 : 0;
        const bytes = scene.fields.bytes;
        scene.active = scene.fields;
        // 생성자의 미초기화 padding 도 대조한다.
        for (let index = 0; index < bytes.length; index += 1) bytes[index] = (index * 13 + 7) & 0xff;
        const fire = scene.n(0) === 1 || scene.n(0) === 2 || scene.n(0) === 4;
        put(bytes, 0, fire ? 0x50b120 : 0x50b450);
        put(bytes, 4, 81);
        put(bytes, 8, 50);
        put(bytes, 12, floatToBits(60.0));
        put(bytes, 16, floatToBits(60.0));
        put(bytes, 20, floatToBits(59.5));
        put(bytes, 24, floatToBits(59.5));
        put(bytes, 32, fire ? 0x50b11c : 0x50b448);
        bytes[40] = 0x20;
        put(bytes, 42, 70, 2);
        put(bytes, 44, scene.n(10), 2);
        const time = new DataView(new ArrayBuffer(8));
        time.setFloat64(0, scene.n(2) ? 10.0 : 10.25, true);
        put(bytes, 48, time.getUint32(0, true));
        put(bytes, 52, time.getUint32(4, true));
        put(bytes, 56, 0);
        // 원문과 같은 네 관찰 슬롯과 Gun 을 준비한다.
        for (const [sid, type] of [[50, 140], [51, 140], [60, 158], [61, 158], [70, 34]]) {
            const raw = scene.pool.physical(sid);
            raw.fill(0);
            put(raw, 0, 0x1131a000);
            raw[10] = type;
            raw[11] = sid === 60 ? scene.n(3) : 0;
            raw[12] = 0xa5;
            raw[34] = 1;
            raw[30] = scene.n(9);
            put(raw, 14, floatToBits(sid < 60 ? 50.5 : 60.0));
            put(raw, 18, floatToBits(sid < 60 ? 50.5 : 60.0));
            put(raw, 36, scene.n(1));
        }
    };
    // 원본 실행과 동일하게 선언한 외부 재진입만 현재 입력에 적용한다.
    scene.mutate = (where) => {
        const mutation = scene.n(11);
        const bytes = scene.active.bytes;
        if (where === 'Center' && mutation === 1) put(scene.pool.physical(50), 36, 7);
        if (where === 'Center' && mutation === 2) put(bytes, 44, 61, 2);
        if (where === 'Unpop' && mutation === 3) {
            scene.state.now = 100;
            const bits = new DataView(new ArrayBuffer(8));
            bits.setFloat64(0, 777.0, true);
            put(bytes, 48, bits.getUint32(0, true));
            put(bytes, 52, bits.getUint32(4, true));
        }
        if (where === 'Repop' && mutation === 4) scene.pool.physical(60)[11] = 2;
        if (where === 'Point' && mutation === 5) {
            put(bytes, 8, 51);
            put(bytes, 44, 61, 2);
            put(bytes, 12, floatToBits(77.25));
            put(bytes, 16, floatToBits(78.5));
            scene.state.speed = 37;
            scene.state.displayType = 82;
        }
        if (where === 'Missile' && mutation === 6) put(bytes, 42, 71, 2);
    };
    // 실제 생성 descriptor 의 인수만 직렬화하고 투사체 실행으로 간주하지 않는다.
    scene.projectile = (request) => {
        scene.event('A:128');
        const args = [request.firing, request.target, floatToBits(request.speed), request.displayType, request.effectType,
            floatToBits(request.targetPosition[0]), floatToBits(request.targetPosition[1]), floatToBits(request.targetCenter[0]), floatToBits(request.targetCenter[1]),
            request.ricochet, floatToBits(request.firingPosition[0]), floatToBits(request.firingPosition[1])];
        scene.event(`J:${args.join(',')}`);
        scene.mutate('Missile');
    };
    // 실제 공통 경계의 인수와 순서를 원문과 같은 형태로 관찰한다.
    scene.hooks = () => ({
        center: (sid) => {
            scene.event(`C:${sid}`);
            const point = sid === 50 ? [50, 50] : [bitsToFloat(scene.n(7)), bitsToFloat(scene.n(8))];
            scene.mutate('Center');
            return point;
        },
        unpop: (sid, flags) => { scene.event(`U:${sid}:${flags}`); scene.mutate('Unpop'); },
        repop: (sid, flags) => { scene.event(`P:${sid}:${flags}`); scene.mutate('Repop'); },
        targetPoint: (source, target) => { scene.event(`T:${source}:${target}`); scene.mutate('Point'); return [52.25, 52.75]; },
        createProjectile: (request) => scene.projectile(request),
        notify: (sid, event, info, value) => scene.event(`M:${event}:${info}:${value}`),
        kill: (form, flags) => scene.event(`K:${form}:${flags}`),
        randomInteger: (low, high) => { scene.event(`N:${low}:${high}`); scene.event('F:3'); return 3000; },
        sound: (sid, name) => scene.event(`S:${name}`)
    });
    // 몸체를 실행하고 필드 이미지·효과·네 슬롯을 돌려준다. 회전/회전 생성자는 아처 88바이트, 그 밖은 64바이트 이미지이다.
    scene.run = () => {
        const rotate = new RawArcherAnimation(scene.pool, scene.types, scene.data, scene.rotateState, { common: scene.hooks(), aimDirection: (sid) => scene.pool.slot(sid)[30] });
        let result = 'return';
        const kind = scene.n(0);
        let image;
        if (kind === 0) rotate.rotate(scene.fields);
        else if (kind === 3) {
            rotate.initialize(scene.fields, false, 70, scene.n(10));
            scene.event('B:36:50:0:16');
            put(scene.fields.bytes, 4, 81);
            put(scene.fields.bytes, 8, 50);
        } else {
            // 발사 이미지는 64바이트만 복사하되 외부 효과가 같은 필드를 즉시 바꿀 수 있게 한다.
            image = new GunAnimationFields();
            image.bytes.set(scene.fields.bytes.subarray(0, 64));
            scene.active = image;
            const current = new RawCrossbowAnimation(scene.pool, scene.types, scene.data, scene.state, scene.hooks());
            if (kind === 1) current.frame(image);
            else if (kind === 2) {
                current.initialize(image, 70, scene.n(10));
                scene.event('B:35:50:0:16');
                put(image.bytes, 4, 81);
                put(image.bytes, 8, 50);
            } else result = current.launch(image) ? '1' : '0';
            scene.fields.bytes.set(image.bytes, 0);
        }
        let slots = '';
        // 네 원본 슬롯 200바이트를 전부 대조한다.
        for (const sid of [50, 51, 60, 61]) slots += hex(scene.pool.slot(sid));
        const length = kind === 0 || kind === 3 ? 88 : 64;
        return [result, scene.events.join(';'), hex(scene.fields.bytes.subarray(0, length)), slots];
    };
    return scene;
}

// 지정한 원본 몸체 집합을 독립 기대값과 대조한다. (2026-11-08 WB04-E116)
function replay(low, high) {
    const scene = makeScene();
    let count = 0;
    // 저장된 실제 원본 입력만 재생한다.
    for (const row of rows) {
        if (Number(row[0]) < low || Number(row[0]) > high) continue;
        scene.prepare(row);
        assert.deepEqual(scene.run(), row.slice(12), `크로스보우 행 ${count}: 몸체 ${row[0]} ${row.slice(1, 12).join(',')}`);
        count += 1;
    }
    return count;
}

test('CrossbowAnimation: 다른 부모 TYPE 140 의 실제 공유 회전(아처 rotate)이 원본과 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    assert.ok(replay(0, 0) > 0);
});

test('CrossbowAnimation: 내림 프레임·due 전 추적 금지·생존 snapshot·외부 재진입이 원본과 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    assert.ok(replay(1, 1) > 0);
});

test('CrossbowAnimation: 두 생성자와 launch 의 HP/free/dead/arrow 인자가 원본과 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    assert.ok(replay(2, 4) > 0);
    assert.equal(rows.length, 2638);
});

test('CrossbowAnimation: Missile 경계 누락·판본/표 크기·프로세스 보조를 확인한다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    const scene = makeScene();
    // 실제 투사체 생성 경계를 빠뜨린 살아 있는 HP 표적은 성공으로 처리하지 않는다.
    const found = rows.find((row) => Number(row[0]) === 4 && Number(row[3]) === 0 && Number(row[4]) === 1);
    assert.ok(found);
    scene.prepare(found);
    const hooks = scene.hooks();
    delete hooks.createProjectile;
    const logic = new RawCrossbowAnimation(scene.pool, scene.types, scene.data, scene.state, hooks);
    const image = new GunAnimationFields();
    image.bytes.set(scene.fields.bytes.subarray(0, 64));
    assert.ok(throws(() => logic.launch(image)));
    assert.deepEqual(scene.events, []);
    // 판본/표 크기/현재 TYPE 오류.
    assert.ok(throws(() => new RawCrossbowAnimation({ edition: 'cd1072' }, scene.types, scene.data, scene.state, {})));
    assert.ok(throws(() => new RawCrossbowAnimation(scene.pool, scene.types, scene.data.slice(0, 10), scene.state, {})));
    assert.ok(throws(() => new RawCrossbowAnimation(scene.pool, scene.types, scene.data, new CrossbowAnimationState({ type: 188 }), {})));
    assert.equal(logic.pool, scene.pool);
    assert.equal(logic.type, 140);
    scene.state.currentType = { value: 80 };
    assert.equal(logic.type, 80);
    scene.state.currentType = null;
    assert.ok(throws(() => logic.initialize(new GunAnimationFields(), 0, 60)));
    // 프로세스 보조: 부착 전 Form/Parent 0 → 프레임은 부모 범위 오류로 거부된다(시각 도달 시).
    const fields = new GunAnimationFields();
    logic.initialize(fields, 70, 60);
    const process = new CrossbowAnimationProcess(logic, fields);
    process.onAttached();
    assert.equal(process.fields.form, 0);
    assert.ok(throws(() => process.runFrame()));
});
