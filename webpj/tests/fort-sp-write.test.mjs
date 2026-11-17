// 최초 SP writer198관측의 전체 물리·공유 난수·부분 중단/별칭을 대조한다(WB04-E634).
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, get, put, bitsToFloat } from './raw-support.mjs';
import { GameRandom } from '../www/core/random.js';
import { writeFortSp } from '../www/core/raw-fort-sp-write.js';
// 원본 외부 중단 표식이며 제품 오류와 구별한다.
class FixtureStop extends Error {}
// 전체 BYTE를 원문 소문자 hex로 관측한다.
function hex(bytes) { return Buffer.from(bytes).toString('hex'); }
// 실제 세 버퍼/별칭 stream과 독립 입력의 외부 변경만 소유한다.
class Devices {
    // 셋째 할당 앞16BYTE가 같은 실제 stream이며 사본이 아니다.
    buffers = Array.from({ length: 3 }, () => new Uint8Array(256)); stream;
    // 공유 난수와 입력 float 비트·외부 변경 및 실제 관측 횟수/순서다.
    random; bits; mutation; randoms = 0; assertions = 0; equal = 0; trace = [];
    // 관측 기대 열을 보지 않고 최초8입력만 구성한다.
    constructor(fields) {
        this.bits = Number(fields[0]); this.random = new GameRandom(Number(fields[1])); this.mutation = fields[7];
        this.buffers.forEach((buffer, index) => { /* 물리 인접 BYTE도 원본과 같은 입력 패턴이다. */ for (let byte = 0; byte < buffer.length; byte++) buffer[byte] = Number(fields[5]) + index * 37 + byte * 11; });
        this.stream = this.buffers[2].subarray(0, 16); put(this.stream, 0, 0x12001000 + Number(fields[6]) * 0x1000);
        [4, 8, 12].forEach((at, index) => put(this.stream, at, Number(fields[index + 2])));
    }
    // 지정 경계의 외부 변경만 적용한다. 후속 선택/쓰기는 제품이 결정한다.
    mutate(boundary) {
        const split = this.mutation.indexOf('-'); if (split < 0 || this.mutation.slice(0, split) !== boundary) return; const action = this.mutation.slice(split + 1);
        if (action === 'stop') throw new FixtureStop();
        if (action === 'seed0') this.random.setState(0);
        else if (action === 'baseA' || action === 'baseB') { put(this.stream, 0, action === 'baseB' ? 0x12002000 : 0x12001000); put(this.stream, 4, 5); put(this.stream, 8, 1); put(this.stream, 12, 128); }
        else if (action === 'stream') { put(this.stream, 0, 0x12003000); put(this.stream, 4, 8); }
        else if (action === 'position240') { put(this.stream, 4, 240); put(this.stream, 12, 256); }
        else if (action === 'capacity0') put(this.stream, 12, 0);
        else if (action === 'endHigh') put(this.stream, 8, 0x80000000);
        else if (action === 'position17') { put(this.stream, 4, 17); put(this.stream, 12, 256); }
        else throw new Error('SP writer 미해석 입력 변경');
    }
    // 같은 GameRandom·현재 실제 물리·선택 진단만 연결한다.
    hooks() { return {
        nextRandom: (limit) => { assert.equal(limit, 32); const value = this.random.next(limit); this.randoms++; this.equal += Number(value === Math.floor((this.randoms - 1) / 16)); this.trace.push(`R:${this.randoms}:${value}:${this.random.state}`); this.mutate(`r${this.randoms}`); return value; },
        stream: { buffer: (token) => { const index = (token - 0x12001000) / 0x1000; assert.ok(index === 0 || index === 1 || index === 2); return this.buffers[index]; }, assertion: (kind) => { assert.equal(kind, 'WriteCapacity'); this.assertions++; this.trace.push('A:36'); this.mutate(`a${this.assertions}`); } },
    }; }
}
// 독립 x86 정상178·중단20행의 전체 버퍼와 변환/난수/호출 순서를 대조한다.
test('FortSpWrite_ReplaysIndependentNative198', () => {
    const rows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/fort-sp-write-x86.tsv')); assert.equal(rows.length, 198); let normal = 0, stops = 0;
    // 원본 두 정밀도의 각 입력은 결과열과 독립해서 실행한다.
    for (const row of rows) {
        assert.equal(row.length, 13); const fields = row[1].split(','), d = new Devices(fields); let status = 'normal';
        try { writeFortSp(d.stream, bitsToFloat(d.bits), d.hooks()); } catch (error) { if (!(error instanceof FixtureStop)) throw error; status = 'stop'; }
        assert.deepEqual([status, d.random.state, d.randoms, d.assertions, d.equal, ...d.buffers.map(hex)], [row[2], ...row.slice(4, 8).map(Number), ...row.slice(8, 11)], row.slice(0, 2).join(':'));
        const prefix = `F:${row[3]};`; assert.ok(row[11].startsWith(prefix)); assert.equal(d.trace.join(';'), row[11].slice(prefix.length));
        if (status === 'normal' && fields[6] === '0' && fields[7] === 'none') { let decoded = 0, offset = Number(fields[2]); /* 실제88BYTE 출력에서 보호 bit만 읽어 원본 _ftol EAX와 비교한다. */ for (let bit = 0; bit < 32; bit++) { const width = bit < 8 ? 1 : bit < 16 ? 2 : 4; decoded |= get(d.buffers[0], offset, width) & (1 << bit); offset += width; } assert.equal(decoded >>> 0, Number(row[3])); }
        if (status === 'normal') { normal++; assert.equal(d.randoms, 512); } else stops++;
    }
    assert.deepEqual([normal, stops], [178, 20]);
});
