// 원본처럼 8비트 색 번호로 화면을 합성하는 표면 모듈이다. 모든 그림은 색 번호로 그리고,
// 표시 직전에 팔레트로 RGB 로 바꾼다(render/presenter.js). 원본 DIB 화면(8비트)과 같은 합성 결과를 얻기 위해서다.
// 사각형은 { left, top, right, bottom } 이며 right/bottom 은 포함하지 않는다(원본 ScreenRect 와 같음).
// 복원 기준: cpppj Renderer.cpp(DrawIndexedImage)·UberGump.cpp(Tile)·GumpVisual.cpp(DrawGumpLines).
// (2026-10-23 WB03-M1: 메인 메뉴 화면 합성을 위해 추가)

// 사각형의 네 좌표를 각각 bounds 안으로 제한한다(원본 ClipScreenRect). 결과가 비어 있을 수도 있다.
// 사용: 그림 출력 전에 클립 영역을 화면 경계와 겹칠 때 쓴다. (2026-10-23 WB03-M1)
export function clipRect(rect, bounds) {
    const clamp = (value, low, high) => Math.max(low, Math.min(value, high));
    return {
        left: clamp(rect.left, bounds.left, bounds.right),
        top: clamp(rect.top, bounds.top, bounds.bottom),
        right: clamp(rect.right, bounds.left, bounds.right),
        bottom: clamp(rect.bottom, bounds.top, bounds.bottom),
    };
}

// 8비트 색 번호 화면 버퍼이다. pixels[y * width + x] 가 한 픽셀의 색 번호다.
// 사용: new IndexedSurface(1024, 768) 로 만들고 tile·drawImage·drawLines 로 합성한 뒤 표시기에 넘긴다. (2026-10-23 WB03-M1)
export class IndexedSurface {
    // 가로·세로 크기(정수 픽셀)로 표면을 만든다. 처음에는 모두 색 번호 0 이다.
    constructor(width, height) {
        if (!Number.isInteger(width) || !Number.isInteger(height) || width <= 0 || height <= 0) {
            throw new RangeError(`IndexedSurface: 잘못된 크기입니다: ${width}x${height}`);
        }
        // 가로 크기(픽셀)다.
        this.width = width;
        // 세로 크기(픽셀)다.
        this.height = height;
        // 색 번호 버퍼(가로×세로)다.
        this.pixels = new Uint8Array(width * height);
    }

    // 표면 전체를 나타내는 사각형이다.
    get bounds() {
        return { left: 0, top: 0, right: this.width, bottom: this.height };
    }

    // 표면 전체를 한 색 번호로 채운다.
    fill(color) {
        this.pixels.fill(color);
    }

    // 색 번호 그림을 (left, top) 에 그린다(원본 DrawIndexedImage). 투명 픽셀은 기존 색을 유지한다.
    // clip 은 출력 제한 사각형(생략 시 표면 전체), colorMap 이 있으면 그림의 색 번호를 colorMap[번호] 로 바꿔 쓴다.
    // image 는 { width, height, indices, opacity } 이다.
    drawImage(image, left, top, clip = this.bounds, colorMap = null) {
        const area = clipRect(clip, this.bounds);
        const startY = Math.max(top, area.top);
        const endY = Math.min(top + image.height, area.bottom);
        const startX = Math.max(left, area.left);
        const endX = Math.min(left + image.width, area.right);
        const pixels = this.pixels;
        const { indices, opacity } = image;
        // 클립 안에 들어오는 행만 처리한다.
        for (let y = startY; y < endY; y += 1) {
            const input = (y - top) * image.width - left;
            const output = y * this.width;
            // 불투명 픽셀만 써서 투명 영역의 기존 화면 값을 남긴다.
            for (let x = startX; x < endX; x += 1) {
                if (opacity[input + x]) pixels[output + x] = colorMap ? colorMap[indices[input + x]] : indices[input + x];
            }
        }
    }

    // 그림을 rect 의 왼쪽 위부터 원본 크기 그대로 반복해 채운다(cpppj UberGump 의 Tile). 마지막 타일은 rect 에서 자른다.
    // 사용: 메인 메뉴의 구름 배경(gifcloud.gif)을 화면 (0,0) 기준으로 깔 때 쓴다.
    tile(image, rect = this.bounds) {
        // 영역 위에서 아래로 그림 높이만큼씩 내려간다.
        for (let y = rect.top; y < rect.bottom; y += image.height) {
            // 한 줄에서 왼쪽에서 오른쪽으로 그림 폭만큼씩 이동한다.
            for (let x = rect.left; x < rect.right; x += image.width) this.drawImage(image, x, y, rect);
        }
    }

    // 수평·수직 선 목록을 그린다(원본 VFX 선, 양 끝 포함). 각 선은 { x1, y1, x2, y2, color } 이다.
    // 원본 호출 순서대로 그려 겹치는 픽셀은 뒤 선이 덮는다. 대각선은 원본 UI 에 없으므로 오류다.
    drawLines(lines) {
        // 선을 주어진 순서대로 그린다.
        for (const line of lines) {
            if (line.x1 !== line.x2 && line.y1 !== line.y2) throw new Error('GUI 선이 수평/수직이 아닙니다.');
            const left = Math.max(0, Math.min(line.x1, line.x2));
            const top = Math.max(0, Math.min(line.y1, line.y2));
            const right = Math.min(this.width, Math.max(line.x1, line.x2) + 1);
            const bottom = Math.min(this.height, Math.max(line.y1, line.y2) + 1);
            if (left >= right || top >= bottom) continue;
            // 선이 덮는 각 행을 색 번호로 채운다.
            for (let y = top; y < bottom; y += 1) this.pixels.fill(line.color & 255, y * this.width + left, y * this.width + right);
        }
    }
}
