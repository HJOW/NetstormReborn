// 원본 비트맵 글꼴 캐시(.chfnt)를 읽는 모듈이다. 원본 Screen.cpp(004a3240·004a3430·004a39f0)의 글꼴 표를 복원한다.
// 파일은 16바이트 서명, 머리 정수(높이·ascent·descent), 256항목 표 네 개(측정 폭·ABC A·그리기 전진 폭·글리프 블록 크기)와
// 글자별 독립 VFX 셰이프 블록으로 되어 있다. 글리프 픽셀은 색 번호 100(글자색 자리)만 쓴다.
// 복원 기준: cpppj BitmapFont.cpp. (2026-10-23 WB03-M1: 메인 메뉴 버튼 라벨을 원본 글꼴로 그리기 위해 추가)
import { ShapeDatabase } from './shapes.js';

// .chfnt 파일 서명("BitmapFontData" + 0x1A + 0x00)이다. (2026-10-23 WB03-M1)
const FONT_SIGNATURE = 'BitmapFontData\x1a\0';

// 네 개의 256항목 표가 시작하는 위치(바이트)이다. (2026-10-23 WB03-M1)
const TABLE_START = 36;

// 256항목 표 하나의 크기(바이트)이다. (2026-10-23 WB03-M1)
const TABLE_BYTES = 1024;

// 글리프 셰이프 블록들이 시작하는 위치(바이트)이다. (2026-10-23 WB03-M1)
const PAYLOAD_START = 4132;

// 원본 글꼴 슬롯별 CreateFontA 요청 높이(논리 높이)이다. 슬롯 2는 원본에서도 비어 있다.
// 사용: 원본 글자 측정(MeasureGumpText)은 캐시의 tmHeight 가 아니라 이 값을 글자 높이로 쓴다. 슬롯 0 = 14. (2026-10-23 WB03-M1)
export const FONT_SLOT_HEIGHTS = Object.freeze([14, 13, 0, 20, 48, 14, 12]);

// 원본 슬롯 0(기본 UI 글꼴: Arial 굵게 14)의 캐시 파일 경로(데이터 폴더 기준)이다. 돌 버튼 라벨이 쓴다. (2026-10-23 WB03-M1)
export const DEFAULT_FONT_PATH = 'd/!Arial.normal.14.700.chfnt';

// 원본 비트맵 글꼴 한 벌이다. glyphs[바이트코드] 는 { advance, bearing, drawAdvance, present, image } 이다.
//  - advance: 측정 폭(첫 표). 글자 폭 측정에만 쓴다.
//  - drawAdvance: 그리기 전진 폭(셋째 표). 글리프를 그린 뒤 다음 글자 위치로 옮길 때 쓴다.
//  - present: 글리프 블록이 있는지. 없으면 그리기에서 건너뛰고 전진하지 않는다.
//  - image: 그림이 있는 글리프의 해독 그림(공백 등 특수 프레임은 null).
// 사용: BitmapFont.parse(bytes) 로 만들고 measure·drawGlyphText 에 넘긴다. (2026-10-23 WB03-M1)
export class BitmapFont {
    // parse 가 읽은 값으로 글꼴을 만든다. 직접 부르지 말고 BitmapFont.parse 를 쓴다.
    constructor(height, ascent, descent, glyphs) {
        // 캐시에 저장된 전체 글자 높이(tmHeight)다.
        this.height = height;
        // 기준선 위쪽 높이다.
        this.ascent = ascent;
        // 기준선 아래쪽 높이다.
        this.descent = descent;
        // 바이트 코드 0~255 의 글리프 표다.
        this.glyphs = glyphs;
    }

    // .chfnt 파일 바이트를 읽는다. 서명·머리·표·글리프 블록이 맞지 않으면 오류를 던진다.
    static parse(bytes) {
        const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
        const signature = String.fromCharCode(...bytes.subarray(0, FONT_SIGNATURE.length));
        if (bytes.length < PAYLOAD_START || signature !== FONT_SIGNATURE || view.getUint32(16, true) !== 1 || view.getUint32(32, true) !== 256) {
            throw new Error('비트맵 글꼴 머리가 잘못됐습니다.');
        }
        const height = view.getInt32(20, true);
        const ascent = view.getInt32(24, true);
        const descent = view.getInt32(28, true);
        if (height <= 0 || height > 512 || ascent < 0 || descent < 0 || ascent > height || descent > height) {
            throw new Error('비트맵 글꼴 치수가 잘못됐습니다.');
        }
        const glyphs = [];
        let position = PAYLOAD_START;
        // 바이트 코드 0~255 의 표 값과 독립 VFX 블록을 차례로 읽는다.
        for (let code = 0; code < 256; code += 1) {
            const advance = view.getInt32(TABLE_START + code * 4, true);
            const bearing = view.getInt32(TABLE_START + TABLE_BYTES + code * 4, true);
            const drawAdvance = view.getInt32(TABLE_START + TABLE_BYTES * 2 + code * 4, true);
            const size = view.getUint32(TABLE_START + TABLE_BYTES * 3 + code * 4, true);
            if (size > bytes.length - position) throw new Error('비트맵 글꼴 글리프가 잘렸습니다.');
            let image = null;
            if (size !== 0) {
                const shape = new ShapeDatabase(bytes.subarray(position, position + size));
                if (shape.blocks.length !== 1 || shape.blocks[0].frames.length !== 1) throw new Error('비트맵 글꼴 글리프 블록이 잘못됐습니다.');
                if (!shape.frame(0, 0).special) image = shape.decode(0, 0);
            }
            glyphs.push(Object.freeze({ advance, bearing, drawAdvance, present: size !== 0, image }));
            position += size;
        }
        if (position !== bytes.length) throw new Error('비트맵 글꼴 뒤에 남는 바이트가 있습니다.');
        return new BitmapFont(height, ascent, descent, Object.freeze(glyphs));
    }

    // 글자 코드의 글리프를 돌려준다. 글자 그리기(render/glyph-text.js)가 글꼴 종류와 관계없이 같은 방식으로 글리프를 얻도록 하는 접근 함수이다.
    // 사용: font.glyph(65) → { advance, bearing, drawAdvance, present, image }. (2026-10-31 WB03-N: 유니코드 글꼴 UnicodeGlyphFont 와 같은 인터페이스를 위해 추가)
    glyph(code) {
        return this.glyphs[code];
    }

    // Windows-1252 바이트 배열의 측정 폭(첫 표의 합)을 돌려준다(원본 004a3df0). 원본처럼 32비트 감김을 유지한다.
    // 사용: 돌 버튼 크기 맞춤(글자 폭 + 12) 등 원본 측정 경로에서 쓴다.
    measure(bytes) {
        let width = 0;
        // 각 바이트의 측정 폭을 더한다.
        for (const code of bytes) width = (width + this.glyphs[code].advance) | 0;
        return width;
    }
}
