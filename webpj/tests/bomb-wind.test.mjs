// 주문 활성화 수신(41e860)과 공유 바람 갱신(004dd2b0)을 원본 10.78 관측으로 대조한다(WB04-E17, cpppj BombActivationTests·AmbientWindTests 이식).
//  - bomb-activation-x86.tsv 434행: 원격 비용/현재 genus 진단/할당/예약 호출 인자와 부모 50 BYTE 전체(두 정밀도 열 동일).
//  - ambient-wind-hardware-x86.tsv 의 originals 행: 장기 감쇠·난수 소비·각도 정규화 시퀀스의 세 float 비트·게임 난수 상태·checking·진단.
//  - 계약: 짧은 패킷 거부, dead 부모는 효과 없음, 선택된 경계 누락은 그 위치에서 거부.
// cpppj 의 Client Storage/난수 owner 수명 검사는 해당 단위를 옮길 때 대조한다. (2026-11-08 WB04-E17: 주문 활성화·공유 바람 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, floatToBits, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { RawBombActivation } from '../www/core/raw-bomb-activation.js';
import { stepAmbientWind, createAmbientWindState } from '../www/core/raw-ambient-wind.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E17)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const bombRows = fixture('bomb-activation-x86.tsv');
const windAll = fixture('ambient-wind-hardware-x86.tsv');
const number = (text) => Number(BigInt(text) & 0xffffffffn);
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 원본의 선언 경제/생성 입력만 공급하며 수신 관문은 제품이 실행하는 장면이다(cpppj ActivationScene). (2026-11-08 WB04-E17)
class ActivationScene {
    constructor() {
        this.pool = new SidPool(24000, { server: true });
        this.types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
        this.localOwner = 0;
        this.input = [];
        this.trace = [];
    }

    raw() { return this.pool.physical(50); }

    event(name, values) { this.trace.push(`${name}${values.map((value) => `:${value >>> 0}`).join('')}`); }

    // 기대값을 계산하지 않고 raw/현재 전역을 초기화한다.
    prepare(text) {
        this.input = text.split(',').map(number);
        assert.equal(this.input.length, 7);
        this.trace = [];
        const raw = this.raw();
        raw.fill(0);
        raw[10] = 168;
        raw[11] = this.input[0] & 0xff;
        raw[34] = this.input[1] & 0xff;
        this.localOwner = this.input[2];
        this.types[168].flags2 = this.input[4];
        this.types[169].flags2 = 0;
    }

    hooks() {
        return {
            cost: (type) => { assert.equal(type, 168); return this.input[3] | 0; },
            subtract: (owner, amount) => { this.event('subtract', [owner, floatToBits(amount)]); if (this.input[6] === 1) this.raw()[10] = 169; },
            assertion: (line) => { this.event('assert', [line]); if (this.input[6] === 2) this.raw()[10] = 169; },
            allocate: (bytes) => { this.event('allocate', [bytes]); return this.input[5] !== 0; },
            construct: (event, parent, delay) => this.event('construct', [event, parent, floatToBits(delay)]),
        };
    }
}

// 실제 원본 기대와 전체 부모 50 BYTE 를 두 정밀도 모두 대조한다.
test('주문 활성화: 원본10.78 434행(원격 비용·진단·할당·예약·부모 50 BYTE)', { skip: !bombRows && '원본 fixture 없음' }, () => {
    const scene = new ActivationScene();
    let count = 0;
    for (const row of bombRows) {
        assert.equal(row.length, 5);
        scene.prepare(row[0]);
        const activation = new RawBombActivation(scene.pool, scene.types, () => scene.localOwner, scene.hooks());
        activation.receive(Uint8Array.of(1, 0x5c, 6, 0, 50, 0));
        const label = `행 ${count} ${row[0]}`;
        assert.equal(scene.trace.join(';') || '-', row[1], `${label} 호출`);
        assert.equal(hex(scene.raw()), row[2], `${label} 부모`);
        assert.deepEqual(row.slice(1, 3), row.slice(3, 5), `${label} 두 정밀도 동일`);
        count += 1;
    }
    assert.equal(count, 434);
});

// 짧은 패킷 거부와 dead 의 미도달 효과/실제 선택 경계 누락을 확인한다.
test('주문 활성화: 패킷 길이와 지연 경계', () => {
    const scene = new ActivationScene();
    const activation = new RawBombActivation(scene.pool, scene.types, () => scene.localOwner, {});
    assert.equal(activation.pool, scene.pool);
    assert.throws(() => activation.receive(new Uint8Array(5)), TypeError);
    const packet = Uint8Array.of(1, 0x5c, 6, 0, 50, 0);
    // dead 부모는 어떤 효과도 요구하지 않는다.
    scene.raw()[11] = 2;
    activation.receive(packet);
    scene.raw()[11] = 0;
    scene.raw()[10] = 168;
    scene.types[168].flags2 = 0x100;
    assert.throws(() => activation.receive(packet), /실제 하위 미연결/);
});

// 양판본/현재 DWORD/0 재시드/장기 감쇠·난수 소비 행을 동일 물리 비트로 비교한다(10.78 행).
test('공유 바람: 원본10.78 originals 행(장기 감쇠·난수 소비·각도 정규화)', { skip: !windAll && '원본 fixture 없음' }, () => {
    const rows = windAll.filter((row) => row[0] === 'originals');
    let count = 0;
    for (const row of rows) {
        assert.equal(row.length, 14);
        const state = { x: bitsToFloat(number(row[4])), y: bitsToFloat(number(row[5])), angle: bitsToFloat(number(row[6])) };
        const random = new GameRandom(number(row[2]));
        let checking = number(row[7]);
        const events = [];
        const diagnostics = { checking: () => checking, assertion: (expression, file, line) => events.push(`${line}:${expression}:${file}`) };
        // 매 프레임 Native 처럼 감쇠/같은 난수/선택 각도를 진행하며 시각을 따로 적분하지 않는다.
        for (let step = 0; step < number(row[3]); step += 1) stepAmbientWind(state, random, diagnostics);
        const label = `행 ${count} ${row.slice(0, 8).join(',')}`;
        assert.deepEqual([floatToBits(state.x), floatToBits(state.y), floatToBits(state.angle), random.state], [number(row[8]), number(row[9]), number(row[10]), number(row[11])], `${label} 바람/난수`);
        assert.equal(checking, number(row[12]), `${label} checking`);
        assert.equal(events.length ? events.join('|') : '-', row[13], `${label} 진단`);
        count += 1;
    }
    assert.ok(count > 0);
    assert.equal(count, rows.length);
});

// 최초 상태(X 0, Y 1, 각도 0)는 원본 PE 의 BSS/초기값이다.
test('공유 바람: 최초 상태와 비잡음 프레임의 보존', () => {
    const state = createAmbientWindState();
    assert.deepEqual(state, { x: 0, y: 1, angle: 0 });
    const random = new GameRandom(55);
    stepAmbientWind(state, random);
    assert.equal(state.y, Math.fround(0.9));
});
