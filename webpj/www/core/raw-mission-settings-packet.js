// 실제 로그인 마지막의 7DWORD 현재 설정 송신(0x32)과 수신을 옮긴 모듈이다(cpppj `o/RawMissionSettingsPacket.cpp/.h`, 10.78: 0044dbc0→0040a700/004df230 송신 작성, 0044dc20 수신 직접 저장, observer 0040b380).
// 사용: const settings = new RawMissionSettingsPacket({ current(field), setCurrent(field, value), send(target, bytes) }); settings.send(9); settings.receive(packet); registerMissionSettingsPacket(dispatch, settings | current7);
//   hooks.current(field) = 각 원본 load 위치에서 같은 현재 DWORD 를 읽는 실제 getter(설정 사본/캐시 없음), setCurrent(field, value) = 0044dc20 의 각 store 위치에서 같은 DWORD 를 직접 쓰는 setter, send(target, bytes) = 원본 004e2dc0 의 실제 32BYTE 송신 경계.
// payload 필드 번호(MISSION_SETTINGS_FIELD)와 실제 조회 순서는 다르다: 송신은 원문 asm 의 load 순서 1,2,0,4,5,3,6 으로 읽고 읽은 값은 다음 callback 변화에도 보존하며, 수신은 payload 순서 0..6 으로 현재 packet 바이트를 매번 다시 읽어 쓴다.
// 기준선 뒤 cpppj 변경(52eae6bc0·a526af5e4·ae0e58b3a, 3.6절 ②): 비로그인(Editor/현재 Session)이 송신 객체 없이 같은 7DWORD 수신을 등록하는 경로(`registerMissionSettingsPacket(dispatch, current7)`)와 직접 쓰기(`receiveCurrentMissionSettingsPacket`)를 반영했다.
//   실제 장기 Native 32h 수신기를 대여하는 분기(a526af5e4)는 E306 의 ③(고정 Native 수신기)과 같은 묶음이라 옮기지 않았다. CD(10.72)는 옮기지 않았다(판본 인자 없음).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E316: 4-C RawMissionSettingsPacket 이식)

// payload 의 필드 번호이다(+4 GeneratorTimer 부터 +0x1c CombatGumpRows 까지). 번호 순서는 실제 현재 조회 순서와 다르다. (2026-11-11 WB04-E316)
export const MISSION_SETTINGS_FIELD = Object.freeze({
    GeneratorTimer: 0, // +4, 005326cc 의 현재 제네레이터 타이머 설정
    HackMover: 1, // +8, 0055a488 의 현재 이동 디버그 설정
    NoDamageDebug: 2, // +c, 0055a48c 의 현재 피해 디버그 설정
    ShootMyFriends: 3, // +10, 0055a490 의 현재 친군 사격 설정
    FillMyTank: 4, // +14, 0055a494 의 현재 탱크 설정 DWORD
    CombatGumpColumns: 5, // +18, 005326d0 의 현재 전투 패널 열 수
    CombatGumpRows: 6, // +1c, 005326d4 의 현재 전투 패널 행 수
});

// 0x32 패킷 타입과 길이(32바이트)이다. (2026-11-11 WB04-E316)
const SETTINGS_TYPE = 0x32;
const SETTINGS_LENGTH = 32;
const FIELD = MISSION_SETTINGS_FIELD;

// 패킷의 비정렬 DWORD 를 원본 little endian 으로 쓴다. (2026-11-11 WB04-E316)
function putDword(packet, offset, value) {
    // 각 현재 DWORD 의 네 BYTE 를 낮은 위치부터 기록한다.
    for (let byte = 0; byte < 4; byte += 1) packet[offset + byte] = (value >>> (byte * 8)) & 0xff;
}

// 수신의 현재 원본 DWORD 를 읽되 실제 물리 부족을 범위 밖 읽기로 바꾸지 않고 거부한다. (2026-11-11 WB04-E316)
function readDword(packet, offset) {
    if (!Number.isInteger(offset) || offset < 0 || packet.length - offset < 4) throw new RangeError('미션 설정 패킷의 현재 DWORD 물리 범위 부족');
    let value = 0;
    // 현재 네 BYTE 를 낮은 자리부터 조립한다.
    for (let byte = 0; byte < 4; byte += 1) value |= packet[offset + byte] << (byte * 8);
    return value >>> 0;
}

// 설정 사본을 두지 않고 송신마다 원본 읽기 순서로 현재값을 전송하는 몸체이다. (2026-11-11 WB04-E316)
export class RawMissionSettingsPacket {
    // 사용: 현재 getter 의 대상은 이 객체보다 오래 살아야 한다. 현재 설정 조회가 없으면 성공 상수로 채우지 않고 거부한다. (2026-11-11 WB04-E316)
    constructor(hooks) {
        if (typeof hooks?.current !== 'function') throw new TypeError('미션 설정 패킷의 PATCH 현재 설정 조회 누락');
        this.hooks = hooks;
    }

    // 0044dbc0→0040a700/004df230: version 1 / type 0x32 / length 32 와 7DWORD 를 작성해 실제 송신을 요청한다. 원문 asm 의 load 순서는 1,2,0,4,5,3,6 이며 읽은 값은 다음 callback 변화에도 보존된다. 송신 장치가 없으면 거부한다. (2026-11-11 WB04-E316)
    send(target) {
        const hooks = this.hooks;
        const packet = new Uint8Array(SETTINGS_LENGTH);
        packet[0] = 1;
        packet[1] = SETTINGS_TYPE;
        packet[2] = SETTINGS_LENGTH;
        const hack = hooks.current(FIELD.HackMover) >>> 0;
        const noDamage = hooks.current(FIELD.NoDamageDebug) >>> 0;
        const generator = hooks.current(FIELD.GeneratorTimer) >>> 0;
        putDword(packet, 8, hack);
        const fill = hooks.current(FIELD.FillMyTank) >>> 0;
        putDword(packet, 12, noDamage);
        const columns = hooks.current(FIELD.CombatGumpColumns) >>> 0;
        putDword(packet, 4, generator);
        const shoot = hooks.current(FIELD.ShootMyFriends) >>> 0;
        putDword(packet, 20, fill);
        putDword(packet, 24, columns);
        putDword(packet, 16, shoot);
        const rows = hooks.current(FIELD.CombatGumpRows) >>> 0;
        putDword(packet, 28, rows);
        if (typeof hooks.send !== 'function') throw new Error('미션 설정 패킷의 실제 현재 연결 송신 미연결');
        hooks.send(target | 0, packet);
    }

    // 0044dc20: 현재 packet+4 부터 각 DWORD 를 재조회하고 0..6 순서로 같은 설정에 쓴다(앞선 store 는 물리 부족에도 보존). 헤더 검증은 실제 분배기가 담당한다. (2026-11-11 WB04-E316)
    receive(packet) {
        // 실제 payload 순서 0..6 으로 쓰며 송신의 서로 다른 read 순서를 수신에 적용하지 않는다.
        for (let index = 0; index < 7; index += 1) {
            const value = readDword(packet, 4 + index * 4);
            if (typeof this.hooks.setCurrent !== 'function') throw new Error('미션 설정 패킷의 실제 현재 설정 쓰기 미연결');
            this.hooks.setCurrent(index, value);
        }
    }
}

// 비로그인에서도 쓰는 0044dc20 의 실제 7개 현재 DWORD 직접 쓰기이다: 앞선 store 는 보존하고 다음 packet DWORD 를 그때 읽는다. Session/송신자/연결·UI 관문은 없으며 사본/정규화/새 설정 초기화가 아니다. (2026-11-11 WB04-E316)
export function receiveCurrentMissionSettingsPacket(current, packet) {
    if (!current || current.length !== 7) throw new TypeError('미션 설정의 현재 DWORD 7개가 필요합니다.');
    // 원문 44dc20 의 7개 DWORD 직접 쓰기 순서이다.
    for (let index = 0; index < current.length; index += 1) current[index] = readDword(packet, 4 + index * 4);
}

// 0040b91a 가 선택한 실제 0x32 의 버전 1/정책 0 수신기를 등록한다. 첫 인자 형태에 따라 두 경로이다:
//  - RawMissionSettingsPacket(로그인 객체): observer 는 인자를 버리는 ret4 뿐이라 송신/수신 모두 추가 효과 없이 자연 반환하고 수신기는 객체의 수신을 부른다.
//  - 7칸 현재 설정 배열(Editor/현재 Session): 송신 객체 없이 같은 원문 7DWORD 수신을 등록하고 observer 는 비우며 길이 32 를 요구한다.
// (2026-11-11 WB04-E316)
export function registerMissionSettingsPacket(dispatch, target) {
    if (target instanceof RawMissionSettingsPacket) {
        dispatch.observe(SETTINGS_TYPE, () => {});
        dispatch.version(SETTINGS_TYPE, 1, 0);
        dispatch.register(SETTINGS_TYPE, (sender, connection, bytes) => { target.receive(bytes); });
        return;
    }
    if (!target || target.length !== 7) throw new TypeError('미션 설정 등록은 설정 객체 또는 7칸 현재 설정 배열이어야 합니다.');
    dispatch.observe(SETTINGS_TYPE, null);
    dispatch.version(SETTINGS_TYPE, 1, 0);
    dispatch.requireLength(SETTINGS_TYPE, SETTINGS_LENGTH);
    dispatch.register(SETTINGS_TYPE, (sender, connection, bytes) => { receiveCurrentMissionSettingsPacket(target, bytes); });
}
