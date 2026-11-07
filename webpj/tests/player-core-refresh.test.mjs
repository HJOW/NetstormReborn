// Player Core 갱신(refreshPlayerCore, 원문 0044fc00)을 원본 10.78 관측으로 대조한다(WB04-E3, cpppj PlayerCoreRefreshTests 이식).
//  - player-core-refresh-x86.tsv 642행 중 지원 618행: 세 Core 16 BYTE·두 Pool 400 BYTE 전체와 현재 전역, 하위 호출 trace 순서를 비교한다(논리 span 쓰기 정지 24행은 store 커밋의 별도 관측이라
//    제품과 같다고 세지 않고 제외한다). 하위 장치는 입력으로 공급하고 부모의 max/Core 갱신 기대는 장치가 계산하지 않는다.
//  - guard 20행 중 지원 16행(원문 page 마지막 16 BYTE 경계): 물리 부족에서의 거부와 getter 중복 호출 trace. Unicorn 부분 store 4행은 host 단일 store 거부와 같다고 하지 않고 제외한다.
//  - 계약: 짧은 span 은 단일 DWORD store 전에 거부하고 이후 장치 부재는 Core+6 의 0 을 남기며, bit 범위 진단 뒤 current 가 바뀌어도 이미 얻은 같은 this 에 쓰고 재진입을 허용한다.
// (2026-11-08 WB04-E3: Player Core 갱신 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get } from './raw-support.mjs';
import { readPlayerCoreData } from '../www/core/raw-player-core-data.js';
import { refreshPlayerCore } from '../www/core/raw-player-core-refresh.js';

// 원본 관측 행이다. 파일이 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E3)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const refreshRows = readRows(path.join(FIXTURES, 'player-core-refresh-x86.tsv'));
const guardRows = readRows(path.join(FIXTURES, 'player-core-refresh-guard-x86.tsv'));

// 전체 Core/Pool 의 미작성 BYTE 까지 모두 비교하는 소문자 hex 이다(NUL 도 버리지 않음). (2026-11-08 WB04-E3)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 실제 하위의 명시 중단을 물리 계약 오류와 구별한다. (2026-11-08 WB04-E3)
class RefreshStop extends Error {}

// 현재 Native 입력만 소유하고 부모의 max/Core 갱신 기대를 장치에서 계산하지 않는 장치이다. (2026-11-08 WB04-E3)
class RefreshDevice {
    constructor(values) {
        assert.equal(values.length, 14);
        // 현재 A/B/lastKnown 의 같은 전체 물리, Pool 변경 전후의 두 실제 raw 표, Native 명시 Player 물리 64KiB 이다(앞 1620 BYTE 가 9기록, 범위 밖 owner 256 도 실제 0 물리를 읽음).
        this.cores = [new Uint8Array(16), new Uint8Array(16), new Uint8Array(16)];
        this.pools = [new Uint8Array(400), new Uint8Array(400)];
        this.players = new Uint8Array(65536);
        this.current = null;
        this.poolIndex = 0;
        this.containedType = 6;
        this.known = 1;
        this.metamap = 0;
        this.templateOwner = 0;
        this.active = 0;
        this.calls = {};
        this.levelCalls = new Map();
        this.trace = [];
        this.currentLevelIndex = 0;
        const floor = Number(values[0]);
        const mask = Number(values[1]);
        const free = Number(values[5]);
        const dead = Number(values[6]);
        // TYPE 별 signed 기술 bit 를 보존한다.
        this.bits = [Number(values[2]) | 0, Number(values[3]) | 0, Number(values[4]) | 0];
        this.owner = Number(values[7]) >>> 0;
        this.playerType = Number(values[8]) >>> 0;
        this.capacity = Number(values[9]) >>> 0;
        this.levelMode = Number(values[10]);
        const kind = Number(values[11]);
        this.coreLength = Number(values[12]);
        this.mutation = values[13];
        // Native 주소 차이에서 나온 패턴도 입력 그대로 재현한다.
        for (let block = 0; block < 3; block += 1) {
            const bias = block === 0 ? 0 : block === 1 ? 29 : -33786073;
            for (let byte = 0; byte < 16; byte += 1) this.cores[block][byte] = (41 + bias + byte * 11) & 0xff;
            this.cores[block][5] = (block === 0 ? floor : floor + 17) & 0xff;
        }
        // 두 Pool 의 같은 필드를 원문 입력과 같이 쓴다.
        for (let block = 0; block < 2; block += 1) {
            const pool = this.pools[block];
            for (let index = 0; index < 8; index += 1) {
                const offset = index * 50;
                put(pool, offset, 0x12080000);
                pool[offset + 10] = (index === 2 || index === 6 || index === 7) ? 152 : 6;
                pool[offset + 11] = ((free >> index) & 1) | (((dead >> index) & 1) << 1);
                pool[offset + 34] = 1;
            }
            put(pool, 56, 3, 2);
            // 기술 목록의 실제 next/kind/flags 입력이다.
            for (let index = 3; index <= 5; index += 1) {
                put(pool, index * 50 + 4, index === 5 ? 0 : index + 1, 2);
                put(pool, index * 50 + 18, index === 5 ? 65535 : index - 2, 2);
                pool[index * 50 + 20] = (mask & (1 << (index - 3))) ? 4 : 0;
            }
            if (block) pool[111] = 2;
        }
        put(this.players, 180 + 156, 1);
        put(this.players, 8 * 180 + 156, 1);
        if (kind < 3) this.current = this.cores[kind].subarray(0, this.coreLength);
    }

    // 같은 입력 물리 주소로만 현재 Core 를 정규화한다(BYTE 내용으로 주소를 추정하지 않음).
    pointer() {
        if (this.current === null) return '0';
        for (let index = 0; index < 3; index += 1) {
            if (this.current.buffer === this.cores[index].buffer && this.current.byteOffset === this.cores[index].byteOffset) return 'ABC'[index];
        }
        throw new Error('CoreRefresh 미등록 현재 Core');
    }

    // 실제 하위의 같은 종류 호출 번호를 재진입에서도 공유한다.
    event(kind, text) {
        this.trace.push(text);
        this.calls[kind] = (this.calls[kind] ?? 0) + 1;
        return `${kind}${this.calls[kind]}`;
    }

    // callback 은 현재 입력만 바꾸며 부모의 분기/최종 값을 대신 계산하지 않는다.
    mutate(boundary) {
        const dash = this.mutation.indexOf('-');
        if (dash < 0 || this.mutation.slice(0, dash) !== boundary) return;
        const action = this.mutation.slice(dash + 1);
        if (action === 'stop') throw new RefreshStop('stop');
        if (action === 'currentB') this.current = this.cores[1].subarray(0, this.coreLength);
        else if (action === 'cache') {
            this.current = this.cores[2].subarray(0, this.coreLength);
            this.metamap = 1;
        } else if (action === 'poolB') this.poolIndex = 1;
        else if (action === 'cap2') this.capacity = 2;
        else if (action === 'known0') this.known = 0;
        else if (action === 'owner8') this.owner = 8;
        else if (action === 'owner0') this.owner = 0;
        else if (action === 'contained152') this.containedType = 152;
        else if (action === 'playerType6') this.playerType = 6;
        else if (action === 'dead') this.pools[this.poolIndex][this.currentLevelIndex * 50 + 11] = 2;
        else assert.equal(action, 'reenter', 'CoreRefresh 미해석 명시 변경');
    }

    // 선택한 진단은 현재 변경 후 복귀하거나 그 위치에서 중단한다.
    assertion(line) {
        const boundary = this.event('a', `A:${line}`);
        this.mutate(boundary);
    }

    // 기존 실제 0048f0d0 몸체를 사용하며 현재 span 을 사본 버퍼로 교체하지 않는다.
    core() {
        const boundary = this.event('g', `G:${this.pointer()}`);
        this.mutate(boundary);
        const device = this;
        return readPlayerCoreData({
            get templateOwner() { return device.templateOwner; },
            get battleOwner() { return device.owner; },
            get playerKnown() { return device.known; },
            get active() { return device.active; },
            get metamap() { return device.metamap; },
            get current() { return device.current; },
            cached: this.cores[2],
            players: this.players,
        }, { assertion: (line) => this.assertion(line) });
    }

    // Native 와 동일한 원시 Player+9c 하위 경계이다. 부모의 기술 순회 결과를 계산하지 않는다.
    technologyParent() {
        const boundary = this.event('p', 'P');
        this.mutate(boundary);
        if (!this.known) return 0;
        return get(this.players, this.owner * 180 + 156, 4);
    }

    // 독립 원시 finder 장치의 next/current 는 Begin 별로 따로 소유한다.
    begin(parent) {
        const boundary = this.event('b', `B:${parent}`);
        this.mutate(boundary);
        const cursor = { current: 0, next: get(this.pools[this.poolIndex], parent * 50 + 6, 2) };
        // 실제 현재 Pool 과 containedType 을 후보마다 읽는 하위 fixture 장치이다.
        const advance = () => {
            for (;;) {
                cursor.current = cursor.next;
                if (!cursor.current) {
                    cursor.next = 0;
                    return;
                }
                const offset = cursor.current * 50;
                cursor.next = get(this.pools[this.poolIndex], offset + 4, 2);
                if (this.pools[this.poolIndex][offset + 10] === this.containedType) return;
            }
        };
        advance();
        return {
            current: () => cursor.current,
            next: () => {
                const next = this.event('n', 'N');
                this.mutate(next);
                advance();
            },
        };
    }

    // 가상 level 의 명시 반환만 공급하며 max 선택은 제품 본문이 수행한다.
    level(formObject) {
        const block = formObject.address >= 0x12010000 ? 1 : 0;
        const index = Math.floor((formObject.address - (block ? 0x12010000 : 0x12000000)) / 50);
        this.currentLevelIndex = index;
        const key = `${block}:${index}`;
        const count = (this.levelCalls.get(key) ?? 0) + 1;
        this.levelCalls.set(key, count);
        const second = count % 2 === 0;
        let value = ((4 - ((index - 2) >>> 0) % 3) >>> 0) + block;
        value |= 0;
        if (this.levelMode === 1 && second) value = -3;
        else if (this.levelMode === 2 && second) value = -2147483648;
        else if (this.levelMode === 3) value = 2147483647;
        else if (this.levelMode === 4) value = -1;
        assert.ok(formObject.bytes.buffer === this.pools[block].buffer && formObject.bytes.byteOffset === index * 50);
        const boundary = this.event('v', `V:${block}:${index}:${value}`);
        this.mutate(boundary);
        if (this.mutation === `${boundary}-reenter`) {
            this.trace.push('R:begin');
            refreshPlayerCore(this.hooks());
            this.trace.push('R:end');
        }
        return value;
    }

    // 기존 실제 Core helper 와 부모가 선택한 하위 장치만 연결한다.
    hooks() {
        return {
            core: () => this.core(),
            technologyParent: () => this.technologyParent(),
            begin: (parent) => this.begin(parent),
            pool: () => ({ bytes: this.pools[this.poolIndex], address: this.poolIndex ? 0x12010000 : 0x12000000, capacity: this.capacity }),
            technologyBit: (kind) => {
                const boundary = this.event('t', `T:${kind}`);
                this.mutate(boundary);
                return this.bits[kind === 1 ? 0 : kind === 2 ? 1 : 2];
            },
            playerType: () => this.playerType,
            owner: () => this.owner,
            level: (formObject) => this.level(formObject),
            assertion: (line) => this.assertion(line),
        };
    }

    // 현재 전역/모든 Core 와 Pool/trace 를 원시 기대 칸과 같은 순서로 출력한다.
    state() { return `${this.poolIndex},${this.capacity},${this.owner},${this.playerType},${this.containedType},${this.known},${this.metamap}`; }

    coresHex() { return this.cores.map(hex).join(''); }

    poolsHex() { return this.pools.map(hex).join(''); }

    traceText() { return this.trace.length ? this.trace.join(';') : '-'; }
}

// 논리 span 쓰기 정지 24행은 store 커밋의 별도 관측이고 실제 제품과 동일하다고 세지 않는다.
test('refreshPlayerCore: 독립 PE 618개 지원 관측(Core·Pool 전체와 하위 호출 trace)', { skip: !refreshRows && '원본 fixture 없음' }, () => {
    let row = 0;
    let compared = 0;
    let excluded = 0;
    for (const values of refreshRows) {
        row += 1;
        assert.equal(values.length, 9);
        if (values[2] === 'fault') {
            excluded += 1;
            continue;
        }
        const device = new RefreshDevice(values[1].split(','));
        let status = 'normal';
        try {
            refreshPlayerCore(device.hooks());
        } catch (error) {
            if (error instanceof RefreshStop) status = 'stop';
            else if (error instanceof RangeError) {
                if (device.current) throw error;
                status = 'fault-null';
            } else throw error;
        }
        compared += 1;
        assert.equal(status, values[2], `행 ${row} status`);
        assert.equal(device.pointer(), values[3], `행 ${row} current`);
        assert.equal(device.state(), values[4], `행 ${row} state`);
        assert.equal(device.coresHex(), values[5], `행 ${row} cores`);
        assert.equal(device.poolsHex(), values[6], `행 ${row} pools`);
        assert.equal(device.traceText(), values[7], `행 ${row} trace`);
    }
    assert.deepEqual([row, compared, excluded], [642, 618, 24]);
});

// 짧은 host span 은 단일 DWORD store 전에 거부하는 계약이며 Unicorn 부분 store 와 같다고 하지 않는다.
test('refreshPlayerCore: 짧은 Core 는 전체 store 를 거부하고 이후 장치 부재는 0 을 남긴다', () => {
    // 실제 10 BYTE 미만의 모든 host view 를 별도 계약으로 검증한다.
    for (let length = 0; length < 10; length += 1) {
        const bytes = new Uint8Array(16).fill(0x5a);
        assert.throws(() => refreshPlayerCore({ core: () => bytes.subarray(0, length) }), RangeError, `길이 ${length}`);
        assert.ok(bytes.every((byte) => byte === 0x5a));
    }
    const bytes = new Uint8Array(10).fill(0x5a);
    assert.throws(() => refreshPlayerCore({ core: () => bytes }), /technologyParent/);
    assert.deepEqual([bytes[5], get(bytes, 6, 4)], [0x5a, 0]);
});

// bit 범위 진단 뒤 current 가 바뀌어도 Ch 는 이미 얻은 같은 this 에 쓰고, level 호출 안의 재진입을 허용한다.
test('refreshPlayerCore: 기술 bit 범위 진단은 얻은 Core 에 쓰고 재진입을 허용한다', () => {
    const device = new RefreshDevice('0,7,32,31,-1,0,0,1,152,8,0,0,10,a1-currentB'.split(','));
    refreshPlayerCore(device.hooks());
    assert.equal(device.pointer(), 'B');
    assert.equal(get(device.cores[0], 6, 4), 1);
    assert.ok((get(device.cores[1], 6, 4) & 0x80000000) !== 0);
    const nested = new RefreshDevice('0,7,0,31,-1,0,0,1,152,8,0,0,10,v1-reenter'.split(','));
    refreshPlayerCore(nested.hooks());
    assert.ok(nested.trace.includes('R:begin'));
    assert.equal(nested.cores[0][5], 3);
});

// 실제 guard 20행 중 Unicorn 부분 store 4행을 구별하고 지원 16행의 물리/trace 를 대조한다.
test('refreshPlayerCore: guard 지원 16행과 부분 store 4행 분리', { skip: !guardRows && '원본 fixture 없음' }, () => {
    let rows = 0;
    let compared = 0;
    let excluded = 0;
    for (const values of guardRows) {
        const args = values[1].split(',');
        const length = Number(args[0]);
        const kind = Number(args[1]);
        rows += 1;
        // 실제 guard 에서 Unicorn 이 앞 3 BYTE 를 쓴 4행은 host 단일 store 거부와 같다고 하지 않는다.
        if (length === 9) {
            excluded += 1;
            continue;
        }
        // 원문 page 마지막 16 BYTE 의 동일 입력이다.
        const tails = [new Uint8Array(16), new Uint8Array(16)];
        const pool = new Uint8Array(400);
        const trace = [];
        for (let block = 0; block < 2; block += 1) {
            tails[block].fill(0x5a + block);
            for (let byte = 0; byte < length; byte += 1) tails[block][16 - length + byte] = (0x31 + block * 29 + byte * 11) & 0xff;
        }
        const hooks = {
            core: () => {
                trace.push(kind ? 'G:B' : 'G:A');
                return tails[kind].subarray(16 - length, 16);
            },
            technologyParent: () => {
                trace.push('P');
                return 0;
            },
            begin: (parent) => {
                trace.push(`B:${parent}`);
                return { current: () => 0, next: () => {} };
            },
            pool: () => ({ bytes: pool, address: 0x12000000, capacity: 0 }),
        };
        let status = 'normal';
        try {
            refreshPlayerCore(hooks);
        } catch (error) {
            if (!(error instanceof RangeError)) throw error;
            status = 'fault-guard';
        }
        compared += 1;
        assert.equal(status, values[2], `guard 행 ${rows} status`);
        assert.equal(hex(tails[0]) + hex(tails[1]), values[3], `guard 행 ${rows} bytes`);
        assert.equal(trace.join(';'), values[4], `guard 행 ${rows} trace`);
    }
    assert.deepEqual([rows, compared, excluded], [20, 16, 4]);
});
