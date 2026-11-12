// 총(Gun) 프로세스 raw-gun-process.js(GunProcessFields·RawGunProcess·GunProcess·attachGunProcess·createGunProcess)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: gunprocess-x86.tsv 의 originals 행(생성 0·프레임 1·표적 해제 2·표적 지정 3·부착 직후 4·통지 5·발사 6·writeData 7·writeProcess 8·수신 관찰 9)이 호출 순서(검색/난수/애니메이션/종속/전송)·필드 0x38바이트·패킷 64바이트·길이까지 같고,
//    gun-notify-checking-x86.tsv 904행(checking 진단 복귀·중단·진단이 바꾼 표적/부모 상태 재조회)이 호출 순서·필드·outcome·checking·부모 상태까지 같다. cpppj GunProcessTests.cpp 의 GunScene 을 같은 순서로 옮겼다.
//  - cpppj 시험 이식: 방향별 동맹 getter 재생, boss/동맹 짧은 평가 순서, 미연결 경계·패킷 부족의 앞선 쓰기 보존, 표적 0 자연 해제 42행, 실제 Host 전송/수명, 공유 권한 재생·무동기화, 선택된 통지 진단/현재 소유자.
//  - 웹 계약: 수신 읽기(readData)·수신 기본 생성자·모듈 입력 검사.
// 10.72(CD)는 범위 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E359: 7-P2 RawGunProcess 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { RawAuthorityState } from '../www/core/raw-authority-state.js';
import { GunProcess, GunProcessFields, RawGunProcess, attachGunProcess, createGunProcess } from '../www/core/raw-gun-process.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E359)
const fixtures = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const allRows = readRows(path.join(fixtures, 'gunprocess-x86.tsv'));
const rows = allRows ? allRows.filter((row) => row[0] === 'originals') : null;
const noticeRows = readRows(path.join(fixtures, 'gun-notify-checking-x86.tsv'));

// 풀 크기와 native 가 쓰는 부모/표적/갱신 대상 SID 이다. (2026-11-13 WB04-E359)
const CAPACITY = 24000;

// 예외가 나는지 본다. (2026-11-13 WB04-E359)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열의 소문자 16진 문자열이다. (2026-11-13 WB04-E359)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 188칸 TYPE 입력이다(cpppj MakeFormTypeInputs: 모든 칸 0 + 내장 Form 네 생성자 주소). (2026-11-13 WB04-E359)
function blankTypes() {
    const types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0 }));
    for (const form of [1, 2, 3, 6]) types[form].constructorAddress = typeConstructorAddress(form);
    return types;
}

// fixture 의 64비트 정수 열을 double 로 본다(little endian 비트 그대로). (2026-11-13 WB04-E359)
function wideDouble(text) {
    const view = new DataView(new ArrayBuffer(8));
    view.setBigUint64(0, BigInt(text), true);
    return view.getFloat64(0, true);
}

// cpppj `GunScene`: 실제 효과 대신 oracle 과 같은 명시 외부 경계를 제공하는 검사 장면이다. 판단은 fixture 의 native 결과가 정한다. (2026-11-13 WB04-E359)
class GunScene {
    constructor() {
        this.pool = new SidPool(CAPACITY, { server: false });
        this.types = blankTypes();
        this.fields = new GunProcessFields();
        this.packet = new Uint8Array(64);
        this.events = [];
        this.exists = false;
        this.shotResult = false;
        this.animation = 0;
        this.found = 0;
        this.random = 0;
        this.state = GunScene.defaults();
        // native 의 관찰 번호를 client 영역에 확보한다.
        for (let sid = 5; sid < 128; sid += 1) assert.equal(this.pool.allocate(2), sid);
    }

    // 원문 전역의 기본값이다(cpppj `GunProcessState{}`: boss/server 참, 나머지 거짓, processType 34·TYPE 140/131/129). (2026-11-13 WB04-E359)
    static defaults() {
        return {
            now: 0, boss: true, server: true, fortMode: false, fixedMode: false, disabled: false, friendlyFire: false, checking: 0,
            windArcherType: 140, thunderCannonType: 131, rainCannonType: 129,
            allies: Array.from({ length: 9 }, () => new Array(9).fill(false)), allianceRead: null, checkingRead: null,
        };
    }

    // 현재 값을 소비 위치마다 읽는 전역 묶음이다. authority 가 있으면 boss/server 를 그 상자에서만 읽는다. (2026-11-13 WB04-E359)
    globals(authority = null) {
        const state = this.state;
        return {
            now: () => state.now,
            boss: () => (authority ? authority.boss : state.boss),
            server: () => (authority ? authority.server : state.server),
            fortMode: () => state.fortMode,
            fixedMode: () => state.fixedMode,
            disabled: () => state.disabled,
            friendlyFire: () => state.friendlyFire,
            checking: () => (state.checkingRead ? state.checkingRead() : state.checking),
            windArcherType: () => state.windArcherType,
            thunderCannonType: () => state.thunderCannonType,
            rainCannonType: () => state.rainCannonType,
            alliance: (owner, target) => (state.allianceRead ? state.allianceRead(owner, target) : (state.allies[owner][target] ? 1 : 0)),
        };
    }

    // 입력 필드와 padding 을 만들고 native 결과의 판단은 읽지 않는다. (2026-11-13 WB04-E359)
    prepare(row) {
        assert.equal(row.length, 34);
        this.events = [];
        this.state = GunScene.defaults();
        // 원본 new padding 과 패킷의 이전 바이트도 모두 보존한다.
        for (let index = 0; index < this.fields.bytes.length; index += 1) this.fields.bytes[index] = (index * 13 + 7) & 0xff;
        for (let index = 0; index < this.packet.length; index += 1) this.packet[index] = (index * 7 + 3) & 0xff;
        put(this.fields.bytes, 0, 0x50b668);
        this.fields.bytes[8] = Number(row[4]);
        const time = BigInt(row[17]);
        put(this.fields.bytes, 16, Number(time & 0xffffffffn));
        put(this.fields.bytes, 20, Number(time >> 32n));
        put(this.fields.bytes, 24, Number(row[3]), 2);
        put(this.fields.bytes, 28, Number(row[2]));
        put(this.fields.bytes, 32, Number(row[5]));
        put(this.fields.bytes, 40, 0x50b648);
        put(this.fields.bytes, 44, 70);
        put(this.fields.bytes, 48, 50);
        const parent = this.pool.physical(50);
        parent.fill(0);
        parent[10] = Number(row[6]);
        parent[11] = Number(row[8]);
        parent[34] = 1;
        parent[40] = Number(row[7]);
        put(parent, 6, 70, 2);
        put(parent, 14, 0x42480000);
        put(parent, 18, 0x42500000);
        const target = this.pool.physical(60);
        target.fill(0);
        target[11] = Number(row[9]);
        target[34] = 2;
        this.types[parent[10]].group = Number(row[5]) | 0;
        this.state.now = wideDouble(row[18]);
        this.state.boss = Number(row[10]) !== 0;
        this.state.server = Number(row[11]) !== 0;
        this.state.fortMode = Number(row[12]) !== 0;
        this.state.fixedMode = Number(row[13]) !== 0;
        this.state.disabled = Number(row[14]) !== 0;
        this.state.friendlyFire = Number(row[15]) !== 0;
        this.state.allies[1][2] = Number(row[16]) !== 0;
        this.found = Number(row[19]);
        this.random = Number(row[20]);
        this.animation = Number(row[21]);
        this.exists = Number(row[22]) !== 0;
        this.shotResult = Number(row[23]) !== 0;
    }

    // 각 현재 필드 재조회와 외부 호출의 인자/순서를 oracle 과 같은 사건으로 기록한다. (2026-11-13 WB04-E359)
    hooks() {
        const events = this.events;
        const pool = this.pool;
        return {
            removeDependency: (form, target) => { events.push(`D:${form}:${target}`); },
            addDependency: (target) => { events.push(`A:${target}`); },
            repopForm: () => { events.push('P'); },
            animationForm: (sid) => { events.push(`L:${sid}`); return this.animation; },
            killAnimation: (form, flags) => { if (this.exists) events.push(`K:${form}:${flags}`); },
            refreshRange: (sid) => { events.push(`R:${sid}`); },
            targetingMode: (sid, owner) => { events.push(`M:${sid}:${owner}`); return 2; },
            findTarget: (x, y, type, mode, owner) => { events.push(`F:${floatToBits(x)}:${floatToBits(y)}:${type}:${mode}:${owner}`); return this.found; },
            randomBetween: (low, high) => { events.push(`N:${floatToBits(low)}:${floatToBits(high)}`); return bitsToFloat(this.random); },
            startAnimation: (stage, form, target) => { events.push(`S:${stage}:${form}:${target}`); },
            parentBlocked: (sid) => (pool.slot(sid)[40] & 0x20) !== 0,
            shotContext: (sid) => {
                const raw = pool.slot(sid);
                events.push(`M:${sid}:1`, `C:${sid}`);
                return [get(raw, 14), get(raw, 18), 0x42500000, 0x42540000, raw[10], 2, raw[34], get(raw, 36, 4)];
            },
            shoot: (context, target) => { events.push(`H:${context.join(',')},${target}`); return this.shotResult; },
            firingInterval: () => 2.25,
            attach: (data, parent) => { events.push(`T:34:${parent}:0:0`); put(data.bytes, 44, 70); put(data.bytes, 48, parent); },
        };
    }

    // 새 로직을 만들고(TYPE group 복사 시점은 행 준비 뒤) 각 원본 진입점을 같은 fixture 입력으로 실행한다. (2026-11-13 WB04-E359)
    run(row, logic) {
        switch (Number(row[1])) {
        case 0: logic.construct(this.fields, 50); break;
        case 1: logic.frame(this.fields); break;
        case 2: logic.clearTarget(this.fields, Number(row[24]) !== 0); break;
        case 3: logic.setTarget(this.fields, 61); break;
        case 4: RawGunProcess.attached(this.fields); break;
        case 5: logic.notify(this.fields, Number(row[25]), 0x12345678, Number(row[26])); break;
        case 6: logic.fire(this.fields); break;
        case 7: return RawGunProcess.writeData(this.fields, this.packet, 70, Number(row[29]), Number(row[27]) !== 0, 1, 0x3a);
        case 8: return RawGunProcess.writeProcess(this.fields, this.packet, Number(row[28]) !== 0, Number(row[27]) !== 0, 1, 0x3a);
        case 9: logic.receiveUpdate(this.fields, Number(row[4]), Number(row[29])); break;
        default: throw new Error('GunProcess fixture 진입점 오류');
        }
        return 0;
    }

    // 원본의 전체 raw/padding/packet 과 사건 순서를 직렬화한다. (2026-11-13 WB04-E359)
    output(length) {
        return [this.events.join(';'), hex(this.fields.bytes), hex(this.packet), String(length)];
    }

    // 로직을 새로 만든다(TYPE group 은 생성 때 복사). (2026-11-13 WB04-E359)
    logic(hooks = this.hooks(), authority = null) {
        return new RawGunProcess(this.pool, this.types, this.globals(authority), hooks);
    }
}

// 같은 판본의 모든 저장 native 행을 복원 결과와 비교한다. sharedAuthority 이면 boss/server 를 같은 RawAuthorityState 에서만 읽고 일반 상태에는 반대값을 넣는다. (2026-11-13 WB04-E359)
function replay(sharedAuthority = false) {
    const scene = new GunScene();
    const authority = new RawAuthorityState();
    let count = 0;
    // 정상 반환당 전체 결과를 독립 비교한다.
    for (const row of rows) {
        scene.prepare(row);
        authority.server = scene.state.server;
        authority.boss = scene.state.boss;
        // 잘못된 fallback 이 실제 관문으로 새지 않는지 독립 검증한다.
        if (sharedAuthority) {
            scene.state.server = !authority.server;
            scene.state.boss = !authority.boss;
        }
        const logic = scene.logic(scene.hooks(), sharedAuthority ? authority : null);
        const length = scene.run(row, logic);
        assert.deepEqual(scene.output(length), row.slice(30), `Gun 행 ${count} body ${row[1]}`);
        count += 1;
    }
    assert.equal(count, rows.length);
    return count;
}

test('GunProcess: 원본 10.78 관측 전 행(생성·프레임·표적 해제/지정·부착·통지·발사·writer 둘·수신 관찰)이 호출 순서·필드·패킷·길이까지 같다', { skip: !rows }, () => {
    assert.equal(replay(), 1498);
});

test('GunProcess: 같은 관측을 공유 권한 상자의 boss/server 로 재생해도 같다(일반 상태의 반대값은 읽지 않는다)', { skip: !rows }, () => {
    assert.equal(replay(true), 1498);
});

test('GunProcess: 방향별 동맹 getter 를 Frame 입력에 다시 대고 사본 표의 반대값을 무시한다', { skip: !rows }, () => {
    const scene = new GunScene();
    let compared = 0;
    let reads = 0;
    // 표적을 검사하는 Frame 입력만 재사용하며 새 native 고유입력을 만들지 않는다.
    for (const row of rows) {
        if (Number(row[1]) !== 1) continue;
        scene.prepare(row);
        // 실제 소비자는 bool 로 복제하지 않은 방향별 DWORD 를 읽는다.
        const current = new Array(81).fill(0);
        scene.state.allianceRead = (owner, target) => { reads += 1; return current[owner * 9 + target]; };
        current[11] = Number(row[16]) ? 7 : 0;
        current[19] = Number(row[16]) ? 0 : 9;
        // 반대값의 초기 사본을 쓰면 원본 결과가 달라진다.
        scene.state.allies[1][2] = Number(row[16]) === 0;
        const length = scene.run(row, scene.logic());
        assert.deepEqual(scene.output(length), row.slice(30));
        compared += 1;
    }
    assert.ok(compared > 0 && reads > 0);
});

test('GunProcess: 현재 동맹 조회는 boss/유효표적/죽음 분기 뒤이며 친군사격 조회보다 앞서고 한 방향만 읽는다', () => {
    const scene = new GunScene();
    scene.prepare(rows[0]);
    const parent = scene.pool.physical(50);
    const target = scene.pool.physical(60);
    parent[10] = 128;
    parent[40] = 0;
    parent[34] = 1;
    target[11] = 0;
    target[34] = 2;
    scene.types[128].group = 1;
    put(scene.fields.bytes, 32, 1);
    put(scene.fields.bytes, 24, 60, 2);
    put(scene.fields.bytes, 16, 0);
    put(scene.fields.bytes, 20, 0);
    scene.state.fixedMode = false;
    scene.state.disabled = false;
    scene.state.now = -1;
    scene.state.boss = false;
    // 원본의 선택된 한 방향 읽기 횟수다.
    let reads = 0;
    scene.state.allianceRead = (owner, other) => { assert.deepEqual([owner, other], [1, 2]); reads += 1; return 0; };
    const logic = scene.logic();
    logic.frame(scene.fields);
    assert.deepEqual([reads, scene.fields.target], [0, 60]);
    scene.state.boss = true;
    logic.frame(scene.fields);
    assert.deepEqual([reads, scene.fields.target], [1, 60]);
    // 죽은 표적은 표를 읽기 전에 해제로 이동한다.
    target[11] = 2;
    logic.frame(scene.fields);
    assert.deepEqual([reads, scene.fields.target], [1, 0]);
    target[11] = 0;
    put(scene.fields.bytes, 24, 60, 2);
    scene.state.friendlyFire = true;
    scene.state.allianceRead = () => { reads += 1; scene.state.friendlyFire = false; return 7; };
    logic.frame(scene.fields);
    assert.deepEqual([reads, scene.fields.target], [2, 0]);
});

test('GunProcess: 미복원 검색/생성/효과를 빈 함수로 성공시키지 않고 패킷 부족은 선행 변경 전에 거부한다', () => {
    const scene = new GunScene();
    scene.prepare(rows[0]);
    const logic = new RawGunProcess(scene.pool, scene.types, scene.globals(), {});
    const before = Uint8Array.from(scene.fields.bytes);
    assert.ok(throws(() => logic.construct(scene.fields, 50)));
    assert.deepEqual(scene.fields.bytes, before);
    assert.ok(throws(() => logic.setTarget(scene.fields, 61)));
    assert.deepEqual(scene.fields.bytes, before);
    assert.ok(throws(() => RawGunProcess.writeData(scene.fields, scene.packet.subarray(0, 8), 70, 50, true, 1, 0x3a)));
    assert.deepEqual(scene.fields.bytes, before);
});

test('GunProcess: 원본이 실제 외부 효과를 호출하지 않는 boss0/표적0/전송없음 해제 42행을 빈 경계로 재생한다', { skip: !rows }, () => {
    let count = 0;
    // 같은 native 입력 중 실제 외부 효과가 없는 정상 반환만 독립 비교한다.
    for (const row of rows) {
        if (Number(row[1]) !== 2) continue;
        if (Number(row[10]) && (Number(row[3]) || (Number(row[24]) && Number(row[2]) !== 1))) continue;
        const scene = new GunScene();
        scene.prepare(row);
        const logic = new RawGunProcess(scene.pool, scene.types, scene.globals(), {});
        const length = scene.run(row, logic);
        assert.deepEqual(scene.output(length), row.slice(30));
        count += 1;
    }
    assert.equal(count, 21);
});

test('GunProcess: 실제 flags 0 공장·writer 의 OnAttached 이전 base 필드·Kernel 실행·repop·form 삭제 수명을 지킨다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const types = blankTypes();
    types[128].group = 1;
    let parent = 0;
    // 부모 입력을 native 와 같은 SID 50 에 확보한다.
    for (let id = 5; id <= 50; id += 1) parent = pool.allocate(2);
    assert.equal(parent, 50);
    const raw = pool.allocated(parent);
    raw.fill(0);
    raw[10] = 128;
    const destroy = new RawSquidDestroy(pool, new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536)), types);
    const kernel = new Kernel(32);
    const hostState = new SquidProcessState();
    hostState.now = 10;
    const state = GunScene.defaults();
    state.now = 10;
    state.boss = false;
    const buffer = new Uint8Array(64);
    const packets = [];
    const changed = [];
    const host = new SquidProcessHost(pool, types, kernel, destroy, hostState, {
        transmit: {
            writeData: (process, parentChanged) => {
                assert.ok(process instanceof GunProcess);
                changed.push(parentChanged);
                assert.ok(pool.slot(process.form)[11] & 4);
                return process.writePacket(buffer, parentChanged);
            },
            transmit: (length) => { packets.push(Uint8Array.from(buffer.subarray(0, length))); },
        },
    });
    const scene = new GunScene();
    scene.state = state;
    const logic = new RawGunProcess(pool, types, scene.globals(), {});
    try {
        const gun = createGunProcess(host, logic, parent);
        assert.ok(gun instanceof GunProcess);
        const form = gun.form;
        assert.deepEqual([form, host.fromForm(form) === gun, kernel.size], [pool.layout.serverFirst, true, 1]);
        assert.deepEqual([gun.fields.form, gun.fields.parent, gun.fields.dirty, gun.fields.stage], [form, parent, 0, 0]);
        assert.deepEqual([packets.length, changed], [1, [true]]);
        assert.deepEqual([packets[0].length, get(packets[0], 2, 2), get(packets[0], 4), get(packets[0], 9)], [31, 31, form, parent]);
        assert.deepEqual([packets[0][0], packets[0][1], packets[0][8], get(packets[0], 21, 2), get(packets[0], 23), get(packets[0], 27)], [1, 0x3a, 0xff, 0, 0, 1]);
        assert.deepEqual([get(pool.slot(parent), 6, 2), pool.slot(form)[11] & 0xc], [form, 8]);
        kernel.runFrame();
        assert.equal(gun.fields.stage, 1);
        put(gun.fields.bytes, 28, 3);
        gun.fields.bytes[8] = 8;
        host.refreshForm(gun);
        assert.deepEqual([gun.fields.stage, gun.fields.dirty, packets.length, changed[1], packets[1].length, get(packets[1], 9)], [3, 0, 2, false, 13, 3]);
        host.kill(gun, 0x40);
        assert.deepEqual([kernel.size, get(pool.slot(parent), 6, 2)], [0, 0]);
        assert.ok(pool.slot(form)[11] & 3);
        assert.deepEqual([destroy.preDepth, destroy.postDepth], [0, 0]);
    } finally {
        kernel.clear();
    }
});

test('GunProcess: 생성 후 바뀐 boss/server 를 각 해제 호출이 다시 읽고 일반 상태의 값은 무시한다(동기화 없음)', () => {
    const authority = new RawAuthorityState();
    authority.boss = false;
    authority.server = false;
    const scene = new GunScene();
    scene.prepare(rows[0]);
    scene.state.boss = true;
    scene.state.server = true;
    put(scene.fields.bytes, 24, 0, 2);
    put(scene.fields.bytes, 28, 1);
    scene.fields.bytes[8] = 0;
    const before = Uint8Array.from(scene.fields.bytes);
    const logic = new RawGunProcess(scene.pool, scene.types, scene.globals(authority), {});
    logic.clearTarget(scene.fields, false);
    assert.deepEqual(scene.fields.bytes, before);
    authority.boss = true;
    assert.ok(throws(() => logic.clearTarget(scene.fields, false)));
    authority.server = true;
    logic.clearTarget(scene.fields, false);
    // 자연 빈 표적도 원문 target dirty 4 는 쓰고 효과 장치는 요구하지 않는다.
    assert.deepEqual([scene.fields.dirty, scene.fields.target, scene.fields.stage], [4, 0, 1]);
});

// 통지 검사만의 식/파일 문자열을 원문 hex 로 기록한다. (2026-11-13 WB04-E359)
const noticeHex = (text) => Buffer.from(text, 'latin1').toString('hex');

test('GunProcess 통지: checking 진단 복귀·중단과 지역 단계/표적 재조회를 904개 전체 원문 결과와 대조한다', { skip: !noticeRows }, () => {
    const scene = new GunScene();
    let count = 0;
    // 통지와 종속 정리만 실행하고 Frame/Fire 는 호출하지 않는다.
    for (const row of noticeRows) {
        assert.equal(row.length, 9);
        const input = row[0].split(',');
        const control = row[1].split(',');
        assert.deepEqual([input.length, control.length], [29, 3]);
        const legacy = ['originals', ...input];
        while (legacy.length < 34) legacy.push('');
        scene.prepare(legacy);
        let checking = Number(control[0]);
        scene.state.checkingRead = () => checking;
        const hooks = scene.hooks();
        const animation = hooks.animationForm;
        // 조회가 checking 을 꺼도 이미 선택된 진단은 취소하지 않는다.
        hooks.animationForm = (parent) => {
            if (Number(control[1]) === 1) { checking = 0; put(scene.fields.bytes, 28, 9); }
            return animation(parent);
        };
        hooks.assertion = (expression, file, line) => {
            scene.events.push(`X:${noticeHex(expression)}:${noticeHex(file)}:${line}`);
            if (!Number(control[2])) throw new Error('명시 진단 중단');
            const mutation = Number(control[1]);
            // 진단이 표적을 고치면 통지 본문은 현재 WORD 를 다시 비교하고 부모 free 를 다시 읽는다.
            if (mutation === 2 || mutation === 3) {
                checking = 0;
                put(scene.fields.bytes, 24, Number(legacy[26]) & 65535, 2);
                put(scene.fields.bytes, 28, 2);
                scene.pool.physical(50)[11] = mutation === 2 ? 2 : 0;
            }
        };
        // 실제 공격 경로를 실행하지 못하게 한다.
        hooks.shoot = () => { throw new Error('통지 검사에서 발사 도달'); };
        hooks.startAnimation = () => { throw new Error('통지 검사에서 애니메이션 시작 도달'); };
        const logic = scene.logic(hooks);
        let outcome = 'return';
        try {
            logic.notify(scene.fields, Number(legacy[25]), 0x12345678, Number(legacy[26]));
        } catch (error) {
            if (error.message !== '명시 진단 중단') throw error;
            outcome = 'assertion';
        }
        assert.deepEqual(scene.output(0), row.slice(2, 6), `통지 행 ${count}`);
        assert.equal(outcome, row[6]);
        assert.equal(checking, Number(row[7]));
        assert.equal(scene.pool.slot(50)[11], Number(row[8]));
        count += 1;
    }
    assert.equal(count, 904);
});

test('GunProcess 통지: 이전 관측의 통지 352행만 재생하며 Frame/Fire/패킷 송신은 실행하지 않는다', { skip: !rows }, () => {
    const scene = new GunScene();
    let count = 0;
    // 그 밖의 단계/발사/패킷 진입점은 실행하지 않는다.
    for (const row of rows) {
        if (Number(row[1]) !== 5) continue;
        scene.prepare(row);
        const hooks = scene.hooks();
        hooks.shoot = () => { throw new Error('통지 회귀에서 발사 도달'); };
        scene.logic(hooks).notify(scene.fields, Number(row[25]), 0x12345678, Number(row[26]));
        assert.deepEqual(scene.output(0), row.slice(30));
        count += 1;
    }
    assert.equal(count, 352);
});

test('GunProcess 통지: 현재 외부 checking/서버 owner 를 읽고 선택된 getter/진단 누락만 원문 지점에서 중단한다', () => {
    const authority = new RawAuthorityState();
    const scene = new GunScene();
    scene.prepare(rows[0]);
    let checking = 0;
    scene.state.checkingRead = () => checking;
    let hooks = scene.hooks();
    delete hooks.animationForm;
    const missingGetter = new RawGunProcess(scene.pool, scene.types, scene.globals(), hooks);
    missingGetter.notify(scene.fields, 1, 0, 4);
    assert.equal(scene.fields.stage, 4);
    put(scene.fields.bytes, 28, 6);
    checking = 0x80000000;
    assert.ok(throws(() => missingGetter.notify(scene.fields, 1, 0, 4)));
    assert.equal(scene.fields.stage, 6);
    hooks = scene.hooks();
    scene.animation = 0;
    const missingDiagnostic = new RawGunProcess(scene.pool, scene.types, scene.globals(), hooks);
    assert.ok(throws(() => missingDiagnostic.notify(scene.fields, 1, 0, 4)));
    assert.equal(scene.fields.stage, 6);
    // 동일 현재 서버가 false 면 표적 진단/정리 관문 전체를 건너뛴다.
    authority.server = false;
    scene.state.server = true;
    const gated = new RawGunProcess(scene.pool, scene.types, scene.globals(authority), hooks);
    scene.events.length = 0;
    put(scene.fields.bytes, 24, 60, 2);
    gated.notify(scene.fields, 2, 0, 61);
    assert.deepEqual([scene.events, scene.fields.target], [[], 60]);
});

test('GunProcess 수신: 기본 생성자는 표적/시각/단계와 두 VT 만 쓰고 group·padding 은 보존하며 readData 는 dirty 별 부분 쓰기와 before/after 관찰을 한다', () => {
    const scene = new GunScene();
    scene.prepare(rows[0]);
    scene.state.now = 12.5;
    const logic = scene.logic();
    // 수신 기본 생성자: group(+0x20)·padding 은 입력 그대로, 나머지는 원문 값이다.
    scene.fields.bytes.fill(0xcc);
    logic.initializeReceived(scene.fields);
    assert.deepEqual([get(scene.fields.bytes, 0), get(scene.fields.bytes, 40), scene.fields.dirty, scene.fields.form, scene.fields.parent, scene.fields.target, scene.fields.stage, scene.fields.time],
        [0x50b668, 0x50b648, 0xff, 0, 0, 0, 0, 12.5]);
    assert.deepEqual([get(scene.fields.bytes, 32), scene.fields.bytes[0x34], scene.fields.bytes[0x37]], [0xcccccccc, 0xcc, 0xcc]);
    // 수신 기본 생성자는 부모를 0 으로 두므로, 이후 observer 가 조회할 부모를 실제 부착처럼 채운다.
    put(scene.fields.bytes, 48, 50);
    // dirty 별 부분 쓰기: 부모 DWORD 는 상자로, 시각/표적/단계/group 은 필드로, 나머지 바이트는 그대로 둔다.
    const packet = new Uint8Array(9 + 4 + 8 + 2 + 4 + 4);
    packet[8] = 0x1f;
    put(packet, 9, 50);
    packet.set([1, 2, 3, 4, 5, 6, 7, 8], 13);
    put(packet, 21, 61, 2);
    put(packet, 23, 3);
    put(packet, 27, 1);
    const before = Uint8Array.from(scene.fields.bytes);
    const parent = { value: 99 };
    scene.events.length = 0;
    scene.animation = 80;
    logic.readData(scene.fields, packet, parent);
    assert.equal(parent.value, 50);
    assert.deepEqual([...scene.fields.bytes.subarray(16, 24)], [1, 2, 3, 4, 5, 6, 7, 8]);
    assert.deepEqual([scene.fields.target, scene.fields.stage, scene.fields.group, scene.fields.dirty], [61, 3, 1, 0xff]);
    // 바뀐 곳은 시각·표적·단계·group 뿐이다(dirty 바이트와 padding 은 보존).
    for (let index = 0; index < before.length; index += 1) {
        if ((index >= 16 && index < 26) || (index >= 28 && index < 36)) continue;
        assert.equal(scene.fields.bytes[index], before[index], `바이트 ${index}`);
    }
    // 옛 단계는 0 이라 중지 관찰은 일어나지 않는다. 단계 6 입력 + 패킷 dirty 8 이면 읽기 전에 애니메이션을 중지한다.
    assert.deepEqual(scene.events, []);
    put(scene.fields.bytes, 28, 6);
    const small = new Uint8Array(13);
    small[8] = 8;
    put(small, 9, 4);
    scene.events.length = 0;
    logic.readData(scene.fields, small, parent);
    assert.equal(parent.value, 0);
    assert.deepEqual(scene.events, ['L:50', 'L:50', 'K:80:64']);
    assert.equal(scene.fields.stage, 4);
    // 패킷이 모자라면 앞선 observer/필드 효과는 되돌리지 않고 거부한다.
    put(scene.fields.bytes, 28, 3);
    const cut = new Uint8Array(9 + 4 + 3);
    cut[8] = 0x0b;
    put(cut, 9, 77);
    scene.events.length = 0;
    const timeBefore = Uint8Array.from(scene.fields.bytes.subarray(16, 24));
    assert.ok(throws(() => logic.readData(scene.fields, cut, parent)));
    assert.equal(parent.value, 77);
    assert.deepEqual([...scene.fields.bytes.subarray(16, 24)], [...timeBefore]);
    assert.deepEqual(scene.events, ['L:50', 'L:50', 'K:80:64']);
    assert.ok(throws(() => logic.readData(scene.fields, new Uint8Array(8), parent)));
});

test('GunProcess 입력 검사: PATCH 풀·188칸 TYPE 표만 받고 생성은 부모 SID/group 을 쓰기 전에 거부한다', () => {
    const scene = new GunScene();
    scene.prepare(rows[0]);
    assert.throws(() => new RawGunProcess({ edition: 'cd1072' }, scene.types, scene.globals(), {}), TypeError);
    assert.throws(() => new RawGunProcess(scene.pool, scene.types.slice(0, 171), scene.globals(), {}), RangeError);
    const logic = scene.logic();
    const before = Uint8Array.from(scene.fields.bytes);
    assert.throws(() => logic.initialize(scene.fields, 0), TypeError);
    assert.throws(() => logic.initialize(scene.fields, CAPACITY), TypeError);
    scene.pool.physical(50)[10] = 100;
    scene.types[100].group = 2;
    const grouped = scene.logic();
    assert.throws(() => grouped.initialize(scene.fields, 50), /group 원본 assert/);
    assert.deepEqual(scene.fields.bytes, before);
    assert.ok(logic.pool === scene.pool);
    const host = { pool: {}, attach: () => { throw new RangeError('풀 불일치인데 부착 경계에 도달'); } };
    const mismatch = (error) => error instanceof TypeError && /풀 불일치/.test(error.message);
    assert.throws(() => attachGunProcess(host, logic, scene.fields, 50), mismatch);
    assert.throws(() => createGunProcess(host, logic, 50), mismatch);
    // 풀 불일치 검사는 필드 초기화(부모 검사)보다 먼저다: 잘못된 부모를 함께 주어도 풀 불일치 문구가 나온다.
    assert.throws(() => createGunProcess(host, logic, 0), mismatch);
    assert.deepEqual(scene.fields.bytes, before);
});

// 단계 시험용 장면이다: 부모 128(group 1, 서버 소유자 1)·시각 0(기한 지남)·now 10·boss/서버 참·검사 꺼짐, 필드는 지정한 단계/표적이다. (2026-11-13 WB04-E359)
function stageScene(stage, target = 0) {
    const scene = new GunScene();
    scene.prepare(rows[0]);
    const parent = scene.pool.physical(50);
    parent.fill(0);
    parent[10] = 128;
    parent[34] = 1;
    put(parent, 6, 70, 2);
    put(parent, 14, 0x42480000);
    put(parent, 18, 0x42500000);
    const other = scene.pool.physical(60);
    other.fill(0);
    other[34] = 2;
    scene.types[128].group = 1;
    put(scene.fields.bytes, 32, 1);
    put(scene.fields.bytes, 24, target, 2);
    put(scene.fields.bytes, 28, stage);
    scene.fields.bytes[8] = 0;
    put(scene.fields.bytes, 16, 0);
    put(scene.fields.bytes, 20, 0);
    put(scene.fields.bytes, 44, 70);
    put(scene.fields.bytes, 48, 50);
    Object.assign(scene.state, { now: 10, boss: true, server: true, fortMode: false, fixedMode: false, disabled: false, friendlyFire: false, checking: 0 });
    scene.animation = 0;
    scene.exists = true;
    return scene;
}

test('GunProcessFields: 접근자는 바이트 위치·폭·부호를 그대로 읽는다(시각 8바이트 전체·group 부호·Form/부모 하위 WORD·표적 WORD)', () => {
    // 공용 보기에 낮은 바이트 0xff 를 남긴 뒤에도 다음 시각 읽기는 자기 여덟 바이트만 쓴다.
    const other = new GunProcessFields();
    put(other.bytes, 16, 0xffffffff);
    put(other.bytes, 20, 0x3ff00000);
    assert.ok(other.time > 1);
    const fields = new GunProcessFields();
    put(fields.bytes, 16, 0x9999999a);
    put(fields.bytes, 20, 0x3ff19999);
    assert.equal(fields.time, 1.1);
    // 표적은 WORD 만 읽고 이웃 두 바이트는 무시한다.
    put(fields.bytes, 24, 0xabcd1234);
    assert.equal(fields.target, 0x1234);
    put(fields.bytes, 28, 0xfffffffe);
    assert.equal(fields.stage, 0xfffffffe);
    put(fields.bytes, 32, 0xfffffffe);
    assert.equal(fields.group, -2);
    put(fields.bytes, 44, 0x12345678);
    put(fields.bytes, 48, 0x9abcdef0);
    assert.deepEqual([fields.form, fields.parent], [0x5678, 0xdef0]);
    fields.bytes[8] = 0x5a;
    assert.equal(fields.dirty, 0x5a);
});

test('GunProcess 생성: 쓰는 필드는 원문 값이고 쓰지 않는 padding 은 입력 그대로이며 부모 group 0/1 이 아니면 거부한다', () => {
    for (const group of [0, 1]) {
        const scene = new GunScene();
        scene.prepare(rows[0]);
        scene.pool.physical(50)[10] = 128;
        scene.types[128].group = group;
        scene.state.now = 12.5;
        scene.fields.bytes.fill(0xcc);
        scene.logic().initialize(scene.fields, 50);
        const expected = new Uint8Array(0x38).fill(0xcc);
        put(expected, 0, 0x50b668);
        expected[8] = 0xff;
        // 12.5 = 0x4029000000000000
        put(expected, 16, 0);
        put(expected, 20, 0x40290000);
        put(expected, 24, 0, 2);
        put(expected, 28, 0);
        put(expected, 32, group);
        put(expected, 40, 0x50b648);
        put(expected, 44, 0);
        put(expected, 48, 0);
        assert.equal(hex(scene.fields.bytes), hex(expected), `group ${group}`);
    }
    // 부모 TYPE 이 표 길이(188)와 같거나 크면 RangeError, 풀 용량 이상/0 이면 TypeError 이다.
    const scene = new GunScene();
    scene.prepare(rows[0]);
    const logic = scene.logic();
    for (const type of [188, 255]) {
        scene.pool.physical(50)[10] = type;
        assert.throws(() => logic.initialize(scene.fields, 50), (error) => error instanceof RangeError && /부모 타입 오류/.test(error.message));
    }
    for (const parent of [0, CAPACITY, 65535]) {
        assert.throws(() => logic.initialize(scene.fields, parent), (error) => error instanceof TypeError && /부모 SID 오류/.test(error.message));
    }
});

test('GunProcess 수신 기본 생성자: 두 VT·dirty·Form/부모·표적·시각·단계만 쓰고 나머지 padding 은 그대로다', () => {
    const scene = new GunScene();
    scene.prepare(rows[0]);
    scene.state.now = 12.5;
    scene.fields.bytes.fill(0xcc);
    scene.logic().initializeReceived(scene.fields);
    const expected = new Uint8Array(0x38).fill(0xcc);
    put(expected, 0, 0x50b668);
    expected[8] = 0xff;
    put(expected, 16, 0);
    put(expected, 20, 0x40290000);
    put(expected, 24, 0, 2);
    put(expected, 28, 0);
    put(expected, 40, 0x50b648);
    put(expected, 44, 0);
    put(expected, 48, 0);
    assert.equal(hex(scene.fields.bytes), hex(expected));
});

test('GunProcess 부모 검사: 부모 SID 0·용량 이상·TYPE 188 이상은 풀 밖 오류로 거부하고 free/dead 부모는 거부하지 않는다', () => {
    for (const parent of [0, CAPACITY, 65535]) {
        const scene = stageScene(0);
        put(scene.fields.bytes, 48, parent);
        assert.throws(() => scene.logic().frame(scene.fields), (error) => error instanceof RangeError && /부모 SID 오류/.test(error.message), `부모 ${parent}`);
    }
    for (const type of [188, 255]) {
        const scene = stageScene(0);
        scene.pool.physical(50)[10] = type;
        assert.throws(() => scene.logic().frame(scene.fields), (error) => error instanceof RangeError && /부모 타입 오류/.test(error.message), `TYPE ${type}`);
    }
    // free(1)·dead(2) 부모도 여기서 거부하지 않고 단계 진행을 계속한다.
    for (const state of [1, 2, 3]) {
        const scene = stageScene(0);
        scene.state.boss = false;
        scene.pool.physical(50)[11] = state;
        scene.logic().frame(scene.fields);
        assert.equal(scene.fields.stage, 1, `상태 ${state}`);
    }
});

test('GunProcess 프레임 관문: group 불일치와 abstract(1)/astral(8) 비트는 각각 원문 assert 이고 정지(0x20) 비트는 무동작이다', () => {
    const assertText = /Frame group\/abstract\/astral 원본 assert/;
    {
        const scene = stageScene(0);
        put(scene.fields.bytes, 32, 0);
        assert.throws(() => scene.logic().frame(scene.fields), assertText);
    }
    for (const extra of [1, 8, 9]) {
        const scene = stageScene(0);
        scene.pool.physical(50)[40] = extra;
        assert.throws(() => scene.logic().frame(scene.fields), assertText, `extra ${extra}`);
    }
    // 다른 비트는 assert 가 아니며 idle 이 검색 단계로 넘어간다. 정지 비트는 필드를 바꾸지 않는다.
    for (const extra of [0, 2, 4, 0x10, 0x40, 0x80, 0x20]) {
        const scene = stageScene(0);
        scene.state.boss = false;
        scene.pool.physical(50)[40] = extra;
        const before = Uint8Array.from(scene.fields.bytes);
        scene.logic().frame(scene.fields);
        if (extra === 0x20) assert.deepEqual(scene.fields.bytes, before);
        else assert.equal(scene.fields.stage, 1, `extra ${extra}`);
    }
    // 고정 모드·비활성도 같은 무동작이다.
    for (const key of ['fixedMode', 'disabled']) {
        const scene = stageScene(0);
        scene.state[key] = true;
        const before = Uint8Array.from(scene.fields.bytes);
        scene.logic().frame(scene.fields);
        assert.deepEqual(scene.fields.bytes, before, key);
    }
});

test('GunProcess 프레임 표적 관문: 동맹 소유자 범위(8 허용·9 거부)·풀 밖 표적의 즉시 해제·해제 효과가 바꾼 부모 extra 를 다시 읽는다', () => {
    // 소유자 번호는 0~8 만 허용한다(부모 쪽 9·표적 쪽 9 는 각각 거부, 8/8 은 방향별 동맹 한 칸을 읽는다).
    for (const [parentOwner, targetOwner, rejected] of [[9, 2, true], [1, 9, true], [8, 8, false]]) {
        const scene = stageScene(0, 60);
        scene.pool.physical(50)[34] = parentOwner;
        scene.pool.physical(60)[34] = targetOwner;
        const seen = [];
        scene.state.allianceRead = (owner, target) => { seen.push([owner, target]); return 0; };
        const run = () => scene.logic().frame(scene.fields);
        if (rejected) assert.throws(run, (error) => error instanceof RangeError && /동맹 소유자 범위 오류/.test(error.message));
        else {
            run();
            assert.deepEqual(seen, [[8, 8]]);
        }
    }
    // 표적 WORD 가 풀 용량과 같으면 유효 표적이 아니므로 슬롯을 읽지 않고 해제한다.
    const outside = stageScene(0, CAPACITY);
    outside.logic().frame(outside.fields);
    assert.deepEqual([outside.events, outside.fields.target], [[`D:70:${CAPACITY}`, 'P'], 0]);
    // 해제 효과가 부모 extra 를 바꾸면 그 뒤 비트 1 검사는 바뀐 값을 읽는다(1 이면 무동작, 2 이면 계속).
    for (const [extra, stays] of [[1, true], [2, false]]) {
        const scene = stageScene(0, 60);
        scene.pool.physical(60)[11] = 2;
        const hooks = scene.hooks();
        const remove = hooks.removeDependency;
        hooks.removeDependency = (form, target) => { remove(form, target); scene.pool.physical(50)[40] = extra; };
        scene.logic(hooks).frame(scene.fields);
        assert.equal(scene.fields.stage, stays ? 0 : 1, `extra ${extra}`);
    }
});

test('GunProcess 외부 경계 누락: 선택된 경계가 없으면 효과 전에 원문 문구로 거부하고 필드를 바꾸지 않는다', () => {
    // 훅 하나를 뺀 로직으로 동작을 실행해 문구, 필드 불변, 효과 없음을 확인한다.
    const rejects = (scene, hookName, action, pattern) => {
        const hooks = scene.hooks();
        delete hooks[hookName];
        const logic = scene.logic(hooks);
        const before = Uint8Array.from(scene.fields.bytes);
        assert.throws(() => action(logic, scene.fields), (error) => error instanceof Error && pattern.test(error.message), hookName);
        assert.deepEqual(scene.fields.bytes, before, hookName);
        assert.deepEqual(scene.events, [], hookName);
    };
    // 해제: 기존 표적이 있으면 종속 제거가, 재전송이 필요하면 재전송이 필요하다.
    rejects(stageScene(1, 60), 'removeDependency', (logic, fields) => logic.clearTarget(fields, false), /clear 실제 효과 경계 누락/);
    rejects(stageScene(1, 60), 'repopForm', (logic, fields) => logic.clearTarget(fields, false), /clear 실제 효과 경계 누락/);
    rejects(stageScene(2, 0), 'repopForm', (logic, fields) => logic.clearTarget(fields, true), /clear 실제 효과 경계 누락/);
    // 표적 지정: 여섯 경계 중 하나라도 없거나 서버가 아니면 거부한다.
    for (const hookName of ['removeDependency', 'addDependency', 'animationForm', 'killAnimation', 'refreshRange', 'repopForm']) {
        rejects(stageScene(2, 60), hookName, (logic, fields) => logic.setTarget(fields, 61), /표적 지정 효과 누락/);
    }
    {
        const scene = stageScene(2, 60);
        scene.state.server = false;
        const before = Uint8Array.from(scene.fields.bytes);
        assert.throws(() => scene.logic().setTarget(scene.fields, 61), /표적 지정 효과 누락/);
        assert.deepEqual(scene.fields.bytes, before);
    }
    // 검색 단계: 세 경계가 모두 필요하다.
    for (const hookName of ['targetingMode', 'findTarget', 'randomBetween']) {
        const scene = stageScene(1);
        scene.state.boss = false;
        rejects(scene, hookName, (logic, fields) => logic.frame(fields), /검색 경계 누락/);
    }
    // 발사: 네 경계가 모두 필요하다.
    for (const hookName of ['parentBlocked', 'shotContext', 'shoot', 'firingInterval']) {
        rejects(stageScene(7, 60), hookName, (logic, fields) => logic.fire(fields), /실제 발사 경계 누락/);
    }
    // 수신 관찰: 현재 애니메이션 조회와 중지가 모두 필요하다.
    for (const hookName of ['animationForm', 'killAnimation']) {
        rejects(stageScene(3), hookName, (logic, fields) => logic.receiveUpdate(fields, 8, 1), /수신 animation 중지 경계 누락/);
    }
    // 검사가 켜진 idle 은 조회 경계가 없으면 원문 assert 로 거부하고, 꺼져 있으면 경계 없이 진행한다.
    {
        const scene = stageScene(0);
        scene.state.boss = false;
        scene.state.checking = 1;
        rejects(scene, 'animationForm', (logic, fields) => logic.frame(fields), /getAnimFormSid==0 원본 assert/);
        const quiet = stageScene(0);
        quiet.state.boss = false;
        const hooks = quiet.hooks();
        delete hooks.animationForm;
        quiet.logic(hooks).frame(quiet.fields);
        assert.equal(quiet.fields.stage, 1);
    }
});

test('GunProcess 통지: 표적 삭제 뒤 부모 상태 bit1(dead)이 서 있지 않을 때만 한 번 더 재전송한다(상태 0·1·2·3)', () => {
    for (const [state, repops] of [[0, 2], [1, 2], [2, 1], [3, 1]]) {
        const scene = stageScene(2, 60);
        scene.pool.physical(50)[11] = state;
        scene.logic().notify(scene.fields, 2, 0, 60);
        assert.equal(scene.events.filter((event) => event === 'P').length, repops, `상태 ${state}`);
        assert.deepEqual([scene.fields.target, scene.fields.stage], [0, 1]);
    }
    // boss 가 아니면 해제는 아무것도 하지 않으므로 표적은 그대로이고, 부모 상태 bit1 이 없을 때만 두 번째(유일한) 재전송 경계를 요구한다.
    for (const [state, rejected] of [[0, true], [1, true], [2, false], [3, false]]) {
        const missing = stageScene(2, 60);
        missing.state.boss = false;
        missing.pool.physical(50)[11] = state;
        const hooks = missing.hooks();
        hooks.repopForm = undefined;
        const run = () => missing.logic(hooks).notify(missing.fields, 2, 0, 60);
        if (rejected) assert.throws(run, (error) => error instanceof Error && /notify 재전송 누락/.test(error.message), `상태 ${state}`);
        else run();
        assert.equal(missing.fields.target, 60);
    }
});

test('GunProcess 발사: 풀 밖 표적(WORD 용량)은 유효 표적이 아니라 shot 을 부르지 않고 해제하며 cooldown 은 현재 시각 + 간격으로 쓴다', () => {
    const scene = stageScene(7, CAPACITY);
    const hooks = scene.hooks();
    const intervalArguments = [];
    hooks.firingInterval = (...args) => { intervalArguments.push(args); return 2.25; };
    scene.logic(hooks).fire(scene.fields);
    assert.deepEqual(intervalArguments, [[50]]);
    assert.ok(!scene.events.some((event) => event.startsWith('H:')));
    assert.deepEqual([scene.fields.target, scene.fields.stage, scene.fields.time], [0, 1, 12.25]);
    // 죽은(2)·free(1) 표적도 shot 없이 해제한다.
    for (const state of [1, 2, 3]) {
        const dead = stageScene(7, 60);
        dead.pool.physical(60)[11] = state;
        dead.logic().fire(dead.fields);
        assert.ok(!dead.events.some((event) => event.startsWith('H:')), `상태 ${state}`);
        assert.equal(dead.fields.target, 0);
    }
});

test('GunProcess 수신 관찰: 읽기 전(beforeRead)·dirty 8·옛 단계 3/6 일 때만 현재 애니메이션을 두 번 조회해 중지한다', () => {
    const cases = [[3, 8, 1, true], [6, 8, 1, true], [3, 8, 0, false], [3, 1, 1, false], [3, 0x10, 1, false], [4, 8, 1, false], [5, 8, 1, false], [0, 8, 1, false], [7, 8, 1, false]];
    for (const [stage, dirty, before, kills] of cases) {
        const scene = stageScene(stage);
        scene.animation = 80;
        scene.logic().receiveUpdate(scene.fields, dirty, before);
        assert.deepEqual(scene.events, kills ? ['L:50', 'L:50', 'K:80:64'] : [], `단계 ${stage} dirty ${dirty} 전 ${before}`);
    }
    // 애니메이션이 없으면 한 번만 조회하고 중지하지 않는다.
    const none = stageScene(3);
    none.logic().receiveUpdate(none.fields, 8, 1);
    assert.deepEqual(none.events, ['L:50']);
});

test('GunProcess 수신 읽기: dirty 비트마다 정확한 폭만 읽고 나머지 필드를 보존하며 모자란 패킷은 비트별로 거부한다', () => {
    // [dirty 비트, 폭, 필드 위치(부모는 상자)]이다.
    for (const [bit, width, at] of [[1, 4, -1], [2, 8, 16], [4, 2, 24], [8, 4, 28], [0x10, 4, 32]]) {
        const scene = stageScene(0);
        const logic = scene.logic();
        const payload = Array.from({ length: width }, (_, index) => 0x31 + index * 7 + bit);
        const packet = new Uint8Array(9 + width);
        packet[8] = bit;
        packet.set(payload, 9);
        const expected = Uint8Array.from(scene.fields.bytes);
        if (at >= 0) expected.set(payload, at);
        const parent = { value: 99 };
        logic.readData(scene.fields, packet, parent);
        assert.equal(hex(scene.fields.bytes), hex(expected), `비트 ${bit}`);
        assert.equal(parent.value, bit === 1 ? get(packet, 9) : 0, `비트 ${bit} 부모`);
        // 한 바이트 모자란 패킷은 필드를 바꾸지 않고 거부한다.
        const cut = new Uint8Array(8 + width);
        cut[8] = bit;
        const second = stageScene(0);
        const before = Uint8Array.from(second.fields.bytes);
        assert.throws(() => second.logic().readData(second.fields, cut, { value: 0 }), (error) => error instanceof RangeError && /dirty 필드 부족/.test(error.message), `모자란 비트 ${bit}`);
        assert.deepEqual(second.fields.bytes, before);
    }
    // 머리만 있는 패킷(dirty 0)은 허용되고 아무것도 바꾸지 않으며, 머리가 모자라면 거부한다.
    const scene = stageScene(0);
    const before = Uint8Array.from(scene.fields.bytes);
    const parent = { value: 99 };
    scene.logic().readData(scene.fields, new Uint8Array(9), parent);
    assert.deepEqual([parent.value, scene.fields.bytes], [0, before]);
    assert.throws(() => scene.logic().readData(scene.fields, new Uint8Array(8), parent), (error) => error instanceof RangeError && /머리 부족/.test(error.message));
});

test('GunProcess writer: 모든 dirty/부모/force 조합에서 정확한 길이·내용을 쓰고 딱 맞는 버퍼는 허용하며 모자라면 아무것도 바꾸지 않는다', () => {
    for (let dirty = 0; dirty < 0x20; dirty += 1) {
        for (const parent of [0, 77]) {
            for (const [force, spare] of [[false, 0], [true, 0], [false, 3], [true, 3]]) {
                const fields = new GunProcessFields();
                for (let index = 0; index < fields.bytes.length; index += 1) fields.bytes[index] = (index * 11 + 3) & 0xff;
                fields.bytes[8] = dirty;
                const snapshot = Uint8Array.from(fields.bytes);
                const effective = force ? 0xff : dirty;
                const length = 9 + (parent ? 4 : 0) + ((effective & 2) ? 8 : 0) + ((effective & 4) ? 2 : 0) + ((effective & 8) ? 4 : 0) + ((effective & 0x10) ? 4 : 0);
                const label = `dirty ${dirty} 부모 ${parent} force ${force} 여분 ${spare}`;
                const small = new Uint8Array(length - 1).fill(0xee);
                assert.throws(() => RawGunProcess.writeData(fields, small, 70, parent, force, 1, 0x3a), RangeError, label);
                assert.deepEqual(fields.bytes, snapshot, label);
                assert.ok(small.every((value) => value === 0xee), label);
                // 여분 0 은 딱 맞는 버퍼(허용), 여분 3 은 뒤의 사용하지 않는 바이트가 보존되는지 본다.
                const packet = new Uint8Array(length + spare).fill(0xee);
                assert.equal(RawGunProcess.writeData(fields, packet, 70, parent, force, 1, 0x3a), length, label);
                const expected = [1, 0x3a, length & 0xff, length >> 8, 70, 0, 0, 0, force ? 0xff : (dirty | (parent ? 1 : 0))];
                if (parent) expected.push(77, 0, 0, 0);
                // 선택된 필드는 시각 8·표적 2·단계 4·group 4 바이트를 이 순서로 복사한다.
                for (const [bit, offset, width] of [[2, 16, 8], [4, 24, 2], [8, 28, 4], [0x10, 32, 4]]) {
                    if (effective & bit) expected.push(...snapshot.subarray(offset, offset + width));
                }
                assert.deepEqual([...packet.subarray(0, length)], expected, label);
                assert.deepEqual([...packet.subarray(length)], new Array(spare).fill(0xee), label);
                assert.equal(fields.dirty, 0, label);
                for (let index = 0; index < snapshot.length; index += 1) if (index !== 8) assert.equal(fields.bytes[index], snapshot[index], `${label} 바이트 ${index}`);
            }
        }
    }
});

test('GunProcess writeProcess: 기본 protocol 1·message 0x3a 이고 부모 DWORD 는 includeParent 또는 force 일 때만 현재 inner 부모에서 쓴다', () => {
    const make = () => {
        const fields = new GunProcessFields();
        put(fields.bytes, 44, 70);
        put(fields.bytes, 48, 50);
        fields.bytes[8] = 0;
        return fields;
    };
    const none = new Uint8Array(64).fill(0xee);
    assert.equal(RawGunProcess.writeProcess(make(), none, false), 9);
    assert.deepEqual([...none.subarray(0, 9)], [1, 0x3a, 9, 0, 70, 0, 0, 0, 0]);
    const withParent = new Uint8Array(64).fill(0xee);
    assert.equal(RawGunProcess.writeProcess(make(), withParent, true), 13);
    assert.deepEqual([...withParent.subarray(0, 13)], [1, 0x3a, 13, 0, 70, 0, 0, 0, 1, 50, 0, 0, 0]);
    const forced = new Uint8Array(64).fill(0xee);
    assert.equal(RawGunProcess.writeProcess(make(), forced, false, true, 5, 0x41), 31);
    assert.deepEqual([...forced.subarray(0, 13)], [5, 0x41, 31, 0, 70, 0, 0, 0, 0xff, 50, 0, 0, 0]);
});

test('GunProcess 공장: 풀이 같은 Host 에만 flags 0 으로 부착하고 기본 TYPE 34 를 쓰며 GunProcess 가 아닌 결과는 null 이다', () => {
    const scene = new GunScene();
    scene.prepare(rows[0]);
    const logic = scene.logic();
    const calls = [];
    const host = { pool: scene.pool, attach: (process, type, parent, flags) => { calls.push([process instanceof GunProcess, type, parent, flags]); return process; } };
    const gun = createGunProcess(host, logic, 50);
    assert.ok(gun instanceof GunProcess);
    assert.deepEqual(calls, [[true, 34, 50, 0]]);
    assert.deepEqual([get(gun.fields.bytes, 0), gun.fields.dirty, gun.fields.stage], [0x50b668, 0xff, 0]);
    // 지정한 TYPE 이 그대로 전달된다.
    createGunProcess(host, logic, 50, 40);
    assert.deepEqual(calls.at(-1), [true, 40, 50, 0]);
    assert.ok(attachGunProcess(host, logic, new GunProcessFields(), 50) instanceof GunProcess);
    assert.deepEqual(calls.at(-1), [true, 34, 50, 0]);
    attachGunProcess(host, logic, new GunProcessFields(), 50, 45);
    assert.deepEqual(calls.at(-1), [true, 45, 50, 0]);
    // 부착 결과가 없거나 GunProcess 가 아니면 null 이다.
    assert.equal(attachGunProcess({ pool: scene.pool, attach: () => null }, logic, new GunProcessFields(), 50), null);
    assert.equal(attachGunProcess({ pool: scene.pool, attach: () => ({}) }, logic, new GunProcessFields(), 50), null);
});

test('GunProcess 래퍼: 부착 직후 처리는 Form/부모를 필드에 쓴 뒤 attached 로 단계 3→2·6→5 만 되돌리고 통지는 같은 필드에 전달된다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const types = blankTypes();
    types[128].group = 1;
    let parent = 0;
    for (let id = 5; id <= 50; id += 1) parent = pool.allocate(2);
    const raw = pool.allocated(parent);
    raw.fill(0);
    raw[10] = 128;
    const destroy = new RawSquidDestroy(pool, new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536)), types);
    const kernel = new Kernel(32);
    const hostState = new SquidProcessState();
    hostState.now = 10;
    const buffer = new Uint8Array(64);
    const host = new SquidProcessHost(pool, types, kernel, destroy, hostState, {
        transmit: { writeData: (process, parentChanged) => process.writePacket(buffer, parentChanged), transmit: () => {} },
    });
    const scene = new GunScene();
    scene.state = GunScene.defaults();
    scene.state.now = 10;
    scene.state.boss = false;
    const logic = new RawGunProcess(pool, types, scene.globals(), {});
    try {
        const gun = createGunProcess(host, logic, parent);
        for (const [from, to] of [[3, 2], [6, 5], [4, 4], [0, 0]]) {
            put(gun.fields.bytes, 28, from);
            put(gun.fields.bytes, 44, 0);
            put(gun.fields.bytes, 48, 0);
            gun.fields.bytes[8] = 0;
            gun.onAttached();
            assert.deepEqual([gun.fields.stage, gun.fields.form, gun.fields.parent, gun.fields.dirty], [to, gun.form, parent, from !== to ? 8 : 0], `단계 ${from}`);
        }
        // 가상 통지는 같은 필드 이미지에서 처리한다(애니메이션 종료 4).
        put(gun.fields.bytes, 28, 0);
        gun.notify(1, 0, 4);
        assert.equal(gun.fields.stage, 4);
    } finally {
        kernel.clear();
    }
});

test('GunProcess TYPE 표: group 이 없는 칸은 0(C++ 기본값)이고, 단계 2 의 회전 판정은 효과 뒤 현재 부모 TYPE 의 group 을 다시 읽는다', () => {
    // group 속성이 없는 TYPE 은 0 으로 복사한다(부모 TYPE 128 의 필드 group 0 과 같아 진행한다).
    const plain = stageScene(0);
    plain.state.boss = false;
    delete plain.types[128].group;
    put(plain.fields.bytes, 32, 0);
    plain.logic().frame(plain.fields);
    assert.equal(plain.fields.stage, 1);
    // 검사가 켜진 단계 2 의 현재 AnimationForm 조회가 부모 TYPE 을 group 0 인 129 로 바꾸면, 회전 판정(group 0 이고 바람 아처가 아님)은 단계 4 로 간다.
    const scene = stageScene(2, 60);
    scene.state.checking = 1;
    scene.types[129].group = 0;
    const hooks = scene.hooks();
    hooks.animationForm = () => { scene.pool.physical(50)[10] = 129; return 0; };
    scene.logic(hooks).frame(scene.fields);
    assert.deepEqual([scene.fields.stage, scene.events.some((event) => event.startsWith('S:'))], [4, false]);
    // 바뀌지 않으면 group 1 이라 회전 애니메이션을 시작하고 단계 3 이 된다.
    const rotate = stageScene(2, 60);
    rotate.state.checking = 1;
    rotate.logic().frame(rotate.fields);
    assert.deepEqual([rotate.fields.stage, rotate.events.filter((event) => event.startsWith('S:'))], [3, ['S:2:70:60']]);
});
