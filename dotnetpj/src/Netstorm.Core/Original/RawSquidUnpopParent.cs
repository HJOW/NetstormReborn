using System.Buffers.Binary;
using Netstorm.Core.Rules;

// 실제 풀/공간 지도에서 원문 Unpop의 현재 진단과 선행 부분 쓰기를 보존한다.
namespace Netstorm.Core.Original;

/// <summary>처음 고른 물리 TYPE 칸을 보유하고 현재 원소의 필드를 읽는 비소유 보기다.</summary>
public sealed class SquidUnpopTypeReference
{
    /// <summary>선택한 물리 표/번호를 그대로 빌리는 getter다.</summary>
    private readonly Func<OriginalTypeRecord> _current;
    /// <summary>현재 표를 다시 선택하지 않고 같은 물리 칸을 읽는다.</summary>
    public OriginalTypeRecord Current => _current();
    /// <summary>호출자 TYPE 표 수명이 이 보기보다 길게 유지되어야 한다.</summary>
    public SquidUnpopTypeReference(Func<OriginalTypeRecord> current) => _current = current ?? throw new ArgumentNullException(nameof(current));
}

/// <summary>일반 공간 해제가 현재 Client의 같은 진단/TYPE 소유자를 빌린다.</summary>
public sealed class SquidUnpopDiagnostics
{
    /// <summary>각 PATCH 점유/표면 진단 소비 위치에서 현재 DWORD를 읽는다.</summary>
    public RawCheckingFlag Checking { get; set; } = new();
    /// <summary>비어 있으면 독립 호출의 TYPE 입력을 사용한다.</summary>
    public IReadOnlyList<OriginalTypeRecord> Types { get; set; } = [];
    /// <summary>정상 복귀 가능한 원문 식/파일/줄의 진단이다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
    /// <summary>장치 함수는 사본으로, TYPE/checking은 같은 실제 소유자로 빌린다.</summary>
    internal SquidUnpopDiagnostics Copy() => new() { Checking = new(Checking), Types = Types, Assertion = Assertion };
}

/// <summary>선택된 시점에만 요구하는 현재 TYPE/미니맵·옆면·종속 분리·표시 장치다.</summary>
public sealed class SquidUnpopParentHooks
{
    /// <summary>현재 TYPE BYTE가 고른 실제 물리 칸의 참조를 돌려준다.</summary>
    public Func<byte, SquidUnpopTypeReference>? Type { get; set; }
    /// <summary>void 기록 뒤 저장한 genus로 실제 미니맵에 알린다.</summary>
    public Action<int, uint>? MiniMap { get; set; }
    /// <summary>현재 표면이 있는 중간 옆면의 실제 공간 몸체다.</summary>
    public Action<int, int>? Side { get; set; }
    /// <summary>공간 점유 처리 뒤 현재 contained 비트가 선택한 실제 분리다.</summary>
    public Action<int>? Contained { get; set; }
    /// <summary>해시 제거/복귀 진단 뒤의 현재 가상88/8c를 실행한다.</summary>
    public Action<int, uint>? Update { get; set; }
    /// <summary>선택한 원문 진단이 정상 복귀하면 저장값/현재값을 구별해 계속한다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
}

/// <summary>PATCH004afe50/CD004ad0b0의 순차 공간 해제를 같은 물리 객체에 실행한다.</summary>
public static class RawSquidUnpopParent
{
    /// <summary>PE의 최초 건물 부류 마스크이며 현재 TYPE genus와 비교한다.</summary>
    private const uint BuildingMask = 0x50444200;
    /// <summary>선택한 현재 함수를 호출 끝까지 보유하며 앞선 부분 효과를 취소하지 않는다.</summary>
    private static T Need<T>(T? body) where T : class => body ?? throw new InvalidOperationException("공통 Unpop의 선택된 실제 장치 미연결");
    /// <summary>현재 raw 좌표의 전체 단정밀도 비트를 읽는다.</summary>
    private static float Coordinate(ReadOnlySpan<byte> raw, int offset) => BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw[offset..]));
    /// <summary>실제로 선택된 지도 접근에서만 유한 좌표/물리 범위를 검사하고 양수 절삭한다.</summary>
    private static int Cell(float value)
    {
        if (!float.IsFinite(value) || value < 0 || value >= 256) throw new ArgumentOutOfRangeException(nameof(value), "Unpop 지도 좌표");
        return (int)value;
    }
    /// <summary>두 판본의 원문 경로와 정상 복귀 진단을 보존한다.</summary>
    private static void Diagnose(SquidUnpopParentHooks hooks, bool patch, string expression, uint line) => Need(hooks.Assertion)(expression, patch ? @"\Ns\O\Squid.cpp" : @"\ns\o\Squid.cpp", line);

    /// <summary>진단→void/현재TYPE→순차지도/옆면→현재종속분리 또는 실제해시제거→늦은표시를 실행한다.</summary>
    public static void Run(RawSidPool pool, RawSquidHash hash, byte[] spots, int sid, uint flags, RawCheckingFlag? checking, SquidUnpopParentHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        bool patch = pool.Edition == OriginalEdition.Patch1078; int extra = patch ? 40 : 35;
        Span<byte> raw = pool.RawSlot(sid);
        if (raw[10] < 70) Diagnose(hooks, patch, "!isForm()", patch ? 0x5d8u : 0x5bfu);
        if ((raw[11] & 8) != 0) Diagnose(hooks, patch, "!isContained()", patch ? 0x5d9u : 0x5c0u);
        if (sid == 0 || sid >= pool.Capacity) Diagnose(hooks, patch, "isValid()", patch ? 0x5e0u : 0x5c7u);
        if ((raw[11] & 5) != 0) return;
        raw[11] |= 4; SquidUnpopTypeReference initial = Need(hooks.Type)(raw[10]); uint genus = initial.Current.Flags2;
        if ((genus & 6) != 0 && (raw[extra] & 8) == 0) Need(hooks.MiniMap)(sid, genus);
        if (((genus & (BuildingMask | 0x400ff)) != 0 || (Need(hooks.Type)(raw[10]).Current.Flags1 & 0x800) != 0) && (raw[extra] & 8) == 0 && (raw[11] & 8) == 0)
        {
            int right = Cell(Coordinate(raw, 14)), bottom = Cell(Coordinate(raw, 18));
            long left = (long)right - initial.Current.FootX + 1, top = (long)bottom - initial.Current.FootY + 1;
            if (left < 0 || top < 0 || initial.Current.FootX < 0 || initial.Current.FootY < 0 || initial.Current.FootX > 256 || initial.Current.FootY > 256) throw new ArgumentOutOfRangeException(nameof(sid), "Unpop 물리 발자국");
            // 저장한 y/x 범위는 진단 뒤 다시 만들지 않고 현재 TYPE/좌표는 셀마다 재조회한다.
            for (int y = (int)top; y <= bottom; y++)
            {
                // 원문 x 순서로 저장 BYTE를 고르고 정상 복귀 진단 뒤 저장값의 마스크를 쓴다.
                for (int x = (int)left; x <= right; x++)
                {
                    OriginalTypeRecord current = Need(hooks.Type)(raw[10]).Current;
                    byte effective = unchecked((byte)SpotRules.EffectiveGenus(current.Flags2, Coordinate(raw, 14), Coordinate(raw, 18), current.FootX, current.FootY, x, y));
                    if (effective != 0)
                    {
                        byte saved = spots[y * 256 + x];
                        if (patch && checking && (saved & effective) != effective) Diagnose(hooks, patch, "(flags & effectiveGenus) == ( unsigned char)effectiveGenus", 0x617);
                        spots[y * 256 + x] = unchecked((byte)(saved & ~effective));
                    }
                    if ((genus & BuildingMask) != 0 && (x == left || x == right) && y != top && y != bottom)
                    {
                        ushort surface = hash.Cell(0, x, y);
                        if (surface != 0 && surface < pool.Capacity) Need(hooks.Side)(x, y);
                    }
                    uint surfaceFlags = Need(hooks.Type)(raw[10]).Current.Flags1;
                    if (patch && (surfaceFlags & 0x800) != 0 && checking && hash.Cell(0, x, y) != sid) Diagnose(hooks, patch, "getGridSid( (float)fx, (float)fy) == getSid()", 0x629);
                }
            }
        }
        if ((raw[11] & 8) != 0) { Need(hooks.Contained)(sid); return; }
        bool found = false;
        // 각 단계는 그 시점의 현재 좌표로 실제 버킷을 선택한다.
        for (int level = 0; level < 4 && !found; level++)
        {
            ref ushort head = ref hash.Bucket(level, Coordinate(raw, 14), Coordinate(raw, 18)); ushort previous = 0, current = head; var seen = new HashSet<ushort>();
            // 범위/순환은 실제 소비 위치에서만 검사하고 이미 쓴 지도/void를 유지한다.
            while (current != 0)
            {
                if (!seen.Add(current)) throw new InvalidOperationException("Unpop 순환 체인");
                ReadOnlySpan<byte> entry = pool.RawSlot(current); ushort next = BinaryPrimitives.ReadUInt16LittleEndian(entry[4..]);
                if (current == sid)
                {
                    if (previous == 0) head = next;
                    else
                    {
                        if (previous == next) Need(hooks.Assertion)("n != getSid()", patch ? @"c:\ns\o\squid.h" : @"\ns\o\squid.h", patch ? 0x123u : 0x10cu);
                        BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(previous)[4..], next);
                    }
                    found = true; break;
                }
                previous = current; current = next;
            }
        }
        if (!found && patch) Diagnose(hooks, patch, "found", 0x65d);
        Need(hooks.Update)(sid, flags);
    }
}
