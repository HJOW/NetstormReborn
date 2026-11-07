// Player 참여(RawPlayerParticipation: 로그인·방향별 동맹·AI 부착/해제·State 반납 증빙)를 원본 10.78 관측으로 대조한다(WB04-E5, cpppj PlayerParticipationTests 이식).
//  - player-participation-x86.tsv 608행: 전체 Player 9×180 BYTE·방향표 81 DWORD(324 BYTE)·외부효과 순서를 같은 독립 입력 13개에서 대조한다(Login/Alliance/Add/Attach/Remove/RemoveAll).
//  - player-alliance-packet-x86.tsv 288행: 41 BYTE 보호 저장소 안의 9 BYTE 수신 packet 이 바뀐 뒤 broadcast 가 같은 객체를 읽는 순서까지 대조한다.
//  - 계약: 방향별 표의 독립성, 파괴/반납 중 이전 brain 조회, BrainResetPass 의 단일 소비·타 모듈 거부·진행 중 소비 거부·소멸한 모듈 증빙 거부.
// cpppj 의 RawAiCreate 연동 검사(닫힌 owner 에서 생성 거부)는 RawAiCreate 이식 단위로 미룬다. 원본 fixture 두 개는 10.78 전용이다.
// (2026-11-08 WB04-E5: Player 참여 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { BrainResetPass, PLAYER_PARTICIPATION_ASSERTION, PlayerParticipationState, RawPlayerParticipation } from '../www/core/raw-player-participation.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E5)
const participationRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'player-participation-x86.tsv'));
const packetRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'player-alliance-packet-x86.tsv'));

// 바이트를 낮은 쪽부터 소문자 hex 로 기록한다. (2026-11-08 WB04-E5)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// fixture 숫자 열을 32비트로 감아 읽는다(음수는 two's complement). (2026-11-08 WB04-E5)
const number = (text) => Number(BigInt(text) & 0xffffffffn);

// 원본 상태·효과를 준비하며 기대 로그인/동맹/AI 판단은 계산하지 않는다(cpppj ParticipationScene). (2026-11-08 WB04-E5)
class ParticipationScene {
    // 독립 입력 13(또는 14)개를 현재 raw 자료에만 적용한다.
    constructor(fields) {
        assert.ok(fields.length === 13 || fields.length === 14);
        this.players = new Uint8Array(1620);
        this.allies = new Uint32Array(81);
        this.hidden = new Uint8Array(9);
        this.state = new PlayerParticipationState();
        this.trace = '';
        const owner = Number(BigInt(fields[0]));
        this.state.playerKnown = number(fields[6]);
        this.state.localOwner = number(fields[7]);
        this.state.metamap = number(fields[8]) !== 0;
        this.state.battle = number(fields[9]) !== 0;
        this.state.localTransport = number(fields[10]) !== 0;
        // 실제 Player 이름/Subscriber/채움 바이트와 방향별 표 초기 패턴이다.
        for (let player = 0; player < 9; player += 1) {
            const name = `P${player}`;
            for (let index = 0; index < name.length; index += 1) this.players[player * 180 + 12 + index] = name.charCodeAt(index);
            put(this.players, player * 180 + 44, player + 100);
            this.players.fill(0x5a, player * 180 + 48, player * 180 + 78);
            this.players[player * 180 + 78] = 0x7e;
            this.players[player * 180 + 79] = 127;
            this.hidden[player] = (number(fields[11]) >>> player) & 1;
            // 대칭으로 완성한 동맹이 아닌 입력 패턴이며 원본과 클론에 똑같이 공급한다.
            for (let target = 0; target < 9; target += 1) this.allies[player * 9 + target] = player === target ? 1 : ((player + target) % 3 === 0 ? 7 : 0);
        }
        const selected = owner >= 0 && owner < 9 ? owner : 0;
        put(this.players, selected * 180 + 80, number(fields[3]));
        put(this.players, selected * 180 + 84, number(fields[2]));
        put(this.players, selected * 180 + 176, number(fields[4]));
        put(this.players, selected * 180 + 44, number(fields[5]));
        // 선택 Player 이름 길이 입력이 있으면 'N' 을 채우고 NUL 로 닫는다.
        if (fields.length === 14) {
            const length = number(fields[13]);
            this.players.fill(0x4e, selected * 180 + 12, selected * 180 + 12 + length);
            this.players[selected * 180 + 12 + length] = 0;
        }
        this.hooks = {
            assertion: (kind) => this.add(`A,${kind === PLAYER_PARTICIPATION_ASSERTION.brainAlreadyAssigned ? 'B' : 'M'}`),
            assignConnection: (pending, target) => this.add(`M,${pending >>> 0},${target >>> 0}`),
            rejectLogin: (pending) => this.add(`R,${pending >>> 0}`),
            disconnect: (pending) => this.add(`D,${pending >>> 0}`),
            clearChat: (channel) => this.add(`X,${channel}`),
            addChat: (subscriber, name, channel) => this.add(`C,${subscriber},${Array.from(name, (char) => char.charCodeAt(0).toString(16).padStart(2, '0')).join('')},${channel}`),
            sendAlliance: (server, player, mask) => this.add(`S,${server},${player},${mask}`),
            destructBrain: (token) => this.add(`K,${token}`),
            freeBrain: (token) => this.add(`F,${token}`),
        };
    }

    // 실제 발생한 효과의 순서만 기록한다.
    add(value) { this.trace += (this.trace ? ';' : '') + value; }

    // 현재 저장소를 빌리는 처리기를 만든다(게임 객체/brain 은 생성하지 않는다).
    runtime() { return new RawPlayerParticipation(this.players, this.allies, this.hidden, this.state, this.hooks); }

    // 방향표 전체를 원본 DWORD 바이트 순서로 직렬화한다(0/1 로 정규화하지 않음).
    allyBytes() {
        const bytes = new Uint8Array(324);
        for (let index = 0; index < 81; index += 1) put(bytes, index * 4, this.allies[index]);
        return hex(bytes);
    }

    // 효과가 없으면 '-' 로 표기한다.
    effects() { return this.trace || '-'; }
}

// 전체 기록 1620 BYTE·행렬 324 BYTE·모든 효과를 고유 입력 608개에서 대조한다.
test('RawPlayerParticipation: 원본10.78 로그인·동맹·AI 부착/해제 608행', { skip: !participationRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of participationRows) {
        if (row[0] === 'kind') continue;
        assert.equal(row.length, 6);
        const fields = row[1].split(',');
        const scene = new ParticipationScene(fields);
        const runtime = scene.runtime();
        const owner = number(fields[0]);
        const label = `행 ${count} ${row[0]} ${row[1]}`;
        if (row[0] === 'Login') assert.equal(runtime.checkLogin(Number(fields[1]), Number(fields[0])), Number(row[2]) !== 0, label);
        else if (row[0] === 'Alliance') runtime.setAlliance(owner, number(fields[2]));
        else if (row[0] === 'Add') runtime.addAlly(owner, number(fields[1]));
        else if (row[0] === 'Attach') runtime.attachBrain(owner, number(fields[12]));
        else if (row[0] === 'Remove') runtime.removeBrain(owner);
        else if (row[0] === 'RemoveAll') runtime.removeAllBrains();
        else assert.fail(`알 수 없는 종류 ${row[0]}`);
        assert.equal(hex(scene.players), row[3], `${label} Player`);
        assert.equal(scene.allyBytes(), row[4], `${label} 방향표`);
        assert.equal(scene.effects(), row[5], `${label} 효과`);
        count += 1;
    }
    assert.equal(count, 608);
});

// 원본 수신 288개 입력의 전체 Player/방향표/현재 packet 과 모든 효과를 두 정밀도 원문에 대조한다.
test('RawPlayerParticipation: 원본10.78 동맹 packet 수신 288행', { skip: !packetRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of packetRows) {
        if (row[0] === 'input') continue;
        const values = row[0].split(',');
        assert.equal(values.length, 5);
        assert.equal(row.length, 9);
        const scene = new ParticipationScene([values[0], '1', '1515870810', '0', '0', '0', values[2], values[3], values[4], '0', '0', '341', '7002']);
        const runtime = scene.runtime();
        const storage = new Uint8Array(41).fill(0xa7);
        const packet = storage.subarray(16, 25);
        packet[0] = 1;
        packet[1] = 0x3d;
        packet[2] = 9;
        packet[3] = 0;
        packet[4] = Number(values[0]);
        put(packet, 5, Number(BigInt(values[1]) & 0xffffffffn));
        runtime.receiveAlliance(packet, (bytes) => {
            assert.equal(bytes, packet, '같은 packet 객체를 broadcast 가 읽는다');
            scene.add(`B,${hex(bytes)}`);
        });
        const actual = [hex(scene.players), scene.allyBytes(), scene.effects(), hex(packet)];
        const label = `행 ${count} ${row[0]}`;
        assert.deepEqual(actual, row.slice(1, 5), `${label} 53비트 정밀도`);
        assert.deepEqual(actual, row.slice(5), `${label} 64비트 정밀도`);
        // packet 바깥의 보호 바이트는 보존한다.
        assert.ok(storage.subarray(0, 16).every((byte) => byte === 0xa7) && storage.subarray(25).every((byte) => byte === 0xa7), label);
        count += 1;
    }
    assert.equal(count, 288);
});

// 같은 brain 토큰은 파괴/반납 경계가 모두 끝날 때까지 조회되며 flags 0x20 은 해제 뒤에도 원본대로 유지된다.
test('RawPlayerParticipation: brain 토큰 수명과 방향별 동맹 독립성', () => {
    const scene = new ParticipationScene('1,1,0,0,0,0,0,1,0,0,0,0,7002'.split(','));
    const runtime = scene.runtime();
    runtime.attachBrain(1, 7002);
    assert.equal(runtime.brain(1), 7002);
    let observed = 0;
    scene.hooks.destructBrain = (token) => { assert.equal(token, 7002); assert.equal(runtime.brain(1), 7002); observed += 1; };
    scene.hooks.freeBrain = (token) => { assert.equal(token, 7002); assert.equal(runtime.brain(1), 7002); observed += 1; };
    runtime.removeBrain(1);
    assert.equal(observed, 2);
    assert.equal(runtime.brain(1), 0);
    assert.ok(scene.players[180 + 80] & 0x20);
    // 서로 다른 방향은 독립이며 한쪽 설정이 반대 방향을 바꾸지 않는다.
    runtime.setAlliance(1, 1 << 2);
    assert.equal(scene.allies[1 * 9 + 2], 1);
    assert.equal(scene.allies[2 * 9 + 1], 7);
    runtime.setAlliance(2, 0);
    assert.equal(scene.allies[1 * 9 + 2], 1);
    assert.equal(scene.allies[2 * 9 + 1], 0);
});

// 실제 owner 1..8 자식만 정리하며 정상 복귀 증빙을 소비한 뒤 같은 owner 의 설치/반복 효과를 닫는다.
test('RawPlayerParticipation: State 반납 증빙이 owner1..8 을 닫는다', () => {
    const scene = new ParticipationScene('1,1,0,0,0,0,0,1,0,0,0,0,7002'.split(','));
    const runtime = scene.runtime();
    put(scene.players, 0xb0, 9000);
    for (let owner = 1; owner < 9; owner += 1) {
        put(scene.players, owner * 180 + 0xb0, 7000 + owner);
        put(scene.players, owner * 180 + 0x50, 0x12340020);
        const pass = runtime.removeBrainForState(owner);
        assert.ok(pass instanceof BrainResetPass);
        assert.equal(pass.owner, owner);
        assert.equal(runtime.brainReturnedForState(owner), false);
        assert.equal(runtime.brain(owner), 0);
        runtime.returnBrainForState(pass);
        assert.equal(runtime.brainReturnedForState(owner), true);
        assert.throws(() => runtime.attachBrain(owner, 8000 + owner));
        assert.throws(() => runtime.removeBrain(owner));
        assert.ok((scene.players[owner * 180 + 0x50] & 0x20) !== 0);
    }
    assert.equal(runtime.brain(0), 9000);
    assert.equal(runtime.brainReturnedForState(0), false);
    assert.equal(scene.trace, 'K,7001;F,7001;K,7002;F,7002;K,7003;F,7003;K,7004;F,7004;K,7005;F,7005;K,7006;F,7006;K,7007;F,7007;K,7008;F,7008');
    assert.throws(() => runtime.removeBrainForState(0));
    assert.throws(() => runtime.removeBrainForState(9));
});

// 실제 free 실패에서는 정상 복귀 증빙이 없고 파괴 선행효과와 아직 0이 아닌 현재 brain 을 그대로 남긴다.
test('RawPlayerParticipation: 반납 효과 실패는 증빙을 발급하지 않고 선행 효과를 보존한다', () => {
    const scene = new ParticipationScene('1,1,0,0,7002,0,0,1,0,0,0,0,7002'.split(','));
    scene.hooks.freeBrain = (token) => { scene.add(`F,${token}`); throw new Error('명시 free 실패'); };
    const runtime = scene.runtime();
    const other = runtime.removeBrainForState(2);
    let failed = false;
    try { runtime.removeBrainForState(1); } catch (error) { failed = error.message === '명시 free 실패'; }
    assert.ok(failed);
    assert.equal(scene.trace, 'K,7002;F,7002');
    assert.equal(runtime.brain(1), 7002);
    assert.equal(runtime.brainReturnedForState(1), false);
    // 예외 복귀 뒤 호출 대여만 종료되며 다른 정상 증빙은 소비할 수 있다.
    runtime.returnBrainForState(other);
    assert.equal(runtime.brainReturnedForState(2), true);
});

// 다른 모듈/이미 소비한 증빙/소멸한 모듈에서 발급한 증빙/위조 증빙은 재사용할 수 없다.
test('RawPlayerParticipation: 타 모듈·재소비·위조 증빙을 거부한다', () => {
    const scene = new ParticipationScene('1,1,0,0,0,0,0,1,0,0,0,0,7002'.split(','));
    const first = scene.runtime();
    const second = scene.runtime();
    const pass = first.removeBrainForState(1);
    assert.throws(() => second.returnBrainForState(pass), TypeError);
    first.returnBrainForState(pass);
    assert.equal(first.brainReturnedForState(1), true);
    assert.equal(second.brainReturnedForState(1), false);
    assert.throws(() => first.returnBrainForState(pass), /이미 소비한/);
    // 소멸한 모듈의 증빙은 같은 저장소를 빌리는 새 모듈에서도 쓸 수 없다.
    let old = scene.runtime();
    const oldPass = old.removeBrainForState(3);
    old = null;
    const reused = scene.runtime();
    assert.throws(() => reused.returnBrainForState(oldPass), TypeError);
    assert.equal(reused.brainReturnedForState(3), false);
    // 증빙은 발급 모듈 밖에서 만들 수 없다(비공개 수명 토큰 불일치).
    assert.throws(() => reused.returnBrainForState(new BrainResetPass(reused, {}, 4)), TypeError);
    assert.throws(() => reused.returnBrainForState({ owner: 4 }), TypeError);
});

// 파괴 callback 이 아직 실행 중이면 이전 owner 의 정상 증빙도 소비할 수 없다.
test('RawPlayerParticipation: 파괴 효과가 복귀하기 전에는 증빙을 소비할 수 없다', () => {
    const scene = new ParticipationScene('1,1,0,0,7002,0,0,1,0,0,0,0,7002'.split(','));
    let current = null;
    let previous = null;
    let rejected = false;
    scene.hooks.destructBrain = (token) => {
        assert.equal(token, 7002);
        assert.equal(current.brain(1), 7002);
        try { current.returnBrainForState(previous); } catch (error) { rejected = error.message.includes('진행 중'); }
    };
    const runtime = scene.runtime();
    current = runtime;
    previous = runtime.removeBrainForState(2);
    const pass = runtime.removeBrainForState(1);
    assert.ok(rejected);
    assert.equal(runtime.brainReturnedForState(1), false);
    assert.equal(runtime.brainReturnedForState(2), false);
    runtime.returnBrainForState(pass);
    runtime.returnBrainForState(previous);
    assert.equal(runtime.brainReturnedForState(1), true);
    assert.equal(runtime.brainReturnedForState(2), true);
});

// 닫힌 owner 에 대한 raw 외부 변조는 허용된 새 설치가 아닌 명시 수명 위반이다(RawAiCreate 거부는 해당 단위 이식 때 대조).
test('RawPlayerParticipation: 닫힌 owner 의 raw brain 변조는 수명 위반으로 거부한다', () => {
    const scene = new ParticipationScene('1,1,0,0,0,0,0,1,0,0,0,0,7002'.split(','));
    const runtime = scene.runtime();
    runtime.returnBrainForState(runtime.removeBrainForState(1));
    put(scene.players, 180 + 0xb0, 0x12345678);
    assert.throws(() => runtime.brainReturnedForState(1), /정체성 변경/);
    put(scene.players, 180 + 0xb0, 0);
    assert.equal(runtime.brainReturnedForState(1), true);
});

// 전체 반납은 이미 State 가 정상 반납한 NULL 을 허용하되 각 현재 owner 조회를 생략하지 않는다.
test('RawPlayerParticipation: removeAllBrains 는 닫힌 NULL 을 허용하고 뒤 owner 를 방문한다', () => {
    const scene = new ParticipationScene('1,1,0,0,0,0,0,1,0,0,0,0,7002'.split(','));
    const runtime = scene.runtime();
    put(scene.players, 0xb0, 9000);
    runtime.returnBrainForState(runtime.removeBrainForState(1));
    put(scene.players, 2 * 180 + 0xb0, 7002);
    put(scene.players, 2 * 180 + 0x50, 0x12340020);
    runtime.removeAllBrains();
    assert.equal(runtime.brainReturnedForState(1), true);
    assert.equal(runtime.brainReturnedForState(2), false);
    assert.equal(runtime.brain(2), 0);
    assert.equal(runtime.brain(0), 9000);
    assert.equal(scene.trace, 'K,7002;F,7002');
    assert.ok(scene.players[2 * 180 + 0x50] & 0x20);
    // 같은 NULL 재방문은 파괴·free 효과를 추가하지 않는다.
    runtime.removeAllBrains();
    assert.equal(scene.trace, 'K,7002;F,7002');
    // 닫힌 owner1 의 정체성 위반은 뒤 owner2 보다 먼저 거부한다.
    put(scene.players, 180 + 0xb0, 0x12345678);
    put(scene.players, 2 * 180 + 0xb0, 7003);
    assert.throws(() => runtime.removeAllBrains(), /정체성 변경/);
    assert.equal(runtime.brain(2), 7003);
    assert.equal(scene.trace, 'K,7002;F,7002');
});

// 저장소 크기와 현재 입력 조회 누락·외부 효과 장치 누락은 첫 변경 전에 호스트 오류로 거부한다.
test('RawPlayerParticipation: 저장소·현재 입력·효과 장치 누락을 변경 전에 거부한다', () => {
    const scene = new ParticipationScene('1,1,0,0,0,0,0,1,0,0,0,0,7002'.split(','));
    assert.throws(() => new RawPlayerParticipation(new Uint8Array(10), scene.allies, scene.hidden, scene.state, scene.hooks), RangeError);
    assert.throws(() => new RawPlayerParticipation(scene.players, new Uint32Array(80), scene.hidden, scene.state, scene.hooks), RangeError);
    const runtime = scene.runtime();
    assert.throws(() => runtime.bindCurrentInputs({ localOwner: () => 1 }), TypeError);
    // 실제 현재 조회가 값형 상태보다 우선한다.
    runtime.bindCurrentInputs({ localOwner: () => 1, playerKnown: () => 0, battle: () => true, metamap: () => false, localTransport: () => false });
    runtime.addAlly(1, 2);
    // 초기 mask 0 에 대상 bit 2 만 켠 값(4)을 서버(9)에 요청하고 해당 방향 한 cell 만 켠다.
    assert.equal(scene.trace, 'S,9,1,4');
    assert.equal(scene.allies[1 * 9 + 2], 1);
    // 효과 장치가 없으면 해당 위치에서 오류이며 이미 쓴 필드를 되돌리지 않는다.
    const broken = new ParticipationScene('1,1,0,0,0,0,0,1,0,0,0,0,7002'.split(','));
    delete broken.hooks.rejectLogin;
    assert.throws(() => broken.runtime().checkLogin(1, 0), /rejectLogin/);
});
