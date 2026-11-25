// cpppj 의 ManaPictureData.inc(원본 CD 마나 25프레임 단일 SHP 블록)를 웹 모듈 www/assets/mana-picture-data.js 로 변환하는 도구이다.
// 사용(저장소 루트 기준):
//   node webpj/tools/make-mana-picture-data.mjs          모듈을 다시 쓴다.
//   node webpj/tools/make-mana-picture-data.mjs --check  모듈이 .inc 와 같은지 검사만 한다(다르면 종료 코드 1).
// originalCD 는 읽지 않는다. cpppj 가 이미 CD 에서 뽑아 둔 .inc 만 읽는다. (2026-11-25 WB04-E1336: cpppj 1335 마나 아이콘 보충 판정 ②의 웹 반영)
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

// 저장소 루트의 절대 경로이다.
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..');
// 입력(.inc)과 출력(.js) 경로이다.
const source = path.join(root, 'cpppj', 'src', 'client', 'ManaPictureData.inc');
const target = path.join(root, 'webpj', 'www', 'assets', 'mana-picture-data.js');

// .inc 의 `0x..` 바이트 목록을 읽어 Buffer 로 만든다. 주석 줄은 무시한다.
function readInc() {
    const text = fs.readFileSync(source, 'utf8');
    const values = [];
    // 줄마다 16진 바이트만 모은다.
    for (const line of text.split(/\r?\n/)) {
        if (line.trimStart().startsWith('//')) continue;
        for (const match of line.matchAll(/0x([0-9a-fA-F]{2})/g)) values.push(parseInt(match[1], 16));
    }
    return Buffer.from(values);
}

// 출력 모듈 본문을 만든다. base64 는 76자씩 끊어 Git 비교를 쉽게 한다.
function buildModule(bytes) {
    const base64 = bytes.toString('base64').match(/.{1,76}/g).join('\n');
    return `// CD 원본 마나 25프레임의 단일 SHP 블록(VFX 1.10)이다. 10.78 자료는 마나 그림이 8프레임뿐이라 안내·도움말 그림(Imana.8~)에 보충한다.
// 생성물이다. 재생성: node webpj/tools/make-mana-picture-data.mjs (원본은 cpppj/src/client/ManaPictureData.inc, 2026-11-25 WB04-E1336)
export const MANA_PICTURE_BASE64 = \`
${base64}
\`;
`;
}

const bytes = readInc();
const expected = buildModule(bytes);
if (process.argv.includes('--check')) {
    const current = fs.existsSync(target) ? fs.readFileSync(target, 'utf8') : '';
    if (current !== expected) { console.error('[webpj mana] mana-picture-data.js 가 ManaPictureData.inc 와 다릅니다.'); process.exit(1); }
    console.log(`[webpj mana] 일치(${bytes.length}바이트)`);
} else {
    fs.writeFileSync(target, expected, 'utf8');
    console.log(`[webpj mana] ${bytes.length}바이트를 ${path.relative(root, target)} 로 썼습니다.`);
}
