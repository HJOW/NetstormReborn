// 아이스 캐논 80바이트 파생 발사 raw-ice-cannon-fire.js(RawIceCannonFire)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: ice-cannon-fire-x86.tsv 3585행 — 몸체(0 Frame/1 Initialize/2 Step/3 Launch)·부모 프레임·시각 도달·표적 상태/HP·문자 배치·Step 프레임·active·전환 시각·mayTarget 결과·재진입 입력으로
//    반환, 외부 효과 순서(E 냉각/X 문맥/A 표적 판정/C 중심/U·P 공간/T 표적점/A·J Missile/M 통지/K 삭제/B 부착), 80바이트 필드 이미지(공통 64 + 전용 16), 네 슬롯 200바이트가 같다.
//  - 계약: 냉각/문맥/표적 판정 경계 누락을 성공으로 우회하지 않음, 프로세스 보조.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E114: 아이스 캐논 발사 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { CannonFireState } from '../www/core/raw-cannon-fire.js';
import { IceCannonFireFields, IceCannonFireProcess, RawIceCannonFire } from '../www/core/raw-ice-cannon-fire.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E114)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'ice-cannon-fire-x86.tsv')) ?? [];
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 예외가 나는지 본다. (2026-11-08 WB04-E114)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 문자 배치 입력으로 32칸 프레임 코드 표를 만든다. (2026-11-08 WB04-E114)
function makeFrames(layout) {
    const codes = [];
    // 연속·단일·교차·불균등 문자 입력을 원문과 똑같이 설치한다.
    for (let index = 0; index < 32; index += 1) {
        const letter = layout === 0 ? 65 + Math.floor(index / 8) : layout === 1 ? 65 : layout === 2 ? 65 + (index % 2) : index === 0 ? 65 : index < 6 ? 66 : index < 19 ? 67 : 68;
        codes.push({ side: letter, variant: 0, flags: 0 });
    }
    return new FrameCodeTable(codes);
}

// double 을 필드의 두 DWORD 에 쓴다. (2026-11-08 WB04-E114)
function putDouble(raw, offset, value) {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    put(raw, offset, view.getUint32(0, true));
    put(raw, offset + 4, view.getUint32(4, true));
}

// 같은 실제 풀/타입/필드와 선언한 외부 효과의 독립 장면이다. (2026-11-08 WB04-E114)
function makeScene() {
    const scene = {
        pool: new SidPool(24000, { server: false }),
        types: Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, group: 0 })),
        frames: Array.from({ length: 188 }, () => ({ frames: null })),
        fields: new IceCannonFireFields(),
        events: [],
        row: null
    };
    // 실제 원문 번호까지 할당한다.
    for (let id = 5; id <= 70; id += 1) assert.equal(scene.pool.allocate(2), id);
    scene.n = (index) => Number(scene.row[index]);
    scene.input = (sid) => scene.pool.physical(sid);
    scene.event = (text) => scene.events.push(text);
    // 출력 자료를 읽지 않고 같은 raw 입력을 구성한다.
    scene.prepare = (row) => {
        scene.row = row;
        assert.equal(row.length, 15);
        scene.events = [];
        scene.state = new CannonFireState({ type: 129, displayType: 130, delay: 0.08, now: 10 });
        scene.types[158].flags1 = scene.n(4) ? 0x10 : 0;
        scene.frames[129].frames = makeFrames(scene.n(5));
        const common = scene.fields.common.bytes;
        // 생성자가 건드리지 않는 접두 padding 도 보존한다.
        for (let index = 0; index < 64; index += 1) common[index] = (index * 13 + 7) & 0xff;
        // 전용 +44 padding 까지 원문과 같은 패턴이다.
        for (let index = 0; index < 16; index += 1) scene.fields.extra[index] = ((index + 64) * 13 + 7) & 0xff;
        put(common, 0, 0x50b3f0);
        put(common, 4, 81);
        put(common, 8, 50);
        put(common, 12, floatToBits(60.0));
        put(common, 16, floatToBits(60.0));
        put(common, 20, floatToBits(59.5));
        put(common, 24, floatToBits(59.5));
        put(common, 32, 0x50b3ec);
        common[40] = 0x20;
        put(common, 42, 70, 2);
        put(common, 44, 60, 2);
        putDouble(common, 48, scene.n(2) ? 10.0 : 10.25);
        put(common, 56, 0);
        put(scene.fields.extra, 0, scene.n(7));
        putDouble(scene.fields.extra, 8, scene.n(8) === 0 ? 9.0 : scene.n(8) === 1 ? 10.0 : 10.25);
        // 두 부모와 두 표적을 매 행 독립 설치한다.
        for (const [sid, type] of [[50, 129], [51, 129], [60, 158], [61, 158], [70, 34]]) {
            const raw = scene.input(sid);
            raw.fill(0);
            put(raw, 0, 0x1131a000);
            raw[10] = type;
            raw[11] = sid === 60 ? scene.n(3) : 0;
            raw[12] = 0xa5;
            raw[34] = 1;
            put(raw, 14, floatToBits(sid < 60 ? 50.5 : 60.0));
            put(raw, 18, floatToBits(sid < 60 ? 50.5 : 60.0));
            put(raw, 36, scene.n(1));
        }
    };
    // 원문 관찰기와 같은 외부 재진입 입력만 공급한다.
    scene.mutate = (where) => {
        const mut = scene.n(10);
        const common = scene.fields.common.bytes;
        if (where === 'Center' && mut === 1) put(scene.input(50), 36, 2);
        if (where === 'Center' && mut === 2) put(common, 44, 61, 2);
        if (where === 'Unpop' && mut === 3) {
            scene.state.now = 100;
            putDouble(common, 48, 777.0);
        }
        if (where === 'Repop' && mut === 4) scene.input(60)[11] = 2;
        if (where === 'Point' && mut === 5) {
            put(common, 8, 51);
            put(common, 44, 61, 2);
            put(common, 12, floatToBits(77.25));
            put(common, 16, floatToBits(78.5));
            scene.state.speed = 37;
            scene.state.displayType = 133;
        }
        if (where === 'Missile' && mut === 6) put(common, 42, 71, 2);
    };
    // 실제 12개 인수를 원문 순서로 관찰한다.
    scene.projectile = (request) => {
        scene.event('A:128');
        const args = [request.firing, request.target, floatToBits(request.speed), request.displayType, request.effectType,
            floatToBits(request.targetPosition[0]), floatToBits(request.targetPosition[1]), floatToBits(request.targetCenter[0]), floatToBits(request.targetCenter[1]),
            request.ricochet, floatToBits(request.firingPosition[0]), floatToBits(request.firingPosition[1])];
        scene.event(`J:${args.join(',')}`);
        scene.mutate('Missile');
    };
    // 공간/중심/투사체/완료만 선언된 외부 효과다.
    scene.common = () => ({
        center: (sid) => { scene.event(`C:${sid}`); scene.mutate('Center'); return [60, 50]; },
        unpop: (sid, flags) => { scene.event(`U:${sid}:${flags}`); scene.mutate('Unpop'); },
        repop: (sid, flags) => { scene.event(`P:${sid}:${flags}`); scene.mutate('Repop'); },
        targetPoint: (sid, target) => { scene.event(`T:${sid}:${target}`); scene.mutate('Point'); return [52.25, 52.75]; },
        createProjectile: (request) => scene.projectile(request),
        notify: (sid, event, info, value) => scene.event(`M:${event}:${info}:${value}`),
        kill: (form, flags) => scene.event(`K:${form}:${flags}`)
    });
    // 선택된 냉각/문맥/표적 결과를 명시 입력으로만 제공한다.
    scene.ice = () => ({
        cooling: (sid) => {
            scene.event(`E:${sid}`);
            if (scene.n(10) === 7) { scene.state.now = 100; put(scene.fields.common.bytes, 8, 51); }
        },
        context: (sid) => {
            scene.event(`X:${sid}`);
            if (scene.n(10) === 8) { put(scene.fields.common.bytes, 8, 51); put(scene.fields.common.bytes, 44, 61, 2); }
            return [201, 202, 203, 204, 205, 206, 207, 208];
        },
        // 현재 target 재조회와 결과 뒤의 active 재조회를 관찰한다.
        mayTarget: (context, target) => {
            scene.event(`A:${[...context, target].join(',')}`);
            if (scene.n(10) === 9) put(scene.fields.extra, 0, 0);
            return scene.n(9) !== 0;
        }
    });
    // 네 원문 진입과 80바이트/슬롯/효과를 비교할 관찰을 반환한다.
    scene.run = () => {
        const logic = new RawIceCannonFire(scene.pool, scene.types, scene.frames, scene.state, scene.common(), scene.ice());
        let result = 'return';
        switch (scene.n(0)) {
        case 0: logic.frame(scene.fields); break;
        case 1:
            logic.initialize(scene.fields, 70, 60);
            scene.event('B:35:50:0:16');
            put(scene.fields.common.bytes, 4, 81);
            put(scene.fields.common.bytes, 8, 50);
            break;
        case 2: result = String(logic.step(scene.fields, Number(scene.row[6]))); break;
        case 3: result = logic.launch(scene.fields) ? '1' : '0'; break;
        default: throw new Error('아이스 자료의 몸체 입력 오류');
        }
        let slots = '';
        // 네 슬롯 200바이트 전체다.
        for (const sid of [50, 51, 60, 61]) slots += hex(scene.pool.slot(sid));
        return [result, scene.events.join(';'), hex(scene.fields.common.bytes) + hex(scene.fields.extra), slots];
    };
    return scene;
}

test('IceCannonFire: 원본 10.78 80바이트 파생의 공통 프레임·생성자 padding·Step 현재 필드·launch getter 가 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    const scene = makeScene();
    const counts = [0, 0, 0, 0];
    // 저장된 원문 결과와만 대조한다.
    for (let index = 0; index < rows.length; index += 1) {
        const row = rows[index];
        scene.prepare(row);
        assert.deepEqual(scene.run(), row.slice(11), `아이스 행 ${index}: 몸체 ${row[0]} ${row.slice(1, 11).join(',')}`);
        counts[Number(row[0])] += 1;
    }
    assert.ok(counts.every((count) => count > 0));
    assert.equal(counts.reduce((sum, count) => sum + count, 0), 3585);
});

test('IceCannonFire: 냉각·문맥·표적 판정 경계 누락을 우회하지 않고 프로세스 보조를 확인한다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    const scene = makeScene();
    // 냉각 경계 누락: 전환 시각이 지난 active 입력(Step)에서 거부한다.
    const cooling = rows.find((row) => Number(row[0]) === 2 && Number(row[7]) !== 0 && Number(row[8]) === 0);
    assert.ok(cooling);
    scene.prepare(cooling);
    const logic = new RawIceCannonFire(scene.pool, scene.types, scene.frames, scene.state, scene.common(), {});
    assert.ok(throws(() => logic.step(scene.fields, Number(cooling[6]))));
    // 문맥/표적 판정 경계 누락: 전환 시각이 오지 않은 입력(Step)에서 구간 안이면 거부한다.
    const inner = rows.find((row) => Number(row[0]) === 2 && Number(row[8]) === 2 && Number(row[6]) === 0 && Number(row[1]) === 0 && Number(row[5]) === 0);
    assert.ok(inner);
    scene.prepare(inner);
    assert.ok(throws(() => new RawIceCannonFire(scene.pool, scene.types, scene.frames, scene.state, scene.common(), {}).step(scene.fields, 0)));
    assert.ok(throws(() => new RawIceCannonFire(scene.pool, scene.types, scene.frames, scene.state, scene.common(), { context: () => [] }).step(scene.fields, 0)));
    // 판본/표 크기 오류는 공통 생성자가 거부한다.
    assert.ok(throws(() => new RawIceCannonFire({ edition: 'cd1072' }, scene.types, scene.frames, scene.state, {}, {})));
    // 초기화는 두 가상 표·active 1·전환 시각을 쓰고 padding 은 보존한다.
    const fields = new IceCannonFireFields();
    fields.extra.fill(0xcd);
    new RawIceCannonFire(scene.pool, scene.types, scene.frames, new CannonFireState({ type: 129, now: 3 }), {}, {}).initialize(fields, 70, 60);
    assert.equal(Buffer.from(fields.common.bytes.subarray(0, 4)).readUInt32LE(0), 0x50b3f0);
    assert.equal(Buffer.from(fields.common.bytes.subarray(32, 36)).readUInt32LE(0), 0x50b3ec);
    assert.equal(Buffer.from(fields.extra.subarray(0, 4)).readUInt32LE(0), 1);
    assert.equal(fields.extra[4], 0xcd);
    assert.equal(Buffer.from(fields.extra.buffer, 8, 8).readDoubleLE(0), 7);
    // 프로세스 보조: 부착 전 Form/Parent 0 → 프레임은 표적 0 assert 로 거부된다.
    const process = new IceCannonFireProcess(new RawIceCannonFire(scene.pool, scene.types, scene.frames, scene.state, scene.common(), scene.ice()), new IceCannonFireFields());
    process.onAttached();
    assert.equal(process.fields.common.form, 0);
    assert.ok(throws(() => process.runFrame()));
});
