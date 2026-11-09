// checking(진단 모드) 전용 순수 진단 raw-checking-diagnostics.js(공통 Form 검사 004ad730·총의 noIsland 소유자 진단 0046b830)를 원본 10.78 관측과 cpppj `CheckingConsumersTests` 계약으로 대조한다.
//  - 원본 관측 checking-forms-x86.tsv 288행: 부모/자식/마지막 후보의 TYPE·상태 조합과 head 개수(0/1/2)에서 자산 부모·free/void 후보 판정(dead 허용)의 결과가 같고 풀이 변하지 않는다.
//  - 원본 관측 checking-island-owner-x86.tsv 192행: checking·부모 소유자·noIsland TYPE·첫 진단 뒤 변경·발자국 상한 세 가지에서 진단 횟수와 진단 뒤 checking/noIsland/부모 소유자, 발자국 상한 정수 변환(float 편향 포함)이 같다.
//  - 계약: 죽은 Form 자식은 허용, 자기 자신을 가리키는 체인·풀 밖 next 는 호스트가 거부, checking 이 꺼져 있으면 finder 를 만들지 않음.
// cpppj 의 앞선 세 시험(RawCheckingFlag 의 DWORD 뷰 수명)은 C++ 참조 수명 계약이라 웹에서는 getter 함수로 대체한다. 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E240: checking 진단 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { checkRawObjectForms, checkingInteger, checkingStoredInteger, diagnoseRawGunIslandOwners } from '../www/core/raw-checking-diagnostics.js';

// 저장된 독립 기계어 출력을 한 번만 읽는다. (2026-11-10 WB04-E240)
const fixtures = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const formRows = readRows(path.join(fixtures, 'checking-forms-x86.tsv'));
const ownerRows = readRows(path.join(fixtures, 'checking-island-owner-x86.tsv'));

// 쉼표 구분 수 목록(원시 uint32 입력을 signed 로 줄이지 않는다)이다. (2026-11-10 WB04-E240)
const numbers = (text) => text.split(',').map(Number);
// 예외가 나는지 확인한다(호스트 물리 범위/순환은 가짜 성공 대신 명시 실패한다). (2026-11-10 WB04-E240)
function fails(operation) {
    try { operation(); } catch { return true; }
    return false;
}

test('checking Form 검사: 원본 10.78 288행(자산 부모·free/void 자식·dead 허용·head 개수 0/1/2)의 결과가 같고 풀은 변하지 않는다', { skip: !formRows && '원본 fixture 없음' }, () => {
    const pool = new SidPool(24000);
    const parent = pool.allocate();
    const child = pool.allocate();
    const last = pool.allocate(); // 일반 raw 슬롯만 할당하며 TYPE factory/Pop/게임 Frame 은 실행하지 않는다.
    const p = pool.allocated(parent);
    const c = pool.allocated(child);
    const d = pool.allocated(last);
    let rows = 0;
    for (const fields of formRows) {
        const v = numbers(fields[0]);
        assert.equal(v.length, 7);
        rows += 1;
        p[10] = v[0];
        p[11] = v[1];
        put(p, 6, v[2] ? child : 0, 2);
        c[10] = v[3];
        c[11] = v[4];
        put(c, 4, v[2] === 2 ? last : 0, 2);
        d[10] = v[5];
        d[11] = v[6];
        put(d, 4, 0, 2);
        const before = pool.bytes();
        assert.equal(checkRawObjectForms(pool, parent), fields[1] === '1', `행 ${rows} 입력 ${fields[0]}`);
        assert.deepEqual(pool.bytes(), before, `행 ${rows} 풀 불변`);
    }
    assert.equal(rows, 288);
});

test('checking Form 검사 계약: dead 인 Form 자식은 허용하고 자기 순환 체인과 풀 밖 next 는 호스트가 명시 거부한다', () => {
    const pool = new SidPool(24000);
    const parent = pool.allocate();
    const child = pool.allocate();
    const p = pool.allocated(parent);
    const c = pool.allocated(child);
    p[10] = 70;
    p[11] = 0;
    put(p, 6, child, 2);
    c[10] = 6;
    c[11] = 2;
    put(c, 4, 0, 2);
    assert.ok(checkRawObjectForms(pool, parent)); // dead 2 의 Form 은 원문이 허용한다.
    put(c, 4, child, 2);
    assert.ok(fails(() => checkRawObjectForms(pool, parent)));
    put(c, 4, 65534, 2);
    assert.ok(fails(() => checkRawObjectForms(pool, parent)));
    // 자식은 TYPE ≤ 69 의 Form 이어야 한다: 69 는 참, 자산 TYPE 70 은 거짓. 부모가 free(1)이거나 void(4)여도 거짓이고, 자산이 아닌 부모(TYPE 69)도 거짓이다.
    put(c, 4, 0, 2);
    c[10] = 69;
    assert.ok(checkRawObjectForms(pool, parent));
    c[10] = 70;
    assert.ok(!checkRawObjectForms(pool, parent));
    c[10] = 6;
    p[11] = 1;
    assert.ok(!checkRawObjectForms(pool, parent));
    p[11] = 4;
    assert.ok(!checkRawObjectForms(pool, parent));
    p[11] = 0;
    p[10] = 69;
    assert.ok(!checkRawObjectForms(pool, parent));
});

test('총 noIsland 소유자 진단: 원본 10.78 192행(진단 횟수·진단 뒤 전역/부모 소유자·발자국 상한 정수 변환)이 모두 같다', { skip: !ownerRows && '원본 fixture 없음' }, () => {
    const pool = new SidPool(24000);
    const parent = pool.allocate();
    const first = pool.allocate();
    const second = pool.allocate();
    const last = pool.allocate();
    const p = pool.allocated(parent);
    const a = pool.allocated(first);
    const b = pool.allocated(second);
    const c = pool.allocated(last);
    const hash = new SquidHash();
    const types = Array.from({ length: 188 }, () => ({ footX: 0, footY: 0 }));
    for (const sid of [first, second, last]) { // 모두 같은 finder 영역의 합성 표면이며 게임 가상 생성/Pop 은 호출하지 않는다.
        const raw = pool.allocated(sid);
        raw[11] = 0;
        raw[40] = 0;
        put(raw, 14, 0x41a40000); // 20.5f
        put(raw, 18, 0x41a40000);
    }
    types[98].footX = types[98].footY = types[99].footX = types[99].footY = 1;
    hash.setBucket(0, 20.5, 20.5, first);
    put(a, 4, second, 2);
    put(b, 4, last, 2);
    put(c, 4, 0, 2);
    let rows = 0;
    for (const fields of ownerRows) {
        const v = numbers(fields[0]);
        assert.ok(v.length === 5 && fields.length === 6);
        rows += 1;
        let checking = v[0];
        let noIsland = v[2];
        let calls = 0;
        p[34] = v[1];
        a[10] = b[10] = 99;
        c[10] = 98;
        a[34] = b[34] = 2;
        c[34] = 3;
        const upper = Math.fround(v[4] === 0 ? 21.0 : v[4] === 1 ? 21.000002 : 21.00002);
        const footprint = { left: 20.25, top: 20.25, right: upper, bottom: upper };
        diagnoseRawGunIslandOwners(pool, hash, types, parent, footprint, () => checking, () => noIsland, (number) => {
            assert.equal(number, 0xad);
            calls += 1;
            if (calls === 1) { if (v[3] === 1) checking = 0; if (v[3] === 2) p[34] = 2; if (v[3] === 3) noIsland = 98; }
        });
        const label = `행 ${rows} 입력 ${fields[0]}`;
        assert.equal(calls, Number(fields[1]), `${label} 진단 횟수`);
        assert.equal(checking, Number(fields[2]), `${label} checking`);
        assert.equal(noIsland, Number(fields[3]), `${label} noIsland`);
        assert.equal(p[34], Number(fields[4]), `${label} 부모 소유자`);
        if (checking || fields[5] !== '-') {
            const area = numbers(fields[5]);
            assert.equal(area.length, 4);
            assert.equal(checkingStoredInteger(upper, true), area[2] | 0, `${label} 상한`);
        }
    }
    assert.equal(rows, 192);
});

test('총 noIsland 소유자 진단 계약: checking 이 꺼져 있으면 finder 를 만들지 않고 진단 장치 누락은 진단 지점에서 거부하며 float 정수 변환은 불확정값의 하위 0 으로 감긴다', () => {
    const pool = new SidPool(24000);
    const hash = new SquidHash();
    // 꺼져 있으면 풀/해시/발자국을 읽기 전에 반환한다(잘못된 인자를 넘겨도 오류가 없다).
    diagnoseRawGunIslandOwners(null, null, null, 0, null, () => 0, () => 0, undefined);
    // 정수 변환: 비유한·64비트 밖은 0, 정상은 DWORD 감김, 상한은 편향 float 를 더하고 다시 ftol 한다.
    assert.deepEqual([Infinity, -Infinity, NaN, 2 ** 63, -(2 ** 63) - 4096].map(checkingInteger), [0, 0, 0, 0, 0]);
    assert.equal(checkingInteger(2 ** 32 + 5), 5);
    assert.equal(checkingInteger(-(2 ** 62) - 4096), -4096); // 64비트 안의 큰 음수도 하위 32비트가 정상 DWORD 로 감긴다.
    assert.equal(checkingInteger(-1.9), -1);
    assert.equal(checkingStoredInteger(20.25, false), 20);
    assert.equal(checkingStoredInteger(21.0, true), 21);
    assert.equal(checkingStoredInteger(-0.5, true), 0);
    // 진단 장치가 없으면 소유자가 다른 noIsland 후보를 만난 지점에서 거부한다.
    const parent = pool.allocate();
    const other = pool.allocate();
    pool.allocated(parent)[34] = 1;
    const raw = pool.allocated(other);
    raw[10] = 99;
    raw[34] = 2;
    put(raw, 14, 0x41a40000);
    put(raw, 18, 0x41a40000);
    hash.setBucket(0, 20.5, 20.5, other);
    const types = Array.from({ length: 188 }, () => ({ footX: 1, footY: 1 }));
    const footprint = { left: 20.25, top: 20.25, right: 21, bottom: 21 };
    assert.throws(() => diagnoseRawGunIslandOwners(pool, hash, types, parent, footprint, () => 1, () => 99, undefined), TypeError);
    // 소유자가 같거나 noIsland TYPE 이 아니면 진단하지 않는다.
    raw[34] = 1;
    let calls = 0;
    diagnoseRawGunIslandOwners(pool, hash, types, parent, footprint, () => 1, () => 99, () => { calls += 1; });
    raw[34] = 2;
    diagnoseRawGunIslandOwners(pool, hash, types, parent, footprint, () => 1, () => 98, () => { calls += 1; });
    assert.equal(calls, 0);
});

test('총 noIsland 소유자 진단 계약: 발자국 오른쪽·아래는 상한 편향으로 한 칸 넓게 읽어 그 칸의 후보도 진단하고 왼쪽·위는 편향 없이 자른다', () => {
    const run = (x, y, footprint) => {
        const pool = new SidPool(24000);
        const hash = new SquidHash();
        const types = Array.from({ length: 188 }, () => ({ footX: 1, footY: 1 }));
        const parent = pool.allocate();
        const other = pool.allocate();
        pool.allocated(parent)[34] = 1;
        const raw = pool.allocated(other);
        raw[10] = 99;
        raw[34] = 2;
        const view = new DataView(new ArrayBuffer(4));
        view.setFloat32(0, x, true);
        put(raw, 14, view.getUint32(0, true));
        view.setFloat32(0, y, true);
        put(raw, 18, view.getUint32(0, true));
        hash.setBucket(0, x, y, other);
        let calls = 0;
        diagnoseRawGunIslandOwners(pool, hash, types, parent, footprint, () => 1, () => 99, () => { calls += 1; });
        return calls;
    };
    // 오른쪽 20.5 는 편향으로 21 칸까지 읽으므로 x = 21.5 의 후보를 진단하고, 아래 20.5 도 y = 21.5 의 후보를 진단한다.
    assert.equal(run(21.5, 20.5, { left: 20.25, top: 20.25, right: 20.5, bottom: 20.5 }), 1);
    assert.equal(run(20.5, 21.5, { left: 20.25, top: 20.25, right: 20.5, bottom: 20.5 }), 1);
    // 왼쪽·위는 편향 없이 20 칸으로 자르므로 20.25 가 아닌 21 칸 이상의 후보만 있으면(왼쪽 21.25) 그 칸 왼쪽의 후보 x = 20.5 는 영역 밖이다.
    assert.equal(run(20.5, 20.5, { left: 21.25, top: 20.25, right: 22, bottom: 21 }), 0);
    assert.equal(run(20.5, 20.5, { left: 20.25, top: 21.25, right: 21, bottom: 22 }), 0);
});

