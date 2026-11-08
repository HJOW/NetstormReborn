// 간헐천 등록/제거가 Player 1..8 의 AI 본체에 알리는 순회(00416720)를 옮긴 모듈이다(cpppj `o/RawGeyserAiRegistration.cpp/.h`, 10.78 전용: Player 한 칸 0xb4, AI 포인터 +0xb0).
// 최초 로드처럼 AI 가 없는 Player 는 실제 순회만 하고 자연 반환하며, 비영 AI 포인터를 만나면 실제 하위(004153c0, 갱신)를 불러 현재 본체를 갱신한다. 하위가 연결되지 않았다면 빈 콜백으로 성공 처리하지 않고
// 첫 실제 하위 요청의 인자(Player 번호·원본 AI 포인터·간헐천 SID·등록/제거 DWORD)를 담은 `GeyserAiRegistrationBoundary` 로 거부한다.
//  - register(sid, state): Player 0 은 순회 대상이 아니다. 1..8 의 현재 AI 포인터 DWORD 를 번호순으로 매 호출 다시 읽는다(정렬되지 않아도 리틀엔디언 그대로). 각 하위 뒤 다음 Player 의 현재 포인터를 다시 읽는다. state 0 은 제거, 비영은 등록 하위 요청이다.
//  - bindPatch(update): 실제 004153c0 (ai, sid, state) 하위를 연결한다. 비함수는 거부한다.
// 생성자가 받는 Player 저장소(Uint8Array)는 Money/Technology 등과 공유하는 실제 9기록이며 복사하지 않는다(호출 동안 유효해야 함). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E78: 간헐천 AI 등록 이식으로 추가)

// 10.78 Player 한 기록의 폭, AI 포인터 위치, 기록 수이다. (2026-11-08 WB04-E78)
const PLAYER_STRIDE = 0xb4;
const AI_OFFSET = 0xb0;
const PLAYER_COUNT = 9;

// 실제 순회에서 만난 첫 미연결 하위 요청의 네 DWORD 를 그대로 유지하고 완료하지 않은 AI 몸체를 알리는 오류이다. (2026-11-08 WB04-E78)
export class GeyserAiRegistrationBoundary extends Error {
    // player = 실제 순회에서 발견한 Player 1..8 번호, aiPointer = 현재 기록 +0xb0 의 32비트 AI 포인터, sid = 원본 첫 인자, state = 원본 둘째 인자(0 은 제거)이다.
    constructor(player, pointer, geyser, operation) {
        super('간헐천 등록의 nonnull AI 하위 몸체는 아직 복원되지 않았습니다');
        this.name = 'GeyserAiRegistrationBoundary';
        this.owner = player;
        this.aiPointer = pointer;
        this.sid = geyser;
        this.state = operation;
    }
}

export class RawGeyserAiRegistration {
    // 공유 Player 9기록 저장소(복사하지 않음)와 PATCH 에서만 연결하는 실제 하위 갱신이다.
    #players;
    #update = null;

    // 9기록이 모두 있는 저장소를 받는다. 부족하면 거부한다. (2026-11-08 WB04-E78)
    constructor(playerRecords) {
        if (!(playerRecords instanceof Uint8Array) || playerRecords.length < PLAYER_COUNT * PLAYER_STRIDE) throw new RangeError('간헐천 등록에는 실제 Player 9기록이 필요합니다');
        this.#players = playerRecords;
    }

    // 실제 004153c0 을 연결한다. 빈 성공을 허용하지 않는다. (2026-11-08 WB04-E78)
    bindPatch(update) {
        if (typeof update !== 'function') throw new TypeError('간헐천 AI의 실제 PATCH 하위 누락');
        this.#update = update;
    }

    // 현재 AI 포인터를 번호순으로 읽고 연결된 하위를 호출한다. 미연결 nonnull 이면 첫 요청을 명시 거부한다. (2026-11-08 WB04-E78)
    register(sid, state) {
        const players = this.#players;
        // Player0 은 원본 순회 대상이 아니다. 1..8 의 현재 포인터를 매 호출 다시 읽는다.
        for (let owner = 1; owner < PLAYER_COUNT; owner += 1) {
            const field = owner * PLAYER_STRIDE + AI_OFFSET;
            // 정렬되지 않아도 원본 little endian DWORD 를 보존한다.
            const ai = (players[field] | (players[field + 1] << 8) | (players[field + 2] << 16) | (players[field + 3] << 24)) >>> 0;
            if (ai) {
                if (!this.#update) throw new GeyserAiRegistrationBoundary(owner, ai, sid >>> 0, state >>> 0);
                this.#update(ai, sid >>> 0, state >>> 0);
            }
        }
    }
}
