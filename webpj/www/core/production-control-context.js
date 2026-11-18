// 생산 UI가 Client 장기 물리·전역·Player와 실제 옵션을 사본 없이 공유하는 문맥이다.
// 사용: new ProductionControlContext(storage, gameplay, players, options, sp, ownerColorTables).
export class ProductionControlContext {
    // World 수명과 독립인 실제 표면 저장소다.
    storage;
    // 현재 Player·커서·검색 전역의 실제 소유자다.
    gameplay;
    // 현재 Player 기록과 Deck을 소유하는 실제 객체다.
    players;
    // 같은 87BYTE 전투 옵션 배열이다.
    options;
    // 선택적인 Client 장기 분산 SP 소유자다.
    sp;
    // 선택적인 현재 소유자 색표 소유자다.
    ownerColorTables;
    // 전달받은 실제 참조를 그대로 보관한다. 선택 항목이 없으면 null이다.
    constructor(storage, gameplay, players, options, sp = null, ownerColorTables = null) {
        this.storage = storage; this.gameplay = gameplay; this.players = players; this.options = options; this.sp = sp; this.ownerColorTables = ownerColorTables;
    }
}
