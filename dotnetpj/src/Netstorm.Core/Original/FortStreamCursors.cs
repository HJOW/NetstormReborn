namespace Netstorm.Core.Original;

/// <summary>
/// Territory getter가 현재 위치에서 직접 읽는 원본 MemStream의 물리 바이트와 pos/end다(cpppj <c>o/RawFortWorldLoad.h</c>의 <c>FortTerritoryStream</c>).
/// 누락된 자료에 가짜 0을 공급하지 않는다. C++의 값 구조체를 참조로 돌려주던 것과 같은 의미를 위해 클래스로 옮겼다:
/// 저장소가 한 manager에 대해 인스턴스 하나를 평생 소유하고, 교체하지 않고 <see cref="Reset"/>·<see cref="Set"/>으로 필드만 바꾼다.
/// </summary>
/// <remarks>2026-11-01 0.130(TS0)에서 추가했다. 이 단위는 자료형만 옮겼고 Territory 로더(<c>RawFortWorldLoad</c>)는 F2에서 옮긴다.</remarks>
public sealed class FortTerritoryStream
{
    /// <summary>현재 원본 섹션의 실제 물리 바이트다.</summary>
    public ReadOnlyMemory<byte> Bytes { get; set; }

    /// <summary>getter가 120을 더하는 현재 위치다.</summary>
    public uint Position { get; set; }

    /// <summary>논리 끝이며 getter는 보존한다.</summary>
    public uint End { get; set; }

    /// <summary>C++의 <c>territory = {}</c>와 같이 모든 필드를 기본값으로 돌린다.</summary>
    public void Reset()
    {
        Bytes = default;
        Position = 0;
        End = 0;
    }

    /// <summary>C++의 <c>territory = {bytes, position, end}</c>와 같이 모든 필드를 한 번에 바꾼다.</summary>
    /// <param name="bytes">물리 바이트.</param>
    /// <param name="position">현재 위치.</param>
    /// <param name="end">논리 끝.</param>
    public void Set(ReadOnlyMemory<byte> bytes, uint position, uint end)
    {
        Bytes = bytes;
        Position = position;
        End = end;
    }
}

/// <summary>
/// Money MemStream이 실제로 읽는 pos/end DWORD다(cpppj <c>o/RawFortMoneyLoad.h</c>의 <c>FortMoneyCursor</c>).
/// 저장된 금액의 float 값은 이 형식이 보관하지 않는다(편의 사본을 만들지 않는다). 참조 의미가 필요하므로 클래스다.
/// </summary>
/// <remarks>2026-11-01 0.130(TS0)에서 추가했다. Money 로더(<c>RawFortMoneyLoad</c>)는 F2에서 옮긴다.</remarks>
public sealed class FortMoneyCursor
{
    /// <summary>현재 스트림 위치다. 실제 본문이 4를 감김 가산한다.</summary>
    public uint Position { get; set; }

    /// <summary>현재 스트림 끝이다. 증가한 위치와 signed로 비교한다.</summary>
    public uint End { get; set; }

    /// <summary>C++의 <c>money = {}</c>와 같이 위치와 끝을 0으로 돌린다.</summary>
    public void Reset()
    {
        Position = 0;
        End = 0;
    }

    /// <summary>C++의 <c>money = {position, end}</c>와 같이 두 필드를 한 번에 바꾼다.</summary>
    /// <param name="position">현재 위치.</param>
    /// <param name="end">논리 끝.</param>
    public void Set(uint position, uint end)
    {
        Position = position;
        End = end;
    }
}

/// <summary>
/// Badges·Technology 섹션의 현재 물리 바이트와 pos/end 및 data 포인터 존재 여부다(cpppj <c>o/RawFortMetaLoad.h</c>의 <c>FortMetaStream</c>).
/// 원본 data 포인터가 널인 섹션과 길이 0인 섹션을 <see cref="Present"/>로 구별한다. 참조 의미가 필요하므로 클래스다.
/// </summary>
/// <remarks>2026-11-01 0.130(TS0)에서 추가했다. Badges·Technology 로더(<c>RawFortMetaLoad</c>)는 F2에서 옮긴다.</remarks>
public sealed class FortMetaStream
{
    /// <summary>현재 섹션의 원본 바이트다. badge는 직접 읽는다.</summary>
    public ReadOnlyMemory<byte> Bytes { get; set; }

    /// <summary>lookup은 위치를 0으로 두고 wrapper와 하위 contents가 현재 위치와 끝을 쓴다.</summary>
    public uint Position { get; set; }

    /// <summary>논리 끝이다.</summary>
    public uint End { get; set; }

    /// <summary>원본 data 포인터가 널이 아닌가. <c>Bytes</c>가 비어 있는 것과 구별한다. 기본값은 참이다.</summary>
    public bool Present { get; set; } = true;

    /// <summary>C++의 <c>stream = {}</c>와 같이 모든 필드를 기본값(<see cref="Present"/>는 참)으로 돌린다.</summary>
    public void Reset()
    {
        Bytes = default;
        Position = 0;
        End = 0;
        Present = true;
    }

    /// <summary>C++의 <c>stream = {bytes, position, end, present}</c>와 같이 모든 필드를 한 번에 바꾼다.</summary>
    /// <param name="bytes">물리 바이트.</param>
    /// <param name="position">현재 위치.</param>
    /// <param name="end">논리 끝.</param>
    /// <param name="present">원본 data 포인터가 널이 아닌가.</param>
    public void Set(ReadOnlyMemory<byte> bytes, uint position, uint end, bool present)
    {
        Bytes = bytes;
        Position = position;
        End = end;
        Present = present;
    }
}
