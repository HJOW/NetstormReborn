// 한국어 지원 자산(D2Coding 글꼴 + UI 번역표)을 내려받은 파일 Map 에서 그리기 자원으로 바꾸는 모듈이다.
//  - 글꼴: 프로젝트 fonts/ 의 D2Coding woff2(보통체·굵은체)를 webpj/www/data/fonts/ 로 복사한 것이다. FontFace 가 woff2 를 바로 읽으므로 변환하지 않는다.
//    래스터라이저(브라우저는 Canvas 2D)에 두 파일을 넘기고, 글꼴 슬롯·스타일마다 글리프를 만드는 UnicodeFontProvider 를 돌려준다.
//  - 번역: d/xlat.korean(원본 xlat 형식, UTF-8)을 XlatTable 로 읽어 translate(원문) 함수를 돌려준다.
// 래스터라이저 만들기는 브라우저(FontFace·Canvas)에 의존하므로 호출자가 createRasterizer 로 넘긴다. 검사에서는 가짜 래스터라이저를 넘긴다.
// (2026-10-31 WB03-N: 한국어 메인 메뉴·펼침 메뉴 라벨을 위해 추가)
import { decodeOriginalText } from './text-encoding.js';
import { UnicodeFontProvider } from './unicode-font.js';
import { XlatTable } from './xlat.js';

// 한국어 자산 묶음 이름이다(매니페스트 bundle 값). 한국어일 때만 내려받는다. (2026-10-31 WB03-N)
export const KOREAN_BUNDLE = 'korean';

// 한국어 자산의 데이터 폴더 기준 경로이다. 글꼴 파일 이름은 프로젝트 fonts/ 의 사용자 제공 woff2 와 같다(2026-10-31 사용자 지시: woff2 를 webpj 안으로 복사해 사용).
// license 는 글꼴 사용 허가 문서로 글꼴과 함께 배포한다. (2026-10-31 WB03-N)
export const KOREAN_ASSET_PATHS = Object.freeze({
    regular: 'fonts/D2Coding.woff2',
    bold: 'fonts/D2CodingBold.woff2',
    license: 'fonts/License.txt',
    translations: 'd/xlat.korean',
});

// 내려받은 파일 Map 으로 한국어 지원을 만든다. createRasterizer({ regular, bold }) 는 보통체·굵은체 woff2 바이트(Uint8Array)로 래스터라이저를 만드는 함수(동기 또는 Promise)이다.
// 반환: { translate(원문) → 번역문, unicode(UnicodeFontProvider), translations(XlatTable) }. 필요한 파일이 없으면 오류를 던진다.
// 사용: const korean = await buildKoreanSupport(files, { createRasterizer }); buildMenuResources(menuFiles, korean). (2026-10-31 WB03-N)
export async function buildKoreanSupport(files, { createRasterizer }) {
    const need = (path) => {
        const bytes = files.get(path);
        if (!bytes) throw new Error(`필수 자산이 없습니다: ${path}`);
        return bytes;
    };
    const translations = new XlatTable(decodeOriginalText(need(KOREAN_ASSET_PATHS.translations)));
    const rasterizer = await createRasterizer({ regular: need(KOREAN_ASSET_PATHS.regular), bold: need(KOREAN_ASSET_PATHS.bold) });
    return Object.freeze({
        translate: (text) => translations.translate(text),
        unicode: new UnicodeFontProvider(rasterizer),
        translations,
    });
}
