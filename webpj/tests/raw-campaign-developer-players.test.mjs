// 개발자 Player·동맹 진단의 현재 기록과 추적 순서를 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { executeDeveloperPlayers } from '../www/core/raw-campaign-developer-players.js';

// 이름과 플래그를 같은 9개 Player 바이트 배열에 쓴다.
function record(players, owner, name, flags) {
    players.set(Uint8Array.from(`${name}\0`, (letter) => letter.charCodeAt(0)), owner * 180 + 12);
    new DataView(players.buffer).setUint32(owner * 180 + 0x50, flags, true);
}

// 출력 콜백이 변경한 다음 Player 상태는 다음 반복에서 다시 읽는다.
test('개발자 Player 진단: 현재 이름과 상태', () => {
    const players = new Uint8Array(9 * 180), calls = []; record(players, 1, 'One', 0x23); record(players, 2, 'Two', 0);
    const handled = executeDeveloperPlayers('DP', players, (level, text) => { calls.push([level, text]); if (text.startsWith('id:1')) record(players, 2, 'Two', 0x22); });
    assert.equal(handled, true); assert.deepEqual(calls, [[29, 'id:1 One Logged in\n'], [29, 'id:2 Two NotLogged in\n']]);
    assert.equal(executeDeveloperPlayers('other', players), false);
});

// 동맹 머리말 뒤 바뀐 현재 비트와 이름을 대상마다 다시 읽는다.
test('개발자 동맹 진단: 현재 비트와 입력 경계', () => {
    const players = new Uint8Array(9 * 180), view = new DataView(players.buffer), calls = [];
    record(players, 1, 'One', 0); record(players, 2, 'Two', 0); record(players, 3, 'Three', 0);
    executeDeveloperPlayers('dally 1', players, (level, text) => {
        calls.push([level, text]);
        if (text.startsWith('Ally')) view.setUint32(180 + 0x54, 1 << 2, true);
        if (text.startsWith(' 2')) view.setUint32(180 + 0x54, 1 << 3, true);
    });
    assert.deepEqual(calls, [[29, 'Ally bits for 1 One\n'], [29, ' 2 Two\n'], [29, ' 3 Three\n']]);
    assert.equal(executeDeveloperPlayers('dumpally', players), true);
    assert.throws(() => executeDeveloperPlayers('dumpally -1', players, () => {}), /물리 범위/);
    players.fill(65, 180 + 12, 180 + 44);
    assert.throws(() => executeDeveloperPlayers('dally 1', players, () => {}), /NUL 부족/);
});

// 원본 패치에서는 해제 별칭도 추가 함수를 사용하며 불완전한 인자는 호출하지 않는다.
test('개발자 동맹 변경: 두 별칭과 정수 변환', () => {
    const players = new Uint8Array(9 * 180), calls = [], add = (...args) => calls.push(args);
    assert.equal(executeDeveloperPlayers('sally 1 2', players, null, add), true);
    assert.equal(executeDeveloperPlayers('cally -1 8', players, null, add), true);
    assert.equal(executeDeveloperPlayers('setally 1x 2', players, null, add), true);
    assert.deepEqual(calls, [[1, 2], [0xffffffff, 8]]);
    assert.throws(() => executeDeveloperPlayers('setally 1 2', players), /참여 장치 미연결/);
});
