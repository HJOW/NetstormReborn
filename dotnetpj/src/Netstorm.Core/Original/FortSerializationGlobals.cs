namespace Netstorm.Core.Original;

/// <summary>
/// Template 생성자와 객체 저장·로더가 같은 Client 수명으로 공유하는 원본 전역이다(cpppj <c>o/FortSerializationGlobals.h</c>).
/// 10.78의 005c89c0은 실제 DWORD 한 개이며, Storage/Session/목록이 값 사본 대신 같은 객체를 빌린다.
/// </summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public sealed class FortSerializationGlobals
{
    /// <summary>
    /// 직렬화 버전이다. manager 생성자는 0, 전체 Objects 저장은 2를 쓰며 각 소비자는 현재 값을 읽는다.
    /// </summary>
    public uint Version { get; set; }
}
