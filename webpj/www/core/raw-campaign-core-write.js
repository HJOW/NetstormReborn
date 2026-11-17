// Session4447..4482·4488..4509의 현재 Core 갱신과 같은 실제 CoreData stream 결선이다(225553df, WB04-E635).
// 사용: refreshCampaignPlayerCore(owner), writeCampaignFortCoreData(owner, slot). 전체 snapshot/Load는 상위 작업이다.
import { refreshPlayerCore } from './raw-player-core-refresh.js';
import { RawContainedFinder } from './raw-contained-finder.js';
import { writeFortCoreData } from './raw-fort-core-write.js';
import { writeFortSp } from './raw-fort-sp-write.js';
// 같은50BYTE Pool의 상대 주소만 나타내는 원문 C++ opaque32 기준이다.
const POOL_BASE = 0x12000000;
// 도달한 실제 원문 진단을 빈 성공으로 넘기지 않는다.
function missing(label) { return (reason) => { throw new Error(`${label}의 실제 진단 장치 미연결:${reason}`); }; }
// 같은 실제9Template 헤더의 현재 manager DWORD만 읽는다.
function manager(owner, slot) { if (!Number.isInteger(slot) || slot < 0 || slot >= 9) throw new RangeError('현재 Core/snapshot Template9 범위'); const headers = owner.templateStorage.headers(); return new DataView(headers.buffer, headers.byteOffset + slot * 20, 4).getUint32(0, true); }
// bytes() 사본 대신 같은 실제 첫 물리 슬롯의 할당을 빌린다. Pool reset 뒤에는 호출 위치에서 다시 조회한다.
function currentPoolBytes(owner) { const pool = owner.core.pool; if (!pool.capacity) return new Uint8Array(0); const first = pool.physical(0); return new Uint8Array(first.buffer, first.byteOffset, pool.capacity * 50); }
// 실제 현재 Core 포인터·기술 finder와 현재 다이스 가상80으로 Core를 갱신한다.
export function refreshCampaignPlayerCore(owner) {
    refreshPlayerCore({
        core: () => owner.coreDataGlobals.read(owner.playerState, Number(Boolean(owner.core.ownerMode.challenge)), { assertion: missing('현재 Core 조회') }),
        technologyParent: () => owner.player.currentTechnology(),
        begin: (parent) => { if (!Number.isInteger(parent) || parent < 0 || parent > 65535 || parent >= owner.core.pool.capacity) throw new RangeError('현재 Core 갱신의 실제 기술 부모SID 범위'); const finder = new RawContainedFinder(owner.core.pool, owner.contentFinder); let current = finder.begin(parent); return { current: () => current, next: () => { current = finder.next(); } }; },
        pool: () => ({ bytes: currentPoolBytes(owner), address: POOL_BASE, capacity: owner.core.pool.capacity }),
        technologyBit: (type) => { if (!Number.isInteger(type) || type < 0 || type >= owner.core.types.length) throw new RangeError('현재 Core 갱신의 실제 TYPE 기술bit 물리 범위'); return owner.core.types[type].techBitIndex; },
        playerType: () => owner.state.runeState.readDaisType(), owner: () => owner.playerState.battleOwner,
        level: (form) => { const bytes = currentPoolBytes(owner), number = (form.address - POOL_BASE) / 50; if (!Number.isInteger(number) || number < 0 || number >= owner.core.pool.capacity || number > 65535 || form.bytes.length !== 50 || form.bytes.buffer !== bytes.buffer || form.bytes.byteOffset !== bytes.byteOffset + number * 50) throw new RangeError('현재 Core 가상 객체의 같은 실제 Pool 물리 범위'); return owner.rewardCount(number) | 0; },
        assertion: missing('현재 Core 갱신'),
    });
}
// 같은 현재 Player/Client seed와 실제 Create 반환 stream에 CoreData 자식을 연결한다.
export function writeCampaignFortCoreData(owner, slot) {
    if (!Number.isInteger(slot) || slot < 0 || slot >= 9) throw new RangeError('현재 CoreWrite의 실제9슬롯 범위');
    let stream; // 한 번 Create한 실제 노드/생성 수명만 부모 끝까지 보관한다.
    const globals = { metamap: () => Number(Boolean(owner.core.ownerMode.challenge)), missionPost: () => owner.globals.missionPost, editor: () => owner.globals.editor, players: () => owner.players, playerOwner: () => owner.playerState.battleOwner };
    if (owner.services.randomGlobals) { const random = owner.services.randomGlobals; globals.seed = () => ({ get value() { return random.fortCoreSeed; }, set value(value) { random.fortCoreSeed = value >>> 0; } }); }
    // 임시 seed/SP/현재 Core를 만들지 않고 선택된 실제 장치만 빌린다.
    const resolve = (token) => { if (token !== 1 || !stream) throw new RangeError('현재 CoreWrite의 고정 stream 참조'); return stream; };
    writeFortCoreData(slot, globals, {
        manager: (selected) => manager(owner, selected), assertion: missing('현재 CoreWrite Template.cpp'),
        create: (selected, name) => { if (selected !== manager(owner, slot)) throw new Error('현재 CoreWrite manager 불일치'); stream = owner.templateStorage.createSection(slot, name); return stream.manager && stream.section ? 1 : 0; },
        write: (token, bytes) => owner.templateStorage.writeSection(resolve(token), bytes), refresh: () => refreshCampaignPlayerCore(owner),
        prepareRandom: () => { owner.seed.reseed(); }, nextRandom: (limit) => owner.random.next(limit),
        sp: (token, value) => { const selected = resolve(token); if (!selected.manager || !selected.section) throw new RangeError('현재 SP writer의 고정 stream 참조'); owner.templateStorage.accessSection(selected, (fields, memory) => writeFortSp(fields, value, { nextRandom: (limit) => owner.random.next(limit), stream: memory })); },
    });
}
