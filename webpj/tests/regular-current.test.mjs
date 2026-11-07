// Process 호스트의 현재 Regular 번호·프로세스 finder·공유 Regular 생성 진단을 원본 10.78 관측과 대조한다(WB04-D5c, cpppj RegularCurrentTests 4장면의 이식).
//  - regular-current-find-x86.tsv 1232행: 두 finder(FindEvent·FindEventMasked)가 현재 일반/공유 Regular TYPE 전체 DWORD 와 signed 프로세스 상한, 자산 선행 필터를 어떻게 적용하는지.
//  - regular-current-constructor-x86.tsv: 부착 장치가 시각을 바꾸지 않는 32행의 실제 부착/후단 필드(타입·부모·이벤트·payload 비트·횟수·시각)와 공유 생성 진단(basePostPop == 0).
//  - 공유 진단이 등록 전에 멈추는 부분 효과와 진단 장치가 스스로 연결을 풀어도 callable 이 살아 있음, 없는 PID 를 빈 검색으로 숨기지 않음.
// 실제 Kernel/풀/호스트만 쓰고 게임/전투 프레임은 실행하지 않는다. (2026-11-08 WB04-D5c: Process 호스트 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';

// 원본 관측 파일 행 읽기이다(판본 열이 없는 10.78 전용 표). 파일이 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-D5c)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const findRows = readRows(path.join(FIXTURES, 'regular-current-find-x86.tsv'));
const constructorRows = readRows(path.join(FIXTURES, 'regular-current-constructor-x86.tsv'));

// 부모 SID 이다(cpppj kSource). (2026-11-08 WB04-D5c)
const SOURCE = 50;

// 내장 Form 네 생성자 주소만 넣은 188칸 TYPE 입력이다(cpppj MakeFormTypeInputs). (2026-11-08 WB04-D5c)
const formTypeInputs = () => Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));

// 오류가 나는지 확인한다(성공을 무조건 인정하지 않음). (2026-11-08 WB04-D5c)
function throws(action) {
    try {
        action();
        return false;
    } catch {
        return true;
    }
}

// 실제 Kernel/풀/호스트 장면이다: 로컬 Form 번호를 실제 client 영역에서 할당하고(서버 0, 24000 슬롯), 부모는 TYPE 74 의 물리 입력만 만든다. (2026-11-08 WB04-D5c)
class RegularCurrentScene {
    constructor() {
        const types = formTypeInputs();
        this.pool = new SidPool(24000, { server: false });
        this.destroy = new RawSquidDestroy(this.pool, new SquidUnpop(this.pool, new SquidHash(), new Uint8Array(65536)), types);
        this.kernel = new Kernel(16);
        this.state = new SquidProcessState();
        this.host = new SquidProcessHost(this.pool, types, this.kernel, this.destroy, this.state);
        const raw = this.raw(SOURCE);
        raw.fill(0);
        raw[10] = 74;
    }

    // free 상태와 무관하게 원문의 같은 슬롯을 읽고 쓰는 물리 슬롯이다.
    raw(sid) {
        return this.pool.physical(sid);
    }
}

// 두 실제 finder 의 현재 번호/상한/자산 선행 필터를 1232행 재생한다.
test('SquidProcessHost.findEvent: 원본10.78 finder 1232행(전체 DWORD 번호와 signed 상한)', { skip: !findRows && '원본 fixture 없음' }, () => {
    const scene = new RegularCurrentScene();
    const events = [0x5101, 0x5102, 0x5203];
    const processes = [null, null, null];
    // 머리 삽입이므로 역순으로 만들어 Native1→2→3 과 같은 순서를 얻는다.
    for (let index = 2; index >= 0; index -= 1) processes[index] = scene.host.addRegular(SOURCE, events[index], Math.fround(0.05));
    let count = 0;
    for (const row of findRows) {
        assert.equal(row.length, 9);
        scene.state.regularType = Number(row[1]);
        scene.state.sharedRegularType = Number(row[2]);
        scene.state.processTypeEnd = Number(row[3]);
        // 실제 연결/PID 는 보존하고 TYPE BYTE 만 교차한다.
        for (let index = 0; index < processes.length; index += 1) scene.raw(processes[index].form)[10] = Number(row[4 + index]) & 0xff;
        const found = Number(row[0]) ? scene.host.findEventMasked(SOURCE, 0xfff0, 0x5100) : scene.host.findEvent(SOURCE, 0x5102);
        const expected = Number(row[7]);
        assert.equal(found, expected ? processes[expected - 1] : null, `행 ${count}: ${row.join(' ')}`);
        count += 1;
    }
    // 실제 Host/Kernel 반납 전에 검사 입력을 원래 분류로 돌린다.
    scene.state.regularType = 46;
    scene.state.sharedRegularType = 61;
    scene.state.processTypeEnd = 63;
    for (const process of processes) scene.raw(process.form)[10] = 46;
    assert.equal(count, 1232);
});

// 부착 장치 시각 변경이 없는 32행의 실제 C++ 부착/후단 필드를 비교한다(외부 장치가 시각을 바꾸는 32행은 별도 관찰로 유지).
test('SquidProcessHost: 원본10.78 Regular 생성자 부착과 공유 진단 복귀 32행', { skip: !constructorRows && '원본 fixture 없음' }, () => {
    let count = 0;
    let externalTime = 0;
    for (const row of constructorRows) {
        assert.equal(row.length, 10);
        if (Number(row[4])) {
            externalTime += 1;
            continue;
        }
        const scene = new RegularCurrentScene();
        scene.state.now = 17.5;
        scene.state.regularType = Number(row[1]);
        scene.state.sharedRegularType = Number(row[1]);
        scene.state.postPopDepth = Number(row[2]);
        let diagnostics = 0;
        scene.state.assertion = (expression, file, line) => {
            assert.deepEqual([expression, file, line], ['basePostPop == 0', '\\Ns\\O\\Regular.cpp', 0x70]);
            diagnostics += 1;
            // 진단이 정상 복귀한 뒤의 번호 선택을 확인한다.
            if (Number(row[3])) scene.state.sharedRegularType = 48;
        };
        const process = Number(row[0]) ? scene.host.addSharedRegular(SOURCE, 0x5102, Math.fround(0.05)) : scene.host.addRegular(SOURCE, 0x5102, Math.fround(0.05));
        assert.ok(process, row.join(' '));
        const calls = row[5].split('|');
        const attach = calls.at(-1).split(':');
        assert.equal(attach.length, 5);
        assert.deepEqual(
            [scene.raw(process.form)[10], process.parent, process.event, floatToBits(process.payload), process.count, process.time],
            [Number(attach[1]), SOURCE, Number(row[6]), Number(row[7]), Number(row[8]), Number(row[9])],
            row.join(' '),
        );
        assert.equal(diagnostics, Number(row[0]) && Number(row[2]) ? 1 : 0);
        assert.equal(scene.kernel.size, 1);
        count += 1;
    }
    assert.deepEqual([count, externalTime], [32, 32]);
});

// 공유 진단이 실패하면 Form/Kernel 을 만들기 전에 멈추고(부분 효과 없음), 진단 장치가 스스로 연결을 풀어도 선택된 callable 은 끝까지 살아 있으며 진단은 깊이를 임의로 보정하지 않는다.
test('SquidProcessHost: 공유 진단은 등록 전에 멈추고 스스로 연결을 풀어도 이어진다', () => {
    const scene = new RegularCurrentScene();
    scene.state.postPopDepth = 1;
    const free = scene.pool.freeCount;
    // 진단 장치가 없으면 원문 진단 이전에 Form/Kernel 을 만들지 않는다.
    assert.ok(throws(() => scene.host.addSharedRegular(SOURCE, 1, Math.fround(0.05))));
    assert.deepEqual([scene.kernel.size, scene.pool.freeCount], [0, free]);
    scene.state.assertion = () => {
        scene.state.assertion = null;
        scene.state.sharedRegularType = 47;
        scene.state.now = 33.25;
    };
    const process = scene.host.addSharedRegular(SOURCE, 1, Math.fround(0.05));
    assert.ok(process);
    assert.deepEqual([scene.raw(process.form)[10], process.time, scene.state.postPopDepth], [47, 33.25, 1]);
});

// 머리 후보의 PID 가 Kernel 에 없거나 Regular 가 아니면 finder 는 뒤의 정상 예약으로 넘어가지 않고 명시 오류이다(Native 4afbb1 의 null+18 읽기를 빈 검색으로 대체하지 않는다).
// 원문 분류에서 선택되지 않은 TYPE 은 PID 를 읽지 않는다.
test('SquidProcessHost: 없는 PID 를 뒤의 정상 예약 앞에서 조용히 건너뛰지 않는다', () => {
    const scene = new RegularCurrentScene();
    const later = scene.host.addRegular(SOURCE, 0x5102, Math.fround(0.05));
    const first = scene.host.addRegular(SOURCE, 0x5101, Math.fround(0.05));
    const raw = scene.raw(first.form);
    const saved = get(raw, 18);
    // 머리 후보의 PID 만 바꾸고 뒤의 정상 예약은 보존한다.
    for (const pid of [0, 16, 0xffffffff]) {
        put(raw, 18, pid);
        assert.ok(throws(() => scene.host.findEvent(SOURCE, 0x5102)), `pid ${pid}`);
        assert.deepEqual([scene.kernel.size, get(raw, 18)], [2, pid]);
        // 원문 분류에서 선택되지 않은 TYPE 는 PID 를 읽지 않는다.
        scene.state.regularType = 302;
        assert.equal(scene.host.findEvent(SOURCE, 0x5102), null);
        scene.state.regularType = 46;
    }
    // 같은 실제 슬롯/Kernel 을 복원한 뒤에는 원래 정상 검색을 유지한다.
    put(raw, 18, saved);
    assert.equal(scene.host.findEvent(SOURCE, 0x5102), later);
});
