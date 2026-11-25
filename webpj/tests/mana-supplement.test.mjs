// 10.78 에서 빠진 마나 아이콘 보충(cpppj 1335 판정 ②)을 검사한다: 별칭표, 보충 SHP 의 앞 8프레임 동일성, 8프레임 밖 마나 그림 해석, 다른 타입 불변, 도움말 별칭.
// (2026-11-25 WB04-E1336)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT, readData } from './helpers.mjs';
import { TaffArchive } from '../www/assets/taff.js';
import { ShapeDatabase } from '../www/assets/shapes.js';
import { decodeOriginalText } from '../www/assets/text-encoding.js';
import { TypeTable, TYPE_LOAD_ORDER } from '../www/assets/type-table.js';
import { EMPTY_PICTURE, createPictureResolver } from '../www/assets/picture-resolver.js';
import { MANA_BLOCK, MANA_SUPPLEMENT_FRAMES, repairManaPictureText, resolveTextPictureResource } from '../www/assets/text-picture-resources.js';
import { htmlPictureText, htmlPictureTextRepaired } from '../www/ui/html-text-parser.js';
import { MANA_PICTURE_BASE64 } from '../www/assets/mana-picture-data.js';

// 공용 자료: 실제 10.78 셰이프와 타입 표다.
const shapes = new ShapeDatabase(readData('d/_shapes.shp'));
const table = TypeTable.fromArchive(new TaffArchive(readData('netstorm.tarc')), decodeOriginalText, () => {});
const resolve = createPictureResolver({ shapes, getTable: () => table });

// 별칭표는 다섯 개만 바꾸고 다른 그림 명령은 그대로 둔다.
test('repairManaPictureText: imana.8~11·17 만 현행 번호로 바꾼다', () => {
    assert.equal(repairManaPictureText('~[Imana.8]'), '~[Imana.4]');
    assert.equal(repairManaPictureText('~[Imana.9]'), '~[Imana.5]');
    assert.equal(repairManaPictureText('~[Imana.10]'), '~[Imana.6]');
    assert.equal(repairManaPictureText('~[Imana.11]'), '~[Imana.7]');
    assert.equal(repairManaPictureText('~[Imana.17]'), '~[Imana.9]');
    assert.equal(repairManaPictureText('~[IMANA.8]'), '~[Imana.4]', 'ASCII 대소문자 무시');
    for (const same of ['~[Imana.3]', '~[Imana.12]', '~[Ifortgump.8]', '~[Imana.8x]', '~[Imana.*]']) assert.equal(repairManaPictureText(same), same);
    assert.equal(htmlPictureText('!"mana.8">'), '~[Imana.8]');
    assert.equal(htmlPictureTextRepaired('!"mana.8">'), '~[Imana.4]');
});

// 보충 SHP 는 CD 마나 25프레임이며 앞 8프레임은 10.78 그림과 픽셀·사각형이 같아야 한다.
test('resolveTextPictureResource: 마나만 25프레임 보충본이고 앞 8프레임은 원본과 같다', () => {
    assert.equal(TYPE_LOAD_ORDER[MANA_BLOCK], 'mana');
    assert.equal(shapes.blocks[MANA_BLOCK].frames.length, 8);
    const resource = resolveTextPictureResource(shapes, MANA_BLOCK, 8);
    assert.notEqual(resource.shapes, shapes);
    assert.equal(resource.frames, MANA_SUPPLEMENT_FRAMES);
    assert.equal(resource.shapes.blocks[0].frames.length, 25);
    // 앞 8프레임의 사각형·해독 결과가 같다.
    for (let frame = 0; frame < 8; frame += 1) {
        assert.deepEqual(resource.shapes.blocks[0].frames[frame].rect, shapes.blocks[MANA_BLOCK].frames[frame].rect);
        assert.deepEqual(resource.shapes.decode(0, frame), shapes.decode(MANA_BLOCK, frame));
        assert.deepEqual(resource.shapes.squidHeader(0, frame), shapes.squidHeader(MANA_BLOCK, frame));
    }
    // 다른 타입·다른 클러스터 수는 빌려 준 저장소 그대로다.
    const other = resolveTextPictureResource(shapes, 22, 80);
    assert.deepEqual({ ...other, shapes: other.shapes === shapes }, { shapes: true, block: 22, frames: 80 });
    assert.equal(resolveTextPictureResource(shapes, MANA_BLOCK, 7).shapes, shapes);
});

// 마나 그림 해석: 8프레임 밖 번호도 보충본에서 실제 그림이 나오고 앞 8프레임은 이전과 같다.
test('createPictureResolver: mana.8~24 는 보충 그림이고 mana.25 이상은 빈 그림이다', () => {
    for (let frame = 0; frame < 8; frame += 1) {
        const picture = resolve('mana', String(frame));
        assert.notEqual(picture, null);
        assert.notEqual(picture, EMPTY_PICTURE, `mana.${frame}`);
        assert.deepEqual(picture.image, shapes.decode(MANA_BLOCK, frame));
    }
    for (let frame = 8; frame < MANA_SUPPLEMENT_FRAMES; frame += 1) {
        const picture = resolve('mana', String(frame));
        assert.notEqual(picture, null, `mana.${frame}`);
        assert.ok(picture.width > 0 && picture.height > 0 || picture.image.width === 0, `mana.${frame} 크기`);
    }
    assert.equal(resolve('mana', '25'), EMPTY_PICTURE);
    // 별칭이 가리키는 현행 번호 4~7·9 는 모두 실제 그림이다.
    for (const frame of [4, 5, 6, 7, 9]) assert.ok(resolve('mana', String(frame)).image.width > 0, `mana.${frame}`);
});

// 모듈 데이터는 cpppj 의 .inc 와 같다.
test('mana-picture-data.js: cpppj ManaPictureData.inc 와 같은 바이트', (t) => {
    const source = path.join(REPO_ROOT, 'cpppj', 'src', 'client', 'ManaPictureData.inc');
    if (!fs.existsSync(source)) return t.skip('cpppj 소스가 없음');
    const expected = Buffer.from([...fs.readFileSync(source, 'utf8').matchAll(/0x([0-9a-fA-F]{2})/g)].map((match) => parseInt(match[1], 16)));
    assert.deepEqual(Buffer.from(MANA_PICTURE_BASE64.replace(/\s+/g, ''), 'base64'), expected);
    return undefined;
});
