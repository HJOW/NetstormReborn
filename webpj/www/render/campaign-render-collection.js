// 같은 미션 Session의 지면·객체·입자·커서 표시 항목을 원본 순서와 5,000칸 한도로 모은다.
// 사용: new CampaignRenderCollection(world, camera, width, height, uiCollector).collect(dirty, flags).
import { DRAW_SUBMISSION_CAPACITY, collectDrawEntries, dispatchDrawEntries, sortRenderEntries } from './draw-submission.js';
import { ObjectCollection } from './object-collection.js';
import { TileGroupCollection } from './tile-group-collection.js';
import { rendererCollectionRowLimit, rendererCollectionViewport, rendererWorldRect } from './renderer-collection-geometry.js';

// World·카메라와 작업 배열을 빌리며 수집 결과는 다음 collect 전까지 유효하다.
export class CampaignRenderCollection {
    // 같은 Session의 실제 수집기와 작업 배열이다.
    #world; #camera; #width; #height; #ui; #groups; #objects; #entries = new Uint8Array(DRAW_SUBMISSION_CAPACITY * 16); #count = null;

    // 화면 높이는 지면 묶음 저장소 크기에 쓰므로 화면 변경 시 새 수집기를 만든다.
    constructor(world, camera, width, height, uiCollector = null) {
        if (!world || typeof world.session !== 'function' || typeof world.mapView !== 'function' || typeof camera?.screenToWorld !== 'function' || !Number.isInteger(width) || !Number.isInteger(height) || width <= 0 || height <= 0 || (uiCollector !== null && typeof uiCollector !== 'function')) throw new TypeError('미션 Renderer의 World·카메라·화면·UI 수집 경계 오류');
        const core = world.session().core(); this.#world = world; this.#camera = camera; this.#width = width; this.#height = height; this.#ui = uiCollector;
        this.#groups = new TileGroupCollection(core.hash, core.pool, { screenHeight: height });
        this.#objects = new ObjectCollection(core.hash, core.pool, core.types, core.shapes);
    }

    // UI→입자→지면 묶음→객체→커서 순서로 같은 작업 배열을 채운다.
    collect(dirty, flags) {
        this.#count = null;
        const session = this.#world.session(), map = this.#world.mapView(), camera = this.#camera;
        const request = { screen: dirty, visible: rendererCollectionViewport(dirty, map, camera), unclipped: rendererWorldRect(dirty, camera),
            dirtyFlags: flags >>> 0, groupRowLimit: rendererCollectionRowLimit(this.#width, this.#height) };
        const hooks = { ui: this.#ui, construction: (view, output, limit) => session.particles().collectDrawing(view, output, limit),
            groups: (view, limit, covered) => this.#groups.collect(view, limit, covered),
            objects: (view, output, limit) => this.#objects.collect(view, output, limit) };
        if (session.hasCursorRuntimeForState()) hooks.preview = (view, output, limit) => session.cursor().display.collectDrawing(view, output, limit);
        const result = collectDrawEntries(request, this.#entries, hooks);
        this.#count = result.count;
        return { entries: this.#entries.subarray(0, result.count * 16), covered: result.backgroundCovered, request };
    }

    // 이번 수집의 묶음 토큰은 실제 지면 행 저장소에서만 해석한다.
    group(token) { this.#world.session(); return this.#groups.group(token); }

    // 현재 수집분만 원본 정렬 뒤 같은 Session의 지면·자산·입자·커서·UI 장치로 출력한다.
    submit(pixels, pitch, height, clip, { drawUi = null, hideGroups = false, hideBuildings = 0 } = {}) {
        if (this.#count === null) throw new Error('미션 Renderer 제출 전 현재 수집 누락');
        if (!(pixels instanceof Uint8Array) || !Number.isInteger(pitch) || !Number.isInteger(height) || pitch < this.#width || height < this.#height || pixels.length < pitch * height || (drawUi !== null && typeof drawUi !== 'function')) throw new TypeError('미션 Renderer의 실제 화면·UI 출력 경계 오류');
        const session = this.#world.session(), entries = this.#entries.subarray(0, this.#count * 16);
        sortRenderEntries(entries);
        const state = { get hideGroups() { return typeof hideGroups === 'function' ? Boolean(hideGroups()) : Boolean(hideGroups); }, debugAfterEach: false };
        dispatchDrawEntries(entries, state, {
            sprite: (entry) => { const camera = this.#camera.pixels, hidden = typeof hideBuildings === 'function' ? hideBuildings() : hideBuildings;
                this.#world.factoryDisplay().drawEntry(entry, pixels, pitch, height, clip, [camera.x, camera.y], session.localOwner(), hidden,
                    (sid, x, y) => session.drawBridgeAdditional(sid, x, y)); },
            group: (token) => this.group(token), ui: drawUi,
            construction: (token) => session.particles().drawEntry(token, pixels, pitch, height, clip),
            preview: (entry) => session.cursor().display.drawEntry(entry, pixels, pitch, height, clip),
        });
    }
}
