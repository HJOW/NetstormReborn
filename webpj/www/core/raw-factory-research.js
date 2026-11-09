// 원본 Factory 의 지식 선택·서버 연구 수신·생산 내용물 통지를 옮긴 모듈이다(cpppj `o/RawFactoryResearch.cpp/.h`, 10.78 전용).
//  - writeFactoryResearchCommand(40aba0 생성자 + 4547d0 필드 기록): 패킷 0x58 의 16 BYTE(프로토콜 1, 선언 길이 16, 부모 SID WORD, kind 최종 BYTE, mode 0, 현재 시각 double)를 만든다.
//  - learn(453cc0): 예측 내용물(client SID 범위)을 삭제한 뒤 슬롯을 다시 조회하고, 같은 종류가 다시 검색되지 않으면 실제 생성한다.
//  - input(4547d0 의 03000080): packedArgument 의 상위 signed WORD 가 선택한 종류이며 0 이면 도움말(researchHelp), 슬롯이 남아 있으면 현재 시각을 읽어 패킷을 만들고 learn 뒤 채널 9 로 송신한다.
//  - receive(453d70): 서버 연구 수신(부모 free/dead 면 무시, kind 0 + mode 1 은 업그레이드 수신, 현재 TYPE 의 flags2 0x4000·sender==owner·비 아웃포스트일 때만 권한 확인 뒤 learn).
//  - notice(453b50 → 475220): 플레어·중복 예측 삭제·현재 dirty/Deck 갱신 순서를 보존한다. 효과가 값을 바꿀 수 있으므로 kind 와 후속 flags 는 효과 뒤에 읽는다.
// RegisterFactoryResearchCommand(패킷 0x58 분배기 등록)는 RawPacketDispatch(미이식) 이식 뒤로 미룬다. 실제 하위(검색·삭제·생성·송신·도움말·플레어·재계산·진단·업그레이드 수신·입력 로그)는 선택한 위치에서만 요구한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E217: Factory 연구 이식으로 추가)

// 원본 58h 의 실제 선언/송신 길이, 메뉴 연구 명령의 수정키 제외 값, 두 실제 Factory VT 이다. (2026-11-10 WB04-E217)
export const FACTORY_RESEARCH_PACKET_BYTES = 16;
const RESEARCH_COMMAND = 0x03000080;
const FACTORY_VTABLE = 0x00509148;
const OUTPOST_VTABLE = 0x00509210;
const TYPE_COUNT = 188;

// 선언 길이/SID 와 double 시각을 호스트 정렬과 무관하게 기록한다. 지정한 바이트만 덮어써 뒤의 버퍼는 보존한다. (2026-11-10 WB04-E217)
function putBytes(bytes, offset, value, width) {
    let rest = BigInt.asUintN(64, BigInt(value));
    for (let index = 0; index < width; index += 1) {
        bytes[offset + index] = Number(rest & 0xffn);
        rest >>= 8n;
    }
}

// 원본이 쓰는 필드 폭만 읽고 다음 필드는 섞지 않는다(낮은 BYTE 부터 조립). (2026-11-10 WB04-E217)
function getBytes(bytes, offset, width) {
    let value = 0;
    for (let index = 0; index < width; index += 1) value += bytes[offset + index] * 2 ** (8 * index);
    return value;
}

// 선택된 효과가 연결되지 않았으면 실제 호출 위치에서 거부한다. (2026-11-10 WB04-E217)
function need(effect) {
    if (typeof effect !== 'function') throw new Error('Factory 연구 실제 하위 미연결');
    return effect;
}

// 원본 41bce0 의 client SID 판정이며 풀의 예측 영역과 구분한다. (2026-11-10 WB04-E217)
const isClient = (sid) => sid > 4 && sid < 15000;

// 시각 double 의 64비트 패턴을 얻는 변환 도구이다. (2026-11-10 WB04-E217)
const doubleView = new DataView(new ArrayBuffer(8));

// 입력의 종류는 최종 BYTE 만 쓰며 시각의 8 BYTE 는 현재 double 비트 그대로이다. packet 은 16 BYTE 이상이어야 하고 뒤의 호스트 버퍼는 건드리지 않는다. 쓴 길이(16)를 돌려준다. (2026-11-10 WB04-E217)
export function writeFactoryResearchCommand(parent, kind, now, packet) {
    if (packet.length < FACTORY_RESEARCH_PACKET_BYTES) throw new RangeError('Factory 연구 16바이트 저장소 부족');
    packet[0] = 1;
    packet[1] = 0x58;
    putBytes(packet, 2, FACTORY_RESEARCH_PACKET_BYTES, 2);
    putBytes(packet, 4, parent, 2);
    packet[6] = kind & 0xff;
    packet[7] = 0;
    doubleView.setFloat64(0, now, true);
    putBytes(packet, 8, doubleView.getBigUint64(0, true), 8);
    return FACTORY_RESEARCH_PACKET_BYTES;
}

// Factory 연구 처리기이다. pool 은 10.78 SidPool, types 는 188칸 TYPE 표(flags2), state = { loadingDepth(), productionDirty: { value }, boss(), now(), outpostType() } 현재 전역(효과 뒤에도 최신 값을 다시 읽음),
// hooks = { find(parent, mask, kind, index) → sid, slots(parent) → 남은 슬롯, destroy(sid, flags), create(parent, kind, quantity, flags, attach, quality) → sid, send(channel, packet), help(text, flags),
//   flare(parent, count, event), refresh(owner), assertion(text), upgradeReceive(packet), inputLog(parent, slots) }. 선택한 하위만 요구한다.
// 사용: const research = new RawFactoryResearch(pool, types, state, hooks); research.input(parent, 0x03000080, (kind << 16) | parent). (2026-11-10 WB04-E217)
export class RawFactoryResearch {
    // 모든 현재 부모·내용물의 동일 저장소, 현재 genus 를 읽는 PATCH 188 타입, 현재 전역, 실제 계산 하위이다.
    #pool;
    #types;
    #state;
    #hooks;

    // 판본/타입 표를 확인한다. 생성 때는 실제 효과를 실행하지 않고 콜백 누락은 해당 분기에서 검사한다. (2026-11-10 WB04-E217)
    constructor(pool, types, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== TYPE_COUNT) throw new TypeError('Factory 연구 PATCH 자료 오류');
        this.#pool = pool;
        this.#types = types;
        this.#state = state;
        this.#hooks = hooks ?? {};
    }

    // 필요한 BYTE/WORD/DWORD 만 비정렬 little endian 으로 읽는다. form 0 도 물리 범위 안의 원문 필드를 읽는다. (2026-11-10 WB04-E217)
    #read(sid, offset, width = 4) { return getBytes(this.#pool.slot(sid), offset, width); }

    // 두 실제 Factory VT(Factory 0x509148·아웃포스트 wrapper 0x509210)인지 확인한다. 타입 번호만으로 미복원 파생을 허용하지 않는다. (2026-11-10 WB04-E217)
    handlesNotice(parent) {
        const vtable = this.#read(parent, 0);
        return vtable === FACTORY_VTABLE || vtable === OUTPOST_VTABLE;
    }

    // 453cc0: 예측 내용물 삭제 → 슬롯 재조회 → 동일 종류 재검색 → 실제 생성이다. 첫 검색 결과는 슬롯 재조회 뒤의 원본 진단에도 쓴다. (2026-11-10 WB04-E217)
    learn(parent, kind, attachFlags) {
        const hooks = this.#hooks;
        const found = need(hooks.find)(parent, 16, kind, 0);
        if (isClient(found)) need(hooks.destroy)(found, 0);
        const remaining = need(hooks.slots)(parent);
        if (found && remaining <= 0) need(hooks.assertion)('slotsRemaining > 0');
        if (remaining < 1) return;
        if (!need(hooks.find)(parent, 16, kind, 0)) need(hooks.create)(parent, kind, 0, 16, attachFlags, 0);
    }

    // 4547d0 의 03000080 만 처리한다. packedArgument 의 상위 signed WORD 가 선택한 종류이며, 패킷 시각은 learn 전에(로그 뒤 현재 값으로) 저장한다. 처리했음을 뜻하는 1 을 돌려준다. (2026-11-10 WB04-E217)
    input(parent, code, packedArgument) {
        if ((code & 0x03ffffff) !== RESEARCH_COMMAND) throw new Error('Factory 연구 외 입력 몸체 미복원');
        const hooks = this.#hooks;
        const kind = ((packedArgument >>> 16) << 16) >> 16;
        if (!kind) {
            need(hooks.help)('researchHelp', 1);
            return 1;
        }
        const slots = need(hooks.slots)(parent);
        if (hooks.inputLog) hooks.inputLog(parent, slots);
        if (slots > 0) {
            const packet = new Uint8Array(FACTORY_RESEARCH_PACKET_BYTES);
            writeFactoryResearchCommand(parent, kind >>> 0, this.#state.now(), packet);
            this.learn(parent, kind >>> 0, 16);
            need(hooks.send)(9, packet);
        }
        return 1;
    }

    // 453d70 의 연구/무동작 분기와 동일 원문의 업그레이드 수신 연결이다. owner BYTE 와 sender DWORD 를 비교하며 연구 시각/flags 는 생성 판단에 쓰지 않는다. 미공급 선택은 변경 전에 거부한다. (2026-11-10 WB04-E217)
    receive(sender, packet) {
        if (packet.length < 8) throw new RangeError('Factory 연구 수신 본문 부족');
        const hooks = this.#hooks;
        const parent = getBytes(packet, 4, 2);
        if (this.#read(parent, 11, 1) & 3) return;
        if (!packet[6]) {
            if (packet[7] === 1) need(hooks.upgradeReceive)(packet);
            return;
        }
        const type = this.#read(parent, 10, 1);
        if (type >= this.#types.length) throw new RangeError('Factory 연구 현재 TYPE 범위 오류');
        if (!(this.#types[type].flags2 & 0x4000) || this.#read(parent, 34, 1) !== (sender >>> 0) || this.#read(parent, 10, 1) === (this.#state.outpostType() >>> 0)) return;
        if (!this.#state.boss()) {
            need(hooks.assertion)('boss');
            if (!this.#state.boss()) return;
        }
        this.learn(parent, packet[6], 0);
    }

    // 453b50 → 475220: 플레어·중복 예측 삭제·현재 dirty/Deck 갱신 순서이다. 플레어가 내용을 변경할 수도 있으므로 kind 와 후속 flags 는 효과 뒤에 읽는다. (2026-11-10 WB04-E217)
    notice(parent, form, reason) {
        const hooks = this.#hooks;
        if (reason === 1 && form && (this.#read(form, 20, 1) & 16)) {
            need(hooks.flare)(parent, 6, 2);
            const found = need(hooks.find)(parent, 16, this.#read(form, 18, 2), 0);
            if (found && found !== form && isClient(found)) need(hooks.destroy)(found, 0);
        }
        this.#manaNotice(parent, form, reason);
    }

    // 475220: form 0 도 물리 0번 슬롯의 flags 를 읽는다. flags 는 한 번 읽은 원본 지역값이며 진단 뒤 owner 는 현재 값을 다시 읽는다. (2026-11-10 WB04-E217)
    #manaNotice(parent, form, reason) {
        const hooks = this.#hooks;
        const state = this.#state;
        const flags = this.#read(form, 20, 1);
        if (!state.loadingDepth() && (flags & 16)) state.productionDirty.value = (state.productionDirty.value + 1) >>> 0;
        if (reason === 1 && !state.loadingDepth() && (flags & 16)) {
            if (this.#read(parent, 40, 1) & 9) need(hooks.assertion)('!isAstral() && !isAbstract()');
            need(hooks.refresh)(this.#read(parent, 34, 1));
        }
    }

    // 동일 월드의 생성/삭제 연결을 검증하는 같은 풀이다. (2026-11-10 WB04-E217)
    get pool() { return this.#pool; }
}
