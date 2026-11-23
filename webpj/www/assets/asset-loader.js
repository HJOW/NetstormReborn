// 웹 공개 루트의 자산 묶음(www/data)을 매니페스트에 따라 내려받는 모듈이다.
// 매니페스트(data/manifest.json)는 webpj/tools/prepare-assets.js 가 assets/game-data 의 원본 파일을 복사하면서 만든다.
// 파일마다 경로·용도 묶음·크기·SHA-256 이 기록되며, 실행 중에는 크기를 확인해 잘린 파일을 조기에 보고한다.
// (SHA-256 대조는 Node 검사(webpj/tests)가 맡는다. 브라우저의 crypto.subtle 은 보안 문맥에서만 쓸 수 있기 때문이다.)
// (2026-10-23 WB03-M1: 메인 메뉴 자산을 원본 설치 없이 공개 루트에서 읽기 위해 추가)

// 매니페스트 파일 이름(데이터 폴더 기준)이다. (2026-10-23 WB03-M1)
export const MANIFEST_FILE = 'manifest.json';

// 이 모듈이 읽을 수 있는 매니페스트 형식 번호이다. 형식을 바꾸면 준비 도구와 함께 올린다. (2026-10-23 WB03-M1)
export const MANIFEST_FORMAT = 1;

// 일시적 실패(네트워크 끊김·잘린 응답·혼잡한 서버 응답)에서 같은 요청을 다시 시도하기까지 기다리는 시간(밀리초) 목록이다. 길이가 곧 재시도 횟수이며 첫 시도는 바로 한다.
// 처음 방문(캐시 없음)의 100여 개 요청 가운데 하나만 실패해도 메뉴 묶음이 실패해 오류 화면이 되거나 한국어 묶음이 실패해 영어로 뜨던 문제를 줄이려고 더했다. (2026-11-24 WB04-E1300)
export const DEFAULT_RETRY_DELAYS = Object.freeze([400, 1200, 2800]);

// 한 묶음 안에서 동시에 내려받는 파일 수의 기본 한도이다. 처음 방문에 요청 100여 개를 한꺼번에 보내지 않고 이 수만큼씩 이어 받아 서버·브라우저 혼잡을 줄인다. (2026-11-24 WB04-E1300)
export const DEFAULT_CONCURRENCY = 12;

// 다시 시도해 볼 만한 HTTP 상태인지 판정한다: 요청 시간 초과(408)·너무 이른(425)·요청 과다(429)·서버 오류(5xx). 없는 파일(404)·권한(403) 같은 영구 실패는 다시 시도하지 않는다. (2026-11-24 WB04-E1300)
const retryableStatus = (status) => status === 408 || status === 425 || status === 429 || status >= 500;

// 데이터 폴더 기준 상대 경로를 URL 로 안전하게 바꾼다. 각 경로 조각을 인코딩해 '!' 같은 문자를 보존한다. (2026-10-23 WB03-M1)
function encodePath(relativePath) {
    return relativePath.split('/').map((part) => encodeURIComponent(part)).join('/');
}

// 매니페스트 기반 자산 적재기이다.
// 사용: const loader = new AssetLoader({ baseUrl: new URL('./data/', document.baseURI), fetchFunction: fetch.bind(window) });
//       await loader.loadManifest(); const files = await loader.loadBundle('menu', (loaded, total) => ...);
// (2026-10-23 WB03-M1)
export class AssetLoader {
    // 데이터 폴더 URL(끝이 '/')과 fetch 함수를 받는다. 검사에서는 디스크를 읽는 가짜 fetch 를 넘길 수 있다.
    // retryDelays: 일시적 실패 뒤 재시도 간격(밀리초) 목록(기본 DEFAULT_RETRY_DELAYS, 빈 배열이면 재시도 없음), concurrency: 묶음 안 동시 내려받기 수(기본 DEFAULT_CONCURRENCY),
    // sleep: (밀리초) => Promise 대기 함수(검사에서 시간을 건너뛰도록 바꿔 끼운다). (2026-11-24 WB04-E1300: 재시도·동시성 한도 추가)
    constructor({ baseUrl, fetchFunction, retryDelays = DEFAULT_RETRY_DELAYS, concurrency = DEFAULT_CONCURRENCY, sleep = (milliseconds) => new Promise((resolve) => setTimeout(resolve, milliseconds)) }) {
        if (typeof fetchFunction !== 'function') throw new TypeError('AssetLoader: fetch 함수가 필요합니다.');
        if (!Array.isArray(retryDelays) || retryDelays.some((value) => !(value >= 0))) throw new TypeError('AssetLoader: retryDelays 는 0 이상 숫자 배열이어야 합니다.');
        if (!Number.isInteger(concurrency) || concurrency < 1) throw new RangeError('AssetLoader: concurrency 는 1 이상 정수여야 합니다.');
        // 재시도 간격 목록과 동시 내려받기 한도, 대기 함수이다.
        this.retryDelays = [...retryDelays];
        this.concurrency = concurrency;
        this.sleep = sleep;
        // 데이터 폴더의 절대 URL 이다.
        this.baseUrl = new URL(baseUrl);
        // 파일을 내려받는 fetch 함수다.
        this.fetchFunction = fetchFunction;
        // 읽은 매니페스트다. loadManifest 전에는 null 이다.
        this.manifest = null;
    }

    // 데이터 폴더 기준 경로의 절대 URL 을 만든다.
    url(relativePath) {
        return new URL(encodePath(relativePath), this.baseUrl).href;
    }

    // 시도 함수 하나를 일시적 실패에서 재시도하며 실행한다. attempt 가 던진 오류에 `retryable === false` 가 있으면(없는 파일 등) 바로 던지고,
    // 그 밖의 오류(네트워크 끊김·잘린 응답 등)는 retryDelays 만큼 기다려 다시 시도하며 마지막에도 실패하면 마지막 오류를 던진다. (2026-11-24 WB04-E1300)
    async #withRetry(attempt) {
        for (let tries = 0; ; tries += 1) {
            try {
                return await attempt();
            } catch (error) {
                if (error?.retryable === false || tries >= this.retryDelays.length) throw error;
                await this.sleep(this.retryDelays[tries]);
            }
        }
    }

    // 매니페스트를 읽고 형식을 확인한다. 형식이 다르거나 파일 목록이 없으면 오류다. 일시적 실패는 다시 시도한다.
    async loadManifest() {
        const manifest = await this.#withRetry(async () => {
            const response = await this.fetchFunction(this.url(MANIFEST_FILE));
            if (!response.ok) throw Object.assign(new Error(`자산 매니페스트를 읽지 못했습니다 (${response.status}).`), { retryable: retryableStatus(response.status) });
            const loaded = await response.json();
            if (loaded?.format !== MANIFEST_FORMAT || !Array.isArray(loaded.files)) throw new Error('자산 매니페스트 형식이 맞지 않습니다.');
            return loaded;
        });
        this.manifest = manifest;
        return manifest;
    }

    // 묶음 이름에 속한 매니페스트 항목 목록을 돌려준다. 매니페스트를 먼저 읽어야 한다.
    entries(bundle) {
        if (!this.manifest) throw new Error('자산 매니페스트를 먼저 읽어야 합니다.');
        return this.manifest.files.filter((entry) => entry.bundle === bundle);
    }

    // 파일 하나를 내려받아 Uint8Array 로 돌려준다. onBytes(늘어난 바이트 수)로 진행을 알리며 크기가 다르면 오류다.
    // 일시적 실패(네트워크 오류·408/425/429/5xx·잘린 응답)는 같은 파일을 처음부터 다시 받는다. 실패한 시도가 알린 바이트는 음수로 되돌려 진행률이 부풀지 않게 한다. 404 같은 영구 실패는 바로 던진다. (2026-11-24 WB04-E1300)
    async loadFile(entry, onBytes = null) {
        return this.#withRetry(async () => {
            let counted = 0;
            const report = (count) => { counted += count; onBytes?.(count); };
            try {
                const response = await this.fetchFunction(this.url(entry.path));
                if (!response.ok) throw Object.assign(new Error(`자산 파일을 읽지 못했습니다: ${entry.path} (${response.status})`), { retryable: retryableStatus(response.status) });
                let bytes;
                if (response.body && typeof response.body.getReader === 'function') {
                    const reader = response.body.getReader();
                    const chunks = [];
                    let total = 0;
                    // 스트림 조각을 끝까지 읽으며 진행량을 알린다.
                    for (;;) {
                        const { done, value } = await reader.read();
                        if (done) break;
                        chunks.push(value);
                        total += value.length;
                        report(value.length);
                    }
                    bytes = new Uint8Array(total);
                    let offset = 0;
                    // 읽은 조각들을 하나의 배열로 이어 붙인다.
                    for (const chunk of chunks) {
                        bytes.set(chunk, offset);
                        offset += chunk.length;
                    }
                } else {
                    bytes = new Uint8Array(await response.arrayBuffer());
                    report(bytes.length);
                }
                if (bytes.length !== entry.size) throw new Error(`자산 파일 크기가 매니페스트와 다릅니다: ${entry.path} (${bytes.length} ≠ ${entry.size})`);
                return bytes;
            } catch (error) {
                // 이 시도가 알린 진행량을 되돌린 뒤 오류를 그대로 올려 재시도 여부를 바깥이 정한다.
                if (counted > 0) onBytes?.(-counted);
                throw error;
            }
        });
    }

    // 묶음의 모든 파일을 내려받아 경로 → Uint8Array 의 Map 으로 돌려준다. 동시에 받는 파일 수는 concurrency 이하이다.
    // onProgress(받은 바이트, 전체 바이트)로 진행률을 알린다. 파일마다 일시적 실패는 다시 시도하고, 그래도 하나라도 실패하면 오류를 던진다(남은 파일은 더 시작하지 않는다).
    async loadBundle(bundle, onProgress = null) {
        const entries = this.entries(bundle);
        if (entries.length === 0) throw new Error(`자산 묶음이 비어 있습니다: ${bundle}`);
        const total = entries.reduce((sum, entry) => sum + entry.size, 0);
        let loaded = 0;
        onProgress?.(0, total);
        const results = new Array(entries.length);
        let next = 0;
        let failure = null;
        // 일꾼 하나는 남은 파일을 차례로 집어 받는다. 한 일꾼이 실패하면 다른 일꾼도 새 파일을 시작하지 않는다.
        const worker = async () => {
            while (!failure && next < entries.length) {
                const index = next;
                next += 1;
                const entry = entries[index];
                try {
                    const bytes = await this.loadFile(entry, (count) => {
                        loaded += count;
                        onProgress?.(Math.max(0, Math.min(loaded, total)), total);
                    });
                    results[index] = [entry.path, bytes];
                } catch (error) {
                    failure ??= error;
                }
            }
        };
        await Promise.all(Array.from({ length: Math.min(this.concurrency, entries.length) }, worker));
        if (failure) throw failure;
        return new Map(results);
    }
}
