// 실제 미션 Client 장치로 생산 패널의 각 원본 구성 요소를 조립한다.
import { PriestPlacementAssets } from '../core/priest-placement-assets.js';
import { RawCanonPixelShape } from '../core/raw-canon-pixel-shape.js';
import { ClientGumpBackground } from './client-gump-background.js';
import { ClientHudDivider } from './client-hud-divider.js';
import { ClientImageGump } from './client-image-gump.js';
import { ClientInventoryGrid } from './client-inventory-grid.js';
import { ClientInventoryItem } from './client-inventory-item.js';
import { ClientTextGump } from './client-text-gump.js';
import { makeClientTextMarkup } from './client-text-markup.js';
import { ClientProductionBudget } from './client-production-budget.js';
import { ClientProductionHint } from './client-production-hint.js';
import { ClientFloatingSp } from './client-floating-sp.js';
import { ClientProductionSelection } from './client-production-selection.js';
import { ClientProductionPanel } from './client-production-panel.js';
import { createClientProductionMenu } from './client-production-menu.js';

// 현재 Session·Client 자원과 실제 입력/표시 장치에서 생산 UI 소유자를 만든다.
export function createClientProductionAssembly(root, session, game, fonts, screen, colors, globals, panel, devices) {
    if (!root || !session?.core || !game || !fonts || !screen || !colors || !globals || !panel || !devices || ['now', 'translate', 'sound', 'tell', 'about', 'changed', 'closePopups', 'poll', 'cursor', 'baseHandle', 'menuInstances'].some((name) => typeof devices[name] !== 'function')) throw new TypeError('미션 생산 UI의 실제 장치 누락');
    const context = session.productionContext(), core = session.core(), cursor = session.cursorDropSettings(), ownerTables = colors.ownerTables;
    globals.backgroundType = game.find('fortGump').block + 70; globals.bridgeType = core.bridgeState.bridgeType;
    const color = (token) => {
        if (token === 0x556038) return colors.maps[2]; if (token === 0x556338) return colors.maps[5]; if (token === 0x556438) return colors.maps[6];
        return ownerTables.colorToken(token);
    };
    const background = new ClientGumpBackground(root, game, () => 35, () => screen.shadeMaps(), () => screen.colors(), devices.baseHandle);
    const text = new ClientTextGump(root, fonts, devices.baseHandle, makeClientTextMarkup(game, () => screen.colors(), () => colors.maps[2], devices.sound));
    const hint = new ClientProductionHint(root, text, { now: devices.now, translate: devices.translate, palette: () => screen.colors(), changed: devices.changed });
    const floating = devices.gameSeconds ? new ClientFloatingSp(root, text, devices.gameSeconds, devices.changed) : null;
    const budget = new ClientProductionBudget(root, text, { now: devices.now, money: () => session.sp(context.gameplay.player.battleOwner),
        known: () => context.gameplay.player.playerKnown, fort: () => core.ownerMode.fort, editor: () => context.gameplay.world.editor,
        backgroundType: () => globals.backgroundType, changed: devices.changed });
    const imageState = { get localOwner() { return context.gameplay.player.battleOwner; }, ownerMaps: ownerTables.tokens, shadowOffset: { x: 5, y: 5 } };
    const images = new ClientImageGump(root, game, imageState, color, devices.baseHandle);
    const metadata = new PriestPlacementAssets(game.typeTable, game.shapes, core.types);
    const measure = new RawCanonPixelShape(core.pool, core.types, metadata.frames, metadata.shapes, metadata.hotspots, metadata.geometry, { scaleX: 16, scaleY: 11 });
    const point = { get x() { return cursor.screenX; }, get y() { return cursor.screenY; } }, gridState = { cursorPoint: point, get cursorFlags() { return cursor.inputFlags; } };
    let lastType = 0, grid, menu = null;
    const itemState = { get localOwner() { return context.gameplay.player.battleOwner; }, get editor() { return context.gameplay.world.editor; },
        get stockEnabled() { return globals.stockEnabled; }, inputDisabled: 0, get lastType() { return lastType; }, set lastType(value) { lastType = value; },
        get bridgeType() { return globals.bridgeType; }, pixelsX: 16, pixelsY: 11, get backgroundType() { return globals.backgroundType; }, get backgroundFrame() { return globals.backgroundFrame; } };
    const item = new ClientInventoryItem(root, session, game, metadata, measure, images, itemState, {
        idleItem: (token) => grid.idleItem(token), allocateText: () => text.allocate(), text: (token) => text.construct(token, 0, 0, 0, 0),
        styledText: (token, value) => text.style(token, value), fitText: (token) => text.fit(token),
        background: (target, clip, ...args) => background.texture(target, clip, ...args),
        denied: (first, flags) => first === globals.policyFirst && (flags & 255) === globals.policyByte ? 1 : 0,
        tell: devices.tell, about: (token, x, y) => menu ? menu.open(root.node(token), { x, y }) : devices.about(token, x, y),
    });
    grid = new ClientInventoryGrid(root, session, gridState, { now: devices.now, baseHandle: devices.baseHandle, allocateItem: (size) => item.allocate(size) });
    // 현재 Player 기록의 기술 SID를 원본 비정렬 DWORD로 읽는다.
    const technology = (owner) => {
        if (!context.gameplay.player.playerKnown || owner < 0 || owner >= 9) throw new Error('생산 UI의 현재 Player 기술 누락');
        const records = context.players.records(); return new DataView(records.buffer, records.byteOffset + owner * 180 + 0x9c, 4).getUint32(0, true);
    };
    const selection = new ClientProductionSelection(session, item, { technology, templateOwner: () => context.gameplay.player.templateOwner,
        stock: () => globals.stockEnabled, failure: (empty, serial) => hint.show(empty, serial), flash: () => budget.flash(), sound: devices.sound, changed: devices.changed });
    if (devices.menu) menu = createClientProductionMenu(root, session, game, devices.menu.data, fonts, screen, colors, background, globals.backgroundType, {
        ...devices.menu, baseHandle: devices.baseHandle, sound: devices.sound, translate: devices.translate, affordable: (sid) => item.affordable(sid),
        now: devices.now, poll: devices.poll, cursor: devices.cursor,
    });
    const divider = new ClientHudDivider(root, { background, backgroundColor: () => 35, dividerColor: () => 55, baseHandle: devices.baseHandle,
        closePopups: devices.closePopups, menuInstances: () => menu ? menu.instances() : devices.menuInstances(), poll: devices.poll, cursor: devices.cursor });
    return new ClientProductionPanel(root, session, { grid, background, divider, budget, selection,
        technology: () => context.gameplay.player.playerKnown ? technology(context.gameplay.player.battleOwner) : 0, changed: devices.changed, openPriest: (sid, event) => menu?.openPriest(sid, event) ?? 0, openFactory: (sid, event) => menu?.openFactory(sid, event) ?? 0,
        showSp: (cost, x, y, reference) => { if (!floating) throw new Error('생산 UI의 실제 비용 표시 미연결'); return floating.showSp(cost, x, y, reference); },
        close: () => { floating?.close(); menu?.close(); } }, globals, panel);
}
