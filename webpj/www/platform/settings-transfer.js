// 설정과 캠페인 진행의 JSON 복사/복원을 담당한다. 저장소의 다른 앱 항목과 설치한 모드 패키지는 건드리지 않는다.
import { SETTING_DEFINITIONS, SETTINGS_STORAGE_KEY, SETTINGS_VERSION, normalizeSetting } from './settings.js';
import { PROGRESS_STORAGE_KEY, PROGRESS_VERSION, isProgressKey, normalizeProgressValue } from './progress.js';

// 게임 설정 백업의 종류·버전과 한 번에 입력할 수 있는 최대 문자 수다.
export const SETTINGS_TRANSFER_FORMAT = 'netstorm-settings';
export const SETTINGS_TRANSFER_VERSION = 1;
export const SETTINGS_TRANSFER_MAX_LENGTH = 1024 * 1024;

// 배열/null 대신 JSON 키/값 객체인지 검사한다.
function object(value) { return value !== null && typeof value === 'object' && !Array.isArray(value); }

// UI가 지원 언어로 안내할 수 있도록 고정 오류 종류를 붙인다.
function invalid(code) { const error = new Error(`설정 불러오기 실패: ${code}`); error.code = code; return error; }

// 전체 입력을 먼저 검증하고 값 종류를 정규화한다. 누락된 알려진 설정은 원본 기본값을 쓴다.
export function validateSettingsTransfer(value) {
    if (!object(value) || value.format !== SETTINGS_TRANSFER_FORMAT || value.version !== SETTINGS_TRANSFER_VERSION || !object(value.settings) || !object(value.progress)) throw invalid('format');
    const settings = {}, progress = {}, seen = new Set(); // 적용 전 완전히 검증할 설정/진행 사본과 대소문자 중복 검사다.
    for (const key of Object.keys(value.settings)) { // 알 수 없는 키와 prototype 속성은 설정으로 받아들이지 않는다.
        if (!Object.hasOwn(SETTING_DEFINITIONS, key)) throw invalid('settings');
    }
    for (const [key, definition] of Object.entries(SETTING_DEFINITIONS)) { // 빈 값/잘못된 음량 등을 조용히 기본값으로 덮지 않는다.
        const next = Object.hasOwn(value.settings, key) ? normalizeSetting(key, value.settings[key]) : definition.fallback; // 현재 키의 정규화 값이다.
        if (next === null) throw invalid('settings');
        settings[key] = next;
    }
    for (const [key, valueText] of Object.entries(value.progress)) { // 모든 Done* 키를 검사하고 중복 별칭은 거부한다.
        const next = normalizeProgressValue(valueText), lower = key.toLowerCase(); // 저장 형식에 맞춘 값과 정규화 이름이다.
        if (!isProgressKey(key) || next === null || seen.has(lower)) throw invalid('progress');
        seen.add(lower); progress[key] = next;
    }
    return { format: SETTINGS_TRANSFER_FORMAT, version: SETTINGS_TRANSFER_VERSION, settings, progress };
}

// 빈 입력은 취소(null)이며 잘못된 JSON은 저장소에 접근하기 전에 거부한다.
export function parseSettingsTransfer(source) {
    if (typeof source !== 'string' || source.length > SETTINGS_TRANSFER_MAX_LENGTH) throw invalid('format');
    if (!source.trim()) return null;
    let value; // JSON 해석 결과는 검증하기 전까지 적용하지 않는다.
    try { value = JSON.parse(source); } catch { throw invalid('json'); }
    return validateSettingsTransfer(value);
}

// 현재 메모리의 모든 저장 설정과 완료 값을 한 JSON에 담는다. 저장소가 차단되어도 복사는 가능하다.
export function serializeSettingsTransfer(settings, progress) {
    return JSON.stringify(validateSettingsTransfer({ format: SETTINGS_TRANSFER_FORMAT, version: SETTINGS_TRANSFER_VERSION, settings: settings.snapshot(), progress: progress.snapshot() }), null, 2);
}

// 두 항목을 교체하고 두 번째 쓰기 실패 시 첫 항목도 원래대로 복원한다. 이전 진행과 합치지 않으므로 과거 백업도 정확히 복구된다.
export function storeSettingsTransfer(storage, value) {
    const validated = validateSettingsTransfer(value); // 쓰기 전에 전체 입력을 다시 검사한다.
    if (!storage?.getItem || !storage?.setItem || !storage?.removeItem) throw invalid('storage');
    const records = [[SETTINGS_STORAGE_KEY, JSON.stringify({ version: SETTINGS_VERSION, values: validated.settings })], [PROGRESS_STORAGE_KEY, JSON.stringify({ version: PROGRESS_VERSION, values: validated.progress })]]; // 기존 저장 형식 두 항목이다.
    let previous; // 실패 시 원문 바이트 그대로 복원할 두 이전 값이다.
    try { previous = records.map(([key]) => [key, storage.getItem(key)]); } catch (cause) { const error = invalid('storage'); error.cause = cause; throw error; }
    let written = 0; // 실제 교체를 끝낸 항목만 되돌린다.
    try {
        for (const [key, text] of records) { storage.setItem(key, text); written += 1; } // 검증된 설정과 진행을 순서대로 한 번씩 쓴다.
    } catch (cause) {
        for (let index = written - 1; index >= 0; index -= 1) { // 뒤에서부터 원문 또는 부재 상태를 복원한다.
            const [key, text] = previous[index]; // 이번에 되돌릴 저장 항목이다.
            try { if (text === null) storage.removeItem(key); else storage.setItem(key, text); } catch (rollbackError) { console.error('Netstorm: 설정 불러오기 저장소 복원 실패', rollbackError); }
        }
        const error = invalid('storage'); error.cause = cause; throw error; // 현재 앱은 종료하지 않고 사용자가 입력을 다시 확인할 수 있게 한다.
    }
    return validated;
}
