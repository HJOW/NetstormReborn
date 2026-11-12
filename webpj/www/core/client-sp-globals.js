// 원문 공통 시작의 SP 저장소와 사제 추적 offset(0054d4e0)을 메뉴·Session·사제보다 오래 같은 객체로 소유하는 모듈이다(cpppj `client/ClientSpGlobals.h/.cpp`, 10.78 전용, 기준선 1f00a25a 뒤 +3줄).
// 같은 Client 의 게임/CRT 난수(`ClientRandomGlobals`)와 실제 분산 SP 저장소(`ScrambledSpStore`)를 한 번 조립한다. 생성자·접근자·Session 반납은 **숨은 재시드나 초기화를 하지 않는다**:
// 실제 초기화(재시드 → offset → 순열 → 32배열 확보)는 원문 호출자(공통 시작)가 `store().initialize()` 로 명시 실행한다.
//  - 조립: random(ClientRandomGlobals, 필수) · unixSeconds(): 현재 유닉스 초 공급 함수(`ClientTimeSource.unixSeconds`) · allocation(선택): 36BYTE 할당 입력 함수(기본 `currentAllocationBytes`, 웹은 새 할당의 0).
//    `GameRandomSeed(random, unixSeconds)` 가 재시드를 맡고(004559d0, 모드를 잠시 1 로 바꿔 시각 시드), `ScrambledSpStore` 는 그 `reseed()` 와 같은 게임 난수 객체를 빌리며 추적 offset 소유자(`tracking()`, 0054d4e0 의 최초 BSS 0 `Int32Box`)를 즉시 연결한다.
//  - store(): 32×9 DWORD·offset·순열·초기화 상태의 유일한 저장소. randomGlobals(): Session 이 다른 난수와 SP 를 섞지 않도록 실제 소유자 정체성을 돌려준다(난수 상태 복사 없음). tracking(): 같은 추적 offset 상자(저장소의 `trackingOffset` 이 읽는 칸).
// 이 모듈은 DOM 을 쓰지 않는다. 앱 수명 연결(Client 가 한 번 만듦)은 4-D 조립 몫이다. (2026-11-12 WB04-E336: 4-D ClientSpGlobals 이식으로 추가)
import { Int32Box } from './client-ai-debug-globals.js';
import { currentAllocationBytes } from './current-allocation-bytes.js';
import { ClientRandomGlobals, GameRandomSeed } from './random.js';
import { ScrambledSpStore } from './sp-store.js';

// Client 수명 SP 전역 소유자이다. 사용: const sp = new ClientSpGlobals(random, () => time.unixSeconds()); sp.store().initialize(); sp.store().set(1, bits). 복사하지 않고 한 객체를 모든 소비자가 빌린다. (2026-11-12 WB04-E336)
export class ClientSpGlobals {
    // seed/SP 보다 먼저 만들고 뒤에 반납하는 같은 장기 게임·CRT 상태이다.
    #random;
    // 0054d4e0 의 최초 BSS 0 사제 추적 offset 이며 Session 생성/소멸에서 초기화하지 않는다.
    #tracking = new Int32Box(0);
    // 실제 같은 시각/CRT/게임 난수/모드에 재시드하는 시드 초기화기와 분산 SP 저장소이다.
    #seed;
    #store;

    // 주소만 연결한다(원문 004391ed 에서 명시 Initialize 하기 전까지 시각·난수·할당을 쓰지 않는다). random 이 없으면 거부한다. (2026-11-12 WB04-E336)
    constructor(random, unixSeconds, allocation = null) {
        if (!(random instanceof ClientRandomGlobals)) throw new TypeError('SP 실제 Client 난수 소유자 누락');
        this.#random = random;
        this.#seed = new GameRandomSeed(random, unixSeconds);
        this.#store = new ScrambledSpStore(random.game, () => this.#seed.reseed(), allocation ?? currentAllocationBytes);
        this.#store.bindTrackingOffset(this.#tracking);
    }

    // 같은 32×9 DWORD/offset/순열/초기화 상태의 유일한 저장소이다(읽기 조회에서도 호출자가 부르기 전에는 초기화하지 않는다). (2026-11-12 WB04-E336)
    store() {
        return this.#store;
    }

    // Session 이 다른 난수와 SP 를 섞지 않도록 실제 난수 소유자 정체성을 읽는다. (2026-11-12 WB04-E336)
    randomGlobals() {
        return this.#random;
    }

    // 0054d4e0 사제 추적 offset 상자이다(`store().trackingOffset` 이 읽는 같은 칸). (2026-11-12 WB04-E336)
    tracking() {
        return this.#tracking;
    }
}
