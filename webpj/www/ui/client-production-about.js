// 실제 Session의 생산 항목을 원본 정보 메뉴 작성기에 연결한다.
import { typeString } from '../assets/type-table.js';
import { RawObjectMenuInfo, createObjectMenuInfoState } from '../core/raw-object-menu-info.js';
import { formatPlayerMenuText } from '../core/raw-player-menu.js';
import { InventoryAboutMenu, InventoryAboutState } from './inventory-about-menu.js';

// 원본 객체 설명과 생산 정보 작성 순서를 현재 Session·번역 및 메뉴 작성기에 결합한다.
export class ClientProductionAbout {
    // 현재 Session과 원본 파생 처리기들이다.
    #session; #infoState; #about;
    // 현재 자산의 TYPE class와 UI의 실제 작성·번역 장치를 빌린다.
    constructor(session, game, data, writer, devices) {
        if (!session?.core || !game || !data || !writer || !devices || ['translate', 'ownerName', 'helpWord', 'affordable', 'sound'].some((name) => typeof devices[name] !== 'function') || ['begin', 'root', 'idle', 'reference', 'title', 'header', 'entry', 'attach'].some((name) => typeof writer[name] !== 'function')) throw new TypeError('생산 정보의 실제 Session·작성 장치 누락');
        this.#session = session;
        const context = session.productionContext(), core = session.core(), classes = new Array(core.types.length).fill('');
        // 실제 TYPE 정의의 마지막 class 속성을 각 원본 타입 번호에 보충한다.
        for (const asset of game.types()) { const value = typeString(asset.definition, 'class'); if (value !== null) { const name = value.split('\0', 1)[0]; if (name.length > 23) throw new RangeError('생산 정보 TYPE class 길이 초과'); classes[asset.block + 70] = name; } }
        const state = this.#infoState = createObjectMenuInfoState({ localOwner: () => context.gameplay.player.battleOwner,
            editor: () => context.gameplay.world.editor, missionPost: () => context.gameplay.world.missionPost });
        const info = new RawObjectMenuInfo(core.pool, core.types, classes, state, {
            description: (type) => devices.translate(core.types[type].description), ownerName: devices.ownerName, translate: devices.translate,
            substitute: (value) => data.substitute(value), print: formatPlayerMenuText, title: (value) => writer.title(value), header: (value) => writer.header(value),
            alliance: (owner, local) => { if (owner >= 9 || local >= 9) throw new RangeError('생산 정보 동맹 범위 오류'); return session.alliances()[owner * 9 + local]; },
        });
        const aux = context.storage.gameAux, aboutState = new InventoryAboutState();
        aboutState.sharedSuppressedClass = { get value() { return aux.geyserType; } }; aboutState.sharedAltarType = { get value() { return aux.altarType; } };
        aboutState.sharedDaisType = { get value() { return aux.daisType; } }; aboutState.sharedNuggetType = { get value() { return aux.nuggetType; } };
        this.#about = new InventoryAboutMenu(core.pool, aboutState, {
            begin: () => writer.begin(), root: () => writer.root(), templateOwner: () => context.gameplay.player.templateOwner,
            editor: () => context.gameplay.world.editor, cost: (type) => session.hitPoints().typeCostUnits(type), affordable: devices.affordable,
            helpWord: devices.helpWord, idle: (...args) => writer.idle(...args), reference: (...args) => writer.reference(...args),
            describe: (...args) => info.describe(...args), translate: devices.translate, print: (pattern, value) => formatPlayerMenuText(pattern, [value]),
            header: (value) => writer.header(value), entry: (...args) => writer.entry(...args), attach: (...args) => writer.attach(...args), sound: devices.sound,
        });
    }
    // 호출자가 보유한 실제 항목과 화면 좌표를 같은 원본 처리기에 전달한다.
    open(item, point) { const aux = this.#session.productionContext().storage.gameAux; this.#infoState.altar = aux.altarType; this.#infoState.nugget = aux.nuggetType; return this.#about.open(item, point); }
}
