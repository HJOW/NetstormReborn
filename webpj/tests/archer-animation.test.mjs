// SunArcher 애니메이션 raw-archer-animation.js(RawArcherAnimation)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: gunarcher-x86.tsv 의 `originals` 행 — 몸체(0 회전/1 발사/2 가장 가까운 방향/3 초기 각/4 발사 프레임/5 생성자+Fire 초기화/6 launch)와
//    프레임·delta·각상태·표적 상태·시각 도달·방향 자료로 반환(또는 Missile 생성 접두), 외부 효과 순서(C 중심/U·P 공간/S 소리/N·F 난수/M 통지/K 삭제/T 표적점/A·J Missile 인자/Q 출력 통지/B 부착),
//    0x58바이트 필드 이미지, 부모 슬롯 50바이트, float 반환 버퍼가 같다. `originalCD` 행은 10.72 라 사용하지 않는다.
//  - 계약: 실제 Missile 경계 누락을 성공으로 우회하지 않음, 판본/표 크기, 프로세스 보조.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E115: 아처 애니메이션 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { ArcherAnimationFields, ArcherAnimationProcess, ArcherAnimationState, RawArcherAnimation } from '../www/core/raw-archer-animation.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E115)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'gunarcher-x86.tsv'));
const rows = allRows ? allRows.filter((row) => row[0] === 'originals') : [];
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 예외가 나는지 본다. (2026-11-08 WB04-E115)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 별도 launch 의 생성 접두 정지를 같은 시점에 관찰하는 표식이다. (2026-11-08 WB04-E115)
const PREFIX_STOP = Symbol('archer-prefix-stop');

// 원본 현재 타입/각상태와 외부 관찰 경계를 공유하는 독립 입력 장면이다. (2026-11-08 WB04-E115)
function makeScene() {
    const scene = {
        pool: new SidPool(24000, { server: false }),
        types: Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, group: 0 })),
        data: Array.from({ length: 188 }, () => ({ directions: 0, frameCount: 0 })),
        state: new ArcherAnimationState(),
        fields: new ArcherAnimationFields(),
        output: new Uint8Array(4),
        row: null,
        events: []
    };
    // 각 독립 행은 동일한 원본 번호 5~70 을 사용한다.
    for (let id = 5; id <= 70; id += 1) assert.equal(scene.pool.allocate(2), id);
    scene.n = (index) => Number(scene.row[index]);
    scene.float = (index) => bitsToFloat(scene.n(index));
    scene.event = (text) => scene.events.push(text);
    // 현재 입력의 raw 상태를 작성하고 기대값은 읽지 않는다.
    scene.prepare = (row) => {
        assert.equal(row.length, 23);
        scene.row = row;
        scene.events = [];
        scene.output.fill(0);
        scene.state = new ArcherAnimationState({ now: 10 });
        const delta = new DataView(new ArrayBuffer(8));
        delta.setBigUint64(0, BigInt(row[3]), true);
        scene.state.delta = delta.getFloat64(0, true);
        scene.data[71] = { directions: scene.n(15) | 0, frameCount: scene.n(16) | 0 };
        scene.types[158].flags1 = scene.n(17) ? 0x10 : 0;
        const kind = scene.n(1);
        const fire = kind === 1 || kind >= 3;
        const bytes = scene.fields.bytes;
        // 생성자가 쓰지 않는 padding 도 같은 입력으로 남긴다.
        for (let index = 0; index < bytes.length; index += 1) bytes[index] = (index * 13 + 7) & 0xff;
        put(bytes, 0, fire ? 0x50b1c0 : 0x50b450);
        put(bytes, 4, 81);
        put(bytes, 8, 50);
        put(bytes, 12, floatToBits(60.0));
        put(bytes, 16, floatToBits(60.0));
        put(bytes, 20, floatToBits(59.5));
        put(bytes, 24, floatToBits(59.5));
        put(bytes, 32, fire ? 0x50b1bc : 0x50b448);
        bytes[40] = 0x20;
        put(bytes, 42, 70, 2);
        put(bytes, 44, scene.n(17) || fire ? 60 : 0, 2);
        const time = new DataView(new ArrayBuffer(8));
        time.setFloat64(0, scene.n(11) ? 10.0 : 10.25, true);
        put(bytes, 48, time.getUint32(0, true));
        put(bytes, 52, time.getUint32(4, true));
        put(bytes, 56, 0);
        put(bytes, 64, scene.n(4));
        put(bytes, 68, scene.n(5));
        put(bytes, 72, scene.n(6));
        put(bytes, 76, scene.n(8));
        put(bytes, 80, scene.n(7));
        // 원본의 세 슬롯은 표적 free/dead 입력도 전부 재구성한다.
        for (const [sid, type] of [[50, 71], [60, 158], [70, 34]]) {
            const raw = scene.pool.physical(sid);
            raw.fill(0);
            put(raw, 0, 0x1131a000);
            put(raw, 6, sid === 50 ? 81 : 0, 2);
            raw[10] = type;
            raw[11] = sid === 60 ? scene.n(9) : 0;
            raw[12] = 0xa5;
            raw[34] = 1;
            put(raw, 14, floatToBits(sid === 50 ? 50.5 : 60.0));
            put(raw, 18, floatToBits(sid === 50 ? 50.5 : 60.0));
            put(raw, 36, scene.n(2));
        }
        if (kind === 0 && !scene.n(17)) scene.pool.physical(50)[30] = scene.n(12);
    };
    // 실제 Missile 호출 인자를 관찰하며 그 생성 완료로 세지 않는다.
    scene.projectile = (request) => {
        scene.event('A:128');
        const args = [request.firing, request.target, floatToBits(request.speed), request.displayType, request.effectType,
            floatToBits(request.targetPosition[0]), floatToBits(request.targetPosition[1]), floatToBits(request.targetCenter[0]), floatToBits(request.targetCenter[1]),
            request.ricochet, floatToBits(request.firingPosition[0]), floatToBits(request.firingPosition[1])];
        scene.event(`J:${args.join(',')}`);
        if (scene.n(1) === 6) throw PREFIX_STOP;
    };
    // 중심/가상 공간/소리/완료/생성은 원본과 같은 명시 관찰 경계다.
    scene.hooks = () => ({
        common: {
            center: (sid) => { scene.event(`C:${sid}`); return sid === 50 ? [50, 50] : [scene.float(13), scene.float(14)]; },
            unpop: (sid, flags) => scene.event(`U:${sid}:${flags}`),
            repop: (sid, flags) => scene.event(`P:${sid}:${flags}`),
            sound: (sid, name) => scene.event(`S:${name}`),
            randomInteger: (low, high) => { scene.event(`N:${low}:${high}`); scene.event('F:3'); return 3000; },
            notify: (sid, event, info, value) => scene.event(`M:${event}:${info}:${value}`),
            kill: (form, flags) => scene.event(`K:${form}:${flags}`),
            targetPoint: (firing, target) => { scene.event(`T:${firing}:${target}`); return [52.25, 52.75]; },
            createProjectile: (request) => scene.projectile(request)
        },
        aimDirection: (sid) => scene.pool.slot(sid)[30],
        notifySurface: (sid) => scene.event(`Q:${sid}`)
    });
    // 각 독립 몸체와 동일 field/기하/경계를 실행한다.
    scene.run = () => {
        const logic = new RawArcherAnimation(scene.pool, scene.types, scene.data, scene.state, scene.hooks());
        let result = 'return';
        try {
            switch (scene.n(1)) {
            case 0: logic.rotate(scene.fields); break;
            case 1: logic.fire(scene.fields); break;
            case 2: result = String(RawArcherAnimation.nearestDirection(scene.float(4))); break;
            case 3: put(scene.output, 0, floatToBits(RawArcherAnimation.initialAngle(scene.n(2) | 0))); break;
            case 4: result = String(RawArcherAnimation.fireFrame(scene.float(4), scene.n(8) !== 0)); break;
            case 5:
                logic.initialize(scene.fields, true, 70, 60);
                scene.event('B:35:50:0:16');
                put(scene.fields.bytes, 4, 81);
                put(scene.fields.bytes, 8, 50);
                logic.initializeFire(scene.fields);
                break;
            case 6: result = logic.launch(scene.fields) ? '1' : '0'; break;
            default: throw new Error('Archer fixture body 오류');
            }
        } catch (error) {
            if (error !== PREFIX_STOP) throw error;
            result = 'prefix';
        }
        return [result, scene.events.join(';'), hex(scene.fields.bytes), hex(scene.pool.slot(50)), hex(scene.output)];
    };
    return scene;
}

test('ArcherAnimation: 원본 10.78 회전/각상태 발사/helper/생성자/launch 접두가 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    const scene = makeScene();
    let count = 0;
    // 저장된 독립 PE 관찰 이외에 기대값을 계산하지 않는다.
    for (const row of rows) {
        scene.prepare(row);
        assert.deepEqual(scene.run(), row.slice(18), `아처 행 ${count}: 몸체 ${row[1]} 프레임 ${row[2]} ${row.slice(3, 18).join(',')}`);
        count += 1;
    }
    assert.equal(count, allRows.length / 2);
});

test('ArcherAnimation: 실제 sunDisc 생성 경계 누락·판본/표 크기·초기화·프로세스 보조를 확인한다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    const scene = makeScene();
    const first = [...rows[0]];
    first[1] = '6';
    first[17] = '1';
    scene.prepare(first);
    // 실제 sunDisc 생성 경계가 없는 live HP 표적을 묵시 성공으로 바꾸지 않는다.
    const hooks = scene.hooks();
    delete hooks.common.createProjectile;
    const logic = new RawArcherAnimation(scene.pool, scene.types, scene.data, scene.state, hooks);
    assert.ok(throws(() => logic.launch(scene.fields)));
    assert.deepEqual(scene.events, []);
    // 판본/표 크기/현재 TYPE 오류.
    assert.ok(throws(() => new RawArcherAnimation({ edition: 'cd1072' }, scene.types, scene.data, scene.state, {})));
    assert.ok(throws(() => new RawArcherAnimation(scene.pool, scene.types, scene.data.slice(0, 10), scene.state, {})));
    assert.ok(throws(() => new RawArcherAnimation(scene.pool, scene.types, scene.data, new ArcherAnimationState({ type: 188 }), {})));
    assert.equal(logic.pool, scene.pool);
    assert.equal(logic.type, 71);
    scene.state.currentType = { value: 80 };
    assert.equal(logic.type, 80);
    scene.state.currentType = null;
    // 초기화: 가상 표·dirty·건/표적 WORD·현재 시각, 발사면 +38 을 0.
    const fields = new ArcherAnimationFields();
    fields.bytes.fill(0xcd);
    logic.initialize(fields, true, 70, 60);
    assert.equal(get(fields.bytes, 0), 0x50b1c0);
    assert.equal(get(fields.bytes, 32), 0x50b1bc);
    assert.equal(fields.bytes[40], 0xff);
    assert.equal(fields.gunForm, 70);
    assert.equal(fields.target, 60);
    assert.equal(fields.time, scene.state.now);
    assert.equal(get(fields.bytes, 56), 0);
    const rotate = new ArcherAnimationFields();
    rotate.bytes.fill(0xcd);
    logic.initialize(rotate, false, 70, 0);
    assert.equal(get(rotate.bytes, 0), 0x50b450);
    assert.equal(get(rotate.bytes, 32), 0x50b448);
    assert.equal(get(rotate.bytes, 56), 0xcdcdcdcd);
    assert.ok(throws(() => logic.initialize(new ArcherAnimationFields(), true, 0, 60)));
    // helper 범위 오류.
    assert.ok(throws(() => RawArcherAnimation.nearestDirection(Number.NaN)));
    assert.ok(throws(() => RawArcherAnimation.fireFrame(Number.POSITIVE_INFINITY, false)));
    assert.ok(throws(() => RawArcherAnimation.initialAngle(100)));
    assert.equal(RawArcherAnimation.degrees([1, 1], [1, 1]), 0);
    assert.equal(RawArcherAnimation.directionAngle([1, 1], [1, 1]), 0);
    // 프로세스 보조: 부착 전 Form/Parent 0 → 프레임은 부모 범위 오류로 거부된다.
    const process = new ArcherAnimationProcess(logic, new ArcherAnimationFields(), false);
    process.onAttached();
    assert.equal(process.fields.form, 0);
    assert.ok(throws(() => process.runFrame()));
    assert.ok(throws(() => new ArcherAnimationProcess(logic, new ArcherAnimationFields(), true).runFrame()));
});
