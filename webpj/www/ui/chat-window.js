// 원본 ChatWindow(0x13c 바이트 root)의 네 실제 자식(helper·배경·입력줄 Console·로그 ScrollingText) 생성, 명령 등록(clear/quit), 입력줄 접두(수신자 목록) 편집, 사건 처리, 로그 추가, 도우미 메뉴를 공통 Gump/텍스트 계층에 연결한다(cpppj client/ChatWindow.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// root 자체는 그리지 않으며 재부착(0042e6e0)은 이미 이식한 HudChatGeometry 본체에 맡긴다. 실제 자식 저장소/문자 버퍼/편집 본체/번역/메뉴/Chatmanager 연결은 호스트 훅이고, 도달하지 않은 효과는 요구하지 않는다(미연결은 Error).
// 사건/모드/색/종류/포인터 전역은 사용하는 지점마다 `hooks.global(주소)` 로 현재 값을 다시 읽는다. 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// 사용: const window = new ChatWindow(new ChatWindowState(), hooks); const token = window.create(); window.handle(token, event, first, second, text); window.appendLog(token, '글', context8);
// (2026-11-14 WB04-E447: RawMenuControls 선행 ChatWindow 이식)
import { RawPlayerIdentity } from '../core/raw-player-identity.js';
import { cameraFtolLow } from '../render/camera-view.js';

// 실제 10.78 가상 표와 기본 비트/사건 마스크다: Chat root 가상 표, 표시 비트, 코드 마스크, 명령 사건 비트, 해제 비트.
const CHAT_VTABLE = 0x504a94; const VISIBLE = 1; const CODE_MASK = 0x03ffffff; const COMMAND = 0x02000000; const RELEASE = 0x80000000;

// 현재 root(548740)/clear·quit 명령 등록 여부(54da91 부근)/자동완성 목록 초기화 여부와 세 자동완성 문자열 목록(색 0054de88/배경 0054dea8/아이콘 0054de98)이다. Client 보다 짧게 살면 안 된다.
export class ChatWindowState {
    // 현재 root 토큰과 clear/quit 등록 여부, 문자열 목록 초기화 여부다.
    current = 0; registered = 0; completionReady = 0;
    // ~[C.n] 텍스트 색 / ~[B.n] 배경 색 / ~[I<type>.<frame>] 아이콘 완성 문자열 목록이다.
    colors = []; backgrounds = []; icons = [];
}

// 도달한 장치를 빈 성공으로 바꾸지 않는다(C++ logic_error → Error).
function need(hook) { if (typeof hook !== 'function') throw new Error('ChatWindow의 실제 하위 연결 누락'); return hook; }

// 원본 C문자열은 첫 NUL 까지만 사용한다(바이트 문자열).
function z(value) { const at = value.indexOf('\0'); return at < 0 ? value : value.slice(0, at); }

// 실제 편집 API 가 빌리는 안정된 NUL 포함 바이트 입력이다.
function terminated(value) {
    const text = z(value); const bytes = new Uint8Array(text.length + 1);
    // 문자열의 각 바이트를 복사한다.
    for (let index = 0; index < text.length; index += 1) bytes[index] = text.charCodeAt(index) & 0xff;
    return bytes;
}

// 바이트 배열의 NUL 앞 부분을 바이트 문자열로 바꾼다(NUL 이 없으면 전체).
function stringOf(bytes) { let value = ''; for (let index = 0; index < bytes.length && bytes[index]; index += 1) value += String.fromCharCode(bytes[index]); return value; }

// DWORD 감김 뺄셈이다.
const sub = (first, second) => (first - second) | 0;

// 이름 lookup 에 쓰는 실제 subscriber DWORD 만 원문 record+4 에 쓴다(비정렬 little endian).
function writeSubscriber(record, value) { for (let index = 0; index < 4; index += 1) record[4 + index] = (value >>> (index * 8)) & 0xff; }

// 한 Client 의 ChatWindow 본체다. hooks = { chat(token), base(token), console(token), text(token), buffer(token)(Console+108 의 실제 0x28 버퍼), entry(실제 TextEntryGump), geometry(실제 HudChatGeometry), global(주소), allocate(크기) → 토큰,
//   constructBase(token, w, h), constructHelper(token, label, color), fitHelper(token), title/help(token, text), translate(키), measure(token, 문자) → [폭, 높이], constructBackground(token, w, h, flags),
//   constructScrolling(token, w, h, font, color), replaceBuffer(token, buffer, 0), sharedBuffer(), setSlider/setScrollable(token, 값), attach(token, x, y, parent, mode), push/destructBase(token),
//   registerCommand(이름, callback, flag), bufferAppendCallback(buffer, callback), frameCount(type), frameHeight(type, frame), requestScroll/relativeScroll(token, 값), requestBottom(token), atBottom(token),
//   appendLog(token, 글), baseHandle(token, e, f, s), bubble(token, a, b, c), recipient(이름) → boolean, sendChat(manager, 글), resolve(record), log(수준, 서식, 값), config(키),
//   menuBegin/menuFinalize(), menuTitle(글), menuItem(라벨, 사건, 인자|null), menuChecked(체크, 라벨, 사건, 인자), menuTake() → 하위 메뉴, menuSubmenu(라벨, 하위), menuOpen(x, y, 참조), assertion(줄) } 이다.
export class ChatWindow {
    // Client 수명의 현재 전역과 자식/장치 경계다.
    #state; #hooks;
    // 현재 root/기본 조회만 미리 요구한다(C++ Need).
    constructor(state, hooks) { need(hooks?.chat); need(hooks.base); this.#state = state; this.#hooks = hooks; }

    // 실제 base 주소와 root 파생 주소는 같은 객체여야 한다(없거나 다르면 Error).
    #node(token) {
        const node = token ? this.#hooks.chat(token) : null;
        if (!node || node !== this.#hooks.base(token)) throw new Error('ChatWindow 공유 root 없음');
        return node;
    }

    // 실제 사용 시점의 자식 포인터를 읽는다.
    #base(token) {
        const node = token ? this.#hooks.base(token) : null;
        if (!node) throw new Error('ChatWindow 실제 자식 없음');
        return node;
    }

    // Console 은 같은 기본 Gump 저장소를 공유한다.
    #console(token) {
        const child = this.#node(token).console; const node = child ? need(this.#hooks.console)(child) : null;
        if (!node || node !== this.#base(child)) throw new Error('ChatWindow 실제 Console 없음');
        return node;
    }

    // 로그도 같은 실제 ScrollingText 저장소여야 한다.
    #text(token) {
        const child = this.#node(token).text; const node = child ? need(this.#hooks.text)(child) : null;
        if (!node || node !== this.#base(child)) throw new Error('ChatWindow 실제 ScrollingText 없음');
        return node;
    }

    // 원본처럼 각 호출마다 현재 Console+108 의 편집 버퍼를 다시 읽는다.
    #buffer(token) {
        const address = this.#console(token).buffer; const buffer = address ? need(this.#hooks.buffer)(address) : null;
        if (!buffer) throw new Error('ChatWindow 현재 편집 버퍼 없음');
        return buffer;
    }

    // 포인터만 채운 임시 Console 로 성공하지 않게 실제 편집 본체를 요구한다.
    #entry() { if (!this.#hooks.entry) throw new Error('ChatWindow 실제 TextEntry 본체 없음'); return this.#hooks.entry; }

    // 사건·색·모드·원문 포인터를 사용하는 위치마다 현재 값을 조회한다(부호 없는 32비트).
    #global(address) { return need(this.#hooks.global)(address) >>> 0; }

    // 실제 CopyText(128) 결과를 NUL 까지 읽는다. 전체 NUL 이 128바이트를 넘는 원문 stack overflow 는 Error 로 거부한다.
    #copyInput(token) {
        const bytes = new Uint8Array(128);
        if (this.#entry().copyText(this.#node(token).console, bytes) === 0xffffffff) throw new Error('ChatWindow 원본 입력 복사128 범위');
        if (bytes.indexOf(0) < 0) throw new Error('ChatWindow 원본 입력 NUL 없음');
        return stringOf(bytes);
    }

    // 현재 128바이트 원문 접두에만 쓴다(미작성 꼬리는 덮지 않고 128바이트 이상이면 Error).
    #prefix(token, value) {
        const text = z(value); const prefix = this.#node(token).inputPrefix;
        if (text.length >= prefix.length) throw new Error('ChatWindow 원본 접두128 범위');
        for (let index = 0; index < text.length; index += 1) prefix[index] = text.charCodeAt(index) & 0xff;
        prefix[text.length] = 0;
    }

    // 0043f820/004bc060: 실제 자식 생성과 같은 버퍼 소유 교체를 원문 순서대로 연결한다: root 기본 생성(float 폭/높이)·가상 표·종류(signed BYTE 부호 확장) → helper(0x170, ">", 갈색)·크기 맞춤·제목/도움말 설치 →
    // root 크기(float32, 배율 곱은 배정도) → 글자 'M' 측정 → 배경(0xb8) → Console(0x128, 폭-20, 측정 높이, 초기 없음, 글꼴 5)·변경 통지 1·최대 길이 80 → 로그(0xd8) → 공유 버퍼 교체·콜백 1 → 네 자식 부착 →
    // Slider/스크롤 설정 → Console 단일 줄·숨김·root 플래그 0xe00·접두 비움·본문 비움·접두 정규화·자동완성 0x42e540 → 최초 한 번만 완성 목록 구성.
    construct(token, width, height) {
        const hooks = this.#hooks; const node = this.#node(token); need(hooks.constructBase)(token, Math.fround(width), Math.fround(height));
        node.vtable = CHAT_VTABLE; node.command = (((this.#global(0x540424) & 0xff) << 24) >> 24) >>> 0;
        let child = need(hooks.allocate)(0x170); if (child) need(hooks.constructHelper)(child, '>', this.#global(0x5acd24)); node.helper = child; need(hooks.fitHelper)(node.helper);
        const helperTitle = need(hooks.translate)('Chat Helper'); need(hooks.title)(this.#node(token).helper, helperTitle); need(hooks.help)(this.#node(token).helper, 'chathelper');
        node.baseWidth = Math.fround(width); node.width = Math.fround(width * node.scale); node.baseHeight = Math.fround(height); node.height = Math.fround(height * node.scale); node.helperVisible = 1;
        const metric = need(hooks.measure)(token, 'M'); node.capacity = 128; node.leftPadding = 0; child = need(hooks.allocate)(0xb8);
        if (child) need(hooks.constructBackground)(child, Math.fround(sub(width, 20)), Math.fround(sub(height, metric[1])), (this.#global(0x5b5dc0) | 0x04000000) >>> 0); node.background = child;
        child = need(hooks.allocate)(0x128); if (child) this.#entry().constructConsole(child, sub(width, 20), metric[1], null, 5, (this.#global(0x5b5e60) | 0x04000000) >>> 0, this.#global(0x5b5dd4));
        node.console = child; this.#console(token).changeNotice = 1; this.#console(token).maxLength = 80;
        child = need(hooks.allocate)(0xd8); if (child) need(hooks.constructScrolling)(child, sub(width, 20), sub(height, metric[1]), 5, this.#global(0x5b5dd8)); node.text = child;
        need(hooks.replaceBuffer)(this.#node(token).text, need(hooks.sharedBuffer)(), 0); this.#text(token).callback = 1;
        need(hooks.attach)(this.#node(token).background, Math.fround(this.#node(token).leftPadding), 0, token, 0);
        need(hooks.attach)(this.#node(token).console, Math.fround(this.#node(token).leftPadding), Math.fround(sub(height, metric[1])), token, 0);
        need(hooks.attach)(this.#node(token).helper, Math.fround(this.#node(token).leftPadding), Math.fround(sub(height, metric[1])), token, 0);
        need(hooks.attach)(this.#node(token).text, 0, 0, this.#node(token).background, 0); need(hooks.setSlider)(this.#node(token).text, 1); need(hooks.setScrollable)(this.#node(token).text, 1);
        { const text = this.#text(token); text.textFlags = (text.textFlags & ~0x10) >>> 0; }
        this.#console(token).multiline = 0; { const console = this.#console(token); console.flags = (console.flags & ~VISIBLE) >>> 0; }
        node.flags = (node.flags | 0xe00) >>> 0; node.inputPrefix[0] = 0; this.#entry().setText(this.#node(token).console, terminated('')); this.normalizePrefix(token); this.#console(token).autocomplete = 0x42e540;
        if (!this.#state.completionReady) this.#completions(); this.#state.completionReady = 1; return token;
    }

    // 00431ee0: clear/quit 명령은 처음 한 번만 등록하고, 200×100 root 를 만들어 현재 root 로 저장하며 같은 현재 로그 버퍼에 실제 append callback(0x42f590)을 설치한다. 할당/생성 실패(0)는 Error 다.
    create() {
        const hooks = this.#hooks; const state = this.#state;
        if (!state.registered) { need(hooks.registerCommand)('clear', 0x42f680, 1); need(hooks.registerCommand)('quit', 0x42e520, 1); state.registered = 1; }
        const token = need(hooks.allocate)(0x13c); state.current = token ? this.construct(token, 200, 100) : 0; if (!state.current) throw new Error('ChatWindow 원본 new 실패 뒤 null 자식 접근');
        need(hooks.bufferAppendCallback)(this.#text(state.current).buffer, 0x42f590); return state.current;
    }

    // 동결된 0042e6e0 본체(HudChatGeometry)에 위임한다(없으면 Error).
    attach(token, x, y, parent, mode) { if (!this.#hooks.geometry) throw new Error('ChatWindow 실제 Attach 본체 없음'); this.#hooks.geometry.attach(token, x, y, parent, mode); }

    // 0042eca0: 원문은 로그 자식 문맥(8 DWORD)을 저장하지만 지정/복원은 부모(root)의 문맥에 쓴다(자식 스타일을 고쳐 원문 동작을 바꾸지 않는다). 첫 자식과 32바이트를 하위 호출 전에 고정하고, context 가 있으면 정확히 8개여야 하며(아니면 TypeError),
    // 로그 추가 전 현재 맨 아래 판정을 하고 추가한 뒤 맨 아래였다면 맨 아래 요청을 하며, 정상 복귀에서만 저장한 문맥을 복원한다(하위 예외에서 앞선 부분 효과를 취소하지 않는다).
    appendLog(token, value, context = []) {
        const hooks = this.#hooks; const child = this.#node(token).text; const saved = Uint32Array.from(this.#base(child).textContext);
        if (context.length) {
            if (context.length !== 8) throw new TypeError('Chat 로그 문맥8DWORD 계약');
            this.#base(token).textContext.set(Uint32Array.from(context));
        }
        const bottom = need(hooks.atBottom)(child); need(hooks.appendLog)(this.#node(token).text, value);
        if (bottom) need(hooks.requestBottom)(this.#node(token).text);
        this.#base(token).textContext.set(saved);
    }

    // 두 256색 목록(~[C.n]/~[B.n])을 만들고(호출마다 256개씩 추가한다) 이미 초기화됐으면 끝낸다. 처음에는 TYPE 수(전역 541348, 매 반복 다시 읽음)만큼 각 TYPE 프레임 코드 수(SHP 추가 레이어 수와 구별)를 안쪽 반복 전에 한 번 읽고 프레임 높이 8..16 만 ~[I<type>.<frame>] 후보로 저장한다.
    #completions() {
        const hooks = this.#hooks; const state = this.#state;
        // 두 실제 256개 문자열 목록이다.
        for (let color = 0; color < 256; color += 1) { state.colors.push(`~[C.${color}]`); state.backgrounds.push(`~[B.${color}]`); }
        if (state.completionReady) return;
        for (let type = 0; type < (this.#global(0x541348) | 0); type += 1) {
            const count = need(hooks.frameCount)(type) | 0;
            // 높이 8..16 만 아이콘 후보로 저장한다.
            for (let frame = 0; frame < count; frame += 1) { const height = (need(hooks.frameHeight)(type, frame) << 16) >> 16; if (height > 7 && height < 17) state.icons.push(`~[I${type}.${frame}]`); }
        }
    }

    // 0042ebc0: 접두와 본문은 실제 입력을 두 번 복사한 뒤 다시 합친다. 접두 = 입력의 첫 ':' 앞(없으면 비움)+비어 있지 않으면 ':', 본문 = 첫 ':' 뒤(없으면 전체). 입력을 접두로 되쓰고 본문을 끝에 붙이며 원문 접두 버퍼에도 쓴다.
    normalizePrefix(token) {
        let prefix = this.#copyInput(token); const colon = prefix.indexOf(':'); prefix = colon < 0 ? '' : prefix.slice(0, colon); if (prefix) prefix += ':';
        let body = this.#copyInput(token); const second = body.indexOf(':'); if (second >= 0) body = body.slice(second + 1);
        this.#entry().setText(this.#node(token).console, terminated(prefix)); this.#entry().appendText(this.#node(token).console, terminated(body), false); this.#prefix(token, prefix);
    }

    // 0042ea60: 수신자 목록은 대소문자를 바꾸지 않는 부분 문자열/공백 구분 문자열이다. 작업 2 이상은 진단(0x561)을 알리고(장치가 있을 때) 계속하며 이름이 없으면(null) 끝낸다. operation 0 은 없으면 공백 뒤에 이름+공백을 더하고, 1 은 처음 찾은 곳부터 다음 비공백까지 지운다.
    changeRecipient(token, name, operation) {
        const hooks = this.#hooks;
        if (operation > 1 && hooks.assertion) hooks.assertion(0x561);
        if (name === null || name === undefined) return;
        const target = z(name); let prefix = this.#copyInput(token); const colon = prefix.indexOf(':'); let valid = true;
        if (colon >= 0) { prefix = prefix.slice(0, colon); valid = need(hooks.recipient)(prefix); } else prefix = '';
        if (!valid) prefix = '';
        const found = prefix.indexOf(target);
        if (operation === 0 && found < 0) { if (prefix && !prefix.endsWith(' ')) prefix += ' '; prefix += target; prefix += ' '; }
        else if (operation === 1 && found >= 0) {
            let end = found; while (end < prefix.length && prefix[end] !== ' ') end += 1; while (end < prefix.length && prefix[end] === ' ') end += 1; prefix = prefix.slice(0, found) + prefix.slice(end); // 먼저 찾은 부분 문자열에서 다음 비공백까지 지운다.
        }
        if (prefix) prefix += ':';
        let body = this.#copyInput(token); const bodyColon = body.indexOf(':'); if (bodyColon >= 0 && valid) body = body.slice(bodyColon + 1);
        this.#entry().setText(this.#node(token).console, terminated(prefix)); this.#entry().appendText(this.#node(token).console, terminated(body), false); this.#prefix(token, prefix);
    }

    // 0042e9e0: 실제 입력을 128바이트 지역으로 복사하고 유효한 접두만 남긴 뒤 부분 문자열을 대소문자 구분 검색한다(빈 이름은 빈 접두에서도 일치하며 이름 경계를 추가하지 않는다).
    containsRecipient(token, name) {
        let input = this.#copyInput(token); const colon = input.indexOf(':');
        if (colon < 0) input = ''; else { input = input.slice(0, colon); if (!need(this.#hooks.recipient)(input)) input = ''; }
        return input.indexOf(z(name)) >= 0;
    }

    // 0042fab0: command 사건 분기에는 일반 해제 관문을 추가하지 않으며 기본 Help 위임 순서도 원문을 따른다. 일반 사건은 PageUp/PageDown/Home/End/방향키만 로그 스크롤로 처리하고(좌우 버튼·해제 제외),
    // 명령 사건은 삽입(문자열 필요)/helper 메뉴(helper 의 serial 일치)/부모 전달(현재 root 있을 때)/입력줄 제출(serial 일치 → 개행 추가·길이 -2 가 아니면 전송·접두 정규화)/수신자 추가·제거(subscriber 조회)/재부착 순서로 처리한다.
    handle(token, event, first, second, text = undefined) {
        const hooks = this.#hooks; const state = this.#state; const node = this.#node(token); const code = (event & CODE_MASK) >>> 0;
        const base = () => need(hooks.baseHandle)(token, event, first, second);
        if (!(event & COMMAND)) {
            if (code !== 0x10000 && code !== 0x20000 && !(event & RELEASE)) {
                if (code === 0x240000) { need(hooks.requestScroll)(this.#node(token).text, 0); return 1; }
                if (code === 0x230000) { need(hooks.requestBottom)(this.#node(token).text); return 1; }
                if (code === 0x210000 || code === 0x220000 || code === 0x260000 || code === 0x280000) {
                    const delta = code === 0x210000 ? cameraFtolLow(-node.baseHeight) : code === 0x220000 ? cameraFtolLow(node.baseHeight) : code === 0x260000 ? -5 : 5;
                    need(hooks.relativeScroll)(this.#node(token).text, delta); return 1;
                }
            }
            return base();
        }
        if (code === this.#global(0x542b24)) {
            if (text === null || text === undefined) throw new Error('ChatWindow 삽입 사건의 실제 문자열 없음');
            const value = z(text);
            if (value === 'copy') this.#buffer(token).copy(); else if (value === 'paste') this.#buffer(token).paste(); else this.#entry().appendText(this.#node(token).console, terminated(value), false);
        }
        if (code === this.#global(0x5427a8) && (first >>> 0) === (this.#base(this.#node(token).helper).serial >>> 0)) this.#helperMenu();
        if (code === this.#global(0x5429f0)) { if (state.current) need(hooks.bubble)(state.current, this.#global(0x542914), 0, 1); return 1; }
        if (code === this.#global(0x5427d8) && (first >>> 0) === (this.#base(this.#node(token).console).serial >>> 0)) {
            this.#entry().appendText(this.#node(token).console, terminated('\n'), false);
            if (this.#buffer(token).state().length !== -2) need(hooks.sendChat)(this.#global(0x548714), this.#buffer(token).text());
            let prefix = this.#copyInput(token); const colon = prefix.indexOf(':');
            if (colon >= 0) { prefix = prefix.slice(0, colon); if (need(hooks.recipient)(prefix)) prefix += ':'; else prefix = ''; } else prefix = '';
            this.#prefix(token, prefix); this.#entry().setText(this.#node(token).console, terminated(prefix)); this.normalizePrefix(token); return 1;
        }
        const add = code === this.#global(0x542794); const remove = !add && code === this.#global(0x542af8); // 원본은 add 가 아니어야 remove 사건을 조회한다.
        if (add || remove) {
            const record = new Uint8Array(60); RawPlayerIdentity.initialize(record, 0); writeSubscriber(record, second);
            if (need(hooks.resolve)(record)) {
                const end = record.indexOf(0, 8); const stop = end < 0 || end > 48 ? 48 : end; let name = '';
                // 레코드 +8 부터 최대 40바이트의 이름을 읽는다.
                for (let index = 8; index < stop; index += 1) name += String.fromCharCode(record[index]);
                this.changeRecipient(token, name, remove ? 1 : 0); return 1;
            }
            need(hooks.log)(0x13, 'Could not lookup a name for subId == %d\n', first); return 1;
        }
        if (code === this.#global(0x54298c)) { need(hooks.push)(token); need(hooks.attach)(token, node.x, node.y, 0, 0); }
        return base();
    }

    // 0042e8d0: 원본은 현재 전역 root 를 조건 없이 0 으로 쓰고 공통 자식 우선 수명에 나머지를 맡긴다.
    destruct(token) { this.#node(token).vtable = CHAT_VTABLE; this.#state.current = 0; need(this.#hooks.destructBase)(token); }

    // 실제 helper 메뉴는 하위 메뉴를 완성한 뒤 현재 controlled 스택에 연결한다. 번역 호출 뒤에 현재 사건 전역을 읽고 체크 항목은 설정을 두 번(할당 문자열용/체크 상태용) 조회한다. 메타맵/전투 모드일 때만 서식/효과/채널/플레이어/채팅 설정 항목이 있다.
    #helperMenu() {
        const hooks = this.#hooks; const state = this.#state; const global = (address) => this.#global(address);
        const begin = () => need(hooks.menuBegin)(); const tr = (value) => need(hooks.translate)(value);
        const item = (label, command, payload) => need(hooks.menuItem)(label, command, payload === undefined ? null : payload); // include/enabled 1·force 0 이다.
        const translatedItem = (label, address, payload) => { const translated = tr(label); const command = global(address); item(translated, command, payload); };
        const sub = (label) => { const translated = tr(label); const child = need(hooks.menuTake)(); need(hooks.menuSubmenu)(translated, child); }; // 원문은 번역 뒤 실제 현재 메뉴를 pop 한다.
        const toggle = (key, label) => {
            const first = need(hooks.config)(key); const assignment = key + (first ? '=0' : '=1'); const translated = tr(label); const command = global(0x5428ac); const checked = need(hooks.config)(key);
            need(hooks.menuChecked)(checked >>> 0, translated, command, assignment); // 원문은 두 번째 설정 조회 전에 사건값을 스택에 저장한다.
        };
        begin(); need(hooks.menuTitle)(tr('Chat Helper:'));
        if (global(0x594fc8) || global(0x594fbc)) {
            begin(); translatedItem('Edit Back Format', 0x542b08, null); translatedItem('Edit Front Format', 0x542b0c, null); sub('Format');
            begin(); toggle('allowzonesounds', 'Allow Chat Sound'); toggle('zonebgcolor', 'Allow Background Colors'); toggle('zonefgcolor', 'Allow Foreground Colors'); toggle('zoneoffset', 'Allow Embossed Offset'); sub('Chat Effects ');
            if (global(0x594fbc)) {
                begin(); begin(); toggle('ShowEveryone', 'Show General Channel'); toggle('ShowAllies', 'Show Allies Channel'); toggle('ShowObserver', 'Show Observer Channel'); toggle('ShowWisper', 'Show Whisper Channel'); sub('Channel Controls');
                begin(); toggle('ShowMyAllies', 'Show Allies Chat'); toggle('ShowMyEmenys', 'Show Enemies Chat'); toggle('ShowMyNeutrals', 'Show Observers Chat'); sub('Player Controls'); toggle('ShowChatWindow', 'Open Window on Message'); toggle('PlaySoundMessage', 'Play sound on Message'); sub('Chat Controls');
            }
        }
        begin(); begin();
        // 색/아이콘의 실제 그룹 크기와 마지막 제목 수식을 유지한다. 비어 있으면 Error, 마지막 원소는 항목에서 제외한다.
        const grouped = (list, step, prefix, title) => {
            if (list.length === 0) throw new Error('ChatWindow 원본 helper의 빈 자동완성 벡터 범위');
            let secondGroup = false;
            // 원문은 매 반복 현재 size 를 읽고 마지막 원소를 제외한다.
            for (let index = 0; index < ((list.length - 1) >>> 0); index += 1) {
                if (!index) begin(); const value = list[index]; const label = `${value}~${value}`; item(label, global(0x542b24), value);
                if (index && index % step === 0) { const first = index - (step === 23 || secondGroup ? step - 1 : step); secondGroup = true; sub(`${prefix}${first} to ${index}`); begin(); }
            }
            const size = list.length >>> 0; sub(`${prefix}${size - ((size - 1) % step)} to ${(size - 2) >>> 0}`); sub(title); begin();
        };
        grouped(state.colors, 21, 'Colors ', 'Text Colors'); grouped(state.backgrounds, 21, 'BGColors ', 'Background Colors'); grouped(state.icons, 23, 'Icons ', 'Icons');
        item('Mimic', global(0x542b24), '~[]'); item('Sound', global(0x542b24), '~[S.name]'); item('Icon', global(0x542b24), '~[I.name]'); item('Emboss Offset', global(0x542b24), '~[E.X#xY#]'); sub('KeyWords'); begin();
        const styles = [['~BBold', '~B'], ['~EEmboss', '~E'], ['~IItalic', '~I'], ['~SStrikeout', '~S'], ['~UUnderline', '~U'], ['~.Reset Style', '~.'], ['~NReset Color & Emboss', '~N'], ['~*Reset to Default', '~*']]; // 실제 스타일 항목 순서다.
        for (const [label, value] of styles) translatedItem(label, 0x542b24, value); sub('Styles'); begin(); // 개별 기본 스타일 문자열을 입력에 삽입한다.
        const colors = [['~kBlack', '~k'], ['~xBrown', '~x'], ['~qDark Tan', '~q'], ['~jLight Tan', '~j'], ['~rRed', '~r'], ['~gGreen', '~g'], ['~uLight Green', '~u'], ['~dTeal', '~d'], ['~bBlue', '~b'], ['~lLight Blue', '~l'],
            ['~mMagenta', '~m'], ['~sDark Magenta', '~s'], ['~pPink', '~p'], ['~yYellow', '~y'], ['~iLight Yellow', '~i'], ['~oOrange', '~o'], ['~aLight Orange', '~a'], ['~wWhite', '~w'], ['~tSilver', '~t'], ['~nReset Color', '~n']]; // 실제 20개 미리 정의된 색 문자열이다.
        for (const [label, value] of colors) translatedItem(label, 0x542b24, value); sub('Prefabricated Colors'); begin(); // 임의 RGB 팔레트 이름으로 재계산하지 않는다.
        for (const value of ['.front', '.back', '.format']) translatedItem(value, 0x542b24, value); sub('Commands'); sub('Insert'); translatedItem('Copy', 0x542b24, 'copy'); translatedItem('Paste', 0x542b24, 'paste');
        need(hooks.menuFinalize)(); need(hooks.menuOpen)(global(0x5cae94), global(0x5cae98), global(0x563e38));
    }
}
