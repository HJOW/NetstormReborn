// Player 식별자 조회(RawPlayerIdentity: 초기화·resolve·네 getter)를 원본 10.78 관측으로 대조한다(WB04-E4, cpppj PlayerIdentityTests 이식).
//  - player-identity-x86.tsv 10.78 행: ctor 4 + resolve 179 의 전체 60 BYTE 레코드(이름 뒤 미작성 바이트와 마지막 DWORD 표식 포함)와 반환, 네 getter 644행(subscriber·owner·address·name 각 161)을
//    같은 로컬 전역/Player 9/metamap 40/채팅 100 입력으로 대조한다. 객체 메뉴 DWORD 24행(menu)은 client RawObjectMenu 단위가 필요해 제외한다.
//  - 계약: 조회기는 저장소를 복사하지 않고 현재 원문 변경을 읽으며, metamap 모드에 기록이 없으면 거부하고, 부분 완료와 모드 누락을 구별한다.
// (2026-11-08 WB04-E4: Player 식별자 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get } from './raw-support.mjs';
import { RawPlayerIdentity } from '../www/core/raw-player-identity.js';

// 원본 관측 행(10.78 행만)이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E4)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'player-identity-x86.tsv'));
const rows = allRows?.filter((row) => row[0] === 'originals') ?? null;

// fixture 13개 독립 입력 열의 위치이다. (2026-11-08 WB04-E4)
const INPUT = Object.freeze({ modes: 0, table: 1, local: 2, subscriber: 3, address: 4, name: 5, queryOwner: 6, querySub: 7, queryAddress: 8, queryName: 9, argument: 10, rawOwner: 11, initMode: 12 });

// 원본 바이트 hex 를 문자 인코딩 변환 없이 이진 문자열로 읽는다("-" 는 빈 문자열). (2026-11-08 WB04-E4)
function decode(value) {
    if (value === '-') return '';
    assert.equal(value.length % 2, 0);
    let result = '';
    for (let index = 0; index < value.length; index += 2) result += String.fromCharCode(parseInt(value.slice(index, index + 2), 16));
    return result;
}

// 문자열을 원문 효과와 같은 소문자 hex 로 기록한다. (2026-11-08 WB04-E4)
const textHex = (value) => Array.from(value, (char) => char.charCodeAt(0).toString(16).padStart(2, '0')).join('');
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// NUL 까지만 쓰고 이름 뒤의 원문 표식을 지우지 않는다. (2026-11-08 WB04-E4)
function writeText(bytes, offset, value) {
    assert.ok(offset + value.length < bytes.length);
    for (let index = 0; index < value.length; index += 1) bytes[offset + index] = value.charCodeAt(index) & 0xff;
    bytes[offset + value.length] = 0;
}

// 같은 현재 전역/Player/meta/chat 과 getter 를 조립한다. 반환값은 실제 몸체가 계산한다. (2026-11-08 WB04-E4)
class IdentityScene {
    constructor(row) {
        this.players = new Uint8Array(9 * 180);
        this.meta = new Uint8Array(40 * 460);
        this.chat = new Uint8Array(4800);
        this.input = new Array(13).fill(0);
        // 각 숫자 열은 상위 DWORD 비트도 그대로 보존한다.
        for (let index = 0; index < 13; index += 1) if (index !== INPUT.name && index !== INPUT.queryName) this.input[index] = Number(BigInt(row[index + 2]) & 0xffffffffn);
        this.localName = decode(row[INPUT.name + 2]);
        this.queryName = decode(row[INPUT.queryName + 2]);
        const table = this.input[INPUT.table];
        // 원문과 같은 독립 Player 입력이다.
        for (const entry of [{ bit: 1, index: 2, subscriber: 0x80000002, name: 'Alpha' }, { bit: 2, index: 8, subscriber: 0xfffffff0, name: 'LAST' }, { bit: 4, index: 3, subscriber: 0x80000002, name: 'Duplicate' },
            { bit: 8, index: 4, subscriber: 0, name: 'OnlyName' }, { bit: 2048, index: 2, subscriber: 0x80000055, name: 'localuser' }]) {
            if (table & entry.bit) {
                const bytes = this.players.subarray(entry.index * 180, (entry.index + 1) * 180);
                put(bytes, 44, entry.subscriber);
                writeText(bytes, 12, entry.name);
            }
        }
        // 두 판본에 같은 의미의 독립 metadata 입력이다(10.78 은 이름 +38, 활성 flag +100).
        for (const entry of [{ bit: 16, index: 0, owner: 6, subscriber: 0x80000002, name: 'MetaFirst', active: 1 }, { bit: 32, index: 39, owner: 7, subscriber: 0xfffffff1, name: 'MetaLast', active: 1 },
            { bit: 64, index: 1, owner: 5, subscriber: 0x80000100, name: 'Inactive', active: 2 }]) {
            if (table & entry.bit) {
                const bytes = this.meta.subarray(entry.index * 460, (entry.index + 1) * 460);
                put(bytes, 0, entry.owner);
                put(bytes, 4, entry.subscriber);
                writeText(bytes, 38, entry.name);
                put(bytes, 100, entry.active);
            }
        }
        // 처음/마지막/0/중복 Subscriber 입력이다.
        for (const entry of [{ bit: 128, index: 0, subscriber: 0x80000002, name: 'ChatFirst' }, { bit: 256, index: 99, subscriber: 0xfffffff2, name: 'ChatLast' },
            { bit: 512, index: 1, subscriber: 0, name: 'ZeroSub' }, { bit: 1024, index: 2, subscriber: 0x80000002, name: 'ChatSecond' }]) {
            if (table & entry.bit) {
                const bytes = this.chat.subarray(entry.index * 48, (entry.index + 1) * 48);
                writeText(bytes, 0, entry.name);
                put(bytes, 44, entry.subscriber);
            }
        }
        this.reads = {
            localOwner: () => this.input[INPUT.local],
            subscriber: () => this.input[INPUT.subscriber],
            address: () => this.input[INPUT.address],
            name: () => this.localName,
            battle: () => (this.input[INPUT.modes] & 1) !== 0,
            editor: () => (this.input[INPUT.modes] & 2) !== 0,
            metamap: () => (this.input[INPUT.modes] & 4) !== 0,
            unknownName: () => 'SOMEONE',
        };
        // 초기 표식과 원문 필드를 보존하는 60 BYTE 이다.
        this.record = new Uint8Array(60).fill(0xa5);
        put(this.record, 0, this.input[INPUT.queryOwner]);
        put(this.record, 4, this.input[INPUT.querySub]);
        writeText(this.record, 8, this.queryName);
        put(this.record, 48, this.input[INPUT.queryAddress]);
        put(this.record, 52, 0xdeadbeef);
    }

    // 저장소/전역 사본 없이 실제 조회기를 생성한다.
    identity() { return new RawPlayerIdentity(this.players, this.meta, this.chat, this.reads); }
}

// 생성/resolve 의 전체 60 BYTE 와 정상 반환 입력을 원문과 비교한다.
test('RawPlayerIdentity: 원본10.78 생성/resolve 전체 60 BYTE 레코드', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of rows) {
        if (row[1] !== 'ctor' && row[1] !== 'resolve') continue;
        assert.equal(row.length, 18);
        const scene = new IdentityScene(row);
        const identity = scene.identity();
        let result = 0;
        if (row[1] === 'ctor') RawPlayerIdentity.initialize(scene.record, scene.input[INPUT.initMode]);
        else result = identity.resolve(scene.record) ? 1 : 0;
        const label = `행 ${count} ${row[1]} modes${scene.input[INPUT.modes]} table${scene.input[INPUT.table]} owner${scene.input[INPUT.queryOwner]} sub${scene.input[INPUT.querySub]}`;
        assert.equal(String(result), row[15], label);
        assert.equal(hex(scene.record), row[16], label);
        count += 1;
    }
    assert.equal(count, 183);
});

// 네 getter 는 숫자 세 종류/실제 이름과 실패 대체 이름을 별도로 대조한다(resolve 성공을 추가 관문으로 쓰지 않음).
test('RawPlayerIdentity: 원본10.78 네 getter 644행', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of rows) {
        const kind = row[1];
        if (!['subscriber', 'owner', 'address', 'name'].includes(kind)) continue;
        const scene = new IdentityScene(row);
        const identity = scene.identity();
        const argument = scene.input[INPUT.argument];
        let result;
        if (kind === 'subscriber') result = String(identity.subscriberForOwner(argument));
        else if (kind === 'owner') result = String(identity.ownerForSubscriber(argument));
        else if (kind === 'address') result = String(identity.addressForSubscriber(argument));
        else result = textHex(identity.nameForSubscriber(argument));
        assert.equal(result, row[15], `행 ${count} ${kind}`);
        count += 1;
    }
    assert.equal(count, 644);
});

// 동일 조회기가 현재 원문 변경을 읽으며 부분 완료와 누락 모드를 실제 계약대로 구분한다.
test('RawPlayerIdentity: 현재 저장소 변경을 읽고 metamap 기록 누락을 거부한다', () => {
    const scene = new IdentityScene(allRows[1]);
    const identity = scene.identity();
    assert.equal(identity.subscriberForOwner(2), 0x80000002);
    put(scene.players, 2 * 180 + 44, 0xabcd9911);
    assert.equal(identity.subscriberForOwner(2), 0xabcd9911);
    scene.input[INPUT.modes] = 4;
    scene.record.fill(0);
    put(scene.record, 4, 0x80000002);
    const missing = new RawPlayerIdentity(scene.players, new Uint8Array(0), scene.chat, scene.reads);
    assert.throws(() => missing.resolve(scene.record), /metamap 식별자 기록 미연결/);
    scene.input[INPUT.modes] = 0;
    scene.record.fill(0);
    put(scene.record, 4, 0x1234);
    writeText(scene.record, 8, 'Unlisted');
    assert.equal(identity.resolve(scene.record), true);
    assert.equal(get(scene.record, 0, 4), 0);
    // 저장소 크기 오류와 이름 NUL 부재는 호스트 오류로 거부한다.
    assert.throws(() => new RawPlayerIdentity(new Uint8Array(10), new Uint8Array(0), scene.chat, scene.reads), RangeError);
    const bad = new Uint8Array(60).fill(0x41);
    assert.throws(() => identity.resolve(bad), RangeError);
});
