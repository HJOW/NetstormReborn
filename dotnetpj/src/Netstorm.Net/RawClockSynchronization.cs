using System.Buffers.Binary;
using Netstorm.Core.Original;

// 표본 비교·첫 일치·부분 쓰기와 실제 같은 현재 시계 흐름을 유지한다.
namespace Netstorm.Net;

/// <summary>004c1e70/1eb0/1f60/2070의 서버 응답과 Commander 표본 부모다.</summary>
public sealed class RawClockSynchronization
{
    /// <summary>같은 실제 연결/응답/보정 DWORD다.</summary>
    private readonly ClockSyncResetGlobals _globals;
    /// <summary>같은 장기 Client의70개 표본과 진단값이다.</summary>
    private readonly ClockSynchronizationData _data;
    /// <summary>선택된 실제 시계/직접 송신이다.</summary>
    private readonly ClockSynchronizationHooks _hooks;
    /// <summary>장기 고정F만 보유할 실제 Client 전역 소유자이며 Session을 대신하지 않는다.</summary>
    private readonly object? _dataOwner;
    /// <summary>실제 같은 표본/전역을 빌리며 생성 시 등록/초기화를 수행하지 않는다.</summary>
    public RawClockSynchronization(ClockSyncResetGlobals globals, ClockSynchronizationData data, ClockSynchronizationHooks hooks, object? dataOwner = null)
    {
        ArgumentNullException.ThrowIfNull(globals); ArgumentNullException.ThrowIfNull(data); ArgumentNullException.ThrowIfNull(hooks);
        if (data.Samples.Length != 2240) throw new ArgumentException("시간 동기화70×32 실제 표본 누락", nameof(data)); _globals = globals; _data = data; _hooks = hooks; _dataOwner = dataOwner;
    }
    /// <summary>선택된 실제 하위만 요구하여 앞선 원문 쓰기를 유지한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("시간 동기화의 실제 장치 미연결");
    /// <summary>정렬되지 않은 현재8바이트를 그대로 double로 읽는다.</summary>
    private static double Read(ReadOnlySpan<byte> bytes, int at) => BitConverter.UInt64BitsToDouble(BinaryPrimitives.ReadUInt64LittleEndian(bytes[at..]));
    /// <summary>같은8바이트만 바꾸고 인접 헤더/필드를 보존한다.</summary>
    private static void Write(Span<byte> bytes, int at, double value) => BinaryPrimitives.WriteUInt64LittleEndian(bytes[at..], BitConverter.DoubleToUInt64Bits(value));
    /// <summary>세 원문 double만0을 쓰며 헤더/미작성DWORD를 유지한다.</summary>
    private static void ClearTimes(Span<byte> packet) { Write(packet, 8, 0); Write(packet, 16, 0); Write(packet, 24, 0); }
    /// <summary>유효 signed64 절삭 하위DWORD이며 x86 indefinite의 하위값은0이다.</summary>
    private static uint Truncate(double value) => !double.IsFinite(value) || value >= 9223372036854775808.0 || value < -9223372036854775808.0 ? 0u : unchecked((uint)(long)value);
    /// <summary>실제 같은5번 수신기/비NULL 빈 observer를 등록하며 version/길이/정책은 유지한다.</summary>
    private void Register(RawPacketDispatch dispatch)
    {
        dispatch.Observe(5, static _ => { });
        if (_dataOwner is { } owner)
        {
            ClockSyncResetGlobals globals = _globals; ClockSynchronizationData data = _data; ClockSynchronizationHooks hooks = _hooks;
            dispatch.RegisterCanonicalDirect(5, 0x004c1f60, (_, connection, packet) =>
            {
                if (connection is null) throw new InvalidOperationException("시간 수신의 실제 connection 누락"); if (packet.Length < 32) throw new ArgumentOutOfRangeException(nameof(packet), "시간 동기화 실제32바이트 부족");
                byte[] current = packet.Span[..32].ToArray(); new RawClockSynchronization(globals, data, hooks).Receive(connection, current); GC.KeepAlive(owner);
            });
            dispatch.PersistCanonicalBaseline(5); return;
        }
        dispatch.Register(5, (_, connection, packet) =>
        {
            if (packet.Length < 32) throw new ArgumentOutOfRangeException(nameof(packet), "시간 동기화 실제32바이트 부족"); Receive(connection, packet.Span[..32].ToArray());
        });
    }
    /// <summary>등록 뒤 송수신 수만0으로 쓰고 응답모드1을 선택한다. 기존 표본/연결은 보존한다.</summary>
    public void InitializeServer(RawPacketDispatch dispatch) { Register(dispatch); _data.Sent.Value = 0; _data.Received.Value = 0; _globals.ReplyMode.Value = 1; }
    /// <summary>등록→표본 전체0→현재 벽시각→카운터/모드/기한→연결 순서로 Commander를 붙인다.</summary>
    public void Attach(RawPacketDispatch dispatch, uint connection)
    {
        Register(dispatch); _data.Samples.Span.Clear(); double now = Need(_hooks.WallSeconds)(); _data.Sent.Value = 0; _data.Received.Value = 0; _globals.ReplyMode.Value = 0; _data.Next.Value = now; _globals.Connection.Value = connection;
    }
    /// <summary>현재 응답모드에 따라 직접 응답하거나 같은 요청시간의 첫 표본만 복사한다.</summary>
    public void Receive(PacketConnection connection, Memory<byte> packet)
    {
        ArgumentNullException.ThrowIfNull(connection); if (packet.Length < 32) throw new ArgumentOutOfRangeException(nameof(packet), "시간 수신 실제32바이트 범위"); byte[] address = new byte[20]; address[0] = 2;
        if ((connection.Flags & 0x220) != 0) connection.Address.CopyTo(address, 0);
        if (_globals.ReplyMode.Value != 0)
        {
            double now = Need(_hooks.GameSeconds)(); Write(packet.Span, 16, now); ReadOnlyMemory<byte>? destination = null; if ((connection.Flags & 0x220) != 0) destination = address; Need(_hooks.Reply)(connection, packet[..32], destination); return;
        }
        double wall = Need(_hooks.WallSeconds)(); Write(packet.Span, 24, wall);
        // 중복/빈 요청시간도 첫 double 동등 표본만 선택하고 횟수는 누적한다.
        for (int index = 0; index < 70; index++)
        {
            Memory<byte> sample = _data.Samples.Slice(index * 32, 32); if (Read(sample.Span, 8) != Read(packet.Span, 8)) continue; _data.Received.Value = unchecked(_data.Received.Value + 1); packet.Span[..32].CopyTo(sample.Span); return;
        }
    }
    /// <summary>현재70표본을 만료/선별하고 빠른 절반 평균 뒤 기한별 표본 하나만 실제 송신한다.</summary>
    public void Poll()
    {
        if (_globals.ReplyMode.Value != 0 || _globals.Connection.Value == 0) return; double now = Need(_hooks.WallSeconds)(); var accepted = new SamplePair[70]; int count = 0;
        // 모든 현재 표본의 세 시각을 원문 순서로 읽고 만료 시간만 지운다.
        for (int index = 0; index < 70; index++)
        {
            Span<byte> sample = _data.Samples.Span.Slice(index * 32, 32); double request = Read(sample, 8), server = Read(sample, 16), received = Read(sample, 24);
            if (!(now - request <= 2) && received == 0) { ClearTimes(sample); continue; }
            if (request != 0 && server != 0 && received != 0)
            {
                double roundTrip = received - request, offset = received - server; if (roundTrip >= 2 || double.IsNaN(roundTrip)) { ClearTimes(sample); continue; } accepted[count++] = new(offset, roundTrip);
            }
        }
        if (count != 0)
        {
            Sort(accepted, count); int used = Math.Max(count / 2, 1); double roundTrips = 0, offsets = 0;
            // RTT가 빠른 절반의 각 합계를 같은 인덱스 순서대로 누적한다.
            for (int index = 0; index < used; index++) { roundTrips += accepted[index].RoundTrip; offsets += accepted[index].Offset; }
            double reciprocal = 1.0 / used, latency = (reciprocal * roundTrips) * 0.5; _data.LatencyMilliseconds.Value = Truncate(1000 * latency); _globals.GameTicks.Value = Truncate((reciprocal * offsets - latency) * 1000);
        }
        if (!((unchecked((int)_data.Sent.Value) <= 30 || _data.Received.Value != 0) && _data.Next.Value != 0 && _data.Next.Value <= now)) return;
        // 첫 빈 표본 하나만 선택하며 공간이 없어도 뒤 기한/횟수 관문은 처리한다.
        for (int index = 0; index < 70; index++)
        {
            Memory<byte> sample = _data.Samples.Slice(index * 32, 32); if (now - Read(sample.Span, 8) > 30) ClearTimes(sample.Span); if (Read(sample.Span, 8) != 0) continue;
            ClearTimes(sample.Span); sample.Span[1] = 5; sample.Span[2] = 32; sample.Span[3] = 0; Write(sample.Span, 8, now); Need(_hooks.Send)(_globals.Connection.Value, sample); _data.Sent.Value = unchecked(_data.Sent.Value + 1); break;
        }
        _data.Next.Value = 0; if (unchecked((int)_data.Sent.Value) < 60) _data.Next.Value = now + 1;
    }
    /// <summary>원문 두 double의 현재 offset/RTT이며 정렬 때 함께 교환한다.</summary>
    /// <param name="Offset">수신 시각과 서버 시각 차이다.</param>
    /// <param name="RoundTrip">수신 시각과 요청 시각 차이다.</param>
    private readonly record struct SamplePair(double Offset, double RoundTrip);
    /// <summary>PATCH CRT가 작은 선택 정렬로 바꾸는 항목 수다.</summary>
    private const int Small = 8;
    /// <summary>원본 CRT의 고정 구간 스택 크기다.</summary>
    private const int Ranges = 30;
    /// <summary>양 끝을 포함하는 원문 정렬 구간이다.</summary>
    /// <param name="Low">현재 첫 항목이다.</param>
    /// <param name="High">현재 마지막 항목이다.</param>
    private readonly record struct SortRange(int Low, int High);
    /// <summary>PATCH004e6010/004e5fa0의 피벗/동률/교환 순서다. Array.Sort로 대체하지 않는다.</summary>
    private static void Sort(SamplePair[] entries, int count)
    {
        var stack = new SortRange[Ranges]; int used = 0, low = 0, high = count - 1;
        // 원문 포인터 대신 현재 항목 인덱스를 비교한다.
        int Cmp(int a, int b) => entries[a].RoundTrip < entries[b].RoundTrip ? -1 : entries[b].RoundTrip < entries[a].RoundTrip ? 1 : 0;
        // 두 double 전체를 같은 항목으로 교환한다.
        void Swap(int a, int b) { if (a != b) (entries[a], entries[b]) = (entries[b], entries[a]); }
        // 큰 구간을 고정 스택에 보관한다.
        void Push(int first, int last)
        {
            if (used == stack.Length) throw new InvalidOperationException("시간 표본 정렬 스택 넘침");
            stack[used++] = new(first, last);
        }
        // 현재 구간과 보관 구간을 원문 분기 순서대로 처리한다.
        for (;;)
        {
            if (high - low + 1 <= Small)
            {
                // 작은 구간은 엄격히 큰 최대 항목만 끝으로 보내는 선택 정렬이다.
                while (low < high)
                {
                    int maximum = low;
                    // 동률 항목으로 최대 위치를 바꾸지 않는다.
                    for (int current = low + 1; current <= high; current++) if (Cmp(current, maximum) > 0) maximum = current;
                    Swap(maximum, high); high--;
                }
            }
            else
            {
                int middle = low + (high - low + 1) / 2;
                if (Cmp(low, middle) > 0) Swap(low, middle);
                if (Cmp(low, high) > 0) Swap(low, high);
                if (Cmp(middle, high) > 0) Swap(middle, high);
                int left = low, right = high;
                // 자기 피벗 비교를 건너뛰며 두 탐색 위치가 교차할 때까지 나눈다.
                for (;;)
                {
                    if (left < middle)
                    {
                        // 피벗 앞 작은 항목과 동률 항목을 지난다.
                        do { left++; } while (left < middle && Cmp(left, middle) <= 0);
                    }
                    if (left >= middle)
                    {
                        // 피벗 뒤의 작은/동률 항목도 끝까지 검사한다.
                        do { left++; } while (left <= high && Cmp(left, middle) <= 0);
                    }
                    // 오른쪽은 엄격히 큰 항목만 지난다.
                    do { right--; } while (right > middle && Cmp(right, middle) > 0);
                    if (left > right) break;
                    Swap(left, right); if (middle == right) middle = left;
                }
                right++;
                if (right > middle)
                {
                    // 피벗 오른쪽의0 비교 항목을 제거한다.
                    do { right--; } while (right > middle && Cmp(right, middle) == 0);
                }
                if (right <= middle)
                {
                    // 피벗을 건너뛰고 왼쪽 동률 구간을 제거한다.
                    do { right--; } while (right > low && Cmp(right, middle) == 0);
                }
                if (right - low < high - left)
                {
                    if (left < high) Push(left, high); high = right;
                    if (low < high) continue;
                }
                else
                {
                    if (low < right) Push(low, right); low = left;
                    if (low < high) continue;
                }
            }
            if (used == 0) return;
            SortRange range = stack[--used]; low = range.Low; high = range.High;
        }
    }
}
