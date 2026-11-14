// 실제 Client 파일/설정과 원본 GIF 창(BackgroundGifGump)을 공통 등록표·픽셀·수명에 연결한다(cpppj client/RawBackgroundGifControls.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 같은 실제 노드(0xb0 파생)와 소유 GIF 색 번호 메모리를 단일 공통 UI 에 유지한다. 원문 raw blt 는 GIF 투명 마스크/팔레트 변환/리샘플링을 추가하지 않고 색 번호를 그대로 복사한다.
// 파일 해독은 기존 `decodeBackgroundGif`(첫 descriptor 규칙)를 쓰고 두 치수는 해독 앞에 먼저 출력한다(실패 전에도 남는다). 호스트 원시 이미지 주소 별칭은 입력/Help/Chat 문자열과 구별되는 0x72000000 대역이다.
// 사용: const gifs = new RawBackgroundGifControls(tree, { files, config, dispatch, outside }, baseHandle); const token = gifs.create('titleMenu'); gifs.draw(token, pixelTarget); gifs.close(). (2026-11-14 WB04-E462)
import { decodeBackgroundGif, gifFileDimensions } from '../assets/gif.js';
import { BackgroundGifGump, BackgroundGifGumpNode, BackgroundGifGumpState } from './background-gif-gump.js';

// 호스트 원시 이미지 주소 별칭의 시작/끝이다. 마지막 값까지 쓰면 소진이다.
const BITMAP_BASE = 0x72000000; const BITMAP_LAST = 0x72ffffff;
// 부착되지 않은 노드를 구별하는 기본 flags 의 dead 비트다.
const DEAD_FLAG = 0x10000;
// 원시 복사 클립은 제한 없는 사각형이다(실제 표면 클립은 대상 장치가 적용한다).
const UNLIMITED = Object.freeze({ left: -0x80000000, top: -0x80000000, right: 0x7fffffff, bottom: 0x7fffffff });

// 표시/입력/소멸보다 오래 유지되는 실제 Client 경계 묶음이다: files(has/read, 보호 VFS), config(pathSpec, 현재 BackgroundGifSpec), dispatch(event, first, second)→452f90 에 정규화 없이 넣는 원시 큐 사건,
// outside(bitmapToken, offset)→BYTE 는 보이는 원시 복사가 실제 할당 밖일 때만 필요한 물리 공급이다.
export class RawBackgroundGifControls {
    // 공통 트리·Client 경계·본체 상태·노드/이미지 저장소·현재 Draw 표면이다.
    #common; #context; #state = new BackgroundGifGumpState(); #nodes = new Map(); #bitmaps = new Map(); #nextBitmap = BITMAP_BASE; #backend; #activePixels = null;

    // 모든 원문 콜백은 이 고정 소유자와 현재 장치를 읽는다. 실제 10.78 파일/설정/기본 입력만 미리 요구한다.
    constructor(tree, context, baseHandle) {
        if (!tree || !context?.files || !context.config || typeof baseHandle !== 'function') throw new TypeError('BackgroundGif 실제 10.78 파일/설정/기본 입력 필요');
        this.#common = tree; this.#context = context;
        const common = tree; const hooks = {
            node: (token) => this.node(token),
            base: common.node,
            constructBase: (token) => { common.factory.construct(token, 0, 0, 0, 1); },
            destructBase: (token) => { common.lifetime.destructBase(token); },
            pathSpec: (name) => context.config.pathSpec('BackgroundGifSpec', name),
            // 현재 VFS 의 메타 우선 존재만 조회하고 뒤의 load/치수/decoder 실패는 원문 부모에 남긴다.
            exists: (path) => context.files.has(path),
            // 두 치수는 해독 앞에 쓰고 bitmap 공개/geometry 는 정상 복귀 뒤 기존 부모가 수행한다. 읽기/치수/해독 실패는 호출자에게 전파된다.
            load: (path, outputs) => {
                const bytes = context.files.read(path); if (!bytes) throw new Error(`BackgroundGif 파일 없음: ${path}`);
                const { width, height } = gifFileDimensions(bytes); outputs.width = width; outputs.height = height;
                const image = decodeBackgroundGif(bytes);
                if (this.#nextBitmap === BITMAP_LAST) throw new RangeError('BackgroundGif 이미지 주소 별칭 소진');
                const token = this.#nextBitmap++; this.#bitmaps.set(token, image); return token;
            },
            // 같은 소유 메모리만 반납한다. 0 이거나 없는 주소는 불일치다(C++ logic_error).
            releaseBitmap: (token) => { if (!token || !this.#bitmaps.delete(token)) throw new Error('BackgroundGif 같은 소유 메모리 반납 불일치'); },
            position: (token) => common.collection.position(token),
            attach: common.attach,
            baseHandle,
            dispatch: (event, first, second) => { if (typeof context.dispatch !== 'function') throw new Error('BackgroundGif 실제 원시 입력 큐 미연결'); context.dispatch(event, first, second); },
            // 원문 Draw 안에서만 유효한 표면에 원시 색 번호를 복사한다. 소스 시작 = 위 줄 × 행 간격 + 왼쪽.
            copy: (request) => {
                if (!this.#activePixels) throw new Error('BackgroundGif 실제 Draw 문맥 없음');
                const image = this.#bitmaps.get(request.bitmap); if (!image) throw new Error('BackgroundGif 실제 원시 메모리 없음');
                const outside = typeof context.outside === 'function' ? (offset) => context.outside(request.bitmap, offset) : null;
                this.#activePixels.rawBitmap(image.indices, request.stride, BigInt(request.source.top) * BigInt(request.stride) + BigInt(request.source.left),
                    { x: request.source.right - request.source.left, y: request.source.bottom - request.source.top }, request.position, UNLIMITED, outside);
            },
        };
        this.#backend = new BackgroundGifGump(this.#state, hooks);
    }

    // 원문 new 0xb0 에 대응하는 실제 고정 파생 저장소다. 기본 생성 전부터 노드를 소유한다.
    allocate() { const token = this.#common.token(); this.#nodes.set(token, new BackgroundGifGumpNode()); return token; }
    // 같은 실제 공장/파일 적재를 수행하는 전체 생성자다.
    create(name) { return this.#backend.construct(this.allocate(), name); }
    // 현재 실제 기본 주소와 같은 노드만 반환한다. 없는 토큰은 null 이다.
    node(token) { return this.#nodes.get(token) ?? null; }
    // 새 Uber 도 같은 원문 생성/설정 본체를 빌린다.
    get backend() { return this.#backend; }
    // 같은 소유 GIF 버퍼의 실제 색 번호를 읽는다. 없는 주소는 RangeError 이다(C++ out_of_range).
    bitmap(token) { const image = this.#bitmaps.get(token); if (!image) throw new RangeError('BackgroundGif 이미지 없음'); return image; }
    // 실제 소유 메모리 수만 관측한다(게임 값을 바꾸지 않는다).
    get imageCount() { return this.#bitmaps.size; }
    // 현재 공통 클립/표면으로 원시 복사를 실행한다. Draw 안에서만 표면을 빌리며 정상/예외 반환에서도 반납한다. 재진입은 거부한다.
    draw(token, pixels) {
        if (this.#activePixels) throw new Error('BackgroundGif Draw 재진입 불가');
        this.#activePixels = pixels;
        try { this.#backend.draw(token); } finally { this.#activePixels = null; }
    }
    // 같은 실제 입력 본체다(원문 입력의 전체 DWORD 반환).
    handle(token, event, first, second) { return this.#backend.handle(token, event, first, second); }
    // 원문 이미지 free → 기본 소멸 → 객체 free 순서다. flags 의 1 비트가 객체 반납이다.
    deleteScalar(token, flags) { this.#backend.destruct(token); if (flags & 1) this.release(token); }
    // 공통 scalar 의 실제 노드 반납 경계다.
    release(token) { this.#nodes.delete(token); }
    // 미부착/부분 생성도 공통 Kill/Cleanup 으로 반납한다. 등록표에 올라간 노드는 dead 가 아니면 Kill 하고, 기본 생성 전 실패한 노드만 직접 반납한다.
    close() {
        const common = this.#common;
        // 등록표의 모든 GIF 노드를 자식 우선 수명으로 내린다(1부터: 0번은 루트).
        for (let id = 1; id < common.create.entries.length; id += 1) {
            const token = common.create.entries[id]; const node = this.node(token);
            if (node && !(node.flags & DEAD_FLAG)) common.lifetime.kill(token);
        }
        common.lifetime.cleanup();
        // 정리 뒤에도 등록표에 남은 노드가 있으면 불일치다. 기본 생성 전 실패한 노드만 직접 반납한다.
        while (this.#nodes.size > 0) {
            const token = this.#nodes.keys().next().value;
            if (common.create.entries.includes(token)) throw new Error('BackgroundGif 정리 뒤 등록 노드 잔류');
            this.release(token);
        }
        common.changed();
    }
}
