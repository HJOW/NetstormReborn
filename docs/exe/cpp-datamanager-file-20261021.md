# 파일 읽기 부모·Template 서명·현재 파일 할당 반납

2026-10-21 단위270. 시작 커밋은 `ec3c418c`다. 이전 턴은 전투 표시 수정·34개 집중 검사·GUI 종료 확인·테스트 중단 기록을 마친 진행 턴이다. 현재 PC와 마지막 정밀 디컴파일 PC는 **VM-W11-CODEX / 2026-10-21**로 같다. 자동 goal 계속 지시는 구현 작업에 적용했으며, 사용자가 중단한 구동/집중 검사는 재개하지 않았다. 새 디컴파일·원본/CD/AGENTS/dotnetpj 변경은 없다.

## 원본에서 확인한 제어

`originals/Netstorm.exe`의 PE와 현재 PC의 기존 정밀 원문을 읽었다. 원본 명령을 에뮬레이션하거나 실행하지 않았다. 아래 내용은 정적 명령/가상 표 대조이며 정상 반환 횟수나 새 native 검사의 통과 수가 아니다.

| 항목 | 10.78 주소 | 정적 근거 |
|---|---|---|
| 파일 읽기 부모 | `0044d980` | 첫16BYTE0/mark-1, `readFile(name,1,&alloc,0)`, data/end 기록, 가상+c, 성공 뒤44d380, 현재 data 반납 |
| Template manager 가상 표 | `00514690` | +0=4bcca0, +4=4bccb0, +8=4bcd80, +c=4bce10 |
| 서명 판정 | `004bce10` | data/end 비영, signed `numTypes<256`, BYTE 두 개/pos 증가, +10c 서명, +110의 두 번째 BYTE==FE, F 판정 |
| manager 생성의 저장 폭 | `004bcc60` | 276BYTE, +c 파일명256, +10c=0, +110=ffffffff. 별도 전역5c89c0 초기화도 있으나 이번 전체 생성자 연결 범위에는 포함하지 않음 |
| 읽기 경계 | `0041baa0` | 파일 입력/최소4BYTE 할당/읽기/길이 기록. 이번에는 경계이며 전체 CRT/VFS/OS 구현이 아님 |
| 서명 거부의 제거 요청 | `0041aef0` → `004e58ba` | 최종 IAT `005000b0`은 실제 `DeleteFileA`. 파일을 지우는지 추측으로 판정하지 않음 |

서명은 논리 end가 **비영인지**만 확인한다. end1이어도 물리적으로 두 BYTE가 있으면 읽는다. signed 음수 타입 수에는 진단이 없고,256 이상이면 진단 뒤 현재 source를 다시 읽는다. FE 이외의 두 번째 BYTE는 모두 +110=0이다. 잘못된 첫 BYTE에서도 두 꼬리DWORD와 pos 변경을 먼저 수행한 뒤 현재 이름을 다시 조회하여 제거를 요청한다.

## 연결한 코드

`RawDataManagerImage`에 `ReadDataManagerFile`과 `ReadTemplateFileSignature`를 추가했다. 파일 읽기 부모는 같은 지역 MarkStream20을 초기화하고, 길이DWORD를 읽기 경계에 직접 빌려 주며, 성공한 서명 뒤 같은 source를 기존 전체 섹션 본문으로 넘긴다. 서명/본문이 source.data를 바꾸면 마지막의 현재 포인터를 반납한다. 호스트 예외에서도 한 번 반납을 시도하며, 반납 경계 자체가 예외를 던져도 중복 호출하지 않는다. 이 C++ 예외 방어를 원본 Windows SEH 전체 복원으로 세지 않는다.

`CampaignTemplateStorage::Image`의 manager는12BYTE 편의 헤더에서 실제276BYTE 저장소로 확장했다. 파일명과 서명 초기값을 같은 manager에 두고, 실제 `InstallFile`은 현재 VFS 캡처의 파일 원문을 바깥 부모→서명→기존 섹션 읽기로 연결한다. `ReadDataManagerImage` 전에 pos2를 수동으로 쓰던 코드는 제거했다. 서명이 두 BYTE를 소비해 pos2가 된다. 파일 원문 할당은 부모가 끝날 때 반납하고 생성된 섹션 할당과 State/Territory cursor는 같은 저장소에 유지한다. `FileHeader`는 빌린 헤더에서도 원래 소유자의 같은 서명/FE 판정DWORD를 읽는다. 실제 Session은 현재 TYPE 수를 전달한다.

현재 로더는 이미 읽은 VFS 원문의 캡처를 인계한다. 원본41baa0의 최소4BYTE/미정 tail/모든 OS 오류, 원본 전체 파일명 조회 정체성, manager 생성자의 별도 전역 초기화, 바깥 Template Load004bf550의 전체 분기는 아직 남는다. `Image::name`은 기존 요새 식별자를 유지하며 이를 원본 전체 파일 경로와 동일하다고 주장하지 않는다. 빈/짧은 손상 입력은 호스트의 실제 물리 범위를 보호한다.

서명 실패의 제거 요청은 명시 경계다. 실제 보관소는 **보호 자료 삭제 대신 예외로 거부**한다. 원본 보호 규칙을 이 함수 때문에 해제하지 않으며 원본 파일을 수정하거나 삭제하지 않았다. 전체 저장·사용자 파일 오류 정책을 완성한 것은 아니다.

## 빌드와 실행하지 않은 검사

최종 전체 Release `extracted/cpp-datamanager-file-20261021-build1.log`는 **종료0·경고0·오류0**이다. 원시 로그와 실제 종료 코드 JSON을 보존한다. 새 검사5개를 작성·컴파일했다.

- `DataManagerFile_SignatureBytesAndSignedTypeLimit`: signed -1/255/256, 두 번째 BYTE0/FE/FF, end1, 보호 영역.
- `DataManagerFile_SignatureFailureAndCurrentRereads`: 진단 뒤 현재 source, 잘못된 서명의 이름/제거 요청, NULL/짧은 물리 입력.
- `DataManagerFile_ReadParentBranchesAndCleanupCurrent`: 파일 없음, 서명 거부/예외, 현재 data 교체 반납, 정상 빈 이름 표.
- `DataManagerFile_BodyAndReleaseFailureOnlyOnce`: 본문 예외, 반납 경계 예외의 중복 반납 방지, 필수 연결.
- `CampaignTemplates_FileHeaderAndRejectedImageOwnership`: 초기값, FE/빌림의 같은 결과, 실제 보관소 거부 뒤 소유권, 보호 원문의 복사본 입력.

기존 목표11파일 검사에는 같은 서명/빌림 결과 확인을 더했다. **새/수정 C++검사·CTest·게임은 실행하지 않았다.** 이전34개나 원본 읽기144행의 과거 통과를 새 소스/바이너리의 통과로 옮겨 적지 않는다.

`python tools/cpp_datamanager_file_audit.py`는 원본 두 함수의 정밀 범위/실제 바이트·명령, 네 가상 슬롯, 파일 제거 import, 빌드/바이너리 SHA와 새5개 검사 등록 문자열을 읽기만 한다. 최초 기록은 `cpppj/datamanager-file-static-20261021.json`이며 작성 뒤 덮어쓰지 않는다. **검사 문자열 존재는 실행 증거가 아니다.** 기록은 실제 작업 트리 바이트와 Git 줄바꿈 정규화 이후 바이트를 구별한다. 기존 실행/바이너리/원문 보고서는 변경하지 않는다.

다음은 바깥 Template Load/현재 파일명·전체 생성자/기본 섹션 이름 표·저장 및 전체 State7/전투 정리 연결이다. 중단한 Devastation·포획·희생 구동은 사용자 재지시 전 재개하지 않는다. 전체11미션의 자연 플레이·전략AI·외관/음향/동작 동일성은 미완료이며 전체 goal은 active다. 인수인계 뒤 **20261021-39 CP**로 로컬 커밋하며 push하지 않는다.
