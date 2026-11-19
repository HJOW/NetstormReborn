// Client가 소유하는 원본 Gump 기본 루트·등록표·수명·입력 전역을 같은 저장소에 연결한다.
// 사용: new ClientGumpRoot(width, height, checking) → root(), input(), resize(width, height), dispose(). 파생 UI와 실제 사건 연결은 별도다.
import { GUMP_BASE_SIZE, GumpCreate, GumpCreateNode, GumpCreateState } from './gump-create.js';
import { GumpLifetime, GumpLifetimeState } from './gump-lifetime.js';
import { GumpEventState, GumpEvents } from './gump-events.js';
import { GumpInput, GumpInputState } from './gump-input.js';
import { GumpTree, GumpTreeState } from './gump-tree.js';
import { GumpCollection } from '../render/gump-collection.js';

// 10.78의 별도 overlay 사건과 기본 루트 주소 토큰이다.
const OVERLAY_EVENT = 0x0200003b;
const ROOT_TOKEN = 1;

// 하나의 Client 수명에서 기본 루트와 입력 전역의 실제 소유권을 유지한다.
export class ClientGumpRoot {
    // 생성·수명·사건·입력 상태와 같은 노드·처리기를 보유한다.
    #create = new GumpCreateState(); #life = new GumpLifetimeState(); #event = new GumpEventState(); #inputState = new GumpInputState();
    #nodes = new Map(); #handlers = new Map(); #destructors = new Map(); #attachers = new Map(); #nextToken = 2; #factory; #lifetime; #collection; #events; #input; #tree; #root = 0; #disposed = false;

    // 원본 등록표를 초기화하고 기본 루트를 생성한 뒤 현재 화면 크기를 적용한다.
    constructor(width, height, checking, redraw = null, diagnostic = null) {
        if (typeof checking !== 'function') throw new TypeError('Client Gump의 현재 진단 전역 누락');
        this.#life.checking = checking; this.#inputState.overlayEvent = OVERLAY_EVENT;
        const node = (token) => this.#nodes.get(token) ?? null;
        this.#factory = new GumpCreate(this.#create, { node, allocate: (bytes) => {
            if (bytes !== GUMP_BASE_SIZE || this.#nodes.has(ROOT_TOKEN)) throw new Error('Client Gump의 실제 루트 할당 계약');
            this.#nodes.set(ROOT_TOKEN, new GumpCreateNode()); return ROOT_TOKEN;
        } });
        this.#collection = new GumpCollection(this.#create.collection, this.#factory.collectionHooks());
        const handle = (token, event, first, second) => {
            const handler = this.#handlers.get(token);
            if (!handler && token === this.#root && this.#nodes.get(token)?.constructor === GumpCreateNode) return 0;
            if (!handler) throw new Error('Client Gump 파생 처리기 미연결');
            return handler(event, first, second) >>> 0;
        };
        this.#events = new GumpEvents(this.#create, this.#life, this.#event, this.#collection, { node, handle });
        this.#input = new GumpInput(this.#create, this.#life, this.#inputState, this.#collection, this.#events, { node, handle });
        const treeState = new GumpTreeState(); treeState.checking = checking;
        this.#tree = new GumpTree(this.#create, treeState, { node,
            redraw: (token, erase) => { if (typeof redraw !== 'function') throw new Error('Client Gump 다시 그리기 장치 누락'); redraw(token, erase); },
            push: (token) => this.#tree.push(token), attach: (...args) => this.#attach(...args), kill: (token) => this.#lifetime.kill(token),
            diagnostic: (text) => { if (typeof diagnostic !== 'function') throw new Error('Client Gump 상세 진단 장치 누락'); diagnostic(text); },
        });
        this.#lifetime = new GumpLifetime(this.#create, this.#life, { node,
            assertion: (expression, file, line) => { throw new Error(`Gump 원본 진단 ${file}:${line} ${expression}`); },
            push: (token) => this.#tree.push(token),
            deletingDestructor: (token, flags) => {
                const destroy = this.#destructors.get(token);
                if (destroy) return destroy(token, flags);
                if (this.#nodes.get(token)?.constructor !== GumpCreateNode) throw new Error('Client Gump 파생 소멸자 누락');
                return this.#lifetime.deleteScalar(token, flags);
            },
            release: (token) => { if (!this.#nodes.delete(token)) throw new Error('Client Gump 반납 대상 누락'); this.#handlers.delete(token); this.#destructors.delete(token); this.#attachers.delete(token); },
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
    // 같은 노드 주소의 기본 생성기를 빌린다.
    factory() { this.#live(); return this.#factory; }
    // 같은 지연 삭제와 기본 소멸자를 빌린다.
    lifetime() { this.#live(); return this.#lifetime; }
    // 현재 토큰의 실제 기본 또는 파생 노드를 빌린다.
    node(token) { this.#live(); return this.#nodes.get(token) ?? null; }
    // 호출자가 소유한 실제 파생 노드를 등록 전 주소에 두고 처리기·소멸자·선택적 가상 부착을 연결한다.
    allocateDerived(node, handler, destructor, attacher = null) {
        this.#live();
        if (!(node instanceof GumpCreateNode) || node.constructor === GumpCreateNode || typeof handler !== 'function' || typeof destructor !== 'function') throw new TypeError('Client Gump 파생 노드·처리기·소멸자 계약');
        if (attacher !== null && typeof attacher !== 'function') throw new TypeError('Client Gump 파생 부착 계약');
        if (this.#nextToken > 0xffffffff) throw new RangeError('Client Gump 주소 소진');
        // 같은 물리 노드를 서로 다른 토큰으로 중복 등록하지 않는다.
        for (const current of this.#nodes.values()) if (current === node) throw new Error('Client Gump 파생 노드 중복 할당');
        const token = this.#nextToken++; this.#nodes.set(token, node); this.#handlers.set(token, handler); this.#destructors.set(token, destructor); if (attacher) this.#attachers.set(token, attacher); return token;
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
    // 같은 수명의 지연 삭제를 요청하고 현재 큐를 정리한다.
    remove(token) { this.#live(); this.#lifetime.kill(token); this.#lifetime.cleanup(); }
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
