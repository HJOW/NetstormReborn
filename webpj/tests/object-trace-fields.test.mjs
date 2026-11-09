// 객체 trace 필드 서식 raw-object-trace-fields.js(Base 004ac400·Form 004ae1e0·Process 004ae3f0·Depend 004ae530·Content 004aea90)를 원본 10.78 관측과 cpppj `ObjectTraceFieldsTests` 계약으로 대조한다.
//  - 원본 관측 object-trace-fields-x86.tsv 308행(상태 16조합·덤프 모드·이름·정수 경계·기록 뒤 필드 변경): 입력 6칸(진입 본체·SID·덤프 모드·변경·이름 hex·50 BYTE 슬롯 hex)에서
//    실제 로그 장치가 받은 레벨과 서식 완료 바이트 전체(`29:<hex>;…`)가 한 바이트도 다르지 않다.
//  - 계약: 잘못된 TYPE/미정의 CRT 서식/임시 버퍼 초과를 거부해도 앞선 Base 로그는 보존하고, 로그 장치가 없으면 로그 전에 거부한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E230: 객체 trace 필드 서식 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { ObjectTraceFieldKind, traceRawObjectFields } from '../www/core/raw-object-trace-fields.js';

// 저장된 독립 기계어 출력을 한 번만 읽는다(7칸: 진입·SID·덤프·변경·이름 hex·슬롯 hex·기대 로그). (2026-11-10 WB04-E230)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'object-trace-fields-x86.tsv'));

// 16진수 열을 바이트 배열로 푼다. '-' 는 빈 이름의 명시 입력이다. (2026-11-10 WB04-E230)
function unhex(text) {
    if (text === '-') return [];
    return Array.from({ length: text.length / 2 }, (_, index) => parseInt(text.slice(index * 2, index * 2 + 2), 16));
}
// 로그 장치의 최종 바이트를 인코딩 변환 없이 16진수로 기록한다. (2026-11-10 WB04-E230)
const hexOf = (text) => Array.from(text, (char) => char.charCodeAt(0).toString(16).padStart(2, '0')).join('');
// 진입 본체 이름을 원문 다섯 본체 선택으로 바꾼다. (2026-11-10 WB04-E230)
const KIND = Object.freeze({ Base: ObjectTraceFieldKind.Base, Form: ObjectTraceFieldKind.Form, Process: ObjectTraceFieldKind.Process, Depend: ObjectTraceFieldKind.Depend, Content: ObjectTraceFieldKind.Content });

// 원문 도구와 같은 입력 슬롯/이름/콜백 변경을 구성하고 기대 계산은 하지 않는다. (2026-11-10 WB04-E230)
class FieldsScene {
    constructor(columns) {
        this.pool = new SidPool(24000, { server: true }); // 원본 50 BYTE 의 물리 풀이다.
        this.types = Array.from({ length: 188 }, () => ({ name: '' })); // 현재 전체 TYPE 이름 표이다.
        this.events = []; // 실제 장치가 받은 level/서식 완료 바이트이다.
        this.calls = 0;
        this.kind = KIND[columns[0]];
        assert.notEqual(this.kind, undefined);
        this.sid = Number(columns[1]);
        this.suppress = Number(columns[2]);
        this.mutation = Number(columns[3]);
        const name = String.fromCharCode(...unhex(columns[4]));
        const raw = unhex(columns[5]);
        assert.equal(raw.length, 50);
        this.pool.physical(this.sid).set(raw);
        this.types[46].name = name;
        this.types[82].name = name;
        this.types[94].name = 'islandAfterCallback';
    }

    // 지정한 원문 물리 폭만 변경한다(인접 필드는 보존). (2026-11-10 WB04-E230)
    put(offset, value, width) { put(this.pool.physical(this.sid), offset, value, width); }

    // 첫 공통 로그 반환 때 현재 파생 필드를 바꿔 캐시된 사본 사용을 검출한다. (2026-11-10 WB04-E230)
    log() {
        return (level, bytes) => {
            this.events.push(`${level}:${hexOf(bytes)}`);
            this.calls += 1;
            if (this.calls === 1 && this.mutation) { this.put(14, 65535, 2); this.put(16, 4321, 2); this.put(18, 94, 4); this.put(12, 65534, 2); this.put(22, 0, 2); }
        };
    }

    // 로그 장치가 받은 레벨/바이트를 원문 표 형식으로 반환한다. (2026-11-10 WB04-E230)
    eventText() { return this.events.join(';'); }
}

test('객체 trace 필드: 원본 10.78 308행(Base 와 Form·Process·Depend·Content)의 모든 서식 바이트와 파생 필드 재조회 시점이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const columns of rows) {
        assert.equal(columns.length, 7);
        const scene = new FieldsScene(columns);
        traceRawObjectFields(scene.pool, scene.types, scene.suppress, scene.sid, scene.kind, scene.log());
        assert.equal(scene.eventText(), columns[6], `행 ${count + 1} ${columns[0]} sid ${columns[1]} 덤프 ${columns[2]} 변경 ${columns[3]}`);
        count += 1;
    }
    assert.equal(count, 308);
});

test('객체 trace 필드 계약: 잘못된 TYPE·미정의 CRT 서식·임시 버퍼 초과를 거부해도 앞선 Base 로그는 보존하고 로그 장치 부재는 로그 전에 거부한다', { skip: !rows && '원본 fixture 없음' }, () => {
    const raw = '000000000000000000002e000000000000005200000000000000000000000000000000000000000000000000000000000000'; // TYPE 46/Content TYPE 82 의 50 BYTE 입력이다.
    const scene = new FieldsScene(['Content', '123', '0', '0', '41', raw]);
    scene.types[82].name = '%s/%s';
    assert.throws(() => traceRawObjectFields(scene.pool, scene.types, scene.suppress, scene.sid, scene.kind, scene.log()), TypeError);
    assert.equal(scene.calls, 1);
    const large = new FieldsScene(['Content', '123', '0', '0', '41', raw]);
    large.types[82].name = 'x'.repeat(80);
    assert.throws(() => traceRawObjectFields(large.pool, large.types, large.suppress, large.sid, large.kind, large.log()), RangeError);
    assert.equal(large.calls, 1);
    const absent = new FieldsScene(['Base', '123', '0', '0', '41', raw]);
    assert.throws(() => traceRawObjectFields(absent.pool, absent.types, absent.suppress, absent.sid, absent.kind, undefined), TypeError);
    assert.equal(absent.calls, 0);
});

test('객체 trace 필드 계약: Content 이름 서식은 단일 %s/%% 와 폭·정밀도·왼쪽 정렬만 받고 다른 플래그와 범위 밖 SID/TYPE 는 거부한다', () => {
    // Content TYPE 82 슬롯: 이름은 types[82] 의 서식, 수량/flags 는 0 이다. 첫 로그 뒤 읽는 파생 필드만 본다.
    const raw = '000000000000000000002e000000000000005200000000000000000000000000000000000000000000000000000000000000';
    const contentLine = (format) => {
        const scene = new FieldsScene(['Content', '123', '0', '0', '41', raw]);
        scene.types[82].name = format;
        traceRawObjectFields(scene.pool, scene.types, scene.suppress, scene.sid, scene.kind, scene.log());
        return String.fromCharCode(...scene.events[1].slice(3).match(/../g).map((pair) => parseInt(pair, 16)));
    };
    assert.match(contentLine('[%s]'), /ContentType=82\/~y\[XXX\]~n q=0 flags=0\n$/);
    assert.match(contentLine('[%5s]'), /\/~y\[  XXX\]~n/);
    assert.match(contentLine('[%-5s]'), /\/~y\[XXX  \]~n/);
    assert.match(contentLine('[%.2s]'), /\/~y\[XX\]~n/);
    assert.match(contentLine('[%.s]'), /\/~y\[\]~n/);
    assert.match(contentLine('100%%'), /\/~y100%~n/);
    assert.throws(() => contentLine('%+s'), TypeError);
    assert.throws(() => contentLine('%d'), TypeError);
    // 범위 밖 SID 는 로그 전에, 범위 밖 현재 TYPE 은 이름을 읽는 위치에서 거부한다.
    const scene = new FieldsScene(['Base', '123', '0', '0', '41', raw]);
    assert.throws(() => traceRawObjectFields(scene.pool, scene.types, 0, 24000, ObjectTraceFieldKind.Base, scene.log()), RangeError);
    scene.pool.physical(123)[10] = 200;
    assert.throws(() => traceRawObjectFields(scene.pool, scene.types, 0, 123, ObjectTraceFieldKind.Base, scene.log()), RangeError);
    assert.equal(scene.calls, 0);
    // 다른 판본의 풀 껍데기나 188개가 아닌 TYPE 표는 첫 로그 전에 거부한다(실제 풀의 슬롯을 그대로 빌려도).
    const cdPool = { edition: 'cd1072', capacity: 24000, slot: (sid) => scene.pool.slot(sid) };
    scene.pool.physical(123)[10] = 46;
    assert.throws(() => traceRawObjectFields(cdPool, scene.types, 0, 123, ObjectTraceFieldKind.Base, scene.log()), TypeError);
    assert.throws(() => traceRawObjectFields(scene.pool, scene.types.slice(0, 187), 0, 123, ObjectTraceFieldKind.Base, scene.log()), TypeError);
    assert.equal(scene.calls, 0);
});
