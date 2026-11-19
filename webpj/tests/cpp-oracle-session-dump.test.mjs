// cpppj 실행 없이 고정 시각·Session 덤프 수집 명령과 임시 파일 수명을 검증한다. (2026-11-19 WB04-E868)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { runCppScenario } from '../tools/cpp-oracle.mjs';
import { collectionInputs } from '../tools/headless.mjs';

// 가짜 실행기에 기록되는 옵션과 단계표를 확인하고 캡처·보고서 파일을 직접 만든다.
test('cpppj 오라클: 고정 시각과 Session 덤프 파일을 전달하고 수집한다', () => {
    let work;
    const execute = (exe, args, options) => {
        assert.equal(exe, 'fake.exe');
        assert.equal(args[args.indexOf('--fixed-frame-ms') + 1], '25');
        assert.equal(args[args.indexOf('--fixed-unix-seconds') + 1], '946684800');
        assert.equal(args[args.indexOf('--mission') + 1], 'Tutorial1');
        assert.equal(options.env.NETSTORM_CPP_NO_AUDIO, undefined);
        work = path.dirname(options.env.NETSTORM_USER_DATA);
        const steps = fs.readFileSync(args[args.indexOf('--ui-script') + 1], 'utf8').trim().split('\n');
        const first = steps[0].split('\t'), second = steps[1].split('\t');
        assert.deepEqual(first.slice(0, 2), ['10', 'snapshot']);
        assert.deepEqual(second.slice(0, 2), ['20', 'session-dump']);
        assert.notEqual(second[2], args[args.indexOf('--ui-report') + 1]);
        assert.equal(path.dirname(first[2]), work); assert.equal(path.dirname(second[2]), work);
        fs.writeFileSync(first[2], Buffer.from([1, 2, 3]));
        fs.writeFileSync(second[2], 'netstorm_session_dump\t1\nend\n');
        fs.writeFileSync(args[args.indexOf('--ui-report') + 1], 'report\n');
        return { status: 0, stdout: 'ok', stderr: '' };
    };
    const result = runCppScenario({ exe: 'fake.exe', gameDir: process.cwd(), shared: { settings: {}, fixedFrameMs: 25 }, execute,
        scenario: { id: 'test', mission: 'Tutorial1', fixedUnixSeconds: 946684800, frames: 20, steps: [[10, 'snapshot', 'main'], [20, 'session-dump', 'report']] } });
    assert.deepEqual([...result.snapshots.main.bmp], [1, 2, 3]);
    assert.equal(result.dumps.report['tsv'].toString('utf8'), 'netstorm_session_dump\t1\nend\n');
    assert.equal(result.report, 'report\n'); assert.equal(fs.existsSync(work), false);
});

// 잘못된 이름·시간과 누락 덤프는 성공처럼 넘기지 않으며 실패 뒤 임시 폴더도 정리한다.
test('cpppj 오라클: 고정 시각·경로·결과 오류를 거부하고 정리한다', () => {
    const common = { exe: 'fake.exe', gameDir: process.cwd(), shared: {}, scenario: { id: 'test', steps: [[1, 'session-dump', 'dump']] } };
    assert.throws(() => runCppScenario({ ...common, scenario: { ...common.scenario, fixedFrameMs: 0 } }), /밀리초/);
    assert.throws(() => runCppScenario({ ...common, scenario: { ...common.scenario, fixedUnixSeconds: 1 } }), /Unix/);
    assert.throws(() => runCppScenario({ ...common, scenario: { ...common.scenario, mission: '' } }), /미션 이름/);
    assert.throws(() => runCppScenario({ ...common, scenario: { ...common.scenario, id: '../escape' } }), /이름/);
    assert.throws(() => runCppScenario({ ...common, scenario: { ...common.scenario, steps: [[1, 'session-dump', '../escape']] } }), /이름/);
    assert.throws(() => runCppScenario({ ...common, scenario: { ...common.scenario, steps: [[1, 'session-dump', 'a'], [2, 'session-dump', 'a']] } }), /중복/);
    let work;
    assert.throws(() => runCppScenario({ ...common, execute: (exe, args, options) => { work = path.dirname(options.env.NETSTORM_USER_DATA); return { status: 0, stdout: '', stderr: '' }; } }), /덤프 없음/);
    assert.equal(fs.existsSync(work), false);
});

// 수집 입력 해시는 고정 시각의 변경을 포함하고 값이 없는 이전 형식의 직렬화는 바꾸지 않는다.
test('cpppj 오라클: 고정 시각은 시나리오 수집 입력에 포함된다', () => {
    const base = { scenarios: [{ id: 'a', steps: [[1, 'session-dump', 'state']] }] };
    assert.equal(collectionInputs(base), JSON.stringify({ language: 'english', screen: null, settings: {}, scenarios: [{ id: 'a', settings: {}, frames: 0, steps: base.scenarios[0].steps }] }));
    assert.notEqual(collectionInputs(base), collectionInputs({ ...base, fixedFrameMs: 25 }));
    assert.notEqual(collectionInputs(base), collectionInputs({ ...base, scenarios: [{ ...base.scenarios[0], fixedFrameMs: 25 }] }));
    assert.notEqual(collectionInputs(base), collectionInputs({ ...base, fixedUnixSeconds: 123 }));
    assert.notEqual(collectionInputs(base), collectionInputs({ ...base, scenarios: [{ ...base.scenarios[0], mission: 'Tutorial1' }] }));
});
