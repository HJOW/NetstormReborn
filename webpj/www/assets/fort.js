// 요새/맵 파일(`.fort`, 원본 Template.cpp)을 구조로 읽는 모듈이다. 형식은 docs/formats/fort.md, 복원 기준은 cpppj `o/Template.cpp`(원본 Datamanager.cpp 0044d380 섹션 읽기,
// Template.cpp 004bd800 타입 번호 변환 · 004bdc60 오브젝트 · 004bd130 내용물 · 004be160 청크 · 004bf190 덱 · 004bd450 구독자 · 00483300 Money)이다.
// 범위: 파일 내용을 구조로 읽는 데까지이다. 원본은 읽으면서 곧바로 오브젝트를 만들어 월드에 놓지만, 이 모듈은 오브젝트 생성·영역 배치·소유자 결정을 하지 않고
// 저장 값을 그대로 보존한다. `State`·`Mission`·`CoreData`·`Badges`·`CompressedData` 섹션은 원시 바이트로만 두고 `Territory` 는 120바이트 원시 값이다.
// 파일 구조: 매직 0x46 + 플래그 1바이트 + 길이 접두 섹션(길이 u16 은 길이 필드 포함) 35개. 이름 순서는 SECTION_NAMES 이다.
// 청크 섹션(`Chaff`·`TerrNN`): 버전 u8 + 청크 레코드(표지 'c' + 오브젝트 수 u16 + 오브젝트들). `Chaff` 의 청크 i 는 월드 청크 (i % 16, i / 16) 이다.
// 이 모듈은 DOM 을 쓰지 않아 Node 에서도 불러와 검사할 수 있다.
// (2026-11-01 WB03-U: 정적 맵 화면(M2)의 첫 단계로 추가)
import { TypeFlag1, TypeFlag2, nameHash } from './type-flags.js';
import { decodeWindows1252 } from './text-encoding.js';

// 파일 첫 바이트('F')이다. 다르면 원본은 파일을 버린다. (2026-11-01 WB03-U)
const FORT_MAGIC = 0x46;

// 청크 레코드의 시작 표지('c')이다. (2026-11-01 WB03-U)
const CHUNK_MARK = 99;

// 타입 바이트가 이 값 이상이면 다리 조각 약식 표기이다(소유자 = 0xff - 값). (2026-11-01 WB03-U)
const BRIDGE_SHORTHAND_MIN = 0xf6;

// 섹션 길이 필드의 크기이다(길이에 이 2바이트가 포함된다). (2026-11-01 WB03-U)
const SECTION_LENGTH_BYTES = 2;

// 섹션 길이로 받아들이는 최댓값이다(원본은 부호 있는 16비트로 읽는다). (2026-11-01 WB03-U)
const MAX_SECTION_LENGTH = 0x7fff;

// `Territory` 섹션의 고정 길이이다(6바이트 × 20, 원본 004bd660). (2026-11-01 WB03-U)
const TERRITORY_BYTES = 0x78;

// 소유자 바이트의 유효 범위 끝(플레이어 1~8)이다. (2026-11-01 WB03-U)
const MAX_OWNER = 8;

// 영역 섹션의 수이다. (2026-11-01 WB03-U)
export const TERRITORY_COUNT = 20;

// 월드 한 변의 청크 수이다(`Chaff` 는 y 바깥·x 안쪽 순서로 16 × 16 개를 모두 담는다). (2026-11-01 WB03-U)
export const WORLD_CHUNKS_PER_SIDE = 16;

// 청크 한 변의 칸 수이다. (2026-11-01 WB03-U)
export const CHUNK_CELLS = 16;

// 섹션 이름이다(패치 005146b0 의 15개 + "Terr%02d" 20개, 파일 안 저장 순서). 로더는 이 35개만 읽고 뒤의 섹션은 무시한다. (2026-11-01 WB03-U)
export const SECTION_NAMES = Object.freeze([
    'Subscriber', 'State', 'Mission', 'CoreData', 'Chaff', 'Badges', 'Territory', 'TypeNames', 'CompressedData',
    'Technology', 'Money', 'Deck', 'Reserved2', 'Reserved3', 'Reserved4',
    ...Array.from({ length: TERRITORY_COUNT }, (_, i) => `Terr${String(i).padStart(2, '0')}`),
]);

// 바이트 열을 리틀 엔디언으로 읽는 스트림이다(원본 memstream). 끝을 넘으면 오류를 던진다(원본은 assert "pos<=end"). 섹션 이름은 오류 메시지에만 쓴다.
// 사용: const stream = new Stream(바이트, 'Chaff'); stream.u8(); stream.u16(); stream.remaining. (2026-11-01 WB03-U)
class Stream {
    // 읽을 바이트와 오류 메시지용 섹션 이름을 받는다.
    constructor(bytes, name) {
        // 읽을 바이트 열이다.
        this.bytes = bytes;
        // 오류 메시지용 섹션 이름이다.
        this.name = name;
        // 현재 읽는 위치이다.
        this.position = 0;
    }

    // 남은 바이트 수이다.
    get remaining() {
        return this.bytes.length - this.position;
    }

    // 형식 오류를 섹션 이름·위치와 함께 던진다.
    fail(message) {
        throw new Error(`fort ${this.name} @${this.position}: ${message}`);
    }

    // 읽을 만큼 남았는지 확인한다.
    require(count) {
        if (this.remaining < count) this.fail('pos<=end');
    }

    // 1바이트를 읽는다.
    u8() {
        this.require(1);
        const value = this.bytes[this.position];
        this.position += 1;
        return value;
    }

    // 부호 없는 16비트를 읽는다.
    u16() {
        this.require(2);
        const value = this.bytes[this.position] | (this.bytes[this.position + 1] << 8);
        this.position += 2;
        return value;
    }

    // 부호 없는 32비트를 읽는다.
    u32() {
        const low = this.u16();
        return (low | (this.u16() << 16)) >>> 0;
    }
}

// 부호 없는 16비트를 부호 있는 16비트로 바꾼다(`saveQB` 수량). (2026-11-01 WB03-U)
function toInt16(value) {
    return (value << 16) >> 16;
}

// 변환한 타입 번호의 타입 레코드를 찾는다. 표 밖의 번호는 형식 오류이다. (2026-11-01 WB03-U)
function typeOf(types, type, stream) {
    if (type < 0 || type >= types.records.length) stream.fail('type number outside the type table');
    return types.records[type];
}

// 파일 안 타입 번호를 현재 번호로 바꾼다. 다리 약식 표기(0xf6 이상)와 `TypeNames` 가 비어 있는 파일은 번호를 그대로 쓴다. (원본은 0xf6 미만의 번호만 변환표로 바꾼다)
// 사용: convertType(fort, 안저장번호). (2026-11-01 WB03-U)
export function convertType(fort, storedType) {
    if (storedType >= BRIDGE_SHORTHAND_MIN || !fort.hasTypeConversion) return storedType;
    return fort.typeConversion[storedType];
}

// 내용물 목록 하나를 읽는다(원본 004bd130). containerListFlags 는 그릇 타입의 목록 플래그 값이다(0 이면 항목마다 파일에서 1바이트를 읽는다).
// 변환한 타입이 0 인 항목에서 읽기를 그만두고 complete 를 거짓으로 돌려준다(원본은 0 반환). 반환: { items[], complete }. (2026-11-01 WB03-U)
function readContents(stream, fort, types, containerListFlags) {
    const list = { items: [], complete: true };
    // 개수 바이트만큼 항목을 읽는다.
    for (let count = stream.u8(); count !== 0; count -= 1) {
        const item = { storedType: stream.u8(), type: 0, quantity: 0, listFlags: 0, listFlagsStored: false, contents: [], contentsComplete: true };
        item.type = convertType(fort, item.storedType);
        if (item.type === 0) {
            list.complete = false;
            break;
        }
        const type = typeOf(types, item.type, stream);
        if ((type.flags1 & TypeFlag1.saveQA) !== 0) item.quantity = stream.u8();
        if ((type.flags1 & TypeFlag1.saveQB) !== 0) item.quantity = toInt16(stream.u16());
        // 목록 플래그: 타입의 +0x100 → 그릇 타입의 +0xfc → 둘 다 0 이면 파일의 1바이트 순서이다.
        item.listFlags = type.contentListFlags;
        if (item.listFlags === 0) item.listFlags = containerListFlags;
        if (item.listFlags === 0) {
            item.listFlags = stream.u8();
            item.listFlagsStored = true;
        }
        if ((type.flags1 & TypeFlag1.container) !== 0) {
            const nested = readContents(stream, fort, types, type.containerListFlags);
            item.contents = nested.items;
            item.contentsComplete = nested.complete;
            list.items.push(item);
            // 중첩이 끊기면 원본은 0 을 돌려주고 바깥 읽기도 멈춘다.
            if (!item.contentsComplete) {
                list.complete = false;
                break;
            }
            continue;
        }
        list.items.push(item);
    }
    return list;
}

// 청크 안 오브젝트 레코드 하나를 읽는다(원본 004bdc60). version 은 그 섹션의 버전 바이트이다.
// 반환 필드: position(상위 4비트 x·하위 4비트 y)·cellX·cellY·storedType·type(현재 번호, 0 이면 빈 레코드)·shorthandOwner·frame·quantityByte·quantityWord·bridgeFrame·
// legacyState·factoryState·owner(파일의 값 그대로)·islandByte·contents. 없는 선택 필드는 null 이다. (2026-11-01 WB03-U)
function readObject(stream, fort, types, version) {
    const object = {
        position: stream.u8(), cellX: 0, cellY: 0, storedType: stream.u8(), type: 0, shorthandOwner: null, frame: null, quantityByte: null, quantityWord: null,
        bridgeFrame: null, legacyState: null, factoryState: null, owner: null, islandByte: null, contents: null,
    };
    object.cellX = object.position >> 4;
    object.cellY = object.position & 0xf;
    if (object.storedType < BRIDGE_SHORTHAND_MIN) object.type = convertType(fort, object.storedType);
    else {
        object.shorthandOwner = 0xff - object.storedType;
        object.type = types.bridgeType;
    }
    // 타입 0 은 뒤의 1바이트(0 이어야 함)만 읽고 끝난다(원본 assert).
    if (object.type === 0) {
        if (stream.u8() !== 0) stream.fail('num == 0');
        return object;
    }
    // 섬 타입은 뒤의 1바이트만 건너뛴다.
    if (object.type === types.islandType) {
        object.islandByte = stream.u8();
        return object;
    }
    const type = typeOf(types, object.type, stream);
    if ((type.flags1 & TypeFlag1.saveFrame) !== 0) object.frame = stream.u8();
    if ((type.flags1 & TypeFlag1.saveQA) !== 0) object.quantityByte = stream.u8();
    if ((type.flags1 & TypeFlag1.saveQB) !== 0) object.quantityWord = toInt16(stream.u16());
    if ((type.flags2 & TypeFlag2.bridge) !== 0) object.bridgeFrame = stream.u8();
    // 버전 0: 원본 004ad0a0 은 `or edx, 0x4000000` 뒤의 분기 때문에 항상 참이라 모든 타입에서 1바이트를 읽는다.
    if (version === 0) object.legacyState = stream.u8();
    if (version > 1 && (type.flags2 & TypeFlag2.factory) !== 0) object.factoryState = stream.u8();
    if (version > 0 && (type.flags2 & TypeFlag2.ownerSavedMask) !== 0) object.owner = stream.u8();
    if ((type.flags1 & TypeFlag1.container) !== 0) object.contents = readContents(stream, fort, types, type.containerListFlags);
    return object;
}

// 오브젝트 소유자 바이트를 원본이 쓰는 값으로 바꾼다: 0 이거나 8 보다 크면 1 이다(원본 004bdc60). 소유자 바이트가 없는 오브젝트는 null 이다. (2026-11-01 WB03-U)
export function normalizedOwner(object) {
    if (object.owner === null) return null;
    return object.owner === 0 || object.owner > MAX_OWNER ? 1 : object.owner;
}

// `Chaff`·`TerrNN` 섹션을 읽는다: 버전 바이트 뒤로 섹션이 끝날 때까지 청크 레코드를 읽는다(원본 004be160).
// 원본은 월드 청크를 y 바깥·x 안쪽으로 돌며 해당 청크마다 레코드 하나를 읽는다. 어느 청크인지는 영역 배치가 정하므로 이 모듈은 저장 순서의 청크 목록만 돌려준다.
// 반환: { present, version, chunks: [{ objects[] }] }. 빈 섹션은 present 거짓, 청크 없음이다. (2026-11-01 WB03-U)
function readChunkSection(bytes, name, fort, types) {
    const section = { present: false, version: 0, chunks: [] };
    if (bytes.length === 0) return section;
    const stream = new Stream(bytes, name);
    section.version = stream.u8();
    section.present = stream.remaining !== 0;
    // 청크 레코드마다 표지와 오브젝트 수가 온다.
    while (stream.remaining !== 0) {
        if (stream.u8() !== CHUNK_MARK) stream.fail('data.readByte() == 99');
        const chunk = { objects: [] };
        // 빈 레코드(타입 0)도 개수에 들어 있으므로 그대로 보존한다.
        for (let count = stream.u16(); count !== 0; count -= 1) chunk.objects.push(readObject(stream, fort, types, section.version));
        section.chunks.push(chunk);
    }
    return section;
}

// 섹션 이름(대소문자 무시)의 원시 바이트를 돌려준다. 파일에 없으면 빈 배열이다. (2026-11-01 WB03-U)
export function fortSection(fort, name) {
    const lower = name.toLowerCase();
    // 이름 표의 위치가 곧 파일 안의 섹션 순서이다.
    for (let i = 0; i < SECTION_NAMES.length && i < fort.sections.length; i += 1) {
        if (SECTION_NAMES[i].toLowerCase() === lower) return fort.sections[i];
    }
    return new Uint8Array(0);
}

// `TypeNames` 섹션으로 파일의 타입 번호 → 현재 타입 번호 변환표를 만든다(원본 004bd800). 현재 타입마다 이름 해시가 같은 첫 파일 번호에 자기 번호를 적는다.
// 같은 해시의 현재 타입이 여럿이면 번호가 큰 것이 남는다(이름 없는 타입의 해시 0 이 그렇다). 섹션이 비어 있으면 변환 없이 파일의 번호를 그대로 쓴다. (2026-11-01 WB03-U)
function readTypeConversion(fort, types) {
    fort.typeHashes = [];
    fort.hasTypeConversion = false;
    fort.typeConversion = new Uint8Array(255);
    const section = fortSection(fort, 'TypeNames');
    if (section.length === 0) return;
    const stream = new Stream(section, 'TypeNames');
    // 개수 바이트 뒤로 해시가 이어진다.
    for (let count = stream.u8(); count !== 0; count -= 1) fort.typeHashes.push(stream.u32());
    fort.hasTypeConversion = true;
    // 현재 타입 번호를 차례로 훑어 해시가 같은 첫 파일 번호의 변환값을 고친다.
    types.records.forEach((record, type) => {
        const hash = nameHash(record.name);
        // 파일의 번호 가운데 해시가 같은 첫 번호만 고친다.
        for (let stored = 0; stored < fort.typeHashes.length; stored += 1) {
            if (fort.typeHashes[stored] !== hash) continue;
            if (stored < fort.typeConversion.length) fort.typeConversion[stored] = type;
            break;
        }
    });
}

// `.fort` 파일 전체를 읽는다. types 는 type-flags.js 의 createFortTypes 결과이고, 파일을 읽을 판본(10.78)의 타입 표여야 한다. 형식 오류는 오류를 던진다.
// 반환: { flag, sections[], trailingBytes, subscriber{ id, nameBytes, name }|null, money|null, typeHashes[], hasTypeConversion, typeConversion, technology{ items, complete },
//        technologyTrailingBytes, deck[{ storedType, type, chance, power, remaining }], territory(Uint8Array 120)|null, chaff, territories[20] }.
// 사용: const fort = parseFort(archive.read('\\d\\savetheisland.fort'), createFortTypes(table)). (2026-11-01 WB03-U)
export function parseFort(bytes, types) {
    if (bytes.length < 2 || bytes[0] !== FORT_MAGIC) throw new Error('fort: first byte is not 0x46');
    const fort = {
        flag: bytes[1], sections: [], trailingBytes: 0, subscriber: null, money: null, typeHashes: [], hasTypeConversion: false, typeConversion: new Uint8Array(255),
        technology: { items: [], complete: true }, technologyTrailingBytes: 0, deck: [], territory: null, chaff: null, territories: [],
    };
    let position = 2;
    // 길이(길이 필드 포함)와 내용이 이어진다. 이름 목록(35개)만큼만 읽고, 길이가 맞지 않으면 거기서 멈춘다(새 구현의 보호 처리).
    while (fort.sections.length < SECTION_NAMES.length && bytes.length - position >= SECTION_LENGTH_BYTES) {
        const length = bytes[position] | (bytes[position + 1] << 8);
        if (length < SECTION_LENGTH_BYTES || length > MAX_SECTION_LENGTH || length > bytes.length - position) break;
        fort.sections.push(bytes.slice(position + SECTION_LENGTH_BYTES, position + length));
        position += length;
    }
    fort.trailingBytes = bytes.length - position;

    // Subscriber: 소유자 ID 4바이트 + 문자열(길이 16비트 + 글자). 이름은 원본 바이트 그대로 두고 Windows-1252 로 해석한 글도 함께 준다.
    const subscriber = fortSection(fort, 'Subscriber');
    if (subscriber.length !== 0) {
        const stream = new Stream(subscriber, 'Subscriber');
        const id = stream.u32();
        const nameBytes = [];
        // 이름은 길이만큼의 바이트이다.
        for (let length = stream.u16(); length !== 0; length -= 1) nameBytes.push(stream.u8());
        fort.subscriber = { id, nameBytes: Uint8Array.from(nameBytes), name: decodeWindows1252(Uint8Array.from(nameBytes)) };
    }
    // Money: 32비트 실수(시작 Storm Power).
    const money = fortSection(fort, 'Money');
    if (money.length >= 4) fort.money = new DataView(money.buffer, money.byteOffset, money.byteLength).getFloat32(0, true);
    // Territory: 120바이트를 그대로 둔다. 해석은 영역 배치 단계에서 한다.
    const territory = fortSection(fort, 'Territory');
    if (territory.length >= TERRITORY_BYTES) fort.territory = territory.slice(0, TERRITORY_BYTES);
    readTypeConversion(fort, types);
    // Technology: 플레이어를 그릇으로 한 내용물 목록이다. 그 타입(PlayerSquid)의 그릇 목록 플래그는 0 이라 목록 플래그를 파일에서 읽는다.
    const technology = fortSection(fort, 'Technology');
    if (technology.length !== 0) {
        const stream = new Stream(technology, 'Technology');
        fort.technology = readContents(stream, fort, types, 0);
        fort.technologyTrailingBytes = stream.remaining;
    }
    // Deck: 개수 + (타입, chance, power, 남은 횟수) 4바이트 항목. 변환한 타입이 0 인 항목도 보존한다(원본은 덱에 넣지 않는다).
    const deck = fortSection(fort, 'Deck');
    if (deck.length !== 0) {
        const stream = new Stream(deck, 'Deck');
        // 항목 개수만큼 읽는다.
        for (let count = stream.u8(); count !== 0; count -= 1) {
            const storedType = stream.u8();
            fort.deck.push({ storedType, type: convertType(fort, storedType), chance: stream.u8(), power: (stream.u8() << 24) >> 24, remaining: stream.u8() });
        }
    }
    fort.chaff = readChunkSection(fortSection(fort, 'Chaff'), 'Chaff', fort, types);
    // 영역 섹션 20개.
    for (let i = 0; i < TERRITORY_COUNT; i += 1) {
        const name = SECTION_NAMES[SECTION_NAMES.length - TERRITORY_COUNT + i];
        fort.territories.push(readChunkSection(fortSection(fort, name), name, fort, types));
    }
    return fort;
}
