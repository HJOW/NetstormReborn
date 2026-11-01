// 원본 타입 구조체(188개)의 이름·플래그·그룹·발자국을 `.type` 정의에서 만드는 모듈이다. `.fort` 오브젝트 레코드는 타입마다 저장하는 필드가 달라서
// (프레임·수량·다리·작업장 상태·소유자·내용물) 타입 플래그가 있어야 읽을 수 있고, 파일 속 타입 번호를 현재 번호로 바꾸는 `TypeNames` 해시에는 모든 타입의 이름이 필요하다.
// 복원 기준: cpppj `RiftTypeTable.cpp`(플래그 단어 48개·속성 읽기 0049c3b0, 후처리 0049b0d0, 이름 해시 004bd6f0/004bd800)의 10.78 패치판 경로이다.
// 옮긴 범위: 플래그 1·2, 그릇/내용물 목록 플래그, `group`, 발자국(`foot_x`·`foot_y`), 이름(내장 타입·프로세스 타입 이름 포함). 비용·체력·사거리·깊이 같은 수치와
// 생성자 주소는 옮기지 않았다(웹 판독기가 쓰지 않음, 타입 표의 그림 규칙은 type-table.js 가 맡는다).
// 이 모듈은 DOM 을 쓰지 않아 Node 에서도 불러와 검사할 수 있다.
// (2026-11-01 WB03-U: `.fort` 판독기를 위해 추가)
import { FIRST_ASSET_TYPE_NUMBER, PATCH_TYPE_COUNT, TYPE_LOAD_ORDER } from './type-table.js';

// 타입 플래그 1(원본 타입 구조체 +0xe8)의 비트 중 이름이 붙은 것이다(cpppj TypeFlag1). (2026-11-01 WB03-U)
export const TypeFlag1 = Object.freeze({
    defaultHotspot: 0x1, mayDropOnIsle: 0x2, mayDropOnRim: 0x4, hasHitPoints: 0x10, saveFrame: 0x20, surface: 0x800,
    saveQA: 0x1000, saveQB: 0x2000, usesPower: 0x4000, container: 0x20000,
});

// 타입 플래그 2(원본 타입 구조체 +0xec)의 비트 중 이름이 붙은 것이다(cpppj TypeFlag2). (2026-11-01 WB03-U)
export const TypeFlag2 = Object.freeze({
    island: 0x2, bridge: 0x4, dropBlocking: 0x10, bomb: 0x100, vortex: 0x200, groupBattery: 0x800, buried: 0x2000, factory: 0x4000,
    walker: 0x10000, balloon: 0x20000, emplacement: 0x40000, groupBlocker: 0x4000000, groupArcherOrCannon: 0x8000000,
    // 소유 플레이어 1바이트를 저장하는 타입의 마스크이다(패치 DAT_00542644). (2026-11-01 WB03-U)
    ownerSavedMask: 0x5d77cf00,
    // 읽은 뒤 소유자를 0 으로 만드는 타입(geyser·buried)의 마스크이다(패치 DAT_00542648). (2026-11-01 WB03-U)
    ownerClearedMask: 0x10002000,
});

// `group` 속성이 없을 때의 값이다(원본 NO_GROUP). (2026-11-01 WB03-U)
export const NO_GROUP = 10;

// `group` 속성의 이름 표이다(패치 00542468). 번호 = 배열 위치이고 없는 이름은 -1 이다. (2026-11-01 WB03-U)
const GROUP_NAMES = Object.freeze(['archer', 'cannon', 'blocker', 'aviary', 'flyer', 'battery', 'fence', 'walker', 'balloon', 'misc']);

// 내장 프로세스 타입 이름 53개이다(패치 00540fa0, 번호 10 부터). 타입 구조체의 이름 필드가 20바이트라 20글자까지만 쓰인다. (2026-11-01 WB03-U)
export const PROCESS_TYPE_NAMES = Object.freeze([
    'evilRaySystemProcessType', 'fieryTailSystemProcessType', 'streakSystemProcessType', 'ropeSystemProcessType',
    'burningSystemProcessType', 'kaboomSystemProcessType', 'flyingShrapnelSystemProcessType', 'buildSystemProcessType',
    'dissolveSystemProcessType', 'lightningSystemProcessType', 'beamOverSystemProcessType', 'smokeSystemProcessType',
    'fountainSystemProcessType', 'teleportEffectProcessType', 'dudeProcessType', 'canonRealizeProcessType',
    'rookAttackProcessType', 'targetProcessType', 'missileProcessType', 'pingPongProcessType',
    'constructionProcessType', 'baseFlyerProcessType', 'sunAviaryProcessType', 'islandDropperProcessType',
    'gunProcessType', 'fireAnimProcessType', 'rotateAnimProcessType', 'scrollTestProcessType',
    'simpleWalkerProcessType', 'simpleFlyProcessType', 'wormProcessType', 'bulfProcessType',
    'balloonProcessType', 'ambleProcessType', 'driftProcessType', 'rainVortexAnimProcessType',
    'regularProcessType', 'soundProcessType', 'windBatteryProcessType', 'buildPlaybackProcessType',
    'manaDisplayProcessType', 'rangeDisplayProcessType', 'islandAnimatorProcessType', 'cameraAnimatorProcessType',
    'priestProcessType', 'paletteFlasherProcessType', 'thunderArcherSystemProcessType', 'bombEffectTimerProcessType',
    'daisProcessType', 'windAviaryProcessType', 'rainAviaryProcessType', 'sharedRegularProcessType',
    'thunderAviaryProcessType',
]);

// 내장 프로세스 타입의 첫 번호이다(원본 루프의 시작값). (2026-11-01 WB03-U)
const FIRST_PROCESS_TYPE_NUMBER = 10;

// 타입 구조체 이름 필드의 길이(+4~+0x17)이다. 프로세스 타입 이름은 이 길이까지만 들어간다. (2026-11-01 WB03-U)
const TYPE_NAME_BYTES = 20;

// 설명 필드(+0x18)에 복사하는 최대 글자 수이다. (2026-11-01 WB03-U)
const DESCRIPTION_BYTES = 40;

// 후처리(0049b0d0)가 이름을 채우는 내장 타입이다(번호 → 이름). (2026-11-01 WB03-U)
const BUILTIN_NAMES = Object.freeze({ 1: 'DependForm', 2: 'ProcessForm', 3: 'GumpForm', 4: 'PlayerSquid', 6: 'ContentForm' });

// 플래그 단어 하나가 켜는 비트이다(원본 0049c3b0 의 단어 비교 순서, 대소문자 무시). [단어, 플래그 1 비트, 플래그 2 비트] (2026-11-01 WB03-U)
const FLAG_WORDS = Object.freeze([
    ['carribleInVehicle', 0x10000, 0], ['container', 0x20000, 0],
    ['default_hotspot', 0x1, 0], ['defaulthotspot', 0x1, 0],
    ['shadow', 0x40000, 0], ['flyershadow', 0x400000, 0],
    ['not_selectable', 0x8000000, 0], ['notselectable', 0x8000000, 0],
    ['not_real', 0x28000000, 0x2000000], ['notreal', 0x28000000, 0x2000000],
    ['createsisland', 0x400, 0], ['mayDropOnRim', 0x4, 0], ['mayDropOnIsle', 0x2, 0],
    ['surface', 0x800, 0], ['randframe', 0x100000, 0], ['matchframe', 0x200000, 0],
    ['opaqueCollide', 0x800000, 0], ['dontSave', 0x20000000, 0],
    ['saveFrame', 0x20, 0], ['saveQA', 0x1000, 0], ['saveQB', 0x2000, 0],
    ['dropblocking', 0, 0x10], ['blocking', 0, 0x18], ['shotblocking', 0, 0x20],
    ['fencePost', 0, 0x1], ['fence', 0, 0x20000000],
    ['buried', 0, 0x2000], ['nugget', 0, 0x8000], ['bomb', 0, 0x100],
    ['emplacement', 0, 0x40000], ['bridge', 0, 0x4], ['walker', 0, 0x10000],
    ['balloon', 0, 0x20000], ['island', 0, 0x2], ['islandThreeByThree', 0, 0x1000000],
    ['factory', 0, 0x4000], ['vortex', 0, 0x200], ['edgefarm', 0, 0x80000],
    ['focus', 0, 0], ['guy', 0, 0x400], ['geyser', 0, 0x10000000],
    ['flyer', 0, 0x100000], ['residence', 0, 0x40000000], ['fringe', 0, 0x1000],
    ['priest', 0, 0x200000], ['dais', 0, 0x400000], ['altar', 0, 0x80000000],
    ['predictable', 0, 0],
].map(([word, flags1, flags2]) => Object.freeze({ word: word.toLowerCase(), flags1, flags2 })));

// 이름 해시를 낸다(원본 004bd6f0). 글자를 부호 있는 8비트로 읽어 `(위치 % 4) × 8` 비트 왼쪽으로 밀어 32비트로 감기게 더한다.
// 이름은 20글자에서 자르지 않는다(타입 구조체의 이름 필드를 NUL 까지 읽는다). 사용: nameHash('mana') → 부호 없는 32비트 정수. (2026-11-01 WB03-U)
export function nameHash(name) {
    let hash = 0;
    // 이름의 글자마다 더한다.
    for (let i = 0; i < name.length; i += 1) {
        // 부호 있는 8비트로 넓힌 글자를 위치에 따라 밀고, 32비트로 감아 더한다.
        const extended = (name.charCodeAt(i) << 24) >> 24;
        hash = (hash + ((extended << ((i & 3) * 8)) >>> 0)) >>> 0;
    }
    return hash;
}

// 속성 이름(대소문자 무시)이 맞는지 확인한다. (2026-11-01 WB03-U)
function isProperty(property, name) {
    return property.name.toLowerCase() === name;
}

// 숫자를 원본 `FUN_004e49c0`(float → int, 0쪽으로 버림)처럼 32비트 정수로 바꾼다. 범위를 벗어나면 0 이다. (2026-11-01 WB03-U)
function toInt(value) {
    if (!(value > -2147483649 && value < 2147483648)) return 0;
    return Math.trunc(value);
}

// `.type` 정의 하나의 플래그·속성을 타입 레코드에 적용한다(원본 0049c3b0 의 플래그 단어와 `group`·`foot_x/y`·사용량·`maxHitPoints` 부분).
// definition 은 parseTypeDefinition 의 결과이고 null 이면 읽지 못한 타입이라 아무것도 켜지 않는다. 원본 assert 세 가지는 오류로 던진다. (2026-11-01 WB03-U)
function applyDefinition(record, definition) {
    record.footX = 1;
    record.footY = 1;
    if (!definition) return;
    let hasHitPoints = false;
    // 플래그 단어마다 표의 비트를 켠다. 표에 없는 단어는 원본처럼 조용히 지나간다.
    for (const word of definition.flags) {
        const lower = word.toLowerCase();
        if (lower === 'dontdrawdefault') throw new Error('타입 플래그 dontdrawdefault 는 원본이 거부합니다.');
        // 같은 단어가 표의 한 항목에만 맞지만 원본처럼 전체를 훑는다.
        for (const entry of FLAG_WORDS) {
            if (entry.word !== lower) continue;
            record.flags1 = (record.flags1 | entry.flags1) >>> 0;
            record.flags2 = (record.flags2 | entry.flags2) >>> 0;
        }
    }
    // 원본은 shadow 와 flyershadow 가 함께 있으면 assert 한다.
    if ((record.flags1 & 0x40000) !== 0 && (record.flags1 & 0x400000) !== 0) throw new Error('shadow 와 flyershadow 는 함께 쓸 수 없습니다.');
    // 사용량 속성(원소 순서: 비·천둥·바람)이다. 0 이 아닌 값이 하나라도 있으면 usesPower 가 켜진다.
    const minUsage = [0, 0, 0];
    const maxUsage = [0, 0, 0];
    // 속성은 파일에 나온 순서대로 적용한다. 같은 이름이 여러 번이면 뒤의 값이 남는다.
    for (const property of definition.properties) {
        const number = typeof property.value === 'number' ? property.value : null;
        const text = typeof property.value === 'string' ? property.value : null;
        if (isProperty(property, 'maxhitpoints')) {
            if (number === null) throw new Error('maxHitPoints 는 숫자여야 합니다.');
            hasHitPoints = true;
        } else if (isProperty(property, 'minusage') && number !== null) minUsage[0] = Math.fround(number);
        else if ((isProperty(property, 'maxusage') || isProperty(property, 'maxrainbattleusage')) && number !== null) maxUsage[0] = Math.fround(number);
        else if (isProperty(property, 'maxthunderbattleusage') && number !== null) maxUsage[1] = Math.fround(number);
        else if (isProperty(property, 'maxwindbattleusage') && number !== null) maxUsage[2] = Math.fround(number);
        else if ((isProperty(property, 'foot_x') || isProperty(property, 'footx')) && number !== null) record.footX = toInt(number);
        else if ((isProperty(property, 'foot_y') || isProperty(property, 'footy')) && number !== null) record.footY = toInt(number);
        else if (isProperty(property, 'group') && text !== null) {
            record.group = -1;
            // 이름 표에서 첫 일치의 위치가 그룹 번호이다.
            for (let i = 0; i < GROUP_NAMES.length; i += 1) {
                if (text.toLowerCase() === GROUP_NAMES[i]) {
                    record.group = i;
                    break;
                }
            }
        } else if (isProperty(property, 'description') && text !== null) {
            record.description = text.slice(0, DESCRIPTION_BYTES);
            record.hasDescription = true;
        }
    }
    if (hasHitPoints) record.flags1 = (record.flags1 | TypeFlag1.hasHitPoints) >>> 0;
    // 원본 0049ad00/0049ad60: 사용량을 0 이 아닌 값으로 정하면 usesPower 가 켜진다.
    for (let i = 0; i < 3; i += 1) {
        if (minUsage[i] !== 0 || maxUsage[i] !== 0) record.flags1 = (record.flags1 | TypeFlag1.usesPower) >>> 0;
    }
}

// 후처리(원본 0049b0d0)가 켜는 파생 플래그이다. 내장·빈 타입을 포함한 모든 타입에 번호순으로 적용한다. (2026-11-01 WB03-U)
function postProcess(record) {
    if (record.group === 5) record.flags2 = (record.flags2 | TypeFlag2.groupBattery) >>> 0;
    if (record.group === 1 || record.group === 0) record.flags2 = (record.flags2 | TypeFlag2.groupArcherOrCannon) >>> 0;
    if (record.group === 2) record.flags2 = (record.flags2 | TypeFlag2.groupBlocker) >>> 0;
    const flags2 = record.flags2;
    // vortex·factory·walker·balloon 은 내용물을 가진다(그릇 목록 플래그는 walker·balloon 이면 0x20, 아니면 0x10).
    if ((flags2 & 0x34200) !== 0) {
        record.flags1 = (record.flags1 | TypeFlag1.container) >>> 0;
        record.containerListFlags = (record.containerListFlags | ((flags2 & 0x30000) === 0 ? 0x10 : 0x20)) >>> 0;
    }
    if ((flags2 & TypeFlag2.bomb) !== 0) record.contentListFlags = (record.contentListFlags | 1) >>> 0;
    if ((flags2 & TypeFlag2.buried) !== 0) record.flags1 = (record.flags1 | TypeFlag1.saveQA) >>> 0;
    if ((flags2 & 0xa00) !== 0) record.flags1 = (record.flags1 | 0x10000000) >>> 0;
    if ((flags2 & TypeFlag2.factory) !== 0) record.flags1 = (record.flags1 | 0x4000000) >>> 0;
    if ((flags2 & TypeFlag2.vortex) !== 0) record.flags1 = (record.flags1 | 0x84000) >>> 0;
    // 원본 assert: saveQA 와 saveQB 는 함께 쓸 수 없다.
    if ((record.flags1 & TypeFlag1.saveQA) !== 0 && (record.flags1 & TypeFlag1.saveQB) !== 0) throw new Error('saveQA 와 saveQB 는 함께 쓸 수 없습니다.');
    if ((record.flags1 & TypeFlag1.mayDropOnRim) === 0) record.flags1 = (record.flags1 | TypeFlag1.mayDropOnIsle) >>> 0;
    if ((record.flags2 & TypeFlag2.emplacement) !== 0) record.flags2 = (record.flags2 | TypeFlag2.dropBlocking) >>> 0;
    const finalFlags2 = record.flags2;
    if ((finalFlags2 & TypeFlag2.emplacement) !== 0 && (finalFlags2 & 0x4200) === 0 && record.footX === 3 && record.footY === 3) record.flags1 = (record.flags1 | 6) >>> 0;
    if ((finalFlags2 & 0x408100) !== 0) record.flags1 = (record.flags1 | TypeFlag1.mayDropOnIsle) >>> 0;
    if ((finalFlags2 & 0x70000) !== 0 && (finalFlags2 & TypeFlag2.factory) === 0) record.flags1 = (record.flags1 | 0x8000) >>> 0;
    // 원본: foot_y 6 은 8 로 바꾼다.
    if (record.footY === 6) record.footY = 8;
}

// 비어 있는 타입 레코드(모든 필드 0, 그룹 없음)를 만든다. (2026-11-01 WB03-U)
function emptyRecord(number) {
    return {
        number, name: '', assetName: null, description: '', hasDescription: false, fromAsset: false,
        flags1: 0, flags2: 0, containerListFlags: 0, contentListFlags: 0, group: NO_GROUP, footX: 0, footY: 0,
    };
}

// 타입 레코드 188개를 번호순으로 만든다(원본 전체 타입 배열, 패치판). definitions 는 type-table.js 의 TYPE_LOAD_ORDER 순서의 `.type` 정의 배열(읽지 못한 것은 null)이다.
// 순서: 자산 타입(70~185)에 플래그·속성 적용 → 프로세스 타입 이름(10~62, 20글자까지) → 내장 타입 이름 → 모든 타입 후처리.
// 사용: const records = buildTypeRecords(definitions); records[82].name === 'bridge'. 반환 레코드는 얼려 있다. (2026-11-01 WB03-U)
export function buildTypeRecords(definitions) {
    if (definitions.length !== TYPE_LOAD_ORDER.length) throw new Error('타입 정의 수가 로딩 순서와 다릅니다.');
    const records = [];
    // 모든 번호의 빈 레코드를 먼저 만든다.
    for (let number = 0; number < PATCH_TYPE_COUNT; number += 1) records.push(emptyRecord(number));
    // 자산 타입: 이름은 `typename` 이 아니라 파일 이름이다(원본은 실행 파일의 이름 목록에서 복사).
    TYPE_LOAD_ORDER.forEach((assetName, index) => {
        const record = records[FIRST_ASSET_TYPE_NUMBER + index];
        record.name = assetName;
        record.assetName = assetName;
        record.fromAsset = true;
        applyDefinition(record, definitions[index]);
        // 이름이 20글자 이상이면 종료 문자가 없어 바로 뒤의 설명 필드까지 이어 읽힌다(`fakeThreeByThreeSurface`). 설명이 있을 때만 그렇게 만든다.
        if (record.name.length >= TYPE_NAME_BYTES && record.hasDescription) record.name = record.name.slice(0, TYPE_NAME_BYTES) + record.description;
    });
    // 내장 프로세스 타입 이름: 20바이트 필드에 복사하므로 긴 이름은 잘린다.
    PROCESS_TYPE_NAMES.forEach((name, index) => {
        records[FIRST_PROCESS_TYPE_NUMBER + index].name = name.slice(0, TYPE_NAME_BYTES);
    });
    // 후처리가 이름을 채우는 내장 타입.
    for (const [number, name] of Object.entries(BUILTIN_NAMES)) records[Number(number)].name = name;
    // 후처리는 번호순으로 모든 타입에 적용한다.
    for (const record of records) postProcess(record);
    return records.map((record) => Object.freeze(record));
}

// `.fort` 판독기가 쓰는 타입 정보 묶음을 타입 표에서 만든다: 번호별 타입 레코드 188개와 다리·섬 타입 번호(원본 전역 DAT_005411a0/DAT_005411d0, 패치판 82·94).
// 사용: const types = createFortTypes(TypeTable.fromArchive(archive, decodeOriginalText)); parseFort(bytes, types). (2026-11-01 WB03-U)
export function createFortTypes(table) {
    const bridgeType = table.find('bridge');
    const islandType = table.find('island');
    if (bridgeType < 0 || islandType < 0) throw new Error('타입 표에 bridge 또는 island 가 없습니다.');
    return Object.freeze({ records: buildTypeRecords(table.definitions), bridgeType, islandType });
}
