// Client 수명의 Player 9기록(1620BYTE)·기록별 공장 배열(120BYTE)의 실제 할당과 세션별 고정뷰 대여를 한 소유자에 보관하는 모듈이다
// (cpppj `client/ClientPlayerRecords.cpp/.h`, 10.78 전용, 기준선 1f00a25a 뒤 +7줄: `ReuseOrBindFactory` 와 공장 목록 연결의 소유자/행 정체성).
// 원문 부모(`initializePlayerRecords`, E7)가 쓰는 메모리 장치만 제공한다: 발급하는 32비트 주소 토큰(0x30000000 부터 0x1000 씩, 겹치지 않음)은 실제 보관 블록의 식별자이며 호스트 포인터가 아니다.
//   블록은 DWORD 정렬 저장소로 Player 405칸 또는 공장 30칸이고, 현재 생존 블록만 정확한 폭으로 해석한다(해제된 주소·잘못된 해제 종류·범위 밖 폭은 거부). 동맹/Chat 계산은 기존 실제 자식에 맡긴다.
// 사용: const records = new ClientPlayerRecords(allocationBytes?); const use = records.acquire(); use.initialize((owner, mask) => …); use.bindFactory(list, owner); … use.release();
//   records.initialize(alliance) 는 고정뷰 소비자가 없을 때 원문 공통 시작/State 부모의 실제 초기화이다. records.bindCoreData(core) 는 같은 Player base 변경을 `ClientCoreDataGlobals.players` 슬롯에 잇는다.
// 대여(`Use`)는 한 번에 하나이고, 살아 있는 동안 재할당(initialize)·두 번째 대여를 거부한다. 해제는 명시 `release()` 이다(C++ 소멸자 대응). 반납은 Player/목록 내용을 초기화하지 않는다.
// 공장 목록 연결(`bindFactory`/`reuseOrBindFactory`)은 owner 번호만 캡처하고 Player 주소/포인터/용량/개수는 호출마다 현재 기록에서 읽는다(재할당 이전 행을 캡처하지 않음).
// 기준선 뒤 변경 판정: ② `reuseOrBindFactory`(Native 공유 목록이 같은 Client/행이면 재사용, `bindCurrent` 에 소유자/행 정체성 전달). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-12 WB04-E332: 4-D ClientPlayerRecords 이식)
import {
    INITIALIZED_FACTORY_BYTES, INITIALIZED_PLAYER_BYTES, INITIALIZED_PLAYER_COUNT, INITIALIZED_PLAYER_STRIDE, initializePlayerRecords,
} from './raw-player-initialization.js';
import { RawProductionDeckStorage } from './raw-production-deck.js';
import { DwordBox } from './client-subscriber-globals.js';
import { currentAllocationBytes } from './current-allocation-bytes.js';

// 발급 주소의 첫 값과 간격(블록끼리 겹치지 않게)이다. (2026-11-12 WB04-E332)
const FIRST_TOKEN = 0x30000000;
const TOKEN_STRIDE = 0x1000;

// Client 수명의 Player 기록 소유자이다. (2026-11-12 WB04-E332)
export class ClientPlayerRecords {
    // 실제 생존 할당만 token → { words: Uint32Array, bytes: Uint8Array, player } 로 유지한다(해제된 주소로의 조회는 실패).
    #blocks = new Map();
    // 다음 발급 주소, 0059534c 의 현재 Player base, 005951f8/005951fc 두 DWORD, 동맹 81칸이다(모두 최초 0).
    #next = FIRST_TOKEN;
    #base = 0;
    #fortCount = new DwordBox(0);
    #lastLoginPlayer = new DwordBox(0);
    #alliances = new Uint32Array(81);
    // new 가 초기화하지 않는 공장 120BYTE 의 외부 할당 입력(검사는 고정 입력, 실제 Client 는 현재 힙 입력)이다.
    #allocationBytes;
    // 기존 고정뷰 소비자가 살아 있는 동안 재할당을 막는 호스트 수명 관문이다.
    #inUse = false;
    // 전체 실제 초기화가 정상 복귀했는지, 같은 Player base 변경을 잇는 외부 장기 Core 소유자이다.
    #initialized = false;
    #core = null;
    // World 없는 HUD 와 현재 Session 이 같은 Player+68 블록을 빌리는 Deck 할당 소유자(초기화/대여/반납으로 재생성하지 않음)이다.
    #productionDeck = new RawProductionDeckStorage();
    // 현재 base 의 Player 뷰 캐시(같은 토큰이면 같은 객체)이다.
    #recordsView = null;
    #recordsViewBase = 0;

    // 할당은 실제 원문 초기화 호출 때 시작한다. allocationBytes 를 생략하면 현재 힙 입력 대응(`currentAllocationBytes`)을 쓴다. firstToken 은 주소 소진 경계를 검사하려는 시험용 인자이다(제품 기본 0x30000000). (2026-11-12 WB04-E332)
    constructor(allocationBytes = null, firstToken = FIRST_TOKEN) {
        this.#allocationBytes = allocationBytes ?? currentAllocationBytes;
        this.#next = firstToken >>> 0;
    }

    // 고정 Player 뷰의 모든 소비자보다 먼저 만들고 나중에 반납하는 단일 세션 대여를 발급한다. 이미 대여가 있으면 거부하며 조회만으로 할당/초기화하지 않는다. (2026-11-12 WB04-E332)
    acquire() {
        return new ClientPlayerRecordsUse(this, {
            claim: () => {
                if (this.#inUse) throw new Error('Player 고정뷰를 사용하는 이전 세션이 아직 살아 있음');
                this.#inUse = true;
            },
            release: () => { this.#inUse = false; },
            initialize: (alliance) => { this.#initializeBody(alliance); },
            isInitialized: () => this.#initialized,
            base: () => this.#base,
            bindFactory: (list, owner) => { this.#bindFactory(list, owner); },
            // 명시 Native 재사용 비교에 쓰는 소유자 정체성(이 소유자 자신)이다.
            source: this,
        });
    }

    // 고정뷰 소비자가 없을 때 원문 공통 시작/State 부모의 실제 초기화를 실행한다. (2026-11-12 WB04-E332)
    initialize(alliance) {
        if (this.#inUse) throw new Error('현재 Player 고정뷰 사용 중 Native 재할당');
        this.#initializeBody(alliance);
    }

    // 같은 Player base 변경을 실제 Core 의 빌린 Player 슬롯(`players`)에 연결한다. 현재 선택을 임의로 바꾸지 않고, 초기 base 0 도 같은 빈 물리뷰로 공개한다. (2026-11-12 WB04-E332)
    bindCoreData(core) {
        if (!core || this.#inUse || (this.#core && this.#core !== core)) throw new TypeError('Player와 실제 Core 소유자 연결 계약');
        this.#core = core;
        core.players = this.records();
    }

    // 현재 연결된 같은 장기 Core 소유자를 순수 조회한다. (2026-11-12 WB04-E332)
    boundCoreData() { return this.#core; }

    // 전체 실제 초기화가 정상 복귀했는지 관측한다. 부분 실패는 초기화 완료로 대여하지 않는다. (2026-11-12 WB04-E332)
    initialized() { return this.#initialized; }

    // 현재 base 가 가리키는 실제 1620BYTE 이다(base 0 이면 빈 뷰). 재할당을 넘어 보관하려면 대여가 필요하다. 같은 base 면 같은 뷰 객체를 돌려준다. (2026-11-12 WB04-E332)
    records() {
        if (!this.#base) return new Uint8Array(0);
        if (!this.#recordsView || this.#recordsViewBase !== this.#base) {
            this.#recordsView = this.#memory(this.#base, INITIALIZED_PLAYER_BYTES);
            this.#recordsViewBase = this.#base;
        }
        return this.#recordsView;
    }

    // 원시 0059534c 의 현재 32비트 주소 토큰이다. (2026-11-12 WB04-E332)
    base() { return this.#base; }

    // 초기화와 실제 MissionLogin 이 함께 쓰는 005951f8 상자이다. (2026-11-12 WB04-E332)
    fortCount() { return this.#fortCount; }

    // 같은 005951fc 의 현재 DWORD 상자이다. (2026-11-12 WB04-E332)
    lastLoginPlayer() { return this.#lastLoginPlayer; }

    // 실제 동맹 자식과 모든 세션 소비자가 공유하는 방향별 81칸 표이다(최초 0). (2026-11-12 WB04-E332)
    alliances() { return this.#alliances; }

    // World 없는 HUD 와 현재 Session 이 같은 Player+68 블록을 빌리는 Deck 저장소이다. (2026-11-12 WB04-E332)
    productionDeckMemory() { return this.#productionDeck; }

    // 공통/대여 경로가 같은 실제 Native 부모와 메모리 장치를 공유하는 초기화 본체이다. 현재 Core 가 이전 Player 를 가리킨 상태면 원문 앞선 Core 0 선택을 요구한다. (2026-11-12 WB04-E332)
    #initializeBody(alliance) {
        const core = this.#core;
        if (core && core.current !== null && this.#base) {
            const selected = core.current;
            const start = this.records();
            // 선택한 10BYTE 가 현재 Player 영역 안이면 해제 전에 Core 0 선택이 필요하다(현재 캐시의 사본을 만들거나 포인터를 조용히 지우지 않음).
            if (selected.buffer === start.buffer && selected.byteOffset >= start.byteOffset && selected.byteOffset - start.byteOffset < INITIALIZED_PLAYER_BYTES) {
                throw new Error('이전 Player 해제 전 실제 Core0 선택 필요');
            }
        }
        this.#initialized = false;
        const self = this;
        initializePlayerRecords({
            get fortCount() { return self.#fortCount.value; }, set fortCount(value) { self.#fortCount.value = value; },
            get lastLoginPlayer() { return self.#lastLoginPlayer.value; }, set lastLoginPlayer(value) { self.#lastLoginPlayer.value = value; },
            playerBase: () => self.#base,
            writePlayerBase: (base) => { self.#writeBase(base); },
        }, {
            memory: (address, bytes) => self.#memory(address, bytes),
            freeFactory: (token) => { self.#free(token, false); },
            freePlayers: (token) => { self.#free(token, true); },
            mallocPlayers: (bytes) => self.#allocate(bytes, true),
            newFactory: (bytes) => self.#allocate(bytes, false),
            alliance,
        });
        this.#initialized = true;
    }

    // 실제 주소의 host 표현만 갱신한다. 같은 실제 할당의 Core 뷰 슬롯도 즉시 갱신하며 새 Core 선택/복사/known 쓰기는 하지 않는다. (2026-11-12 WB04-E332)
    #writeBase(base) {
        this.#base = base >>> 0;
        if (this.#core) this.#core.players = this.records();
    }

    // 원문이 요구한 크기(Player 1620 / 공장 120)로만 실제 블록을 확보하고 겹치지 않는 32비트 주소를 발급한다. 공장은 미초기화 입력을 외부 할당 입력으로 채운다. (2026-11-12 WB04-E332)
    #allocate(bytes, player) {
        if (bytes !== (player ? INITIALIZED_PLAYER_BYTES : INITIALIZED_FACTORY_BYTES)) throw new Error('Player 실제 할당 크기 오류');
        if (this.#next > 0xffffffff - TOKEN_STRIDE) throw new RangeError('Player 물리 주소 소진');
        const words = new Uint32Array(bytes / 4);
        const view = new Uint8Array(words.buffer);
        if (!player) this.#allocationBytes(view);
        const token = this.#next;
        this.#blocks.set(token, { words, bytes: view, player });
        this.#next += TOKEN_STRIDE;
        return token;
    }

    // free(NULL) 도 도달하지만 물리를 지우지 않는다. 그 외 주소는 정확한 종류의 생존 할당이어야 한다. (2026-11-12 WB04-E332)
    #free(token, player) {
        if (!token) return;
        const found = this.#blocks.get(token >>> 0);
        if (!found || found.player !== player) throw new Error('Player 실제 반납 주소/종류 오류');
        this.#blocks.delete(token >>> 0);
    }

    // DWORD 주소를 호스트 포인터로 변환하지 않고 발급한 실제 블록의 현재 범위만 빌린다(주소 이하의 가장 큰 블록 시작 기준). (2026-11-12 WB04-E332)
    #memory(address, bytes) {
        const target = address >>> 0;
        let start = -1;
        // 발급된 블록 시작 중 주소 이하의 가장 큰 값을 찾는다.
        for (const token of this.#blocks.keys()) if (token <= target && token > start) start = token;
        if (start < 0) throw new RangeError('Player 물리 주소 없음');
        const block = this.#blocks.get(start);
        const offset = target - start;
        const available = block.bytes.length;
        if (offset > available || bytes > available - offset) throw new RangeError('Player 물리 요청폭 범위 오류');
        return block.bytes.subarray(offset, offset + bytes);
    }

    // 현재 Player 행의 little endian DWORD 를 읽는다(현재 base 를 매번 조회). (2026-11-12 WB04-E332)
    #read(owner, offset) {
        if (owner >= INITIALIZED_PLAYER_COUNT || offset > INITIALIZED_PLAYER_STRIDE - 4) throw new RangeError('현재 Player DWORD 범위 오류');
        const bytes = this.#memory(this.#base + owner * INITIALIZED_PLAYER_STRIDE + offset, 4);
        return (bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24)) >>> 0;
    }

    // 같은 현재 행에 4BYTE 만 쓴다(먼저 읽기로 범위를 검증). (2026-11-12 WB04-E332)
    #write(owner, offset, value) {
        this.#read(owner, offset);
        const bytes = this.#memory(this.#base + owner * INITIALIZED_PLAYER_STRIDE + offset, 4);
        for (let byte = 0; byte < 4; byte += 1) bytes[byte] = (value >>> (byte * 8)) & 0xff;
    }

    // 현재 Player 의 포인터/용량으로 같은 실제 공장 배열(DWORD 뷰)을 해석한다. 용량 0 이면 빈 배열, 정렬이 안 맞으면 거부한다. (2026-11-12 WB04-E332)
    #factory(owner) {
        const capacity = this.#read(owner, 4);
        const token = this.#read(owner, 0);
        if (capacity > 0xffffffff / 4) throw new RangeError('Player factory 용량 넘침');
        if (!capacity) return new Uint32Array(0);
        const bytes = this.#memory(token, capacity * 4);
        if (bytes.byteOffset % 4) throw new Error('Player factory 정렬 오류');
        return new Uint32Array(bytes.buffer, bytes.byteOffset, capacity);
    }

    // 실제 Player+0/+4/+8 에 공장 목록을 연결한다. owner 번호만 캡처하고 나머지는 호출마다 현재 기록에서 읽는다. 소유자/행 정체성은 명시 Native 재사용 비교용이다. (2026-11-12 WB04-E332)
    #bindFactory(list, owner) {
        list.bindCurrent({
            storage: () => this.#factory(owner),
            capacity: () => this.#read(owner, 4),
            readCount: () => this.#read(owner, 8),
            writeCount: (value) => { this.#write(owner, 8, value); },
            sourceOwner: this,
            sourceSlot: owner,
        });
    }
}

// 단일 세션 대여이다. 만든 쪽이 마지막에 `release()` 로 반납한다(다음 세션의 재할당을 허용, 원문 정리는 암묵 실행하지 않음). (2026-11-12 WB04-E332)
class ClientPlayerRecordsUse {
    // 소유자의 내부 장치(대여 선점/반납/초기화 본체/상태 조회)이다.
    #device;
    // 실패한 부모를 같은 고정뷰 대여 중 재진입하지 않는 표시와 이 대여 안의 초기화 완료 표시이다.
    #attempted = false;
    #initialized = false;
    #released = false;

    // 소유자가 만드는 대여이다. 동시 세션이 기존 span 을 무효화하기 전에 거부한다. (2026-11-12 WB04-E332)
    constructor(owner, device) {
        this.#device = device;
        device.claim();
    }

    // 반납한다. 중복 반납은 무시한다. (2026-11-12 WB04-E332)
    release() {
        if (this.#released) return;
        this.#released = true;
        this.#device.release();
    }

    // 같은 대여의 첫 호출만 원문 236BYTE 초기화를 실행한다. 반복 호출은 거부한다. (2026-11-12 WB04-E332)
    initialize(alliance) {
        if (this.#attempted) throw new Error('살아 있는 Player 고정뷰 대여에서 초기화를 반복할 수 없음');
        this.#attempted = true;
        this.#device.initialize(alliance);
        this.#initialized = true;
    }

    // 앞선 실제 Native 부모가 초기화한 기록만 대여한다. 할당/동맹/필드 초기화를 다시 실행하지 않는다. (2026-11-12 WB04-E332)
    adoptCurrent() {
        if (this.#attempted || !this.#device.isInitialized() || !this.#device.base()) throw new Error('초기화 완료된 현재 Player 대여 계약');
        this.#attempted = true;
        this.#initialized = true;
    }

    // 실제 Player+0/+4/+8 에 목록을 연결한다. 초기화되지 않았거나 owner 가 범위 밖이면 거부한다. (2026-11-12 WB04-E332)
    bindFactory(list, owner) {
        if (!this.#initialized || owner >= INITIALIZED_PLAYER_COUNT) throw new Error('초기화되지 않은 Player factory 연결');
        this.#device.bindFactory(list, owner);
    }

    // Native 공유 목록이 같은 Client/행에 이미 연결돼 있으면 재사용하고 아니면 새로 연결한다(다른 Client/행/이미 채운 독립 목록의 기존 거부는 유지). 초기화/재할당/장부 기록은 생략하지 않는다. (2026-11-12 WB04-E332: 기준선 뒤 반영)
    reuseOrBindFactory(list, owner) {
        if (!this.#initialized || owner >= INITIALIZED_PLAYER_COUNT) throw new Error('초기화되지 않은 Native Player factory 재사용');
        if (list.matchesCurrentSource(this.#device.source, owner)) return;
        this.#device.bindFactory(list, owner);
    }
}
