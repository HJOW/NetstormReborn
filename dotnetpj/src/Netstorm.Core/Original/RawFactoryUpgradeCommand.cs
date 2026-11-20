using System.Buffers.Binary;

// 시설 업그레이드의 원본 요청·수신과 공통16바이트 메시지를 처리한다.
namespace Netstorm.Core.Original;

/// <summary>시설 연구와 업그레이드가 공유하는 58h 요청을 원본 폭으로 작성한다.</summary>
/// <remarks>근거: cpppj/src/o/RawFactoryResearch.cpp의 WriteFactoryResearchCommand만 이식했다.</remarks>
public static class FactoryResearchCommand
{
    /// <summary>헤더를 포함한 원본 메시지 길이다.</summary>
    public const int PacketBytes = 16;

    /// <summary>헤더·SID·kind 하위BYTE·mode0·현재 QWORD 시각을 쓰고 버퍼 꼬리를 보존한다.</summary>
    public static int Write(int parent, uint kind, double time, Span<byte> packet)
    {
        if (parent < 0 || parent > ushort.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(parent), "시설 연구 현재 WORD SID 범위");
        }
        if (packet.Length < PacketBytes)
        {
            throw new ArgumentOutOfRangeException(nameof(packet), "시설 연구16바이트 저장소 부족");
        }
        packet[0] = 1;
        packet[1] = 0x58;
        BinaryPrimitives.WriteUInt16LittleEndian(packet.Slice(2, 2), PacketBytes);
        BinaryPrimitives.WriteUInt16LittleEndian(packet.Slice(4, 2), (ushort)parent);
        packet[6] = unchecked((byte)kind);
        packet[7] = 0;
        BinaryPrimitives.WriteUInt64LittleEndian(packet.Slice(8, 8), BitConverter.DoubleToUInt64Bits(time));
        return PacketBytes;
    }
}

/// <summary>요청·수신의 각 원본 위치에서 현재 권한·진단·시각을 읽는다.</summary>
public sealed class FactoryUpgradeCommandState
{
    /// <summary>현재 서버 권한이다. 수신 말미 방송 여부를 고른다.</summary>
    public Func<bool>? Boss { get; set; }
    /// <summary>현재 genus 진단 DWORD의 비영 뷰다.</summary>
    public RawCheckingFlag Checking { get; set; } = new();
    /// <summary>현재 QWORD 게임 시각이다.</summary>
    public Func<double>? Now { get; set; }
}

/// <summary>실제 비용·SP·SharedRegular·전송과 진단 장치만 연결한다.</summary>
public sealed class FactoryUpgradeCommandHooks
{
    /// <summary>현재 TYPE별 복호화 비용 단위다.</summary>
    public Func<uint, int>? Cost { get; set; }
    /// <summary>현재 소유자에게 실제 SP와 AI 지갑을 가산한다.</summary>
    public Action<uint, float>? AddSp { get; set; }
    /// <summary>SharedRegular 메모리 예약 성공 여부다. 실패해도 요청/방송을 계속한다.</summary>
    public Func<bool>? ReserveShared { get; set; }
    /// <summary>예약된 실제 SharedRegular를 같은 부모·사건·지연으로 부착한다.</summary>
    public Action<int, uint, float>? AddShared { get; set; }
    /// <summary>채널9로 실제 요청 원문을 송신한다.</summary>
    public Action<uint, ReadOnlyMemory<byte>>? Send { get; set; }
    /// <summary>수신된 현재 원문의 처음16바이트를 방송한다.</summary>
    public Action<ReadOnlyMemory<byte>>? Broadcast { get; set; }
    /// <summary>원본 종류4의 예약 로그다.</summary>
    public Action<uint, string>? Log { get; set; }
    /// <summary>반환 뒤 현재 genus를 다시 조회하는 원본 진단이다.</summary>
    public Action<string>? Assertion { get; set; }
}

/// <summary>00453bf0 업그레이드 요청과 00453d70 mode1 수신 접두를 실행한다.</summary>
/// <remarks>근거: cpppj/src/o/RawFactoryUpgradeCommand.cpp·h. 2026-11-20 0.666에서 이식했다.
/// event3 착공·그림·완료는 별도 본체이며 실제 네트워크 장치는 생성하지 않는다.</remarks>
public sealed class RawFactoryUpgradeCommand
{
    /// <summary>요청의 사건4 SharedRegular 대기 시간이다.</summary>
    private const float RequestWait = 20f;
    /// <summary>요청 메시지에 더하는 업그레이드 완료 간격이다.</summary>
    private const double UpgradeDuration = 15.0;
    /// <summary>수신 지연의 유한 최소 시간이다.</summary>
    private const double MinimumWait = 1.0;
    /// <summary>현재 시설의 실제 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>현재 TYPE genus 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>현재 전역을 빌리는 장치다.</summary>
    private readonly FactoryUpgradeCommandState _state;
    /// <summary>선택된 실제 하위 효과다.</summary>
    private readonly FactoryUpgradeCommandHooks _hooks;

    /// <summary>게임 효과 없이 같은 PATCH 풀·TYPE 계약을 확인한다.</summary>
    public RawFactoryUpgradeCommand(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types,
        FactoryUpgradeCommandState state, FactoryUpgradeCommandHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count == 0)
        {
            throw new ArgumentException("시설 업그레이드 같은 PATCH/TYPE 계약 누락");
        }
        _pool = pool;
        _types = types;
        _state = state;
        _hooks = hooks;
    }

    /// <summary>사건4/20초 예약 뒤 현재시각+15초의 kind0/mode1 요청을 송신한다.</summary>
    public void Request(int factory)
    {
        if (Need(_hooks.ReserveShared)())
        {
            Need(_hooks.AddShared)(factory, 4, RequestWait);
        }
        var packet = new byte[FactoryResearchCommand.PacketBytes];
        FactoryResearchCommand.Write(factory, 0, Need(_state.Now)() + UpgradeDuration, packet);
        packet[7] = 1;
        Need(_hooks.Send)(9, packet);
    }

    /// <summary>현재 상태·mode·genus 확인→비용→로그→사건3 예약→현재 권한 방송 순서다.</summary>
    public void Receive(ReadOnlyMemory<byte> packet)
    {
        int factory = BinaryPrimitives.ReadUInt16LittleEndian(packet.Span.Slice(4, 2));
        if ((Read(factory, 11) & 3u) != 0)
        {
            return;
        }
        if (packet.Span[6] != 0 || packet.Span[7] != 1)
        {
            return;
        }
        if (_state.Checking.Read() != 0 && (Genus(factory) & 0x4000u) == 0)
        {
            Need(_hooks.Assertion)("factory.getGenus() & gFACTORY");
        }
        if ((Genus(factory) & 0x4000u) == 0)
        {
            return;
        }
        int cost = Need(_hooks.Cost)(Read(factory, 10));
        float delta = (float)-(double)cost;
        Need(_hooks.AddSp)(Read(factory, 34), delta);
        Need(_hooks.Log)(4, "new ftUPGRADE sharedRegPro\n");
        if (Need(_hooks.ReserveShared)())
        {
            double due = BitConverter.UInt64BitsToDouble(BinaryPrimitives.ReadUInt64LittleEndian(packet.Span.Slice(8, 8)));
            double difference = due - Need(_state.Now)();
            // unordered에서는 원래 차이를 선택하므로 NaN을 최소1초로 바꾸지 않는다.
            float wait = (float)(difference < MinimumWait ? MinimumWait : difference);
            Need(_hooks.AddShared)(factory, 3, wait);
        }
        if (Need(_state.Boss)())
        {
            if (packet.Length < FactoryResearchCommand.PacketBytes)
            {
                throw new ArgumentOutOfRangeException(nameof(packet), "시설 업그레이드 원문16바이트 방송 범위");
            }
            Need(_hooks.Broadcast)(packet.Slice(0, FactoryResearchCommand.PacketBytes));
        }
    }

    /// <summary>실제 세션 조립 시 같은 풀인지 확인한다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>같은 현재 슬롯의 지정 BYTE를 읽는다.</summary>
    private uint Read(int factory, int offset) => _pool.Slot(factory)[offset];
    /// <summary>진단 등 하위 효과 뒤 현재 TYPE에서 genus를 다시 읽는다.</summary>
    private uint Genus(int factory)
    {
        uint type = Read(factory, 10);
        return type < (uint)_types.Count ? _types[(int)type].Flags2
            : throw new ArgumentOutOfRangeException(nameof(factory), "시설 업그레이드 현재 TYPE 범위");
    }
    /// <summary>선택된 미연결을 빈 성공으로 대신하지 않는다.</summary>
    private static T Need<T>(T? function) where T : Delegate => function ?? throw new InvalidOperationException("시설 업그레이드 실제 하위·전역 미연결");
}
