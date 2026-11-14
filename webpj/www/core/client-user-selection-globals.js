// 실제 선택·강조의 아홉 DWORD와 P/R 누적 DWORD·다중 선택 배열을 Runtime 보다 오래 보관하는 Client 수명 전역이다(cpppj `client/ClientUserSelectionGlobals.cpp/.h`, 10.78).
// 원본 주소: 005434e4(enabled, 기본 1)·005cae90(mode)·005caea0(selected)·005caea4(mouseType)·005cae94/98(원시 화면 정수 좌표)·005caf38/3c(기억한 두 float 의 원시 비트)·
// 005caf34(signed 다중 개수)·005caf40(P/R 누적 선택 DWORD)·005caf2c(다중 선택 DWORD 30칸). 모든 값은 bool/SID16 으로 좁히지 않는 전체 DWORD 이다.
// RawUserSelection/RawUserHighlight 는 이 owner 의 같은 현재 값을 빌린다(selectionState/highlightState 는 사본이 아니라 접근자 보기이다).
// 이 모듈은 DOM 을 쓰지 않는다. 사용: const owner = new ClientUserSelectionGlobals(); new RawUserSelection(pool, types, owner.selectionState(), hooks);
// (2026-11-15 WB04-E495: 사용자 선택 처리기 이식과 함께 장기 소유자로 추가)

// 원문 다중 선택 배열의 고정 용량이다(원본 확대 없음).
export const MULTIPLE_CAPACITY = 30;
// readDword/writeDword 가 받는 DWORD 필드 이름 목록이다(원문 C++ enum 순서).
export const SELECTION_FIELDS = Object.freeze(['Enabled', 'Mode', 'Selected', 'MouseType', 'MouseX', 'MouseY', 'RememberedX', 'RememberedY', 'MultipleCount', 'PriestSelections']);

// 선택 상태의 장기 단일 소유자이다. 복제하지 않으며 처리기들이 같은 현재 값을 빌린다.
export class ClientUserSelectionGlobals {
    // PE 의 enabled 1 과 나머지 BSS 0 만으로 구성한다. 선택 SID 도 소비 전까지 전체 DWORD 이다.
    enabled = 1; mode = 0; selected = 0; mouseType = 0;
    // 원시 화면 정수 좌표(커서의 보정 좌표와 별개)와 기억한 두 float 의 원시 DWORD 비트이다.
    mouse = { x: 0, y: 0 }; remembered = [0, 0];
    // 반복 조건이 매번 다시 읽는 signed DWORD 개수와 P/R 의 누적 선택 DWORD(일반 World reset/선택 정리에서도 유지)이다.
    multipleCount = 0; priestSelections = 0;
    // 005caf2c 가 가리키는 실제 DWORD 요소들이다(호스트 배열 머리를 Native 포인터/머리와 같다고 하지 않는다).
    multiple = new Array(MULTIPLE_CAPACITY).fill(0);

    // 00414450 의 고정 용량 30·중복 제외 추가이다. 가득 찬 용량은 자동 확장하지 않으며 용량 관문이 중복 검색보다 먼저이다.
    appendMultiple(sid) {
        sid >>>= 0;
        if (this.multipleCount >= this.multiple.length) return;
        if (this.multipleCount < 0) throw new RangeError('다중 선택 추가의 음수 물리 인덱스');
        // 같은 SID 는 현재 개수의 앞부분에서만 찾는다.
        for (let index = 0; index < this.multipleCount; index += 1) if (this.multiple[index] === sid) return;
        this.multiple[this.multipleCount] = sid; this.multipleCount += 1;
    }

    // 0040ea00 처럼 전체 일치를 제거하고 남은 순서를 보존한다. 꼬리 칸은 지우지 않으며 음수 개수는 방문 없이 그대로 보존한다.
    removeMultiple(sid) {
        sid >>>= 0;
        if (this.multipleCount > this.multiple.length) throw new RangeError('다중 선택 제거의 현재 배열 물리 범위');
        let removed = 0;
        // 현재 개수 앞의 안정된 압축만 수행한다.
        for (let index = 0; index < this.multipleCount; index += 1) {
            const value = this.multiple[index];
            if (value === sid) removed += 1; else this.multiple[index - removed] = value;
        }
        this.multipleCount -= removed;
    }

    // 일반 World 의 새 입력 상태만 같은 주소에서 준비한다(Native Clear 나 prepared 대여에는 쓰지 않으며 priestSelections 는 유지한다).
    resetForGeneralWorld() {
        this.enabled = 1; this.mode = 0; this.selected = 0; this.mouseType = 0; this.mouse.x = 0; this.mouse.y = 0;
        this.remembered[0] = 0; this.remembered[1] = 0; this.multipleCount = 0; this.multiple.length = MULTIPLE_CAPACITY; this.multiple.fill(0);
    }

    // RawUserSelection 에 빌려 주는 현재 선택 상태 보기이다. 배열은 callback 의 재할당 뒤에도 다음 반복에서 현재 값을 다시 읽는다.
    selectionState() {
        const owner = this;
        return {
            get selected() { return owner.selected; }, set selected(value) { owner.selected = value >>> 0; },
            get mouseType() { return owner.mouseType; }, set mouseType(value) { owner.mouseType = value >>> 0; },
            get rememberedPoint() { return owner.remembered; },
            get multiple() { return owner.multiple; },
            get multipleCount() { return owner.multipleCount; }, set multipleCount(value) { owner.multipleCount = value | 0; },
            currentMultiple: () => owner.multiple,
        };
    }

    // RawUserHighlight 가 직접 빌리는 같은 선택·마우스 필드 보기이다(selectedSid 는 읽기 전용 용도이지만 같은 DWORD 이다).
    highlightState() {
        const owner = this;
        return {
            get mode() { return owner.mode; }, set mode(value) { owner.mode = value >>> 0; },
            get selectedSid() { return owner.selected; }, set selectedSid(value) { owner.selected = value >>> 0; },
            get mouseType() { return owner.mouseType; }, set mouseType(value) { owner.mouseType = value >>> 0; },
            get mouseX() { return owner.mouse.x; }, set mouseX(value) { owner.mouse.x = value | 0; },
            get mouseY() { return owner.mouse.y; }, set mouseY(value) { owner.mouse.y = value | 0; },
        };
    }

    // signed 좌표/개수를 참조로 위장하지 않고 전체 DWORD 비트(unsigned)를 반환한다. 모르는 필드 이름은 거부한다.
    readDword(field) {
        switch (field) {
        case 'Enabled': return this.enabled >>> 0;
        case 'Mode': return this.mode >>> 0;
        case 'Selected': return this.selected >>> 0;
        case 'MouseType': return this.mouseType >>> 0;
        case 'MouseX': return this.mouse.x >>> 0;
        case 'MouseY': return this.mouse.y >>> 0;
        case 'RememberedX': return this.remembered[0] >>> 0;
        case 'RememberedY': return this.remembered[1] >>> 0;
        case 'MultipleCount': return this.multipleCount >>> 0;
        case 'PriestSelections': return this.priestSelections >>> 0;
        default: throw new RangeError('선택 전역의 알려지지 않은 DWORD 조회');
        }
    }

    // 실제 필드 하나에만 쓰며 bool 정규화·SID16 절삭·이웃 필드 reset 을 추가하지 않는다. 좌표/개수는 signed 비트로 저장한다.
    writeDword(field, value) {
        const bits = value >>> 0;
        switch (field) {
        case 'Enabled': this.enabled = bits; return;
        case 'Mode': this.mode = bits; return;
        case 'Selected': this.selected = bits; return;
        case 'MouseType': this.mouseType = bits; return;
        case 'MouseX': this.mouse.x = bits | 0; return;
        case 'MouseY': this.mouse.y = bits | 0; return;
        case 'RememberedX': this.remembered[0] = bits; return;
        case 'RememberedY': this.remembered[1] = bits; return;
        case 'MultipleCount': this.multipleCount = bits | 0; return;
        case 'PriestSelections': this.priestSelections = bits; return;
        default: throw new RangeError('선택 전역의 알려지지 않은 DWORD 쓰기');
        }
    }
}
