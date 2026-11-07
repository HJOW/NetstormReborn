// 부모에 붙은 프로세스 일괄 제거(removeAttachedProcesses, 원본 PATCH 004b27a0)를 원본 10.78 관측과 대조한다(WB04-D5c).
//  - process-removal-filter-x86.tsv 64행: 혼합 Form 목록(프로세스 Form·자산 입력)과 요청 타입의 조합마다 삭제 순서와 전달 flags(index:flags)를 비교한다.
//    cpppj rawProcessRemoval_native_mixed_form_filter 와 같은 입력 구성이다(Path 프로세스 장면은 WB05 이후라 빈 부모에 목록만 연결한다).
//    단, cpppj 입력의 프로세스가 아닌 raw 노드는 가상 표가 0 이나 현재 소스의 링크 getter/setter(공통 contained)가 미복원 가상 표를 거부하므로(옛 검사는 소스 읽기상 이 점에서 기준선과 다르다)
//    웹 검사는 그 노드에 실제 Form(Content) / 자산 가상 표를 넣는다. 필터·삭제 순서·flags 비교는 같다.
//  - 계약: 후보의 다음 링크를 삭제 전에 저장하고 소멸 콜백이 새로 붙인 객체는 지우지 않으며, 프로세스 TYPE 인데 Kernel 에 없으면 명시 오류이다.
// (2026-11-08 WB04-D5c: Process 부착 부속 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcess, SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { SQUID_ASSET_VTABLES, SQUID_FORM_VTABLES } from '../www/core/squid-vtables.js';
import { removeAttachedProcesses } from '../www/core/raw-process-removal.js';

// 원본 관측 파일 행 읽기이다(판본 열이 없는 10.78 전용 표). 파일이 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-D5c)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'process-removal-filter-x86.tsv'));

// 내장 Form 네 생성자 주소만 넣은 188칸 TYPE 입력이다(cpppj MakeFormTypeInputs). (2026-11-08 WB04-D5c)
const formTypeInputs = () => Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));

// 같은 풀의 공장·삭제기·Kernel·호스트 묶음과 부모(자산 74)이다. (2026-11-08 WB04-D5c)
function scene() {
    const types = formTypeInputs();
    const pool = new SidPool(32768, { server: false });
    const factory = new SquidFactory(pool, types);
    const destroy = new RawSquidDestroy(pool, new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536)), types);
    const kernel = new Kernel();
    const host = new SquidProcessHost(pool, types, kernel, destroy, new SquidProcessState());
    return { pool, factory, kernel, host, parent: factory.create(74, 2) };
}

// onFormDestroy 의 전달 flags 를 관측하는 프로세스이다. (2026-11-08 WB04-D5c)
class Probe extends SquidProcess {
    constructor(effect) {
        super();
        this.effect = effect;
    }

    onFormDestroy(flags) { this.effect(flags); }
}

// 64행: 목록 순서/판본별 Form·자산 링크를 부착 순서와 무관하게 다시 연결한 뒤 삭제 관측(index:flags)을 이어 붙여 비교한다.
test('removeAttachedProcesses: 원본10.78 혼합 Form 목록 필터 64행', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of rows) {
        assert.equal(row.length, 3);
        const parts = scene();
        const trace = [];
        const types = row[0] === '-' ? [] : row[0].split(',').map(Number);
        const nodes = [];
        // 프로세스 타입은 실제 Kernel 에 부착하고 그 밖의 타입은 명시적인 raw Form 입력이다.
        types.forEach((type, index) => {
            if (type >= 10 && type < 63) {
                const process = parts.host.attach(new Probe((flags) => trace.push(`${index}:${flags}`)), type, parts.parent, 0x10);
                assert.ok(process);
                nodes.push(process.form);
            } else {
                const form = parts.pool.allocate(2);
                const raw = parts.pool.allocated(form);
                raw[10] = type;
                // 가상 표: 70 미만은 Form 링크 배치(+14 이전·+16 부모), 이상은 지원 자산 배치이다.
                put(raw, 0, type < 70 ? SQUID_FORM_VTABLES.content : SQUID_ASSET_VTABLES[0][0]);
                nodes.push(form);
            }
        });
        put(parts.pool.allocated(parts.parent), 6, nodes.length ? nodes[0] : 0, 2);
        // 부착 순서와 무관하게 원문과 같은 목록 순서/판본별 Form·asset 링크를 제공한다.
        nodes.forEach((node, index) => {
            const raw = parts.pool.allocated(node);
            put(raw, 4, index + 1 < nodes.length ? nodes[index + 1] : 0, 2);
            raw[11] |= 8;
            put(raw, raw[10] < 70 ? 14 : 16, index ? nodes[index - 1] : 0, 2);
            put(raw, raw[10] < 70 ? 16 : 14, parts.parent, 2);
        });
        removeAttachedProcesses(parts.host, parts.parent, Number(row[1]));
        assert.equal(trace.length ? trace.join(';') : '-', row[2], `${row[0]} 요청 ${row[1]}`);
        count += 1;
    }
    assert.equal(count, 64);
});

// 유효한 프로세스 타입의 PID 가 없는 손상 입력은 필터 복원 뒤에도 명시적으로 거부한다.
test('removeAttachedProcesses: 프로세스 PID 가 없는 손상 입력은 명시 오류', () => {
    const parts = scene();
    const form = parts.pool.allocate(2);
    const raw = parts.pool.allocated(form);
    raw[10] = 39;
    raw[11] |= 8;
    put(raw, 4, 0, 2);
    put(raw, 14, 0, 2);
    put(raw, 16, parts.parent, 2);
    put(raw, 18, 0);
    put(parts.pool.allocated(parts.parent), 6, form, 2);
    assert.throws(() => removeAttachedProcesses(parts.host, parts.parent, 0), /실제 제거할 프로세스 부재/);
});

// 소멸 콜백이 같은 부모에 새 프로세스를 붙여도 이미 저장한 next 만 따라가므로 새 객체는 지우지 않는다(머리부터 다시 검색하지 않음).
test('removeAttachedProcesses: 삭제 콜백이 붙인 새 프로세스는 남는다', () => {
    const parts = scene();
    const flags = [];
    let added = 0;
    const probe = parts.host.attach(new Probe((value) => {
        flags.push(value);
        added = parts.kernel.find(parts.host.addRegular(parts.parent, 1, 0.05));
    }), 39, parts.parent, 0x10);
    const probeId = parts.kernel.find(probe);
    removeAttachedProcesses(parts.host, parts.parent, 61);
    assert.ok(parts.kernel.get(probeId) !== null);
    removeAttachedProcesses(parts.host, parts.parent, 0);
    assert.deepEqual(flags, [0x40]);
    assert.equal(parts.kernel.get(probeId), null);
    assert.ok(parts.kernel.get(added) !== null);
    assert.equal(get(parts.pool.slot(parts.parent), 6, 2) !== 0, true);
});
