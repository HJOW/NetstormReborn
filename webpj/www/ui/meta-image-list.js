// 원본 21칸 Image 목록(VT 0050d0d4, 종류 22)의 등록·배치·적중·선택·삭제를 옮긴다(원문 0047ccb0~0047d1f0·0047f360, cpppj client/MetaImageList.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 목록은 MetaImage 자식을 소유하고 각 Image 는 창/호출자가 소유한다. 같은 기본 Gump 뒤의 원문 목록 필드(개수·이미지/ID 배열·선택·열 수·셀 크기)를 쓰며 사용하지 않는 배열 끝도 보존한다.
// 부작용 순서와 float32 저장/CRT 절삭을 보존한다(생성 중 현재 count 가 바뀌어도 각 읽기 시점을 유지). 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// 사용: const list = new MetaImageList(factory, { node, image, createImage, attach, baseAttach, push, kill, destructBase, bounds, redraw, notify, green, fill }); list.construct(token, 4); list.add(token, rawImage, id).
// (2026-11-14 WB04-E440: RawMenuControls 선행 MetaImage 목록 이식)
import { GumpCreateNode } from './gump-create.js';
import { cameraFtolLow } from '../render/camera-view.js';

// 원문 IML_MAX_SLOTS 저장소 크기다.
export const META_IMAGE_LIST_CAPACITY = 21;
// 원본 가상 표/타입 BYTE/부모 통지(선택 사건) 번호다.
const LIST_VTABLE = 0x50d0d4; const LIST_KIND = 22; const SELECTION_EVENT = 0x02000004;

// 도달한 실제 하위만 필수로 검사한다(logic_error→Error).
function need(hook) { if (typeof hook !== 'function') throw new Error('Image 목록 실제 하위 미연결'); return hook; }

// 같은 기본 Gump 뒤의 원문 목록 필드다.
export class MetaImageListNode extends GumpCreateNode {
    // 현재 유효한 자식/ID 접두부 길이다.
    count = 0;
    // 실제 MetaImage 토큰 배열과 별도 원문 ID 배열이다(21칸).
    images = new Uint32Array(META_IMAGE_LIST_CAPACITY); ids = new Uint32Array(META_IMAGE_LIST_CAPACITY);
    // 선택 인덱스(-1 은 없음)와 열 수(0 이하는 가로 배치)다.
    selection = -1; columns = 0;
    // 원문 +154/+158 의 배치 후 셀 높이/너비(float32)다.
    cellHeight = 0; cellWidth = 0;
}

// 목록 본체다. factory 는 GumpCreate(construct), hooks = { node(token) → MetaImageListNode, image(token) → MetaImageGumpNode, createImage(RawImage) → 토큰, attach/baseAttach(token,x,y,parent,mode),
// push/kill/destructBase(token), bounds(token) → {left,top,right,bottom}, redraw(token,erase), notify(token, 사건, 값1, 값2), green() → 색, fill(사각형, 색) } 이다.
export class MetaImageList {
    // 실제 공통 기본 생성자와 훅 묶음이다.
    #factory; #hooks;
    constructor(factory, hooks) { this.#factory = factory; this.#hooks = hooks; }

    // 비정상 포인터/저장소 밖 count 를 성공으로 숨기지 않는다(RangeError).
    #node(token) {
        const node = token ? need(this.#hooks.node)(token) : null;
        if (!node || node.count > META_IMAGE_LIST_CAPACITY) throw new RangeError('Image 목록 실제 저장소/count 오류');
        return node;
    }
    // 현재 배열이 가리키는 실제 MetaImage 파생을 빌린다(없으면 RangeError).
    #child(token) {
        const node = token ? need(this.#hooks.image)(token) : null;
        if (!node) throw new RangeError('Image 목록 실제 자식 없음');
        return node;
    }

    // 0047ccb0: 공통 기본 생성 뒤 가상 표/종류 22/count 0/이미지 배열 0/플래그 |0x200/선택 -1/열 수를 쓴다. 원문은 ID 배열과 두 셀 크기를 쓰지 않으므로 이전 입력 비트를 보존한다.
    construct(token, columns) {
        const current = need(this.#hooks.node)(token);
        if (!current) throw new RangeError('Image 목록 생성 저장소 없음');
        this.#factory.construct(token, 0, 0, 0, 1);
        current.vtable = LIST_VTABLE; current.command = LIST_KIND; current.count = 0; current.images.fill(0); current.flags = (current.flags | 0x200) >>> 0; current.selection = -1; current.columns = columns | 0;
        return token;
    }

    // 0047ce80: 21칸이 차면 false. ID → 자식 생성(createImage) → 자식 배열 저장/count+1 → 자식 부착(0,0,목록,0) → 목록 Push → 현재 x/y 로 목록 재부착 순서다(생성 중 count 가 바뀌어도 각 읽기 시점을 유지).
    add(token, image, id) {
        const node = this.#node(token);
        if (node.count >= META_IMAGE_LIST_CAPACITY) return false;
        node.ids[node.count] = id;
        const child = need(this.#hooks.createImage)(image) >>> 0;
        node.images[node.count] = child; node.count = (node.count + 1) >>> 0;
        need(this.#hooks.attach)(child, 0, 0, token, 0); need(this.#hooks.push)(token); need(this.#hooks.attach)(token, node.x, node.y, 0, 0);
        return true;
    }

    // 0047cf40: 기본 부착 뒤 현재 크기와 count 를 읽어 셀 크기를 정한다. 열 수가 0 이하면 가로 한 줄(셀 너비=기본 너비/개수, 높이=min(기본 높이, 셀 너비)), 아니면 열×행 격자(행 수=개수/열+(나머지>0)).
    // 각 자식은 현재 count 를 종료 조건으로 Push 한 뒤 셀 크기를 float32 로 쓰고 누적 좌표(float32)로 부착하며 한 줄이 차면 다음 행으로 내린다.
    attach(token, x, y, parent, mode) {
        need(this.#hooks.baseAttach)(token, x, y, parent, mode);
        const node = this.#node(token); const count = node.count | 0;
        if (count <= 0) return;
        const columns = node.columns; let stepX = 0; let stepY = 0; let rowLimit = columns;
        if (columns <= 0) {
            const width = node.baseWidth / count; stepX = Math.fround(width); node.cellWidth = stepX;
            node.cellHeight = Math.fround(width > node.baseHeight ? node.baseHeight : width); rowLimit = count;
        } else {
            stepX = Math.fround(node.baseWidth / columns); stepY = Math.fround(node.baseHeight / (Math.trunc(count / columns) + (count % columns > 0 ? 1 : 0)));
            node.cellWidth = stepX; node.cellHeight = stepY;
        }
        let cellX = 0; let cellY = 0; let column = 0;
        // 각 셀의 누적 좌표는 원문처럼 매번 float32 로 저장한다. 자식 통지 중 변경된 현재 count 를 종료 조건으로 쓴다.
        for (let index = 0; index < node.count; index += 1) {
            need(this.#hooks.push)(node.images[index]);
            const child = this.#child(node.images[index]); child.baseWidth = node.cellWidth; child.width = Math.fround(node.cellWidth * child.scale);
            const current = this.#child(node.images[index]); current.baseHeight = node.cellHeight; current.height = Math.fround(node.cellHeight * current.scale);
            need(this.#hooks.attach)(node.images[index], cellX, cellY, token, 0); cellX = Math.fround(cellX + stepX);
            column += 1;
            if (column >= rowLimit) { column = 0; cellX = 0; cellY = Math.fround(cellY + stepY); }
        }
    }

    // 0047d0d0: 표시 scale 은 원문 적중 산술에 없다. 목록 밖(오른쪽/아래 제외)은 -1, 가로 분기는 count×dx 를 정수로 먼저 계산해 기본 너비로 나눈 ftol, 격자는 ftol(dy/셀 높이)×열+ftol(dx/셀 너비)(개수보다 크면 -1, 같으면 그 인덱스).
    hit(token, x, y) {
        const node = this.#node(token); const bounds = need(this.#hooks.bounds)(token); x |= 0; y |= 0;
        if (x < bounds.left || x >= bounds.right || y < bounds.top || y >= bounds.bottom) return -1;
        const dx = (x - bounds.left) | 0;
        if (node.columns <= 0) return cameraFtolLow(Math.imul(node.count, dx) / node.baseWidth);
        const dy = (y - bounds.top) | 0;
        const row = cameraFtolLow(dy / node.cellHeight); const column = cameraFtolLow(dx / node.cellWidth);
        const index = (Math.imul(row, node.columns) + column) | 0;
        return (node.count | 0) < index ? -1 : index;
    }

    // 0047d150: 좌누름(0x10000, 뗌 비트 없음)만 처리한다. 적중한 칸이면 선택을 토글(같으면 -1)하고 Redraw, 화면 밖 클릭도 현재 비교 결과로 부모에 통지한다(사건 0x02000004, 값1=적중 인덱스, 값2=선택됨 1/0). 처리하면 1, 아니면 0.
    handle(token, event, first, second) {
        event >>>= 0;
        if (((event & 0x03ffffff) >>> 0) !== 0x10000 || (event & 0x80000000)) return 0;
        const node = this.#node(token); const index = this.hit(token, first | 0, second | 0);
        if (index >= 0) { node.selection = node.selection === index ? -1 : index; need(this.#hooks.redraw)(token, false); }
        need(this.#hooks.notify)(token, SELECTION_EVENT, index >>> 0, node.selection === index ? 1 : 0);
        return 1;
    }

    // 0047d1f0: 선택 셀을 체크무늬(녹색)로 채운다. 오른쪽/아래는 절삭된 왼쪽/위쪽에 셀 크기를 더해 다시 절삭한다. 선택이 없으면 아무것도 하지 않는다.
    draw(token) {
        const node = this.#node(token);
        if (node.selection < 0) return;
        const bounds = { ...need(this.#hooks.bounds)(token) };
        if (node.columns <= 0) {
            bounds.left = cameraFtolLow(node.selection * node.baseWidth / node.count + bounds.left); bounds.right = cameraFtolLow(bounds.left + node.baseWidth / node.count);
        } else {
            bounds.left = cameraFtolLow((node.selection % node.columns) * node.cellWidth + bounds.left); bounds.right = cameraFtolLow(bounds.left + node.cellWidth);
            bounds.top = cameraFtolLow(Math.trunc(node.selection / node.columns) * node.cellHeight + bounds.top); bounds.bottom = cameraFtolLow(bounds.top + node.cellHeight);
        }
        const color = need(this.#hooks.green)();
        need(this.#hooks.fill)(bounds, color);
    }

    // 원문 별도 ID 이며 같은 인덱스와 구별한다. 인덱스가 0..20 밖이면 RangeError(원문 index 진단), 유효 접두부 밖이면 0 이다.
    id(token, index) {
        index |= 0;
        if (index < 0 || index >= META_IMAGE_LIST_CAPACITY) throw new RangeError('Image 목록 ID 원문 index 진단');
        const node = this.#node(token);
        return index < node.count ? node.ids[index] : 0;
    }

    // 현재 count 안의 자식이 빌린 실제 Image 다(유효 접두부 밖은 null). selection/ID 와 별도로 판정한다.
    image(token, index) {
        const node = this.#node(token); index |= 0;
        return index >= 0 && index < node.count ? this.#child(node.images[index]).image : null;
    }

    // 원문처럼 Kill 만 예약하고 count 만 줄이며 두 배열 접두부를 한 칸씩 압축한다. 마지막 중복 포인터/ID 와 현재 selection 은 쓰지 않는다. 범위 밖(0..20)은 RangeError, 개수 밖/빈 목록은 false 다.
    remove(token, index) {
        index |= 0;
        if (index < 0 || index >= META_IMAGE_LIST_CAPACITY) throw new RangeError('Image 목록 삭제 원문 index 진단');
        const node = this.#node(token);
        if (index >= node.count || !node.count) return false;
        need(this.#hooks.kill)(node.images[index]); node.count = (node.count - 1) >>> 0;
        // 두 접두부만 한 칸씩 압축한다.
        for (let current = index; current < node.count; current += 1) { node.images[current] = node.images[current + 1]; node.ids[current] = node.ids[current + 1]; }
        return true;
    }

    // 목록 가상 표 복원 뒤 실제 기본 소멸자를 호출한다. Image 해제는 이 소멸자에 없으며 실제 기본 Gump 수명으로 넘어간다.
    destruct(token) { this.#node(token).vtable = LIST_VTABLE; need(this.#hooks.destructBase)(token); }
}
