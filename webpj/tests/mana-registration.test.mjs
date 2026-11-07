// 마나 공급원 등록(registerManaProvider)을 원본 10.78 관측으로 대조한다(WB04-E10, cpppj ManaRegistrationTests 이식).
//  - mana-registration-hardware-x86.tsv 의 originals 542행: 중복/양끝/전체 DWORD 토큰·signed 용량·카운터 감김·진단 뒤 현재 owner(목록 교체·checking 해제·카운터 변경·중복 삭제)를
//    포인터/개수/용량/카운터/checking/두 물리 배열/진단 이벤트로 대조한다. originalCD 542행은 10.72 전용이라 제외한다.
//  - 계약: 첫 중복 검사는 다음 현재 개수 읽기보다 먼저 끝남, 진단 장치 누락/중단은 앞선 효과를 남김, 선택하지 않은 배열 장치는 읽거나 쓰지 않음.
// cpppj 의 실제 Client/World 통합(ActualClientWorldCurrentOwnerAndPostPopContinuation)은 공통 PostPop 본체와 World 조립 단위에서 대조한다.
// (2026-11-08 WB04-E10: 마나 공급원 등록 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { registerManaProvider } from '../www/core/raw-mana-registration.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E10)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'mana-registration-hardware-x86.tsv'));
const rows = allRows?.filter((row) => row[0] === 'originals') ?? null;

// 입력 DWORD 를 32비트로 감아 읽는다. (2026-11-08 WB04-E10)
const number = (text) => Number(BigInt(text) & 0xffffffffn);

// 16개 DWORD 를 명시 little endian 으로 직렬화한 소문자 hex 이다. (2026-11-08 WB04-E10)
function words(values) {
    const raw = new Uint8Array(64);
    for (let index = 0; index < values.length; index += 1) put(raw, index * 4, values[index]);
    return Buffer.from(raw).toString('hex');
}

// 현재 포인터/용량/개수/두 물리 배열을 서로 다른 저장소로 유지하는 입력 장면이다(cpppj ManaRegistrationScene). (2026-11-08 WB04-E10)
class ManaRegistrationScene {
    constructor(row) {
        this.a = new Uint32Array(16);
        this.b = new Uint32Array(16);
        this.value = number(row[2]);
        this.used = number(row[3]);
        this.capacity = number(row[4]);
        this.state = { productionDirty: number(row[6]) };
        this.checking = number(row[7]);
        this.action = number(row[8]);
        this.pointer = 0;
        this.events = '';
        const pattern = number(row[5]);
        // 예약 꼬리도 서로 구별한다.
        for (let index = 0; index < 16; index += 1) {
            this.a[index] = 0x11220000 + index;
            this.b[index] = 0x33440000 + index;
        }
        if (this.used && (pattern & 1)) this.a[0] = this.value;
        if (this.used && (pattern & 2)) this.a[this.used - 1] = this.value;
        this.list = new SquidPostPopList();
        this.list.bindCurrent({
            storage: () => (this.pointer ? this.b : this.a),
            capacity: () => this.capacity,
            readCount: () => this.used,
            writeCount: (count) => { this.used = count; },
        });
        this.diagnostics = {
            checking: () => this.checking,
            assertion: (expression, file, line) => {
                this.events += `D,${line}:${expression}:${file};`;
                // 진단에서 실제 입력만 바꾸며 부모 분기 결과는 만들지 않는다.
                if (this.action & 1) this.checking = 0;
                if (this.action & 2) { this.pointer = 1; this.used = 0; this.capacity = 2; }
                if (this.action & 4) this.state.productionDirty = 0xffffffff;
                if (this.action & 8) for (let index = 0; index < 16; index += 1) if (this.a[index] === this.value) this.a[index] = 0xdead0000 + index;
            },
        };
    }

    // 출력 모델 없이 현재 메모리만 읽는다.
    run() {
        registerManaProvider(this.list, this.state, this.value, this.diagnostics);
        return [String(this.pointer), String(this.used), String(this.capacity), String(this.state.productionDirty), String(this.checking >>> 0), words(this.a), words(this.b), this.events || '-'];
    }
}

// 중복/양끝/전체 DWORD 토큰·signed 용량·카운터 감김·진단 뒤 현재 owner 를 실제 패치 CPU 와 비교한다.
test('마나 공급원 등록: 원본10.78 542행(중복·양끝·signed 용량·카운터 감김·진단 뒤 owner)', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of rows) {
        assert.equal(row.length, 17);
        const scene = new ManaRegistrationScene(row);
        const actual = scene.run();
        assert.deepEqual(actual, row.slice(9), `행 ${row[1]} 입력 ${row.slice(2, 9).join(',')}`);
        count += 1;
    }
    assert.equal(count, 542);
});

// 기계어의 최초 양수 개수 분기 뒤 첫 원소 검사/중복 종료는 다음 count 읽기보다 앞선다.
test('마나 공급원 등록: 첫 검사가 다음 현재 개수 읽기보다 먼저 끝난다', () => {
    const list = new SquidPostPopList();
    const wordsArray = new Uint32Array([50]);
    let used = 1;
    let reads = 0;
    const state = { productionDirty: 0 };
    list.bindCurrent({
        storage: () => wordsArray,
        capacity: () => 1,
        // 첫 읽기가 양수 값을 반환한 직후 현재 개수가 0 이 된다는 명시 외부 입력이다.
        readCount: () => { const value = used; reads += 1; if (reads === 1) used = 0; return value; },
        writeCount: (value) => { used = value; },
    });
    registerManaProvider(list, state, 50);
    // 첫 중복을 찾았으므로 다음 count/용량/배열 쓰기를 선택하지 않는다.
    assert.deepEqual([used, reads, state.productionDirty, wordsArray[0]], [0, 1, 1, 50]);
});

// 진단 누락/중단과 선택되지 않은 배열 장치를 빈 성공/사전 효과 취소로 바꾸지 않는다.
test('마나 공급원 등록: 선택 장치 누락과 부분 효과', () => {
    const row = allRows.find((candidate) => candidate[0] === 'originals').slice();
    row[2] = '50'; row[3] = '1'; row[4] = '4'; row[5] = '1'; row[6] = '123'; row[7] = '2147483648';
    const missing = new ManaRegistrationScene(row);
    delete missing.diagnostics.assertion;
    assert.throws(() => missing.run(), /진단 장치 미연결/);
    assert.deepEqual([missing.state.productionDirty, missing.used, missing.a[0]], [123, 1, 50]);
    const stopped = new ManaRegistrationScene(row);
    stopped.diagnostics.assertion = () => { stopped.pointer = 1; stopped.used = 0; stopped.state.productionDirty = 42; throw new Error('선택 진단 중단'); };
    assert.throws(() => stopped.run(), /선택 진단 중단/);
    assert.deepEqual([stopped.pointer, stopped.used, stopped.state.productionDirty, stopped.a[0]], [1, 0, 42, 50]);
    // signed 음수 용량/빈 검색은 배열을 읽거나 쓰지 않는다.
    const state = { productionDirty: 0xffffffff };
    const noArray = new SquidPostPopList();
    noArray.bindCurrent({
        storage: () => { throw new Error('미선택 배열 장치'); },
        capacity: () => 0x80000000,
        readCount: () => 0,
        writeCount: () => { throw new Error('미선택 개수 쓰기'); },
    });
    registerManaProvider(noArray, state, 0xffffffff);
    assert.equal(state.productionDirty, 0);
});
