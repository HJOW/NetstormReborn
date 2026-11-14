// 원본 Userinput.cpp 의 사용자 선택 부모(4d5c90 Select·4d5c60 Clear·4d5490 다중 선택 정리·004da6b0 다중 토글·004da39a Shift 확장·004d5500 다중 이동·4d5440 기억·004da650 마우스 재확인)와
// 짧은 메뉴 입구(Carrier 4268c0/492080, Archer 41e660)를 같은 현재 물리 풀에서 실행한다(cpppj `client/RawUserSelection.cpp/.h`, 기준선 1f00a25a, 10.78 전용).
// 단일 선택(selected 005caea0)·마우스 TYPE(005caea4)·기억 좌표(005caf38/3c)·다중 선택 배열(005caf2c)과 signed 개수(005caf34)는 서로 다른 전역이며 호출 중 가상 통지가 바꿀 수 있으므로
// 원본 위치마다 `state` 객체의 현재 값을 다시 읽고 쓴다(복사하지 않는다). 모든 외부 효과(통지·갱신·강조·음향·이동·진단)는 호스트 훅이며 도달한 경계가 연결되지 않았으면 그 위치에서 Error 이다(선행 효과는 되돌리지 않는다).
// 사용: const selection = new RawUserSelection(pool, types, state, hooks); selection.select(sid, 0, 0); selection.clear();
// (2026-11-15 WB04-E495: 5-B 사용자 선택 이식으로 추가)

// 00542808/00542da8 의 WorldDestSelected 명령값이다. 사용자 이동점 통지를 일반 큐에 넣는 쪽이 쓴다.
export const WORLD_DESTINATION_SELECTED_COMMAND = 0x02000044;

// 선택 대상 TYPE 레코드 번호 상한(10.78 TYPE 188개)과 SID 상한(WORD)이다.
const TYPE_COUNT = 188; const MAX_SID = 0xffff;
// 슬롯 오프셋: 다음 링크 WORD(+4), 하위 링크 WORD(+6), TYPE BYTE(+10), 상태 BYTE(+11), 좌표 float(+14/+18), 소유자 BYTE(+34), 선택 비트 BYTE(+40)다.
const NEXT_AT = 4; const CHILD_AT = 6; const TYPE_AT = 10; const FLAGS_AT = 11; const X_AT = 14; const Y_AT = 18; const OWNER_AT = 34; const MARK_AT = 40;
// 객체 flags2 의 이동 가능 비트와 TYPE flags1 의 마나 표시/선택 허용 비트다.
const MOVABLE = 0x30000; const SHOW_MANA = 0x10000000; const SELECTABLE = 0x08000000;

// 선택된 하위가 없으면 그 위치에서 실패하여 선행 원본 효과를 감추지 않는다(C++ logic_error → Error).
function need(hook) {
    if (typeof hook !== 'function') throw new Error('실제 사용자 선택 하위 미연결');
    return hook;
}

// float32 비트를 값으로 읽는다(기억 좌표 비교용).
function floatOf(bits) {
    const view = new DataView(new ArrayBuffer(4)); view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 현재 물리 풀(10.78)과 188 TYPE 표, 선택 상태 객체, 하위 훅으로 구성하는 사용자 선택 처리기이다.
// state = { selected, mouseType, rememberedPoint: [x비트, y비트], multiple(DWORD 배열), multipleCount(signed), currentMultiple?() → 현재 배열 } 이며 처리기가 직접 읽고 쓴다.
// hooks = { paused, enabled, blocked, localOwner, editor, dais, manaProcess, rangeProcess, selectNotice(sid, on), refresh(sid), processNotice(sid, event, info, value), dump(sid), showMana(sid, flag), showRange(sid, flag),
//   rangeType(type), content(sid) → sid, altar(sid) → sid, moving(sid), movementPreview(sid), highlight(mode), sound(x비트, y비트, 이름, 0), assertion(), previewAssertion(), appendMultiple(sid), removeMultiple(sid), writeEnabled(value) } 이다.
export class RawUserSelection {
    // 같은 물리 풀·TYPE 표·현재 상태·훅 묶음이다.
    #pool; #types; #state; #hooks;

    // 10.78 풀이 아니거나 TYPE 이 188개가 아니면 거부한다(C++ invalid_argument → TypeError). 생성 때 외부 효과는 없다.
    constructor(pool, types, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || types?.length !== TYPE_COUNT) throw new TypeError('사용자 선택 10.78 풀/TYPE 오류');
        this.#pool = pool; this.#types = types; this.#state = state; this.#hooks = hooks;
    }

    // 전체 원문 DWORD 를 현재 지원 풀/WORD SID 범위 확인 뒤에만 번호로 인정한다(0 슬롯은 범위 확인에만 쓰고 선택 객체로 읽지 않는다).
    #slot(sid) {
        const value = sid >>> 0;
        if (!value || value >= this.#pool.capacity || value > MAX_SID) throw new RangeError('사용자 선택 SID 범위 오류');
        return value;
    }

    // 현재 슬롯의 살아 있는 물리 바이트이다(free/dead 를 추가로 제외하지 않는다).
    #raw(sid) { return this.#pool.physical(this.#slot(sid)); }

    // 현재 TYPE 번호의 레코드를 콜백 뒤에도 다시 조회한다.
    #type(index) {
        if (index >= this.#types.length) throw new RangeError('사용자 선택 TYPE 범위 오류');
        return this.#types[index];
    }

    // 비정렬 little endian 원문을 필요한 폭만큼 읽는다(WORD/DWORD).
    static #read(raw, offset, width = 4) {
        let value = 0;
        // 낮은 바이트부터 호스트 정렬과 무관하게 조립한다.
        for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (index * 8);
        return value >>> 0;
    }

    // 명시 장기 owner 가 있으면 매번 그 현재 배열을, 없으면 상태의 배열을 읽는다(가상 통지가 재할당한 배열을 다음 반복에서 다시 읽는다).
    #multiple() { return this.#state.currentMultiple ? this.#state.currentMultiple() : this.#state.multiple; }

    // 4d5490: 현재 signed 개수만큼 다중 선택을 비선택으로 통지(+7c(0)→+88)하고 마지막에 개수를 0 으로 쓴다. 죽음/반납 상태 비트(&3)가 있거나 범위 밖 번호는 건너뛴다.
    clearMultiple() {
        // 통지가 개수/다음 항목을 바꾸면 다음 반복에서 그 현재 값을 사용한다.
        for (let index = 0; index < (this.#state.multipleCount | 0); index += 1) {
            const multiple = this.#multiple();
            if (index >= multiple.length) throw new RangeError('사용자 다중 선택 배열 범위 오류');
            const number = multiple[index] >>> 0;
            if (!number || number >= this.#pool.capacity || (this.#raw(number)[FLAGS_AT] & 3)) continue;
            const sid = this.#slot(number); need(this.#hooks.selectNotice)(sid, 0); need(this.#hooks.refresh)(sid);
        }
        this.#state.multipleCount = 0;
    }

    // 종속 프로세스 체인(+6 하위 → 각 +4 다음)을 효과 전에 다음 링크를 저장하며 따라가 마나/사정거리 프로세스(TYPE 10..69 중 manaProcess/rangeProcess)에만 통지(3,0,0)하고, 대상 자신을 비선택 통지/갱신한 뒤 selected 를 0 으로 쓴다.
    #clearPrevious(sid) {
        let next = RawUserSelection.#read(this.#raw(sid), CHILD_AT, 2); const visited = new Set();
        // 손상된 순환만 호스트에서 명시 거부한다.
        while (next) {
            const number = next; if (visited.has(number)) throw new Error('사용자 선택 프로세스 체인 순환'); visited.add(number);
            const raw = this.#raw(number); next = RawUserSelection.#read(raw, NEXT_AT, 2); const type = raw[TYPE_AT];
            if (type >= 10 && type < 70 && (type === need(this.#hooks.manaProcess)() || type === need(this.#hooks.rangeProcess)())) need(this.#hooks.processNotice)(this.#slot(number), 3, 0, 0);
        }
        need(this.#hooks.selectNotice)(sid, 0); need(this.#hooks.refresh)(sid); this.#state.selected = 0;
    }

    // 4d5440: 대상 TYPE 의 flags2 에 이동 가능 비트가 있을 때만 마우스 TYPE 과 기억 좌표(float 비트 두 값)를 쓴다.
    remember(x, y, type) {
        if (this.#type(type).flags2 & MOVABLE) { this.#state.rememberedPoint[0] = x >>> 0; this.#state.rememberedPoint[1] = y >>> 0; this.#state.mouseType = type >>> 0; }
    }

    // 004da650: 마우스 TYPE 이 0 이면 아무것도 하지 않고, 아니면 기억한 점/TYPE 으로 찾은 객체가 현재 선택과 같거나 선택이 없을 때 Select(found,0,1) 뒤 현재 +40 의 비트(9)가 없으면 마우스 TYPE 을 0 으로 쓴다. 선택 거부의 반환값이나 재지정된 SID 로 원래 found 를 대체하지 않는다.
    recheckMouse(find) {
        if (!this.#state.mouseType) return;
        const found = need(find)(this.#state.rememberedPoint[0], this.#state.rememberedPoint[1], this.#state.mouseType) >>> 0;
        if (!found || (this.#state.selected && this.#state.selected !== found)) return;
        this.select(found, 0, 1);
        if (!(this.#raw(found)[MARK_AT] & 9)) this.#state.mouseType = 0;
    }

    // 4d5c90 Select(sid, forceSelect, activateMove): 첫 관문(정지·비활성·차단)과 기존 다중/단일 선택 정리 뒤 새 대상을 덤프·마나/사정거리 표시·선택 통지하고 기억 좌표/마우스 TYPE/이동 강조/미리보기/소리를 처리한다.
    // 거부는 0, 성공은 최종 선택 번호(dais 가 같은 위치의 유효한 제단을 가리키면 제단)이다. forceSelect 는 TYPE flags1 의 선택 허용 비트를 우회하고 activateMove 는 모드 5 이동 강조 활성화에만 관여한다.
    select(number, forceSelect, activateMove) {
        number >>>= 0;
        // dais 가 같은 위치의 유효 제단으로 위임하면 최초 관문과 다중 선택 정리부터 다시 수행한다.
        for (;;) {
            if ((number && need(this.#hooks.paused)()) || (!need(this.#hooks.enabled)() && !number) || need(this.#hooks.blocked)()) return 0;
            this.clearMultiple();
            if (this.#state.selected) { this.#raw(this.#state.selected); this.#clearPrevious(this.#slot(this.#state.selected)); }
            if (!number || (this.#raw(number)[FLAGS_AT] & 2)) { need(this.#hooks.highlight)(0); return 0; }
            const sid = this.#slot(number); need(this.#hooks.dump)(sid);
            if (this.#type(this.#raw(number)[TYPE_AT]).flags1 & SHOW_MANA) need(this.#hooks.showMana)(sid, 0);
            if (need(this.#hooks.rangeType)(this.#raw(number)[TYPE_AT])) need(this.#hooks.showRange)(sid, 0);
            if ((this.#type(this.#raw(number)[TYPE_AT]).flags2 & MOVABLE) && need(this.#hooks.content)(sid)) need(this.#hooks.showRange)(sid, 0);
            if (this.#raw(number)[TYPE_AT] === need(this.#hooks.dais)()) {
                const altar = need(this.#hooks.altar)(sid) >>> 0; // 원본 40ea40 은 상태 없이 물리 범위만 확인한다.
                if (altar && altar < this.#pool.capacity) { number = altar; continue; }
            }
            if (!(this.#type(this.#raw(number)[TYPE_AT]).flags1 & SELECTABLE) || forceSelect) {
                this.#state.selected = number; need(this.#hooks.selectNotice)(sid, 1); need(this.#hooks.refresh)(sid);
                const raw = this.#raw(number); const x = RawUserSelection.#read(raw, X_AT); const y = RawUserSelection.#read(raw, Y_AT); // 비교 전 원본 float 두 개의 비트를 캡처한다.
                if (!(floatOf(x) === floatOf(this.#state.rememberedPoint[0]) && floatOf(y) === floatOf(this.#state.rememberedPoint[1]))) this.#state.mouseType = 0;
                if (this.#raw(number)[MARK_AT] & 9) this.remember(RawUserSelection.#read(this.#raw(number), X_AT), RawUserSelection.#read(this.#raw(number), Y_AT), this.#raw(number)[TYPE_AT]);
                if ((this.#type(this.#raw(number)[TYPE_AT]).flags2 & MOVABLE) && !(this.#raw(number)[MARK_AT] & 1)) {
                    if (!this.#state.selected) need(this.#hooks.assertion)();
                    if (this.#state.selected) {
                        if (!need(this.#hooks.moving)(sid) && activateMove && (need(this.#hooks.editor)() || this.#raw(number)[OWNER_AT] === need(this.#hooks.localOwner)())) need(this.#hooks.highlight)(5);
                        need(this.#hooks.movementPreview)(sid);
                    }
                }
                need(this.#hooks.sound)(RawUserSelection.#read(this.#raw(number), X_AT), RawUserSelection.#read(this.#raw(number), Y_AT), 'select.wav', 0);
            }
            return number;
        }
    }

    // 4d5c60: 정지/콘솔 관문 없이 enabled 만 읽고, 켜져 있으면 다중 선택 정리 → 현재 selected 0 → 강조 0 이다(기존 단일 선택 가상 통지를 대신 호출하지 않는다).
    clear() {
        if (need(this.#hooks.enabled)()) { this.clearMultiple(); this.#state.selected = 0; need(this.#hooks.highlight)(0); }
    }

    // 004da6b0: 대상이 유효하고 죽음/반납이 아니면 on(0/1, 0xffffffff 는 현재 반대) 로 토글한다. 현재 선택 대상이면 승격/해제, 아니면 켜질 때 목록 추가·소리·갱신이다. 명시 값 2 같은 비정규 DWORD 도 bool 로 바꾸지 않는다.
    toggleMultiple(number, on) {
        number >>>= 0; on >>>= 0;
        if (!number || number >= this.#pool.capacity || number > MAX_SID || (this.#raw(number)[FLAGS_AT] & 3)) return;
        const sid = this.#slot(number); const marked = (this.#raw(number)[MARK_AT] >> 2) & 1;
        if (on === 0xffffffff) on = (~marked) & 1;
        if (marked === on) return;
        need(this.#hooks.selectNotice)(sid, on);
        if (this.#state.selected === number) {
            if (!this.#state.multipleCount) {
                if (!need(this.#hooks.enabled)() || need(this.#hooks.blocked)()) return;
                this.clearMultiple(); if (this.#state.selected) this.#clearPrevious(this.#slot(this.#state.selected)); need(this.#hooks.highlight)(0); return;
            }
            const current = this.#multiple(); if (current.length === 0) throw new RangeError('다중 선택 승격의 현재 첫 원소 없음');
            this.#state.selected = current[0] >>> 0; number = this.#state.selected;
        } else if (on) {
            need(this.#hooks.appendMultiple)(number); need(this.#hooks.sound)(RawUserSelection.#read(this.#raw(number), X_AT), RawUserSelection.#read(this.#raw(number), Y_AT), 'select.wav', 0); need(this.#hooks.refresh)(sid); return;
        }
        need(this.#hooks.removeMultiple)(number); need(this.#hooks.refresh)(sid); // 원문은 승격된 selected 가 아니라 처음 보관한 대상의 +88 을 호출한다.
    }

    // 004da39a..40e: Shift 클릭 관문이다. 정지이거나 대상이 없거나 이동 불가 TYPE 이면 아무것도 하지 않고, 기본 선택이 없거나 죽음/이동 불가이면 단일 선택, 아니면 추가 토글이다. 소유자 제한은 추가하지 않는다.
    extendSelection(number, forceSelect) {
        number >>>= 0;
        if (need(this.#hooks.paused)() || !number || !(this.#type(this.#raw(number)[TYPE_AT]).flags2 & MOVABLE)) return;
        if (!this.#state.selected || (this.#raw(this.#state.selected)[FLAGS_AT] & 1) || !(this.#type(this.#raw(this.#state.selected)[TYPE_AT]).flags2 & MOVABLE)) this.select(number, forceSelect, 1);
        else this.toggleMultiple(number, 0xffffffff);
    }

    // 004d5500: enabled 를 0 으로 쓰고 현재 개수/배열을 각 전송 뒤 다시 읽으며 유효하고 죽음/반납이 아닌 목록 항목마다 move(번호, x, y)를 보낸 뒤 enabled 를 1 로 쓰고 다중 정리를 한다. 전송 실패 때 enabled 를 1 로 되돌리지 않는다.
    moveMultiple(x, y, move) {
        need(this.#hooks.writeEnabled)(0);
        // 원문 현재 signed 개수와 다음 원소를 재조회해 콜백의 변경을 보존한다.
        for (let index = 0; index < (this.#state.multipleCount | 0); index += 1) {
            const current = this.#multiple(); if (index >= current.length) throw new RangeError('다중 이동의 현재 배열 물리 범위');
            const number = current[index] >>> 0;
            if (number && number < this.#pool.capacity && number <= MAX_SID && !(this.#raw(number)[FLAGS_AT] & 3)) need(move)(number, x >>> 0, y >>> 0);
        }
        need(this.#hooks.writeEnabled)(1); this.clearMultiple();
    }

    // Carrier(4268c0/492080)의 메뉴 입구다: 대상 +40 의 비트 1 이 없으면 Select(sid,0,0) 뒤 공통 메뉴를 열고 그 반환 DWORD 를 선택 결과와 무관하게 그대로 돌려준다.
    carrierMenu(sid, event, common) {
        if (!(this.#raw(sid)[MARK_AT] & 1)) this.select(sid, 0, 0);
        return need(common)(sid, event);
    }

    // Archer(41e660)의 메뉴 입구다: 둘째 인자는 0 으로 두고 셋째 인자만 1 로 선택한 뒤(이동 모드의 별도 분기를 제어한다) 공통 메뉴를 연다.
    archerMenu(sid, event, common) {
        if (!(this.#raw(sid)[MARK_AT] & 1)) this.select(sid, 0, 1);
        return need(common)(sid, event);
    }
}
