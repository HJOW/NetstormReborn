// 원본 생산 예산 표시의 금액 이동·점멸과 실제 TextGump를 연결한다.
// 사용: new ClientProductionBudget(root, text, devices).create(parent), 이후 GumpFrame.idle().

// 원본 단정도 초 간격을 JavaScript 수로 승격한 값이다.
const FLASH_INTERVAL = 0.15000000596046448;

// 생산 패널과 같은 글자 Gump 및 현재 돈·시간을 빌린다.
export class ClientProductionBudget {
    // 공통 수명과 현재 장치 및 원본 예산 상태다.
    #root; #text; #devices; #token = 0; #amount = 0; #flash = 0; #until = 0;
    // 돈과 편집 모드·시간·배경 타입은 호출 시점마다 실제 상태에서 읽는다.
    constructor(root, text, devices) {
        if (!root || !text || !devices || ['now', 'money', 'known', 'fort', 'editor', 'backgroundType', 'changed'].some((name) => typeof devices[name] !== 'function')) throw new TypeError('생산 예산의 실제 장치 누락');
        this.#root = root; this.#text = text; this.#devices = devices;
    }
    // 현재 실제 텍스트 토큰과 숫자를 패널에서 조회한다.
    token() { return this.#token; }
    amount() { return this.#amount; }
    // 원본 점멸 요청은 표시를 즉시 바꾸지 않고 다음 150ms 뒤부터 갱신한다.
    flash() { this.#flash = 10; this.#until = this.#devices.now() + FLASH_INTERVAL; }
    // 스타일 체인을 교체하고 Push·부착 뒤 기존 깊이를 복구한다.
    #refresh() {
        const node = this.#root.node(this.#token), depth = node.depth;
        this.#text.clear(this.#token); this.#root.tree().push(this.#token);
        if (!(this.#flash & 1)) {
            if (!this.#devices.editor()) {
                const color = this.#amount < 1001 ? 'r' : this.#amount < 2001 ? 'y' : 'w';
                this.#text.style(this.#token, `~3~E~${color}${this.#amount}~[I${this.#devices.backgroundType()}.3]`);
                this.#text.fit(this.#token);
            } else this.#text.style(this.#token, '');
        }
        this.#root.attach(this.#token, 0, 2, 0, 2); node.depth = depth; this.#devices.changed();
    }
    // 원본 한 줄 생성·정렬·idle 등록과 최초 0 표시를 만든다.
    create(parent) {
        if (this.#token) throw new Error('생산 예산 중복 생성');
        this.#token = this.#text.allocate(() => this.idle()); this.#text.construct(this.#token, 0, 0, 0, 0);
        const node = this.#root.node(this.#token); node.vtable = 0x506948; node.alignFlags |= 4;
        this.#root.lifetime().setIdle(this.#token, true); this.#root.attach(this.#token, 0, 0, parent, 2); this.#refresh(); return this.#token;
    }
    // 원본 순서로 점멸 후 목표 금액을 갱신한다. fort 모드는 목표값으로 즉시 이동한다.
    idle() {
        const target = this.#devices.known() ? this.#devices.money() | 0 : 0;
        if (this.#flash && this.#until <= this.#devices.now()) { this.#flash -= 1; this.#until = this.#devices.now() + FLASH_INTERVAL; this.#refresh(); }
        if (this.#amount !== target) {
            const distance = target - this.#amount;
            this.#amount += distance < 0 ? (distance < -300 ? -100 : distance < -30 ? -10 : -1) : (distance > 300 ? 100 : distance > 30 ? 10 : 1);
            if (this.#devices.fort()) this.#amount = target;
            this.#refresh();
        }
    }
}
