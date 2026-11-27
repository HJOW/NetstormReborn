// 설치와 실행을 분리하는 단일 키 저장소다. 검증/저장 실패는 기존 정상 목록을 덮어쓰지 않는다.
import { ModError, MOD_LIMITS } from './mod-api.js';

// Reset All도 이 키를 제거해야 한다. 기존 설정/진행 키와 독립이다.
export const MOD_STORAGE_KEY = 'netstorm.mods.v0';
// JSON 문자열을 다시 담는 저장소의 최악 이스케이프 크기까지 제한한다.
const MAX_STORE_CHARACTERS = MOD_LIMITS.maxInstalledMods * (2 * MOD_LIMITS.maxPackageBytes + 4096);
// 저장소 접근/용량 오류를 사용자에게 구분해 전달한다.
function storageError(error) { return new ModError(error?.name === 'QuotaExceededError' ? 'quotaExceeded' : 'permissionDenied', `모드 저장 실패: ${String(error?.message ?? error).slice(0, 512)}`); }

// validatePackage는 격리 구문 분석기에 위임할 수 있는 비동기 함수다. 게임 코드 실행은 맡기지 않는다.
export class ModStore {
    // 저장소와 신뢰 검증기를 소유하며 저장 성공 뒤에만 현재 목록을 교체한다.
    #storage; #validate; #items = new Map(); #raw = null; #loaded = false;
    // 가져오기/켜기/교체가 겹쳐 이전 상태를 덮어쓰지 않도록 작업을 직렬화한다.
    #tail = Promise.resolve();
    // 앱 초기화/ResetAll 뒤 대기 중인 검증 결과가 저장소를 다시 쓰지 못하게 한다.
    #disposed = false;
    // 브라우저 localStorage가 없으면 실패를 명시하고 메모리 저장 성공으로 꾸미지 않는다.
    constructor({ storage, validatePackage }) {
        if (typeof validatePackage !== 'function') throw new TypeError('모드 저장소의 패키지 검증기가 필요합니다.');
        this.#storage = storage; this.#validate = validatePackage;
    }
    // UI는 준비 전/반납 뒤 쓰기를 막되 일반 quota 오류 때 삭제/재시도를 허용한다.
    get loaded() { return this.#loaded && !this.#disposed; }
    // 이전 작업 실패도 다음 명시 조작을 막지 않는다.
    #enqueue(callback) { const result = this.#tail.then(() => { if (this.#disposed) throw new ModError('disposed', '모드 저장소 수명이 끝났습니다.'); return callback(); }); this.#tail = result.catch(() => {}); return result; }
    // 숨겨진 읽기 실패를 빈 목록으로 저장하지 않는다.
    #read() { try { if (!this.#storage) throw new Error('브라우저 저장소를 사용할 수 없습니다.'); return this.#storage.getItem(MOD_STORAGE_KEY); } catch (error) { throw storageError(error); } }
    // 명시 초기 읽기가 성공하기 전에는 기존 자료가 없는 것으로 가정하지 않는다.
    #ready() { if (this.#disposed) throw new ModError('disposed', '모드 저장소 수명이 끝났습니다.'); if (!this.#loaded) throw new ModError('permissionDenied', '모드 저장소를 먼저 읽어야 합니다.'); }
    // 현재 인스턴스가 읽은 뒤 다른 탭이 변경했다면 사용자 자료를 덮어쓰지 않는다.
    #unchanged() { this.#ready(); if (this.#read() !== this.#raw) throw new ModError('permissionDenied', '다른 창에서 모드 저장소가 바뀌었습니다. 다시 읽으십시오.'); }
    // 목록 전체를 단일 키에 저장한 뒤에만 메모리 상태를 확정한다.
    #save(items) {
        this.#unchanged();
        const mods = [...items.values()].sort((a, b) => a.pkg.manifest.id < b.pkg.manifest.id ? -1 : 1).map((item) => ({ package: item.text, enabled: item.enabled, lastError: item.lastError }));
        const raw = JSON.stringify({ version: 1, mods });
        try { this.#storage.setItem(MOD_STORAGE_KEY, raw); } catch (error) { throw storageError(error); }
        this.#items = items; this.#raw = raw;
    }
    // 저장된 선언/소스는 검증만 하며 켜진 모드도 실행하지 않는다. 손상 시 이전 목록/원문을 보존한다.
    load() {
        return this.#enqueue(async () => {
            const raw = this.#read(), items = new Map();
            if (raw !== null) {
                if (typeof raw !== 'string' || raw.length > MAX_STORE_CHARACTERS) throw new ModError('invalidPackage', '모드 저장소 크기 오류');
                let data; try { data = JSON.parse(raw); } catch (error) { throw new ModError('invalidPackage', '모드 저장소 JSON 손상'); }
                if (!data || typeof data !== 'object' || Object.keys(data).length !== 2 || data.version !== 1 || !Array.isArray(data.mods) || data.mods.length > MOD_LIMITS.maxInstalledMods) throw new ModError('invalidPackage', '모드 저장소 형식 오류');
                // 모든 항목이 정상이어야 현재 목록을 바꾼다. 중간 실패로 일부만 저장하지 않는다.
                for (const item of data.mods) {
                    if (!item || typeof item !== 'object' || Object.keys(item).length !== 3 || typeof item.package !== 'string' || typeof item.enabled !== 'boolean' || !(item.lastError === null || (typeof item.lastError === 'string' && item.lastError.length <= 2048))) throw new ModError('invalidPackage', '모드 저장 항목 오류');
                    const pkg = await this.#validate(item.package), id = pkg.manifest.id;
                    if (items.has(id)) throw new ModError('invalidPackage', '중복된 저장 모드 id');
                    items.set(id, { pkg, text: JSON.stringify(pkg), enabled: item.enabled, lastError: item.lastError });
                }
            }
            if (this.#disposed) throw new ModError('disposed', '모드 저장소 수명이 끝났습니다.');
            this.#items = items; this.#raw = raw; this.#loaded = true; return this.list();
        });
    }
    // UI에 필요한 선언/활성/오류만 사본으로 돌려준다. 코드 원문은 실행 요청 때 따로 읽는다.
    list() { return [...this.#items.values()].sort((a, b) => a.pkg.manifest.id < b.pkg.manifest.id ? -1 : 1).map((item) => ({ manifest: { ...item.pkg.manifest }, enabled: item.enabled, lastError: item.lastError })); }
    // 호스트 실행기가 받는 패키지도 사본이다.
    getPackage(id) { this.#ready(); const item = this.#items.get(id); if (!item) throw new ModError('invalidArgument', '설치되지 않은 모드 id'); return JSON.parse(item.text); }
    // 신규/명시 교체는 항상 꺼짐이다. 같은 id를 암묵적으로 덮어쓰지 않는다.
    install(text, { replace = false } = {}) {
        return this.#enqueue(async () => {
            this.#ready(); if (typeof replace !== 'boolean') throw new ModError('invalidArgument', '모드 교체 선택은 불리언이어야 합니다.');
            const pkg = await this.#validate(text), id = pkg.manifest.id, items = new Map(this.#items);
            if (items.has(id) && !replace) throw new ModError('invalidPackage', '같은 id의 모드가 있습니다. 명시적으로 교체하십시오.');
            if (!items.has(id) && items.size >= MOD_LIMITS.maxInstalledMods) throw new ModError('quotaExceeded', '모드 설치 개수 한도 초과');
            items.set(id, { pkg, text: JSON.stringify(pkg), enabled: false, lastError: null }); this.#save(items); return this.list();
        });
    }
    // 켜기/끄기는 저장이 성공해야 확정한다. 런타임의 즉시 비활성화는 실행 관리자가 먼저 수행한다.
    setEnabled(id, enabled, { error = null, expectedPackageText = null } = {}) {
        return this.#enqueue(() => {
            this.#ready(); if (typeof enabled !== 'boolean' || !(error === null || (typeof error === 'string' && error.length <= 2048)) || !(expectedPackageText === null || (typeof expectedPackageText === 'string' && expectedPackageText.length <= MOD_LIMITS.maxPackageBytes))) throw new ModError('invalidArgument', '모드 활성/오류 값 형식 오류');
            const item = this.#items.get(id);
            // 설치 검증과 같은 직렬 경계 안에서 비교해야 옛 VM의 늦은 오류가 새 교체본/삭제를 덮지 않는다.
            if (expectedPackageText !== null && (!item || item.text !== expectedPackageText)) return this.list();
            if (!item) throw new ModError('invalidArgument', '설치되지 않은 모드 id');
            const items = new Map(this.#items); items.set(id, { ...item, enabled, lastError: error }); this.#save(items); return this.list();
        });
    }
    // 설치 자료 삭제도 단일 저장이 성공한 뒤 목록에 반영한다.
    remove(id) { return this.#enqueue(() => { this.#ready(); const items = new Map(this.#items); if (!items.delete(id)) throw new ModError('invalidArgument', '설치되지 않은 모드 id'); this.#save(items); return this.list(); }); }
    // 정상 상태의 전체 삭제다. 손상 저장소의 강제 Reset All은 앱의 기존 초기화 절차가 직접 키를 지운다.
    clear() { return this.#enqueue(() => { this.#unchanged(); try { this.#storage.removeItem(MOD_STORAGE_KEY); } catch (error) { throw storageError(error); } this.#items = new Map(); this.#raw = null; return []; }); }
    // 이미 진행 중인 검증과 이후 대기 작업 모두 쓰기를 금지한다. 원래 저장 자료는 지우지 않는다.
    dispose() { this.#disposed = true; this.#items = new Map(); }
}
