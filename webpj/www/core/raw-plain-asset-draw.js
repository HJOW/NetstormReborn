// 원본 TYPE 기본 프레임/빈 추가 그림을 공유하는 자산의 실제 Draw 분배 조건을 옮긴 모듈이다(cpppj `o/RawPlainAssetDraw.cpp/.h`, 10.78 전용).
//  - isPlainAssetDraw(raw): 일반 유닛 캡처(raw-plain-unit-capture)이거나 일반 워크샾 캡처(raw-plain-workshop-capture)이거나, 아래 열 건물/제단/보호막 쌍과 TYPE·가상표가 둘 다 일치할 때 참이다.
//    검증한 TYPE/VT 쌍만 일반 Draw 로 연결하며 다른 파생의 전용 그림을 생략하지 않는다(같은 주소만 가진 다른 TYPE, 타입만 같은 미복원 가상 표는 거부).
// 열 건물의 +78 = 4acfe0, +a4 = 41c100 이 일반 건물·제단·사제 보호막의 공통 Draw 이다. 제단은 현재 BYTE 대신 원래 소유자 WORD 를, 보호막은 TYPE 발 기준점을 같은 공통 Draw 에서 쓴다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E372: 5-A 착수의 `o/` 단위로 추가. cpppj 이력: 2026-10-27 포스 필드 170/509388 추가, 2026-10-28 제단 164/5076b8·사제 보호막 167/507770 실제 생성자 VT 연결)
import { isPlainUnitCapture, vtableOf } from './raw-plain-unit-capture.js';
import { isPlainWorkshopCapture } from './raw-plain-workshop-capture.js';

// 일반 건물·제단·보호막·포스 필드 열 개의 (TYPE, 원본 가상표) 쌍이다. 78 과 147 은 같은 가상표를 공유한다. (2026-11-13 WB04-E372)
const BUILDINGS = Object.freeze([
    [78, 0x50e940], [79, 0x50ea08], [139, 0x509aa0], [142, 0x50ead0], [147, 0x50e940],
    [149, 0x50e668], [150, 0x50e888], [164, 0x5076b8], [167, 0x507770], [170, 0x509388],
]);

// 슬롯의 최소 길이이다. (2026-11-13 WB04-E372)
const MIN_LENGTH = 50;

// 이 슬롯이 일반 자산 Draw 분배 대상인지 판별한다. 사용: isPlainAssetDraw(pool.slot(sid)). (2026-11-13 WB04-E372)
export function isPlainAssetDraw(raw) {
    if (raw.length < MIN_LENGTH) return false;
    if (isPlainUnitCapture(raw) || isPlainWorkshopCapture(raw)) return true;
    const table = vtableOf(raw);
    // 정확한 원본 쌍만 일반화한다.
    for (const [type, value] of BUILDINGS) if (raw[10] === type && table === value) return true;
    return false;
}
