// 썬/썬더 캐논·반더 타워 공통 발사 raw-cannon-fire.js(RawCannonFire)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측 1: cannon-fire-x86.tsv 5534행 — 프로파일(0 썬/1 썬더)·몸체(0 Frame/1 Initialize/2 Step/3 Launch)·부모 프레임·시각 도달·표적 상태/HP·문자 배치·소리 차이·재진입 입력으로
//    반환, 외부 효과 순서(C 중심/U·P 공간/T 표적점/A·J Missile/S 소리/M 통지/K 삭제/B 부착), 0x40바이트 필드 이미지, 네 슬롯 200바이트가 같다.
//  - 원본 관측 2: vander-fire-x86.tsv 의 반더 발사 393행(종류 0~3: Initialize/Step/Launch/Frame) — 즉시 빔(생성·회피 난수·refresh/hit)과 난수 상태·현재 시각까지 같다. 이름 wrapper 24행(종류 4 이상)은 RawLightningLifetime 이식 때 쓴다.
//  - 계약: 실제 Missile 경계 누락을 성공으로 우회하지 않음, 판본/표 크기, 프로세스 보조.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E114: 공통 캐논 발사 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { GunAnimationFields } from '../www/core/raw-gun-animation.js';
import { CannonFireKind, CannonFireProcess, CannonFireState, RawCannonFire } from '../www/core/raw-cannon-fire.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E114)
const cannonRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'cannon-fire-x86.tsv')) ?? [];
const vanderAll = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'vander-fire-x86.tsv')) ?? [];
const vanderRows = vanderAll.filter((row) => Number(row[0]) < 4);
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 예외가 나는지 본다. (2026-11-08 WB04-E114)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 문자 배치 입력(0 연속/1 단일/2 교차/그 밖 불균등)으로 32칸 프레임 코드 표를 만든다. (2026-11-08 WB04-E114)
function makeFrames(layout) {
    const codes = [];
    // 연속·단일·교차·불균등 문자 입력을 원문과 똑같이 설치한다.
    for (let index = 0; index < 32; index += 1) {
        const letter = layout === 0 ? 65 + Math.floor(index / 8) : layout === 1 ? 65 : layout === 2 ? 65 + (index % 2) : index === 0 ? 65 : index < 6 ? 66 : index < 19 ? 67 : 68;
        codes.push({ side: letter, variant: 0, flags: 0 });
    }
    return new FrameCodeTable(codes);
}

// 실제 슬롯 풀과 원본 입력을 공유하는 장면의 공통 바탕이다. (2026-11-08 WB04-E114)
function basis() {
    const scene = {
        pool: new SidPool(24000, { server: false }),
        types: Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, group: 0 })),
        frames: Array.from({ length: 188 }, () => ({ frames: null })),
        fields: new GunAnimationFields(),
        events: [],
        row: null
    };
    // 원본 부모 50/51·표적 60/61·GunForm 70 까지 실제 번호를 확보한다.
    for (let id = 5; id <= 70; id += 1) assert.equal(scene.pool.allocate(2), id);
    scene.n = (index) => Number(scene.row[index]);
    // free/dead 입력도 다음 독립 행에서 다시 설치하는 검사 전용 접근이다.
    scene.input = (sid) => scene.pool.physical(sid);
    scene.event = (text) => scene.events.push(text);
    return scene;
}

// 시각 double 을 필드의 +48/+52 에 쓴다. (2026-11-08 WB04-E114)
function putDouble(raw, offset, value) {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    put(raw, offset, view.getUint32(0, true));
    put(raw, offset + 4, view.getUint32(4, true));
}

// 썬/썬더 장면이다. (2026-11-08 WB04-E114)
function makeScene() {
    const scene = basis();
    // 원문과 같은 패턴·슬롯·문자 코드 입력을 만들며 기대 출력은 읽지 않는다.
    scene.prepare = (row) => {
        scene.row = row;
        assert.equal(row.length, 15);
        scene.events = [];
        const sun = scene.n(0) === 0;
        scene.state = new CannonFireState({ now: 10, growlOffset: Number(row[8]), roarOffset: Number(row[9]) });
        if (sun) Object.assign(scene.state, { kind: CannonFireKind.Sun, type: 128, displayType: 80, effectType: 11, delay: 0.07 });
        scene.types[158].flags1 = scene.n(5) ? 0x10 : 0;
        scene.frames[scene.state.type].frames = makeFrames(scene.n(6));
        const bytes = scene.fields.bytes;
        for (let index = 0; index < bytes.length; index += 1) bytes[index] = (index * 13 + 7) & 0xff;
        put(bytes, 0, sun ? 0x50b318 : 0x50b370);
        put(bytes, 4, 81);
        put(bytes, 8, 50);
        put(bytes, 12, floatToBits(60.0));
        put(bytes, 16, floatToBits(60.0));
        put(bytes, 20, floatToBits(59.5));
        put(bytes, 24, floatToBits(59.5));
        put(bytes, 32, sun ? 0x50b310 : 0x50b368);
        bytes[40] = 0x20;
        put(bytes, 42, 70, 2);
        put(bytes, 44, 60, 2);
        putDouble(bytes, 48, scene.n(3) ? 10.0 : 10.25);
        put(bytes, 56, 0);
        // 네 원문 관찰 슬롯과 GunForm 을 준비한다.
        for (const [sid, type] of [[50, scene.state.type], [51, scene.state.type], [60, 158], [61, 158], [70, 34]]) {
            const raw = scene.input(sid);
            raw.fill(0);
            put(raw, 0, 0x1131a000);
            raw[10] = type;
            raw[11] = sid === 60 ? scene.n(4) : 0;
            raw[12] = 0xa5;
            raw[34] = 1;
            put(raw, 14, floatToBits(sid < 60 ? 50.5 : 60.0));
            put(raw, 18, floatToBits(sid < 60 ? 50.5 : 60.0));
            put(raw, 36, scene.n(2));
        }
    };
    // 원문과 같은 선언된 외부 효과 뒤 현재 필드/전역 변경만 공급한다.
    scene.mutate = (where) => {
        const mut = scene.n(10);
        const bytes = scene.fields.bytes;
        if (where === 'Center' && mut === 1) put(scene.input(50), 36, 2);
        if (where === 'Center' && mut === 2) put(bytes, 44, 61, 2);
        if (where === 'Unpop' && mut === 3) {
            scene.state.now = 100;
            putDouble(bytes, 48, 777.0);
        }
        if (where === 'Repop' && mut === 4) scene.input(60)[11] = 2;
        if (where === 'Point' && mut === 5) {
            put(bytes, 8, 51);
            put(bytes, 44, 61, 2);
            put(bytes, 12, floatToBits(77.25));
            put(bytes, 16, floatToBits(78.5));
            scene.state.speed = 37;
            scene.state.displayType = scene.state.kind === CannonFireKind.Sun ? 82 : 133;
        }
        if (where === 'Missile' && mut === 6) put(bytes, 42, 71, 2);
        if (where === 'Sound' && mut === 7) {
            scene.state.roarOffset = scene.state.growlOffset;
            put(bytes, 8, 51);
            scene.state.now = 100;
        }
    };
    // 실제 Missile 경계 인수만 직렬화하며 생성/비행 완료로 간주하지 않는다.
    scene.projectile = (request) => {
        scene.event('A:128');
        const args = [request.firing, request.target, floatToBits(request.speed), request.displayType, request.effectType,
            floatToBits(request.targetPosition[0]), floatToBits(request.targetPosition[1]), floatToBits(request.targetCenter[0]), floatToBits(request.targetCenter[1]),
            request.ricochet, floatToBits(request.firingPosition[0]), floatToBits(request.firingPosition[1])];
        scene.event(`J:${args.join(',')}`);
        scene.mutate('Missile');
    };
    // 실제 공통 공간/중심/소리/완료 효과를 같은 원문 형식으로 관찰한다.
    scene.hooks = () => ({
        center: (sid) => { scene.event(`C:${sid}`); scene.mutate('Center'); return [60, 50]; },
        unpop: (sid, flags) => { scene.event(`U:${sid}:${flags}`); scene.mutate('Unpop'); },
        repop: (sid, flags) => { scene.event(`P:${sid}:${flags}`); scene.mutate('Repop'); },
        targetPoint: (sid, target) => { scene.event(`T:${sid}:${target}`); scene.mutate('Point'); return [52.25, 52.75]; },
        createProjectile: (request) => scene.projectile(request),
        sound: (sid, name) => { scene.event(`S:${name}`); scene.mutate('Sound'); },
        notify: (sid, event, info, value) => scene.event(`M:${event}:${info}:${value}`),
        kill: (form, flags) => scene.event(`K:${form}:${flags}`)
    });
    // 네 원문 몸체 중 현재 독립 입력을 실행하고 전체 raw/효과를 반환한다.
    scene.run = () => {
        const logic = new RawCannonFire(scene.pool, scene.types, scene.frames, scene.state, scene.hooks());
        let result = 'return';
        switch (scene.n(1)) {
        case 0: logic.frame(scene.fields); break;
        case 1:
            logic.initialize(scene.fields, 70, 60);
            scene.event('B:35:50:0:16');
            put(scene.fields.bytes, 4, 81);
            put(scene.fields.bytes, 8, 50);
            break;
        case 2: result = String(logic.step(scene.fields, Number(scene.row[7]))); break;
        case 3: result = logic.launch(scene.fields) ? '1' : '0'; break;
        default: throw new Error('캐논 원문 fixture 몸체 오류');
        }
        let slots = '';
        // 원문 네 슬롯 200바이트를 모두 비교한다.
        for (const sid of [50, 51, 60, 61]) slots += hex(scene.pool.slot(sid));
        return [result, scene.events.join(';'), hex(scene.fields.bytes), slots];
    };
    return scene;
}

// 프로파일/종류 범위의 원본 행을 재생한다. (2026-11-08 WB04-E114)
function replay(profile, kindLow, kindHigh) {
    const scene = makeScene();
    let count = 0;
    // C++ 계산으로 기대값을 만들지 않고 저장된 원문 행을 읽는다.
    for (const row of cannonRows) {
        if (Number(row[0]) !== profile || Number(row[1]) < kindLow || Number(row[1]) > kindHigh) continue;
        scene.prepare(row);
        assert.deepEqual(scene.run(), row.slice(11), `캐논 행 ${count}: 프로파일 ${row[0]} 몸체 ${row[1]} ${row.slice(2, 11).join(',')}`);
        count += 1;
    }
    return count;
}

test('CannonFire: 원본 10.78 썬 캐논의 전체 프레임/실패/재진입과 생성자·단계·launch 가 같다', { skip: cannonRows.length === 0 && '원본 fixture 없음' }, () => {
    const frames = replay(0, 0, 0);
    const others = replay(0, 1, 3);
    assert.ok(frames > 0 && others > 0);
});

test('CannonFire: 원본 10.78 썬더 캐논의 문자 구간·포효·발사 지연과 재진입, 생성자·단계·launch 가 같다', { skip: cannonRows.length === 0 && '원본 fixture 없음' }, () => {
    const frames = replay(1, 0, 0);
    const others = replay(1, 1, 3);
    assert.ok(frames > 0 && others > 0);
    const sun = cannonRows.filter((row) => Number(row[0]) === 0).length;
    const thunder = cannonRows.filter((row) => Number(row[0]) === 1).length;
    assert.equal(sun + thunder, 5534);
});

// 반더 장면이다(빔 생성은 이름 wrapper 의 생존 검사 + 훅 호출을 같은 순서로 재현한다). (2026-11-08 WB04-E114)
function makeVanderScene() {
    const scene = basis();
    scene.random = new GameRandom(0);
    scene.prepare = (row) => {
        scene.row = row;
        assert.equal(row.length, 16);
        scene.events = [];
        scene.state = new CannonFireState({ kind: CannonFireKind.Vander, type: 148, displayType: 80, effectType: 19, delay: 0.3, now: 10 });
        scene.random = new GameRandom(scene.n(6));
        scene.types[146].flags1 = scene.types[158].flags1 = scene.n(4) ? 0x10 : 0;
        const bytes = scene.fields.bytes;
        // 원본 64바이트의 동일 미초기화 패턴을 먼저 보존한다.
        for (let index = 0; index < bytes.length; index += 1) bytes[index] = (index * 13 + 7) & 0xff;
        put(bytes, 0, 0x50b170);
        put(bytes, 4, 81);
        put(bytes, 8, 50);
        put(bytes, 12, floatToBits(60.0));
        put(bytes, 16, floatToBits(60.0));
        put(bytes, 20, floatToBits(59.5));
        put(bytes, 24, floatToBits(59.5));
        put(bytes, 32, 0x50b16c);
        bytes[40] = 0x20;
        put(bytes, 42, 70, 2);
        put(bytes, 44, 60, 2);
        putDouble(bytes, 48, scene.n(2) ? 10 : 10.25);
        put(bytes, 56, 0);
        // 네 관찰 슬롯과 실제 GunForm 을 원문과 같은 바이트로 준비한다.
        for (const [sid, type] of [[50, 148], [51, 148], [60, scene.n(5) ? 146 : 158], [61, 158], [70, 34]]) {
            const raw = scene.input(sid);
            raw.fill(0);
            put(raw, 0, 0x12400000);
            put(raw, 6, 81, 2);
            raw[10] = type;
            raw[11] = sid === 60 ? scene.n(3) : 0;
            raw[12] = 0xa5;
            raw[34] = 1;
            put(raw, 14, floatToBits(sid < 60 ? 50.5 : 60.0));
            put(raw, 18, floatToBits(sid < 60 ? 50.5 : 60.0));
            put(raw, 36, scene.n(1));
        }
    };
    // 원문 관찰과 같은 callback 뒤에서만 현재 입력을 바꾼다.
    scene.mutate = (where) => {
        const mutation = scene.n(7);
        const bytes = scene.fields.bytes;
        if (where === 'Center' && mutation === 1) put(bytes, 44, 61, 2);
        if (where === 'Point' && mutation === 2) { put(bytes, 8, 51); put(bytes, 44, 61, 2); }
        if (where === 'Beam' && mutation === 3) scene.input(60)[11] = 2;
        if (where === 'Beam' && mutation === 4) put(bytes, 44, 61, 2);
        if (where === 'Hit' && mutation === 5) scene.input(60)[11] = 2;
        if (where === 'Refresh' && mutation === 6) { put(bytes, 8, 51); put(bytes, 44, 61, 2); scene.state.now = 100; }
        if (where === 'Unpop' && mutation === 7) put(bytes, 8, 51);
        if (where === 'Notify' && mutation === 8) put(bytes, 4, 82);
    };
    // 빔 wrapper: 표적이 사용 불가이면 아무것도 하지 않고, 아니면 104바이트 확보 → 실패 시 중단 → 생성 → 변경 순서이다.
    scene.beam = (parent, target, sound) => {
        if (scene.pool.slot(target)[11] & 3) return;
        scene.event('A:104');
        if (!scene.n(8)) return;
        scene.event(`L:${parent}:${target}:${sound ?? 'null'}`);
        scene.mutate('Beam');
    };
    scene.hooks = () => ({
        center: (sid) => { scene.event(`C:${sid}`); scene.mutate('Center'); return [59.5, 59.5]; },
        targetPoint: (source, target) => { scene.event(`T:${source}:${target}`); scene.mutate('Point'); return [52.25, 52.75]; },
        unpop: (sid, flags) => { scene.event(`U:${sid}:${flags}`); scene.mutate('Unpop'); },
        repop: (sid, flags) => scene.event(`P:${sid}:${flags}`),
        notify: (sid, event, info, value) => { scene.event(`M:${event}:${info}:${value}`); scene.mutate('Notify'); },
        kill: (sid, flags) => scene.event(`K:${sid}:${flags}`)
    });
    // 빔 생성 뒤 초기 source 의 refresh 또는 현재 target hit 를 실제 모듈이 고른다.
    scene.beamHooks = () => ({
        create: (source, target) => scene.beam(source, target, 'shortZap.wav'),
        refresh: (source) => { scene.event(`R:${source}`); scene.mutate('Refresh'); },
        hit: (source, target) => { scene.event(`H:${source}:${target}`); scene.mutate('Hit'); },
        random: scene.random
    });
    return scene;
}

test('CannonFire: 원본 10.78 반더 발사의 초기 source/target 분리·회피 난수·동시 완료·통지 재진입이 같다', { skip: vanderRows.length === 0 && '원본 fixture 없음' }, () => {
    const scene = makeVanderScene();
    let count = 0;
    // 저장된 원문 반더 행을 기대값 계산 없이 소비한다.
    for (const row of vanderRows) {
        scene.prepare(row);
        const logic = new RawCannonFire(scene.pool, scene.types, scene.frames, scene.state, scene.hooks(), scene.beamHooks());
        let result = 'return';
        const kind = scene.n(0);
        if (kind === 0) {
            logic.initialize(scene.fields, 70, 60);
            put(scene.fields.bytes, 4, 81);
            put(scene.fields.bytes, 8, 50);
            scene.event('G:70:0:16');
            scene.event('B:35:50:0:16');
        } else if (kind === 1) result = String(logic.step(scene.fields, Number(row[1])));
        else if (kind === 2) result = logic.launch(scene.fields) ? '1' : '0';
        else logic.frame(scene.fields);
        assert.equal(result, row[10], `반더 행 ${count} 결과`);
        assert.equal(scene.events.length === 0 ? '-' : scene.events.join(';'), row[11], `반더 행 ${count} 효과`);
        assert.equal(hex(scene.fields.bytes), row[12], `반더 행 ${count} 64바이트`);
        let slots = '';
        for (const sid of [50, 51, 60, 61]) slots += hex(scene.pool.slot(sid));
        assert.equal(slots, row[13], `반더 행 ${count} 네 슬롯`);
        assert.equal(scene.random.state, Number(row[14]), `반더 행 ${count} 난수`);
        const now = new DataView(new ArrayBuffer(8));
        now.setFloat64(0, scene.state.now, true);
        assert.equal(hex(new Uint8Array(now.buffer)), row[15], `반더 행 ${count} 현재 시각`);
        count += 1;
    }
    assert.equal(count, 393);
});

test('CannonFire: Missile 생성 경계 누락·판본/표 크기·프로세스 보조를 확인한다', { skip: cannonRows.length === 0 && '원본 fixture 없음' }, () => {
    const scene = makeScene();
    // 살아 있는 HP 표적에서 실제 Missile 경계가 누락되면 성공으로 우회하지 않는다.
    const found = cannonRows.find((row) => Number(row[1]) === 3 && Number(row[4]) === 0 && Number(row[5]) === 1);
    assert.ok(found);
    scene.prepare(found);
    const hooks = scene.hooks();
    delete hooks.createProjectile;
    const logic = new RawCannonFire(scene.pool, scene.types, scene.frames, scene.state, hooks);
    assert.ok(throws(() => logic.launch(scene.fields)));
    assert.deepEqual(scene.events, []);
    // 판본/표 크기/현재 TYPE 범위 오류.
    assert.ok(throws(() => new RawCannonFire({ edition: 'cd1072' }, scene.types, scene.frames, scene.state, {})));
    assert.ok(throws(() => new RawCannonFire(scene.pool, scene.types.slice(0, 10), scene.frames, scene.state, {})));
    assert.ok(throws(() => new RawCannonFire(scene.pool, scene.types, scene.frames.slice(0, 10), scene.state, {})));
    assert.ok(throws(() => new RawCannonFire(scene.pool, scene.types, scene.frames, new CannonFireState({ type: 188 }), {})));
    assert.equal(logic.pool, scene.pool);
    assert.equal(logic.type, scene.state.type);
    scene.state.currentType = { value: 77 };
    assert.equal(logic.type, 77);
    assert.ok(throws(() => logic.initialize(new GunAnimationFields(), 0, 60)));
    // 프로세스 보조: 부착 전 Form/Parent 0 → 프레임은 부모 범위 오류로 거부된다.
    scene.state.currentType = null;
    const process = new CannonFireProcess(new RawCannonFire(scene.pool, scene.types, scene.frames, scene.state, scene.hooks()), scene.fields);
    assert.equal(process.fields, scene.fields);
    process.onAttached();
    assert.equal(scene.fields.form, 0);
    assert.ok(throws(() => process.runFrame()));
});
