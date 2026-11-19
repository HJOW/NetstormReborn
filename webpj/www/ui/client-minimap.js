// 실제 Client Gump 루트에 원본 미니맵(Sky Overview)을 연결하는 조립 장치다(cpppj `RawMinimapControls`의 `RawMenuTree`·`RawMenuControls` 경계를 `ClientGumpRoot`로 대신한다).
// 사용: const minimap = new ClientMinimap(root, devices); const token = minimap.create(); minimap.refresh(); minimap.bindWorld(session.core()); minimap.close().
//   devices = { storage, dirty, globals, view(), center(x, y), move(x, y), inputMode(), poll(), baseHandle(token, event, first, second), changed() } — 모두 현재 Client 수명이 소유한다.
// 이 모듈은 DOM·Canvas를 쓰지 않으며 비트맵과 카메라 사각형은 Gump 출력 장치(`GumpPixelTarget`)로 그린다. (2026-11-20 WB04-E1082: 좌측 HUD 틀의 미니맵 칸으로 추가)
import { RawMinimapControls } from './raw-minimap-controls.js';

// 실제 Gump 루트와 미니맵 본체를 같은 수명으로 묶는다.
export class ClientMinimap {
    // 현재 루트·미니맵 본체·생성한 토큰과 마지막으로 기록한 도움말 문자열이다.
    #root; #control; #token = 0; #helpText = ''; #helpWord = '';
    // 같은 Client 수명의 공유 장치를 빌린다. 월드 연결은 별도 bindWorld가 맡는다.
    constructor(root, devices) {
        for (const name of ['storage', 'dirty', 'globals']) if (!devices?.[name]) throw new TypeError(`미니맵 조립 장치 누락: ${name}`);
        for (const name of ['view', 'center', 'move', 'inputMode', 'poll', 'baseHandle', 'changed']) if (typeof devices[name] !== 'function') throw new TypeError(`미니맵 조립 장치 누락: ${name}`);
        if (!root) throw new TypeError('미니맵의 실제 Gump 루트 필요');
        this.#root = root;
        const self = this;
        // RawMinimapControls가 요구하는 공통 트리 경계를 같은 ClientGumpRoot의 부품으로 채운다.
        const tree = {
            // 파생 노드를 등록 전 주소에 두고 입력·소멸·부착·idle·출력을 같은 본체로 연결한다.
            token: (node) => {
                let token = 0;
                token = root.allocateDerived(node, (event, first, second) => self.#control.handle(token, event, first, second),
                    (current, flags) => { root.lifetime().deleteScalar(current, flags); self.#control.release(current); },
                    (current, x, y, parent, mode) => self.#control.attach(current, x, y, parent, mode), () => {}, (current) => self.#control.idle(current));
                root.bindDraw(token, (current, target) => self.#control.draw(current, target));
                return token;
            },
            create: root.createState(), factory: root.factory(), tree: root.tree(), lifetime: root.lifetime(), input: root.input(), collection: root.collection(),
            changed: () => devices.changed(), requestRedrawBase: (token, erase) => root.redrawBase(token, erase),
        };
        // 도움말 문자열은 우클릭 도움말 메뉴 장치가 연결될 때까지 포인터로 만들지 않고 값만 기억한다.
        const menus = {
            registerExternalGumps: () => 1, unregisterExternalGumps: () => {},
            setHelpText: (token, text) => { if (token === self.#token) self.#helpText = text; }, setHelpWord: (token, word) => { if (token === self.#token) self.#helpWord = word; },
            baseHandle: (token, event, first, second) => devices.baseHandle(token, event, first, second),
        };
        this.#control = new RawMinimapControls(tree, menus, devices.storage, devices.dirty, devices.globals, devices);
    }
    // 현재 미니맵 노드의 토큰이다(생성 전에는 0).
    get token() { return this.#token; }
    // 원본 `ConstructMinimap`처럼 미니맵 Gump만 만든다. 루트 부착과 기본 128 크기는 Uber 루트 생성자가 정하고 HUD가 이어서 80×75로 도킹하며, 비트맵은 첫 `refresh()`가 확보한다.
    create() {
        if (this.#token) throw new Error('미니맵 중복 생성');
        this.#token = this.#control.construct(); return this.#token;
    }
    // 현재 Gump 크기를 원본 HUD 모드 3의 80×75와 화면 높이에 맞춘다(하단 구분선 아래 5픽셀부터).
    resize(height) { this.#control.resize(height); }
    // 현재 Screen 팔레트에서 미니맵 전용 색을 정한다.
    setPalette(palette) { this.#control.setPalette(palette); }
    // 현재 월드 Core의 Pop/Unpop에 dirty 통지를 대여한다.
    bindWorld(core) { this.#control.bindWorld(core); }
    // 현재 월드 통지를 반납하되 비트맵은 유지한다.
    unbindWorld() { this.#control.unbindWorld(); }
    // 프레임마다 dirty 큐를 소비해 비트맵을 갱신한다. 실제로 바뀌면 true다.
    refresh() { return this.#control.refresh(); }
    // 가장 최근에 기록한 도움말 문자열과 도움말 주제 이름이다.
    help() { return { text: this.#helpText, word: this.#helpWord }; }
    // 실제 노드는 루트 정리가 먼저 반납하고 월드 대여와 출력 등록은 이 호출이 끊는다.
    dispose() { this.#control.dispose(); this.#token = 0; }
}
