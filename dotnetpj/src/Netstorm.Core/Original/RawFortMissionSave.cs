namespace Netstorm.Core.Original;

/// <summary>
/// 004bcbf0의 null 판정과 일반 쓰기 용량 진단을 구분한다(cpppj <c>o/RawFortMissionSave.h</c>의 <c>FortMissionSaveAssertion</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-4에서 추가했다.</remarks>
public enum FortMissionSaveAssertion
{
    /// <summary>이름이 null이다(Template.cpp 155 진단).</summary>
    NullName,
}

/// <summary>
/// 같은 명시 stream에 일반 WriteMemStream으로 연결한다. 호출 뒤 원문 범위를 무효화하지 않는다(cpppj <c>o/RawFortMissionSave.h</c>의 write).
/// </summary>
/// <param name="stream">쓸 stream 토큰이다.</param>
/// <param name="source">쓸 바이트 구간이다.</param>
/// <remarks>2026-11-03 F2c-4에서 추가했다. <c>Span</c>을 담는 대리자는 전용 대리자로 둔다.</remarks>
public delegate void FortMissionSaveWriter(uint stream, ReadOnlySpan<byte> source);

/// <summary>
/// 이름·필드 getter와 실제 주소는 조회하며 write·진단 callback은 실제 효과만 둔다(cpppj <c>o/RawFortMissionSave.h</c>의 <c>FortMissionSaveHooks</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-4에서 추가했다. 검사가 일부 장치를 빼고 호출하므로 속성은 읽고 쓸 수 있게 열었다.</remarks>
public sealed class FortMissionSaveHooks
{
    /// <summary>00463ca0: Guide+800의 현재 Mission 토큰이다. 진입·문자열 비교·조회에서 다시 읽는다.</summary>
    public Func<uint>? Current { get; set; }

    /// <summary>같은 Guide의 NUL 포함 현재 문자열 범위다. null이면 004bcbf0 null 판정이다.</summary>
    public Func<byte[]?>? Name { get; set; }

    /// <summary>선택된 실제 Mission의 0x58 sequence·0x54 flags를 읽는다. 0 토큰 물리 접근은 장치가 거부한다.</summary>
    public Func<uint, uint, uint>? Field { get; set; }

    /// <summary>같은 명시 stream의 쓰기다.</summary>
    public FortMissionSaveWriter? Write { get; set; }

    /// <summary>Template.cpp 155 진단이며 복귀 뒤 같은 문자로 계속한다.</summary>
    public Action<FortMissionSaveAssertion>? Assertion { get; set; }
}

/// <summary>
/// 최초 길이와 선택 Mission만 읽고 원본 표를 조회하지 않으며 실제 값만 해당 위치에서 쓴다(cpppj <c>o/RawFortMissionSave</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-4에서 추가했다.</remarks>
public static class RawFortMissionSave
{
    /// <summary>
    /// 도달한 미연결 장치만 거부하여 앞선 조회의 부분 효과를 보존한다.
    /// </summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식).</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <exception cref="InvalidOperationException">Fort Mission 저장의 실제 경계가 연결되지 않았다.</exception>
    private static T NeedMission<T>(T? function) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("Fort Mission 저장 실제 경계 미연결");
        }
        return function;
    }

    /// <summary>
    /// callback 안에 캡처된 DWORD를 원본 little endian 순서대로 전달한다.
    /// </summary>
    /// <param name="stream">쓸 stream 토큰이다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓸 너비(BYTE 수)다.</param>
    /// <param name="hooks">실제 쓰기 경계다.</param>
    private static void Word(uint stream, uint value, int width, FortMissionSaveHooks hooks)
    {
        byte[] bytes = new byte[4];
        // 상위 바이트부터 기록하지 않고 처음 바이트 길이만 쓴다
        for (int index = 0; index < bytes.Length; index++)
        {
            bytes[index] = (byte)(value >> (index * 8));
        }
        NeedMission(hooks.Write)(stream, new ReadOnlySpan<byte>(bytes, 0, width));
    }

    /// <summary>
    /// 한 write가 이름을 바꾸기 전후에도 길이는 최초 길이로 쓰고 NUL 뒤는 검사하지 않는다.
    /// </summary>
    /// <param name="stream">쓸 stream 토큰이다.</param>
    /// <param name="name">NUL 포함 이름 범위다. null이면 null 판정을 보고한 뒤 빈 이름으로 둔다.</param>
    /// <param name="hooks">실제 쓰기 경계다.</param>
    public static void SaveFortMissionName(uint stream, byte[]? name, FortMissionSaveHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (name is null)
        {
            NeedMission(hooks.Assertion)(FortMissionSaveAssertion.NullName);
            name = new byte[1];
        }
        ReadOnlySpan<byte> source = name;
        int terminator = source.IndexOf((byte)0);
        if (terminator < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(name), "Fort Mission 이름의 실제 NUL 물리 범위 누락");
        }
        if ((ulong)terminator > uint.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(name), terminator, "Fort Mission 이름의 32비트 문장 길이 범위");
        }
        // 길이 표시는 항상 WORD로 쓰고 내용은 길이만큼만 쓴다
        uint length = (uint)terminator;
        Word(stream, length, 2, hooks);
        NeedMission(hooks.Write)(stream, source[..terminator]);
    }

    /// <summary>
    /// Guide 현재 확인이 바뀐 뒤에도 선택된 Mission의 flags를 읽는다.
    /// </summary>
    /// <param name="stream">쓸 stream 토큰이다.</param>
    /// <param name="mission">선택된 Mission 토큰이다.</param>
    /// <param name="hooks">실제 쓰기 경계다.</param>
    public static void SaveFortMissionState(uint stream, uint mission, FortMissionSaveHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        Word(stream, NeedMission(hooks.Field)(mission, 0x58), 4, hooks);
        Word(stream, NeedMission(hooks.Field)(mission, 0x54), 4, hooks);
    }

    /// <summary>
    /// 이름 길이가 난 뒤에 Guide 현재 확인을 다시 읽으며 일반 Battle과 같은 경로를 사용한다.
    /// </summary>
    /// <param name="stream">쓸 stream 토큰이다.</param>
    /// <param name="hooks">실제 쓰기 경계다.</param>
    public static void SaveFortMission(uint stream, FortMissionSaveHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (NeedMission(hooks.Current)() == 0)
        {
            return;
        }
        SaveFortMissionName(stream, NeedMission(hooks.Name)(), hooks);
        SaveFortMissionState(stream, NeedMission(hooks.Current)(), hooks);
    }
}
