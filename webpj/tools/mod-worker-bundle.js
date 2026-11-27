// 모드 실행기를 별도 JS 요청 없이 본 번들에 데이터로 넣는다. 메인 페이지에서는 SES 코드를 실행하지 않는다.
'use strict';
const path = require('node:path');
const webpack = require('webpack');

// Native debug도 번들을 평가하지 않고 같은 봉투만 추출한다. 브라우저 해독기의 상수와 검사로 대조한다.
const PAYLOAD_KEY = '__netstormModWorkerPayload';
const PAYLOAD_PREFIX = 'NETSTORM_MOD_WORKER_V0:';
const PAYLOAD_SUFFIX = ':END_NETSTORM_MOD_WORKER';

// 선행 ESLint를 통과한 부모 compilation 안에서 신뢰 Worker를 묶는 플러그인이다.
class ModWorkerBundlePlugin {
    // 실제 프로젝트와 작은 빌드 검사에서 각각 지정할 입력/출력 이름이다.
    #entry; #bundle;
    // 부모 출력은 하나이며 자식의 임시 자산은 내보내지 않는다.
    constructor({ entry = path.resolve(__dirname, '../www/modding/mod-worker-entry.js'), bundle = 'netstorm.bundle.js' } = {}) { this.#entry = entry; this.#bundle = bundle; }
    // thisCompilation은 자식에게 복사되지 않아 실행기 안에 실행기를 다시 넣지 않는다.
    apply(compiler) {
        compiler.hooks.thisCompilation.tap('ModWorkerBundlePlugin', (compilation) => {
            compilation.hooks.processAssets.tapPromise({ name: 'ModWorkerBundlePlugin', stage: webpack.Compilation.PROCESS_ASSETS_STAGE_REPORT }, async () => {
                const temporary = 'netstorm.mod-worker.internal.js';
                const child = compilation.createChildCompiler('NetstormModWorker', { filename: temporary, globalObject: 'self' }, [new webpack.webworker.WebWorkerTemplatePlugin(), new webpack.EntryPlugin(path.dirname(this.#entry), this.#entry, { name: 'mod-worker' }), new webpack.LoaderTargetPlugin('webworker')]);
                child.options.devtool = false;
                child.options.optimization = { ...child.options.optimization, splitChunks: false, runtimeChunk: false };
                const result = await new Promise((resolve, reject) => child.runAsChild((error, entries, built) => error ? reject(error) : resolve(built)));
                if (result.errors.length) throw new Error(result.errors.map((error) => error.message).join('\n'));
                const asset = result.getAsset(temporary);
                if (!asset) throw new Error('모드 실행기 자식 산출물이 없습니다.');
                const source = String(asset.source.source()), payload = PAYLOAD_PREFIX + Buffer.from(source, 'utf8').toString('base64') + PAYLOAD_SUFFIX;
                // watch에서도 실행기의 변경과 누락 의존성을 다시 확인한다.
                for (const file of result.fileDependencies) compilation.fileDependencies.add(file);
                for (const directory of result.contextDependencies) compilation.contextDependencies.add(directory);
                for (const file of result.missingDependencies) compilation.missingDependencies.add(file);
                // 자식 라이선스/임시 스크립트도 별도 배포하지 않는다. 라이선스 본문은 www/licenses에 제공한다.
                for (const { name } of result.getAssets()) compilation.deleteAsset(name);
                if (!compilation.getAsset(this.#bundle)) throw new Error('모드 데이터를 넣을 부모 번들이 없습니다.');
                const preamble = `Object.defineProperty(globalThis,${JSON.stringify(PAYLOAD_KEY)},{value:${JSON.stringify(payload)},configurable:true});\n`;
                compilation.updateAsset(this.#bundle, (original) => new webpack.sources.ConcatSource(preamble, original));
                // 추가한 첫 한 줄만큼 개발용 평면 소스 맵을 옮긴다. 원래 코드의 열 위치는 보존한다.
                const mapName = compilation.getAsset(this.#bundle).info.related?.sourceMap;
                if (mapName && compilation.getAsset(mapName)) {
                    const map = JSON.parse(String(compilation.getAsset(mapName).source.source()));
                    if (typeof map.mappings !== 'string') throw new Error('지원하지 않는 번들 소스 맵 형식');
                    map.mappings = ';' + map.mappings; compilation.updateAsset(mapName, new webpack.sources.RawSource(JSON.stringify(map)));
                }
            });
        });
    }
}

module.exports = { ModWorkerBundlePlugin, PAYLOAD_KEY, PAYLOAD_PREFIX, PAYLOAD_SUFFIX };
