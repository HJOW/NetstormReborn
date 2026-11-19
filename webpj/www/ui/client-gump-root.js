// Client가 소유하는 원본 Gump 기본 루트·등록표·수명·입력 전역을 같은 저장소에 연결한다.
// 사용: new ClientGumpRoot(width, height, checking) → root(), input(), resize(width, height), dispose(). 파생 UI와 실제 사건 연결은 별도다.
import { GUMP_BASE_SIZE, GumpCreate, GumpCreateNode, GumpCreateState } from './gump-create.js';
import { GumpLifetime, GumpLifetimeState } from './gump-lifetime.js';
import { GumpEventState, GumpEvents } from './gump-events.js';
import { GumpInput, GumpInputState } from './gump-input.js';
import { GumpCollection } from '../render/gump-collection.js';

// 10.78의 별도 overlay 사건과 기본 루트 주소 토큰이다.
const OVERLAY_EVENT = 0x0200003b;
const ROOT_TOKEN = 1;

// 하나의 Client 수명에서 기본 루트와 입력 전역의 실제 소유권을 유지한다.
export class ClientGumpRoot {
    // 생성·수명·사건·입력 상태와 같은 노드·처리기를 보유한다.
    #create = new GumpCreateState(); #life = new GumpLifetimeState(); #event = new GumpEventState(); #inputState = new GumpInputState();
    #nodes = new Map(); #factory; #lifetime; #collection; #events; #input; #root = 0; #disposed = false;

    // 원본 등록표를 초기화하고 기본 루트를 생성한 뒤 현재 화면 크기를 적용한다.
    constructor(width, height, checking) {
        if (typeof checking !== 'function') throw new TypeError('Client Gump의 현재 진단 전역 누락');
        this.#life.checking = checking; this.#inputState.overlayEvent = OVERLAY_EVENT;
        const node = (token) => this.#nodes.get(token) ?? null;
        this.#factory = new GumpCreate(this.#create, { node, allocate: (bytes) => {
            if (bytes !== GUMP_BASE_SIZE || this.#nodes.has(ROOT_TOKEN)) throw new Error('Client Gump의 실제 루트 할당 계약');
            this.#nodes.set(ROOT_TOKEN, new GumpCreateNode()); return ROOT_TOKEN;
        } });
        this.#collection = new GumpCollection(this.#create.collection, this.#factory.collectionHooks());
        const handle = (token) => { if (token === this.#root) return 0; throw new Error('Client Gump 파생 처리기 미연결'); };
        this.#events = new GumpEvents(this.#create, this.#life, this.#event, this.#collection, { node, handle });
        this.#input = new GumpInput(this.#create, this.#life, this.#inputState, this.#collection, this.#events, { node, handle });
        this.#lifetime = new GumpLifetime(this.#create, this.#life, { node,
            assertion: (expression, file, line) => { throw new Error(`Gump 원본 진단 ${file}:${line} ${expression}`); },
            deletingDestructor: (token, flags) => this.#lifetime.deleteScalar(token, flags),
            release: (token) => { if (!this.#nodes.delete(token)) throw new Error('Client Gump 반납 대상 누락'); },
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
