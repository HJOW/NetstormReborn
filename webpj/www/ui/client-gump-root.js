// Client가 소유하는 원본 Gump 기본 루트·등록표·수명·입력 전역을 같은 저장소에 연결한다.
// 사용: new ClientGumpRoot(width, height, checking) → root(), input(), resize(width, height), dispose(). 파생 UI와 실제 사건 연결은 별도다.
import { GUMP_BASE_SIZE, GumpCreate, GumpCreateNode, GumpCreateState } from './gump-create.js';
import { GumpLifetime, GumpLifetimeState } from './gump-lifetime.js';
import { GumpEventState, GumpEvents } from './gump-events.js';
import { GumpInput, GumpInputState } from './gump-input.js';
import { GumpFrame } from './gump-frame.js';
import { GumpTree, GumpTreeState } from './gump-tree.js';
import { refreshStormPowerGump } from './storm-power-refresh.js';
import { GumpCollection } from '../render/gump-collection.js';
import { GumpDraw, GumpDrawState, GumpPixelTarget } from '../render/gump-draw.js';

// 10.78의 별도 overlay 사건과 기본 루트 주소 토큰이다.
const OVERLAY_EVENT = 0x0200003b;
const ROOT_TOKEN = 1;

// 하나의 Client 수명에서 기본 루트와 입력 전역의 실제 소유권을 유지한다.
export class ClientGumpRoot {
    // 생성·수명·사건·입력·출력 상태와 같은 노드·처리기를 보유한다.
    #textRelease = null; #create = new GumpCreateState(); #life = new GumpLifetimeState(); #event = new GumpEventState(); #inputState = new GumpInputState();
    #nodes = new Map(); #handlers = new Map(); #destructors = new Map(); #attachers = new Map(); #broadcasters = new Map(); #idlers = new Map(); #drawers = new Map(); #redrawHandlers = new Map(); #objectGoneHandlers = new Map(); #nextToken = 2; #treeState; #factory; #lifetime; #collection; #events; #input; #tree; #frame; #redrawDevice; #formHost = null; #root = 0; #disposed = false;

    // 원본 등록표를 초기화하고 기본 루트를 생성한 뒤 현재 화면 크기를 적용한다.
    constructor(width, height, checking, redraw = null, diagnostic = null, project = null) {
        if (typeof checking !== 'function') throw new TypeError('Client Gump의 현재 진단 전역 누락');
        if (project !== null && typeof project !== 'function') throw new TypeError('Client Gump의 월드 투영 계약');
        this.#life.checking = checking; this.#inputState.overlayEvent = OVERLAY_EVENT; this.#redrawDevice = redraw;
        const node = (token) => this.#nodes.get(token) ?? null;
        this.#factory = new GumpCreate(this.#create, { node, allocate: (bytes) => {
            if (bytes !== GUMP_BASE_SIZE || this.#nodes.has(ROOT_TOKEN)) throw new Error('Client Gump의 실제 루트 할당 계약');
            this.#nodes.set(ROOT_TOKEN, new GumpCreateNode()); return ROOT_TOKEN;
        } });
        this.#collection = new GumpCollection(this.#create.collection, this.#factory.collectionHooks(project));
        const handle = (token, event, first, second) => this.handle(token, event, first, second);
        this.#events = new GumpEvents(this.#create, this.#life, this.#event, this.#collection, { node, handle });
        this.#input = new GumpInput(this.#create, this.#life, this.#inputState, this.#collection, this.#events, { node, handle });
        this.#frame = new GumpFrame(this.#create, this.#life, { node, redraw: (token, erase) => this.redraw(token, erase), idle: (token) => {
            const idle = this.#idlers.get(token);
            if (idle) return idle(token);
            if (node(token)?.constructor !== GumpCreateNode) throw new Error('Client Gump 파생 idle 처리기 미연결');
        }, broadcast: (token, event, first, second) => {
            const broadcast = this.#broadcasters.get(token);
            if (broadcast) return broadcast(token, event, first, second);
            if (node(token)?.constructor !== GumpCreateNode) throw new Error('Client Gump 파생 방송 처리기 미연결');
        } });
        const treeState = new GumpTreeState(); treeState.checking = checking; this.#treeState = treeState;
        this.#tree = new GumpTree(this.#create, treeState, { node, project,
            redraw: (token, erase) => this.redraw(token, erase),
            push: (token) => this.#tree.push(token), attach: (...args) => this.#attach(...args), kill: (token) => this.#lifetime.kill(token),
            diagnostic: (text) => { if (typeof diagnostic !== 'function') throw new Error('Client Gump 상세 진단 장치 누락'); diagnostic(text); },
        });
        this.#lifetime = new GumpLifetime(this.#create, this.#life, { node,
            assertion: (expression, file, line) => { throw new Error(`Gump 원본 진단 ${file}:${line} ${expression}`); },
            // 현재 Session의 같은 Form 호스트를 삭제 시점에 조회한다.
            deleteForm: (form, flags) => { if (!this.#formHost?.isGumpForm(form)) throw new Error('Client Gump의 실제 Form 삭제 장치 누락'); this.#formHost.destroy(form, flags); },
            push: (token) => this.#tree.push(token),
            deletingDestructor: (token, flags) => {
                const destroy = this.#destructors.get(token);
                if (destroy) return destroy(token, flags);
                if (this.#nodes.get(token)?.constructor !== GumpCreateNode) throw new Error('Client Gump 파생 소멸자 누락');
                return this.#lifetime.deleteScalar(token, flags);
            },
            // 원본 한 release 콜백은 노드 저장소와 소유 문자열(도움말 문구·주제 이름 포인터)을 같이 반납한다. 노드가 아닌 토큰은 문자열 반납 장치(bindTextRelease)가 맡는다. (2026-11-22 WB04-E1236)
            release: (token) => { if (!this.#nodes.has(token) && this.#textRelease?.(token)) return; if (!this.#nodes.delete(token)) throw new Error('Client Gump 반납 대상 누락'); this.#handlers.delete(token); this.#destructors.delete(token); this.#attachers.delete(token); this.#broadcasters.delete(token); this.#idlers.delete(token); this.#drawers.delete(token); this.#redrawHandlers.delete(token); this.#objectGoneHandlers.delete(token); },
        });
        this.#root = this.#factory.initializeRoot(); this.resize(width, height);
    }

    // 반납 뒤 이전 입력·노드를 다시 빌리지 못하게 한다.
    #live() { if (this.#disposed) throw new Error('반납한 Client Gump 루트'); }
    // 현재 원본 루트 노드를 빌린다.
    root() { this.#live(); return this.#nodes.get(this.#root); }
    // 같은 원본 등록표를 빌린다.
    createState() { this.#live(); return this.#create; }
    // 같은 삭제·dialog 수명 전역을 빌린다.
    lifetimeState() { this.#live(); return this.#life; }
    // 같은 현재 루트와 수명에 연결된 입력기를 빌린다.
    input() { this.#live(); return this.#input; }
    // 같은 Client의 실제 다시 그리기 장치에 요청한다.
    redraw(token, erase) { this.#live(); const handler = this.#redrawHandlers.get(token); if (handler) return handler(token, erase); this.redrawBase(token, erase); }
    // 파생 Redraw가 기본 장치를 재귀 없이 호출할 때 사용한다.
    redrawBase(token, erase) { this.#live(); if (typeof this.#redrawDevice !== 'function') throw new Error('Client Gump 다시 그리기 장치 누락'); this.#redrawDevice(token, erase); }
    // 실제 파생 노드의 확장 재그리기 가상 함수를 등록하거나 해제한다.
    bindRedraw(token, handler) {
        this.#live(); if (!this.#nodes.has(token) || (handler !== null && typeof handler !== 'function')) throw new TypeError('Client Gump 재그리기 토큰 또는 처리기 오류');
        if (handler === null) this.#redrawHandlers.delete(token); else this.#redrawHandlers.set(token, handler);
    }
    // 가상 구분선 입력과 위치 계산에 실제 트리를 빌린다.
    tree() { this.#live(); return this.#tree; }
    // 원본43dad0: 현재 예산 serial의 실제 노드를 Push/가상 Attach하고 깊이를 복원한다. 금액·Idle·전체 화면은 갱신하지 않는다. (WB04-E1325)
    refreshStormPower(serial) {
        this.#live(); refreshStormPowerGump(this.#create.entries, serial, { node: (token) => this.node(token), push: (token) => this.#tree.push(token), attach: (...args) => this.attach(...args) });
    }
    // 같은 현재 트리의 좌표 수집기를 빌린다.
    collection() { this.#live(); return this.#collection; }
    // 원본 `GumpTree` 지도 영역(005ca9cc~d8)의 같은 저장소를 빌린다. HUD 도킹이 이 객체에 정수 LTRB를 쓰고 월드·Renderer·중앙 정렬이 같은 객체를 읽는다. (2026-11-20 WB04-E1082)
    mapView() { this.#live(); return this.#treeState.mapView; }
    // 현재 토큰의 실제 파생 출력기를 설치하거나 해제한다.
    bindDraw(token, draw) {
        this.#live(); if (!this.#nodes.has(token) || (draw !== null && typeof draw !== 'function')) throw new TypeError('Client Gump 출력 토큰 또는 처리기 오류');
        if (draw === null) this.#drawers.delete(token); else this.#drawers.set(token, draw);
    }
    // 공통 클립 보호 안에서 같은 실제 픽셀 장치로 현재 토큰의 가상 출력을 분배한다.
    draw(token, state, target) {
        this.#live(); if (!(state instanceof GumpDrawState) || !(target instanceof GumpPixelTarget)) throw new TypeError('Client Gump 출력의 현재 클립·픽셀 장치 누락');
        const draw = new GumpDraw(this.#collection, state, { draw: (current) => {
            const node = this.#nodes.get(current), handler = this.#drawers.get(current);
            if (!node) throw new Error('Client Gump 출력 토큰 누락');
            if (handler) handler(current, target, state);
            else if (node.constructor !== GumpCreateNode) throw new Error('Client Gump 파생 출력기 미연결');
        } });
        draw.draw(token);
    }
    // 현재 노드의 가상 처리기를 직접 호출한다. 재귀 전달과 달리 처리기의 DWORD 반환을 보존한다.
    handle(token, event, first, second) {
        this.#live(); const node = this.#nodes.get(token), handler = this.#handlers.get(token);
        if (!node) throw new Error('Client Gump 처리 토큰 누락');
        if (!handler && token === this.#root && node.constructor === GumpCreateNode) return 0;
        if (!handler) throw new Error('Client Gump 파생 처리기 미연결');
        return handler(event, first, second) >>> 0;
    }
    // 같은 현재 트리의 부모 사건 전달기를 빌린다.
    events() { this.#live(); return this.#events; }
    // 같은 노드 주소의 기본 생성기를 빌린다.
    factory() { this.#live(); return this.#factory; }
    // 같은 지연 삭제와 기본 소멸자를 빌린다.
    lifetime() { this.#live(); return this.#lifetime; }
    // 현재 토큰의 실제 기본 또는 파생 노드를 빌린다.
    node(token) { this.#live(); return this.#nodes.get(token) ?? null; }
    // 현재 Session의 실제 GumpForm 호스트를 최초로 빌린다.
    setFormHost(host) {
        this.#live(); if (this.#formHost || host?.pool?.edition !== 'patch1078') throw new Error('Client Gump Form 호스트의 현재 수명 오류');
        this.#formHost = host;
    }
    // 등록 ID와 실제 객체 SID를 같은 풀의 GumpForm으로 연결한다.
    bindObject(token, object) {
        this.#live(); const node = this.#nodes.get(token), host = this.#formHost;
        if (!node || !host) throw new Error('Client Gump 객체 부착의 현재 수명 오류');
        if (!object) return;
        if (node.flags & 0x40) throw new Error('Client Gump 객체 부착의 현재 수명 오류');
        if (!Number.isInteger(object) || object < 0 || object > 0xffff || object >= host.pool.capacity) throw new RangeError('Client Gump 객체 SID 범위');
        const form = host.create(2); host.setOwner(form, node.id); host.attach(form, object, 0);
        node.flags = (node.flags | 0x40) >>> 0; node.object = object; node.form = form;
    }
    // 파생 Gump의 가상 객체 소멸 통지를 등록하거나 기본 처리로 되돌린다.
    bindObjectGone(token, handler) {
        this.#live(); if (!this.#nodes.has(token) || (handler !== null && typeof handler !== 'function')) throw new TypeError('Client Gump 객체 통지 토큰 또는 처리기 오류');
        if (handler === null) this.#objectGoneHandlers.delete(token); else this.#objectGoneHandlers.set(token, handler);
    }
    // 원본 등록 ID의 가상 객체 삭제 통지를 현재 파생 처리기 또는 기본 Kill에 전달한다.
    objectGone(owner, object) {
        this.#live(); if (!Number.isInteger(owner) || owner < 0 || owner >= this.#create.entries.length) throw new RangeError('Client Gump 소유 등록 ID 범위');
        const token = this.#create.entries[owner]; if (!token) throw new Error('Client Gump 소유 등록 토큰 누락');
        const handler = this.#objectGoneHandlers.get(token); if (handler) handler(token, object); else this.#lifetime.kill(token);
    }
    // World보다 먼저 살아 있는 Form 부착 노드를 지우고 실제 반납을 마친다.
    // 노드가 소유한 도움말 문자열 포인터를 반납하는 장치를 등록한다: (포인터) => 반납했으면 true. 노드 삭제 때 문구·주제 포인터가 이 장치로 간다. (2026-11-22 WB04-E1236)
    bindTextRelease(release) { this.#live(); if (typeof release !== 'function') throw new TypeError('Gump 문자열 반납 장치 필요'); this.#textRelease = release; }
    releaseFormHost() {
        this.#live(); if (!this.#formHost) return;
        // Kill이 자식을 함께 예약하므로 매 방문 때 삭제 상태를 다시 확인한다.
        for (const token of this.#create.entries) {
            if (!token) continue; const node = this.#nodes.get(token);
            if (node && (node.flags & 0x40) && !(node.flags & 0x10000)) this.#lifetime.kill(token);
        }
        this.#lifetime.cleanup(); this.#formHost = null;
    }
    // 호출자가 소유한 실제 파생 노드를 등록 전 주소에 두고 처리기·소멸자·선택적 가상 부착·방송·idle을 연결한다.
    allocateDerived(node, handler, destructor, attacher = null, broadcaster = null, idler = null) {
        this.#live();
        if (!(node instanceof GumpCreateNode) || node.constructor === GumpCreateNode || typeof handler !== 'function' || typeof destructor !== 'function') throw new TypeError('Client Gump 파생 노드·처리기·소멸자 계약');
        if (attacher !== null && typeof attacher !== 'function') throw new TypeError('Client Gump 파생 부착 계약');
        if (broadcaster !== null && typeof broadcaster !== 'function') throw new TypeError('Client Gump 파생 방송 계약');
        if (idler !== null && typeof idler !== 'function') throw new TypeError('Client Gump 파생 idle 계약');
        if (this.#nextToken > 0xffffffff) throw new RangeError('Client Gump 주소 소진');
        // 같은 물리 노드를 서로 다른 토큰으로 중복 등록하지 않는다.
        for (const current of this.#nodes.values()) if (current === node) throw new Error('Client Gump 파생 노드 중복 할당');
        const token = this.#nextToken++; this.#nodes.set(token, node); this.#handlers.set(token, handler); this.#destructors.set(token, destructor); if (attacher) this.#attachers.set(token, attacher); if (broadcaster) this.#broadcasters.set(token, broadcaster); if (idler) this.#idlers.set(token, idler); return token;
    }
    // 실제 기본 노드를 등록하고 고유 사건 처리기를 연결한다. 부착은 별도 호출이다.
    createBase(command, width, height, flags, handler) {
        this.#live(); if (typeof handler !== 'function' || this.#nextToken > 0xffffffff) throw new TypeError('Client Gump 기본 노드 처리기 또는 주소 누락');
        const token = this.#nextToken++; this.#nodes.set(token, new GumpCreateNode()); this.#handlers.set(token, handler);
        this.#factory.construct(token, command, width, height, flags); return token;
    }
    // 파생 가상 부착이 있으면 같은 토큰으로 분배하고 없으면 실제 기본 본체를 호출한다.
    #attach(token, ...args) { const attacher = this.#attachers.get(token); return attacher ? attacher(token, ...args) : this.#tree.attach(token, ...args); }
    // 등록된 노드를 가상 부착으로 지정 부모에 연결한다. 부모를 생략하면 루트다.
    attach(token, x, y, parent = this.#root, mode = 0) { this.#live(); this.#attach(token, x, y, parent, mode); }
    // 파생 부착 본체가 재분배 없이 같은 공통 기본 부착을 호출한다.
    attachBase(token, x, y, parent, mode, bits = {}) { this.#live(); this.#tree.attach(token, x, y, parent, mode, bits); }
    // 원본 Move가 Push 뒤 실제 파생 가상 부착을 사용한다.
    move(token, x, y, parent = 0) { this.#live(); this.#tree.move(token, x, y, parent); }
    // 등록표의 표시·통지·삭제 상태를 검사해 현재 파생 방송에 전달한다.
    broadcast(event, first, second) { this.#live(); this.#frame.broadcast(event, first, second); }
    // 같은 Client 수명의 원본 idle 표를 순서대로 처리한다.
    idle() { this.#live(); this.#frame.idle(); }
    // 원본 차단 관문은 루트의 직계 자식에서 0x4000만 읽는다(숨김·삭제 대기도 포함).
    blockingPopups() {
        this.#live(); const seen = new Set();
        // 현재 형제 링크를 따라가며 순환과 유효하지 않은 노드를 거부한다.
        for (let token = this.root().child; token;) {
            if (seen.has(token)) throw new Error('Client popup 형제 순환'); seen.add(token);
            const node = this.node(token); if (!node) throw new Error('Client popup 토큰 누락');
            if (node.flags & 0x4000) return true; token = node.next;
        }
        return false;
    }
    // 원본 팝업 닫기는 삭제 전 다음 형제를 저장하고 비삭제 직계 팝업만 Kill한다.
    closePopups() {
        this.#live(); const seen = new Set();
        // Kill이 형제 링크를 바꾸어도 직전 저장한 다음 노드를 방문한다.
        for (let token = this.root().child; token;) {
            if (seen.has(token)) throw new Error('Client popup 형제 순환'); seen.add(token);
            const node = this.node(token); if (!node) throw new Error('Client popup 토큰 누락'); const next = node.next;
            if ((node.flags & 0x4000) && !(node.flags & 0x10000)) this.#lifetime.kill(token);
            token = next;
        }
    }
    // 현재 루트 자식의 원본 visible·삭제 관문과 재귀 순서로 다시 그리기를 요청한다.
    redrawRoots() { this.#live(); this.#frame.redrawRoots(); }
    // 원본 포인터 단계처럼 창 안에서 선택적 overlay 다음 같은 사건을 방송한다.
    notifyPointer(event, mode, cursorActive, allowOverlay) {
        this.#live();
        if (!event || !Number.isInteger(event.code) || !Number.isInteger(event.x) || !Number.isInteger(event.y)) throw new TypeError('Client Gump 포인터 사건 계약');
        const code = event.code >>> 0, x = event.x | 0, y = event.y | 0;
        let overlay = null;
        if (!(code & 0x04000000)) {
            if (this.#life.currentDialog || (allowOverlay && (mode >>> 0) === 0 && !this.#life.focus && !cursorActive)) overlay = this.#input.overlay(code, { x, y });
            this.#frame.broadcast(code, x >>> 0, y >>> 0);
        }
        return { overlay };
    }
    // 같은 수명의 지연 삭제를 요청하고 현재 큐를 정리한다.
    remove(token) { this.#live(); this.#lifetime.kill(token); this.#lifetime.cleanup(); }
    // 기본 생성 전이고 등록·트리 연결이 없는 파생 할당만 직접 반납한다.
    discardUnregistered(token) {
        this.#live(); const node = this.#nodes.get(token);
        if (!node || node.constructor === GumpCreateNode || this.#create.entries.includes(token) || node.parent || node.child || node.next || node.previous) throw new Error('Client Gump 미등록 반납 계약');
        this.#nodes.delete(token); this.#handlers.delete(token); this.#destructors.delete(token); this.#attachers.delete(token); this.#broadcasters.delete(token); this.#idlers.delete(token); this.#drawers.delete(token); this.#redrawHandlers.delete(token); this.#objectGoneHandlers.delete(token);
    }
    // 임시 등록을 반환한 실제 새 루트로 공통 수명과 Client의 현재 별칭을 함께 바꾼다.
    replaceRoot(token) { this.#live(); this.#lifetime.replaceRoot(token); this.#root = token >>> 0; }
    // 루트의 현재 기본·표시 크기를 원본 SetControls와 같이 갱신한다.
    resize(width, height) {
        this.#live(); if (!Number.isInteger(width) || !Number.isInteger(height) || width <= 0 || height <= 0) throw new RangeError('Client Gump 화면 크기 오류');
        const root = this.root(); this.#create.screenWidth = width; this.#create.screenHeight = height;
        root.baseWidth = root.width = Math.fround(width); root.baseHeight = root.height = Math.fround(height);
    }
    // 현재 기본 루트의 원본 지연 삭제와 정리를 실행한 뒤 소유자를 반납한다.
    dispose() {
        if (this.#disposed) return;
        this.#lifetime.kill(this.#root); this.#lifetime.cleanup(); this.#root = 0; this.#disposed = true;
    }
}
