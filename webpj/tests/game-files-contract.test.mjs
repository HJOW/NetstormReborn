// 파일 계층(cpppj BaseFile 계열 adapt 8단위)의 웹 대응 `GameFiles`·`TaffArchive`·`Config.loadBytes` 가 지키는 관측 가능한 계약을 고정한다(4-A BaseFile 계열, WB04-E271).
//  - BaseFileSystem 읽기 순서(디스크 → TAFF)와 대소문자 무시·경로 표기 통일, 와일드카드(`*` 0개 이상·`?` 한 글자, 폴더를 넘지 않음), 첫 대소문자 무시 중복만 남기는 FileFinder 목록 순서.
//  - RawConfigFileLoad: 첫째·셋째 바이트가 0 이면 XOR 인코딩 설정 파일로 풀어 읽고, 첫 NUL 에서 끝나는 C 문자열로 붙인다.
// FILE 열기/읽기 정책·키 필드·CD/보조 디렉터리·Win32 FindFirstFile 핸들 수명은 브라우저에 없는 플랫폼 경계라 옮기지 않았다(docs/webpj-file-layer-20261110.md).
// 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E271)
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameFiles, normalizeFilePath } from '../www/assets/game-files.js';
import { Config, ConfigRegistry, CONFIG_SIGNATURE } from '../www/assets/config.js';
import { xorDecode } from '../www/assets/taff.js';
import { decodeOriginalText } from '../www/assets/text-encoding.js';

// 느슨한 파일만 가진 조회 계층이다. 값은 파일마다 구별되는 한 바이트이다. (2026-11-10 WB04-E271)
function sampleFiles() {
    return new GameFiles({ loose: new Map([
        ['d/a.english', Uint8Array.of(1)], ['d/sub/x.english', Uint8Array.of(2)], ['d/Offical1.english', Uint8Array.of(3)],
        ['d/offical10.english', Uint8Array.of(4)], ['d/b.ENGLISH', Uint8Array.of(5)], ['d/A.ENGLISH', Uint8Array.of(6)],
    ]) });
}

test('GameFiles: 와일드카드는 * 0개 이상·? 한 글자이며 폴더를 넘지 않는다', () => {
    const files = sampleFiles();
    // ? 는 한 글자만 대신한다(offical10 은 두 글자라 제외).
    assert.deepEqual(files.find('d/offical?.english'), ['\\d\\Offical1.english']);
    // * 는 같은 폴더의 파일만 모으고 하위 폴더 파일은 넘지 않는다.
    assert.ok(!files.find('d/*').some((name) => name.toLowerCase().includes('sub')));
    assert.deepEqual(files.find('d/*/x.english'), []);
    assert.deepEqual(files.find('*.english'), []);
    // 이름 비교는 대소문자를 무시한다.
    assert.equal(files.find('d/*.english').length, 4);
});

test('GameFiles: 첫 대소문자 무시 중복만 남기고 등록 순서를 유지하며 정렬하지 않는다', () => {
    const files = sampleFiles();
    // d/b.ENGLISH 보다 d/A.ENGLISH 가 나중에 등록되었지만 d/a.english 와 대소문자만 다르므로 앞선 a 만 남는다.
    assert.deepEqual(files.find('d/*.english'), ['\\d\\a.english', '\\d\\Offical1.english', '\\d\\offical10.english', '\\d\\b.ENGLISH']);
    // 읽기는 먼저 등록된 파일을 쓰고 경로 표기(역슬래시/슬래시/앞의 ./)가 달라도 같은 파일이다.
    assert.deepEqual([...files.read('\\D\\A.english')], [1]);
    assert.deepEqual([...files.read('./d/a.ENGLISH')], [1]);
    assert.equal(files.read('d/missing.english'), null);
    assert.equal(normalizeFilePath('.\\D\\Sub\\X.English'), 'd/sub/x.english');
    // 같은 바이트 배열을 돌려주고 복사하지 않는다.
    assert.equal(files.read('d/a.english'), files.read('D/A.ENGLISH'));
});

// 설정 파일 바이트를 XOR 키로 인코딩한다(원본 저장 형식, xorDecode 는 대칭이다). 서명 앞에 NUL 두 개를 두는 원본 형식의 첫째·셋째 바이트가 0 이다.
function encodeConfigFile(text) {
    const plain = new TextEncoder().encode(text);
    return xorDecode(plain);
}

test('Config.loadBytes: 첫째·셋째 바이트가 0 이면 XOR 인코딩 파일로 풀고 첫 NUL 에서 끝낸다', () => {
    const registry = new ConfigRegistry();
    // 평문(첫째/셋째 바이트가 0 이 아님)은 그대로 붙인다.
    const plain = new Config(registry);
    plain.loadBytes(new TextEncoder().encode('Key = "Value"\r\n'), decodeOriginalText);
    assert.equal(plain.get('Key'), 'Value');
    // 인코딩된 파일: 풀면 서명 mQdsT 로 시작하고(서명의 m·d 가 키 첫째·셋째 글자와 같아 인코딩된 첫째·셋째 바이트가 0 이다) 서명은 떼지 않는다.
    const body = 'mQdsT\r\nKey2 = "Value2"\r\n';
    const encoded = encodeConfigFile(body);
    assert.ok(encoded[0] === 0 && encoded[2] === 0);
    const decoded = new Config(registry);
    decoded.loadBytes(encoded, decodeOriginalText);
    assert.equal(decoded.get('Key2'), 'Value2');
    // 첫 NUL 이후는 읽지 않는다.
    const truncated = new Config(registry);
    truncated.loadBytes(Uint8Array.from([...new TextEncoder().encode('A = "1"\r\n'), 0, ...new TextEncoder().encode('B = "2"\r\n')]), decodeOriginalText);
    assert.equal(truncated.get('A'), '1');
    assert.equal(truncated.get('B'), null);
    assert.equal(typeof CONFIG_SIGNATURE, 'string');
});
