// Dude 드롭 두 부모 raw-dude-drop.js(RawDudeDrop)를 원본 10.78 관측으로 대조한다(cpppj DudeDropTests).
//  - dudedrop-x86.tsv originals 154행: 드롭 종류(난수/진입점)·index·부모 발자국/genus/소유자/상태·source SID·난수 seed·population 용량/개수/차단·시각 비트 입력과,
//    결과(반환/assert)·새 SID·Regular 조회/생성/부착 호출 순서·최종 Regular·난수 상태·population·슬롯·장부·변경 사각형 출력. originalCD 행은 범위 밖이다.
//  - 장면은 E206 과 같은 실제 보호 Dude 생성 장면(`dude-spawn-support.mjs`, cpppj SpawnScene)에 부모 89 의 발자국/genus 입력·실제 Kernel/SquidProcessHost Regular·
//    공유 GameRandom·RawDudeNotice 진입점·RawDudeTask 를 더한 cpppj DropScene 이다. 각 행은 입력만 설치하고 기대 칸은 비교에만 쓴다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E208: Dude 드롭 부모 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { spawnScene, spawnHooks, sceneTail } from './dude-spawn-support.mjs';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { GameRandom } from '../www/core/random.js';
import { RawDudeSpawn } from '../www/core/raw-dude-spawn.js';
import { RawDudeTask, makeDudeTaskHostHooks } from '../www/core/raw-dude-task.js';
import { RawDudeNotice, createDudeNoticeState } from '../www/core/raw-dude-notice.js';
import { RawDudeDrop, DudeDropSourceAssertError } from '../www/core/raw-dude-drop.js';

// 원본 관측 10.78 행이다. (2026-11-09 WB04-E208)
const rows = (readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'dudedrop-x86.tsv')) ?? []).filter((row) => row[0] === 'originals');

// 예외가 지정 종류로 나는지 본다. (2026-11-09 WB04-E208)
function throws(action, kind = Error) {
    try { action(); } catch (error) { return error instanceof kind; }
    return false;
}

// double 의 64비트 패턴을 10진 문자열로 읽고 쓰는 공용 DataView 이다. (2026-11-09 WB04-E208)
const view = new DataView(new ArrayBuffer(8));
const doubleToBits = (value) => { view.setFloat64(0, value, true); return view.getBigUint64(0, true).toString(); };
const bitsToDouble = (text) => { view.setBigUint64(0, BigInt(text), true); return view.getFloat64(0, true); };

// 실제 보호 Dude 생성 장면에 부모 입력·Regular Host·난수·진입점 몸체를 더한 장면이다(cpppj DropScene). (2026-11-09 WB04-E208)
function dropScene(row) {
    assert.equal(row.length, 33);
    const scene = spawnScene(Number(row[10]), Number(row[11]), Number(row[12]) !== 0);
    // 부모 89 의 명시 발자국/genus 입력이며 새 Dude 70 은 보호 자료다(공유 표를 바꾸지 않도록 사본).
    const sourceTypes = scene.assets.types.map((record) => ({ ...record }));
    Object.assign(sourceTypes[89], { footX: Number(row[3]) | 0, footY: Number(row[4]) | 0, flags2: Number(row[5]) >>> 0 });
    const speeds = sourceTypes.map(() => ({ speed: 0 }));
    const destroy = new RawSquidDestroy(scene.pool, scene.unpop, sourceTypes);
    const clock = new SquidProcessState();
    const host = new SquidProcessHost(scene.pool, sourceTypes, new Kernel(16), destroy, clock, { regular: () => 1 });
    const random = new GameRandom(Number(row[9]));
    const notice = new RawDudeNotice(scene.pool, scene.hash, sourceTypes, scene.assets.frames, speeds, new SquidPostPopList(), createDudeNoticeState(clock), {});
    // 준비 SID 5 를 부모 raw 로 바꾼다. 새 Dude 6/Regular 7 의 같은 할당 순서를 쓴다.
    const raw = scene.pool.allocated(5);
    raw.fill(0);
    raw[10] = 89;
    raw[34] = Number(row[6]);
    put(raw, 14, floatToBits(20.75));
    put(raw, 18, floatToBits(21.9));
    raw[11] = Number(row[7]);
    clock.now = bitsToDouble(row[13]);
    return { ...scene, sourceTypes, host, clock, random, notice, events: [], current: null };
}

// 원본 관찰 순서를 남기면서 조회/생성 자체는 현재 Host 의 실제 몸체를 수행한다. (2026-11-09 WB04-E208)
function taskHooks(scene) {
    const base = makeDudeTaskHostHooks(scene.pool, scene.host, {});
    return {
        taskRegular: (sid) => { scene.events.push(`R:${sid}:65520:20736`); return base.taskRegular(sid); },
        addTaskRegular: (sid, event) => {
            scene.events.push('N:40');
            scene.current = base.addTaskRegular(sid, event);
            scene.events.push(`B:46:${sid}:0:80`);
            return scene.current;
        },
    };
}

// 실제 생성/Pop/후처리와 task 예약을 실행한 뒤 원본과 같은 전체 상태를 직렬화한다. (2026-11-09 WB04-E208)
function runDrop(scene, row) {
    const spawn = new RawDudeSpawn(scene.pool, scene.factory, scene.owner, scene.state, spawnHooks(scene));
    const task = new RawDudeTask(scene.pool, { clock: scene.clock }, taskHooks(scene));
    const drop = new RawDudeDrop(scene.pool, scene.sourceTypes, spawn, task, scene.notice, scene.random);
    let status = 'return';
    try {
        drop.drop(Number(row[8]), Number(row[2]) | 0, Number(row[1]) !== 0);
    } catch (error) {
        if (!(error instanceof DudeDropSourceAssertError)) throw error;
        status = 'assert';
        scene.events.push(`A:${Number(row[1]) ? 452 : 469}`);
    }
    const born = scene.pool.slot(6)[10] === 70 ? 6 : 0;
    const regular = scene.current ? `${doubleToBits(scene.current.time)},${scene.current.event},${floatToBits(scene.current.payload)},${scene.current.count}` : '-';
    const { populationText, raw, tail } = sceneTail(scene, born, { heads: false });
    return [status, String(born), scene.events.join(';'), regular, String(scene.random.state), String(scene.population.count), populationText, raw, ...tail];
}

test('DudeDrop: 원본 10.78 두 부모의 난수/진입점/float 지연과 sourceSID 0 진단·실제 Host 가 같다', { skip: rows.length === 0 && 'dudedrop-x86.tsv 없음' }, () => {
    let count = 0;
    // 부모 입력 13개와 전체 출력 19개를 구분해 대조한다.
    for (const row of rows) {
        const output = runDrop(dropScene(row), row);
        assert.deepEqual(output, row.slice(14), `행 ${count} 종류 ${row[1]} index ${row[2]} source ${row[8]} 상태 ${row[7]}`);
        count += 1;
    }
    assert.equal(count, 154);
});

test('DudeDrop: 다른 풀의 생성/task/진입점 연결을 거부한다(웹 계약 검사)', { skip: rows.length === 0 && 'dudedrop-x86.tsv 없음' }, () => {
    const scene = dropScene(rows[0]);
    const spawn = new RawDudeSpawn(scene.pool, scene.factory, scene.owner, scene.state, spawnHooks(scene));
    const task = new RawDudeTask(scene.pool, { clock: scene.clock }, taskHooks(scene));
    const drop = new RawDudeDrop(scene.pool, scene.sourceTypes, spawn, task, scene.notice, scene.random);
    assert.equal(drop.pool, scene.pool);
    assert.ok(throws(() => new RawDudeDrop(scene.pool, scene.sourceTypes, spawn, { pool: null }, scene.notice, scene.random), TypeError));
    assert.ok(throws(() => new RawDudeDrop(scene.pool, scene.sourceTypes, { pool: null }, task, scene.notice, scene.random), TypeError));
    assert.ok(throws(() => drop.drop(0, 1, true), DudeDropSourceAssertError));
});
