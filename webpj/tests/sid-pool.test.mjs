// 원본10.78의 전체 풀/삭제 기록 관측과 SID 수명 경계를 검증한다. (2026-11-04 WB04-A)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { SidPool } from '../www/core/sid-pool.js';

// 기존 원본 실행 TSV의 위치다. CD판 세션은 이번10.78 이식 검사에 포함하지 않는다.
const FIXTURE = path.join(REPO_ROOT, 'cpppj/tests/fixtures/sid-x86.tsv');
// 원본50바이트 슬롯에서 타입·상태·next를 읽는 위치와 세 영역의 경계다.
const TYPE = 10, STATE = 11, NEXT = 4, STRIDE = 50, SERVER = 15000, PREDICTABLE = 23001;
// Adler-32의 표준 소수와 중간 누적량을 제한하는 블록 크기다.
const ADLER_PRIME = 65521, ADLER_BLOCK = 4096;

// Python zlib가 기록한 기대값과 대조할 실제 바이트의 Adler-32를 계산한다.
function adler(bytes) {
    let a = 1, b = 0;
    // 긴 풀도 Number 정수의 정확한 범위 안에서 블록마다 나머지를 구한다.
    for (let start = 0; start < bytes.length; start += ADLER_BLOCK) {
        const end = Math.min(start + ADLER_BLOCK, bytes.length);
        // 바이트 순서를 바꾸지 않고 표준 두 누적합을 적용한다.
        for (let index = start; index < end; index += 1) { a += bytes[index]; b += a; }
        a %= ADLER_PRIME;
        b %= ADLER_PRIME;
    }
    return ((b << 16) | a) >>> 0;
}

// 두20항목 삭제 기록의 DWORD를 client→other 순서로 직렬화한다.
function historyBytes(pool) {
    const bytes = new Uint8Array(320);
    const view = new DataView(bytes.buffer);
    let offset = 0;
    // 영역별 최신 기록부터 각 SID/type DWORD를 쓴다.
    for (const client of [true, false]) {
        // 원본 기록 배열의20쌍을 모두 검사하며0도 생략하지 않는다.
        for (const record of pool.deletions(client)) {
            view.setUint32(offset, record.sid, true);
            view.setUint32(offset + 4, record.type, true);
            offset += 8;
        }
    }
    assert.equal(offset, bytes.length);
    return bytes;
}

// 원본 관측9열과 같은 순서로 반환값·전역·전체 저장소 해시를 만든다.
function snapshot(pool, result = 0) {
    return [result, pool.freeCount, pool.predictableCursor, pool.firstFree(true), pool.tail(true),
        pool.firstFree(false), pool.tail(false), adler(pool.bytes()), adler(historyBytes(pool))];
}

// payload 삭제와 FIFO를 관측할 명시적인 파생 생성자 입력만 쓴다.
function fill(pool, sid, seed, state, type) {
    const bytes = pool.allocated(sid);
    // native 관측기에 공급한 입력 식을 그대로 옮기며 기대값은 생성하지 않는다.
    for (let index = 0; index < bytes.length; index += 1) bytes[index] = (seed + index * 37) & 255;
    bytes[TYPE] = type;
    bytes[STATE] = state;
}

// 오류가 카운터·원시 저장소·이력을 바꾸기 전에 발생하는지 검사한다.
function rejectsWithoutMutation(pool, operation) {
    const before = snapshot(pool);
    assert.throws(operation);
    assert.deepEqual(snapshot(pool), before);
}

// 원본 실행1584전이의 전체 바이트와 기록을 fixture 출력만으로 대조한다.
test('SID 풀: 원본10.78의8세션·1584전이·768입력 payload를 재생한다', { skip: !fs.existsSync(FIXTURE) }, () => {
    let pool = null, patch = false, begins = 0, steps = 0, fills = 0;
    // 판본별 Begin을 따르며 입력 Fill과 출력 Step을 구분한다.
    for (const [index, line] of fs.readFileSync(FIXTURE, 'utf8').split(/\r?\n/).entries()) {
        if (!line || line.startsWith('#')) continue;
        const row = line.split('\t');
        if (row[0] === 'Begin') {
            assert.equal(row.length, 4);
            patch = row[1] === 'originals';
            pool = patch ? new SidPool(Number(row[2]), { server: row[3] === '1' }) : null;
            if (patch) begins += 1;
            continue;
        }
        if (!patch) continue;
        assert.ok(pool);
        if (row[0] === 'Fill') {
            assert.equal(row.length, 5);
            fill(pool, Number(row[1]), Number(row[2]), Number(row[3]), Number(row[4]));
            fills += 1;
            continue;
        }
        assert.equal(row.length, 12);
        const argument = Number(row[2]);
        let result = 0;
        if (row[1] === 'Reset') pool.reset();
        else if (row[1] === 'Allocate') result = pool.allocate(argument);
        else if (row[1] === 'Release') pool.release(argument);
        else if (row[1] === 'Rebuild') pool.rebuildServer();
        else assert.fail(`알 수 없는 원본 SID 명령: ${row[1]}`);
        assert.deepEqual(snapshot(pool, result), row.slice(3).map(Number), `원본 TSV ${index + 1}행 ${row[1]}`);
        steps += 1;
    }
    assert.deepEqual({ begins, steps, fills }, { begins: 8, steps: 1584, fills: 768 });
});

// 일반 영역의 마지막 free를 예약하며 반납 번호를 FIFO 꼬리로 잇는지 확인한다.
test('SID 풀: 일반 클라이언트 소진 뒤 예약 꼬리와 FIFO를 보존한다', () => {
    const pool = new SidPool(23300);
    const initial = pool.freeCount;
    // 5..14998만 할당하고14999는 원본 예약 꼬리로 남긴다.
    for (let sid = 5; sid < SERVER - 1; sid += 1) assert.equal(pool.allocate(2), sid);
    assert.equal(pool.firstFree(true), SERVER - 1);
    assert.equal(pool.tail(true), SERVER - 1);
    rejectsWithoutMutation(pool, () => pool.allocate(2));
    pool.allocated(8)[TYPE] = 77;
    pool.release(8);
    assert.equal(pool.tail(true), 8);
    assert.equal(pool.slot(8)[TYPE], 77);
    assert.equal(pool.slot(8)[STATE], 3);
    assert.equal(pool.allocate(2), SERVER - 1);
    assert.equal(pool.firstFree(true), 8);
    rejectsWithoutMutation(pool, () => pool.allocate(2));
    assert.equal(pool.freeCount, initial - (SERVER - 6));
});

// 예측 플래그 우선순위·서버 마지막 번호 예약·클라이언트 반납 의미를 검사한다.
test('SID 풀: 예측 커서와 서버/클라이언트 경계를 보존한다', () => {
    const server = new SidPool(23005);
    const first = server.allocate(3);
    assert.equal(first, PREDICTABLE + 1);
    assert.equal(server.allocate(1), PREDICTABLE + 2);
    const count = server.freeCount;
    server.release(first);
    assert.equal(server.slot(first)[STATE], 3);
    assert.equal(server.freeCount, count);
    rejectsWithoutMutation(server, () => server.allocate(1));
    assert.equal(server.predictableCursor, 3);
    const client = new SidPool(23004, { server: false });
    const predicted = client.allocate(1);
    assert.equal(predicted, PREDICTABLE + 1);
    const clientCount = client.freeCount;
    client.release(predicted);
    assert.equal(client.slot(predicted)[STATE], 1);
    assert.equal(client.freeCount, clientCount);
    assert.equal(client.allocate(1), PREDICTABLE + 2);
    rejectsWithoutMutation(client, () => client.allocate(1));
    rejectsWithoutMutation(client, () => client.allocate(0));
});

// 재구성 카운터를 free 재계산으로 바꾸면 원본과 달라지는 경계를 확인한다.
test('SID 풀: 서버 free 목록 재구성은 번호순이며 카운터에 누적한다', () => {
    const pool = new SidPool(23300);
    assert.equal(pool.allocate(), SERVER);
    assert.equal(pool.allocate(), SERVER + 1);
    pool.release(SERVER);
    const before = pool.freeCount;
    pool.rebuildServer();
    assert.equal(pool.firstFree(false), SERVER);
    assert.equal(pool.tail(false), PREDICTABLE - 1);
    assert.equal(pool.freeCount, before + (PREDICTABLE - SERVER - 1));
    assert.equal(pool.allocate(), SERVER);
    assert.equal(pool.allocate(), SERVER + 2);
});

//20개를 넘긴 삭제 기록의 이동과 반납 시 payload 제거를 검증한다.
test('SID 풀: 반납은 타입만 남기며 영역별 최신20개 이력을 보존한다', () => {
    const pool = new SidPool(23300);
    // client와other 각각21번 반납하여 오래된 첫 항목이 사라지는지 확인한다.
    for (const flags of [2, 0]) {
        const ids = [];
        // 명시적인 비어 있지 않은 payload로 슬롯 전체 삭제를 관측한다.
        for (let index = 0; index < 21; index += 1) {
            const sid = pool.allocate(flags);
            fill(pool, sid, 83 + index, 6, 101 + index);
            ids.push(sid);
        }
        // 할당 순서대로 반납하면 최근20개가 역순으로 남아야 한다.
        for (const sid of ids) pool.release(sid);
        const records = pool.deletions(flags === 2);
        assert.equal(records.length, 20);
        // 생성 당시 type을 기록하고 가장 오래된 항목 하나만 버린다.
        for (let index = 0; index < 20; index += 1) {
            assert.deepEqual(records[index], { sid: ids[20 - index], type: 121 - index });
        }
        const freed = pool.slot(ids[0]);
        // 처음 반납한 슬롯의 next는 뒤에 반납한 번호로 이어지므로 별도로 검사한다.
        assert.equal(freed[NEXT] | (freed[NEXT + 1] << 8), ids[1]);
        // 타입·상태·FIFO next 외의50바이트 payload는 전부 지워져야 한다.
        for (let byte = 0; byte < freed.length; byte += 1) {
            if (byte === NEXT || byte === NEXT + 1) continue;
            assert.equal(freed[byte], byte === TYPE ? 101 : (byte === STATE ? 3 : 0));
        }
    }
});

// 하위 Reset은 삭제 기록을 보존하고0/음수는 같은 저장소를 재사용한다.
test('SID 풀: 재초기화/재확보의 저장소 수명과 삭제 기록 수명을 구분한다', () => {
    const pool = new SidPool(23300);
    const sid = pool.allocate();
    fill(pool, sid, 91, 4, 123);
    const oldView = pool.allocated(sid);
    pool.release(sid);
    const history = historyBytes(pool);
    pool.reset();
    assert.equal(pool.allocate(), SERVER);
    assert.equal(pool.allocated(SERVER).buffer, oldView.buffer);
    assert.deepEqual(historyBytes(pool), history);
    pool.reset(-17);
    assert.equal(pool.allocate(), SERVER);
    assert.equal(pool.allocated(SERVER).buffer, oldView.buffer);
    assert.deepEqual(historyBytes(pool), history);
    pool.reset(40000);
    assert.equal(pool.capacity, 40000);
    assert.equal(pool.freeCount, 39995);
    assert.equal(pool.predictableCursor, 1);
    assert.equal(pool.allocate(), SERVER);
    const current = pool.allocated(SERVER);
    assert.notEqual(current.buffer, oldView.buffer);
    assert.deepEqual(historyBytes(pool), history);
    oldView[TYPE] = 99;
    assert.equal(current[TYPE], 0);
    const before = snapshot(pool);
    pool.clearDeletionHistory();
    assert.deepEqual(snapshot(pool).slice(0, 8), before.slice(0, 8));
    assert.deepEqual(historyBytes(pool), new Uint8Array(320));
});

// authority를 생성 당시 복사하면 권한 전환 뒤 반납/할당이 달라짐을 검사한다.
test('SID 풀: 공유 서버 권한을 호출마다 다시 읽는다', () => {
    const serverOwner = { value: false };
    const pool = new SidPool(23300, { server: true, serverOwner });
    assert.equal(pool.isServer, false);
    rejectsWithoutMutation(pool, () => pool.allocate());
    serverOwner.value = true;
    assert.equal(pool.isServer, true);
    const sid = pool.allocate();
    const count = pool.freeCount;
    serverOwner.value = false;
    pool.release(sid);
    assert.equal(pool.slot(sid)[STATE], 1);
    assert.equal(pool.freeCount, count);
    serverOwner.value = true;
    pool.rebuildServer();
    assert.equal(pool.allocate(), sid);
    const independent = new SidPool(23300, { server: false });
    assert.equal(independent.isServer, false);
});

// 공개 읽기는 복사본이고allocated만 실제 현재 슬롯을 빌리는지 확인한다.
test('SID 풀: 읽기 복사본의 쓰기가 실제 저장소와 기록을 바꾸지 않는다', () => {
    const pool = new SidPool();
    assert.equal(pool.edition, 'patch1078');
    assert.deepEqual(pool.layout, { stride: STRIDE, serverFirst: SERVER, predictableFirst: PREDICTABLE });
    const sid = pool.allocate(2);
    pool.allocated(sid)[TYPE] = 123;
    const slot = pool.slot(sid), bytes = pool.bytes();
    slot[TYPE] = 8;
    bytes[sid * STRIDE + TYPE] = 9;
    assert.equal(pool.slot(sid)[TYPE], 123);
    pool.release(sid);
    const records = pool.deletions(true);
    records[0].type = 1;
    records.push({ sid: 5, type: 2 });
    assert.deepEqual(pool.deletions(true)[0], { sid, type: 123 });
    assert.equal(pool.deletions(true).length, 20);
});

// JS의 넓은 입력값을 잘못된 포인터 산술로 바꾸지 않고 첫 변경 전에 거부한다.
test('SID 풀: 잘못된 크기·SID·할당 상태는 저장소 변경 전에 거부한다', () => {
    // 원본 고정 영역을 담지 못하거나16비트 번호 범위를 넘는 크기는 금지한다.
    for (const capacity of [0, 23002, 65536, -1, 23300.5, NaN, Infinity, '23300']) {
        assert.throws(() => new SidPool(capacity));
    }
    const pool = new SidPool(23300);
    // raw 읽기/쓰기/반납의 범위 검사는 같은 SID 계약을 사용한다.
    for (const sid of [-1, 23300, 65536, 5.5, NaN, Infinity, '5']) {
        rejectsWithoutMutation(pool, () => pool.slot(sid));
        rejectsWithoutMutation(pool, () => pool.allocated(sid));
        rejectsWithoutMutation(pool, () => pool.release(sid));
    }
    // 예약/미할당 슬롯은 유효 물리 번호라도allocated 경로로 쓸 수 없다.
    for (const sid of [0, 1, 2, 3, 4, 5, PREDICTABLE]) {
        rejectsWithoutMutation(pool, () => pool.allocated(sid));
        rejectsWithoutMutation(pool, () => pool.release(sid));
    }
    // 잘못된 재확보 인자는 현재 버퍼와 삭제 기록을 유지한다.
    for (const capacity of [1, 23002, 65536, 23300.5, NaN, Infinity, '23300']) {
        rejectsWithoutMutation(pool, () => pool.reset(capacity));
    }
    const sid = pool.allocate(2);
    pool.allocated(sid)[STATE] = 0;
    rejectsWithoutMutation(pool, () => pool.release(sid));
    pool.allocated(sid)[STATE] = 4;
    pool.release(sid);
    rejectsWithoutMutation(pool, () => pool.release(sid));
});
