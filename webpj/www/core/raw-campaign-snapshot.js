// Session5903..5910·5920..5982의 전체 Fort/Battle snapshot 결선이다(225553df, WB04-E636).
// 사용: snapshotCampaignTemplate(owner, slot, battle). 같은 실제 Template/Player/난수/Client 장부를 빌린다.
// 전체 Load가 선행이며 Editor 예외 관문은 옮기지 않는다. 정상 미션 통합 검사는 4D-2h 뒤에 수행한다.
import { snapshotFortTemplate, snapshotBattleTemplate } from './raw-fort-snapshot-parent.js';
import { saveFortMoney } from './raw-fort-money-save.js';
import { saveFortMission } from './raw-fort-mission-save.js';
import { saveFortBadges, readFortBadgeCount } from './raw-fort-badges-save.js';

// 진단/기술/Money/Mission/Create가 요청할 때마다 같은9Template 헤더의 현재 manager를 읽는다.
function manager(owner, slot) {
    if (!Number.isInteger(slot) || slot < 0 || slot >= 9) throw new RangeError('전체 snapshot의 실제 Template9 범위');
    const headers = owner.templateStorage.headers();
    return new DataView(headers.buffer, headers.byteOffset + slot * 20, 4).getUint32(0, true);
}
// 실제 선택한 원문 진단을 빈 성공으로 넘기지 않는다. 앞선 child의 부분 쓰기는 보존한다.
function missing(label) { return (reason) => { throw new Error(label + '의 실제 진단 장치 미연결:' + reason); }; }

// 이미 설치된 Session의 실제 child들을 Fort/Battle 부모가 정한 순서로 같은 저장소에 연결한다.
export function snapshotCampaignTemplate(owner, slot, battle) {
    if (!owner.loaded || !Number.isInteger(slot) || slot < 0 || slot >= 9) throw new Error('실제 전체 Fort snapshot의 설치/슬롯 계약 위반');
    const streams = []; // 동기 부모 한 번에 발급한 manager/노드/생성 수명을 종료까지 보관한다.
    // 실제 Create 반환 참조에만 호출별 비영 토큰을 부여한다.
    const remember = (stream) => { if (!stream.manager || !stream.section) return 0; streams.push(stream); return streams.length; };
    // current가 바뀌어도 최초 반환된 같은 생성 수명만 돌려준다.
    const resolve = (token) => {
        if (!Number.isInteger(token) || token < 1 || token > streams.length) throw new RangeError('전체 Fort snapshot의 명시 stream 참조 범위');
        return streams[token - 1];
    };
    // 일반 writer와 인라인 writer 모두 같은 명시 참조를 사용한다.
    const write = (token, bytes) => owner.templateStorage.writeSection(resolve(token), bytes);
    const access = (token, operation) => owner.templateStorage.accessSection(resolve(token), operation);
    const hooks = { // 부모의 Create와 각 child 본문을 구별해 같은 이름의 노드를 중복 생성하지 않는다.
        // 현재 manager 재조회와 선택 위치의 실제 진단이다.
        manager: (selected) => manager(owner, selected), assertion: missing('전체 Fort snapshot'),
        // 부모가 넘긴 현재 manager와 같은 슬롯에 실제 새 노드를 한 번 만든다.
        create: (selected, name) => {
            if (selected !== manager(owner, slot)) throw new Error('전체 Fort snapshot의 현재 manager 불일치');
            return remember(owner.templateStorage.createSection(slot, name));
        },
        // 기술 부모는 Create 전에 보관하고 writer는 반환 당시 같은 stream에 연결한다.
        typeNames: (selected) => owner.snapshotFortBasicSection(selected, true),
        technologyParent: () => owner.player.currentTechnology(),
        technology: (parent, token) => owner.writeFortTechnology(parent, resolve(token)),
        // 현재 Objects/Territory/Deck child는 각자의 실제 부모를 그대로 사용한다.
        objects: (selected) => owner.writeFortObjects(selected), territory: (selected) => owner.writeFortTerritory(selected),
        deck: (selected) => owner.snapshotFortBasicSection(selected, false),
        // Money는 자체 Create 부모를 부르지 않고 같은 난수와 명시 stream만 넘긴다.
        money: (token) => saveFortMoney(token, owner.random, write),
        // Mission을 만든 뒤 현재 읽기 장치를 요구하고 쓰기만 이 호출의 고정 참조에 연결한다.
        mission: (token) => {
            if (!owner.services.fortMission) throw new Error('전체 Fort snapshot의 실제 현재 Mission 읽기 장치 미연결');
            saveFortMission(token, { ...owner.services.fortMission, write });
        },
        // Badges는 자체 Create/최초 count BYTE 쓰기를 소유하며 장부 누락도 첫 getter에서 거부한다.
        badges: (selected) => saveFortBadges(selected, {
            manager: hooks.manager, create: hooks.create, access, assertion: missing('전체 Badges snapshot'),
            // 같은 실제 Client 장부의 현재 기록을 조회하고 파일값으로 대체하지 않는다.
            count: (type) => {
                if (!owner.services.fortBadges) throw new Error('전체 Fort snapshot의 실제 Client Badges 장부 미연결');
                return readFortBadgeCount(owner.services.fortBadges.records(), type);
            },
        }),
        // 실제 State 통지와 E635의 같은 현재 CoreData 본문을 사용한다.
        notify: (selected) => owner.templateState.changed(selected), coreData: (selected) => owner.writeFortCoreDataBody(selected),
    };
    const globals = { // 각 읽기 위치에서 현재 owner/두 독립 모드를 다시 조회한다.
        templateOwner: () => owner.playerState.templateOwner,
        fort: () => Number(Boolean(owner.core.ownerMode.fort)), battle: () => Number(Boolean(owner.core.ownerMode.battle)),
    };
    if (battle) snapshotBattleTemplate(slot, globals, hooks); else snapshotFortTemplate(slot, globals, hooks);
}
