// 비로컬 의식 보상 raw-foreign-ritual-reward.js(grantForeignRitualSp·grantForeignRitualType)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: foreign-ritual-reward-x86.tsv 의 기술 모드(mode 0) 144행 — 입력 9 값(x87 제어·모드·owner·local·known·editor·spBits·기술 비트(signed)·seed)로 256바이트 Core 창 전체(owner 32비트 감김·비정렬 DWORD·이웃 보존·known 이웃 DWORD)와
//    효과 순서(T:TYPE 조회 / A:진단 0x15a 뒤 정상 복귀). SP 모드(mode 1)는 기본 53비트 제어(0x27f) 180행을 실제 분산 SP 저장소(ScrambledSpStore)와 현재 정책 getter/setter(readPlayerSpAmount/setPlayerSpAmount)를 거쳐
//    저장값·표시 갱신 횟수·이웃 지갑까지 대조한다(확장 정밀도 0x37f 180행은 감사 수만 센다. WB04-E371).
//  - 계약: SP 덧셈의 ftol 절삭(비유한/범위 밖은 0·signed32 감김)·owner 비트 변환·미연결 거부, 기술 Core 의 비트 없는 TYPE 무쓰기·진단 뒤 현재 DWORD 읽기·물리 끝 초과 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E106: 비로컬 의식 보상 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { readPlayerSpAmount, setPlayerSpAmount } from '../www/core/raw-fort-money-load.js';
import { ClientCoreDataGlobals } from '../www/core/client-core-data-globals.js';
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

// SP 모드 입력의 효과 열(S:owner:값 저장 이벤트, U 표시 갱신)에서 원본 저장값과 표시 횟수를 읽는다. (2026-11-13 WB04-E371)
function nativeSpEffects(effectText) {
    let stored = null;
    let owner = null;
    let refreshes = 0;
    // 원문 getter 호출(G)은 건너뛰고 저장(S)과 표시(U)만 센다.
    for (const effect of effectText.split(';')) {
        if (effect.startsWith('S:')) {
            const fields = effect.split(':');
            owner = Number(fields[1]);
            stored = Number(fields[2]);
        } else if (effect === 'U') refreshes += 1;
    }
    return { stored, owner, refreshes };
}

test('ForeignRitualReward: 원본 10.78 SP 모드 기본 53비트 180입력이 실제 분산 SP 저장소와 현재 정책 getter/setter 를 거쳐 저장값·표시 횟수·이웃 지갑까지 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    let extendedAudit = 0;
    // 원본 기본 0x27f 관측만 현재 제품의 계산과 대조하고 확장 정밀도(0x37f)는 감사 수만 센다.
    for (const row of rows) {
        const input = row[0].split(',');
        if (Number(input[1]) !== 1) continue;
        if (Number(input[0]) !== 0x27f) { extendedAudit += 1; continue; }
        assert.equal(row.length, 3);
        const owner = Number(BigInt.asUintN(32, BigInt(input[2])));
        const known = Number(input[4]);
        const local = Number(input[3]);
        const editor = Number(input[5]);
        const initial = Number(input[6]);
        // 같은 게임 SP 저장소와 난수를 쓰고 Native 경계가 돌려주는 원시 float 입력을 모든 owner 에 준비한다.
        const store = new ScrambledSpStore(new GameRandom(), () => 7);
        store.initialize();
        for (let player = 0; player < 9; player += 1) store.set(player, initial);
        const reads = { clientActive: () => known, localOwner: () => local, editor: () => editor };
        let gets = 0;
        let sets = 0;
        let refreshes = 0;
        grantForeignRitualSp(owner, (selected) => {
            gets += 1;
            assert.equal(selected, owner);
            return readPlayerSpAmount(selected, store, reads);
        }, (selected, amount) => {
            sets += 1;
            assert.equal(selected >>> 0, owner);
            setPlayerSpAmount(selected, amount, store, reads, () => { refreshes += 1; });
        });
        const native = nativeSpEffects(row[2]);
        const label = `행 ${count}: 입력 ${row[0]}`;
        assert.ok(gets === 1 && sets === 1, label);
        assert.equal(refreshes, native.refreshes, label);
        const written = native.stored !== null;
        assert.equal(written, owner > 0 && owner < 9, label);
        // 저장된 owner 칸만 원본 값이 되고 이웃 지갑은 처음 입력 그대로이다.
        for (let player = 0; player < 9; player += 1) assert.equal(store.get(player), player === owner && written ? native.stored : initial, `${label} 지갑 ${player}`);
        count += 1;
    }
    assert.ok(count === 180 && extendedAudit === 180);
});

test('ForeignRitualReward: SP getter 는 비활성/비로컬 0 에서 lazy 저장소를 읽지 않고 활성 로컬은 비교 뒤 현재 owner 를 다시 조회한다', () => {
    const store = new ScrambledSpStore(new GameRandom(), () => 7);
    // 비활성 로컬 0 과 활성이지만 비로컬 0xffffffff 는 저장소를 초기화하지 않고 0 이다.
    assert.equal(readPlayerSpAmount(0, store, { clientActive: () => 0, localOwner: () => 0, editor: () => 0 }), 0);
    assert.equal(store.initialized, false);
    assert.equal(readPlayerSpAmount(0xffffffff, store, { clientActive: () => 1, localOwner: () => 2, editor: () => 0 }), 0);
    assert.equal(store.initialized, false);
    store.set(8, floatToBits(123.75));
    let localReads = 0;
    // 활성 로컬이 owner 와 같으면 비교에 한 번, 저장소 조회에 다시 한 번 읽어 두 번째 값(8)의 지갑을 돌려준다.
    const balance = readPlayerSpAmount(0, store, { clientActive: () => 1, localOwner: () => (localReads++ ? 8 : 0), editor: () => 0 });
    assert.equal(balance, 123.75);
    assert.equal(localReads, 2);
});

test('ForeignRitualReward: 기술 Core 쓰기가 Client 수명의 실제 9기록 장부를 같은 주소로 그대로 쓰고 이웃 필드를 보존한다', () => {
    const globals = new ClientCoreDataGlobals();
    const physical = globals.records();
    let diagnoses = 0;
    // 진단 뒤 현재 장부의 DWORD 를 읽어 비트 31 을 보존한 채 비트 0 을 켠다(32 는 마스크 shift 로 0).
    grantForeignRitualType({ bytes: globals.records(), origin: 0 }, 8, 85, () => 32, (line) => {
        assert.equal(line, 0x15a);
        diagnoses += 1;
        put(globals.records(), 8 * 16 + 10, 0x80000000);
    });
    assert.equal(diagnoses, 1);
    assert.equal(get(globals.records(), 8 * 16 + 10), 0x80000001);
    assert.equal(globals.records(), physical);
    assert.equal(get(globals.records(), 8 * 16), 0);
    assert.equal(globals.records()[8 * 16 + 9], 0);
    assert.equal(globals.records()[8 * 16 + 14], 0);
    // 비트 없는 TYPE 은 실제 장부나 진단을 요구하지 않는다.
    grantForeignRitualType({ bytes: new Uint8Array(0), origin: 0 }, 0xffffffff, 85, () => -1, null);
});

test('ForeignRitualReward: 기술 DWORD 가 물리 장부 끝에 정확히 들어맞으면 쓰고 한 바이트라도 모자라면 거부한다', () => {
    // origin 0·owner 0 이면 기술 DWORD 는 +10..+13 이므로 길이 14 가 정확히 맞는 경계이다.
    const exact = new Uint8Array(14);
    grantForeignRitualType({ bytes: exact, origin: 0 }, 0, 85, () => 3, null);
    assert.equal(hex(exact), `${'00'.repeat(10)}08000000`);
    // 같은 owner 를 origin 으로 옮겨도 끝에 정확히 맞으면 허용된다(origin 2 + 길이 16).
    const shifted = new Uint8Array(16);
    grantForeignRitualType({ bytes: shifted, origin: 2 }, 0, 85, () => 31, null);
    assert.equal(hex(shifted), `${'00'.repeat(12)}00000080`);
    const short = new Uint8Array(13);
    assert.ok(throws(() => grantForeignRitualType({ bytes: short, origin: 0 }, 0, 85, () => 3, null)));
    assert.equal(hex(short), '00'.repeat(13));
    // 시작 위치가 길이와 같거나 넘어도 거부한다.
    assert.ok(throws(() => grantForeignRitualType({ bytes: new Uint8Array(10), origin: 0 }, 0, 85, () => 3, null)));
});
