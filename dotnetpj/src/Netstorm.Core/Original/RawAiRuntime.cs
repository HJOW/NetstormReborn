// 원본 AI 전체행동·개별프레임·Player1..8 자동방문/삭제 수명을 복원한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// AI 전체 프레임 모듈을 담는 네임스페이스다. 2026-11-19 0.647에서 R4-5 AI 런타임 결선용으로 추가했다.

// 각 원본 조회 위치에서 현재 전역을 읽는다.
public sealed class AiRuntimeGlobals
{
    /// <summary>현재 전투 모드다. 서버/권한과 합치지 않는다. 0.647에서 추가했다.</summary>
    public Func<bool>? Battle { get; set; }

    /// <summary>현재 실행시간 로그 활성 DWORD다. 0.647에서 추가했다.</summary>
    public Func<uint>? Timing { get; set; }

    /// <summary>현재 부족좌표 로그 종류다. 0.647에서 추가했다.</summary>
    public Func<uint>? DryLog { get; set; }
}

// 현재 동일 본체의 실제 행동·외부시계/로그·소유자만 연결한다.
public sealed class AiRuntimeHooks
{
    /// <summary>실제 준비 순서다. 0.647에서 추가했다.</summary>
    public Action<uint>? Initialize { get; set; }

    /// <summary>실제 준비 순서다. 0.647에서 추가했다.</summary>
    public Action<uint>? Prepare { get; set; }

    /// <summary>실제 목표 선택이다. 0.647에서 추가했다.</summary>
    public Action<uint>? SelectGoal { get; set; }

    /// <summary>현재 수집자 행동이다. 0.647에서 추가했다.</summary>
    public Action<uint>? Collectors { get; set; }

    /// <summary>현재 시설 행동이다. 0.647에서 추가했다.</summary>
    public Action<uint>? Factories { get; set; }

    /// <summary>제거 TYPE이며 0은 제거 없음이다. 0.647에서 추가했다.</summary>
    public Func<uint, uint>? DryScan { get; set; }

    /// <summary>현재 fake TYPE 비교 결과와 현재 로그 종류다. 0.647에서 추가했다.</summary>
    public Action<uint, uint, bool>? DryNotice { get; set; }

    /// <summary>현재 29c 전체 DWORD다. 0.647에서 추가했다.</summary>
    public Action<uint, uint>? Priest { get; set; }

    /// <summary>실제 배치 행동이다. 0.647에서 추가했다.</summary>
    public Action<uint>? Drops { get; set; }

    /// <summary>실제 다리 행동이다. 0.647에서 추가했다.</summary>
    public Action<uint>? Bridges { get; set; }

    /// <summary>실제 운반 행동이다. 0.647에서 추가했다.</summary>
    public Action<uint>? Carriers { get; set; }

    /// <summary>실제 경제 행동이다. 0.647에서 추가했다.</summary>
    public Action<uint>? Economy { get; set; }

    /// <summary>현재 owner SP float다. 0.647에서 추가했다.</summary>
    public Func<uint, float>? Money { get; set; }

    /// <summary>매 호출 현재 벽시각이며 게임 시각 snapshot으로 대체하지 않는다. 0.647에서 추가했다.</summary>
    public Func<double>? Wall { get; set; }

    /// <summary>0.1초를 엄격히 넘을 때 전달한다. 0.647에서 추가했다.</summary>
    public Action<uint, double>? Took { get; set; }

    /// <summary>현재 진단이며 반환하면 원본처럼 계속한다. 0.647에서 추가했다.</summary>
    public Action<string, uint>? Assertion { get; set; }

    /// <summary>현재 Player+b0 본문의 실제 소멸이다. 0.647에서 추가했다.</summary>
    public Action<uint>? Destruct { get; set; }

    /// <summary>현재 본문의 실제 반납이다. 0.647에서 추가했다.</summary>
    public Action<uint>? Free { get; set; }
}

/// <summary>00417c70/00417d30/00417de0의 순서와 현재 필드 재조회만 수행한다.
/// 수집자→시설→부족좌표→사제→배치→다리→운반자→경제→float 지갑 검사의 전체 행동 순서다.</summary>
/// <remarks>2026-11-19 0.647에서 추가했다. 근거: cpppj <c>o/RawAiRuntime.h/.cpp</c>의 <c>RawAiRuntime</c>이다.</remarks>
public sealed class RawAiRuntime
{
    /// <summary>원본 Player stride다. 0.647에서 추가했다.</summary>
    private const int PlayerBytes = 180;

    /// <summary>방문하는 Player 수(1~8)다. 0.647에서 추가했다.</summary>
    private const int PlayerCount = 9;

    /// <summary>원본 Player 기록 안 현재 본체 토큰 필드다. 0.647에서 추가했다.</summary>
    private const int BrainField = 0xb0;

    /// <summary>원본 실행시간 기준이다. 0.647에서 추가했다.</summary>
    private const double SlowFrame = 0.1;

    /// <summary>기존 생성자/행동과 같은 실제 소유자다. 0.647에서 추가했다.</summary>
    private readonly RawAiBrainMemory _memory;

    /// <summary>현재 9Player의 유일한 저장소다. 0.647에서 추가했다.</summary>
    private readonly byte[] _players;

    /// <summary>현재 전역의 getter다. 0.647에서 추가했다.</summary>
    private readonly AiRuntimeGlobals _globals;

    /// <summary>실제 행동/장치/수명의 선택된 하위다. 0.647에서 추가했다.</summary>
    private readonly AiRuntimeHooks _hooks;

    /// <summary>AI 전체 프레임 본체를 만든다. 같은 9×180바이트 Player와 실제 본체 저장소를 빌린다.</summary>
    public RawAiRuntime(RawAiBrainMemory memory, byte[] players, AiRuntimeGlobals globals, AiRuntimeHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(memory);
        ArgumentNullException.ThrowIfNull(players);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (players.Length != PlayerBytes * PlayerCount)
        {
            throw new ArgumentException("AI 프레임 실제9Player 누락", nameof(players));
        }

        _memory = memory;
        _players = players;
        _globals = globals;
        _hooks = hooks;
    }

    /// <summary>수집자→시설→부족좌표→사제→배치→다리→운반자→경제→float 지갑 검사다. 0.647에서 추가했다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    public void Actions(uint token)
    {
        Body(token);
        Need(_hooks.Collectors)(token);
        Need(_hooks.Factories)(token);
        uint removed = Need(_hooks.DryScan)(token);
        if (removed != 0)
        {
            Need(_hooks.DryNotice)(token, Need(_globals.DryLog)(), removed == Field(token, 0x280));
        }

        Need(_hooks.Priest)(token, Field(token, 0x29c));
        Need(_hooks.Drops)(token);
        Need(_hooks.Bridges)(token);
        Need(_hooks.Carriers)(token);
        Need(_hooks.Economy)(token);
        // 실제 00417c70은 지갑 정수를 float에 먼저 저장해 현재 SP float와 비교한다.
        float wallet = unchecked((int)Field(token, 0x2d8));
        float money = Need(_hooks.Money)(Field(token, 0x260));
        if (!(wallet == money))
        {
            Need(_hooks.Assertion)("money == getMoney( playerId )", 0x1048);
        }
    }

    /// <summary>254가 비제로일 때만 거짓이며 초기화 미완료는 본문을 유지한다. 0.647에서 추가했다.</summary>
    /// <param name="token">살아 있는 실제 본체 토큰이다.</param>
    /// <returns>계속 유지하면 참, 소멸 대상이면 거짓이다.</returns>
    public bool Tick(uint token)
    {
        if (Field(token, 0x254) != 0)
        {
            return false;
        }

        if (Field(token, 0x258) == 0)
        {
            Need(_hooks.Initialize)(token);
        }

        if (Field(token, 0x258) == 0)
        {
            return true;
        }

        // 초기화 효과 뒤 258을 다시 읽고 타이밍 전역은 전체 행동 뒤 현재값을 읽는다.
        double started = Need(_hooks.Wall)();
        Need(_hooks.Prepare)(token);
        Need(_hooks.SelectGoal)(token);
        Actions(token);
        if (Need(_globals.Timing)() != 0)
        {
            double elapsed = Need(_hooks.Wall)() - started;
            if (elapsed > SlowFrame)
            {
                Need(_hooks.Took)(token, elapsed);
            }
        }

        return true;
    }

    /// <summary>현재 전투에서 Player1..8의 현재 brain을 방문하고 거짓 반환 본문을 소멸/반납 뒤 0으로 쓴다. 0.647에서 추가했다.</summary>
    public void Frame()
    {
        if (!Need(_globals.Battle)())
        {
            return;
        }

        // 현재 Player1..8을 같은 stride로 차례로 방문한다.
        for (int owner = 1; owner < PlayerCount; owner++)
        {
            int at = owner * PlayerBytes + BrainField;
            uint token = BinaryPrimitives.ReadUInt32LittleEndian(_players.AsSpan(at, 4));
            if (token == 0 || Tick(token))
            {
                continue;
            }

            uint current = BinaryPrimitives.ReadUInt32LittleEndian(_players.AsSpan(at, 4));
            if (current != 0)
            {
                Need(_hooks.Destruct)(current);
                Need(_hooks.Free)(current);
            }

            BinaryPrimitives.WriteUInt32LittleEndian(_players.AsSpan(at, 4), 0);
        }
    }

    /// <summary>해제된 본문/잘못된 크기의 가짜 본체를 거부한다. 0.647에서 추가했다.</summary>
    private byte[] Body(uint token)
    {
        byte[] raw = _memory.Bytes(token);
        if (raw.Length != RawAiBrainConstructor.BytesCount)
        {
            throw new ArgumentException("AI 프레임 실제0xb08 본체 누락", nameof(token));
        }

        return raw;
    }

    /// <summary>각 원본 위치에서 현재 본체를 다시 조회한다. 0.647에서 추가했다.</summary>
    private uint Field(uint token, uint offset) => BinaryPrimitives.ReadUInt32LittleEndian(Body(token).AsSpan((int)offset, 4));

    /// <summary>도달한 하위 누락을 빈 성공으로 대신하지 않는다. 0.647에서 추가했다.</summary>
    private static T Need<T>(T? function) where T : class => function ?? throw new InvalidOperationException("AI 전체 프레임 실제 하위·전역 미연결");
}
