// Session5869..5896의 실제 Template/지역 청크와 전체 Objects 부모 결선이다(225553df, WB04-E631).
import { saveFortObjects } from './raw-fort-objects-save.js';
import { saveFortTerritoryChunks, saveFortChaffChunks } from './raw-fort-region-save.js';

// 실제 Template9의 현재 manager DWORD다. 호출마다 같은 헤더를 읽는다.
function manager(owner, slot) { if (!Number.isInteger(slot) || slot < 0 || slot >= 9) throw new RangeError('실제 Objects Template9 범위'); const headers = owner.templateStorage.headers(); return new DataView(headers.buffer, headers.byteOffset + slot * 20, 4).getUint32(0, true); }
// 같은 공간/섬 판정과 현재 크기·청크 본문을 쓰는 실제 지역 조회 장치다. resolve는 명시 stream 참조를 반환한다.
export function makeCampaignFortRegionHooks(owner, resolve) {
    return {
        chunksX: () => owner.objectState.chunksX, chunksY: () => owner.objectState.chunksY, chunkRegion: (x, y) => owner.chunks.at(x, y).island,
        allowed: (island, player, region) => Number(owner.islands.matches(island, player | 0, region | 0)),
        chunk: (token, x, y) => owner.writeFortObjectChunkBody(resolve(token), x, y),
    };
}
// 모든 Parent Create/Find/Select는 반환 당시 manager/노드/생성 수명을 별도 registry에 보관한다.
export function writeCampaignFortObjects(owner, slot) {
    if (!Number.isInteger(slot) || slot < 0 || slot >= 9) throw new RangeError('실제 Objects snapshot의 슬롯 계약 위반');
    const streams = [];
    // 원문 빈 stream 참조만0으로 반환하고 실제 노드의 논리 길이와 혼동하지 않는다.
    const remember = (stream) => { if (!stream.manager || !stream.section) return 0; streams.push(stream); return streams.length; };
    // current가 바뀌어도 같은 생성 수명만 반환하며 범위 밖 토큰을 다른 노드로 대체하지 않는다.
    const resolve = (token) => { if (!Number.isInteger(token) || token < 1 || token > streams.length) throw new RangeError('Objects 현재 stream 참조 범위'); return streams[token - 1]; };
    // 각 DataManager 호출의 현재 manager가 선택한 같은 Template에 속하는지 확인한다.
    const currentManager = (selected) => { if (selected !== manager(owner, slot)) throw new Error('Objects 현재 Template manager 불일치'); };
    const regions = makeCampaignFortRegionHooks(owner, resolve);
    // 두 참조 모두 같은 objectState의 장기 칸이다. 순회/예외 뒤 scope와 version을 복구하지 않는다.
    const scope = { get value() { return owner.objectState.scope; }, set value(value) { owner.objectState.scope = value; } };
    const globals = { get templateOwner() { return owner.playerState.templateOwner; }, get version() { return owner.objectState.version; }, set version(value) { owner.objectState.version = value; }, fort: () => Number(owner.core.ownerMode.fort) };
    const hooks = {
        manager: (selected) => manager(owner, selected), assertion: (reason) => { throw new Error('Objects 원문 진단 장치 미연결:' + reason); },
        find: (selected, name) => { currentManager(selected); return remember(owner.templateStorage.findSection(slot, name)); },
        create: (selected, name) => { currentManager(selected); return remember(owner.templateStorage.createSection(slot, name)); },
        select: (selected, name, writable) => { currentManager(selected); return remember(owner.templateStorage.selectSection(slot, name, writable)); },
        rename: (selected, destination, source) => { currentManager(selected); owner.templateStorage.renameSection(slot, destination, source); },
        access: (token, operation) => owner.templateStorage.accessSection(resolve(token), operation),
        territory: (token, player, region) => saveFortTerritoryChunks(token, player, region, scope, regions), chaff: (token) => saveFortChaffChunks(token, scope, regions),
    };
    saveFortObjects(slot, globals, hooks);
}
