using System.Buffers.Binary;

// 같은 실제 풀의 내용물·감시·삭제를 원문 효과 순서로 처리한다.
namespace Netstorm.Core.Original;

/// <summary>RawSquidMutationReceive.h의 현재 가상/공장/환불/로그 장치다. 선택한 위치에서만 요구한다.</summary>
public sealed class SquidMutationHooks
{
    /// <summary>현재00541088의 내용물 Form TYPE다.</summary>
    public Func<uint>? ContentType { get; set; }
    /// <summary>현재00541074의 감시 Form TYPE다.</summary>
    public Func<uint>? DependType { get; set; }
    /// <summary>삭제 입구의 현재boss 값이다.</summary>
    public Func<bool>? Boss { get; set; }
    /// <summary>같은 Factory.Take로 지정 SID의 실제 생성자를 실행한다.</summary>
    public Func<uint, int, int>? Take { get; set; }
    /// <summary>실제 WORD setter와 현재 다리 통지를 호출한다.</summary>
    public Action<int, ushort>? Word { get; set; }
    /// <summary>현재 가상+30의 첫 배치 플래그다.</summary>
    public Func<int, uint>? FirstPop { get; set; }
    /// <summary>현재 가상+50의 실제 파생 부착이다.</summary>
    public Action<int, int, uint>? Attach { get; set; }
    /// <summary>현재 감시 가상+74(parent,0)의 실제 부착이다.</summary>
    public Action<int, int, uint>? DependencyAttach { get; set; }
    /// <summary>현재 HP에 비례한 실제 환불이다.</summary>
    public Action<int, uint>? Refund { get; set; }
    /// <summary>현재 가상+10의 실제 삭제다.</summary>
    public Action<int, uint>? Destroy { get; set; }
    /// <summary>로그가 선택한 현재 TYPE 이름이다.</summary>
    public Func<int, string>? TypeName { get; set; }
    /// <summary>같은 수준1d의 실제 로그다.</summary>
    public Action<int, string>? Log { get; set; }
    /// <summary>삭제 꼬리의 현재 분산SP/추적값/검사 횟수를 조회한다.</summary>
    public Action? AuditSp { get; set; }
    /// <summary>원문 식/파일/줄을 전달하며 진단 정상 복귀도 허용한다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
}

/// <summary>cpppj/src/o/RawSquidMutationReceive.cpp의 실제 수신 다섯 종과62h 송신 관찰기다.</summary>
public static class RawSquidMutationReceive
{
    /// <summary>선택된 실제 callable만 요구하며 현재 호출의 참조를 유지한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("객체 변경 수신의 선택된 실제 장치 미연결");
    /// <summary>선택한 현재WORD만 읽어 앞선 부분 효과를 보존한다.</summary>
    private static ushort Word(ReadOnlySpan<byte> bytes, int offset)
    {
        if (offset < 0 || offset > bytes.Length || bytes.Length - offset < 2) throw new ArgumentOutOfRangeException(nameof(bytes), "원격 삭제의 현재 WORD 물리 부족");
        return BinaryPrimitives.ReadUInt16LittleEndian(bytes.Slice(offset, 2));
    }
    /// <summary>62h의 현재DWORD를 부호 해석 전에 읽는다.</summary>
    private static uint Count(ReadOnlySpan<byte> packet)
    {
        if (packet.Length < 8) throw new ArgumentOutOfRangeException(nameof(packet), "원격 다중 삭제의 현재 count DWORD 물리 부족");
        return BinaryPrimitives.ReadUInt32LittleEndian(packet.Slice(4, 4));
    }
    /// <summary>판본/전체 고정 머리를 확인하고 선택 위치 이전의 다른 장치는 조회하지 않는다.</summary>
    private static void Contract(RawSidPool pool, ReadOnlySpan<byte> packet, int length)
    {
        if (pool.Edition != OriginalEdition.Patch1078 || packet.Length < length) throw new ArgumentException($"객체 변경 수신의 PATCH{length}BYTE 계약");
    }
    /// <summary>2dh는 Take 전 상태와 현재 payload/반환 SID를 구별해 저장한 뒤 실제 가상 부착한다.</summary>
    public static void ReceiveContent(RawSidPool pool, ReadOnlyMemory<byte> packet, SquidMutationHooks hooks)
    {
        ReadOnlySpan<byte> input = packet.Span; Contract(pool, input, 18);
        int incoming = Word(input, 4); byte old = pool.Slot(incoming)[11];
        int current = Need(hooks.Take)(Need(hooks.ContentType)(), incoming);
        Need(hooks.Word)(current, Word(input, 6));
        Span<byte> raw = pool.RawSlot(current);
        BinaryPrimitives.WriteUInt16LittleEndian(raw.Slice(18, 2), Word(input, 8));
        BinaryPrimitives.WriteUInt16LittleEndian(raw.Slice(20, 2), Word(input, 10));
        BinaryPrimitives.WriteUInt16LittleEndian(raw.Slice(24, 2), Word(input, 12)); raw[23] = input[16];
        byte received = input[17]; raw[11] = (byte)(received | 4); uint flags = 8;
        if ((received & 0x10) != 0 || (old & 1) != 0) { flags = Need(hooks.FirstPop)(current) | 8; raw[11] &= 0xef; }
        Need(hooks.Attach)(current, Word(input, 14), flags);
    }
    /// <summary>2eh는 boss 진단/현재 이름 로그 뒤 free/환불/실제 삭제를 선택하고 마지막에SP를 감사한다.</summary>
    public static void ReceiveDeleted(RawSidPool pool, ReadOnlyMemory<byte> packet, SquidMutationHooks hooks)
    {
        ReadOnlySpan<byte> input = packet.Span; Contract(pool, input, 8);
        int current = Word(input, 4); ReadOnlySpan<byte> raw = pool.Slot(current);
        if (Need(hooks.Boss)()) Need(hooks.Assertion)("!boss", @".\Clientboard.cpp", 0x260);
        string name = Need(hooks.TypeName)(current); Need(hooks.Log)(0x1d, "destroying " + name + "\n");
        if ((raw[11] & 1) == 0)
        {
            uint flags = (uint)Word(input, 6) << 16;
            if (raw[10] >= 70 && (flags & 0x200000) != 0) Need(hooks.Refund)(current, raw[34]);
            Need(hooks.Destroy)(current, flags | 8);
        }
        else if (raw[10] != Need(hooks.DependType)()) Need(hooks.Log)(0x1d, "~rdouble destroy");
        Need(hooks.AuditSp)();
    }
    /// <summary>65h는 소유자 setter나firstPop을 호출하지 않고 현재 감시 가상+74로 부착한다.</summary>
    public static void ReceiveDependency(RawSidPool pool, ReadOnlyMemory<byte> packet, SquidMutationHooks hooks)
    {
        ReadOnlySpan<byte> input = packet.Span; Contract(pool, input, 13);
        int current = Need(hooks.Take)(Need(hooks.DependType)(), Word(input, 4)); Span<byte> raw = pool.RawSlot(current);
        raw[11] = input[8]; BinaryPrimitives.WriteUInt16LittleEndian(raw.Slice(18, 2), Word(input, 9));
        BinaryPrimitives.WriteUInt16LittleEndian(raw.Slice(20, 2), Word(input, 11)); raw[11] |= 4;
        Need(hooks.DependencyAttach)(current, Word(input, 6), 0);
    }
    /// <summary>53h는SID1 이상/물리 범위/free만 검사하고 현재 가상 삭제에flags0을 전달한다.</summary>
    public static void ReceiveRemoteDeleted(RawSidPool pool, ReadOnlyMemory<byte> packet, Action<int, uint>? destroy)
    {
        if (pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("53h 원격 삭제의 PATCH 판본 불일치");
        int sid = Word(packet.Span, 4); if (sid == 0 || sid >= pool.Capacity) return;
        if ((pool.Slot(sid)[11] & 1) == 0) Need(destroy)(sid, 0);
    }
    /// <summary>62h는 각 삭제 뒤 같은 현재 signed count를 다시 읽으며 범위 선검사/중복 제거를 하지 않는다.</summary>
    public static void ReceiveRemoteDeletedMany(RawSidPool pool, ReadOnlyMemory<byte> packet, Action<int, uint>? destroy)
    {
        if (pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("62h 원격 삭제의 PATCH 판본 불일치");
        ReadOnlySpan<byte> input = packet.Span; int count = unchecked((int)Count(input)); if (count <= 0) return; int index = 0;
        // 삭제 효과 뒤 count와 다음 현재WORD를 다시 선택해 이전 부분 삭제를 보존한다.
        do
        {
            int sid = Word(input, checked(8 + index * 2)); if ((pool.Slot(sid)[11] & 1) == 0) Need(destroy)(sid, 0);
            count = unchecked((int)Count(input)); index++;
        } while (index < count);
    }
    /// <summary>송신에서만DWORD sentinel 진단 뒤 현재lowWORD로 길이를 쓰고16비트 감김을 보존한다.</summary>
    public static void ObserveRemoteDeletedMany(Memory<byte> packet, uint sending, Action<string, string, uint>? assertion)
    {
        if (sending == 0) return; Span<byte> bytes = packet.Span;
        if (Count(bytes) == uint.MaxValue) Need(assertion)("count != (int)-1", @"\Ns\O\_p_packets.cpp", 0x9ac);
        BinaryPrimitives.WriteUInt16LittleEndian(bytes.Slice(2, 2), unchecked((ushort)(((uint)Word(bytes, 4) + 4) * 2)));
    }
}
