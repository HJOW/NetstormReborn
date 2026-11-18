// 31h 장치 명령의 원본 관문·별칭과 선택된 장치 호출을 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { executeDeveloperDevices } from '../www/core/raw-campaign-developer-devices.js';

// 입력 실패는 원본 진단만 남기고 장치를 요구하지 않는다.
test('개발자 장치: 힙 추적과 시스템 조회', () => {
    const calls = [], services = { developerHeapTrace: (value) => calls.push(['heap', value]), developerQueryDirectDraw: () => calls.push(['query']) };
    const run = (text, current = services) => executeDeveloperDevices(text, { recording: 0 }, current, (...args) => calls.push(args));
    assert.equal(run('HT -2 extra'), true); assert.equal(run('heaptrace 0'), true); assert.equal(run('qdd ignored'), true);
    assert.equal(run('ht nope', {}), true); assert.equal(run('other'), false);
    assert.deepEqual(calls, [['heap', true], ['heap', false], ['query'], [29, 'bad command\n']]);
    assert.throws(() => run('ht 1', {}), /Client 프레임 미연결/); assert.throws(() => run('qdd', {}), /시스템 버전 장치 미연결/);
});

// 기록 상태와 토큰 유무를 먼저 확인하고 선택된 토큰만 길이 제한해 전달한다.
test('개발자 장치: 기록 라벨 현재 관문', () => {
    const cursor = { recording: 0 }, labels = [], services = { developerRecordLabel: (label) => labels.push(label) };
    assert.equal(executeDeveloperDevices('al label', cursor, {}, null), true);
    cursor.recording = 1;
    assert.equal(executeDeveloperDevices('al   ', cursor, {}, null), true);
    assert.throws(() => executeDeveloperDevices('al x', cursor, {}, null), /기록 파일 미연결/);
    assert.equal(executeDeveloperDevices(`addLabel ${'a'.repeat(300)} extra`, cursor, services, null), true);
    assert.deepEqual(labels, ['a'.repeat(255)]);
});
