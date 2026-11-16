// Session5825..5845의 같은 Template/생산 덱에 두 snapshot writer를 연결한다(225553df, WB04-E628).
// 외곽 설치 관문은 caller가 맡으며 이 공통 내부 부모는 실제 고정 stream과 현재 State를 구별한다.
import { snapshotFortTypeNames, snapshotFortDeck } from './raw-fort-snapshot-basics.js';

// 현재 선택된 장치의 누락만 원문 호출 위치에서 거부한다.
function need(device, name) { if (!device) throw new Error(`실제 snapshot ${name} 미설치`); return device; }
// 실제 Template9 헤더의 manager DWORD다. 선택된 슬롯을 clamp하지 않는다.
function manager(owner, slot) {
    if (!Number.isInteger(slot) || slot < 0 || slot >= 9) throw new RangeError('실제 snapshot Template9 범위');
    return new DataView(owner.templateStorage.headers().buffer, owner.templateStorage.headers().byteOffset + slot * 20, 4).getUint32(0, true);
}
// 현재 owner·실제 manager/생성 수명과 같은 생산 장치를 원문 두 writer에 빌린다. 새 Template/덱 사본을 만들지 않는다.
export function snapshotCampaignFortBasicSection(owner, slot, names) {
    if (!Number.isInteger(slot) || slot < 0 || slot >= 9) throw new RangeError('실제 Fort snapshot의 슬롯 계약 위반');
    const streams = [];
    // 일반 쓰기/인라인 접근은 Create가 반환한 같은 노드만 빌린다.
    const streamAt = (token) => { if (!Number.isInteger(token) || token < 1 || token > streams.length) throw new RangeError('실제 snapshot stream 참조 범위'); return streams[token - 1]; };
    const hooks = {
        manager: (selected) => manager(owner, selected),
        assertion: (reason) => { throw new Error('실제 snapshot 원문 진단 장치 미연결:' + reason); },
        create: (selected, section) => { if (selected !== manager(owner, slot)) throw new Error('실제 snapshot manager 선택 불일치'); streams.push(owner.templateStorage.createSection(slot, section)); return streams.length; },
        write: (token, bytes) => owner.templateStorage.writeSection(streamAt(token), bytes), access: (token, operation) => owner.templateStorage.accessSection(streamAt(token), operation),
        state: (selected) => need(owner.templateState, 'State').get(selected),
        typeName: (index) => { if (!Number.isInteger(index) || index < 0 || index >= owner.core.types.length) throw new RangeError('실제 snapshot TYPE 범위'); return owner.core.types[index].name; },
        refreshDeck: (selected) => need(owner.production, '생산').refresh(selected), getDeck: (selected) => need(owner.production, '생산').deck.get(selected), deck: (token) => need(owner.production, '생산').deck.bytes(token),
    };
    // C++의 typeCount 지역값만 고정하고 templateOwner는 const 참조처럼 현재 칸을 읽는다.
    const typeCount = owner.core.types.length; const globals = { get templateOwner() { return owner.playerState.templateOwner; }, typeCount };
    if (names) snapshotFortTypeNames(slot, globals, hooks); else snapshotFortDeck(slot, hooks);
}
