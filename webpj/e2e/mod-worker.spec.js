// 실제 브라우저의 격리 Worker에서 공통 적합성·권한·실시간 중단과 반납을 확인한다. 게임 플레이는 하지 않는다.
const { test, expect } = require('@playwright/test');
const fs = require('node:fs'), path = require('node:path'), http = require('node:http');
// DN도 같은 원문/출력을 소비한다. 이 검사는 WB 실행만 증명한다.
const packageText = fs.readFileSync(path.join(__dirname, '../../shared/modding/v0/conformance/core-probe.nsmod.json'), 'utf8');
const trace = JSON.parse(fs.readFileSync(path.join(__dirname, '../../shared/modding/v0/conformance/core-trace.json'), 'utf8'));

test('단일 번들의 격리 실행은 공통 출력을 보존하고 무한루프/외부 권한/자원 반납을 처리한다', async ({ page }) => {
    test.setTimeout(120000);
    // 실제 요청 도착을 세어 CORS 오류를 네트워크 차단으로 잘못 세지 않는다.
    let requests = 0;
    const server = http.createServer((request, response) => { requests++; response.setHeader('Access-Control-Allow-Origin', '*'); response.end('forbidden'); });
    await new Promise((resolve) => server.listen(0, '127.0.0.1', resolve));
    try {
        await page.goto('/index.html?title=1');
        const result = await page.evaluate(async ({ packageText, trace, networkUrl }) => {
            const { ModWorker, validateModPackageIsolated } = await import('./modding/mod-worker.js');
            const { loadModWorkerSource } = await import('./modding/mod-worker-source.js');
            const source = await loadModWorkerSource();
            // index.html의 메인 realm에는 SES lockdown/Compartment가 생성되지 않는다.
            const mainUnchanged = typeof globalThis.Compartment === 'undefined' && !Object.isFrozen(Array.prototype);
            const normalized = await validateModPackageIsolated(packageText), worker = await ModWorker.open();
            const registration = await worker.register(normalized, trace.sessionSeed), outputs = [];
            for (const item of trace.cases) outputs.push(await worker.dispatch(item.event, item.snapshot));
            worker.dispose();
            // 순환 정적 모듈과 import 문구가 들어간 문자열의 값 보존을 실제 SES에서 확인한다.
            const pkg = JSON.parse(packageText); pkg.manifest.id = 'test.authority'; pkg.modules = {
                'main.js': `import {value} from './helper.js'; export const circular = 7; export default function(api) { const keys=['fetch','Worker','postMessage','window','parent','document','navigator','performance','Date','Promise','eval','Function','Compartment','Intl','WeakRef','FinalizationRegistry','Atomics','SharedArrayBuffer','WebAssembly','setTimeout','queueMicrotask']; api.log('info',JSON.stringify(keys.map(key=>typeof globalThis[key]))); let blocked=0; for(const fn of [api.getState,api.on,api.commands.click]) {try{fn.constructor('return globalThis')();}catch(error){blocked++;}} try{Math.random();}catch(error){blocked++;} api.log('info',JSON.stringify({blocked,value:value(),text:"import('https://invalid.example')"})); }`,
                'helper.js': "import {circular} from './main.js'; export function value(){return circular;}"
            };
            const authorityWorker = await ModWorker.open(), authority = await authorityWorker.register(pkg, 1); authorityWorker.dispose();
            // 검증만으로 엔트리 무한루프를 실행하지 않는다. 동적 import는 설치 때 거절한다.
            pkg.manifest.id = 'test.loop'; pkg.modules = { 'main.js': 'export default function(){while(true){}}' };
            const validatedLoop = await validateModPackageIsolated(JSON.stringify(pkg));
            const loop = await ModWorker.open(); let heartbeat = false, loopCode, afterDispose;
            const heartbeatTimer = setTimeout(() => { heartbeat = true; }, 50), started = performance.now();
            try { await loop.register(validatedLoop, 1); } catch (error) { loopCode = error.code; }
            const elapsed = performance.now() - started; clearTimeout(heartbeatTimer);
            try { await loop.register(validatedLoop, 1); } catch (error) { afterDispose = error.code; }
            // 사건 콜백의 반복과 대기 중 명시 반납도 같은 실제 장치로 중단한다.
            pkg.modules = { 'main.js': "export default function(api){api.on('sessionStart',()=>{while(true){}});}" };
            const callbackLoop = await ModWorker.open(); await callbackLoop.register(pkg, 1); let callbackCode;
            try { await callbackLoop.dispatch(trace.cases[0].event, trace.cases[0].snapshot); } catch (error) { callbackCode = error.code; }
            const cancelled = await ModWorker.open(); await cancelled.register(pkg, 1);
            const pending = cancelled.dispatch(trace.cases[0].event, trace.cases[0].snapshot).catch((error) => error.code);
            await new Promise((resolve) => setTimeout(resolve, 50)); cancelled.dispose(); const cancelCode = await pending;
            pkg.modules['main.js'] = "export default function(){import('./helper.js');}"; let forbiddenImport;
            try { await validateModPackageIsolated(JSON.stringify(pkg)); } catch (error) { forbiddenImport = error.code; }
            // 일반 콜백 예외도 해당 Worker만 닫는다. 다른 모드는 이어서 같은 입력을 처리한다.
            pkg.modules = { 'main.js': "export default function(api){api.on('sessionStart',()=>{throw new Error('bad-only');});}" };
            const bad = await ModWorker.open(); await bad.register(pkg, 1); let fault;
            try { await bad.dispatch(trace.cases[0].event, trace.cases[0].snapshot); } catch (error) { fault = error.code; }
            const healthy = await ModWorker.open(), healthyOutput = await healthy.register(packageText, trace.sessionSeed); healthy.dispose();
            // 신뢰 시험 Worker도 iframe의 connect-src none을 상속한다. guest API에는 source 주입점이 없다.
            const probeSource = `postMessage({type:'ready',protocol:1,apiVersion:'0.1.0'}); onmessage=async({data})=>{let blocked=false;try{await fetch(${JSON.stringify(networkUrl)});}catch(error){blocked=true;}postMessage({type:'result',protocol:1,id:data.id,op:data.op,result:{blocked}});};`;
            const probe = await ModWorker.open({ source: probeSource }), csp = await probe.register('{}', 1); probe.dispose();
            // Native debug의 경로도 생산 JS를 평가하지 않고 같은 봉투만 읽는다.
            const debugSource = await loadModWorkerSource({ view: {}, fetch: window.fetch.bind(window) });
            await new Promise((resolve) => setTimeout(resolve, 200));
            return { mainUnchanged, registration, outputs, authority, heartbeat, loopCode, callbackCode, cancelCode, afterDispose, elapsed, forbiddenImport, fault, healthyOutput, csp, sameSource: source === debugSource, remainingFrames: document.querySelectorAll('iframe[sandbox]').length };
        }, { packageText, trace, networkUrl: 'http://127.0.0.1:' + server.address().port + '/blocked' });
        expect(result.mainUnchanged).toBe(true); expect(result.registration).toEqual(trace.registration); expect(result.outputs).toEqual(trace.expected);
        expect(JSON.parse(result.authority.logs[0].text)).toEqual(Array(21).fill('undefined')); expect(JSON.parse(result.authority.logs[1].text)).toEqual({ blocked: 4, value: 7, text: "import('https://invalid.example')" });
        expect(result.heartbeat).toBe(true); expect(result.loopCode).toBe('timeLimit'); expect(result.callbackCode).toBe('timeLimit'); expect(result.cancelCode).toBe('disposed'); expect(result.afterDispose).toBe('disposed'); expect(result.elapsed).toBeGreaterThanOrEqual(950); expect(result.elapsed).toBeLessThan(3000);
        expect(result.forbiddenImport).toBe('permissionDenied'); expect(result.fault).toBe('scriptError'); expect(result.healthyOutput).toEqual(trace.registration); expect(result.csp.blocked).toBe(true); expect(requests).toBe(0); expect(result.sameSource).toBe(true); expect(result.remainingFrames).toBe(0);
        await test.info().attach('isolated-worker', { body: JSON.stringify({ browser: test.info().project.name, result, requests }), contentType: 'application/json' });
    } finally { await new Promise((resolve) => server.close(resolve)); }
});
