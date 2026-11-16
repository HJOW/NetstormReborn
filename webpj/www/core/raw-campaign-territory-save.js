// Session5853..5866의 현재 Runtime/지도/섬/공유 난수와 Territory snapshot 부모 결선이다(225553df, WB04-E632).
import { snapshotFortTerritory, findFortTerritoryIsland, findAnyFortTerritoryIsland, fortTerritoryIslandRecord, seedFortTerritoryRecord } from './raw-fort-territory-save.js';

// 같은 실제 Template9의 현재 manager DWORD만 읽는다.
function manager(owner, slot) { if (!Number.isInteger(slot) || slot < 0 || slot >= 9) throw new RangeError('실제 Territory Template9 범위'); const headers = owner.templateStorage.headers(); return new DataView(headers.buffer, headers.byteOffset + slot * 20, 4).getUint32(0, true); }
// 새 Territory 생성 뒤 현재 manager/current를 다시 읽어 같은 생성 수명의 stream을 저장한다.
export function writeCampaignFortTerritory(owner, slot) {
    if (!Number.isInteger(slot) || slot < 0 || slot >= 9) throw new RangeError('실제 Territory snapshot의 슬롯 계약 위반');
    const streams = [];
    // 각 소비 위치의 manager만 검사하며 Create 반환값을 current 대신 보관하지 않는다.
    const currentManager = (selected) => { if (selected !== manager(owner, slot)) throw new Error('Territory 현재 Template manager 불일치'); };
    const hooks = {
        load: (selected, records) => { if (!owner.runtime) throw new Error('Territory 저장의 실제 Runtime 미설치'); owner.runtime.readTerritory(selected, records); },
        special: () => owner.globals.editor, noRegion: () => owner.noRegion,
        island: (player, region) => findFortTerritoryIsland(owner.islands.bytes, owner.core.ownerMode.fort, player, region), anyIsland: (region) => findAnyFortTerritoryIsland(owner.islands.bytes, region),
        record: (island) => fortTerritoryIslandRecord(owner.chunks, island), seed: (record, index) => seedFortTerritoryRecord(record, index, (limit) => owner.random.next(limit)),
        manager: (selected) => manager(owner, selected), create: (selected) => { currentManager(selected); owner.templateStorage.createSection(slot, 'Territory'); },
        current: (selected) => { currentManager(selected); streams.push(owner.templateStorage.currentWriteStream(slot)); return streams.length; },
        access: (token, operation) => { if (!Number.isInteger(token) || token < 1 || token > streams.length) throw new RangeError('Territory 현재 stream 참조 범위'); owner.templateStorage.accessSection(streams[token - 1], operation); },
        assertion: (reason) => { throw new Error('Territory 원문 진단 장치 미연결:' + reason); },
    };
    snapshotFortTerritory(slot, hooks);
}
