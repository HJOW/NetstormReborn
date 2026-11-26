// 실제 미션의 생산 항목 정보와 원본 장식 메뉴를 같은 Client 장치에 결합한다.
import { GumpBackgroundAssets } from '../render/gump-background-draw.js';
import { ButtonGumpState } from './button-gump.js';
import { StoneButtonGumpState } from './stone-button-gump.js';
import { ScrollGumpState } from './scroll-gump.js';
import { ScrollGumpWidgetState } from './scroll-gump-widgets.js';
import { ScrollingTextGumpWidgetState } from './scrolling-text-gump-widgets.js';
import { ClientTextGump } from './client-text-gump.js';
import { makeClientTextMarkup } from './client-text-markup.js';
import { ClientStyleTextBuffers } from './client-style-text-buffers.js';
import { ClientScrollControls } from './client-scroll-controls.js';
import { ClientScrollingText } from './client-scrolling-text.js';
import { ClientGumpDecoration, createClientDecorationState } from './client-gump-decoration.js';
import { ClientMenuGump } from './client-menu-gump.js';
import { ClientProductionAbout } from './client-production-about.js';
import { ClientPlayerMenu } from './client-player-menu.js';
import { ClientObjectMenu } from './client-object-menu.js';
import { ClientObjectMenuInfo } from './client-object-menu-info.js';
import { ClientCarrierMenu } from './client-carrier-menu.js';
import { ClientPriestMenu } from './client-priest-menu.js';
import { ClientPriestMenuOpen } from './client-priest-menu-open.js';
import { ClientFactoryMenu } from './client-factory-menu.js';
import { ClientFactoryMenuOpen } from './client-factory-menu-open.js';
import { ClientGameMenuBar } from './client-game-menu-bar.js';
import { ClientObjectMenuOpen } from './client-object-menu-open.js';
import { ClientKnowledgeMenu } from './client-knowledge-menu.js';
import { ClientWarnings } from './client-warnings.js';
import { ClientMoneyDialog } from './client-money-dialog.js';
import { ClientChatWindow } from './client-chat-window.js';

// 공유 채팅 로그의 글자·그림 조각과 버퍼를 Client 수명에 보관한다. 현재 장치는 미션마다 교체해 이전 Session을 붙잡지 않는다.
function clientMenuText(root, game, fonts, screen, colors, devices) {
    const globals = devices.chat?.globals;
    let owner = globals?.uiOwner;
    if (owner) {
        if (owner.root !== root || owner.game !== game || owner.fonts !== fonts) throw new Error('Client 공유 로그의 실제 UI 소유자 불일치');
        // 창 크기 변경은 같은 Client의 Screen을 교체한다. 로그/글자 버퍼는 보존하고 현재 팔레트 장치만 다시 빌린다.
        owner.devices = devices; owner.colors = colors; owner.screen = screen; return owner;
    }
    owner = { root, game, fonts, screen, colors, devices, text: null, pictures: null, markup: null, buffers: null };
    owner.text = new ClientTextGump(root, fonts, (...args) => owner.devices.baseHandle(...args),
        makeClientTextMarkup(game, () => owner.screen.colors(), () => owner.colors.maps[2], (...args) => owner.devices.sound(...args), undefined,
            (pictures, markup) => { owner.pictures = pictures; owner.markup = markup; }));
    owner.buffers = new ClientStyleTextBuffers(owner.text, fonts, owner.pictures);
    if (globals) globals.uiOwner = owner;
    return owner;
}

// 공유 작성기·버퍼·Chat은 World 없이 생성하며 실제 객체 어댑터는 bindWorld에서 연결한다.
export function createClientSharedProductionMenu(root, game, data, fonts, screen, colors, background, backgroundType, devices) {
    if (!root || !game || !data || !fonts || !screen || !colors || !background || !Number.isInteger(backgroundType) || !devices || ['formHost', 'baseHandle', 'sound', 'translate', 'ownerName', 'helpWord', 'affordable', 'now', 'poll', 'cursor', 'command', 'invalidate'].some((name) => typeof devices[name] !== 'function')) throw new TypeError('생산 메뉴의 실제 장치 누락');
    const textOwner = clientMenuText(root, game, fonts, screen, colors, devices), { text, markup, buffers } = textOwner, assets = new GumpBackgroundAssets(game);
    const buttonState = new ButtonGumpState(), stoneState = new StoneButtonGumpState(), scrollState = new ScrollGumpState(), widgetState = new ScrollGumpWidgetState();
    Object.assign(buttonState, { valueEvent: 0x02000070, clickEvent: 0x0200002c, rightEvent: 0x02000012, repeatEvent: 0x0200002f, baseColor: 56 });
    Object.assign(scrollState, { clickEvent: 0x0200002c, repeatEvent: 0x0200002f, stepEvent: 0x0200002e, changeEvent: 0x02000036 });
    stoneState.color = 49; stoneState.palette = { black: screen.colors().basic[0], white: screen.colors().basic[7], edge: screen.colors().basic[8] };
    stoneState.backgroundType = backgroundType; stoneState.backgroundFrame = 1; widgetState.defaultType = backgroundType;
    const scroll = new ClientScrollControls(root, { buttonState, stoneState, scrollState, widgetState, backgroundColor: () => 35,
        type: (type) => assets.type(type), frame: (...args) => assets.frame(...args), output: { fonts, background },
        now: devices.now, poll: devices.poll, sound: () => devices.sound('button.wav'), baseEvent: devices.baseHandle }); // Gump 버튼 본체는 인자 없이 소리를 요청하므로 원본 button.wav 이름을 채운다. (2026-11-22 WB04-E1168)
    const titleState = new ScrollingTextGumpWidgetState(); Object.assign(titleState, { changeEvent: 0x02000036, stepEvent: 0x0200002e });
    const title = new ClientScrollingText(root, fonts, text, buffers, scroll, markup, titleState, devices.cursor);
    const decoration = new ClientGumpDecoration(root, game, background, createClientDecorationState(backgroundType), devices.baseHandle, devices.invalidate);
    const dimColor = screen.colors().named.find((color) => color.red === 47 && color.green === 24 && color.blue === 17);
    if (!dimColor) throw new Error('메뉴 원본 비활성/그림자 색 전역 없음');
    const menu = new ClientMenuGump(root, { assets: game, fonts, text, background, decoration, title, formHost: devices.formHost, config: data, backgroundType,
        // 원본(005c78d0)·cpppj RawMenuControls 는 메뉴 비활성 글자(하위 메뉴 `›` 포함)와 그림자 색을 같은 어두운 갈색 이름 색 (47,24,17) 로 둔다. 회색(basic[8])을 쓰면 비활성 `›` 가 석판 배경에 묻혀 보이지 않았다. (2026-11-21 WB04-E1136)
        foreground: screen.colors().basic[7], disabled: dimColor.index, shadow: dimColor.index,
        sound: devices.sound, command: devices.command, invalidate: devices.invalidate });
    let about = null, priestOpen = null, factoryOpen = null, playerOpen = null, objectOpen = null, knowledge = null, bound = false, closed = false;
    // 같은 공유 작성기·Chat을 유지하며 현재 실제 World 전용 어댑터를 한 번 연결한다.
    function bindWorld(session, worldDevices = devices.objectMenu, knowledgeDevices = devices.knowledge) {
        if (closed || bound) throw new Error('공유 생산 메뉴의 World 연결 수명 오류');
        if (!session?.core) throw new TypeError('공유 생산 메뉴의 실제 World Session 누락');
        about = new ClientProductionAbout(session, game, data, menu, devices);
        if (worldDevices) {
            const current = worldDevices, player = new ClientPlayerMenu(session, current.identity, menu, current.player);
            const objects = new ClientObjectMenu(session, current.identity, player, menu, data, { translate: devices.translate, helpWord: devices.helpWord, staticText: current.staticText });
            const carrier = new ClientCarrierMenu(session, menu, objects, devices.translate); playerOpen = (subscriber) => player.open(subscriber);
            const priest = new ClientPriestMenu(session, menu, { translate: devices.translate, baseMenu: (sid) => carrier.open(sid), tell: current.tell });
            const summary = new ClientObjectMenuInfo(session, game, current.identity, menu, data, { translate: devices.translate, ownerName: devices.ownerName });
            priestOpen = new ClientPriestMenuOpen(current.world, menu, summary, priest, { ...current.open, sound: devices.sound });
            const factory = new ClientFactoryMenu(session, menu, objects, { translate: devices.translate, helpWord: devices.helpWord, staticText: current.staticText });
            factoryOpen = new ClientFactoryMenuOpen(current.world, menu, summary, factory, devices.sound);
            // 사제·Factory 가 아닌 공통 객체(발사기 같은 건물)의 회수 포함 메뉴다. (2026-11-20 WB04-E1086)
            objectOpen = new ClientObjectMenuOpen(current.world, menu, summary, objects, { ...current.open, sound: devices.sound });
        }
        knowledge = knowledgeDevices ? new ClientKnowledgeMenu(session, menu, { ...knowledgeDevices, translate: devices.translate }) : null;
        bound = true; return true;
    }
    // 아직 World가 없는 생성 단계의 객체 효과를 가짜 성공으로 바꾸지 않는다.
    function requireWorld() { if (closed || !bound) throw new Error('공유 생산 메뉴의 현재 World 미연결'); }
    // 원본 가로 게임 메뉴막대(ESC)는 같은 메뉴 작성기와 Player 하위 메뉴를 쓴다. 현재 게임 조회 장치가 없으면 만들지 않는다. (2026-11-20 WB04-E1084)
    const gameMenu = devices.gameMenu ? new ClientGameMenuBar(root, menu, { ...devices.gameMenu, translate: devices.translate,
        playerMenu: (subscriber) => { if (!playerOpen) throw new Error('게임 메뉴막대의 Player 하위 장치 미연결'); return playerOpen(subscriber); } }) : null;
    // F6 지식 창(Storm·World 카드와 선택 메뉴)도 같은 메뉴 작성기와 현재 Session 을 쓴다. 화면·언어·정적 문자열 장치가 없으면 만들지 않는다. (2026-11-20 WB04-E1088)
    // 실제 지식 어댑터도 bindWorld에서 연결한다.
    // 오른쪽 위 경고·상태 안내(Game Paused!·Game Time·Warning)도 같은 글자 장치로 만든다. 장치가 없으면 만들지 않는다. (2026-11-20 WB04-E1089)
    const warnings = devices.warnings ? new ClientWarnings(root, text, fonts, { changed: devices.changed, ...devices.warnings, translate: devices.translate, sound: () => devices.sound('newWarnMessage.wav') }) : null;
    // Players ▸ Send Money ▸ Send X 의 숫자 입력 창(Send X Storm Power)이다. 확정 장치(원본 004c9350)가 없으면 만들지 않는다. (2026-11-22 WB04-E1168)
    const sendMoney = devices.sendMoney ? new ClientMoneyDialog(root, game, fonts, screen, { text, decoration, background, backgroundType },
        { now: devices.now, poll: devices.poll, sound: devices.sound, baseHandle: devices.baseHandle, translate: devices.translate, confirm: devices.sendMoney.confirm, changed: devices.changed }) : null;
    // 채팅 도우미 ▸ Format ▸ Edit Back/Front Format 입력 창이다(송금 숫자 창과 같은 조립). 확정 장치가 없으면 만들지 않는다. (2026-11-22 WB04-E1179)
    const chatFormat = devices.chatFormat ? new ClientMoneyDialog(root, game, fonts, screen, { text, decoration, background, backgroundType, format: true },
        { now: devices.now, poll: devices.poll, sound: devices.sound, baseHandle: devices.baseHandle, translate: devices.translate, confirm: devices.chatFormat.confirm, changed: devices.changed }) : null;
    // 미션 화면 아래의 채팅 창(F3)이다. 로그·입력줄·도우미를 만들고 HUD 가 표시를 전환한다. 장치가 없으면 만들지 않는다. (2026-11-22 WB04-E1169)
    const chat = devices.chat ? new ClientChatWindow(root, game, fonts, screen, { scrolling: title, buffers, background, menu, globals: devices.chat.globals, backgroundType },
        { now: devices.now, poll: devices.poll, sound: devices.sound, baseHandle: devices.baseHandle, translate: devices.translate, changed: devices.changed, config: devices.chat.config,
            read: devices.chat.read, recipient: devices.chat.recipient, sendChat: devices.chat.sendChat, resolve: devices.chat.resolve, log: devices.chat.log,
            dotCommands: devices.chat.dotCommands, complete: devices.chat.complete, refreshTechnology: devices.chat.refreshTechnology,
            clipboardRead: devices.chat.clipboardRead, clipboardWrite: devices.chat.clipboardWrite, setNodeHelp: devices.setNodeHelp }) : null;
    // 우클릭 빠른 도움말 팝업(원본 BaseHelp: 세로 메뉴 시작→제목→Help 항목→현재 루트에 부착)을 같은 메뉴 작성기로 만드는 장치이다. (2026-11-22 WB04-E1236)
    const helpMenu = { begin: (flags) => menu.beginVertical(flags), title: (value) => menu.title(value), entry: (value, command, topic) => menu.entry(value, command, topic, true, true, false), attach: (x, y, parent) => menu.attach({ x, y }, parent) };
    return { helpMenu, gameMenu, get knowledge() { return knowledge; }, warnings, chat, bindWorld, worldBound: () => bound,
        open: (item, point) => { requireWorld(); return about.open(item, point); },
        openPriest: (sid, event) => { requireWorld(); return priestOpen?.supports(sid) ? priestOpen.open(sid, event) : 0; },
        openFactory: (sid, event) => { requireWorld(); return factoryOpen?.supports(sid) ? factoryOpen.open(sid, event) : 0; },
        openObject: (sid, event) => { requireWorld(); return objectOpen?.supports(sid) ? objectOpen.open(sid, event) : 0; },
        instances: () => menu.instances(), closePopups: () => menu.closePopups(),
        sendMoney: sendMoney ? { open: (target) => sendMoney.open(target), current: () => sendMoney.current(), text: () => sendMoney.text() } : null,
        chatFormat: chatFormat ? { open: (front, text) => chatFormat.openFormat(front, text), current: () => chatFormat.current(), text: () => chatFormat.text() } : null,
        close: () => { if (closed) return; closed = true; chat?.close(); chatFormat?.close(); sendMoney?.close(); warnings?.close(); gameMenu?.close(); menu.close(); scroll.close();
            // 공유 조각만 남기고 이전 미션의 장치/색표 연결은 놓는다. 늦은 중복 close가 새 미션 장치를 지우지 않게 한다.
            if (textOwner.devices === devices) { textOwner.devices = null; textOwner.colors = null; }
            about = priestOpen = factoryOpen = playerOpen = objectOpen = knowledge = null; bound = false; } };
}

// 기존 실제 Session 호출자는 같은 공유 생성과 World 연결을 연속 실행한다.
export function createClientProductionMenu(root, session, game, data, fonts, screen, colors, background, backgroundType, devices) {
    if (typeof session?.gumpFormHost !== 'function') throw new TypeError('생산 메뉴의 실제 World Session 누락');
    const shared = createClientSharedProductionMenu(root, game, data, fonts, screen, colors, background, backgroundType, { ...devices, formHost: () => session.gumpFormHost() });
    shared.bindWorld(session); return shared;
}
