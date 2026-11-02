// 정적 지형을 눈으로 점검하는 개발용 화면이다. 미션/게임 규칙/저장 진행을 실행하지 않는다.
// 카메라 초기 위치·제한·키/드래그는 이 뷰의 점검 정책이며 원본 미션 입력 복원이 아니다. (2026-11-02 WB03-AA)
import { CameraView } from '../render/camera-view.js';
import { WorldSpriteRenderer } from '../render/world-sprites.js';
import { createOwnerColorTables } from '../assets/owner-colors.js';
import { drawGlyphText, createTextContext } from '../render/glyph-text.js';

// 아래 안내 띠의 높이와 카메라 키 이동량(칸4개)의 픽셀 값이다. 개발용 입력에만 사용한다.
const LABEL_HEIGHT = 28;
const KEY_PAN_X = 64;
const KEY_PAN_Y = 44;

// 장면의 원본 그림 범위를 카메라0에서 합친다. 받침 바위의 아래쪽도 카메라 점검 범위에 포함한다.
export function terrainPixelBounds(scene) {
    const camera = new CameraView();
    camera.initialize();
    const renderer = new WorldSpriteRenderer(scene);
    const plan = renderer.plan(camera, scene.entries, { sort: false });
    let bounds = null;
    // 실제 압축 픽셀 영역만 합치며 그림 없는 레코드는 무시한다.
    for (const sprite of plan) {
        const frame = scene.shapes.frame(sprite.block, sprite.frames[0]);
        if (frame.special) continue;
        const box = { left: sprite.screenX + frame.rect.left, top: sprite.screenY + frame.rect.top,
            right: sprite.screenX + frame.rect.right + 1, bottom: sprite.screenY + frame.rect.bottom + 1 };
        bounds = bounds ? { left: Math.min(bounds.left, box.left), top: Math.min(bounds.top, box.top),
            right: Math.max(bounds.right, box.right), bottom: Math.max(bounds.bottom, box.bottom) } : box;
    }
    return bounds ?? { left: 0, top: 0, right: 1, bottom: 1 };
}

// 한 맵의 계산 자료·렌더러·점검 카메라를 소유한다. 앱의 destroy/메뉴 복귀에서 참조를 놓으면 정리된다.
export class TerrainPreview {
    // resources는 기존 메뉴 자원(팔레트·구름·한국어/영어 글꼴)이다. 미리보기용 추가 자산은 없다.
    constructor(scene, resources, language = 'english') {
        // 복원된 지면·절벽·받침과 원본 자산이다.
        this.scene = scene;
        // 기존 팔레트·구름·글꼴과 화면 언어이다.
        this.resources = resources;
        this.language = language;
        // 이 뷰에서만 사용하는 카메라와 실제 그림 범위이다.
        this.camera = new CameraView();
        this.camera.initialize();
        this.bounds = terrainPixelBounds(scene);
        // 호출자가 제공한 소유자 색 번호에만 원본 색표를 적용한다. 기본 장면은 중립이다.
        this.renderer = new WorldSpriteRenderer({ ...scene, ownerColors: createOwnerColorTables(resources.palette.logical, { useBanners: 0 }) });
        // 논리 화면 크기와 다시 그리기 필요 상태이다.
        this.width = 0;
        this.height = 0;
        this.dirty = true;
        // 끌기 시작의 화면 좌표·카메라 원점이다. null이면 끌지 않는다.
        this.drag = null;
    }

    // 처음에는 실제 지형 그림의 가운데로, 이후 크기 변경에서는 같은 월드 중심으로 맞춘다.
    resize(width, height) {
        if (width === this.width && height === this.height) return;
        const pixels = this.camera.pixels;
        const centerX = this.width ? pixels.x + this.width / 2 : (this.bounds.left + this.bounds.right) / 2;
        const centerY = this.height ? pixels.y + (this.height - LABEL_HEIGHT) / 2 : (this.bounds.top + this.bounds.bottom) / 2;
        this.width = width;
        this.height = height;
        this.cancel();
        this.moveTo(centerX - width / 2, centerY - (height - LABEL_HEIGHT) / 2);
        this.dirty = true;
    }

    // 그림 범위 주변 화면 한 장까지 허용하는 개발용 카메라 제한이다. 원본 맵 경계/목표 이동과 구분한다.
    moveTo(x, y) {
        const maxX = Math.max(0, this.bounds.right - 1);
        const maxY = Math.max(0, this.bounds.bottom - 1);
        const nextX = Math.trunc(Math.max(0, Math.min(maxX, x)));
        const nextY = Math.trunc(Math.max(0, Math.min(maxY, y)));
        const old = this.camera.pixels;
        if (nextX === old.x && nextY === old.y) return false;
        this.camera.setPixels(nextX, nextY);
        this.dirty = true;
        return true;
    }

    // 왼쪽 끌기는 지도 영역에서만 시작한다. 화면 밖/안내 띠에서는 시작하지 않는다.
    pointerDown(x, y) {
        if (x < 0 || y < 0 || x >= this.width || y >= this.height - LABEL_HEIGHT) return false;
        this.drag = { x, y, camera: this.camera.pixels };
        return true;
    }

    // 포인터 캡처 중에는 지도 밖으로 나가도 같은 시작점에서 논리 픽셀 이동량을 계산한다.
    pointerMove(x, y) {
        if (!this.drag) return false;
        return this.moveTo(this.drag.camera.x + this.drag.x - x, this.drag.camera.y + this.drag.y - y);
    }

    // 포인터 취소/포커스 상실/메뉴 복귀 때 끌기 기준을 버린다.
    cancel() { this.drag = null; }

    // 방향키 한 번에4칸씩 이동한다. 눌림 반복도 브라우저의 키 반복을 그대로 점검 입력으로 쓴다.
    keyDown(code) {
        const point = this.camera.pixels;
        if (code === 'ArrowLeft') this.moveTo(point.x - KEY_PAN_X, point.y);
        else if (code === 'ArrowRight') this.moveTo(point.x + KEY_PAN_X, point.y);
        else if (code === 'ArrowUp') this.moveTo(point.x, point.y - KEY_PAN_Y);
        else if (code === 'ArrowDown') this.moveTo(point.x, point.y + KEY_PAN_Y);
        else return false;
        this.cancel();
        return true;
    }

    // 중립 정적 지형을8비트로 합성한다. 구름은 기존 메뉴의 고정 타일이며 원본 카메라 배경 위상은 후속이다.
    render(surface) {
        surface.fill(0);
        if (this.resources.clouds) surface.tile(this.resources.clouds);
        const clip = { left: 0, top: 0, right: surface.width, bottom: Math.max(0, surface.height - LABEL_HEIGHT) };
        this.renderer.draw(surface, this.camera, this.scene.entries, { clip });
        // 미션과 혼동하지 않도록 점검 화면임과 조작법을 기존 글꼴로 안내한다.
        const labelTop = Math.max(0, surface.height - LABEL_HEIGHT);
        surface.pixels.fill(0, labelTop * surface.width);
        const context = createTextContext({ color: 255 });
        const text = this.language === 'korean' ? `정적 지형 미리보기: ${this.scene.name}  |  드래그·방향키  |  Esc: 메뉴`
            : `Terrain preview: ${this.scene.name}  |  Drag / Arrow keys  |  Esc: menu`;
        const bytes = this.resources.fonts.encode(text);
        const font = this.resources.fonts.drawFont(context, bytes);
        if (font) drawGlyphText(surface, font, bytes, 10, labelTop + 6, context);
        this.dirty = false;
    }
}
