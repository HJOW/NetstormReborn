// 신전·제네레이터·타워 파편 캡처의 원본 10.78 관측 481행(181+180+120)(cpppj `CampaignTempleShrapnel_Native181CapturePixels` 가 읽는 temple-capture-x86.tsv)을 웹 파편 캡처가 같은 그림으로 만드는지 대조한다.
// 입력 열(TYPE 이름·프레임·단어·추가 비트·소유자)만 실제 Session 의 신전 객체에 쓰고 관측 열(그림 크기·기준점·전체 픽셀·픽셀 수)은 비교에만 쓴다. 원본 게임을 실행하지 않는다. (2026-11-23 WB04-E1275)
// 관측 조건: 실제 10.78 ShapeToBuffer·가상 Draw·보호 SHP·전체 RLE, TYPE 기준점 (-2,3), 단위 배율·일반 색.
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { put } from './raw-support.mjs';
import { campaignOwners } from './campaign-session-support.mjs';

// 실제 Session 생성자 순서를 건물 캡처 설치(connectBuilding)까지 조립한다(신전 파편은 건물/유닛 캡처를 쓴다). 전체 Load 를 가장하지 않는다.
function scene() {
    const quiet = () => {}, sound = { play: quiet, playOnceAt: quiet, playLoopAt: quiet, stop: quiet, isPlaying: () => false };
    const o = campaignOwners({ carrierRequest: quiet, deletion: { selected: () => 0 }, sound, priest: { shield: { noticeSound: quiet, translate: (text) => text, tell: quiet } } });
    try {
        o.connectWorldInstall({ sections: Array.from({ length: 35 }, () => new Uint8Array(0)) });
        o.connectOutcome(); o.connectAiBrains(); o.connectAiWorld(); o.connectAiProduction(); o.connectBuilding();
        return o;
    } catch (error) { o.dispose(); throw error; }
}
// 16진 문자열을 바이트로 바꾼다.
const bytesOf = (hex) => Uint8Array.from(Buffer.from(hex, 'hex'));

// 한 fixture 의 모든 행을 실제 Session 의 파편 캡처로 대조한다. columns 는 행의 열 수(신전은 소리 횟수 열이 있어 10, 제네레이터·타워는 9)이다.
// 입력 열: 단어(+12, 2바이트)·프레임(+36)·추가 비트(+40)·소유자(+34), TYPE 기준점 (-2, 3). 관측 열: 그림 크기·기준점·전체 픽셀·픽셀 수. 모든 행의 효과 생성 열은 '1' 이다.
function checkCaptureFixture(name, columns, cases, totalPixels) {
    const lines = fs.readFileSync(new URL(`../../cpppj/tests/fixtures/${name}`, import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('#'));
    assert.equal(lines.length, cases);
    const o = scene(); const failures = []; let pixels = 0;
    try {
        for (const [index, line] of lines.entries()) {
            const row = line.split('\t'); assert.equal(row.length, columns, `행 ${index}`); assert.equal(row[8], '1');
            const type = o.core.types.findIndex((record, number) => number >= 70 && record.name === row[0]); assert.ok(type >= 70, `TYPE ${row[0]}`);
            const sid = o.world.create(type, 2); const raw = o.core.pool.allocated(sid);
            put(raw, 12, Number(row[2]), 2); put(raw, 36, Number(row[1])); raw[40] = Number(row[3]); raw[34] = Number(row[4]);
            const record = o.core.types[type], savedX = record.hotspotX, savedY = record.hotspotY; record.hotspotX = -2; record.hotspotY = 3;
            let image; try { image = o.particleModules.shrapnel.lifetime.capture(sid); } catch (error) { failures.push(`${index}/${row[0]}: ${error.message}`); record.hotspotX = savedX; record.hotspotY = savedY; continue; }
            record.hotspotX = savedX; record.hotspotY = savedY;
            const metadata = `${image.width},${image.height},${image.hotspotX},${image.hotspotY}`;
            const same = metadata === row[5] && Buffer.from(image.pixels).toString('hex') === row[6] && image.pixels.length === Number(row[7]);
            if (!same) failures.push(`${index}/${row[0]} frame${row[1]} word${row[2]} extra${row[3]} owner${row[4]}: 웹 ${metadata} 원본 ${row[5]}`); else pixels += image.pixels.length;
        }
    } finally { o.dispose(); }
    assert.deepEqual(failures.slice(0, 12), []);
    assert.equal(pixels, totalPixels);
}

test('temple-capture 원본 181행: 신전 파편 캡처의 크기·기준점·전체 픽셀이 같다', () => { checkCaptureFixture('temple-capture-x86.tsv', 10, 181, 2684495); });
test('generator-capture 원본 180행: 제네레이터 세 TYPE 파편 캡처의 크기·기준점·전체 픽셀이 같다', () => { checkCaptureFixture('generator-capture-x86.tsv', 9, 180, 1844370); });
test('blocker-capture 원본 120행: 스톤/아이스 타워 파편 캡처의 크기·기준점·전체 픽셀이 같다', () => { checkCaptureFixture('blocker-capture-x86.tsv', 9, 120, 1317660); });
