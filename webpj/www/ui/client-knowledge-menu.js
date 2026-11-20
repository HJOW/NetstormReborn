// 미션 중 F6 / 게임 메뉴 View ▸ "View Netstorm Knowledge" 로 여는 지식 창(원본 ShowTechnology 의 Storm·World 카드와 Storm/World 선택 메뉴)을 실제 Client 메뉴 작성기와 현재 Session 에 결합한다.
// 근거: cpppj `RawMenuControls::OpenKnowledge`/`KnowledgeOpen`/`RequestClosePopups`(client/RawMenuControls.cpp)와 `UberGump` 의 ShowTechnology/ShowTecSideMenu/ShowTecMenu2 분배.
// 사용: const knowledge = new ClientKnowledgeMenu(session, writer, { translate, mark, width, height, language, storeText });
//   knowledge.open(KnowledgeMenuKind.Storm) → 카드를 만들었으면 true. Player 가 알려지지 않았거나 메타맵이면 false.
//   knowledge.isOpen() → 지식 카드 중 하나라도 살아 있고 보이는 팝업이면 true(ESC 가 팝업을 닫을지 판단하는 데 쓴다).
//   knowledge.close() → 열린 카드를 닫는다(공통 팝업 닫기 요청).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-20 WB04-E1088: F6 지식 창 연결 단위로 추가)
import { KnowledgeMenuKind, createKnowledgeMenuState } from '../core/raw-knowledge-menu.js';

// 지식 카드(팝업) 노드의 표시·삭제 대기 flags 이다. 표시 bit 가 있고 삭제 대기 bit 가 없으며 루트 직속이면 열린 창으로 센다.
const VISIBLE = 0x1, DELETING = 0x10000;

// 현재 Session 의 기술·마나·생산·제외 목록으로 지식 카드를 만들고 같은 실제 루트의 팝업으로 부착한다.
export class ClientKnowledgeMenu {
    // 현재 Session, 메뉴 작성 장치, 번역·화면·언어·정적 문자열 장치와 마지막으로 만든 카드 토큰들이다.
    #session; #writer; #devices; #tokens = [];
    // session 은 `showKnowledge`/`knowledgeAvailable`/`core` 를, writer 는 `knowledgeWriter(storeText)`/`closePopups`/`root`/`nodeOf` 를 제공해야 한다. 장치가 빠지면 만들지 않는다.
    constructor(session, writer, devices) {
        if (!session?.showKnowledge || !session?.knowledgeAvailable || !session?.core || !writer?.knowledgeWriter || !writer?.closePopups || !writer?.root || !writer?.nodeOf || !devices ||
            ['translate', 'mark', 'width', 'height', 'language', 'storeText'].some((name) => typeof devices[name] !== 'function')) throw new TypeError('지식 창의 실제 Session·작성 장치 누락');
        this.#session = session; this.#writer = writer; this.#devices = devices;
    }
    // 지식 카드를 연다. 원본처럼 Player 가 알려져 있고 메타맵이 아닐 때만 만들며 이전 팝업은 첫 카드를 만들기 직전에 정리한다. 만든 카드가 있으면 true.
    open(kind = KnowledgeMenuKind.Storm) {
        if (!this.#session.knowledgeAvailable()) return false;
        const types = this.#session.core().types, d = this.#devices;
        const state = createKnowledgeMenuState({ width: d.width(), height: d.height(), language: d.language() });
        const hooks = { ...this.#writer.knowledgeWriter(d.storeText), translate: (text) => d.translate(text), description: (type) => d.translate(types[type].description), mark: (command) => d.mark(command) };
        this.#tokens = this.#session.showKnowledge(state, hooks, kind); return this.#tokens.length > 0;
    }
    // 마지막으로 만든 카드 중 하나라도 표시 중이고 삭제 대기가 아닌 루트 직속 팝업이면 true 이다. 다른 팝업이 이미 닫은 카드는 세지 않는다.
    isOpen() {
        const root = this.#writer.root();
        // 살아 있는 카드가 하나라도 있으면 같은 창이 열린 것이다.
        for (const token of this.#tokens) { const node = this.#writer.nodeOf(token); if (node && (node.flags & VISIBLE) && !(node.flags & DELETING) && node.parent === root) return true; }
        return false;
    }
    // 마지막으로 만든 카드 토큰의 사본이다(시험·진단용).
    tokens() { return [...this.#tokens]; }
    // 열린 지식 카드를 공통 팝업 닫기 경로로 닫는다.
    close() { this.#writer.closePopups(); this.#tokens = []; }
}
