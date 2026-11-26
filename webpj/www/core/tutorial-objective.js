// 공식 초기 미션의 현재 단계와 영어 번역 키를 대응시킨다. 실제 완료 조건과 진행 상태는 기존 gate가 소유한다.
// 40단계 중 두 안내가 공유되므로 행동 키38개와 대기 키1개다. 항목과 목록은 모두 불변이다.
export const TUTORIAL_OBJECTIVES = Object.freeze([
    {
        "tutorial": 1,
        "stage": 65,
        "english": "Press F4 to return to your island, or place a bridge."
    },
    {
        "tutorial": 1,
        "stage": 66,
        "english": "Place a total of 8 bridges."
    },
    {
        "tutorial": 1,
        "stage": 67,
        "english": "Place a total of 19 bridges."
    },
    {
        "tutorial": 1,
        "stage": 68,
        "english": "Connect a Geyser to your Temple with bridges."
    },
    {
        "tutorial": 1,
        "stage": 69,
        "english": "Reach 200 Storm Power."
    },
    {
        "tutorial": 1,
        "stage": 70,
        "english": "Reach 600 Storm Power."
    },
    {
        "tutorial": 2,
        "stage": 65,
        "english": "Finish building a Temple."
    },
    {
        "tutorial": 2,
        "stage": 66,
        "english": "Finish building a Workshop."
    },
    {
        "tutorial": 2,
        "stage": 67,
        "english": "Put the Sun Disc Thrower into production."
    },
    {
        "tutorial": 2,
        "stage": 68,
        "english": "Build a total of 1 Sun Disc Thrower."
    },
    {
        "tutorial": 2,
        "stage": 69,
        "english": "Build a total of 2 Sun Disc Throwers."
    },
    {
        "tutorial": 2,
        "stage": 70,
        "english": "Select your Temple and wait 4 seconds."
    },
    {
        "tutorial": 2,
        "stage": 71,
        "english": "Build a total of 4 Sun Disc Throwers."
    },
    {
        "tutorial": 2,
        "stage": 72,
        "english": "Order a unit to Salvage, then wait 2 seconds."
    },
    {
        "tutorial": 3,
        "stage": 65,
        "english": "Have a Golem bring a Storm Crystal to your Temple."
    },
    {
        "tutorial": 3,
        "stage": 66,
        "english": "Connect your island to the enemy island with bridges."
    },
    {
        "tutorial": 3,
        "stage": 67,
        "english": "Put the Sun Disc Thrower into production."
    },
    {
        "tutorial": 3,
        "stage": 68,
        "english": "Put a Wind, Rain, or Thunder Generator into production."
    },
    {
        "tutorial": 3,
        "stage": 69,
        "english": "Extend your energy and build a Sun Disc Thrower on the enemy island."
    },
    {
        "tutorial": 3,
        "stage": 70,
        "english": "Attack the enemy Priest until his health is below half."
    },
    {
        "tutorial": 3,
        "stage": 71,
        "english": "Capture the enemy Priest with a Golem."
    },
    {
        "tutorial": 4,
        "stage": 65,
        "english": "Put the Sun Disc Thrower and Wind Generator into production."
    },
    {
        "tutorial": 4,
        "stage": 66,
        "english": "Destroy all enemy Sun Cannons."
    },
    {
        "tutorial": 4,
        "stage": 67,
        "english": "Put the Sun Cannon into production."
    },
    {
        "tutorial": 4,
        "stage": 68,
        "english": "Destroy all enemy Sun Disc Throwers."
    },
    {
        "tutorial": 4,
        "stage": 69,
        "english": "Capture the enemy Priest with a Golem."
    },
    {
        "tutorial": 4,
        "stage": 70,
        "english": "Build an Altar with your Priest."
    },
    {
        "tutorial": 4,
        "stage": 71,
        "english": "Bring the captured Priest to the center of your Altar."
    },
    {
        "tutorial": 4,
        "stage": 72,
        "english": "Start the ritual with your Priest and wait for the sacrifice."
    },
    {
        "tutorial": 5,
        "stage": 65,
        "english": "Build a Stone Tower in the Thunder Cannon line of fire so it takes a hit."
    },
    {
        "tutorial": 5,
        "stage": 66,
        "english": "Put Sun Disc Throwers, Sun Cannons, Stone Towers, and Wind Generators into production."
    },
    {
        "tutorial": 5,
        "stage": 67,
        "english": "Select the enemy Whirlibase and wait 5 seconds."
    },
    {
        "tutorial": 5,
        "stage": 68,
        "english": "Destroy all enemy Thunder Cannons and Whirlibases."
    },
    {
        "tutorial": 5,
        "stage": 69,
        "english": "Have a Golem learn a spell from an Obelisk."
    },
    {
        "tutorial": 5,
        "stage": 70,
        "english": "Capture the enemy Priest and sacrifice him at your Altar."
    },
    {
        "tutorial": 6,
        "stage": 65,
        "english": "Build an Air Ship and a Wind Generator."
    },
    {
        "tutorial": 6,
        "stage": 66,
        "english": "Have an Air Ship bring a Storm Crystal to your Temple."
    },
    {
        "tutorial": 6,
        "stage": 67,
        "english": "Put the Crossbow into production."
    },
    {
        "tutorial": 6,
        "stage": 68,
        "english": "Build a Crossbow."
    },
    {
        "tutorial": 6,
        "stage": 69,
        "english": "Destroy all enemy Sun Disc Throwers, Whirlibases, and Thunder Cannons."
    }
].map((entry) => Object.freeze(entry)));
// 단계 예약과 선택 뒤의 실제 지연 타이머가 사용하는 공통 번역 키다.
export const TUTORIAL_WAITING = "Wait for the next tutorial message.";

// 공식 미션 이름·번호와 현재 단계만 읽어 다음 알림 조건의 키를 반환한다. 범위 밖/완료 단계는 빈 문자열이다.
export function tutorialObjectiveKey(mission, gate, editing = 0) {
    if (!gate) throw new TypeError('튜토리얼 안내의 실제 gate 필요');
    if (editing || gate.demo || !gate.started || gate.tutorialNumber < 1 || gate.tutorialNumber > 6) return '';
    if (String(mission).replace(/[A-Z]/g, (letter) => letter.toLowerCase()) !== 'tutorial' + gate.tutorialNumber) return '';
    const entry = TUTORIAL_OBJECTIVES.find((value) => value.tutorial === gate.tutorialNumber && value.stage === gate.stage);
    if (!entry) return '';
    // 타이머는 Tutorial2 F/H와 Tutorial5 C에서만 다음 알림을 예약한다. NaN 비교도 원문 !=0과 같다.
    const timed = (entry.tutorial === 2 && (entry.stage === 70 || entry.stage === 72)) || (entry.tutorial === 5 && entry.stage === 67);
    return gate.lockFrames > 0 || (gate.timerSeconds !== 0 && timed) ? TUTORIAL_WAITING : entry.english;
}
