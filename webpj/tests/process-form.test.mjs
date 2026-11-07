// 지정 ProcessForm 수신(Take)과 재수신(Reattach)·Kernel 중복 포인터·삭제 순서를 원본 10.78 관측과 대조한다(WB04-D5c).
//  - processform-x86.tsv 의 Attach 행(10.78 288행): 지정 free 서버 Form 의 Take·타입/PID 쓰기·부모 연결·가상 통지와 Form/부모/머리 슬롯 전체, 풀 메타(FIFO·예측 커서)가 불변인지.
//    같은 파일의 단일 즉시 패킷 분배 행(Dispatch)은 패킷 분배(RawPacketDispatch) 단위에서 옮긴다.
//  - processretake-x86.tsv(10.78 194행): live 무수정·free Take·Kernel 중복 포인터 재등록·삭제 PID 재조회/소멸자 순서·Clear 중 재등록/제거의 슬롯 생존.
//  - 계약: 잘못된 지정 form 은 Kernel/목록/부모를 바꾸기 전에 거부, live 지정 수신의 부모/PID 불일치 거부, 패치판은 부모 상태를 확인하기 전에 두 SID 를 기록하고 free/dead 부모에서는 통지를 생략,
//    호스트 종료 정리는 소멸 재진입 때도 슬롯 수명을 지킨다.
// 입력 payload 는 cpppj ProcessFormTests·ProcessRetakeTests 와 같은 합성 바이트이며 기대값은 fixture 에서 읽는다. CD 10.72·10.37 행은 범위 밖이다.
// (2026-11-08 WB04-D5c: Process 호스트 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { Kernel } from '../www/core/kernel.js';
import { RegularProcess, SquidProcess, SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';

// 원본 관측 파일 행 읽기이다(10.78 행만). 파일이 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-D5c)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const patchRows = (name) => readRows(path.join(FIXTURES, name))?.filter((row) => row[0] === 'originals') ?? null;
const formRows = patchRows('processform-x86.tsv')?.filter((row) => row[1] === 'Attach') ?? null;
const retakeRows = patchRows('processretake-x86.tsv');

// 실행기가 쓰는 풀 크기이다. (2026-11-08 WB04-D5c)
const CAPACITY = 32768;
// 바이트 배열의 소문자 16진 문자열이다. (2026-11-08 WB04-D5c)
const hex = (bytes) => Buffer.from(bytes).toString('hex');
// 쉼표로 나눈 입력 수 목록이다. (2026-11-08 WB04-D5c)
const inputs = (text) => text.split(',').map(Number);
// 오류가 나는지 확인한다(성공을 무조건 인정하지 않음). (2026-11-08 WB04-D5c)
function throws(action) {
    try {
        action();
        return false;
    } catch {
        return true;
    }
}

// 내장 Form 네 생성자 주소만 넣은 188칸 TYPE 입력이다(cpppj MakeFormTypeInputs). (2026-11-08 WB04-D5c)
const formTypeInputs = () => Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));

// Take 가 유지해야 하는 카운터·예측 커서·client/server 머리·꼬리를 한 문자열로 만든다. (2026-11-08 WB04-D5c)
function meta(pool) {
    return `${pool.freeCount}:${pool.predictableCursor}:${get(pool.slot(0), 4, 2)}:${pool.tail(true)}:${get(pool.slot(0), 18, 2)}:${pool.tail(false)}`;
}

// 빈 실행/부착 통지만 제공하는 프로세스이다. 실제 Kernel 등록과 form 연결은 host 가 수행한다. (2026-11-08 WB04-D5c)
class TaggedProcess extends SquidProcess {
    constructor(events) {
        super();
        this.events = events;
    }

    runFrame() {}

    // form/부모가 확정된 뒤의 원본 가상 통지이다.
    onAttached() { this.events.push(`N:${this.form}:${this.parent}`); }
}

// 실제 Kernel/풀/삭제 모듈을 소유한 named form 장면이다. 부모(20000)는 TYPE 100 의 물리 입력이다. (2026-11-08 WB04-D5c)
class FormScene {
    constructor() {
        const types = formTypeInputs();
        this.pool = new SidPool(CAPACITY, { server: false });
        this.destroy = new RawSquidDestroy(this.pool, new SquidUnpop(this.pool, new SquidHash(), new Uint8Array(65536)), types);
        this.kernel = new Kernel();
        this.state = new SquidProcessState();
        this.host = new SquidProcessHost(this.pool, types, this.kernel, this.destroy, this.state);
        this.events = [];
        const parent = this.pool.physical(20000);
        parent.fill(0);
        parent[10] = 100;
    }
}

// 지정 free form 수신: 저장된 원본 관찰의 이벤트·세 슬롯 전체·필드·풀 메타를 비교한다.
test('SquidProcessHost.attach: 원본10.78 지정 free ProcessForm 수신 288행', { skip: !formRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of formRows) {
        assert.equal(row.length, 9);
        const input = inputs(row[2]);
        assert.equal(input.length, 5);
        const scene = new FormScene();
        const form = input[0];
        const raw = scene.pool.physical(form);
        const parent = scene.pool.physical(20000);
        const head = scene.pool.physical(50);
        // 초기 raw 표식만 공급하고 기대 출력은 계산하지 않는다.
        for (const bytes of [raw, parent, head]) for (let index = 0; index < bytes.length; index += 1) bytes[index] = (index * 29 + 7) & 0xff;
        raw[11] = input[1];
        parent[10] = 100;
        parent[11] = 0;
        put(parent, 6, input[2] ? 50 : 0, 2);
        put(head, 0, input[2] === 1 ? 0x512098 : 0x501dd0);
        head[10] = input[2] === 1 ? 46 : 100;
        head[11] = 8;
        // 경계가 돌려준 번호 7 을 실제 Kernel 의 첫 여섯 슬롯을 채워 재현한다.
        for (let index = 0; index < 6; index += 1) assert.equal(scene.kernel.add(new RegularProcess(1, 0)), index + 1);
        const before = meta(scene.pool);
        const process = scene.host.attach(new TaggedProcess(scene.events), input[4], 20000, input[3], form);
        assert.ok(process);
        assert.deepEqual([scene.kernel.find(process), get(raw, 18), scene.kernel.size, scene.state.typeRotation], [7, 7, 7, 1], `행 ${count}`);
        const events = scene.events.length ? scene.events.join(';') : '-';
        assert.deepEqual(
            [events, hex(raw), hex(parent), hex(head), `${process.form}:${process.parent}`, meta(scene.pool), meta(scene.pool)],
            [row[3], row[4], row[5], row[6], row[7], row[8], before],
            `행 ${count}: ${row[2]}`,
        );
        count += 1;
    }
    assert.equal(count, 288);
});

// 잘못된 지정 form: client/예약 번호의 Take 는 원문 진단 장치가 없으면 그 위치에서 거부되고, 물리 범위 밖은 범위 오류이다.
// 기준선 소스(AttachBaseProcess)에는 사전 검증이 없어 **Kernel 등록과 타입 순환 전진은 이미 일어난 뒤** 거부된다(부분 실패를 되돌리지 않음).
// cpppj 옛 검사 ProcessForm_RejectsInvalidNamedFormsBeforeRegistration 은 "등록 전 거부"를 기대하나 소스 읽기로는 그렇지 않아(20261105-42 이후) 웹은 소스를 따른다 — cpppj 를 실행해 확인하지는 않았다.
test('SquidProcessHost.attach: 잘못된 지정 form 은 소비 위치에서 거부되고 부분 효과가 남는다', () => {
    for (const [value, message] of [[5, /assertion/], [CAPACITY, /범위/]]) {
        const scene = new FormScene();
        const parent = hex(scene.pool.slot(20000));
        assert.throws(() => scene.host.attach(new TaggedProcess(scene.events), 30, 20000, 0x10, value), message, `form ${value}`);
        // 등록·회전은 이미 진행됐고 부모 슬롯과 사건은 바뀌지 않았다.
        assert.deepEqual([scene.kernel.size, scene.state.typeRotation, scene.events.length, hex(scene.pool.slot(20000))], [1, 1, 0, parent], `form ${value}`);
    }
});

// 프로세스 통지/소멸자만 기록하며 실제 풀·Kernel·삭제 동작은 복원 모듈을 쓰는 재수신 입력 프로세스이다. (2026-11-08 WB04-D5c)
class RetakeProcess extends SquidProcess {
    constructor(kernel, events, index) {
        super();
        this.kernel = kernel;
        this.events = events;
        this.index = index;
        // 검사 입력에 지정한 실제 Kernel 번호, 다른 소멸자에서 앞 슬롯이 비었는지 조회하는 번호, 삭제 통지 중 PID 를 바꾸는 입력 경계, Clear 중 실제 Kernel 등록/제거를 요청하는 입력 경계이다.
        this.slot = 0;
        this.watched = 0;
        this.redirect = null;
        this.duringDestructor = null;
    }

    runFrame() {}

    // 기존 Form/부모 필드를 원본 가상 +0x14 시점에 기록한다.
    onAttached() { this.events.push(`N:${this.form}:${this.parent}`); }

    // 원본 통지가 현재 Form PID 를 바꾸는 입력을 받아 다음 Kernel 제거의 재조회 여부를 검사한다.
    onFormDestroy(flags) {
        this.events.push(`P:${this.index}:${flags}`);
        if (this.redirect) this.redirect();
    }

    // 원본 scalar deleting destructor 의 인자 1과 소멸자 안의 슬롯 존재를 관찰한다(Kernel 이 제거 때 부르는 destroy).
    destroy() {
        if (this.watched) this.events.push(`G:${this.index}:${this.kernel.get(this.watched) ? 1 : 0}`);
        this.events.push(`D:${this.index}:${this.kernel.get(this.slot) === this ? 1 : 0}:1`);
        if (this.duringDestructor) this.duringDestructor();
    }
}

// 실제 풀/Kernel/Form host 를 소유하며 fixture 입력으로 raw 슬롯만 공급하는 재수신 장면이다. (2026-11-08 WB04-D5c)
class RetakeScene {
    constructor() {
        const types = formTypeInputs();
        this.pool = new SidPool(CAPACITY, { server: false });
        this.destroy = new RawSquidDestroy(this.pool, new SquidUnpop(this.pool, new SquidHash(), new Uint8Array(65536)), types);
        this.state = new SquidProcessState();
        this.events = [];
        this.kernel = new Kernel();
        this.host = new SquidProcessHost(this.pool, types, this.kernel, this.destroy, this.state);
    }

    // 원하는 뒤 슬롯에 객체를 등록하고 앞 임시 객체를 제거해 원본 합성 포인터 배치를 만든다.
    stage(slot, index) {
        const temporary = [];
        // 앞의 빈 슬롯은 잠시 채웠다가 대상 등록 뒤 제거한다.
        while (this.kernel.size < slot - 1) {
            const entry = new RetakeProcess(this.kernel, this.events, 99);
            entry.slot = this.kernel.add(entry);
            temporary.push(entry.slot);
        }
        const result = new RetakeProcess(this.kernel, this.events, index);
        result.slot = this.kernel.add(result);
        assert.equal(result.slot, slot);
        // 이미 있던 대상 슬롯은 남기고 이번에 넣은 임시 객체만 제거한다.
        for (const id of temporary) this.kernel.remove(id);
        return result;
    }
}

// 기대값과 무관한 합성 raw 입력을 원본 실행기와 같은 바이트로 공급한다. (2026-11-08 WB04-D5c)
function retakeRaw(scene, operation, input) {
    const form = input[1];
    const raw = scene.pool.physical(form);
    const parent = scene.pool.physical(20000);
    const head = scene.pool.physical(50);
    // 세 슬롯의 입력 payload 표식이다. 실행 출력은 fixture 만 기대값으로 사용한다.
    for (const bytes of [raw, parent, head]) for (let index = 0; index < bytes.length; index += 1) bytes[index] = (index * 29 + 7) & 0xff;
    parent[10] = 100;
    parent[11] = 0;
    put(parent, 6, operation === 'Free' ? (input[3] ? 50 : 0) : form, 2);
    put(raw, 0, 0x512098);
    raw[10] = 46;
    raw[11] = input[2];
    put(raw, 4, 50, 2);
    put(raw, 6, 0, 2);
    put(raw, 14, 0, 2);
    put(raw, 16, 20000, 2);
    put(raw, 18, input[0]);
    put(head, 0, input[3] === 1 ? 0x512098 : 0x501dd0);
    head[10] = input[3] === 1 ? 46 : 100;
    head[11] = 8;
    put(head, 4, 0, 2);
    put(head, 14, form, 2);
}

// 저장된 독립 실제 PE 입력을 복원 모듈에 적용한다.
test('SquidProcessHost.reattach: 원본10.78 지정 ProcessForm 재수신·Kernel 중복 포인터·삭제 순서 194행', { skip: !retakeRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of retakeRows) {
        assert.equal(row.length, 13);
        const input = inputs(row[2]);
        assert.equal(input.length, 8);
        const scene = new RetakeScene();
        const form = input[1];
        const source = scene.stage(input[0], 0);
        const raw = scene.pool.physical(form);
        const parent = scene.pool.physical(20000);
        parent[10] = 100;
        parent[11] = 0;
        put(parent, 6, 0, 2);
        raw[11] = 1;
        // 소유 객체의 C++ 필드만 정상 부착으로 준비한다. 이후 raw/회전/사건은 입력 상태로 되돌린다.
        if (['Register', 'Delete', 'Clear'].includes(row[1])) assert.equal(scene.host.reattach(source, 46, 20000, 0x50, form), source);
        const later = (input[7] || row[1] === 'Clear') ? scene.stage(9, 1) : null;
        retakeRaw(scene, row[1], input);
        scene.state.typeRotation = input[6];
        scene.events.length = 0;
        if (row[1] === 'Delete' && input[7]) source.redirect = () => put(scene.pool.physical(form), 18, 9);
        if (row[1] === 'Clear') {
            later.watched = input[0];
            // 원본 소멸자 경계 안에서도 등록/제거 자체는 실제 Kernel 메서드를 호출한다.
            if (input[7] === 1 || input[7] === 2) {
                const registerNext = () => {
                    const process = new RetakeProcess(scene.kernel, scene.events, 2);
                    process.slot = scene.kernel.add(process);
                    scene.events.push(`A:2:${process.slot}`);
                };
                if (input[7] === 1) later.duringDestructor = registerNext;
                else source.duringDestructor = registerNext;
            } else if (input[7] === 3) {
                source.duringDestructor = () => {
                    scene.kernel.remove(9);
                    scene.events.push('R:9');
                };
            }
        }
        let result = 0;
        let fields = `${source.form}:${source.parent}`;
        if (row[1] === 'Live' || row[1] === 'Free') assert.equal(scene.host.reattach(source, input[4], 20000, input[5], form), source);
        else if (row[1] === 'Register') result = scene.kernel.add(source);
        else if (row[1] === 'Clear') scene.kernel.clear();
        else scene.destroy.destroy(form, input[5], scene.host.hooks());
        // Clear 중 새 객체가 해제 주소를 재사용할 수 있으므로 종료 객체의 필드는 살아 있을 때 저장한 값만 쓴다.
        if (row[1] !== 'Clear' && scene.kernel.get(input[0]) === source) fields = `${source.form}:${source.parent}`;
        const slots = [];
        // 두 실제 PE 의 관찰 범위와 같은 앞 64개 슬롯만 직접 읽는다.
        for (let id = 1; id < 65; id += 1) {
            const process = scene.kernel.get(id);
            if (process instanceof RetakeProcess) slots.push(`${id}:${process.index}`);
        }
        const label = `${row[1]} ${row[2]} (행 ${count})`;
        assert.equal(scene.events.length ? scene.events.join(';') : '-', row[3], label);
        assert.equal(hex(scene.pool.slot(form)), row[4], `${label} 슬롯`);
        assert.equal(hex(scene.pool.slot(20000)), row[5], `${label} 부모`);
        assert.equal(hex(scene.pool.slot(50)), row[6], `${label} 머리`);
        assert.equal(fields, row[7], label);
        assert.equal(slots.length ? slots.join(';') : '-', row[8], `${label} Kernel 슬롯`);
        if (!(row[1] === 'Register' || (row[1] === 'Clear' && (input[7] === 1 || input[7] === 2)))) assert.equal(String(scene.state.typeRotation), row[9], `${label} 회전`);
        assert.equal(meta(scene.pool), row[10], `${label} 메타`);
        assert.equal(`${scene.destroy.preDepth},${scene.destroy.postDepth},0`, row[11], `${label} 깊이`);
        assert.equal(String(result), row[12], `${label} 반환`);
        count += 1;
    }
    assert.equal(count, 194);
});

// live 지정 수신의 신규 소유권/다른 부모·다른 PID 는 원본 assert 의 안전한 입력 경계이다: 진단 장치가 없으면 거부하고 슬롯/사건을 바꾸지 않는다.
test('SquidProcessHost.reattach: live 소유권 불일치 입력을 거부한다', () => {
    const scene = new RetakeScene();
    const process = scene.stage(7, 0);
    const form = 15000;
    const parent = scene.pool.physical(20000);
    parent[10] = 100;
    parent[11] = 0;
    put(parent, 6, 0, 2);
    assert.equal(scene.host.reattach(process, 46, 20000, 0x50, form), process);
    const before = hex(scene.pool.slot(form));
    scene.events.length = 0;
    put(scene.pool.physical(form), 18, 1);
    assert.ok(throws(() => scene.host.reattach(process, 30, 20000, 0x50, form)));
    put(scene.pool.physical(form), 18, 7);
    const second = scene.pool.physical(20001);
    second[10] = 100;
    second[11] = 0;
    assert.ok(throws(() => scene.host.reattach(process, 30, 20001, 0x50, form)));
    assert.deepEqual([hex(scene.pool.slot(form)), scene.kernel.size, scene.events.length], [before, 1, 0]);
    // Kernel 이 소유하지 않은 프로세스는 reattach 할 수 없다.
    assert.ok(throws(() => scene.host.reattach(new RegularProcess(1, 0), 46, 20000, 0x50, form)));
});

// 패치판은 부모 상태를 확인하기 전에 두 SID 를 기록하고 free/dead 부모에서는 통지를 생략한다.
test('SquidProcessHost.reattach: 패치판은 비활성 부모 앞에 두 SID 를 먼저 기록한다', () => {
    const scene = new RetakeScene();
    const process = scene.stage(7, 0);
    const before = meta(scene.pool);
    // free 와 dead 의 원본 조기 반환은 Kernel 등록/회전/Form 쓰기를 수행하지 않는다.
    for (const state of [1, 2]) {
        const form = 15000;
        const parent = 20000;
        scene.pool.physical(parent)[11] = state;
        scene.events.length = 0;
        const raw = hex(scene.pool.slot(form));
        assert.equal(scene.host.reattach(process, 46, parent, 0x50, form), null);
        assert.deepEqual([process.form, process.parent, scene.state.typeRotation, scene.events.length], [form, parent, 0, 0]);
        assert.deepEqual([hex(scene.pool.slot(form)), scene.kernel.size, meta(scene.pool)], [raw, 1, before]);
    }
});

// 호스트 종료 정리는 두 배열이 살아 있는 동안 수행하고 원본 Clear 에서 남는 앞 신규 슬롯도 후속 정리한다.
test('Kernel.drainOwnedForShutdown: 소멸 재진입 때도 슬롯 수명을 지킨다', () => {
    const events = [];
    const kernel = new Kernel(8);
    const first = new RetakeProcess(kernel, events, 0);
    first.slot = kernel.add(first);
    const second = new RetakeProcess(kernel, events, 1);
    second.slot = kernel.add(second);
    second.watched = 1;
    second.duringDestructor = () => {
        const next = new RetakeProcess(kernel, events, 2);
        next.slot = kernel.add(next);
        events.push(`A:2:${next.slot}`);
    };
    kernel.dispose();
    assert.equal(events.join(';'), 'D:0:1:1;G:1:0;D:1:1:1;A:2:1;D:2:1:1');
});
