// 10.78 CheatAllTechnology(00492ae0)의 현재 기술 부모 조회와 테마별 contained 지급 순서를 옮긴 모듈이다(cpppj `o/RawThemeTechnology.cpp/.h`).
// 타입 번호 0 부터 현재 개수(signed, 반복마다 다시 읽음) 미만까지: 기록의 group 이 10 이거나 theme 이 5 이거나 선택 테마와 다르면 건너뛰고, 아니면 현재 기술 부모를 읽어 이미 contained 가 있는지(004acac0(parent, 4, type, 0)) 본 뒤
//   없을 때만 부모를 다시 읽어 004afa70(parent, type, 0, 4, 0x10, 0) 으로 생성한다(반환은 무시). 개수·기술 부모를 callback 뒤 다시 읽으므로 하위 효과의 변경이 다음 경계에 반영된다.
// 안내창 종료·PlayerKnown/타입 assertion 은 추가하지 않는다. 모든 실제 경계가 없으면 아무 동작도 하지 않은 성공으로 숨기지 않고 생성 시 거부한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E13: 테마 기술 지급 이식으로 추가)

// 현재 타입/기술 부모/contained 경계로 테마 기술을 지급하는 처리기이다.
//  hooks = { typeCount() → signed, typeAt(i) → { group, theme }, currentTechnology() → SID, findContained(parent, mask, type, index) → SID/0, createContained(parent, type, 0, 4, 0x10, 0) }. (2026-11-08 WB04-E13)
export class RawThemeTechnology {
    #hooks;

    // 누락된 실제 경계를 빈 성공으로 대체하지 않는다.
    constructor(hooks) {
        for (const name of ['typeCount', 'typeAt', 'currentTechnology', 'findContained', 'createContained']) {
            if (typeof hooks?.[name] !== 'function') throw new TypeError('테마 기술은 현재 타입/현재 부모/실제 contained 경계가 모두 필요합니다');
        }
        this.#hooks = hooks;
    }

    // group 10·theme 5 를 제외하고 선택 테마의 없는 기술을 타입 0 부터 원본 순서로 생성한다. theme 는 signed 32비트이다.
    apply(theme) {
        const hooks = this.#hooks;
        const selected = theme | 0;
        // count 를 캐시하지 않아 typeAt/Find/Create callback 의 변경이 다음 반복 경계에 반영된다.
        for (let type = 0; type < (hooks.typeCount() | 0); type += 1) {
            const record = hooks.typeAt(type);
            if (record.group === 10 || record.theme === 5 || record.theme !== selected) continue;
            const parent = hooks.currentTechnology();
            if (!hooks.findContained(parent, 4, type, 0)) hooks.createContained(hooks.currentTechnology(), type, 0, 4, 0x10, 0);
        }
    }
}
