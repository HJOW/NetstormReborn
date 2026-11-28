using System.Text;
using Netstorm.Assets;

// CD 디렉터리 재귀 탐색과 구별되는 루트 목록·표시 파일 확인을 제공한다.
namespace Netstorm.Client;

/// <summary>현재 OS 논리 드라이브와 종류를 원문 읽기 위치에서 공급한다.</summary>
public sealed class CdDriveRootsHooks
{
    /// <summary>128BYTE 실제 scratch에 MULTI_SZ를 쓰고 OS 반환값을 돌려준다.</summary>
    public Func<Memory<byte>, uint>? LogicalDrives { get; init; }
    /// <summary>현재 루트의 비소유 문자열 뷰를 읽고 드라이브 종류를 반환한다.</summary>
    public Func<ReadOnlyMemory<byte>, uint>? DriveType { get; init; }
    /// <summary>NULL 출력의 Winutil.cpp529 진단이며 정상 복귀하면 계속한다.</summary>
    public Action<uint>? Assertion { get; init; }
}

/// <summary>같은 현재 루트의 경로 작성·속성 존재·CD 루트 열거만 제공한다.</summary>
public sealed class CdPresenceHooks
{
    /// <summary>현재 root의 같은 BYTE 뷰로 표시 이름의 경로를 작성한다.</summary>
    public Func<ReadOnlyMemory<byte>, string, string>? Path { get; init; }
    /// <summary>현재 파일 속성 비영 판단이며 디렉터리도 성공으로 반환한다.</summary>
    public Func<string, bool>? Exists { get; init; }
    /// <summary>현재128BYTE scratch에 CD 루트 MULTI_SZ를 작성한다.0이면 읽지 않는다.</summary>
    public Func<Memory<byte>, uint>? Roots { get; init; }
}

/// <summary>004dde40/00440c60의 현재 BYTE 재조회·부분 작성·선택된 효과를 보존한다.</summary>
public static class CdPresence
{
    /// <summary>원문의 cache·루트 목록·OS scratch 물리 폭이다.</summary>
    public const int BufferSize = 128;
    /// <summary>00532544의 같은 고정 표시 파일 이름이다.</summary>
    public const string Marker = "cd.netstorm.titanic.activision";

    /// <summary>CD-ROM 종류5만 현재 순서대로 복사하고 엄격한 용량/끝NUL 규칙을 적용한다.</summary>
    public static uint CollectRoots(uint capacity, Memory<byte> output, Memory<byte> scratch, CdDriveRootsHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks); RequireScratch(scratch);
        if (output.IsEmpty) Need(hooks.Assertion)(0x529);
        uint result = Need(hooks.LogicalDrives)(scratch);
        if (result == 0 || result > BufferSize) return 0;
        int used = 0, cursor = 0;
        if (scratch.Span[0] == 0) return 0;
        // 길이는 종류 조회 전에 보관하고 실제 복사 BYTE는 콜백 뒤 다시 읽는다.
        for (;;)
        {
            ReadOnlyMemory<byte> root = Text(scratch, cursor); int length = root.Length + 1, next = used;
            if (Need(hooks.DriveType)(root) == 5)
            {
                next = used + length;
                if ((uint)next < capacity)
                {
                    Require(used, length, output.Length);
                    // 지정된 전체 길이만 원문 순서로 복사하며 뒤의 기존 물리는 유지한다.
                    for (int index = 0; index < length; index++) output.Span[used + index] = scratch.Span[cursor + index];
                }
            }
            cursor += length; used = next; Require(cursor, 1, scratch.Length);
            if (scratch.Span[cursor] == 0) break;
        }
        if (used == 0) return 0;
        uint required = (uint)used + 1;
        if (capacity < required) { Require(0, 1, output.Length); output.Span[0] = 0; return required; }
        Require(used, 1, output.Length); output.Span[used] = 0; return required;
    }

    /// <summary>캐시 성공은 즉시1이며 실패의 첫BYTE0 뒤에만 현재 CD 루트를 순회한다.</summary>
    public static uint Check(Memory<byte> cache, Memory<byte> scratch, CdPresenceHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks); RequireScratch(scratch); Require(0, 1, cache.Length);
        if (cache.Span[0] != 0)
        {
            string path = Need(hooks.Path)(Text(cache), Marker);
            if (Need(hooks.Exists)(path)) return 1;
            cache.Span[0] = 0;
        }
        if (Need(hooks.Roots)(scratch) == 0) return 0;
        int cursor = 0;
        // 다음 루트 첫BYTE를 읽기만 하며 존재 확인 실패 때문에 scratch를 변경하지 않는다.
        while (scratch.Span[cursor] != 0)
        {
            ReadOnlyMemory<byte> root = Text(scratch, cursor); int length = root.Length + 1;
            string path = Need(hooks.Path)(root, Marker);
            if (Need(hooks.Exists)(path))
            {
                // 존재 콜백이 바꾼 현재 source를 첫NUL까지 전방 복사하고 앞선 쓰기를 보존한다.
                for (int index = 0; ; index++)
                {
                    Require(cursor + index, 1, scratch.Length); Require(index, 1, cache.Length);
                    byte current = scratch.Span[cursor + index]; cache.Span[index] = current;
                    if (current == 0) break;
                }
                return 1;
            }
            cursor += length; Require(cursor, 1, scratch.Length);
        }
        return 0;
    }

    /// <summary>현재 같은 경로 고리의 -2루트/-3표시 이름 작성 결과를 BYTE 문자열로 반환한다.</summary>
    public static string JoinRootPath(RawFileName names, ReadOnlyMemory<byte> root, string marker)
    {
        ArgumentNullException.ThrowIfNull(names);
        return Encoding.Latin1.GetString(names.Format([new(-2, root), new(-3, Encoding.Latin1.GetBytes(marker)), new(0, ReadOnlyMemory<byte>.Empty)]).Span);
    }
    /// <summary>첫NUL 앞까지 현재 같은 물리 뷰를 빌린다.</summary>
    private static ReadOnlyMemory<byte> Text(Memory<byte> bytes, int begin = 0)
    {
        Require(begin, 1, bytes.Length); int length = bytes.Span[begin..].IndexOf((byte)0);
        if (length < 0) throw new ArgumentOutOfRangeException(nameof(bytes), "CD 현재 C문자열 NUL 물리 부족");
        return bytes.Slice(begin, length);
    }
    /// <summary>논리 capacity와 구별해 원문의 실제 쓰기 물리 범위를 확인한다.</summary>
    private static void Require(int at, int count, int capacity)
    {
        if (at < 0 || count < 0 || at > capacity || count > capacity - at) throw new ArgumentOutOfRangeException(nameof(at), "CD 원문 복사·쓰기 실제 물리 범위");
    }
    /// <summary>고정128BYTE의 원문 stack 인자를 관리 환경에서 검증한다.</summary>
    private static void RequireScratch(Memory<byte> scratch) { if (scratch.Length != BufferSize) throw new ArgumentException("CD scratch128BYTE 필요", nameof(scratch)); }
    /// <summary>선택된 효과만 요구하며 앞선 상태 쓰기를 되돌리지 않는다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("CD 확인의 선택된 실제 장치 미연결");
}
