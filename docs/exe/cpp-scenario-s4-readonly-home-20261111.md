# S4: 읽기 전용 설치와 실제 사용자 홈에서의 저장·불러오기·Editor/Test Battle (제품 1282, 2026-11-11)

마지막 디컴파일 PC 호스트명: HJOW-Athlon(변경 없음). 제품 소스 변경 없음(문서·실행 확인).

## 1. 방법

1. 현재 빌드로 배포 ZIP을 새로 만들고(1430항목) `tools/verify_cpp_package.py --extract`로 원천과 SHA-256 전수 대조(통과)한 뒤 짧은 경로에 풀었다.
2. 푼 설치 폴더(`NetstormReborn-cpppj`, 게임 자료 `game\d` 포함)에 현재 사용자 쓰기·삭제 거부 ACL(`icacls /deny USER:(OI)(CI)(WD,AD,WEA,WA,DC,DE)`)을 걸고 폴더 루트와 `game\d`에 파일 생성이 실제로 거부되는지 확인했다(읽기·실행은 허용).
3. 환경 변수 `NETSTORM_USER_DATA`를 **주지 않고** 설치된 `Netstorm.exe --run game --mcp --window --no-audio`를 실행해 실제 사용자 홈(`C:\Users\<사용자>\Saved Games\Netstorm`)을 쓰게 했다. 그 폴더는 이 시험 전에 없었다.
4. 흐름: 메인 → `Edit`(첫 Edit에서 원본 맵 복사) → 페이지 이동 → `TEST01` → 편집 화면 → `Edit → Scenario...` → `Player start` → `myStartMoney`를 6666으로 편집 → `Save` → `Game → Test Battle` → 브리핑 `Go!` → 전투 진입·대기 → 정상 종료.

## 2. 결과

- 종료 코드 0, 오류 로그 파일 없음(`logs` 폴더 자체가 만들어지지 않음).
- 설치 폴더: 파일 1430개/242,052,004바이트로 ZIP과 동일하다(쓰기 거부 상태에서 어떤 파일도 생성/수정되지 않음). `game` 자료 매니페스트 불변(탐침의 `dataUnchanged`).
- 실제 사용자 홈에 만들어진 것: `Cache\!color.dat`, `CustomMaps\D\`(원본 433맵 복사 + 편집한 TEST01.english, 434파일), `Editor\catalog-v1.dat`·`catalog.lock`, `Saves\options.cfg`. 편집한 `TEST01.english`에는 `myStartMoney="6666"`이 기록되었다.
- 시험 뒤 이 폴더는 사용자 홈에 원래 없던 시험 산물이라 삭제했고, ACL 거부도 풀었다.

## 3. 같은 시험에서 발견한 것

- 첫 시도는 **이전 ZIP(교정 전 exe)** 로 실행해 Test Battle에서 1282에서 고친 `복합 전송이 소유하지 않은 현재 수신 노드` 오류로 종료했다. 새 오류 로그가 홈 `logs`에 정확히 남았고(설치 폴더에는 쓰지 않음) 새 ZIP으로 다시 만들어 통과했다. 배포 ZIP은 코드를 고칠 때마다 다시 만들어야 한다는 점을 M8 최종 재실행 지침에 이미 기록했다.
- `icacls`에 단순 권한 `W`를 거부로 주면 실행 파일 로드(READ_CONTROL/SYNCHRONIZE 포함)까지 막혀 프로세스 생성이 실패한다. 읽기 전용 설치 시험에는 `WD,AD,WEA,WA,DC,DE`만 거부해야 한다.

## 4. 한계

- 읽기 전용 설치 시험은 쓰기 거부 ACL로 모의했다(예: Program Files 설치의 UAC 가상화는 확인하지 않음). 실제 Program Files 배치·관리자 권한 설치는 사용자 확인 대상이다.
- 첫 Edit이 원본 433맵을 홈으로 복사하는 양(약 수 MB)과 시간은 이 단위의 범위 밖이다.
