namespace Netstorm.Core.Original;

/// <summary>
/// Territory 기록 쓰기가 받는 20개 레코드 120바이트다(cpppj <c>o/RawFortTerritoryRecover.h</c>의 <c>FortTerritoryRecords</c>).
/// 원문은 고정 120바이트 span이며 비어 있으면 NULL 입력을 뜻한다.
/// </summary>
/// <remarks>2026-11-03 F2c-1에서 추가했다.</remarks>
public static class FortTerritoryRecordShape
{
    /// <summary>20개 기록의 고정 길이다(원문 004bd600의 NULL 입력 구분 길이).</summary>
    public const uint Bytes = 120u;

    /// <summary>기록 개수다(원문 004b5ec0이 두 번째 BYTE의 bit4를 앞에서부터 확인한다).</summary>
    public const uint Count = 20u;
}

/// <summary>
/// 같은 고정 this의 현재 manager 조회를 매 원문 위치에서 다시 읽는 Territory 쓰기 경계다(cpppj <c>o/RawFortTerritoryRecover.h</c>의 <c>FortTerritoryWriteHooks</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-1에서 추가했다. 검사가 일부 장치를 빼고 호출하므로 속성은 읽고 쓸 수 있게 열었다.</remarks>
public sealed class FortTerritoryWriteHooks
{
    /// <summary>고정 this 칸의 현재 manager다. 첫 진단·Create·current 조회에서 각각 다시 읽는다.</summary>
    public Func<uint, uint>? Manager { get; set; }

    /// <summary>원문 Template.cpp 521·522 진단이다. 복귀하면 현재값으로 계속한다.</summary>
    public Action<uint>? Assertion { get; set; }

    /// <summary>현재 manager로 Create("Territory")한다. 반환 stream을 쓰지 않고 같은 this의 현재 manager만 다시 읽는다.</summary>
    public Action<uint, string>? Create { get; set; }

    /// <summary>Create 뒤 현재 manager+8의 실제 현재 stream이다. 주소 0은 장치의 물리 거부로 둔다.</summary>
    public Func<uint, uint>? Current { get; set; }

    /// <summary>같은 현재 stream의 일반 0044cce0 쓰기다. 기록이 없으면 NULL 입력 진단 뒤 길이 120으로 전송한다.</summary>
    public TerritoryStreamWriter? Write { get; set; }
}

/// <summary>
/// 같은 현재 stream에 120바이트를 쓰는 일반 쓰기다. 기록이 없으면 NULL 입력으로 진단한다.
/// </summary>
/// <param name="stream">쓸 MemStream 토큰이다.</param>
/// <param name="records">쓸 120바이트다. null이면 NULL 입력이다.</param>
/// <param name="length">쓸 길이다(원문은 항상 120이다).</param>
/// <remarks>2026-11-03 F2c-1에서 추가했다. 길이가 120이 아니어도 그대로 전달하며 장치가 판정한다.</remarks>
public delegate void TerritoryStreamWriter(uint stream, byte[]? records, uint length);

/// <summary>
/// 같은 현재 owner의 120바이트 Territory를 읽는 getter다.
/// </summary>
/// <param name="slot">읽을 Template 칸이다.</param>
/// <param name="result">읽은 120바이트를 받을 버퍼다. 호출자가 120바이트 배열을 준비한다.</param>
/// <remarks>2026-11-03 F2c-1에서 추가했다. 배열 참조를 그대로 전달하며 사본을 만들지 않는다.</remarks>
public delegate void TerritoryRecordsReader(uint slot, byte[] result);

/// <summary>
/// 같은 현재 owner의 120바이트 Territory를 쓰는 writer다.
/// </summary>
/// <param name="owner">쓸 Template 칸이다.</param>
/// <param name="records">쓸 120바이트다. 같은 부모 지역 배열을 그대로 전달한다.</param>
/// <remarks>2026-11-03 F2c-1에서 추가했다.</remarks>
public delegate void TerritoryRecordsWriter(uint owner, byte[] records);

/// <summary>
/// 같은 현재 owner의 State 주소를 빌리는 조회다. null을 반환하면 물리 범위 밖이다.
/// </summary>
/// <param name="slot">조회할 Template 칸이다.</param>
/// <remarks>2026-11-03 F2c-1에서 추가했다. 배열 참조를 그대로 빌리며 사본을 만들지 않는다.</remarks>
public delegate byte[]? TerritoryStateLookup(uint slot);

/// <summary>
/// 같은 현재 owner의 getter·기록 writer·State·29번 로그만 분리한다(cpppj <c>o/RawFortTerritoryRecover.h</c>의 <c>FortTerritoryRecoverHooks</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-1에서 추가했다. 검사가 일부 장치를 빼고 호출하므로 속성은 읽고 쓸 수 있게 열었다.</remarks>
public sealed class FortTerritoryRecoverHooks
{
    /// <summary>00540cac의 현재 DWORD를 그때그때 읽는다. State 조회와 같은 순서로 다시 읽는다.</summary>
    public Func<uint>? Owner { get; set; }

    /// <summary>004bd660의 실제 20바이트 읽기다. 부모는 먼저 비운 같은 지역 120바이트를 빌린다.</summary>
    public TerritoryRecordsReader? Read { get; set; }

    /// <summary>bit4가 있으면 29번 로그를 호출한다. 복구 뒤 같은 flags BYTE를 다시 읽는다.</summary>
    public Action<uint, string>? Log { get; set; }

    /// <summary>004bd600 제본문으로 연결한다. 현재 owner는 이 경계 직전에 다시 읽는다.</summary>
    public TerritoryRecordsWriter? Write { get; set; }

    /// <summary>004bd550이 반환한 같은 State 주소다. 부모는 첫 BYTE만 읽고 bit4를 지운다.</summary>
    public TerritoryStateLookup? State { get; set; }
}

/// <summary>
/// 원문 004bd600의 같은 manager 지정 기록 쓰기와 004b5ec0의 Territory 회복을 복원한다(cpppj <c>o/RawFortTerritoryRecover</c>).
/// 진단 위치를 임시 생성하지 않고 실제 원문 조건에서만 호출한다.
/// </summary>
/// <remarks>2026-11-03 F2c-1에서 추가했다.</remarks>
public static class RawFortTerritoryRecover
{
    /// <summary>
    /// 도달한 미연결 장치만 거부하여 앞선 manager 조회·진단의 부분 효과를 보존한다.
    /// </summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식).</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <exception cref="InvalidOperationException">Territory 실제 경계가 연결되지 않았다.</exception>
    private static T NeedTerritory<T>(T? function) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("Territory 실제 경계 미연결");
        }
        return function;
    }

    /// <summary>
    /// 진단 위치를 실제 소비자에게 전달한다. 임시 진단 위치를 만들지 않는다.
    /// </summary>
    /// <param name="hooks">쓰기 경계다.</param>
    /// <param name="line">원문 진단 줄 번호다(521·522).</param>
    private static void TerritoryAssert(FortTerritoryWriteHooks hooks, uint line)
    {
        NeedTerritory(hooks.Assertion)(line);
    }

    /// <summary>
    /// 004bd600: manager 521·NULL 입력 522·현재 manager Create·현재 stream·일반 Write 120 순서로 보존한다.
    /// </summary>
    /// <param name="slot">고정 this의 Template 칸 번호다.</param>
    /// <param name="records">쓸 120바이트다. null이면 NULL 입력이다.</param>
    /// <param name="hooks">실제 수명 소유자와 연결한 writer 경계다.</param>
    public static void WriteFortTerritoryRecords(uint slot, byte[]? records, FortTerritoryWriteHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (NeedTerritory(hooks.Manager)(slot) == 0)
        {
            TerritoryAssert(hooks, 521u);
        }
        if (records is null)
        {
            TerritoryAssert(hooks, 522u);
        }
        NeedTerritory(hooks.Create)(NeedTerritory(hooks.Manager)(slot), "Territory");
        uint stream = NeedTerritory(hooks.Current)(NeedTerritory(hooks.Manager)(slot));
        NeedTerritory(hooks.Write)(stream, records, FortTerritoryRecordShape.Bytes);
    }

    /// <summary>
    /// 004b5ec0: 0 초기화 120 읽기→20 flags·조건부 로그·현재 owner 읽기·현재 owner State 첫 BYTE 회복 순서로 실행한다.
    /// </summary>
    /// <param name="hooks">같은 현재 owner의 getter·기록 writer·State·29번 로그 경계다.</param>
    public static void RecoverFortTerritory(FortTerritoryRecoverHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        byte[] records = new byte[FortTerritoryRecordShape.Bytes];
        NeedTerritory(hooks.Read)(NeedTerritory(hooks.Owner)(), records);
        // 매 6바이트 기록의 두 번째 BYTE만 읽는다. 로그가 버퍼를 바꾸면 복구 뒤 같은 BYTE의 현재값에서 bit4를 지운다
        for (uint index = 0; index < FortTerritoryRecordShape.Count; index++)
        {
            int offset = (int)(index * 6u + 1u);
            if ((records[offset] & 4u) != 0)
            {
                NeedTerritory(hooks.Log)(29u, "Territory recovered from Battle!\n");
                records[offset] = (byte)(records[offset] & 0xfbu);
            }
        }
        NeedTerritory(hooks.Write)(NeedTerritory(hooks.Owner)(), records);
        byte[]? state = NeedTerritory(hooks.State)(NeedTerritory(hooks.Owner)());
        if (state is null || state.Length == 0)
        {
            throw new ArgumentOutOfRangeException(nameof(hooks), "Territory 회복 State 첫 BYTE 물리 범위");
        }
        state[0] = (byte)(state[0] & 0xfb);
    }
}
