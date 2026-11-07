// Player CoreData 조회·선택/정리(readPlayerCoreData·selectPlayerCoreData)와 복원(restorePlayerCoreData)을 원본 10.78 관측과 소스 계약으로 검사한다(WB04-E3, cpppj PlayerCoreDataTests 이식).
//  - player-core-data-x86.tsv 804행(x87 정밀도 두 제어값 × 402 입력): 유효 비영 Player 선택, NULL/짧은 현재 source, 진단 변경/중단의 실제 포인터·캐시·부분 쓰기·가변 alias 를
//    Player 1620 BYTE·캐시 10 BYTE·외부 16 BYTE 전체와 trace 로 대조한다.
//  - 계약: 반환 뷰의 쓰기가 실제 Player/캐시에 반영, 범위 밖 Player 물리는 앞선 store 뒤 거부, 복원의 local/remote 경로·진단 뒤 Player base 재조회·저장 게임 BYTE/Chat 재초기화(복원은 cpppj 직접 관측 없음).
// (2026-11-08 WB04-E3: Player CoreData 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { readPlayerCoreData, restorePlayerCoreData, samePointer, selectPlayerCoreData } from '../www/core/raw-player-core-data.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E3)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'player-core-data-x86.tsv'));

// NUL 뒤 바이트를 포함한 전체 물리 비교용 소문자 hex 이다. (2026-11-08 WB04-E3)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 명시 assertion 중단을 물리 fault 와 분리한다. (2026-11-08 WB04-E3)
class CoreStop extends Error {}

// 실제 원시 입력과 동일 배열/슬롯만 제공하며 부모 제어 흐름의 기대를 계산하지 않는다. 이 객체 자체가 globals 이다(원문 전역을 속성으로 직접 빌림). (2026-11-08 WB04-E3)
class CoreDevice {
    constructor(values) {
        assert.equal(values.length, 10);
        // 실제 9×180 BYTE 이며 선택한 +6c 도 같은 물리이다. 캐시 10 BYTE, 짧은 실제 source 의 별도 물리이다.
        this.playerBytes = new Uint8Array(1620);
        this.cached = new Uint8Array(10);
        this.external = new Uint8Array(16);
        this.players = this.playerBytes;
        this.current = null;
        this.templateOwner = 66;
        this.battleOwner = 55;
        this.playerKnown = 0;
        this.active = 0;
        this.metamap = 0;
        this.trace = [];
        this.mode = Number(values[0]);
        const seed = Number(values[1]);
        const kind = Number(values[2]);
        const length = Number(values[3]);
        this.metamap = Number(values[4]);
        this.playerKnown = Number(values[5]);
        this.active = Number(values[6]);
        this.ownerArg = Number(values[7]);
        this.templateArg = Number(values[8]);
        this.mutation = values[9];
        // 전체 Player 미작성 BYTE 도 비교하고 부분 캐시 쓰기와 외부 source 를 구별한다.
        for (let byte = 0; byte < this.playerBytes.length; byte += 1) this.playerBytes[byte] = (seed + byte * 17) & 0xff;
        for (let byte = 0; byte < this.cached.length; byte += 1) this.cached[byte] = (seed + byte * 11 + 23) & 0xff;
        for (let byte = 0; byte < this.external.length; byte += 1) this.external[byte] = (seed + byte * 7 + 49) & 0xff;
        if (kind === 1) this.current = this.players.subarray(0xb4 + 0x6c, 0xb4 + 0x6c + 10);
        else if (kind === 2) this.current = this.players.subarray(8 * 0xb4 + 0x6c, 8 * 0xb4 + 0x6c + 10);
        else if (kind === 3) this.current = this.cached;
        else if (kind === 4) this.current = this.external.subarray(0, length);
        else assert.equal(kind, 0, 'CoreData 원문 포인터 종류');
    }

    // pointer 가 가리키는 같은 배열 주소로만 원문 가상 주소와 대응한다(내용으로 owner 를 추정하지 않음).
    pointer(value) {
        if (value === null) return '0';
        if (samePointer(value, this.cached)) return 'CACHED';
        if (samePointer(value, this.external)) return 'EXTERNAL';
        for (let owner = 0; owner < 9; owner += 1) {
            const base = this.playerBytes.subarray(owner * 0xb4 + 0x6c);
            if (samePointer(value, base)) return `PLAYER${owner}`;
        }
        throw new Error('CoreData 미등록 포인터');
    }

    // 두 owner/known/active/metamap 의 현재 DWORD 를 원문과 같은 순서로 출력한다.
    state() { return `${this.templateOwner},${this.battleOwner},${this.playerKnown},${this.active},${this.metamap}`; }

    // 유일한 실제 하위 진단에서 현재값 변화/중단만 수행한다.
    hooks() {
        return {
            assertion: (line) => {
                this.trace.push(`A:${line}:_myCoreData == &lastKnownCoreData`);
                if (this.mutation === 'current0') this.current = null;
                else if (this.mutation === 'currentPlayer8') this.current = this.players.subarray(8 * 0xb4 + 0x6c, 8 * 0xb4 + 0x6c + 10);
                else if (this.mutation === 'currentCache') this.current = this.cached;
                else if (this.mutation === 'metamap0') this.metamap = 0;
                else if (this.mutation === 'active0') this.active = 0;
                else if (this.mutation === 'stop') throw new CoreStop('stop');
                else assert.equal(this.mutation, 'none', 'CoreData 원문 변경 종류');
            },
        };
    }

    // 정상 반환/실제 물리 읽기 실패/명시 중단을 서로 다른 원시 결과로 유지한다.
    run() {
        try {
            if (this.mode === 0) return ['normal', this.pointer(readPlayerCoreData(this, this.hooks()))];
            assert.equal(this.mode, 1, 'CoreData 원문 진입점');
            selectPlayerCoreData(this, this.ownerArg, this.templateArg);
            return ['normal', '-'];
        } catch (error) {
            if (error instanceof CoreStop) return ['stop', '-'];
            if (error instanceof RangeError) return ['fault', '-'];
            throw error;
        }
    }
}

// 유효 비영 Player 선택과 NULL/짧은 현재 source·진단 변경/중단의 PE 804관측을 전부 대조한다.
test('Player CoreData: 독립 PE 804관측(포인터·캐시·부분 쓰기·가변 alias)', { skip: !rows && '원본 fixture 없음' }, () => {
    const statuses = {};
    let count = 0;
    for (const values of rows) {
        assert.equal(values.length, 11);
        assert.ok(values[0] === '639' || values[0] === '895');
        assert.ok(Number(values[10]) > 0);
        const device = new CoreDevice(values[1].split(','));
        const [status, result] = device.run();
        count += 1;
        statuses[status] = (statuses[status] ?? 0) + 1;
        const label = `행 ${count}`;
        assert.equal(status, values[2], `${label} status`);
        assert.equal(result, values[3], `${label} return`);
        assert.equal(device.state(), values[4], `${label} state`);
        assert.equal(device.pointer(device.current), values[5], `${label} current`);
        assert.equal(hex(device.cached), values[6], `${label} cached10`);
        assert.equal(hex(device.playerBytes), values[7], `${label} players1620`);
        assert.equal(hex(device.external), values[8], `${label} external16`);
        assert.equal(device.trace.length ? device.trace.join(';') : '-', values[9], `${label} trace`);
    }
    assert.deepEqual([count, statuses.normal, statuses.fault, statuses.stop], [804, 730, 72, 2]);
});

// 반환 뷰의 쓰기가 실제 Player 에 반영되고 정리가 같은 10 BYTE 를 실제 캐시에 보존하는지 확인한다.
test('Player CoreData: 반환 뷰는 Player 와 캐시 물리의 alias 이다', () => {
    const device = new CoreDevice('1,165,0,10,0,7,9,8,3,none'.split(','));
    // 별도 Template owner 3 과 실제 Player 8 을 선택한다.
    selectPlayerCoreData(device, 8, 3);
    let current = readPlayerCoreData(device, {});
    // active 는 비영 선택에서 임의 1 로 바꾸지 않는다.
    assert.ok(samePointer(current, device.playerBytes.subarray(8 * 0xb4 + 0x6c)));
    assert.deepEqual([current.length, device.templateOwner, device.active], [10, 3, 9]);
    current[4] = 201;
    // 동일 주소를 빌리므로 별도 동기화가 없다.
    assert.equal(device.playerBytes[8 * 0xb4 + 0x70], 201);
    const known = device.playerKnown;
    selectPlayerCoreData(device, 0, 1);
    assert.deepEqual([device.current, device.active, device.playerKnown, device.cached[4]], [null, 0, known, 201]);
    device.metamap = 1;
    selectPlayerCoreData(device, 1, 7);
    current = readPlayerCoreData(device, {});
    assert.ok(samePointer(current, device.cached));
    current[8] = 177;
    // 실제 cache alias 이며 이전 Player 를 덮지 않는다.
    assert.deepEqual([device.cached[8], device.playerBytes[8 * 0xb4 + 0x74] !== 177], [177, true]);
});

// 범위 밖 가상 포인터를 host 주소로 꾸미지 않는 API 물리 관문이며 Native 유효 선택 대조와 구별한다.
test('Player CoreData: 표현할 수 없는 Player 뷰는 앞선 store 뒤에 거부한다', () => {
    const device = new CoreDevice('1,165,3,10,0,7,9,8,3,none'.split(','));
    // 유효 9×180 BYTE 계약을 일부러 만족하지 않는 장치이며 현재 cache 주소는 물리 오류 뒤에도 그대로이다.
    device.players = device.playerBytes.subarray(0, 100);
    assert.throws(() => selectPlayerCoreData(device, 8, 3), RangeError);
    assert.deepEqual([device.templateOwner, device.battleOwner, device.playerKnown, samePointer(device.current, device.cached), device.active], [3, 8, 1, true, 9]);
});

// 복원: local(known 0 또는 owner 가 battleOwner)이면 active 1 과 현재 포인터를 battleOwner 의 Player 에 맞추고, remote 면 건드리지 않는다. 진단은 정상 복귀한 뒤 Player base 를 다시 읽는다.
test('Player CoreData 복원: local/remote 경로·구독자 진단 뒤 base 재조회·저장 게임', () => {
    // 기록 장부(9×16)에 owner 2 의 구독자 ID 와 Core 10 BYTE 를 둔다.
    const makeScene = () => {
        const device = new CoreDevice('1,0,0,10,0,1,0,0,0,none'.split(','));
        device.players.fill(0);
        device.battleOwner = 2;
        const records = { bytes: new Uint8Array(144), origin: 0 };
        records.bytes.set([0x78, 0x56, 0x34, 0x12], 2 * 16);
        records.bytes.set([1, 2, 3, 4, 5, 6, 7, 8, 9, 10], 2 * 16 + 4);
        device.players.set([0x78, 0x56, 0x34, 0x12], 2 * 0xb4 + 0x2c);
        device.players[2 * 0xb4 + 0x71] = 0x5b;
        return { device, records };
    };
    const flags = (checking = 0, saved = 0, post = 0) => ({ missionPost: () => post, savedGame: () => saved, checking: () => checking });
    // 선행 missionPost 가 켜져 있으면 아무것도 하지 않는다.
    let { device, records } = makeScene();
    restorePlayerCoreData(device, records, flags(0, 0, 1), 2, {});
    assert.deepEqual([device.active, device.current], [0, null]);
    // local(owner === battleOwner): active 1, Core 4/4/2 복원, 현재 포인터는 battleOwner 의 +6c.
    restorePlayerCoreData(device, records, flags(), 2, {});
    assert.equal(device.active, 1);
    assert.deepEqual([...device.players.subarray(2 * 0xb4 + 0x6c, 2 * 0xb4 + 0x76)], [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]);
    assert.ok(samePointer(device.current, device.playerBytes.subarray(2 * 0xb4 + 0x6c)));
    // remote(known 1 이고 다른 owner): 현재 포인터와 active 를 건드리지 않는다.
    ({ device, records } = makeScene());
    device.playerKnown = 1;
    restorePlayerCoreData(device, records, flags(), 2, {});
    // owner 2 == battleOwner 라 이 경우도 local 이다. battleOwner 를 바꿔 remote 로 만든다.
    ({ device, records } = makeScene());
    device.playerKnown = 1;
    device.battleOwner = 5;
    restorePlayerCoreData(device, records, flags(), 2, {});
    assert.deepEqual([device.active, device.current], [0, null]);
    assert.deepEqual([...device.players.subarray(2 * 0xb4 + 0x6c, 2 * 0xb4 + 0x76)], [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]);
    // checking: 구독자 불일치는 진단(local 0x224 / remote 0x21f) 뒤 Player base 를 다시 읽고 복원을 계속한다.
    ({ device, records } = makeScene());
    device.players[2 * 0xb4 + 0x2c] = 0;
    const lines = [];
    const replaced = new Uint8Array(device.players.length);
    replaced.set(device.players);
    restorePlayerCoreData(device, records, flags(1), 2, {
        assertion: (expression, file, line) => {
            lines.push(line);
            assert.equal(file, '\\Ns\\O\\Player.cpp');
            device.players = replaced;
        },
    });
    assert.deepEqual(lines, [0x224]);
    assert.deepEqual([...replaced.subarray(2 * 0xb4 + 0x6c, 2 * 0xb4 + 0x70)], [1, 2, 3, 4], '진단이 바꾼 새 base 에 복원');
    // 진단 장치가 없으면 불일치 분기에서 거부한다.
    const mismatched = makeScene();
    mismatched.device.players[2 * 0xb4 + 0x2c] = 0;
    assert.throws(() => restorePlayerCoreData(mismatched.device, mismatched.records, flags(1), 2, {}), /진단 장치 누락/);
    // 저장 게임: 보존했던 저장 BYTE 를 되돌리고 Chat 재초기화를 부른다(장치 부재는 그 위치에서 거부).
    ({ device, records } = makeScene());
    let chat = 0;
    restorePlayerCoreData(device, records, flags(0, 1), 2, { initializeChat: () => { chat += 1; } });
    assert.deepEqual([device.players[2 * 0xb4 + 0x71], chat], [0x5b, 1]);
    ({ device, records } = makeScene());
    assert.throws(() => restorePlayerCoreData(device, records, flags(0, 1), 2, {}), /Chat 재초기화 장치 누락/);
});
