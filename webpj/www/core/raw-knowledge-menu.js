// 원본 ShowTechnology(00492b60 Storm 카드, 00493210 World 카드, 004938c0 선택 메뉴)의 전체 기술 카드 필터·문자열·정렬·네 줄 배치를 같은 실제 Gump 에 작성하는 모듈이다(cpppj `client/RawKnowledgeMenu.cpp/.h`, 10.78 전용).
// 사용:
//   showKnowledgeMenu(types, state, hooks) → 만든 메뉴 토큰 배열(Storm: theme 0..3, 머리 WIND/RAIN/THUN./SUN)
//   showWorldKnowledgeMenu(types, state, hooks) → World 카드(theme 4..7, 머리 EARTH/FIRE/WATER./MOON, 아이콘 12..15)
//   showKnowledgeSideMenu(state, hooks) → Storm/World 선택 두 메뉴(300 폭 팝업 2개, 기술 보유·번역은 조회하지 않음)
//   knowledgeScriptCode(name) → ShowTechnology/CheatAllTechnology/ShowTecSideMenu/ShowTecMenu2/GetTechnology 명령 번호 또는 0xffffffff(ASCII 대소문자 무시)
//   types = 타입 레코드 배열(`group`·`flags2`·`theme`), state = { width, height, iconWidth, iconHeight, language, templeProduct, helpCommand }(createKnowledgeMenuState 로 기본값 생성)
//   hooks = { blocked(), cancel(), currentTechnology(), owns(parent, type), mana(type), description(type), inProduction(type), excluded(type), translate(text), begin() → 토큰,
//             node(token) → 같은 현재 메뉴 노드(flags/menuFlags/width/baseWidth/targetWidth 를 같은 주소에 읽고 씀), title(text), separator(), item(text, command, 인자(숫자 또는 'themeHelp' 문자열), enabled),
//             attach(x, y), push(token), pop(token), width(token, float), height(token, float), position(token) → {x, y}, mark(command) }
//   선택한 분기에서 도달한 효과만 필수이며 미연결은 성공으로 대체하지 않는다(Error). 문자열은 바이트 문자열이며 첫 NUL 뒤는 설명·마나에 포함하지 않는다.
// 원본 관측: `knowledge-menu-x86.tsv` 25관측과 `knowledge-world-menu-x86.tsv` 50관측(World 25·선택 25)을 tests/raw-knowledge-menu.test.mjs 가 대조한다. 기준선 뒤 cpppj 변경 없음(2026-11-15 확인). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-15 WB04-E537: 6단계 게임 메뉴 명령 표의 선행 부품 RawKnowledgeMenu 이식)
import { sortKnowledgeRecords } from '../ui/knowledge-sort.js';
import { asciiLower } from '../assets/config.js';
import { GET_TECHNOLOGY_COMMAND } from './get-technology-input.js';

// 보호 00542974 의 실제 스크립트/사제/F6 명령이다. (2026-11-15 WB04-E537)
export const SHOW_TECHNOLOGY_COMMAND = 0x020000a0;
// 00542b84/88 의 원문 Storm/World 선택 메뉴와 World 카드 명령이다. (2026-11-15 WB04-E537)
export const SHOW_TEC_SIDE_MENU_COMMAND = 0x02000132;
export const SHOW_TEC_MENU2_COMMAND = 0x02000133;
// 보호 명령표 00542798 의 CheatAllTechnology 원본 코드이다. (2026-11-15 WB04-E537)
export const CHEAT_ALL_TECHNOLOGY_COMMAND = 0x02000028;

// 원문 492b60/493210/4938c0 의 별도 일반 명령만 선택하며 기본 메뉴는 Storm 을 유지한다. (2026-11-15 WB04-E537)
export const KnowledgeMenuKind = Object.freeze({ Storm: 0, World: 1, Side: 2 });

// 원본 strlen 이 읽는 바이트 수이다. 한글처럼 ASCII 가 아닌 글자는 원본 문자 집합(한국어 CP949)에서 2바이트로 센다. (2026-11-21 WB04-E1142)
export function displayByteLength(text) {
    let length = 0;
    for (const letter of String(text)) length += letter.codePointAt(0) < 0x80 ? 1 : 2;
    return length;
}

// 카드 이름을 두 줄로 나눈다. 바이트 길이가 8 을 넘고 위치 1 보다 뒤에 공백/하이픈이 있으면 가장 뒤의 것부터 한 번만 자른다(첫 줄을 다시 줄이지 않는다).
// 영어 이름은 글자 수와 바이트 수가 같고, 한국어 이름은 글자당 2바이트라 5글자만 넘어도 나뉘어 칸 폭(약 72픽셀)에 들어간다. 이전에는 글자 수로 재서 8글자 이하 한국어 이름이 나뉘지 않아 칸에서 잘렸다.
// (2026-11-21 WB04-E1142)
export function splitKnowledgeName(description) {
    let first = String(description), second = '', splitName = false;
    for (let split = first.length; split > 0; split -= 1) {
        if (split < first.length && (first[split] === ' ' || first[split] === '-') && split > 1 && displayByteLength(first) > 8) {
            second = first.slice(split + 1);
            first = first.slice(0, split);
            splitName = true;
            break;
        }
    }
    return { first, second, splitName };
}

// 화면/언어/아이콘 치수와 원본 타입/명령 전역의 기본값이다(00531860/64·00540cbc/cc0·00542384/0054126c/00542754). 언어 3 은 설명 줄을 비운다. (2026-11-15 WB04-E537)
export function createKnowledgeMenuState(overrides = {}) {
    return { width: 640, height: 480, iconWidth: 65, iconHeight: 60, language: 1, templeProduct: 133, helpCommand: 0x02000017, ...overrides };
}

// 원본 이름표의 ShowTechnology/CheatAllTechnology/ShowTecSideMenu/ShowTecMenu2/GetTechnology 를 ASCII 대소문자 무시로 조회한다. 미등록은 원본 0xffffffff 이다. (2026-11-15 WB04-E537)
export function knowledgeScriptCode(name) {
    const lower = asciiLower(name);
    if (lower === 'showtecsidemenu') return SHOW_TEC_SIDE_MENU_COMMAND;
    if (lower === 'showtecmenu2') return SHOW_TEC_MENU2_COMMAND;
    if (lower === 'gettechnology') return GET_TECHNOLOGY_COMMAND;
    if (lower === 'showtechnology') return SHOW_TECHNOLOGY_COMMAND;
    return lower === 'cheatalltechnology' ? CHEAT_ALL_TECHNOLOGY_COMMAND : 0xffffffff;
}

// 선택된 실제 효과의 누락을 빈 성공으로 숨기지 않는다. (2026-11-15 WB04-E537)
function need(effect) {
    if (typeof effect !== 'function') throw new Error('지식 창의 실제 게임/GUI 효과 연결 누락');
    return effect;
}

// 원본 strlen/strcpy 의 첫 NUL 뒤는 설명·마나에 포함하지 않는다. (2026-11-15 WB04-E537)
function knowledgeText(value) {
    const end = value.indexOf('\0');
    return end < 0 ? value : value.slice(0, end);
}

// 두 원문 카드의 동일 112BYTE 수집/정렬/행 배치를 공유하고 원소 범위와 머리만 구별한다(world=false 는 492b60 의 Storm, true 는 493210 의 World). (2026-11-15 WB04-E537)
function showKnowledgeThemeMenu(types, state, hooks, world) {
    if (need(hooks.blocked)()) return [];
    need(hooks.cancel)();
    const technology = need(hooks.currentTechnology)();
    const records = [];
    // 원본 타입 순서를 그대로 순회하고 내용물 조회 뒤 선택한 네 테마의 범위를 판정한다.
    for (let type = 0; type < types.length; type += 1) {
        if (types[type].group === 10 || (types[type].flags2 & 0x200000) !== 0 || type === state.templeProduct) continue;
        const owned = need(hooks.owns)(technology, type);
        const mana = knowledgeText(need(hooks.mana)(type));
        const theme = types[type].theme >>> 0;
        if (world ? theme < 4 || theme > 7 : theme > 3) continue;
        const record = { type, mana, available: Boolean(owned) || mana.length === 0, description: knowledgeText(need(hooks.description)(type)), inProduction: Boolean(need(hooks.inProduction)(type)) };
        if (record.mana.length > 9 || record.description.length > 81) throw new RangeError('원본 지식 행 112BYTE 의 문자열 범위');
        records.push(record);
    }
    if (records.length > 96) throw new RangeError('원본 지식 창의 머리 포함 100개 메뉴 범위');
    sortKnowledgeRecords(records, types);
    const offsets = [0, 0, 0, 0];
    const menus = [];
    // 원본 네 테마 머리의 생성 순서이다.
    const headings = world ? ['EARTH', 'FIRE', 'WATER.', 'MOON'] : ['WIND', 'RAIN', 'THUN.', 'SUN'];
    // 먼저 네 머리를 만들고 같은 정렬 행의 카드들을 같은 네 줄에 누적한다.
    for (let index = 0; index < records.length + 4; index += 1) {
        const token = need(hooks.begin)();
        menus.push(token);
        const menuNode = need(hooks.node)(token);
        menuNode.menuFlags = (menuNode.menuFlags | 0x1000000) >>> 0;
        let theme = 0;
        if (index < 4) {
            theme = index;
            need(hooks.title)(need(hooks.translate)(headings[index]));
            need(hooks.separator)();
            need(hooks.item)(`~[Imana.${index + (world ? 12 : 4)}]`, state.helpCommand, 'themeHelp', true);
            need(hooks.item)('', state.helpCommand, 'themeHelp', true);
            need(hooks.item)('', state.helpCommand, 'themeHelp', true);
        } else {
            const record = records[index - 4];
            theme = types[record.type].theme >>> 0;
            need(hooks.item)(`~[I${record.type}.${record.available ? '#' : '&'}]`, state.helpCommand, record.type, true);
            // 원본은 마지막 공백/하이픈부터 역순으로 한 번만 자르고 첫 줄을 다시 줄이지 않는다. 길이는 바이트 수로 잰다(splitKnowledgeName).
            const { first, second, splitName } = splitKnowledgeName(record.description);
            if (need(hooks.excluded)(record.type)) {
                need(hooks.item)(need(hooks.translate)('~y~EExcluded'), state.helpCommand, record.type, record.available);
                need(hooks.item)('', state.helpCommand, record.type, record.available);
            } else if (state.language === 3) {
                need(hooks.item)('', state.helpCommand, record.type, record.available);
                need(hooks.item)('', state.helpCommand, record.type, record.available);
            } else {
                need(hooks.item)(`~5${first}`, state.helpCommand, record.type, record.available);
                need(hooks.item)(splitName ? `~5${second}` : '', state.helpCommand, record.type, record.available);
            }
        }
        const row = (theme + 1) & 3;
        const step = (state.iconHeight + 46) | 0;
        // 네 줄의 원본 signed 정수 배치이다.
        const y = (Math.imul(step, row) - Math.trunc(Math.imul(step, 4) / 2) - 18 + Math.trunc(state.height / 2)) | 0;
        const x = ((Math.trunc(Math.trunc(state.width / 2) - state.iconWidth * 8.8 * 0.5)) + offsets[row]) | 0;
        need(hooks.attach)(x, y);
        need(hooks.push)(token);
        need(hooks.width)(token, Math.fround(state.iconWidth * 1.1));
        need(hooks.height)(token, Math.fround(step));
        need(hooks.pop)(token);
        const node = need(hooks.node)(token);
        node.targetWidth = Math.fround((node.width - node.targetWidth) * 0.5 + node.targetWidth);
        offsets[row] = Math.trunc(offsets[row] + node.baseWidth + 2.0) | 0;
        node.flags = (node.flags & ~0x4000) >>> 0;
    }
    // 원본의 마지막 위치 순회를 같은 실제 getter 에 전달한다. 계산된 전체 경계는 외부에서 사용하지 않는다.
    for (const token of menus) need(hooks.position)(token);
    // 모두 만든 뒤에만 popup bit 를 켜서 작성 도중 앞 카드를 닫지 않는다.
    for (const token of menus) {
        const node = need(hooks.node)(token);
        node.flags = (node.flags | 0x4000) >>> 0;
    }
    need(hooks.mark)(world ? SHOW_TEC_MENU2_COMMAND : SHOW_TECHNOLOGY_COMMAND);
    return menus;
}

// 기존 Storm 호출의 공개 계약과 기본 F6 를 유지한다. (2026-11-15 WB04-E537)
export function showKnowledgeMenu(types, state, hooks) {
    return showKnowledgeThemeMenu(types, state, hooks, false);
}

// World 원소는 별도 명령으로만 선택한다. (2026-11-15 WB04-E537)
export function showWorldKnowledgeMenu(types, state, hooks) {
    return showKnowledgeThemeMenu(types, state, hooks, true);
}

// 원문 4938c0 은 기술 보유/번역을 조회하지 않고 두 고정 메뉴의 명령 행을 작성한다. (2026-11-15 WB04-E537)
export function showKnowledgeSideMenu(state, hooks) {
    if (need(hooks.blocked)()) return [];
    need(hooks.cancel)();
    const menus = [];
    // Storm 메뉴를 먼저 만들고 World 메뉴를 그 오른쪽에 붙인다.
    for (let index = 0; index < 2; index += 1) {
        const token = need(hooks.begin)();
        menus.push(token);
        const menuNode = need(hooks.node)(token);
        menuNode.menuFlags = (menuNode.menuFlags | 0x1000000) >>> 0;
        const command = index ? SHOW_TEC_MENU2_COMMAND : SHOW_TECHNOLOGY_COMMAND;
        need(hooks.item)('~[IMANA.16]', command, 0, true);
        need(hooks.item)(index ? '~3      The Furies of the World' : '~3      The Furies of the Storm', command, 0, true);
        need(hooks.attach)((Math.trunc(state.width / 2) - 260 + index * 320) | 0, (Math.trunc(state.height / 2) - 190) | 0);
        need(hooks.push)(token);
        need(hooks.width)(token, 300.0);
        need(hooks.height)(token, 300.0);
        need(hooks.pop)(token);
        const node = need(hooks.node)(token);
        node.targetWidth = Math.fround((node.width - node.targetWidth) * 0.5 + node.targetWidth);
        node.flags = (node.flags & ~0x4000) >>> 0;
    }
    // 둘 다 완성한 뒤 실제 현재 위치를 다시 읽는다.
    for (const token of menus) need(hooks.position)(token);
    // 원문의 두 번째 순회에서만 팝업 표식을 켠다.
    for (const token of menus) {
        const node = need(hooks.node)(token);
        node.flags = (node.flags | 0x4000) >>> 0;
    }
    need(hooks.mark)(SHOW_TEC_SIDE_MENU_COMMAND);
    return menus;
}
