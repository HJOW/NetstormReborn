namespace Netstorm.Core.Original;

/// <summary>
/// Template.cpp의 실제 진단이며 복귀하면 그 뒤 현재 전역을 다시 읽는다(cpppj <c>o/RawFortCoreWrite.h</c>의 <c>FortCoreWriteAssertion</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-6에서 추가했다.</remarks>
public enum FortCoreWriteAssertion : uint
{
    /// <summary>진입의 고정 this 첫 DWORD가 0이다. Editor에서도 먼저 진단한다.</summary>
    MissingTemplate = 0x45f,

    /// <summary>현재 00594fc8이 비영이다. 진단 후 post·editor 자연 관문을 계속한다.</summary>
    InMetamap = 0x460,

    /// <summary>clamp한 고정 this의 Player 번호와 현재 00540c70이 다르다.</summary>
    WrongPlayer = 0x467,
}

/// <summary>
/// 같은 현재 Player 배열을 빌리는 조회다. C++ <c>std::function&lt;std::span&lt;uint8_t&gt;()&gt;</c>를 옮겼다.
/// 호출자가 소유한 9×180바이트 배열의 같은 물리를 빌리며 사본을 만들지 않는다.
/// </summary>
/// <remarks>2026-11-03 F2c-6에서 추가했다.</remarks>
public delegate Span<byte> FortCoreWritePlayers();

/// <summary>
/// 0044cce0의 일반 writer다. 고정 Player+6c 10바이트 다음 로컬 seed 2바이트를 쓴다.
/// </summary>
/// <param name="stream">쓸 stream 토큰이다.</param>
/// <param name="bytes">쓸 바이트 구간이다.</param>
/// <remarks>2026-11-03 F2c-6에서 추가했다. <c>Span</c>을 담는 대리자는 전용 대리자로 둔다.</remarks>
public delegate void FortCoreWriteWriter(uint stream, ReadOnlySpan<byte> bytes);

/// <summary>
/// 같은 Client·Session의 현재 전역을 빌린다(cpppj <c>o/RawFortCoreWrite.h</c>의 <c>FortCoreWriteGlobals</c>).
/// 이 모듈은 독립 전역이나 난수를 소유하지 않는다.
/// </summary>
/// <remarks>2026-11-03 F2c-6에서 추가했다. C++의 <c>uint32_t&amp;</c> seed 참조는 읽기·쓰기 대리자 쌍으로 나눈다.</remarks>
public sealed class FortCoreWriteGlobals
{
    /// <summary>00594fc8이며 manager 진단 복귀 뒤 조회한다.</summary>
    public Func<uint>? Metamap { get; set; }

    /// <summary>00594fa4이며 metamap 진단 뒤 조회한다. 비영이면 editor도 읽지 않는다.</summary>
    public Func<uint>? MissionPost { get; set; }

    /// <summary>005c85a4이며 post 0일 때만 조회한다. 비영이면 하위 writer를 선택하지 않는다.</summary>
    public Func<uint>? Editor { get; set; }

    /// <summary>0059534c의 현재 Player 배열이다. Create 복귀 뒤 한 번 받은 물리를 본문 끝까지 유지한다.</summary>
    public FortCoreWritePlayers? Players { get; set; }

    /// <summary>00540c70이며 고정 Player 주소 계산 뒤 진단에 사용한다. Template owner와 구별한다.</summary>
    public Func<uint>? PlayerOwner { get; set; }

    /// <summary>005c89b4의 같은 DWORD를 읽는다. 첫 10바이트 write 뒤에만 요구하며 자연 반환에는 요구하지 않는다.</summary>
    public Func<uint>? Seed { get; set; }

    /// <summary>005c89b4의 같은 DWORD를 쓴다. C++ 참조 대입과 같다.</summary>
    public Action<uint>? SetSeed { get; set; }
}

/// <summary>
/// 도달하지 않은 하위는 요구하지 않는다(cpppj <c>o/RawFortCoreWrite.h</c>의 <c>FortCoreWriteHooks</c>).
/// 선택된 장치의 실제 본문은 빈 성공으로 대체하지 않는다.
/// </summary>
/// <remarks>2026-11-03 F2c-6에서 추가했다. 검사가 일부 장치를 빼고 호출하므로 속성은 읽고 쓸 수 있게 열었다.</remarks>
public sealed class FortCoreWriteHooks
{
    /// <summary>고정 this의 현재 첫 DWORD를 두 원문 읽기 위치에서 조회한다.</summary>
    public Func<int, uint>? Manager { get; set; }

    /// <summary>실제 Template.cpp 행 번호 진단이다. 부분 중단은 그대로 전파한다.</summary>
    public Action<FortCoreWriteAssertion>? Assertion { get; set; }

    /// <summary>0044d1b0의 현재 manager·CoreData이며 반환한 같은 stream을 모든 child에 넘긴다.</summary>
    public Func<uint, string, uint>? Create { get; set; }

    /// <summary>0044fc00의 실제 선행 효과다. 직렬화보다 먼저 호출하며 부재를 성공으로 채우지 않는다.</summary>
    public Action? Refresh { get; set; }

    /// <summary>0044cce0의 일반 writer다. 고정 Player+6c 10바이트 다음 로컬 seed 2바이트를 쓴다.</summary>
    public FortCoreWriteWriter? Write { get; set; }

    /// <summary>현재 seed low16이 0일 때 한 번 실행하는 004559d0이다.</summary>
    public Action? PrepareRandom { get; set; }

    /// <summary>004558c0(65536)의 실제 공유 난수다. 반환 low16이 0이면 반복하며 GameRandom의 새 owner를 만들지 않는다.</summary>
    public Func<uint, uint>? NextRandom { get; set; }

    /// <summary>004be960의 실제 SP writer다. 각 호출 직전 고정 Player의 byte+70·word+6c·word+6e를 다시 읽는다.</summary>
    public Action<uint, float>? Sp { get; set; }
}

/// <summary>
/// 004beb30: manager·metamap 진단→post·editor 자연 반환→실제 CoreData 생성·쓰기 순서다(cpppj <c>o/RawFortCoreWrite</c>).
/// slot 1..8만 같은 Player 번호이고 그 밖 slot은 Player 0이다. 범위 밖 물리는 명시 실패하며 callback 부분 효과를 복구하지 않는다.
/// </summary>
/// <remarks>2026-11-03 F2c-6에서 추가했다.</remarks>
public static class RawFortCoreWrite
{
    /// <summary>
    /// 원문에서 실제로 선택된 하위 함수만 필수로 요구한다.
    /// </summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식).</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <exception cref="InvalidOperationException">CoreWrite의 선택된 실제 장치가 연결되지 않았다.</exception>
    private static T NeedCore<T>(T? function) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("CoreWrite의 선택된 실제 장치 누락");
        }
        return function;
    }

    /// <summary>
    /// 원문 포인터의 같은 살아 있는 물리 할당에서 선택 폭만 반환한다.
    /// </summary>
    /// <param name="bytes">고를 물리다.</param>
    /// <param name="offset">고를 위치다.</param>
    /// <param name="width">고를 너비다.</param>
    /// <exception cref="ArgumentOutOfRangeException">CoreWrite의 고정 Player 물리 범위다.</exception>
    private static ReadOnlySpan<byte> Physical(ReadOnlySpan<byte> bytes, int offset, int width)
    {
        if (offset < 0 || offset > bytes.Length || width < 0 || width > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), offset, "CoreWrite의 고정 Player 물리 범위");
        }
        return bytes.Slice(offset, width);
    }

    /// <summary>
    /// unsigned 바이트·워드를 실제 little endian 필드 폭으로 다시 읽는다.
    /// </summary>
    /// <param name="bytes">읽을 물리다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <param name="width">읽을 너비다.</param>
    private static uint Unsigned(ReadOnlySpan<byte> bytes, int offset, int width)
    {
        ReadOnlySpan<byte> field = Physical(bytes, offset, width);
        uint result = 0;
        // 상위의 다른 Player 바이트는 읽지 않는다
        for (int index = 0; index < field.Length; index++)
        {
            result |= (uint)field[index] << (index * 8);
        }
        return result;
    }

    /// <summary>
    /// 004beb30: manager·metamap 진단→post·editor 자연 반환→실제 CoreData 생성·쓰기 순서다.
    /// </summary>
    /// <param name="slot">고정 this 슬롯이다.</param>
    /// <param name="globals">같은 Client·Session의 현재 전역이다.</param>
    /// <param name="hooks">선택된 실제 하위 경계다.</param>
    public static void WriteFortCoreData(int slot, FortCoreWriteGlobals globals, FortCoreWriteHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (NeedCore(hooks.Manager)(slot) == 0)
        {
            NeedCore(hooks.Assertion)(FortCoreWriteAssertion.MissingTemplate);
        }
        if (NeedCore(globals.Metamap)() != 0)
        {
            NeedCore(hooks.Assertion)(FortCoreWriteAssertion.InMetamap);
        }
        if (NeedCore(globals.MissionPost)() != 0)
        {
            return;
        }
        if (NeedCore(globals.Editor)() != 0)
        {
            return;
        }
        uint manager = NeedCore(hooks.Manager)(slot);
        uint stream = NeedCore(hooks.Create)(manager, "CoreData");
        uint player = slot >= 1 && slot <= 8 ? (uint)slot : 0u;
        Span<byte> players = NeedCore(globals.Players)();
        int offset = unchecked((int)(player * 180u));
        // Create 뒤 현재 base를 한 번만 보관하며 callback의 현재 base 변경으로 바꾸지 않는다
        if (player != NeedCore(globals.PlayerOwner)())
        {
            NeedCore(hooks.Assertion)(FortCoreWriteAssertion.WrongPlayer);
        }
        NeedCore(hooks.Refresh)();
        FortCoreWriteWriter write = NeedCore(hooks.Write);
        write(stream, Physical(players, offset + 0x6c, 10));
        // 앞선 write 복귀 뒤 실제 고정 DWORD를 빌리며 Editor 자연 반환의 장치를 미리 요구하지 않는다
        uint seed = NeedCore(globals.Seed)();
        if ((ushort)seed == 0)
        {
            NeedCore(hooks.PrepareRandom)();
            // 준비 callback이 seed를 바꿔도 원문 do의 첫 Next를 생략하지 않는다
            seed = NeedCore(globals.Seed)();
            ushort low;
            do
            {
                // 실제 반환 low16이 비영일 때까지 같은 난수를 계속 소비한다. 임의 횟수 제한을 넣지 않는다
                low = (ushort)NeedCore(hooks.NextRandom)(0x10000u);
                uint current = NeedCore(globals.Seed)();
                current = (current & 0xffff0000u) | low;
                NeedCore(globals.SetSeed)(current);
                seed = current;
            }
            while (low == 0);
        }
        uint local = NeedCore(globals.Seed)();
        byte[] seedBytes = [(byte)local, (byte)(local >> 8)];
        // 앞 writer가 현재 global을 바꿔도 이 로컬 두 바이트는 바뀌지 않는다
        NeedCore(hooks.Write)(stream, seedBytes);
        uint rank = Unsigned(players, offset + 0x70, 1);
        NeedCore(hooks.Sp)(stream, rank);
        uint first = Unsigned(players, offset + 0x6c, 2);
        NeedCore(hooks.Sp)(stream, first);
        uint second = Unsigned(players, offset + 0x6e, 2);
        NeedCore(hooks.Sp)(stream, second);
    }
}
