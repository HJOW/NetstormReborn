// webpack 설정이다: 웹 판본의 모든 스크립트(`www/bootstrap.js` 에서 import 로 닿는 ES 모듈 전체)를 번들 하나로 묶는다.
// 사용(webpj 폴더에서): `npm run build`(운영, 최소화) · `npm run build:dev`(개발, 최소화 없음·소스 맵) · `npm run check-bundle`(크기 한도 검사).
// 산출물은 `www/dist/netstorm.bundle.js` 하나이며 index.html 이 <script defer> 로 불러온다. 게임 자료(`www/data`)·글꼴은 번들에 넣지 않고 지금처럼 같은 출처에서 따로 받는다.
// 클래스·함수 이름은 최소화해도 보존한다(`keep_classnames`/`keep_fnames`): 시험·진단이 `constructor.name` 등을 읽고 오류 문구가 이름을 쓰기 때문이다.
// (2026-11-24 WB04-E1293: 사용자 지시로 스크립트 전체를 번들 하나로 묶어 크기를 확인하려고 추가. 크기 한도는 tools/check-bundle-size.js)
'use strict';

const path = require('path');
const TerserPlugin = require('terser-webpack-plugin');
// 운영/개발 모드와 webpack API 호출에서 출력 정리 전에 전체 lint를 기다리는 관문이다.
const { ESLintBeforeBuildPlugin } = require('./tools/eslint-before-build.js');
// iframe 안에서만 실행할 SDK도 같은 최종 번들 데이터에 넣는다.
const { ModWorkerBundlePlugin } = require('./tools/mod-worker-bundle.js');
const webpack = require('webpack');

// webpack 이 부르는 설정 함수이다. argv.mode 가 development 면 개발 번들, 그 밖에는 운영 번들이다.
module.exports = (env, argv) => {
    const production = argv.mode !== 'development';
    return {
        // CLI와 외부 디렉터리의 webpack API 호출에서 내장 실행기의 모듈 ID도 같게 만든다.
        context: __dirname,
        mode: production ? 'production' : 'development',
        entry: { netstorm: path.resolve(__dirname, 'www', 'bootstrap.js') },
        output: {
            path: path.resolve(__dirname, 'www', 'dist'),
            filename: '[name].bundle.js',
            clean: true, // dist 에는 번들만 있으므로 빌드마다 비운다.
            // 검사에 통과한 빌드는 같은 바이트여도 출력 시각을 갱신해 ensure-bundle의 반복 재생성을 막는다.
            compareBeforeEmit: false,
            publicPath: 'auto', // 하위 경로 정적 배포에서도 번들 자신의 주소를 기준으로 한다(번들에는 추가 조각이 없다).
        },
        devtool: production ? false : 'source-map',
        target: 'web',
        plugins: [new ESLintBeforeBuildPlugin(__dirname), new ModWorkerBundlePlugin(), new webpack.DefinePlugin({ 'process.env.BABEL_8_BREAKING': 'false', 'process.env.BABEL_TYPES_8_BREAKING': 'false' })],
        optimization: {
            minimize: production,
            minimizer: [new TerserPlugin({
                extractComments: false, // 라이선스 주석 파일을 따로 만들지 않는다(번들 하나만 남긴다).
                terserOptions: { keep_classnames: true, keep_fnames: true, format: { comments: false } },
            })],
            splitChunks: false, // 청크를 나누지 않는다.
            runtimeChunk: false,
        },
        performance: { hints: false }, // 크기 한도는 tools/check-bundle-size.js 가 판정한다.
    };
};
