// ProcessForm 서버 부착/갱신의 WriteData·송신·체인/Pop 순서를 원본 10.78 관측과 대조한다(WB04-D5c, cpppj ProcessTransmitTests 이식).
//  - processtransmit-x86.tsv 10.78 행(Attach·Refresh): 전송/억제 flags·부모 이웃·새/지정 Form·양수/음수 WriteData(크기 변환 포함)에서 writer(W)·송신(S)·부착 통지(N)의 순서와
//    Form/부모/머리 슬롯 전체, 타입 순환, 풀 메타를 비교한다. 직렬화(WriteData)와 송신은 입력 장치이고 파생 onAttached 만 외부 경계이다.
// CD 10.72 행은 범위 밖이다. (2026-11-08 WB04-D5c: Process 호스트 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcess, SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';

// 원본 관측 파일 행(10.78 행만)이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-D5c)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'processtransmit-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 검사 풀 크기이다. (2026-11-08 WB04-D5c)
const CAPACITY = 32768;
// 바이트 배열의 소문자 16진 문자열이다. (2026-11-08 WB04-D5c)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 내장 Form 네 생성자 주소만 넣은 188칸 TYPE 입력이다(cpppj MakeFormTypeInputs). (2026-11-08 WB04-D5c)
const formTypeInputs = () => Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));

// 부착 통지만 관찰하는 프로세스이다. 실행 몸체는 이 전송 검사에서 호출하지 않는다. (2026-11-08 WB04-D5c)
class TransmittedProcess extends SquidProcess {
    constructor(events) {
        super();
        this.events = events;
    }

    runFrame() {}

    // WriteData/송신/부모 체인 Pop 뒤 한 번 호출되는 실제 가상 통지이다.
    onAttached() { this.events.push(`N:${this.form}:${this.parent}`); }
}

// 같은 SID/Kernel 에서 지정 슬롯과 신규 슬롯을 대조하는 장면이다. size 는 writer 가 돌려줄 int32 이다. (2026-11-08 WB04-D5c)
class TransmitScene {
    constructor(server, size) {
        const types = formTypeInputs();
        this.pool = new SidPool(CAPACITY, { server });
        this.destroy = new RawSquidDestroy(this.pool, new SquidUnpop(this.pool, new SquidHash(), new Uint8Array(65536)), types);
        this.kernel = new Kernel();
        this.state = new SquidProcessState();
        this.events = [];
        this.size = size;
        // WriteData 와 버퍼 송신 경계를 원본이 호출한 순서 그대로 기록한다.
        this.host = new SquidProcessHost(this.pool, types, this.kernel, this.destroy, this.state, {
            transmit: {
                writeData: (process, changed) => {
                    const form = process.form;
                    this.events.push(`W:${form}:${changed ? 1 : 0}:${hex(this.pool.slot(form))}:${hex(this.pool.slot(20000))}:${form}:${process.parent}`);
                    return this.size;
                },
                transmit: (length) => this.events.push(`S:${length >>> 0}`),
            },
        });
    }
}

// 전송/억제 flags·부모 이웃·새/지정 Form·양수/음수 WriteData 를 모두 재생한다.
test('SquidProcessHost: 원본10.78 ProcessForm 서버 부착/갱신의 WriteData·송신 순서', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of rows) {
        assert.equal(row.length, 10);
        const input = row[2].split(',').map(Number);
        assert.equal(input.length, 7);
        const scene = new TransmitScene(input[0] !== 0, input[6] | 0);
        const parent = scene.pool.physical(20000);
        const head = scene.pool.physical(50);
        // 원본 합성 입력과 같은 표식이다. 생성 결과는 계산하지 않는다.
        for (const bytes of [parent, head]) for (let index = 0; index < bytes.length; index += 1) bytes[index] = (index * 29 + 7) & 0xff;
        parent[10] = 100;
        parent[11] = 0;
        put(parent, 6, input[3] ? 50 : 0, 2);
        put(head, 0, input[3] === 1 ? 0x512098 : 0x501dd0);
        head[10] = input[3] === 1 ? 46 : 100;
        head[11] = 8;
        const form = input[1] ? scene.pool.layout.serverFirst + 50 : 0;
        if (form) {
            const raw = scene.pool.physical(form);
            // 지정 Take 는 기존 주변 바이트를 보존한다.
            for (let index = 0; index < raw.length; index += 1) raw[index] = (index * 29 + 7) & 0xff;
            raw[11] = input[2];
        }
        const process = scene.host.attach(new TransmittedProcess(scene.events), input[5], 20000, input[4], form);
        assert.ok(process, `행 ${count}`);
        if (row[1] === 'Refresh') {
            scene.events.length = 0;
            scene.host.refreshForm(process);
        }
        const actualForm = process.form;
        const meta = `${scene.pool.freeCount}:${scene.pool.predictableCursor}:${scene.kernel.get(1) === process ? 1 : 0}`;
        const label = `${row[1]} ${row[2]} (행 ${count})`;
        assert.equal(scene.events.length ? scene.events.join(';') : '-', row[3], label);
        assert.equal(hex(scene.pool.slot(actualForm)), row[4], `${label} Form`);
        assert.equal(hex(parent), row[5], `${label} 부모`);
        assert.equal(hex(head), row[6], `${label} 머리`);
        assert.equal(`${actualForm}:${process.parent}`, row[7], label);
        assert.equal(String(scene.state.typeRotation), row[8], `${label} 회전`);
        assert.equal(meta, row[9], `${label} 메타`);
        count += 1;
    }
    assert.ok(count > 0);
});
