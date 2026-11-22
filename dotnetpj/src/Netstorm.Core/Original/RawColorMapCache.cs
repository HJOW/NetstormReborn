using System.Buffers.Binary;

namespace Netstorm.Core.Original; // 최초 고정 명도/소유자 표와 같은 실제 캐시 파일 단계를 실행한다.

/// <summary>Native EX_UNABLE_TO_OPEN_FILE만 구별하는 원문 typed 분류다.</summary>
public sealed class ColorCacheUnableToOpenException : Exception
{
    /// <summary>일반 파일 오류 전체를 이 분류로 바꾸지 않는다.</summary>
    public ColorCacheUnableToOpenException() : base("EX_UNABLE_TO_OPEN_FILE") { }
}

/// <summary>같은 실제 장기 출력 배열과 현재 primary getter다.</summary>
public sealed class ColorMapCacheGlobals
{
    /// <summary>경로 callback 뒤 현재 raw primary를 한 번 빌린다.</summary>
    public required Func<ReadOnlyMemory<byte>> Primary { get; init; }
    /// <summary>행마다 현재 lowBYTE를 읽는8DWORD다.</summary>
    public required ReadOnlyMemory<uint> OwnerColors { get; init; }
    /// <summary>현재8×256BYTE 명도 치환 출력이다.</summary>
    public required Memory<byte> OwnerHighlight { get; init; }
    /// <summary>현재8×256BYTE 고정색 출력이다.</summary>
    public required Memory<byte> OwnerSolid { get; init; }
    /// <summary>메모리 주소순과 다른 실제 파일 순서의9개256BYTE 표다.</summary>
    public required Memory<byte>[] Maps { get; init; }
}

/// <summary>실제 경로/파일 생성·부분 입출력/동일 객체 반납 경계다.</summary>
public sealed class ColorMapCacheHooks
{
    /// <summary>같은 NUL 경로 할당 전체 물리다.</summary>
    public Func<ReadOnlyMemory<byte>>? Path { get; set; }
    /// <summary>실제 TYPE별 현재 owner 표 초기화다.</summary>
    public Action? Owners { get; set; }
    /// <summary>최초 고정 입력/현재 빨강으로 실제9표를 재생성한다.</summary>
    public Action? BuildMaps { get; set; }
    /// <summary>같은24BYTE reader의 원문0/1 열기다.</summary>
    public Action<Memory<byte>, ReadOnlyMemory<byte>, uint, uint>? OpenRead { get; set; }
    /// <summary>같은24BYTE writer의 원문1/1 열기다.</summary>
    public Action<Memory<byte>, ReadOnlyMemory<byte>, uint, uint>? OpenWrite { get; set; }
    /// <summary>반환 길이를 분기 조건으로 쓰지 않는 실제 부분 읽기다.</summary>
    public Func<Memory<byte>, Memory<byte>, uint, uint>? Read { get; set; }
    /// <summary>반환 길이를 분기 조건으로 쓰지 않는 실제 부분 쓰기다.</summary>
    public Func<Memory<byte>, ReadOnlyMemory<byte>, uint, uint>? Write { get; set; }
    /// <summary>정상/catch cleanup에서 같은24BYTE 객체를 반납한다.</summary>
    public Action<Memory<byte>>? Close { get; set; }
}

/// <summary>Source RawColorMapCache의43c4e0 준비/독립 catch/cleanup 전체다.</summary>
public static class RawColorMapCache
{
    /// <summary>같은 값과 NaN은 교체하지 않는 원문 double이다.</summary>
    private const double HighlightThreshold = 0.800000011920929;
    /// <summary>선택된 실제 하위만 도달 위치에서 요구한다.</summary>
    private static T Need<T>(T? callback) where T : Delegate => callback ?? throw new InvalidOperationException("ColorMap cache 실제 하위 장치 미연결");
    /// <summary>원문14DWORD 지역 복사와 각 복사 직전 범위 확인 뒤 저장 명도를 읽는다.</summary>
    private static double CurrentLight(ReadOnlyMemory<byte> primary, int offset)
    {
        Span<byte> record = stackalloc byte[56];
        // 실패 전 작성한 출력은 유지하고 복사할 현재4BYTE만 확인한다.
        for (int word = 0; word < 14; word++) { int source = offset + word * 4; if (source > primary.Length || primary.Length - source < 4) throw new ArgumentOutOfRangeException(nameof(primary), "ColorMap cache raw 기록 물리 범위"); primary.Span.Slice(source, 4).CopyTo(record.Slice(word * 4, 4)); }
        return BitConverter.UInt64BitsToDouble(BinaryPrimitives.ReadUInt64LittleEndian(record[8..]));
    }
    /// <summary>원문 고정 span의 실제 물리만 확인한다. primary는 각 DWORD 복사 위치에서 확인한다.</summary>
    private static void Validate(ColorMapCacheGlobals globals)
    {
        ArgumentNullException.ThrowIfNull(globals); if (globals.OwnerColors.Length != 8 || globals.OwnerHighlight.Length != 2048 || globals.OwnerSolid.Length != 2048 || globals.Maps.Length != 9 || globals.Maps.Any(map => map.Length != 256)) throw new ArgumentException("ColorMap cache 실제 출력 물리 계약");
    }
    /// <summary>Path→현재 primary 한 번→solid/identity/명도8행→실제 TYPE 등록 순서다.</summary>
    public static ReadOnlyMemory<byte> Prepare(ColorMapCacheGlobals globals, ColorMapCacheHooks hooks)
    {
        Validate(globals); ArgumentNullException.ThrowIfNull(hooks); ReadOnlyMemory<byte> path = Need(hooks.Path)(), primary = Need(globals.Primary)();
        // 각 행 시작에서 현재 owner DWORD의 lowBYTE를 다시 읽는다.
        for (int row = 0; row < 8; row++)
        {
            byte selected = unchecked((byte)globals.OwnerColors.Span[row]);
            // 원문은 고정색→항등→56BYTE 복사→조건 치환 순서다.
            for (int color = 0; color < 256; color++) { int offset = row * 256 + color; globals.OwnerSolid.Span[offset] = selected; globals.OwnerHighlight.Span[offset] = (byte)color; if (CurrentLight(primary, color * 56) > HighlightThreshold) globals.OwnerHighlight.Span[offset] = selected; }
        }
        Need(hooks.Owners)(); return path;
    }
    /// <summary>같은 실제 객체와 장치만 cleanup에 전달한다.</summary>
    public static void Close(Memory<byte> file, ColorMapCacheHooks hooks) { if (file.Length != 24) throw new ArgumentException("ColorMap cache FILE24BYTE 계약"); Need(hooks.Close)(file); }
    /// <summary>독립 catch의9표 재생성→writer→9write→정상 또는 unwind close다.</summary>
    public static void Regenerate(ReadOnlyMemory<byte> path, ColorMapCacheGlobals globals, ColorMapCacheHooks hooks, Memory<byte> writer)
    {
        Validate(globals); if (writer.Length != 24) throw new ArgumentException("ColorMap cache writer24BYTE 계약"); Need(hooks.BuildMaps)(); bool constructed = false;
        try
        {
            Need(hooks.OpenWrite)(writer, path, 1, 1); constructed = true;
            // 부분 쓰기의 반환 길이는 검사하거나 재시도하지 않는다.
            foreach (var table in globals.Maps) Need(hooks.Write)(writer, table, uint.MaxValue);
            constructed = false; Close(writer, hooks);
        }
        catch { if (constructed) { constructed = false; Close(writer, hooks); } throw; }
    }
    /// <summary>준비는try앞·reader 끝8BYTE만0·정확 marker만 재생성하며 같은 reader try로 돌아가지 않는다.</summary>
    public static void Initialize(ColorMapCacheGlobals globals, ColorMapCacheHooks hooks, Memory<byte> reader, Memory<byte> writer)
    {
        if (reader.Length != 24 || writer.Length != 24) throw new ArgumentException("ColorMap cache 두 FILE24BYTE 계약"); var path = Prepare(globals, hooks); reader.Span[16..].Clear(); bool constructed = false;
        try
        {
            try
            {
                Need(hooks.OpenRead)(reader, path, 0, 1); constructed = true;
                // 같은9개 목적 물리에 실제 부분 읽기를 남긴다.
                foreach (var table in globals.Maps) Need(hooks.Read)(reader, table, uint.MaxValue);
                constructed = false; Close(reader, hooks);
            }
            catch { if (constructed) { constructed = false; Close(reader, hooks); } throw; }
        }
        catch (ColorCacheUnableToOpenException) { Regenerate(path, globals, hooks, writer); }
    }
}
