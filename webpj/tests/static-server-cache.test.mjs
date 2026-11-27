// 실제 HTTP 서버에서 캐시 재검증·배포 파일 교체·하위 경로 경계를 확인한다. 게임 원본이나 공개 자산은 수정하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { request as httpRequest } from 'node:http';
import { once } from 'node:events';
import { mkdtemp, writeFile, stat, utimes, rm } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import path from 'node:path';
import { setTimeout as delay } from 'node:timers/promises';
import serverModule from '../server.js';

// 게임과 같은 서버를 별도 임시 배포 경로에서 시작하고 해당 검사 소유 폴더만 정리한다.
async function fixture(t) {
    const root = await mkdtemp(path.join(tmpdir(), 'netstorm-http-cache-'));
    const file = path.join(root, 'index.html');
    await writeFile(file, 'AAAA', 'utf8');
    const server = serverModule.createStaticServer('/preview/netstorm/', { rootDirectory: root });
    server.listen(0, '127.0.0.1'); await once(server, 'listening');
    t.after(async () => {
        server.closeAllConnections(); await new Promise((resolve, reject) => server.close((error) => error ? reject(error) : resolve()));
        assert.equal(path.dirname(root), path.resolve(tmpdir()));
        assert.ok(path.basename(root).startsWith('netstorm-http-cache-'));
        await rm(root, { recursive: true, force: true });
    });
    return { root, file, port: server.address().port };
}

// 브라우저 캐시를 흉내 내지 않고 원시 HTTP 상태·응답 헤더·실제 몸체를 읽는다.
function get(port, headers = {}, method = 'GET', target = '/preview/netstorm/') {
    return new Promise((resolve, reject) => {
        const req = httpRequest({ hostname: '127.0.0.1', port, path: target, method, headers }, (response) => {
            const chunks = [];
            response.on('data', (chunk) => chunks.push(chunk));
            response.on('end', () => resolve({ status: response.statusCode, headers: response.headers, body: Buffer.concat(chunks).toString('utf8') }));
            response.on('error', reject);
        });
        req.on('error', reject); req.end();
    });
}

// GET/HEAD가 같은 검증자를 내고 일치하는 약/강 태그·목록·별표 요청에 몸체 없이304를 반환한다.
test('정적 캐시: GET/HEAD와 ETag 목록 재검증', async (t) => {
    const { port } = await fixture(t), first = await get(port), head = await get(port, {}, 'HEAD');
    assert.equal(first.status, 200); assert.equal(first.body, 'AAAA');
    assert.equal(first.headers['cache-control'], 'no-cache'); assert.match(first.headers.etag, /^W\/".+"$/u);
    assert.equal(first.headers['content-type'], 'text/html; charset=utf-8'); assert.equal(first.headers['content-length'], '4');
    assert.equal(head.status, 200); assert.equal(head.body, '');
    assert.equal(head.headers.etag, first.headers.etag); assert.equal(head.headers['last-modified'], first.headers['last-modified']);
    // HTTP의 약한 비교가 적용되는 네 가지 유효 조건이다.
    for (const value of [first.headers.etag, first.headers.etag.slice(2), `"other,tag", ${first.headers.etag}`, '*']) {
        const result = await get(port, { 'If-None-Match': value });
        assert.equal(result.status, 304); assert.equal(result.body, ''); assert.equal(result.headers.etag, first.headers.etag);
        assert.equal(result.headers['cache-control'], 'no-cache'); assert.equal(result.headers['content-length'], undefined);
    }
    assert.equal((await get(port, { 'If-None-Match': '"old"' })).status, 200);
});

// 날짜 검증은 초 정밀도를 따르고 If-None-Match가 존재하면 날짜 결과로 뒤집지 않는다.
test('정적 캐시: 수정 날짜와 ETag 우선순위', async (t) => {
    const { port } = await fixture(t), first = await get(port), future = new Date(Date.now() + 60000).toUTCString();
    assert.equal((await get(port, { 'If-Modified-Since': first.headers['last-modified'] })).status, 304);
    assert.equal((await get(port, { 'If-Modified-Since': 'Thu, 01 Jan 1970 00:00:00 GMT' })).status, 200);
    assert.equal((await get(port, { 'If-Modified-Since': 'invalid' })).status, 200);
    assert.equal((await get(port, { 'If-None-Match': '"old"', 'If-Modified-Since': future })).status, 200);
    assert.equal((await get(port, { 'If-None-Match': first.headers.etag, 'If-Modified-Since': 'Thu, 01 Jan 1970 00:00:00 GMT' })).status, 304);
});

// 같은 파일명·크기·수정 시각으로 교체해도 상태변경 시각이 달라져 오래된 태그에 새 몸체를 보낸다.
test('정적 캐시: mtime을 보존한 같은 크기 배포 교체', async (t) => {
    const { port, file } = await fixture(t), previous = await stat(file), first = await get(port);
    await delay(25); await writeFile(file, 'BBBB', 'utf8'); await utimes(file, previous.atime, previous.mtime);
    const changed = await get(port, { 'If-None-Match': first.headers.etag, 'If-Modified-Since': first.headers['last-modified'] });
    assert.equal(changed.status, 200); assert.equal(changed.body, 'BBBB'); assert.notEqual(changed.headers.etag, first.headers.etag);
    assert.equal((await get(port, { 'If-None-Match': changed.headers.etag })).status, 304);
});

// 파일 존재/메서드/공개 경로 검사 실패를 별표 캐시 조건이304로 숨기지 않는다.
test('정적 캐시: 오류와 하위 경로 경계 유지', async (t) => {
    const { port } = await fixture(t), headers = { 'If-None-Match': '*' };
    assert.equal((await get(port, headers, 'GET', '/preview/netstorm/missing.js')).status, 404);
    assert.equal((await get(port, headers, 'POST')).status, 405);
    assert.equal((await get(port, headers, 'GET', '/index.html')).status, 404);
    assert.equal((await get(port, headers, 'GET', '/preview/netstorm/%2e%2e/private')).status, 403);
    assert.equal((await get(port, headers, 'GET', '/preview/netstorm/%')).status, 400);
});
