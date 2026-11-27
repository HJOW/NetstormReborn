// 공통 UTF-8 패키지의 선언·경로·정적 ES 모듈을 실행 없이 검사한다. 구문 분석기는 신뢰 호스트가 주입한다.
import { ModError, MOD_API_VERSION, MOD_LIMITS } from './mod-api.js';

// 세 필드는 공통 선언 스키마와 같은 어휘를 쓴다. exact가 마지막 줄바꿈까지 거절한다.
const ID = /^[a-z][a-z0-9]*(?:[.-][a-z0-9]+)*$/u, VERSION = /^(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$/u, PATH = /^[A-Za-z0-9_-]+(?:\/[A-Za-z0-9_-]+)*\.js$/u;
// Windows DN 디렉터리에도 표현 가능한 모듈 이름만 허용한다.
const RESERVED = /^(?:con|prn|aux|nul|com[1-9]|lpt[1-9])$/iu;
// 선언/구문 오류를 정해진 코드로 표시한다.
function check(condition, message, code = 'invalidPackage') { if (!condition) throw new ModError(code, message); }
// 정규식의 마지막 줄바꿈 예외를 허용하지 않고 문자열 전체를 대조한다.
function exact(pattern, text) { return typeof text === 'string' && pattern.exec(text)?.[0] === text; }
// JSON에서 온 객체의 허용/필수 키를 검사한다. 코드를 실행하는 reviver는 쓰지 않는다.
function fields(value, allowed, required, label) { check(value !== null && typeof value === 'object' && !Array.isArray(value) && Object.keys(value).every((key) => allowed.includes(key)) && required.every((key) => Object.hasOwn(value, key)), `${label} 필드 오류`); }
// UTF-8로 동일하게 재표현할 수 없는 단독 surrogate를 거절한다.
function bytes(text) { check(typeof text === 'string' && !/[\uD800-\uDFFF]/u.test(text), '패키지의 UTF-8 문자열 오류'); return new TextEncoder().encode(text).length; }

// 모듈 사전의 정규 경로를 검사한다. URL·드라이브·구분자 우회는 어휘 단계에서 거절한다.
export function validateModPath(path) {
    check(exact(PATH, path) && path.length <= 128, '허용되지 않은 모듈 경로', 'permissionDenied');
    // 예약 이름은 디렉터리와 마지막 파일 basename 모두 검사한다.
    for (const part of path.slice(0, -3).split('/')) check(!RESERVED.test(part), 'Windows 예약 모듈 이름', 'permissionDenied');
    return path;
}

// 패키지 안의 ./ 또는 ../ 정적 상대 참조만 정규화한다. 실제 파일/네트워크는 조회하지 않는다.
export function resolveModSpecifier(specifier, referrer) {
    validateModPath(referrer);
    check(typeof specifier === 'string' && specifier.length <= 256 && (specifier.startsWith('./') || specifier.startsWith('../')), '정적 상대 모듈만 허용합니다.', 'permissionDenied');
    const parts = referrer.split('/'); parts.pop();
    // 각 구간을 검증하며 패키지 루트 위로 올라가는 참조는 즉시 거절한다.
    for (const part of specifier.split('/')) {
        if (part === '.') continue;
        if (part === '..') { check(parts.length > 0, '모드 패키지 밖 import 금지', 'permissionDenied'); parts.pop(); }
        else { check(part.length > 0, '빈 모듈 경로 구간 금지', 'permissionDenied'); parts.push(part); }
    }
    return validateModPath(parts.join('/'));
}

// 분석기 예외를 모드 패키지 오류로 바꾸며 AST를 실행하지 않는다.
function syntax(parse, source, label) {
    try { return parse(source, { ecmaVersion: 'latest', sourceType: 'module' }); }
    catch (error) { throw new ModError('invalidPackage', `${label} 구문 오류: ${String(error?.message ?? error).slice(0, 512)}`); }
}
// AST를 반복 순회해 깊은 코드에서 별도 재귀 스택을 만들지 않는다.
function visit(root, callback) {
    const pending = [root]; let count = 0;
    // 제한된 패키지에서 유한한 노드만 순회한다.
    while (pending.length) {
        const node = pending.pop(); if (!node || typeof node !== 'object') continue;
        check(++count <= 262144, '모드 구문 트리 한도 초과'); callback(node);
        // ESTree의 자식 객체/배열만 쌓고 원문 위치/문자열은 순회하지 않는다.
        for (const value of Object.values(node)) {
            if (Array.isArray(value)) { for (const child of value) if (child && typeof child === 'object') pending.push(child); }
            else if (value && typeof value === 'object') pending.push(value);
        }
    }
}

// 파일 내용은 UTF-8 바이트 또는 문자열로 받는다. 불러오기/검증만 하며 어떤 엔트리도 호출하지 않는다.
export function readModPackage(input, { parse }) {
    check(typeof parse === 'function', '모드 구문 분석기 누락');
    let text = input;
    if (input instanceof Uint8Array) {
        check(input.byteLength <= MOD_LIMITS.maxPackageBytes, '모드 패키지 바이트 한도 초과');
        try { text = new TextDecoder('utf-8', { fatal: true }).decode(input); } catch (error) { throw new ModError('invalidPackage', '모드 파일은 올바른 UTF-8이어야 합니다.'); }
    }
    check(typeof text === 'string' && text.length <= MOD_LIMITS.maxPackageBytes && bytes(text) <= MOD_LIMITS.maxPackageBytes, '모드 패키지 바이트 한도 초과');
    let pkg; try { pkg = JSON.parse(text); } catch (error) { throw new ModError('invalidPackage', '모드 패키지 JSON 구문 오류'); }
    fields(pkg, ['format', 'formatVersion', 'manifest', 'modules'], ['format', 'formatVersion', 'manifest', 'modules'], '패키지');
    check(pkg.format === 'netstorm-mod-package', '알 수 없는 모드 파일 형식');
    check(pkg.formatVersion === 1, '지원하지 않는 모드 파일 버전', 'unsupportedVersion');
    const manifest = pkg.manifest;
    fields(manifest, ['id', 'name', 'version', 'apiVersion', 'entry', 'description'], ['id', 'name', 'version', 'apiVersion', 'entry'], '모드 선언');
    check(exact(ID, manifest.id) && manifest.id.length <= 64, '모드 id 형식 오류');
    check(typeof manifest.name === 'string' && manifest.name.length >= 1 && manifest.name.length <= 80 && bytes(manifest.name) > 0, '모드 표시 이름 오류');
    check(exact(VERSION, manifest.version) && manifest.version.length <= 32, '모드 버전 형식 오류');
    check(manifest.apiVersion === MOD_API_VERSION, '지원하지 않는 모드 API 버전', 'unsupportedVersion');
    validateModPath(manifest.entry);
    if (Object.hasOwn(manifest, 'description')) check(typeof manifest.description === 'string' && manifest.description.length <= 1024 && bytes(manifest.description) >= 0, '모드 설명 형식 오류');
    check(pkg.modules !== null && typeof pkg.modules === 'object' && !Array.isArray(pkg.modules), '모듈 사전이 필요합니다.');
    const names = Object.keys(pkg.modules).sort(), folded = new Set();
    check(names.length >= 1 && names.length <= MOD_LIMITS.maxModules, '모듈 개수 한도 초과');
    // 모듈 키·원문 크기를 분석 전에 검사하고 Windows 대소문자 충돌도 닫는다.
    for (const name of names) {
        validateModPath(name); const lower = name.toLowerCase(); check(!folded.has(lower), '대소문자만 다른 모듈 경로 충돌'); folded.add(lower);
        check(typeof pkg.modules[name] === 'string' && pkg.modules[name].length <= MOD_LIMITS.maxModuleBytes && bytes(pkg.modules[name]) <= MOD_LIMITS.maxModuleBytes, '모듈 원문 바이트 한도 초과');
    }
    check(Object.hasOwn(pkg.modules, manifest.entry), '엔트리 모듈이 없습니다.', 'moduleNotFound');
    // JSON.parse가 덮어쓴 중복 키도 AST에서 찾아 두 플랫폼의 해석 차이를 막는다.
    visit(syntax(parse, `(${text}\n)`, '패키지 JSON'), (node) => {
        if (node.type !== 'ObjectExpression') return;
        const keys = new Set();
        // JSON 객체의 모든 키는 문자열 Literal이다.
        for (const property of node.properties) { const key = property.key.value; check(!keys.has(key), '패키지 JSON의 중복 키'); keys.add(key); }
    });
    // 사용하지 않는 모듈도 검사한다. 파일 선택 시 코드가 실행되지는 않는다.
    for (const name of names) {
        let defaultExport = false;
        visit(syntax(parse, pkg.modules[name], name), (node) => {
            check(node.type !== 'ImportExpression' && !(node.type === 'MetaProperty' && node.meta?.name === 'import'), '동적 import/import.meta 금지', 'permissionDenied');
            check(!node.async && node.type !== 'AwaitExpression' && !node.await, 'v0 모드는 동기 코드만 허용합니다.', 'permissionDenied');
            if (node.type === 'ExportDefaultDeclaration' || node.specifiers?.some((item) => (item.exported?.name ?? item.exported?.value) === 'default')) defaultExport = true;
            if (['ImportDeclaration', 'ExportNamedDeclaration', 'ExportAllDeclaration'].includes(node.type) && node.source) {
                check(!node.attributes?.length && !node.assertions?.length && !node.phase, '모듈 import 속성 금지', 'permissionDenied');
                const target = resolveModSpecifier(node.source.value, name); check(Object.hasOwn(pkg.modules, target), `모듈이 없습니다: ${target}`, 'moduleNotFound');
            }
        });
        if (name === manifest.entry) check(defaultExport, '엔트리의 default export가 없습니다.');
    }
    // 직렬화 순서만 고정하고 코드 줄바꿈/유니코드는 변환하지 않는다.
    const declaration = { id: manifest.id, name: manifest.name, version: manifest.version, apiVersion: manifest.apiVersion, entry: manifest.entry, ...(Object.hasOwn(manifest, 'description') ? { description: manifest.description } : {}) };
    const modules = Object.create(null); for (const name of names) modules[name] = pkg.modules[name];
    return Object.freeze({ format: pkg.format, formatVersion: 1, manifest: Object.freeze(declaration), modules: Object.freeze(modules) });
}
