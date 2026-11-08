// 비로컬 의식 보상 raw-foreign-ritual-reward.js(grantForeignRitualSp·grantForeignRitualType)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: foreign-ritual-reward-x86.tsv 의 기술 모드(mode 0) 144행 — 입력 9 값(x87 제어·모드·owner·local·known·editor·spBits·기술 비트(signed)·seed)로 256바이트 Core 창 전체(owner 32비트 감김·비정렬 DWORD·이웃 보존·known 이웃 DWORD)와
//    효과 순서(T:TYPE 조회 / A:진단 0x15a 뒤 정상 복귀). SP 모드(mode 1)의 180행 실제 분산 저장소·정책 대조는 해당 저장소 이식 전이라 사용하지 않는다.
//  - 계약: SP 덧셈의 ftol 절삭(비유한/범위 밖은 0·signed32 감김)·owner 비트 변환·미연결 거부, 기술 Core 의 비트 없는 TYPE 무쓰기·진단 뒤 현재 DWORD 읽기·물리 끝 초과 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E106: 비로컬 의식 보상 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get } from './raw-support.mjs';
import { grantForeignRitualSp, grantForeignRitualType } from '../www/core/raw-foreign-ritual-reward.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E106)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'foreign-ritual-reward-x86.tsv'));

// 예외가 나는지 본다. (2026-11-08 WB04-E106)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

const hex = (bytes) => Buffer.from(bytes).toString('hex');

test('ForeignRitualReward: 원본 10.78 기술 모드 144입력의 256바이트 Core 창 전체와 T/A 효과 순서가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 양 x87 모드의 Core 결과는 동일하게 검사한다.
    for (const row of rows) {
        assert.equal(row.length, 3);
        const input = row[0].split(',');
        if (Number(input[1]) !== 0) continue;
        const owner = Number(BigInt.asUintN(32, BigInt(input[2])));
        const seed = Number(input[8]);
        const bit = Number(input[7]);
        const records = new Uint8Array(256);
        const effects = [];
        // 장부 앞뒤와 padding 까지 독립 Native 와 같은 입력이다(출력 기대값을 계산하지 않는다).
        for (let byte = 0; byte < records.length; byte += 1) records[byte] = (seed + byte * 17) & 0xff;
        put(records, 0x24, Number(input[4]));
        put(records, 0x2c, 0x12200000);
        grantForeignRitualType({ bytes: records, origin: 64 }, owner, 85, (type) => { effects.push(`T:${type}`); return bit; }, (line) => effects.push(`A:${line}`));
        const native = row[2].split(';').filter((effect) => effect.startsWith('T:') || effect.startsWith('A:'));
        const label = `행 ${count}: 입력 ${row[0]}`;
        assert.equal(hex(records), row[1], label);
        assert.equal(effects.join(';'), native.join(';'), label);
        count += 1;
    }
    assert.equal(count, 144);
});

test('ForeignRitualReward: SP 덧셈의 ftol 절삭·owner 비트·기술 Core 의 무쓰기/진단 뒤 현재 읽기/물리 끝 거부가 cpppj 와 같다', () => {
    // SP: getter → +5000 → ftol 하위 signed DWORD → setter(owner signed32 비트, 값).
    const calls = [];
    const grant = (owner, balance) => grantForeignRitualSp(owner, () => balance, (selected, amount) => calls.push([selected, amount]));
    grant(2, 1.5);
    grant(0xffffffff, -5000.5);
    grant(3, Number.NaN);
    grant(4, 2 ** 70);
    grant(5, 4000000000);
    assert.deepEqual(calls, [[2, 5001], [-1, 0], [3, 0], [4, 0], [5, Number(BigInt.asIntN(32, 4000005000n))]]);
    assert.ok(throws(() => grantForeignRitualSp(1, null, () => {})));
    assert.ok(throws(() => grantForeignRitualSp(1, () => 1, null)));
    // 기술 Core: 비트 없는 TYPE(-1)은 장부나 진단을 요구하지 않는다.
    grantForeignRitualType({ bytes: new Uint8Array(0), origin: 0 }, 0xffffffff, 85, () => -1, null);
    // 진단 뒤 현재 DWORD 를 읽어 비트 31 을 켠다(이웃 필드는 그대로).
    const core = new Uint8Array(160);
    let diagnoses = 0;
    grantForeignRitualType({ bytes: core, origin: 0 }, 8, 85, () => 32, (line) => {
        assert.equal(line, 0x15a);
        diagnoses += 1;
        put(core, 8 * 16 + 10, 0x80000000);
    });
    assert.equal(diagnoses, 1);
    assert.equal(get(core, 8 * 16 + 10), 0x80000001);
    assert.equal(get(core, 8 * 16), 0);
    assert.equal(core[8 * 16 + 9], 0);
    assert.equal(core[8 * 16 + 14], 0);
    // 단일 DWORD 가 물리 끝을 넘으면 부분 BYTE 쓰기를 남기지 않는다.
    const short = new Uint8Array(13);
    assert.ok(throws(() => grantForeignRitualType({ bytes: short, origin: 0 }, 0, 85, () => 0, null)));
    assert.equal(hex(short), '00'.repeat(13));
    assert.ok(throws(() => grantForeignRitualType({ bytes: core, origin: 0 }, 0, 85, null, null)));
    assert.ok(throws(() => grantForeignRitualType({ bytes: core, origin: 0 }, 0, 85, () => -2, null)));
});
