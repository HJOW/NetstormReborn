// 시작 title 그림 세 장의 표시 시간표와 화면 픽셀 변환이다(원문 Netstorm.exe 0x4ab3e0 `runTitleBitmaps`: title0.gif → title6.gif → title7.gif 를 3초씩 보이고, 입력이 있으면 남은 그림을 건너뛴 뒤 메인 메뉴로 간다).
// 사용: titleScreenIndexAt(경과 밀리초) → 0·1·2 이면 그 그림, 3 이면 끝(메인 메뉴로); titleScreenRgba(screen) → 640×480 RGBA 바이트(캔버스 ImageData 에 넣는다).
//   screen = { image: decodeGif 결과({ width, height, indices }), palette: buildScreenPalette 결과({ rgb }) } (assets/menu-assets.js buildMenuResources 의 titleScreens 한 칸).
// 원본은 그림을 검은 화면 가운데에 놓으므로 가운데 좌표는 titleScreenOrigin 이 계산한다. 웹은 DOM 을 여기서 쓰지 않는다(앱 netstorm.js 가 캔버스에 그린다).
// 원본과 같은 점: 그림 3장×3초, 입력 때 남은 그림 전체를 건너뜀, 팔레트는 title0 → title00.col·title6/7 → title06.col. 다른 점: 원문의 첫 3초 기한은 title6/7 적재 시간을 포함하지만 웹은 자산이 이미 적재돼 있어 표시 시간만 센다.
// (2026-11-22 WB04-E1184: 시작 title 그림 표시로 추가)

// 그림 한 장이 보이는 시간(밀리초)이다(원문 4ab3e0 의 3.0초·waitTitleBitmap(3000)).
export const TITLE_SCREEN_MILLISECONDS = 3000;
// 보여 줄 그림 수이다(title0·title6·title7).
export const TITLE_SCREEN_COUNT = 3;

// 경과 시간으로 지금 보일 그림 번호를 돌려준다. 0~2 는 그림 번호, TITLE_SCREEN_COUNT(3)는 끝이다. 음수·비유한 값은 첫 그림이다. 사용: titleScreenIndexAt(4500) → 1. (2026-11-22 WB04-E1184)
export function titleScreenIndexAt(elapsedMilliseconds) {
    if (!Number.isFinite(elapsedMilliseconds) || elapsedMilliseconds < 0) return 0;
    return Math.min(TITLE_SCREEN_COUNT, Math.floor(elapsedMilliseconds / TITLE_SCREEN_MILLISECONDS));
}

// 화면(너비·높이) 가운데에 그림(너비·높이)을 놓는 왼쪽 위 좌표이다(원문은 가운데 정렬 복사, 그림이 화면보다 크면 음수가 될 수 있다). 사용: titleScreenOrigin(1024, 768, 640, 480) → { x: 192, y: 144 }. (2026-11-22 WB04-E1184)
export function titleScreenOrigin(screenWidth, screenHeight, imageWidth, imageHeight) {
    return { x: Math.trunc((screenWidth - imageWidth) / 2), y: Math.trunc((screenHeight - imageHeight) / 2) };
}

// 색 번호 그림과 팔레트로 RGBA 바이트(불투명)를 만든다. 사용: const pixels = titleScreenRgba(screen); new ImageData(pixels, width, height). 팔레트가 256색(768바이트)이 아니거나 그림 크기가 어긋나면 거부한다. (2026-11-22 WB04-E1184)
export function titleScreenRgba(screen) {
    const { image, palette } = screen ?? {};
    if (!image?.indices || !palette?.rgb || palette.rgb.length !== 768) throw new TypeError('시작 title 그림의 색 번호와 256색 팔레트 필요');
    if (image.indices.length !== image.width * image.height) throw new RangeError('시작 title 그림 크기와 색 번호 수 불일치');
    const pixels = new Uint8ClampedArray(image.width * image.height * 4);
    // 각 픽셀의 색 번호를 팔레트 RGB 로 바꾸고 알파는 255 로 둔다.
    for (let index = 0; index < image.indices.length; index += 1) {
        const color = image.indices[index] * 3, target = index * 4;
        pixels[target] = palette.rgb[color]; pixels[target + 1] = palette.rgb[color + 1]; pixels[target + 2] = palette.rgb[color + 2]; pixels[target + 3] = 255;
    }
    return pixels;
}
