// 게임 보조 전역(WB04-C2)을 검사한다.
//  - 초기값: 보호 원본 originals/Netstorm.exe(10.78) 의 PE 섹션 표로 각 원본 주소를 파일 위치로 바꿔 .data 초기값/BSS(0)을 직접 읽어 대조한다.
//    파일을 읽기만 하며 실행하지 않는다. 원본이 없는 환경에서는 이 대조만 건너뛴다.
//  - TYPE 번호 칸: 웹 타입 표(TYPE_LOAD_ORDER·PROCESS_TYPE_NAMES·내장 Form 이름)의 같은 번호 이름과 맞는지 본다.
//  - 폭 검사·자원 비트·일반 월드 준비/정리 되돌리기·장식 배열 대여·인스턴스 독립.
// (2026-11-05 WB04-C2: 세션 장기 상태의 현재 TYPE/모드 전역 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { GAME_AUX_FIELDS, GameAuxState, WORLD_DECORATION_FIELDS, MISSION_SETTING_FIELDS } from '../www/core/game-aux-state.js';
import { FIRST_ASSET_TYPE_NUMBER, TYPE_LOAD_ORDER } from '../www/assets/type-table.js';
import { PROCESS_TYPE_NAMES } from '../www/assets/type-flags.js';
import { ClientGameplayGlobals } from '../www/core/client-gameplay-globals.js';
import { GunProcessState } from '../www/core/raw-gun-process.js';
import { PriestDamageState } from '../www/core/raw-priest-damage.js';
import { ConstructionProcessState } from '../www/core/raw-construction-process.js';
import { CAMPAIGN_TUTORIAL_TYPES, createCampaignTutorialRoles } from '../www/core/raw-campaign-session-install.js';
import { createPlacementState, currentForcePlacement } from '../www/core/raw-canon-placement.js';
import { RawMissionSettingsPacket, receiveCurrentMissionSettingsPacket } from '../www/core/raw-mission-settings-packet.js';

// 보호 원본 실행 파일(10.78) 경로이다. (2026-11-05 WB04-C2)
const ORIGINAL_EXE = path.join(REPO_ROOT, 'originals', 'Netstorm.exe');

// 원본 10.78 실행 파일의 SHA-256 이다(현재 저장소 보호 원본). 다른 판본으로 대조하지 않도록 확인한다. (2026-11-05 WB04-C2)
const ORIGINAL_EXE_SHA256 = 'a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236';

// 독립 typed Session 역할도 보호 PE의 같은 현재 TYPE 기본값이며 Map/필드는 각 owner가 따로 소유한다(WB04-E622).
test('Session tutorial 역할: 보호 PE의15주소·기본 사제 기술과 인스턴스 독립', () => {
    const roles = createCampaignTutorialRoles(); const other = createCampaignTutorialRoles();
    if (fs.existsSync(ORIGINAL_EXE)) {
        const bytes = fs.readFileSync(ORIGINAL_EXE); const read = peReader(bytes); assert.equal(sha256(bytes), ORIGINAL_EXE_SHA256);
        // 각 원문 주소는 실제 PE 섹션 표로 변환해 읽는다.
        for (const [address, type] of CAMPAIGN_TUTORIAL_TYPES) assert.equal(read(address, 4).bytes.readUInt32LE(0), type);
        assert.equal(read(0x5411ac, 4).bytes.readUInt32LE(0), roles.tutorialWorkshopType); assert.equal(read(0x541174, 4).bytes.readUInt32LE(0), roles.tutorialArcherType);
    }
    roles.tutorialTypes.set(0x541258, 77); roles.tutorialArcherType = 100;
    assert.notEqual(roles.tutorialTypes, other.tutorialTypes); assert.equal(other.tutorialTypes.get(0x541258), 128); assert.equal(other.tutorialArcherType, 71);
});

// 실제 로컬 건설이 매번 읽는 생성자4BYTE는 PE 초기값과 같고 일반 월드 reset으로 바뀌지 않는다. (WB04-E569)
test('GameAuxState: 건설 요청의 현재 헤더/WORD는 보호 PE와 같고 장기 물리를 유지한다', () => {
    const aux = new GameAuxState(); const header = aux.constructionRequestHeader; assert.deepEqual([...header], [0x3e, 1]); assert.equal(aux.constructionRequestLength, 25);
    if (fs.existsSync(ORIGINAL_EXE)) { const bytes = fs.readFileSync(ORIGINAL_EXE); assert.equal(sha256(bytes), ORIGINAL_EXE_SHA256); assert.deepEqual([...peReader(bytes)(0x0052ede6, 4).bytes], [0x3e, 1, 25, 0]); }
    header[0] = 0x77; aux.constructionRequestLength = 65535; aux.resetForGeneralWorld(); assert.equal(aux.constructionRequestHeader, header); assert.deepEqual([...header], [0x77, 1]); assert.equal(aux.constructionRequestLength, 65535);
    assert.throws(() => { aux.constructionRequestLength = -1; }, RangeError); assert.throws(() => { aux.constructionRequestLength = 65536; }, RangeError); assert.throws(() => { aux.constructionRequestLength = 1.5; }, RangeError); assert.equal(new GameAuxState().constructionRequestLength, 25);
});

// Dais/건설의 현재 헤더는 보호 PE 초기값과 같고 일반 reset 뒤에도 같은2BYTE다. (WB04-E618)
test('GameAuxState: Dais/건설 현재 헤더·보호 PE·reset/별도 owner 수명', () => {
    const aux = new GameAuxState(); const dais = aux.daisProcessHeader; const construction = aux.constructionHeader; // 실제 장기 헤더 참조다.
    assert.deepEqual([...dais], [0x6a, 1]); assert.deepEqual([...construction], [0x37, 1]);
    if (fs.existsSync(ORIGINAL_EXE)) {
        const bytes = fs.readFileSync(ORIGINAL_EXE); const read = peReader(bytes); // 보호 PE는 읽기 전용이다.
        assert.equal(sha256(bytes), ORIGINAL_EXE_SHA256);
        assert.deepEqual([...read(0x00532690, 2).bytes], [...dais]); assert.deepEqual([...read(0x005325b8, 2).bytes], [...construction]);
    }
    dais[0] = 0xea; construction[1] = 3; aux.resetForGeneralWorld();
    assert.equal(aux.daisProcessHeader, dais); assert.equal(aux.constructionHeader, construction);
    assert.deepEqual([...aux.daisProcessHeader], [0xea, 1]); assert.deepEqual([...aux.constructionHeader], [0x37, 3]);
    const other = new GameAuxState(); // 새 Client의 별도 초기 상태다.
    assert.notEqual(other.daisProcessHeader, dais); assert.deepEqual([...other.daisProcessHeader], [0x6a, 1]); assert.deepEqual([...other.constructionHeader], [0x37, 1]);
});

// 두 현재 애니메이션 헤더와 공통 전송 owner는 일반 월드 초기화 뒤에도 같은 물리이다. (2026-11-16 WB04-E567)
test('GameAuxState: 애니메이션 현재 헤더·공유 전송은 보호 PE 초기값이며 일반 reset과 별개로 유지된다', () => {
    // 원문의 현재 BYTE 헤더와 실제 공통 버퍼 객체다.
    const aux = new GameAuxState(); const other = new GameAuxState();
    const rotate = aux.rotateAnimationHeader; const fire = aux.fireAnimationHeader; const wire = aux.squidTransmission;
    assert.deepEqual([...rotate], [0x3c, 1]); assert.deepEqual([...fire], [0x3b, 1]); assert.equal(wire.buffer.length, 2048);
    if (fs.existsSync(ORIGINAL_EXE)) {
        const bytes = fs.readFileSync(ORIGINAL_EXE); assert.equal(sha256(bytes), ORIGINAL_EXE_SHA256);
        const read = peReader(bytes);
        assert.deepEqual([read(0x00540468, 1).bytes[0], read(0x00540469, 1).bytes[0], read(0x0054046e, 1).bytes[0], read(0x0054046f, 1).bytes[0]], [0x3c, 1, 0x3b, 1]);
    }
    rotate[0] = 0x60; fire[1] = 3; wire.buffer[7] = 0xa5; wire.compress = -7; aux.sunArcherType = 72; aux.sunCannonType = 127; aux.windArcherType = 139;
    aux.resetForGeneralWorld();
    assert.equal(aux.rotateAnimationHeader, rotate); assert.equal(aux.fireAnimationHeader, fire); assert.equal(aux.squidTransmission, wire);
    assert.deepEqual([rotate[0], fire[1], wire.buffer[7], wire.compress, aux.sunArcherType, aux.sunCannonType, aux.windArcherType], [0x60, 3, 0xa5, -7, 72, 127, 139]);
    assert.notEqual(other.squidTransmission, wire); assert.notEqual(other.squidTransmission.buffer, wire.buffer); assert.deepEqual([...other.rotateAnimationHeader], [0x3c, 1]);
});

// PE 파일에서 가상 주소의 초기 DWORD 를 읽는 함수를 만든다. 섹션의 파일 자료 밖(초기화되지 않은 BSS)은 0 이다. 어느 섹션에도 없으면 오류이다. (2026-11-05 WB04-C2)
function peReader(bytes) {
    const pe = bytes.readUInt32LE(0x3c);
    assert.equal(bytes.toString('latin1', pe, pe + 4), 'PE\0\0');
    const count = bytes.readUInt16LE(pe + 6);
    const optionalSize = bytes.readUInt16LE(pe + 20);
    const imageBase = bytes.readUInt32LE(pe + 24 + 28);
    const table = pe + 24 + optionalSize;
    // 섹션 표를 읽는다.
    const sections = Array.from({ length: count }, (_, index) => {
        const offset = table + index * 40;
        return { name: bytes.toString('latin1', offset, offset + 8).replace(/\0+$/, ''), virtualSize: bytes.readUInt32LE(offset + 8), virtualAddress: bytes.readUInt32LE(offset + 12), rawSize: bytes.readUInt32LE(offset + 16), rawOffset: bytes.readUInt32LE(offset + 20) };
    });
    return (address, width = 4) => {
        const relative = address - imageBase;
        const section = sections.find((entry) => relative >= entry.virtualAddress && relative < entry.virtualAddress + Math.max(entry.virtualSize, entry.rawSize));
        if (!section) throw new RangeError(`섹션 밖 주소 ${address.toString(16)}`);
        const inside = relative - section.virtualAddress;
        if (inside + width > section.rawSize) return { section: section.name, bytes: Buffer.alloc(width) };
        return { section: section.name, bytes: bytes.subarray(section.rawOffset + inside, section.rawOffset + inside + width) };
    };
}

// 바이트의 SHA-256 16진 문자열이다. (2026-11-05 WB04-C2)
function sha256(bytes) {
    return crypto.createHash('sha256').update(bytes).digest('hex');
}

// 비연속 원문 주소7개를 각각 보호 PE에서 읽고 같은 현재 배열의 초기값과 비교한다. (WB04-E572)
test('GameAuxState: Session 미션 설정7DWORD의 주소/초기값이 보호 PE와 같다', (t) => {
    const aux = new GameAuxState(); assert.deepEqual([...aux.missionSettings()], [1, 0, 0, 0, 0, 1, 8]);
    assert.equal(aux.missionSettings(), aux.missionSettings()); assert.equal(aux.missionSettings().byteLength, 28);
    assert.deepEqual(MISSION_SETTING_FIELDS.map((field) => field.address), [0x5326cc, 0x55a488, 0x55a48c, 0x55a490, 0x55a494, 0x5326d0, 0x5326d4]);
    if (!fs.existsSync(ORIGINAL_EXE)) { t.diagnostic('보호 PE가 없어 실제 파일 바이트 대조만 생략한다'); return; }
    const bytes = fs.readFileSync(ORIGINAL_EXE); assert.equal(sha256(bytes), ORIGINAL_EXE_SHA256); const read = peReader(bytes);
    // 연속된 구조체라고 가정하지 않고 각 실제 원문 DWORD를 읽는다.
    for (const [index, field] of MISSION_SETTING_FIELDS.entries()) assert.equal(read(field.address).bytes.readUInt32LE(0), aux.missionSettings()[index]);
});

// 현재 피해/발사/배치 소비자는 패킷 수신 뒤 새 소유자나 다음 프레임 없이 같은 입력을 읽는다. 전체 Session 설치 검사는 아니다. (WB04-E572)
test('GameAuxState: 같은 미션 설정의 패킷 왕복이 무적·동맹 사격·강제 배치에 즉시 보인다', () => {
    const aux = new GameAuxState(), current = aux.missionSettings(), damage = new PriestDamageState(), gun = new GunProcessState(), placement = createPlacementState();
    damage.sharedInvulnerable = () => aux.missionSettings()[2]; gun.friendlyFire = () => aux.missionSettings()[3]; placement.sharedForcePlacement = () => aux.missionSettings()[1];
    const readers = gun.globals(); assert.equal(damage.isInvulnerable(), false); assert.equal(readers.friendlyFire(), false); assert.equal(currentForcePlacement(placement), 0);
    let packet; const settings = new RawMissionSettingsPacket({ current: (field) => aux.missionSettings()[field], setCurrent: (field, value) => { aux.missionSettings()[field] = value; }, send: (target, bytes) => { assert.equal(target, 9); packet = bytes.slice(); } });
    current.set([9, 0x80000000, 0xffffffff, 0x80000001, 17, 3, 16]); settings.send(9); current.fill(0); settings.receive(packet);
    assert.equal(aux.missionSettings(), current); assert.deepEqual([...current], [9, 0x80000000, 0xffffffff, 0x80000001, 17, 3, 16]);
    assert.equal(damage.isInvulnerable(), true); assert.equal(readers.friendlyFire(), true); assert.equal(currentForcePlacement(placement), 0x80000000);
    current[1] = current[2] = current[3] = 0; assert.equal(damage.isInvulnerable(), false); assert.equal(readers.friendlyFire(), false); assert.equal(currentForcePlacement(placement), 0);
});

// 원문 수신은 각DWORD를 읽는 즉시 현재 owner에 쓴다. 뒤 물리 부족이 앞선 저장을 되돌리지 않는다. (WB04-E572)
test('GameAuxState: 짧은 미션 패킷의 앞선 저장을 같은 owner에 남기고 다음 송신이 읽는다', () => {
    const aux = new GameAuxState(), current = aux.missionSettings(), packet = Uint8Array.of(1, 0x32, 32, 0, 0xef, 0xcd, 0xab, 0x89, 0x44, 0x33, 0x22, 0x11, 0xaa);
    current.fill(0xcccccccc); assert.throws(() => receiveCurrentMissionSettingsPacket(current, packet), /물리 범위/);
    assert.deepEqual([...current], [0x89abcdef, 0x11223344, 0xcccccccc, 0xcccccccc, 0xcccccccc, 0xcccccccc, 0xcccccccc]);
    let sent; new RawMissionSettingsPacket({ current: (field) => aux.missionSettings()[field], send: (target, bytes) => { sent = bytes; } }).send(9);
    assert.deepEqual([...sent.subarray(4, 12)], [...packet.subarray(4, 12)]);
});

// Session의 일반/준비 대여와 반납도 같은 GameAux 물리를 유지하며 현재 생성/완료 상태 reader는 효과 뒤 값을 읽는다. (WB04-E572)
test('GameAuxState: Session 선행 건설·운반·미션 입력을 일반/준비 대여·reset에서 보존한다', () => {
    const aux = new GameAuxState(), other = new GameAuxState(), globals = new ClientGameplayGlobals(null, null, aux), settings = aux.missionSettings(), building = new ConstructionProcessState(), gun = new GunProcessState();
    building.currentProcessType = () => aux.constructionProcessType; building.quickBuild = () => aux.quickBuild; building.quickVortex = () => aux.quickVortex;
    gun.sharedRainCannonType = () => aux.rainCannonType; gun.sharedThunderCannonType = () => aux.thunderCannonType;
    Object.assign(aux, { constructionProcessType: 31, constructionBlocked: 0x80000000, constructionDisplaceGenus: 0xffffffff, emptyCarrierType: 186, emptyGeyserType: 83, firstLocalGeyserQuantity: -1, lastLocalGeyserQuantity: -2147483648, islandDropBlocked: 3, islandStumpType: 95, quickBuild: 0x80000001, quickVortex: 0xffffffff, rainCannonType: 130, thunderCannonType: 132 });
    settings.set([3, 4, 5, 6, 7, 8, 9]); const general = globals.acquire(); general.release(); const prepared = globals.acquirePrepared(); prepared.release(); aux.resetForGeneralWorld();
    assert.equal(globals.gameAux(), aux); assert.equal(aux.missionSettings(), settings); assert.deepEqual([...settings], [3, 4, 5, 6, 7, 8, 9]);
    assert.deepEqual([building.currentType(), building.read('quickBuild'), building.read('quickVortex'), gun.readRainCannonType(), gun.readThunderCannonType()], [31, 0x80000001, 0xffffffff, 130, 132]);
    assert.deepEqual([aux.constructionBlocked, aux.constructionDisplaceGenus, aux.emptyCarrierType, aux.emptyGeyserType, aux.firstLocalGeyserQuantity, aux.lastLocalGeyserQuantity, aux.islandDropBlocked, aux.islandStumpType], [0x80000000, 0xffffffff, 186, 83, -1, -2147483648, 3, 95]);
    aux.constructionProcessType = 32; aux.quickBuild = 0; aux.quickVortex = 1; aux.rainCannonType = 129; aux.thunderCannonType = 131;
    assert.deepEqual([building.currentType(), building.read('quickBuild'), building.read('quickVortex'), gun.readRainCannonType(), gun.readThunderCannonType()], [32, 0, 1, 129, 131]);
    assert.notEqual(other.missionSettings(), settings); assert.deepEqual([...other.missionSettings()], [1, 0, 0, 0, 0, 1, 8]); assert.equal(other.firstLocalGeyserQuantity, 3);
    assert.throws(() => { aux.firstLocalGeyserQuantity = 0x80000000; }, RangeError); assert.throws(() => { aux.quickBuild = -1; }, RangeError); assert.equal(aux.quickBuild, 0);
});

// 필드 폭의 바이트 수이다. (2026-11-05 WB04-C2)
const WIDTH = Object.freeze({ u32: 4, i32: 4, f32: 4, f64: 8, bool: 1 });

// 바이트를 필드 폭에 맞춰 값으로 해석한다. (2026-11-05 WB04-C2)
function decode(kind, raw) {
    if (kind === 'u32') return raw.readUInt32LE(0);
    if (kind === 'i32') return raw.readInt32LE(0);
    if (kind === 'f32') return raw.readFloatLE(0);
    if (kind === 'f64') return raw.readDoubleLE(0);
    return raw[0] !== 0;
}

// 초기값: 모든 필드와 장식 네 칸이 원본 PE 의 .data 초기값/BSS 0 과 같고, 새 객체의 값도 같다.
test('GameAuxState: 초기값이 원본10.78 PE 의 .data/BSS 와 같다', (t) => {
    const aux = new GameAuxState();
    // 새 객체의 값이 표의 초기값이다.
    for (const field of GAME_AUX_FIELDS) assert.equal(aux[field.name], field.initial, field.name);
    assert.deepEqual([...aux.worldDecorationTypes], WORLD_DECORATION_FIELDS.map((field) => field.initial));
    if (!fs.existsSync(ORIGINAL_EXE)) {
        t.diagnostic('originals/Netstorm.exe 가 없는 환경이라 PE 대조를 건너뜀');
        return;
    }
    const bytes = fs.readFileSync(ORIGINAL_EXE);
    assert.equal(sha256(bytes), ORIGINAL_EXE_SHA256, '보호 원본 10.78 실행 파일');
    const read = peReader(bytes);
    const mismatches = [];
    // 표의 각 주소를 원본 폭으로 읽어 비교한다.
    for (const field of GAME_AUX_FIELDS) {
        const { section, bytes: raw } = read(field.address, WIDTH[field.kind]);
        const value = decode(field.kind, raw);
        if (section !== '.data' || value !== field.initial) mismatches.push(`${field.name}@${field.address.toString(16)}: ${section} ${value} ≠ ${field.initial}`);
    }
    // 장식 네 칸도 같은 방식으로 비교한다.
    for (const field of WORLD_DECORATION_FIELDS) {
        const value = read(field.address).bytes.readUInt32LE(0);
        if (value !== field.initial) mismatches.push(`장식@${field.address.toString(16)}: ${value} ≠ ${field.initial}`);
    }
    // 장기 보조 배열의 세 DWORD도 같은 보호 PE의 정적 초기값과 비교한다.
    for (let index = 0; index < aux.manoSpellTypes.length; index += 1) {
        const address = 0x00541324 + index * 4, value = read(address).bytes.readUInt32LE(0);
        if (value !== aux.manoSpellTypes[index]) mismatches.push(`주문@${address.toString(16)}: ${value} ≠ ${aux.manoSpellTypes[index]}`);
    }
    assert.deepEqual(mismatches, []);
    // 필드90개(double4개의8바이트 전체 포함)와 장식4칸이다.
    assert.deepEqual([GAME_AUX_FIELDS.length, WORLD_DECORATION_FIELDS.length], [90, 4]);
});

// TYPE 번호 칸: 웹 타입 표에서 같은 번호의 이름이 표의 대응 이름과 같다(자산 70~, 프로세스 10~, 내장 Form 1·2·3·6). 프로세스 상한 63 은 마지막 프로세스(62)+1 이다.
test('GameAuxState: TYPE 번호가 웹 타입 표의 이름과 맞는다', () => {
    // 내장 Form 네 종의 이름이다(WB04-D3 에서 Gump 3·Content 6 추가).
    const builtin = { 1: 'DependForm', 2: 'ProcessForm', 3: 'GumpForm', 6: 'ContentForm' };
    // 번호의 웹 이름을 찾는다.
    const nameOf = (number) => (number >= FIRST_ASSET_TYPE_NUMBER ? TYPE_LOAD_ORDER[number - FIRST_ASSET_TYPE_NUMBER] : number >= 10 ? PROCESS_TYPE_NAMES[number - 10] : builtin[number]);
    const typed = [...GAME_AUX_FIELDS.filter((field) => field.typeName !== null).map((field) => [field.name, field.initial, field.typeName]),
        ...WORLD_DECORATION_FIELDS.map((field) => [`decoration@${field.address.toString(16)}`, field.initial, field.typeName])];
    assert.equal(typed.length, 55);
    for (const [name, number, typeName] of typed) assert.equal(nameOf(number), typeName, `${name}=${number}`);
    assert.equal(new GameAuxState().processTypeEnd, 10 + PROCESS_TYPE_NAMES.length);
});

// 폭 검사: u32/i32 범위·f32 반올림·f64·bool 형식을 원본 폭으로 저장하고 잘못된 값은 쓰기 전에 거부한다. 자원 DWORD 의 unsigned 비트 읽기/쓰기.
test('GameAuxState: 원본 폭 검사와 자원 비트 해석', () => {
    const aux = new GameAuxState();
    aux.priestType = 0xffffffff;
    assert.equal(aux.priestType, 0xffffffff);
    assert.throws(() => { aux.priestType = -1; }, RangeError);
    assert.throws(() => { aux.priestType = 1.5; }, RangeError);
    assert.equal(aux.priestType, 0xffffffff, '거부된 쓰기는 값을 바꾸지 않는다');
    aux.totalCost = -0x80000000;
    assert.equal(aux.totalCost, -0x80000000);
    assert.throws(() => { aux.totalCost = 0x80000000; }, RangeError);
    aux.lostX = 0.1;
    assert.equal(aux.lostX, Math.fround(0.1));
    aux.geyserDeadline = 0.1;
    assert.equal(aux.geyserDeadline, 0.1);
    assert.throws(() => { aux.lostY = '1'; }, TypeError);
    aux.priestQuarterHp = true;
    assert.equal(aux.priestQuarterHp, true);
    assert.throws(() => { aux.priestQuarterHp = 1; }, TypeError);
    // 자원 DWORD 는 signed 로 저장하고 unsigned 비트로도 읽고 쓴다.
    aux.resourcePool = -1;
    assert.equal(aux.resourcePoolBits, 0xffffffff);
    aux.storeResourcePoolBits(0x80000000);
    assert.equal(aux.resourcePool, -0x80000000);
    assert.throws(() => aux.storeResourcePoolBits(-1), RangeError);
    // 장식 배열은 같은 저장소를 빌려 주며 원소 쓰기는 32비트로 감긴다.
    const decorations = aux.worldDecorationTypes;
    decorations[3] = 186;
    assert.equal(aux.worldDecorationTypes[3], 186);
    assert.equal(aux.worldDecorationTypes, decorations);
    // 새 객체는 독립이다.
    const other = new GameAuxState();
    assert.deepEqual([other.priestType, other.totalCost, other.worldDecorationTypes[3]], [158, 0, 185]);
});

// 일반 월드 준비: 정해진 fresh 필드만 되돌리고 현재 TYPE·Form/예약 번호·기한·군도 밀도는 유지한다. 정리는 손실 좌표만 -10000 으로 쓴다.
test('GameAuxState: 일반 월드 준비와 손실 좌표 정리', () => {
    const aux = new GameAuxState();
    Object.assign(aux, { ordinaryGeysers: 3, totalCost: 50, resourcePool: -7, refundPercent: 40, allowAnyCapture: 1, archipelago: 2, lostX: 5, lostY: 6, geyserType: 123, priestQuarterHp: true });
    Object.assign(aux, { priestType: 200, processFormType: 9, regularProcessType: 47, geyserDeadline: 12.5, archipelagoMinimum: 3, defaultSp: 900 });
    aux.resetForGeneralWorld();
    assert.deepEqual([aux.ordinaryGeysers, aux.totalCost, aux.resourcePool, aux.refundPercent, aux.allowAnyCapture, aux.archipelago, aux.lostX, aux.lostY, aux.geyserType, aux.priestQuarterHp],
        [0, 0, 0, 25, 0, 0, 0, 0, 122, false]);
    assert.deepEqual([aux.priestType, aux.processFormType, aux.regularProcessType, aux.geyserDeadline, aux.archipelagoMinimum, aux.defaultSp], [200, 9, 47, 12.5, 3, 900]);
    aux.resetLostPositionForCleanup();
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, aux.lostX, true);
    assert.equal(view.getUint32(0, true), 0xc61c4000);
    assert.equal(aux.lostY, aux.lostX);
    assert.equal(aux.totalCost, 0, '정리는 손실 좌표만 쓴다');
});
