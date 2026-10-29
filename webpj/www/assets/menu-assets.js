// 메인 메뉴 화면에 필요한 원본 자산의 경로와, 내려받은 파일 바이트를 그리기 자원 묶음으로 바꾸는 모듈이다.
// 경로·선택 근거(원본 setup.cfg 값과 실행 파일 전역):
//  - 팔레트: fortPal = "gifcloud", GamePalSpec = "{DataDir}\{local.1}.COL" → d/GIFCLOUD.COL
//  - 구름: CloudFileSpec = "{DataDir}\gifcloud.gif" → d/Gifcloud.gif (원본 00470bb0 은 512×512 일 때만 사용)
//  - 타이틀: backgroundGifSpec = "{DataDir}\{local.1}.gif" + "titleMenu" → d/titleMenu.gif
//  - 돌 버튼 질감: 타입 번호 92(DAT_005411c8, fortGump = 70 + 블록 22)의 프레임 1(DAT_00542664 = 1, 클러스터 A01 "Sandy marble")
//  - 글꼴: 슬롯 0(Arial 굵게 14) 캐시, 커서: 기본 커서 번호 1, 효과음: button.wav, 음악: ser22.mus(전투 밖 곡)
// (2026-10-23 WB03-M1: 메인 메뉴 화면 복원)
import { GamePalette, buildButtonPalette, buildScreenPalette, buildShadeMaps } from './palette.js';
import { decodeGif } from './gif.js';
import { ShapeDatabase } from './shapes.js';
import { BitmapFont, DEFAULT_FONT_PATH } from './bitmap-font.js';
import { FontSet } from './font-set.js';
import { buttonTextContexts } from '../render/gump-visual.js';
import { StyleColors } from '../ui/style-text.js';
import { DEFAULT_CURSOR_PATH, decodeCursorResource } from '../ui/cursor.js';

// 메인 메뉴 화면 표시 전에 반드시 받아야 하는 자산 묶음 이름이다(매니페스트 bundle 값). (2026-10-23 WB03-M1)
export const MENU_BUNDLE = 'menu';

// 메인 메뉴 음악 묶음 이름이다. 크기가 커서(약 18MB) 화면 표시를 막지 않고 뒤에서 받는다. (2026-10-23 WB03-M1)
export const MENU_MUSIC_BUNDLE = 'menu-music';

// 메인 메뉴 자산의 데이터 폴더 기준 경로이다. 원본 파일 이름의 대소문자를 그대로 쓴다(대소문자를 구분하는 서버 대비). (2026-10-23 WB03-M1)
export const MENU_ASSET_PATHS = Object.freeze({
    palette: 'd/GIFCLOUD.COL',
    clouds: 'd/Gifcloud.gif',
    title: 'd/titleMenu.gif',
    shapes: 'd/_shapes.shp',
    font: DEFAULT_FONT_PATH,
    cursor: DEFAULT_CURSOR_PATH,
    buttonSound: 'sound/button.wav',
    music: 'music/ser22.mus',
});

// 효과음·음악을 재생기에 등록할 때 쓰는 원본 이름이다. (2026-10-23 WB03-M1)
export const BUTTON_SOUND_NAME = 'button.wav';
export const MENU_MUSIC_NAME = 'ser22.mus';

// _shapes.shp 에서 fortGump 타입의 블록 번호(원본 타입 로딩 순서 23번째, 0부터 22)이다. (2026-10-23 WB03-M1)
export const FORT_GUMP_BLOCK = 22;

// 돌 버튼 배경 자식이 쓰는 fortGump 프레임 번호이다. 원본 전역 DAT_00542664 의 정적 초기값 1(쓰기 없음)이며,
// 원본 메인 메뉴 스크린샷의 버튼 질감 위상 대조에서도 프레임 1(A01)이 일치했다. (cpppj 는 A00=프레임 0 을 쓴다.) (2026-10-23 WB03-M1)
export const STONE_BUTTON_TEXTURE_FRAME = 1;

// 원본 구름 파일이 받아들여지는 크기(가로·세로 0x200)이다. 다르면 원본은 구름 그림을 쓰지 않는다. (2026-10-23 WB03-M1)
export const CLOUD_IMAGE_SIZE = 512;

// 대화상자 패널 배경 질감의 fortGump 프레임 번호이다(클러스터 A00 "Marble pattern texture", 원본 장식 배경 전역 backgroundFrame = 0). (2026-10-28 WB03-D)
export const PANEL_TEXTURE_FRAME = 0;

// 대화상자 패널 네 모서리 금장식의 fortGump 프레임 번호이다(클러스터 I00~I03: 왼쪽 위·오른쪽 위·오른쪽 아래·왼쪽 아래, 타입 정의 순서 54~57). (2026-10-28 WB03-D)
export const PANEL_CORNER_FRAMES = Object.freeze([54, 55, 56, 57]);

// 목록 항목의 체크 표시 fortGump 프레임 번호이다(클러스터 J02, 타입 정의 순서 60: J00 = 58 + 2). (2026-10-28 WB03-D)
export const CHECK_MARKER_FRAME = 60;

// 목록 행을 눌러 명령을 실행할 때 내는 효과음의 원본 이름과 데이터 폴더 경로이다(원본 MenuGumpInput 의 Sound("openSubGump.wav")). (2026-10-28 WB03-D)
export const MENU_SOUND_NAME = 'openSubGump.wav';
export const MENU_SOUND_PATH = 'sound/openSubGump.wav';

// 본문 속 그림 명령 `~[I이름.프레임]` 을 해석하는 함수를 만든다. 반환 함수 (이름, 프레임식) → { image, width, height, hotspotX, hotspotY } 또는 null 이다.
// 원본은 타입 이름으로 타입 표를 찾고 프레임 식(숫자·글자 + 숫자·`*`·`#`)을 타입별 프레임 표로 해석하지만, 타입 표(.type)가 아직 없어
// 지금은 fortGump(저장 에너지 아이콘 등)의 숫자 프레임만 지원한다. 그 밖의 이름·글자 프레임은 null 이다(호출자가 그리지 않고 알림).
// 크기·기준점은 Squid 추가 헤더(출력 크기·기준점)이며 그림은 기준점 기준 상대 위치(image.left/top)로 그린다. (2026-10-30 WB03-G)
function createPictureResolver(shapes) {
    return (name, expression) => {
        if (String(name).toLowerCase() !== 'fortgump' || !/^[0-9]+$/.test(expression)) return null;
        try {
            const frame = Number(expression);
            const metrics = shapes.squidMetrics(FORT_GUMP_BLOCK, frame);
            return Object.freeze({ image: shapes.decode(FORT_GUMP_BLOCK, frame), width: metrics.width, height: metrics.height, hotspotX: metrics.hotspotX, hotspotY: metrics.hotspotY });
        } catch (error) {
            // 범위 밖 프레임이나 그림 없는 레코드는 지원하지 않는 그림으로 본다.
            return null;
        }
    };
}

// 경로로 파일 바이트를 꺼낸다. 없으면 어떤 파일이 빠졌는지 알리는 오류를 던진다. (2026-10-23 WB03-M1)
function requireFile(files, path) {
    const bytes = files.get(path);
    if (!bytes) throw new Error(`필수 자산이 없습니다: ${path}`);
    return bytes;
}

// 내려받은 파일 Map(경로 → Uint8Array)으로 메인 메뉴 그리기 자원 묶음을 만든다.
// palette 는 원본 화면 팔레트 규칙(0번 검정·255번 흰색 고정)을 적용한 화면 팔레트다.
// 반환: { palette, shadeMaps, buttonPalette, textContexts, font, clouds(512×512 아니면 null), title, texture, textureMetrics, cursor }
// 사용: new MainMenuScreen({ resources: buildMenuResources(files) }) (2026-10-23 WB03-M1)
export function buildMenuResources(files) {
    const palette = buildScreenPalette(GamePalette.parse(requireFile(files, MENU_ASSET_PATHS.palette)));
    const shapes = new ShapeDatabase(requireFile(files, MENU_ASSET_PATHS.shapes));
    const clouds = decodeGif(requireFile(files, MENU_ASSET_PATHS.clouds));
    const buttonPalette = buildButtonPalette(palette.logical);
    return Object.freeze({
        palette,
        shadeMaps: buildShadeMaps(palette.logical),
        buttonPalette,
        textContexts: buttonTextContexts(buttonPalette),
        font: BitmapFont.parse(requireFile(files, MENU_ASSET_PATHS.font)),
        // 대화상자 글자 그리기용 글꼴 묶음(슬롯·스타일별 지연 해독)과 글자색 번호 표이다.
        fonts: new FontSet(files),
        colors: new StyleColors(palette.logical),
        // 대화상자 패널(질감 프레임 0 + 네 모서리 금장식)이다. 목록 상자·돌 버튼은 위의 texture(프레임 1)를 쓴다.
        panel: Object.freeze({
            texture: shapes.decode(FORT_GUMP_BLOCK, PANEL_TEXTURE_FRAME),
            textureMetrics: shapes.squidMetrics(FORT_GUMP_BLOCK, PANEL_TEXTURE_FRAME),
            corners: Object.freeze(PANEL_CORNER_FRAMES.map((frame) => shapes.decode(FORT_GUMP_BLOCK, frame))),
        }),
        marker: shapes.decode(FORT_GUMP_BLOCK, CHECK_MARKER_FRAME),
        // 본문 그림 명령(~[I…]) 해석기이다(2026-10-30 WB03-G: 도움말 본문의 저장 에너지 아이콘).
        resolvePicture: createPictureResolver(shapes),
        clouds: clouds.width === CLOUD_IMAGE_SIZE && clouds.height === CLOUD_IMAGE_SIZE ? clouds : null,
        title: decodeGif(requireFile(files, MENU_ASSET_PATHS.title)),
        texture: shapes.decode(FORT_GUMP_BLOCK, STONE_BUTTON_TEXTURE_FRAME),
        textureMetrics: shapes.squidMetrics(FORT_GUMP_BLOCK, STONE_BUTTON_TEXTURE_FRAME),
        cursor: decodeCursorResource(requireFile(files, MENU_ASSET_PATHS.cursor)),
    });
}
