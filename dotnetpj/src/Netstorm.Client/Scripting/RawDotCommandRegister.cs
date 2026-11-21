using System.Buffers.Binary;

// 원본 이름/함수/표시/count의 인접 물리와 복사 후 현재 count를 보존한다.
namespace Netstorm.Client.Scripting;

/// <summary>실제31BYTE CRT 복사와 도중 현재 count 변경을 제공하는 경계다.</summary>
public delegate void DotCommandNameCopy(Span<byte> destination, ReadOnlySpan<byte> source, uint count);
/// <summary>상한 return에서 요구하지 않는 실제 이름 복사 장치다.</summary>
public sealed class DotCommandRegisterHooks
{
    /// <summary>기본은 같은 물리의 strncpy이며 명시 누락도 선택 위치에서만 실패한다.</summary>
    public DotCommandNameCopy? Copy { get; set; } = RawDotCommandRegister.CopyDotCommandName;
}
/// <summary>RawDotCommandRegister.cpp의0043f660과 순방향 strncpy 본체다.</summary>
public static class RawDotCommandRegister
{
    /// <summary>이름2048/함수256/표시260/count4의 인접2568BYTE다.</summary>
    public const int StateBytes = 0xa08;
    /// <summary>표시 항목이 사용하는 원문 이름표 주소 식별자다.</summary>
    public const uint NamesToken = 0x531b30;
    /// <summary>숨긴 명령의 원문 빈 문자열 주소 식별자다.</summary>
    public const uint EmptyToken = 0x503c8b;
    /// <summary>같은 표 안의 callback 시작 위치다.</summary>
    private const uint CallbackOffset = 0x800;
    /// <summary>같은 표 안의 표시 포인터 시작 위치다.</summary>
    private const uint VisibleOffset = 0x900;
    /// <summary>표시 종단과 겹칠 수 있는 실제 count 위치다.</summary>
    public const uint CountOffset = 0xa04;
    /// <summary>32비트 주소 산술 뒤 한 접근의 전체 실제 폭을 확인한다.</summary>
    private static Span<byte> Physical(Span<byte> state, uint offset, uint count)
    {
        if ((ulong)offset + count > (ulong)state.Length) throw new ArgumentOutOfRangeException(nameof(offset), "점 명령 등록의 실제 인접 표 범위");
        return state.Slice((int)offset, (int)count);
    }
    /// <summary>같은 인접 물리의 네BYTE를 정렬과 무관하게 읽는다.</summary>
    private static uint Read(Span<byte> state, uint offset) => BinaryPrimitives.ReadUInt32LittleEndian(Physical(state, offset, 4));
    /// <summary>전체 DWORD 폭을 확인한 뒤 같은 little endian 필드에 쓴다.</summary>
    private static void Write(Span<byte> state, uint offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(Physical(state, offset, 4), value);
    /// <summary>첫NUL 이후 source를 더 읽지 않고 count 전체의 NUL padding을 쓴다.</summary>
    public static void CopyDotCommandName(Span<byte> destination, ReadOnlySpan<byte> source, uint count)
    {
        if (count > destination.Length) throw new ArgumentOutOfRangeException(nameof(count), "점 명령 이름 복사의 목적 물리 범위");
        bool ended = false;
        // 같은 source에서 한BYTE를 읽고 바로 쓰는 CRT 순서를 유지한다.
        for (int index = 0; index < (int)count; index++)
        {
            if (!ended && index >= source.Length) throw new ArgumentOutOfRangeException(nameof(source), "점 명령 이름 복사의 원본 물리 범위");
            byte value = ended ? (byte)0 : source[index]; ended |= value == 0; destination[index] = value;
        }
    }
    /// <summary>진입 signed count 관문과 복사 뒤 단일 count 캡처·인접 store 순서를 보존한다.</summary>
    public static uint RegisterDotCommand(Span<byte> state, ReadOnlySpan<byte> name, uint callback, uint flag, DotCommandRegisterHooks? hooks = null)
    {
        uint initial = Read(state, CountOffset); int signedCount = unchecked((int)initial);
        if (signedCount >= 64) return 0;
        if (signedCount < 0) throw new ArgumentOutOfRangeException(nameof(state), "점 명령 음수 초기count의 원시 주소는 지원 물리 계약 밖");
        var copy = (hooks ?? new()).Copy ?? throw new InvalidOperationException("점 명령 등록의 실제 strncpy 장치 미연결");
        copy(Physical(state, unchecked(initial * 32), 31), name, 31);
        uint selected = Read(state, CountOffset);
        Write(state, unchecked(CallbackOffset + selected * 4), callback);
        Physical(state, unchecked(selected * 32 + 31), 1)[0] = 0;
        Write(state, unchecked(VisibleOffset + selected * 4), flag != 0 ? unchecked(NamesToken + selected * 32) : EmptyToken);
        uint updated = unchecked(selected + 1); Write(state, CountOffset, updated);
        Write(state, unchecked(VisibleOffset + updated * 4), 0); return 1;
    }
}
