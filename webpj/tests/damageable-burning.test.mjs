// burning 판정 raw-damageable-burning.js(RawDamageableBurning·DamageableBurningState)의 현재 전역·진단 복귀·저장값/현재값 계약을 원본 10.78 하드웨어 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: damageable-burning-checking-hardware-x86.tsv 10.78(originals) 550행 — 판 index·HP·최대 HP·검색 결과 둘·생성 결과·checking(전체 DWORD)·타입·상태·damageEffect·동작 비트(1 첫 검색이 전역을 바꿈 / 2 진단이 슬롯을 바꿈 / 4 진단이 free 로 만듦 / 8 진단이 첫 결과를 뒤집음)
//    → 반환, 슬롯 50 전체 raw, 현재 checking·burningType·bulfType·최대 HP·첫 결과, 호출 순서(F 검색 / D 진단 / S 위치음 / C 생성 / X 삭제). CD 10.72 행은 웹 범위(10.78) 밖이다.
//  - 계약: DamageableBurningChecking_SelectedDevicesAndPartialEffects(누락한 선택 장치와 진단 중단은 이미 실행한 검색/위치음을 되돌리지 않음).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E67: burning 판정 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { RawDamageableBurning, DamageableBurningState } from '../www/core/raw-damageable-burning.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E67)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'damageable-burning-checking-hardware-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 원본 관찰 피해 SID, 판본당 독립 행 수이다. (2026-11-08 WB04-E67)
const VICTIM = 50;
const ROWS = 550;

// 예외가 나는지 본다. (2026-11-08 WB04-E67)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E67)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 같은 입력 슬롯/장치와 현재 전역을 한 갱신 동안 유지하는 장면이다. (2026-11-08 WB04-E67)
class BurningCheckingScene {
    constructor(row) {
        assert.equal(row.length, 20);
        this.pool = new SidPool(24000, { server: true });
        this.types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, damageEffect: 0 }));
        this.book = new SquidPostPopState();
        this.hpState = new SquidRewardState();
        this.store = new ScrambledSpStore(new GameRandom(), () => 1);
        this.state = new DamageableBurningState();
        this.hooks = {};
        this.trace = '';
        // 결과를 계산하지 않고 명시 입력만 복제한다.
        for (let id = 5; id <= VICTIM; id += 1) assert.equal(this.pool.allocate(2), id);
        this.checking = Number(row[7]);
        this.first = Number(row[4]);
        this.second = Number(row[5]);
        this.created = Number(row[6]);
        this.action = Number(row[11]);
        this.maximum = parseInt(row[3], 10) | 0;
        this.state.sharedChecking = () => this.checking;
        const raw = this.pool.allocated(VICTIM);
        // 예약 바이트도 CPU 와 같다.
        for (let byte = 0; byte < raw.length; byte += 1) raw[byte] = (byte * 7 + 19) & 0xff;
        put(raw, 0, 0x61f000);
        raw[10] = Number(row[8]);
        raw[11] = Number(row[9]);
        put(raw, 14, 0x3fa00000);
        put(raw, 18, 0xc0200000);
        put(raw, 26, Number(row[2]));
        this.types[raw[10]].flags1 = 0x10;
        this.types[raw[10]].maxHitPoints = this.maximum;
        this.types[raw[10]].damageEffect = Number(row[10]);
        this.types[135].damageEffect = Number(row[10]);
        this.hp = new SquidReward(this.pool, this.types, this.book, this.hpState, this.store);
        this.hooks.find = (victim, type, index) => {
            assert.ok(victim === VICTIM && index <= 1);
            const result = index ? this.second : this.first;
            this.trace += `F,50,${type},${index},${result};`;
            if (!index && (this.action & 1)) {
                this.checking = 0x80000000;
                this.state.burningType = 58;
                put(this.raw(), 26, 0x7fffffff);
                this.maximum = 0;
            }
            return result;
        };
        this.hooks.assertion = (expression, file, line) => {
            this.trace += `D,${line}:${expression}:${file};`;
            if (this.action & 2) {
                this.checking = 0;
                this.raw()[10] = 135;
                put(this.raw(), 14, 0x40700000);
                put(this.raw(), 18, 0x40900000);
                put(this.raw(), 26, 0x7fffffff);
                this.maximum = -2147483648;
                this.state.bulfType = 135;
                this.state.burningType = 61;
            }
            if (this.action & 4) {
                this.raw()[11] |= 1;
                this.checking = 0;
            }
            if (this.action & 8) this.first = this.first ? 0 : 90;
        };
        this.hooks.create = (victim) => {
            assert.equal(victim, VICTIM);
            this.trace += `C,50,${this.created};`;
            return this.created;
        };
        this.hooks.destroy = (form, flags) => { this.trace += `X,${form},${flags};`; };
        this.hooks.bulfSound = (victim, x, y) => {
            assert.equal(victim, VICTIM);
            this.trace += `S,50,${floatToBits(x)},${floatToBits(y)},bulfHurt.wav,0;`;
        };
        this.rebind();
    }

    // free 입력에도 관측용 raw 바이트를 제공하는 live 슬롯이다.
    raw() {
        return this.pool.physical(VICTIM);
    }

    // 선택 장치를 바꾼 뒤 같은 전역/HP 몸체를 다시 연결한다.
    rebind() {
        this.body = new RawDamageableBurning(this.pool, this.types, this.hp, this.state, this.hooks);
    }

    // 반환/전체 raw/현재 전역/순서만 직렬화한다.
    run() {
        const result = this.body.refresh(VICTIM);
        return [String(result), hex(this.raw()), String(this.checking), String(this.state.burningType), String(this.state.bulfType), String(this.maximum), String(this.first), this.trace === '' ? '-' : this.trace];
    }
}

test('DamageableBurningChecking: 원본 10.78 550행의 현재 DWORD·중복 진단·저장값/현재값·오버플로 절반 계산이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 첫 불일치를 원문과 제품 필드로 나란히 남긴다.
    for (const row of rows) {
        const scene = new BurningCheckingScene(row);
        assert.deepEqual(scene.run(), row.slice(12), `행 ${row[1]}`);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('DamageableBurningChecking: 누락한 선택 장치와 진단 중단은 이미 실행한 검색/위치음을 되돌리지 않는다', { skip: !rows && '원본 fixture 없음' }, () => {
    const row = [...rows[0]];
    row[2] = '49';
    row[3] = '100';
    row[4] = '0';
    row[5] = '91';
    row[6] = '92';
    row[7] = '2147483648';
    row[8] = '135';
    row[11] = '0';
    const missing = new BurningCheckingScene(row);
    missing.hooks.assertion = undefined;
    missing.rebind();
    assert.ok(throws(() => missing.run()));
    assert.equal(missing.trace, 'F,50,14,0,0;F,50,14,1,91;');
    const stopped = new BurningCheckingScene(row);
    stopped.hooks.assertion = () => { stopped.checking = 0; throw new Error('선택 진단 중단'); };
    stopped.rebind();
    assert.ok(throws(() => stopped.run()));
    assert.ok(!stopped.checking && stopped.trace === missing.trace);
    row[7] = '0';
    const sound = new BurningCheckingScene(row);
    sound.hooks.create = undefined;
    sound.rebind();
    assert.ok(throws(() => sound.run()));
    assert.equal(sound.trace, 'F,50,14,0,0;S,50,1067450368,3223322624,bulfHurt.wav,0;');
    row[9] = '1';
    const free = new BurningCheckingScene(row);
    free.hooks = {};
    free.rebind();
    assert.ok(!free.body.refresh(VICTIM) && free.trace === '');
    row[9] = '0';
    row[10] = '0';
    const noEffect = new BurningCheckingScene(row);
    noEffect.hooks = {};
    noEffect.rebind();
    assert.ok(!noEffect.body.refresh(VICTIM) && noEffect.trace === '');
});
