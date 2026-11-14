// controlled 메뉴의 문자열/체크/제목/머리/하위 메뉴/구분선 작성 래퍼를 같은 메뉴 저장소의 기존 항목 추가·제목 파서에 연결한다.
// (cpppj client/MenuGumpControls.cpp/.h, 기준선 1f00a25a, 10.78 전용 — CD 10.72 의 서식 없는 전달·명령 0 처리와 CD 전용 자식 문맥 복사/강조 지우기 helper 는 옮기지 않는다)
// 패치는 현재 설정에서 전경→배경 색을 읽어 `~[C.%d]~[B.%d]~E%s` 접두를 붙인다. 설정 조회는 현재 controlled 칸을 바꿀 수 있으므로 실제 Append 직전에 칸을 다시 읽는다.
// 문자열은 원본의 NUL 종료 바이트 입력(바이트 하나가 문자 하나인 바이너리 문자열 또는 Uint8Array)이다. 번호 인자/임의 printf 서식과 포인터 alias 는 별도 공용 서식기 범위다.
// 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// (2026-11-14 WB04-E429: RawMenuControls 선행 MenuGumpControls 이식)

// 항목 flags: 문자열/스타일 문자열/명시 인자/체크/비활성 비트다.
const TEXT = 0x10000; const STYLED = 0x400; const PARAMETER = 0x80; const CHECKED = 4; const DISABLED = 0x200;
// 메뉴 flags: 장식 비트다.
const FANCY = 0x800000;
// 각 래퍼가 전달하는 기본 flags 다. 일반 항목(문자열+스타일+인자), 하위 메뉴(문자열+스타일), 구분선(문자열+0x10)이다.
const NORMAL_ITEM = TEXT | STYLED | PARAMETER; const SUBMENU_ITEM = TEXT | STYLED; const SEPARATOR_ITEM = 0x10010;
// 원본 스택 포맷 버퍼 크기(NUL 포함)다.
const FORMAT_BYTES = 256;

// 선택된 장치가 없으면 같은 지점의 TypeError(C++ std::invalid_argument)로 거부한다.
function need(fn, message) { if (typeof fn !== 'function') throw new TypeError(message); return fn; }
// 원문 바이트 문자열(바이트 하나가 문자 하나) 또는 Uint8Array 를 첫 NUL 앞까지의 바이트 문자열로 바꾼다. 원본 문자열은 첫 NUL 에서 끝난다.
function cString(text) {
    let value = typeof text === 'string' ? text : Array.from(text, (byte) => String.fromCharCode(byte)).join('');
    const nul = value.indexOf('\0'); if (nul >= 0) value = value.slice(0, nul); return value;
}
// 원본 256바이트 지역 결과를 넘는 입력만 거부한다. NUL 까지 들어갈 1바이트를 확보한다.
function bounded(value) { if (value.length >= FORMAT_BYTES) throw new RangeError('메뉴 원본 포맷 버퍼 한도 초과'); return value; }
// 실제 추가 인자가 없는 원본 포맷 경로다. %% 는 한 '%' 로 출력하고 다른 '%' 변환은 이 래퍼가 원본에서 인자를 넘기지 않는 무효 스택 입력이므로 호스트에서 거부한다.
function noArguments(source) {
    source = cString(source); let result = '';
    // 출력 순서와 %% 소비를 유지하며 원본 버퍼 밖 결과를 쓰기 전에 확인한다.
    for (let index = 0; index < source.length; index += 1) {
        if (source[index] === '%') {
            if (index + 1 >= source.length || source[index + 1] !== '%') throw new TypeError('무인자 메뉴 서식에 변환 인자 없음');
            index += 1;
        }
        result += source[index];
    }
    return bounded(result);
}

// 원본 controlled 래퍼의 정상 몸체다. 기존 제목 파서/항목 Append 를 공유하며 자동 Client 조립은 후속이다.
// 사용: new MenuGumpControls(createState, menuState, menu, layout, titleMarkup, { node, base, readInt }).addText('열기', 5)
export class MenuGumpControls {
    // 같은 생성/controlled 상태·메뉴/레이아웃/제목 처리기와 현재 가상 경계다.
    #create; #state; #menu; #layout; #title; #hooks;
    // 판본·등록/controlled 표·같은 노드/기존 처리기는 래퍼보다 오래 살아야 하며 동일 상태를 사용해야 한다. 설정 경계는 실제 색 조회에 도달할 때 확인한다.
    constructor(create, state, menu, layout, title, hooks = {}) {
        if (typeof hooks.node !== 'function' || typeof hooks.base !== 'function') throw new TypeError('controlled 메뉴 공유 저장소 필요');
        this.#create = create; this.#state = state; this.#menu = menu; this.#layout = layout; this.#title = title; this.#hooks = hooks;
    }

    // 제목/자식은 메뉴 아닌 기본 파생 객체도 조회할 수 있다. 토큰 0 은 조회하지 않는다.
    #base(token) {
        const node = token ? this.#hooks.base(token) : null; if (!node) throw new Error('controlled 기본 노드 없음'); return node;
    }

    // controlled/표시/입력과 같은 실제 메뉴를 반환한다.
    #node(token) {
        const node = token ? this.#hooks.node(token) : null;
        if (!node || node !== this.#base(token)) throw new Error('controlled 메뉴 공유 노드 없음');
        return node;
    }

    // 패치의 null 현재 메뉴 assert 는 include 검사보다 먼저다. 현재 토큰을 돌려준다.
    #currentRequired() { const token = this.#menu.current(); this.#node(token); return token; }

    // 누락 설정도 두 번의 원본 조회를 수행하고 값은 각 0 초기값을 유지한다. 전경→배경 순서로 읽으며 앞선 조회가 바꾼 상태를 다음 조회/현재 칸에서 반영한다.
    #colors(foreground, background) {
        const read = need(this.#hooks.readInt, '메뉴 색 설정 ReadInt 경계 필요'); const first = { value: 0 }; const second = { value: 0 };
        read(foreground, first); read(background, second); return [first.value | 0, second.value | 0];
    }

    // 고정 원본 "~[C.%d]~[B.%d]~E%s" 의 signed DWORD 십진수 결과를 생성한다. 입력의 '%' 는 %s 인자로 들어가므로 재해석하지 않는다.
    #colored(text, colors) { return bounded(`~[C.${colors[0]}]~[B.${colors[1]}]~E${cString(text)}`); }

    // 현재 메뉴에 문자열 항목을 추가한다(00477cb0). include 가 false 면 생략, enabled 가 false 면 disabled 이며 접두는 enabled 또는 forceColor 에서만 붙인다.
    // 명령 0 은 command 비트를 세우지 않는다. 사용: addText('열기', 5, 0, true, true, false)
    addText(text, command = 0, parameter = 0, include = true, enabled = true, forceColor = false) {
        this.#currentRequired(); if (!include) return;
        const colors = this.#colors('MenuColor', 'MenuBGColor');
        const source = (enabled || forceColor) ? this.#colored(text, colors) : noArguments(text);
        this.#layout.append(this.#currentRequired(), source, NORMAL_ITEM | (command ? 1 : 0) | (enabled ? 0 : DISABLED), command, parameter, 0);
    }

    // 체크 항목을 추가한다(00477dd0). 체크 비트/명시 인자/조건/비활성 처리이며 패치는 command 0 에도 command 비트를 세운다.
    addChecked(checked, text, command, parameter = 0, include = true, enabled = true) {
        this.#currentRequired(); if (!include) return;
        const colors = this.#colors('MenuCheckedColor', 'MenuCheckedBGColor');
        const source = enabled ? this.#colored(text, colors) : noArguments(text);
        this.#layout.append(this.#currentRequired(), source, NORMAL_ITEM | 1 | (checked ? CHECKED : 0) | (enabled ? 0 : DISABLED), command, parameter, 0);
    }

    // 제목/머리의 256바이트 포맷과 fancy Parse 또는 현재 일반 항목 추가다. 원본 제목 슬롯/색·본문의 포맷 순서를 사용하며 fancy Parse 뒤 메뉴 Reflow 를 추가하지 않는다.
    #heading(text, title) {
        const colors = title ? this.#colors('MenuTitleColor', 'MenuTitleBGColor') : this.#colors('MenuHeaderColor', 'MenuHeaderBGColor');
        let source = bounded(`${title ? '~3~E' : '~E'}~[C.${colors[0]}]~[B.${colors[1]}]${cString(text)}~.`);
        const token = this.#currentRequired();
        if (this.#node(token).menuFlags & FANCY) {
            const target = this.#node(token).title; if (!target) throw new Error('fancy 메뉴 제목 없음');
            this.#title.parse(target, source); return;
        }
        source = this.#colored(source, this.#colors('MenuColor', 'MenuBGColor'));
        this.#layout.append(this.#currentRequired(), source, NORMAL_ITEM | DISABLED, 0, 0, 0);
    }

    // 제목은 패치 `~3~E`+제목 색을 붙이며 fancy 이면 현재 title 에 다중 줄 Parse 만 호출하고, 일반 메뉴는 disabled 항목으로 Append/Reflow 한다(00477ee0).
    addTitle(text) { this.#heading(text, true); }

    // 머리는 패치 `~E`+머리 색을 붙이며 제목과 같은 경로로 추가한다(00478020).
    addHeader(text) { this.#heading(text, false); }

    // 하위 메뉴 항목을 추가한다(00478160). 자식 포인터/체크/비활성 항목이며 패치는 항상 styled 이다. 자식 Attach/flags 0x20 은 실제 Append 에 맡긴다.
    addSubmenu(text, child, enabled = true, checked = false) {
        this.#currentRequired(); const colors = this.#colors('MenuSubColor', 'MenuSubBGColor');
        const source = enabled ? this.#colored(text, colors) : noArguments(text);
        this.#layout.append(this.#currentRequired(), source, SUBMENU_ITEM | (enabled ? 0 : DISABLED) | (checked ? CHECKED : 0), 0, 0, child);
    }

    // 구분선을 추가한다(00478260). 선두/연속/40개 한도는 실제 기존 Append 가 처리하고 미작성 명령/인자는 보존한다. 명령/인자 비트가 없어 전달 1 도 쓰이지 않는다.
    addSeparator() { this.#layout.append(this.#currentRequired(), '', SEPARATOR_ITEM, 1, 1, 0); }

    // 실제 메뉴 생성/소멸 개수가 0인지다(00475ef0). controlled 표/visible/dead 를 검사하지 않는다. 메뉴는 지연 Kill 뒤 실제 소멸 전까지 개수에 남는다.
    noMenus() { return this.#state.instances === 0; }
}

// 원본 전역 설정 ReadInt 경계를 기존 설정 인터페이스(GameData.readInt)에 연결한다. 조회 반환값(변경 여부)은 원본 wrapper 처럼 버리고 원본 설정 파일을 쓰지 않는다.
// 사용: bindMenuGumpControlConfig(hooks, gameData)
export function bindMenuGumpControlConfig(hooks, config) {
    // 누락 설정은 전달 초기값을 유지한다.
    hooks.readInt = (key, holder) => { holder.value = config.readInt(key, holder.value).value | 0; };
}
