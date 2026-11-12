// Template9 저장소의 로그인 송신 이미지(campaign-template-storage.js 3부: readLoginImage·cloneSectionFrom·saveImage)를 실제 보호 요새 파일과 cpppj 시험 2건, 웹 계약으로 확인한다.
//  - cpppj 시험 이식: CampaignTemplateLoginImage_ActualFileStripsFiveSectionsAndKeepsGlobalOwner(실제 `thewarbegins.fort` 의 네 고정 섹션과 현재 Terr 하나만 남기고 나머지는 빈 payload 인 이미지, 전역 owner/판본 전역/이름표 할당/State cursor 보존)와
//    CampaignTemplateLoginImage_FalsePartialAndDeviceExceptionsPreserveGlobalOwner(정상 false·장치 예외·부분 읽기 예외·현재 Terr 누락 뒤 전역 owner 보존과 다음 정상 읽기).
//  - 제외 확인: 원문 level29 덤프 `Stripped Image:`(exclude-host)와 `SaveFile`(exclude-edit)은 옮기지 않았다. 덤프는 임시 NONAME 의 current 검색뿐이고 그 manager 는 곧 반납되어 이미지 결과에 영향이 없다.
//  - 웹 계약: 현재 Terr 이름 서식(두 자리·signed), strip 이미지 서명(NONAME 은 기본 0 서명이라 source 의 FE 서명을 복제하지 않음), 이미지 끝 길이, 지역 Template 의 readonly/현재 슬롯/제목 조회 금지.
// 입력은 아카이브의 실제 보호 요새이며 게임/통신은 실행하지 않는다. (2026-11-13 WB04-E347)
import test from 'node:test';
import assert from 'node:assert/strict';
import { readData } from './helpers.mjs';
import { TaffArchive } from '../www/assets/taff.js';
import { SECTION_NAMES } from '../www/assets/fort.js';
import { FortSerializationGlobals } from '../www/core/fort-serialization-globals.js';
import { ClientTemplateSectionNames } from '../www/core/client-template-section-names.js';
import { CampaignTemplateStorage } from '../www/core/campaign-template-storage.js';

// 한 번만 읽는 실제 보호 요새 원문이다. (2026-11-13 WB04-E347)
let cachedFort = null;
function warBegins() {
    cachedFort ??= new TaffArchive(readData('netstorm.tarc')).read('\\d\\thewarbegins.fort');
    return Uint8Array.from(cachedFort);
}

// 파일의 35개 길이 WORD 와 payload 를 독립 해석한다(제품 직렬화기를 쓰지 않음). (2026-11-13 WB04-E347)
function loginImageSections(bytes) {
    assert.ok(bytes.length >= 2);
    const result = [];
    let position = 2;
    // 원본 고정 35개 이름에 대응하는 길이와 payload 를 파일 순서대로 읽는다.
    for (let index = 0; index < 35; index += 1) {
        assert.ok(position <= bytes.length && bytes.length - position >= 2);
        const length = bytes[position] + (bytes[position + 1] << 8);
        assert.ok(length >= 2 && length <= bytes.length - position);
        result.push(Array.from(bytes.subarray(position + 2, position + length)));
        position += length;
    }
    return result;
}
// 선택한 파일 이미지만 공급한다. 지역 Template 는 권한/현재 전역/제목을 조회하면 실패하도록 지정한다. (2026-11-13 WB04-E347)
function loginImageFile(bytes) {
    return {
        read: () => Uint8Array.from(bytes),
        typeCount: () => 188,
        readOnly: () => { throw new Error('지역readonlyTemplate의파일권한조회금지'); },
        currentSlot: () => { throw new Error('지역Template의전역현재슬롯조회금지'); },
        refreshTitle: () => { throw new Error('지역Template의전역제목갱신금지'); },
        tooManyTypes: () => { throw new Error('검사입력에없는TYPE진단'); },
        rejectFile: () => { throw new Error('검사입력에없는파일제거요청'); },
    };
}
// 명시 예외 관측은 다른 진단을 성공으로 바꾸지 않고 메시지를 그대로 돌려준다. (2026-11-13 WB04-E347)
function failureOf(action) {
    try { action(); } catch (error) { return error.message; }
    return '';
}

test('cpppj CampaignTemplateLoginImage_ActualFileStripsFiveSectionsAndKeepsGlobalOwner: 고정 네 이름과 현재 Terr 하나만 남기고 전역 owner 를 보존한다', () => {
    const bytes = warBegins();
    const original = loginImageSections(bytes);
    bytes[1] = 0xfe; // source 의 FE 서명이 NONAME 의 기본 0 서명으로 복제되지 않는 별도 입력이다.
    const globals = new FortSerializationGlobals();
    let nameAllocations = 0;
    const names = new ClientTemplateSectionNames((size) => { nameAllocations += 1; return new Uint8Array(size).fill(0xa5); });
    const storage = new CampaignTemplateStorage(globals, names);
    storage.bindDiagnosticLog(() => {});
    storage.installFile(1, {}, bytes, 'global-kept.fort');
    storage.selectState(1).position = 5;
    const allocation = storage.state(1).bytes;
    const headers = Uint8Array.from(storage.headers());
    const namePointer = names.names();
    assert.equal(nameAllocations, 1);
    assert.ok(original[4].length > 0); // 원본 Chaff 의 비영 payload 도 strip 후 사라지는지 확인한다.
    // 서로 다른 현재 영역은 각각 자기 Terr 만 보유하며 이전 호출의 임시 manager 를 재사용하지 않는다.
    for (const territory of [0, 7, 19]) {
        let reads = 0;
        let typeReads = 0;
        const file = loginImageFile(bytes);
        globals.version = 123;
        file.read = (name) => {
            reads += 1;
            assert.equal(name, 'source.fort');
            assert.equal(globals.version, 0);
            assert.equal(storage.ownedImages(), 1);
            assert.equal(storage.currentSection(1), 'State');
            globals.version = 19;
            return Uint8Array.from(bytes);
        };
        file.typeCount = () => {
            typeReads += 1;
            if (typeReads === 1) { assert.equal(globals.version, 19); globals.version = 37; } else { assert.equal(typeReads, 2); assert.equal(globals.version, 0); globals.version = 53; }
            return 188;
        }; // source 생성 뒤 읽기와 NONAME 생성 뒤 저장의 같은 전역 0 쓰기를 구별한다.
        const image = storage.readLoginImage('source.fort', file, territory);
        const sections = loginImageSections(image.bytes());
        const selected = `Terr${String(territory).padStart(2, '0')}`;
        let expectedEnd = 72;
        assert.equal(image.storage.length, 32768);
        assert.ok(image.bytes()[0] === 0x46 && image.bytes()[1] === 0);
        assert.deepEqual([reads, typeReads, globals.version], [1, 2, 53]);
        // 원문 5개 이름의 독립 payload 와 빈 나머지를 모두 비교한다.
        for (let index = 0; index < SECTION_NAMES.length; index += 1) {
            const name = SECTION_NAMES[index];
            const keep = name === 'Subscriber' || name === 'Territory' || name === 'TypeNames' || name === 'CoreData' || name === selected;
            if (keep) { assert.deepEqual(sections[index], original[index], name); expectedEnd += original[index].length; } else assert.deepEqual(sections[index], [], name);
        }
        assert.equal(image.end, expectedEnd);
        assert.equal(storage.ownedImages(), 1);
        assert.equal(storage.name(1), 'global-kept.fort');
        assert.equal(storage.currentSection(1), 'State');
        assert.deepEqual(Array.from(storage.headers()), Array.from(headers));
        assert.equal(storage.state(1).position, 5);
        assert.ok(storage.state(1).bytes.buffer === allocation.buffer && storage.state(1).bytes.byteOffset === allocation.byteOffset);
        assert.equal(names.names(), namePointer);
        assert.equal(nameAllocations, 1);
    }
});

test('cpppj CampaignTemplateLoginImage_FalsePartialAndDeviceExceptionsPreserveGlobalOwner: 임시 실패 뒤 전역 owner/current 를 그대로 두고 다음 정상 읽기도 가능하다', () => {
    const bytes = warBegins();
    const storage = new CampaignTemplateStorage();
    storage.bindDiagnosticLog(() => {});
    storage.installFile(1, {}, bytes, 'kept.fort');
    storage.selectState(1).position = 3;
    const headers = Uint8Array.from(storage.headers());
    // 부재 정상 false / 장치 예외 / 일부 섹션 읽기 예외 / 현재 Terr 누락을 각각 독립 임시 수명으로 실행한다.
    for (let variant = 0; variant < 4; variant += 1) {
        const file = loginImageFile(bytes);
        const territory = variant === 3 ? 20 : 0;
        storage.serializationVersion = 91;
        if (variant === 0) file.read = () => new Uint8Array(0);
        if (variant === 1) file.read = () => { throw new Error('선택파일장치중단'); };
        if (variant === 2) file.read = () => Uint8Array.of(0x46, 0xfe, 5, 0, 11, 13, 17, 8, 0); // 첫 Subscriber 노드는 붙고 둘째 길이 읽기가 물리 범위를 벗어난다.
        const failure = failureOf(() => storage.readLoginImage('temporary.fort', file, territory));
        assert.notEqual(failure, '', `variant ${variant}`);
        if (variant === 0) assert.equal(failure, '로그인송신의지역Template읽기실패진단');
        if (variant === 1) assert.equal(failure, '선택파일장치중단');
        if (variant === 3) assert.equal(failure, '로그인이미지복제의원문섹션누락진단');
        assert.deepEqual([storage.serializationVersion, storage.ownedImages(), storage.currentSection(1), storage.state(1).position], [0, 1, 'State', 3]);
        assert.deepEqual(Array.from(storage.headers()), Array.from(headers));
        const recovered = storage.readLoginImage('temporary.fort', loginImageFile(bytes), 0);
        assert.ok(recovered.end > 72);
        assert.equal(storage.ownedImages(), 1);
        assert.equal(storage.name(1), 'kept.fort');
    }
});

test('로그인 이미지의 현재 Terr 이름은 두 자리 서식이고 음수는 signed 로 쓰며 NONAME 은 기본 0 서명이다', () => {
    const bytes = warBegins();
    const storage = new CampaignTemplateStorage();
    storage.bindDiagnosticLog(() => {});
    // 영역 5 와 19 의 이미지는 서로 다른 Terr payload 를 가지며 같은 길이 레코드 순서를 쓴다.
    const five = loginImageSections(storage.readLoginImage('s.fort', loginImageFile(bytes), 5).bytes());
    const nineteen = loginImageSections(storage.readLoginImage('s.fort', loginImageFile(bytes), 19).bytes());
    const index5 = SECTION_NAMES.indexOf('Terr05');
    const index19 = SECTION_NAMES.indexOf('Terr19');
    assert.deepEqual(five[index5], loginImageSections(bytes)[index5]);
    assert.deepEqual(five[index19], []);
    assert.deepEqual(nineteen[index19], loginImageSections(bytes)[index19]);
    assert.deepEqual(nineteen[index5], []);
    // 음수 영역 번호는 `Terr-1` 이라는 없는 이름이라 원문 누락 진단이다(signed 서식).
    assert.equal(failureOf(() => storage.readLoginImage('s.fort', loginImageFile(bytes), -1)), '로그인이미지복제의원문섹션누락진단');
    assert.equal(failureOf(() => storage.readLoginImage('s.fort', loginImageFile(bytes), 0xffffffff)), '로그인이미지복제의원문섹션누락진단');
    // 첫 바이트는 'F' 이고 서명 판정은 NONAME 의 기본 0 이다(source 가 FE 여도).
    const fe = warBegins();
    fe[1] = 0xfe;
    const image = storage.readLoginImage('s.fort', loginImageFile(fe), 0).bytes();
    assert.ok(image[0] === 0x46 && image[1] === 0);
});

test('지역 Template 는 readonly 사전 설정으로 파일 권한을 조회하지 않고 전역 슬롯/제목에 닿지 않으며 임시 manager 를 모두 반납한다', () => {
    const storage = new CampaignTemplateStorage();
    storage.bindDiagnosticLog(() => {});
    const file = loginImageFile(warBegins());
    // loginImageFile 은 readOnly/currentSlot/refreshTitle 을 호출하면 던지므로 정상 반환이 곧 비조회의 증거이다.
    const image = storage.readLoginImage('s.fort', file, 3);
    assert.ok(image.end > 72);
    assert.equal(storage.ownedImages(), 0);
    assert.ok(storage.headers().every((byte) => byte === 0));
    // typeCount 는 DWORD 로 넘기고 연결이 없으면 도달한 저장 위치에서 거부한다.
    const noTypes = loginImageFile(warBegins());
    delete noTypes.typeCount;
    assert.throws(() => storage.readLoginImage('s.fort', noTypes, 3), /Template 실제 파일 Load 장치 미연결|typeCount/u);
    assert.equal(storage.ownedImages(), 0);
});
