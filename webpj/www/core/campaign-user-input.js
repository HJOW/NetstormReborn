// 실제 캠페인 World의 현재 마우스 폴링과 커서 옵션을 처리한다.
// 사용: new CampaignUserInput(world, config, services).updateCursor({ code, x, y }) / event(사건). 좌클릭과 브라우저 장치 연결은 후속이다.
import { InputCode } from './input-queue.js';
import { queueProductionShortcut, PRODUCTION_BRIDGE_COMMAND, PRODUCTION_REPEAT_COMMAND } from './production-shortcuts.js';
import { dispatchSelectionKeyInput } from './client-input-commands.js';
import { gameMenuCommandValue } from './game-menu-commands.js';

// UserInput.cpp의 커서 설정·폴링 앞단이며 World와 설정 소유자는 이 객체보다 오래 살아야 한다.
export class CampaignUserInput {
    // 실제 World와 원본 ReadInt 형식의 현재 설정을 빌린다.
    #world; #config; #services;
    // 생성할 때 현재 커서나 설정을 변경하지 않는다.
    constructor(world, config, services = {}) { this.#world = world; this.#config = config; this.#services = { ...services }; }

    // 두 설정의 지역 기본값 0을 각각 읽고 같은 실제 커서 전역에 기록한다.
    readCursorOptions() {
        const snap = this.#config.readInt('cursorSnapToGrid', 0).value, cannotPlace = this.#config.readInt('cursorShowCannotPlace', 0).value;
        const cursor = this.#world.session().cursor(); cursor.snapToGrid.value = snap >>> 0; cursor.placement.placementFlags = cannotPlace >>> 0;
    }
    // 실제 정지 상태를 도달한 위치마다 다시 읽는다.
    #paused() { if (typeof this.#services.paused !== 'function') throw new Error('사용자 입력의 현재 정지 장치 미연결'); return this.#services.paused(); }
    // 생산·선택 명령은 같은 40칸 입력 큐에 signed 인자를 보존해 예약한다.
    #queue(command, first, second) { const queue = this.#services.queue; if (!queue || typeof queue.pushCommand !== 'function') throw new Error('사용자 입력의 실제 명령 큐 미연결'); queue.pushCommand(command, first | 0, second | 0); }
    // 정지·활성 커서·생산·선택 키와 우클릭을 원본 순서로 처리한다. 좌클릭은 후속까지 명시 거부한다.
    event(event) {
        if ((event.code & InputCode.release) || this.#paused()) return null;
        this.readCursorOptions();
        if (this.#world.session().cursor().handleInput(event, this.#world.cursorRotationMode())) return null;
        const queue = (command, first, second) => this.#queue(command, first, second);
        if (queueProductionShortcut(event, this.#paused(), { command: () => PRODUCTION_BRIDGE_COMMAND, repeatCommand: () => PRODUCTION_REPEAT_COMMAND, queue })) return null;
        const selection = {
            paused: () => this.#paused(), command: (address) => { const value = gameMenuCommandValue(address); if (value === null) throw new Error('선택 키의 현재 메뉴 명령 누락'); return value; },
            localOwner: () => this.#world.session().localOwner(), queue,
            next: (add) => this.#world.selectNextUnit(add), priest: () => this.#world.selectPriest(queue),
        };
        if (dispatchSelectionKeyInput(event, selection)) return null;
        const key = (event.code >>> InputCode.virtualKeyShift) & 255;
        if (key >= 8) return null;
        const mouse = (event.code & 0x03ffffff) >>> 0;
        if (mouse === InputCode.rightButton) {
            if (this.#world.inputMode() !== 0) { this.#world.highlight(0); this.#world.resetSelection(); return null; }
            const session = this.#world.session(), force = (event.code & InputCode.control) && (this.#world.specialSelection() || session.editor()) ? 1 : 0;
            const picked = this.#world.pick(event.x, event.y, force);
            if (picked) { if (session.editor()) throw new Error('웹 제외 편집 객체 메뉴 미연결'); return picked; }
            if (!session.core().ownerMode.challenge || !session.playerKnown()) this.#world.resetSelection();
            return null;
        }
        if (mouse === InputCode.leftButton) throw new Error('사용자 입력의 좌클릭 사건 분배 미연결');
        return null;
    }
    // 현재 화면점을 먼저 보관하고 창 밖·잠금·활성 커서 순서로 원본 폴링을 분배한다.
    updateCursor(mouse) {
        this.#world.setMousePoint(mouse.x, mouse.y);
        if (mouse.code & InputCode.outside) return;
        if (this.#world.session().cursorDropSettings().lockDepth) { this.#world.updateMovementCursor(); return; }
        this.readCursorOptions(); this.#world.session().cursor().update(mouse.x, mouse.y);
    }
}
