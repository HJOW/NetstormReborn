// Missile(투사체) 원본 128바이트 객체 이미지(MissileFields)의 읽기 접근을 옮긴 모듈이다(cpppj `o/RawMissile.h` 의 MissileFields, 10.78 전용).
// 반사 Missile(RawIceRicochet)과 이후 RawMissile 이식이 같은 이미지를 공유하도록 먼저 분리했다. 아직 RawMissile 의 생성·이동·피해 본체는 포함하지 않는다.
//  - MissileFields.bytes: 판본별 raw 객체 필드와 padding 이미지(0x80바이트)이다.
//  - form(+4) / parent(+8, 표시 자산 부모) / firing(+14, 발사 자산) / target(+18, 피해 표적): 실제 Baseprocess 의 SID WORD 를 읽는다(낮은 16비트).
//  - 그 밖의 필드(+c 반사 횟수·+10 통과·+1c 코너·+30 속도·+38/+3c 원점 좌표 등)는 소비자가 bytes 를 직접 읽는다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E124: RawIceRicochet 이식을 위해 RawMissile 의 필드 이미지만 먼저 추가)

// Missile 객체 이미지의 크기와 SID 필드 위치(Form/Parent/Firing/Target)이다. (2026-11-08 WB04-E124)
export const MISSILE_FIELD_SIZE = 0x80;
const FORM = 4;
const PARENT = 8;
const FIRING = 20;
const TARGET = 24;

// 미정렬 리틀엔디언 WORD 를 읽는다(원본은 DWORD 를 읽어 하위 16비트를 SID 로 쓴다). (2026-11-08 WB04-E124)
function sid(bytes, offset) {
    return (bytes[offset] | (bytes[offset + 1] << 8)) & 0xffff;
}

// 원본 128바이트 Missile 이미지이다. 사용: new MissileFields() 의 bytes 를 직접 읽고 쓴다. (2026-11-08 WB04-E124)
export class MissileFields {
    // 판본별 raw 객체 필드와 padding 이미지이다.
    bytes = new Uint8Array(MISSILE_FIELD_SIZE);

    // 실제 Baseprocess 의 Form SID, 표시 자산 부모, 발사 자산, 피해 표적 SID 이다. (2026-11-08 WB04-E124)
    get form() { return sid(this.bytes, FORM); }
    get parent() { return sid(this.bytes, PARENT); }
    get firing() { return sid(this.bytes, FIRING); }
    get target() { return sid(this.bytes, TARGET); }
}
