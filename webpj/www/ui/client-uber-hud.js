// 실제 미션 화면의 원본 Uber HUD 틀(세로·미니맵·채팅 구분선, 미니맵, 생산 패널)을 같은 Client Gump 루트에 설치하고 지도 영역을 도킹한다.
// 원본 `UberRootGump` 생성·교체 순서와 `HudDock` 모드 3 도킹을 그대로 호출하며(cpppj `UberHud.inc` `InstallHud`·`RawUberRootControls`), 앱이 아직 연결하지 않은 지식(NSInfo) 패널과 채팅 창은 만들지 않고 해당 자식 필드를 0으로 둔다.
// 사용: const hud = new ClientUberHud(root, devices); hud.install(); hud.refresh(); hud.reflow(width, height); hud.close().
//   devices = { assets, panelGlobals, divider, fullDeck(), mapRevision(), createMinimap(), createProduction(), minimap, closePopups(), baseHandle(token, event, first, second),
//   invalidate(rect, flags), finishResize() } — 모두 현재 Client 수명이 소유하며 이 모듈은 DOM·Canvas를 쓰지 않는다. (2026-11-20 WB04-E1082: 좌측 사이드바 위치 보정으로 추가)
import { HudDock, HudDividerNode } from './hud-dock.js';
import { RawUberRootLookup } from './raw-uber-root-lookup.js';
import { UberRootGump, UberRootNode, uberRootDockView } from './uber-root-gump.js';

// 원본 HUD 모드 명령(0200003d)과 채팅 표시 전환 명령(02000088), 삭제 중 표시 비트이다.
const HUD_MODE = 0x0200003d, CHAT_TOGGLE = 0x02000088, DEAD = 0x10000;
// UberRootGump가 읽는 원본 전역 주소와 값이다. 0x540451은 구분선 종류 색 번호(54), 0x542664는 구분선 프레임 번호(1)다.
const ADDRESS = Object.freeze({ width: 0x005c78f0, height: 0x005c78f4, fortGump: 0x005411c8, title: 0x00565e70, divider: 0x00540451, frame: 0x00542664,
    backgroundType: 0x005ca920, backgroundFrame: 0x005ca924, panelFrame: 0x005ca928 });
// 구분선 파생 노드의 원본 크기(0x11c)와 Uber 루트 파생 노드의 원본 크기(0xc8)다.
const DIVIDER_BYTES = 0x11c, ROOT_BYTES = 0xc8;

// 실제 Client 수명의 같은 Gump 루트에 HUD 틀을 만들고 지도 영역을 갱신한다.
export class ClientUberHud {
    // 현재 루트·장치와 HudDock이 읽고 쓰는 실제 전역, 구분선 조회기와 현재 Uber 루트 토큰이다.
    #root; #devices; #globals; #lookup; #token = 0; #previous = { width: 0, height: 0 }; #closed = new Set();
    // 모든 장치를 확인하고 HudDock 전역을 현재 화면 크기로 초기화한다. 지도 사각형은 Gump 트리가 읽는 같은 객체를 쓴다.
    constructor(root, devices) {
        if (!root || !devices?.assets || !devices.panelGlobals || !devices.divider) throw new TypeError('Uber HUD의 실제 루트·자산·전역·구분선 필요');
        for (const name of ['fullDeck', 'mapRevision', 'createMinimap', 'createProduction', 'closePopups', 'baseHandle', 'invalidate', 'finishResize']) {
            if (typeof devices[name] !== 'function') throw new TypeError(`Uber HUD 장치 누락: ${name}`);
        }
        this.#root = root; this.#devices = devices; this.#lookup = new RawUberRootLookup(root);
        const create = root.createState(), width = create.screenWidth, height = create.screenHeight, view = root.mapView();
        // 원본 정수 지도 전역과 float 사본이다. 도킹 전에는 전체 화면이며 도킹 이벤트가 정수 LTRB를 같은 객체에 쓴다.
        Object.assign(view, { left: 0, top: 0, right: width, bottom: height });
        this.#globals = { screenWidth: width, screenHeight: height, mapLeft: 0, mapTop: 0, mapWidth: width, mapHeight: height, mapFloatRect: [0, 0, width, height], mapRect: view,
            leftBottomRect: { left: 0, top: 0, right: 0, bottom: 0 }, savedChatY: 0,
            get showHud() { return devices.fullDeck().value; }, get fullRefresh() { return devices.mapRevision().value; }, set fullRefresh(value) { devices.mapRevision().value = value >>> 0; } };
        this.#previous = { width, height };
    }
    // 현재 Uber 루트의 토큰이다(설치 전에는 0).
    get token() { return this.#token; }
    // 현재 Uber 루트의 파생 노드다.
    node() { const node = this.#token ? this.#root.node(this.#token) : null; return node instanceof UberRootNode ? node : null; }
    // 실제 구분선만 같은 저장소에서 꺼낸다.
    #dividerNode(token) { const node = token ? this.#root.node(token) : null; return node instanceof HudDividerNode ? node : null; }
    // 현재 토큰을 읽는 HudDock을 짧게 만든다. 자식 토큰은 매 소비 때 Uber 루트에서 다시 읽는다.
    #dock(token) {
        const root = this.#root, devices = this.#devices;
        return new HudDock(uberRootDockView(token, { node: (current) => root.node(current), base: (current) => root.node(current) }), this.#globals, () => 55, {
            node: (current) => root.node(current), divider: (current) => this.#dividerNode(current), baseAttach: (...args) => root.attachBase(...args),
            push: (current) => root.tree().push(current), attach: (...args) => root.attach(...args), move: (...args) => root.move(...args),
            resolveSerial: (serial) => this.#lookup.reference(serial), bubble: (...args) => root.events().bubble(...args), closePopups: () => devices.closePopups(),
            invalidate: (left, top, width, height, flags) => devices.invalidate({ left, top, right: (left + width) | 0, bottom: (top + height) | 0 }, flags),
            finishResize: () => devices.finishResize(), baseHandle: (event, first, second) => devices.baseHandle(token, event, first, second),
            // 채팅 창 공장이 앱에 연결되기 전에는 표시 전환이 바꿀 실제 창이 없으므로 구분선 이동만 수행한다.
            chatVisible: () => {}, chatInput: () => {},
        });
    }
    // 원본 전역 읽기다. 앱 값만 돌려주고 모르는 주소는 거부한다.
    #read(address) {
        const root = this.#root, panel = this.#devices.panelGlobals, create = root.createState();
        switch (address) {
        case ADDRESS.width: return create.screenWidth >>> 0;
        case ADDRESS.height: return create.screenHeight >>> 0;
        case ADDRESS.fortGump: return (this.#devices.assets.find('fortGump').block + 70) >>> 0;
        case ADDRESS.title: return 0;
        case ADDRESS.divider: return 54;
        case ADDRESS.frame: return 1;
        case ADDRESS.backgroundType: return panel.backgroundType >>> 0;
        case ADDRESS.backgroundFrame: return panel.backgroundFrame >>> 0;
        default: throw new Error(`Uber HUD의 미연결 전역: ${address.toString(16)}`);
        }
    }
    // 원본 전역 쓰기다. 배경 종류·프레임·패널 프레임만 생산 패널 전역에 기록한다.
    #write(address, value) {
        const panel = this.#devices.panelGlobals;
        if (address === ADDRESS.backgroundType) panel.backgroundType = value >>> 0;
        else if (address === ADDRESS.backgroundFrame) panel.backgroundFrame = value >>> 0;
        else if (address === ADDRESS.panelFrame) panel.panelFrame = value >>> 0;
        else throw new Error(`Uber HUD의 미연결 전역 쓰기: ${address.toString(16)}`);
    }
    // 원본 `new` 크기별로 Uber 루트 또는 구분선 파생 노드를 Gump 루트에 등록한다.
    #allocate(bytes) {
        const root = this.#root;
        if (bytes === ROOT_BYTES) {
            const node = new UberRootNode(); let token = 0;
            // 루트의 기본 방송은 원본에서 빈 본문이므로 방송 처리기는 아무것도 하지 않는다.
            // HUD 자식을 지운 빈 루트는 다음 미션이 교체하기 전까지 현재 루트로 남는다. 그 사이의 HUD 모드·크기 사건은 도킹하지 않고 기본 처리로 돌려보낸다. (2026-11-20 WB04-E1085)
            token = root.allocateDerived(node, (event, first, second) => this.#closed.has(token) ? this.#devices.baseHandle(token, event, first, second) >>> 0 : this.#dock(token).handle(event, first, second),
                (current, flags) => root.lifetime().deleteScalar(current, flags), null, () => {});
            return token;
        }
        if (bytes === DIVIDER_BYTES) return this.#devices.divider.allocate();
        throw new Error('Uber HUD의 미지원 할당 크기');
    }
    // 원본 Uber 생성·교체에 필요한 실제 하위 장치 묶음이다. 지식·채팅 공장은 의도적으로 제공하지 않는다.
    #hooks() {
        const root = this.#root, devices = this.#devices;
        return {
            base: (token) => root.node(token), node: (token) => root.node(token), divider: (token) => this.#dividerNode(token),
            read: (address) => this.#read(address), write: (address, value) => this.#write(address, value), allocate: (bytes) => this.#allocate(bytes),
            constructBase: (token, kind, width, height, flags) => root.factory().construct(token, kind, width, height, flags),
            constructDivider: (token, axis, width, height, flags, type, frame) => devices.divider.construct(token, axis, width, height, flags, type, frame),
            attach: (child, x, y, parent, mode) => root.attach(child, x, y, parent, mode), move: (token, x, y, parent) => root.move(token, x, y, parent),
            push: (token) => root.tree().push(token), registerSlave: (master, slave, flags) => devices.divider.registerSlave(master, slave, flags),
            createMinimap: () => devices.createMinimap(), createProduction: () => devices.createProduction(),
            cleanup: () => root.lifetime().cleanup(), kill: (token) => root.lifetime().kill(token), replaceRoot: (token) => root.replaceRoot(token),
        };
    }
    // 새 Uber 루트를 만들어 기존 직계 자식을 옮기고 현재 루트로 공개한 뒤 모드 3 도킹과 채팅 숨김을 적용한다.
    install() {
        if (this.#token) throw new Error('Uber HUD 중복 설치');
        const root = this.#root, create = root.createState();
        new UberRootGump(create, this.#hooks()).replace();
        this.#token = create.entries[0]; this.#previous = { width: create.screenWidth, height: create.screenHeight };
        // 첫 World 대여의 모드 3: 세로 구분선을 x80, 미니맵 구분선을 아래 80에 두고 같은 도킹 순서로 자식 크기를 맞춘다.
        root.handle(this.#token, HUD_MODE, 3, 0);
        // 원본 미션 화면은 채팅 창을 숨긴 상태로 시작하므로 같은 전환 명령으로 채팅 구분선을 화면 아래로 내린다.
        root.handle(this.#token, CHAT_TOGGLE, 0, 0);
        return this.#token;
    }
    // 현재 도킹으로 정해진 정수 지도 영역이다(Gump 트리·World·Renderer가 읽는 같은 객체).
    mapView() { return this.#root.mapView(); }
    // 화면 크기가 바뀌면 사용자가 끈 구분선 위치의 하단 간격을 유지하며 다시 배치한다(cpppj `RawUberRootControls::Reflow`, 고정 해상도 원본에 없는 Windows 호환 재배치).
    reflow(width, height) {
        if (!this.#token) throw new Error('설치되지 않은 Uber HUD의 화면 재배치');
        if (!Number.isInteger(width) || !Number.isInteger(height) || width <= 0 || height <= 0) throw new RangeError('Uber HUD 화면 크기 오류');
        const root = this.#root, node = this.node(), globals = this.#globals;
        globals.screenWidth = width; globals.screenHeight = height;
        if (width === this.#previous.width && height === this.#previous.height) return;
        const vertical = this.#dividerNode(node.vertical), minimap = this.#dividerNode(node.minimapDivider), chat = this.#dividerNode(node.chatDivider);
        const delta = height - this.#previous.height, miniY = Math.fround(minimap.y + delta), chatY = Math.fround(chat.y + delta);
        // 숨김 중에는 화면 끝으로 옮긴 구분선 대신 마지막 표시 위치도 같은 높이 차이만큼 옮겨 다음 전환이 이전 해상도로 돌아가지 않게 한다.
        if (chat.y >= this.#previous.height) globals.savedChatY = Math.fround(globals.savedChatY + delta);
        vertical.baseHeight = Math.fround(height); vertical.height = Math.fround(height * vertical.scale);
        minimap.minY = (height - 123) | 0; minimap.maxY = height | 0; chat.maxY = (height - 25) | 0;
        root.lifetime().cleanup(); root.move(node.vertical, vertical.x, vertical.y, 0); root.lifetime().cleanup();
        root.move(node.minimapDivider, minimap.x, miniY, 0); root.lifetime().cleanup();
        root.move(node.chatDivider, chat.x, chatY, 0); root.lifetime().cleanup();
        this.#previous = { width, height };
        this.#devices.finishResize();
        // 구분선을 하나씩 옮기는 동안 미니맵이 잠깐 새 화면 높이와 옛 위치의 차이만큼 커져 crop이 음수로 눌릴 수 있다. 호스트 크기 변경은 같은 호출에서 곧바로 그리므로, 그리기 전에 한 번 갱신해 crop을 새 지도 영역으로 다시 계산한다.
        this.refresh();
    }
    // 프레임마다 미니맵 dirty 큐를 소비한다. 실제로 바뀌면 true다.
    refresh() { return this.#devices.minimap ? this.#devices.minimap.refresh() : false; }
    // 월드 반납 전에 HUD 자식을 모두 지우고 미니맵의 월드 대여를 끊는다. 빈 Uber 루트는 다음 설치가 원본처럼 교체한다.
    close() {
        if (!this.#token) return;
        const root = this.#root, node = this.node();
        if (node) {
            for (const field of ['vertical', 'minimapDivider', 'chatDivider', 'minimap', 'production', 'knowledge', 'chat']) {
                const token = node[field], child = token ? root.node(token) : null;
                if (child && !(child.flags & DEAD)) root.remove(token);
            }
        }
        this.#devices.minimap?.dispose(); this.#closed.add(this.#token); this.#token = 0;
        // HUD 가 사라지면 지도 영역은 다시 전체 논리 화면이다(메인 메뉴 복귀 뒤 중앙 정렬 등이 같은 객체를 읽는다).
        const create = root.createState(); Object.assign(root.mapView(), { left: 0, top: 0, right: create.screenWidth, bottom: create.screenHeight });
    }
    // 현재 HUD의 직계 자식 토큰과 도킹된 지도 영역을 한 번에 진단한다.
    report() {
        const node = this.node(), view = this.mapView();
        return { token: this.#token, children: node ? { vertical: node.vertical, minimapDivider: node.minimapDivider, chatDivider: node.chatDivider, minimap: node.minimap, production: node.production } : null,
            map: { left: view.left, top: view.top, right: view.right, bottom: view.bottom } };
    }
}
