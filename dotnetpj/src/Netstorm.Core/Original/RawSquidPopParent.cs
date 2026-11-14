using System.Buffers.Binary;
using System.Globalization;
using System.Text;
using Netstorm.Core.Rules;

// 원문 Pop의 현재 TYPE·진단 복귀·부분 공간 변경과 선택된 가상 효과를 보존한다.
namespace Netstorm.Core.Original;

/// <summary>같은 물리 TYPE 칸을 가리킨다. 배열 원소가 교체돼도 저장한 칸의 현재 필드를 읽는다.</summary>
public readonly struct SquidPopTypeReference
{
    /// <summary>동일 번호의 현재 레코드를 돌려주는 장기 저장소 보기다.</summary>
    private readonly Func<OriginalTypeRecord> _current;
    /// <summary>번호를 캡처한 같은 TYPE 칸의 getter를 빌린다. 보기는 Pop보다 오래 유지한다.</summary>
    public SquidPopTypeReference(Func<OriginalTypeRecord> current) => _current = current ?? throw new ArgumentNullException(nameof(current));
    /// <summary>원문 TYPE 포인터 역참조와 같이 현재 칸을 읽으며 새로운 Type 호출을 만들지 않는다.</summary>
    public OriginalTypeRecord Current => _current();
}

/// <summary>일반 Pop 어댑터가 실제 Client의 현재 조회·진단을 빌리는 묶음이다.</summary>
public sealed class SquidPopDiagnostics
{
    /// <summary>grid 교체를 진단할 때 현재 DWORD를 읽는 보기다.</summary>
    public RawCheckingFlag Checking { get; set; } = new();
    /// <summary>같은 물리 길이의 현재 TYPE 표이며 독립 입력만 빈 표를 쓴다.</summary>
    public IReadOnlyList<OriginalTypeRecord> Types { get; set; } = [];
    /// <summary>단계 계산 시점에 현재 SHP 크기를 선택하는 장치다.</summary>
    public Func<int, float[]>? FrameSize { get; set; }
    /// <summary>잘못된 좌표의 로그만 생략하는 현재 모드다.</summary>
    public Func<bool>? Metamap { get; set; }
    /// <summary>수준29와 완성 로그를 같은 writer에 전달한다.</summary>
    public Action<int, string>? Trace { get; set; }
    /// <summary>정상 복귀할 수 있는 현재 원문 진단이다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
    /// <summary>실제 Client의 현재 signed 세계 크기를 읽는다.</summary>
    public Func<int>? WorldCells { get; set; }
    /// <summary>함수 객체는 값 복사하고 TYPE·checking 소유자는 같은 수명으로 빌린다.</summary>
    internal SquidPopDiagnostics Copy() => new()
    {
        Checking = new RawCheckingFlag(Checking), Types = Types, FrameSize = FrameSize, Metamap = Metamap,
        Trace = Trace, Assertion = Assertion, WorldCells = WorldCells,
    };
}

/// <summary>공통 Pop이 도달하는 시점에만 요구하는 현재 조회·공간·진단·가상 호출이다.</summary>
public sealed class SquidPopParentHooks
{
    /// <summary>현재 BYTE 번호의 같은 물리 TYPE 참조를 선택한다.</summary>
    public Func<byte, SquidPopTypeReference>? Type { get; set; }
    /// <summary>점유 처리 뒤 해시 단계에 필요한 현재 SHP 크기를 조회한다.</summary>
    public Func<int, float[]>? FrameSize { get; set; }
    /// <summary>좌표 오류 로그 앞의 현재 메타맵 모드다.</summary>
    public Func<bool>? Metamap { get; set; }
    /// <summary>원문 수준29와 완성된 로그 바이트를 받는다.</summary>
    public Action<int, string>? Trace { get; set; }
    /// <summary>좌표 저장 뒤 저장한 genus로 미니맵 변경을 통지한다.</summary>
    public Action<int, uint>? MiniMap { get; set; }
    /// <summary>현재 건물 위치와 같은 표면의 옆면 효과를 실행한다.</summary>
    public Action<int, int, float, float>? Side { get; set; }
    /// <summary>해시 연결 뒤 현재 가상88/8c를 실행한다.</summary>
    public Action<int, uint>? Update { get; set; }
    /// <summary>표시 뒤 현재 가상30의 최초 Pop flags를 받는다.</summary>
    public Func<int, uint>? FirstPop { get; set; }
    /// <summary>최초 Pop flags를 합친 원문 인자로 실제 Activate를 실행한다.</summary>
    public Action<int, uint>? Activate { get; set; }
    /// <summary>정상 복귀할 수 있는 원문 식·파일·줄 진단이다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
    /// <summary>입구·시작점 검사마다 한 번 읽는 현재 signed 세계 크기이며 미연결 기본값은256이다.</summary>
    public Func<int>? WorldCells { get; set; }
}

/// <summary>원본004b02d0의 전체 물리 부모다. 진단·하위 실패에서 앞선 변경을 되돌리지 않는다.</summary>
public static class RawSquidPopParent
{
    /// <summary>원문 PE의 고정 건물 genus다.</summary>
    private const uint BuildingMask = 0x50444200;
    /// <summary>타입 번호와 구별하는 무효 섬 식별자다.</summary>
    private const ushort InvalidIsland = 127;
    /// <summary>Pop/SetIsland 원문 진단 소스다.</summary>
    private const string Source = @"\Ns\O\Squid.cpp";

    /// <summary>선택한 함수만 값으로 복사해 자기 연결 교체 중에도 현재 호출을 유지한다.</summary>
    private static T Need<T>(T? callback) where T : Delegate => callback ?? throw new InvalidOperationException("공통 Pop의 선택된 실제 장치 미연결");
    /// <summary>현재 float 필드의 비트를 추가 반올림 없이 읽는다.</summary>
    private static float Coordinate(ReadOnlySpan<byte> raw, int offset) => BinaryPrimitives.ReadSingleLittleEndian(raw[offset..]);
    /// <summary>원문 정상 ftol 범위만 허용하고 실제 지도 밖 접근은 호스트 오류로 남긴다.</summary>
    private static int Integer(float value)
    {
        if (!float.IsFinite(value) || (double)value < int.MinValue || (double)value >= 2147483648.0)
            throw new ArgumentOutOfRangeException(nameof(value), "Pop 원문 정수 좌표 범위");
        return (int)value;
    }
    /// <summary>원문 이름의 첫 NUL까지만 읽어 번역된 설명과 구별한다.</summary>
    private static string Name(SquidPopTypeReference type)
    {
        string text = type.Current.Name; int zero = text.IndexOf('\0'); return zero < 0 ? text : text[..zero];
    }
    /// <summary>현재 MSVC CRT의 %5.1f와 같은 특수값·부호 있는0·짝수 반올림을 쓴다.</summary>
    private static string FormatCoordinate(float value)
    {
        string text;
        if (float.IsNaN(value))
        {
            uint bits = BitConverter.SingleToUInt32Bits(value);
            text = (bits & 0x80000000) == 0 ? "nan" : (bits & 0x7fffffff) == 0x7fc00000 ? "-nan(ind)" : "-nan";
        }
        else if (float.IsPositiveInfinity(value)) text = "inf";
        else if (float.IsNegativeInfinity(value)) text = "-inf";
        else text = ((double)value).ToString("F1", CultureInfo.InvariantCulture);
        return text.PadLeft(5);
    }
    /// <summary>원문512바이트 버퍼를 넘기는 완성 로그는 writer 호출 전에 진단한다.</summary>
    private static void Log(SquidPopParentHooks hooks, string text)
    {
        if (Encoding.UTF8.GetByteCount(text) >= 512) throw new ArgumentOutOfRangeException(nameof(text), "Pop 로그의 호스트 버퍼 범위");
        Need(hooks.Trace)(29, text);
    }
    /// <summary>현재 진단을 선택하고 정상 복귀하면 호출 다음의 저장된 분기에서 계속한다.</summary>
    private static void Assert(SquidPopParentHooks hooks, string expression, uint line, string file = Source) => Need(hooks.Assertion)(expression, file, line);

    /// <summary>x>0 뒤에만 현재 signed 크기를 float로 한 번 저장하고 두 축의 ordered 비교에 공유한다.</summary>
    public static bool IsCurrentPopPointValid(float x, float y, Func<int>? worldCells = null)
    {
        if (!(x > 0)) return false;
        float limit = worldCells is not null ? worldCells() : 256;
        return x < limit && y > 0 && y < limit;
    }

    /// <summary>같은 PATCH 풀·해시·전체 점유 지도에 원문 좌표→spot→해시→가상 효과 순서를 적용한다.</summary>
    /// <remarks>근거: cpppj/src/o/RawSquidPopParent.h/.cpp. 성공·실패의 부분 raw/지도 변경은 호출자가 관측한다.</remarks>
    public static RawPopResult Run(RawSidPool pool, RawSquidHash hash, byte[] spots, int sid,
        float x, float y, uint flags, RawCheckingFlag checking, SquidPopParentHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(hash); ArgumentNullException.ThrowIfNull(spots);
        ArgumentNullException.ThrowIfNull(checking); ArgumentNullException.ThrowIfNull(hooks);
        checking = new RawCheckingFlag(checking);
        if (pool.Edition != OriginalEdition.Patch1078 || spots.Length != 65536) throw new ArgumentException("현재 Pop 부모의 PATCH·전체 spot 필요");
        if (sid == 0 || (uint)sid >= (uint)pool.Capacity) return RawPopResult.Unchanged;
        Span<byte> raw = pool.RawSlot(sid); if ((raw[11] & 4) == 0 || (raw[11] & 8) != 0) return RawPopResult.Unchanged;
        if (!IsCurrentPopPointValid(x, y, hooks.WorldCells))
        {
            if (!Need(hooks.Metamap)()) Log(hooks, $"Error {Name(Need(hooks.Type)(raw[10]))} popped to( {FormatCoordinate(x)},{FormatCoordinate(y)}). Set to( 10,10)\n");
            x = y = 10;
        }
        BinaryPrimitives.WriteSingleLittleEndian(raw[14..], x); BinaryPrimitives.WriteSingleLittleEndian(raw[18..], y);
        // 넓은 정수 절삭 뒤 WORD 하위 비트를 써 uint 포화가 원문 감김을 바꾸지 않게 한다.
        BinaryPrimitives.WriteUInt16LittleEndian(raw[22..], unchecked((ushort)(long)((double)x * 16 + 0.5)));
        BinaryPrimitives.WriteUInt16LittleEndian(raw[24..], unchecked((ushort)(long)((double)y * 11 + 0.5)));
        SquidPopTypeReference initial = Need(hooks.Type)(raw[10]); uint genus = initial.Current.Flags2;
        if ((genus & 6) != 0 && (raw[40] & 8) == 0) Need(hooks.MiniMap)(sid, genus);
        if (((genus & (BuildingMask | 0x400ff)) != 0 || (Need(hooks.Type)(raw[10]).Current.Flags1 & 0x800) != 0)
            && (raw[40] & 8) == 0 && (raw[11] & 8) == 0)
        {
            int right = Integer(x), bottom = Integer(y); OriginalTypeRecord bounds = initial.Current;
            int left = unchecked(right - bounds.FootX + 1), top = unchecked(bottom - bounds.FootY + 1);
            if (!IsCurrentPopPointValid(left, top, hooks.WorldCells)) Assert(hooks, "start.isValid()", 0x6b7);
            // 처음 저장한 행 범위를 유지하고 각 칸의 현재 TYPE·좌표는 안쪽에서 다시 읽는다.
            for (int cy = top; cy <= bottom; cy++)
            {
                // 진단 뒤에도 최초 열 범위로 순회하며 물리 지도 밖은 접근 전에 진단한다.
                for (int cx = left; cx <= right; cx++)
                {
                    if (cx < 0 || cy < 0 || cx >= 256 || cy >= 256) throw new ArgumentOutOfRangeException(nameof(spots), "Pop spot의 물리 지도 범위");
                    OriginalTypeRecord current = Need(hooks.Type)(raw[10]).Current;
                    byte effective = unchecked((byte)SpotRules.EffectiveGenus(current.Flags2, Coordinate(raw, 14), Coordinate(raw, 18), current.FootX, current.FootY, cx, cy));
                    if (effective != 0)
                    {
                        byte saved = spots[cy * 256 + cx]; if ((saved & effective) != 0) return RawPopResult.Overlap;
                        spots[cy * 256 + cx] = (byte)(saved | effective);
                    }
                    if ((genus & BuildingMask) != 0 && (cx == left || cx == right) && cy != top && cy != bottom)
                    {
                        ushort surface = hash.Cell(0, cx, cy);
                        if (surface > 0 && surface < pool.Capacity) Need(hooks.Side)(cx, cy, Coordinate(raw, 14), Coordinate(raw, 18));
                    }
                    if ((Need(hooks.Type)(raw[10]).Current.Flags1 & 0x800) != 0 && hash.Cell(0, cx, cy) != 0)
                    {
                        ushort replaced = hash.Cell(0, cx, cy);
                        if (checking) Assert(hooks, "false", 0x6e8);
                        string name = Name(Need(hooks.Type)(raw[10])); string oldName = Name(Need(hooks.Type)(pool.RawSlot(replaced)[10]));
                        Log(hooks, $"ERROR: gridSid {sid}( {name}) replacing {replaced}( {oldName})\n");
                    }
                }
            }
        }
        uint levelGenus = Need(hooks.Type)(raw[10]).Current.Flags2; int level = 0;
        if ((levelGenus & 6) == 0) { float[] size = Need(hooks.FrameSize)(sid); level = RawSquidHash.ObjectLevel(levelGenus, size[0], size[1]); }
        raw[33] = (byte)level; ref ushort head = ref hash.Bucket(level, Coordinate(raw, 14), Coordinate(raw, 18)); ushort next = head;
        if (next == sid) Assert(hooks, "n != getSid()", 0x123, @"c:\ns\o\squid.h");
        BinaryPrimitives.WriteUInt16LittleEndian(raw[4..], next); head = unchecked((ushort)sid);
        if (raw[33] == 0)
        {
            uint islandGenus = Need(hooks.Type)(raw[10]).Current.Flags2;
            if ((islandGenus & 0x1000006) == 0) Assert(hooks, "genus & (gISLAND|gBRIDGE|gISLAND3x3)", 0x4b9);
            else if ((islandGenus & 4) == 0) BinaryPrimitives.WriteUInt16LittleEndian(raw[8..], InvalidIsland);
        }
        Need(hooks.Update)(sid, flags);
        if ((raw[40] & 0x80) == 0) { flags |= Need(hooks.FirstPop)(sid); raw[40] |= 0x80; }
        Need(hooks.Activate)(sid, flags);
        if ((raw[11] & 8) != 0) Assert(hooks, "!isContained()", 0x71a);
        return RawPopResult.Registered;
    }
}
