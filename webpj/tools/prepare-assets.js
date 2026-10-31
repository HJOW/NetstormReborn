// webpj 자산 준비 도구이다. 클론 공용 원본 자산 폴더(assets/game-data)에서 웹 실행에 필요한 파일을
// 웹 공개 루트(webpj/www/data)로 변환 없이 그대로 복사하고, 경로·용도 묶음·크기·SHA-256 을 기록한 manifest.json 을 만든다.
// 원본 파일을 바이트 그대로 복사하므로 Git 에서는 같은 blob 을 공유해 저장소 크기가 늘지 않는다.
// 한국어 지원 자산(프로젝트 fonts/ 의 D2Coding woff2·라이선스, webpj/translations/ 의 번역표)도 같은 방식으로 복사한다. 자산마다 source(저장소 루트 기준 원본 경로)를 따로 적을 수 있다.
// 사용법(저장소 루트 기준):
//   node webpj/tools/prepare-assets.js          복사 + 매니페스트 작성
//   node webpj/tools/prepare-assets.js --check  복사본·매니페스트가 원본과 같은지 검사만 한다(다르면 종료 코드 1)
// originals/·originalCD/ 는 읽지 않는다. 브라우저 실행 중에도 원본 설치 폴더를 찾지 않는다.
// (2026-10-23 WB03-M1: 메인 메뉴 자산 묶음 준비를 위해 추가, 2026-10-28 WB02-D: 아카이브·기본 설정 추가, 2026-10-31 WB03-N: 한국어 자산 묶음 추가, WB03-O: 한국어 언어 자료 파일 추가)
'use strict';

const fs = require('fs');
const path = require('path');
const crypto = require('crypto');

// webpj 폴더의 절대 경로이다. (2026-10-23 WB03-M1)
const WEBPJ_ROOT = path.resolve(__dirname, '..');

// 저장소 루트의 절대 경로이다. (2026-10-23 WB03-M1)
const REPO_ROOT = path.resolve(WEBPJ_ROOT, '..');

// 복사할 원본 자산이 있는 폴더(클론 공용 실행 데이터)이다. (2026-10-23 WB03-M1)
const SOURCE_ROOT = path.join(REPO_ROOT, 'assets', 'game-data');

// 복사본을 둘 웹 공개 데이터 폴더이다. 정적 서버는 webpj/www 만 공개하므로 이 아래에 둔다. (2026-10-23 WB03-M1)
const DATA_ROOT = path.join(WEBPJ_ROOT, 'www', 'data');

// 매니페스트 파일 경로이다. 브라우저 적재기(www/assets/asset-loader.js)가 읽는다. (2026-10-23 WB03-M1)
const MANIFEST_PATH = path.join(DATA_ROOT, 'manifest.json');

// 매니페스트 형식 번호이다. 브라우저 적재기의 MANIFEST_FORMAT 과 같아야 한다. (2026-10-23 WB03-M1)
const MANIFEST_FORMAT = 1;

// 용도 묶음 이름과 설명이다. 'menu' 는 메인 메뉴 표시 전에 필수, 'menu-music' 은 뒤에서 받는 음악이다. (2026-10-23 WB03-M1)
const BUNDLES = Object.freeze({
    menu: '메인 메뉴 표시 전에 받아야 하는 필수 자산',
    'menu-music': '메인 메뉴 음악(크기가 커서 화면 표시를 막지 않고 뒤에서 받음)',
    korean: '한국어 지원(D2Coding 글꼴 woff2·UI 번역표 xlat.korean·Tell 본문 tell.korean·캠페인 목록 offical*.korean·용어표 config.korean). 언어가 한국어일 때만 받는다(2026-10-31 WB03-N, WB03-O)',
});

// 원본 글꼴 캐시 파일 목록(슬롯 0~6 × 스타일 중 원본에 존재하는 조합, 파일 이름 규칙은 www/assets/font-set.js 와 같다)이다.
// 대화상자의 제목(슬롯 3)·본문(슬롯 5)·굵게/기울임 글자와 코드 글꼴(슬롯 1) 등을 그리는 데 쓴다. 검사(tests/dialog.test.mjs)가 font-set.js 의 목록과 같은지 확인한다. (2026-10-28 WB03-D)
const FONT_FILES = Object.freeze(
    [
        '!Arial.normal.14.700.chfnt', '!Arial.italic.14.700.chfnt', '!Arial.bold.14.700.chfnt', '!Arial.strikeout.14.700.chfnt', '!Arial.underline.14.700.chfnt',
        '!Courier New.normal.13.0.chfnt',
        '!Arial.normal.20.700.chfnt',
        '!Arial.normal.48.0.chfnt',
        '!Arial.normal.14.0.chfnt', '!Arial.italic.14.0.chfnt', '!Arial.bold.14.0.chfnt', '!Arial.strikeout.14.0.chfnt', '!Arial.underline.14.0.chfnt',
        '!Arial.normal.12.0.chfnt', '!Arial.italic.12.0.chfnt', '!Arial.bold.12.0.chfnt', '!Arial.strikeout.12.0.chfnt', '!Arial.underline.12.0.chfnt',
    ].map((name) => `d/${name}`),
);

// 한국어 언어 자료 파일 목록이다(웹 판본에서 닿는 Tell 섹션·캠페인 목록·용어표). 원본 영어 파일과 같은 이름에서 확장자만 `.korean` 이다.
// tell.korean 은 번역한 섹션만 담은 부분 파일이라 없는 섹션은 영어로 대체된다. 검사(tests/korean-tell.test.mjs)가 영어 원문과 구조를 대조한다. (2026-10-31 WB03-O)
const KOREAN_LANGUAGE_FILES = Object.freeze([
    { name: 'tell.korean', use: '한국어 Tell 대화상자 본문(UTF-8, 부분 번역): 환영·팁·캠페인·크레딧·데모·버전·기술 지원 섹션과 팁 목록' },
    { name: 'config.korean', use: '한국어 용어표(UTF-8): 워크샾·신전·사제 같은 이름 용어와 기본 확인 단추 OkButton' },
    { name: 'help.korean', use: '한국어 F1 도움말(UTF-8, 번역 앵커 126개: 본문 104개와 별칭 22개): 번역하지 않은 주제(serverHelp·map)는 영어 help.english 에서 주제 단위로 대체된다(2026-10-31 WB03-P, 2026-11-01 WB03-Q, WB03-R, WB03-S)' },
    ...['offical1', 'offical2', 'offical2sep', 'offical3', 'offical4', 'offical5', 'offical5sep', 'offical6'].map((name) => ({
        name: `${name}.korean`,
        use: `한국어 캠페인 목록 파일(UTF-8): 원본 ${name}.english 의 제목·소개·미션 이름(명령·치환 키는 원문 그대로)`,
    })),
]);

// 복사할 자산 목록이다. path 는 www/data 안의 상대 경로(원본이 assets/game-data 이면 그쪽의 같은 경로), use 는 원본 근거와 용도다.
// source 는 원본이 assets/game-data 밖일 때 저장소 루트 기준 원본 경로이다(생략하면 assets/game-data/<path>). (2026-10-23 WB03-M1, 2026-10-31 WB03-N: source 추가)
const ASSET_FILES = Object.freeze([
    { path: 'd/GIFCLOUD.COL', bundle: 'menu', use: '기본 게임 팔레트(setup.cfg fortPal="gifcloud", GamePalSpec)' },
    { path: 'd/Gifcloud.gif', bundle: 'menu', use: '메인 메뉴 구름 배경(setup.cfg CloudFileSpec, 512x512)' },
    { path: 'd/titleMenu.gif', bundle: 'menu', use: '메인 메뉴 타이틀 그림(backgroundGifSpec + titleMenu)' },
    { path: 'd/_shapes.shp', bundle: 'menu', use: '셰이프 데이터베이스(fortGump 블록 22 프레임 1 = 돌 버튼 질감)' },
    ...FONT_FILES.map((fontPath) => ({ path: fontPath, bundle: 'menu', use: '원본 비트맵 글꼴 캐시(슬롯·스타일별, 슬롯 0 보통 = 돌 버튼 라벨, 그 밖 = 대화상자 제목·본문·목록 글자)' })),
    { path: 'cursors/RT_CURSOR_8.bin', bundle: 'menu', use: '원본 기본 커서 번호 1(흰 화살표, 핫스폿 10,6)' },
    { path: 'cursors/RT_CURSOR_17.bin', bundle: 'menu', use: '원본 커서 번호 15(가로 막대를 지나는 상하 화살표, 핫스폿 15,13): 스크롤할 수 있는 본문 위(ScrollingTextGumpWidgets kScrollCursor)' },
    { path: 'cursors/RT_CURSOR_18.bin', bundle: 'menu', use: '원본 커서 번호 16(가리키는 손, 핫스폿 12,9): 본문의 링크 위(ScrollingTextGumpWidgets kTagCursor)' },
    { path: 'sound/button.wav', bundle: 'menu', use: '돌 버튼을 누르는 순간의 효과음' },
    { path: 'sound/openSubGump.wav', bundle: 'menu', use: '대화상자 목록 행을 눌러 명령을 실행할 때의 효과음(원본 MenuGumpInput Sound("openSubGump.wav"))' },
    { path: 'netstorm.tarc', bundle: 'menu', use: '데이터 아카이브 TAFF(메뉴 대화상자 본문 tell.english·용어표 config.english·미션 스크립트·타입 정의, 246개 엔트리)' },
    { path: 'd/setup.cfg', bundle: 'menu', use: '게임 기본 설정(스타일 코드 H2·Text·CText, 경로 지정값 missionSpec·guideSpec, 인코딩된 원본 파일)' },
    { path: 'music/ser22.mus', bundle: 'menu-music', use: '메인 메뉴(전투 밖) 음악, RIFF WAV PCM' },
    { path: 'fonts/D2Coding.woff2', source: 'fonts/D2Coding.woff2', bundle: 'korean', use: '한국어 UI 글꼴 D2Coding 보통체(woff2, 사용자 제공, FontFace 로 등록해 Canvas 로 글리프를 만듦)' },
    { path: 'fonts/D2CodingBold.woff2', source: 'fonts/D2CodingBold.woff2', bundle: 'korean', use: '한국어 UI 글꼴 D2Coding 굵은체(woff2, 굵은 글꼴 슬롯·굵게 스타일)' },
    { path: 'fonts/License.txt', source: 'fonts/License.txt', bundle: 'korean', use: 'D2Coding 글꼴 사용 허가(SIL Open Font License 1.1). 글꼴과 함께 배포한다.' },
    { path: 'd/xlat.korean', source: 'webpj/translations/xlat.korean', bundle: 'korean', use: '한국어 UI 문자열 번역표(원본 xlat 형식, UTF-8): 메인 메뉴 버튼·Help·Options 펼침 메뉴 라벨, 일반 도움말 창 Back·OK' },
    ...KOREAN_LANGUAGE_FILES.map(({ name, use }) => ({ path: `d/${name}`, source: `webpj/translations/${name}`, bundle: 'korean', use })),
]);

// 자산의 저장소 루트 기준 원본 경로를 돌려준다. source 가 없으면 assets/game-data 아래의 같은 경로이다. (2026-10-31 WB03-N)
function sourceOf(asset) {
    return asset.source ?? `assets/game-data/${asset.path}`;
}

// 바이트 배열의 SHA-256 을 16진 문자열로 계산한다. (2026-10-23 WB03-M1)
function sha256(buffer) {
    return crypto.createHash('sha256').update(buffer).digest('hex');
}

// 상대 경로(슬래시 구분)를 운영체제 경로로 바꿔 기준 폴더와 합친다. (2026-10-23 WB03-M1)
function resolveIn(root, relativePath) {
    return path.join(root, ...relativePath.split('/'));
}

// 원본 자산을 읽어 매니페스트 객체를 만든다. 원본 파일이 없으면 오류를 던진다. (2026-10-23 WB03-M1)
function buildManifest() {
    const files = ASSET_FILES.map((asset) => {
        const source = resolveIn(REPO_ROOT, sourceOf(asset));
        if (!fs.existsSync(source)) throw new Error(`원본 자산이 없습니다: ${sourceOf(asset)}`);
        const bytes = fs.readFileSync(source);
        return {
            path: asset.path,
            source: sourceOf(asset),
            bundle: asset.bundle,
            size: bytes.length,
            sha256: sha256(bytes),
            use: asset.use,
        };
    });
    return {
        format: MANIFEST_FORMAT,
        generator: 'webpj/tools/prepare-assets.js',
        source: 'assets/game-data',
        description: 'webpj 공개 루트의 원본 자산 복사본 목록. 파일은 변환 없이 복사하며 크기·SHA-256 은 원본(각 항목의 source)과 같다.',
        bundles: BUNDLES,
        files,
    };
}

// 매니페스트를 JSON 문자열(UTF-8, LF, 두 칸 들여쓰기)로 만든다. 같은 입력이면 같은 문자열이 나온다. (2026-10-23 WB03-M1)
function serializeManifest(manifest) {
    return `${JSON.stringify(manifest, null, 2)}\n`;
}

// 원본을 복사하고 매니페스트를 쓴다. 복사한 파일 목록을 돌려준다. (2026-10-23 WB03-M1)
function prepareAssets() {
    const manifest = buildManifest();
    // 각 자산을 원본과 같은 상대 경로로 복사한다.
    for (const entry of manifest.files) {
        const target = resolveIn(DATA_ROOT, entry.path);
        fs.mkdirSync(path.dirname(target), { recursive: true });
        fs.copyFileSync(resolveIn(REPO_ROOT, entry.source), target);
    }
    fs.writeFileSync(MANIFEST_PATH, serializeManifest(manifest), 'utf8');
    return manifest.files.map((entry) => entry.path);
}

// 복사본·매니페스트가 현재 원본과 같은지 검사한다. 문제 설명 문자열 목록을 돌려준다(비어 있으면 정상). (2026-10-23 WB03-M1)
function checkAssets() {
    const problems = [];
    let expected;
    try {
        expected = buildManifest();
    } catch (error) {
        return [error.message];
    }
    if (!fs.existsSync(MANIFEST_PATH)) return ['매니페스트가 없습니다: webpj/www/data/manifest.json'];
    const written = fs.readFileSync(MANIFEST_PATH, 'utf8');
    if (written !== serializeManifest(expected)) problems.push('매니페스트 내용이 현재 원본·자산 목록과 다릅니다.');
    // 각 복사본의 존재·크기·해시를 원본 기준으로 확인한다.
    for (const entry of expected.files) {
        const target = resolveIn(DATA_ROOT, entry.path);
        if (!fs.existsSync(target)) {
            problems.push(`복사본이 없습니다: www/data/${entry.path}`);
            continue;
        }
        const bytes = fs.readFileSync(target);
        if (bytes.length !== entry.size || sha256(bytes) !== entry.sha256) problems.push(`복사본이 원본과 다릅니다: www/data/${entry.path}`);
    }
    return problems;
}

// 명령행 진입점이다. --check 면 검사만 하고, 아니면 복사와 매니페스트 작성을 한다. (2026-10-23 WB03-M1)
function main(argv) {
    if (argv.includes('--check')) {
        const problems = checkAssets();
        // 발견한 문제를 한 줄씩 알린다.
        for (const problem of problems) console.error(`[webpj assets] ${problem}`);
        if (problems.length === 0) console.log('[webpj assets] 복사본과 매니페스트가 원본과 같습니다.');
        return problems.length === 0 ? 0 : 1;
    }
    const copied = prepareAssets();
    console.log(`[webpj assets] ${copied.length}개 파일을 webpj/www/data 로 복사하고 manifest.json 을 썼습니다.`);
    return 0;
}

if (require.main === module) process.exitCode = main(process.argv.slice(2));

module.exports = { ASSET_FILES, BUNDLES, MANIFEST_FORMAT, SOURCE_ROOT, DATA_ROOT, MANIFEST_PATH, buildManifest, checkAssets, prepareAssets, sha256 };
