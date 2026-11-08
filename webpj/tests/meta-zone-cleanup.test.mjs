// 메타구역 기록 정리 raw-meta-zone-cleanup.js(clearMetaZoneRecords·clearMetaZoneMarker)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: meta-zone-cleanup-x86.tsv 38행(두 x87 정밀도의 부모 정상 28·명시 중단 10, 부모는 부동소수 계산이 없어 결과가 같음) — 시드·초기 플래그·자식 변경(n:flag1|flagMax|touch)·중단 번째 → 상태(normal/stop)·최종 플래그·
//    표 7344+인접 canary 16 바이트 전체·자식 호출 번호·콜백 기록. 부모 계산을 대체하지 않고 초기 실제 표와 선택 자식의 변경/중단만 제공한다.
//  - 계약: 첫 비영 관문만 자식을 생략하고 이후 미연결 자식은 전체 0 쓰기보다 먼저 거부하며 자식 부분 효과는 남는다. 짧은 물리는 DWORD 경계에서만 거부하고 완료한 접두와 뒤 canary 를 보존한다. 마커의 상태/SID 관문과 복귀 뒤 같은 DWORD 0 쓰기.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E77: 메타구역 기록 정리 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get } from './raw-support.mjs';
import { META_ZONE_BYTES, META_ZONE_COUNT, META_ZONE_STRIDE, clearMetaZoneMarker, clearMetaZoneRecords } from '../www/core/raw-meta-zone-cleanup.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E77)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'meta-zone-cleanup-x86.tsv'));

// 독립 입력 수(정상/중단)와 표 뒤 canary 폭이다. (2026-11-08 WB04-E77)
const ROWS = 38;
const NORMAL_ROWS = 28;
const STOPPED_ROWS = 10;
const CANARY = 16;

// 예외가 나는지 본다. (2026-11-08 WB04-E77)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E77)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 명시 자식 장치의 중단을 물리 오류와 구별하는 표식이다. (2026-11-08 WB04-E77)
class MetaStop extends Error {}

// 부모 계산을 대체하지 않고 초기 실제 표·선택 자식 효과만 제공하는 장치이다. (2026-11-08 WB04-E77)
class MetaDevice {
    // 최초 Native 와 같은 초기 입력만 구성하며 원문 출력은 계산하지 않는다.
    constructor(fields) {
        assert.equal(fields.length, 4);
        const seed = Number(fields[0]);
        this.flag = Number(fields[1]) >>> 0;
        this.stop = Number(fields[3]);
        const effect = fields[2].split(':');
        assert.equal(effect.length, 2);
        this.mutationAt = Number(effect[0]);
        this.mutation = effect[1];
        this.calls = [];
        this.trace = [];
        this.count = 0;
        this.records = new Uint8Array(META_ZONE_BYTES + CANARY);
        // 실제 7344 바이트 밖도 원시 입력과 같다.
        for (let byte = 0; byte < this.records.length; byte += 1) this.records[byte] = (seed + byte * 17) & 0xff;
    }

    // 실제 00479520 본문은 대체하지 않으며 관측에서 명시한 변경·중단 장치만 제공한다.
    child(index) {
        this.calls.push(String(index));
        this.trace.push(`C:${index}`);
        this.count += 1;
        if (this.count === this.stop) throw new MetaStop('stop');
        if (this.count !== this.mutationAt) return;
        this.trace.push(`M:${this.mutation}`);
        if (this.mutation === 'flag1') this.flag = 1;
        else if (this.mutation === 'flagMax') this.flag = 0xffffffff;
        else if (this.mutation === 'touch') put(this.records, index * META_ZONE_STRIDE + 76, 0x89abcdef);
        else throw new Error('메타구역 미해석 명시 효과');
    }

    // 현재 플래그를 읽고 쓰는 속성과 고정 표를 가진 전역 뷰이다.
    globals() {
        const device = this;
        return {
            get firstReset() { return device.flag; },
            set firstReset(value) { device.flag = value; },
            records: device.records
        };
    }
}

test('MetaZoneCleanup: 원본 부모 38관측(정상 28·명시 중단 10)의 표·플래그·자식 호출 순서가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    let normal = 0;
    let stopped = 0;
    // 두 정밀도 전체 38관측을 재생한다.
    for (const row of rows) {
        assert.equal(row.length, 10);
        const device = new MetaDevice(row[1].split(','));
        let status = 'normal';
        try {
            clearMetaZoneRecords(device.globals(), { clearMarker: (index) => device.child(index) });
        } catch (error) {
            if (!(error instanceof MetaStop)) throw error;
            status = 'stop';
        }
        const label = `행 ${count}: 입력 ${row[1]}`;
        assert.equal(status, row[2], label);
        assert.equal(String(device.flag), row[3], label);
        assert.equal(hex(device.records), row[4], label);
        assert.equal(device.calls.length === 0 ? '-' : device.calls.join(','), row[5], label);
        assert.equal(device.trace.length === 0 ? '-' : device.trace.join(';'), row[6], label);
        assert.ok(row[7] === (status === 'normal' ? '1836' : '0') && Number(row[9]) > 0, label);
        count += 1;
        if (status === 'normal') normal += 1; else stopped += 1;
    }
    assert.ok(count === ROWS && normal === NORMAL_ROWS && stopped === STOPPED_ROWS);
});

test('MetaZoneCleanup: 최초 비영 관문만 자식을 생략하고 이후 미연결 자식은 전체 0 쓰기보다 먼저 거부하며 부분 효과를 남긴다', () => {
    const records = new Uint8Array(META_ZONE_BYTES + CANARY).fill(0xa5);
    const holder = { firstReset: 0xffffffff, records };
    clearMetaZoneRecords(holder, {});
    assert.ok(holder.firstReset === 0 && records.slice(0, META_ZONE_BYTES).every((byte) => byte === 0) && records[records.length - 1] === 0xa5);
    records[0] = 0x7a;
    assert.ok(throws(() => clearMetaZoneRecords(holder, {})));
    assert.ok(holder.firstReset === 0 && records[0] === 0x7a && records[records.length - 1] === 0xa5);
    let stopped = false;
    try {
        clearMetaZoneRecords(holder, { clearMarker: (index) => { assert.equal(index, 0); holder.firstReset = 1; records[1] = 0x6b; throw new MetaStop('stop'); } });
    } catch (error) {
        stopped = error instanceof MetaStop;
    }
    assert.ok(stopped && holder.firstReset === 1 && records[0] === 0x7a && records[1] === 0x6b);
});

test('MetaZoneCleanup: 짧은 물리는 DWORD 경계에서만 거부하고 완료한 접두와 뒤 canary 를 보존하며 자식 호출은 앞서 51회 끝난다', () => {
    // 첫/중간/마지막 DWORD 와 인접 물리의 경계 입력이다.
    for (const length of [0, 1, 3, 4, 5, 7, 8, 142, 143, 144, 7339, 7340, 7343, 7344, 7348]) {
        const records = new Uint8Array(META_ZONE_BYTES + CANARY).fill(0xa5);
        const holder = { firstReset: 1, records: records.subarray(0, length) };
        let rejected = false;
        try { clearMetaZoneRecords(holder, {}); } catch (error) { rejected = error instanceof RangeError; }
        const written = Math.min(Math.floor(length / 4), META_ZONE_BYTES / 4) * 4;
        assert.ok(rejected === (length < META_ZONE_BYTES) && holder.firstReset === (rejected ? 1 : 0), `길이 ${length}`);
        // 완료된 접두와 뒤 canary 를 한 번에 확인한다.
        for (let byte = 0; byte < records.length; byte += 1) assert.equal(records[byte], byte < written ? 0 : 0xa5, `길이 ${length} 바이트 ${byte}`);
    }
    const records = new Uint8Array(META_ZONE_BYTES).fill(0xa5);
    const holder = { firstReset: 0, records: records.subarray(0, 8) };
    let calls = 0;
    let rejected = false;
    try {
        clearMetaZoneRecords(holder, { clearMarker: (index) => { assert.equal(index, calls); calls += 1; if (index === 50) holder.firstReset = 0x12345678; } });
    } catch (error) {
        rejected = error instanceof RangeError;
    }
    assert.ok(rejected && calls === META_ZONE_COUNT && holder.firstReset === 0x12345678 && records[0] === 0 && records[7] === 0 && records[8] === 0xa5);
});

test('MetaZoneMarker: 상태 signed 0 이하·SID 0 은 삭제 없이 반환하고 삭제 복귀 뒤 같은 DWORD 만 0 으로 쓴다', () => {
    const records = new Uint8Array(META_ZONE_BYTES).fill(0xa5);
    const at = 3 * META_ZONE_STRIDE;
    // 상태 0 과 음수는 SID 를 읽지 않고 반환한다.
    put(records, at, 0);
    clearMetaZoneMarker(records, 3, {});
    put(records, at, 0x80000000);
    clearMetaZoneMarker(records, 3, {});
    // 양수 상태라도 SID 가 0 이면 반환한다.
    put(records, at, 1);
    put(records, at + 76, 0);
    clearMetaZoneMarker(records, 3, {});
    // 비영 SID 는 원시 DWORD 와 flags 0 으로 삭제하고 삭제가 필드를 바꿔도 같은 DWORD 만 지운다.
    put(records, at + 76, 0xdeadbeef);
    const seen = [];
    clearMetaZoneMarker(records, 3, { destroy: (sid, flags) => { seen.push([sid, flags]); put(records, at + 76, 0x11223344); put(records, at + 80, 0x55667788); } });
    assert.deepEqual(seen, [[0xdeadbeef, 0]]);
    assert.ok(get(records, at + 76) === 0 && get(records, at + 80) === 0x55667788);
    // 삭제 장치가 없거나 실패하면 SID 를 앞당겨 지우지 않는다.
    put(records, at + 76, 7);
    assert.ok(throws(() => clearMetaZoneMarker(records, 3, {})));
    assert.ok(throws(() => clearMetaZoneMarker(records, 3, { destroy: () => { throw new Error('삭제 실패'); } })));
    assert.equal(get(records, at + 76), 7);
    assert.ok(throws(() => clearMetaZoneMarker(records, 51, {})));
    // 짧은 물리는 읽는 DWORD 에서만 거부한다.
    assert.ok(throws(() => clearMetaZoneMarker(records.subarray(0, 3), 0, {})));
});
