// 썬 캐논 애니메이션 raw-gun-animation.js(RawGunAnimation)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: gunanimation-x86.tsv 의 `originals` 행 — 종류(0 회전/1 발사/2 원위치/3 launch)·부모 프레임·표적 점·원위치 방향·표적 상태/HP·시각 도달·dirty 입력으로
//    반환/Missile ctor 접두 종류, 외부 효과 순서(C 중심/T 표적점/U·P 공간/N 난수/F·S 소리/M 통지/K 삭제/A·J Missile 인자 12개), 0x40바이트 필드 이미지, 부모 프레임 결과가 같다. `originalCD` 행은 10.72 라 사용하지 않는다.
//  - 계약: 실제 Missile 생성 경계 누락을 성공으로 우회하지 않음, free/dead 표적은 launch 하지 않음, 수신 생성자·직렬화·가상 writer 선택·프로세스 보조.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E113: 썬 캐논 애니메이션 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import {
    GunAnimationFields, GunAnimationKind, GunAnimationProcess, GunAnimationReceiveKind, RawGunAnimation,
    gunAnimationUsesFirePacket, initializeReceivedGunAnimation, readGunAnimationProcessData, writeGunAnimationProcessData
} from '../www/core/raw-gun-animation.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E113)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'gunanimation-x86.tsv'));
const rows = allRows ? allRows.filter((row) => row[0] === 'originals') : [];
const hex = (bytes) => Buffer.from(bytes).toString('hex');
const bits = (value) => {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
};

// 예외가 나는지 본다. (2026-11-08 WB04-E113)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 실제 Missile ctor 진입 정지를 구별하는 표식이다. (2026-11-08 WB04-E113)
const MISSILE_PREFIX = Symbol('missile-prefix');

// 원본 입력의 현재 풀/타입/시각과 외부 효과 관찰을 보관하는 장면이다. (2026-11-08 WB04-E113)
function makeScene() {
    const scene = {
        pool: new SidPool(24000, { server: false }),
        types: Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, group: 0 })),
        state: { now: 10, sunCannonType: 128, displayType: 80, effectType: 11 },
        fields: new GunAnimationFields(),
        events: [],
        point: [0, 0]
    };
    // 실제 입력 SID 를 미리 확보한다.
    for (let id = 5; id <= 100; id += 1) assert.equal(scene.pool.allocate(2), id);
    // 원본과 같은 월드 경계의 인자/순서를 기록한다.
    scene.hooks = {
        center: (sid) => { scene.events.push(`C:${sid}`); return sid === 50 ? [50, 50] : scene.point; },
        targetPoint: (firing, target) => { scene.events.push(`T:${firing}:${target}`); return [52.25, 52.75]; },
        unpop: (sid, flags) => scene.events.push(`U:${sid}:${flags}`),
        repop: (sid, flags) => scene.events.push(`P:${sid}:${flags}`),
        randomInteger: (low, high) => { scene.events.push(`N:${low}:${high}`); return 3000; },
        sound: (sid, name) => { scene.events.push('F:3'); scene.events.push(`S:${name}`); },
        notify: (sid, event, info, value) => scene.events.push(`M:${event}:${info}:${value}`),
        kill: (sid, flags) => scene.events.push(`K:${sid}:${flags}`),
        createProjectile: (request) => {
            scene.events.push('A:128');
            const args = [request.firing, request.target, bits(request.speed), request.displayType, request.effectType,
                bits(request.targetPosition[0]), bits(request.targetPosition[1]), bits(request.targetCenter[0]), bits(request.targetCenter[1]),
                request.ricochet, bits(request.firingPosition[0]), bits(request.firingPosition[1])];
            scene.events.push(`J:${args.join(',')}`);
            throw MISSILE_PREFIX;
        }
    };
    // 행의 숫자 입력으로 실제 필드를 준비한다(미초기화 padding 은 별도 패턴으로 보존).
    scene.prepare = (row) => {
        assert.equal(row.length, 13);
        scene.events = [];
        scene.state.now = 10;
        const kind = Number(row[1]);
        const index = kind === 3 ? 1 : kind;
        const bytes = scene.fields.bytes;
        for (let at = 0; at < bytes.length; at += 1) bytes[at] = (at * 13 + 7) & 0xff;
        const outer = [0x50b260, 0x50b318, 0x50b2c8];
        const inner = [0x50b258, 0x50b310, 0x50b2c4];
        put(bytes, 0, outer[index]);
        put(bytes, 4, 81);
        put(bytes, 8, 50);
        put(bytes, 12, 0x42700000);
        put(bytes, 16, 0x42700000);
        put(bytes, 20, 0x426e0000);
        put(bytes, 24, 0x426e0000);
        put(bytes, 32, inner[index]);
        bytes[40] = Number(row[8]);
        put(bytes, 42, 70, 2);
        put(bytes, 44, 60, 2);
        const time = new DataView(new ArrayBuffer(8));
        time.setFloat64(0, Number(row[7]) ? 10.0 : 10.25, true);
        put(bytes, 48, time.getUint32(0, true));
        put(bytes, 52, time.getUint32(4, true));
        put(bytes, 56, Number(row[4]));
        const parent = scene.pool.physical(50);
        parent.fill(0);
        put(parent, 0, 0x1131a000);
        parent[10] = 128;
        put(parent, 14, 0x424a0000);
        put(parent, 18, 0x424a0000);
        parent[34] = 1;
        put(parent, 36, Number(row[2]));
        const target = scene.pool.physical(60);
        target.fill(0);
        target[10] = 71;
        target[11] = Number(row[5]);
        put(target, 14, 0x42700000);
        put(target, 18, 0x42700000);
        scene.types[71].flags1 = Number(row[6]) ? 0x10 : 0;
        scene.point = [[50, 40], [60, 50], [50, 60], [40, 50]][Number(row[3]) % 4];
    };
    // 같은 입력의 프레임 또는 launch 접두만 실행한다.
    scene.run = (row) => {
        const logic = new RawGunAnimation(scene.pool, scene.types, scene.state, scene.hooks);
        let prefix = false;
        try {
            if (Number(row[1]) === 3) logic.launch(scene.fields);
            else logic.frame(scene.fields, Number(row[1]));
        } catch (error) {
            if (error !== MISSILE_PREFIX) throw error;
            prefix = true;
        }
        return [prefix ? 'prefix' : 'return', scene.events.join(';'), hex(scene.fields.bytes), String(get(scene.pool.slot(50), 36, 4))];
    };
    return scene;
}

test('GunAnimation: 원본 10.78 입력의 프레임/완료/launch 접두 효과 순서·필드 이미지·부모 프레임이 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    const scene = makeScene();
    let count = 0;
    // 저장된 실제 원본 관찰을 직접 소비한다.
    for (const row of rows) {
        scene.prepare(row);
        const actual = scene.run(row);
        assert.deepEqual(actual, row.slice(9), `행 ${count}: 종류 ${row[1]} 프레임 ${row[2]} ${row.slice(3, 9).join(',')}`);
        count += 1;
    }
    assert.equal(count, allRows.length / 2);
});

test('GunAnimation: Missile 생성 경계 누락·free 표적·범위 오류·초기화 필드를 확인한다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    const scene = makeScene();
    scene.prepare(rows[0]);
    scene.types[71].flags1 = 0x10;
    const logic = new RawGunAnimation(scene.pool, scene.types, scene.state, {});
    // 실제 투사체 효과가 없으면 valid launch 를 성공이나 완료로 우회하지 않는다.
    assert.ok(throws(() => logic.launch(scene.fields)));
    scene.pool.physical(60)[11] = 2;
    assert.equal(logic.launch(scene.fields), false);
    // 판본/표 크기/범위 오류.
    assert.ok(throws(() => new RawGunAnimation({ edition: 'cd1072' }, scene.types, scene.state, {})));
    assert.ok(throws(() => new RawGunAnimation(scene.pool, scene.types.slice(0, 10), scene.state, {})));
    assert.equal(logic.pool, scene.pool);
    assert.equal(logic.sunCannonType(), 128);
    scene.state.currentSunCannonType = { value: 77 };
    assert.equal(logic.sunCannonType(), 77);
    delete scene.state.currentSunCannonType;
    assert.ok(throws(() => logic.initialize(new GunAnimationFields(), GunAnimationKind.SunRotate, 0, 60)));
    assert.ok(throws(() => logic.initialize(new GunAnimationFields(), GunAnimationKind.SunRotate, 24000, 60)));
    // 초기화는 가상 표·dirty·건/표적 WORD·현재 시각을 쓰고 원위치는 표적을 0 으로 둔다.
    const rotate = new GunAnimationFields();
    logic.initialize(rotate, GunAnimationKind.SunRotate, 70, 60);
    assert.equal(get(rotate.bytes, 0), 0x50b260);
    assert.equal(get(rotate.bytes, 32), 0x50b258);
    assert.equal(rotate.bytes[40], 0xff);
    assert.equal(rotate.gunForm, 70);
    assert.equal(rotate.target, 60);
    assert.equal(rotate.time, 10);
    const rest = new GunAnimationFields();
    logic.initialize(rest, GunAnimationKind.SunRest, 70, 60);
    assert.equal(get(rest.bytes, 0), 0x50b2c8);
    assert.equal(rest.target, 0);
    assert.equal(rest.restDirection, 0);
    const fire = new GunAnimationFields();
    logic.initialize(fire, GunAnimationKind.SunFire, 70, 60);
    assert.equal(get(fire.bytes, 0), 0x50b318);
    assert.equal(fire.target, 60);
});

test('GunAnimation: 수신 생성자·선택 필드 복원·가상 writer 선택·공통 직렬화·프로세스 보조를 확인한다', () => {
    // 수신 전용 생성자 네 종류.
    const fire = new Uint8Array(88).fill(0xaa);
    initializeReceivedGunAnimation(fire, GunAnimationReceiveKind.SunFire);
    assert.equal(get(fire, 0), 0x50b318);
    assert.equal(get(fire, 32), 0x50b310);
    assert.equal(fire[40], 0xff);
    assert.equal(get(fire, 12), 0xaaaaaaaa);
    const rotate = new Uint8Array(88).fill(0xaa);
    initializeReceivedGunAnimation(rotate, GunAnimationReceiveKind.SunRotate);
    assert.equal(get(rotate, 0), 0x50b260);
    assert.equal(get(rotate, 32), 0x50b258);
    const archerFire = new Uint8Array(88).fill(0xaa);
    initializeReceivedGunAnimation(archerFire, GunAnimationReceiveKind.ArcherFire);
    assert.equal(get(archerFire, 0), 0x50b1c0);
    assert.equal(get(archerFire, 32), 0x50b1bc);
    assert.equal(get(archerFire, 68), 0x43340000);
    assert.equal(get(archerFire, 80), 0);
    const archerRotate = new Uint8Array(88).fill(0xaa);
    initializeReceivedGunAnimation(archerRotate, GunAnimationReceiveKind.ArcherRotate);
    // 아처 회전은 마지막에 0x50b450/0x50b448 로 덮어쓴다.
    assert.equal(get(archerRotate, 0), 0x50b450);
    assert.equal(get(archerRotate, 32), 0x50b448);
    assert.ok(throws(() => initializeReceivedGunAnimation(new Uint8Array(55), GunAnimationReceiveKind.SunFire)));
    assert.ok(throws(() => initializeReceivedGunAnimation(new Uint8Array(87), GunAnimationReceiveKind.ArcherFire)));
    // 가상 writer 선택(발사 true / 회전·원위치 false / 미연결 거부).
    for (const [table, expected] of [[0x50b0d0, true], [0x50b318, true], [0x50b3f0, true], [0x50b210, false], [0x50b2c8, false], [0x50b450, false]]) {
        const outer = new Uint8Array(64);
        put(outer, 0, table);
        assert.equal(gunAnimationUsesFirePacket(outer), expected);
    }
    const unknown = new Uint8Array(64);
    put(unknown, 0, 0x505480);
    assert.ok(throws(() => gunAnimationUsesFirePacket(unknown)));
    assert.ok(throws(() => gunAnimationUsesFirePacket(new Uint8Array(3))));
    // 직렬화: 부모 포함 + dirty 2/4/8 선택 필드, 쓰고 나면 dirty 가 0.
    const outer = new Uint8Array(64);
    put(outer, 4, 81);
    put(outer, 8, 50);
    put(outer, 40, 0x0e);
    put(outer, 42, 70, 2);
    put(outer, 44, 60, 2);
    put(outer, 48, 0x11223344);
    put(outer, 52, 0x55667788);
    const packet = new Uint8Array(32).fill(0xcc);
    const length = writeGunAnimationProcessData(outer, packet, true, false, 0x3b);
    assert.equal(length, 9 + 4 + 2 + 2 + 8);
    assert.equal(packet[0], 1);
    assert.equal(packet[1], 0x3b);
    assert.equal(get(packet, 2, 2), length);
    assert.equal(get(packet, 4), 81);
    assert.equal(packet[8], 0x0f);
    assert.equal(get(packet, 9), 50);
    assert.equal(get(packet, 13, 2), 70);
    assert.equal(get(packet, 15, 2), 60);
    assert.equal(get(packet, 17), 0x11223344);
    assert.equal(outer[40], 0);
    // 부모 없이, 강제 직렬화는 모든 선택 필드를 쓴다.
    const forced = writeGunAnimationProcessData(outer, new Uint8Array(32), false, true, 0x3c, 2);
    assert.equal(forced, 9 + 4 + 2 + 2 + 8);
    assert.ok(throws(() => writeGunAnimationProcessData(outer, new Uint8Array(8), true, true, 0x3c)));
    assert.ok(throws(() => writeGunAnimationProcessData(new Uint8Array(55), packet, true, true, 0x3c)));
    // 복원: 부모는 반환하고 선택 필드만 복사한다.
    const received = new Uint8Array(64).fill(0x5a);
    const parent = readGunAnimationProcessData(received, packet.subarray(0, 25));
    assert.equal(parent, 50);
    assert.equal(get(received, 42, 2), 70);
    assert.equal(get(received, 44, 2), 60);
    assert.equal(get(received, 48), 0x11223344);
    assert.equal(received[40], 0x5a);
    assert.ok(throws(() => readGunAnimationProcessData(received, packet.subarray(0, 8))));
    assert.ok(throws(() => readGunAnimationProcessData(received, packet.subarray(0, 20))));
    // 프로세스 보조: 부착 전 Form/Parent 는 0 이고 packet 은 같은 공통 직렬화이다.
    const logic = new RawGunAnimation(new SidPool(24000, { server: false }), Array.from({ length: 188 }, () => ({ flags1: 0 })), { now: 3 }, {});
    const fields = new GunAnimationFields();
    logic.initialize(fields, GunAnimationKind.SunFire, 70, 60);
    // 초기화 전에 Pool 슬롯이 확보되지 않았으므로 gunForm 범위만 확인했다.
    const process = new GunAnimationProcess(logic, fields, GunAnimationKind.SunFire);
    assert.equal(process.fields, fields);
    process.onAttached();
    assert.equal(fields.form, 0);
    const out = new Uint8Array(32);
    assert.equal(process.writePacket(out, false, false), 9 + 2 + 2 + 8);
    assert.equal(out[1], 0x3b);
    assert.ok(throws(() => process.runFrame()));
});
