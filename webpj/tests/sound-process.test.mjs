// SoundProcessTests의10.78 관측6127행과 실제 Host/이름 표·현재 상태 계약을 대조한다. 재생 장치만 기록 경계다. (2026-11-16 WB04-E571)
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits, adler32 } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { GameAuxState } from '../www/core/game-aux-state.js';
import { SoundList, SOUND_LIST_BASE, SOUND_LIST_BYTES, SOUND_HEADER_BYTES, SOUND_FIELD, FALLBACK_SOUND_NAME } from '../www/audio/sound-list.js';
import { SoundProcessState, SoundProcessSystem, SoundProcess, makeSoundProcessHooks, makePriestShieldSoundHooks, makeConstructionProcessSoundHooks, makePriestShieldNoticeHooks, makePriestFallSound } from '../www/core/sound-process.js';

// 저장한 실제 원본 관측 중10.78만 선택한다. CD/10.37은 웹 대상 밖이다.
const rows = (readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/soundprocess-x86.tsv')) ?? []).filter((row) => row[0] === 'originals');
// 원본 실행기와 같은 부모·TYPE·최대 사전 확보SID다.
const PARENT = 50, OBJECT_TYPE = 82, LAST_SID = 127;
// 16진수double을 산술 변환 없이 조합한다.
function doubleFromHex(hex) { return Buffer.from(hex, 'hex').readDoubleBE(); }
// 보관한double의 비트를 원본 출력과 같은16진수로 만든다.
function doubleHex(value) { const bytes = Buffer.alloc(8); bytes.writeDoubleBE(value); return bytes.toString('hex'); }
// 실제 Host/Kernel/공통 삭제에 소리를 부착한다. 경계는 원본 실행기와 같은 사건만 기록한다.
class SoundScene {
    // 각 장면이 소유하는 실제 장치/상태와 기록/행 응답이다.
    pool; types; hash; unpop; destroy; kernel; clock; host; state; system; events = []; reply = 0; now = 0;
    // 공통 Form 네 생성자와 전체 TYPE188칸을 준비한다.
    constructor() {
        this.pool = new SidPool(24000, { server: false });
        this.types = Array.from({ length: 188 }, (_, number) => ({ flags1: 0, flags2: 0, cost: 0, footX: 0, footY: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));
        this.hash = new SquidHash(); this.unpop = new SquidUnpop(this.pool, this.hash, new Uint8Array(65536));
        this.destroy = new RawSquidDestroy(this.pool, this.unpop, this.types); this.kernel = new Kernel(); this.clock = new SquidProcessState();
        this.host = new SquidProcessHost(this.pool, this.types, this.kernel, this.destroy, this.clock); this.state = new SoundProcessState();
        this.system = new SoundProcessSystem(this.host, this.state, this.hooks());
        // 원본 실행기의 입력SID들을 유효한 client 할당으로 확보한다.
        for (let sid = 5; sid <= LAST_SID; sid += 1) assert.equal(this.pool.allocate(2), sid);
    }
    // 원본의 명시 경계 인자들을 그대로 문자열로 기록한다.
    hooks() { return { wallSeconds: () => { this.events.push('T'); return this.now; },
        playLoopAt: (x, y, current, sound) => { this.events.push(`L:${floatToBits(x)}:${floatToBits(y)}:${current}:${sound}`); return this.reply; },
        playOnceAt: (x, y, sound, loop, priority) => { this.events.push(`O:${floatToBits(x)}:${floatToBits(y)}:${sound}:${loop}:${priority}`); return this.reply; },
        play: (...args) => { this.events.push(['G', ...args].join(':')); return this.reply; },
        stop: (sound) => { this.events.push(`S:${sound}`); }, isPlaying: (sound) => { this.events.push(`P:${sound}`); return this.reply; }, report: (expression, line) => { this.events.push(`!${line}`); } }; }
    // 부모의 입력 바이트만 설정한다. TYPE/WORD/좌표 이외는0이다.
    parent(word = 0, x = 0, y = 0) { const raw = this.pool.allocated(PARENT); raw.fill(0); raw[10] = OBJECT_TYPE; put(raw, 12, word, 2); put(raw, 14, x); put(raw, 18, y); }
    // 기록을 원본 구분자로 결합한다.
    text() { return this.events.join(';') || '-'; }
    // 살아 있는 프로세스의 필드와 원본 덤프를 비교한다. 소멸한 객체를 읽지 않는다.
    fields(process, hex, skip = 0) {
        const bytes = Buffer.from(hex, 'hex');
        assert.deepEqual([process.current, process.sound, process.alternate, process.flags], [12, 16, 20, 24].map((at) => bytes.readUInt32LE(at - skip)));
        assert.equal(doubleHex(process.startTime), Buffer.from(bytes.subarray(32 - skip, 40 - skip)).reverse().toString('hex'));
    }
    // 비교 뒤 실제 Form/Kernel을 반납한다. 이 정리의 소리 사건은 비교에서 뺀다.
    discard(process) { if (this.kernel.size) this.host.kill(process, 0x10); assert.equal(this.kernel.size, 0); this.events.length = 0; }
}

test('SoundProcess: 원본10.78 실행5808행의 사건·살아 있는 필드·타입별 프레임이 실제 Host에서 같다', () => {
    const scene = new SoundScene(), selected = rows.filter((row) => row[1] === 'Run'); assert.equal(selected.length, 5808);
    // 각 행을 같은 실제 Host에서 재생하고 관측한 사건/필드만 비교한다.
    for (const row of selected) {
        scene.parent(Number(row[4]), Number(row[11]), Number(row[12])); scene.state.frame = Number(row[5]); scene.state.typeFrames.fill(0xa5a5a5a5); scene.state.typeFrames[OBJECT_TYPE] = Number(row[6]);
        scene.state.debug = Number(row[10]); scene.reply = Number(row[7]); scene.now = doubleFromHex(row[8]);
        const process = scene.system.add(PARENT, Number(row[13]), Number(row[14]), Number(row[2])); assert.ok(process);
        process.setCurrent(Number(row[3])); process.setStartTime(doubleFromHex(row[9])); scene.events.length = 0; process.runFrame();
        const killed = scene.kernel.size === 0; if (killed) scene.events.push('K:0');
        assert.equal(scene.text(), row[15], row.slice(0, 15).join('/')); assert.equal(scene.state.typeFrames[OBJECT_TYPE], Number(row[17])); assert.equal(scene.state.typeFrames[OBJECT_TYPE + 1], 0xa5a5a5a5);
        if (!killed) scene.fields(process, row[16]); scene.discard(process);
    }
});
test('SoundProcess: 원본10.78 통지144행의 정지·현재 재생 조회·진단과 필드가 같다', () => {
    const scene = new SoundScene(), selected = rows.filter((row) => row[1] === 'Destroy'); assert.equal(selected.length, 144);
    // 통지 인자는 읽지 않으며 모든 원본 조합을 재생한다.
    for (const row of selected) {
        scene.parent(); scene.reply = Number(row[4]); const bytes = Buffer.from(row[7], 'hex');
        const process = scene.system.add(PARENT, bytes.readUInt32LE(16), bytes.readUInt32LE(20), Number(row[2])); process.setCurrent(Number(row[3]));
        scene.events.length = 0; process.onFormDestroy(Number(row[5])); assert.equal(scene.text(), row[6]); scene.fields(process, row[7]); scene.discard(process);
    }
});
test('SoundProcess: 원본10.78 생성168행의 현재TYPE·부모·client Form 부착과 초기 필드가 같다', () => {
    const scene = new SoundScene(), selected = rows.filter((row) => row[1] === 'Ctor'); assert.equal(selected.length, 168);
    // 같은 실제 할당에 원본 부착 인자를 공급한다.
    for (const row of selected) {
        scene.state.processType = Number(row[6]); const process = scene.system.add(Number(row[2]), Number(row[3]), Number(row[4]), Number(row[5])); assert.ok(process);
        assert.equal(`B:${scene.pool.slot(process.form)[10]}:${process.parent}:0:${process.form > 4 && process.form < scene.pool.layout.serverFirst ? 16 : 0}`, row[7]);
        scene.fields(process, row[8], 4); assert.equal(scene.text(), '-'); scene.discard(process);
    }
});
test('SoundList: 원본10.78 이름 연산7묶음·124조회 반환/진단·빈 위치·전체32768바이트 Adler가 같다', () => {
    const selected = rows.filter((row) => row[1] === 'Lookup'); assert.equal(selected.length, 7); assert.equal(rows.length, 6127);
    // 각 독립 연산 묶음에 새 표를 쓰고 저장된 원본 결과와 대조한다.
    for (const row of selected) {
        const tokens = [], list = new SoundList((expression, line) => tokens.push(`!${line}`));
        // 원본 실행기 연산을 순서대로 같은 공개 API에 전달한다.
        for (const operation of row[2].split('|')) {
            let result;
            if (operation === 'i') { list.initialize(); result = list.fallback; }
            else if (operation === 'z') result = list.lookup(null);
            else if (operation[0] === 'p') { const at = Number(operation.slice(2)); result = at < SOUND_LIST_BYTES ? list.lookupPointer(SOUND_LIST_BASE + at) : list.lookup('edge.wav'); }
            else result = list.lookup(Buffer.from(operation.slice(2), 'hex').toString('latin1'));
            tokens.push(result ? String((result - SOUND_LIST_BASE) | 0) : 'null');
        }
        assert.equal(tokens.join(','), row[3]); assert.equal(list.firstFree - SOUND_LIST_BASE, Number(row[4])); assert.equal(adler32(list.raw()), Number(row[5]));
    }
});
test('SoundProcess: 지연·대체·같은TYPE 억제·다음 프레임·한 번 종료·정지를 실제 Kernel에서 연결한다', () => {
    const scene = new SoundScene(); scene.parent(0, floatToBits(3.5), floatToBits(4.25)); scene.state.frame = 9; scene.state.typeFrames[OBJECT_TYPE] = 1; scene.now = 5; scene.reply = 0x15000100;
    const loop = scene.system.add(PARENT, 0x15000040, 0x15000080, 3); loop.setStartTime(5.5); scene.kernel.runFrame(); assert.equal(scene.text(), 'T'); assert.equal(loop.current, 0);
    scene.now = 5.5; put(scene.pool.allocated(PARENT), 12, 0x100, 2); scene.events.length = 0; scene.kernel.runFrame(); assert.equal(scene.text(), `T;L:${floatToBits(3.5)}:${floatToBits(4.25)}:0:${0x15000080}`);
    const second = scene.system.add(PARENT, 0x15000040, 0, 1); scene.events.length = 0; second.runFrame(); assert.equal(scene.text(), 'T'); assert.equal(second.current, 0);
    scene.state.frame += 1; second.runFrame(); assert.equal(second.current, scene.reply); assert.equal(scene.state.typeFrames[OBJECT_TYPE], 10);
    scene.state.frame += 1; const once = scene.system.add(PARENT, 0x15000040, 0, 4); scene.events.length = 0; once.runFrame(); assert.equal(scene.text(), `T;G:${0x15000040}:0:0:0:0:0`); assert.equal(scene.kernel.size, 2);
    scene.reply = 0; scene.events.length = 0; scene.host.kill(second, 0x10); assert.equal(scene.text(), `S:${0x15000100};P:${0x15000100}`); scene.discard(loop);
});
test('SoundProcess: 필수 효과·dead 부모·잘못된TYPE·다른 풀을 실제 부착 전에 거부한다', () => {
    const scene = new SoundScene(); scene.parent(); scene.pool.allocated(60)[11] = 2; assert.equal(scene.system.add(60, 1, 0, 0), null); scene.pool.allocated(60)[11] = 0;
    scene.state.processType = 5; assert.throws(() => scene.system.add(PARENT, 1, 0, 0)); assert.equal(scene.kernel.size, 0);
    // 각 필수 재생 효과가 빠진 구성을 개별로 검사한다.
    for (const key of ['wallSeconds', 'playLoopAt', 'playOnceAt', 'play', 'stop', 'isPlaying']) assert.throws(() => new SoundProcessSystem(scene.host, scene.state, { ...scene.hooks(), [key]: null }), /필수/);
    const list = new SoundList(), other = new SidPool(24000, { server: false });
    assert.throws(() => makePriestShieldSoundHooks(other, list, scene.system, {}), /풀이 다릅니다/); assert.throws(() => makeConstructionProcessSoundHooks(other, list, scene.system, {}, {}), /풀이 다릅니다/);
    assert.throws(() => makeSoundProcessHooks({}, null), /벽시계/); assert.throws(() => new SoundProcessSystem({ pool: { edition: 'cd1072' } }, scene.state, scene.hooks()), /10.78/);
});
test('SoundProcess: 현재 GameAux TYPE·checking 재조회·재생 뒤 현재 부모TYPE/frame·정지 뒤 current를 보존한다', () => {
    const scene = new SoundScene(), aux = new GameAuxState(); scene.parent(); scene.state.frame = 7; scene.state.sharedProcessType = () => aux.soundProcessType;
    aux.soundProcessType = 48; const base = scene.hooks(); let debug = -1; scene.state.debug = () => debug;
    assert.equal(aux.soundProcessType, 48);
    const system = new SoundProcessSystem(scene.host, scene.state, { ...base, report: (expression, line) => { scene.events.push(`!${line}`); debug = 0; }, play: (...args) => { scene.state.frame = 8; scene.pool.allocated(PARENT)[10] = 83; return base.play(...args); } });
    const process = system.add(PARENT, 1, 2, 8); assert.equal(scene.pool.slot(process.form)[10], 48); aux.resetForGeneralWorld(); assert.equal(aux.soundProcessType, 48); aux.soundProcessType = 47; assert.equal(scene.pool.slot(process.form)[10], 48);
    process.runFrame(); assert.equal(scene.text(), 'T;!53;G:1:1:0:0:8:0'); assert.equal(scene.state.typeFrames[83], 8); assert.equal(scene.state.typeFrames[82], 0);
    scene.host.kill(process, 0x10); scene.events.length = 0;
    // stop이 현재 항목을 바꾸면 뒤 상태 조회도 변경된 항목을 쓴다.
    let current; const stopping = new SoundProcessSystem(scene.host, scene.state, { ...base, stop: (sound) => { scene.events.push(`S:${sound}`); current.setCurrent(0x44); }, report: null });
    current = stopping.add(PARENT, 1, 0, 0); current.setCurrent(0x33); scene.reply = 1; current.onFormDestroy(0); assert.equal(scene.text(), 'S:51;P:68'); current.setCurrent(0); scene.discard(current);
});
test('SoundProcess: 부착 전은 시계를 읽지 않고 float 지연만 절단하며 보고 예외는 그 접두에서 멈춘다', () => {
    const scene = new SoundScene(); const process = new SoundProcess(scene.system, -1, -2, -1); process.runFrame(); assert.equal(scene.text(), '-'); assert.equal(process.sound, 0xffffffff); assert.equal(process.alternate, 0xfffffffe); assert.equal(process.flags, 0xffffffff);
    scene.now = 16777216.25; process.delay(1.00000006); assert.equal(process.startTime, scene.now + Math.fround(1.00000006)); assert.notEqual(process.startTime, Math.fround(scene.now) + Math.fround(1.00000006));
    scene.parent(); scene.state.debug = 1; scene.state.frame = 4;
    const hooks = scene.hooks(), system = new SoundProcessSystem(scene.host, scene.state, { ...hooks, report: () => { throw new Error('보고 중단'); } }); const attached = system.add(PARENT, 1, 0, 8);
    scene.events.length = 0; assert.throws(() => attached.runFrame(), /보고 중단/); assert.equal(scene.text(), 'T'); assert.equal(scene.state.typeFrames[82], 0); assert.equal(scene.kernel.size, 1); scene.discard(attached);
});
test('SoundProcess: 선택 소리는 벽시계 전 캡처하고 TYPE/좌표는 벽시계·보고 뒤 같은 물리에서 읽는다', () => {
    const scene = new SoundScene(); scene.parent(0, floatToBits(1), floatToBits(2)); scene.state.frame = 7; scene.state.debug = 1;
    const base = scene.hooks(), system = new SoundProcessSystem(scene.host, scene.state, { ...base,
        wallSeconds: () => { const raw = scene.pool.allocated(PARENT); raw[10] = 83; put(raw, 12, 1, 2); put(raw, 14, floatToBits(3)); scene.state.typeFrames[83] = 7; return 1; },
        report: () => { put(scene.pool.allocated(PARENT), 18, floatToBits(4)); scene.state.typeFrames[83] = 0; },
    });
    const skipped = system.add(PARENT, 10, 20, 3); skipped.runFrame(); assert.equal(scene.text(), '-'); assert.equal(skipped.current, 0); scene.discard(skipped);
    scene.parent(0, floatToBits(1), floatToBits(2)); scene.state.typeFrames[83] = 0; scene.reply = 99;
    // priority 보고는 먼저 캡처한 played=false를 바꾸지 못한다. 좌표는 보고 후 현재 값을 쓴다.
    const live = new SoundProcessSystem(scene.host, scene.state, { ...base, wallSeconds: () => { put(scene.pool.allocated(PARENT), 12, 1, 2); put(scene.pool.allocated(PARENT), 14, floatToBits(3)); return 1; },
        report: () => { put(scene.pool.allocated(PARENT), 18, floatToBits(4)); scene.state.typeFrames[82] = 7; },
    }).add(PARENT, 10, 20, 11);
    live.runFrame(); assert.equal(scene.text(), `L:${floatToBits(3)}:${floatToBits(4)}:0:10`); assert.equal(live.current, 99); scene.reply = 0; scene.discard(live);
});
test('SoundPlayer 경계 어댑터: 현재 실제 receiver와 signed DWORD·안내·낙하·건설 인자 순서가 같다', () => {
    const calls = [], player = {};
    // 실제 재생기와 같은 공개 메서드의 receiver/인자만 기록한다.
    for (const name of ['playLoopAt', 'playOnceAt', 'play', 'stop', 'isPlaying', 'playByName', 'playNameAt']) player[name] = function (...args) { assert.equal(this, player); calls.push([name, ...args]); return 17; };
    const hooks = makeSoundProcessHooks(player, () => 1, () => {});
    assert.equal(hooks.wallSeconds(), 1); assert.equal(hooks.playLoopAt(1, 2, 3, 4), 17); assert.equal(hooks.playOnceAt(5, 6, 7, 8, 9), 17); assert.equal(hooks.play(1, 2, 0xffffffff, 0x80000000, 8, 0xfffffffe), 17); hooks.stop(3); assert.equal(hooks.isPlaying(4), 17);
    makePriestShieldNoticeHooks(player, { keep: 1 }).noticeSound({ file: 'a.wav', arguments: [1, 0xffffffff, 0x80000000, 8, 0xfffffffe] }); makePriestFallSound(player)(999, 1.5, -0);
    const scene = new SoundScene(), list = new SoundList(); scene.parent(); list.initialize(); const construction = makeConstructionProcessSoundHooks(scene.pool, list, scene.system, player, { keep: 1 });
    const sound = construction.loadSound('progress.wav'); construction.attachSound(PARENT, sound, 0, 1); assert.equal(scene.host.fromForm(scene.pool.slot(PARENT)[6] | scene.pool.slot(PARENT)[7] << 8).sound, sound);
    construction.playAt(3, 4, 'done.wav', 2); construction.play(sound); assert.equal(construction.keep, 1);
    assert.deepEqual(calls, [['playLoopAt', 1, 2, 3, 4], ['playOnceAt', 5, 6, 7, 8, 9], ['play', 1, 2, -1, -2147483648, 8, -2], ['stop', 3], ['isPlaying', 4], ['playByName', 'a.wav', 1, -1, -2147483648, 8, -2], ['playNameAt', 1.5, -0, 'priestFall.wav', 0], ['playNameAt', 3, 4, 'done.wav', 2], ['play', sound, 0, 0, 0, 0, 0]]);
    const attached = scene.host.fromForm(scene.pool.slot(PARENT)[6] | scene.pool.slot(PARENT)[7] << 8); scene.discard(attached);
});
test('SoundList: 초기화 중복·이름/포인터/머리 범위와 꽉 찬 초기화 전 대체0을 구별한다', () => {
    const list = new SoundList(); assert.equal(list.edition, 'patch1078'); assert.equal(list.fallback, 0); assert.equal(list.lookup(null), 0);
    // 유효하지 않은 C++ byte 문자열과 표 밖 항목을 거부한다.
    for (const name of ['', 'a\0b', '한글', undefined]) assert.throws(() => list.lookup(name));
    assert.throws(() => list.lookupPointer(SOUND_LIST_BASE - 1)); assert.throws(() => list.lookupPointer(SOUND_LIST_BASE + SOUND_LIST_BYTES)); assert.throws(() => list.name(0));
    assert.throws(() => list.field(0, SOUND_FIELD.buffer)); assert.throws(() => list.setField(SOUND_LIST_BASE + SOUND_LIST_BYTES - 27, 0, 1)); assert.throws(() => list.name(SOUND_LIST_BASE + SOUND_LIST_BYTES - 28));
    list.initialize(); const free = list.firstFree; list.initialize(); assert.equal(list.firstFree, free); assert.equal(list.fallback, SOUND_LIST_BASE); assert.equal(list.name(list.fallback), FALLBACK_SOUND_NAME); assert.equal(list.lookup('NonExistant.WAV'), SOUND_LIST_BASE);
    const full = new SoundList(); full.lookup('x'.repeat(0x7fb0 - SOUND_HEADER_BYTES - 1)); const before = full.raw().slice(); assert.equal(full.lookup('next.wav'), 0); assert.deepEqual(full.raw(), before);
    const tooLong = new SoundList(); assert.throws(() => tooLong.lookup('x'.repeat(SOUND_LIST_BYTES)), /끝/); assert.equal(tooLong.firstFree, SOUND_LIST_BASE); assert.ok(tooLong.raw().every((x) => x === 0));
});
test('SoundList: 원문 복제 링크·buffer/root·보존된 머리와 lookup 첫 항목·추가 한계가 같다', () => {
    const list = new SoundList(); list.initialize(); const named = list.lookup('unit.wav'); list.setField(named, SOUND_FIELD.volume, -1); list.setField(named, SOUND_FIELD.attenuation, 27);
    const next = list.firstFree, at = next - SOUND_LIST_BASE; list.raw().fill(0xa5, at + 12, at + 28);
    const duplicate = list.appendDuplicate(named, named, 0x89abcdef); assert.equal(duplicate, next); assert.equal(list.name(duplicate), 'unit.wav'); assert.equal(list.field(duplicate, SOUND_FIELD.root), named); assert.equal(list.field(duplicate, SOUND_FIELD.buffer), 0x89abcdef); assert.equal(list.field(named, SOUND_FIELD.next), duplicate); assert.equal(list.field(duplicate, SOUND_FIELD.next), 0);
    assert.ok(list.raw().subarray(at + 12, at + 28).every((x) => x === 0xa5)); assert.equal(list.lookup('UNIT.WAV'), named); assert.equal(list.field(named, SOUND_FIELD.volume), 0xffffffff); assert.equal(list.field(named, SOUND_FIELD.attenuation), 27);
    const second = list.appendDuplicate(named, duplicate, 33); assert.equal(list.field(second, SOUND_FIELD.root), named); assert.equal(list.field(duplicate, SOUND_FIELD.next), second);
    const full = new SoundList(), huge = full.lookup('x'.repeat(SOUND_LIST_BYTES - 29)); assert.throws(() => full.appendDuplicate(huge, huge, 1), /끝/);
});
test('SoundList: 순회/빈 위치 불일치 보고 뒤 쓰기와 보고 예외 접두를 구별한다', () => {
    const events = [], list = new SoundList((expression, line) => events.push([expression, line])); list.initialize(); list.raw()[28] = 0; list.raw().fill(0x77, 16, 20);
    assert.equal(list.lookup('a.wav'), SOUND_LIST_BASE); assert.deepEqual(events, [['s == firstFreeSound', 455]]); assert.ok(list.raw().subarray(16, 20).every((x) => x === 0x77));
    const throwing = new SoundList(() => { throw new Error('이름 보고 중단'); }); throwing.initialize(); throwing.raw()[28] = 0; const before = throwing.raw().slice(), free = throwing.firstFree;
    assert.throws(() => throwing.lookup('b.wav'), /이름 보고 중단/); assert.deepEqual(throwing.raw(), before); assert.equal(throwing.firstFree, free);
});
