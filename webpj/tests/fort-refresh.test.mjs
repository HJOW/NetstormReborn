// Template::RefreshObjectRecords(cpppj `o/Template.cpp`)의 웹 대응 `assets/fort.js` 의 `refreshFortObjectRecords` 를 검사한다(4-B, WB04-E280).
//  - 보호 원본 Fort 465개(느슨한 433 + 아카이브 32): 읽은 그대로 다시 해석하면 처음 읽은 결과와 완전히 같다(멱등).
//  - 원시 섹션을 다른 Fort 의 `TypeNames`/`Chaff`/`Terr00` 로 바꾼 사본: 변환표·chaff·영역이 바뀐 섹션을 재직렬화해 처음부터 읽은 결과와 같고,
//    technology·deck·subscriber·money·territory·sections 는 다시 읽지 않는다(같은 객체 그대로).
//  - 빈 `TypeNames`: 이전 해시/변환이 새 빈 섹션으로 넘어가지 않고 같은 배열이 제자리에서 비워진다.
//  - 손상된 영역 섹션: 앞선 chaff·영역은 이미 바뀌고 뒤 영역은 이전 값 그대로인 부분 갱신(원본/cpppj 와 같은 순서).
// cpppj 에는 이 함수의 전용 시험이 없다(소비자는 Editor 와 CampaignSession 시험의 준비 입력 도우미뿐). 위는 호출 계약을 직접 검사한 것이다. 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E280)
import test from 'node:test';
import assert from 'node:assert/strict';
import { readData, loadOriginalForts } from './helpers.mjs';
import { ARCHIVE_PATH } from '../www/assets/game-data.js';
import { TaffArchive } from '../www/assets/taff.js';
import { decodeOriginalText } from '../www/assets/text-encoding.js';
import { TypeTable } from '../www/assets/type-table.js';
import { createFortTypes } from '../www/assets/type-flags.js';
import { SECTION_NAMES, TERRITORY_COUNT, fortSection, parseFort, refreshFortObjectRecords } from '../www/assets/fort.js';

// 보호 자료를 읽는 아카이브와 타입 표, 원본 Fort 목록이다(목록이 없으면 해당 검사를 건너뛴다). (2026-11-11 WB04-E280)
const archive = new TaffArchive(readData(ARCHIVE_PATH));
const types = createFortTypes(TypeTable.fromArchive(archive, decodeOriginalText));
const forts = loadOriginalForts(archive);

// 섹션 이름의 색인이다. (2026-11-11 WB04-E280)
const INDEX = Object.fromEntries(SECTION_NAMES.map((name, index) => [name, index]));

// 읽은 섹션을 다시 .fort 바이트로 묶는다(매직 + 플래그 + 길이 접두 섹션). (2026-11-11 WB04-E280)
function encode(fort) {
    const parts = [Uint8Array.of(0x46, fort.flag)];
    // 섹션마다 길이 WORD(자기 2바이트 포함)가 앞선다.
    for (const section of fort.sections) {
        parts.push(Uint8Array.of((section.length + 2) & 255, ((section.length + 2) >> 8) & 255), section);
    }
    const bytes = new Uint8Array(parts.reduce((sum, part) => sum + part.length, 0));
    let at = 0;
    for (const part of parts) {
        bytes.set(part, at);
        at += part.length;
    }
    return bytes;
}

// 새로 읽는 쪽과 제자리 갱신이 같아야 하는 필드 묶음이다. (2026-11-11 WB04-E280)
function refreshed(fort) {
    return { typeHashes: fort.typeHashes, hasTypeConversion: fort.hasTypeConversion, typeConversion: fort.typeConversion, chaff: fort.chaff, territories: fort.territories };
}

// 복사본(원시 섹션 배열은 얕게 복제해 섹션 교체가 원본에 번지지 않게 한다). (2026-11-11 WB04-E280)
function copyOf(fort) {
    return { ...fort, sections: fort.sections.slice(), typeHashes: fort.typeHashes.slice(), typeConversion: fort.typeConversion.slice(), territories: fort.territories.slice() };
}

// 보호 Fort 465개를 읽은 그대로 다시 해석하면 처음 결과와 같다(멱등). (2026-11-11 WB04-E280)
test('refreshFortObjectRecords: 원본 Fort 465개를 다시 해석해도 처음 읽은 결과와 완전히 같다', { skip: forts ? false : 'assets/game-data 없음' }, () => {
    assert.equal(forts.length, 465);
    // 각 Fort 를 읽고 복사본을 다시 해석해 처음 결과와 비교한다.
    for (const { source, bytes } of forts) {
        const original = parseFort(bytes, types);
        const again = copyOf(original);
        refreshFortObjectRecords(again, types);
        assert.deepStrictEqual(again, original, source);
    }
});

// 다른 Fort 의 원시 섹션으로 바꾼 사본: 변환표·chaff·영역이 재직렬화해 처음부터 읽은 결과와 같고 나머지 해석 필드는 다시 읽지 않는다. (2026-11-11 WB04-E280)
test('refreshFortObjectRecords: 원시 TypeNames/Chaff/Terr 를 바꾸면 처음부터 읽은 것과 같은 변환표·객체 자료이고 나머지 필드는 그대로다', { skip: forts ? false : 'assets/game-data 없음' }, () => {
    // 변환표와 chaff, 첫 영역 섹션이 모두 있는 서로 다른 Fort 두 개를 고른다.
    const rich = forts.map(({ source, bytes }) => ({ source, fort: parseFort(bytes, types) }))
        .filter(({ fort }) => fort.hasTypeConversion && fort.chaff.chunks.length > 0 && fort.sections.length === SECTION_NAMES.length);
    assert.ok(rich.length >= 3, `후보 Fort ${rich.length}개`);
    let changed = 0;
    // 서로 다른 짝을 여러 개 시험한다.
    for (let pair = 0; pair + 1 < Math.min(rich.length, 12); pair += 1) {
        const target = copyOf(rich[pair].fort);
        const donor = rich[pair + 1].fort;
        const keep = { technology: target.technology, deck: target.deck, subscriber: target.subscriber, money: target.money, territory: target.territory, sections: target.sections };
        for (const name of ['TypeNames', 'Chaff', 'Terr00']) target.sections[INDEX[name]] = donor.sections[INDEX[name]];
        refreshFortObjectRecords(target, types);
        const expected = parseFort(encode(target), types);
        assert.deepStrictEqual(refreshed(target), refreshed(expected), `${rich[pair].source} <- ${rich[pair + 1].source}`);
        // 다시 읽지 않는 필드는 같은 객체 그대로이다(원시 섹션은 호출자가 바꾼 배열 자체).
        for (const [name, value] of Object.entries(keep)) assert.strictEqual(target[name], value, name);
        if (JSON.stringify(refreshed(target)) !== JSON.stringify(refreshed(rich[pair].fort))) changed += 1;
    }
    assert.ok(changed > 0, '섹션 교체가 결과를 바꾼 짝이 없음');
});

// 빈 TypeNames: 이전 파일의 해시/변환이 새 빈 섹션으로 넘어가지 않고 같은 배열이 제자리에서 비워진다. (2026-11-11 WB04-E280)
test('refreshFortObjectRecords: 빈 TypeNames 는 이전 해시/변환을 비우고 같은 배열을 제자리에서 갱신한다', { skip: forts ? false : 'assets/game-data 없음' }, () => {
    const source = forts.map(({ bytes }) => parseFort(bytes, types)).find((fort) => fort.hasTypeConversion && fort.typeHashes.length > 0 && fort.typeConversion.some((value) => value !== 0));
    assert.ok(source, 'TypeNames 가 있는 Fort');
    const fort = copyOf(source);
    const { typeHashes, typeConversion } = fort;
    fort.sections[INDEX.TypeNames] = new Uint8Array(0);
    refreshFortObjectRecords(fort, types);
    assert.equal(fort.hasTypeConversion, false);
    assert.strictEqual(fort.typeHashes, typeHashes);
    assert.strictEqual(fort.typeConversion, typeConversion);
    assert.equal(typeHashes.length, 0);
    assert.ok(typeConversion.every((value) => value === 0));
    // 변환표가 없으면 파일의 번호를 그대로 쓰므로 새로 읽은 결과와 같다.
    assert.deepStrictEqual(refreshed(fort), refreshed(parseFort(encode(fort), types)));
});

// 손상된 영역 섹션: chaff 와 앞선 영역은 이미 바뀌고 뒤 영역은 이전 값 그대로인 부분 갱신이다. (2026-11-11 WB04-E280)
test('refreshFortObjectRecords: 손상된 영역 섹션은 앞선 chaff·영역만 갱신한 채 오류를 던지고 뒤 영역은 이전 값이다', { skip: forts ? false : 'assets/game-data 없음' }, () => {
    // 영역 섹션 20개를 모두 가진 Fort 이면 충분하다(영역 내용이 비어 있어도 다시 읽으면 새 객체가 된다).
    const source = forts.map(({ bytes }) => parseFort(bytes, types)).find((fort) => fort.sections.length === SECTION_NAMES.length);
    assert.ok(source, '섹션 35개를 모두 가진 Fort');
    const fort = copyOf(source);
    const before = fort.territories.slice();
    const oldChaff = fort.chaff;
    fort.sections[INDEX.Chaff] = Uint8Array.of(2, 99, 0, 0); // 청크 하나(빈 목록)만 있는 새 chaff
    fort.sections[INDEX.Terr05] = Uint8Array.of(2, 0); // 청크 표지(99)가 아닌 바이트라 형식 오류
    assert.throws(() => refreshFortObjectRecords(fort, types), /99/);
    assert.notStrictEqual(fort.chaff, oldChaff);
    assert.deepStrictEqual(fort.chaff, { present: true, version: 2, chunks: [{ objects: [] }] });
    // 앞선 영역 0..4 는 새로 읽은 객체(이전 객체와 다른 참조)이고 5 이후는 이전 객체 그대로이다.
    for (let index = 0; index < TERRITORY_COUNT; index += 1) {
        if (index < 5) assert.notStrictEqual(fort.territories[index], before[index], `영역 ${index}`);
        else assert.strictEqual(fort.territories[index], before[index], `영역 ${index}`);
    }
});

// 새 변환표를 먼저 만든 뒤 그 표로 객체를 읽는다: 두 저장 번호의 해시를 바꾸면 같은 저장 번호의 객체가 바뀐 현재 타입으로 읽힌다(변환표 갱신이 객체 해석보다 앞선다). (2026-11-11 WB04-E280)
test('refreshFortObjectRecords: 갱신한 변환표가 객체 해석에 쓰인다(두 저장 번호의 해시 교환)', { skip: forts ? false : 'assets/game-data 없음' }, () => {
    // 변환표가 있고 chaff 에 약식이 아닌 저장 번호의 객체가 있는 Fort 를 고른다.
    let chosen = null;
    for (const { bytes } of forts) {
        const fort = parseFort(bytes, types);
        if (!fort.hasTypeConversion || fort.typeHashes.length < 3) continue;
        const object = fort.chaff.chunks.flatMap((chunk) => chunk.objects).find((candidate) => candidate.type !== 0 && candidate.type !== types.islandType && candidate.storedType < 0xf6 && candidate.storedType < fort.typeHashes.length);
        if (!object) continue;
        // 바꿔도 뒤따르는 저장 BYTE 구성이 같도록 같은 저장 구성(flags1/flags2/목록 플래그)의 다른 현재 타입만 짝으로 삼는다.
        const layout = (type) => `${types.records[type].flags1}:${types.records[type].flags2}:${types.records[type].containerListFlags}`;
        const current = fort.typeConversion[object.storedType];
        const other = fort.typeHashes.findIndex((hash, index) => index !== object.storedType && hash !== fort.typeHashes[object.storedType]
            && fort.typeConversion[index] !== 0 && fort.typeConversion[index] !== current && fort.typeConversion[index] !== types.islandType && layout(fort.typeConversion[index]) === layout(current));
        if (other >= 0) {
            chosen = { fort, stored: object.storedType, other };
            break;
        }
    }
    assert.ok(chosen, '해시를 바꿀 수 있는 Fort');
    const { fort: source, stored, other } = chosen;
    const copy = copyOf(source);
    const names = copy.sections[INDEX.TypeNames].slice();
    // 개수 BYTE 뒤 해시 DWORD 두 개를 서로 바꾼다.
    for (let byte = 0; byte < 4; byte += 1) {
        const left = 1 + stored * 4 + byte;
        const right = 1 + other * 4 + byte;
        [names[left], names[right]] = [names[right], names[left]];
    }
    copy.sections[INDEX.TypeNames] = names;
    refreshFortObjectRecords(copy, types);
    // 바뀐 변환표는 저장 번호 둘의 현재 번호를 서로 바꿔 놓는다.
    assert.equal(copy.typeConversion[stored], source.typeConversion[other]);
    assert.equal(copy.typeConversion[other], source.typeConversion[stored]);
    // 처음부터 읽은 결과와 같고, 바뀐 변환표를 쓴 객체가 이전 읽기와 다르다.
    assert.deepStrictEqual(refreshed(copy), refreshed(parseFort(encode(copy), types)));
    assert.notDeepStrictEqual(copy.chaff, source.chaff);
});

// 섹션이 모자란 Fort(머리만 있는 파일)는 모든 chaff/영역이 빈 섹션이다. (2026-11-11 WB04-E280)
test('refreshFortObjectRecords: 섹션이 없는 Fort 는 빈 chaff/영역과 변환 없음이다', () => {
    const fort = parseFort(Uint8Array.of(0x46, 0), types);
    assert.equal(fort.sections.length, 0);
    refreshFortObjectRecords(fort, types);
    assert.deepStrictEqual(fort.chaff, { present: false, version: 0, chunks: [] });
    assert.equal(fort.territories.length, TERRITORY_COUNT);
    assert.ok(fort.territories.every((territory) => !territory.present && territory.chunks.length === 0));
    assert.equal(fort.hasTypeConversion, false);
    assert.equal(fortSection(fort, 'TypeNames').length, 0);
});
