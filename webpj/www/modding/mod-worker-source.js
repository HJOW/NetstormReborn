// 운영 번들의 실행기 데이터 또는 Native debug가 읽은 같은 봉투를 해독한다. 번들 텍스트를 평가하지 않는다.
import { ModError } from './mod-api.js';

// webpack 삽입기와 대조하는 고정 형식이다. guest 모드의 전역에는 이 데이터가 없다.
export const MOD_WORKER_PAYLOAD_KEY = '__netstormModWorkerPayload';
export const MOD_WORKER_PAYLOAD_PREFIX = 'NETSTORM_MOD_WORKER_V0:';
export const MOD_WORKER_PAYLOAD_SUFFIX = ':END_NETSTORM_MOD_WORKER';
// SDK는 신뢰 코드지만 손상된 데이터로 무제한 해독을 시도하지 않는다.
const MAX_PAYLOAD_CHARACTERS = 12 * 1024 * 1024;
// Native 모듈 주소는 변수로 사용해 webpack의 new URL 자산 처리와 번들의 자기 포함을 막는다.
const MODULE_URL = import.meta.url;

// UTF-8 실행기 원문을 정확히 한 번 해독한다. 네이티브 fetch/eval/Function을 모드에 주지 않는다.
export function decodeModWorkerPayload(payload) {
    if (typeof payload !== 'string' || payload.length > MAX_PAYLOAD_CHARACTERS || !payload.startsWith(MOD_WORKER_PAYLOAD_PREFIX) || !payload.endsWith(MOD_WORKER_PAYLOAD_SUFFIX)) throw new ModError('scriptError', '모드 실행기 데이터 형식 오류');
    const encoded = payload.slice(MOD_WORKER_PAYLOAD_PREFIX.length, -MOD_WORKER_PAYLOAD_SUFFIX.length);
    if (!encoded || encoded.length % 4 || !/^[A-Za-z0-9+/]*={0,2}$/u.test(encoded)) throw new ModError('scriptError', '모드 실행기 base64 오류');
    try { const binary = atob(encoded); return new TextDecoder('utf-8', { fatal: true }).decode(Uint8Array.from(binary, (character) => character.charCodeAt(0))); }
    catch (error) { throw new ModError('scriptError', '모드 실행기 UTF-8 데이터 손상'); }
}

// debug.html도 생산 번들의 데이터만 읽으므로 별도 Worker 파일이나 별도 빌드가 필요 없다.
export async function loadModWorkerSource({ view = globalThis, fetch = view.fetch?.bind(view), signal } = {}) {
    if (typeof view[MOD_WORKER_PAYLOAD_KEY] === 'string') return decodeModWorkerPayload(view[MOD_WORKER_PAYLOAD_KEY]);
    if (import.meta.webpack) throw new ModError('scriptError', '번들에 모드 실행기가 없습니다. 빌드를 갱신하십시오.');
    if (typeof fetch !== 'function') throw new ModError('scriptError', '모드 실행기를 읽을 수 없습니다.');
    const file = '../dist/netstorm.bundle.js', response = await fetch(new URL(file, MODULE_URL), { cache: 'no-store', ...(signal ? { signal } : {}) });
    if (!response.ok) throw new ModError('scriptError', `모드 실행기 번들 읽기 실패(${response.status})`);
    const bundle = await response.text(), matches = [...bundle.matchAll(/NETSTORM_MOD_WORKER_V0:[A-Za-z0-9+/=]+:END_NETSTORM_MOD_WORKER/gu)];
    if (matches.length !== 1) throw new ModError('scriptError', '번들의 모드 실행기 봉투가 없거나 중복되었습니다.');
    return decodeModWorkerPayload(matches[0][0]);
}
