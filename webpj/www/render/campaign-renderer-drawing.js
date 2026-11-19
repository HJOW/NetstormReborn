// 미션 표시 수집·원본 구름 배경·정렬 제출을 현재 Screen/Renderer의 dirty 경계에 연결한다.
// 사용: new CampaignRendererDrawing(devices) → renderer.draw(pixels, pitch, 0, drawing) → renderer.present(update).
import { CampaignRenderCollection } from './campaign-render-collection.js';
import { clampOriginalScreenRect } from './gump-redraw.js';
import { drawRendererBackdrop } from './raw-renderer-clouds.js';

// 실제 World·Screen·구름 및 Client 프레임 전역을 복사하지 않고 빌린다.
export class CampaignRendererDrawing {
    // 수집/출력 대상과 현재 dirty의 지도 잘림 사각형이다.
    #devices; #collection; #mapClip = null;

    // tint와 분모도 실제 현재 값을 공급해야 한다. UI 수집·출력은 제공된 경우에만 쓴다.
    constructor(devices) {
        const { world, screen, camera, globals, clouds, files, config, random, denominator, tint, uiCollect = null, drawUi = null } = devices ?? {};
        if (!world || !screen || !camera || !globals || !clouds || !files || !config || !random || !denominator || typeof tint !== 'function' || (uiCollect !== null && typeof uiCollect !== 'function') || (drawUi !== null && typeof drawUi !== 'function')) throw new TypeError('미션 Renderer의 실제 표시·구름 수명 누락');
        this.#devices = { ...devices, uiCollect, drawUi };
        this.#collection = new CampaignRenderCollection(world, camera, screen.width, screen.height, uiCollect);
    }

    // 같은 dirty에서 먼저 Screen clip과 지도 clip을 정하고 실제 표시 항목을 수집한다.
    collect(clip, flags) {
        const { world, screen } = this.#devices;
        screen.setClip(clip.left, clip.top, clip.right, clip.bottom);
        this.#mapClip = clampOriginalScreenRect(clip, world.mapView());
        return this.#collection.collect(clip, flags).covered ? 1 : 0;
    }

    // 원본 구름/날씨 선택을 같은 잠긴 Screen과 현재 카메라·색상에 전달한다.
    background(clip, flags, covered) {
        if (!this.#mapClip) throw new Error('미션 Renderer 배경의 현재 수집 누락');
        const { screen, camera, globals, clouds, files, config, random, denominator, tint, clearOutsideMap = false } = this.#devices;
        // 아직 상단 원본 HUD가 없는 앱은 지도 밖 dirty를 검은 바탕으로 복구하고 UI를 다시 합성한다.
        if (clearOutsideMap && !(flags & 4) && !covered) {
            const map = this.#mapClip, black = screen.colors().basic[0];
            const margins = [
                { left: clip.left, top: clip.top, right: clip.right, bottom: map.top },
                { left: clip.left, top: map.bottom, right: clip.right, bottom: clip.bottom },
                { left: clip.left, top: map.top, right: map.left, bottom: map.bottom },
                { left: map.right, top: map.top, right: clip.right, bottom: map.bottom },
            ];
            // 비어 있거나 역전된 영역은 화면 지우기 호출에서 제외한다.
            for (const rect of margins) if (rect.left < rect.right && rect.top < rect.bottom) screen.clear(rect, black);
        }
        const values = { mode: { get value() { return screen.flags; } }, hideGroups: globals.hideGroups(),
            tint: { get value() { return tint(); } }, black: { get value() { return screen.colors().basic[0]; } },
            cameraX: { get value() { return camera.pixels.x; } }, cameraY: { get value() { return camera.pixels.y; } }, denominator: { get value() { return denominator.current; } } };
        drawRendererBackdrop(this.#mapClip, { value: flags }, { value: covered }, values, {
            clear: (rect, color) => screen.clear(rect, color),
            draw: (rect, x, y, alternate) => clouds.draw(screen, [rect.left, rect.top, rect.right, rect.bottom, x, y], files, config, random, alternate),
        });
    }

    // 같은 dirty 표면에 현재 목록을 원본 순서로 출력한다.
    submit(pixels, pitch, height, clip) {
        const { globals, drawUi, hideBuildings = 0 } = this.#devices;
        this.#collection.submit(pixels, pitch, height, clip, { drawUi: drawUi && ((token) => drawUi(token, pixels, pitch, height, clip)), hideGroups: () => globals.hideGroups().value, hideBuildings });
    }
}
