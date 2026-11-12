// Client 수명 전역 두 묶음(ClientFortGlobals·ClientCoreDataGlobals)의 계약을 cpppj 시험과 같은 장면으로 대조한다(WB04-E331, 4-D).
//  - cpppj `FortNameDevices_ClientBuffersAndPendingPhysicalCopy`(Client 이름 버퍼·파일 키 부분)·`ClientCoreRecords_OneOwnerHasSeparateRecordsAndLastKnown`·`ClientCoreRecords_CoreSelectionDoesNotResetSavedRecords` 이식.
//  - 웹 계약: NUL 경계, 키 버퍼 주소 고정, dirty/모드 상자, borrow 의 살아 있는 접근자, 읽기 진단 복귀, 복원 경로.
// (2026-11-12 WB04-E331: 4-D 전역 소유자 두 개 이식과 함께 추가)
import assert from 'node:assert/strict';
import test from 'node:test';
import { ClientFortGlobals, FILE_SPEC_BYTES, FORT_NAME_BYTES, SoloMapFileSpec } from '../www/core/client-fort-globals.js';
import { ClientCoreDataGlobals } from '../www/core/client-core-data-globals.js';
import { PlayerWorldState } from '../www/core/raw-player-world.js';

// 문자열을 이름 버퍼에 NUL 종료로 쓴다(한 글자 = 한 바이트). (2026-11-12 WB04-E331)
function writeName(buffer, text) {
    buffer.fill(0);
    // 글자마다 한 바이트를 쓴다.
    for (let index = 0; index < text.length; index += 1) buffer[index] = text.charCodeAt(index);
}

// ---------------------------------------------------------------- ClientFortGlobals

// 새 소유자: 두 이름 빈 문자열·서로 다른 256BYTE 버퍼·목록 dirty 1·모드 0·기본 키, 접근자는 같은 객체를 돌려준다.
test('Fort 전역: 초기값과 안정된 참조', () => {
    const fort = new ClientFortGlobals();
    assert.deepEqual([fort.current(), fort.pending(), fort.catalogDirty().value, fort.selectionMode().value], ['', '', 1, 0]);
    assert.deepEqual([fort.currentBytes().length, fort.pendingBytes().length, FORT_NAME_BYTES], [256, 256, 256]);
    assert.notEqual(fort.currentBytes(), fort.pendingBytes());
    assert.notEqual(fort.currentBytes().buffer, fort.pendingBytes().buffer);
    assert.deepEqual([fort.fileSpec().current(), fort.selectedFileSpec()], ['fortSpec', 'fortSpec']);
    for (const name of ['currentBytes', 'pendingBytes', 'fileSpec', 'catalogDirty', 'selectionMode']) assert.equal(fort[name](), fort[name](), name);
    // 두 소유자는 서로 독립이다.
    const other = new ClientFortGlobals();
    assert.notEqual(other.currentBytes(), fort.currentBytes());
    assert.notEqual(other.fileSpec(), fort.fileSpec());
});

// 현재 이름과 예약 이름은 서로 복사되지 않는 별개 물리이다.
test('Fort 전역: 현재/예약 이름은 서로 복사하지 않는다', () => {
    const fort = new ClientFortGlobals();
    writeName(fort.currentBytes(), 'Current');
    writeName(fort.pendingBytes(), 'Pending');
    assert.deepEqual([fort.current(), fort.pending()], ['Current', 'Pending']);
    // 예약 이름을 바꿔도 현재 이름과 두 주소는 그대로이다.
    const current = fort.currentBytes();
    const pending = fort.pendingBytes();
    writeName(fort.pendingBytes(), 'OK');
    assert.deepEqual([fort.current(), fort.pending(), fort.currentBytes() === current, fort.pendingBytes() === pending], ['Current', 'OK', true, true]);
});

// 읽기는 첫 NUL 까지이며 NUL 뒤 미작성 바이트를 읽지 않고, NUL 이 없는 버퍼는 빈 문자열로 가장하지 않고 범위 오류로 거부한다.
test('Fort 전역: 이름의 NUL 경계', () => {
    const fort = new ClientFortGlobals();
    fort.currentBytes().set([0x42, 0x61, 0x64, 0x2e, 0, 0x71, 0x72, 0x73]);
    assert.equal(fort.current(), 'Bad.');
    // 한 바이트가 한 글자이다(ANSI 상위 바이트 포함).
    fort.currentBytes().fill(0);
    fort.currentBytes().set([0xc0, 0xff, 0x41]);
    assert.equal(fort.current(), 'ÀÿA');
    // 전체가 채워져 NUL 이 없으면 거부하고 버퍼는 그대로이다.
    fort.pendingBytes().fill(0x41);
    assert.throws(() => fort.pending(), /NUL 물리 범위/);
    assert.ok(fort.pendingBytes().every((byte) => byte === 0x41));
    fort.currentBytes().fill(0x42);
    assert.throws(() => fort.current(), RangeError);
    // 마지막 칸만 NUL 이면 255글자 이름이다.
    fort.pendingBytes()[255] = 0;
    assert.equal(fort.pending().length, 255);
});

// 파일 키: 기본 fortSpec, SelectSolo 로 SOLOSpec, 같은 9BYTE 배열(주소 고정)이며 selectFileSpec 은 내용을 바꾸지 않는다.
test('Fort 전역: 파일 키와 선택 포인터', () => {
    const fort = new ClientFortGlobals();
    const spec = fort.fileSpec();
    const bytes = spec.bytes;
    assert.equal(bytes.length, FILE_SPEC_BYTES);
    assert.deepEqual([...bytes], [0x66, 0x6f, 0x72, 0x74, 0x53, 0x70, 0x65, 0x63, 0]);
    spec.selectSolo();
    assert.equal(spec.current(), 'SOLOSpec');
    assert.deepEqual([...bytes], [0x53, 0x4f, 0x4c, 0x4f, 0x53, 0x70, 0x65, 0x63, 0]);
    // 포인터를 다시 지정해도 키 내용은 보존되고 주소가 같다.
    fort.selectFileSpec();
    assert.deepEqual([fort.selectedFileSpec(), spec.bytes === bytes, fort.fileSpec() === spec], ['SOLOSpec', true, true]);
    spec.selectFort();
    assert.deepEqual([spec.current(), fort.selectedFileSpec(), bytes[8]], ['fortSpec', 'fortSpec', 0]);
    // 독립 키 객체도 같은 기본값이다.
    assert.equal(new SoloMapFileSpec().current(), 'fortSpec');
});

// 목록 dirty 와 선택 모드는 DWORD 상자이다.
test('Fort 전역: dirty 와 모드 상자의 DWORD 폭', () => {
    const fort = new ClientFortGlobals();
    fort.catalogDirty().value = 0;
    fort.selectionMode().value = -1;
    assert.deepEqual([fort.catalogDirty().value, fort.selectionMode().value], [0, 0xffffffff]);
    fort.catalogDirty().value = 2 ** 32 + 3;
    assert.equal(fort.catalogDirty().value, 3);
});

// ---------------------------------------------------------------- ClientCoreDataGlobals

// 최초 CRT 뒤 장부 144BYTE 와 lastKnown 10BYTE 는 별도 고정 물리이고 metadata 가 같은 장부를 직접 갱신한다.
test('CoreData 전역: 장부와 lastKnown 은 별개이고 metadata 가 장부를 직접 쓴다', () => {
    const owner = new ClientCoreDataGlobals();
    const records = owner.records();
    // 새 소유자의 현재 포인터는 NULL, 활성 0, Player 뷰는 빈 뷰이다.
    assert.deepEqual([owner.current, owner.active, owner.players.length], [null, 0, 0]);
    assert.deepEqual([records.length, owner.cached().length, owner.records() === records, owner.cached() === records], [144, 10, true, false]);
    assert.notEqual(owner.cached().buffer, records.buffer);
    assert.ok(records.every((byte) => byte === 0));
    assert.ok(owner.cached().every((byte) => byte === 0));
    // 끝 padding 두 바이트와 lastKnown 을 먼저 표시한다.
    records[142] = 0x5a;
    records[143] = 0x6b;
    owner.cached()[0] = 0x7c;
    const core = Uint8Array.of(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    owner.storeMetadata(8, 0x12345678, core);
    core.fill(0);
    // owner 8 의 기록(128..143): ID DWORD → Core 10BYTE, padding 보존, lastKnown 불변.
    assert.deepEqual([...records.subarray(128, 132)], [0x78, 0x56, 0x34, 0x12]);
    assert.deepEqual([...records.subarray(132, 142)], [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]);
    assert.deepEqual([records[142], records[143], owner.cached()[0], owner.records() === records], [0x5a, 0x6b, 0x7c, true]);
});

// Select(owner 0)는 현재 10BYTE 를 lastKnown 에 4/4/2 로 복사하고 현재를 NULL 로 하되 9기록 장부를 복제/초기화하지 않는다. 새 소유자만 새로 만든다.
test('CoreData 전역: 선택 0 은 저장된 장부를 초기화하지 않는다', () => {
    const owner = new ClientCoreDataGlobals();
    const core = Uint8Array.of(10, 9, 8, 7, 6, 5, 4, 3, 2, 1);
    owner.storeMetadata(1, 9, core);
    const before = Uint8Array.from(owner.records());
    owner.players = new Uint8Array(1620);
    owner.current = core;
    const player = new PlayerWorldState();
    owner.select(player, 0, 0, 1);
    assert.equal(owner.current, null);
    assert.deepEqual([...owner.cached()], [10, 9, 8, 7, 6, 5, 4, 3, 2, 1]);
    assert.deepEqual([...owner.records()], [...before]);
    // 오너 0 선택은 owner/template 두 값을 쓰되 known 과 active 는 건드리지 않고 active 만 0 이 된다.
    assert.deepEqual([player.battleOwner, player.templateOwner, owner.active], [0, 1, 0]);
    const next = new ClientCoreDataGlobals();
    assert.notEqual(next.records(), owner.records());
    assert.deepEqual([next.records()[16], owner.records()[16]], [0, 9]);
});

// borrow 는 Player 의 현재 속성과 이 소유자의 슬롯을 사본 없이 잇는 살아 있는 묶음이다. 비영 선택은 known 1 과 실제 Player 안의 10BYTE 별칭을 만든다.
test('CoreData 전역: 비영 선택과 살아 있는 borrow', () => {
    const owner = new ClientCoreDataGlobals();
    const players = new Uint8Array(1620);
    owner.players = players;
    const player = new PlayerWorldState();
    player.playerKnown = 0;
    let metamap = 0;
    const view = owner.borrow(player, () => metamap);
    owner.select(player, () => metamap, 3, 2);
    assert.deepEqual([player.battleOwner, player.templateOwner, player.playerKnown], [3, 2, 1]);
    // current 는 owner×0xb4+0x6c 부터의 10BYTE 별칭이다(같은 buffer·byteOffset).
    assert.equal(owner.current.buffer, players.buffer);
    assert.deepEqual([owner.current.byteOffset - players.byteOffset, owner.current.length], [3 * 0xb4 + 0x6c, 10]);
    owner.current[0] = 0x99;
    assert.equal(players[3 * 0xb4 + 0x6c], 0x99);
    // 살아 있는 묶음은 이후 변경을 그대로 본다.
    player.battleOwner = 7;
    metamap = 1;
    assert.deepEqual([view.templateOwner, view.battleOwner, view.metamap, view.current === owner.current, view.players === players, view.cached === owner.cached()], [2, 7, 1, true, true, true]);
    view.active = -1;
    assert.equal(owner.active, 0xffffffff);
    view.current = null;
    assert.equal(owner.current, null);
    // 미션 메타맵이 켜져 있으면 현재는 lastKnown 캐시이다.
    owner.select(player, () => metamap, 2, 2);
    assert.equal(owner.current, owner.cached());
    // 범위 밖 Player 물리는 host 뷰로 가장하지 않고 거부한다.
    metamap = 0;
    owner.players = new Uint8Array(100);
    assert.throws(() => owner.select(player, () => metamap, 5, 5), RangeError);
});

// metamap 인자 형식과 활성 DWORD 폭.
test('CoreData 전역: metamap 인자 형식과 active 폭', () => {
    const owner = new ClientCoreDataGlobals();
    const player = new PlayerWorldState();
    assert.equal(owner.borrow(player, 5).metamap, 5);
    const box = { value: 3 };
    const view = owner.borrow(player, box);
    box.value = 4;
    assert.equal(view.metamap, 4);
    assert.throws(() => owner.borrow(player, 'x'), TypeError);
    owner.active = 2 ** 32 + 9;
    assert.equal(owner.active, 9);
    owner.active = -2;
    assert.equal(owner.active, 0xfffffffe);
});

// 조회(read): metamap 이 켜져 있고 현재가 캐시가 아니면 진단(0x55) 뒤 현재 포인터를 다시 읽는다.
test('CoreData 전역: read 의 진단 복귀 뒤 현재 재조회', () => {
    const owner = new ClientCoreDataGlobals();
    const player = new PlayerWorldState();
    owner.players = new Uint8Array(1620);
    owner.current = new Uint8Array(10);
    const calls = [];
    const result = owner.read(player, 1, { assertion: (line) => { calls.push(line); owner.current = owner.cached(); } });
    assert.deepEqual(calls, [0x55]);
    assert.equal(result, owner.cached());
    // 진단 장치가 없으면 거부한다.
    owner.current = new Uint8Array(10);
    assert.throws(() => owner.read(player, 1, {}), /진단 장치 누락/);
    // metamap 이 0 이면 진단 없이 현재를 돌려준다.
    const current = new Uint8Array(10);
    owner.current = current;
    assert.equal(owner.read(player, 0, {}), current);
});

// 복원(restore): 선행 missionPost 가 비영이면 아무 것도 하지 않고, 영이면 같은 9기록 장부의 owner 기록을 Player 에 4/4/2 로 복원한다.
test('CoreData 전역: restore 는 같은 장부에서 Player Core 를 복원한다', () => {
    const owner = new ClientCoreDataGlobals();
    const players = new Uint8Array(1620);
    owner.players = players;
    const player = new PlayerWorldState();
    player.playerKnown = 1;
    player.battleOwner = 1;
    const core = Uint8Array.of(11, 12, 13, 14, 15, 16, 17, 18, 19, 20);
    owner.storeMetadata(2, 0x01020304, core);
    // 선행 관문: missionPost 비영이면 복원하지 않는다.
    owner.restore(player, 0, { missionPost: () => 1, savedGame: () => 0, checking: () => 0 }, 2, {});
    assert.ok(players.every((byte) => byte === 0));
    assert.equal(owner.active, 0);
    // missionPost 0, 원격 owner(2 ≠ battleOwner 1): 장부 owner 2 의 Core 가 Player 2 의 +0x6c 로 복사된다.
    let chat = 0;
    owner.restore(player, 0, { missionPost: () => 0, savedGame: () => 1, checking: () => 0 }, 2, { initializeChat: () => { chat += 1; } });
    // 저장 게임이면 Core 의 여섯 번째 BYTE(Player+0x71 저장 BYTE)를 복사 전 Player 값(여기서는 0)으로 되돌리고 Chat 을 다시 초기화한다. 원격 owner 라 active 는 쓰지 않는다.
    assert.deepEqual([...players.subarray(2 * 0xb4 + 0x6c, 2 * 0xb4 + 0x6c + 10)], [11, 12, 13, 14, 15, 0, 17, 18, 19, 20]);
    assert.deepEqual([chat, owner.active], [1, 0]);
    // 저장 게임이 아니면 Chat 장치를 요구하지 않는다.
    owner.restore(player, 0, { missionPost: () => 0, savedGame: () => 0, checking: () => 0 }, 2, {});
    // 로컬 owner(= battleOwner) 복원은 active 1 과 현재 포인터를 Player 안의 별칭으로 둔다.
    owner.storeMetadata(1, 0x0a0b0c0d, Uint8Array.of(1, 1, 1, 1, 2, 2, 2, 2, 3, 3));
    owner.restore(player, 0, { missionPost: () => 0, savedGame: () => 0, checking: () => 0 }, 1, {});
    assert.equal(owner.active, 1);
    assert.deepEqual([owner.current.byteOffset - players.byteOffset, owner.current.length], [0xb4 + 0x6c, 10]);
    assert.deepEqual([...owner.current], [1, 1, 1, 1, 2, 2, 2, 2, 3, 3]);
});
