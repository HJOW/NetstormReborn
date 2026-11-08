// Burning 연소점 개수·피해 부모·TYPE HP raw-burning-source-count.js(burningSourceCount)와 raw-burning-sources.js 의 높이 2 이른 복귀를 실제 PE 관측 fixture 와 cpppj 계약으로 대조한다.
//  - burning-source-count-current-x86.tsv 의 10.78(originals) 2,826행(두 판본 합 5,652행 중 10.78): 열 = 판본, 입력 8개(HP DWORD·TYPE 최대 HP·위크 마나·checking·flag 0x400·너비·사제 TYPE·진단 복귀 프로필), 결과(ok/zero/overflow), 호출 뒤 상태 9칸
//    (선택 수·HP·피해 슬롯 TYPE·피해 SID·checking·위크 마나·사제 TYPE·다이스 TYPE·TYPE 74 최대 HP), 진단 순서, (예약), robust 앞단 호출 횟수. 높이 2 로 입자·난수·픽셀을 실행하지 않고 부모/HP getter/정수식/IDIV 예외/진단 복귀 뒤 현재 선택·전역만 원문과 대조한다.
//  - cpppj 계약: 장기 owner(사제/다이스/위크 마나)와 현재 다이스 예외를 구별, HP 진단이 자기 연결을 반납해도 현재 getter 복귀와 이전 HP 저장을 보존. CD 행은 웹 범위(10.78) 밖이다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E183: Burning 연소점 개수 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { GameAuxState } from '../www/core/game-aux-state.js';
import { RawRenderRegistry } from '../www/core/raw-construction-lifetime.js';
import { RawBurningVisual, createBurningVisualFields } from '../www/core/raw-burning-visual.js';
import { RawBurningSources } from '../www/core/raw-burning-sources.js';
import { BurningSourceCountError, burningSourceCount } from '../www/core/raw-burning-source-count.js';

// 실제 PE 관측 fixture 의 10.78 행이다. (2026-11-09 WB04-E183)
const rows = (readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'burning-source-count-current-x86.tsv')) ?? []).filter((row) => row[0] === 'originals');

// 예외가 나는지 본다. (2026-11-09 WB04-E183)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 같은 풀/TYPE/장기 전역/현재 진단을 실제 연소점 선택기에 공급하는 장면이다. (2026-11-09 WB04-E183)
class CountScene {
    // 원문 입력의 두 실제 피해 슬롯(50/51)까지 확보하고 장기 owner 를 선택기 상태에 getter 로 연결한다.
    constructor() {
        this.pool = new SidPool(24000, { server: true });
        this.random = new GameRandom();
        this.types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, maxHitPoints: 0 }));
        this.owner = new GameAuxState();
        this.checking = 0;
        this.profile = 0;
        this.robustCalls = 0;
        this.diagnostics = [];
        this.fields = createBurningVisualFields();
        for (let id = 5; id <= 51; id += 1) assert.equal(this.pool.allocate(2), id);
        const refuse = (name) => () => { throw new Error(name); };
        const registry = new RawRenderRegistry(this.pool, { reserve: refuse('높이2의 표시 확보 금지'), releaseArray: refuse('높이2의 표시 반납 금지'), attach: refuse('높이2의 표시 부착 금지') });
        const visual = new RawBurningVisual(this.pool, registry, { processType: 14, palette: 0 }, { reserve: refuse('높이2의 입자 확보 금지') });
        this.mode = {
            weakenedMana: false, manaType: 158, daisType: 165,
            sharedWeakenedMana: () => this.owner.priestQuarterHp, sharedManaType: () => this.owner.priestType, sharedDaisType: () => this.owner.daisType,
            victim: {
                checking: () => this.checking,
                robust: () => { this.robustCalls += 1; return true; },
                // 같은 원문 진단이 복귀하는 동안 명시 입력만 변경한다.
                assertion: (expression, file, line) => {
                    this.diagnostics.push(`${expression}|${file}|${line}`);
                    assert.equal(line, 0x1e3);
                    if (this.profile) {
                        const raw = this.pool.allocated(50);
                        put(raw, 26, this.profile === 2 ? 5 : 0);
                        this.types[74].maxHitPoints = 200;
                        this.owner.priestQuarterHp = this.profile !== 3;
                        this.owner.priestType = this.profile === 3 ? 75 : 74;
                        if (this.profile === 2 || this.profile === 3) {
                            raw[10] = 75;
                            this.fields.victim = 51;
                            this.types[75].flags2 = this.profile === 2 ? 0x400 : 0;
                        }
                        if (this.profile === 3) this.owner.daisType = 75;
                    }
                }
            }
        };
        this.sources = new RawBurningSources(this.pool, this.types, visual, this.random, this.mode);
    }

    // 원문 관측기와 같은 초기 물리 입력을 설치한다. (2026-11-09 WB04-E183)
    prepare(input) {
        assert.equal(input.length, 8);
        for (const type of this.types) Object.assign(type, { flags1: 0, flags2: 0, maxHitPoints: 0 });
        this.types[74].maxHitPoints = Number(input[1]) | 0;
        this.checking = Number(input[3]);
        this.types[74].flags1 = this.checking ? 0 : 16;
        this.types[74].flags2 = (0x400000 | (Number(input[4]) ? 0x400 : 0)) >>> 0;
        this.types[75].maxHitPoints = 999;
        this.owner.priestQuarterHp = Number(input[2]) !== 0;
        this.owner.priestType = Number(input[6]);
        this.owner.daisType = 165;
        this.profile = Number(input[7]);
        this.diagnostics = [];
        this.robustCalls = 0;
        this.fields = createBurningVisualFields();
        this.fields.victim = 50;
        this.fields.image.width = Number(input[5]) | 0;
        this.fields.image.height = 2;
        const raw = this.pool.allocated(50);
        raw.fill(0);
        raw[10] = 74;
        raw[40] = 1;
        put(raw, 26, (Number(input[0]) | 0) >>> 0);
    }

    // 반환 슬롯/피해 번호/현재 전역/선택 TYPE HP 의 독립 후상태를 조립한다. (2026-11-09 WB04-E183)
    post() {
        const raw = this.pool.slot(50);
        const hp = get(raw, 26) | 0;
        return `${this.fields.selected},${hp},${raw[10]},${this.fields.victim},${this.checking},${this.owner.priestQuarterHp ? 1 : 0},${this.owner.priestType},${this.owner.daisType},${this.types[74].maxHitPoints}`;
    }
}

test('Burning 연소점 개수: 10.78 높이 2 부모의 정상/실제 IDIV 예외와 진단 뒤 선택/전역을 실제 PE 2826행 전부와 대조한다', () => {
    assert.equal(rows.length, 2826);
    const scene = new CountScene();
    // 기대값은 기존 C++ 수식으로 만들지 않는다.
    rows.forEach((row, index) => {
        assert.equal(row.length, 7);
        scene.prepare(row[1].split(','));
        const seed = scene.random.state;
        const free = scene.pool.freeCount;
        let outcome = 'ok';
        try { scene.sources.select(scene.fields); } catch (error) {
            if (!(error instanceof BurningSourceCountError)) throw error;
            outcome = error.kind;
        }
        assert.equal(outcome, row[2], `개수 결과 행 ${index} ${row[1]}`);
        assert.equal(scene.post(), row[3], `개수 후상태 행 ${index} ${row[1]}`);
        assert.equal(scene.diagnostics.length === 0 ? '-' : scene.diagnostics.join(';'), row[4], `개수 진단 행 ${index} ${row[1]}`);
        assert.equal(scene.robustCalls, Number(row[6]), `robust 앞단 호출 행 ${index}`);
        assert.ok(scene.random.state === seed && scene.pool.freeCount === free && scene.fields.total === 0 && scene.fields.particles.length === 0, `높이 2 부작용 행 ${index}`);
    });
});

test('Burning 연소점 개수: 장기 owner(사제/다이스/위크 마나)와 현재 다이스 예외를 구별한다', () => {
    const scene = new CountScene();
    scene.prepare(['50', '200', '1', '0', '0', '30', '74', '0']);
    scene.sources.select(scene.fields);
    assert.equal(scene.fields.selected, 1);
    scene.owner.priestType = 75;
    scene.fields.selected = 0;
    scene.sources.select(scene.fields);
    assert.ok(scene.fields.selected === 4 && scene.mode.manaType === 158);
    scene.types[74].flags1 = 0;
    scene.checking = 0x80000000;
    scene.owner.priestQuarterHp = false;
    scene.owner.daisType = 74;
    scene.mode.victim.assertion = undefined;
    scene.fields.selected = 0;
    scene.sources.select(scene.fields);
    assert.ok(scene.fields.selected === 4 && scene.diagnostics.length === 0);
    scene.owner.daisType = 75;
    scene.fields.selected = 0;
    assert.ok(throws(() => scene.sources.select(scene.fields)));
    assert.equal(scene.fields.selected, 0);
});

test('Burning 연소점 개수: HP 진단이 자기 연결을 반납해도 현재 getter 복귀와 이전 HP 저장을 보존한다', () => {
    const scene = new CountScene();
    scene.prepare(['50', '200', '0', '2147483648', '0', '30', '74', '0']);
    scene.mode.victim.assertion = () => {
        scene.mode.victim.assertion = undefined;
        put(scene.pool.allocated(50), 26, 0);
        scene.checking = 0;
    };
    scene.sources.select(scene.fields);
    assert.ok(scene.fields.selected === 4 && get(scene.pool.slot(50), 26) === 0);
});

test('Burning 연소점 개수: 원문 DWORD 감김 정수식과 두 IDIV 예외를 순수 계산으로 확인한다', () => {
    // 분모가 0 인 경우: 최대 HP 0, 최대 HP −… 의 half 와 같아지는 입력은 'zero' 이다.
    assert.throws(() => burningSourceCount(1, 0, 30, 2), (error) => error instanceof BurningSourceCountError && error.kind === 'zero');
    // HP INT_MIN × 5 의 하위 DWORD 는 INT_MIN(5 × 2^31 ≡ 2^31 mod 2^32)이며 분모가 −1 이면 몫 범위 초과이다(최대 HP −2 → half −1 → 분모 −2 − (−1) = −1).
    assert.throws(() => burningSourceCount(-2147483648, -2, 30, 2), (error) => error instanceof BurningSourceCountError && error.kind === 'overflow');
    // 일반 값은 목표를 [1, 행 수] 로 제한한다(최대 200 → half 100 → 분모 100, 분자 250 → 몫 2 → 목표 6 − 2 = 4, 행 수 10).
    assert.equal(burningSourceCount(50, 200, 30, 20), 4);
    assert.equal(burningSourceCount(200, 200, 30, 20), 1);
});
