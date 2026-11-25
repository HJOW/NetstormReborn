// AI의 실제 경로 조회/정지·대상/좌표 이동·자기 신전 복귀를 같은 본체에서 처리한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// AI 이동 모듈을 담는 네임스페이스다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.

// 기존 실제 경로/의식/이름과 전송·로그 경계를 빌린다.
public sealed class AiMovementHooks
{
    /// <summary>00488fe0의 현재 TYPE별 실제 경로 TYPE다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    public Func<byte, uint>? Movement { get; set; }

    /// <summary>004b1a90의 같은 종속 조회/삭제다. 1은 getter 인자가 아닌 remove다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    public Func<Sid, uint, uint, bool, Sid>? Lookup { get; set; }

    /// <summary>004ade70(target,rectangle,0)의 현재 실제 발자국이다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    public Func<Sid, SquidFootprint>? Footprint { get; set; }

    /// <summary>004e2dc0(9,18BYTE)의 실제 요청 송신이다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    public Action<int, byte[]>? Send { get; set; }

    /// <summary>00494eb0의 실제 의식 시작이다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    public Action<Sid, Sid>? Sacrifice { get; set; }

    /// <summary>004894e0(actor,target,0,0)의 실제 접근/BFS/Host다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    public Func<Sid, Sid, uint, uint, ulong>? Target { get; set; }

    /// <summary>00489080의 실제 부모/목적지/나머지 인자 0이다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    public Func<RawPathStartRequest, ulong>? Start { get; set; }

    /// <summary>현재 가상 68 이름이다. 디버그 여부 조회 전 원본 순서대로 실행한다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    public Func<Sid, string>? Name { get; set; }

    /// <summary>현재 AI/Move 로그 종류로 Move source to target을 출력한다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    public Action<uint, string, string>? LogTarget { get; set; }

    /// <summary>원본 ftol 후 Move source to (x,y)를 출력한다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    public Action<uint, string, int, int>? LogPoint { get; set; }

    /// <summary>0041d840의 현재 목표 발자국 거리다. 출발 좌표는 효과 전에 저장한다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    public Func<RawPathPoint, Sid, double>? Distance { get; set; }
}

// 사용자 이동의 선택 해제/동일 목적지 관문을 섞지 않고 AI 원문 흐름만 처리한다.
// 근거: cpppj o/RawAiMovement.h/.cpp의 RawAiMovement다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.
public sealed class RawAiMovement
{
    /// <summary>원본 복귀 거리 경계다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    private const float HomeDistance = 2.0F;

    /// <summary>생성자/전략과 같은 실제 본체 저장소다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    private readonly RawAiBrainMemory _memory;

    /// <summary>현재 부모/목표의 실제 원문이다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>00540bc4의 현재값을 원본 위치에서 재조회한다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    private readonly Func<bool>? _boss;

    /// <summary>원본 41d530의 현재 일반 접근점 기하다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    private readonly RawBoardTarget _board;

    /// <summary>사용자 이동과 같은 현재 40h 헤더다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    private readonly CarrierMoveState _packet;

    /// <summary>선택된 실제 경로/의식/이름/로그 효과다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    private readonly AiMovementHooks _hooks;

    /// <summary>같은 PATCH 풀/현재 권한/기하/헤더를 빌린다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    /// <param name="memory">같은 실제 본체 저장소다.</param>
    /// <param name="pool">현재 부모/목표의 실제 풀이다.</param>
    /// <param name="boss">현재 권한을 읽는다.</param>
    /// <param name="board">현재 일반 접근점 기하다.</param>
    /// <param name="packet">현재 40h 헤더다.</param>
    /// <param name="hooks">선택된 실제 하위다.</param>
    public RawAiMovement(RawAiBrainMemory memory, RawSidPool pool, Func<bool>? boss, RawBoardTarget board, CarrierMoveState packet, AiMovementHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(memory);
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(board);
        ArgumentNullException.ThrowIfNull(packet);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078)
        {
            throw new ArgumentException("AI 이동 같은 PATCH 풀 누락");
        }

        _memory = memory;
        _pool = pool;
        _boss = boss;
        _board = board;
        _packet = packet;
        _hooks = hooks;
    }

    /// <summary>0041d840의 축별 signed 거리다. x 차이만 float에 저장하고 unordered에서는 x를 선택한다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    /// <param name="point">잴 점이다.</param>
    /// <param name="bounds">잴 발자국이다.</param>
    /// <returns>발자국까지의 거리다.</returns>
    public static double AiDistanceToFootprint(RawPathPoint point, SquidFootprint bounds)
    {
        float x = point.X < bounds.Left ? (float)((double)bounds.Left - point.X) : (float)((double)point.X - bounds.Right);
        double y = point.Y < bounds.Top ? (double)bounds.Top - point.Y : (double)point.Y - bounds.Bottom;
        return y > (double)x ? y : (double)x;
    }

    /// <summary>비정렬 DWORD를 범위 검사 뒤 낮은 BYTE부터 읽는다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트 범위다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>리틀 엔디언 DWORD다.</returns>
    private static uint Read(ReadOnlySpan<byte> raw, int at)
    {
        if (at < 0 || at > raw.Length || raw.Length - at < 4)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "AI 이동 DWORD 범위");
        }

        // 실제 네 BYTE만 조합하는 반복문이다.
        return BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(at, 4));
    }

    /// <summary>선택된 실제 하위 누락을 가짜 경로 성공/빈 이름으로 대신하지 않는다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    private static T Need<T>(T? function) where T : class => function ?? throw new InvalidOperationException("AI 이동 실제 하위 미연결");

    /// <summary>원본 ftol의 signed64 절삭 하위 DWORD와 무효 입력의 0을 보존한다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    /// <param name="value">절삭할 값이다.</param>
    /// <returns>절삭한 signed 값이다.</returns>
    private static int Ftol(double value)
    {
        if (!double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0)
        {
            return 0;
        }

        return unchecked((int)(long)Math.Truncate(value));
    }

    /// <summary>같은 현재 권한 값을 읽는다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    /// <returns>현재 권한이다.</returns>
    private bool Boss() => Need(_boss)();

    /// <summary>가짜/해제 토큰과 다른 크기의 본체를 거부한다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>현재 살아 있는 실제 0xb08 본체 DWORD다.</returns>
    private uint Field(uint token, int offset)
    {
        byte[] raw = _memory.Bytes(token);
        if (raw.Length != RawAiBrainConstructor.BytesCount)
        {
            throw new ArgumentException("AI 이동 실제0xb08 본체 누락");
        }

        return Read(raw, offset);
    }

    /// <summary>위치 조회에 소유자/생존 필터를 추가하지 않는다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    /// <param name="actor">조회할 SID다.</param>
    /// <returns>현재 raw 두 float 좌표다.</returns>
    private RawPathPoint Position(Sid actor)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(actor.Value);
        return new RawPathPoint(
            BitConverter.UInt32BitsToSingle(Read(raw, 14)),
            BitConverter.UInt32BitsToSingle(Read(raw, 18)));
    }

    /// <summary>잘못된 DWORD를 다른 SID로 절단하지 않는다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    /// <param name="value">바꿀 DWORD 값이다.</param>
    /// <returns>현재 목표 SID다.</returns>
    private Sid PhysicalSid(uint value)
    {
        if (value >= (uint)_pool.Capacity || value > ushort.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "AI 이동 현재 목표 SID 범위");
        }

        return new Sid(unchecked((ushort)value));
    }

    /// <summary>순번 0/remove 0은 경로 시작의 다른 추가 인자가 아니다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    /// <param name="actor">조회할 SID다.</param>
    /// <returns>현재 TYPE의 첫 현재 경로 Form이다.</returns>
    public Sid MovementForm(Sid actor)
    {
        byte type = _pool.Slot(actor.Value)[RawSidPool.TypeOffset];
        return Need(_hooks.Lookup)(actor, Need(_hooks.Movement)(type), 0, false);
    }

    /// <summary>actor 0일 때는 TYPE나 목록에 접근하지 않는다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    /// <param name="actor">정지할 SID다.</param>
    public void Stop(Sid actor)
    {
        if (actor.Value == 0)
        {
            return;
        }

        byte type = _pool.Slot(actor.Value)[RawSidPool.TypeOffset];
        _ = Need(_hooks.Lookup)(actor, Need(_hooks.Movement)(type), 0, true);
    }

    /// <summary>헤더는 실제 송신 직전 현재 공유값이며 사용자 선택/중복 목적지 검사를 추가하지 않는다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    /// <param name="actor">보낼 SID다.</param>
    /// <param name="point">보낼 좌표다.</param>
    /// <param name="sacrifice">보낼 희생 값이다.</param>
    private void Send(Sid actor, RawPathPoint point, uint sacrifice)
    {
        byte[] packet = new byte[18];
        CarrierMoveCommand.Write(_packet.Request, actor, point, sacrifice, packet);
        Need(_hooks.Send)(9, packet);
    }

    /// <summary>처음 서버였던 희생 분기만 즉시의식으로 돌아가며 송신 뒤 권한 변경은 대상 접근 분기를 따른다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="actor">이동할 SID다.</param>
    /// <param name="target">목표 SID다.</param>
    /// <param name="sacrifice">희생 값이다.</param>
    public void ToTarget(uint token, Sid actor, Sid target, uint sacrifice)
    {
        if (!Boss())
        {
            SquidFootprint footprint = Need(_hooks.Footprint)(target);
            RawPathPoint source = Position(actor);
            RawPathPoint point = _board.Nearest(footprint, source, 1);
            Send(actor, point, sacrifice);
        }
        else if (sacrifice != 0)
        {
            Need(_hooks.Sacrifice)(actor, target);
            return;
        }

        if (Boss() && Need(_hooks.Target)(actor, target, 0, 0) == 0)
        {
            return;
        }

        string targetName = Need(_hooks.Name)(target);
        string sourceName = Need(_hooks.Name)(actor);
        if (Field(token, 0xb00) != 0)
        {
            Need(_hooks.LogTarget)(token, sourceName, targetName);
        }
    }

    /// <summary>송신 뒤 현재 권한이 생기면 동일한 인자 0의 실제 경로를 시작한다. 실패하면 이름/로그도 생략한다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="actor">이동할 SID다.</param>
    /// <param name="point">목표 좌표다.</param>
    /// <param name="sacrifice">희생 값이다.</param>
    public void ToPoint(uint token, Sid actor, RawPathPoint point, uint sacrifice)
    {
        if (!Boss())
        {
            Send(actor, point, sacrifice);
        }

        if (Boss() && Need(_hooks.Start)(new RawPathStartRequest(actor.Value, point, 0, 0, 0, 0, 0)) == 0)
        {
            return;
        }

        int y = Ftol(point.Y);
        int x = Ftol(point.X);
        string name = Need(_hooks.Name)(actor);
        if (Field(token, 0xb00) != 0)
        {
            Need(_hooks.LogPoint)(token, name, x, y);
        }
    }

    /// <summary>거리 판정 뒤 현재 신전 SID를 다시 읽어 원본 이동에 전달한다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <param name="actor">이동할 SID다.</param>
    public void Home(uint token, Sid actor)
    {
        uint temple = Field(token, 0x294);
        if (temple == 0)
        {
            return;
        }

        RawPathPoint point = Position(actor);
        double distance = Need(_hooks.Distance)(point, PhysicalSid(temple));
        if (!(distance < HomeDistance))
        {
            ToTarget(token, actor, PhysicalSid(Field(token, 0x294)), 0);
        }
    }

    /// <summary>같은 실제 월드의 풀인지 연결 단계에서 확인한다. 2026-11-25 R4-5b 이동 이식에서 추가됐다.</summary>
    /// <returns>이 몸체가 쓰는 슬롯 풀이다.</returns>
    public RawSidPool Pool => _pool;
}
