namespace Netstorm.Core.Original;

/// <summary>
/// 같은 일반 stream writer에 4바이트를 쓰는 Money 저장 경계다(cpppj <c>o/RawFortMoneySave.h</c>의 <c>FortMoneySaveWrite</c>).
/// 진단 뒤 현재 필드를 다시 조회하지 않는다.
/// </summary>
/// <param name="stream">쓸 stream 토큰이다.</param>
/// <param name="bytes">쓸 4바이트다.</param>
/// <remarks>2026-11-03 F2c-4에서 추가했다. <c>Span</c>을 담는 대리자는 전용 대리자로 둔다.</remarks>
public delegate void FortMoneySaveWrite(uint stream, ReadOnlySpan<byte> bytes);

/// <summary>
/// 단독 Template 부모의 현재 manager·Create·실제 일반 쓰기 경계다(cpppj <c>o/RawFortMoneySave.h</c>의 <c>FortMoneySaveHooks</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-4에서 추가했다. 검사가 일부 장치를 빼고 호출하므로 속성은 읽고 쓸 수 있게 열었다.</remarks>
public sealed class FortMoneySaveHooks
{
    /// <summary>고정 this 슬롯의 현재 첫 DWORD를 읽는다. 0이면 부분 저장에 진단한다.</summary>
    public Func<int, uint>? Manager { get; set; }

    /// <summary>현재 manager로 Create("Money")하고 실제 stream 토큰을 반환한다.</summary>
    public Func<uint, string, uint>? Create { get; set; }

    /// <summary>반환된 같은 stream의 4바이트를 쓴다.</summary>
    public FortMoneySaveWrite? Write { get; set; }
}

/// <summary>
/// 004832c0의 같은 sharedRandom.Next(0x3f2)와 signed 정수 333.0f 가산 float32 저장 4바이트 읽기를 복원한다.
/// Money 저장과 실제 SP 저장을 분리한다(cpppj <c>o/RawFortMoneySave</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-4에서 추가했다.</remarks>
public static class RawFortMoneySave
{
    /// <summary>원본 004832c1 공유 난수에 전달하는 1010 범위다.</summary>
    private const uint MoneyRandomLimit = 0x3f2u;

    /// <summary>원본 005d7c74 float32 상수 0x43a68000이다.</summary>
    private const float MoneyBase = 333.0f;

    /// <summary>
    /// 도달한 미연결 장치만 거부하여 앞선 조회의 부분 효과를 보존한다.
    /// </summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식).</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <exception cref="InvalidOperationException">Fort Money 저장의 실제 경계가 연결되지 않았다.</exception>
    private static T NeedMoney<T>(T? function) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("Fort Money 저장 실제 경계 미연결");
        }
        return function;
    }

    /// <summary>
    /// 실제 난수 결과 0..1009와 signed FILD 333 가산을 float32 비트 그대로 쓴다.
    /// </summary>
    /// <param name="stream">쓸 stream 토큰이다.</param>
    /// <param name="random">공유 게임 난수다. 호출자가 소유한다.</param>
    /// <param name="write">일반 stream writer다.</param>
    public static void SaveFortMoney(uint stream, GameRandom random, FortMoneySaveWrite write)
    {
        ArgumentNullException.ThrowIfNull(random);
        float value = random.Next(MoneyRandomLimit) + MoneyBase;
        uint bits = BitConverter.SingleToUInt32Bits(value);
        Span<byte> bytes = stackalloc byte[4];
        // 원본 little endian 네 바이트만 전달한다
        for (int index = 0; index < bytes.Length; index++)
        {
            bytes[index] = (byte)(bits >> (index * 8));
        }
        // 미연결 검사는 난수 소비 뒤에 하며 앞선 효과를 되돌리지 않는다
        NeedMoney(write)(stream, bytes);
    }

    /// <summary>
    /// 단독 부모는 slot을 clamp하지 않으며 isMine·manager 진단을 추가하지 않는다.
    /// </summary>
    /// <param name="slot">고정 this의 Template 칸 번호다.</param>
    /// <param name="random">공유 게임 난수다.</param>
    /// <param name="hooks">실제 저장소·writer 경계다.</param>
    public static void SnapshotFortMoney(int slot, GameRandom random, FortMoneySaveHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(random);
        ArgumentNullException.ThrowIfNull(hooks);
        uint manager = NeedMoney(hooks.Manager)(slot);
        uint stream = NeedMoney(hooks.Create)(manager, "Money");
        // 미연결 검사는 난수 소비 뒤에 하며 선행 효과를 되돌리지 않는다
        SaveFortMoney(stream, random, (target, bytes) => NeedMoney(hooks.Write)(target, bytes));
    }
}
