// Fort 파일의 Badges 섹션(원본 28개 기록 장부)과 Technology 섹션을 같은 로딩 깊이/내용물 몸체로 연결하는 로더를 옮긴 모듈이다(cpppj `o/RawFortMetaLoad.cpp/.h`, 10.78 전용).
//  - setBadge(type, count)(0047d800): count 가 0..255 가 아니면 진단을 보고하고 계속한다. 진단 뒤 현재 장부를 다시 읽어, 28개를 앞에서부터 선형 검색해 처음 일치한 type DWORD 기록의 count DWORD(+216)에만 count 의 32비트를 쓴다(없으면 쓰지 않는다).
//  - findBadge(type): 같은 선형 검색이며 못 찾으면 −1 이다(type 이 중복돼도 처음 기록만).
//  - loadBadges(stream)(004be8e0): 조회 위치를 0 으로 놓고 Template 없음 진단 → 섹션이 없거나 끝이 0 이면 돌아감 → 개수 BYTE → (type BYTE, count BYTE) 쌍마다 setBadge. 저장 순서를 그대로 따르며 새 종류/개수로 정렬하지 않는다.
//    loadBadgesSelected(select)는 Template 진단 뒤에 현재 섹션을 고르는 실제 선택을 연결한다(선택 효과는 진단이 바꾼 현재 상태 뒤에 한다).
//  - loadTechnology(stream)(004beee0): 깊이 +1 → Template 없음/isMine 아님 진단 → 기술 부모 조회 → 섹션 선택 → (섹션이 있고 끝이 0 이 아니면) 변환표 준비 → 내용물 생성(flags 0x11, 느슨한 변환) → 현재 변환표가 있으면 해제하고 없음으로 표시 → 항상 기술 UI 갱신 → 깊이 −1.
//    실패하면 깊이만 복구하고(앞의 실제 효과는 되돌리지 않음) 예외를 다시 던진다. loadTechnologySelected(select)는 선택을 기술 부모 조회 뒤의 원본 위치에서 수행한다.
//  - makeFortMetaContentsHooks(pool, objects, fort, hooks): 이미 TypeNames 파싱을 끝낸 같은 Fort 의 parsed Technology 목록을 같은 풀의 `FortObjectLoad.contents` 로 전달하는 내용물 경계를 만든다.
// 장부(badges)는 `RawFortBadgesGlobals.records()` 같은 6160 BYTE 배열이며 깊이는 `{ value }` 상자(Money/Template/사제와 공유하는 현재 DWORD)이다. 내용물 생성/TypeNames/화면 출력은 실제 하위 몸체를 호출자가 연결한다.
// CD 10.72 에서 Badges 진입 때 깊이를 +1/−1 하는 분기는 웹 범위 밖이라 옮기지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E226: Fort 메타(Badges/Technology) 로더 이식으로 추가)

import { fortSection } from '../assets/fort.js';

// 실제 MetaDisplay 기록 수/stride 와 count DWORD 위치이다. (2026-11-10 WB04-E226)
const BADGE_RECORDS = 28;
const BADGE_STRIDE = 220;
const BADGE_COUNT = 216;
// 원본 진단이 보고된 뒤 계속할 수 있는 각 진단의 의미(cpppj FortMetaAssertion)이다. (2026-11-10 WB04-E226)
export const FortMetaAssertion = Object.freeze({
    MissingBadgesTemplate: 0, // Badges wrapper 의 Template 첫 DWORD 가 0 이다.
    BadgeCountRange: 1, // 직접 setter count 가 0..255 범위를 벗어났다.
    MissingTechnologyTemplate: 2, // Technology wrapper 의 Template 첫 DWORD 가 0 이다.
    TechnologyNotMine: 3, // 실제 현재 플레이어 Template 주소와 this 가 다르다.
});
// 필수 하위 효과 이름이다. 누락은 생성 때 거부한다. (2026-11-10 WB04-E226)
const REQUIRED_HOOKS = Object.freeze(['assertion', 'loadingDepthChanged', 'technologyParent', 'prepareTypeNames', 'contents', 'releaseTypeConversion', 'refreshTechnologyUi']);
// Technology 내용물 생성 flags(0x11)와 DWORD 상한이다. (2026-11-10 WB04-E226)
const TECHNOLOGY_CONTENT_FLAGS = 0x11;
const DWORD_MAX = 0xffffffff;

// Fort 메타 로더이다. 사용: new RawFortMetaLoad(pool, globals.records(), depthBox, state, hooks).loadBadges(stream).
//  pool: 10.78 SidPool, badges: 28×220 BYTE 장부(Uint8Array), loadingDepth: { value }, state: { templatePresent, templateSlot, localOwner, hasTypeConversion }(진단/하위 호출 뒤 다시 읽음),
//  hooks: { assertion(kind), loadingDepthChanged(value), technologyParent() → SID, prepareTypeNames(), contents(parent, stream, flags, relaxedConversion), releaseTypeConversion(), refreshTechnologyUi() } (모두 필수).
// stream 은 { bytes: Uint8Array, position, end, present } 이다(원본 MemStream 의 현재 pointer 존재/pos/end 와 저장 바이트). (2026-11-10 WB04-E226)
export class RawFortMetaLoad {
    // 같은 풀·장기 소유한 장부·공유 깊이·진단/하위 호출 뒤에 다시 읽는 상태·하위 효과이다. 장부는 이 객체보다 오래 살아야 한다.
    #pool;
    #badges;
    #loadingDepth;
    #state;
    #hooks;

    // 판본·장부 크기·깊이 상자·필수 하위 효과를 로더 몸체보다 앞서 확인한다. (2026-11-10 WB04-E226)
    constructor(pool, badges, loadingDepth, state, hooks = {}) {
        if (pool?.edition !== 'patch1078') throw new TypeError('Fort 메타 로더는 10.78 SidPool 만 받습니다.');
        if (!(badges instanceof Uint8Array) || badges.length !== BADGE_RECORDS * BADGE_STRIDE ||
            typeof loadingDepth?.value !== 'number' || !state || REQUIRED_HOOKS.some((name) => typeof hooks[name] !== 'function')) throw new TypeError('Fort meta 실제 badge 기록/하위 효과 누락');
        this.#pool = pool;
        this.#badges = badges;
        this.#loadingDepth = loadingDepth;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 비정렬 little endian DWORD 의 현재 바이트를 조합한다. (2026-11-10 WB04-E226)
    #readBadge(offset) {
        let value = 0;
        for (let byte = 0; byte < 4; byte += 1) value |= this.#badges[offset + byte] << (byte * 8); // 원본 DWORD 폭의 각 BYTE 만 읽는다.
        return value >>> 0;
    }

    // 원본 count 필드의 네 BYTE 만 대입한다. type/padding/이웃 기록은 쓰지 않는다. (2026-11-10 WB04-E226)
    #writeBadge(offset, value) {
        for (let byte = 0; byte < 4; byte += 1) this.#badges[offset + byte] = (value >>> (byte * 8)) & 0xff;
    }

    // 실제 선형 순서에서 처음 일치한 type DWORD 의 기록 번호이다. 종류가 중복돼도 처음 기록만 반환하며 없으면 −1 이다. (2026-11-10 WB04-E226)
    findBadge(type) {
        const wanted = type >>> 0;
        for (let index = 0; index < BADGE_RECORDS; index += 1) if (this.#readBadge(index * BADGE_STRIDE) === wanted) return index; // 종류 중복도 처음 기록만 반환한다.
        return -1;
    }

    // 0047d800: 범위 밖 count 는 진단을 보고하고 계속하며, 진단이 바꾼 현재 장부를 다시 검색한 뒤 처음 일치한 기록의 count DWORD 에 32비트를 쓴다. (2026-11-10 WB04-E226)
    setBadge(type, count) {
        const signed = count | 0;
        if (signed < 0 || signed > 255) this.#hooks.assertion(FortMetaAssertion.BadgeCountRange);
        const index = this.findBadge(type);
        if (index !== -1) this.#writeBadge(index * BADGE_STRIDE + BADGE_COUNT, signed >>> 0);
    }

    // Badges 의 raw pointer 직접 읽기는 호스트 물리 범위 밖이면 기존 효과를 보존한 채 거부한다. (2026-11-10 WB04-E226)
    #readByte(stream) {
        if (stream.position >= stream.bytes.length) throw new RangeError('Fort Badges 실제 stream 메모리 소진');
        const value = stream.bytes[stream.position];
        stream.position += 1;
        return value;
    }

    // 현재 로딩 깊이 쓰기와 같은 소비자의 관찰을 함께 실행한다. DWORD 는 32비트로 감긴다. (2026-11-10 WB04-E226)
    #changeDepth(value) {
        this.#loadingDepth.value = value >>> 0;
        this.#hooks.loadingDepthChanged(this.#loadingDepth.value);
    }

    // 004be8e0: 조회 위치를 0 으로 놓은 stream 으로 Badges 를 읽는다. 독립 원문 입력의 선택 효과도 같은 시점에 수행한다. (2026-11-10 WB04-E226)
    loadBadges(stream) {
        this.loadBadgesSelected(() => { stream.position = 0; return stream; });
    }

    // 실제 Badges 선택은 현재 Template 진단 뒤에 수행한다(10.78 은 깊이를 건드리지 않는다). 섹션이 없거나 끝이 0 이면 돌아가고, 아니면 개수 BYTE 뒤 (type, count) 쌍마다 setBadge 한다. (2026-11-10 WB04-E226)
    loadBadgesSelected(select) {
        if (typeof select !== 'function') throw new TypeError('Fort Badges 실제 섹션 선택 누락');
        if (!this.#state.templatePresent) this.#hooks.assertion(FortMetaAssertion.MissingBadgesTemplate);
        const stream = select();
        if (!stream.present || !stream.end) return;
        const count = this.#readByte(stream);
        // 저장 BYTE 순서를 그대로 따르며 앞 setter 효과를 새 종류/개수로 정렬하지 않는다.
        for (let index = 0; index < count; index += 1) {
            const type = this.#readByte(stream);
            const quantity = this.#readByte(stream);
            this.setBadge(type, quantity);
        }
    }

    // 004beee0: 독립 원문의 cursor 초기화도 실제 기술 부모 조회 뒤에 한다. 현재 부모/변환표/깊이를 다시 읽고 항상 UI 를 호출한 뒤 원래 깊이만 복구한다. (2026-11-10 WB04-E226)
    loadTechnology(stream) {
        this.loadTechnologySelected(() => { stream.position = 0; return stream; });
    }

    // 실제 Technology 선택을 깊이/Template/isMine 진단과 기술 부모 조회 뒤의 원본 위치에서 수행한다. 선택 중 실패도 깊이를 복구하지만 앞의 실제 효과는 되돌리지 않는다. (2026-11-10 WB04-E226)
    loadTechnologySelected(select) {
        if (typeof select !== 'function') throw new TypeError('Fort Technology 실제 섹션 선택 누락');
        this.#changeDepth(this.#loadingDepth.value + 1);
        try {
            if (!this.#state.templatePresent) this.#hooks.assertion(FortMetaAssertion.MissingTechnologyTemplate);
            if (this.#state.templateSlot !== this.#state.localOwner) this.#hooks.assertion(FortMetaAssertion.TechnologyNotMine);
            const parent = this.#hooks.technologyParent();
            const stream = select();
            if (stream.present && stream.end) {
                this.#hooks.prepareTypeNames();
                this.#hooks.contents(parent, stream, TECHNOLOGY_CONTENT_FLAGS, true);
                if (this.#state.hasTypeConversion) this.#hooks.releaseTypeConversion();
                this.#state.hasTypeConversion = false;
            }
            this.#hooks.refreshTechnologyUi();
        } catch (error) {
            this.#changeDepth(this.#loadingDepth.value - 1); // 호스트 실패에서 깊이를 복구하지만 앞의 실제 효과는 되돌리지 않는다.
            throw error;
        }
        this.#changeDepth(this.#loadingDepth.value - 1);
    }

    // 실제 내용물 연결이 같은 풀인지 확인하는 데 쓰는 풀이다. (2026-11-10 WB04-E226)
    get pool() { return this.#pool; }
}

// 이미 원본 TypeNames 파싱을 끝낸 같은 Fort 의 실제 Contents 몸체만 연결한다. 변환표 준비/해제·현재 기술 부모·UI 의 전체 복원은 이 함수의 범위가 아니다.
// objects 는 같은 풀의 `FortObjectLoad`(contents(parent, list, flags))이고 fort 는 `parseFort` 결과이다. 반환은 hooks 에 contents 만 바꾼 새 객체이다. (2026-11-10 WB04-E226)
export function makeFortMetaContentsHooks(pool, objects, fort, hooks) {
    if (pool !== objects?.pool) throw new TypeError('Fort meta Contents SID 풀 불일치');
    return {
        ...hooks,
        contents: (parent, stream, flags, relaxedConversion) => {
            if (!relaxedConversion) throw new Error('Fort Technology 변환 0 디버그 진단 모드 미복원');
            const bytes = fortSection(fort, 'Technology');
            const trailing = fort.technologyTrailingBytes;
            if (trailing > bytes.length || bytes.length - trailing > DWORD_MAX) throw new RangeError('Fort Technology parsed 원본 소비 폭 오류');
            if (stream.bytes?.buffer !== bytes.buffer || stream.bytes.byteOffset !== bytes.byteOffset || stream.bytes.length !== bytes.length) throw new TypeError('Fort Technology parsed 목록과 현재 원본 섹션 불일치');
            objects.contents(parent, fort.technology, flags);
            stream.position = bytes.length - trailing;
        },
    };
}
