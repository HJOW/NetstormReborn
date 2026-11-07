// 객체 부착 프로세스 호스트(SquidProcessHost)·Regular 이벤트·Process 부착 부모를 원본 10.78 관측과 대조한다(WB04-D5c).
//  - 원본 관측(cpppj/tests/fixtures, 10.78 행만): process-x86 의 시나리오 재생(부모 가상 pre/post/처리기·new/free·로그·전파만 대체), process-attach-current-local 24행,
//    baseprocess-attach-current-local 24행. 기대값은 fixture 에서 읽는다(cpppj ProcessTests·ProcessAttachCurrentTests·BaseProcessAttachCurrentTests 와 같은 입력/장치).
//  - 계약 검사는 cpppj ProcessTests 의 개별 장면(기본 처리기 주기·다리 지연 낙하·재진입 Kill·체인 순서/해제·Unpop 부모 사망·부착 가드·공통 Unpop 훅 요구·공유 권한)과
//    ProcessAttachCurrent/BaseProcessAttachCurrent 의 Refresh 진단·VT 선택·생성 실패 부분 효과·타입 진단을 같은 입력으로 옮겼다.
// CD 10.72 행과 Client/World(RawSurfaceWorld) 접합 장면은 범위 밖이다(WB05에서 옮긴다). 새 CPU 관측이나 실제 플레이 검사가 아니다.
// (2026-11-08 WB04-D5c: Process 호스트 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, adler32, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy, SQUID_DESTROY_EFFECT } from '../www/core/squid-destroy.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SQUID_FORM_VTABLES } from '../www/core/squid-vtables.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { GameAuxState } from '../www/core/game-aux-state.js';
import { Kernel } from '../www/core/kernel.js';
import { RawAuthorityState } from '../www/core/raw-authority-state.js';
import { attachBaseProcess, attachProcessForm } from '../www/core/raw-process-attach.js';
import { RegularProcess, SquidProcess, SquidProcessHost, SquidProcessState, createProcessHostRuntime, hasScheduledBridgeFall, scheduleBridgeFall } from '../www/core/squid-process.js';
import { BRIDGE_FALL_EVENT } from '../www/core/bridge-lifecycle.js';

// 원본 관측 파일 폴더와 행 읽기이다. 파일이 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-D5c)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
// 첫 열이 originals 인 행만 고른다(10.78 패치). (2026-11-08 WB04-D5c)
const patchRows = (name) => readRows(path.join(FIXTURES, name))?.filter((row) => row[0] === 'originals') ?? null;
const processRows = patchRows('process-x86.tsv');
const processAttachRows = patchRows('process-attach-current-local-x86.tsv');
const baseAttachRows = patchRows('baseprocess-attach-current-local-x86.tsv');

// 풀 크기·합성 부모 가상 표 기록값·관찰 슬롯 수·못 찾음 표식(기계어 생성기와 같음)이다. (2026-11-08 WB04-D5c)
const CAPACITY = 32768, PARENT_VTABLE = 0x12010000, WATCH_SLOTS = 96, NOT_FOUND = 0xffffffff;

// 바이트 배열의 소문자 16진 문자열이다. (2026-11-08 WB04-D5c)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 오류가 나는지 확인한다(성공을 무조건 인정하지 않음). (2026-11-08 WB04-D5c)
function throws(action) {
    try {
        action();
    } catch {
        return true;
    }
    return false;
}

// 64비트 double 비트(10진 문자열/BigInt)와 수의 변환이다. (2026-11-08 WB04-D5c)
function bitsToDouble(text) {
    const view = new DataView(new ArrayBuffer(8));
    view.setBigUint64(0, BigInt(text), true);
    return view.getFloat64(0, true);
}
function doubleToBits(value) {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    return view.getBigUint64(0, true);
}

// 내장 Form 네 생성자 주소만 넣은 188칸 TYPE 입력이다(cpppj MakeFormTypeInputs). (2026-11-08 WB04-D5c)
const formTypeInputs = () => Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));

// 같은 풀의 공장·공간 해제·삭제기·Kernel·상태·호스트 묶음이다(cpppj 검사의 공통 준비). (2026-11-08 WB04-D5c)
function worldParts({ types = formTypeInputs(), server = true, regular = null, asset = null } = {}) {
    const pool = new SidPool(CAPACITY, { server });
    const factory = new SquidFactory(pool, types);
    const unpop = new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536));
    const destroy = new RawSquidDestroy(pool, unpop, types);
    const kernel = new Kernel();
    const state = new SquidProcessState();
    const host = new SquidProcessHost(pool, types, kernel, destroy, state, { regular, asset });
    return { pool, types, factory, destroy, kernel, state, host };
}

// 원본 관측 장면이다: 기계어 생성기와 같은 연산을 같은 순서로 실행하고 같은 형식의 관찰 문자열을 만든다. (2026-11-08 WB04-D5c)
class Scene {
    // 서버/권한/시각/타입 순환 초기값으로 만든다. sharedAuthority 이면 실제 두 bool 을 빌려 독립 fallback 값과 달라도 소비 경로가 빌린 값을 읽는지 본다.
    constructor(server, boss, now, rotation, sharedAuthority = false) {
        // 합성 부모 타입의 genus(flags2)는 생성기와 같다.
        const types = formTypeInputs();
        types[82].flags2 = 0;
        types[90].flags2 = 0x4000;
        types[122].flags2 = 0x10000010;
        types[100].flags2 = 0x400000;
        this.events = [];
        this.script = [];
        this.cursor = 0;
        this.parents = [];
        this.created = [];
        this.slotIndex = new Map();
        const parts = worldParts({
            types,
            server,
            // 부모 이벤트 처리기 대체: 호출을 기록하고 입력 목록의 다음 값을 돌려준다.
            regular: (parent, event, count, payload) => {
                this.events.push(`H:${parent}:${event}:${count}:${floatToBits(payload)}`);
                assert.ok(this.cursor < this.script.length, '처리기 반환값 목록 소진');
                return this.script[this.cursor++];
            },
            asset: { emit: (event) => this.asset(event), selected: () => 0 },
        });
        Object.assign(this, parts);
        this.state.now = now;
        this.state.typeRotation = rotation;
        if (sharedAuthority) {
            // Kill/Form 의 소비 경로가 fallback 을 읽으면 독립 fixture 와 달라지도록 독립 값은 반대로 둔다.
            this.authority = new RawAuthorityState();
            this.authority.server = server;
            this.authority.boss = boss;
            this.state.boss = !boss;
            this.state.server = !server;
            this.state.bindAuthority(this.authority);
        } else {
            this.state.boss = boss;
        }
    }

    // 부모 객체의 가상 pre/post 와 삭제 전파 대체이다. 깊이 감소는 실제 공통 훅과 같은 위치에서 한다.
    asset(event) {
        const tail = `:${event.sid}:${event.flags}`;
        switch (event.effect) {
        case SQUID_DESTROY_EFFECT.preDestroy:
            this.events.push(`P${tail}`);
            this.destroy.completePreDestroy();
            break;
        case SQUID_DESTROY_EFFECT.postDestroy:
            this.events.push(`O${tail}`);
            this.destroy.completePostDestroy();
            break;
        case SQUID_DESTROY_EFFECT.transmit:
            this.events.push(`X${tail}`);
            break;
        default:
            throw new Error('장면에 없는 자산 삭제 효과');
        }
    }

    // 살아 있는 프로세스의 [슬롯, 생성 번호]를 슬롯 순서로 모은다. 비어 있는 슬롯의 대응은 버린다.
    alive() {
        const result = [];
        // 생성기와 같은 범위의 슬롯만 직접 비교한다.
        for (let slot = 1; slot < WATCH_SLOTS; slot += 1) {
            if (this.kernel.get(slot)) result.push([slot, this.slotIndex.get(slot)]);
            else this.slotIndex.delete(slot);
        }
        return result;
    }

    // 연산 하나를 실행하고 생성기의 반환값과 같은 수를 돌려준다.
    apply(text) {
        const op = text.split(':');
        const kind = op[0][0];
        const number = (index) => Number(BigInt(op[index]) & 0xffffffffn);
        this.events = [];
        switch (kind) {
        case 'P': {
            const sid = this.pool.allocate(number(1));
            const raw = this.pool.allocated(sid);
            // 가상 표 기록값은 호스트에서 역참조하지 않는다.
            put(raw, 0, PARENT_VTABLE);
            raw[10] = number(2);
            raw[11] = number(3);
            raw[40] = number(4);
            this.parents.push(sid);
            return sid;
        }
        case 'A':
        case 'B': {
            const process = this.host.addRegular(this.parents[number(1)], number(2), bitsToFloat(number(3)), kind === 'A' ? 0x50 : number(4));
            this.created.push(process);
            if (process) this.slotIndex.set(this.kernel.find(process), this.created.length - 1);
            return this.created.length - 1;
        }
        case 'T':
            this.state.now = bitsToDouble(op[1]);
            return 0;
        case 'R':
            this.script = op[1].split(',').map((item) => bitsToFloat(Number(item)));
            this.cursor = 0;
            this.kernel.runFrame();
            return this.cursor;
        case 'K':
            this.host.kill(this.created[number(1)], number(2));
            return 0;
        case 'F':
        case 'M': {
            const parent = this.parents[number(1)];
            const found = kind === 'F' ? this.host.findEvent(parent, number(2)) : this.host.findEventMasked(parent, number(2), number(3));
            return found ? this.slotIndex.get(this.kernel.find(found)) : NOT_FOUND;
        }
        case 'D':
            this.destroy.destroy(this.parents[number(1)], number(2), this.host.hooks());
            return 0;
        case 'S': {
            const raw = this.pool.allocated(this.parents[number(1)]);
            raw[number(2) === 0 ? 11 : 40] ^= number(3);
            return 0;
        }
        default:
            throw new Error('알 수 없는 연산');
        }
    }

    // client/서버 삭제 기록 20쌍씩의 Adler-32 이다.
    logAdler() {
        const bytes = new Uint8Array(2 * 20 * 8);
        let offset = 0;
        // client 기록, 서버 기록 순서이다.
        for (const client of [true, false]) {
            // 레코드마다 SID·TYPE DWORD 를 넣는다.
            for (const record of this.pool.deletions(client)) {
                put(bytes, offset, record.sid);
                put(bytes, offset + 4, record.type);
                offset += 8;
            }
        }
        return adler32(bytes);
    }

    // 생성기의 observe 와 같은 순서의 문자열이다. Pop 깊이는 form 의 postPop 이 바로 되돌리므로 항상 0 이다.
    observe(result, before) {
        const alive = this.alive();
        const pool = this.pool;
        const numbers = [result, pool.freeCount, pool.predictableCursor, pool.firstFree(true), pool.tail(true), pool.firstFree(false), pool.tail(false), adler32(pool.bytes()), this.logAdler(),
            this.destroy.preDepth, this.destroy.postDepth, 0, this.state.typeRotation];
        let text = `${numbers.join(',')}|`;
        const slots = [];
        const fields = [];
        const now = [];
        // 슬롯과 프로세스 필드는 슬롯 순서다.
        for (const [slot, index] of alive) {
            now.push(index);
            slots.push(`${slot}:${index}`);
            const process = this.created[index];
            fields.push(`${index}:${process.form}:${process.parent}:${doubleToBits(process.time)}:${process.event}:${floatToBits(process.payload)}:${process.count}`);
        }
        text += `${slots.length ? slots.join(',') : '-'}|${fields.length ? fields.join(';') : '-'}|`;
        text += `${this.events.length ? this.events.join(';') : '-'}|`;
        // 이번 연산에서 사라진 프로세스가 해제된 프로세스다.
        const freed = before.filter((index) => !now.includes(index));
        return text + (freed.length ? freed.join(',') : '-');
    }

    // 살아 있는 생성 번호만 오름차순으로 돌려준다.
    aliveIndices() {
        return this.alive().map((item) => item[1]).sort((a, b) => a - b);
    }
}

// 시나리오 재생이다. 한 줄이 한 시나리오이며 연산마다 직전의 살아 있는 프로세스와 비교해 해제 목록을 만든다. (2026-11-08 WB04-D5c)
function replay(rows, minimum, sharedAuthority = false) {
    let scenarios = 0;
    let operations = 0;
    for (const row of rows) {
        assert.equal(row.length, 8);
        const scene = new Scene(row[2] === '1', row[3] === '1', bitsToDouble(row[4]), Number(row[5]), sharedAuthority);
        const ops = row[6].split(' ');
        const expected = row[7].split(' ');
        assert.equal(expected.length, ops.length + 1);
        assert.equal(scene.observe(0, []), expected[0], `시나리오 ${row[1]} 초기`);
        for (let index = 0; index < ops.length; index += 1) {
            const before = scene.aliveIndices();
            const result = scene.apply(ops[index]);
            assert.equal(scene.observe(result, before), expected[index + 1], `시나리오 ${row[1]} 연산 ${index}(${ops[index]})`);
            operations += 1;
        }
        scenarios += 1;
    }
    assert.ok(scenarios >= minimum, `시나리오 ${scenarios}개`);
    assert.ok(operations >= minimum * 10, `연산 ${operations}개`);
}

// 패치 10.78 의 실제 기계어 관찰을 재생한다.
test('SquidProcessHost: 원본10.78 Process 시나리오 재생', { skip: !processRows && '원본 fixture 없음' }, () => {
    replay(processRows, 100);
});

// Native Kill 의 서버 번호/권한/flags 관문을 같은 장기 bool 조회로 다시 비교한다(cpppj Process_SharedAuthorityReplaysBothNativeBodies).
test('SquidProcessHost: 공유 권한(빌린 boss/server)으로도 같은 관찰이 나온다', { skip: !processRows && '원본 fixture 없음' }, () => {
    replay(processRows, 100, true);
});

// 처리기가 없으면 원본 base 처럼 payload 가 주기가 된다. 첫 호출은 부착한 다음 프레임이다.
test('SquidProcessHost: 기본 처리기는 payload 를 주기로 반복한다', () => {
    const scene = new Scene(true, true, 100.0, 0);
    const plain = new SquidProcessHost(scene.pool, scene.types, scene.kernel, scene.destroy, scene.state);
    const parent = scene.apply('P:2:82:0:0');
    const process = plain.addRegular(parent, 7, 0.5);
    assert.ok(process);
    assert.deepEqual([process.count, process.time], [0, 100.0]);
    scene.kernel.runFrame();
    assert.deepEqual([process.count, process.time], [1, 100.5]);
    scene.kernel.runFrame();
    assert.equal(process.count, 1);
    scene.state.now = 100.5;
    scene.kernel.runFrame();
    assert.deepEqual([process.count, process.time], [2, 101.0]);
});

// 다리의 지연 낙하 예약(이벤트 0x2692)은 같은 부모에서 조회되고, 처리기가 0 을 돌려주면 프로세스와 form 이 사라진다.
test('SquidProcessHost: 다리 지연 낙하 예약이 등록되고 0 반환으로 끝난다', () => {
    const scene = new Scene(true, true, 5.0, 0);
    const parent = scene.apply('P:0:82:0:0');
    const free = scene.pool.freeCount;
    assert.equal(hasScheduledBridgeFall(scene.host, parent), false);
    const fall = scheduleBridgeFall(scene.host, parent, 20.75, 21.9);
    scene.created.push(fall);
    scene.slotIndex.set(scene.kernel.find(fall), 0);
    // payload 는 (20 & 255) | (21 << 8) = 5396 을 float 로 바꾼 값이다.
    assert.equal(fall.event, BRIDGE_FALL_EVENT);
    assert.equal(fall.payload, 5396);
    assert.equal(hasScheduledBridgeFall(scene.host, parent), true);
    assert.equal(scene.host.findEvent(parent, 1), null);
    assert.equal(scene.host.findEventMasked(parent, 0xff00, 0x2600), fall);
    assert.equal(scene.pool.freeCount, free - 1);
    scene.apply('R:0');
    assert.equal(scene.kernel.size, 0);
    assert.equal(hasScheduledBridgeFall(scene.host, parent), false);
    assert.equal(scene.pool.freeCount, free);
    assert.deepEqual([scene.destroy.preDepth, scene.destroy.postDepth], [0, 0]);
});

// 처리기 안에서 자기 프로세스를 지워도 이후 필드를 건드리지 않는다.
test('SquidProcessHost: 처리기가 자기 프로세스를 지워도 안전하다', () => {
    const scene = new Scene(true, true, 0.0, 0);
    let target = null;
    const reentrant = new SquidProcessHost(scene.pool, scene.types, scene.kernel, scene.destroy, scene.state, {
        regular: () => {
            reentrant.kill(target, 0);
            return 3.0;
        },
    });
    const parent = scene.apply('P:2:82:0:0');
    target = reentrant.addRegular(parent, 1, 1.0);
    scene.kernel.runFrame();
    assert.equal(scene.kernel.size, 0);
    assert.equal(reentrant.findEvent(parent, 1), null);
});

// 가장 최근에 붙인 프로세스가 체인 머리이고, 중간 프로세스를 지워도 앞뒤 링크가 이어진다.
test('SquidProcessHost: 종속 체인 순서와 중간 해제', () => {
    const scene = new Scene(true, true, 0.0, 0);
    const parent = scene.apply('P:2:82:0:0');
    // 같은 이벤트 세 개를 붙이면 마지막 것이 먼저 조회된다.
    for (let index = 0; index < 3; index += 1) scene.apply('A:0:5:1065353216');
    assert.equal(scene.host.findEvent(parent, 5), scene.created[2]);
    scene.host.kill(scene.created[1], 0);
    assert.equal(scene.host.findEvent(parent, 5), scene.created[2]);
    scene.host.kill(scene.created[2], 0);
    assert.equal(scene.host.findEvent(parent, 5), scene.created[0]);
    scene.host.kill(scene.created[0], 0);
    assert.equal(scene.host.findEvent(parent, 5), null);
    assert.equal(scene.kernel.size, 0);
});

// 부모가 지워지는 중(dead)이면 form 의 Unpop 은 void 만 켜고 종속 체인과 contained 비트를 그대로 둔다(004ae140/004ac6b0).
// 최종 풀 상태만 비교하는 기계어 관찰로는 부모가 정리된 뒤 이 차이가 지워지므로 Unpop 직후의 raw 필드를 직접 확인한다.
test('SquidProcessHost: 부모가 죽는 중이면 Unpop 이 체인을 유지한다', () => {
    // 체인 머리 쪽 form 을 부모가 살아 있을 때와 죽는 중일 때 각각 Unpop 한다.
    for (const dying of [false, true]) {
        const scene = new Scene(true, true, 0.0, 0);
        const parent = scene.apply('P:2:82:0:0');
        scene.apply('A:0:1:1065353216');
        scene.apply('A:0:2:1065353216');
        const older = scene.created[0].form;
        const head = scene.created[1].form;
        // 부모의 머리(+6)는 가장 최근 form, 그 form 의 다음(+4)은 먼저 붙인 form 이다.
        const word = (sid, offset) => get(scene.pool.slot(sid), offset, 2);
        assert.deepEqual([word(parent, 6), word(head, 4)], [head, older]);
        if (dying) scene.pool.allocated(parent)[11] |= 2;
        scene.host.hooks().unpopForm(head);
        // 어느 쪽이든 form 은 void 가 된다.
        assert.notEqual(scene.pool.slot(head)[11] & 4, 0);
        if (dying) {
            assert.deepEqual([word(parent, 6), word(head, 4), scene.pool.slot(head)[11] & 8], [head, older, 8]);
        } else {
            assert.deepEqual([word(parent, 6), scene.pool.slot(head)[11] & 8], [older, 0]);
        }
    }
});

// 진단 장치가 없는 잘못된 프로세스 타입(9·63)은 등록 전에 거부하고, 패치의 죽은 부모는 조용히 건너뛴다.
// 전송이 필요한 flags(0x50 비트 없음)는 기준선 소스에서는 **등록·Create 뒤 writer 선택 위치**에서 거부된다: 타입 순환이 전진하고 Kernel 에 프로세스가 남는다.
// (cpppj 옛 검사 process_attach_guards 는 이 경우도 "등록 전 거부"를 기대하나, 20261105-42 이후 소스는 사전 검증이 없어 소스 읽기로는 그 기대와 다르다. 웹은 기준선 소스를 따르며
// 이 불일치는 단위 문서에 보고한다 — cpppj 를 실행해 확인하지는 않았다.)
test('SquidProcessHost: 부착 가드(진단 없는 잘못된 타입과 비활성 부모, writer 없는 전송 flags)', () => {
    const patch = new Scene(true, true, 0.0, 7);
    const parent = patch.apply('P:2:82:4:0');
    assert.ok(throws(() => patch.host.attach(new RegularProcess(1, 1.0), 9, parent, 0x50)));
    assert.ok(throws(() => patch.host.attach(new RegularProcess(1, 1.0), 63, parent, 0x50)));
    assert.equal(patch.kernel.size, 0);
    assert.equal(patch.state.typeRotation, 7);
    patch.apply('D:0:0');
    assert.equal(patch.host.addRegular(parent, 1, 1.0), null);
    assert.equal(patch.kernel.size, 0);
    // 전송이 필요한 flags: 등록 뒤 writer 미연결에서 거부되고 부분 효과(회전 전진·등록)가 남는다.
    const sending = new Scene(true, true, 0.0, 7);
    const sendingParent = sending.apply('P:2:82:4:0');
    assert.throws(() => sending.host.attach(new RegularProcess(1, 1.0), 46, sendingParent, 0), /writer 미연결/);
    assert.deepEqual([sending.kernel.size, sending.state.typeRotation], [1, 8]);
});

// form 루트는 전용 Unpop 훅 없이는 공통 삭제에 넣을 수 없다.
test('SquidProcessHost: Form 루트는 Unpop 훅 없이 공통 삭제에 넣을 수 없다', () => {
    const scene = new Scene(true, true, 0.0, 0);
    scene.apply('P:2:82:0:0');
    scene.apply('A:0:1:1065353216');
    const form = scene.created[0].form;
    const bare = { emit: () => {}, selected: () => 0 };
    assert.ok(throws(() => scene.destroy.destroy(form, 0, bare)));
    assert.equal(scene.kernel.size, 1);
    assert.equal(scene.pool.slot(form)[11] & 2, 0);
});

// Process 부착 접두(서버 0): 원본 24행의 진단 순서와 공통 자식 인자, 전체 raw 슬롯을 대조한다. writer/송신은 실행하지 않는다.
test('attachProcessForm: 원본10.78 서버0 파생 접두 24행', { skip: !processAttachRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of processAttachRows) {
        assert.equal(row.length, 4);
        const [stored, flags, profile] = row[1].split(',').map(Number);
        const pool = new SidPool(CAPACITY);
        const raw = pool.physical(5);
        // 입력 payload 는 생성기와 같다.
        for (let index = 0; index < raw.length; index += 1) raw[index] = (index * 29 + 7) & 0xff;
        put(raw, 16, stored, 2);
        let events = '';
        const event = (value) => { events += events ? `;${value}` : value; };
        attachProcessForm(pool, 5, 7, flags, {
            server: () => false,
            lookup: () => assert.fail('서버 0 에서는 lookup 하지 않는다'),
            writeData: () => assert.fail('서버 0 에서는 writer 를 부르지 않는다'),
            transmit: () => assert.fail('서버 0 에서는 송신하지 않는다'),
            assertion: (expression, file, line) => {
                event(`D${expression}|${file}|${line}`);
                if (profile) put(raw, 16, 11, 2);
            },
            attach: (form, parent, value) => {
                assert.equal(form, 5);
                event(`A${parent},${value},${get(raw, 16, 2)}`);
            },
        });
        assert.equal(events, row[2], row[1]);
        assert.equal(hex(pool.slot(5)), row[3], row[1]);
        count += 1;
    }
    assert.equal(count, 24);
});

// 서버 1 이면 현재 PID lookup(L)을 먼저 하고, flags 에 0x50 비트가 없을 때만 writer(W, 인자는 부모 변경 여부)로 크기를 받아 진단(D, 행 번호 16진)과 송신(T)을 한 뒤 flags|0x40 으로 공통 부착(A)한다.
// 크기 0 은 transmitSize!=0(0xa7f), 0x7ff 초과는 TRANSMIT_BUFFER_SIZE(0xa81) 진단 뒤 이어지고, 음수는 진단도 송신도 없다. 부모가 바뀌었는데 0x80 비트가 없으면 pfPROCESS_DEBUG(0xa77) 진단이 가장 먼저이다.
test('attachProcessForm: 서버 접두는 lookup→writer→진단→송신→0x40 부착 순서를 따른다', () => {
    // [flags, 저장된 부모, writer 크기, 기대 사건]
    const cases = [
        [0x80, 7, 5, 'L;W0;T5;A192'],
        [0, 3, 5, 'Da77;L;W1;T5;A64'],
        [0, 7, 0, 'L;W0;Da7f;A64'],
        [0, 7, 0x800, 'L;W0;Da81;T2048;A64'],
        [0, 7, -1, 'L;W0;A64'],
        [0x10, 7, 9, 'L;A80'],
        [0x40, 7, 9, 'L;A64'],
        [0x90, 7, 9, 'L;A208'],
    ];
    for (const [flags, stored, size, expected] of cases) {
        const pool = new SidPool(CAPACITY);
        put(pool.physical(5), 16, stored, 2);
        const events = [];
        attachProcessForm(pool, 5, 7, flags, {
            server: () => true,
            lookup: () => events.push('L'),
            writeData: (changed) => {
                events.push(`W${changed ? 1 : 0}`);
                return size;
            },
            transmit: (value) => events.push(`T${value}`),
            assertion: (expression, file, line) => {
                assert.equal(file, '\\Ns\\O\\Squid.cpp');
                events.push(`D${line.toString(16)}`);
            },
            attach: (form, parent, value) => {
                assert.deepEqual([form, parent], [5, 7]);
                events.push(`A${value}`);
            },
        });
        assert.equal(events.join(';'), expected, `flags ${flags} 부모 ${stored} 크기 ${size}`);
    }
    // 선택되지 않은 장치는 요구하지 않지만, 선택되었는데 연결되지 않았으면 그 위치에서 오류이다(앞선 lookup 은 남는다).
    const pool = new SidPool(CAPACITY);
    put(pool.physical(5), 16, 7, 2);
    const seen = [];
    assert.throws(() => attachProcessForm(pool, 5, 7, 0, { server: () => true, lookup: () => seen.push('L'), assertion: () => {}, attach: () => {} }), /writeData/);
    assert.deepEqual(seen, ['L']);
    assert.doesNotThrow(() => attachProcessForm(pool, 5, 7, 0x10, { server: () => true, lookup: () => {}, attach: () => {} }));
});

// 신규 로컬 BaseProcess 부착(Kernel/Create/가상 부착은 명시 경계): 원본 24행의 최초 호출/필드/슬롯 관측을 대조한다.
test('attachBaseProcess: 원본10.78 신규 로컬 입력 24행', { skip: !baseAttachRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of baseAttachRows) {
        assert.equal(row.length, 6);
        const [state, kind, profile] = row[1].split(',').map(Number);
        const pool = new SidPool(CAPACITY);
        const raw = pool.physical(5);
        // Native 의 입력 payload 만 동일하게 준비한다.
        for (let index = 0; index < raw.length; index += 1) raw[index] = (index * 29 + 7) & 0xff;
        put(raw, 0, 0x10002000);
        raw[10] = 2;
        pool.physical(7)[11] = state;
        const fields = { form: 0, parent: 0 };
        let events = '';
        let cause = 'returned';
        const event = (value) => { events += events ? `;${value}` : value; };
        try {
            attachBaseProcess(pool, fields, kind, 7, 0, 0x50, {
                typeEnd: () => 63,
                processFormType: () => 2,
                assertion: (expression, file, line) => event(`D${expression}|${file}|${line}`),
                registerProcess: () => {
                    event('K');
                    return 7;
                },
                create: (flags) => {
                    event(`C2,${flags}`);
                    if (profile) throw Object.assign(new Error('create'), { stop: 'create' });
                    return 5;
                },
                attach: (selected, target, flags) => {
                    assert.equal(selected, 5);
                    event(`A${target},${flags}`);
                },
                onAttached: () => event('N'),
            });
        } catch (error) {
            if (!error.stop) throw error;
            cause = error.stop;
        }
        assert.equal(cause, row[2], row[1]);
        assert.equal(events || '-', row[3], row[1]);
        assert.equal(`${fields.form},${fields.parent}`, row[4], row[1]);
        assert.equal(hex(pool.slot(5)), row[5], row[1]);
        count += 1;
    }
    assert.equal(count, 24);
});

// 지정 Form(수신) 분기: free 면 Take→TYPE/PID→0x80 부착, 살아 있으면 Take 없이 부모/PID 일치 진단만 하고 가상 통지한다. 불일치 진단은 정상 복귀 뒤 이어진다.
test('attachBaseProcess: 지정 Form 분기의 Take 와 부모/PID 진단', () => {
    const run = ({ freeState, parent, pid }) => {
        const pool = new SidPool(CAPACITY);
        // 부모(7)는 활성 상태여야 한다(새 풀의 슬롯은 모두 free).
        pool.physical(7)[11] = 0;
        const raw = pool.physical(5);
        raw[11] = freeState ? 1 : 8;
        put(raw, 16, parent, 2);
        put(raw, 18, pid, 4);
        raw[10] = 2;
        const events = [];
        const fields = { form: 0, parent: 0 };
        attachBaseProcess(pool, fields, 46, 7, 5, 0x50, {
            typeEnd: () => 63,
            processFormType: () => 2,
            assertion: (expression, file, line) => events.push(`D${expression}|${line}`),
            registerProcess: () => 9,
            create: () => assert.fail('지정 Form 이면 Create 하지 않는다'),
            take: (selected) => events.push(`take${selected}`),
            parent: () => parent,
            attach: (selected, target, flags) => events.push(`A${selected},${target},${flags}`),
            onAttached: () => events.push('N'),
        });
        return { events, type: raw[10], pid: get(raw, 18, 4), fields };
    };
    // free 슬롯: Take→TYPE/PID 쓰기→부착 뒤 getter(parent 인자가 7 이어야 일치).
    let result = run({ freeState: true, parent: 7, pid: 0 });
    assert.deepEqual(result.events, ['take5', 'A5,7,208', 'N']);
    assert.deepEqual([result.type, result.pid, result.fields.form, result.fields.parent], [46, 9, 5, 7]);
    // 살아 있는 슬롯: Take 없음, 부모 불일치와 PID 불일치 진단이 차례로 나온 뒤 통지한다.
    result = run({ freeState: false, parent: 3, pid: 4 });
    assert.deepEqual(result.events, ['Ds.getParentSid() == attachToSid|94', 'Ds.getPid() == pid|95', 'N']);
    // 일치하면 진단 없이 통지한다.
    result = run({ freeState: false, parent: 7, pid: 9 });
    assert.deepEqual(result.events, ['N']);
});

// 현재 생성자가 쓰는 내장 Form VT 의 실제 +74 를 선택하며 raw TYPE 로 고정하지 않는다(cpppj BaseProcessAttachCurrent_CurrentVtableSelectsNewLocalAttach).
test('SquidProcessHost: 현재 VT 가 신규 로컬 부착의 +74 를 고른다', () => {
    for (const kind of [1, 2, 3, 6]) {
        const parts = worldParts({ server: false });
        parts.state.processFormType = kind;
        const parent = parts.factory.create(74, 2);
        const process = parts.host.attach(new RegularProcess(0x123, 1.0), 46, parent, 0x50);
        assert.ok(process, `kind ${kind}`);
        const raw = parts.pool.slot(process.form);
        const table = { 1: SQUID_FORM_VTABLES.depend, 2: SQUID_FORM_VTABLES.process, 3: SQUID_FORM_VTABLES.gump, 6: SQUID_FORM_VTABLES.content }[kind];
        assert.deepEqual([get(raw, 0), raw[10], raw[11], get(raw, 16, 2), get(parts.pool.slot(parent), 6, 2), parts.pool.isServer], [table, 46, 8, parent, process.form, false], `kind ${kind}`);
        parts.kernel.clear();
    }
});

// 공통 Create 가 실패해도 앞선 Kernel/회전/프로세스 필드는 rollback 하지 않는다(cpppj BaseProcessAttachCurrent_CreateFailureKeepsRegistrationAndWrittenFields).
test('SquidProcessHost: Create 실패는 등록과 쓴 필드를 되돌리지 않는다', () => {
    const types = formTypeInputs();
    types[2].constructorAddress = 0xfffffff0;
    const parts = worldParts({ types, server: false });
    const parent = parts.factory.create(74, 2);
    const free = parts.pool.freeCount;
    assert.ok(throws(() => parts.host.attach(new RegularProcess(0x123, 1.0), 46, parent, 0x50)));
    const registered = parts.kernel.get(1);
    assert.ok(registered instanceof SquidProcess);
    assert.deepEqual([parts.kernel.size, registered.form, registered.parent, parts.host.isRegistered(1, registered), parts.host.owns(registered)], [1, 0, parent, true, true]);
    assert.deepEqual([parts.state.typeRotation, parts.pool.freeCount, get(parts.pool.slot(parent), 6, 2), parts.pool.isServer], [1, free - 1, 0, false]);
    parts.kernel.clear();
});

// 무효 프로세스 타입 진단은 정상 복귀 후 같은 입력으로 생성/현재 가상 부착을 계속한다(cpppj BaseProcessAttachCurrent_TypeDiagnosticResumesBeforeFieldWritesAndRegister).
test('SquidProcessHost: 타입 진단이 필드 쓰기와 등록 전에 정상 복귀한다', () => {
    const parts = worldParts({ server: false });
    let diagnostics = 0;
    parts.state.assertion = (expression, file, line) => {
        assert.deepEqual([expression, file, line, parts.kernel.size], ['isProcessType(_processType)', '\\Ns\\O\\Baseprocess.cpp', 0x1c, 0]);
        diagnostics += 1;
    };
    const parent = parts.factory.create(74, 2);
    const process = parts.host.attach(new RegularProcess(0x123, 1.0), 9, parent, 0x50);
    assert.ok(process);
    assert.deepEqual([diagnostics, parts.kernel.size], [1, 1]);
    const raw = parts.pool.slot(process.form);
    assert.deepEqual([raw[10], raw[11], get(raw, 16, 2), get(parts.pool.slot(parent), 6, 2), parts.pool.isServer], [9, 8, parent, process.form, false]);
    parts.kernel.clear();
});

// 갱신(RefreshForm)은 부모의 dead/free 두 진단이 정상 복귀하면 현재 Form 으로 해제/재부착을 계속한다.
test('SquidProcessHost: 갱신은 dead 와 free 부모를 따로 진단한다', () => {
    const parts = worldParts({ server: false });
    const parent = parts.factory.create(74, 2);
    const process = parts.host.addRegular(parent, 0x123, 1.0);
    const form = process.form;
    const parentRaw = parts.pool.allocated(parent);
    parentRaw[11] |= 3;
    const lines = [];
    parts.state.assertion = (expression, file, line) => {
        assert.equal(file, '\\Ns\\O\\Baseprocess.cpp');
        lines.push(line);
        if (line === 0xb0) {
            assert.equal(expression, '!parent.isDead()');
            parentRaw[11] &= ~2;
        } else {
            assert.deepEqual([line, expression], [0xb1, '!parent.isFree()']);
            parentRaw[11] &= ~1;
        }
    };
    parts.host.refreshForm(process);
    assert.deepEqual(lines, [0xb0, 0xb1]);
    assert.deepEqual([parts.pool.slot(form)[11], get(parentRaw, 6, 2), get(parts.pool.slot(form), 16, 2), parts.kernel.size, parts.pool.isServer], [8, form, parent, 1, false]);
    parts.kernel.clear();
});

// Unpop 이 free 부모에서 반환한 뒤 진단 장치가 체인을 고치면 실제 부착을 계속한다(cpppj ProcessAttachCurrent_ActualNonServerRefreshResumesAfterParentDiagnostic).
test('SquidProcessHost: 갱신은 free 부모 진단 뒤 이어서 부착한다', () => {
    const parts = worldParts({ server: false });
    const parent = parts.factory.create(74, 2);
    const freeParent = parts.factory.create(74, 2);
    const process = parts.host.addRegular(parent, 0x123, 1.0);
    const form = process.form;
    const raw = parts.pool.allocated(form);
    parts.pool.allocated(freeParent)[11] = 1;
    raw[11] |= 4;
    const diagnostics = [];
    parts.state.assertion = (expression, file, line) => {
        assert.equal(file, '\\Ns\\O\\Squid.cpp');
        diagnostics.push(line);
        if (line === 0xa5b) {
            assert.equal(expression, '!isVoid()');
            put(raw, 16, freeParent, 2);
        } else {
            assert.ok(line === 0xa77 && expression === 'popFlags & pfPROCESS_DEBUG' && (raw[11] & 4));
            put(parts.pool.allocated(parent), 6, 0, 2);
        }
    };
    parts.host.refreshForm(process);
    assert.deepEqual(diagnostics, [0xa5b, 0xa77]);
    assert.deepEqual([raw[11], get(raw, 16, 2), get(raw, 4, 2), get(parts.pool.slot(parent), 6, 2), parts.host.fromForm(form), parts.kernel.size, parts.pool.isServer], [8, parent, 0, form, process, 1, false]);
    parts.kernel.clear();
});

// 장기 호스트: 같은 Pool/Kernel/상태를 빌리고 현재 World 의 처리기 묶음만 호출 위치에서 조회한다. 묶음이 없으면 소비 위치에서 오류, activeCalls 는 호출 동안만 오른다.
test('SquidProcessHost.borrowing: 현재 World 처리기 묶음 대여와 최종 반납', () => {
    const types = formTypeInputs();
    const pool = new SidPool(CAPACITY, { server: false });
    const factory = new SquidFactory(pool, types);
    const destroy = new RawSquidDestroy(pool, new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536)), types);
    const kernel = new Kernel();
    const state = new SquidProcessState();
    const aux = new GameAuxState();
    state.bindGameAux(aux);
    let runtime = null;
    const calls = [];
    let active = -1;
    const host = SquidProcessHost.borrowing(pool, types, kernel, state, () => runtime);
    const parent = factory.create(74, 2);
    // 방금 만든 부모는 void 상태이므로 Regular 가 실행되도록 활성 상태·비추상 장르로 둔다.
    pool.allocated(parent)[11] = 0;
    pool.allocated(parent)[40] = 0;
    // 묶음이 없으면 소비 위치에서 거부한다. 거부 전에 등록된 프로세스는 되돌리지 않는다(Create 앞 Kernel 등록).
    assert.throws(() => host.addRegular(parent, 1, 1.0), /현재 World/);
    assert.equal(kernel.size, 1);
    kernel.clear();
    state.now = 5;
    runtime = createProcessHostRuntime({
        destroy,
        factory,
        regular: (...args) => {
            active = runtime.activeCalls;
            calls.push(args);
            return 0;
        },
        asset: { emit: () => {}, selected: () => 0 },
    });
    // 일반/공유 TYPE 은 GameAuxState 의 현재 값을 읽는다.
    aux.regularProcessType = 47;
    const process = host.addRegular(parent, 0x2692, 1.5);
    assert.ok(process && pool.slot(process.form)[10] === 47);
    assert.equal(runtime.activeCalls, 0);
    kernel.runFrame();
    assert.deepEqual(calls, [[parent, 0x2692, 0, 1.5]]);
    assert.equal(active, 1, '호출 중에는 대여 수가 오른다');
    assert.equal(kernel.size, 0, '처리기가 0 을 반환하면 Kill 된다');
    // 최종 반납은 자기 현재 대여가 만든 프로세스만 정리한다.
    const other = host.addRegular(parent, 1, 1.0);
    assert.ok(other);
    const older = createProcessHostRuntime({ destroy, factory });
    host.drainRuntimeBindings(older);
    assert.equal(kernel.size, 1, '다른 대여의 프로세스는 남는다');
    host.drainRuntimeBindings(runtime);
    assert.equal(kernel.size, 0);
    assert.equal(host.hasOnlyRegularProcesses(), true);
});
