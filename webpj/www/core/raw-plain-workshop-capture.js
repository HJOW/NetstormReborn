// 일반 기본 그림과 빈 추가 그림을 공유하는 다섯 워크샾 자산의 정확한 분배 조건을 옮긴 모듈이다(cpppj `o/RawPlainWorkshopCapture.cpp/.h`, 10.78 전용).
//  - isPlainWorkshopCapture(raw): 슬롯 길이가 50 이상이고 TYPE 바이트(+10)와 비정렬 little endian 가상표 DWORD(+0)가 다섯 쌍 중 하나와 둘 다 일치할 때만 참이다.
//    아이스 캐논(129)·썬 바리케이드(136)·크로스보우(140)·맨오워 풀(143)·반더 타워(148)이며, 같은 가상표를 공유하는 128(별도 처리기)을 이름이나 주소 하나만으로 포함하지 않는다.
// 보호 PE 의 +78=4acfe0, +a0=41c0f0, +a4=41c100 에 연결되는 현재 슬롯만 허용한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E372: 5-A 착수의 `o/` 단위로 추가)
import { vtableOf } from './raw-plain-unit-capture.js';

// 다섯 워크샾의 (TYPE, 원본 가상표) 쌍이다. (2026-11-13 WB04-E372)
const WORKSHOPS = Object.freeze([[129, 0x50b9b8], [136, 0x509470], [140, 0x50b6e8], [143, 0x5099e8], [148, 0x50b7c0]]);

// 슬롯의 최소 길이이다. (2026-11-13 WB04-E372)
const MIN_LENGTH = 50;

// 이 슬롯이 일반 워크샾 캡처를 쓰는 정확한 TYPE/가상표 쌍인지 판별한다. 사용: isPlainWorkshopCapture(pool.slot(sid)). (2026-11-13 WB04-E372)
export function isPlainWorkshopCapture(raw) {
    if (raw.length < MIN_LENGTH) return false;
    const table = vtableOf(raw);
    // 현재 TYPE 도 함께 대조한다.
    for (const [type, value] of WORKSHOPS) if (raw[10] === type && table === value) return true;
    return false;
}
