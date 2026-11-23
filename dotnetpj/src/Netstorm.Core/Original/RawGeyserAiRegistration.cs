// 같은 Player 원본 바이트를 직접 읽으며 AI가 없는 최초 로드의 실제 순회를 실행한다.
// cpppj/src/o/RawGeyserAiRegistration.h·cpp 의 RawGeyserAiRegistration/GeyserAiRegistrationBoundary 를 이식한다.
// 0.901에서 RU-1 실제 owner 조립용으로 추가됐다.
namespace Netstorm.Core.Original;

/// <summary>첫 미복원 요청의 Player 번호·원본 AI 포인터·간헐천 SID·등록/제거 DWORD를 저장하는 예외다.</summary>
public sealed class GeyserAiRegistrationBoundary : InvalidOperationException
{
    /// <summary>실제 순회에서 발견한 Player1..8 번호다. 0.901에서 추가됐다.</summary>
    public uint Owner { get; }

    /// <summary>현재 기록 +b0/CD+a8의 32비트 AI 포인터 값이다. 0.901에서 추가됐다.</summary>
    public uint AiPointer { get; }

    /// <summary>원본 첫 인자이며 null AI 경로에서는 SID 유효성 검사를 하지 않는다. 0.901에서 추가됐다.</summary>
    public uint Sid { get; }

    /// <summary>원본 둘째 인자다. 0은 제거이고 nonzero는 등록 하위 요청이다. 0.901에서 추가됐다.</summary>
    public uint State { get; }

    /// <summary>첫 미복원 요청의 네 DWORD를 그대로 유지하고 완료하지 않은 AI 몸체를 알린다. 0.901에서 추가됐다.</summary>
    /// <param name="owner">실제 순회에서 발견한 Player 번호다.</param>
    /// <param name="pointer">현재 기록의 32비트 AI 포인터 값이다.</param>
    /// <param name="geyser">원본 첫 인자다.</param>
    /// <param name="operation">원본 둘째 인자다.</param>
    public GeyserAiRegistrationBoundary(uint owner, uint pointer, uint geyser, uint operation)
        : base("간헐천 등록의 nonnull AI 하위 몸체는 아직 복원되지 않았습니다")
    {
        Owner = owner;
        AiPointer = pointer;
        Sid = geyser;
        State = operation;
    }
}

/// <summary>PATCH00416720/CD004d8670의 Player1..8 순회다. PATCH 실제 하위를 연결하면 현재 본체를 갱신한다.</summary>
public sealed class RawGeyserAiRegistration
{
    /// <summary>Money/Technology 등과 공유하는 실제 Player 저장소다. 0.901에서 추가됐다.</summary>
    private readonly ReadOnlyMemory<byte> _players;

    /// <summary>PATCH Player0xb4/CD Player0xac의 한 기록 크기다. 0.901에서 추가됐다.</summary>
    private readonly int _stride;

    /// <summary>PATCH+b0/CD+a8의 현재 AI 포인터 위치다. 0.901에서 추가됐다.</summary>
    private readonly int _aiOffset;

    /// <summary>현재 Player가 가리키는 본체·SID·원본 state를 같은 하위에 전달한다. 0.901에서 추가됐다.</summary>
    private Action<uint, uint, uint>? _update;

    /// <summary>동일 Player 9기록을 복사하지 않고 참조한다. 호출 동안 해당 바이트 저장소가 유효해야 한다. 0.901에서 추가됐다.</summary>
    /// <param name="edition">판본별 기록 크기와 AI 위치를 고른다.</param>
    /// <param name="playerRecords">실제 Player 9기록이다.</param>
    public RawGeyserAiRegistration(OriginalEdition edition, ReadOnlyMemory<byte> playerRecords)
    {
        if (playerRecords.Length < (edition == OriginalEdition.Patch1078 ? 9 * 0xb4 : 9 * 0xac))
        {
            throw new ArgumentException("간헐천 등록에는 실제 Player 9기록이 필요합니다", nameof(playerRecords));
        }

        _players = playerRecords;
        _stride = edition == OriginalEdition.Patch1078 ? 0xb4 : 0xac;
        _aiOffset = edition == OriginalEdition.Patch1078 ? 0xb0 : 0xa8;
    }

    /// <summary>실제 PATCH 하위만 바인딩한다. 빈 성공을 허용하지 않는다. 0.901에서 추가됐다.</summary>
    /// <param name="update">실제 하위다.</param>
    public void BindPatch(Action<uint, uint, uint> update)
    {
        ArgumentNullException.ThrowIfNull(update);
        if (_stride != 0xb4)
        {
            throw new ArgumentException("간헐천 AI의 실제 PATCH 하위 누락/판본 오류", nameof(update));
        }

        _update = update;
    }

    /// <summary>현재 AI 포인터를 번호순으로 읽고 연결된 하위를 호출한다. 미연결 nonnull이면 첫 요청을 명시 거부한다. 0.901에서 추가됐다.</summary>
    /// <param name="sid">원본 첫 인자다.</param>
    /// <param name="state">원본 둘째 인자다.</param>
    public void Register(uint sid, uint state)
    {
        ReadOnlySpan<byte> players = _players.Span;
        // Player0은 원본 순회 대상이 아니다. 1..8의 현재 포인터를 매 호출 다시 읽는다.
        for (uint owner = 1; owner < 9; owner++)
        {
            int field = unchecked((int)(owner * (uint)_stride + (uint)_aiOffset));
            uint ai = unchecked((uint)(players[field] | (players[field + 1] << 8) | (players[field + 2] << 16) | (players[field + 3] << 24)));
            if (ai != 0)
            {
                Action<uint, uint, uint>? update = _update;
                if (update is null)
                {
                    throw new GeyserAiRegistrationBoundary(owner, ai, sid, state);
                }

                update(ai, sid, state);
            }
        }
    }
}
