// 웹 공개 루트의 자산 묶음(www/data)을 매니페스트에 따라 내려받는 모듈이다.
// 매니페스트(data/manifest.json)는 webpj/tools/prepare-assets.js 가 assets/game-data 의 원본 파일을 복사하면서 만든다.
// 파일마다 경로·용도 묶음·크기·SHA-256 이 기록되며, 실행 중에는 크기를 확인해 잘린 파일을 조기에 보고한다.
// (SHA-256 대조는 Node 검사(webpj/tests)가 맡는다. 브라우저의 crypto.subtle 은 보안 문맥에서만 쓸 수 있기 때문이다.)
// (2026-10-23 WB03-M1: 메인 메뉴 자산을 원본 설치 없이 공개 루트에서 읽기 위해 추가)

// 매니페스트 파일 이름(데이터 폴더 기준)이다. (2026-10-23 WB03-M1)
export const MANIFEST_FILE = 'manifest.json';

// 이 모듈이 읽을 수 있는 매니페스트 형식 번호이다. 형식을 바꾸면 준비 도구와 함께 올린다. (2026-10-23 WB03-M1)
export const MANIFEST_FORMAT = 1;

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
    constructor({ baseUrl, fetchFunction }) {
        if (typeof fetchFunction !== 'function') throw new TypeError('AssetLoader: fetch 함수가 필요합니다.');
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

    // 매니페스트를 읽고 형식을 확인한다. 형식이 다르거나 파일 목록이 없으면 오류다.
    async loadManifest() {
        const response = await this.fetchFunction(this.url(MANIFEST_FILE));
        if (!response.ok) throw new Error(`자산 매니페스트를 읽지 못했습니다 (${response.status}).`);
        const manifest = await response.json();
        if (manifest?.format !== MANIFEST_FORMAT || !Array.isArray(manifest.files)) throw new Error('자산 매니페스트 형식이 맞지 않습니다.');
        this.manifest = manifest;
        return manifest;
    }

    // 묶음 이름에 속한 매니페스트 항목 목록을 돌려준다. 매니페스트를 먼저 읽어야 한다.
    entries(bundle) {
        if (!this.manifest) throw new Error('자산 매니페스트를 먼저 읽어야 합니다.');
        return this.manifest.files.filter((entry) => entry.bundle === bundle);
    }

    // 파일 하나를 내려받아 Uint8Array 로 돌려준다. onBytes(늘어난 바이트 수)로 진행을 알리며 크기가 다르면 오류다.
    async loadFile(entry, onBytes = null) {
        const response = await this.fetchFunction(this.url(entry.path));
        if (!response.ok) throw new Error(`자산 파일을 읽지 못했습니다: ${entry.path} (${response.status})`);
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
                onBytes?.(value.length);
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
            onBytes?.(bytes.length);
        }
        if (bytes.length !== entry.size) throw new Error(`자산 파일 크기가 매니페스트와 다릅니다: ${entry.path} (${bytes.length} ≠ ${entry.size})`);
        return bytes;
    }

    // 묶음의 모든 파일을 동시에 내려받아 경로 → Uint8Array 의 Map 으로 돌려준다.
    // onProgress(받은 바이트, 전체 바이트)로 진행률을 알린다. 하나라도 실패하면 오류를 던진다.
    async loadBundle(bundle, onProgress = null) {
        const entries = this.entries(bundle);
        if (entries.length === 0) throw new Error(`자산 묶음이 비어 있습니다: ${bundle}`);
        const total = entries.reduce((sum, entry) => sum + entry.size, 0);
        let loaded = 0;
        onProgress?.(0, total);
        const results = await Promise.all(entries.map(async (entry) => {
            const bytes = await this.loadFile(entry, (count) => {
                loaded += count;
                onProgress?.(Math.min(loaded, total), total);
            });
            return [entry.path, bytes];
        }));
        return new Map(results);
    }
}
