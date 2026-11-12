// Template9 실제 manager 이미지·섹션 노드·typed cursor 소유자(campaign-template-storage.js 1부)를 cpppj 시험과 웹 계약으로 확인한다.
//  - cpppj 시험 이식: CampaignTemplateDynamicTests 의 Temp 이름 교체가 노드/할당/생성 수명을 보존하는 시험과 typed cursor 가 현재 이름을 따르는 시험. 20개 영역·저장 시험(`TwentyTerritoriesAndCurrentSave`)은 SaveFile 이 필요해 3부(E347)에서 옮긴다.
//  - 웹 계약: 9헤더 초기 상태와 빌림/반납(원래 소유자 반납 거부·자기 빌림 거부), 빈 manager 할당→원문 ctor→35개 이름 초기화, 섹션 생성/검색/선택의 current 규칙과 오래된 쓰기 참조 거부, 인라인 쓰기 빌림 중 구조 변경 금지,
//    State 28BYTE 초기화·최소 확장·분리 확장 복사, Subscriber 이름 읽기의 cursor 진행과 범위 거부, Money/Badges/Technology cursor 공개, 판본 전역(005c89c0)의 ctor 0 쓰기, 실제 디렉터리 순서 보존.
// 실제 게임·브라우저 실행이 아니다. (2026-11-12 WB04-E345)
import test from 'node:test';
import assert from 'node:assert/strict';
import { SECTION_NAMES, fortSection } from '../www/assets/fort.js';
import { FortSerializationGlobals } from '../www/core/fort-serialization-globals.js';
import { ByteVector, CampaignTemplateStorage } from '../www/core/campaign-template-storage.js';
import { writeMemStream } from '../www/core/raw-mem-stream.js';

// 기존 35개 빈 파일 섹션을 최초 입력으로 만든다. 실행 중 Temp 를 고정 Terr handle 로 대신하지 않는다. (2026-11-12 WB04-E345)
const emptyFort = () => ({ sections: Array.from({ length: SECTION_NAMES.length }, () => new Uint8Array(0)) });
// 원본 진단/해제된 참조는 실제 예외여야 한다. (2026-11-12 WB04-E345)
function rejects(action) {
    try { action(); } catch { return true; }
    return false;
}
// 같은 MemStream 의 실제 버퍼와 세 DWORD 를 공개하는 동기 관찰이다. (2026-11-12 WB04-E345)
function observe(storage, stream) {
    const observed = { buffer: null, offset: -1, pointer: 0, position: 0, end: 0, capacity: 0 };
    storage.accessSection(stream, (fields, memory) => {
        const read = (at) => (fields[at] | (fields[at + 1] << 8) | (fields[at + 2] << 16) | (fields[at + 3] << 24)) >>> 0;
        observed.pointer = read(0);
        observed.position = read(4);
        observed.end = read(8);
        observed.capacity = read(12);
        if (observed.pointer) {
            const view = memory.buffer(observed.pointer);
            observed.buffer = view.buffer;
            observed.offset = view.byteOffset;
        }
    });
    return observed;
}
// 두 관찰이 같은 실제 저장소를 가리키는지이다. (2026-11-12 WB04-E345)
const sameStorage = (left, right) => left.buffer === right.buffer && left.offset === right.offset;
// 뷰가 관찰한 저장소와 같은지이다. (2026-11-12 WB04-E345)
const viewIs = (view, observed) => view.buffer === observed.buffer && view.byteOffset === observed.offset;
// 개수를 센다. (2026-11-12 WB04-E345)
const count = (list, value) => list.filter((item) => item === value).length;

test('cpppj CampaignTemplates_DynamicTempRenamePreservesNodeAndAllocation: Rename 은 Temp 노드와 할당을 남기고 대상 노드만 반납한다', () => {
    const storage = new CampaignTemplateStorage();
    storage.install(1, emptyFort(), 'dynamic.fort');
    storage.borrow(8, 1);
    const previous = storage.findSection(1, 'Terr00');
    const source = storage.createSection(8, 'Temp');
    const bytes = Uint8Array.of(2, 7, 9);
    storage.writeSection(source, bytes);
    const before = observe(storage, source);
    // 새 섹션은 노드 핸들로 만든 버퍼 토큰과 16384BYTE 할당을 가진다.
    assert.equal(before.pointer, 0x10000000 + source.section);
    assert.equal(before.capacity, 16384);
    assert.equal(before.end, 3);
    const renamed = storage.renameSection(1, 'Terr00', 'Temp');
    const after = observe(storage, source);
    assert.deepEqual(renamed, source);
    assert.notEqual(renamed.section, previous.section);
    assert.ok(before.pointer === after.pointer && sameStorage(before, after) && before.position === after.position && before.end === after.end && before.capacity === after.capacity);
    assert.ok(viewIs(fortSection(storage.template(8), 'Terr00'), before));
    assert.equal(fortSection(storage.template(1), 'Terr00').length, 3);
    assert.equal(storage.currentSection(8), 'Terr00');
    const order = storage.sectionOrder(1);
    assert.equal(order.length, 35);
    assert.equal(order.at(-1), 'Terr00');
    assert.equal(count(order, 'Terr00'), 1);
    assert.equal(count(order, 'Temp'), 0);
    // 반납된 대상 노드의 이전 참조로 쓰는 것은 거부한다.
    assert.ok(rejects(() => storage.writeSection(previous, bytes)));
    const absent = storage.findSection(8, 'Temp');
    assert.equal(absent, null);
    assert.equal(storage.currentSection(1), '');
    assert.ok(rejects(() => storage.currentWriteStream(1)));
    storage.writeSection(source, bytes.subarray(0, 1));
    assert.equal(fortSection(storage.template(1), 'Terr00').length, 4);
    const selected = storage.selectSection(8, 'tErR00', 0);
    assert.equal(selected.section, source.section);
    assert.equal(observe(storage, source).position, 0);
    storage.clear(8);
    assert.equal(storage.ownedImages(), 1);
    storage.clear(1);
    assert.ok(rejects(() => storage.writeSection(source, bytes)));
    assert.equal(storage.ownedImages(), 0);
});

test('cpppj CampaignTemplates_DynamicTypedCursorFollowsCurrentName: 표준 이름에서 동적 이름으로 옮긴 같은 노드는 typed cursor 를 재사용하지 않는다', () => {
    const storage = new CampaignTemplateStorage();
    storage.install(1, emptyFort(), 'typed.fort');
    const temp = storage.createSection(1, 'Temp');
    storage.writeSection(temp, Uint8Array.of(17, 29));
    const state = storage.renameSection(1, 'State', 'Temp');
    assert.equal(state.section, temp.section);
    assert.equal(storage.state(1).position, 2);
    assert.equal(storage.state(1).end, 2);
    assert.ok(viewIs(storage.state(1).bytes, observe(storage, temp)));
    storage.state(1).position = 1;
    const before = observe(storage, temp);
    const renamed = storage.renameSection(1, 'Scratch', 'State');
    assert.equal(renamed.section, temp.section);
    assert.equal(observe(storage, temp).position, before.position);
    assert.equal(storage.state(1).bytes.length, 0);
    assert.equal(storage.findSection(1, 'State'), null);
    assert.equal(count(storage.sectionOrder(1), 'State'), 0);
    const restored = storage.renameSection(1, 'State', 'Scratch');
    assert.ok(restored.section === temp.section && restored.generation === temp.generation);
    assert.equal(storage.state(1).position, 1);
    assert.equal(storage.state(1).end, 2);
    const replacement = storage.createSection(1, 'State');
    assert.notEqual(replacement.section, temp.section);
    assert.ok(rejects(() => storage.accessSection(temp, () => {})));
});

test('9개 헤더는 0 으로 시작하고 설치·빌림·반납의 소유 규칙을 지킨다', () => {
    const storage = new CampaignTemplateStorage();
    assert.equal(storage.headers().length, 180);
    assert.ok(storage.headers().every((byte) => byte === 0));
    assert.throws(() => storage.template(0), /manager가 없거나/u);
    assert.throws(() => storage.clear(9), RangeError);
    storage.install(2, emptyFort(), 'owner.fort');
    assert.equal(storage.ownedImages(), 1);
    const owner = storage.template(2);
    storage.borrow(5, 2);
    assert.equal(storage.template(5), owner);
    assert.equal(storage.headers()[5 * 20 + 4], 1);
    // 자기 manager 빌림과 원래 소유자를 반납하는 같은 manager 빌림은 거부한다.
    assert.throws(() => storage.borrow(2, 2), TypeError);
    assert.throws(() => storage.borrow(4, 3), /manager/u);
    // 빌린 헤더를 비워도 소유자의 이미지는 유지된다.
    storage.clear(5);
    assert.equal(storage.ownedImages(), 1);
    assert.equal(storage.template(2), owner);
    storage.clearAll();
    assert.equal(storage.ownedImages(), 0);
    assert.ok(storage.headers().every((byte) => byte === 0));
    // 같은 슬롯에 다시 설치하면 이전 한 manager 를 정확히 해제한 뒤 새 객체를 소유한다.
    storage.install(1, emptyFort(), 'a.fort');
    storage.install(1, emptyFort(), 'b.fort');
    assert.equal(storage.ownedImages(), 1);
    assert.equal(storage.name(1), 'b.fort');
});

test('설치한 manager 의 파일 이름·서명 판정·판본 전역: 생성자는 005c89c0 에 0 을 쓴다', () => {
    const globals = new FortSerializationGlobals();
    const storage = new CampaignTemplateStorage(globals);
    globals.version = 2;
    storage.install(3, emptyFort(), 'named.fort');
    assert.equal(globals.version, 0);
    assert.equal(storage.serializationVersion, 0);
    storage.serializationVersion = 5;
    assert.equal(globals.version, 5);
    assert.equal(storage.serializationGlobals, globals);
    assert.equal(storage.name(3), 'named.fort');
    // 서명 판정은 읽기/저장 전에는 0/ffffffff 이다.
    assert.deepEqual(storage.fileHeader(3), [0, 0xffffffff]);
    assert.throws(() => storage.install(3, emptyFort(), 'x'.repeat(256)), TypeError);
    assert.throws(() => storage.install(3, emptyFort(), 'a\0b'), TypeError);
});

test('빈 manager 할당→원문 ctor→35개 이름 초기화: 표준 이름 순서의 섹션 목록이 만들어진다', () => {
    const storage = new CampaignTemplateStorage();
    assert.throws(() => storage.allocateEmptyManager(275), TypeError);
    const token = storage.allocateEmptyManager(276);
    assert.ok(token > 0);
    // 원문 ctor 이전 초기화와 중복 ctor 는 거부한다.
    assert.throws(() => storage.initializeEmptyManager(token, () => {}), /ctor이전/u);
    assert.equal(storage.constructEmptyManager(token, 'empty.fort'), token);
    assert.throws(() => storage.constructEmptyManager(token, 'again'), TypeError);
    assert.equal(storage.managerBytes(token).length, 276);
    storage.initializeEmptyManager(token, () => { throw new Error('초기화 진단이 나면 안 됨'); });
    // 헤더에 설치하지 않은 채로도 같은 manager 물리가 35개 이름의 노드를 가진다(manager head 가 비어 있지 않다).
    storage.headers().set([token & 255, (token >> 8) & 255, (token >> 16) & 255, token >>> 24], 0);
    assert.deepEqual(storage.sectionOrder(0), Array.from(SECTION_NAMES));
    assert.equal(storage.currentSection(0), SECTION_NAMES.at(-1));
    // 소멸/반납 수명 장치는 같은 Image 를 대상으로 한다.
    const lifetime = storage.managerLifetime();
    lifetime.destroy(token);
    lifetime.release(token);
    assert.equal(storage.ownedImages(), 0);
});

test('섹션 생성은 같은 이름의 기존 노드를 교체하고 오래된 참조·없는 manager 참조를 거부한다', () => {
    const storage = new CampaignTemplateStorage();
    storage.install(1, emptyFort(), 'create.fort');
    const first = storage.createSection(1, 'Chaff');
    storage.writeSection(first, Uint8Array.of(1, 2, 3, 4));
    assert.deepEqual(Array.from(fortSection(storage.template(1), 'Chaff')), [1, 2, 3, 4]);
    const second = storage.createSection(1, 'Chaff');
    assert.notEqual(second.section, first.section);
    assert.ok(rejects(() => storage.writeSection(first, Uint8Array.of(9))));
    assert.deepEqual(Array.from(fortSection(storage.template(1), 'Chaff')), []);
    storage.writeSection(second, Uint8Array.of(7, 8));
    assert.deepEqual(Array.from(fortSection(storage.template(1), 'Chaff')), [7, 8]);
    // 현재 쓰기 참조와 검색: 검색 성공은 current 를 그 노드로 바꾸고(이름은 대소문자 무시) 실패는 current 를 0 으로 쓴다(빈 참조).
    assert.deepEqual(storage.currentWriteStream(1), second);
    assert.equal(storage.findSection(1, 'chaff').section, second.section);
    assert.equal(storage.currentSection(1), 'Chaff');
    assert.equal(storage.findSection(1, 'NoSuchSection'), null);
    assert.equal(storage.currentSection(1), '');
    assert.ok(rejects(() => storage.currentWriteStream(1)));
    storage.findSection(1, 'Chaff');
    // 동적(비표준) 이름은 노드가 소유한 별도 버퍼를 쓰며 표준 섹션에는 나타나지 않는다.
    const dynamic = storage.createSection(1, 'Scratch');
    storage.writeSection(dynamic, Uint8Array.of(5));
    assert.equal(observe(storage, dynamic).end, 1);
    assert.deepEqual(Array.from(fortSection(storage.template(1), 'Scratch')), []);
    // manager 토큰으로 직접 만들기와 없는 manager 거부.
    assert.throws(() => storage.createManagerSection(999, 'X'), RangeError);
    assert.throws(() => storage.createManagerSection(0, 'X'), RangeError);
    const manager = new DataView(storage.headers().buffer, storage.headers().byteOffset + 20, 4).getUint32(0, true);
    const direct = storage.createManagerSection(manager, 'Direct');
    assert.equal(direct.manager, manager);
    assert.ok(rejects(() => storage.writeSection({ ...direct, generation: direct.generation + 1 }, Uint8Array.of(1))));
    assert.ok(rejects(() => storage.writeSection({ manager: direct.manager, section: direct.section, generation: 0 }, Uint8Array.of(1))));
    assert.ok(rejects(() => storage.writeSection(undefined, Uint8Array.of(1))));
});

test('인라인 쓰기 빌림 중에는 구조 변경과 같은 노드의 중첩 빌림을 금지한다', () => {
    const storage = new CampaignTemplateStorage();
    storage.install(1, emptyFort(), 'access.fort');
    const stream = storage.createSection(1, 'Scratch');
    let nested = false;
    storage.accessSection(stream, () => {
        assert.throws(() => storage.accessSection(stream, () => { nested = true; }), /중첩/u);
        assert.throws(() => storage.createSection(1, 'Other'), /구조 변경 금지/u);
        assert.throws(() => storage.renameSection(1, 'A', 'Scratch'), /구조 변경 금지/u);
        assert.throws(() => storage.ensureStateSize(1, 28), /구조 변경 금지/u);
    });
    assert.equal(nested, false);
    // 끝난 뒤에는 다시 가능하다.
    storage.createSection(1, 'Other');
    assert.throws(() => storage.accessSection(stream, undefined), TypeError);
});

test('State: 28BYTE 초기화·최소 확장 복사·분리 확장(growState/commit)', () => {
    const storage = new CampaignTemplateStorage();
    storage.install(1, emptyFort(), 'state.fort');
    assert.throws(() => storage.initializeState(1, new Uint8Array(27)), RangeError);
    const initial = Uint8Array.from({ length: 28 }, (_, index) => index + 1);
    const state = storage.initializeState(1, initial);
    assert.equal(state.end, 28);
    assert.equal(state.position, 28);
    assert.equal(state.present, true);
    assert.deepEqual(Array.from(fortSection(storage.template(1), 'State')), Array.from(initial));
    // 최소 확장: 이미 28 이상이면 그대로, 더 크면 임시 노드로 복사해 같은 State 로 넘기고 앞 28BYTE 를 보존한다.
    const same = storage.ensureStateSize(1, 28);
    assert.equal(same.end, 28);
    const grown = storage.ensureStateSize(1, 60);
    assert.ok(grown.end >= 60);
    assert.deepEqual(Array.from(grown.bytes.subarray(0, 28)), Array.from(initial));
    assert.equal(count(storage.sectionOrder(1), 'State'), 1);
    assert.equal(count(storage.sectionOrder(1), 'T$e$m$p'), 0);
    // 분리 확장: 새 0 채움 버퍼를 만들고 commit 전에는 두 번째 확장을 거부한다.
    const bigger = storage.growState(1, 100);
    assert.equal(bigger.length, 100);
    assert.ok(bigger.every((byte) => byte === 0));
    assert.throws(() => storage.growState(1, 10), /복사 미완료/u);
    assert.throws(() => storage.createSection(1, 'Other'), /State 확장 복사 중/u);
    storage.commitStateGrowth(1);
    assert.equal(storage.growState(1, 5).length, 5);
});

test('Subscriber 이름 읽기: ID 4BYTE 를 건너뛰고 길이 WORD 뒤 이름을 읽으며 cursor 를 진행한다', () => {
    const storage = new CampaignTemplateStorage();
    storage.install(1, emptyFort(), 'sub.fort');
    const stream = storage.createSection(1, 'Subscriber');
    const name = [...'Champion'].map((ch) => ch.charCodeAt(0));
    storage.writeSection(stream, Uint8Array.of(9, 0, 0, 0, name.length, 0, ...name));
    assert.equal(storage.readSubscriberName(1), 'Champion');
    // 읽은 만큼 pos 가 진행한다(4 + 2 + 8).
    assert.equal(observe(storage, stream).position, 14);
    // 짧은 섹션은 범위 오류로 거부한다.
    const other = new CampaignTemplateStorage();
    other.install(1, emptyFort(), 'short.fort');
    const shortStream = other.createSection(1, 'Subscriber');
    other.writeSection(shortStream, Uint8Array.of(1, 0, 0, 0, 5, 0, 65));
    assert.throws(() => other.readSubscriberName(1), RangeError);
});

test('Money/Badges/Technology/Territory cursor: 현재 이름의 실제 노드를 공개하고 선택은 pos 0 으로 되돌린다', () => {
    const storage = new CampaignTemplateStorage();
    storage.install(1, emptyFort(), 'cursors.fort');
    const money = storage.createSection(1, 'Money');
    storage.writeSection(money, Uint8Array.of(0, 0, 0x80, 0x3f));
    assert.deepEqual({ position: storage.money(1).position, end: storage.money(1).end }, { position: 4, end: 4 });
    assert.deepEqual({ position: storage.selectMoney(1).position, end: storage.selectMoney(1).end }, { position: 0, end: 4 });
    const badges = storage.createSection(1, 'Badges');
    storage.writeSection(badges, Uint8Array.of(5, 6));
    assert.equal(storage.badges(1).end, 2);
    assert.equal(storage.badges(1).present, true);
    assert.equal(storage.selectBadges(1).position, 0);
    const technology = storage.createSection(1, 'Technology');
    storage.writeSection(technology, Uint8Array.of(1, 2, 3));
    assert.equal(storage.technology(1).end, 3);
    assert.equal(storage.selectTechnology(1).position, 0);
    const territory = storage.createSection(1, 'Territory');
    storage.writeSection(territory, Uint8Array.of(1, 2, 3, 4));
    assert.equal(storage.territory(1).end, 4);
    assert.equal(storage.selectTerritory(1).position, 0);
    // 선택된 읽기 섹션이 없는 manager 는 원본 선택 진단으로 거부한다(가짜 빈 성공 금지).
    const bare = new CampaignTemplateStorage();
    bare.install(1, emptyFort(), 'bare.fort');
    assert.throws(() => bare.selectMoney(1), /진단/u);
    assert.equal(bare.selectBadges(1).end, 0);
});

test('설치만 한 빈 표준 섹션은 버퍼 토큰 0·할당 0 이고 Money 이름 교체는 해당 cursor 만 비운다', () => {
    const storage = new CampaignTemplateStorage();
    storage.install(1, emptyFort(), 'empty.fort');
    const found = storage.findSection(1, 'Chaff');
    const empty = observe(storage, found);
    assert.deepEqual([empty.pointer, empty.capacity, empty.end], [0, 0, 0]);
    // 표준 이름을 다른 이름으로 바꾸면 그 이름의 typed cursor 만 비워진다.
    const money = storage.createSection(1, 'Money');
    storage.writeSection(money, Uint8Array.of(0, 0, 0x80, 0x3f));
    assert.equal(storage.money(1).end, 4);
    const badges = storage.createSection(1, 'Badges');
    storage.writeSection(badges, Uint8Array.of(1));
    assert.equal(storage.badges(1).end, 1);
    storage.renameSection(1, 'Scratch', 'Money');
    // 이름이 사라진 Money 는 getter 가 현재 노드 누락으로 거부하고 Badges 는 영향이 없다.
    assert.throws(() => storage.money(1), /현재 노드 누락/u);
    assert.equal(storage.badges(1).end, 1);
    // 되돌리면 같은 노드의 현재 pos/end 로 다시 공개된다.
    storage.renameSection(1, 'Money', 'Scratch');
    assert.equal(storage.money(1).end, 4);
    storage.renameSection(1, 'Tech2', 'Badges');
    assert.throws(() => storage.badges(1), /현재 노드 누락/u);
});

test('분리 확장 중에는 원본 최소 확장을 거부하고 Subscriber 이름은 255BYTE 까지만 읽는다', () => {
    const storage = new CampaignTemplateStorage();
    storage.install(1, emptyFort(), 'limits.fort');
    storage.initializeState(1, new Uint8Array(28));
    storage.growState(1, 40);
    assert.throws(() => storage.ensureStateSize(1, 60), /분리 State 복사 중/u);
    storage.commitStateGrowth(1);
    // 구독자 이름: 255BYTE 는 읽고 256BYTE 는 거부한다.
    const make = (length) => {
        const other = new CampaignTemplateStorage();
        other.install(1, emptyFort(), 'name.fort');
        const stream = other.createSection(1, 'Subscriber');
        other.writeSection(stream, Uint8Array.of(1, 0, 0, 0, length & 255, length >> 8, ...new Array(length).fill(66)));
        return other;
    };
    assert.equal(make(255).readSubscriberName(1).length, 255);
    assert.throws(() => make(256).readSubscriberName(1), RangeError);
    // NUL 이전까지만 이름이다.
    const withNul = new CampaignTemplateStorage();
    withNul.install(1, emptyFort(), 'nul.fort');
    withNul.writeSection(withNul.createSection(1, 'Subscriber'), Uint8Array.of(1, 0, 0, 0, 4, 0, 65, 0, 66, 67));
    assert.equal(withNul.readSubscriberName(1), 'A');
});

// ByteVector(std::vector<uint8_t> 의 size/capacity/move 의미)를 직접 확인한다. (2026-11-12 WB04-E345)
test('ByteVector: resize 는 용량 안에서 같은 저장소를 쓰며 늘린 칸을 0 으로 채우고, 용량을 넘으면 내용을 보존해 옮기고, take 는 원본을 비운다', () => {
    const calls = [];
    const vector = new ByteVector(Uint8Array.of(1, 2, 3, 4), () => calls.push('changed'));
    assert.deepEqual([vector.size, vector.capacity], [4, 4]);
    // 줄여도 용량은 그대로이고 같은 저장소의 뷰이다.
    vector.resize(2);
    const shared = vector.bytes.buffer;
    assert.deepEqual([vector.size, vector.capacity, Array.from(vector.bytes)], [2, 4, [1, 2]]);
    // 용량 안에서 다시 늘리면 같은 저장소이고 늘어난 칸은 이전 값(3,4)이 아니라 0 이다.
    vector.resize(4);
    assert.equal(vector.bytes.buffer, shared);
    assert.deepEqual(Array.from(vector.bytes), [1, 2, 0, 0]);
    // 용량을 넘으면 새 저장소로 내용을 보존해 옮기고 용량이 새 크기가 된다.
    vector.resize(6);
    assert.notEqual(vector.bytes.buffer, shared);
    assert.deepEqual([vector.size, vector.capacity, Array.from(vector.bytes)], [6, 6, [1, 2, 0, 0, 0, 0]]);
    // 이동 대입은 저장소와 크기를 가져오고 원본은 비운다.
    const target = new ByteVector();
    target.take(vector);
    assert.deepEqual([target.size, target.capacity, Array.from(target.bytes)], [6, 6, [1, 2, 0, 0, 0, 0]]);
    assert.deepEqual([vector.size, vector.capacity], [0, 0]);
    // release 는 저장소를 모두 놓는다. 변경 통지는 resize/release/take 마다 불린다.
    target.release();
    assert.deepEqual([target.size, target.capacity], [0, 0]);
    assert.ok(calls.length >= 4);
    // adopt 하지 않으면 복사하고 adopt 하면 같은 저장소를 인수한다.
    const original = Uint8Array.of(9, 9);
    original[0] = 7;
    const copied = new ByteVector(original);
    const adopted = new ByteVector(original, null, true);
    original[1] = 5;
    assert.deepEqual([Array.from(copied.bytes), Array.from(adopted.bytes)], [[7, 9], [7, 5]]);
});

test('typed cursor 는 현재 이름을 따라 위치를 노드에 반영하고, 이름이 바뀐 노드에는 이전 위치를 쓰지 않는다', () => {
    const storage = new CampaignTemplateStorage();
    storage.install(1, emptyFort(), 'sync.fort');
    // Money: 위치를 바꾸면 인라인 접근 전에 노드 pos 로 반영된다.
    const money = storage.createSection(1, 'Money');
    storage.writeSection(money, Uint8Array.of(0, 0, 0x80, 0x3f));
    const moneyCursor = storage.money(1);
    moneyCursor.position = 2;
    assert.equal(observe(storage, money).position, 2);
    // 이름을 바꾸기 직전의 위치(3)는 노드에 반영되고, 이름을 잃은 뒤 낡은 cursor 객체의 위치 변경은 새 이름의 노드에 쓰지 않는다.
    moneyCursor.position = 3;
    storage.renameSection(1, 'Scratch', 'Money');
    moneyCursor.position = 1;
    assert.equal(observe(storage, money).position, 3);
    // Territory/Badges/Technology 도 같다.
    for (const [name, accessor] of [['Territory', 'territory'], ['Badges', 'badges'], ['Technology', 'technology']]) {
        const stream = storage.createSection(1, name);
        storage.writeSection(stream, Uint8Array.of(1, 2, 3, 4));
        const cursor = storage[accessor](1);
        assert.equal(cursor.end, 4);
        cursor.position = 1;
        assert.equal(observe(storage, stream).position, 1);
        cursor.position = 3;
        storage.renameSection(1, `Renamed${name}`, name);
        cursor.position = 2;
        assert.equal(observe(storage, stream).position, 3, name);
    }
    // Territory 는 loadCursor 없이도 이름을 잃으면 비워진다.
    assert.equal(storage.territory(1).end, 0);
});

test('MemStream 쓰기 중 예외는 부분 쓰기와 논리 end 를 보존하고 물리 할당을 논리 자료로 공개하지 않는다', () => {
    const storage = new CampaignTemplateStorage();
    storage.install(1, emptyFort(), 'partial.fort');
    const stream = storage.createSection(1, 'Chaff');
    assert.throws(() => storage.accessSection(stream, (fields, hooks) => {
        writeMemStream(fields, Uint8Array.of(4, 5, 6), 3, hooks);
        throw new Error('쓰기 뒤 실패');
    }), /쓰기 뒤 실패/u);
    assert.deepEqual(Array.from(fortSection(storage.template(1), 'Chaff')), [4, 5, 6]);
    assert.equal(observe(storage, stream).end, 3);
    // 잘못된 버퍼 반납 토큰은 범위 오류로 거부한다.
    assert.throws(() => storage.accessSection(stream, (_fields, hooks) => { hooks.release(0x10000000); }), /버퍼 핸들 범위/u);
    assert.throws(() => storage.accessSection(stream, (_fields, hooks) => { hooks.release(0x10000000 + 9999); }), /버퍼 핸들 범위/u);
});

test('표준 섹션이 일부(10개)뿐인 요새에서 Temp 를 표준 이름으로 바꿔도 같은 이름의 노드가 두 번 생기지 않는다', () => {
    const storage = new CampaignTemplateStorage();
    const fort = { sections: Array.from({ length: 10 }, () => new Uint8Array(0)) };
    storage.install(1, fort, 'short.fort');
    assert.deepEqual(storage.sectionOrder(1), Array.from(SECTION_NAMES).slice(0, 10));
    const temp = storage.createSection(1, 'Temp');
    storage.writeSection(temp, Uint8Array.of(1));
    storage.renameSection(1, 'Terr05', 'Temp');
    // 표준 위치가 앞 칸을 채워도(길이 확장) 이미 연결한 Terr05 를 다시 연결하지 않는다.
    storage.findSection(1, 'State');
    const order = storage.sectionOrder(1);
    assert.equal(count(order, 'Terr05'), 1);
    assert.equal(fort.sections.length, SECTION_NAMES.indexOf('Terr05') + 1);
});

test('State 최소 확장 뒤 현재 쓰기 참조로 쓰면 확장한 할당에 쓰인다(노드 토큰이 새 할당을 가리킨다)', () => {
    const storage = new CampaignTemplateStorage();
    storage.install(1, emptyFort(), 'grow.fort');
    storage.initializeState(1, Uint8Array.from({ length: 28 }, (_, index) => index));
    const grown = storage.ensureStateSize(1, 60);
    grown.position = 30;
    const reference = storage.currentWriteStream(1);
    const before = observe(storage, reference);
    assert.equal(before.pointer, 0x10000000 + reference.section);
    assert.equal(before.position, 30);
    storage.writeSection(reference, Uint8Array.of(99));
    assert.equal(fortSection(storage.template(1), 'State').at(30), 99);
});

test('빌림은 이미 같은 manager 를 빌린 칸에 다시 빌릴 수 있고 자기 빌림은 메시지로 구별해 거부한다', () => {
    const storage = new CampaignTemplateStorage();
    storage.install(2, emptyFort(), 'borrow.fort');
    storage.borrow(5, 2);
    storage.borrow(5, 2);
    assert.equal(storage.headers()[5 * 20 + 4], 1);
    assert.throws(() => storage.borrow(2, 2), /자기 manager 빌림/u);
    assert.throws(() => storage.borrow(2, 5), /원래 소유자를 반납하는/u);
});

test('섹션 선택의 쓰기 허용 모드: 빈 표준 섹션은 읽기 선택이 진단을 내고 쓰기 허용 선택은 통과한다', () => {
    const storage = new CampaignTemplateStorage();
    storage.install(1, emptyFort(), 'select.fort');
    assert.throws(() => storage.selectSection(1, 'Chaff', 0), /진단/u);
    const writable = storage.selectSection(1, 'Chaff', 1);
    assert.equal(writable.manager, storage.findSection(1, 'Chaff').manager);
    // 빈 Technology/Badges 는 쓰기 허용 선택으로 통과한다.
    assert.equal(storage.selectTechnology(1).end, 0);
    assert.equal(storage.selectBadges(1).end, 0);
    assert.throws(() => storage.selectMoney(1), /진단/u);
});

test('빌린 cursor 참조는 선택·확장·이름 교체 뒤에도 같은 객체이며 현재 값을 갖는다(C++ 멤버에 값을 대입하는 의미)', () => {
    const storage = new CampaignTemplateStorage();
    storage.install(1, emptyFort(), 'live.fort');
    const state = storage.initializeState(1, new Uint8Array(28));
    assert.equal(storage.state(1), state);
    const territory = storage.territory(1);
    const grown = storage.ensureStateSize(1, 60);
    assert.equal(grown, state);
    assert.equal(state.end, 60);
    assert.equal(storage.selectState(1), state);
    assert.equal(storage.territory(1), territory);
    storage.renameSection(1, 'Scratch', 'State');
    assert.equal(storage.state(1), state);
    assert.equal(state.end, 0);
    assert.equal(state.present, false);
});
