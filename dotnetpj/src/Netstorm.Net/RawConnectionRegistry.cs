using System.Runtime.ExceptionServices;

// 실제 고정표·노드/장치 수명과 원문 등록/정리/접속 버퍼를 하나로 연결한다.
namespace Netstorm.Net;

/// <summary>선택된 원문 진단의 문장·파일·행을 보존하고 정상 복귀를 허용한다.</summary>
public delegate void ConnectionRegistryAssertion(string expression, string file, uint line);

/// <summary>원문 현재 전송ID가 선택한 실제 노드의 수신 결과다.</summary>
/// <param name="Consumed">실제 완성 프레임을 꺼냈는지다.</param>
/// <param name="Again">같은 현재 정상ID를 다시 읽어야 하는지다.</param>
public readonly record struct TransportPollResult(bool Consumed, bool Again);

/// <summary>cpppj/src/o/RawConnectionRegistry.cpp의 같은 숫자token·고정표·수명 본체다.</summary>
public sealed class RawConnectionRegistry
{
    /// <summary>등록 시 선택한 같은 실제 노드와 불변 함수 값이다.</summary>
    private sealed class Binding
    {
        /// <summary>실제 Source가 소유한 동일 노드다.</summary>
        public required PacketConnection Node { get; init; }
        /// <summary>같은 노드의 실제 close다.</summary>
        public required Action Destroy { get; init; }
        /// <summary>같은 노드의 실제 free다.</summary>
        public required Action Release { get; init; }
        /// <summary>선택된 가상+4 송신이다.</summary>
        public Action<Memory<byte>>? Send { get; init; }
        /// <summary>같은 socket의 실제 준비 상태다.</summary>
        public Func<int>? Readiness { get; init; }
        /// <summary>같은 노드의4df470 비영 입구다.</summary>
        public Action? Buffered { get; init; }
        /// <summary>같은 노드의 가상+0c flush다.</summary>
        public Action? Flush { get; init; }
        /// <summary>같은 노드의 실제 프레임/종료 수신이다.</summary>
        public Func<TransportPollResult>? Receive { get; init; }
        /// <summary>실제Client만 등록 노드 수명까지 보유하는 Source 대여다.</summary>
        public ConnectionDevicePin? DeviceOwner { get; set; }
    }
    /// <summary>원문Binding 값 복사처럼 등록 당시 실제Source도 현재 호출 끝까지 보유한다.</summary>
    private readonly struct SelectedBinding : IDisposable
    {
        /// <summary>이미 선택한 노드와 함수 값이다.반납 뒤 재선택하지 않는다.</summary>
        public Binding Value { get; }
        /// <summary>원래Binding이 지워져도 호출 복귀까지 살아 있는 등록 Source다.</summary>
        public ConnectionDevicePin? Owner { get; }
        /// <summary>실제 등록Source를 선택한 뒤에만 값 복사를 공개한다.</summary>
        public SelectedBinding(Binding value) { Value = value; Owner = value.DeviceOwner?.Fork(); }
        /// <summary>선택된 가상 효과가 끝난 뒤 등록Source 대여를 반납한다.</summary>
        public void Dispose() => Owner?.Dispose();
    }
    /// <summary>원문005e4a18의22개 실제 현재 송수신 token이다.</summary>
    private readonly uint[] _transceivers = new uint[22];
    /// <summary>005e4a70의 노드와 별개인22개 접속초기 버퍼다.</summary>
    private readonly uint[] _buffers = new uint[22];
    /// <summary>005e46f0의32개 실제pending이다.0칸을 임의 초기화하지 않는다.</summary>
    private readonly uint[] _pending = new uint[32];
    /// <summary>Registry가 직접 소유한00543604 개수다.</summary>
    private uint _pendingCount;
    /// <summary>독립 기본 동기화 또는 현재 실제Client의 같은 보관자다.</summary>
    private ConnectionSyncState _synchronization = new();
    /// <summary>현재 동기화 주소를 고정해 빌리는 같은 표 뷰다.</summary>
    private ConnectionCleanupGlobals _cleanup;
    /// <summary>다시 발급하지 않는 다음DWORD이며 포인터 절삭이 아니다.</summary>
    private uint _nextToken = 0x6d000000;
    /// <summary>살아 있는 실제 노드만 발급 순서대로 보관한다.</summary>
    private readonly SortedDictionary<uint, Binding> _bindings = [];
    /// <summary>노드와 같은 발급기를 쓰는 별도 실제 복제 배열이다.</summary>
    private readonly Dictionary<uint, byte[]> _connectPackets = [];
    /// <summary>실제Client의 약한 경로가 현재 선택 Source 대여를 반환한다.</summary>
    private Func<PacketConnection, ConnectionDevicePin?>? _devicePin;
    /// <summary>실제Client만 등록 노드에Source를 보유하도록 한다.</summary>
    private bool _retainDeviceOwners;
    /// <summary>앱 최종 반납 진입 뒤에는 새 노드를 만들지 않는다.</summary>
    private bool _deviceShutdown;
    /// <summary>최외곽Native부모의 마지막 표 쓰기까지를 포함하는 동기 깊이다.</summary>
    private uint _deviceCallDepth;
    /// <summary>현재 최외곽 호출이 실제로 선택한 서로 다른Source 제어 블록들이다.</summary>
    private List<ConnectionDevicePin>? _deviceCallOwners;
    /// <summary>Native 부모의 직접 칸/count 쓰기와 일반 free 별칭 반납을 구별한다.</summary>
    private uint _cleanupDepth;
    /// <summary>BSS0과 동일 표 주소만 준비한다.노드/장치 효과는 없다.</summary>
    public RawConnectionRegistry() { _cleanup = MakeCleanup(_synchronization); }
    /// <summary>현재 동기화 주소를 값 복사하지 않고 고정하며 개수는Registry의 같은DWORD를 빌린다.</summary>
    private ConnectionCleanupGlobals MakeCleanup(ConnectionSyncState synchronization) => new(_transceivers, _buffers, _pending,
        new ConnectionSyncState(() => synchronization.Synchronization, value => synchronization.Synchronization = value, () => _pendingCount, value => _pendingCount = value));
    /// <summary>동기화 보관자/값은 쓰지 않고 현재 빌리는 주소만 교체한다.</summary>
    public void BindSynchronization(ConnectionSyncState synchronization)
    {
        ArgumentNullException.ThrowIfNull(synchronization); _synchronization = synchronization; _cleanup = MakeCleanup(synchronization);
    }
    /// <summary>같은 현재 표/동기화 주소를 빌린다.이전 뷰는 기존 주소를 계속 가리킨다.</summary>
    public ConnectionCleanupGlobals Cleanup() => _cleanup;
    /// <summary>독립 직접 소비자는 같은 표에 명시 동기화 주소를 빌릴 수 있다.</summary>
    public ConnectionCleanupGlobals Cleanup(ConnectionSyncState synchronization) => MakeCleanup(synchronization);
    /// <summary>실제Source 선택 getter만 교체하며 노드/게임 값을 바꾸지 않는다.</summary>
    public void BindDevicePin(Func<PacketConnection, ConnectionDevicePin?>? pin, bool retainNodes = false) { _devicePin = pin; _retainDeviceOwners = retainNodes; }
    /// <summary>선택할 실제 장치가 없거나 이미 반납한 token이면 그 위치에서 실패한다.</summary>
    private SelectedBinding Select(uint token, Func<Binding, bool>? available = null)
    {
        if (!_bindings.TryGetValue(token, out Binding? value) || available is not null && !available(value)) throw new InvalidOperationException("현재 연결의 선택된 실제 노드/장치 미연결");
        return new(value);
    }
    /// <summary>현재 같은Source 제어 블록을 동기 부모가 한 번만 보유한다.</summary>
    private void RetainForCall(ConnectionDevicePin? pin)
    {
        if (pin is null) return;
        // 별칭 포인터가 달라도 같은 실제 제어 블록이면 추가 대여하지 않는다.
        if (_deviceCallOwners is { } heldOwners)
        {
            // 이미 보유한 제어 블록만 비교한다.
            foreach (ConnectionDevicePin held in heldOwners) if (ReferenceEquals(held.Identity, pin.Identity)) return;
        }
        ConnectionDevicePin borrowed = pin.Fork();
        try { (_deviceCallOwners ??= []).Add(borrowed); } catch { borrowed.Dispose(); throw; }
    }
    /// <summary>이미 선택한 노드의 현재 Source와 등록Source를 실제 부모의 마지막 쓰기까지 보유한다.</summary>
    private ConnectionDevicePin? PinBinding(SelectedBinding selected)
    {
        Func<PacketConnection, ConnectionDevicePin?>? select = _devicePin;
        if (select is null && selected.Owner is null) return null;
        ConnectionDevicePin owner = select is null ? selected.Owner!.Fork() : select(selected.Value.Node) ?? throw new InvalidOperationException("현재 연결의 실제Source 수명 미연결");
        try { if (_deviceCallDepth != 0) { RetainForCall(selected.Owner); RetainForCall(owner); } return owner; }
        catch { owner.Dispose(); throw; }
    }
    /// <summary>목록을 먼저 분리해Source 최종 반납의 재진입이 옛 목록을 다시 읽지 않게 한다.</summary>
    private void ReleaseDevicePins()
    {
        List<ConnectionDevicePin>? selected = _deviceCallOwners; _deviceCallOwners = null; if (selected is null) return; List<Exception>? failures = null;
        // 모든 실제 대여를 반납하고 장치 예외도 숨기지 않는다.
        for (int index = selected.Count - 1; index >= 0; index--) { try { selected[index].Dispose(); } catch (Exception error) { (failures ??= []).Add(error); } }
        if (failures?.Count == 1) ExceptionDispatchInfo.Capture(failures[0]).Throw();
        if (failures?.Count > 1) throw new AggregateException("실제 전송Source 최종 반납 실패", failures);
    }
    /// <summary>정상/예외 모두 깊이를 먼저 반납하고 최외곽 부모의 마지막 쓰기 뒤Source를 반납한다.</summary>
    private void RunDeviceScope(Action operation)
    {
        bool outer = _deviceCallDepth == 0; _deviceCallDepth++; Exception? bodyFailure = null;
        try { operation(); } catch (Exception error) { bodyFailure = error; throw; }
        finally
        {
            _deviceCallDepth--;
            if (outer)
            {
                try { ReleaseDevicePins(); }
                catch (Exception releaseFailure) when (bodyFailure is not null) { throw new AggregateException("원문부모와실제Source최종반납실패", bodyFailure, releaseFailure); }
            }
        }
    }
    /// <summary>원문 정리 부모가 자신의 슬롯/count 쓰기를 마칠 때까지 일반 별칭 제거를 유예한다.</summary>
    private void RunCleanup(Action operation) => RunDeviceScope(() => { _cleanupDepth++; try { operation(); } finally { _cleanupDepth--; } });
    /// <summary>같은 실제 노드/close/free와 선택적 가상 장치를 묶고 성공한Binding만 공개한다.</summary>
    public uint Track(PacketConnection connection, Action destroy, Action release, Action<Memory<byte>>? send = null, Func<int>? readiness = null, Action? buffered = null, Action? flush = null, Func<TransportPollResult>? receive = null)
    {
        ArgumentNullException.ThrowIfNull(connection); ArgumentNullException.ThrowIfNull(destroy); ArgumentNullException.ThrowIfNull(release);
        if (_deviceShutdown) throw new InvalidOperationException("최종 반납 뒤 새 연결 발급");
        // 동일 노드의 현재 살아 있는 주소를 중복 등록하지 않는다.
        foreach (Binding prior in _bindings.Values) if (ReferenceEquals(prior.Node, connection)) throw new InvalidOperationException("현재 연결의 중복 주소 등록");
        if (_nextToken == 0) throw new OverflowException("연결opaque token 소진");
        uint token = _nextToken; _nextToken = unchecked(token + 1);
        var value = new Binding { Node = connection, Destroy = destroy, Release = release, Send = send, Readiness = readiness, Buffered = buffered, Flush = flush, Receive = receive };
        try
        {
            if (_retainDeviceOwners) { using var selected = new SelectedBinding(value); value.DeviceOwner = PinBinding(selected); }
            _bindings.Add(token, value); return token;
        }
        catch { value.DeviceOwner?.Dispose(); throw; }
    }
    /// <summary>반납한 노드를 다시 읽지 않는 현재 token 존재 조회다.</summary>
    public bool Owns(uint token) => _bindings.ContainsKey(token);
    /// <summary>현재 같은 생존 노드만 빌린다.</summary>
    public PacketConnection Node(uint token) => _bindings.TryGetValue(token, out Binding? value) ? value.Node : throw new ArgumentOutOfRangeException(nameof(token), "현재 연결token 물리 누락");
    /// <summary>sender 번호 대신 현재 같은 실제 노드 주소의 token을 찾는다.</summary>
    public uint TokenFor(PacketConnection connection)
    {
        // 발급 순서의 현재 살아 있는 노드만 비교한다.
        foreach (var entry in _bindings) if (ReferenceEquals(entry.Value.Node, connection)) return entry.Key;
        throw new ArgumentOutOfRangeException(nameof(connection), "직접 연결의 현재token 누락");
    }
    /// <summary>현재 고정표의ID만 선택한다.-32의 논리 관문과32칸 물리 상한은 구별한다.</summary>
    private uint CurrentToken(int id) => id < 0 ? _pending[checked((int)-(long)id)] : _transceivers[id];
    /// <summary>현재 노드의20바이트 주소 또는 원문zero IPv4 주소를 반환한다.</summary>
    public byte[] Address(int id)
    {
        if (id < -32 || id > 21) throw new ArgumentOutOfRangeException(nameof(id)); uint token = CurrentToken(id);
        if (token != 0) return (byte[])Node(token).Address.Clone(); byte[] address = new byte[20]; address[0] = 2; return address;
    }
    /// <summary>Player 조건보다 먼저 현재 슬롯을 읽고 가상 송신 뒤 같은 노드를 재조회하지 않는다.</summary>
    public bool SendMutable(int id, Memory<byte> packet, ReadOnlyMemory<byte> players)
    {
        if (id < -32 || id > 21) throw new ArgumentOutOfRangeException(nameof(id)); return SendSelected(CurrentToken(id), id, packet, players);
    }
    /// <summary>복귀형 원문1e 진단 뒤 현재 물리 슬롯을 읽는다.</summary>
    public bool SendMutableNative(int id, Memory<byte> packet, ReadOnlyMemory<byte> players, ConnectionRegistryAssertion? assertion)
    {
        if (id < -32 || id > 21) AssertNative(assertion, 0x1e); return SendSelected(CurrentToken(id), id, packet, players);
    }
    /// <summary>이미 읽은token과 현재Player flag1을 검사하고 선택된 실제 장치만 부른다.</summary>
    private bool SendSelected(uint token, int id, Memory<byte> packet, ReadOnlyMemory<byte> players)
    {
        if (id is >= 1 and <= 8 && (players.Span[id * 180 + 0x50] & 1) == 0) return false;
        if (token == 0) return false; SendDirect(token, packet); return true;
    }
    /// <summary>같은 현재 실제 socket 장치의 준비값을 직접 반환한다.</summary>
    public int SocketReadiness(uint token) { using var selected = Select(token, static value => value.Readiness is not null); using var owner = PinBinding(selected); return selected.Value.Readiness!(); }
    /// <summary>이미 고른this의가상+4이며Player/번호 관문을 반복하지 않는다.</summary>
    public void SendDirect(uint token, Memory<byte> packet) { using var selected = Select(token, static value => value.Send is not null); using var owner = PinBinding(selected); selected.Value.Send!(packet); }
    /// <summary>전역 표의 이전Source 노드도 같은 실제가상+0c를 호출한다.</summary>
    public void FlushDirect(uint token) { using var selected = Select(token, static value => value.Flush is not null); using var owner = PinBinding(selected); selected.Value.Flush!(); }
    /// <summary>이전Source의실제종료통지도 소비하며 하위결과만 반환한다.</summary>
    public TransportPollResult ReceiveDirect(uint token) { using var selected = Select(token, static value => value.Receive is not null); using var owner = PinBinding(selected); return selected.Value.Receive!(); }
    /// <summary>현재 실제 노드에buffered 가상 효과를 실행한다.</summary>
    public void SetBufferedNative(uint token) { using var selected = Select(token, static value => value.Buffered is not null); using var owner = PinBinding(selected); selected.Value.Buffered!(); }
    /// <summary>현재 실제close만 호출하며 표 제거는free의 성공 뒤 수행한다.</summary>
    public void Destroy(uint token) { using var selected = Select(token); using var owner = PinBinding(selected); selected.Value.Destroy(); }
    /// <summary>같은token의현재별칭만 지우고 새세대의칸은 보존한다.</summary>
    private void RemoveAliases(uint token)
    {
        bool main = false;
        // 현재 양수 칸에서같은token만지운다.
        for (int index = 0; index < _transceivers.Length; index++) if (_transceivers[index] == token) { _transceivers[index] = 0; main = true; }
        if (main && _synchronization.Synchronization == token) _synchronization.Synchronization = 0;
        // pending0은 일반 별칭제거대상이아니며실제지운개수만unsigned감산한다.
        for (int index = 1; index < _pending.Length; index++) if (_pending[index] == token) { _pending[index] = 0; _pendingCount = unchecked(_pendingCount - 1); }
    }
    /// <summary>등록을먼저분리하고실제Source반납의재진입에도같은Binding을다시반납하지않는다.</summary>
    private void DropBinding(uint token)
    {
        if (!_bindings.Remove(token, out Binding? value)) return; ConnectionDevicePin? owner = value.DeviceOwner; value.DeviceOwner = null; owner?.Dispose();
    }
    /// <summary>복제버퍼또는실제free성공뒤장치를반납한다.Native부모는자기store를직접마친다.</summary>
    public void Release(uint token)
    {
        if (_connectPackets.Remove(token)) return;
        using var selected = Select(token); using var owner = PinBinding(selected); selected.Value.Release();
        if (_cleanupDepth == 0) RemoveAliases(token); DropBinding(token);
    }
    /// <summary>호스트Source최종소멸에서남은자기별칭/장치만분리한다.Native전체종료대체가아니다.</summary>
    public void Forget(uint token) { if (token == 0) return; RemoveAliases(token); DropBinding(token); }
    /// <summary>성공한접속뒤기존버퍼free/0→observer1/현재길이/실제복제→현재칸공개순서다.</summary>
    public void StoreConnectPacket(int id, Memory<byte>? packet, RawPacketDispatch dispatch)
    {
        if (id < 0 || id >= _buffers.Length) throw new ArgumentOutOfRangeException(nameof(id));
        if (_buffers[id] != 0) { Release(_buffers[id]); _buffers[id] = 0; }
        RawPacketCopyResult copy = RawPacketCopy.Duplicate(packet, 0, null, dispatch); if (!copy.Present) return;
        if (_nextToken == 0) throw new OverflowException("접속초기패킷opaque token 소진"); uint token = _nextToken; _nextToken = unchecked(token + 1);
        _connectPackets.Add(token, copy.Allocation ?? throw new InvalidOperationException("접속초기실제복제물리누락")); _buffers[id] = token;
    }
    /// <summary>현재버퍼→현재노드가상송신→현재버퍼재조회free→현재칸0의원문순서다.</summary>
    public void SendConnectPacket(int id)
    {
        if (id < 0) return; if (id >= _buffers.Length) throw new ArgumentOutOfRangeException(nameof(id)); if (_buffers[id] == 0) return;
        if (!_connectPackets.TryGetValue(_buffers[id], out byte[]? packet)) throw new InvalidOperationException("접속통지의현재복제버퍼물리미연결");
        SendDirect(_transceivers[id], packet); if (_buffers[id] != 0) Release(_buffers[id]); _buffers[id] = 0;
    }
    /// <summary>기존bootstrap1수신기/버전/정책/observer는덮지않고빈수신기만설치한다.</summary>
    public static void RegisterConnectPacketReceiver(RawPacketDispatch dispatch, RawConnectionRegistry registry)
    {
        ArgumentNullException.ThrowIfNull(registry);
        if (dispatch.Registration(1).Receive is null) dispatch.RegisterCanonicalDirect(1, 0x4e2f00, (sender, _, _) => registry.SendConnectPacket(unchecked((int)sender)));
    }
    /// <summary>같은노드/현재함수와선택된복귀진단만원문등록부모에제공한다.</summary>
    private ConnectionRegistrationHooks RegistrationHooks(Action<uint>? buffered = null, ConnectionRegistryAssertion? assertion = null)
        => new() { Node = Node, Buffered = buffered is null ? null : token => { using var selected = Select(token); using var owner = PinBinding(selected); buffered(token); },
            Assertion = line => { if (assertion is null) throw new InvalidOperationException($"원문연결등록진단:{line}"); AssertNative(assertion, line); } };
    /// <summary>같은현재노드/close/free와선택된복귀진단만정리부모에제공한다.</summary>
    private ConnectionCleanupHooks CleanupHooks(ConnectionRegistryAssertion? assertion = null) => new()
    {
        Node = Node, Destroy = Destroy, Release = Release,
        Assertion = line => { if (assertion is null) throw new InvalidOperationException($"원문연결정리진단:{line}"); AssertNative(assertion, line); },
    };
    /// <summary>실제도달한원문표현/파일/행만진단하고정상복귀후기존부모를계속한다.</summary>
    private static void AssertNative(ConnectionRegistryAssertion? assertion, uint line)
    {
        string expression = line switch { 0x1e => "-zaMAX_PENDING <= id && id < NUM_TRANSCEIVERS", 0xb0 => "FIRST_REAL_PLAYER <= transId && transId < END_REAL_PLAYERS", 0xb3 => "transId == GAME_SERVER_ID", 0xb6 => "transceivers[transId] == NULL", 0xbb => "pendingId < 0", 0xfa => "id < NUM_TRANSCEIVERS", _ => throw new ArgumentOutOfRangeException(nameof(line)) };
        (assertion ?? throw new InvalidOperationException("Native연결의선택된실제진단미연결"))(expression, "\\Ns\\O\\Ztrans.cpp", line);
    }
    /// <summary>004e2870의현재슬롯/sender를같은실제노드에등록한다.</summary>
    public void Register(int id, uint token) { _ = Node(token); ConnectionRegistration.RegisterTransceiver(id, token, Cleanup(), RegistrationHooks()); }
    /// <summary>접속생성뒤기존칸을진단/종료하지않고직접공개한다.sender는호출자가이미썼다.</summary>
    public void PublishConnect(int id, uint token)
    {
        _ = Node(token); if (id < 0 || id >= _transceivers.Length) throw new ArgumentOutOfRangeException(nameof(id)); _transceivers[id] = token;
    }
    /// <summary>원문jge/jl의signed현재개수31관문이다.</summary>
    public bool CanAccept() => unchecked((int)_pendingCount) < 31;
    /// <summary>현재첫빈pending1..31에같은노드를등록하고unsigned개수를증가한다.</summary>
    public int Accept(uint token)
    {
        PacketConnection node = Node(token); if (!CanAccept()) throw new InvalidOperationException("현재pending개수31관문");
        // 현재 첫 빈칸만 선택하며 나머지 칸을 정규화하지 않는다.
        for (int index = 1; index < _pending.Length; index++) if (_pending[index] == 0) { _pending[index] = token; node.Sender = unchecked((uint)-index); _pendingCount = unchecked(_pendingCount + 1); return -index; }
        throw new InvalidOperationException("pending개수와현재31칸의불일치");
    }
    /// <summary>같은실제노드를두별칭으로유지한채buffered뒤pending/count를반납한다.</summary>
    public void Assign(int pending, int target, uint server, Action<uint>? buffered) => RunDeviceScope(() => ConnectionRegistration.AssignPendingConnection(pending, target, server, Cleanup(), RegistrationHooks(buffered)));
    /// <summary>b0/b3/b6/bb복귀진단뒤현재표를재조회하는같은Native부모다.</summary>
    public void AssignNative(int pending, int target, uint server, Action<uint>? buffered, ConnectionRegistryAssertion assertion)
        => RunDeviceScope(() => ConnectionRegistration.AssignPendingConnection(pending, target, server, Cleanup(), RegistrationHooks(buffered, assertion)));
    /// <summary>공통buffered위치에서현재실제노드의장치를선택하는Client입구다.</summary>
    public void AssignNative(int pending, int target, uint server, ConnectionRegistryAssertion assertion) => AssignNative(pending, target, server, SetBufferedNative, assertion);
    /// <summary>004e2940의현재개별종료를같은실제표와장치에실행한다.</summary>
    public void Disconnect(int id) => RunCleanup(() => ConnectionCleanup.DisconnectTransceiver(id, Cleanup(), CleanupHooks()));
    /// <summary>fa진단복귀뒤현재슬롯/버퍼/동기화를정리한다.</summary>
    public void DisconnectNative(int id, ConnectionRegistryAssertion assertion) => RunCleanup(() => ConnectionCleanup.DisconnectTransceiver(id, Cleanup(), CleanupHooks(assertion)));
    /// <summary>양수22칸만원문순서로종료한다.초기버퍼/pending전체정리를더하지않는다.</summary>
    public void CloseAll() => RunCleanup(() => ConnectionCleanup.CloseAllTransceivers(Cleanup(), CleanupHooks()));
    /// <summary>현재server관문뒤owner1..8만종료한다.</summary>
    public void EndPlayers(uint server) => RunCleanup(() => ConnectionCleanup.EndPlayerConnections(server, Cleanup(), CleanupHooks()));
    /// <summary>pending32칸을종료하며원문처럼개수를재계산하지않는다.</summary>
    public void ClosePending() => RunCleanup(() => ConnectionCleanup.CloseAllPendingConnections(Cleanup(), CleanupHooks()));
    /// <summary>모든동기배송이끝난앱최종종료에서만잔류노드/초기복제를명시반납한다.</summary>
    public void ShutdownTrackedDevices()
    {
        if (_deviceCallDepth != 0 || _cleanupDepth != 0) throw new InvalidOperationException("진행중Native부모안의앱최종종료");
        // 깊이를0으로고치거나현재배송을강제로반납하지않는다.
        foreach (Binding value in _bindings.Values) if (value.Node.Depth != 0) throw new InvalidOperationException("진행중노드배송안의앱최종종료");
        _deviceShutdown = true;
        RunDeviceScope(() =>
        {
            CloseAll(); ClosePending();
            // 전역칸에서덮인잔류노드만발급순서로원래close/free한다.
            while (_bindings.Count != 0) { uint token = _bindings.First().Key; ConnectionCleanup.DeferConnectionClose(Node(token), () => Destroy(token), () => Release(token)); }
            // Native부모쓰기뒤남은실제초기패킷만반납한다.
            for (int index = 0; index < _buffers.Length; index++) if (_buffers[index] != 0) { Release(_buffers[index]); _buffers[index] = 0; }
        });
    }
}

/// <summary>원문현재전송ID순회의작은조회경계다.전체Poll본체는실제전송owner가소유한다.</summary>
public static class RawTransportPolling
{
    /// <summary>-31..-1pending과0..21정상칸의같은현재실제token을읽는다.</summary>
    public static uint CurrentTransportToken(RawConnectionRegistry registry, int id)
    {
        ConnectionCleanupGlobals table = registry.Cleanup(); if (id <= -table.Pending.Length || id >= table.Transceivers.Length) throw new ArgumentOutOfRangeException(nameof(id));
        return id < 0 ? table.Pending[-id] : table.Transceivers[id];
    }
}
