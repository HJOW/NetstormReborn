// 원본 43d330 의 생산 내용물 정보 메뉴(Inventory 항목을 눌렀을 때 열리는 설명/비용/회수/Help 메뉴)를 같은 현재 SID 와 작성기에 연결한다(cpppj client/InventoryAboutMenu.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 설명(describe) → 비용 머리(번역 서식에 정수 비용) → SP 부족 머리 → (편집 중이 아니고 회수 가능 비트가 있고 회수 제외 클래스가 아니면) 회수 항목 → Help 항목 → 현재 루트 부착 → 음향 순서다. 문자열/메뉴/시계 사본 없이 현재 읽기와 작성 순서를 복원한다.
// 회수 비용은 원본의 float 비용 저장 뒤 넓은 합/quarter 를 유지한다: 비용을 float32 로 저장한 뒤(43cb53/43cb57) 제단/다이스는 합 0, 그 밖은 저장값을 승격하고 너겟이 아니면 1/4 을 곱해 signed 64비트 절삭의 하위 DWORD 를 쓴다.
// 이 처리기는 10.78 전용이며 실제 경계가 누락된 빈 성공을 허용하지 않는다. 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// 사용: const menu = new InventoryAboutMenu(pool, new InventoryAboutState(), hooks); menu.open({ content, object }, { x, y }) → 1.
// (2026-11-14 WB04-E448: RawMenuControls 선행 Inventory 정보 메뉴 이식)
import { cameraFtolLow } from '../render/camera-view.js';

// 필수 훅 이름 목록이다(생성 시 모두 함수여야 한다).
const REQUIRED_HOOKS = ['begin', 'root', 'templateOwner', 'editor', 'cost', 'affordable', 'helpWord', 'idle', 'reference', 'describe', 'translate', 'print', 'header', 'sound', 'entry', 'attach'];

// 원본 종류/명령 전역이다. 내용물 클래스와 표시 종류를 구별한다. shared* 는 같은 Client 의 장기 owner 값을 빌리는 `{ value }` 상자(없으면 기본값)다.
export class InventoryAboutState {
    // 회수 제외 클래스(122)/개수 보정 종류 제단(164)·다이스(165)/전액 종류 너겟(154)의 기본값이다.
    suppressedClass = 122; none = 164; dais = 165; fullValue = 154;
    // 실제 회수 명령(0x0200005d)과 Quick Help 명령(0x02000017)이다.
    salvageCommand = 0x0200005d; helpCommand = 0x02000017;
    // 실제 00541240/005412e8/005412ec/005412c0 의 현재 값을 같은 Client 장기 owner 에서 빌리는 상자들이다(null 이면 위 기본값).
    sharedSuppressedClass = null; sharedAltarType = null; sharedDaisType = null; sharedNuggetType = null;
    // 편집 관문이 돌아온 뒤 현재 회수 제외 클래스를 읽는다.
    suppressedClassNow() { return this.sharedSuppressedClass ? this.sharedSuppressedClass.value : this.suppressedClass; }
    // 정수 비용 조회 뒤 개수 0 의 제단 보정을 선택한다(상자가 없으면 none).
    altarType() { return this.sharedAltarType ? this.sharedAltarType.value : this.none; }
    // 같은 현재 번호로 다이스의 개수 0 합을 선택한다.
    daisType() { return this.sharedDaisType ? this.sharedDaisType.value : this.dais; }
    // 현재 너겟만 1/4 배율을 생략한다.
    nuggetType() { return this.sharedNuggetType ? this.sharedNuggetType.value : this.fullValue; }
}

// 현재 SID 풀의 한 슬롯을 읽는 정보 메뉴 본체다. hooks = { begin() → 메뉴 토큰, root(), templateOwner(), editor() → boolean, cost(kind) → 정수 비용, affordable(sid) → boolean, helpWord() → 문자열|null, idle(menu, true), reference(menu, object),
//   describe(a, b, c, d), translate(키), print(서식, 정수) → 문자열, header(글), sound(이름), entry(라벨|null, 사건, 인자, true, true, false), attach(점, 루트) } 이다.
export class InventoryAboutMenu {
    // 같은 슬롯 풀/현재 전역/효과를 빌린다.
    #pool; #state; #hooks;
    // 10.78 풀과 모든 필수 훅이 있어야 한다(없으면 TypeError, C++ invalid_argument).
    constructor(pool, state, hooks) {
        if (pool.edition !== 'patch1078' || !hooks || REQUIRED_HOOKS.some((name) => typeof hooks[name] !== 'function')) throw new TypeError('Inventory 정보 메뉴의 실제 경계 연결 필요');
        this.#pool = pool; this.#state = state; this.#hooks = hooks;
    }

    // callback 뒤에도 같은 현재 내용물의 원문을 다시 읽는다(비정렬 little endian WORD).
    #word(sid, offset) { const raw = this.#pool.slot(sid); return raw[offset] | (raw[offset + 1] << 8); }

    // 43cb30(kind, 0)→_ftol: 정수 비용을 먼저 float32 로 저장하고, 제단/다이스는 합 0, 그 밖은 저장값을 배정도로 승격하며 너겟이 아니면 0.25 를 곱한 뒤 signed 64 비트 절삭의 하위 DWORD 를 돌려준다.
    #refund(kind) {
        const cost = Math.fround(this.#hooks.cost(kind));
        let sum = (kind === this.#state.altarType() || kind === this.#state.daisType()) ? 0 : cost;
        if (kind !== this.#state.nuggetType()) sum *= 0.25;
        return cameraFtolLow(sum);
    }

    // 43d330: 내용물 슬롯(범위 밖이면 풀이 RangeError)을 확인하고 새 메뉴 시작 → idle 등록(true) → 내용물 Form 참조 → 설명(종류 WORD(+18), 0, 템플릿 소유자, 0xffffffff) → 비용 머리(번역 "Cost: ~i%d~[IfortGump.3]~.\n"에 정수 비용) →
    // 구매 불가면 부족 머리 → 편집 중이 아니고 +20 WORD 의 0x20 비트가 있고 슬롯 type(+10)이 현재 회수 제외 클래스가 아니면 회수 항목("Salvage for %d~[IfortGump.3]", 사건=회수 명령, 인자=item.object) → Help 항목(NULL 과 빈 문자열을 구별,
    // 사건=Help 명령, 인자=종류) → 현재 루트에 점으로 부착 → "opengump.wav" 음향 → 1 이다.
    open(item, point) {
        const hooks = this.#hooks; const state = this.#state; const content = item.content & 0xffff; this.#pool.slot(content);
        const menu = hooks.begin(); hooks.idle(menu, true); hooks.reference(menu, item.content);
        const owner = hooks.templateOwner(); hooks.describe(this.#word(content, 18), 0, owner, 0xffffffff);
        const kind = this.#word(content, 18); const costPattern = hooks.translate('Cost: ~i%d~[IfortGump.3]~.\n'); const cost = hooks.cost(kind); hooks.header(hooks.print(costPattern, cost));
        if (!hooks.affordable(content)) hooks.header(hooks.translate('~r~ENeed more Storm Power to Build!\n'));
        if (!hooks.editor() && (this.#word(content, 20) & 0x20) && this.#pool.slot(content)[10] !== state.suppressedClassNow()) {
            const pattern = hooks.translate('Salvage for %d~[IfortGump.3]'); const refund = this.#refund(this.#word(content, 18)); const label = hooks.print(pattern, refund); hooks.entry(label, state.salvageCommand, item.object, true, true, false);
        }
        const helpKind = this.#word(content, 18); const help = hooks.helpWord(); hooks.entry(help === null || help === undefined ? null : help, state.helpCommand, helpKind, true, true, false);
        hooks.attach(point, hooks.root()); hooks.sound('opengump.wav'); return 1;
    }
}
