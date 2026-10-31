using System.Buffers.Binary;
using Netstorm.Core.Simulation;

// 원본 AI의 실제 생성 소유자와 Player 부착 경계를 제공한다.
namespace Netstorm.Core.Original;

/// <summary>AI 생성이 호출하는 실제 색·월드·생성자 및 초기 갱신 효과다.</summary>
public sealed class AiCreateHooks
{
    /// <summary>현재 소유자의 색을 변경한다.</summary>
    public Action<uint, uint>? ChangeColor { get; init; }
    /// <summary>소유자의 첫 섬 영역을 조회한다.</summary>
    public Func<uint, uint>? FirstRegion { get; init; }
    /// <summary>영역의 현재 신전 SID를 조회한다.</summary>
    public Func<uint, uint>? IslandTemple { get; init; }
    /// <summary>신전을 원본 플래그로 제거한다.</summary>
    public Action<uint, uint>? DestroyTemple { get; init; }
    /// <summary>실제 AI 생성자에 소유자·설정·할당 크기를 전달하고 유효 brain 토큰을 받는다.</summary>
    public Func<uint, ReadOnlyMemory<byte>, int, uint>? Construct { get; init; }
    /// <summary>실제 AI의 기술과 분류를 초기 갱신한다.</summary>
    public Action<uint, ReadOnlyMemory<byte>>? InitialUpdate { get; init; }
}

/// <summary>10.78 AI 생성의 색→기존 brain→NoTemple→생성→부착→초기 갱신 순서다.
/// 근거: cpppj/src/o/RawAiCreate.h/.cpp, 원본 00417e50.</summary>
public sealed class RawAiCreate
{
    /// <summary>원본 AI 할당 크기다.</summary>
    private const int BrainSize = 0xb08;
    /// <summary>원본 신전 가상 제거 플래그다.</summary>
    private const uint DestroyFlags = 0x400000;
    /// <summary>실제 Player 기록과 brain 수명을 소유하는 참여 모듈이다.</summary>
    private readonly PlayerParticipation _players;
    /// <summary>실제 월드 및 AI 생성 효과다.</summary>
    private readonly AiCreateHooks _hooks;

    /// <summary>같은 Player 저장소와 실제 효과를 빌리는 10.78 생성기를 만든다.</summary>
    /// <param name="edition">기준 판본.</param><param name="players">실제 Player 참여 모듈.</param><param name="hooks">실제 생성 효과.</param>
    public RawAiCreate(OriginalEdition edition, PlayerParticipation players, AiCreateHooks hooks)
    {
        if (edition != OriginalEdition.Patch1078) throw new ArgumentException("AiCreate는 10.78 전용이다", nameof(edition));
        _players = players; _hooks = hooks;
    }

    /// <summary>선택된 실제 효과가 없으면 가짜 성공으로 바꾸지 않고 실패한다.</summary>
    /// <param name="effect">실제 호출할 효과.</param>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("AI 생성의 실제 효과 연결 누락");

    /// <summary>설정의 색과 기존 brain을 확인하고 필요할 때만 AI를 생성하여 같은 Player에 부착한다.</summary>
    /// <param name="owner">AI 소유자 번호.</param><param name="settings">실제 600바이트 설정.</param>
    public uint Create(uint owner, byte[] settings)
    {
        if (settings.Length != RawMissionAiSettings.Size) throw new ArgumentException("AI 생성에는 600바이트 설정이 필요하다", nameof(settings));
        if (_players.BrainReturnedForState(owner)) throw new InvalidOperationException("반납된 AI 소유자를 다시 생성할 수 없다");
        uint color = BinaryPrimitives.ReadUInt32LittleEndian(settings.AsSpan(0x250, 4));
        if (color != 0) Need(_hooks.ChangeColor)(owner, color);
        uint old = _players.Brain(owner);
        if (old != 0) return old;
        if (BinaryPrimitives.ReadUInt32LittleEndian(settings.AsSpan(0x244, 4)) != 0)
        {
            uint region = Need(_hooks.FirstRegion)(owner);
            uint temple = Need(_hooks.IslandTemple)(region);
            if (temple != 0) Need(_hooks.DestroyTemple)(temple, DestroyFlags);
            return 0;
        }
        var construct = Need(_hooks.Construct);
        var update = Need(_hooks.InitialUpdate);
        uint brain = construct(owner, settings, BrainSize);
        if (brain == 0) throw new InvalidOperationException("실제 AI 생성자가 빈 brain을 반환했다");
        _players.AttachBrain(owner, brain);
        update(brain, settings);
        return brain;
    }
}
