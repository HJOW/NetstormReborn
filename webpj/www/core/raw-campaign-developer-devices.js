// 31h 개발자 명령의 힙 추적·DirectDraw 조회·기록 라벨을 현재 Client 장치에 전달한다.
// 사용: executeDeveloperDevices(text, cursor, services, trace) → 처리 여부. 선택된 장치만 요구한다.
import { parseDeveloperCommand, readDeveloperIntegers } from './raw-campaign-developer-settings.js';

// 원본 명령의 입력 관문과 장치 호출 순서를 보존한다(225553df의 RawCampaignSession.cpp).
export function executeDeveloperDevices(text, cursor, services, trace) {
    const { key, rest } = parseDeveloperCommand(text);
    if (key === 'heaptrace' || key === 'ht') {
        const value = readDeveloperIntegers(rest, 1)[0];
        if (value === undefined) {
            if (typeof trace !== 'function') throw new Error('개발자 명령의 실제 추적 장치 미연결');
            trace(29, 'bad command\n'); return true;
        }
        if (typeof services.developerHeapTrace !== 'function') throw new Error('개발자 heaptrace의 실제 Client 프레임 미연결');
        services.developerHeapTrace(value !== 0); return true;
    }
    if (key === 'querydirectdraw' || key === 'qdd') {
        if (typeof services.developerQueryDirectDraw !== 'function') throw new Error('개발자 qdd의 실제 시스템 버전 장치 미연결');
        services.developerQueryDirectDraw(); return true;
    }
    if (key === 'addlabel' || key === 'al') {
        if (!cursor.recording) return true;
        const token = /^[\t\n\v\f\r ]*([^\t\n\v\f\r ]+)/.exec(rest);
        if (!token) return true;
        if (typeof services.developerRecordLabel !== 'function') throw new Error('개발자 addLabel의 실제 기록 파일 미연결');
        services.developerRecordLabel(token[1].slice(0, 255)); return true;
    }
    return false;
}
