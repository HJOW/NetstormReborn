# 반더 타워 발사와 캠페인 빔 출력 — 단위107

현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-16**으로 일치한다. AGENTS.md와 양 LEFT를 다시 읽었으며 직전 커밋은 `e6d9763`이다. 새 디컴파일·원본 게임/GUI 실행은 없다. 앞선 세 단위에서 복원한 실제 빔 경로·프레임/선·생성/Host 수명을 캠페인 발사와 클라이언트 출력에 연결한다.

## 원본 발사 순서

반더의 생성자 `004680f0`는 공통 Fire 뒤 outer `0050b170`/inner `0050b16c`를 쓴다. `00468130`은 현재 프레임에 **0xc000**을 OR하므로 발사와 완료를 같은 단계에서 처리한다. 공통 `00467330`의 due 이전 표적 추적, 프레임 변경, 발사 실패/초기 표적 생존 판정, 완료 통지는 그대로 사용한다. 속도20·지연float0.3·effect19·display80의 가상 값도 보존한다.

`00466f50`은 함수 진입의 source/target **raw 주소**를 보관한다. 초기 target이 dead/void이거나 실제 TYPE HP 플래그가 없으면0이다. targetPoint를 현재 인수로 호출한 뒤, 초기 source 주소의 현재 타입이148이면 **현재 fields.parent/target으로 빔을 생성**한다. 생성 뒤 다음 중 하나면 초기 source에 `004d5000` refresh를 한다.

- 초기 target 주소의 현재 dead/void 비트가 켜져 있다.
- 초기 target 주소의 현재 타입이146 Cloud Floater이며, 실제 RandomFloat(0,1000)가 signed DWORD800보다 작다.

그 밖은 **초기 source→현재 fields.target**으로 `004d4f00` Hit다. 반환은 초기 target 주소의 현재 생존 상태다. 초기 SID를 새로운 fields.parent/target으로 전부 교체하거나 표적 상태를 생성 전 값으로 캐시하면 다르다. 같은 raw 풀의 고정 메모리와 공유 `GameRandom`을 사용한다. 빔 확보가 실패해도 원문은 이후 피해/회피 판정을 계속한다.

완료 통지의 재진입에서도 차이를 찾았다. 기존 C++ Finish는 notify 전에 Form을 캐시해 변경 전 번호로 kill했다. 실제 `00466eb0`은 notify 뒤 this로 kill하므로 **현재 Form**을 사용한다. 이를 수정했으며 새 원문 callback 입력이 그 차이를 검사한다.

## wrapper와 실제 클라이언트

실제 `00472700/00472690`은 target dead/void이면0 대신 **해당 raw 주소**를 반환한다. 살아 있으면104BYTE 객체를 요청하고 확보 실패는0이다. 첫 wrapper의 파일명은 shortZap.wav, 둘째는 전달한 파일명/NULL이다. `CreateLightning`은 호스트 주소를 원본 raw 주소와 혼용하지 않도록 `TargetUnavailable+SID`, `AllocationFailed`, `Created+process`로 구별한다.

`RawCampaignParticles`의 process19도 process11과 **같은 RenderProcessSystem/Host/Kernel·게임 난수/시각·카메라·Renderer/VFX**를 사용한다. 중심은 현재 같은 `core.neighbors.Center`다. 공유 빔11색은 실제 GamePalette에서 앞선 원본 색 띠 몸체로 구성하고44BYTE를 실제 Windows heap에 할당한다. Draw는 원문대로195/85를 직접 사용한다. 빔만 있는 상태에서 Fiery 색 배열의 미생성 snapshot을 읽지 않게 했다.

파일명 음향은 같은 Session SoundList 조회와 실제 playOnceAt(x,y,token,0,0)으로 이어진다. `RawCampaignArcherBindings`는148 stage5에 실제 반더 Fire를 부착하며 시각도 같은 Kernel에서 갱신한다. Session은 빔 생성 뒤 초기 source의 실제 `RawGunHit.SetTime/Hit`를 사용한다. 맨 오 워의 기존 별도 fireSound는 유지하고 `00472690`의 빔 파일명은 NULL로 전달한다.

## 독립 원본 근거와 검증

`tools/decomp_vander_fire.py`는 실제 반더 생성/Step/공통 Frame/Launch·TYPE HP/타입 조회·실제 난수·두 wrapper를 실행한다. 공간/중심/코너, 별도 대조한 빔 ctor/메모리, 피해/refresh, Host 부착/조회/완료/삭제는 명시 외부 경계다. 미초기화 padding·네 raw 슬롯·난수·시각·효과 순서를 보존한다.

- 고유 **417입력**: 생성1, Step8, Launch160, Frame224, wrapper24.
- 두 정밀도 **834정상 반환·차이0**. 정밀도별 관찰을 고유 입력으로 중복 가산하지 않는다.
- dead/void·HP·Cloud Floater·난수 seed·due·발사/완료 동시 비트·할당 실패·파일명 NULL을 포함한다. 중심/코너/빔/피해/refresh/Unpop/notify callback 뒤 재진입도 방문한다.
- C++ 원본 대조는393개 발사/24개 wrapper의 전체64BYTE·네 슬롯200BYTE·난수·현재 시각·요청 순서다. 생성자의 Host 부착은 원문과 같은 명시 경계 입력으로 공급하며 실제 Host 연결은 자연 캠페인 검사와 구별한다.
- 실제 클라이언트 통합 검사는 Fiery 생성 전 빔만 있는 상태의 최초 소리·Renderer195/85 픽셀·소진 갱신·dead 표적 생성을 확인한다.

자료는 `cpppj/vander-fire.json`, `cpppj/tests/fixtures/vander-fire-x86.tsv`다. 새417과 기존 캐논5534·빔 수명55/프레임250·Fiery 전체 픽셀30·필요 에너지2352의 SHA/고유 입력을 감사했다. 기존 원문 기대값은 변경하지 않았다. CMake는 tests 한 파일의 실제 변경 전후 핀/이력만 갱신한다.

Release 전체 대상 빌드와 검사 입력 재설치 수정 빌드는 **경고0·오류0·종료0**이다. **393발사+24wrapper의 전체 원문이 C++와 일치**했고 실제 Renderer/음향/소진 검사도 통과해 새 집중3개가 통과했다. 기존 기대값은 유지했다. 자연 데모1은 기존145프레임/3.625초를 넘겨 **232프레임/5.8초 TYPE144 SID15082의 전투 가상 삭제**까지 진행했다. 전체 CTest 내부 **1445개 중1444통과·1실패,216.77초,종료8**이며 추가 실패는 없다. 실행기/빌드/검사 핸들은 모두 종료했다. 새 파일 UTF-8/LF·한국어 주석/공백을 확인했다. 실제 Renderer 연결과 모듈 원본 대조만으로 사용자 완주나 전체 그래픽/음향 동일성을 완료 처리하지 않는다.

다음144 삭제의 실제 VT는0050a388, +10은004af780, 전용 Pre(+14)는0045e7e0, Post(+18)는기존0044b9e0이다. Pre는 현재+8 WORD가 가리키는 자산을 가상+10(0)으로 먼저 삭제한 뒤0044b4b0을 호출한다. 이 전용 동작과 기존 Damageable 하위를 연결해야 하며 공통 삭제 whitelist로232를 우회하지 않는다.

인수인계 뒤 `20261017-07 CP 반더 발사와 캠페인 빔 출력 및 피해 연결`로 로컬 커밋한다. 로그 접두사는 `extracted/goal-vander-fire-20261017-`다. 보호 원본·AGENTS.md·dotnetpj는 수정하지 않으며 원격 push는 하지 않는다. 전체 GUI/입력·AI 전략·건설/경제·승패/저장/다음 미션·사용자 자연 완주는 계속 검증/복원 대상이다. 전체화면 DirectDraw는 이번1차 범위에서 제외한다.
