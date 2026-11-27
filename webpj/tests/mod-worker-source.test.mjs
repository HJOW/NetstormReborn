// 운영 봉투와 Native debug의 텍스트 읽기를 검증한다. 번들을 평가하는 경로는 없다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { decodeModWorkerPayload, loadModWorkerSource, MOD_WORKER_PAYLOAD_KEY, MOD_WORKER_PAYLOAD_PREFIX, MOD_WORKER_PAYLOAD_SUFFIX } from '../www/modding/mod-worker-source.js';
// UTF-8 원문을 플러그인과 같은 봉투로 만든다.
const envelope = (source) => MOD_WORKER_PAYLOAD_PREFIX + Buffer.from(source).toString('base64') + MOD_WORKER_PAYLOAD_SUFFIX;

test('전역 봉투의 UTF-8 원문만 해독하고 네트워크를 쓰지 않는다', async () => {
    const source = '/* 한국어 */ throw new Error("평가하면 실패");';
    assert.equal(await loadModWorkerSource({ view: { [MOD_WORKER_PAYLOAD_KEY]: envelope(source) }, fetch: () => assert.fail('불필요한 fetch') }), source);
});
test('손상된 base64/UTF-8/중복 봉투를 거절한다', async () => {
    // 각각 구분자·패딩·UTF-8 손상을 대표한다.
    for (const value of ['', 'NETSTORM_MOD_WORKER_V0:??==:END_NETSTORM_MOD_WORKER', MOD_WORKER_PAYLOAD_PREFIX + '/w==' + MOD_WORKER_PAYLOAD_SUFFIX]) assert.throws(() => decodeModWorkerPayload(value), { code: 'scriptError' });
    await assert.rejects(loadModWorkerSource({ view: {}, fetch: async () => ({ ok: true, text: async () => envelope('a') + envelope('b') }) }), { code: 'scriptError' });
});
test('Native debug는 같은 생산 번들에서 봉투 하나만 텍스트로 읽는다', async () => {
    let request;
    assert.equal(await loadModWorkerSource({ view: {}, fetch: async (url, options) => { request = { url: String(url), options }; return { ok: true, text: async () => 'throw new Error("실행금지");' + envelope('worker 원문') }; } }), 'worker 원문');
    assert.ok(request.url.endsWith('/www/dist/netstorm.bundle.js')); assert.equal(request.options.cache, 'no-store');
});
test('누락/HTTP 오류는 실행기 시작 실패로 보고한다', async () => {
    await assert.rejects(loadModWorkerSource({ view: {} }), { code: 'scriptError' });
    await assert.rejects(loadModWorkerSource({ view: {}, fetch: async () => ({ ok: false, status: 404 }) }), /404/u);
});
