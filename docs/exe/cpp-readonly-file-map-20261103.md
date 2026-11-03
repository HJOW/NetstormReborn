# SHP 직접 읽기전용 매핑 부모

단위790은789(`88d0b4a7`) 뒤의 직접 파일 매핑 부모 복원이다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 현재 호스트와 같다. 원본·추출물·AGENTS.md·기존 fixture는 바꾸지 않았다.

## 원문 순서

004dd580은 SECURITY_ATTRIBUTES의 길이12·descriptor0·inherit0을 작성하고 CreateFileA(name,GENERIC_READ,FILE_SHARE_READ,security,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL)을 호출한다. 파일이INVALID_HANDLE_VALUE일 때만Winutil.cpp17e 진단이며 file0은 그 관문과 다르다. 이후 CreateFileMappingA(file,NULL,PAGE_READONLY,0,0,NULL), MapViewOfFile(mapping,FILE_MAP_READ,0,0,0) 순서다. mapping0은181, view0은184의조건문자열`p` 진단이다. 진단이 복귀하면 다음 호출을 계속하고 마지막 view를 반환한다.

이 원문 부모에는 CloseHandle/UnmapViewOfFile이 없다. C++ 순수 부모도 임의 Close나 대체 whole read를 넣지 않는다. 실제 W API와 핸들/view의 수명·반납은791의 현대 어댑터에서 분리한다. 원문의32비트 security12를64비트 OS 구조체 크기로 직접 넘겨서는 안 된다. 핸들 토큰은 uintptr_t이며 원문의INVALID와 실제호스트 INVALID_HANDLE_VALUE를 명시 인자로 구별한다.

## 최초 관측과 집중 검사

독립 CPU는 실제004dd580을 실행하고 세 실제 IAT 슬롯만 명시 Win32 장치에 연결했다. 이름3종(정상·유효빈·첫NUL 뒤 BYTE), 파일0/INVALID/유효·mapping/view0·진단 비복귀/복귀의72입력을 두 x87 정밀도로144회 관측했다. 별도 프로세스의144회가 같은 바이트로 일치했다. 최초 fixture SHA256은 `67a00ccd30c92a885d64f67638f5f490e9a31a3990c95638bda7e15b63339825`다. 실제 OS/파일·매핑 실패와 전체SEH를 실행한 것은 아니다.

C++ 새2개는72행의 모든 OS 인자/보안 필드·진단 문자열/줄·효과열·반환/실패, 별도의64비트 핸들 무절단 계약을 검사한다. 실제 SHP나 새 Win32 매핑을 생성하지 않는다. 빌드/최종 검사값·EXE 해시는 [보고서](../../cpppj/readonly-file-map-20261103.json), 원시 자료는 `extracted/cpp-resume-790/`에 기록한다.

## 실제 연결 잔여

현재 GameAssets는 여전히 기존 VFS 값형 SHP를 사용한다. **실제 SHP 연결은791에 남으며 순수 부모 완료와 구별한다.** 0049ebb0의 이름 작성/0041ad10 선택 뒤 직접 map이라는 경계를 사용해야 한다. 아카이브 이름 일치를 payload 추출이나 VFS whole read 성공으로 바꾸지 않는다. 기존 ShapeDatabase의 물리 범위/프레임 파서·CLI/CD 값형 입구는 보존하고 같은 map/view 소유권을 실제 자산 수명까지 유지한다. 원문이 누락한 핸들 정리는 Windows 호스트의 현대 RAII로 문서화한다.

새 게임/GUI/물리/PCM·공중3종 전용 행동·포획/희생·특정 적 공격·승리/자연완주·멀티플레이 검사는0이다. 전체71대상/전체C++/전체34/ZIP은 반복하지 않았다. checking잔여50·Script·나머지 비멀티/호환성과 전체 목표는 미완료다.

## 최종 확인값

게임/기존 파일 집중 대상 Release 빌드는 경고/오류0이고 새2개가 실패0이다. EXE SHA256은 `8f9438c2ed5e6bba09eb5f351aae99ea277cffda8eaae3d795cf6b0dc6acd04a`이다. 기존 보호2723개 불변·새fixture 포함2724개/fixture904이며 다음 기준은 `extracted/cpp-resume-790/protected-after.json`이다. 새 게임/MP/전체C++/71대상/34경로/ZIP은 추가하지 않았다. 실제 SHP와 자산수명 연결은791에서 계속한다.
