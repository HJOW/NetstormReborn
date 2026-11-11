// Template manager 생성자와 기존 파일을 유지하는 바깥 Load(cpppj `o/RawTemplateLoad`, 원본 004bcc60 · 004bcca0 · 004bf550)와 Client 이름표 소유자(`client/ClientTemplateSectionNames`)의
// 웹 대응 `core/raw-template-load.js` · `core/client-template-section-names.js` 를 검사한다(4-B, WB04-E281).
//  - cpppj TemplateLoadTests 5건(생성자/빌린 파일명, 기존 파일의 헤더 보존, 새 읽기 전용·현재 제목 분기, 실패 뒤 포인터 재조회, NULL/미선택 효과 경계)을 호출 순서 기록(trace)까지 그대로 옮겼다.
//    cpppj 에서 이 5건은 실제로 통과함을 확인했다(netstorm_tests.exe --filter TemplateLoad_, 게임 실행 아님).
//  - cpppj DataManagerInitializeTests 의 ClientTemplateSectionNames 2건(최초 512 할당·재사용·가변·미정 끝 235BYTE 불변, 할당 0 은 물리 실패) 외에, 소유자 계약(복사 없는 같은 배열·할당 실패 뒤 재시도·잘못된 할당 거부)을 직접 검사한다.
//  - 계약: fixture 가 없는 hand test 단위라 변형 시험으로 보강했다(최초 실패와 교정은 단위 문서).
// 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E281)
import test from 'node:test';
import assert from 'node:assert/strict';
import { constructTemplateDataManager, loadTemplateFile, templateDataManagerFileName } from '../www/core/raw-template-load.js';
import { ClientTemplateSectionNames } from '../www/core/client-template-section-names.js';
import { readSnartName } from '../www/core/raw-snart.js';

// 비정렬 DWORD 읽기/쓰기(cpppj Get/Put)이다. (2026-11-11 WB04-E281)
function get(bytes, at) {
    return (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
}
function put(bytes, at, value) {
    for (let index = 0; index < 4; index += 1) bytes[at + index] = (value >>> (8 * index)) & 255;
}

// 파일명 뷰(BYTE)를 글자로 바꾼다. (2026-11-11 WB04-E281)
const text = (view) => String.fromCharCode(...view);

// 예외가 나는지 본다. (2026-11-11 WB04-E281)
function throws(action) {
    try {
        action();
        return false;
    } catch {
        return true;
    }
}

// 생성자가 쓰지 않는 이름 뒤 공간과 양쪽 경계는 별도 입력 값 그대로 남아야 한다(cpppj TemplateLoad_ConstructorAndBorrowedFileName). (2026-11-11 WB04-E281)
test('Template 생성자: 지정 DWORD 와 이름만 쓰고 이름 뒤/양쪽 경계는 보존하며 파일명은 빌린 뷰이다', () => {
    const bytes = new Uint8Array(284).fill(0xa7); // 미정 tail 을 0 으로 가정하지 않는 명시 입력이다.
    const manager = bytes.subarray(4, 280);
    const global = Uint8Array.of(1, 2, 3, 4);
    constructTemplateDataManager(manager, 'MiXeD.fort', global);
    assert.equal(get(manager, 0), 0x514690);
    assert.equal(get(manager, 4), 0);
    assert.equal(get(manager, 8), 0);
    assert.equal(get(manager, 0x10c), 0);
    assert.equal(get(manager, 0x110), 0xffffffff);
    assert.ok(global.every((value) => value === 0));
    assert.equal(text(templateDataManagerFileName(manager)), 'MiXeD.fort');
    assert.equal(manager[22], 0);
    assert.equal(manager[23], 0xa7);
    assert.equal(manager[267], 0xa7);
    assert.equal(get(bytes, 0), 0xa7a7a7a7);
    assert.equal(get(bytes, 280), 0xa7a7a7a7);
    // 이름이 바뀌면 뷰도 같은 BYTE 를 본다.
    const view = templateDataManagerFileName(manager);
    manager[12] = 'm'.charCodeAt(0);
    assert.equal(text(view), 'miXeD.fort');
    // 원본 C 문자열처럼 첫 NUL 까지만 쓴다.
    constructTemplateDataManager(manager, 'x\0ignored', global);
    assert.equal(text(templateDataManagerFileName(manager)), 'x');
    assert.equal(manager[14], 'X'.charCodeAt(0));
    // 최대 255 글자는 쓰고 256 글자는 거부한다.
    const maximum = 'q'.repeat(255);
    constructTemplateDataManager(manager, maximum, global);
    assert.equal(text(templateDataManagerFileName(manager)), maximum);
    assert.equal(manager[267], 0);
    assert.ok(throws(() => constructTemplateDataManager(manager, 'q'.repeat(256), global)));
    assert.ok(throws(() => templateDataManagerFileName(manager.subarray(0, 275))));
    manager[267] = 'q'.charCodeAt(0);
    assert.ok(throws(() => templateDataManagerFileName(manager)));
});

// 계약: 입력 모양 거부와 BYTE 배열 이름(NUL 포함)·Latin-1 밖 글자 거부. (2026-11-11 WB04-E281)
test('Template 생성자 계약: manager/전역/이름 모양을 쓰기 전에 거부한다', () => {
    const manager = new Uint8Array(276).fill(0x55);
    const global = Uint8Array.of(1, 2, 3, 4);
    assert.ok(throws(() => constructTemplateDataManager(manager.subarray(0, 275), 'a', global)));
    assert.ok(throws(() => constructTemplateDataManager(manager, 'a', Uint8Array.of(1, 2, 3))));
    assert.ok(throws(() => constructTemplateDataManager(manager, 'é한', global))); // 한 BYTE 를 넘는 글자
    assert.ok(throws(() => constructTemplateDataManager(manager, 42, global)));
    assert.ok(manager.every((value) => value === 0x55) && global.join() === '1,2,3,4'); // 거부는 아무것도 쓰지 않는다
    // Latin-1 글자(0xe9)와 BYTE 배열 이름은 그대로 쓴다.
    constructTemplateDataManager(manager, 'é', global);
    assert.deepEqual([...templateDataManagerFileName(manager)], [0xe9]);
    constructTemplateDataManager(manager, Uint8Array.of(0x41, 0x42, 0, 0x43), global);
    assert.equal(text(templateDataManagerFileName(manager)), 'AB');
});

// 이미 파일을 가진 헤더는 같음/다름 어느 경우에도 할당/읽기/해제를 하지 않는다(cpppj TemplateLoad_ExistingFileKeepsAllHeaderFields). (2026-11-11 WB04-E281)
test('Template Load: 기존 파일을 가진 헤더는 모든 필드를 보존하고 이름만 비교한다', () => {
    const header = new Uint8Array(20).fill(0xa7);
    put(header, 0, 0x1234);
    const before = header.slice();
    let names = 0;
    let compares = 0;
    let diagnostics = 0;
    const hooks = {
        fileName: (manager) => {
            assert.equal(manager, 0x1234);
            names += 1;
            return 'Keep.fort';
        },
        sameName: (first, second) => {
            assert.equal(first, 'Keep.fort');
            assert.equal(second, 'KEEP.FORT');
            compares += 1;
            return compares === 1;
        },
        wrongName: () => { diagnostics += 1; },
    };
    loadTemplateFile(header, 'KEEP.FORT', hooks);
    assert.deepEqual(header, before);
    assert.deepEqual([names, compares, diagnostics], [1, 1, 0]);
    loadTemplateFile(header, 'KEEP.FORT', hooks);
    assert.deepEqual(header, before);
    assert.deepEqual([names, compares, diagnostics], [2, 2, 1]);
});

// 읽기 전용 사전 설정은 파일 판정을 건너뛰되 1 로 정규화하고 성공 시 현재 슬롯만 제목을 갱신한다(cpppj TemplateLoad_NewReadOnlyAndCurrentTitleBranches). (2026-11-11 WB04-E281)
test('Template Load: 새 파일의 읽기 전용 사전 설정·판정·현재 슬롯 제목 분기와 호출 순서', () => {
    // 네 입력은 preset 비영/영과 판정 true/false 및 현재 슬롯 true/false 를 독립해서 고른다.
    for (let variant = 0; variant < 4; variant += 1) {
        const header = new Uint8Array(20);
        put(header, 4, 0xaabbccdd);
        put(header, 8, variant === 3 ? 0x87654321 : 0);
        put(header, 12, 0x7391);
        put(header, 16, 0x12345678);
        let trace = '';
        const hooks = {
            allocate: (size) => {
                assert.equal(size, 276);
                assert.equal(get(header, 4), 0);
                assert.equal(get(header, 0), 0);
                trace += 'A';
                return 0x101;
            },
            construct: (manager, name) => {
                assert.equal(manager, 0x101);
                assert.equal(name, 'new.fort');
                assert.equal(get(header, 0), 0);
                trace += 'C';
                return 0x202; // 생성자 EAX 가 헤더에 설치되는 것을 구별한다.
            },
            fileName: (manager) => {
                assert.equal(manager, 0x202);
                assert.equal(get(header, 0), 0x202);
                trace += 'N';
                return 'new.fort';
            },
            readOnly: (name) => {
                assert.equal(name, 'new.fort');
                assert.equal(get(header, 16), 0x12345678);
                trace += 'R';
                return variant === 1;
            },
            readFile: (manager) => {
                assert.equal(manager, 0x202);
                assert.equal(get(header, 16), 0);
                assert.equal(get(header, 12), 0x7391);
                assert.equal(get(header, 8), variant === 1 || variant === 3 ? 1 : 0);
                trace += 'F';
                return true;
            },
            isCurrent: () => {
                trace += 'I';
                return variant !== 2;
            },
            refreshTitle: () => { trace += 'T'; },
        };
        loadTemplateFile(header, 'new.fort', hooks);
        assert.equal(trace, variant === 3 ? 'ACFIT' : variant === 2 ? 'ACNRFI' : 'ACNRFIT', `변형 ${variant}`);
        assert.equal(get(header, 0), 0x202);
        assert.equal(get(header, 4), 0);
        assert.equal(get(header, 12), 0x7391);
    }
});

// 실제 read/소멸 효과가 ptr 를 바꾸어도 실패의 free 는 소멸 직전 저장 포인터를 쓴다(cpppj TemplateLoad_FailureRereadsPointerAndPreservesOtherFields). (2026-11-11 WB04-E281)
test('Template Load: 읽기 실패는 소멸 직전 포인터를 재조회해 destroy→release 하고 포인터만 0 으로 쓴다', () => {
    const header = new Uint8Array(20);
    put(header, 8, 4);
    put(header, 12, 0x7391); // readonly preset 은 미선택 이름/판정 경계를 요구하지 않는다.
    let trace = '';
    const hooks = {
        allocate: () => 11,
        construct: (pointer) => pointer,
        readFile: (pointer) => {
            assert.equal(pointer, 11);
            trace += 'F';
            put(header, 0, 22);
            put(header, 4, 1);
            put(header, 8, 7);
            put(header, 16, 9);
            return false;
        },
        lifetime: {
            destroy: (pointer) => {
                assert.equal(pointer, 22);
                trace += 'D';
                put(header, 0, 33);
                put(header, 12, 0x8273);
            },
            release: (pointer) => {
                assert.equal(pointer, 22);
                assert.equal(get(header, 0), 33);
                trace += 'R';
                put(header, 16, 10);
            },
        },
    };
    loadTemplateFile(header, 'failed.fort', hooks);
    assert.equal(trace, 'FDR');
    assert.deepEqual([get(header, 0), get(header, 4), get(header, 8), get(header, 12), get(header, 16)], [0, 1, 7, 0x8273, 10]);
    // 실패 뒤 이미 NULL 이면 소멸/free 를 선택하지 않는다.
    hooks.readFile = () => {
        put(header, 0, 0);
        return false;
    };
    trace = '';
    loadTemplateFile(header, 'failed.fort', hooks);
    assert.equal(trace, '');
    assert.equal(get(header, 0), 0);
});

// 읽기 성공 뒤 ptr 가 NULL 이면 current/title 을 조회하지 않고 NULL 할당도 임의 성공으로 바꾸지 않는다(cpppj TemplateLoad_NullAndUnselectedEffectBoundaries). (2026-11-11 WB04-E281)
test('Template Load: NULL 포인터·미선택 효과 경계와 헤더 크기 거부', () => {
    const header = new Uint8Array(20);
    put(header, 8, 1);
    const hooks = {
        allocate: () => 1,
        construct: (pointer) => pointer,
        readFile: (pointer) => {
            assert.equal(pointer, 1);
            put(header, 0, 0);
            return true;
        },
    };
    loadTemplateFile(header, 'input.fort', hooks);
    assert.equal(get(header, 0), 0);
    header.fill(0);
    put(header, 4, 7);
    hooks.allocate = () => 0;
    delete hooks.construct;
    hooks.readOnly = () => false;
    hooks.fileName = (pointer) => {
        assert.equal(pointer, 0);
        throw new Error('명시 NULL manager 가상 호출 거부');
    };
    assert.ok(throws(() => loadTemplateFile(header, 'input.fort', hooks)));
    assert.equal(get(header, 0), 0);
    assert.equal(get(header, 4), 0);
    assert.ok(throws(() => loadTemplateFile(header.subarray(0, 19), 'input.fort', hooks)));
    assert.ok(throws(() => loadTemplateFile(new Uint8Array(21), 'input.fort', hooks)));
    // 헤더 크기가 맞지 않으면 모든 장치가 갖춰져 있어도 어떤 장치도 부르기 전에 거부한다.
    const called = [];
    const full = {
        fileName: () => { called.push('N'); return 'x'; }, sameName: () => { called.push('S'); return true; }, allocate: () => { called.push('A'); return 1; },
        construct: () => { called.push('C'); return 1; }, readOnly: () => { called.push('R'); return false; }, readFile: () => { called.push('F'); return true; },
    };
    for (const size of [0, 19, 21]) assert.ok(throws(() => loadTemplateFile(new Uint8Array(size), 'x', full)), `헤더 ${size}`);
    assert.deepEqual(called, []);
    // NULL 할당은 생성자를 부르지 않고 0 포인터로 이름 조회·판정·읽기를 이어가며, 실패해도 소멸할 포인터가 없다.
    const nullHeader = new Uint8Array(20);
    put(nullHeader, 4, 5);
    const order = [];
    loadTemplateFile(nullHeader, 'x', {
        allocate: () => { order.push('A'); return 0; },
        construct: () => { order.push('C'); return 99; },
        fileName: (manager) => { order.push(`N${manager}`); return 'x'; },
        readOnly: () => { order.push('R'); return false; },
        readFile: (manager) => { order.push(`F${manager}`); return false; },
    });
    assert.deepEqual(order, ['A', 'N0', 'R', 'F0']);
    assert.deepEqual([get(nullHeader, 0), get(nullHeader, 4), get(nullHeader, 8)], [0, 0, 0]);
});

// 계약: 기존 파일 분기는 도달한 장치만 요구하고, 새 파일 분기는 할당 장치부터 요구하며, 읽기 성공 때 제목 장치는 현재 슬롯일 때만 요구한다. (2026-11-11 WB04-E281)
test('Template Load 계약: 선택된 분기가 요구하는 장치만 그 호출 위치에서 요구한다', () => {
    // 기존 파일: fileName 만 없으면 거부, 같은 이름이면 sameName 만 있으면 충분하다.
    const existing = new Uint8Array(20);
    put(existing, 0, 5);
    assert.ok(throws(() => loadTemplateFile(existing, 'a', {})));
    loadTemplateFile(existing, 'a', { fileName: () => 'a', sameName: () => true });
    // 다른 이름인데 진단 장치가 없으면 거부한다.
    assert.ok(throws(() => loadTemplateFile(existing, 'a', { fileName: () => 'b', sameName: () => false })));
    // 새 파일: 할당 장치가 없으면 헤더 +4 를 0 으로 쓴 뒤 거부한다.
    const fresh = new Uint8Array(20);
    put(fresh, 4, 9);
    assert.ok(throws(() => loadTemplateFile(fresh, 'a', {})));
    assert.equal(get(fresh, 4), 0);
    // 읽기 성공 + 현재 슬롯이 아니면 제목 장치를 요구하지 않는다.
    const readOnlyPreset = new Uint8Array(20);
    put(readOnlyPreset, 8, 1);
    loadTemplateFile(readOnlyPreset, 'a', { allocate: () => 7, construct: (pointer) => pointer, readFile: () => true, isCurrent: () => false });
    assert.equal(get(readOnlyPreset, 0), 7);
    // 현재 슬롯인데 제목 장치가 없으면 거부한다.
    const titled = new Uint8Array(20);
    put(titled, 8, 1);
    assert.ok(throws(() => loadTemplateFile(titled, 'a', { allocate: () => 7, construct: (pointer) => pointer, readFile: () => true, isCurrent: () => true })));
    // 읽기 실패인데 소멸/반납 장치가 없으면 거부하고 포인터를 쓰지 않는다.
    const failing = new Uint8Array(20);
    put(failing, 8, 1);
    assert.ok(throws(() => loadTemplateFile(failing, 'a', { allocate: () => 7, construct: (pointer) => pointer, readFile: () => false })));
    assert.equal(get(failing, 0), 7);
});

// 알려진 할당 입력을 사용해 277BYTE 밖 235BYTE 미작성과 동일 배열/변환 상태를 검사한다(cpppj ClientTemplateSectionNames_LazyMutableAndUnknownTail). (2026-11-11 WB04-E281)
test('ClientTemplateSectionNames: 최초에만 512 할당·같은 배열 재사용·제자리 변환 유지·미정 끝 235BYTE 불변', () => {
    let calls = 0;
    const names = new ClientTemplateSectionNames((size) => {
        calls += 1;
        assert.equal(size, 512);
        return new Uint8Array(size).fill(0xa5);
    });
    assert.equal(names.currentToken(), 0);
    assert.equal(calls, 0);
    const bytes = names.names();
    assert.equal(calls, 1);
    assert.equal(names.currentToken(), 1);
    assert.ok(bytes.subarray(277).every((value) => value === 0xa5));
    assert.equal(bytes[0], '/'.charCodeAt(0));
    assert.equal(bytes[275], '/'.charCodeAt(0));
    assert.equal(bytes[276], 0);
    // SNART 이름 선택은 같은 표를 제자리 변환한다(첫 BYTE 0, 마지막 0xff 가 남는다).
    assert.equal(readSnartName(bytes, 0, null), 'Subscriber');
    assert.equal(readSnartName(bytes, 34, null), 'Terr19');
    assert.equal(readSnartName(bytes, 35, null), '');
    assert.equal(bytes[0], 0);
    assert.equal(bytes[276], 0xff);
    assert.strictEqual(names.names(), bytes);
    assert.equal(calls, 1);
    assert.equal(bytes[0], 0);
    assert.ok(bytes.subarray(277).every((value) => value === 0xa5));
});

// 실제 할당 0 은 물리 실패로 남으며 같은 current 0 을 보존한다(cpppj ClientTemplateSectionNames_AllocationZeroIsFailure). (2026-11-11 WB04-E281)
test('ClientTemplateSectionNames: 할당 실패는 토큰 0 을 보존하고 다시 부르면 다시 할당을 시도한다', () => {
    let calls = 0;
    let fail = true;
    const names = new ClientTemplateSectionNames((size) => {
        calls += 1;
        assert.equal(size, 512);
        return fail ? null : new Uint8Array(size);
    });
    assert.ok(throws(() => names.names()));
    assert.equal(names.currentToken(), 0);
    assert.equal(calls, 1);
    assert.ok(throws(() => names.names()));
    assert.equal(calls, 2);
    fail = false;
    const bytes = names.names();
    assert.equal(calls, 3);
    assert.equal(names.currentToken(), 1);
    assert.equal(bytes[0], '/'.charCodeAt(0));
    // 성공한 뒤에는 더 할당하지 않는다.
    assert.strictEqual(names.names(), bytes);
    assert.equal(calls, 3);
});

// 계약: 기본 할당 장치(0 으로 채운 512BYTE)·두 소유자의 독립·잘못된 할당 거부. (2026-11-11 WB04-E281)
test('ClientTemplateSectionNames 계약: 기본 할당·소유자 독립·잘못된 할당/장치 거부', () => {
    const first = new ClientTemplateSectionNames();
    const bytes = first.names();
    assert.equal(bytes.length, 512);
    assert.ok(bytes.subarray(277).every((value) => value === 0));
    // 두 소유자는 서로 다른 배열이다(각자 Client 전역).
    const second = new ClientTemplateSectionNames();
    assert.notStrictEqual(second.names(), bytes);
    // 길이가 512 가 아닌 할당과 배열이 아닌 반환은 물리 계약 위반이라 거부하고 토큰을 만들지 않는다.
    const short = new ClientTemplateSectionNames(() => new Uint8Array(511));
    assert.ok(throws(() => short.names()));
    assert.equal(short.currentToken(), 0);
    const odd = new ClientTemplateSectionNames(() => [1, 2, 3]);
    assert.ok(throws(() => odd.names()));
    assert.equal(odd.currentToken(), 0);
    assert.ok(throws(() => new ClientTemplateSectionNames(42)));
});
