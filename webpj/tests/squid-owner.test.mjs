// Squid 소유자 지정(SquidOwner·setCommonSquidOwner)과 postPop 장부 목록(SquidPostPopList)을 원본 10.78 관측으로 대조한다(WB04-E2, cpppj OwnerTests 이식).
//  - owner-x86.tsv 의 10.78(originals) Owner 행: 요새/전투/challenge 모드·genus·상태·extra·이전/새 소유자·소유자별 작업장 목록(용량:개수:6칸)을 입력으로,
//    소유자 BYTE·소유자별 개수와 저장 칸 전체(비활성 stale 꼬리 포함)·슬롯 Adler-32 를 비교한다. 기대값은 fixture 에서 읽는다.
//  - 계약: 범위 밖 소유자는 진단 장치가 없으면 쓰기 전에 거부되고, 유효 번호가 선택한 목록 물리(9 이상) 부재는 제거/BYTE 저장 뒤 늦게 거부된다.
//  - 목록 구조: 실제 Player+0/+4/+8 연결 binding·가득 찬 추가·안정 압축·손상 count 거부.
// 실제 Client Player 레코드(ClientPlayerRecords)를 쓰는 bound 재생과 CD/10.37 행은 범위 밖이다. (2026-11-08 WB04-E2: 소유자 지정 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, adler32 } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidOwner, SquidOwnerMode } from '../www/core/squid-owner.js';
import { SquidPostPopList, SquidPostPopState } from '../www/core/squid-postpop-state.js';

// 원본 관측 행(10.78 Owner 행만)이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E2)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'owner-x86.tsv'))?.filter((row) => row[0] === 'Owner' && row[1] === 'originals') ?? null;

// 분석 도구와 같은 합성 풀 크기·객체 번호·타입 번호·목록 저장 칸 수이다. 실제 게임 월드 배치를 뜻하지 않는다. (2026-11-08 WB04-E2)
const CAPACITY = 24000, OBJECT_TYPE = 82, LIST_SLOTS = 6, OWNERS = 9, TARGET = 50;
// 10.78 소유자·extra 필드 위치이다. (2026-11-08 WB04-E2)
const OWNER_OFFSET = 34, EXTRA_OFFSET = 40;

// 오류가 나는지 확인한다(성공을 무조건 인정하지 않음). (2026-11-08 WB04-E2)
function failed(action) {
    try {
        action();
        return false;
    } catch {
        return true;
    }
}

// 풀과 장부를 한 번 만들고 genus 값마다 타입 표가 다른 SquidOwner 를 만들어 두는 장면이다. (2026-11-08 WB04-E2)
class Scene {
    constructor() {
        this.pool = new SidPool(CAPACITY, { server: false });
        this.bookkeeping = new SquidPostPopState();
        this.mode = new SquidOwnerMode();
        this.owners = new Map();
        // 클라이언트 영역의 앞 슬롯들을 할당해 합성 번호 50을 준비한다.
        for (let index = 5; index <= 70; index += 1) assert.equal(this.pool.allocate(2), index);
    }

    // 타입 표는 SquidOwner 가 복사하므로 genus 값마다 하나씩 만들어 재사용한다.
    forGenus(genus) {
        if (!this.owners.has(genus)) {
            const types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
            types[OBJECT_TYPE].flags2 = genus;
            this.owners.set(genus, new SquidOwner(this.pool, types, this.bookkeeping, this.mode));
        }
        return this.owners.get(genus);
    }
}

// 한 줄이 한 입력이다: 슬롯과 목록을 입력으로 채우고 Set 한 뒤 소유자 BYTE·개수:저장 칸 전체·슬롯 Adler 를 비교한다.
test('SquidOwner: 원본10.78 소유자 지정 행(작업장 목록·stale 꼬리·슬롯 체크섬)', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new Scene();
    let count = 0;
    for (const row of rows) {
        assert.equal(row.length, 14);
        scene.mode.fort = Number(row[2]) !== 0;
        scene.mode.battle = Number(row[3]) !== 0;
        scene.mode.challenge = Number(row[4]) !== 0;
        // 슬롯을 0 으로 지우고 타입·상태·소유자·extra 만 채운다.
        const raw = scene.pool.allocated(TARGET);
        raw.fill(0);
        raw[10] = OBJECT_TYPE;
        raw[11] = Number(row[6]);
        raw[OWNER_OFFSET] = Number(row[8]);
        raw[EXTRA_OFFSET] = Number(row[7]);
        // 소유자마다 "용량:개수:항목들"이다. 용량까지만 장부 목록에 넣고 그 뒤 칸은 원본 저장소의 옛 메모리로 따로 든다.
        const lists = row[10].split('|');
        assert.equal(lists.length, OWNERS);
        const stale = Array.from({ length: OWNERS }, () => []);
        for (let owner = 0; owner < OWNERS; owner += 1) {
            const [capacity, used, items] = lists[owner].split(':');
            const entries = items.split(',');
            const list = scene.bookkeeping.ownerFactories[owner];
            list.entries.length = 0;
            // 앞쪽 용량만큼이 실제 목록 배열이다.
            for (let index = 0; index < LIST_SLOTS; index += 1) {
                const value = Number(entries[index]);
                if (index < Number(capacity)) list.entries.push(value);
                else stale[owner].push(value);
            }
            list.count = Number(used);
        }
        scene.forGenus(Number(row[5])).set(TARGET, Number(row[9]));
        // 결과: 소유자 BYTE, 소유자마다 "개수:저장 칸 전체", 슬롯 Adler-32.
        const actual = [];
        for (let owner = 0; owner < OWNERS; owner += 1) {
            const list = scene.bookkeeping.ownerFactories[owner];
            actual.push(`${list.readCount()}:${[...list.entries, ...stale[owner]].join(',')}`);
        }
        const after = scene.pool.slot(TARGET);
        const label = `소유자 ${row[8]}→${row[9]} genus ${row[5]} 모드 ${row[2]}${row[3]}${row[4]}`;
        assert.equal(after[OWNER_OFFSET], Number(row[11]), label);
        assert.equal(actual.join('|'), row[12], label);
        assert.equal(adler32(after), Number(row[13]), label);
        count += 1;
    }
    assert.ok(count >= 1000, `행 ${count}`);
});

// 실제 진단 장치 부재의 중단과 유효 번호가 선택한 목록 물리 부재의 늦은 중단을 구별한다.
test('SquidOwner: 범위 밖 소유자는 쓰기 전에 거부되고 목록 물리 부재는 늦게 거부된다', () => {
    const scene = new Scene();
    const raw = scene.pool.allocated(TARGET);
    raw.fill(0);
    raw[10] = OBJECT_TYPE;
    raw[OWNER_OFFSET] = 3;
    scene.mode.battle = true;
    const owner = scene.forGenus(0x4000);
    scene.bookkeeping.ownerFactories[3].entries = [TARGET, 0];
    scene.bookkeeping.ownerFactories[3].count = 1;
    assert.ok(failed(() => owner.set(TARGET, 9)));
    scene.mode.challenge = true;
    assert.ok(failed(() => owner.set(TARGET, 0)));
    assert.ok(failed(() => owner.set(TARGET, 20)));
    assert.ok(failed(() => owner.set(TARGET, 40)));
    // challenge 의 유효 20 은 제거/BYTE 뒤 목록 물리에서 중단되고, 무효 40 의 미연결 진단은 그 상태를 보존한다.
    assert.equal(scene.pool.slot(TARGET)[OWNER_OFFSET], 20);
    assert.equal(scene.bookkeeping.ownerFactories[3].count, 0);
});

// 진단 장치를 연결하면 범위 진단 뒤에도 하위 BYTE 를 쓰고 현재 TYPE 표(bindDiagnostics)를 매번 읽는다.
test('SquidOwner: 진단은 정상 복귀 뒤 하위 BYTE 를 쓰고 빌린 현재 TYPE 표를 읽는다', () => {
    const scene = new Scene();
    const raw = scene.pool.allocated(TARGET);
    raw.fill(0);
    raw[10] = OBJECT_TYPE;
    scene.mode.fort = true;
    const owner = scene.forGenus(0);
    const lines = [];
    const current = Array.from({ length: 188 }, () => ({ flags2: 0x4200 }));
    owner.bindDiagnostics({ types: current, assertion: (expression, file, line) => lines.push([file, line]) });
    // 빌린 현재 표의 genus 가 작업장이므로 소유자 3 의 목록에 추가된다(생성 때 사본은 0).
    scene.bookkeeping.ownerFactories[3].entries = [0, 0];
    owner.set(TARGET, 3);
    assert.deepEqual([scene.pool.slot(TARGET)[OWNER_OFFSET], scene.bookkeeping.ownerFactories[3].items()], [3, [TARGET]]);
    // 범위 밖 소유자 9 는 진단(0xa02)이 정상 복귀한 뒤 하위 BYTE 를 쓰고, 소유자 9 의 목록 물리가 없어 늦게 거부된다.
    assert.throws(() => owner.set(TARGET, 9), /Player 물리 범위/);
    assert.deepEqual(lines, [['\\Ns\\O\\Squid.cpp', 0xa02]]);
    assert.equal(scene.pool.slot(TARGET)[OWNER_OFFSET], 9);
    assert.throws(() => owner.bindDiagnostics({ types: [] }), RangeError);
});

// 목록: 가득 찬 추가는 배열을 건드리지 않고, 전체 제거는 상대 순서를 지키며 stale 꼬리를 보존하고, 손상 count 는 거부한다. 실제 Player 연결은 현재 +0/+4/+8 을 매번 읽는다.
test('SquidPostPopList: 추가·안정 압축·손상 count·현재 연결', () => {
    const list = new SquidPostPopList();
    list.entries = [7, 8, 9, 7];
    list.count = 3;
    list.append(1);
    assert.deepEqual([list.readCount(), list.entries], [4, [7, 8, 9, 1]]);
    list.append(2);
    assert.deepEqual([list.readCount(), list.entries], [4, [7, 8, 9, 1]], '가득 차면 추가하지 않는다');
    list.removeAll(9);
    assert.deepEqual([list.readCount(), list.entries], [3, [7, 8, 1, 1]], '압축 뒤 stale 꼬리 보존');
    list.removeAll(7);
    assert.deepEqual([list.items(), list.entries], [[8, 1], [8, 1, 1, 1]]);
    list.count = 5;
    assert.throws(() => list.items(), /count 범위 오류/);
    assert.throws(() => list.removeAll(1), /count 범위 오류/);
    // 실제 Player 연결: 현재 저장소/용량/개수를 매번 읽고 개수만 쓴다.
    const bound = new SquidPostPopList();
    let storage = new Uint32Array(3);
    let used = 0;
    bound.bindCurrent({ storage: () => storage, capacity: () => storage.length, readCount: () => used, writeCount: (value) => { used = value; } });
    bound.append(5);
    bound.append(6);
    assert.deepEqual([bound.readCount(), bound.items()], [2, [5, 6]]);
    storage = new Uint32Array([5, 6, 5]);
    used = 3;
    bound.removeAll(5);
    assert.deepEqual([used, bound.items()], [1, [6]]);
    assert.throws(() => bound.bindCurrent({}), /연결의 소유\/장치 오류/);
    assert.throws(() => new SquidPostPopState().ownerFactories[1].bindCurrent({ storage: 1 }), /연결의 소유\/장치 오류/);
});
