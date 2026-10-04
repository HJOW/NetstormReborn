# 모든 타깃에 공통으로 거는 컴파일 옵션

# netstorm_apply_compiler_options(<타깃>)
# 소스 인코딩(UTF-8)과 경고 수준을 타깃에 건다.
function(netstorm_apply_compiler_options target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /utf-8          # 소스와 실행 문자 집합을 UTF-8 로 (한국어 주석·문자열)
            /W4             # 경고 수준 4
            /permissive-)   # 표준 준수 모드
        # fopen 등 표준 C 함수에 대한 MSVC 전용 경고를 끈다 (리눅스와 같은 코드를 쓰기 위해)
        target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()
