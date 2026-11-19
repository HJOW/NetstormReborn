// 같은 미션 Session의 지면·객체·입자·커서 표시 항목을 원본 순서와 5,000칸 한도로 모은다.
// 사용: new CampaignRenderCollection(world, camera, width, height, uiCollector).collect(dirty, flags).
import { DRAW_SUBMISSION_CAPACITY, collectDrawEntries } from './draw-submission.js';
import { ObjectCollection } from './object-collection.js';
import { TileGroupCollection } from './tile-group-collection.js';
import { rendererCollectionRowLimit, rendererCollectionViewport, rendererWorldRect } from './renderer-collection-geometry.js';

// World·카메라와 작업 배열을 빌리며 수집 결과는 다음 collect 전까지 유효하다.
export class CampaignRenderCollection {
    // 같은 Session의 실제 수집기와 작업 배열이다.
    #world; #camera; #width; #height; #ui; #groups; #objects; #entries = new Uint8Array(DRAW_SUBMISSION_CAPACITY * 16);

    // 화면 높이는 지면 묶음 저장소 크기에 쓰므로 화면 변경 시 새 수집기를 만든다.
    constructor(world, camera, width, height, uiCollector = null) {
        if (!world || typeof world.session !== 'function' || typeof world.mapView !== 'function' || typeof camera?.screenToWorld !== 'function' || !Number.isInteger(width) || !Number.isInteger(height) || width <= 0 || height <= 0 || (uiCollector !== null && typeof uiCollector !== 'function')) throw new TypeError('미션 Renderer의 World·카메라·화면·UI 수집 경계 오류');
        const core = world.session().core(); this.#world = world; this.#camera = camera; this.#width = width; this.#height = height; this.#ui = uiCollector;
        this.#groups = new TileGroupCollection(core.hash, core.pool, { screenHeight: height });
        this.#objects = new ObjectCollection(core.hash, core.pool, core.types, core.shapes);
    }

    // UI→입자→지면 묶음→객체→커서 순서로 같은 작업 배열을 채운다.
    collect(dirty, flags) {
        const session = this.#world.session(), map = this.#world.mapView(), camera = this.#camera;
        const request = { screen: dirty, visible: rendererCollectionViewport(dirty, map, camera), unclipped: rendererWorldRect(dirty, camera),
            dirtyFlags: flags >>> 0, groupRowLimit: rendererCollectionRowLimit(this.#width, this.#height) };
        const hooks = { ui: this.#ui, construction: (view, output, limit) => session.particles().collectDrawing(view, output, limit),
            groups: (view, limit, covered) => this.#groups.collect(view, limit, covered),
            objects: (view, output, limit) => this.#objects.collect(view, output, limit) };
        if (session.hasCursorRuntimeForState()) hooks.preview = (view, output, limit) => session.cursor().display.collectDrawing(view, output, limit);
        const result = collectDrawEntries(request, this.#entries, hooks);
        return { entries: this.#entries.subarray(0, result.count * 16), covered: result.backgroundCovered, request };
    }

    // 이번 수집의 묶음 토큰은 실제 지면 행 저장소에서만 해석한다.
    group(token) { this.#world.session(); return this.#groups.group(token); }
}
