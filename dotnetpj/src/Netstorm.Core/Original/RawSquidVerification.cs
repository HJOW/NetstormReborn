using System.Buffers.Binary;
using System.Globalization;
using System.Text;

// 객체 수신과 같은 장기 두DWORD로 실제 검증 간격을 순회한다.
namespace Netstorm.Core.Original;

/// <summary>값을 복사하지 않는 현재 검증SID/이전SID 참조다.</summary>
public sealed class SquidVerificationState
{
    /// <summary>0054d4d8의 같은 현재DWORD다.</summary>
    public UIntBox Sid { get; }
    /// <summary>0054d4dc의 같은 순회 위치DWORD다.</summary>
    public UIntBox Previous { get; }
    /// <summary>실제 장기 owner의 두 주소를 빌리며 현재 값을 변경하지 않는다.</summary>
    public SquidVerificationState(UIntBox sid, UIntBox previous) { ArgumentNullException.ThrowIfNull(sid); ArgumentNullException.ThrowIfNull(previous); Sid = sid; Previous = previous; }
}

/// <summary>현재 TYPE 이름/실제 로그와 원문 복귀형 진단이다.</summary>
public sealed class SquidVerificationHooks
{
    /// <summary>선택한 실제 SID의 현재 TYPE 이름이다.</summary>
    public Func<int, string>? TypeName { get; set; }
    /// <summary>수준1d의 실제 VERIFY 로그다.</summary>
    public Action<int, string>? Log { get; set; }
    /// <summary>원문 식/파일/줄을 받고 복귀하면 같은 현재 입력으로 계속한다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
}

/// <summary>cpppj/src/o/RawSquidVerification.cpp의4ah/4bh와 간격 순회다.</summary>
public static class RawSquidVerification
{
    /// <summary>현재 선택된 장치만 요구한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("객체 검증의 선택된 실제 장치 미연결");
    /// <summary>DWORD를WORD로 자르기 전에 실제 SID 물리를 확인한다.</summary>
    private static int Sid(RawSidPool pool, uint value) => value <= ushort.MaxValue && value < pool.Capacity ? (int)value : throw new ArgumentOutOfRangeException(nameof(value), "객체 검증의 실제 SID 물리 범위");
    /// <summary>현재 이름 함수와 로그 함수를 원문 선택 순서로 호출한다.</summary>
    private static void Log(RawSidPool pool, SquidVerificationHooks hooks, uint value, string field, uint server, uint client)
    {
        string name = Need(hooks.TypeName)(Sid(pool, value)); int zero = name.IndexOf('\0'); if (zero >= 0) name = name[..zero];
        string text = string.Create(CultureInfo.InvariantCulture, $"VERIFY {value} (client type={name}): {field} {server} != {client}\n");
        if (Encoding.UTF8.GetByteCount(text) >= 512) throw new ArgumentException("객체 검증의 실제 로그 범위"); Need(hooks.Log)(0x1d, text);
    }
    /// <summary>현재SID 직전까지 다음 번호를 조사하고 signed 비교의65535 경계에서15000으로 감는다.</summary>
    public static void Advance(RawSidPool pool, SquidVerificationState state, SquidVerificationHooks hooks)
    {
        if (pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("검증 간격의 PATCH SID 순회 계약");
        if (state.Previous.Value == 0) { state.Previous.Value = state.Sid.Value; return; }
        // 콜백이 바꾼 현재 두DWORD와 저장 슬롯의 현재TYPE를 매 소비 위치에서 다시 읽는다.
        while (state.Previous.Value != state.Sid.Value)
        {
            state.Previous.Value = unchecked(state.Previous.Value + 1);
            if (unchecked((int)state.Previous.Value) >= 65535) state.Previous.Value = 15000;
            if (state.Previous.Value == state.Sid.Value) return;
            ReadOnlySpan<byte> raw = pool.Slot(Sid(pool, state.Previous.Value)); bool freeOrForm = (raw[11] & 1) != 0 || raw[10] < 70;
            if (!freeOrForm) Log(pool, hooks, state.Previous.Value, "on server is free||form, but not on client", 1, 0);
            if (raw[10] != 0) Log(pool, hooks, state.Previous.Value, "server type != client type", 0, raw[10]);
        }
    }
    /// <summary>4bh는 기존SID 진단이 복귀한 뒤 현재 입력SID를 저장하고 간격을 순회한다.</summary>
    public static void Begin(RawSidPool pool, ReadOnlyMemory<byte> packet, SquidVerificationState state, SquidVerificationHooks hooks)
    {
        if (pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("검증 시작의 PATCH 계약");
        ReadOnlySpan<byte> input = packet.Span; if (input.Length < 6) throw new ArgumentOutOfRangeException(nameof(packet), "검증 시작의6BYTE 부족");
        if (state.Sid.Value != 0) Need(hooks.Assertion)("!sidToVerify", @".\Clientboard.cpp", 0x4c);
        state.Sid.Value = BinaryPrimitives.ReadUInt16LittleEndian(input.Slice(4, 2)); Advance(pool, state, hooks);
    }
    /// <summary>4ah는 현재 슬롯/입력 비교 뒤 간격을 조사하고 정상 종료에서SID만0으로 만든다.</summary>
    public static void ReceiveFree(RawSidPool pool, ReadOnlyMemory<byte> packet, SquidVerificationState state, SquidVerificationHooks hooks)
    {
        if (pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("자유 객체 검증의 PATCH 계약");
        ReadOnlySpan<byte> input = packet.Span; if (input.Length < 7) throw new ArgumentOutOfRangeException(nameof(packet), "자유 객체 검증의7BYTE 부족");
        uint incoming = BinaryPrimitives.ReadUInt16LittleEndian(input.Slice(4, 2)); ReadOnlySpan<byte> raw = pool.Slot(Sid(pool, incoming)); state.Sid.Value = incoming;
        if ((raw[11] & 1) == 0) Log(pool, hooks, incoming, "isFree", 1, (uint)(raw[11] & 1));
        if (input[6] != raw[10]) Log(pool, hooks, state.Sid.Value, "freeType", input[6], raw[10]);
        Advance(pool, state, hooks); state.Sid.Value = 0;
    }
}
