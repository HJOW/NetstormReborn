// 격리 SDK의 실제 설치된 전이 의존성 라이선스/NOTICE를 배포 자산에 복사한다. npm lock 버전을 함께 기록한다.
'use strict';
const fs = require('node:fs'), path = require('node:path');
// 코드 생성이나 패키지 실행 없이 package.json과 라이선스 원문만 읽는다.
const root = path.resolve(__dirname, '..'), output = path.join(root, 'www/licenses/mod-worker'), seen = new Set(), records = [];
// Node와 같은 상위 node_modules 탐색으로 정확한 설치 경로를 찾는다.
function locate(name, origin) {
    let directory = origin;
    // 드라이브 루트에 도달하면 명시 실패해 불완전한 고지를 만들지 않는다.
    while (true) { const candidate = path.join(directory, 'node_modules', name); if (fs.existsSync(path.join(candidate, 'package.json'))) return candidate; const parent = path.dirname(directory); if (parent === directory) throw new Error('의존성 누락: ' + name); directory = parent; }
}
// 전이 그래프는 경로별 한 번만 방문하고 같은 이름의 다른 버전도 따로 보존한다.
function visit(directory) {
    if (seen.has(directory)) return; seen.add(directory);
    const pkg = JSON.parse(fs.readFileSync(path.join(directory, 'package.json'), 'utf8'));
    const names = fs.readdirSync(directory).filter((name) => /^(?:licen[cs]e|notice|copyright)(?:[.-]|$)/iu.test(name) && fs.statSync(path.join(directory, name)).isFile());
    if (!names.some((name) => /^licen[cs]e(?:[.-]|$)/iu.test(name))) throw new Error('라이선스 원문 누락: ' + pkg.name);
    const folder = pkg.name.replace('/', '__') + '@' + pkg.version;
    fs.mkdirSync(path.join(output, folder), { recursive: true });
    // 원문 바이트를 변경하지 않는다. 상위 인덱스에 라이선스 식별자와 원본 파일명을 적는다.
    for (const name of names) fs.copyFileSync(path.join(directory, name), path.join(output, folder, name));
    records.push({ name: pkg.name, version: pkg.version, license: pkg.license, files: names.map((name) => folder + '/' + name) });
    // npm의 선택/개발 의존성은 제외하고 실제 런타임 dependencies만 포함한다.
    for (const name of Object.keys(pkg.dependencies ?? {})) visit(locate(name, directory));
}
// SDK 진입점이 직접 import하는 네 패키지가 시작점이다.
for (const name of ['ses', '@endo/module-source', '@endo/evasive-transform', 'acorn']) visit(locate(name, root));
records.sort((a, b) => a.name < b.name ? -1 : a.name > b.name ? 1 : a.version.localeCompare(b.version));
fs.writeFileSync(path.join(output, 'index.json'), JSON.stringify({ description: '모드 격리 실행기의 직접/전이 의존성 라이선스 원문', packages: records }, null, 2) + '\n');
process.stdout.write(JSON.stringify({ packages: records.length, output }) + '\n');
