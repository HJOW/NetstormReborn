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

// Session·원본 자산과 실제 입력·출력 장치를 빌리고 생성한 메뉴를 반납할 수 있게 한다.
export function createClientProductionMenu(root, session, game, data, fonts, screen, colors, background, backgroundType, devices) {
    if (!root || !session || !game || !data || !fonts || !screen || !colors || !background || !Number.isInteger(backgroundType) || !devices || ['baseHandle', 'sound', 'translate', 'ownerName', 'helpWord', 'affordable', 'now', 'poll', 'cursor', 'command', 'invalidate'].some((name) => typeof devices[name] !== 'function')) throw new TypeError('생산 메뉴의 실제 장치 누락');
    let pictures, markup;
    const text = new ClientTextGump(root, fonts, devices.baseHandle, makeClientTextMarkup(game, () => screen.colors(), () => colors.maps[2], devices.sound, undefined, (p, m) => { pictures = p; markup = m; }));
    const buffers = new ClientStyleTextBuffers(text, fonts, pictures), assets = new GumpBackgroundAssets(game);
    const buttonState = new ButtonGumpState(), stoneState = new StoneButtonGumpState(), scrollState = new ScrollGumpState(), widgetState = new ScrollGumpWidgetState();
    Object.assign(buttonState, { valueEvent: 0x02000070, clickEvent: 0x0200002c, rightEvent: 0x02000012, repeatEvent: 0x0200002f, baseColor: 56 });
    Object.assign(scrollState, { clickEvent: 0x0200002c, repeatEvent: 0x0200002f, stepEvent: 0x0200002e, changeEvent: 0x02000036 });
    stoneState.color = 49; stoneState.palette = { black: screen.colors().basic[0], white: screen.colors().basic[7], edge: screen.colors().basic[8] };
    stoneState.backgroundType = backgroundType; stoneState.backgroundFrame = 1; widgetState.defaultType = backgroundType;
    const scroll = new ClientScrollControls(root, { buttonState, stoneState, scrollState, widgetState, backgroundColor: () => 35,
        type: (type) => assets.type(type), frame: (...args) => assets.frame(...args), output: { fonts, background },
        now: devices.now, poll: devices.poll, sound: devices.sound, baseEvent: devices.baseHandle });
    const titleState = new ScrollingTextGumpWidgetState(); Object.assign(titleState, { changeEvent: 0x02000036, stepEvent: 0x0200002e });
    const title = new ClientScrollingText(root, fonts, text, buffers, scroll, markup, titleState, devices.cursor);
    const decoration = new ClientGumpDecoration(root, game, background, createClientDecorationState(backgroundType), devices.baseHandle, devices.invalidate);
    const menu = new ClientMenuGump(root, { assets: game, fonts, text, background, decoration, title, session, config: data, backgroundType,
        foreground: screen.colors().basic[7], disabled: screen.colors().basic[8], shadow: screen.colors().basic[0],
        sound: devices.sound, command: devices.command, invalidate: devices.invalidate });
    const about = new ClientProductionAbout(session, game, data, menu, devices);
    return { open: (item, point) => about.open(item, point), instances: () => menu.instances(), closePopups: () => menu.closePopups(),
        close: () => { menu.close(); scroll.close(); } };
}
