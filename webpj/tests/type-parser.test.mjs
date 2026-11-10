// `.type` 정의 해석(cpppj TypeParser.cpp, AssetTests TypeParser_PreservesCodesAndSpecialFrameOrder·TypeParser_RejectsIncompleteOrInvalidDefinitions)을 웹 parseTypeDefinition 에 대조한다.
// 4-A 의 TypeParser 항목(WB04-E267): 클러스터 그림 참조(images)·속성 조회(마지막 지정값/대소문자/형 비변환)·발자국·문법 거부·숫자 토큰·NUL 절단. 실제 게임·브라우저 실행이 아니다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { parseTypeDefinition, typeFootprint, typeNumber, typeProperty, typeString } from '../www/assets/type-table.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { realCaptureAssets } from './capture-assets-support.mjs';

// 프레임 코드와 도움말/기본 프레임의 덮어쓰기 순서는 원본/CD 로더에서 확인한 규칙이다(cpppj 시험과 같은 입력).
const SAMPLE = 'typename Sample client constructor typeflags shadow dontSave;\n'
    + '{ description="한글; //본문\\경로"; foot_x=7; foot_y=6; COST=1; cost=2; }\n'
    + 'A00: default Fringe: "a.gif" #01 : "shadow.gif" #00;\n'
    + 'AA260: HELP hard suck rim lit unlit cracked: "b.gif" #2;\n'
    + 'B00: default GumpFrame BaseFrame dirt: "c.gif" #3;\n';

test('TypeParser: 프레임 코드·특수 프레임 순서·그림 참조·속성 조회가 cpppj 와 같다', () => {
    const type = parseTypeDefinition(SAMPLE);
    assert.equal(type.name, 'Sample');
    assert.equal(type.constructorWords.join(' '), 'client constructor');
    assert.equal(type.flags.length, 2);
    // 문자열 안의 ;·//·역슬래시는 그대로 남고 문자형 속성은 숫자로 바뀌지 않는다.
    assert.equal(typeString(type, 'DESCRIPTION'), '한글; //본문\\경로');
    assert.equal(typeNumber(type, 'cost'), 2);
    assert.equal(typeNumber(type, 'description'), null);
    assert.equal(typeString(type, 'cost'), null);
    assert.equal(typeProperty(type, 'missing'), null);
    assert.deepEqual(typeFootprint(type), [7, 8]);
    assert.equal(type.clusters.length, 3);
    // 첫 클러스터는 그림 참조가 둘이고 번호는 십진수로 읽는다('01' 도 8진수가 아님).
    assert.deepEqual(type.clusters[0].images, [{ file: 'a.gif', number: 1 }, { file: 'shadow.gif', number: 0 }]);
    assert.equal(type.clusters[0].imageCount, 2);
    assert.equal(type.clusters[0].code.variant, 'P'.charCodeAt(0));
    // AA260: 변형 'A', 번호 260 의 하위 8비트 4, 알려진 7개 비트(1·2·4·8·0x10·0x20·0x40 중 hard suck rim lit unlit cracked = 0x7e).
    assert.deepEqual([type.clusters[1].code.side, type.clusters[1].code.variant, type.clusters[1].code.number, type.clusters[1].code.flags], [65, 65, 4, 126]);
    assert.equal(type.clusters[2].code.flags, 0);
    assert.deepEqual([type.defaultFrame, type.gumpFrame, type.helpFrame, type.baseFrame], [2, 2, 1, 2]);
    assert.equal(FrameCodeTable.fromDefinition(type).findNumber(65, 65, 4), 1);
    const defaults = parseTypeDefinition('typename Empty {} A00:default:"a"#0; B00:default:"b"#0;');
    assert.deepEqual([defaults.defaultFrame, defaults.helpFrame], [1, 1]);
    // gumpframe 미지정은 원본 .type 로딩의 -1 이며 그림 파서는 기본 프레임으로 복귀한다.
    assert.deepEqual([defaults.gumpFrame, defaults.baseFrame], [-1, -1]);
});

test('TypeParser: 같은 속성은 마지막 지정값이 이기고 이름은 대소문자를 구분하지 않는다', () => {
    const type = parseTypeDefinition('typename A { COST=1; cost=2; Range=3; range="x"; } A00:default:"a"#0;');
    assert.equal(typeNumber(type, 'cost'), 2);
    // 마지막 range 는 문자열이라 숫자 조회는 비고 문자열 조회가 값을 돌려준다(이전 숫자 값으로 돌아가지 않는다).
    assert.equal(typeNumber(type, 'RANGE'), null);
    assert.equal(typeString(type, 'range'), 'x');
    assert.deepEqual(typeFootprint(type), [0, 0]);
});

test('TypeParser: 불완전하거나 잘못된 정의를 거부한다', () => {
    assert.throws(() => parseTypeDefinition('typename A {x="unterminated'));
    assert.throws(() => parseTypeDefinition('typename A {x=1}'));
    assert.throws(() => parseTypeDefinition('typename A {} a00::"x"#0;'));
    assert.throws(() => parseTypeDefinition('typename A {} A00::"x"#0'));
    assert.throws(() => parseTypeDefinition('typename A {x=1e999;}'));
    // 이름이 없거나 typename 이 아니면 거부한다.
    assert.throws(() => parseTypeDefinition('name A {}'));
    assert.throws(() => parseTypeDefinition(''));
    // 클러스터 번호는 십진수 전체여야 한다(A0x1 은 거부).
    assert.throws(() => parseTypeDefinition('typename A {} A0x1::"x"#0;'));
    // 프레임 번호가 정수 범위를 벗어나면 거부한다.
    assert.throws(() => parseTypeDefinition('typename A {} A99999999999::"x"#0;'));
});

test('TypeParser: 숫자 토큰은 부호·소수점·지수를 읽고 NUL 이후는 읽지 않는다', () => {
    const type = parseTypeDefinition('typename A {x=1e3; y=.5; z=-.5; w=+2; v=1.;} A00:default:"a"#0;');
    assert.deepEqual(type.properties.map((property) => property.value), [1000, 0.5, -0.5, 2, 1]);
    // 문자열 안과 바깥의 // 은 바깥만 주석이다.
    const commented = parseTypeDefinition('typename A { // 주석 ; } 무시\n s="//keep"; } A00:default:"a"#0; // 끝\n');
    assert.equal(typeString(commented, 's'), '//keep');
    // NUL 뒤의 데이터는 읽지 않는다.
    assert.equal(parseTypeDefinition(`typename A {} A00::"x"#0;${String.fromCharCode(0)}garbage`).clusters.length, 1);
});

test('TypeParser: 발자국은 0 쪽으로 버리고 foot_y 6 만 8 로 바꾼다', () => {
    const footprint = (body) => typeFootprint(parseTypeDefinition(`typename A { ${body} } A00:default:"a"#0;`));
    assert.deepEqual(footprint('foot_x=3.9; foot_y=-2.9;'), [3, -2]);
    assert.deepEqual(footprint('foot_y=6;'), [0, 8]);
    assert.deepEqual(footprint('foot_y=6.9;'), [0, 8]);
    assert.deepEqual(footprint('foot_y=7;'), [0, 7]);
    // foot_x/foot_y 만 읽는다(footx 별칭은 레코드 해석에서만 쓴다).
    assert.deepEqual(footprint('footx=4;'), [0, 0]);
    assert.throws(() => footprint('foot_x=9999999999;'));
});

test('TypeParser: 실제 자산 116개의 클러스터 그림 참조 수는 레이어 수와 같다', () => {
    const { table } = realCaptureAssets();
    let clusters = 0;
    // 모든 자산의 모든 클러스터가 그림 참조를 하나 이상 가지고 개수가 imageCount 와 같다.
    for (const definition of table.definitions) {
        for (const cluster of definition.clusters) {
            assert.ok(cluster.images.length >= 1 && cluster.images.length === cluster.imageCount);
            assert.ok(cluster.images.every((image) => typeof image.file === 'string' && Number.isInteger(image.number)));
            clusters += 1;
        }
    }
    assert.ok(clusters > 116);
});
