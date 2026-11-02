namespace Netstorm.Core.Original;

/// <summary>
/// 원본 SID 슬롯 번호다(cpppj <c>o/SidPool.h</c>의 <c>Sid</c>). 값은 16비트이며 기본값 0은 널 SID다.
/// 기존 C# 이식은 슬롯 번호를 int로 들고 다녔으나, Fort 적재 로더의 갈고리 모양을 원본대로 옮기기 위해 감싸는 형식을 둔다.
/// </summary>
/// <param name="Value">SID 번호.</param>
/// <remarks>2026-11-02 F2a에서 추가했다. 비교·복사는 값으로 하며 수명은 호출자가 진다.</remarks>
public readonly record struct Sid(ushort Value);
