// AssetLoader 의 일시적 실패 재시도·진행률 되돌림·동시 내려받기 한도를 검사한다. 실제 네트워크 없이 가짜 fetch 와 가짜 대기 함수를 쓴다.
// 처음 방문(캐시 없음)의 요청 100여 개 중 하나가 잠깐 실패해도 오류 화면이 되거나 한국어가 영어로 대체되지 않게 하려는 단위의 시험이다. (2026-11-24 WB04-E1300)
import test from 'node:test';
import assert from 'node:assert/strict';
import { AssetLoader, DEFAULT_CONCURRENCY, DEFAULT_RETRY_DELAYS } from '../www/assets/asset-loader.js';

// 가짜 응답이다. status 가 2xx 이면 bytes 를 스트림(조각 목록)으로 주고 streamFailure 가 있으면 첫 조각 뒤에 그 오류를 던진다.
function respond(status, bytes = new Uint8Array(0), { chunks = 1, streamFailure = null, json = null } = {}) {
    const response = { ok: status >= 200 && status < 300, status, json: async () => json };
    if (!response.ok) return response;
    const size = Math.ceil(bytes.length / chunks) || 1;
    let offset = 0, delivered = 0;
    response.body = { getReader: () => ({ read: async () => {
        if (streamFailure && delivered >= 1) throw streamFailure;
        if (offset >= bytes.length) return { done: true, value: undefined };
        const value = bytes.subarray(offset, offset + size); offset += value.length; delivered += 1;
        return { done: false, value };
    } }) };
    return response;
}

// 대기 시간을 기록만 하고 즉시 돌아오는 가짜 대기 함수와 이를 쓰는 적재기를 만든다.
function makeLoader(fetchFunction, options = {}) {
    const slept = [];
    const loader = new AssetLoader({ baseUrl: 'http://localhost/data/', fetchFunction, sleep: async (milliseconds) => { slept.push(milliseconds); }, ...options });
    return { loader, slept };
}

const payload = Uint8Array.from({ length: 100 }, (_, index) => index);
const entry = { path: 'd/a.bin', size: 100, bundle: 'menu' };

test('기본 재시도 간격은 세 번이며 동시 내려받기 한도는 12 이다', () => {
    assert.deepEqual([...DEFAULT_RETRY_DELAYS], [400, 1200, 2800]);
    assert.equal(DEFAULT_CONCURRENCY, 12);
    const { loader } = makeLoader(async () => respond(200, payload));
    assert.deepEqual([loader.retryDelays, loader.concurrency], [[400, 1200, 2800], 12]);
    assert.throws(() => new AssetLoader({ baseUrl: 'http://x/', fetchFunction: async () => {}, retryDelays: [-1] }), TypeError);
    assert.throws(() => new AssetLoader({ baseUrl: 'http://x/', fetchFunction: async () => {}, concurrency: 0 }), RangeError);
});

test('서버 오류(503)·요청 과다(429)·네트워크 오류는 간격을 두고 다시 받아 성공한다', async () => {
    const results = [() => respond(503), () => respond(429), () => { throw new TypeError('Failed to fetch'); }, () => respond(200, payload)];
    let calls = 0;
    const { loader, slept } = makeLoader(async () => results[calls++]());
    const progress = [];
    const bytes = await loader.loadFile(entry, (count) => progress.push(count));
    assert.deepEqual([...bytes], [...payload]);
    assert.equal(calls, 4);
    assert.deepEqual(slept, [400, 1200, 2800], '재시도 간격');
    assert.equal(progress.reduce((sum, count) => sum + count, 0), 100, '진행량 합은 파일 크기');
});

test('없는 파일(404)·권한(403)은 다시 시도하지 않는다', async () => {
    for (const status of [404, 403]) {
        let calls = 0;
        const { loader, slept } = makeLoader(async () => { calls += 1; return respond(status); });
        await assert.rejects(() => loader.loadFile(entry), new RegExp(`\\(${status}\\)`));
        assert.deepEqual([calls, slept], [1, []], `${status} 는 한 번만 요청`);
    }
});

test('재시도 횟수를 넘기면 마지막 오류를 그대로 던진다', async () => {
    let calls = 0;
    const { loader, slept } = makeLoader(async () => { calls += 1; return respond(500); });
    await assert.rejects(() => loader.loadFile(entry), /\(500\)/);
    assert.equal(calls, 4, '처음 한 번과 재시도 세 번');
    assert.deepEqual(slept, [400, 1200, 2800]);
    const none = makeLoader(async () => { calls += 1; return respond(500); }, { retryDelays: [] });
    calls = 0;
    await assert.rejects(() => none.loader.loadFile(entry), /\(500\)/);
    assert.equal(calls, 1, '빈 목록이면 재시도 없음');
});

test('잘린 응답(크기 불일치)과 스트림 도중 끊김도 다시 받고 실패한 시도의 진행량은 되돌린다', async () => {
    const attempts = [
        () => respond(200, payload.subarray(0, 60)),
        () => respond(200, payload, { chunks: 4, streamFailure: new TypeError('network error') }),
        () => respond(200, payload, { chunks: 4 }),
    ];
    let calls = 0;
    const { loader } = makeLoader(async () => attempts[calls++]());
    let net = 0, peak = 0;
    const bytes = await loader.loadFile(entry, (count) => { net += count; peak = Math.max(peak, net); });
    assert.deepEqual([...bytes], [...payload]);
    assert.equal(calls, 3);
    assert.equal(net, 100, '실패한 시도의 바이트를 되돌려 최종 진행량은 파일 크기');
    assert.ok(peak <= 100 + 100, '진행량이 크게 부풀지 않는다');
});

test('매니페스트 읽기도 일시적 실패를 다시 시도하고 형식 오류는 재시도 뒤에도 같으면 던진다', async () => {
    const good = { format: 1, files: [entry] };
    let calls = 0;
    const flaky = makeLoader(async () => (calls++ === 0 ? respond(502) : respond(200, new Uint8Array(0), { json: good })));
    assert.deepEqual(await flaky.loader.loadManifest(), good);
    assert.deepEqual([calls, flaky.slept], [2, [400]]);
    const bad = makeLoader(async () => respond(200, new Uint8Array(0), { json: { format: 99, files: [] } }));
    await assert.rejects(() => bad.loader.loadManifest(), /형식/);
    // 형식 오류는 잘린 응답·오류 안내 페이지(200 으로 온 HTML)와 구별할 수 없어 일반 오류처럼 간격 목록만큼 다시 읽은 뒤 던진다.
    assert.deepEqual(bad.slept, [400, 1200, 2800]);
});

test('묶음은 동시 내려받기를 한도 이하로 유지하고 모든 파일을 받아 진행률이 전체 크기에서 끝난다', async () => {
    const entries = Array.from({ length: 30 }, (_, index) => ({ path: `d/f${index}.bin`, size: 100, bundle: 'menu' }));
    let inFlight = 0, peak = 0;
    const { loader } = makeLoader(async () => {
        inFlight += 1; peak = Math.max(peak, inFlight);
        await new Promise((resolve) => setTimeout(resolve, 2));
        inFlight -= 1;
        return respond(200, payload, { chunks: 2 });
    }, { concurrency: 5 });
    loader.manifest = { format: 1, files: entries };
    const reports = [];
    const files = await loader.loadBundle('menu', (loaded, total) => reports.push([loaded, total]));
    assert.equal(files.size, 30);
    assert.ok(peak <= 5 && peak >= 2, `동시 요청 최대 ${peak}`);
    assert.deepEqual(reports.at(-1), [3000, 3000]);
    assert.ok(reports.every(([loaded, total]) => loaded >= 0 && loaded <= total), '진행률은 항상 0~전체');
});

test('묶음의 한 파일이 영구 실패하면 남은 파일을 더 시작하지 않고 그 오류를 던진다', async () => {
    const entries = Array.from({ length: 40 }, (_, index) => ({ path: `d/f${index}.bin`, size: 100, bundle: 'menu' }));
    const started = [];
    const { loader } = makeLoader(async (url) => {
        started.push(url);
        await new Promise((resolve) => setTimeout(resolve, 1));
        return url.endsWith('f3.bin') ? respond(404) : respond(200, payload);
    }, { concurrency: 4 });
    loader.manifest = { format: 1, files: entries };
    await assert.rejects(() => loader.loadBundle('menu'), /f3\.bin.*\(404\)/);
    assert.ok(started.length < 40, `실패 뒤 새 요청을 멈춘다(시작 ${started.length}개)`);
});
