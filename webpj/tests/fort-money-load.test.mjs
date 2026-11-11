// Fort Money 섹션 읽기와 현재 Player SP 지정 정책(cpppj RawFortMoneyLoad, 원본 004bf340 → 00483300 · 00447810 · 0040ef30)의 웹 대응 `core/raw-fort-money-load.js` 를 검사한다(4-B, WB04-E278).
//  - 원본 관측(cpppj/tests/fixtures/fortmoney-x86.tsv, originals 행만 244행): wrapper(Load)·직접 SP 지정(SetPlayerSp)·본문(ReadMoney) 세 진입점을 같은 입력(모드·owner·난이도·flags 마스크·seed/fill·초기화 여부·스트림 pos/end)에서 실행해
//    깊이·pos·SP 검사 잔여 횟수·분산 저장소 초기화/offset/게임 난수 상태·owner 0..8 의 SP 비트·전체 288 DWORD 분산 배열·효과 순서(진단 A:T/A:P, 표시 U)·진단 횟수를 대조하고 Player 기록이 변하지 않음을 확인한다.
//    fixture 의 originalCD 164행은 CD 10.72 몫이라 웹 목표(10.78) 밖이다.
//  - 손 시험(cpppj FortMoneyLoadTests): 로컬 owner/모드 변경이 즉시 반영되는 공유(RawSurfaceWorld 연결은 이 단계의 RawSurfaceWorld 항목에서 같은 월드로 다시 닫는다)·깊이 증가 뒤 Template 진단 → 실제 섹션 선택 → 깊이 복구와 실패 시 복구.
//    ForeignRitualRewardTests 의 getter 계약(비활성/비로컬 0 은 lazy 저장소를 읽지 않고 활성 로컬은 비교 뒤 로컬 owner 를 다시 조회).
//  - 계약: fixture 가 구분하지 못하는 경계(32비트 감김·signed 끝 비교·범위 밖 owner·float 변환·미연결 장치 거부)를 직접 검사한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E278)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { SquidRewardState } from '../www/core/squid-reward.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { FORT_MONEY_ASSERTION, FortMoneyState, RawFortMoneyLoad, readPlayerSpAmount, setPlayerSpAmount } from '../www/core/raw-fort-money-load.js';

// 원본 관측 파일과 패치 10.78 행(originals)이다. 파일이 없으면 해당 검사를 건너뛴다. (2026-11-11 WB04-E278)
const MONEY_ROWS = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'fortmoney-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 패치 Player 기록 stride 와 flags 위치·제외 비트·개수이다. (2026-11-11 WB04-E278)
const STRIDE = 0xb4, FLAGS = 0x50, EXCLUDED = 0x20, PLAYERS = 9;

// 예외가 나는지 본다. (2026-11-11 WB04-E278)
function throws(action) {
    try {
        action();
        return false;
    } catch {
        return true;
    }
}

// 소문자 16진수 문자열이다(빈 배열은 '-'). (2026-11-11 WB04-E278)
function hex(bytes) {
    return bytes.length === 0 ? '-' : Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');
}

// fixture 한 행의 입력으로 만든 Money 장면이다: 모드·owner·난이도·flags 마스크·시드/채움값·초기화·스트림을 원본 입력 그대로 구성한다(기대값은 계산하지 않는다). (2026-11-11 WB04-E278)
function moneyScene(row) {
    const values = row[1].split(',').map(Number);
    if (values.length !== 19) throw new Error(`Money 입력 열 수 오류: ${values.length}`);
    const scene = {
        kind: values[0], owner: values[17] | 0, amount: values[18] | 0, stream: { position: values[11], end: values[12] },
        reward: new SquidRewardState(), players: new SquidPostPopState(), records: new Uint8Array(PLAYERS * STRIDE), depth: { value: values[10] },
        state: new FortMoneyState(), random: new GameRandom(), events: [], depths: [], assertions: 0,
    };
    scene.reward.fixedSpA = values[1] !== 0;
    scene.reward.halfHealthMode = values[2] !== 0;
    scene.state.economyBlocked = values[3] !== 0;
    scene.state.clientActive = values[4] !== 0;
    scene.reward.fixedSpB = values[5] !== 0;
    scene.players.localOwner = values[6];
    scene.state.difficulty = values[7];
    scene.state.templatePresent = values[13] !== 0;
    scene.state.spCheckCount = 17;
    // 9개 Player 의 존재 여부를 추정하지 않고 원본 flags BYTE 입력만 쓴다.
    for (let index = 0; index < PLAYERS; index += 1) scene.records[index * STRIDE + FLAGS] = (values[9] & (1 << index)) ? EXCLUDED : 0;
    scene.store = new ScrambledSpStore(scene.random, () => values[14], values[15]);
    if (values[16]) scene.store.initialize();
    scene.money = new RawFortMoneyLoad(scene.reward, scene.players, scene.records, scene.depth, scene.state, scene.store, {
        assertion: (code) => {
            scene.assertions += 1;
            scene.events.push(code === FORT_MONEY_ASSERTION.missingTemplate ? 'A:T' : 'A:P');
        },
        loadingDepthChanged: (value) => scene.depths.push(value),
        refreshSp: () => scene.events.push('U'),
    });
    return scene;
}

// 손 시험용 빈 Money 장면이다(기본 모드 꺼짐). 필요한 칸만 호출 쪽이 바꾼다. (2026-11-11 WB04-E278)
function handScene(extra = {}) {
    const scene = {
        reward: new SquidRewardState(), players: new SquidPostPopState(), state: new FortMoneyState(), records: new Uint8Array(PLAYERS * STRIDE), depth: { value: 3 },
        random: new GameRandom(12345), effects: [], ...extra,
    };
    scene.store = new ScrambledSpStore(scene.random, () => 12345);
    return scene;
}

// 원본 10.78 244입력: 세 진입점의 깊이/pos/검사 횟수/저장소/난수/전체 분산 배열/효과 순서/진단이 같다. (2026-11-11 WB04-E278)
test('FortMoney: 원본 10.78 244입력의 wrapper·직접 SP 지정·본문 결과(분산 288 DWORD·난수·효과 순서)가 같다', { skip: MONEY_ROWS ? false : 'fortmoney-x86.tsv 없음' }, () => {
    let count = 0;
    // 각 원본 입력을 생산 모듈의 같은 진입점으로 실행한다.
    for (const row of MONEY_ROWS) {
        assert.equal(row.length, 12);
        const scene = moneyScene(row);
        const before = scene.records.slice();
        if (scene.kind === 1) scene.money.setPlayerSp(scene.owner, scene.amount);
        else if (scene.kind === 2) scene.money.readMoney(scene.stream);
        else scene.money.load(scene.stream);
        let values = '-', pools = '-';
        if (scene.store.initialized) {
            // 각 owner 의 실제 SP get 을 읽는다(이미 초기화된 저장소는 난수를 소비하지 않는다).
            values = Array.from({ length: PLAYERS }, (_, owner) => scene.store.get(owner)).join(',');
            // 원본 288 DWORD 순서대로 현재 전체 물리 배열을 리틀엔디언으로 모은다.
            const raw = new Uint8Array(32 * PLAYERS * 4);
            scene.store.pools().forEach((word, index) => put(raw, index * 4, word));
            pools = hex(raw);
        }
        const events = scene.events.length === 0 ? '-' : scene.events.join(';');
        const where = `행 ${count + 1}: ${row[1]}`;
        assert.equal(scene.depth.value, Number(row[2]), `깊이 ${where}`);
        assert.equal(scene.stream.position, Number(row[3]), `pos ${where}`);
        assert.equal(scene.state.spCheckCount, Number(row[4]), `검사 잔여 횟수 ${where}`);
        assert.equal(scene.store.initialized, Number(row[5]) !== 0, `초기화 ${where}`);
        assert.equal(scene.store.offset, Number(row[6]), `offset ${where}`);
        assert.equal(scene.random.state, Number(row[7]), `난수 ${where}`);
        assert.equal(values, row[8], `SP 비트 ${where}`);
        assert.equal(pools, row[9], `분산 배열 ${where}`);
        assert.equal(events, row[10], `효과 순서 ${where}`);
        assert.equal(scene.assertions, Number(row[11]), `진단 횟수 ${where}`);
        assert.deepEqual(scene.records, before, `Player 기록 불변 ${where}`);
        count += 1;
    }
    assert.equal(count, 244);
});

// 로컬 owner/모드 변경이 즉시 반영된다: 같은 store/random 을 쓰며 로컬 활성이면 표시를 갱신한다(cpppj SharesCurrentWorldOwnerStoreAndRandom 의 장부·저장소 부분). (2026-11-11 WB04-E278)
test('FortMoney: 현재 로컬 owner/special 모드를 호출마다 다시 읽고 같은 저장소·난수를 쓴다', () => {
    const scene = handScene();
    let refreshes = 0;
    scene.state.clientActive = true;
    scene.players.localOwner = 1;
    const money = new RawFortMoneyLoad(scene.reward, scene.players, scene.records, scene.depth, scene.state, scene.store, {
        assertion: () => { throw new Error('정상 Money 소비자의 진단'); },
        loadingDepthChanged: () => {},
        refreshSp: () => { refreshes += 1; },
    });
    money.setPlayerSp(1, 7000);
    assert.equal(scene.store.get(1), floatToBits(7000));
    assert.equal(refreshes, 1);
    scene.players.localOwner = 2;
    scene.reward.fixedSpB = true;
    money.setPlayerSp(2, 3000);
    assert.equal(scene.store.get(2), floatToBits(194162));
    assert.equal(refreshes, 2);
    assert.notEqual(scene.random.state, 0);
    assert.equal(scene.depth.value, 3);
});

// 진단에서 현재 섹션을 바꾸면 새 getter 는 증가한 깊이에서 변경된 cursor 를 소비하고 실패 때 깊이를 복구한다(cpppj SelectsAfterDepthAndTemplateDiagnostic). (2026-11-11 WB04-E278)
test('FortMoney: 깊이 증가 → Template 진단 → 실제 섹션 선택 → 깊이 복구, 선택 실패/누락도 깊이를 복구한다', () => {
    const scene = handScene();
    scene.reward.fixedSpA = true;
    scene.players.localOwner = 1;
    scene.state.templatePresent = false;
    const first = { position: 17, end: 4 }, second = { position: 19, end: 4 };
    let current = first;
    const effects = [];
    const money = new RawFortMoneyLoad(scene.reward, scene.players, scene.records, scene.depth, scene.state, scene.store, {
        assertion: (reason) => {
            assert.equal(reason, FORT_MONEY_ASSERTION.missingTemplate);
            assert.equal(scene.depth.value, 4);
            effects.push('A');
            current = second;
            scene.state.templatePresent = true;
        },
        loadingDepthChanged: (value) => {
            assert.equal(value, scene.depth.value);
            effects.push(`L${value}`);
        },
        refreshSp: () => { throw new Error('고정 Fort SP 의 예상 밖 표시'); },
    });
    money.loadSelected(() => {
        assert.equal(scene.depth.value, 4);
        assert.equal(current, second);
        effects.push('S4');
        current.position = 0;
        return current;
    });
    assert.deepEqual(effects, ['L4', 'A', 'S4', 'L3']);
    assert.equal(first.position, 17);
    assert.equal(second.position, 4);
    assert.equal(scene.depth.value, 3);
    assert.equal(scene.store.get(1), floatToBits(194162));
    effects.length = 0;
    assert.ok(throws(() => money.loadSelected(() => {
        assert.equal(scene.depth.value, 4);
        effects.push('SF');
        throw new Error('선택 파일 실패 입력');
    })));
    assert.deepEqual(effects, ['L4', 'SF', 'L3']);
    assert.equal(scene.depth.value, 3);
    effects.length = 0;
    assert.ok(throws(() => money.loadSelected(null)));
    assert.deepEqual(effects, []);
    assert.equal(scene.depth.value, 3);
});

// 비활성/비로컬 0 은 lazy 저장소를 읽지 않으며 활성 로컬은 비교 뒤 현재 owner 를 다시 조회한다(cpppj ForeignRitualReward_GetterPreservesLazyAndCurrentOwnerReads). (2026-11-11 WB04-E278)
test('FortMoney: SP 읽기는 비활성/비로컬에서 lazy 저장소를 만지지 않고 활성 로컬은 owner 를 다시 조회한다', () => {
    const random = new GameRandom();
    const store = new ScrambledSpStore(random, () => 7);
    let localReads = 0;
    assert.equal(readPlayerSpAmount(0, store, { clientActive: () => 0 }), 0);
    assert.equal(store.initialized, false);
    assert.equal(readPlayerSpAmount(0xffffffff, store, { clientActive: () => 1, localOwner: () => 2 }), 0);
    assert.equal(store.initialized, false);
    store.set(8, floatToBits(123.75));
    const balance = readPlayerSpAmount(0, store, { clientActive: () => 1, localOwner: () => (localReads++ ? 8 : 0) });
    assert.equal(balance, 123.75);
    assert.equal(localReads, 2);
    // 비로컬 signed 1~8 은 그 owner 의 잔액이고 9 이상/0 은 0 이다(저장소는 초기화되어 있어도 0 반환).
    store.set(3, floatToBits(-5.5));
    assert.equal(readPlayerSpAmount(3, store, { clientActive: () => 0 }), -5.5);
    assert.equal(readPlayerSpAmount(9, store, { clientActive: () => 0 }), 0);
    assert.equal(readPlayerSpAmount(0, store, { clientActive: () => 0 }), 0);
});

// 계약: fixture 가 구분하지 못하는 경계 — 32비트 감김, 끝과의 signed 비교, 범위 밖 owner, int32→float 변환, 필요한 장치 위치에서만 거부. (2026-11-11 WB04-E278)
test('FortMoney 계약: pos 는 32비트로 감기고 끝과 signed 비교하며 범위 밖 owner 는 저장하지 않는다', () => {
    const scene = handScene();
    const log = [];
    const money = new RawFortMoneyLoad(scene.reward, scene.players, scene.records, scene.depth, scene.state, scene.store, {
        assertion: (code) => log.push(code), loadingDepthChanged: () => {}, refreshSp: () => log.push('U'),
    });
    // signed 비교: pos 0x7ffffffe + 4 = 0x80000002(음수), end 0 은 그보다 크므로 진단이 없다(부호 없는 비교면 진단이 난다).
    const wrapped = { position: 0x7ffffffe, end: 0 };
    money.readMoney(wrapped);
    assert.equal(wrapped.position, 0x80000002);
    assert.deepEqual(log, []);
    // end -1(0xffffffff)은 pos 4 보다 작아 진단이 난다(부호 없는 비교면 나지 않는다).
    money.readMoney({ position: 0, end: 0xffffffff });
    assert.deepEqual(log, [FORT_MONEY_ASSERTION.streamPastEnd]);
    // 32비트 감김: 0xfffffffc + 4 = 0.
    const around = { position: 0xfffffffc, end: 0 };
    money.readMoney(around);
    assert.equal(around.position, 0);
    // 비활성 클라이언트에서 owner 0/9/-1(0xffffffff) 은 저장하지 않고 장치(editor)도 요구하지 않는다.
    const none = new ScrambledSpStore(new GameRandom(), () => 7);
    setPlayerSpAmount(0, 5, none, { clientActive: () => 0 }, null);
    setPlayerSpAmount(9, 5, none, { clientActive: () => 0 }, null);
    setPlayerSpAmount(0xffffffff, 5, none, { clientActive: () => 0 }, null);
    assert.equal(none.initialized, false);
    // 범위 안 owner 에서 editor 장치가 없으면 빈 성공이 아니라 거부하고 저장하지 않는다.
    assert.ok(throws(() => setPlayerSpAmount(3, 5, none, { clientActive: () => 0 }, null)));
    assert.equal(none.initialized, false);
    // int32 → float: 2^24+1 은 2^24 로 반올림, 음수와 0xffffffff(-1) 도 signed 로 바뀐다.
    const reads = { clientActive: () => 0, editor: () => 0 };
    setPlayerSpAmount(1, 16777217, none, reads, null);
    setPlayerSpAmount(2, 0xffffffff, none, reads, null);
    setPlayerSpAmount(3, -7, none, reads, null);
    assert.equal(none.get(1), floatToBits(16777216));
    assert.equal(none.get(2), floatToBits(-1));
    assert.equal(none.get(3), floatToBits(-7));
    // 활성 로컬 분기는 비교 뒤 로컬 owner 를 다시 조회해 그 번호에 저장한다(첫 조회 0 은 같은 owner, 다시 조회한 8 에 쓴다).
    const relook = new ScrambledSpStore(new GameRandom(), () => 7);
    let lookups = 0;
    setPlayerSpAmount(0, 11, relook, { clientActive: () => 1, localOwner: () => (lookups++ ? 8 : 0), editor: () => 0 }, () => {});
    assert.equal(lookups, 2);
    assert.equal(relook.get(8), floatToBits(11));
    assert.equal(relook.get(0), 0);
    // 활성 로컬 분기는 저장 뒤에 표시 갱신을 요구한다(없으면 저장은 이미 끝난 채 거부).
    const local = new ScrambledSpStore(new GameRandom(), () => 7);
    const localReads = { clientActive: () => 1, localOwner: () => 4, editor: () => 0 };
    assert.ok(throws(() => setPlayerSpAmount(4, 9, local, localReads, null)));
    assert.equal(local.get(4), floatToBits(9));
});

// 계약: 난이도 3 이상·플레이어 번호 밖·생성 누락은 거부하고, 제외 비트 Player 는 지정에서 빠지며 fort 고정 SP 는 로컬 owner 에 쓴다. (2026-11-11 WB04-E278)
test('FortMoney 계약: 시작 SP 는 난이도별 값+4000 을 제외 비트 없는 Player 1..8 에 쓰고 범위 밖 난이도/생성 누락은 거부한다', () => {
    const hooks = { assertion: () => {}, loadingDepthChanged: () => {}, refreshSp: () => {} };
    const scene = handScene();
    scene.reward.halfHealthMode = true;
    scene.records[2 * STRIDE + FLAGS] = EXCLUDED;
    scene.records[5 * STRIDE + FLAGS] = 0xdf; // 0x20 비트만 없는 값은 제외 대상이 아니다.
    const money = new RawFortMoneyLoad(scene.reward, scene.players, scene.records, scene.depth, scene.state, scene.store, hooks);
    scene.state.difficulty = 2;
    money.readMoney({ position: 0, end: 4 });
    for (let owner = 1; owner < PLAYERS; owner += 1) {
        if (owner === 2) continue;
        assert.equal(bits(scene.store.get(owner)), 7000, `owner ${owner}`);
    }
    // 제외된 owner 2 는 한 번도 쓰이지 않았으므로 초기 채움(0) 의 float 0 이다.
    assert.equal(scene.store.get(2), 0);
    // 난이도 0·1 의 시작 SP(1000+4000·2500+4000)도 난이도별 배열에서 읽는다(원본 관측이 세 값을 모두 구분하지는 못한다).
    for (const [difficulty, expected] of [[0, 5000], [1, 6500]]) {
        scene.state.difficulty = difficulty;
        money.readMoney({ position: 0, end: 4 });
        assert.equal(bits(scene.store.get(1)), expected, `난이도 ${difficulty}`);
    }
    // 루프는 owner 1 부터이다: 활성 로컬 owner 가 0 이어도 owner 0 은 지정하지 않고 표시도 갱신하지 않는다.
    const zero = handScene();
    let refreshes = 0;
    zero.reward.halfHealthMode = true;
    zero.state.clientActive = true;
    zero.players.localOwner = 0;
    new RawFortMoneyLoad(zero.reward, zero.players, zero.records, zero.depth, zero.state, zero.store, { ...hooks, refreshSp: () => { refreshes += 1; } }).readMoney({ position: 0, end: 4 });
    assert.equal(refreshes, 0);
    assert.equal(zero.store.get(0), 0);
    // 난이도 3 이상은 배열 밖이라 거부한다.
    scene.state.difficulty = 3;
    assert.ok(throws(() => money.readMoney({ position: 0, end: 4 })));
    // 경제 차단이면 시작 SP 를 건너뛰고 난이도도 읽지 않는다.
    scene.state.economyBlocked = true;
    money.readMoney({ position: 0, end: 4 });
    // 생성 때 필수 장치/기록 크기/저장소가 없으면 거부한다.
    assert.ok(throws(() => new RawFortMoneyLoad(scene.reward, scene.players, scene.records.subarray(0, 100), scene.depth, scene.state, scene.store, hooks)));
    assert.ok(throws(() => new RawFortMoneyLoad(scene.reward, scene.players, scene.records, scene.depth, scene.state, null, hooks)));
    assert.ok(throws(() => new RawFortMoneyLoad(scene.reward, scene.players, scene.records, scene.depth, scene.state, scene.store, { ...hooks, refreshSp: null })));
    assert.ok(throws(() => money.playerFlags(9)));
});

// float 비트로 읽은 SP 를 수로 바꾼다. (2026-11-11 WB04-E278)
function bits(word) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, word >>> 0, true);
    return view.getFloat32(0, true);
}
