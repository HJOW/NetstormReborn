// 공용 일본어43개 언어 파일의 공개 데이터 경로다. 준비 도구와 런타임이 같은 목록을 사용한다.
export const JAPANESE_LANGUAGE_PATHS = Object.freeze([
    "d/bc1menu76.japanese",
    "d/bc2bridging76.japanese",
    "d/bc3acid_bar76.japanese",
    "d/bc4bow_rush76.japanese",
    "d/bc5walkers76.japanese",
    "d/bc6mows76.japanese",
    "d/bc7sun_bars76.japanese",
    "d/bc8fury76.japanese",
    "d/bc9ssun_bars76.japanese",
    "d/breakingthrough.japanese",
    "d/config.japanese",
    "d/demostormrages.japanese",
    "d/dissolvedalliance.japanese",
    "d/enemyterritory.japanese",
    "d/finalconfrontation.japanese",
    "d/fragilefortune.japanese",
    "d/guardmyback.japanese",
    "d/help.japanese",
    "d/masterofwhirligigs.japanese",
    "d/offical1.japanese",
    "d/offical2.japanese",
    "d/offical2sep.japanese",
    "d/offical3.japanese",
    "d/offical4.japanese",
    "d/offical5.japanese",
    "d/offical5sep.japanese",
    "d/offical6.japanese",
    "d/rainvsrain.japanese",
    "d/runforit.japanese",
    "d/savetheisland.japanese",
    "d/surrounded.japanese",
    "d/tell.japanese",
    "d/thenoose.japanese",
    "d/thewarbegins.japanese",
    "d/thunderingpower.japanese",
    "d/totherescue.japanese",
    "d/tutorial1.japanese",
    "d/tutorial2.japanese",
    "d/tutorial3.japanese",
    "d/tutorial4.japanese",
    "d/tutorial5.japanese",
    "d/tutorial6.japanese",
    "d/vicious.japanese"
]);
// 언어 선택 때만 적재할 일본어 묶음과 번역표 경로다. 글꼴은 별도 브라우저 제공자가 준비한다.
export const JAPANESE_BUNDLE = 'japanese';
export const JAPANESE_TRANSLATIONS_PATH = 'd/xlat.japanese';

// 일본어는 실제CDN Noto Sans JP를 사용한다. TTF를 배포하거나 변환하지 않는다.
export const JAPANESE_FONT_FAMILY = 'Noto Sans JP';
export const JAPANESE_FONT_STYLESHEET = 'https://fonts.googleapis.com/css2?family=Noto+Sans+JP:wght@400;700&display=swap';
// 일본어 벡터 글꼴은 슬롯 높이로 그리며 같은 줄의 영문도 확장 글자의 기준선에 맞춘다.
export const JAPANESE_FONT_METRICS = Object.freeze({
    // 원본 슬롯14/13/20/48/12px 크기를 그대로 사용한다.
    emSize: (height) => height,
    // 한자의 위쪽 획이 잘리지 않도록 기준선 위 공간을 확보한다.
    ascentRows: (size) => Math.ceil(size * 0.88),
    alignNative: true,
    // 일본어 문장은 공백 없는 문자 경계에서도 줄을 바꾼다.
    lineBreaks: 'japanese',
});
