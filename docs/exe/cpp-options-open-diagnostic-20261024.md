# options.cfg 간헐적 열기 실패의 읽기 진단

**실제 GuardMyBack 실패 원인은 아직 미확정이다.** 별도 합성 파일의 mapped section/view 보유로 UCRT `fopen(w+b)`의 errno22와 Python `Path.write_bytes`의 OSError22를 같은 기전으로 재현했지만, 실제 options.cfg 실패 당시의 OS 오류 코드나 보유 프로세스는 기록되지 않았다.

2026-10-24, 현재/마지막 전체 정밀 디컴파일 PC는 HJOW-Athlon /2026-10-23이다. 이 작업은 원본 옵션의 메타데이터·로그·PE/SDK 소스만 읽었다. 원본 옵션의 내용 쓰기·맵핑·게임/GUI·공용 빌드·기존 소스/CMake/LEFT/git 변경은 하지 않았다. 모든 합성 쓰기/맵핑은 새 [진단 자료](../../extracted/cpp-options-open-diagnostic-20261024) 또는 그 JSON에 기록한 새 C: 임시 디렉터리에만 했다.

## 실제 관측과 과거 이력

[최초 GuardMyBack 결과](../../extracted/cpp-editor-checkpoint-20261024-demo-entry/GuardMyBack/result.json)는 같은 제품 SHA `9694a2617115cb58f21c222c1739fa9d9833fb00e662b6a4eaaeba04070db0cc`에서0.483초/종료1, [stderr](../../extracted/cpp-editor-checkpoint-20261024-demo-entry/GuardMyBack/stderr.log)는 `Unable to open d/options.cfg`, states는0BYTE다. 오류 문자열은 `ClientMain.cpp` SaveOptions의 fopen 실패 분기이며 첫 상태 보고 전 메인 루프 저장에 해당한다. actual errno/_doserrno/GetLastError는 남기지 않아 sharing violation·mapped-file·다른 열기 오류를 구별할 수 없다.

앞선31진입/StormRages와 뒤 DissolvedAlliance가 같은 제품으로 통과했다. 이후 [Guard 재검증1](../../extracted/cpp-editor-checkpoint-20261024-demo-guard-retry1/GuardMyBack/result.json)·[2](../../extracted/cpp-editor-checkpoint-20261024-demo-guard-retry2/GuardMyBack/result.json)도 같은 SHA/종료0/통과다. root는 마지막 허용 설정 복구에서 OSError22가 세 번 출력된 뒤 제한 재시도 안에 복구됐다고 보고했다. 이는 최초 실패를 성공으로 바꾸거나 원인을 확정한 관측이 아니다.

같은 형태는 이번 장기 Config 소유자 변경 전부터 있었다. [10/23 Fury 기록](cpp-fury-technology-runtime-20261023.md)의0.897초 fopen 실패와 복구 OSError22, [10/19 Tutorial 기록](cpp-tutorial-later-runtime-20261019.md)의 Tutorial2/GuardMyBack 간헐 실패와 직렬 실행 뒤 복원 실패, [Thunder 기록](cpp-thunder-hit-event-20261019.md)의6진입 fopen 실패를 그대로 참고한다. 현 보호 도구의5회×100ms 복원 재시도는 이미 존재하며 제품 SaveOptions의 원인을 해결했다는 증거가 아니다.

## 경로·경합·보안의 확인 범위

- 실제 명령의 `--run` 인수는 절대 `H:\T5Workspace\NetstormReborn\originals`다. SaveOptions는 그 아래 d/options.cfg의 절대 경로를 만들며 이 경로에는 비ASCII 문자가 없다. 실행 도구의 cwd도 고정 ROOT이고 독립 상태/스크립트 경로는 절대 경로다. 고정 잘못된 경로의 근거는 찾지 못했다.
- 도구는 child 종료를 wait한 뒤 stdout/stderr를 닫고 다음 실행으로 간다. `preserve_game_settings`는 전체 mission 반복 바깥 한 번이므로 정상 흐름에서 매 진입 사이 옵션 복원이 병행되지 않는다. 원시 결과를 만든 solo/bridge 도구와 별도 goal-stage 도구 모두 읽기 검토했다. 독립 도구 프로세스 사이 전역 mutex는 없어 외부 병렬 실행은 가능하지만 이번 경합의 증거는 없다.
- [환경 기록](../../extracted/cpp-options-open-diagnostic-20261024/environment.json)의 현재 대상은954BYTE/Archive이며 H:는 **USB Samsung Portable SSD T5의 exFAT**, Healthy·약390GB 여유다. C: 비교 대상은 NTFS다. exFAT의 Everyone ACL 표시를 NTFS ACL 검증으로 해석하지 않는다. 현재 readonly가 없다는 관측도 과거 순간 상태의 증명은 아니다.
- 실패 창05:27:30~05:31:00의 Defender Operational/System/Application 이벤트에서 원인에 해당하는 기록을 찾지 못했다. Defender Operational은 활성, 마지막 기록은05:15:39였고 현재 실시간 보호는 켜져 있으며 Controlled Folder Access는0이다. **Defender 원인 근거는 없다.** 정상 스캔이나 section 보유가 모두 Operational 이벤트에 남는다고 가정하지 않으므로 Defender 무관도 확정하지 않는다.
- 현재 cpppj 소스와 해당 smoke 도구에서 options를 memory-map하는 호출은 찾지 못했다. 그 자체가 당시 외부 프로세스/드라이버의 section 보유 여부를 입증하지 않는다.

## 원본과 현재 CRT의 공유 계약

[정적 PE/SDK 근거](../../extracted/cpp-options-open-diagnostic-20261024/crt-static.json)에 원문 바이트/분해·CreateFileA IAT·현재 SDK 소스 SHA를 보존했다. 독립 검토자도 같은 경계를 읽었다.

원본 Config 저장 `00440f30`은 `0041b4c0(path,2,1)`→`0041a5b0` mode2의 `w+b`다. mode1+flag비영일 때만 열기 실패 뒤 w+b fallback이 있으며, **mode2는 단회**다. 원본 `_fopen 004e545a`는 share40h `_SH_DENYNO`로 `__fsopen 004e53fe`에 들어가고, CRT 경로의 `004f5819`는 `FILE_SHARE_READ|WRITE=3`, `004f5863`은 CREATE_ALWAYS2, `004f5955`는 CreateFileA IAT `005000ec`를 호출한다.

현재 SDK10.0.26100 UCRT의 `stdio/fopen.cpp`도 `_SH_DENYNO`, `lowio/open.cpp`의 공유 값도 READ|WRITE이고 CreateFileW를 사용한다. 이번 ASCII 경로에서 A/W 차이를 원인으로 볼 근거는 없고 합성 직접 비교도 같은 결과다. 원본과 현재의 기본 공유 방식이 달라졌다는 근거는 없다. 여기서 원본 게임 전체의 top-level 예외 UI/종료 정책까지 검증한 것은 아니다.

## 별도 합성 파일 프로브

`OpenProbe.cpp`는 H: 새 파일과 C: 새 임시 파일에서20조합을 기록했다. [probe1.log](../../extracted/cpp-options-open-diagnostic-20261024/probe1.log)는 최초 예상 하나가 틀려 **종료1을 그대로 보존**했다. 제품 회귀 실패/성공으로 세지 않는다. `fopen` 그대로 비교하기 위한 격리 빌드는 C4996 경고3개다.

| 합성 조건 | 실제 결과 |
|---|---|
| holder 없음, CRT rb/ifstream, 읽기/쓰기 공유 holder | 두 볼륨 w+b 성공 |
| READ holder가 shareREAD만 허용 | 두 볼륨 errno13 / DOS·Win3232 |
| readonly 속성 | 두 볼륨 errno13 / DOS·Win325 |
| 없는 부모 경로 | 두 볼륨 errno2 / DOS·Win323 |
| DELETE 접근 holder+shareRWD | exFAT는32, NTFS의 CREATE_ALWAYS는 성공 |

마지막 차이를 새 `DirectProbe.cpp`와 [direct1.log](../../extracted/cpp-options-open-diagnostic-20261024/direct1.log)로 좁혔다. holder의 실제 GrantedAccess는 두 볼륨 모두110080h다. A/W 모두 exFAT는 OPEN_EXISTING/CREATE_ALWAYS를32로 거부하고 NTFS는 OPEN_EXISTING만32, CREATE_ALWAYS는 성공했다. 이것은 볼륨의 해당 조합 관측이며 실제 Guard 실패의 원인으로 단정하지 않는다. 첫 직접 probe 컴파일의 C3538과 명시 DWORD 교정 후 성공도 각 build 로그에 보존했다.

### mapped section/view의 별도 재현

[MappedFileProbe.py](../../extracted/cpp-options-open-diagnostic-20261024/MappedFileProbe.py)와 [mapped1.json](../../extracted/cpp-options-open-diagnostic-20261024/mapped1.json)은 실제 UCRT DLL과 CreateFileA/W를 호출한다. backing 파일은 일반 READ|WRITE 접근에 **shareREAD|WRITE|DELETE=7**로 열었다. 두 볼륨 각각 readonly view/readwrite view/mapping handle만/view만의 네 수명, 총8행을 관측했다.

**8행 모두** UCRT w+b는 `errno=22, _doserrno=1224, GetLastError=1224(ERROR_USER_MAPPED_FILE)`로 실패했다. 직접 CreateFileA/W도1224이며 Python `Path.write_bytes`도 `[Errno22] Invalid argument`였다. section/view/handle을 반납한 뒤8행 모두 즉시 fopen에 성공했다. 이 재현은 일반 공유 거부32와 구별되며 NTFS에서도 발생하므로 exFAT 하나만의 현상으로 설명할 수 없다. 성공 fopen에서 남은 LastError183은 CREATE_ALWAYS의 기존 파일 표시이며 실패로 해석하지 않는다.

[원본 오류 매핑](../../extracted/cpp-options-open-diagnostic-20261024/crt-dosmap-static.json)도 정적으로 확인했다. `__dosmaperr 004ed5ee`는004ed5f8에서 원OS코드를 TLS+0xc에 보관하고45쌍 표00544080를 찾는다.1224는 표와19~36/188~202 범위에 없으므로004ed658에서 default errno22가 된다. 현재 UCRT `misc/errno.cpp`도 같은 default이며 SDK winerror.h는 ERROR_USER_MAPPED_FILE을1224로 정의한다. 즉 이 errno22가 잘못된 mode 문자열이라는 뜻은 아니며 구CRT/현UCRT 모두 해당 OS 오류에서 일관된다. 이 역시 원본 실제 실패를 실행 관측한 것은 아니다.

이것으로 **현재 두 오류 형태를 설명할 수 있는 기전**을 재현했다. 그러나 실제 오류 당시 options.cfg가 mapped됐는지, 어느 주체였는지, 그때의 Win32 코드가1224였는지는 미기록이다. errno22에는 다른 OS 오류도 매핑될 수 있으므로 이 합성 재현만으로 실게임 원인을 확정하지 않는다.

## 다음에 필요한 좁은 진단

현재 작업에서는 제품을 고치거나 재시도를 추가하지 않았다. 다음 실제 실패를 좁히려면 fopen 직후 다른 출력/문자열 구성 전에 GetLastError·errno·_get_doserrno와 실제 절대 경로·시각·PID를 보관해야 한다. 실제1224이면 그 시점의 section 보유 근거를,32이면 공유 거부 주체를 별도로 조사할 수 있다. 이는 root가 선택할 후속 관측 변경이며 이번 성공 재실행이나 합성 파일 결과로 대신하지 않는다.

원시 실패·재검증·모든 합성 결과를 지우지 않고 [검증/SHA 목록](../../extracted/cpp-options-open-diagnostic-20261024/verification.json)에 고정한다. 최초 실패를 재현 성공으로 합산하지 않으며 원본1394 동일 확인은 root의 독립 기록을 따른다.
