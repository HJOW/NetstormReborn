// 원본 타입 구조체(188개)의 이름·플래그·그룹·발자국을 `.type` 정의에서 만드는 모듈이다. `.fort` 오브젝트 레코드는 타입마다 저장하는 필드가 달라서
// (프레임·수량·다리·작업장 상태·소유자·내용물) 타입 플래그가 있어야 읽을 수 있고, 파일 속 타입 번호를 현재 번호로 바꾸는 `TypeNames` 해시에는 모든 타입의 이름이 필요하다.
// 복원 기준: cpppj `RiftTypeTable.cpp`(플래그 단어 48개·속성 읽기 0049c3b0, 후처리 0049b0d0, 이름 해시 004bd6f0/004bd800)의 10.78 패치판 경로이다.
// 옮긴 범위: 플래그 1·2, 그릇/내용물 목록 플래그, `group`, 발자국(`foot_x`·`foot_y`), 이름(내장 타입·프로세스 타입 이름 포함). 2026-11-10 WB04-E265 에서 나머지 속성
// 전부(체력·깊이 zorder·비용과 생략 비용·사거리·피해율·공중 피해율·피해 효과·techBit·기준점·레벨·테마·종류·기본 프레임·방향/물리 프레임 수)와 타입별 생성자 주소를 더했다.
// 타입 표의 그림 규칙은 type-table.js 가 맡는다.
// 이 모듈은 DOM 을 쓰지 않아 Node 에서도 불러와 검사할 수 있다.
// (2026-11-01 WB03-U: `.fort` 판독기를 위해 추가)
import { FIRST_ASSET_TYPE_NUMBER, PATCH_TYPE_COUNT, TYPE_LOAD_ORDER } from './type-table.js';
import { typeConstructorAddress } from '../core/squid-constructors.js';
import { readRawArcherTypeData } from '../core/raw-archer-type-data.js';

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

// 종류 필드(+0xf0)에 단어를 복사하는 플래그 단어이다(cpppj FlagWord.kind). 8글자까지 덮어쓰고 종료 문자는 쓰지 않는다. (2026-11-10 WB04-E265)
const KIND_WORDS = Object.freeze(new Set(['buried', 'nugget', 'bomb', 'bridge', 'walker', 'balloon', 'island', 'factory', 'vortex', 'edgefarm', 'focus', 'guy', 'geyser', 'residence']));
const KIND_BYTES = 8;

// 가격 후처리에서 명시 비용도 지우는 사제 타입 번호이다(cpppj kCostExemptPriestType). (2026-11-10 WB04-E265)
const COST_EXEMPT_PRIEST_TYPE = 158;

// zorder 문자열의 깊이 이름 23개와 값이다(패치 00540d10). 이름 비교는 대소문자를 구분하는 부분 문자열(strstr)이고 표 순서의 첫 일치를 쓴다. (2026-11-10 WB04-E265)
const Z_ORDER_NAMES = Object.freeze([
    ['zoNONE', -127], ['zoFALLING', 30], ['zoSTALAG', 20], ['zoCHALRING', 20], ['zoBATTLE', 10],
    ['zoEDGEFARM', 0], ['zoISLAND', 0], ['zoBRIDGE', 0], ['zoBRIDGE_CONNECTOR', -10], ['zoARTIFACTS', -15],
    ['zoEMPLACEMENTS', -20], ['zoILLEGAL_DITHER', -21], ['zoFLARES', -22], ['zoMISSILES', -25],
    ['zoFLYER_SHADOWS', -26], ['zoFLYERS', -30], ['zoFENCE', -35], ['zoMANAICON', -36],
    ['zoUBERGUMP', -40], ['zoMENUGUMP', -60], ['zoDIALOGGUMP', -80], ['zoLOOKGUMP', -100], ['zoRISING', -31],
]);

// zorder 문자열을 깊이 값으로 바꾼다: 첫 식별자를 찾아 표의 부분 문자열 일치(표 순서의 첫 항목)를 고르고, 뒤의 '식별자 +/- 숫자' 식이 있으면 DWORD 로 가감한다.
// 이름이 없거나 표에 없으면 거부하고 오프셋이 2147483647 을 넘으면 범위 오류이다. (2026-11-10 WB04-E265: cpppj RiftTypeTable.cpp ZOrder)
function zOrderFromText(text) {
    const word = /([A-Za-z0-9_]+)[ \t]*/.exec(text);
    if (!word) throw new Error('zorder 이름이 없습니다.');
    // 표 순서대로 첫 부분 문자열 일치를 고른다.
    for (const [name, base] of Z_ORDER_NAMES) {
        if (!name.includes(word[1])) continue;
        let value = base >>> 0;
        const offset = /[ \t]*([A-Za-z0-9_]+)[ \t]*(\+|-)[ \t]*([0-9]+)/.exec(text);
        if (offset) {
            const delta = Number(offset[3]);
            if (delta > 2147483647) throw new RangeError('zorder 오프셋 범위 오류');
            value = offset[2] === '+' ? (value + delta) >>> 0 : (value - delta) >>> 0;
        }
        return value | 0;
    }
    throw new Error('알 수 없는 zorder 이름입니다.');
}

// techBit 숫자는 float32 로 줄인 뒤 CRT _ftol 의 signed 64 비트로 절삭해 낮은 DWORD 만 저장한다(비유한/범위 밖은 0). (2026-11-10 WB04-E265)
function techBitNumber(value) {
    const number = Math.fround(value);
    if (!Number.isFinite(number) || number < -9223372036854775808 || number >= 9223372036854775808) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(number))));
}

// theme 문자열의 첫 BYTE 로 TYPE+98 조회 표를 읽는다(패치: 'wrtsWRTS.?>' 앞 10글자가 인덱스 % 5, 첫 90 미만 BYTE 의 기본 5, 그 외 0). 122 이상이면 표 밖이라 거부한다. (2026-11-10 WB04-E265)
function themeNumber(text) {
    const first = text.length === 0 ? 0 : text.charCodeAt(0) & 0xff;
    if (first >= 122) throw new RangeError('TYPE theme 의 원본 초기화 표 범위 초과');
    let value = first < 90 ? 5 : 0;
    const letters = 'wrtsWRTS.?>';
    // 초기화 몸체와 같은 순서로 지정 글자의 DWORD 를 덮어쓴다.
    for (let i = 0; i < 10; i += 1) if (first === letters.charCodeAt(i)) value = i % 5;
    return value;
}

// hotFootRatioX/Y 를 기준점 정수로 바꾼다: float32 비율에 16(가로)/11(세로)을 곱해 0 쪽으로 절삭하고 int32 범위 밖은 거부한다. (2026-11-10 WB04-E265)
function typeHotFootOffset(ratio, vertical) {
    const value = Math.trunc(Math.fround(ratio) * (vertical ? 11 : 16));
    if (!Number.isFinite(value) || value < -2147483648 || value > 2147483647) throw new RangeError('타입 기준점의 32비트 정수 범위 초과');
    return value;
}

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

// `.type` 정의 하나의 플래그·속성을 타입 레코드에 적용한다(원본 0049c3b0 의 플래그 단어와 `maxHitPoints`·`zorder`·`cost`·`range`·`hpPerSec`·`useairdamage`·`airdamage`·`damageEffect`·`techBit`·사용량·`foot_x/y`·`hotFootRatioX/Y`·`level`·`theme`·`description`·`group`).
// definition 은 parseTypeDefinition 의 결과이고 null 이면 읽지 못한 타입이라 아무것도 켜지 않는다(발자국 1×1 초기화만). 원본 assert 와 거부 속성은 오류로 던진다.
// 속성은 파일에 나온 순서대로 적용하며 같은 이름이 여러 번이면 뒤의 값이 남는다. 수치는 원문 float32 AST 폭(Math.fround)이나 0 쪽 절삭 정수로 저장한다.
// (2026-11-01 WB03-U: 플래그·그룹·발자국·사용량. 2026-11-10 WB04-E265: cpppj RiftTypeTable.cpp ApplyDefinition 전체 — HP·깊이·비용·사거리·피해율·기준점·레벨·테마·종류·방향/프레임 수)
function applyDefinition(record, definition) {
    record.footX = 1;
    record.footY = 1;
    if (!definition) return;
    // 파싱이 고른 기본 프레임(+128)과 아처 소비자의 방향 수/물리 프레임 수(TYPE+110/+114)를 같은 표에 보관한다.
    record.defaultFrame = definition.defaultFrame;
    const animation = readRawArcherTypeData(definition);
    record.directions = animation.directions;
    record.frameCount = animation.frameCount;
    const kind = new Array(KIND_BYTES).fill('\0');
    // 플래그 단어마다 표의 비트를 켠다. 표에 없는 단어는 원본처럼 조용히 지나간다.
    for (const word of definition.flags) {
        const lower = word.toLowerCase();
        if (lower === 'dontdrawdefault') throw new Error('타입 플래그 dontdrawdefault 는 원본이 거부합니다.');
        // 같은 단어가 표의 한 항목에만 맞지만 원본처럼 전체를 훑는다.
        for (const entry of FLAG_WORDS) {
            if (entry.word !== lower) continue;
            record.flags1 = (record.flags1 | entry.flags1) >>> 0;
            record.flags2 = (record.flags2 | entry.flags2) >>> 0;
            // 종류 단어는 8글자까지 종류 필드에 덮어쓴다(종료 문자는 쓰지 않아 짧은 단어는 이전 내용의 뒤가 남는다).
            if (KIND_WORDS.has(lower)) for (let i = 0; i < KIND_BYTES && i < word.length; i += 1) kind[i] = word[i];
        }
    }
    // 원본은 shadow 와 flyershadow 가 함께 있으면 assert 한다.
    if ((record.flags1 & 0x40000) !== 0 && (record.flags1 & 0x400000) !== 0) throw new Error('shadow 와 flyershadow 는 함께 쓸 수 없습니다.');
    // 속성은 파일에 나온 순서대로 적용한다. 같은 이름이 여러 번이면 뒤의 값이 남는다.
    for (const property of definition.properties) {
        const number = typeof property.value === 'number' ? property.value : null;
        const text = typeof property.value === 'string' ? property.value : null;
        if (isProperty(property, 'maxhitpoints')) {
            if (number === null) throw new Error('maxHitPoints 는 숫자여야 합니다.');
            if (record.maxHitPoints !== 0) throw new Error('maxHitPoints == 0');
            record.flags1 = (record.flags1 | TypeFlag1.hasHitPoints) >>> 0;
            record.maxHitPoints = toInt(number);
        } else if (isProperty(property, 'zorder')) {
            if (number !== null) record.zOrder = toInt(number);
            else if (text !== null) record.zOrder = zOrderFromText(text);
        } else if (isProperty(property, 'cost')) {
            if (number === null) throw new Error('cost 는 숫자여야 합니다.');
            record.cost = Math.fround(number);
        } else if (isProperty(property, 'range')) {
            if (number === null) throw new Error('range 는 숫자여야 합니다.');
            record.range = Math.fround(number);
        } else if (isProperty(property, 'hppersec')) {
            if (number === null) throw new Error('hpPerSec 는 숫자여야 합니다.');
            record.damagePerSecond = Math.fround(number);
        } else if (isProperty(property, 'useairdamage')) {
            if (number === null) throw new Error('useairdamage 는 숫자여야 합니다.');
            record.useAirDamage = toInt(number);
        } else if (isProperty(property, 'airdamage')) {
            if (number === null) throw new Error('airdamage 는 숫자여야 합니다.');
            record.airDamagePerSecond = Math.fround(number);
        } else if (isProperty(property, 'damageeffect')) {
            if (number === null) throw new Error('damageEffect 는 숫자여야 합니다.');
            record.damageEffect = toInt(number);
        } else if (isProperty(property, 'techbit')) {
            if (number === null) throw new Error('techBit 는 숫자여야 합니다.');
            record.techBitIndex = techBitNumber(number);
        } else if (isProperty(property, 'minusage') && number !== null) record.minUsage[0] = Math.fround(number);
        else if ((isProperty(property, 'maxusage') || isProperty(property, 'maxrainbattleusage')) && number !== null) record.maxUsage[0] = Math.fround(number);
        else if (isProperty(property, 'maxthunderbattleusage') && number !== null) record.maxUsage[1] = Math.fround(number);
        else if (isProperty(property, 'maxwindbattleusage') && number !== null) record.maxUsage[2] = Math.fround(number);
        else if ((isProperty(property, 'foot_x') || isProperty(property, 'footx')) && number !== null) record.footX = toInt(number);
        else if ((isProperty(property, 'foot_y') || isProperty(property, 'footy')) && number !== null) record.footY = toInt(number);
        else if (isProperty(property, 'hotfootratiox') || isProperty(property, 'hotfootratioy')) {
            if (number === null) throw new Error('hotFootRatio 는 숫자여야 합니다.');
            const vertical = isProperty(property, 'hotfootratioy');
            if (vertical) record.hotspotY = typeHotFootOffset(number, true);
            else record.hotspotX = typeHotFootOffset(number, false);
        } else if (isProperty(property, 'hotfootx') || isProperty(property, 'hotfooty')) throw new Error('원본이 거부하는 hotFootX/Y 속성입니다.');
        else if (isProperty(property, 'level') && number !== null) record.level = (toInt(number) - 1) | 0;
        else if (isProperty(property, 'theme')) {
            if (text === null) throw new Error('theme 은 문자열이어야 합니다.');
            record.theme = themeNumber(text);
        } else if (isProperty(property, 'group') && text !== null) {
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
    // 원본 0049ad00/0049ad60: 사용량을 0 이 아닌 값으로 정하면 usesPower 가 켜진다.
    for (let i = 0; i < 3; i += 1) {
        if (record.minUsage[i] !== 0 || record.maxUsage[i] !== 0) record.flags1 = (record.flags1 | TypeFlag1.usesPower) >>> 0;
    }
    // 종류 필드는 8글자까지 채우고 첫 종료 문자(없으면 전체)까지를 문자열로 읽는다.
    const end = kind.indexOf('\0');
    record.kind = kind.slice(0, end < 0 ? KIND_BYTES : end).join('');
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
    // 비용 생략 보정: 그룹이 있는 타입이 비용 0 이면 (레벨 + 1) × 일반 200 / 운반(워커·벌룬) 400 을 float32 로 저장한다. 내부 level 은 자료 level − 1 이고 DWORD 감김으로 더한다. (2026-11-10 WB04-E265)
    if (record.group !== NO_GROUP && record.cost === 0) {
        const level = (record.level + 1) | 0;
        const unit = (finalFlags2 & 0x30000) !== 0 ? 400 : 200;
        record.cost = Math.fround(level * unit);
    }
    // 원본: foot_y 6 은 8 로 바꾼다.
    if (record.footY === 6) record.footY = 8;
    // 사제 타입과 guy 부류는 명시 비용도 마지막에 지운다. (2026-11-10 WB04-E265)
    if (record.number === COST_EXEMPT_PRIEST_TYPE || (finalFlags2 & 0x400) !== 0) record.cost = 0;
}

// 비어 있는 타입 레코드(모든 필드 0, 그룹 없음)를 만든다. (2026-11-01 WB03-U)
function emptyRecord(number) {
    return {
        number, name: '', assetName: null, description: '', hasDescription: false, fromAsset: false,
        flags1: 0, flags2: 0, containerListFlags: 0, contentListFlags: 0, group: NO_GROUP, footX: 0, footY: 0,
        // 2026-11-10 WB04-E265: 원본 전체 TYPE 초기화(0 으로 지우고 +98 에 패치 5)와 RiftTypeTable 의 나머지 필드. 수치는 float32/int32 로 저장된 값이다.
        maxHitPoints: 0, zOrder: 0, cost: 0, range: 0, damagePerSecond: 0, useAirDamage: 0, airDamagePerSecond: 0, damageEffect: 0, techBitIndex: 0,
        minUsage: [0, 0, 0], maxUsage: [0, 0, 0], level: 0, theme: 5, hotspotX: 0, hotspotY: 0, defaultFrame: 0, directions: 0, frameCount: 0, kind: '',
        constructorAddress: 0,
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
    // 원본은 로딩/후처리 뒤 생성자 표를 대입한다. 주소는 Factory 가 미복원 파생 분배를 감지하는 메타데이터이다. (2026-11-10 WB04-E265)
    for (const record of records) record.constructorAddress = typeConstructorAddress(record.number);
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
