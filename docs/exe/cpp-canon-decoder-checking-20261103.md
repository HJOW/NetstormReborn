# CanonDecoder의 현재 프레임 진단과 커서 소비자

단위 810은 `00425860` 다음 칸의 두 진단 본체와 일부 실제 소비자를 연결했다. 잘못된 명시 프레임의 0x305 진단은 checking과 무관하다. 금 간 프레임의 0x325 진단은 현재 checking과 현재 프레임 코드의 0x20 비트를 읽고 좌표/패턴 인덱스를 갱신하기 전에 호출한다. 진단에서 돌아오면 순회를 계속하며 빈 프레임 -1은 계속 건너뛴다. 진단 장치가 없으면 해당 선택 위치에서 예외다.

`CanonDecoderDiagnostics`는 반복자보다 오래 사는 현재 checking 뷰와 식/파일/행 callback이다. Session의 `metadata.geometry`를 같은 `ClientGameplayGlobals.AiDebug.Checking`과 현재 `services.ai.assertion`에 연결했다. 실제 커서/미리보기의 공유 walk·생산 항목 walk·일반/사제 배치 기하·전체 픽셀 모양 측정이 이 장치를 전달한다. 기하 상태를 복사해도 checking owner 주소는 유지되며 다음 Advance에서 현재값을 읽는다.

**부분 완료다.** 건설 Confirm/Place/Local/Request/Clear의 직접 DecodeCanonType, RawIslandBuilder/RawIslandPlacement/RawPlayerGeyserBounds, 일반 지형 PatternCount와 옛 Islandbuilder/분석 호출자 분류는 남는다. 이들을 연결하기 전까지 함수 전체 완료로 세지 않는다. checking 잔여는 31 그대로이고 분류만 부모 21·부분 7·MP 3으로 바꾼다.

원문 비패턴 156입력을 두 x87 정밀도에서 실행한 312관측과 재관측 312회가 일치했다. 진단 선택 68·중단 40입력이다. 두 유효 프레임과 빈 기본값/잘못된 명시값, 현재 checking의 상위 비트, 금 간 비트, 진단 복귀/중단과 checking 변경을 포함한다. 첫 초기화는 Unicorn mem_write에 bytearray를 전달한 Python 타입 오류로 원문 진입 전에 중단했고 bytes 변환 뒤 최초/반복 관측을 완료했다.

새 C++ 검사 2개와 기존 타입별 Canon 검사 4개가 실패 0이다. 기존 10.78/10.72/10.37의 정상 순회 3,927행을 유지한다. 새 검사는 원문 사건/반환·첫 칸/끝 공개 상태와 현재 checking을 비교한다. 별도 C++ 순서성 검사에서 패턴 칸 사이 checking 변경과 현재 callback 교체를 확인했다. 패치 cracked 진단을 CD에 추가하지 않았다. 잘못된 명시 프레임 진단 복귀 뒤 물리 코드 조회와 Side는 배열 밖 메모리 대신 호스트 예외를 유지하며 이를 정상 원문 메모리 출력으로 세지 않는다.

최초 Release 빌드와 Side 물리 경계 보강 뒤 최종 v2 빌드가 모두 경고·오류 0이다. 전체 CTest/71대상·34경로·ZIP은 반복하지 않았다. 실제 게임/GUI·발사/공격·포획/희생·승리·공중 3종 전용·MP·물리/PCM 검사는 없다. 실제 binder는 소스/빌드 근거이며 전체 진단 UI나 32비트 ABI/SEH를 복원한 것은 아니다.

마지막 전체 정밀 디컴파일은 HJOW-Athlon / 2026-10-23이고 현재 호스트가 같다. 디컴파일 산출물 변경은 없다. 기존 보호 2,738개가 불변이며 새 fixture를 포함해 2,739개/fixture 919개다. 다음 기준은 `extracted/cpp-resume-810/protected-after.json`이다. EXE SHA-256은 `7115bf2300854028aee67973e77919ba26fc2554a03ac3ba0e418486444e7853`다. 원시는 `extracted/cpp-resume-810/`, 커밋되는 근거는 [JSON 보고서](../../cpppj/canon-decoder-checking-20261103.json)다.

다음 단위는 나머지 건설·섬/지형 소비자의 현재 진단 owner 연결이다. 전체 목표는 계속 진행한다.
