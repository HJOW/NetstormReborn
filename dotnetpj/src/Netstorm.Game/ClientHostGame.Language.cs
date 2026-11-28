// 실제 시작 감지와 이후 설정/State가 같은 언어 소유자를 빌린다.
namespace Netstorm.Client.Host;

/// <summary>현재 언어 번호와 용어표 수명을 콘텐츠/Session 교체에서 유지한다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>HostStartupScene가 설정 전에 감지한 같은 소유자다.</summary>
    private readonly ClientLanguageGlobals _nativeLanguageGlobals;
    /// <summary>같은 Config와 실제 용어표 적재의 구독 수명이다.</summary>
    private readonly ClientLanguageOptions? _nativeLanguageOptions;
    /// <summary>조회 자체로 언어/파일/설정을 변경하지 않는 같은 전역이다.</summary>
    public ClientLanguageGlobals LanguageGlobals => _nativeLanguageGlobals;
    /// <summary>실제 전체 옵션 부모가 원문 위치에서 재사용할 언어 부분 해석이다.</summary>
    public void InterpretNativeLanguageOptions() => (_nativeLanguageOptions ?? throw new InvalidOperationException("현재 언어의 실제 설정 장치 미설치")).Interpret();
}
