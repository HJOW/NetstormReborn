using System.Buffers.Binary;
using Netstorm.Core.Original;

// 같은 고정 양끝과 실제 현재 Registry/패킷표를 원문 로컬 전송 순서로 소비한다.
namespace Netstorm.Net;

/// <summary>LocalMissionTransport.cpp의 실제 내부양끝·buffered 송신·현재ID 수신 본체다.</summary>
public sealed partial class LocalMissionTransport
{
    /// <summary>초기 제공된 실제Player9 기록이며 현재base getter가 없을 때만 빌린다.</summary>
    private readonly ReadOnlyMemory<byte> _players;
    /// <summary>현재 실제 서버 전역을 조회하며 성공flag를 만들지 않는다.</summary>
    private readonly Func<bool> _serverMode;
    /// <summary>같은 실제World의 현재장치 값 사본이다.</summary>
    private PacketDispatchHooks _hooks;
    /// <summary>로그인/건설/연구/종료가 공유하는 실제 현재표다.</summary>
    public RawConnectionRegistry Registry { get; }
    /// <summary>같은 실제observer/receiver표를 쓰는 원문 분배기다.</summary>
    public RawPacketDispatch Dispatch { get; }
    /// <summary>Open 때 객체주소를 바꾸지 않는 실제Client9 노드다.</summary>
    public PacketConnection Client { get; } = new();
    /// <summary>Poll이수락한 같은 실제pending/owner 노드다.</summary>
    public PacketConnection Server { get; } = new();
    /// <summary>Client가아직flush하지않은실제buffered 바이트다.</summary>
    private readonly List<byte> _clientOutput = [];
    /// <summary>서버가아직flush하지않은실제buffered 바이트다.</summary>
    private readonly List<byte> _serverOutput = [];
    /// <summary>서버끝이실제로꺼낼완성프레임의소유큐다.</summary>
    private readonly Queue<byte[]> _toServer = new();
    /// <summary>Client끝이실제로꺼낼완성프레임의소유큐다.</summary>
    private readonly Queue<byte[]> _toClient = new();
    /// <summary>두끝이마지막으로받은실제version이며최초ff다.</summary>
    private readonly byte[] _receiveVersion = [0xff, 0xff];
    /// <summary>내부listener의현재열림이다.</summary>
    private bool _open;
    /// <summary>수락된서버끝이현재pending인지다.</summary>
    private bool _pending;
    /// <summary>이번열린수명에서실제서버끝을수락했는지다.</summary>
    private bool _accepted;
    /// <summary>현재per-ID Poll 대여이며중첩을거부한다.</summary>
    private bool _polling;
    /// <summary>서버끝의실제buffered 가상장치상태다.</summary>
    private bool _serverBuffered;
    /// <summary>실제Client9 물리가아직존재하는지다.</summary>
    private bool _clientPresent;
    /// <summary>실제수락된서버물리가아직존재하는지다.</summary>
    private bool _serverPresent;
    /// <summary>Client의반대socket이소멸했는지다.</summary>
    private bool _clientPeerLost;
    /// <summary>서버의반대socket이소멸했는지다.</summary>
    private bool _serverPeerLost;
    /// <summary>현재실제Client9의발급token이다.</summary>
    private uint _clientToken;
    /// <summary>현재실제수락끝의발급token이다.</summary>
    private uint _serverToken;
    /// <summary>공유표가선택한실제첫빈pending번호다.</summary>
    private int _pendingId;
    /// <summary>재접속관문이읽는이번할당owner다.공개Owner는현재표를조회한다.</summary>
    private uint _owner;
    /// <summary>현재실제열린내부포트다.</summary>
    private uint _port;
    /// <summary>Source최종반납을이미마쳤는지다.</summary>
    private bool _disposed;
    /// <summary>같은Player9/현재서버·필수실제장치와공유표를빌리며접속은수행하지않는다.</summary>
    public LocalMissionTransport(ReadOnlyMemory<byte> players, Func<bool> server, PacketDispatchHooks hooks, RawConnectionRegistry? registry = null, PacketRegistrationTable? registrations = null)
    {
        ArgumentNullException.ThrowIfNull(server); ValidateHooks(hooks);
        if (players.Length != 1620) throw new ArgumentException("로컬 전송의 실제Player9 누락", nameof(players));
        _players = players; _serverMode = server; _hooks = hooks.Copy(); Registry = registry ?? new();
        Dispatch = new(OriginalEdition.Patch1078, WrappedHooks(), registrations);
        RawConnectionRegistry.RegisterConnectPacketReceiver(Dispatch, Registry);
        // 원문40b380의실제빈관찰기이며미구현성공대용이아니다.공유표의기본설정을보존한다.
        Dispatch.Observe(0x87, static _ => { }); Dispatch.Version(0x87, 1, 0); Dispatch.RequireLength(0x87, 0x112);
    }
    /// <summary>감싼함수의존재로원래필수장치누락을가리지않는다.</summary>
    private static void ValidateHooks(PacketDispatchHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.Now is null || hooks.Record is null || hooks.Terminate is null || hooks.Close is null || hooks.Release is null) throw new ArgumentException("로컬 전송의 실제 분배 장치 누락", nameof(hooks));
    }
    /// <summary>같은현재노드/큐/깊이/등록표를보존하고실제World장치만바꾼다.</summary>
    public void RebindHooks(PacketDispatchHooks hooks) { ValidateHooks(hooks); _hooks = hooks.Copy(); }
    /// <summary>다른backend의노드는해당실제장치에맡기고자기노드만같은Registry로반납한다.</summary>
    private PacketDispatchHooks WrappedHooks() => new()
    {
        Now = () => _hooks.Now!(), Record = (node, packet, length) => _hooks.Record!(node, packet, length), Terminate = node => _hooks.Terminate!(node),
        Close = node => EndNode(node, false), Release = node => EndNode(node, true),
    };
    /// <summary>이미실제반납한같은끝만중복생략하며살아있는다른세대는진단한다.</summary>
    private void EndNode(PacketConnection node, bool release)
    {
        if (!OwnsConnection(node)) { if (release) _hooks.Release!(node); else _hooks.Close!(node); return; }
        uint token = Token(node); bool present = ReferenceEquals(node, Client) ? _clientPresent : _serverPresent;
        if (!Registry.Owns(token)) { if (present) throw new InvalidOperationException("살아있는내부끝의현재token누락"); return; }
        if (!present || !ReferenceEquals(Registry.Node(token), node)) throw new InvalidOperationException("내부끝의다른세대/주소");
        if (release) Registry.Release(token); else Registry.Destroy(token);
    }
    /// <summary>반납중에도고정물리주소가같은자기끝인지만판정한다.</summary>
    public bool OwnsConnection(PacketConnection node) => ReferenceEquals(node, Client) || ReferenceEquals(node, Server);
    /// <summary>이미선택한같은두물리주소의현재token이다.</summary>
    private uint Token(PacketConnection node) => ReferenceEquals(node, Client) ? _clientToken : ReferenceEquals(node, Server) ? _serverToken : throw new ArgumentException("로컬전송의다른노드주소", nameof(node));
    /// <summary>실제같은끝의close/free·가상송신/readiness/buffered/flush/수신을등록한다.</summary>
    private uint Track(PacketConnection node) => Registry.Track(node,
        () => { _hooks.Close!(node); if (ReferenceEquals(node, Client)) { _serverPeerLost = true; _clientOutput.Clear(); } else { _clientPeerLost = true; _serverOutput.Clear(); } _toServer.Clear(); _toClient.Clear(); },
        () => { bool client = ReferenceEquals(node, Client); _hooks.Release!(node); if (client) _clientPresent = false; else { _serverPresent = false; _pending = false; _owner = 0; } },
        packet => { _ = SendPacket(unchecked((int)node.Sender), packet, packet, node); },
        () => (ReferenceEquals(node, Client) ? _clientPresent && !_clientPeerLost : _serverPresent && !_serverPeerLost) ? 1 : 0,
        () => { node.Flags |= 2; if (ReferenceEquals(node, Server)) _serverBuffered = true; (ReferenceEquals(node, Client) ? _clientOutput : _serverOutput).Clear(); },
        () => { if ((node.Flags & 0x40) == 0) FlushEndpoint(ReferenceEquals(node, Server)); }, () => ReceiveNode(node));
    /// <summary>새수명의동일고정노드를원문0으로초기화한다.실제표의별칭은여기서쓰지않는다.</summary>
    private static void Reset(PacketConnection node)
    {
        node.Flags = node.Sender = node.Received = node.Heartbeat = node.Depth = 0; node.Delayed = false; node.DelayQueue = null; Array.Clear(node.Address);
    }
    /// <summary>실제27h로그인수신이없으면열지않는다.</summary>
    public void RegisterLogin(Action<uint, PacketConnection, ReadOnlyMemory<byte>> receive)
    {
        ArgumentNullException.ThrowIfNull(receive); Dispatch.Version(0x27, 1, 0); Dispatch.Register(0x27, receive);
    }
    /// <summary>현재내부listener만열고요청한경우Client9만생성한다.서버끝은Poll이수락한다.</summary>
    public void Open(string host, uint port, bool connectImmediately = true)
    {
        ObjectDisposedException.ThrowIf(_disposed, this); LocalNetworkPolicy.RequireLoopback(host);
        if (_open || _clientPresent || _serverPresent || _polling || Dispatch.Registrations.ConnectionCallbackActive() || port == 0 || port > 65535 || Dispatch.Registration(0x27).Receive is null) throw new InvalidOperationException("로컬포트/새수명/실제로그인처리기계약오류");
        _port = port; Reset(Client); Reset(Server); Client.Sender = 9; _owner = 0;
        _pending = _accepted = _serverBuffered = _clientPresent = _serverPresent = _clientPeerLost = _serverPeerLost = false; _clientToken = _serverToken = 0; _receiveVersion[0] = _receiveVersion[1] = 0xff; _open = true;
        Client.Address[0] = 20; Client.Address[2] = (byte)(port >> 8); Client.Address[3] = (byte)port; BinaryPrimitives.WriteUInt32LittleEndian(Client.Address.AsSpan(16), 0x10d); Client.Address.CopyTo(Server.Address, 0);
        if (connectImmediately) Connect();
    }
    /// <summary>Client9만실제로만들며서버pending을선생성하거나성공Playerflag를쓰지않는다.</summary>
    public void Connect()
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        if (!_open || _clientPresent || Client.Flags != 0 || _owner != 0 || _accepted || Registry.Cleanup().Transceivers[9] != 0) throw new InvalidOperationException("로컬Client새접속/현재서버계약오류");
        Client.Received = _hooks.Now!(); Client.Heartbeat = _hooks.Now!(); Client.Flags = 0x26; _clientPresent = true; _clientToken = Track(Client); Registry.Register(9, _clientToken);
    }
    /// <summary>현재실제Poll안에서만연결된Client의서버끝을첫빈pending으로수락한다.</summary>
    public void AcceptPending()
    {
        if (!_polling) throw new InvalidOperationException("내부accept의현재Poll대여누락");
        if (!_open || !_clientPresent || _accepted || _clientPeerLost || !Registry.CanAccept()) return;
        Server.Sender = 0; Server.Flags = 0x24; Server.Received = _hooks.Now!(); Server.Heartbeat = _hooks.Now!(); _serverPresent = true; _serverToken = Track(Server); _pendingId = Registry.Accept(_serverToken); _pending = _accepted = true;
    }
    /// <summary>현재같은서버pending을owner1..8로옮기고같은실제buffered장치를설정한다.</summary>
    public void AssignConnection(int pending, int owner)
    {
        if (!_serverMode() || !_open || !_pending || pending != _pendingId || owner < 1 || owner >= 9 || Owner() != 0 || !ReferenceEquals(Connection(pending), Server)) throw new InvalidOperationException("로컬실제pending재배치계약오류");
        Registry.Assign(pending, owner, _serverMode() ? 1u : 0, token => { if (token != _serverToken) throw new InvalidOperationException("로컬재배치의다른buffered연결"); Server.Flags |= 2; _serverBuffered = true; _serverOutput.Clear(); }); _owner = (uint)owner; _pending = false;
    }
    /// <summary>현재연결/Player관문뒤이미작성된같은실제버퍼의선언바이트를송신한다.</summary>
    public bool Send(int destination, ReadOnlyMemory<byte> packet) => SendPacket(destination, packet, null);
    /// <summary>현재실제연결이선택된뒤observer1을호출해현재WORD로송신한다.</summary>
    public bool SendMutable(int destination, Memory<byte> packet) => SendPacket(destination, packet, packet);
    /// <summary>현재같은물리this와수명관문을보존하고번호송신만Playerflag1을검사한다.</summary>
    private bool SendPacket(int destination, ReadOnlyMemory<byte> packet, Memory<byte>? writable, PacketConnection? direct = null)
    {
        ObjectDisposedException.ThrowIf(_disposed, this); if (!_open) throw new InvalidOperationException("로컬전송을열기전송신");
        if (packet.Length >= 4 && packet.Span[1] is 0x7f or 0x80) return false;
        PacketConnection? selected = direct ?? (destination > -32 && destination < 22 ? Connection(destination) : null); bool client = ReferenceEquals(selected, Client);
        if (client && (!_clientPresent || (Client.Flags & 2) == 0 || (Client.Flags & 0x40) != 0 || _clientPeerLost)) return false;
        if (direct is null && destination is >= 1 and < 9 && !RawPacketDispatch.PlayerActive(_players, _hooks.Players, (uint)destination)) return false;
        bool server = _serverPresent && !_serverPeerLost && (Server.Flags & 0x40) == 0 && ReferenceEquals(selected, Server);
        if (!client && !server) return false; if (packet.Length < 4) throw new ArgumentOutOfRangeException(nameof(packet), "로컬송신헤더부족");
        PacketRegistration registration = Dispatch.Registration(packet.Span[1]).Snapshot(); using IDisposable? pin = registration.Pin();
        if (writable is { Length: > 0 } mutable) registration.NotifyObserver(mutable, 1);
        if (direct is null && Connection(destination) is null || (client ? !_clientPresent || (Client.Flags & 0x40) != 0 || _clientPeerLost : !_serverPresent || (Server.Flags & 0x40) != 0 || _serverPeerLost)) return false;
        int length = RawPacketDispatch.Length(packet.Span); if (length < 4 || length > packet.Length) throw new ArgumentOutOfRangeException(nameof(packet), "로컬선언바이트물리부족");
        if (!client && !_serverBuffered) _toClient.Enqueue(packet.Slice(0, length).ToArray());
        else
        {
            List<byte> output = client ? _clientOutput : _serverOutput; if (output.Count + length > 65531) throw new ArgumentOutOfRangeException(nameof(packet), "로컬buffered65535바이트상한");
            ReadOnlySpan<byte> bytes = packet.Span[..length];
            // 같은현재선언바이트만해당실제buffered출력에추가한다.
            for (int index = 0; index < bytes.Length; index++) output.Add(bytes[index]);
        }
        _hooks.Sent?.Invoke(client ? Client : Server, packet, RawPacketDispatch.Length(packet.Span)); return true;
    }
    /// <summary>원문ff첫BYTE를보존하고kind4/전체WORD를확정한뒤상대소유큐로옮긴다.</summary>
    private void FlushEndpoint(bool serverSide)
    {
        List<byte> output = serverSide ? _serverOutput : _clientOutput; if (output.Count == 0) return;
        byte[] frame = new byte[output.Count + 4]; Array.Fill(frame, (byte)0xff); frame[1] = 4; BinaryPrimitives.WriteUInt16LittleEndian(frame.AsSpan(2), (ushort)frame.Length); output.CopyTo(frame, 4); output.Clear(); (serverSide ? _toClient : _toServer).Enqueue(frame);
    }
    /// <summary>현재정상ID의같은자기끝만집합으로flush한다.pending은이순회대상이아니다.</summary>
    public void FlushOne(int id)
    {
        if (id < 0 || id >= 22) throw new ArgumentOutOfRangeException(nameof(id)); PacketConnection? node = Connection(id); if (node is null || (node.Flags & 0x40) != 0) return; FlushEndpoint(ReferenceEquals(node, Server));
    }
    /// <summary>현재0..21칸을순서대로다시읽어실제출력을확정한다.</summary>
    public void Flush()
    {
        if (!_open) throw new InvalidOperationException("로컬전송을열기전flush");
        // 앞장치가다음칸을바꿔도해당위치의현재값을읽는다.
        for (int id = 0; id < 22; id++) FlushOne(id);
    }
    /// <summary>같은실제표와backend장치의단일/집합부모다.</summary>
    public bool DispatchFrame(PacketConnection connection, Memory<byte> frame) => RawPacketFrame.Dispatch(Dispatch, connection, frame, WrappedHooks());
    /// <summary>서버→Client순서로이전닫힘끝만실제다음폴링경계에서반납한다.</summary>
    private void RetireClosed()
    {
        // 같은고정끝의현재존재/깊이/닫힘을각각다시읽는다.
        foreach (bool client in new[] { false, true }) { PacketConnection node = client ? Client : Server; if ((client ? _clientPresent : _serverPresent) && node.Depth == 0 && (node.Flags & 0x40) != 0) { uint token = Token(node); Registry.Destroy(token); Registry.Release(token); } }
    }
    /// <summary>지정끝의실제소유프레임하나를꺼내단일observer0뒤같은부모로분배한다.</summary>
    private TransportPollResult ReceiveEndpoint(bool serverSide)
    {
        Queue<byte[]> queue = serverSide ? _toServer : _toClient; if (queue.Count == 0) return default; byte[] bytes = queue.Dequeue(); if (bytes.Length != 0) _receiveVersion[serverSide ? 1 : 0] = bytes[0];
        if (!Dispatch.Validate(bytes)) return new(true, false); PacketRegistration entry = Dispatch.Registration(bytes[1]).Snapshot(); using IDisposable? pin = entry.Pin();
        if (bytes[1] != 4 && bytes[1] is not (0x7f or 0x80)) entry.NotifyObserver(bytes, 0);
        return new(true, DispatchFrame(serverSide ? Server : Client, bytes));
    }
    /// <summary>현재같은노드만처리하고직접수신이새로빌린Poll guard만반납한다.</summary>
    private TransportPollResult ReceiveNode(PacketConnection node)
    {
        int id = unchecked((int)node.Sender); uint token = Token(node); if (!Registry.Owns(token)) return default; bool entered = !_polling; _polling = true;
        try
        {
            if ((node.Flags & 0x40) != 0) { RetireClosed(); return default; }
            int side = ReferenceEquals(node, Server) ? 1 : 0;
            if ((node.Flags & 0x20) != 0) { node.Flags &= ~0x20u; _ = DispatchFrame(node, new byte[] { _receiveVersion[side], 1, 4, 0 }); }
            if (!ReferenceEquals(Connection(id), node) || (node.Flags & 0x40) != 0) return default;
            if (ReferenceEquals(node, Client) ? _clientPeerLost : _serverPeerLost)
            {
                if ((node.Flags & 0x10) == 0) { node.Flags |= 0x10; _ = DispatchFrame(node, new byte[] { _receiveVersion[side], 2, 4, 0 }); }
                return default;
            }
            return ReceiveEndpoint(ReferenceEquals(node, Server));
        }
        finally { if (entered) _polling = false; }
    }
    /// <summary>per-ID수신의중첩을거부하고이전닫힘반납의예외에도대여를정리한다.</summary>
    public void BeginPoll()
    {
        if (_polling) throw new InvalidOperationException("내부전송Poll중첩"); _polling = true; try { RetireClosed(); } catch { _polling = false; throw; }
    }
    /// <summary>같은현재ID의자기노드한프레임만소비한다.</summary>
    public TransportPollResult ReceiveOne(int id)
    {
        if (!_polling) throw new InvalidOperationException("내부per-ID수신의Poll대여누락"); PacketConnection? node = Connection(id); return node is null ? default : ReceiveNode(node);
    }
    /// <summary>이번per-ID폴링대여만정상/예외복귀에서반납한다.</summary>
    public void EndPoll() => _polling = false;
    /// <summary>실제pending각1회뒤정상0..21현재ID를반복조회하고소비한프레임수만반환한다.</summary>
    public int Poll()
    {
        ObjectDisposedException.ThrowIf(_disposed, this); BeginPoll(); int count = 0;
        try
        {
            AcceptPending();
            // 현재pending을-1부터-31까지각한번방문한다.
            for (int id = -1; id > -32; id--) if (ReceiveOne(id).Consumed) count++;
            // 정상ID는callback재배치뒤에도같은현재칸을다시읽는다.
            for (int id = 0; id < 22; id++) while (true) { TransportPollResult result = ReceiveOne(id); if (result.Consumed) count++; if (!result.Again) break; }
            return count;
        }
        finally { EndPoll(); }
    }
    /// <summary>같은현재token이자기실제끝일때만반환하고다른backend노드는대신하지않는다.</summary>
    public PacketConnection? Connection(int id)
    {
        uint token = RawTransportPolling.CurrentTransportToken(Registry, id);
        if (token != 0 && _clientPresent && token == _clientToken) return Client; if (token != 0 && _serverPresent && token == _serverToken) return Server; return null;
    }
    /// <summary>존재/반대socket/닫힘만보며buffered bit2를연결성공으로쓰지않는다.</summary>
    public bool ConnectionOpen(int id) { PacketConnection? node = Connection(id); return node is not null && (node.Flags & 0x40) == 0 && (ReferenceEquals(node, Client) ? !_clientPeerLost : !_serverPeerLost); }
    /// <summary>내부Connect/Accept의실제즉시완료상태만반환한다.</summary>
    public int ConnectionStatus(int id) => ConnectionOpen(id) ? 1 : 0;
    /// <summary>같은자기두끝만원문실제장치로닫고다른Source의공유표는그대로둔다.</summary>
    public void CloseAll()
    {
        _open = false;
        // Client→서버순서의현재생존token만반납한다.배송깊이가있으면원문닫힘표시만남긴다.
        foreach (PacketConnection node in new[] { Client, Server }) { uint token = Token(node); if (token == 0 || !Registry.Owns(token)) continue; node.Flags |= 0x40; if (node.Depth == 0) { Registry.Destroy(token); Registry.Release(token); } }
    }
    /// <summary>listener열림이며성공Player상태와는독립이다.</summary>
    public bool IsOpen() => _open && !_disposed;
    /// <summary>실제Client9물리가존재하는지다.닫힘flag로존재를지우지않는다.</summary>
    public bool HasClientConnection() => _clientPresent;
    /// <summary>실제수락끝이현재음수등록에남아있는지다.</summary>
    public bool HasPending() => _pending;
    /// <summary>현재서버sender와같은양수칸의실제token으로owner를조회한다.</summary>
    public uint Owner() { uint owner = Server.Sender; return _serverPresent && owner > 0 && owner < 9 && RawTransportPolling.CurrentTransportToken(Registry, (int)owner) == _serverToken ? owner : 0; }
    /// <summary>현재열린실제내부포트다.</summary>
    public uint Port() => _port;
    /// <summary>상대가실제로받을수있게완성된프레임수다.미flush출력은포함하지않는다.</summary>
    public int QueuedFrames() => _toServer.Count + _toClient.Count;
    /// <summary>전투방송도실제현재Player/연결관문을다시읽는송신소비자를빌린다.</summary>
    public Action<ReadOnlyMemory<byte>> Peer(uint owner) => packet => { _ = Send(unchecked((int)owner), packet); };
    /// <summary>Source최종반납에서만남은자기별칭을잊는다.Native방종료/close/free대체가아니다.</summary>
    public void Dispose()
    {
        if (_disposed) return; if (_polling || Client.Depth != 0 || Server.Depth != 0) throw new InvalidOperationException("진행중로컬Source의직접최종반납");
        Registry.Forget(_clientToken); Registry.Forget(_serverToken); _toClient.Clear(); _toServer.Clear(); _clientOutput.Clear(); _serverOutput.Clear(); _disposed = true;
    }
}
