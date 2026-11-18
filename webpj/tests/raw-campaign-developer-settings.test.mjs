// 31h 설정 분기의 원본 별칭·입력·공유 전역을 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAuxState } from '../www/core/game-aux-state.js';
import { executeDeveloperSettings } from '../www/core/raw-campaign-developer-settings.js';

// 현재 장기 설정을 같은 객체에서 갱신하고 선택된 로그만 기록한다.
test('개발자 설정: 별칭·기본값·공유 전역', () => {
    const aux = new GameAuxState(), calls = [], run = (text) => executeDeveloperSettings(text, aux, (...args) => calls.push(args));
    assert.equal(run('  SHM -2 trailing'), true); assert.equal(aux.missionSettings()[1], 1);
    assert.equal(run('snd 0'), true); assert.equal(aux.missionSettings()[2], 0);
    assert.equal(run('smf'), true); assert.equal(aux.missionSettings()[3], 1);
    assert.equal(run('fmt 123abc'), true); assert.equal(aux.missionSettings()[4], 123);
    assert.equal(run('gt 4'), true); assert.equal(aux.missionSettings()[0], 1);
    assert.equal(run('qv 1'), true); assert.equal(run('qb'), true); assert.equal(aux.quickVortex, 1); assert.equal(aux.quickBuild, 1);
    assert.equal(run('rtt nonsense'), true); assert.equal(run('unimplemented 1'), false);
    assert.equal(calls.length, 3); assert.deepEqual(calls[0], [29, 'hackmover now 1\n']);
});

// 원본 미정 필수 입력과 범위 밖 정수를 합성하지 않으며 오류 출력은 현재 상태를 보존한다.
test('개발자 설정: 누락·범위·음수 입력', () => {
    const aux = new GameAuxState(), calls = [], run = (text) => executeDeveloperSettings(text, aux, (...args) => calls.push(args));
    assert.throws(() => run('  '), /첫 토큰/); assert.throws(() => run('shm xx'), /미정 정수/);
    assert.throws(() => run('qv 2147483648'), /DWORD 범위/);
    assert.equal(run('fmt -1'), true); assert.equal(aux.missionSettings()[4], 0);
    assert.deepEqual(calls, [[29, 'bad command\n']]);
});

// 추적 설정은 원본 바이트를 넘기고 패널 크기는 제한 뒤 현재 명령을 큐에 넣는다.
test('개발자 설정: tracelevel과 전투 패널 크기', () => {
    const aux = new GameAuxState(), calls = [], services = {
        install: { setConfig: (name, value) => calls.push(['config', name, [...value]]) },
        developerQueue: (...args) => calls.push(['queue', ...args]),
    };
    assert.equal(executeDeveloperSettings('stl \xff', aux, null, services), true);
    assert.deepEqual(calls.shift(), ['config', 'tracelevel', [255, 0]]);
    aux.developerCombatResizeCommand = 0x02000066;
    executeDeveloperSettings('cr 5 4', aux, null, services);
    assert.deepEqual([...aux.missionSettings()].slice(5), [3, 4]); assert.equal(aux.missionSettings()[0], 0);
    assert.deepEqual(calls.shift(), ['queue', 0x02000066, 3, 4]);
    executeDeveloperSettings('cr 1', aux, null, services);
    assert.deepEqual([...aux.missionSettings()].slice(5), [3, 10]);
    assert.throws(() => executeDeveloperSettings('cr 2 2', aux, null), /입력 큐 미연결/);
    assert.deepEqual([...aux.missionSettings()].slice(5), [2, 2]);
    assert.throws(() => executeDeveloperSettings('stl', aux, null, services), /미정 문자열/);
});
