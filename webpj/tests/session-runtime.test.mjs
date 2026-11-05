// 세션 장기 상태 1단계(WB04-C1)를 검사한다: CRT/게임 난수·시드 초기화·클라이언트 난수 전역·게임 시계·Kernel.
//  - cpppj/tests/fixtures/campaign-seed-x86.tsv: 원본 10.78 시드 초기화/재시드 96행(모드·Unix 초·이전 CRT 조합)의 모드·게임 상태·CRT 상태·시각 호출 수.
//  - cpppj/tests/fixtures/game-clock-reset-x86.tsv: 두 x87 정밀도의 공개 호출 114행(Game/Wall double 비트·IsPaused·Pause/Resume/Reset). raw 24행은 C++ 객체 바이트 배치 증거라 쓰지 않는다.
//  - cpppj ClientRandomGlobalsTests·GameClockResetTests·RecoveryTests(Kernel) 의 계약 검사.
// (2026-11-05 WB04-C1: 세션 장기 상태 1단계 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { ClientRandomGlobals, CrtRandom, FIXED_RANDOM_SEED, GameRandom, GameRandomSeed, INITIAL_GAME_RANDOM_STATE } from '../www/core/random.js';
import { GameClock } from '../www/core/game-clock.js';
import { BaseProcess, Kernel, PROCESS_CAPACITY } from '../www/core/kernel.js';
import { CrtRandom as TerrainCrtRandom, seedGameRandoms } from '../www/assets/terrain-tiles.js';

// 원본 관측 파일 경로이다. (2026-11-05 WB04-C1)
const SEED_FIXTURE = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'campaign-seed-x86.tsv');
const CLOCK_FIXTURE = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'game-clock-reset-x86.tsv');

// 탭 구분 행을 읽는다(주석·빈 줄 제외). 파일이 없으면 null 이다. (2026-11-05 WB04-C1)
function readRows(file) {
    if (!fs.existsSync(file)) return null;
    return fs.readFileSync(file, 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && line[0] !== '#').map((line) => line.split('\t'));
}

const seedRows = readRows(SEED_FIXTURE);
const clockRows = readRows(CLOCK_FIXTURE);

// double 반환값을 원본 출력처럼 리틀엔디언 8바이트 16진 문자열로 바꾼다(허용 오차로 차이를 숨기지 않는다). (2026-11-05 WB04-C1)
function doubleHex(value) {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    return Array.from(new Uint8Array(view.buffer), (byte) => byte.toString(16).padStart(2, '0')).join('');
}

// CRT·게임 난수: MSVC rand 첫 값과 0 상태, 게임 난수의 0 대체·limit 검사, 클라이언트 전역의 새 값과 지형 코드와의 구현 공유.
test('난수: CRT·게임 난수 수열과 클라이언트 전역 초기값', () => {
    const globals = new ClientRandomGlobals();
    assert.deepEqual([globals.crt.state, globals.game.state, globals.gameMode, globals.fortCoreSeed, globals.subscriberIdInitialized], [1, INITIAL_GAME_RANDOM_STATE, 0, 0, 0]);
    // 기존 독립 원문 trace 의 holdrand 1 첫 갱신과 상위 15비트이다.
    assert.equal(globals.crt.next(), 41);
    assert.equal(globals.crt.state, 2745024);
    // 0 도 유효한 CRT 상태이며 게임 난수의 0 대체를 쓰지 않는다.
    globals.crt.setState(0);
    assert.equal(globals.crt.next(), 38);
    assert.equal(globals.crt.state, 2531011);
    assert.throws(() => globals.crt.setState(-1), RangeError);
    // 게임 난수: 0 상태는 0x0bad0bad 로 바뀐 뒤 전진한다.
    const game = new GameRandom(0);
    const expected = Number((((0x0bad0badn * 0x10003n + 3n) & 0xffffffffn) >> 16n) % 1000n);
    assert.equal(game.next(1000), expected);
    assert.equal(game.state, Number((0x0bad0badn * 0x10003n + 3n) & 0xffffffffn));
    assert.throws(() => game.next(0), RangeError);
    // 클라이언트마다 새 전역이며 서로 공유하지 않는다.
    const other = new ClientRandomGlobals();
    assert.equal(other.crt.state, 1);
    assert.notEqual(other.crt, globals.crt);
    // 지형 코드(terrain-tiles)도 같은 CRT 구현을 쓴다. 기존 고정 시드 경로의 결과도 바뀌지 않는다.
    assert.equal(TerrainCrtRandom, CrtRandom);
    const terrain = seedGameRandoms();
    const globalsSeed = new ClientRandomGlobals();
    assert.equal(new GameRandomSeed(globalsSeed, () => 0).initialize(), terrain.gameSeed);
    assert.equal(globalsSeed.crt.state, terrain.crt.state);
});

// 시드 초기화: 원본 10.78 의 Initialize/Reseed 96행을 같은 전역에서 재생한다(모드 0 은 시각을 읽지 않고, Reseed 는 모드를 1 로 잠시 바꾼다).
test('GameRandomSeed: 원본10.78 시드 초기화96행과 같다', { skip: !seedRows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 행마다 모드·Unix 초·이전 CRT 입력으로 초기화한다.
    for (const row of seedRows) {
        const globals = new ClientRandomGlobals();
        globals.game.setState(0x12345678);
        globals.gameMode = Number(row[1]);
        globals.crt.setState(Number(row[3]));
        let calls = 0;
        const seed = new GameRandomSeed(globals, () => {
            calls += 1;
            assert.notEqual(globals.gameMode, 0, '모드 0 에서는 시각을 읽지 않는다');
            return Number(row[2]);
        });
        const value = Number(row[0]) ? seed.reseed() : seed.initialize();
        assert.deepEqual([globals.gameMode, globals.game.state, value, globals.crt.state, calls], [Number(row[4]), Number(row[5]), Number(row[5]), Number(row[6]), Number(row[7])], row.join(' '));
        assert.equal(Number(row[8]), 104, 'CRT 소비는 시드 대입 1 + 100 + 3');
        count += 1;
    }
    assert.equal(count, 96);
});

// 시드 초기화 계약: 시각 공급자 누락 거부, 공급자 예외 때 모드 복원, signed 32비트 밖 시각 거부, 모드 0 고정 시드.
test('GameRandomSeed: 공급자 누락·예외 때 모드 복원·고정 시드', () => {
    const globals = new ClientRandomGlobals();
    assert.throws(() => new GameRandomSeed(globals), TypeError);
    assert.throws(() => new GameRandomSeed({}, () => 0), TypeError);
    const failing = new GameRandomSeed(globals, () => { throw new Error('명시 시각 실패'); });
    assert.throws(() => failing.reseed(), /명시 시각 실패/);
    assert.equal(globals.gameMode, 0);
    globals.gameMode = 7;
    assert.throws(() => new GameRandomSeed(globals, () => 2 ** 31).initialize(), RangeError);
    assert.equal(new GameRandomSeed(globals, () => -1).reseed(), new GameRandomSeed(Object.assign(new ClientRandomGlobals(), { gameMode: 1 }), () => -1).initialize());
    assert.equal(globals.gameMode, 7);
    globals.gameMode = 0;
    new GameRandomSeed(globals, () => { throw new Error('부르지 않아야 함'); }).initialize();
    // 고정 시드로 시작한 CRT 는 104번 전진했다.
    const crt = new CrtRandom(FIXED_RANDOM_SEED);
    for (let index = 0; index < 103; index += 1) crt.next();
    assert.equal(globals.crt.state, crt.state);
});

// 게임 시계: 두 x87 정밀도의 공개 호출 114행을 같은 원점·같은 밀리초 입력으로 재생해 double 비트까지 비교한다.
test('GameClock: 원본10.78 공개 호출114행(double 비트)과 같다', { skip: !clockRows && '원본 fixture 없음' }, () => {
    let key = '';
    let clock = null;
    const counts = { rows: 0, raw: 0, public: 0, resets: 0 };
    // 원문 행을 빠짐없이 분류한다.
    for (const row of clockRows) {
        assert.equal(row.length, 13);
        counts.rows += 1;
        assert.ok(row[0] === '639' || row[0] === '895');
        if (row[1].startsWith('raw')) {
            // C++/원본 객체의 private 바이트 배치 증거이며 웹 객체 배치로 가장하지 않는다.
            assert.equal(row[6], 'Reset');
            counts.raw += 1;
            continue;
        }
        const current = `${row[0]}:${row[1]}`;
        if (current !== key) {
            key = current;
            clock = new GameClock(Number(row[2]));
            assert.equal(row[5], '0');
        }
        const now = Number(row[7]);
        let result = '-';
        if (row[6] === 'Game') result = doubleHex(clock.gameSeconds(now));
        else if (row[6] === 'Wall') result = doubleHex(clock.wallSeconds(now));
        else if (row[6] === 'IsPaused') result = clock.paused ? '1' : '0';
        else if (row[6] === 'Pause') clock.pause(now);
        else if (row[6] === 'Resume') clock.resume(now);
        else if (row[6] === 'Reset') {
            clock.reset();
            counts.resets += 1;
        } else assert.fail(`미해석 명령 ${row[6]}`);
        assert.equal(result, row[8], row.slice(0, 8).join(' '));
        counts.public += 1;
    }
    assert.deepEqual(counts, { rows: 138, raw: 24, public: 114, resets: 22 });
});

// 게임 시계 계약: Reset 은 누적 정지만 지우고 원점·프레임 이력·중첩 정지·저장 게임 시각을 유지한다. 밀리초 감김에서도 같다.
test('GameClock: Reset·중첩 정지·밀리초 감김', () => {
    const clock = new GameClock(1000);
    assert.deepEqual(clock.capture(2000), { game: 1, wall: 1, delta: 1, number: 1 });
    clock.pause(2000);
    clock.resume(5000);
    assert.deepEqual(clock.capture(6000), { game: 2, wall: 5, delta: 1, number: 2 });
    clock.reset();
    assert.deepEqual([clock.paused, clock.gameSeconds(6000), clock.wallSeconds(6000)], [false, 5, 5]);
    assert.deepEqual(clock.capture(6000), { game: 5, wall: 5, delta: 3, number: 3 });
    clock.reset();
    assert.deepEqual(clock.capture(6000), { game: 5, wall: 5, delta: 0, number: 4 });
    // 중첩 정지: 마지막 resume 만 누적한다. reset 은 정지 깊이와 저장 게임 시각을 지우지 않는다.
    const nested = new GameClock(1000);
    nested.pause(2000);
    nested.pause(2500);
    nested.reset();
    assert.deepEqual([nested.paused, nested.gameSeconds(8000)], [true, 1]);
    nested.resume(9000);
    assert.deepEqual([nested.paused, nested.gameSeconds(10000)], [true, 1]);
    nested.reset();
    nested.resume(11000);
    assert.deepEqual([nested.paused, nested.gameSeconds(12000), nested.wallSeconds(12000)], [false, 2, 11]);
    assert.deepEqual(nested.capture(12000), { game: 2, wall: 11, delta: 2, number: 1 });
    nested.reset();
    assert.deepEqual(nested.capture(12000), { game: 11, wall: 11, delta: 9, number: 2 });
    // 정지 중이 아닐 때 resume 은 아무것도 하지 않는다.
    nested.resume(13000);
    assert.equal(nested.gameSeconds(13000), 12);
    // 밀리초 DWORD 감김.
    const origin = 0xfffffe00;
    const at = (elapsed) => (origin + elapsed) >>> 0;
    const wrapped = new GameClock(origin);
    assert.deepEqual(wrapped.capture(at(1000)), { game: 1, wall: 1, delta: 1, number: 1 });
    wrapped.pause(at(1000));
    wrapped.pause(at(1500));
    wrapped.resume(at(2000));
    assert.equal(wrapped.paused, true);
    wrapped.resume(at(3000));
    assert.deepEqual(wrapped.capture(at(4000)), { game: 2, wall: 4, delta: 1, number: 2 });
    wrapped.reset();
    assert.deepEqual(wrapped.capture(at(4000)), { game: 4, wall: 4, delta: 2, number: 3 });
    // 마지막 프레임 사본은 바꿔도 시계에 영향이 없다.
    const frame = wrapped.frame;
    frame.game = 999;
    assert.equal(wrapped.frame.game, 4);
    assert.throws(() => wrapped.capture(-1), RangeError);
    assert.throws(() => new GameClock(2 ** 32), RangeError);
});

// 콜백으로 실행 내용을 정하는 검사용 프로세스이다. (2026-11-05 WB04-C1)
class CallbackProcess extends BaseProcess {
    // 실행 콜백과 소멸 콜백을 받는다.
    constructor(run, destroy = () => {}) {
        super();
        // 한 프레임 실행 콜백이다.
        this.run = run;
        // 소멸 콜백이다.
        this.onDestroy = destroy;
    }

    // 실행 콜백을 부른다.
    runFrame() { this.run(); }

    // 소멸 콜백을 부른다.
    destroy() { this.onDestroy(); }
}

// Kernel: 실행 중 등록은 그 순간의 슬롯으로 판단한다(뒤 슬롯은 같은 프레임, 지난 앞 슬롯은 다음 프레임). cpppj Kernel_MutationUsesLiveSlots 와 같은 장면이다.
test('Kernel: 실행 중 변경은 현재 슬롯 순서를 따른다', () => {
    const kernel = new Kernel();
    const calls = [];
    let added = false;
    let reused = false;
    let phase = 0;
    kernel.add(new CallbackProcess(() => {
        calls.push(1);
        if (!added) {
            added = true;
            kernel.add(new CallbackProcess(() => {
                calls.push(2);
                if (phase === 1 && !reused) {
                    reused = true;
                    kernel.add(new CallbackProcess(() => calls.push(4)));
                }
            }));
        }
    }));
    kernel.runFrame();
    assert.deepEqual(calls, [1, 2]);
    kernel.remove(1);
    phase = 1;
    calls.length = 0;
    kernel.runFrame();
    assert.deepEqual(calls, [2]);
    calls.length = 0;
    kernel.runFrame();
    assert.deepEqual(calls, [4, 2]);
    assert.equal(kernel.size, 2);
    assert.equal(PROCESS_CAPACITY, 39999);
    assert.throws(() => kernel.remove(0), RangeError);
    assert.throws(() => kernel.remove(39999), RangeError);
});

// Kernel 계약: 같은 객체 재등록은 기존 번호(뒤 슬롯이라도), 소멸 중에는 원래 슬롯이 보임, clear 는 한 번 순회(지난 앞 슬롯 신규는 남음), dispose 는 모두 정리, 슬롯 소진 오류.
test('Kernel: 재등록·소멸 순서·clear 한 번 순회·dispose·소진', () => {
    const kernel = new Kernel(4);
    const a = new CallbackProcess(() => {});
    const b = new CallbackProcess(() => {});
    assert.equal(kernel.add(a), 1);
    assert.equal(kernel.add(b), 2);
    kernel.remove(1);
    // 앞 슬롯 1 이 비어 있어도 같은 객체 b 는 기존 번호 2 를 돌려준다.
    assert.equal(kernel.add(b), 2);
    assert.equal(kernel.size, 1);
    assert.equal(kernel.find(b), 2);
    assert.equal(kernel.find(a), 0);
    assert.equal(kernel.get(0), null);
    assert.equal(kernel.get(5), null);
    kernel.clear();
    assert.equal(kernel.size, 0);
    // 소멸 처리 안에서는 원래 슬롯이 아직 차 있다: 자기 번호가 보이고, 새 등록은 다음 빈 슬롯으로 간다.
    const late = new CallbackProcess(() => {});
    let seen = null;
    const c = new CallbackProcess(() => {}, () => {
        seen = kernel.find(c);
        kernel.add(late);
    });
    assert.equal(kernel.add(c), 1);
    kernel.remove(1);
    assert.equal(seen, 1, '소멸 중에는 원래 슬롯이 보인다');
    assert.equal(kernel.find(late), 2);
    assert.equal(kernel.get(1), null);
    // clear 는 한 번만 순회한다: 슬롯 3 의 소멸 처리가 이미 지난 앞 슬롯 1 에 등록한 프로세스는 남는다.
    assert.equal(kernel.add(new CallbackProcess(() => {})), 1);
    let fresh = null;
    const d = new CallbackProcess(() => {}, () => {
        fresh = new CallbackProcess(() => {});
        kernel.add(fresh);
    });
    assert.equal(kernel.add(d), 3);
    kernel.clear();
    assert.equal(kernel.size, 1);
    assert.equal(kernel.get(1), fresh);
    // 호스트 수명 종료 정리는 남은 것까지 지운다.
    kernel.dispose();
    assert.equal(kernel.size, 0);
    // 슬롯 소진·잘못된 프로세스.
    for (let index = 0; index < 4; index += 1) kernel.add(new CallbackProcess(() => {}));
    assert.throws(() => kernel.add(new CallbackProcess(() => {})), /Out of processes/);
    assert.throws(() => kernel.add({}), TypeError);
    assert.throws(() => new Kernel(0), RangeError);
    assert.throws(() => new Kernel(40000), RangeError);
});
