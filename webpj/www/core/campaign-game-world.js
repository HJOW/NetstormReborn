// 지연 로그인 캠페인의 GameWorld 앞단을 같은 Session·카메라·자산 표시기와 묶는다.
// 사용: new CampaignGameWorld(assets, files, fort, mission, services, { x, y }) → startLocalConnection() → 같은 session의 로그인 진행 → dispose().
// 입력·객체 보기·로그인 완료 공개와 브라우저 앱 수명은 후속이며 이 생성자만으로 플레이할 수 없다.
import { ClientGameplayGlobals } from './client-gameplay-globals.js';
import { createCameraHomeState } from './camera-home.js';
import { RawCampaignSession } from './raw-campaign-session.js';
import { RawCampaignFactoryDisplay } from '../render/raw-campaign-factory-display.js';

// 원본 기본 지도 영역에서 상단 게임 메뉴가 차지하는 논리 픽셀 높이다.
const MENU_HEIGHT = 28;

// 실제 캠페인 Session과 표시기의 지연 로그인 수명이다.
export class CampaignGameWorld {
    // 같은 Client 전역·Session·표시기와 화면/외부 표시 장치다.
    #gameplay; #session = null; #factoryDisplay = null; #width; #height; #services; #cameraHome = createCameraHomeState();
    // 로그인 대기·화면 변경 및 외곽 반납 상태다.
    #connecting = false; #changed = false; #disposed = false;

    // 화면 크기와 자산을 확인하고 실제 Session과 표시기를 만든다. 로그인은 호출자가 나중에 시작한다.
    constructor(assets, files, fort, mission, services, viewport) {
        if (!viewport || !(viewport.x > 0) || !(viewport.y > 0)) throw new RangeError('실제 캠페인 설치 전 화면 크기 누락');
        this.#width = viewport.x; this.#height = viewport.y; this.#services = { ...services };
        const storage = services?.surfaceStorage;
        this.#gameplay = services?.gameplayGlobals ?? new ClientGameplayGlobals(services?.subscriberGlobals, storage?.authority, storage?.gameAux);
        const input = { ...services, gameplayGlobals: this.#gameplay, install: { ...services?.install, resetCamera: () => this.resetCampaignCamera() } };
        try {
            this.#session = new RawCampaignSession(assets, files, fort, mission, input);
            this.#factoryDisplay = new RawCampaignFactoryDisplay(this.#session.core(), assets, (sid, frame) => this.#session.drawTemple(sid, frame), this.#gameplay.patchDiagnostic());
            if (services?.ownerColorTables) this.#factoryDisplay.bindOwnerColors(services.ownerColorTables);
            const palette = new Uint8Array(1024), rgb = assets.palette.rgb;
            // 같은 파일 팔레트를 원본 화면 장치의 BGRX 순서로 복사한다.
            for (let index = 0; index < 256; index += 1) {
                palette[index * 4] = rgb[index * 3 + 2]; palette[index * 4 + 1] = rgb[index * 3 + 1]; palette[index * 4 + 2] = rgb[index * 3];
            }
            const colors = services?.colorSortInput;
            this.#factoryDisplay.setPalette(palette, colors?.mapsReady ? colors.logical : [], colors?.mapsReady ? colors.maps : null);
            if (!services?.localLogin) throw new Error('실제 Client 지연 로그인 장치 누락');
        } catch (error) {
            this.#factoryDisplay = null;
            try { this.#session?.dispose(); }
            catch (cleanupError) { console.error('캠페인 GameWorld 생성 실패 후 정리 오류:', cleanupError); }
            finally { this.#session = null; this.#disposed = true; }
            throw error;
        }
    }

    // 반납한 World가 이전 Session이나 표시기를 다시 공개하지 못하게 한다.
    #live() { if (this.#disposed || !this.#session) throw new Error('반납한 캠페인 GameWorld'); return this.#session; }
    // 같은 실제 Session을 빌린다. 외곽이 더 오래 살아야 한다.
    session() { return this.#live(); }
    // 같은 Session의 풀·자산·신전 그림을 빌리는 표시기다.
    factoryDisplay() { this.#live(); return this.#factoryDisplay; }
    // World와 Session이 함께 쓰는 Client 카메라다.
    cameraGlobals() { this.#live(); return this.#gameplay.camera(); }
    // 원본 도킹 장치가 없으면 상단 메뉴 아래의 논리 화면을 사용한다.
    mapView() { this.#live(); return this.#services.mapView ? this.#services.mapView() : { left: 0, top: Math.min(MENU_HEIGHT, this.#height), right: this.#width, bottom: this.#height }; }
    // 카메라 적용 뒤 같은 영역을 표면·입자·선택적 위치 음향에 전달한다.
    #surfaceView() {
        const session = this.#live(), pixels = this.#gameplay.camera().rendererView().pixels, clip = this.mapView();
        const view = { cameraX: pixels.x, cameraY: pixels.y, left: clip.left, top: clip.top, right: clip.right, bottom: clip.bottom };
        session.world().setDisplay({ cameraX: pixels.x, cameraY: pixels.y, zoom: 65536, viewport: clip }, this.#services.surfaceDisplay);
        session.setParticleView(view, this.#services.surfaceDisplay, false);
        if (this.#services.soundViewChanged) this.#services.soundViewChanged(view);
    }
    // 실제 카메라 적용 장치 뒤 픽셀 또는 배율이 바뀌면 현재 표면과 음향을 갱신한다.
    #commitCamera() {
        const session = this.#live(), camera = this.#gameplay.camera(), before = camera.rendererView().pixels, zoom = camera.rendererView().zoom;
        session.commitRequestedCamera(this.#width, this.#height);
        const after = camera.rendererView().pixels;
        if (before.x !== after.x || before.y !== after.y || zoom !== camera.rendererView().zoom) { this.#surfaceView(); this.#changed = true; }
    }
    // 원본 로더가 호출하는 현재 사제 기준 초기 카메라다.
    resetCampaignCamera() {
        const session = this.#live(); this.#cameraHome.width = this.#width; this.#cameraHome.height = this.#height;
        session.resetCamera(this.#cameraHome, () => this.#commitCamera());
    }
    // 외곽이 Session을 공개한 뒤 같은 World의 비동기 로컬 연결을 시작한다.
    startLocalConnection() {
        const session = this.#live(); if (this.#connecting || session.loaded) throw new Error('비동기 World 로그인 새 수명 계약 오류');
        this.#connecting = true; session.startLocalConnection();
    }
    // 이전 World를 반납한 State 부모가 같은 Session의 후반 로그인을 준비한다.
    prepareLocalConnectionForState() {
        const session = this.#live(); if (this.#connecting || session.loaded) throw new Error('부모 State7 World 로그인 새 수명 계약 오류');
        this.#connecting = true; session.prepareLocalConnectionForState();
    }
    // 완료 공개 전의 로그인 대기 여부이며 Session Ready와 구별한다.
    get connecting() { this.#live(); return this.#connecting; }
    // 화면 변경을 한 번 읽고 원본처럼 지운다.
    takeChanged() { this.#live(); const changed = this.#changed; this.#changed = false; return changed; }
    // 표시기를 Session보다 먼저 놓고 같은 외곽 수명을 반납한다.
    dispose() {
        if (this.#disposed) return;
        this.#factoryDisplay = null;
        try { this.#session?.dispose(); } finally { this.#session = null; this.#disposed = true; }
    }
}
