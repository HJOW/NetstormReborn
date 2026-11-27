// 이 신뢰 진입점은 불투명 iframe의 Worker 안에서만 실행된다. 게임과 guest의 전역을 공유하지 않는다.
import 'ses';
import { ModuleSource } from '@endo/module-source';
import { evadeCensorSync } from '@endo/evasive-transform';
import { parse } from 'acorn';
import { ModRuntime, ModError, MOD_API_VERSION, copyModJson } from './mod-api.js';
import { readModPackage, resolveModSpecifier } from './mod-package.js';
import { createModMath } from './mod-math.js';

// 메시지 권한은 신뢰 closure에만 남기고 guest에게는 공개 API만 전달한다.
const send = globalThis.postMessage.bind(globalThis);
globalThis.lockdown();
// 하나의 Worker는 한 미션의 한 모드만 소유한다. 검증 전용 Worker는 엔트리를 실행하지 않는다.
let runtime = null, registered = false;

// 정적 패키지 모듈만 가진 Compartment를 만들며 URL 로더나 호스트 전역은 주입하지 않는다.
function register(text, sessionSeed) {
    if (registered) throw new ModError('scriptError', 'Worker의 모드는 한 번만 등록할 수 있습니다.');
    registered = true;
    const pkg = readModPackage(text, { parse }), modules = Object.create(null);
    // SES의 보수적인 import 문자열 검사를 값 보존 변환으로 피한다. 실제 import AST 검사는 이미 끝났다.
    for (const [name, source] of Object.entries(pkg.modules)) modules[name] = { source: new ModuleSource(evadeCensorSync(source, { sourceType: 'module', sourceUrl: name }).code, name) };
    const compartment = new globalThis.Compartment({ name: pkg.manifest.id, globals: { eval: undefined, Function: undefined, Compartment: undefined, Date: undefined, Promise: undefined }, modules, resolveHook: resolveModSpecifier, __options__: true });
    // 주 realm의 Math.random이 아닌 이 Compartment의 tamed Math만 바탕으로 쓴다.
    compartment.globalThis.Math = createModMath(compartment.globalThis.Math);
    Object.freeze(compartment.globalThis);
    const entry = compartment.importNow(pkg.manifest.entry);
    runtime = new ModRuntime({ modId: pkg.manifest.id, sessionSeed });
    return runtime.register(entry.default);
}

// 신뢰 호스트의 순차 요청만 처리한다. guest는 이 함수나 postMessage에 접근할 수 없다.
globalThis.onmessage = ({ data }) => {
    try {
        if (!data || data.protocol !== 1 || !Number.isSafeInteger(data.id) || data.id < 1) throw new ModError('scriptError', 'Worker 요청 봉투 오류');
        let result;
        if (data.op === 'validate' && !registered) result = JSON.stringify(readModPackage(data.package, { parse }));
        else if (data.op === 'register') result = register(data.package, data.sessionSeed);
        else if (data.op === 'dispatch' && runtime) result = runtime.dispatch(copyModJson(data.event), copyModJson(data.snapshot));
        else throw new ModError('scriptError', 'Worker 요청 순서/종류 오류');
        send({ type: 'result', protocol: 1, id: data.id, op: data.op, result });
    } catch (error) {
        runtime?.dispose(); runtime = null;
        send({ type: 'result', protocol: 1, id: data?.id, op: data?.op, error: { code: error instanceof ModError ? error.code : 'scriptError', message: String(error?.message ?? error).slice(0, 2048) } });
    }
};
send({ type: 'ready', protocol: 1, apiVersion: MOD_API_VERSION });
