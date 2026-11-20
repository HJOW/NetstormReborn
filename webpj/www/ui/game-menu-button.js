// 웹 전용 게임 메뉴 버튼이다. 원본(cpppj·dotnetpj)에는 ESC 로만 여는 가로 게임 메뉴막대를 키보드 없이도 열도록 화면 우측 상단에 DOM 버튼을 둔다.
// 버튼은 Canvas 위(z-index)에 놓이므로 열린 메뉴막대의 오른쪽 끝과 겹쳐도 항상 메뉴보다 앞에 보이고 눌린다. 스타일은 JS 에서만 정한다(CSS 파일 없음).
// 사용: const button = new GameMenuButton(root, { label: 'Menu', title: '게임 메뉴 (ESC)', onClick }); button.layout({ left, top, cssWidth, cssHeight }); button.show(); button.hide(); button.setPressed(true); button.destroy().
// 입력 리스너는 Canvas 에만 있으므로 버튼 위의 포인터는 게임 입력이 아니다. 눌러도 포커스를 Canvas 로 돌려 ESC 같은 키 입력이 계속 게임에 전달되게 하는 것은 onClick 호출자의 몫이다. (2026-11-20 WB04-E1084)

// 버튼과 Canvas 오른쪽 위 모서리 사이 간격(CSS 픽셀)이다.
const MARGIN = 8;

// 한 개의 DOM 버튼을 소유하고 Canvas 배치에 맞춰 위치를 갱신한다.
export class GameMenuButton {
    // 부모 요소와 만든 버튼, 마지막 배치를 보관한다.
    #root; #button; #layout = null; #listener;
    // 호스트 문서에서 버튼을 만들어 루트 끝에 붙인다. 처음에는 숨겨 둔다.
    constructor(root, { label, title, onClick }) {
        if (!root?.ownerDocument || typeof onClick !== 'function') throw new TypeError('게임 메뉴 버튼의 루트·클릭 처리기 필요');
        const button = root.ownerDocument.createElement('button');
        button.type = 'button'; button.textContent = `☰ ${label}`; button.title = title ?? label; button.setAttribute('aria-label', title ?? label); button.setAttribute('aria-pressed', 'false');
        button.setAttribute('data-netstorm', 'game-menu-button'); button.tabIndex = -1;
        Object.assign(button.style, { position: 'absolute', zIndex: '10', display: 'none', margin: '0', padding: '3px 10px', border: '1px solid #c9b88a', borderRadius: '3px',
            background: 'rgba(40, 36, 30, 0.88)', color: '#f0e6c8', font: '14px/18px "Segoe UI", "Malgun Gothic", sans-serif', cursor: 'pointer', userSelect: 'none', touchAction: 'manipulation' });
        this.#listener = (event) => { event.preventDefault(); onClick(event); };
        button.addEventListener('click', this.#listener);
        // 포인터를 눌러도 Canvas 입력으로 새지 않게 한다.
        button.addEventListener('pointerdown', (event) => event.stopPropagation());
        root.appendChild(button); this.#root = root; this.#button = button;
    }
    // 버튼 요소다(검사용).
    get element() { return this.#button; }
    // Canvas 의 현재 CSS 배치(root 안 좌표)의 오른쪽 위 모서리에서 안쪽으로 MARGIN 만큼 떨어진 곳에 둔다.
    layout(layout) {
        this.#layout = layout; if (!layout) return;
        const style = this.#button.style, right = layout.left + layout.cssWidth;
        style.left = 'auto'; style.right = `${Math.max(0, this.#root.clientWidth - right + MARGIN)}px`; style.top = `${layout.top + MARGIN}px`;
    }
    // 버튼이 보일 때 Canvas 오른쪽 끝에서 차지하는 폭을 논리 픽셀로 돌려준다(버튼 폭 + 오른쪽 간격 + 안내와의 간격). 숨겨져 있거나 크기를 알 수 없으면 0 이다. 상단 경고 안내가 버튼 뒤로 가려지지 않게 비우는 데 쓴다. (2026-11-20 WB04-E1097)
    logicalInset(logicalWidth) {
        if (!this.#layout || this.#button.style.display === 'none' || !(this.#layout.cssWidth > 0)) return 0;
        const width = this.#button.getBoundingClientRect?.().width ?? 0;
        return width > 0 ? Math.ceil((width + MARGIN + 6) * logicalWidth / this.#layout.cssWidth) : 0;
    }
    // 버튼을 보인다.
    show() { this.#button.style.display = 'block'; if (this.#layout) this.layout(this.#layout); }
    // 버튼을 숨긴다.
    hide() { this.#button.style.display = 'none'; }
    // 메뉴막대가 열려 있는지를 접근성 속성과 겉모습에 반영한다.
    setPressed(pressed) { this.#button.setAttribute('aria-pressed', pressed ? 'true' : 'false'); this.#button.style.background = pressed ? 'rgba(96, 82, 52, 0.95)' : 'rgba(40, 36, 30, 0.88)'; }
    // 버튼을 문서에서 제거한다.
    destroy() { this.#button.removeEventListener('click', this.#listener); this.#button.remove(); }
}
