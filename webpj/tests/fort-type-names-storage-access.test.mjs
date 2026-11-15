// cpppj FortTypeNamesStorageAccessTests의 실제 Template fields/data·guard를 같은 변환표 소유자에 연결한다. 전체 Session은 아직 없다. (WB04-E573)
import test from 'node:test';
import assert from 'node:assert/strict';
import { CampaignTemplateStorage } from '../www/core/campaign-template-storage.js';
import { FortTypeConversionStorage } from '../www/core/fort-type-conversion-storage.js';
import { loadFortTypeNames, unwindFortTypeNamesLoad } from '../www/core/raw-fort-type-names-load.js';
import { loadFortContents } from '../www/core/raw-fort-contents-load.js';
import { get, put } from './raw-support.mjs';

// 원본 입력으로 고정한 Alpha/Beta hash다. 제품 hash로 기대값을 계산하지 않는다.
const ALPHA = 0x68706ca2, BETA = 0x61746542;
// TYPE 하위의 명시 중단이다.
class TypeStop extends Error {}
// 실제 manager/stream/변환표의 수명을 한 검사 장면이 보유한다.
class StorageDevices {
    // 같은 실제 저장소와255BYTE owner, 동기 접근 상태와 비교용 물리 정체성이다.
    storage = new CampaignTemplateStorage(); conversion = new FortTypeConversionStorage((bytes) => bytes.fill(0xa7)); selected; trace = []; active = false; fields = null; memory = null; observedFields; observedData;
    // callback이 수행하는 명시 변경 입력이다. 결과는 로더가 결정한다.
    changeCurrent = false; checkGuards = false; zeroEndAndCount = false; changeStoredHash = false; stopAtBeta = false;
    // NUL 포함 실제 TYPE 이름들이다.
    names = ['Unused\0', 'Alpha\0', 'Beta\0'].map((text) => Uint8Array.from(text, (character) => character.charCodeAt(0)));
    // 같은 manager를 slot1/8이 빌리며 TypeNames 뒤 새 노드로 tail을 압축한다.
    constructor(empty = false) {
        this.storage.serializationGlobals.loadingDepth = 3; this.storage.install(1, { sections: Array.from({ length: 35 }, () => new Uint8Array(0)) }, 'types-access.fort');
        if (!empty) { const target = this.storage.createSection(1, 'TypeNames'), bytes = new Uint8Array(9); bytes[0] = 2; put(bytes, 1, ALPHA); put(bytes, 5, BETA); this.storage.writeSection(target, bytes); const scratch = this.storage.createSection(1, 'Scratch'); this.storage.writeSection(scratch, Uint8Array.of(99)); }
        this.storage.borrow(8, 1);
    }
    // 실제 loadingDepth와 currentDWORD를 직접 빌린다. 사본을 만들지 않는다.
    globals() { const owner = this; return { get loadingDepth() { return owner.storage.serializationGlobals.loadingDepth; }, set loadingDepth(value) { owner.storage.serializationGlobals.loadingDepth = value; }, get conversion() { return owner.conversion.current().value; }, set conversion(value) { owner.conversion.current().value = value; } }; }
    // 정상/예외 모두 동기 대여 참조를 바깥에서 읽지 못하도록 비운다.
    leave() { this.fields = this.memory = null; this.active = false; }
    // 실제 API와 callback의 현재 물리 변경만 공급한다.
    hooks() { return {
        manager: (slot) => { assert.equal(slot, 8); assert.equal(this.storage.serializationGlobals.loadingDepth, 4); this.trace.push('manager'); return get(this.storage.headers(), slot * 20); },
        select: (manager, name, writable) => { assert.equal(manager, get(this.storage.headers(), 20)); assert.equal(name, 'TypeNames'); assert.equal(writable, 1); this.trace.push('select'); this.selected = this.storage.selectSection(8, name, writable); return this.selected.section; },
        stream: () => assert.fail('동기 access에서 legacy stream을 쓰지 않는다'), buffer: () => assert.fail('동기 access에서 legacy buffer를 쓰지 않는다'),
        access: (token, operation) => {
            assert.equal(token, this.selected.section); assert.equal(this.active, false); assert.equal(this.storage.currentSection(1), 'TypeNames'); this.trace.push('access+');
            try { this.storage.accessSection(this.selected, (fields, memory) => {
                this.active = true; this.fields = fields; this.memory = memory; this.observedFields = fields;
                if (get(fields, 0)) this.observedData = memory.buffer(get(fields, 0));
                try { operation(fields, memory); assert.equal(fields, this.observedFields); } finally { this.leave(); }
            }); } catch (error) { this.leave(); this.trace.push('access-throw'); throw error; }
            this.trace.push('access-'); assert.equal(this.active, false);
        },
        allocate: (size) => { assert.equal(this.active, true); assert.equal(get(this.fields, 4), 1); this.trace.push('allocate'); if (this.zeroEndAndCount) { put(this.fields, 8, 0); this.memory.buffer(get(this.fields, 0))[0] = 0; } return this.conversion.allocate(size); },
        conversionBuffer: (token) => { assert.equal(this.active, true); return this.conversion.bytes(token); }, typeCount: () => { assert.equal(this.active, true); return 3; },
        typeName: (type) => { assert.equal(this.active, true); assert.equal(this.fields, this.observedFields); assert.equal(this.storage.serializationGlobals.loadingDepth, 4); this.trace.push(`type${type}`);
            if (type === 1 && this.changeCurrent) this.storage.findSection(1, 'Scratch');
            if (type === 1 && this.checkGuards) { assert.throws(() => this.storage.accessSection(this.selected, () => {})); assert.throws(() => this.storage.createSection(1, 'TypeNames')); assert.throws(() => this.storage.renameSection(1, 'Renamed', 'TypeNames')); }
            if (type === 1 && this.changeStoredHash) put(this.memory.buffer(get(this.fields, 0)), 1, BETA);
            if (type === 2 && this.stopAtBeta) throw new TypeStop('TYPE2 명시 중단'); return this.names[type];
        },
        assertion: () => assert.fail('기존 conversion이 없는 장면이다'), loadingDepthChanged: (value) => { assert.equal(this.active, false); assert.equal(value, this.storage.serializationGlobals.loadingDepth); this.trace.push(`depth${value}`); },
    }; }
    // 새로운 정상 접근 안에서만 같은 필드/버퍼를 읽고 숫자만 반환한다.
    observe() { let values; this.storage.accessSection(this.selected, (fields, memory) => { assert.equal(fields.buffer, this.observedFields.buffer); assert.equal(fields.byteOffset, this.observedFields.byteOffset); assert.equal(fields.byteLength, this.observedFields.byteLength); const data = get(fields, 0), bytes = data ? memory.buffer(data) : null; if (bytes) { assert.equal(bytes.buffer, this.observedData.buffer); assert.equal(bytes.byteOffset, this.observedData.byteOffset); } values = [data, get(fields, 4), get(fields, 8), bytes?.[0], bytes && bytes.length >= 5 ? get(bytes, 1) : null]; }); return values; }
}

test('FortTypeNames access: 실제 두 slot이 같은 manager/fields를 빌리고 현재 노드 변경·중첩 guard 뒤에도 매핑을 보존한다', () => {
    const scene = new StorageDevices(); scene.changeCurrent = scene.checkGuards = true; loadFortTypeNames(8, scene.globals(), scene.hooks());
    assert.equal(scene.storage.serializationGlobals.loadingDepth, 3); assert.equal(scene.storage.currentSection(1), 'Scratch'); assert.equal(scene.storage.currentSection(8), 'Scratch'); const values = scene.observe(); assert.ok(values[0]); assert.deepEqual(values.slice(1, 3), [1, 9]); assert.deepEqual([...scene.conversion.bytes(scene.conversion.current().value).subarray(0, 3)], [1, 2, 0]);
    assert.deepEqual(scene.trace, ['depth4', 'manager', 'select', 'access+', 'allocate', 'type0', 'type1', 'type2', 'access-', 'depth3']); assert.equal(scene.fields, null); assert.equal(scene.memory, null);
});
test('FortTypeNames access: 할당 callback이 end/count를0으로 바꿔도 캡처count2와 같은 end 확장을 보존한다', () => {
    const scene = new StorageDevices(); scene.zeroEndAndCount = true; loadFortTypeNames(8, scene.globals(), scene.hooks()); const values = scene.observe(); assert.deepEqual(values.slice(1, 4), [1, 1, 0]); assert.deepEqual([...scene.conversion.bytes(scene.conversion.current().value).subarray(0, 2)], [1, 2]);
    const logical = scene.storage.template(1).sections.find((bytes) => bytes.buffer === scene.observedData.buffer); assert.equal(logical.length, 1);
});
test('FortTypeNames access: TYPE callback의 실제 버퍼 hash 변경을 사본 없이 읽는다', () => {
    const scene = new StorageDevices(); scene.changeStoredHash = true; loadFortTypeNames(8, scene.globals(), scene.hooks()); assert.deepEqual([...scene.conversion.bytes(scene.conversion.current().value).subarray(0, 2)], [2, 0]); assert.equal(scene.observe()[4], BETA); assert.equal(scene.storage.serializationGlobals.loadingDepth, 3);
});
test('FortTypeNames access: 부분 실패는 앞선 매핑/pos를 유지하고 실제 guard 해제 뒤 별도 unwind만 깊이를 반납한다', () => {
    const scene = new StorageDevices(); scene.stopAtBeta = true; const effects = scene.hooks(); assert.throws(() => loadFortTypeNames(8, scene.globals(), effects), TypeStop); assert.equal(scene.storage.serializationGlobals.loadingDepth, 4); assert.equal(scene.active, false); assert.equal(scene.fields, null); assert.equal(scene.memory, null); assert.equal(scene.observe()[1], 1);
    assert.deepEqual([...scene.conversion.bytes(scene.conversion.current().value).subarray(0, 2)], [1, 0]); assert.ok(scene.storage.createSection(1, 'AfterFailure').section); unwindFortTypeNamesLoad(scene.globals(), effects); assert.equal(scene.storage.serializationGlobals.loadingDepth, 3); assert.ok(scene.trace.includes('access-throw')); assert.equal(scene.trace.includes('access-'), false);
});
test('FortTypeNames access: 실제 빈 섹션도 Select/Access를 거치며 기존 할당은 free 없이 현재값만0으로 쓴다', () => {
    const scene = new StorageDevices(true), previous = scene.conversion.allocate(255); scene.conversion.current().value = previous; scene.conversion.bytes(previous).fill(0x9a); const effects = scene.hooks();
    // 빈 경로에서 선택되지 않는 모든 변환/TYPE 효과를 제거한다.
    for (const name of ['allocate', 'conversionBuffer', 'typeCount', 'typeName', 'assertion']) effects[name] = null;
    loadFortTypeNames(8, scene.globals(), effects); assert.equal(scene.storage.serializationGlobals.loadingDepth, 3); assert.equal(scene.conversion.current().value, 0); assert.ok(scene.conversion.bytes(previous).every((x) => x === 0x9a)); assert.equal(scene.observe()[1], 0); assert.deepEqual(scene.trace, ['depth4', 'manager', 'select', 'access+', 'access-', 'depth3']);
});
test('FortTypeNames access: 같은 현재 변환표를 실제 Contents 원시 reader가 소비하고 명시 해제 뒤 재조회는 실패한다', () => {
    const scene = new StorageDevices(); loadFortTypeNames(8, scene.globals(), scene.hooks()); const token = scene.conversion.current().value;
    const stream = scene.storage.createSection(1, 'Objects'); scene.storage.writeSection(stream, Uint8Array.of(1, 1)); const calls = [];
    // 같은 Template stream과 현재 토큰을 읽고 최종 가상 생성 요청만 관측한다. 전체 Session/Factory 생성 성공을 대신하지 않는다.
    const globals = { get conversion() { return scene.conversion.current().value; }, checking: () => 1 }, hooks = {
        access: (selected, operation) => scene.storage.accessSection(selected, operation), conversionBuffer: (current) => scene.conversion.bytes(current),
        type: () => ({ flags1: 0, contentListFlags: 4 }), create: (...args) => { calls.push(args); return 123; },
    };
    const selected = scene.storage.selectSection(1, 'Objects', 1); assert.equal(loadFortContents(50, selected, 9, 0, globals, hooks), true); assert.deepEqual(calls, [[50, 2, 0, 4, 9, 0]]);
    scene.conversion.release(token); assert.equal(scene.conversion.current().value, token);
    assert.throws(() => loadFortContents(50, scene.storage.selectSection(1, 'Objects', 1), 9, 0, globals, hooks), /없는 실제 주소/); assert.equal(calls.length, 1); scene.conversion.current().value = 0;
});
