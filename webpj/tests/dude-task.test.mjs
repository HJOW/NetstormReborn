// 일반 Dude task 설정 raw-dude-task.js(RawDudeTask·makeDudeTaskHostHooks)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: dudetask-x86.tsv 10.78(originals) 384행 — 이전 task WORD·상태 바이트·TYPE(70 Dude/82 다리)·기존 Regular 유무·float 지연 비트·현재 double 시각 비트 → 반환/원본 assert 분류,
//    조회·생성·부착·표면 통지 호출 순서, 슬롯 50바이트, 최종 Regular(시각 비트,event,payload 비트,count). CD 10.72 행(396)은 웹 범위(10.78) 밖이다.
//  - 계약: DudeTask_MissingCreationAndSurfaceRejected(미연결 Regular 생성과 표면 효과를 호출 시점에서 거부), 판본/풀 혼합 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E76: 일반 Dude task 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { RawDudeTask, DudeTaskFallAssertError, makeDudeTaskHostHooks } from '../www/core/raw-dude-task.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E76)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'dudetask-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 원본 관찰 SID 와 판본당 독립 행 수이다. (2026-11-08 WB04-E76)
const SOURCE = 50;
const ROWS = 384;

// 예외가 나는지 본다. (2026-11-08 WB04-E76)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E76)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// double 의 64비트 패턴을 10진 문자열로 읽고 쓰는 공용 DataView 이다. (2026-11-08 WB04-E76)
const view = new DataView(new ArrayBuffer(8));
const doubleToBits = (value) => { view.setFloat64(0, value, true); return view.getBigUint64(0, true).toString(); };
const bitsToDouble = (text) => { view.setBigUint64(0, BigInt(text), true); return view.getFloat64(0, true); };

// 내장 Form 네 생성자 주소만 넣은 188칸 TYPE 입력이다(cpppj MakeFormTypeInputs). (2026-11-08 WB04-E76)
const formTypeInputs = () => Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));

// 같은 풀/Host/시계와 현재 Regular 객체를 실제로 소유하는 task 장면이다. (2026-11-08 WB04-E76)
class TaskScene {
    // 실제 client 할당으로 원본 Regular 생성 번호 83 직전까지 확보하고 입력 한 행을 복구한다. 기대값은 계산하지 않는다.
    constructor(row) {
        assert.equal(row.length, 13);
        this.row = row;
        this.pool = new SidPool(24000);
        const hash = new SquidHash();
        const spots = new Uint8Array(65536);
        this.types = formTypeInputs();
        const unpop = new SquidUnpop(this.pool, hash, spots);
        const destroy = new RawSquidDestroy(this.pool, unpop, this.types);
        this.kernel = new Kernel(16);
        this.clock = new SquidProcessState();
        this.host = new SquidProcessHost(this.pool, this.types, this.kernel, destroy, this.clock, { regular: () => 1 });
        this.current = null;
        this.events = [];
        // 원본 입력번호와 같은 일반 client 슬롯을 실제 확보한다.
        for (let id = 5; id <= 82; id += 1) assert.equal(this.pool.allocate(2), id);
        const raw = this.pool.allocated(SOURCE);
        raw.fill(0);
        raw[10] = Number(row[6]);
        put(raw, 12, Number(row[3]), 2);
        put(raw, 14, floatToBits(20.75));
        put(raw, 18, floatToBits(21.9));
        if (Number(row[5])) {
            this.clock.now = 20;
            this.current = this.host.addRegular(SOURCE, 0x5104, 1, 0x50);
            assert.ok(this.current && this.current.form === 83);
            // 기존 count7/time27 은 원본 기대값 대입 없이 실제 runFrame 일곱 번으로 준비한다.
            for (let count = 0; count < 7; count += 1) {
                this.clock.now = this.current.time;
                this.current.runFrame();
            }
            assert.ok(this.current.count === 7 && this.current.time === 27 && this.current.payload === 1);
        }
        this.pool.allocated(SOURCE)[11] = Number(row[2]);
        this.clock.now = bitsToDouble(row[8]);
    }

    // 실제 Host 의 조회/생성을 쓰고 native 의 외부 효과 관찰 순서를 함께 기록한다.
    hooks() {
        const base = makeDudeTaskHostHooks(this.pool, this.host, {});
        return {
            taskRegular: (sid) => { this.events.push('R:50:65520:20736'); return base.taskRegular(sid); },
            addTaskRegular: (sid, event) => {
                this.events.push('N:40');
                this.current = base.addTaskRegular(sid, event);
                this.events.push('B:46:50:0:80');
                return this.current;
            },
            notifySurface: (sid) => { this.events.push(`S:${sid}`); }
        };
    }

    // 원본 결과칸을 읽지 않고 현재 입력으로 실행한 raw/Regular 상태를 직렬화한다.
    run() {
        const task = new RawDudeTask(this.pool, { clock: this.clock }, this.hooks());
        let status = 'return';
        try {
            task.set(Number(this.row[1]), Number(this.row[4]), bitsToFloat(Number(this.row[7])));
        } catch (error) {
            if (!(error instanceof DudeTaskFallAssertError)) throw error;
            this.events.push('A:156');
            status = 'assert';
        }
        const regular = this.current ? `${doubleToBits(this.current.time)},${this.current.event},${floatToBits(this.current.payload)},${this.current.count}` : '-';
        return [status, this.events.join(';'), hex(this.pool.slot(SOURCE)), regular];
    }
}

test('DudeTask: 원본 10.78 384입력의 task DWORD/WORD·낙하 교체 진단·정밀 double 지연이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 원본 입력과 마지막 네 출력칸을 분리해 전수 대조한다.
    for (const row of rows) {
        const scene = new TaskScene(row);
        assert.deepEqual(scene.run(), row.slice(9), `행 ${count}: 입력 ${row.slice(0, 9).join(' ')}`);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('DudeTask: 미연결 Regular 생성과 표면 효과를 원본 호출 시점에서 거부한다', () => {
    const row = rows ? rows[0].slice() : ['originals', '50', '0', '20737', '20739', '0', '70', '0', '4621819117588971520', 'return', '', '', '-'];
    const scene = new TaskScene(row);
    const hooks = scene.hooks();
    delete hooks.addTaskRegular;
    const task = new RawDudeTask(scene.pool, { clock: scene.clock }, hooks);
    assert.ok(throws(() => task.set(SOURCE, 0x5103, 0)));
    assert.equal(get(scene.pool.slot(SOURCE), 12, 2), 0x5101);
    row[6] = '82';
    const bridge = new TaskScene(row);
    const bridgeHooks = bridge.hooks();
    delete bridgeHooks.notifySurface;
    const bridgeTask = new RawDudeTask(bridge.pool, { clock: bridge.clock }, bridgeHooks);
    assert.ok(throws(() => bridgeTask.set(SOURCE, 0x5103, Math.fround(0.1))));
    assert.equal(get(bridge.pool.slot(SOURCE), 12, 2), 0x5103);
    assert.ok(bridge.host.findEvent(SOURCE, 0x5103) !== null);
});

test('DudeTask: 판본·시계·풀 혼합을 거부하고 비유한 지연/시각은 쓰기 전에 거부한다', () => {
    const row = ['originals', '50', '0', '20737', '20739', '0', '70', '0', '4621819117588971520', 'return', '', '', '-'];
    const scene = new TaskScene(row);
    assert.ok(throws(() => new RawDudeTask({ edition: 'cd1072' }, { clock: scene.clock }, scene.hooks())));
    assert.ok(throws(() => new RawDudeTask(scene.pool, {}, scene.hooks())));
    assert.ok(throws(() => makeDudeTaskHostHooks(new SidPool(24000), scene.host, {})));
    const task = new RawDudeTask(scene.pool, { clock: scene.clock }, scene.hooks());
    assert.ok(throws(() => task.set(SOURCE, 0x5103, Infinity)));
    scene.clock.now = Number.NaN;
    assert.ok(throws(() => task.set(SOURCE, 0x5103, 0)));
    assert.equal(get(scene.pool.slot(SOURCE), 12, 2), 0x5101);
    assert.equal(task.pool, scene.pool);
});
