// 실제 캠페인 World의 현재 마우스 폴링과 커서 옵션을 처리한다.
// 사용: new CampaignUserInput(world, config).updateCursor({ code, x, y }). 사건 분배와 브라우저 장치 연결은 후속이다.
import { InputCode } from './input-queue.js';

// UserInput.cpp의 커서 설정·폴링 앞단이며 World와 설정 소유자는 이 객체보다 오래 살아야 한다.
export class CampaignUserInput {
    // 실제 World와 원본 ReadInt 형식의 현재 설정을 빌린다.
    #world; #config;
    // 생성할 때 현재 커서나 설정을 변경하지 않는다.
    constructor(world, config) { this.#world = world; this.#config = config; }

    // 두 설정의 지역 기본값 0을 각각 읽고 같은 실제 커서 전역에 기록한다.
    readCursorOptions() {
        const snap = this.#config.readInt('cursorSnapToGrid', 0).value, cannotPlace = this.#config.readInt('cursorShowCannotPlace', 0).value;
        const cursor = this.#world.session().cursor(); cursor.snapToGrid.value = snap >>> 0; cursor.placement.placementFlags = cannotPlace >>> 0;
    }
    // 현재 화면점을 먼저 보관하고 창 밖·잠금·활성 커서 순서로 원본 폴링을 분배한다.
    updateCursor(mouse) {
        this.#world.setMousePoint(mouse.x, mouse.y);
        if (mouse.code & InputCode.outside) return;
        if (this.#world.session().cursorDropSettings().lockDepth) { this.#world.updateMovementCursor(); return; }
        this.readCursorOptions(); this.#world.session().cursor().update(mouse.x, mouse.y);
    }
}
