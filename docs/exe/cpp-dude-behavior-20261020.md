# Dude 부모의 정지·도착·지면 판정·삭제 후처리 — 단위243

작업은 **2026-10-20 시작·10-21 완료**, 시작 커밋은 `e2dfe0d0`다. 현재/마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-20**이다. 기존 정밀 산출물과 보호 PE를 읽었으며 새 디컴파일·원본/CD 변경은 없다. 앞 목표 턴은 단위240~242의 구현·기록·세 커밋을 만든 진행 턴이다.

## 실제 주소와 범위

| 동작 | 10.78 | 이번 처리 |
| --- | --- | --- |
| DudePath24 생성/가상 표 | `0048bf30` / `0050e3d8` | 사제54와 Init·Run·Arrive 공유, 별도 Attach임을 확인. 실제 경로 연결은 다음 단위 |
| 부모 Halt | `00450e60` | dead 또는 공간 전환12, 계속10 |
| 지면 판정 | `00450e80` | 거의 올림 위치의 표면·점유에 따른 FALL/5106 및 기존 경로 삭제 |
| FALL 설정 | `00450f50` | 기존 task 설정→현재 이동 종류→lookup/remove |
| 부모 Arrival | `00450960` | 현재 task/진단에 따른 가상 삭제와1000/1001 반환 |
| PostDestroy | `00450920` | 일반 객체의 population 중복 항목 전부 제거→공통4b0840 |

10.78 Dude70/가상 표 `00508cc8`의 실제 +38/+3c/+18을 PE에서 확인했다. Halt 진입점 `00450e60`은 기존 정밀 C에 별도 함수로 내보내져 있지 않아 보호 PE의 짧은 명령과 실제 실행 범위 `00450e60..00450e79`로 확인했다. 새 정밀 내보내기를 수행한 것으로 표시하지 않는다.

지면 판정의 원래 명령은 x/y float에 `00500edc=0.9999F`를 더하고 `_ftol`로 거의 올림한다. 디컴파일의 인자 생략만으로 좌표를 구현하지 않았다. 도착의 idle/FALL 진단은 진단 후 원래 fallthrough를 유지한다. free/dead와 낙하의 조기 반환 조건도 서로 구별한다.

CD10.72 Dude의 실제 +38/+3c/+18은 `004e9360`·`004e95c0`·`004e92a0`이다. 이번 제품/독립 실행 범위는 10.78이며 CD의 전체 부모 동작 동일성은 주장하지 않는다.

## 독립 관측과 구현

`tools/decomp_dude_behavior.py`는 제품 C++ 작성 전에 **410입력·두 정밀도820정상 반환·32,960명령**을 관측했다. 실제 Halt/지면 판정/FALL/Arrival/PostDestroy와40ea00 목록 압축·ftol을 실행했고, task 예약·이동 종류 선택·lookup/remove·가상 삭제·공통 PostDestroy·진단만 명시 인자 경계로 두었다. 게임/OS/창은 실행하지 않는다.

원본 관측에서 소수 좌표의 거의 올림 조회를 확인한 뒤 도구 주석을 명확하게 고치고 불필요한 Python 임시 대입을 제거했다. 최초 fixture/보고서는 `extracted/dude-behavior-20261020-first-capture/`에 보존했고, 재관측 fixture의 전체 바이트가 최초와 같은 SHA-256 `575864387a77b4e29d1644d40b1adf5296c2fbcfd1605d16a7c9fec2bb0ca2f5`임을 확인했다. 제품 기대값을 새 구현으로 계산하지 않았다.

`RawDudeBehavior`는 같은 지도·현재 task·diagnostic bool·population을 빌린다. PostDestroy는 모든 같은 SID를 안정 압축하고 count 밖 물리 꼬리를 보존한다. 실제 Session에는 기존 `RawDudeTask`·이동 종류 선택·`RawContainedLookup`·가상 삭제와 전체 공통 Pre/PostDestroy를 연결했다. 새로운 생성 한도나 빈 성공 콜백은 추가하지 않았다.

## 검증과 다음 연결

원본410행 C++ 대조와 실제 Session의 명시 Dude Pop→Arrival→고유/공통 삭제 확인을 위한 새 검사2개를 작성·컴파일했다. 단순 생성/삭제 연결이며 자연 드롭/대피나 실제 DudePath24 진행의 증거가 아니다. 첫 빌드의 목록 카운터 C4267 경고는 제거 수를 원본 count와 같은 DWORD로 맞춰 수정했다. 최종 전체 Release는 **경고0·오류0·종료0**, 독립 원본 감사·이번 감사와 직전 다이스 감사도 종료0이다. 현재 핀121보고서·330항목은 역변환하면 시작 커밋과 같다. 최종 빌드·해시·핀 보존은 `cpppj/dude-behavior-session.json`과 `python tools/cpp_dude_behavior_audit.py --verify`를 따른다. **사용자의 통합 검증 이월 지시대로 게임·C++ 단위 검사·CTest·전체 회귀를 실행하지 않는다.**

다음은 실제 DudePath24를 기존 공통 Init·보행 프레임·Arrive에 연결하고, 부모 Attach 검사·현재 경로 끝 조회·Notice의 현재2배 속도 공급을 연결한다. 이어450fa0 Regular 작업/후속 생성 정책을 복원한다. 현재 Session의 `hitEffects.dudes.pathEndpoint/startPath`와 Dude Regular는 아직 미연결이며 이 단위로 대피가 완료됐다고 표시하지 않는다.

지정11미션의 자연 플레이/전체 동일성·Early5 UI 시전/지형 효과/포획/Drop/제단·희생·F8 Guide도 통합 검증 대상으로 남는다. Devastation 외 주문의3차 이월을 유지한다.
