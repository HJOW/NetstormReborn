// MissionBegin 단추가 가리키는 미션 이름의 별칭 해석이다(웹 전용, cpppj 대응 파일 없음).
// 사용:
//   resolveMissionBeginName('BC1Menu') → 'BC1Menu76'  (별칭이면 연결된 미션 이름, 아니면 입력을 그대로)
//   MISSION_NAME_ALIASES                               소문자 별칭 → 웹 자산에 실제로 있는 미션 이름 표(동결)
// 배경: 10.78 의 Priest Training 7(bc7sun_bars76) 완료 안내의 "Training Menu" 단추는 원본 대본이 `MissionBegin,BC1Menu`(76 접미 없음)라고 적혀 있다.
//   원본 실행 파일은 이름 그대로 d 폴더의 10.72판 loose 파일(BC1Menu.english)을 읽지만 웹 자산에는 그 파일이 없고 netstorm.tarc 의 `bc1menu76` 만 있다.
//   이 오타 하나 때문에 단추가 아무 일도 하지 않으므로, 사용자 결정(2026-11-23)에 따라 같은 훈련 메뉴인 bc1menu76 으로 연결한다.
//   (10.72판 loose 미션 사슬(BC2Bridging 등)을 자산으로 복사하는 방법은 쓰지 않았다.)
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-23 WB04-E1245)

// 소문자 별칭 → 연결할 실제 미션 이름이다. 이름 철자는 다른 Priest Training 대본이 쓰는 `BC1Menu76` 과 같게 둔다. (2026-11-23 WB04-E1245)
export const MISSION_NAME_ALIASES = Object.freeze({ bc1menu: 'BC1Menu76' });

// MissionBegin 인자를 받아 별칭이면 연결된 미션 이름을, 아니면 인자를 그대로 돌려준다. 별칭 비교는 대소문자와 앞뒤 공백을 무시한다.
// 문자열이 아닌 값(undefined 등)은 빈 문자열로 읽는다(원본 대본 인자 누락과 같은 취급). (2026-11-23 WB04-E1245)
export function resolveMissionBeginName(argument) {
    const name = String(argument ?? '');
    const alias = MISSION_NAME_ALIASES[name.trim().toLowerCase()];
    return alias ?? name;
}
