// 실제 Acorn 구문 분석과 고장 가능한 저장소로 모드 설치/검증/보관의 경계를 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import { parse } from 'acorn';
import { readModPackage, resolveModSpecifier, validateModPath } from '../www/modding/mod-package.js';
import { ModStore, MOD_STORAGE_KEY } from '../www/modding/mod-store.js';
import { modRandomSeed, validateModSnapshot, emptyModSnapshot } from '../www/modding/mod-api.js';

// 평가 부작용을 감지할 코드를 포함한 정상 패키지다. 분석기가 실행하면 검사에 걸린다.
function packageObject(id = 'test.package') { return { format: 'netstorm-mod-package', formatVersion: 1, manifest: { id, name: '검사 모드', version: '1.0.0', apiVersion: '0.1.0', entry: 'main.js' }, modules: { 'main.js': 'globalThis.netstormPackageExecuted = true; export default function registerMod(api) {}' } }; }
// UTF-8 파일 선택과 같은 JSON 문자열을 만든다.
const source = (id) => JSON.stringify(packageObject(id));
// 호스트가 소유한 실제 구문 분석기를 주입한다.
const validate = (text) => readModPackage(text, { parse });
// 정해진 오류 코드를 비교한다.
const code = (expected) => (error) => error.code === expected;
// 읽기/쓰기/용량/다른 탭을 제어할 수 있는 WebStorage 장치다.
function storage() {
    const data = new Map();
    return { data, failRead: false, failWrite: false, getItem(key) { if (this.failRead) throw Object.assign(new Error('차단'), { name: 'SecurityError' }); return data.get(key) ?? null; }, setItem(key, value) { if (this.failWrite) throw Object.assign(new Error('가득 참'), { name: 'QuotaExceededError' }); data.set(key, value); }, removeItem(key) { if (this.failWrite) throw Object.assign(new Error('차단'), { name: 'SecurityError' }); data.delete(key); } };
}
// 정상 초기 로드를 거친 독립 저장소를 만든다.
async function store() { const backing = storage(), mods = new ModStore({ storage: backing, validatePackage: validate }); await mods.load(); return { backing, mods }; }

test('패키지 검증은 선언/코드 사본을 동결하며 엔트리를 실행하지 않는다', () => {
    const pkg = validate(source()); assert.equal(pkg.manifest.id, 'test.package'); assert.equal(globalThis.netstormPackageExecuted, undefined); assert.ok(Object.isFrozen(pkg)); assert.ok(Object.isFrozen(pkg.modules)); assert.ok(Object.isFrozen(pkg.manifest));
});

test('공통 관찰/적합성 패키지를 고치지 않고 구문 검증한다', async () => {
    // 두 패키지를 같은 검증기에 공급한다.
    for (const name of ['examples/session-observer.nsmod.json', 'conformance/core-probe.nsmod.json']) { const input = await readFile(new URL('../../shared/modding/v0/' + name, import.meta.url)); assert.equal(validate(input).manifest.apiVersion, '0.1.0'); }
});

test('정적 상대 import/re-export와 순환 그래프를 허용한다', () => {
    const pkg = packageObject(); pkg.modules = { 'main.js': "import {value} from './sub/a.js'; export {default} from './sub/a.js';", 'sub/a.js': "import '../main.js'; export const value = 1; export default function registerMod() {}" };
    assert.equal(validate(JSON.stringify(pkg)).modules['sub/a.js'], pkg.modules['sub/a.js']); assert.equal(resolveModSpecifier('../main.js', 'sub/a.js'), 'main.js'); assert.equal(resolveModSpecifier('./sub/../main.js', 'main.js'), 'main.js');
});

test('코멘트/문자열 안의 import/eval 문구는 코드로 오인하지 않는다', () => {
    const pkg = packageObject(); pkg.modules['main.js'] = "// import 'https://invalid.example/x.js'\nconst text = \"import('https://invalid.example')\"; export default function registerMod() {}";
    assert.equal(validate(JSON.stringify(pkg)).manifest.id, 'test.package');
});

test('루트 밖·URL·드라이브·인코딩·예약 이름과 빈 구간을 거절한다', () => {
    // 경로 문자열을 실행하거나 원래 파일 시스템에 접근하지 않는다.
    for (const path of ['../main.js', '/main.js', 'H:/main.js', 'a\\main.js', 'https://invalid.example/a.js', 'a//b.js', 'a/%2e.js', 'a.js\n', 'CON.js', 'foo/NUL/main.js', 'COM1.js', '__proto__.txt']) assert.throws(() => validateModPath(path), code('permissionDenied'));
    for (const specifier of ['../main.js', 'https://invalid.example/a.js', 'std', '/main.js', './a//main.js', './main.js?x', './main.js#x']) assert.throws(() => resolveModSpecifier(specifier, 'main.js'), code('permissionDenied'));
});

test('선언 스키마의 필수/추가 필드·버전·대소문자 충돌을 거절한다', () => {
    // 독립 패키지마다 한 가지 선언 오류를 넣는다.
    for (const change of [(pkg) => { delete pkg.manifest.name; }, (pkg) => { pkg.manifest.extra = 1; }, (pkg) => { pkg.extra = 1; }, (pkg) => { pkg.manifest.id = 'test\n'; }, (pkg) => { pkg.manifest.version = '01.0.0'; }, (pkg) => { pkg.manifest.version = '1.0.0\n'; }, (pkg) => { pkg.modules['MAIN.js'] = 'export default 1;'; }]) { const pkg = packageObject(); change(pkg); assert.throws(() => validate(JSON.stringify(pkg)), code('invalidPackage')); }
    const version = packageObject(); version.manifest.apiVersion = '0.2.0'; assert.throws(() => validate(JSON.stringify(version)), code('unsupportedVersion')); version.manifest.apiVersion = '0.1.0'; version.formatVersion = 2; assert.throws(() => validate(JSON.stringify(version)), code('unsupportedVersion'));
});

test('중복 JSON 키는 최종 값이 정상이어도 숨겨서 덮어쓰지 못한다', () => {
    const original = source(), duplicateRoot = original.replace('"format":', '"format":"ignored","format":'), duplicateModule = original.replace('"modules":{', '"modules":{"main.js":"throw 1;",');
    assert.throws(() => validate(duplicateRoot), code('invalidPackage')); assert.throws(() => validate(duplicateModule), code('invalidPackage'));
});

test('동적 import/import.meta·비동기/await·외부 export를 거절한다', () => {
    // 같은 결과 문자열을 만드는 정적 주석과 실제 구문을 구분한다.
    for (const text of ["export default function registerMod() { import('./a.js'); }", 'export default function registerMod() { return import.meta.url; }', 'export default async function registerMod() {}', 'await 1; export default function registerMod() {}', "export {default} from 'https://invalid.example/a.js';"]) { const pkg = packageObject(); pkg.modules['main.js'] = text; assert.throws(() => validate(JSON.stringify(pkg)), code('permissionDenied')); }
});

test('누락 모듈·누락 default·구문 오류와 잘못된 UTF-8을 구분한다', () => {
    const pkg = packageObject(); pkg.modules['main.js'] = "import './missing.js'; export default function registerMod() {}"; assert.throws(() => validate(JSON.stringify(pkg)), code('moduleNotFound'));
    pkg.modules['main.js'] = 'export const value = 1;'; assert.throws(() => validate(JSON.stringify(pkg)), code('invalidPackage'));
    pkg.modules['main.js'] = 'export default function ('; assert.throws(() => validate(JSON.stringify(pkg)), code('invalidPackage'));
    assert.throws(() => validate(new Uint8Array([0xc3, 0x28])), code('invalidPackage')); pkg.modules['main.js'] = 'export default "\ud800"'; assert.throws(() => validate(JSON.stringify(pkg)), code('invalidPackage'));
});

test('모듈 수·원문 바이트·전체 파일 바이트를 분석 전에 제한한다', () => {
    const pkg = packageObject(); for (let index = 0; index < 32; index += 1) pkg.modules[`other${index}.js`] = '';
    assert.throws(() => validate(JSON.stringify(pkg)), code('invalidPackage')); pkg.modules = { 'main.js': '가'.repeat(90000) }; assert.throws(() => validate(JSON.stringify(pkg)), code('invalidPackage')); assert.throws(() => validate(' '.repeat(1048577)), code('invalidPackage'));
});

test('id/미션 번호는 정규식 $의 마지막 줄바꿈 예외를 허용하지 않는다', () => {
    assert.throws(() => modRandomSeed(0, 'test.id\n'), code('invalidArgument'));
    const input = emptyModSnapshot(); input.state.sessionId = 'session-1\n'; input.state.mission = 'TheWarBegins'; assert.throws(() => validateModSnapshot(input), code('invalidArgument'));
});

test('신규 설치는 항상 꺼짐이고 새 저장소 수명에서 그대로 읽힌다', async () => {
    const { backing, mods } = await store(); await mods.install(source()); assert.equal(mods.list()[0].enabled, false); assert.equal(globalThis.netstormPackageExecuted, undefined);
    await mods.setEnabled('test.package', true); const fresh = new ModStore({ storage: backing, validatePackage: validate }); await fresh.load(); assert.equal(fresh.list()[0].enabled, true); assert.equal(globalThis.netstormPackageExecuted, undefined);
    const read = fresh.getPackage('test.package'); read.manifest.name = '수정'; assert.equal(fresh.list()[0].manifest.name, '검사 모드');
});

test('같은 id 교체는 명시 선택을 요구하고 새 코드는 다시 꺼진다', async () => {
    const { mods } = await store(); await mods.install(source()); await mods.setEnabled('test.package', true);
    const next = packageObject(); next.manifest.version = '2.0.0'; await assert.rejects(mods.install(JSON.stringify(next)), code('invalidPackage')); assert.equal(mods.list()[0].enabled, true); assert.equal(mods.list()[0].manifest.version, '1.0.0');
    await mods.install(JSON.stringify(next), { replace: true }); assert.equal(mods.list()[0].enabled, false); assert.equal(mods.list()[0].manifest.version, '2.0.0');
});

test('용량 오류는 이전 저장 원문과 메모리의 활성/설치 값을 보존한다', async () => {
    const { backing, mods } = await store(); await mods.install(source()); const old = backing.getItem(MOD_STORAGE_KEY); backing.failWrite = true;
    await assert.rejects(mods.setEnabled('test.package', true), code('quotaExceeded')); await assert.rejects(mods.install(source('test.second')), code('quotaExceeded'));
    assert.equal(backing.getItem(MOD_STORAGE_KEY), old); assert.equal(mods.list().length, 1); assert.equal(mods.list()[0].enabled, false);
    backing.failWrite = false; await mods.setEnabled('test.package', true); assert.equal(mods.list()[0].enabled, true);
});

test('손상/읽기 실패는 빈 목록으로 기존 자료를 덮어쓰지 않는다', async () => {
    const { backing, mods } = await store(); await mods.install(source()); backing.data.set(MOD_STORAGE_KEY, '{broken'); await assert.rejects(mods.load(), code('invalidPackage')); assert.equal(mods.list().length, 1); assert.equal(backing.getItem(MOD_STORAGE_KEY), '{broken');
    await assert.rejects(mods.remove('test.package'), code('permissionDenied')); backing.failRead = true; await assert.rejects(mods.load(), code('permissionDenied')); assert.equal(mods.list().length, 1);
    const absent = new ModStore({ storage: null, validatePackage: validate }); await assert.rejects(absent.load(), code('permissionDenied')); await assert.rejects(absent.install(source()), code('permissionDenied'));
});

test('다른 탭 변경을 감지해 더 오래된 목록을 저장하지 않는다', async () => {
    const { backing, mods } = await store(); await mods.install(source()); const other = new ModStore({ storage: backing, validatePackage: validate }); await other.load(); await other.setEnabled('test.package', true);
    const changed = backing.getItem(MOD_STORAGE_KEY); await assert.rejects(mods.setEnabled('test.package', false), code('permissionDenied')); assert.equal(backing.getItem(MOD_STORAGE_KEY), changed); await mods.load(); assert.equal(mods.list()[0].enabled, true);
});

test('겹친 설치/설정 변경을 직렬화하고 실패 뒤에도 다음 작업을 진행한다', async () => {
    const { mods } = await store(); const a = mods.install(source('test.a')), b = mods.install(source('test.b')); await Promise.all([a, b]); assert.deepEqual(mods.list().map((item) => item.manifest.id), ['test.a', 'test.b']);
    const failed = mods.install('bad JSON'), valid = mods.setEnabled('test.a', true); await assert.rejects(failed, code('invalidPackage')); await valid; assert.equal(mods.list()[0].enabled, true); assert.equal(mods.list()[1].enabled, false);
});

test('런타임 오류 기록은 해당 모드만 끄고 다른 활성 모드는 유지한다', async () => {
    const { mods } = await store(); await mods.install(source('test.a')); await mods.install(source('test.b')); await mods.setEnabled('test.a', true); await mods.setEnabled('test.b', true); await mods.setEnabled('test.a', false, { error: 'timeLimit: 실행 시간 초과' });
    assert.deepEqual(mods.list().map((item) => [item.enabled, item.lastError]), [[false, 'timeLimit: 실행 시간 초과'], [true, null]]);
});

test('설치 수 한도에서 교체/삭제·초기화는 여전히 가능하다', async () => {
    const { backing, mods } = await store(); for (let index = 0; index < 8; index += 1) await mods.install(source(`test.mod${index}`));
    await assert.rejects(mods.install(source('test.ninth')), code('quotaExceeded')); await mods.install(source('test.mod0'), { replace: true }); await mods.remove('test.mod0'); assert.equal(mods.list().length, 7); await mods.clear(); assert.deepEqual(mods.list(), []); assert.equal(backing.getItem(MOD_STORAGE_KEY), null);
});

test('ResetAll 반납 뒤 늦은 설치 검증/대기 쓰기는 삭제한 키를 복원하지 않는다', async () => {
    const backing = storage(); let release, started;
    const gate = new Promise((resolve) => { release = resolve; }), entered = new Promise((resolve) => { started = resolve; });
    const mods = new ModStore({ storage: backing, validatePackage: async (text) => { started(); await gate; return validate(text); } });
    await mods.load(); const install = mods.install(source()), queued = mods.setEnabled('test.package', true);
    const checks = [assert.rejects(install, code('disposed')), assert.rejects(queued, code('disposed'))];
    await entered; mods.dispose(); backing.removeItem(MOD_STORAGE_KEY); release(); await Promise.all(checks);
    assert.equal(backing.getItem(MOD_STORAGE_KEY), null); assert.deepEqual(mods.list(), []); await assert.rejects(mods.load(), code('disposed'));
});

test('앱 반납 도중 저장 로드 검증이 끝나도 메모리/저장 자료를 다시 설치하지 않는다', async () => {
    const { backing, mods } = await store(); await mods.install(source()); const before = backing.getItem(MOD_STORAGE_KEY); let release, started;
    const gate = new Promise((resolve) => { release = resolve; }), entered = new Promise((resolve) => { started = resolve; });
    const pending = new ModStore({ storage: backing, validatePackage: async (text) => { started(); await gate; return validate(text); } });
    const load = pending.load(), checked = assert.rejects(load, code('disposed')); await entered; pending.dispose(); release(); await checked;
    assert.deepEqual(pending.list(), []); assert.equal(backing.getItem(MOD_STORAGE_KEY), before);
});

// 현재 미션의 옛 VM 오류가 다음 미션용 교체/삭제를 덮지 않아야 한다.
test('조건부 오류 저장은 교체된 패키지와 삭제된 id를 다시 바꾸지 않는다', async () => {
    const { mods, backing } = await store(); await mods.install(source()); await mods.setEnabled('test.package', true);
    const old = JSON.stringify(mods.getPackage('test.package')), next = packageObject(); next.manifest.version = '2.0.0';
    await mods.install(JSON.stringify(next), { replace: true }); await mods.setEnabled('test.package', true); const saved = backing.getItem(MOD_STORAGE_KEY);
    await mods.setEnabled('test.package', false, { error: 'old VM failed', expectedPackageText: old });
    assert.equal(backing.getItem(MOD_STORAGE_KEY), saved); assert.equal(mods.list()[0].enabled, true); assert.equal(mods.list()[0].lastError, null);
    await mods.remove('test.package'); const removed = backing.getItem(MOD_STORAGE_KEY);
    await mods.setEnabled('test.package', false, { error: 'late failure', expectedPackageText: old }); assert.equal(backing.getItem(MOD_STORAGE_KEY), removed); assert.deepEqual(mods.list(), []);
});

// 비교를 관리자에서 먼저 수행하면 대기 중인 설치 뒤의 오류 쓰기와 경쟁하므로 저장 큐 안에서 판정한다.
test('새 패키지 설치/켜기 뒤에 대기한 옛 오류 통지는 새 활성 상태를 보존한다', async () => {
    const backing = storage(); let release, started;
    const gate = new Promise((resolve) => { release = resolve; }), entered = new Promise((resolve) => { started = resolve; });
    const mods = new ModStore({ storage: backing, validatePackage: async (text) => { const pkg = validate(text); if (pkg.manifest.version === '2.0.0') { started(); await gate; } return pkg; } });
    await mods.load(); await mods.install(source()); await mods.setEnabled('test.package', true); const old = JSON.stringify(mods.getPackage('test.package'));
    const next = packageObject(); next.manifest.version = '2.0.0'; const replacing = mods.install(JSON.stringify(next), { replace: true }); await entered;
    const enabling = mods.setEnabled('test.package', true), failing = mods.setEnabled('test.package', false, { error: 'old VM failed', expectedPackageText: old });
    release(); await replacing; await enabling; await failing; assert.equal(mods.list()[0].manifest.version, '2.0.0'); assert.equal(mods.list()[0].enabled, true); assert.equal(mods.list()[0].lastError, null);
});
