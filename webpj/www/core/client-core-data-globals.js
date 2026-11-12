// Client 수명의 현재 CoreData 포인터 슬롯·활성 DWORD·lastKnown 10BYTE·9기록 장부와 현재 Player 뷰 슬롯을 소유하는 모듈이다(cpppj `client/ClientCoreDataGlobals.cpp/.h`, 10.78 전용, 기준선 1f00a25a 뒤 변경 없음).
// 원문: 현재 포인터 00595350(null 또는 Player+0x6c·캐시·외부 물리의 10BYTE 뷰), 활성 00595348(최초 0), lastKnownCoreData 005953f0(10BYTE), 9×16BYTE 장부 00595360(144BYTE), 현재 Player 물리 뷰 0059534c(소유하지 않음).
// Player 원문 기록 자체는 외부 실제 소유자(`ClientPlayerRecords`)에서 빌리고 현재/캐시 저장소만 장기 소유한다. 새 Client 에서만 장부 9기록과 lastKnown 을 한 번 구성하며(004feac0 → 42ce70) Session 생성/파괴가 다시 초기화하지 않는다.
// 사용: const core = new ClientCoreDataGlobals(); core.players = playerBytes; core.select(player, metamap, owner, templateOwner); core.read(player, metamap, hooks); core.restore(player, metamap, state, owner, hooks); core.storeMetadata(owner, subscriber, core10).
// `player` 는 `PlayerWorldState`(templateOwner·battleOwner·playerKnown 속성), `metamap` 은 읽는 시점의 현재 DWORD 를 돌려주는 `{ value }` 상자나 함수나 수이다. 모든 참조 대상은 호출과 저장한 callback 보다 오래 살아야 한다.
// 기준선 뒤 변경 판정: 없음(①). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-12 WB04-E331: 4-D ClientCoreDataGlobals 이식)
import { readPlayerCoreData, restorePlayerCoreData, selectPlayerCoreData } from './raw-player-core-data.js';
import { initializePlayerCore, initializePlayerCoreRecords, PLAYER_CORE_RECORD_BYTES, writePlayerCoreMetadata } from './raw-player-core-records.js';

// metamap 인자(상자/함수/수)를 읽는 시점의 현재 값 조회 함수로 바꾼다. (2026-11-12 WB04-E331)
function metamapReader(metamap) {
    if (typeof metamap === 'function') return () => metamap() >>> 0;
    if (metamap && typeof metamap === 'object' && 'value' in metamap) return () => metamap.value >>> 0;
    if (typeof metamap === 'number') return () => metamap >>> 0;
    throw new TypeError('metamap 은 { value } 상자·함수·수여야 합니다.');
}

// Client 수명 CoreData 저장소이다. 접근자는 호출마다 같은 객체/배열을 돌려준다(주소 고정). (2026-11-12 WB04-E331)
export class ClientCoreDataGlobals {
    // 00595350 의 현재 포인터 슬롯이다(최초 NULL, Session 생성/파괴 때 임의로 지우지 않음).
    #current = null;
    // 00595348 의 활성 DWORD 이다(최초 0).
    #active = 0;
    // 00595360 부터의 9기록 장부 144BYTE 이다. 끝 2BYTE padding 은 별도이다.
    #records = new Uint8Array(PLAYER_CORE_RECORD_BYTES);
    // 005953f0 lastKnownCoreData 10BYTE 이다. 장부와 별도 물리이다.
    #cached = new Uint8Array(10);
    // 0059534c 의 현재 실제 Player 물리 뷰 슬롯이다(할당 자체를 소유하지 않음, 최초 빈 뷰).
    #players = new Uint8Array(0);

    // 새 Client 에서만 실제 CRT 표 순서대로 9기록을 초기화한 뒤 별도 lastKnown 10BYTE 를 같은 16BYTE 자식 초기화로 구성한다. (2026-11-12 WB04-E331)
    constructor() {
        initializePlayerCoreRecords({ bytes: this.#records, origin: 0 });
        initializePlayerCore(this.#cached);
    }

    // 현재 포인터 슬롯(null 또는 10BYTE 뷰)이다. 슬롯 자체를 읽고 쓴다. (2026-11-12 WB04-E331)
    get current() { return this.#current; }

    // 현재 포인터 슬롯을 바꾼다(null 또는 Uint8Array 뷰). (2026-11-12 WB04-E331)
    set current(view) { this.#current = view; }

    // 활성 DWORD 이다. 쓰기는 DWORD 로 감긴다. (2026-11-12 WB04-E331)
    get active() { return this.#active; }

    set active(value) { this.#active = value >>> 0; }

    // 같은 고정 lastKnown 10BYTE 를 빌린다(복제 없음). (2026-11-12 WB04-E331)
    cached() { return this.#cached; }

    // 같은 Client 수명의 9기록 장부 144BYTE 전체를 빌린다. (2026-11-12 WB04-E331)
    records() { return this.#records; }

    // 현재 Player 물리 뷰 슬롯이다. 외부 실제 Player 주소를 바꿀 수 있으며 새 base 선택도 같은 슬롯으로 관측한다. (2026-11-12 WB04-E331)
    get players() { return this.#players; }

    set players(view) { this.#players = view; }

    // 원문 metadata(0048f350)를 같은 144BYTE 장부에 쓴다. Session 교체로 장부를 초기화하지 않는다. (2026-11-12 WB04-E331)
    storeMetadata(owner, subscriber, core) {
        writePlayerCoreMetadata({ bytes: this.#records, origin: 0 }, owner, subscriber, core);
    }

    // 실제 두 owner/known/모드를 원문 helper 에 빌리는 살아 있는 globals 이다: templateOwner/battleOwner/playerKnown 은 `player` 의 현재 속성, active/current/players 는 이 소유자의 슬롯,
    // cached 는 고정 10BYTE, metamap 은 읽을 때마다 현재 값이다. 숨은 reset 이나 사본 동기화를 하지 않는다. (2026-11-12 WB04-E331)
    borrow(player, metamap) {
        const owner = this;
        const readMetamap = metamapReader(metamap);
        return {
            get templateOwner() { return player.templateOwner; }, set templateOwner(value) { player.templateOwner = value; },
            get battleOwner() { return player.battleOwner; }, set battleOwner(value) { player.battleOwner = value; },
            get playerKnown() { return player.playerKnown; }, set playerKnown(value) { player.playerKnown = value; },
            get active() { return owner.#active; }, set active(value) { owner.#active = value >>> 0; },
            get metamap() { return readMetamap(); },
            get current() { return owner.#current; }, set current(view) { owner.#current = view; },
            cached: owner.#cached,
            get players() { return owner.#players; },
        };
    }

    // 0048f0d0: 진단과 callback 복귀 뒤 현재 포인터를 다시 읽는 실제 조회이다. (2026-11-12 WB04-E331)
    read(player, metamap, hooks) {
        return readPlayerCoreData(this.borrow(player, metamap), hooks);
    }

    // 0048f3b0: 호출자가 원문 위치에서 명시 요청할 때만 실제 선택/정리 본문을 실행한다. (2026-11-12 WB04-E331)
    select(player, metamap, owner, templateOwner) {
        selectPlayerCoreData(this.borrow(player, metamap), owner, templateOwner);
    }

    // 0048fb30: lastKnown10 이 아닌 같은 실제 9기록을 빌려 Player 에 복원한다. Player base/owner 는 원문 위치에서 다시 읽는다. state = { missionPost(), savedGame(), checking() }. (2026-11-12 WB04-E331)
    restore(player, metamap, state, owner, hooks) {
        restorePlayerCoreData(this.borrow(player, metamap), { bytes: this.#records, origin: 0 }, state, owner, hooks);
    }
}
