// Template9 저장소의 파일 읽기와 원문 Load 부모(campaign-template-storage.js 2부: readFile·loadFile·installFile)를 cpppj 시험 9건과 웹 계약으로 확인한다.
//  - cpppj 시험 이식: CampaignTemplateLoadFileTests 의 기존 이름 유지·읽기 전 공개·미리 정해진 readonly·정상 false 정리·거부된 서명·예외 보존·부분 읽기·다른 owner 재조회·읽기 뒤 헤더 0. 첫 시험 뒤의 SaveFile 단언(저장 이미지 일치)은 3부(E347)에서 옮긴다.
//  - 웹 계약: installFile 의 서명 거부/타입 수 진단/잘린 이미지가 설치하지 않음, Native readWhole 경로(빈 파일·최소 4BYTE 할당·논리 길이 불일치 거부), 중복 Load 없음, 읽은 뒤 35개 표준 섹션 연결과 typed cursor.
//  하위 DataManager 읽기 부품은 각자 원본 관측 fixture(datamanager-read·live-read 등)로 이미 대조되었다. 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E346)
import test from 'node:test';
import assert from 'node:assert/strict';
import { SECTION_NAMES, fortSection } from '../www/assets/fort.js';
import { CampaignTemplateStorage } from '../www/core/campaign-template-storage.js';
import { compareDataManagerSectionNames } from '../www/core/raw-data-manager-selection.js';
import { bindDataManagerFileRead } from '../www/core/client-data-manager-file-read.js';

// 헤더의 DWORD 한 칸을 읽는다. (2026-11-13 WB04-E346)
function value(bytes, at = 0) {
    return (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
}
// 선택한 필드 이외의 stale 헤더를 보존하며 DWORD 한 칸만 쓴다. (2026-11-13 WB04-E346)
function field(bytes, at, next) {
    for (let index = 0; index < 4; index += 1) bytes[at + index] = (next >>> (index * 8)) & 255;
}
// 파일의 35개 길이 레코드만 구성한다(첫 두 섹션 외에는 길이 2 의 빈 섹션). 원본 출력 기대를 이 생성기에서 계산하지 않는다. (2026-11-13 WB04-E346)
function image(subscriber = [7, 0, 0, 0, 1, 0, 65], state = [3, 5, 7]) {
    const result = [0x46, 0xfe];
    // 섹션마다 WORD 길이(자기 2BYTE 포함)와 내용이 이어진다.
    for (let index = 0; index < SECTION_NAMES.length; index += 1) {
        const bytes = index === 0 ? subscriber : index === 1 ? state : [];
        const length = bytes.length + 2;
        result.push(length & 255, length >> 8, ...bytes);
    }
    return Uint8Array.from(result);
}
// 실제 호출한 하위 계약의 거부만 bool 로 읽는다. (2026-11-13 WB04-E346)
function rejects(action) {
    try { action(); } catch { return true; }
    return false;
}
// 명시 입력의 실제 하위 경계를 기본으로 연결하며 읽기 결과만 검사마다 바꾼다. (2026-11-13 WB04-E346)
function effects(bytes) {
    return {
        read: () => Uint8Array.from(bytes),
        sameName: (first, second) => compareDataManagerSectionNames(first, second) === 0, // 합성 공식 ASCII 파일명의 실제 비교 모듈을 재사용한다.
        wrongName: () => { throw new Error('검사에서 선택하지 않은 이름 불일치 진단'); },
        readOnly: () => false,
        typeCount: () => 188,
        tooManyTypes: () => { throw new Error('검사에서 선택하지 않은 TYPE 진단'); },
        rejectFile: () => { throw new Error('검사에서 선택하지 않은 파일 제거 요청'); },
        currentSlot: () => 1,
        refreshTitle: () => {},
    };
}
// stream 의 원문 위치와 실제 할당 위치를 같은 Storage 빌림에서 관측한다. (2026-11-13 WB04-E346)
function observe(storage, stream) {
    const result = { buffer: null, offset: -1, position: 0, end: 0 };
    storage.accessSection(stream, (fields, memory) => {
        result.position = value(fields, 4);
        result.end = value(fields, 8);
        if (value(fields)) {
            const view = memory.buffer(value(fields));
            result.buffer = view.buffer;
            result.offset = view.byteOffset;
        }
    });
    return result;
}
// 뷰가 관찰한 저장소와 같은지이다. (2026-11-13 WB04-E346)
const viewIs = (view, observed) => view.buffer === observed.buffer && view.byteOffset === observed.offset;

test('cpppj CampaignTemplateLoadFile_ExistingNamePreservesActualOwnerAndCursor: 기존 파일의 이름 비교는 State/current/노드 수명/헤더를 그대로 둔다', () => {
    const storage = new CampaignTemplateStorage();
    storage.installFile(1, {}, image(), 'Keep.fort');
    const state = storage.selectSection(1, 'State', 1);
    storage.state(1).position = 2;
    const observed = observe(storage, state);
    const header = storage.headers().subarray(20, 40);
    field(header, 8, 7);
    field(header, 12, 0xaabbccdd);
    field(header, 16, 42);
    const before = Uint8Array.from(header);
    let compares = 0;
    let diagnostics = 0;
    const hooks = {
        sameName: (first, second) => { compares += 1; assert.equal(first, 'Keep.fort'); return compareDataManagerSectionNames(first, second) === 0; },
        wrongName: () => { diagnostics += 1; },
    };
    storage.loadFile(1, {}, 'KEEP.FORT', hooks);
    storage.loadFile(1, {}, 'Other.fort', hooks);
    assert.deepEqual([compares, diagnostics, storage.ownedImages(), storage.currentSection(1)], [2, 1, 1, 'State']);
    assert.deepEqual(Array.from(header), Array.from(before));
    const after = observe(storage, state);
    assert.ok(after.buffer === observed.buffer && after.offset === observed.offset && after.position === 2);
    assert.ok(viewIs(fortSection(storage.template(1), 'State'), observed));
});

test('cpppj CampaignTemplateLoadFile_PublishesManagerBeforeReadAndKeepsActualBytes: 생성/버전 0/현재 header 공개 뒤 readonly, +16 초기화 뒤 read, 현재슬롯 재조회 뒤 Title 순서', () => {
    const storage = new CampaignTemplateStorage();
    storage.serializationVersion = 0x12345678;
    const header = storage.headers().subarray(20, 40);
    field(header, 4, 7);
    field(header, 12, 0x55667788);
    field(header, 16, 99);
    const hooks = effects(image());
    let trace = '';
    let manager = 0;
    let types = 256;
    hooks.readOnly = (name) => {
        trace += 'R';
        manager = value(header);
        assert.equal(name, 'fresh.fort');
        assert.ok(manager && !value(header, 4) && value(header, 12) === 0x55667788 && value(header, 16) === 99 && storage.ownedImages() === 1 && storage.serializationVersion === 0);
        return true;
    };
    hooks.read = (name) => {
        trace += 'F';
        assert.ok(name === 'fresh.fort' && value(header) === manager && value(header, 8) === 1 && !value(header, 16));
        types = 188;
        return image();
    };
    hooks.typeCount = () => { trace += 'Y'; return types; };
    hooks.currentSlot = () => { trace += 'I'; assert.equal(storage.sectionOrder(1).length, 35); return 1; };
    hooks.refreshTitle = () => { trace += 'T'; assert.ok(value(header) === manager && fortSection(storage.template(1), 'State')[1] === 5); };
    storage.loadFile(1, {}, 'fresh.fort', hooks);
    assert.equal(trace, 'RFYIT');
    assert.deepEqual(storage.fileHeader(1), [0x46, 1]);
    const state = storage.findSection(1, 'State');
    assert.ok(viewIs(fortSection(storage.template(1), 'State'), observe(storage, state)));
    assert.equal(storage.state(1).position, 0);
    assert.equal(storage.state(1).end, 3);
    // 읽은 파일의 35개 표준 섹션이 같은 manager 에 이름 순서로 연결되고 구독자 이름을 읽을 수 있다.
    assert.deepEqual(storage.sectionOrder(1), Array.from(SECTION_NAMES));
    assert.equal(storage.readSubscriberName(1), 'A');
});

test('cpppj CampaignTemplateLoadFile_PresetReadOnlyAndNonCurrentSelectedEffects: stale readonly 비영은 판정을 생략하고 1 로 쓰며 다른 현재슬롯에는 제목 hook 을 요구하지 않는다', () => {
    const storage = new CampaignTemplateStorage();
    const header = storage.headers().subarray(20, 40);
    field(header, 8, 0x87654321);
    const hooks = effects(image());
    delete hooks.readOnly;
    hooks.currentSlot = () => 2;
    delete hooks.refreshTitle;
    storage.loadFile(1, {}, 'preset.fort', hooks);
    assert.ok(value(header) && value(header, 8) === 1 && storage.ownedImages() === 1);
});

test('cpppj CampaignTemplateLoadFile_NormalFalseDestroysOnlyCurrentManager: 정상 false 는 실제 owner 를 반납하지만 부가 필드와 callback 의 쓰기를 보존한다', () => {
    const storage = new CampaignTemplateStorage();
    const header = storage.headers().subarray(20, 40);
    field(header, 12, 0x10203040);
    const hooks = effects(new Uint8Array(0));
    delete hooks.typeCount;
    delete hooks.currentSlot;
    delete hooks.refreshTitle;
    hooks.read = () => {
        assert.ok(value(header) && storage.ownedImages() === 1 && !value(header, 16));
        field(header, 4, 17);
        field(header, 8, 3);
        field(header, 16, 19);
        storage.serializationVersion = 27;
        return new Uint8Array(0);
    };
    storage.loadFile(1, {}, 'absent.fort', hooks);
    assert.deepEqual([value(header), value(header, 4), value(header, 8), value(header, 12), value(header, 16), storage.ownedImages(), storage.serializationVersion], [0, 17, 3, 0x10203040, 19, 0, 27]);
    assert.ok(rejects(() => storage.template(1)));
});

test('cpppj CampaignTemplateLoadFile_RejectedSignatureHasNormalFalseCleanup: 잘못된 서명 요청이 정상 복귀하면 같은 서명 필드를 본 뒤 정상 false 정리를 따른다', () => {
    const storage = new CampaignTemplateStorage();
    const header = storage.headers().subarray(20, 40);
    const hooks = effects(Uint8Array.of(0x58, 0xfe));
    let count = 0;
    hooks.rejectFile = (name) => {
        count += 1;
        assert.equal(name, 'reject.fort');
        assert.ok(value(header));
        assert.deepEqual(storage.fileHeader(1), [0x58, 1]);
        field(header, 12, 57);
        field(header, 16, 59);
    };
    storage.loadFile(1, {}, 'reject.fort', hooks);
    assert.deepEqual([count, value(header), value(header, 12), value(header, 16), storage.ownedImages()], [1, 0, 57, 59, 0]);
});

test('cpppj CampaignTemplateLoadFile_ExceptionKeepsPublishedManagerAndMissingEffects: 파일 장치의 throw 는 정상 false 가 아니며 공개한 manager 를 롤백하지 않는다', () => {
    const storage = new CampaignTemplateStorage();
    const header = storage.headers().subarray(20, 40);
    const hooks = effects(image());
    hooks.read = () => { throw new Error('명시 파일 장치 중단'); };
    assert.ok(rejects(() => storage.loadFile(1, {}, 'stopped.fort', hooks)));
    assert.ok(value(header) && storage.ownedImages() === 1 && !value(header, 4) && !value(header, 16));
    const manager = value(header);
    storage.loadFile(1, {}, 'stopped.fort', { sameName: (first, second) => first === second });
    assert.ok(value(header) === manager && storage.ownedImages() === 1);
    storage.clear(1);
    assert.equal(storage.ownedImages(), 0);
    // 실제 readonly 의 미연결 위치에서만 중단한다.
    assert.ok(rejects(() => storage.loadFile(1, {}, 'unbound.fort', {})));
    assert.ok(value(header) && storage.ownedImages() === 1);
});

test('cpppj CampaignTemplateLoadFile_PartialReadKeepsAttachedActualNode: 첫 섹션 append 뒤의 물리 잘림은 이미 붙인 같은 노드와 pos 를 남긴다', () => {
    const storage = new CampaignTemplateStorage();
    const hooks = effects(Uint8Array.of(0x46, 0xfe, 5, 0, 11, 13, 17, 8, 0));
    const header = storage.headers().subarray(20, 40);
    assert.ok(rejects(() => storage.loadFile(1, {}, 'partial.fort', hooks)));
    assert.ok(value(header) && storage.ownedImages() === 1);
    assert.deepEqual(storage.sectionOrder(1), ['Subscriber']);
    const stream = storage.findSection(1, 'Subscriber');
    const bytes = fortSection(storage.template(1), 'Subscriber');
    assert.ok(stream.section && bytes.length === 3 && bytes[0] === 11 && bytes[1] === 13 && bytes[2] === 17);
    const seen = observe(storage, stream);
    assert.ok(seen.end === 3 && !seen.position);
    // 검색(내부 동기화)이 읽지 않은 표준 섹션을 새 노드로 되살리지 않는다.
    assert.deepEqual(storage.sectionOrder(1), ['Subscriber']);
    storage.clear(1);
    assert.ok(!value(header) && storage.ownedImages() === 0);
});

test('cpppj CampaignTemplateLoadFile_RereadsRedirectedManagerAndPreservesEmptyExistingSection: readonly callback 이 header 를 다른 owner 로 돌리면 그 manager 에 병합을 수행한다', () => {
    const storage = new CampaignTemplateStorage();
    storage.installFile(2, {}, image([21, 22, 23], [31, 32, 33, 34]), 'target.fort');
    const target = value(storage.headers().subarray(40, 60));
    const oldSubscriber = storage.findSection(2, 'Subscriber');
    const oldState = storage.selectSection(2, 'State', 1);
    storage.state(2).position = 2;
    const before = observe(storage, oldState);
    const header = storage.headers().subarray(20, 40);
    let abandoned = 0;
    const hooks = effects(image([41, 42], []));
    hooks.readOnly = (name) => { assert.equal(name, 'requested.fort'); abandoned = value(header); field(header, 0, target); return false; };
    hooks.read = (name) => { assert.ok(name === 'target.fort' && value(header) === target); return image([41, 42], []); };
    storage.loadFile(1, {}, 'requested.fort', hooks);
    assert.ok(abandoned && abandoned !== target && value(header) === target && storage.ownedImages() === 2 && storage.template(1) === storage.template(2));
    const retained = storage.findSection(1, 'State');
    assert.ok(retained.section === oldState.section && retained.generation === oldState.generation);
    const kept = observe(storage, retained);
    assert.ok(kept.buffer === before.buffer && kept.offset === before.offset && kept.position === 2);
    assert.ok(fortSection(storage.template(2), 'State').length === 4 && fortSection(storage.template(2), 'State')[0] === 31);
    assert.ok(fortSection(storage.template(1), 'Subscriber').length === 2 && fortSection(storage.template(1), 'Subscriber')[0] === 41);
    assert.ok(rejects(() => storage.accessSection(oldSubscriber, () => {})));
    assert.equal(storage.sectionOrder(1).length, 35);
    field(header, 0, abandoned);
    storage.clear(1);
    storage.clear(2);
    assert.equal(storage.ownedImages(), 0); // 명시로 보관한 원래 owner 도 검사 끝에 반납한다.
});

test('cpppj CampaignTemplateLoadFile_PostReadNullHeaderSkipsTitleAndKeepsOwner: 읽기 callback 뒤 header 가 0 이면 성공한 manager 를 유지해도 제목/현재슬롯을 조회하지 않는다', () => {
    const storage = new CampaignTemplateStorage();
    const header = storage.headers().subarray(20, 40);
    const hooks = effects(image());
    let saved = 0;
    hooks.read = () => { saved = value(header); field(header, 0, 0); return image(); };
    delete hooks.currentSlot;
    delete hooks.refreshTitle;
    storage.loadFile(1, {}, 'detached.fort', hooks);
    assert.ok(saved && !value(header) && storage.ownedImages() === 1);
    field(header, 0, saved);
    assert.equal(storage.sectionOrder(1).length, 35);
    storage.clear(1);
    assert.equal(storage.ownedImages(), 0);
});

test('installFile: 정상 설치는 35개 표준 섹션과 State cursor 를 공개하고, 서명 거부·타입 수 진단·잘린 이미지는 설치하지 않는다', () => {
    const storage = new CampaignTemplateStorage();
    storage.installFile(3, {}, image([9, 0, 0, 0, 2, 0, 66, 67], [1, 2, 3, 4, 5]), 'ok.fort');
    assert.deepEqual(storage.sectionOrder(3), Array.from(SECTION_NAMES));
    assert.equal(storage.readSubscriberName(3), 'BC');
    assert.deepEqual(Array.from(fortSection(storage.template(3), 'State')), [1, 2, 3, 4, 5]);
    assert.equal(storage.selectState(3).end, 5);
    // 내용이 없는 섹션은 최소 1BYTE 할당을 만든 뒤 논리 end 0 으로 공개된다(요새 섹션 길이 0).
    assert.equal(fortSection(storage.template(3), 'Chaff').length, 0);
    assert.equal(fortSection(storage.template(3), 'Terr19').length, 0);
    // 같은 슬롯 재설치는 이전 manager 를 해제한 뒤 새 파일을 읽는다.
    storage.installFile(3, {}, image(), 'again.fort');
    assert.equal(storage.ownedImages(), 1);
    assert.equal(storage.name(3), 'again.fort');
    // 서명 'X' 는 거부 요청이 예외라 설치하지 않는다.
    const before = Uint8Array.from(storage.headers());
    assert.throws(() => storage.installFile(4, {}, Uint8Array.of(0x58, 0xfe), 'bad.fort'), /보호 원문은 삭제하지 않습니다/u);
    assert.deepEqual(Array.from(storage.headers().subarray(80, 100)), Array.from(before.subarray(80, 100)));
    assert.equal(storage.ownedImages(), 1);
    // TYPE 개수 256 이상은 진단 예외이고 빈 이미지는 열기 거부, 잘린 이미지는 범위 오류이다.
    assert.throws(() => storage.installFile(4, {}, image(), 'types.fort', 256), /numTypes<256/u);
    assert.throws(() => storage.installFile(4, {}, new Uint8Array(0), 'empty.fort'), /열기\/서명 거부/u);
    assert.throws(() => storage.installFile(4, {}, Uint8Array.of(0x46, 0xfe, 9, 0, 1), 'cut.fort'));
    assert.equal(storage.ownedImages(), 1);
});

test('Native readWhole 경로: 최소 4BYTE 할당·논리 길이를 분리하고 없는 파일/잘못된 길이는 구별한다', () => {
    const files = new Map([['d/native.fort', image()], ['d/tiny.fort', Uint8Array.of(0x46)]]);
    const gameFiles = { read: (path) => files.get(path) ?? null };
    const storage = new CampaignTemplateStorage();
    const base = effects(new Uint8Array(0));
    delete base.read;
    const hooks = bindDataManagerFileRead(base, gameFiles);
    storage.loadFile(1, {}, 'd/native.fort', hooks);
    assert.equal(storage.sectionOrder(1).length, 35);
    assert.equal(storage.state(1).end, 3);
    // 없는 파일은 정상 false 로 manager 를 반납한다.
    storage.loadFile(2, {}, 'd/missing.fort', hooks);
    assert.equal(value(storage.headers().subarray(40, 60)), 0);
    assert.equal(storage.ownedImages(), 1);
    // 길이 1 의 파일은 4BYTE 할당이고 서명 'F' 는 통과하지만 시작 위치가 2 가 아니라 원문 진단이 나며, 진단은 가짜 성공이 아니라 예외이다.
    assert.throws(() => storage.loadFile(3, {}, 'd/tiny.fort', hooks), /원본 진단/u);
    assert.ok(value(storage.headers().subarray(60, 80)) !== 0);
    // 논리 길이가 할당보다 길다는 장치는 불일치로 거부한다.
    const lying = { ...base, readWhole: () => ({ storage: new Uint8Array(4), length: 9 }) };
    assert.throws(() => storage.loadFile(4, {}, 'd/lying.fort', lying), /논리 길이 불일치/u);
    // 할당이 4BYTE 미만이면 장치 계약 위반으로 거부한다.
    const shortStorage = { ...base, readWhole: () => ({ storage: new Uint8Array(2), length: 2 }) };
    assert.throws(() => storage.loadFile(6, {}, 'd/short.fort', shortStorage), /논리 길이 불일치/u);
    // 존재하는 빈 파일은 최소 할당을 소유하지만 서명이 없어 정상 false 이다.
    const empty = { ...base, readWhole: () => ({ storage: new Uint8Array(4), length: 0 }) };
    storage.loadFile(5, {}, 'd/empty.fort', empty);
    assert.equal(value(storage.headers().subarray(100, 120)), 0);
});
