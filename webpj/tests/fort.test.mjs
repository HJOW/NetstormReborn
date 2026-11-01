// `.fort` 판독기를 검사한다: 타입 레코드(이름·플래그·그룹)와 `TypeNames` 해시, 원본 `.fort` 465개 전체(느슨한 파일 433 + 아카이브 32)를 독립 구현(tools/fort.py)이 만든 기대값 표와
// 줄 단위로 대조, 그리고 합성 입력으로 레코드 선택 필드·버전 0/1/2·다리 약식 표기·섬·내용물 목록·변환표·오류 처리를 확인한다.
// 기대값 표는 webpj/tools/make-fort-expected.py 가 tools/fort.py 로 만들었고 www/ 구현의 출력으로 다시 만들지 않는다.
// (2026-11-01 WB03-U: `.fort` 판독기 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT, readData } from './helpers.mjs';
import { TaffArchive } from '../www/assets/taff.js';
import { decodeOriginalText } from '../www/assets/text-encoding.js';
import { FIRST_ASSET_TYPE_NUMBER, PATCH_TYPE_COUNT, TYPE_LOAD_ORDER, TypeTable } from '../www/assets/type-table.js';
import { NO_GROUP, PROCESS_TYPE_NAMES, TypeFlag1, TypeFlag2, buildTypeRecords, createFortTypes, nameHash } from '../www/assets/type-flags.js';
import { SECTION_NAMES, TERRITORY_COUNT, convertType, fortSection, normalizedOwner, parseFort } from '../www/assets/fort.js';

// 공용 자료이다: 아카이브·타입 표·`.fort` 판독용 타입 정보. (2026-11-01 WB03-U)
const archive = new TaffArchive(readData('netstorm.tarc'));
const table = TypeTable.fromArchive(archive, decodeOriginalText);
const types = createFortTypes(table);
const records = types.records;

// 로딩 순서 이름으로 타입 번호를 찾는다. (2026-11-01 WB03-U)
function typeNumber(name) {
    return FIRST_ASSET_TYPE_NUMBER + TYPE_LOAD_ORDER.indexOf(name);
}

// 원본 `.fort` 전체 목록이다: [{ source, bytes }]. 느슨한 파일은 assets/game-data/d, 아카이브 항목은 netstorm.tarc 에서 읽는다. 저장소 자산이 없으면 null 이다. (2026-11-01 WB03-U)
function loadOriginalForts() {
    const folder = path.join(REPO_ROOT, 'assets', 'game-data', 'd');
    if (!fs.existsSync(folder)) return null;
    const list = [];
    // 느슨한 파일은 이름(대소문자 무시) 순이다.
    const names = fs.readdirSync(folder).filter((name) => name.toLowerCase().endsWith('.fort')).sort((a, b) => a.toLowerCase().localeCompare(b.toLowerCase(), 'en'));
    for (const name of names) list.push({ source: `loose:${name}`, bytes: new Uint8Array(fs.readFileSync(path.join(folder, name))) });
    // 아카이브 안의 .fort 는 이름 인덱스 표 순서이다.
    for (const name of archive.names()) {
        if (name.toLowerCase().endsWith('.fort')) list.push({ source: `tarc:${name}`, bytes: archive.read(name) });
    }
    return list;
}

// 합성 `.fort` 를 만드는 도구이다. 섹션 바이트 배열 목록(이름 순서)을 길이 접두 섹션으로 잇는다. (2026-11-01 WB03-U)
function buildFort(sectionBytes, { flag = 0 } = {}) {
    const parts = [Uint8Array.of(0x46, flag)];
    // 섹션마다 길이(자기 포함)를 앞에 붙인다.
    for (const bytes of sectionBytes) parts.push(Uint8Array.of((bytes.length + 2) & 255, (bytes.length + 2) >> 8), Uint8Array.from(bytes));
    return Uint8Array.from(parts.flatMap((part) => [...part]));
}

// 이름 순서(SECTION_NAMES)에 맞춰 일부 섹션만 채운 섹션 배열을 만든다. 나머지는 빈 섹션이다. (2026-11-01 WB03-U)
function sectionsWith(map) {
    return SECTION_NAMES.map((name) => map[name] ?? []);
}

// 청크 섹션 바이트를 만든다: 버전 + 청크마다 'c' + 개수(u16) + 오브젝트 바이트들. chunks 는 오브젝트 바이트 배열의 배열이다. (2026-11-01 WB03-U)
function chunkSection(version, chunks) {
    const bytes = [version];
    // 청크마다 표지와 오브젝트 수, 오브젝트 바이트를 잇는다.
    for (const objects of chunks) {
        bytes.push(99, objects.length & 255, objects.length >> 8);
        for (const object of objects) bytes.push(...object);
    }
    return bytes;
}

// ----- 타입 레코드 -----

// 레코드는 188개이고 번호·이름이 원본 체계를 따른다: 내장 5개, 프로세스 53개, 자산 116개, 나머지는 이름 없음. 다리·섬 타입 번호는 82·94 이다. (2026-11-01 WB03-U)
test('buildTypeRecords: 188개 번호 체계와 이름', () => {
    assert.equal(records.length, PATCH_TYPE_COUNT);
    assert.equal(types.bridgeType, 82);
    assert.equal(types.islandType, 94);
    assert.deepEqual([1, 2, 3, 4, 6].map((n) => records[n].name), ['DependForm', 'ProcessForm', 'GumpForm', 'PlayerSquid', 'ContentForm']);
    // 프로세스 타입 이름은 번호 10 부터 20글자까지만 들어간다.
    PROCESS_TYPE_NAMES.forEach((name, index) => assert.equal(records[10 + index].name, name.slice(0, 20), `프로세스 타입 ${10 + index}`));
    assert.equal(PROCESS_TYPE_NAMES.length, 53);
    // 자산 타입은 파일 이름(로딩 순서)이다. 단 이름이 20글자 이상이고 설명이 있으면 이름 앞 20글자 + 설명이 된다.
    TYPE_LOAD_ORDER.forEach((name, index) => {
        const record = records[FIRST_ASSET_TYPE_NUMBER + index];
        assert.equal(record.assetName, name);
        assert.equal(record.fromAsset, true);
        if (name.length < 20) assert.equal(record.name, name);
    });
    // 이름이 없는 번호: 0, 5, 7, 8, 9, 63~69, 186, 187.
    for (const n of [0, 5, 7, 8, 9, 63, 64, 65, 66, 67, 68, 69, 186, 187]) assert.equal(records[n].name, '', `빈 타입 ${n}`);
    assert.equal(records[typeNumber('fakeThreeByThreeSurface')].name, 'fakeThreeByThreeSurffake3x3Surface');
    assert.equal(nameHash(records[typeNumber('fakeThreeByThreeSurface')].name), 0x0534a54b, '원본 .fort 의 해시와 같다(cpppj 문서)');
});

// 이름 해시: 글자를 위치에 따라 밀어 32비트로 감아 더한다. 알려진 값과 부호 있는 글자·긴 이름을 확인한다. (2026-11-01 WB03-U)
test('nameHash: 해시 규칙', () => {
    assert.equal(nameHash(''), 0);
    assert.equal(nameHash('a'), 0x61);
    assert.equal(nameHash('ab'), 0x6261);
    assert.equal(nameHash('abcd'), 0x64636261);
    // 5번째 글자는 다시 0바이트 자리로 돌아와 더해진다.
    assert.equal(nameHash('abcde'), (0x64636261 + 0x65) >>> 0);
    // 128 이상의 글자는 부호 있는 8비트로 넓혀 더한다.
    assert.equal(nameHash('ÿ'), 0xffffffff);
    assert.equal(nameHash('ÿa'), ((0xffffffff + (0x61 << 8)) >>> 0));
});

// 타입 플래그: 저장 형식을 정하는 비트와 파생 비트를 대표 타입으로 확인한다. (2026-11-01 WB03-U)
test('타입 플래그: 대표 타입', () => {
    const flags = (name) => records[typeNumber(name)];
    // 다리: bridge 단어(플래그 2 의 0x4).
    assert.ok((flags('bridge').flags2 & TypeFlag2.bridge) !== 0);
    // 작업장: factory(0x4000), 그릇이라 container 이고 그릇 목록 플래그는 0x10, 소유자를 저장한다.
    const factory = flags('sunFactory');
    assert.ok((factory.flags2 & TypeFlag2.factory) !== 0);
    assert.ok((factory.flags1 & TypeFlag1.container) !== 0);
    assert.equal(factory.containerListFlags, 0x10);
    assert.ok((factory.flags2 & TypeFlag2.ownerSavedMask) !== 0);
    // 걷는 유닛과 풍선은 운반체라 그릇 목록 플래그가 0x20 이다.
    assert.equal(flags('sunWalker').containerListFlags, 0x20);
    assert.equal(flags('sunBalloon').containerListFlags, 0x20);
    // 신전: vortex 는 내용물을 가지고 소유자를 저장한다.
    assert.ok((flags('rainVortex').flags1 & TypeFlag1.container) !== 0);
    // 간헐천과 묻힌 것은 소유자를 읽은 뒤 0 으로 만든다. buried 는 saveQA 가 파생된다.
    assert.ok((flags('geyser').flags2 & TypeFlag2.ownerClearedMask) !== 0);
    assert.ok((flags('buried').flags1 & TypeFlag1.saveQA) !== 0);
    // 폭탄(주문)은 그릇을 찾지 않고 자기 목록 플래그 1 을 가진다.
    assert.equal(flags('bombHeal').contentListFlags, 1);
    // 플레이어 그릇(PlayerSquid)은 목록 플래그가 0 이라 Technology 항목마다 파일에서 읽는다.
    assert.equal(records[4].containerListFlags, 0);
    // 그룹: 포대는 battery(5)로 0x800, 활은 archer(0)로 0x8000000, 돌탑은 blocker(2)로 0x4000000 이 켜진다.
    assert.equal(flags('rainBattery').group, 5);
    assert.ok((flags('rainBattery').flags2 & TypeFlag2.groupBattery) !== 0);
    assert.ok((flags('sunArcher').flags2 & TypeFlag2.groupArcherOrCannon) !== 0);
    assert.ok((flags('sunBlocker').flags2 & TypeFlag2.groupBlocker) !== 0);
    // group 이 없는 타입은 NO_GROUP 이다.
    assert.equal(flags('bridge').group, NO_GROUP);
    // 모든 타입이 mayDropOnRim 이 아니면 mayDropOnIsle 이 켜지고, 후처리는 빈 타입에도 적용된다(플래그 1 이 2).
    assert.equal(records[0].flags1, TypeFlag1.mayDropOnIsle);
    // 어떤 타입도 saveQA 와 saveQB 를 함께 갖지 않는다.
    for (const record of records) assert.ok(!((record.flags1 & TypeFlag1.saveQA) !== 0 && (record.flags1 & TypeFlag1.saveQB) !== 0), `타입 ${record.number}`);
});

// 합성 정의로 플래그 단어(대소문자 무시)·group·foot·maxHitPoints·사용량·원본 assert 를 확인한다. (2026-11-01 WB03-U)
test('buildTypeRecords: 합성 정의의 플래그와 속성', () => {
    const base = TYPE_LOAD_ORDER.map(() => null);
    const make = (flags, properties = []) => ({ name: 'x', constructorWords: [], flags, properties, clusters: [] });
    const build = (index, definition) => buildTypeRecords(base.map((value, i) => (i === index ? definition : value)))[FIRST_ASSET_TYPE_NUMBER + index];
    // 단어는 대소문자를 구분하지 않고 비트를 켠다. 3×3 emplacement 는 mayDropOnIsle/Rim 비트(6)가 켜진다.
    const emplacement = build(0, make(['EMPLACEMENT', 'SaveFrame'], [{ name: 'FOOT_X', value: 3 }, { name: 'footY', value: 3 }, { name: 'group', value: 'Cannon' }, { name: 'maxHitPoints', value: 5 }]));
    assert.ok((emplacement.flags1 & TypeFlag1.saveFrame) !== 0);
    assert.ok((emplacement.flags2 & TypeFlag2.emplacement) !== 0);
    assert.ok((emplacement.flags2 & TypeFlag2.dropBlocking) !== 0, 'emplacement 는 dropblocking 이 파생된다');
    assert.ok((emplacement.flags2 & TypeFlag2.groupArcherOrCannon) !== 0, 'cannon 그룹');
    assert.equal(emplacement.group, 1);
    assert.equal(emplacement.footX, 3);
    assert.ok((emplacement.flags1 & 6) === 6, '3×3 emplacement');
    assert.ok((emplacement.flags1 & TypeFlag1.hasHitPoints) !== 0);
    // foot_y 6 은 8 이 된다. 알 수 없는 그룹 이름은 -1 이다. 숫자 group 은 무시한다.
    assert.equal(build(1, make([], [{ name: 'foot_y', value: 6 }])).footY, 8);
    assert.equal(build(1, make([], [{ name: 'group', value: 'nonsense' }])).group, -1);
    assert.equal(build(1, make([], [{ name: 'group', value: 3 }])).group, NO_GROUP);
    // 사용량이 0 이 아니면 usesPower 가 켜지고, 0 이면 켜지지 않는다.
    assert.ok((build(2, make([], [{ name: 'maxRainBattleUsage', value: 2 }])).flags1 & TypeFlag1.usesPower) !== 0);
    assert.equal(build(2, make([], [{ name: 'minUsage', value: 0 }])).flags1 & TypeFlag1.usesPower, 0);
    // 읽지 못한 정의(null)는 플래그가 없고 발자국만 1×1 이다.
    assert.equal(build(3, null).footX, 1);
    // 원본이 거부하는 조합은 오류이다.
    assert.throws(() => build(4, make(['dontdrawdefault'])), /dontdrawdefault/);
    assert.throws(() => build(4, make(['shadow', 'flyershadow'])), /flyershadow/);
    assert.throws(() => build(4, make(['saveQA', 'saveQB'])), /saveQA/);
    assert.throws(() => build(4, make([], [{ name: 'maxHitPoints', value: 'text' }])), /maxHitPoints/);
});

// ----- 원본 .fort 전체 -----

// 원본 `.fort` 전체와 기대값 표의 대조에 쓰는 한 줄 서식이다(tools/make-fort-expected.py 와 같은 서식). (2026-11-01 WB03-U)
function typeLabel(type) {
    return type >= FIRST_ASSET_TYPE_NUMBER && type < FIRST_ASSET_TYPE_NUMBER + TYPE_LOAD_ORDER.length ? records[type].assetName : '-';
}

// 내용물 목록을 `이름[:qa=값][:qb=값][:ct=[…]]` 항목을 쉼표로 이은 글로 만든다. qb 는 부호 없는 16비트로 적는다. (2026-11-01 WB03-U)
function formatContents(items) {
    return items.map((item) => {
        const record = records[item.type];
        let text = typeLabel(item.type);
        if ((record.flags1 & TypeFlag1.saveQA) !== 0) text += `:qa=${item.quantity}`;
        if ((record.flags1 & TypeFlag1.saveQB) !== 0) text += `:qb=${item.quantity & 0xffff}`;
        if ((record.flags1 & TypeFlag1.container) !== 0) text += `:ct=[${formatContents(item.contents)}]`;
        return text;
    }).join(',');
}

// 오브젝트 한 개를 `x,y|타입|필드…` 한 줄로 만든다. 필드 순서는 고정이다. (2026-11-01 WB03-U)
function formatObject(object) {
    let text = `${object.cellX},${object.cellY}|${typeLabel(object.type)}`;
    if (object.frame !== null) text += `|f=${object.frame}`;
    if (object.quantityByte !== null) text += `|qa=${object.quantityByte}`;
    if (object.quantityWord !== null) text += `|qb=${object.quantityWord & 0xffff}`;
    if (object.bridgeFrame !== null) text += `|br=${object.bridgeFrame}`;
    if (object.factoryState !== null) text += `|fa=${object.factoryState}`;
    const owner = object.owner ?? object.shorthandOwner;
    if (owner !== null) text += `|ow=${owner}`;
    if (object.islandByte !== null) text += `|sk=${object.islandByte}`;
    if (object.contents !== null) text += `|ct=[${formatContents(object.contents.items)}]`;
    return text;
}

// 파일 하나의 기대값 표 칸 목록(출처 칸 제외)을 만든다. (2026-11-01 WB03-U)
function describeFort(fort) {
    const lines = [`tech ${fort.technology.items.length}`];
    // 기술 항목마다 저장 타입 번호·이름·수량·목록 플래그·내용물을 적는다.
    for (const item of fort.technology.items) {
        const record = records[item.type];
        let text = `T|${item.storedType}|${typeLabel(item.type)}`;
        if ((record.flags1 & TypeFlag1.saveQA) !== 0) text += `|qa=${item.quantity}`;
        if ((record.flags1 & TypeFlag1.saveQB) !== 0) text += `|qb=${item.quantity}`;
        text += `|lf=${item.listFlags}`;
        if ((record.flags1 & TypeFlag1.container) !== 0) text += `|ct=[${formatContents(item.contents)}]`;
        lines.push(text);
    }
    // 덱 항목.
    for (const entry of fort.deck) lines.push(`D|${typeLabel(entry.type)}|${entry.chance}|${entry.power}|${entry.remaining}`);
    const counts = [];
    // Chaff 와 영역 섹션을 이름 순서로 적는다(버전 바이트 말고도 내용이 있는 섹션만).
    const named = [['Chaff', fort.chaff], ...fort.territories.map((section, i) => [`Terr${String(i).padStart(2, '0')}`, section])];
    for (const [name, section] of named) {
        if (!section.present) continue;
        lines.push(`S|${name}|v${section.version}|${section.chunks.length}`);
        let total = 0;
        // 청크마다 오브젝트 수와 오브젝트 줄을 적는다.
        section.chunks.forEach((chunk, index) => {
            lines.push(`C|${index}|${chunk.objects.length}`);
            total += chunk.objects.length;
            for (const object of chunk.objects) lines.push(formatObject(object));
        });
        counts.push(`${name}=${section.chunks.length}/${total}`);
    }
    const subscriber = fort.subscriber;
    const money = fortSection(fort, 'Money');
    return [
        fort.flag.toString(16).padStart(2, '0'), String(fort.sections.length), String(fort.trailingBytes), subscriber ? String(subscriber.id) : '-',
        subscriber ? (Buffer.from(subscriber.nameBytes).toString('hex') || '-') : '-', money.length >= 4 ? Buffer.from(money.subarray(0, 4)).toString('hex') : '-',
        String(fort.technology.items.length), String(fort.deck.length), counts.join(',') || '-',
        crypto.createHash('sha256').update(`${lines.join('\n')}\n`, 'ascii').digest('hex'),
    ];
}

// 기대값 표를 읽는다: 출처 → 칸 배열. (2026-11-01 WB03-U)
function readExpected() {
    const text = fs.readFileSync(path.join(import.meta.dirname, 'fixtures', 'fort-expected.tsv'), 'utf8');
    const rows = new Map();
    // 주석(#) 줄을 건너뛰고 한 줄씩 칸으로 나눈다.
    for (const line of text.split(/\r?\n/)) {
        if (line === '' || line.startsWith('#')) continue;
        const cells = line.split('\t');
        rows.set(cells[0], cells.slice(1));
    }
    return rows;
}

// 원본 `.fort` 465개를 모두 읽어 독립 구현의 기대값과 같은지 본다. 파일 목록·꼬리·구독자·Money·기술·덱·청크/오브젝트 수·전체 저장 내용의 SHA-256 이 같아야 한다. (2026-11-01 WB03-U)
test('parseFort: 원본 .fort 465개가 독립 구현(tools/fort.py)의 기대값과 같다', (t) => {
    const forts = loadOriginalForts();
    if (!forts) return t.skip('assets/game-data 가 없음');
    const expected = readExpected();
    assert.equal(forts.length, 465);
    assert.equal(expected.size, 465, '기대값 표의 줄 수');
    let objects = 0;
    // 파일마다 읽어 표와 칸 단위로 대조한다.
    for (const { source, bytes } of forts) {
        const fort = parseFort(bytes, types);
        const want = expected.get(source);
        assert.ok(want, `${source}: 기대값 줄이 없음`);
        assert.deepEqual(describeFort(fort), want, source);
        objects += fort.chaff.chunks.reduce((sum, chunk) => sum + chunk.objects.length, 0) + fort.territories.reduce((sum, section) => sum + section.chunks.reduce((s, c) => s + c.objects.length, 0), 0);
    }
    // 오브젝트 레코드 총수는 cpppj 문서(313,712개)와 같다. 문서의 수는 느슨한 파일 + 아카이브 465개 전체이다.
    assert.equal(objects, 313712);
    return undefined;
});

// 타입을 188개로 저장한 파일(10.78 패치판 계열)은 `TypeNames` 해시가 현재 타입 표의 이름 해시와 정확히 같고, 변환은 번호를 그대로 둔다. (2026-11-01 WB03-U)
test('parseFort: 188개 타입 표 파일의 TypeNames 해시가 타입 레코드 이름 해시와 같다', (t) => {
    const forts = loadOriginalForts();
    if (!forts) return t.skip('assets/game-data 가 없음');
    const expectedHashes = records.map((record) => nameHash(record.name));
    let matched = 0;
    // 모든 파일 가운데 타입 188개로 저장한 것만 골라 대조한다.
    for (const { source, bytes } of forts) {
        const fort = parseFort(bytes, types);
        if (fort.typeHashes.length !== PATCH_TYPE_COUNT) continue;
        matched += 1;
        assert.deepEqual(fort.typeHashes, expectedHashes, source);
        let zeroSeen = false;
        // 해시가 0 이 아닌 번호는 자기 번호로 돌아온다. 해시가 0 인 번호 가운데 첫 번호만 가장 큰 빈 타입(187)이 되고 나머지는 0 이다.
        expectedHashes.forEach((hash, stored) => {
            if (hash !== 0) assert.equal(convertType(fort, stored), stored, `${source} ${stored}`);
            else {
                assert.equal(convertType(fort, stored), zeroSeen ? 0 : 187, `${source} 해시 0 번호 ${stored}`);
                zeroSeen = true;
            }
        });
    }
    // cpppj 문서가 센 29개와 같다.
    assert.equal(matched, 29);
    return undefined;
});

// 공식 미션 맵의 대표 내용을 직접 확인한다(기대값 표와 별개로 문서에 적힌 사실): Save the Island! 의 이름·Storm Power·덱·영역. (2026-11-01 WB03-U)
test('parseFort: Save the Island! 의 알려진 내용', () => {
    const fort = parseFort(archive.read('\\d\\savetheisland.fort'), types);
    assert.equal(fort.subscriber.name, 'Fred');
    assert.equal(fort.chaff.chunks.length, 256, 'Chaff 는 16 × 16 청크');
    assert.equal(fort.chaff.version, 2);
    assert.equal(fort.flag, 0);
    assert.equal(fort.territory.length, 120);
    // 플레이어(소유자 1)는 Terr07 의 청크 4개, 적은 Terr00 의 청크 7개이다(영역 번호와 플레이어 번호는 무관).
    assert.equal(fort.territories[7].chunks.length, 4);
    assert.equal(fort.territories[0].chunks.length, 7);
    // 덱의 첫 항목은 sunArcher(chance 4, power 1, 남은 횟수 255) 이고 기술 첫 항목도 sunArcher 이다.
    assert.equal(typeLabel(fort.deck[0].type), 'sunArcher');
    assert.deepEqual([fort.deck[0].chance, fort.deck[0].power, fort.deck[0].remaining], [4, 1, 255]);
    assert.equal(typeLabel(fort.technology.items[0].type), 'sunArcher');
    assert.equal(fort.technology.items.length, 26);
    assert.equal(fort.technology.items[0].listFlags, 4);
    // Terr07 에 사제와 비 신전이 있다.
    const names = fort.territories[7].chunks.flatMap((chunk) => chunk.objects.map((object) => typeLabel(object.type)));
    assert.ok(names.includes('priest'));
    assert.ok(names.includes('rainVortex'));
    // 간헐천도 소유자 바이트를 저장하고(값 0), 중립 거주 구역도 소유자 0 이다. 간헐천은 월드에 놓은 뒤 소유자를 0 으로 만든다(ownerClearedMask, 이 판독기의 일이 아님).
    const geyser = fort.chaff.chunks.flatMap((chunk) => chunk.objects).find((object) => typeLabel(object.type) === 'geyser');
    assert.equal(geyser.owner, 0);
    const residence = fort.territories[7].chunks.flatMap((chunk) => chunk.objects).find((object) => typeLabel(object.type) === 'residence');
    assert.equal(residence.owner, 0);
    assert.equal(normalizedOwner(residence), 1, '저장된 소유자 0 은 1 로 바꿔 쓴다');
});

// ----- 합성 입력 -----

// 타입 레코드 일부를 바꾼 판독용 타입 정보를 만든다(원본 10.78 에 없는 저장 형식 saveQB 등을 합성으로 검사하는 데 쓴다). overrides 는 타입 번호 → 바꿀 필드이다. (2026-11-01 WB03-U)
function customTypes(overrides) {
    const copy = records.map((record) => ({ ...record, ...(overrides[record.number] ?? {}) }));
    return { records: copy, bridgeType: types.bridgeType, islandType: types.islandType };
}

// `TypeNames` 섹션 바이트를 만든다: 개수 + 이름 해시(u32)들. 이름은 현재 타입 레코드의 이름이다. (2026-11-01 WB03-U)
function typeNamesSection(names) {
    const bytes = [names.length];
    // 이름마다 해시를 리틀 엔디언 4바이트로 적는다.
    for (const name of names) {
        const hash = nameHash(name);
        bytes.push(hash & 255, (hash >>> 8) & 255, (hash >>> 16) & 255, hash >>> 24);
    }
    return bytes;
}

// 오브젝트 선택 필드: saveFrame·saveQA·다리·작업장·소유자·내용물을 타입 플래그에 따라 정해진 순서로 읽는다. 위치 바이트는 상위 4비트 x, 하위 4비트 y 이다. (2026-11-01 WB03-U)
test('parseFort: 오브젝트 레코드의 선택 필드와 읽는 순서', () => {
    const rainCannon = typeNumber('rainCannon');
    const priest = typeNumber('priest');
    const buried = typeNumber('buried');
    const factory = typeNumber('sunFactory');
    const bridge = types.bridgeType;
    // 포대: [위치][타입][프레임][소유자]. 사제: [위치][타입][수량 QA][소유자][내용물 개수]. 묻힌 것: [위치][타입][수량 QA](소유자 없음). 다리: [위치][타입][다리 번호].
    // 작업장: [위치][타입][작업장 상태][소유자][내용물 개수].
    const fort = parseFort(buildFort(sectionsWith({
        Chaff: chunkSection(2, [[
            [0x35, rainCannon, 12, 4], [0x71, priest, 9, 2, 0], [0xa0, buried, 77], [0x0f, bridge, 41], [0xf0, factory, 7, 1, 0],
        ]]),
    })), types);
    const [cannon, priestObject, buriedObject, bridgeObject, factoryObject] = fort.chaff.chunks[0].objects;
    assert.deepEqual([cannon.cellX, cannon.cellY, cannon.type, cannon.frame, cannon.owner], [3, 5, rainCannon, 12, 4]);
    assert.deepEqual([priestObject.cellX, priestObject.cellY, priestObject.quantityByte, priestObject.owner], [7, 1, 9, 2]);
    assert.deepEqual(priestObject.contents, { items: [], complete: true });
    assert.deepEqual([buriedObject.quantityByte, buriedObject.owner, buriedObject.contents], [77, null, null]);
    assert.deepEqual([bridgeObject.cellX, bridgeObject.cellY, bridgeObject.bridgeFrame, bridgeObject.owner, bridgeObject.shorthandOwner], [0, 15, 41, null, null]);
    assert.deepEqual([factoryObject.factoryState, factoryObject.owner], [7, 1]);
    assert.equal(fort.chaff.chunks[0].objects.length, 5);
    // 버전 1 은 작업장 상태 바이트가 없다.
    const v1 = parseFort(buildFort(sectionsWith({ Chaff: chunkSection(1, [[[0x71, factory, 1, 0]]]) })), types).chaff.chunks[0].objects[0];
    assert.deepEqual([v1.factoryState, v1.owner], [null, 1]);
    // 버전 0 은 모든 타입 뒤에 상태 1바이트가 있고 소유자 바이트는 없다(원본 004ad0a0). 다리: [다리 번호][상태].
    const v0 = parseFort(buildFort(sectionsWith({ Chaff: chunkSection(0, [[[0x35, bridge, 41, 9]]]) })), types).chaff.chunks[0].objects[0];
    assert.deepEqual([v0.bridgeFrame, v0.legacyState, v0.owner], [41, 9, null]);
    // saveQB 는 부호 있는 16비트이다(10.78 자산에는 없는 형식이라 타입 레코드를 바꿔 검사한다). 순서는 프레임 → QA → QB → 다리 → 작업장 → 소유자이다.
    const qbTypes = customTypes({ [buried]: { flags1: TypeFlag1.saveQB | TypeFlag1.saveFrame } });
    const qb = parseFort(buildFort(sectionsWith({ Chaff: chunkSection(2, [[[0x00, buried, 5, 0xfe, 0xff]]]) })), qbTypes).chaff.chunks[0].objects[0];
    assert.deepEqual([qb.frame, qb.quantityWord, qb.quantityByte], [5, -2, null]);
});

// 다리 약식 표기(0xf6 이상): 타입은 다리, 소유자는 0xff - 타입 바이트이다. 변환표는 적용하지 않는다. 섬 타입은 뒤의 1바이트만 건너뛰고, 타입 0 레코드는 0 바이트로 끝난다. (2026-11-01 WB03-U)
test('parseFort: 다리 약식 표기와 섬 건너뛰기, 빈 레코드', () => {
    const bridge = types.bridgeType;
    const island = types.islandType;
    const fort = parseFort(buildFort(sectionsWith({
        Chaff: chunkSection(2, [[[0x00, 0xf8, 35], [0x11, island, 0xaa], [0x22, 0, 0]]]),
    })), types);
    const [short, skip, empty] = fort.chaff.chunks[0].objects;
    assert.equal(short.type, bridge);
    assert.equal(short.shorthandOwner, 7);
    assert.equal(short.bridgeFrame, 35);
    assert.equal(short.owner, null);
    assert.equal(skip.type, island);
    assert.equal(skip.islandByte, 0xaa);
    assert.equal(empty.type, 0, 'TypeNames 가 없으면 변환하지 않아 타입 바이트 0 은 0 이다');
    // 약식 표기는 변환표를 거치지 않는다(TypeNames 가 있어도 소유자·타입이 그대로).
    const converted = parseFort(buildFort(sectionsWith({ TypeNames: typeNamesSection(['sunArcher']), Chaff: chunkSection(2, [[[0x00, 0xff, 3]]]) })), types);
    assert.equal(converted.chaff.chunks[0].objects[0].type, bridge);
    assert.equal(converted.chaff.chunks[0].objects[0].shorthandOwner, 0);
});

// 타입 번호 변환: `TypeNames` 가 있으면 파일 번호를 해시로 현재 번호로 바꾸고 없는 해시는 0 이다. 약식 표기 번호(0xf6~)와 변환표가 없는 파일은 그대로이다. (2026-11-01 WB03-U)
test('convertType: TypeNames 변환', () => {
    const sunArcher = typeNumber('sunArcher');
    const none = parseFort(buildFort(sectionsWith({})), types);
    assert.equal(convertType(none, 90), 90);
    const fort = parseFort(buildFort(sectionsWith({ TypeNames: typeNamesSection(['bridge', 'sunArcher', 'noSuchType']) })), types);
    assert.equal(fort.hasTypeConversion, true);
    assert.deepEqual([0, 1, 2, 3, 0xf6, 0xff].map((stored) => convertType(fort, stored)), [types.bridgeType, sunArcher, 0, 0, 0xf6, 0xff]);
    assert.equal(fort.typeHashes.length, 3);
});

// 내용물 목록: 목록 플래그를 타입 → 그릇 → 파일 1바이트 순서로 정하고, 변환한 타입이 0 이면 거기서 끊기며 중첩이 끊기면 바깥도 끊긴다. (2026-11-01 WB03-U)
test('parseFort: 내용물 목록의 목록 플래그와 끊김', () => {
    const vortex = typeNumber('rainVortex');
    const walker = typeNumber('sunWalker');
    const bomb = typeNumber('bombHeal');
    const sunArcher = typeNumber('sunArcher');
    // 신전 안에 걷는 유닛 하나(신전의 그릇 목록 플래그 0x10 이라 파일 바이트 없음), 걷는 유닛 안에는 내용물 0개.
    const fort = parseFort(buildFort(sectionsWith({ Chaff: chunkSection(2, [[[0x00, vortex, 3, 1, walker, 0]]]) })), types);
    const object = fort.chaff.chunks[0].objects[0];
    assert.equal(object.owner, 3);
    assert.equal(object.contents.items.length, 1);
    assert.equal(object.contents.items[0].type, walker);
    assert.equal(object.contents.items[0].listFlags, 0x10);
    assert.equal(object.contents.items[0].listFlagsStored, false);
    assert.deepEqual(object.contents.items[0].contents, []);
    // Technology(그릇 PlayerSquid, 목록 플래그 0): 항목마다 목록 플래그 1바이트. 폭탄은 자기 플래그(1)가 있어 바이트가 없다.
    const technology = parseFort(buildFort(sectionsWith({ Technology: [2, sunArcher, 4, bomb] })), types);
    assert.equal(technology.technology.items.length, 2);
    assert.deepEqual([technology.technology.items[0].listFlags, technology.technology.items[0].listFlagsStored], [4, true]);
    assert.deepEqual([technology.technology.items[1].listFlags, technology.technology.items[1].listFlagsStored], [1, false]);
    assert.equal(technology.technology.complete, true);
    assert.equal(technology.technologyTrailingBytes, 0);
    // 변환한 타입이 0 이면 거기서 끊기고 complete 가 거짓이다(TypeNames 가 있는 파일에서 모르는 타입 번호 2). 앞 항목은 남는다.
    const names = typeNamesSection(['sunArcher', 'bridge']);
    const cut = parseFort(buildFort(sectionsWith({ TypeNames: names, Technology: [3, 0, 4, 2] })), types);
    assert.equal(cut.technology.items.length, 1);
    assert.equal(cut.technology.complete, false);
    // 중첩 내용물이 끊기면 바깥 목록도 끊긴다: 신전(저장 0) 안 걷는 유닛(저장 1) 안에 모르는 타입(9).
    const nested = parseFort(buildFort(sectionsWith({
        TypeNames: typeNamesSection(['rainVortex', 'sunWalker']), Chaff: chunkSection(2, [[[0x00, 0, 3, 1, 1, 1, 9]]]),
    })), types).chaff.chunks[0].objects[0];
    assert.equal(nested.contents.complete, false);
    assert.equal(nested.contents.items.length, 1);
    assert.equal(nested.contents.items[0].contentsComplete, false);
    assert.equal(nested.contents.items[0].contents.length, 0);
});

// 소유자 정규화: 저장된 소유자 0 이거나 8 보다 크면 1 로 바꿔 쓰고(원본 004bdc60), 1~8 은 그대로이며, 소유자 바이트가 없으면 null 이다. (2026-11-01 WB03-U)
test('normalizedOwner: 0 또는 8 초과는 1', () => {
    const owner = (value) => normalizedOwner({ owner: value });
    assert.deepEqual([0, 1, 2, 8, 9, 255].map(owner), [1, 1, 2, 8, 1, 1]);
    assert.equal(normalizedOwner({ owner: null }), null);
});

// 내용물의 수량: saveQA 는 1바이트, saveQB 는 부호 있는 16비트이다(10.78 자산에는 saveQB 가 없어 타입 레코드를 바꿔 검사한다). (2026-11-01 WB03-U)
test('parseFort: 내용물 항목의 수량 필드', () => {
    const sunArcher = typeNumber('sunArcher');
    const buried = typeNumber('buried');
    // 묻힌 것(saveQA)은 1바이트이다. 목록 플래그는 항목마다 파일에서 읽는다(Technology): [개수][타입][수량 QA][목록 플래그].
    const qa = parseFort(buildFort(sectionsWith({ Technology: [1, buried, 200, 4] })), types).technology.items[0];
    assert.deepEqual([qa.quantity, qa.listFlags], [200, 4]);
    // saveQB 로 바꾼 타입은 부호 있는 16비트이다: [개수][타입][QB 낮은][QB 높은][목록 플래그].
    const qbTypes = customTypes({ [sunArcher]: { flags1: TypeFlag1.saveQB } });
    const qb = parseFort(buildFort(sectionsWith({ Technology: [1, sunArcher, 0xfe, 0xff, 4] })), qbTypes).technology.items[0];
    assert.deepEqual([qb.quantity, qb.listFlags], [-2, 4]);
});

// Money·Deck·Subscriber: Deck 의 power 는 부호 있는 8비트이고 구독자 이름은 원본 바이트(Windows-1252)이다. (2026-11-01 WB03-U)
test('parseFort: Money·Deck·Subscriber', () => {
    const money = new Uint8Array(4);
    new DataView(money.buffer).setFloat32(0, 1234.5, true);
    const sunArcher = typeNumber('sunArcher');
    const name = [...Buffer.from('Fr\xe9d', 'latin1')];
    const fort = parseFort(buildFort(sectionsWith({
        Subscriber: [1, 2, 3, 4, name.length, 0, ...name],
        Money: [...money],
        Deck: [2, sunArcher, 4, 1, 255, sunArcher, 3, 0xff, 7],
    })), types);
    assert.equal(fort.money, 1234.5);
    assert.equal(fort.subscriber.id, 0x04030201);
    assert.equal(fort.subscriber.name, 'Fréd');
    assert.deepEqual(fort.deck.map((entry) => [entry.type, entry.chance, entry.power, entry.remaining]), [[sunArcher, 4, 1, 255], [sunArcher, 3, -1, 7]]);
    // 섹션이 없거나 짧으면 값이 없다.
    const empty = parseFort(buildFort(sectionsWith({ Money: [1, 2] })), types);
    assert.equal(empty.money, null);
    assert.equal(empty.subscriber, null);
    assert.equal(empty.territory, null);
    assert.equal(empty.deck.length, 0);
});

// 섹션 나누기: 35개까지만 읽고 뒤는 꼬리로 센다. 길이가 맞지 않으면 거기서 멈춘다. 매직이 다르거나 너무 짧으면 오류이다. (2026-11-01 WB03-U)
test('parseFort: 섹션 나누기·꼬리·오류', () => {
    const sections = sectionsWith({ Money: [0, 0, 0, 0] });
    // 36번째 섹션은 읽지 않고 꼬리 바이트로 센다(길이 필드 2바이트 + 내용 3바이트).
    const extra = parseFort(buildFort([...sections, [9, 9, 9]]), types);
    assert.equal(extra.sections.length, 35);
    assert.equal(extra.trailingBytes, 5);
    // 마지막 섹션의 길이가 파일 밖이면 그 섹션부터 읽지 않는다.
    const good = buildFort(sections);
    const truncated = parseFort(good.slice(0, good.length - 1), types);
    assert.equal(truncated.sections.length, 34);
    assert.equal(truncated.trailingBytes, 1);
    // 플래그 바이트는 보존한다.
    assert.equal(parseFort(buildFort(sections, { flag: 0xfe }), types).flag, 0xfe);
    assert.throws(() => parseFort(Uint8Array.of(0x47, 0), types), /0x46/);
    assert.throws(() => parseFort(Uint8Array.of(0x46), types), /0x46/);
    // 청크 표지가 99 가 아니면 오류, 레코드가 섹션 밖으로 나가도 오류이다.
    assert.throws(() => parseFort(buildFort(sectionsWith({ Chaff: [2, 100, 0, 0] })), types), /99/);
    assert.throws(() => parseFort(buildFort(sectionsWith({ Chaff: [2, 99, 1, 0, 0x11] })), types), /pos<=end/);
    // 타입 번호가 타입 표 밖이면 오류이다(변환표가 없을 때 파일의 번호를 그대로 쓰므로 200 은 표 밖).
    assert.throws(() => parseFort(buildFort(sectionsWith({ Chaff: chunkSection(2, [[[0x00, 200, 0]]]) })), types), /type table/);
    // 타입 0 레코드 뒤의 바이트가 0 이 아니면 오류이다.
    assert.throws(() => parseFort(buildFort(sectionsWith({ Chaff: chunkSection(2, [[[0x00, 0, 1]]]) })), types), /num == 0/);
    assert.equal(TERRITORY_COUNT, 20);
    assert.equal(SECTION_NAMES.length, 35);
    assert.equal(fortSection(extra, 'money').length, 4, '섹션 이름은 대소문자를 구분하지 않는다');
});
