// Player 의 기술 내용물(contained) 조회·전체 삭제·보장 생성(00490330/004903b0/00490420)과 40개 기술 목록 문자열 사슬(004914e0/004915c0)을 옮긴 모듈이다
// (cpppj `o/RawPlayerTechnology.cpp/.h`, 10.78 전용).
//  - find(owner, type): 타입 범위 진단(복귀 뒤 계속) → PlayerKnown 진단/현재 기술 부모(RawPlayerWorld.technology) → FindContained(mask 4, type, index 0).
//  - clear(owner): 부모를 한 번 캡처한 뒤 flags 4 내용물을 매회 처음부터 다시 찾아 실제 가상 삭제(flags 0)를 반복한다. 다음 SID 를 미리 캡처하지 않는다.
//  - ensure(owner, type): 타입 범위 진단 → 중첩 find(두 번째 타입 진단·PlayerKnown 진단 포함) → 없으면 부모를 다시 조회해 contained 생성(인자 0, flags 4, 부착 0x10, 품질 0).
//  - parseStep(text, list): ';' 목록의 현재 항목을 이름 조회한다. "all"(대소문자 무시)이면 0번부터 group 10 이 아닌 모든 TYPE 을 넣고, 조회 번호가 양수면 그것을 넣는다.
//    빈 항목이면 완료, 미등록 비빈 항목은 index 를 올리지 않아 원본이 같은 항목을 무한 반복한다(Stalled 로 관찰).
//  - addList(owner, text): 정상 종료 목록 전체를 먼저 만든 뒤 순서대로 ensure 한다. 무진행 입력은 생성 전에 명시 경계 오류다.
// hooks: findType(name) → 번호(-1=없음), assertion(kind) (타입 범위 진단, 선택 시 필수), destroy(sid, flags) (clear 에서 필수).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E56: Player 기술 목록 이식으로 추가)
import { configListItem } from './raw-mission-ai-settings.js';

// 원본의 서로 다른 타입 범위 진단 위치 이름이다(10.78 Player.cpp 0x363/0x376). 진단 뒤 조회/생성을 계속한다. (2026-11-08 WB04-E56)
export const PLAYER_TECHNOLOGY_ASSERTION = Object.freeze({ findTypeRange: 'FindTypeRange', ensureTypeRange: 'EnsureTypeRange' });

// parseStep 한 반복의 결과이다: 종료·진행·같은 항목 반복(미등록 비빈 이름). (2026-11-08 WB04-E56)
export const PLAYER_TECHNOLOGY_PARSE_STEP = Object.freeze({ complete: 'Complete', progress: 'Progress', stalled: 'Stalled' });

// 원본 기술 목록 용량(operator new(0xa0) 의 DWORD 40개)과 all 에서 제외하는 group, 기술 내용물 조건 mask 와 생성 인자이다. (2026-11-08 WB04-E56)
export const PLAYER_TECHNOLOGY_CAPACITY = 40;
const NO_GROUP = 10;
const TECHNOLOGY_MASK = 4;
const CREATE_FLAGS = 4;
const ATTACH_FLAGS = 0x10;

// 원본 DWORD 목록이다: types(40칸 저장소)·count(현재 개수, 최대 40)·index(004914e0 의 현재 ';' 항목 번호, signed 32비트). (2026-11-08 WB04-E56)
export class PlayerTechnologyList {
    // 실제 원소 저장소이다. count 뒤의 칸은 목록으로 쓰지 않는다.
    types = new Int32Array(PLAYER_TECHNOLOGY_CAPACITY);
    // 현재 개수이다.
    count = 0;
    // 현재 세미콜론 항목 번호이다.
    index = 0;
}

// ASCII 대문자만 소문자로 바꾼다(원본 _stricmp 비교, 그 밖의 바이트는 그대로). (2026-11-08 WB04-E56)
function asciiLower(text) {
    return String(text).replace(/[A-Z]/g, (letter) => String.fromCharCode(letter.charCodeAt(0) + 32));
}

// 이름 "all" 을 ASCII 대소문자 무시로 비교한다. (2026-11-08 WB04-E56)
function isAll(text) {
    return text.length === 3 && asciiLower(text) === 'all';
}

// Player 기술 목록 처리기이다. 같은 풀의 RawPlayerWorld·RawCarrierPreDestroy(findContained)·RawContainedCreate 와 번호순 TYPE 표(group 포함)를 빌린다.
// 사용: const technology = new RawPlayerTechnology(player, carrier, create, types, hooks); technology.addList(1, 'sunWalker;windBattery'). (2026-11-08 WB04-E56)
export class RawPlayerTechnology {
    // 현재 9 Player 와 PlayerKnown 진단을 공유하는 RawPlayerWorld 이다.
    #player;
    // 기존 004ac9f0 의 실제 contained 체인 조회(RawCarrierPreDestroy)이다.
    #finder;
    // 기존 Factory/Form/부모 부착의 실제 생성 사슬(RawContainedCreate)이다.
    #create;
    // 번호순 TYPE 표(all 분류와 타입 범위 진단에 직접 쓴다)이다.
    #types;
    // 실제 이름 해석·가상 삭제·타입 진단 경계이다.
    #hooks;

    // 모든 하위가 같은 10.78 풀이어야 하며 이름 조회는 필수이다.
    constructor(player, finder, create, types, hooks = {}) {
        const pool = player?.pool;
        if (pool?.edition !== 'patch1078' || finder?.pool !== pool || create?.pool !== pool) throw new TypeError('Player 기술은 같은 10.78 실제 풀이 필요합니다.');
        if (!Array.isArray(types) || types.length === 0 || types.length > 0x7fffffff) throw new RangeError('Player 기술 TYPE 표가 필요합니다.');
        if (typeof hooks?.findType !== 'function') throw new TypeError('Player 기술 실제 타입 조회가 필요합니다.');
        this.#player = player;
        this.#finder = finder;
        this.#create = create;
        this.#types = types;
        this.#hooks = { ...hooks };
    }

    // 같은 raw 세계 연결의 동일성 검증용 풀이다.
    get pool() { return this.#player.pool; }

    // 타입 범위 진단은 false 성공이나 조기 반환으로 바꾸지 않는다(진단 뒤 계속).
    #checkType(type, where) {
        const value = type | 0;
        if (value < 1 || value >= this.#types.length) {
            if (typeof this.#hooks.assertion !== 'function') throw new Error('Player 기술 실제 타입 assert 경계 누락');
            this.#hooks.assertion(where);
        }
    }

    // Technology DWORD 를 실제 SID 폭과 같은 풀 범위로 바꾼다(PlayerKnown 진단은 RawPlayerWorld 가 한다).
    #parent(owner) {
        const value = this.#player.technology(owner);
        if (value > 0xffff || value >= this.pool.capacity) throw new RangeError('Player 기술 실제 부모 SID 범위 오류');
        return value;
    }

    // 00490330: 현재 부모 기술 중 kind WORD 와 flags 4 가 모두 맞는 첫 실제 SID(없으면 0)이다.
    find(owner, type) {
        this.#checkType(type, PLAYER_TECHNOLOGY_ASSERTION.findTypeRange);
        return this.#finder.findContained(this.#parent(owner), TECHNOLOGY_MASK, type >>> 0, 0);
    }

    // 004903b0: 삭제가 바꾼 같은 체인을 매회 처음부터 조회하며 다음 SID 를 미리 캡처하지 않는다.
    clear(owner) {
        const parent = this.#parent(owner);
        // 실제 가상 삭제는 해당 내용물을 체인에서 제거해야 한다. 실패/미복원은 callback 이 보고한다.
        for (;;) {
            const found = this.#finder.findContained(parent, TECHNOLOGY_MASK, 0, 0);
            if (!found) return;
            if (typeof this.#hooks.destroy !== 'function') throw new Error('Player 기술 실제 가상 삭제 경계 누락');
            this.#hooks.destroy(found, 0);
        }
    }

    // 00490420: 중첩 find 는 두 타입 진단 위치와(없는 경우) 두 PlayerKnown 진단을 그대로 일으킨다.
    ensure(owner, type) {
        this.#checkType(type, PLAYER_TECHNOLOGY_ASSERTION.ensureTypeRange);
        if (!this.find(owner, type)) this.#create.create(this.#parent(owner), type >>> 0, 0, CREATE_FLAGS, ATTACH_FLAGS, 0);
    }

    // 원본 목록의 용량 확인 → 중복 검사 → 끝 추가 순서이다. 가득 차면 중복 검사도 하지 않는다.
    static #append(list, type) {
        if (list.count >= PLAYER_TECHNOLOGY_CAPACITY) return;
        const value = type | 0;
        // 현재 개수 안에서만 중복을 찾는다.
        for (let index = 0; index < list.count; index += 1) if (list.types[index] === value) return;
        list.types[list.count] = value;
        list.count += 1;
    }

    // 004914e0 의 한 반복이다. 빈 항목이면 완료, 미등록 비빈 입력은 건너뛰지 않으며 호출자가 무진행(Stalled)을 관찰한다.
    parseStep(text, list) {
        if (!(list instanceof PlayerTechnologyList) || list.count > PLAYER_TECHNOLOGY_CAPACITY) throw new RangeError('Player 기술 목록 실제 40개 범위 오류');
        const previous = list.index;
        const value = configListItem(text, list.index);
        // 빈 항목도 원본처럼 이름 조회를 한 번 한다.
        const type = this.#hooks.findType(value) | 0;
        if (value.length && isAll(value)) {
            // 원본은 0번부터 전체 TYPE 을 순서대로 보고 group 10 만 제외한다.
            for (let index = 0; index < this.#types.length; index += 1) if (this.#types[index].group !== NO_GROUP) RawPlayerTechnology.#append(list, index);
            list.index = (list.index + 1) | 0;
        }
        if (type > 0) {
            RawPlayerTechnology.#append(list, type);
            list.index = (list.index + 1) | 0;
        }
        if (!value.length) return PLAYER_TECHNOLOGY_PARSE_STEP.complete;
        return list.index === previous ? PLAYER_TECHNOLOGY_PARSE_STEP.stalled : PLAYER_TECHNOLOGY_PARSE_STEP.progress;
    }

    // 004915c0: 전체 목록 생성이 끝나기 전에는 어떤 ensure 도 실행하지 않는다.
    addList(owner, text) {
        const list = new PlayerTechnologyList();
        // 미등록 항목의 무한 반복을 건너뛰기 성공으로 바꾸지 않고 잘못된 스크립트 경계로 보고한다.
        for (;;) {
            const step = this.parseStep(text, list);
            if (step === PLAYER_TECHNOLOGY_PARSE_STEP.complete) break;
            if (step === PLAYER_TECHNOLOGY_PARSE_STEP.stalled) throw new Error('원본 004914e0 이 미등록 기술 이름에서 같은 항목을 무한 반복합니다');
        }
        // 먼저 만든 40개까지의 순서/중복 제거 목록을 바꾸지 않고 실제 ensure 에 넘긴다.
        for (let index = 0; index < list.count; index += 1) this.ensure(owner, list.types[index]);
    }
}

// 원본 이름표 조회(0049a860 RiftTypeTable::Find)를 번호순 TYPE 레코드의 첫 이름 일치(ASCII 대소문자 무시)로 연결한다. 이름 없는 타입은 빈 이름을 찾을 때만 맞는다. (2026-11-08 WB04-E56)
export function bindPlayerTechnologyTypes(records, hooks = {}) {
    if (!Array.isArray(records)) throw new TypeError('Player 기술 이름 조회에는 TYPE 레코드 표가 필요합니다.');
    return {
        ...hooks,
        findType: (name) => {
            const wanted = asciiLower(name);
            // 0번부터 첫 일치를 고른다.
            for (let index = 0; index < records.length; index += 1) if (asciiLower(records[index].name ?? '') === wanted) return index;
            return -1;
        },
    };
}
