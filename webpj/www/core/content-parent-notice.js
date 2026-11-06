// 원본 10.78 내용물 부모 통지(가상 +0x24, form, reason) 분배를 옮긴 모듈이다. 부모의 현재 VT 로 실제 진입점을 고른다.
//  - 네 내장 Form 과 대부분의 자산은 같은 빈 ret8(0x41c100)이라 아무것도 읽거나 쓰지 않는다(TYPE/상태/인자 무관).
//  - 워커/다이스/워크샾/Player 등 파생 진입점(0x429560·0x449660·0x453b50·0x490750)은 derived 장치로 보낸다(없으면 오류).
//  - 지원 표에 없는 VT 는 호출 전에 거부한다.
// 근거: cpppj `o/RawContentParentNotice.cpp/.h`, `o/ContentNoticeVtables.inc`(squid-vtables.js 의 지원 자산 표 셋째 칸).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-06 WB04-D5a: Content 호스트 부모 통지 분배로 추가)
import { isSquidFormVtable, squidAssetVtable } from './squid-vtables.js';

// 원본 공통 빈 내용물 통지의 진입점이다(10.78). (2026-11-06 WB04-D5a)
export const EMPTY_CONTENT_NOTICE = 0x0041c100;

// 현재 부모 VT 의 +0x24 진입점으로 통지를 분배한다. 사용: new RawContentParentNotice(pool, derived).notice(parent, form, reason). (2026-11-06 WB04-D5a)
export class RawContentParentNotice {
    // 같은 실제 풀과 파생 통지 장치(parent, form, reason)이다.
    #pool;
    #derived;

    // 풀과 파생 장치는 이 분배기보다 오래 살아야 한다.
    constructor(pool, derived = null) {
        if (pool?.edition !== 'patch1078') throw new TypeError('내용물 부모 통지는 10.78 SidPool 만 받습니다.');
        if (derived !== null && typeof derived !== 'function') throw new TypeError('파생 내용물 통지는 함수여야 합니다.');
        this.#pool = pool;
        this.#derived = derived;
    }

    // 부모의 현재 VT 가 고른 원본 진입점이다. 알 수 없는 VT 는 거부한다.
    nativeEntry(parent) {
        const raw = this.#pool.slot(parent);
        const table = (raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)) >>> 0;
        if (isSquidFormVtable(table)) return EMPTY_CONTENT_NOTICE;
        const row = squidAssetVtable(table);
        if (row === null) throw new Error(`내용물 부모의 미복원 VT: ${table}`);
        return row[2];
    }

    // 빈 몸체는 그대로 돌아가고 파생 몸체만 실제 장치를 부른다.
    notice(parent, form, reason) {
        const entry = this.nativeEntry(parent);
        if (entry === EMPTY_CONTENT_NOTICE) return;
        if (!this.#derived) throw new Error(`실제 내용물 부모 파생 통지 미연결: ${entry}`);
        this.#derived(parent, form, reason);
    }
}
