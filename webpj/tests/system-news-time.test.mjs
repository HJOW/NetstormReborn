// 서버별 뉴스 키/시각의 현재 메모리와 Config 표현식 경계를 소스 계약으로 대조한다. 전용 원본PE fixture는 없다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { buildSystemNewsTimeKey, readSystemNewsTime, writeSystemNewsTime } from '../www/core/system-news-time.js';

test('뉴스 키: 첫 NUL·점 치환·NUL 뒤 공유128BYTE 꼬리를 보존한다', () => {
    const key = new Uint8Array(128).fill(0xa5); assert.equal(buildSystemNewsTimeKey({ key }, { serverName: () => 'a.b\0ignored' }), 'a_bNewsTime');
    assert.equal(key[11], 0); assert.ok(key.subarray(12).every((byte) => byte === 0xa5));
});
test('뉴스 키: 128BYTE/로컬256BYTE 한계는 쓰기 전에 거부한다', () => {
    // 두 실제 물리 중 짧은 쪽과 정확한 NUL 자리를 각각 확인한다.
    for (const size of [128, 400]) {
        const key = new Uint8Array(size).fill(0xa5); const limit = Math.min(size, 256);
        assert.throws(() => buildSystemNewsTimeKey({ key }, { serverName: () => 'a'.repeat(limit - 8) }), RangeError); assert.ok(key.every((byte) => byte === 0xa5));
        assert.equal(buildSystemNewsTimeKey({ key }, { serverName: () => 'a'.repeat(limit - 9) }).length, limit - 1);
    }
});
test('뉴스 읽기: signed 반환 비트를 DWORD에 쓰고 누락 값0을 기존 시각으로 대체하지 않는다', () => {
    const globals = { key: new Uint8Array(128), value: 123 }; const hooks = { serverName: () => 'host', read: (key) => { assert.equal(key, 'hostNewsTime'); return -1; } };
    readSystemNewsTime(globals, hooks); assert.equal(globals.value, 0xffffffff); hooks.read = () => 0; readSystemNewsTime(globals, hooks); assert.equal(globals.value, 0);
});
test('뉴스 쓰기: 서버 getter보다 먼저 시각을 캡처하고 signed 십진수로 쓴다', () => {
    const globals = { key: new Uint8Array(128), value: 0x80000000 }; const output = [];
    writeSystemNewsTime(globals, { serverName: () => { globals.value = 7; return 'a.b'; }, configure: (value) => output.push(value) });
    assert.deepEqual(output, ['a_bNewsTime="-2147483648"']); assert.equal(globals.value, 7);
});
test('뉴스 키를 별도 이스케이프하지 않으며 빈 서버 이름도 원문 접미사를 만든다', () => {
    const globals = { key: new Uint8Array(128), value: 9 }; let result;
    writeSystemNewsTime(globals, { serverName: () => 'a"=b', configure: (value) => { result = value; } }); assert.equal(result, 'a"=bNewsTime="9"');
    assert.equal(buildSystemNewsTimeKey(globals, { serverName: () => '' }), 'NewsTime');
});
test('뉴스 설정문256BYTE 한계에서 앞선 키 쓰기는 남고 configure는 호출하지 않는다', () => {
    const globals = { key: new Uint8Array(400), value: 0x80000000 }; let calls = 0;
    assert.throws(() => writeSystemNewsTime(globals, { serverName: () => 'a'.repeat(234), configure: () => { calls += 1; } }), RangeError);
    assert.equal(globals.key[242], 0); assert.equal(calls, 0);
});
test('뉴스 하위 미연결/예외는 앞서 작성한 키를 유지하고 값 대입은 성공 뒤에만 한다', () => {
    const globals = { key: new Uint8Array(128), value: 77 };
    assert.throws(() => readSystemNewsTime(globals, { serverName: () => 'x' }), /미연결/); assert.equal(globals.value, 77); assert.equal(globals.key[0], 120);
    assert.throws(() => writeSystemNewsTime(globals, { serverName: () => 'y', configure: () => { throw new Error('중단'); } }), /중단/); assert.equal(globals.key[0], 121);
    assert.throws(() => buildSystemNewsTimeKey(globals, {}), /미연결/);
});
