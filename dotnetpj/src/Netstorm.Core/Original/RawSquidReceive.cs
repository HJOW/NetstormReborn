using System.Buffers.Binary;
using System.Globalization;
using System.Text;

// 공통35BYTE 수신의 현재 슬롯과 같은 장기 검증/경고 전역을 유지한다.
namespace Netstorm.Core.Original;

/// <summary>값 복사 없이 실제 검증SID/수신TYPE 문맥/경고표를 빌린다.</summary>
public sealed class SquidReceiveState
{
    /// <summary>0054d4d8의 같은 현재 검증SID다.</summary>
    public UIntBox VerifySid { get; }
    /// <summary>0054d4e4의 같은 현재 수신TYPE 문맥이다.</summary>
    public UIntBox TypeWas { get; }
    /// <summary>실제 LocalState의 TYPE별 경고DWORD 뷰다. 선택 시점의 물리 상한만 확인한다.</summary>
    public Memory<uint> NoMakeWarnings { get; }
    /// <summary>두 장기DWORD와 같은 경고표만 보관하며 현재 값은 쓰지 않는다.</summary>
    public SquidReceiveState(UIntBox verifySid, UIntBox typeWas, Memory<uint> noMakeWarnings)
    {
        ArgumentNullException.ThrowIfNull(verifySid); ArgumentNullException.ThrowIfNull(typeWas); VerifySid = verifySid; TypeWas = typeWas; NoMakeWarnings = noMakeWarnings;
    }
}

/// <summary>RawSquidReceive.h의 현재 공장/소유자/가상/기술/표시/로그 장치다.</summary>
public sealed class SquidReceiveHooks
{
    /// <summary>같은 Factory.Take를 실행한다.</summary>
    public Func<uint, int, int>? Take { get; set; }
    /// <summary>실제 WORD setter와 현재 다리 통지다.</summary>
    public Action<int, ushort>? Word { get; set; }
    /// <summary>현재 가상+74의 파생 소유자 setter다.</summary>
    public Action<int, uint>? Owner { get; set; }
    /// <summary>수신 원래TYPE의 현재 flags1이다.</summary>
    public Func<uint, uint>? Flags1 { get; set; }
    /// <summary>현재 가상+30의 첫 배치 플래그다.</summary>
    public Func<int, uint>? FirstPop { get; set; }
    /// <summary>현재 실제 Core 장부의 기술 bit를 읽는다.</summary>
    public Func<uint, uint, bool>? HasTechnology { get; set; }
    /// <summary>현재 이름/설명으로 실제 Tell을 실행한다.</summary>
    public Action<int>? NoMake { get; set; }
    /// <summary>현재 Player의400 표식을 실제로 기록한다.</summary>
    public Action<int>? MarkNoMake { get; set; }
    /// <summary>같은 공간의 현재 가상+90 Pop이다.</summary>
    public Action<int, float, float, uint>? Pop { get; set; }
    /// <summary>복귀형 진단 뒤 실제 가상+50 부착이다.</summary>
    public Action<int, int, uint>? ContainedPop { get; set; }
    /// <summary>검증이 선택한 현재 TYPE의 디버그 이름이다.</summary>
    public Func<int, string>? TypeName { get; set; }
    /// <summary>같은 수준1d의 실제 로그다.</summary>
    public Action<int, string>? Log { get; set; }
    /// <summary>원문 식/파일/줄을 전달하고 정상 복귀 뒤 현재 필드를 다시 읽는다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
}

/// <summary>cpppj/src/o/RawSquidReceive.cpp의2ch 실제 부분 쓰기/첫 배치/검증 본체다.</summary>
public static class RawSquidReceive
{
    /// <summary>현재 선택된 실제 장치만 요구한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("객체 수신의 선택된 실제 장치 미연결");
    /// <summary>현재 비정렬DWORD를 little endian으로 읽는다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(bytes.Slice(offset, 4));
    /// <summary>현재DWORD만 저장한다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes.Slice(offset, 4), value);
    /// <summary>원문FTOL의 유효한 정수 범위를 확인한 뒤 하위WORD를 유지한다.</summary>
    private static ushort Coordinate(double value) => double.IsFinite(value) && value >= int.MinValue && value < 2147483648.0
        ? unchecked((ushort)(int)value) : throw new ArgumentOutOfRangeException(nameof(value), "객체 수신 좌표의 실제 정수 범위");
    /// <summary>각 항목의 현재 검증SID를 저장해 현재 이름 선택 뒤 같은 로그로 보낸다.</summary>
    private static void Mismatch(RawSidPool pool, SquidReceiveState state, SquidReceiveHooks hooks, string field, uint server, uint client)
    {
        if (server == client) return;
        uint current = state.VerifySid.Value;
        if (current > ushort.MaxValue || current >= pool.Capacity) throw new ArgumentOutOfRangeException(nameof(state), "검증 로그의 현재 SID 범위");
        string name = Need(hooks.TypeName)((int)current); int zero = name.IndexOf('\0'); if (zero >= 0) name = name[..zero];
        string text = string.Create(CultureInfo.InvariantCulture, $"VERIFY {current} (client type={name}): {field} {server} != {client}\n");
        if (Encoding.UTF8.GetByteCount(text) >= 512) throw new ArgumentException("객체 검증 로그 범위"); Need(hooks.Log)(0x1d, text);
    }
    /// <summary>검증 중이면 다섯 항목만 비교한다. 일반 수신은 같은 슬롯에 원문 순서로 쓰고 정상 Pop 뒤에만TYPE 문맥을0으로 만든다.</summary>
    public static void Receive(RawSidPool pool, ReadOnlyMemory<byte> packet, SquidReceiveState state, SquidReceiveHooks hooks)
    {
        ReadOnlySpan<byte> input = packet.Span;
        if (pool.Edition != OriginalEdition.Patch1078 || input.Length < 35) throw new ArgumentException("객체 수신의 PATCH35BYTE 물리 계약");
        int incoming = BinaryPrimitives.ReadUInt16LittleEndian(input.Slice(4, 2));
        if (state.VerifySid.Value == 0)
        {
            byte selectedType = input[16], old = pool.Slot(incoming)[11]; state.TypeWas.Value = selectedType;
            int current = Need(hooks.Take)(selectedType, incoming); Span<byte> raw = pool.RawSlot(current);
            Put(raw, 14, Read(input, 6)); Put(raw, 18, Read(input, 10));
            BinaryPrimitives.WriteUInt16LittleEndian(raw.Slice(22, 2), Coordinate((double)BitConverter.UInt32BitsToSingle(Read(raw, 14)) * 16.0 + 0.5));
            BinaryPrimitives.WriteUInt16LittleEndian(raw.Slice(24, 2), Coordinate((double)BitConverter.UInt32BitsToSingle(Read(raw, 18)) * 11.0 + 0.5));
            Need(hooks.Word)(current, BinaryPrimitives.ReadUInt16LittleEndian(input.Slice(14, 2)));
            Put(raw, 42, Read(input, 19)); Put(raw, 46, Read(input, 23)); raw[11] = input[17];
            Need(hooks.Owner)(current, input[18]); raw[35] = input[29]; Put(raw, 36, Read(input, 30));
            byte extra = (byte)(input[34] ^ ((raw[40] ^ input[34]) & 4)); raw[40] = (byte)(((raw[40] ^ extra) & 2) ^ extra);
            if ((Need(hooks.Flags1)(selectedType) & 0x10) != 0) RawSquidHitPoints.Set(pool, current, BinaryPrimitives.ReadInt16LittleEndian(input.Slice(27, 2)));
            byte received = raw[11]; raw[11] |= 4; uint flags = 8;
            if ((received & 0x10) != 0 || (old & 1) != 0)
            {
                flags = Need(hooks.FirstPop)(current) | 8; raw[11] &= 0xef; uint ownerMask = 1u << (raw[34] & 31);
                if (!Need(hooks.HasTechnology)(raw[34], raw[10]))
                {
                    if (raw[10] >= state.NoMakeWarnings.Length) throw new ArgumentOutOfRangeException(nameof(state), "객체 수신의 실제 경고 표 범위");
                    if ((state.NoMakeWarnings.Span[raw[10]] & ownerMask) == 0)
                    {
                        Need(hooks.NoMake)(current);
                        if (raw[10] >= state.NoMakeWarnings.Length) throw new ArgumentOutOfRangeException(nameof(state), "Tell 뒤 현재 경고 TYPE 범위");
                        state.NoMakeWarnings.Span[raw[10]] |= ownerMask;
                    }
                    Need(hooks.MarkNoMake)(current);
                }
            }
            if ((raw[11] & 8) != 0) Need(hooks.Assertion)("!(s->isContained())", @".\Clientboard.cpp", 0xc5);
            if ((raw[11] & 8) == 0) Need(hooks.Pop)(current, BitConverter.UInt32BitsToSingle(Read(input, 6)), BitConverter.UInt32BitsToSingle(Read(input, 10)), flags);
            else Need(hooks.ContainedPop)(current, unchecked((ushort)Read(input, 6)), flags);
            state.TypeWas.Value = 0;
            return;
        }
        if ((uint)incoming != state.VerifySid.Value) Need(hooks.Assertion)("zus->sid == sidToVerify", @".\Clientboard.cpp", 0x8d);
        ReadOnlySpan<byte> checkedRaw = pool.Slot(incoming);
        Mismatch(pool, state, hooks, "type", input[16], checkedRaw[10]); Mismatch(pool, state, hooks, "playerId", input[18], checkedRaw[34]);
        Mismatch(pool, state, hooks, "freeFlag", (uint)(input[17] & 1), (uint)(checkedRaw[11] & 1));
        Mismatch(pool, state, hooks, "deadFlag", (uint)(input[17] & 2), (uint)((checkedRaw[11] >> 1) & 1));
        Mismatch(pool, state, hooks, "containedFlag", (uint)(input[17] & 8), (uint)((checkedRaw[11] >> 3) & 1));
        state.VerifySid.Value = 0;
    }
}
