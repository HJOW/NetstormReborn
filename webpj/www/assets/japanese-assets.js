// 공용 일본어 UI 번역과 언어 파일을 메뉴/게임 데이터가 받는 지원 객체로 만든다.
// 실제 글꼴 준비는 브라우저의 createRasterizer가 맡는다. 이 모듈은 DOM과 외부 접속을 사용하지 않는다.
import { decodeOriginalText } from './text-encoding.js';
import { UnicodeFontProvider } from './unicode-font.js';
import { XlatTable } from './xlat.js';
import { JAPANESE_TRANSLATIONS_PATH, JAPANESE_FONT_METRICS } from './japanese-asset-config.js';

// 일본어 느슨한 파일만 게임 데이터에 합친다. 영어 대체는 기존 GameData 규칙을 그대로 쓴다.
const LANGUAGE_FILE_PATTERN = /^d\/[^/]+\.japanese$/iu;

// 내려받은 파일 Map과 비동기 글꼴 공급자로 일본어 지원을 만든다.
// 사용: buildJapaneseSupport(files, { createRasterizer: CDN글꼴준비함수 }). 번역표/글꼴 실패는 호출자에게 전달한다.
export async function buildJapaneseSupport(files, { createRasterizer }) {
    // UI 번역표는 필수이며 개별 언어 파일이 없으면 GameData가 영어 파일로 대체한다.
    const bytes = files.get(JAPANESE_TRANSLATIONS_PATH);
    if (!bytes) throw new Error(`필수 자산이 없습니다: ${JAPANESE_TRANSLATIONS_PATH}`);
    if (typeof createRasterizer !== 'function') throw new Error('일본어 글꼴 공급자가 없습니다.');
    // 실제 브라우저 글꼴이 준비된 뒤에만 유니코드 그리기 자원을 공개한다.
    const translations = new XlatTable(decodeOriginalText(bytes));
    const rasterizer = await createRasterizer();
    return Object.freeze({
        translate: (text) => translations.translate(text),
        unicode: new UnicodeFontProvider(rasterizer, JAPANESE_FONT_METRICS),
        translations,
        languageFiles: new Map([...files].filter(([assetPath]) => LANGUAGE_FILE_PATTERN.test(assetPath))),
    });
}
