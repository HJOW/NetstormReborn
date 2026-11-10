// PATCH의 실제 로그인·방향별 동맹·AI 부착/해제에서 Player 기록의 쓰기와 효과를 복원한다.
using System.Text;

namespace Netstorm.Core.Original;

// Player 참여 모듈을 담는 네임스페이스다. 2026-11-07 F2d Player 이식에서 추가됐다.

// 원문의 진단 후 계속 진행하는 두 assertion 경계다. 2026-11-07 F2d Player 이식에서 추가됐다.
public enum PlayerParticipationAssertion
{
    /// <summary>기존 brain이 0이어야 하는 진단이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    BrainAlreadyAssigned,

    /// <summary>번호 얻기·Chat 방문에서 metamap 아님을 확인하는 진단이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    MetamapMode,
}

// 각 함수가 실제 도달 시 읽는 mutable 전역이다. 2026-11-07 F2d Player 이식에서 추가됐다.
public sealed class PlayerParticipationState
{
    /// <summary>현재 로컬 소유자다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public uint LocalOwner { get; set; } = 1u;

    /// <summary>현재 known 값이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public uint PlayerKnown { get; set; }

    /// <summary>전투 모드다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public bool Battle { get; set; }

    /// <summary>metamap 모드다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public bool Metamap { get; set; }

    /// <summary>로컬 전송 여부다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public bool LocalTransport { get; set; }
}

// 실제 세션의 다른 모듈이 소유한 현재 전역을 각 원문 읽기 위치에 연결한다. 2026-11-07 F2d Player 이식에서 추가됐다.
public sealed class PlayerParticipationReads
{
    /// <summary>같은 PlayerWorld의 현재 소유자·known 값이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Func<uint>? LocalOwner { get; set; }

    /// <summary>같은 PlayerWorld의 현재 known 값이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Func<uint>? PlayerKnown { get; set; }

    /// <summary>같은 월드의 전투 모드다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Func<bool>? Battle { get; set; }

    /// <summary>같은 월드의 metamap 모드다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Func<bool>? Metamap { get; set; }

    /// <summary>전송 계층의 로컬 전송 여부다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Func<bool>? LocalTransport { get; set; }
}

// 실제 전송·Chat UI와 실제 AI 소유자의 파괴·반납 경계다. 2026-11-07 F2d Player 이식에서 추가됐다.
public sealed class PlayerParticipationHooks
{
    /// <summary>원문 assert 보고 후 계속 수행하는 진단이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Action<PlayerParticipationAssertion>? Assertion { get; set; }

    /// <summary>실제 연결 번호 재배치다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Action<int, int>? AssignConnection { get; set; }

    /// <summary>로그인 거부 순서다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Action<int>? RejectLogin { get; set; }

    /// <summary>연결 해제 순서다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Action<int>? Disconnect { get; set; }

    /// <summary>실제 채널 초기화다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Action<uint>? ClearChat { get; set; }

    /// <summary>고정 나머지 인자를 갖는 실제 등록이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Action<uint, string, uint>? AddChat { get; set; }

    /// <summary>논리 packet이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Action<uint, uint, uint>? SendAlliance { get; set; }

    /// <summary>실제 소유자의 파괴다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Action<uint>? DestructBrain { get; set; }

    /// <summary>CRT 반납이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Action<uint>? FreeBrain { get; set; }
}

// 9×0xb4 Player와 원본 9×9 DWORD 표를 참조한다. 2026-11-07 F2d Player 이식에서 추가됐다.
public sealed class RawPlayerParticipation
{
    /// <summary>PATCH 원본 Player 바이트 간격이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const int Stride = 0xb4;

    /// <summary>실제 Player 수다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const int Count = 9;

    /// <summary>플래그 DWORD 위치다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const int FlagsOffset = 0x50;

    /// <summary>동맹 마스크 DWORD 위치다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const int AlliesOffset = 0x54;

    /// <summary>brain DWORD 위치다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const int BrainOffset = 0xb0;

    /// <summary>구독자 DWORD 위치다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const int SubscriberOffset = 0x2c;

    /// <summary>이름 바이트 위치다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const int NameOffset = 0xc;

    /// <summary>이름 최대 바이트 수다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private const int NameWidth = 32;

    /// <summary>실제 정상 복귀 증빙이다. 이동 전용이며 복제하지 않는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public sealed class BrainResetPass
    {
        /// <summary>발급한 실제 모듈이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        internal RawPlayerParticipation Source { get; }

        /// <summary>발급 모듈의 정체성 번호다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        internal long ModuleId { get; }

        /// <summary>증빙 소유자다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        public uint Owner { get; }

        /// <summary>발급 순서 번호다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        internal uint Serial { get; }

        /// <summary>실제 부모 호출 뒤에만 발급한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
        /// <param name="source">발급 모듈이다.</param>
        /// <param name="moduleId">모듈 정체성 번호다.</param>
        /// <param name="owner">증빙 소유자다.</param>
        /// <param name="serial">발급 순서 번호다.</param>
        internal BrainResetPass(RawPlayerParticipation source, long moduleId, uint owner, uint serial)
        {
            Source = source;
            ModuleId = moduleId;
            Owner = owner;
            Serial = serial;
        }
    }

    /// <summary>모듈 정체성 발급 순서다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private static long _nextModuleId;

    /// <summary>이 모듈의 정체성 번호다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private readonly long _moduleId = Interlocked.Increment(ref _nextModuleId);

    /// <summary>소비한 정상 복귀 증빙의 소유자별 순서 번호다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private readonly Dictionary<uint, uint> _returnedSerials = new();

    /// <summary>다음 발급 순서 번호다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private uint _nextSerial;

    /// <summary>실제 파괴·반납 callback이 끝나기 전 재진입을 막는 호출 수다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private int _activeBrainCalls;

    /// <summary>같은 실제 9 Player 기록이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private readonly Memory<byte> _players;

    /// <summary>한쪽 방향만 바뀔 수 있는 원본 81 DWORD다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private readonly uint[] _allies;

    /// <summary>현재 9개 숨김 표시다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private readonly byte[] _hidden;

    /// <summary>효과 callback 이후에도 다시 읽는 현재 전역이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private readonly PlayerParticipationState _state;

    /// <summary>실제 전송·Chat·생성 소유자 수명 효과다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private readonly PlayerParticipationHooks _hooks;

    /// <summary>실제 세션이 바인딩한 현재 전역 getter다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    private PlayerParticipationReads _reads = new();

    /// <summary>
    /// 이 첫 구현은 PATCH 전용이다. 실제 저장소·모드·효과 대상은 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="edition">원본 판본이다.</param>
    /// <param name="players">9×180바이트 기록이다.</param>
    /// <param name="allies">81 DWORD 표다.</param>
    /// <param name="hidden">9바이트 숨김 표다.</param>
    /// <param name="state">현재 전역이다.</param>
    /// <param name="hooks">외부 효과 묶음이다.</param>
    /// <exception cref="ArgumentException">Player 참여 초기화의 PATCH 저장소 크기 오류다.</exception>
    public RawPlayerParticipation(OriginalEdition edition, Memory<byte> players, uint[] allies, byte[] hidden, PlayerParticipationState state, PlayerParticipationHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(allies);
        ArgumentNullException.ThrowIfNull(hidden);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (edition != OriginalEdition.Patch1078 || players.Length != Stride * Count || allies.Length != Count * Count || hidden.Length != Count)
        {
            throw new ArgumentException("Player 참여 초기화의 PATCH 저장소 크기 오류");
        }

        _players = players;
        _allies = allies;
        _hidden = hidden;
        _state = state;
        _hooks = hooks;
    }

    /// <summary>이 몸체가 쓰는 Player 기록이다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public Memory<byte> Players => _players;

    /// <summary>다섯 현재 조회를 모두 공급한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="reads">바인딩할 getter 묶음이다.</param>
    /// <exception cref="ArgumentException">현재 전역 조회 누락이다.</exception>
    public void BindCurrentInputs(PlayerParticipationReads reads)
    {
        ArgumentNullException.ThrowIfNull(reads);
        if (reads.LocalOwner is null || reads.PlayerKnown is null || reads.Battle is null || reads.Metamap is null || reads.LocalTransport is null)
        {
            throw new ArgumentException("Player 참여 현재 전역 조회 누락", nameof(reads));
        }

        _reads = reads;
    }

    /// <summary>
    /// 유효 owner의 bit1 부재에서만 flags를 고치고 연결을 수락한다.
    /// </summary>
    /// <param name="pending">대기 번호다.</param>
    /// <param name="owner">소유자 번호다.</param>
    /// <returns>수락하면 참이다.</returns>
    public bool CheckLogin(int pending, int owner)
    {
        if (owner > 0 && owner < Count && (Read((uint)owner, FlagsOffset) & 1u) == 0u)
        {
            uint player = (uint)owner;
            Put(player, FlagsOffset, (Read(player, FlagsOffset) & ~0x100u) | 3u);
            if (!(LocalTransport() && pending == owner))
            {
                Require(_hooks.AssignConnection)(pending, owner);
            }

            return true;
        }

        Require(_hooks.RejectLogin)(pending);
        Require(_hooks.Disconnect)(pending);
        return false;
    }

    /// <summary>
    /// 자기 bit를 포함한 마스크를 쓰고, known이면 양방향 Chat 표시를 재구성한다.
    /// </summary>
    /// <param name="owner">소유자 번호다.</param>
    /// <param name="mask">쓸 마스크다.</param>
    public void SetAlliance(uint owner, uint mask)
    {
        Player(owner);
        mask |= 1u << (int)(owner & 31u);
        Put(owner, AlliesOffset, mask);
        // 원본은 한 행의 9개 DWORD를 각각 0·1로 갱신하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (uint target = 0u; target < Count; ++target)
        {
            _allies[(int)(owner * Count + target)] = (mask >> (int)target) & 1u;
        }

        if (PlayerKnown() == 0u)
        {
            return;
        }

        Require(_hooks.ClearChat)(0x40u);
        Require(_hooks.ClearChat)(0x80u);
        // 채널 효과 뒤 현재 값을 다시 읽어 다음 소유자를 처리하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (uint player = 0u; player < Count; ++player)
        {
            if (LocalOwner() >= Count)
            {
                throw new ArgumentOutOfRangeException(nameof(owner), "Player 참여 Chat의 로컬 번호 오류");
            }

            if (_allies[(int)(LocalOwner() * Count + player)] != 0u && _allies[(int)(player * Count + LocalOwner())] != 0u)
            {
                Require(_hooks.AddChat)(Read(player, SubscriberOffset), Name(player), 0x40u);
            }

            if (Metamap())
            {
                Require(_hooks.Assertion)(PlayerParticipationAssertion.MetamapMode);
            }

            if (_hidden[(int)player] != 0)
            {
                Require(_hooks.AddChat)(Read(player, SubscriberOffset), Name(player), 0x80u);
            }
        }
    }

    /// <summary>
    /// 지정 방향의 한 bit·한 칸만 켜며 battle이면 현재 마스크를 서버에 요청한다.
    /// </summary>
    /// <param name="owner">소유자 번호다.</param>
    /// <param name="target">대상 번호다.</param>
    public void AddAlly(uint owner, uint target)
    {
        Player(owner);
        Player(target);
        Put(owner, AlliesOffset, Read(owner, AlliesOffset) | (1u << (int)(target & 31u)));
        if (Metamap())
        {
            Require(_hooks.Assertion)(PlayerParticipationAssertion.MetamapMode);
        }

        bool battle = Battle();
        _allies[(int)(owner * Count + target)] = 1u;
        if (battle)
        {
            if (Metamap())
            {
                Require(_hooks.Assertion)(PlayerParticipationAssertion.MetamapMode);
            }

            Require(_hooks.SendAlliance)(9u, owner, Read(owner, AlliesOffset));
        }
    }

    /// <summary>
    /// 지정 방향의 한 bit·한 칸만 끄며 battle이면 현재 마스크를 서버에 요청한다.
    /// 근거: cpppj `o/RawPlayerParticipation.cpp`의 `RemoveAlly`(48f710). 2026-11-11 F3 동맹 해제 본체에서 추가됐다.
    /// </summary>
    /// <param name="owner">소유자 번호다.</param>
    /// <param name="target">끊을 상대 번호다.</param>
    /// <exception cref="ArgumentOutOfRangeException">소유자·상대 방향성 표 범위 오류다.</exception>
    public void RemoveAlly(uint owner, uint target)
    {
        Put(owner, AlliesOffset, Read(owner, AlliesOffset) & ~(1u << (int)(target & 31u)));
        if (Metamap())
        {
            Require(_hooks.Assertion)(PlayerParticipationAssertion.MetamapMode);
        }

        bool battle = Battle();
        long cell = (long)owner * Count + target;
        // 원본의 방향성 표 범위 검사를 그대로 둔다. 2026-11-11 F3 동맹 해제 본체에서 추가됐다.
        if (cell < 0 || cell >= _allies.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), "Player 참여 동맹 방향성 표 범위 오류");
        }

        _allies[(int)cell] = 0u;
        if (battle)
        {
            if (Metamap())
            {
                Require(_hooks.Assertion)(PlayerParticipationAssertion.MetamapMode);
            }

            Require(_hooks.SendAlliance)(9u, owner, Read(owner, AlliesOffset));
        }
    }

    /// <summary>
    /// 같은 수신 packet의 자기 bit→Player 마스크→방향표·Chat→broadcast 순서를 보존한다.
    /// </summary>
    /// <param name="packet">쓸 수신 버퍼다. 9바이트 이상이다.</param>
    /// <param name="broadcast">같은 변경 바이트를 읽는 방송이다.</param>
    public void ReceiveAlliance(Span<byte> packet, Action<ReadOnlySpan<byte>> broadcast)
    {
        ArgumentNullException.ThrowIfNull(broadcast);
        if (packet.Length < 9)
        {
            throw new ArgumentOutOfRangeException(nameof(packet), "동맹 수신의 실제 9바이트 미만 입력");
        }

        uint owner = packet[4];
        Player(owner);
        uint mask = 1u << (int)(owner & 31u);
        // 비정렬 packet+5의 현재 DWORD를 원본 순서로 조립하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (int index = 0; index < 4; ++index)
        {
            mask |= (uint)packet[5 + index] << (index * 8);
        }

        // 원본은 입력 packet에도 self bit를 기록하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (int index = 0; index < 4; ++index)
        {
            packet[5 + index] = (byte)(mask >> (index * 8));
        }

        Put(owner, AlliesOffset, mask);
        SetAlliance(owner, mask);
        Require(broadcast)(packet);
    }

    /// <summary>
    /// 실제 생성 소유자가 전달한 brain을 기록하고 flags를 켠다.
    /// </summary>
    /// <param name="owner">소유자 번호다.</param>
    /// <param name="brain">쓸 brain 값이다.</param>
    public void AttachBrain(uint owner, uint brain)
    {
        if (BrainReturnedForState(owner))
        {
            throw new InvalidOperationException("반납된 AI 소유자의 새 brain 설치 금지");
        }

        if (Read(owner, BrainOffset) != 0u)
        {
            Require(_hooks.Assertion)(PlayerParticipationAssertion.BrainAlreadyAssigned);
        }

        Put(owner, BrainOffset, brain);
        Put(owner, FlagsOffset, Read(owner, FlagsOffset) | 0x20u);
        if (unchecked((int)Read(owner, SubscriberOffset)) < 1)
        {
            if (Metamap())
            {
                Require(_hooks.Assertion)(PlayerParticipationAssertion.MetamapMode);
            }

            Put(owner, SubscriberOffset, owner + 10000u);
            Span<byte> raw = PlayerBytes(owner);
            // 원본은 DWORD 7개와 마지막 WORD를 써서 30바이트만 지우는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
            for (int offset = 0x30; offset < 0x4e; ++offset)
            {
                raw[offset] = 0;
            }
        }
    }

    /// <summary>현재 실제 소유자의 토큰만 읽는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="owner">소유자 번호다.</param>
    /// <returns>brain 값이다.</returns>
    public uint Brain(uint owner) => Read(owner, BrainOffset);

    /// <summary>
    /// 실제 파괴→반납→brain 0이다. 원본대로 flags는 지우지 않는다.
    /// </summary>
    /// <param name="owner">소유자 번호다.</param>
    public void RemoveBrain(uint owner)
    {
        if (BrainReturnedForState(owner))
        {
            throw new InvalidOperationException("반납된 AI 소유자의 반복 reset 금지");
        }

        uint brain = Brain(owner);
        if (brain == 0u)
        {
            return;
        }

        ++_activeBrainCalls;
        try
        {
            Require(_hooks.DestructBrain)(brain);
            Require(_hooks.FreeBrain)(brain);
            Put(owner, BrainOffset, 0u);
        }
        finally
        {
            --_activeBrainCalls;
        }
    }

    /// <summary>
    /// 원문 cleanup의 owner1..8 위치에서 끝낸 뒤에만 이동 전용 증빙을 만든다.
    /// </summary>
    /// <param name="owner">소유자 번호다.</param>
    /// <returns>정상 복귀 증빙이다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">State AI 반납의 실제 owner 범위다.</exception>
    public BrainResetPass RemoveBrainForState(uint owner)
    {
        if (owner < 1u || owner >= Count)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), "State AI 반납의 실제 owner 범위");
        }

        RemoveBrain(owner);
        return new BrainResetPass(this, _moduleId, owner, ++_nextSerial);
    }

    /// <summary>
    /// 정상 복귀와 현재 모듈의 같은 대여만 소비한다.
    /// </summary>
    /// <param name="pass">소비할 증빙이다.</param>
    /// <exception cref="ArgumentException">같은 실제 AI 모듈의 정상 reset 증빙이 아니다.</exception>
    /// <exception cref="InvalidOperationException">진행 중 효과·이미 반납·현재 brain 변경이다.</exception>
    public void ReturnBrainForState(BrainResetPass pass)
    {
        ArgumentNullException.ThrowIfNull(pass);
        uint owner = pass.Owner;
        if (!ReferenceEquals(pass.Source, this) || pass.ModuleId != _moduleId || owner < 1u || owner >= Count)
        {
            throw new ArgumentException("같은 실제 AI 모듈의 정상 reset 증빙 필요", nameof(pass));
        }

        if (_activeBrainCalls != 0)
        {
            throw new InvalidOperationException("진행 중 AI 파괴·반납 효과의 증빙 소비 금지");
        }

        if (_returnedSerials.ContainsKey(owner) || Brain(owner) != 0u)
        {
            throw new InvalidOperationException("현재 brain 0인 미반납 AI 소유자 증빙 필요");
        }

        _returnedSerials[owner] = pass.Serial;
    }

    /// <summary>
    /// 보관한 실제 증빙만 조회하며 현재 brain이 바뀌었으면 명시 수명 오류다.
    /// </summary>
    /// <param name="owner">소유자 번호다.</param>
    /// <returns>반납됐으면 참이다.</returns>
    /// <exception cref="InvalidOperationException">반납된 소유자의 현재 brain 정체성 변경이다.</exception>
    public bool BrainReturnedForState(uint owner)
    {
        Player(owner);
        if (!_returnedSerials.ContainsKey(owner))
        {
            return false;
        }

        if (Brain(owner) != 0u)
        {
            throw new InvalidOperationException("반납된 AI 소유자의 현재 brain 정체성 변경");
        }

        return true;
    }

    /// <summary>0번 중립은 대상이 아니다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    public void RemoveAllBrains()
    {
        // 실제 owner1..8의 현재 값을 각 방문 시점에 새로 읽는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (uint owner = 1u; owner < Count; ++owner)
        {
            if (!BrainReturnedForState(owner))
            {
                RemoveBrain(owner);
            }
        }
    }

    /// <summary>0번 중립 기록은 허용한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="owner">기록 번호다.</param>
    /// <returns>기록 뷰다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">Player 참여 번호 범위 오류다.</exception>
    private Span<byte> Player(uint owner)
    {
        if (owner >= Count)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), "Player 참여 번호 범위 오류");
        }

        return _players.Slice((int)(owner * Stride), Stride).Span;
    }

    /// <summary>기록 뷔를 배열로 본다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="owner">기록 번호다.</param>
    /// <returns>기록 뷰다.</returns>
    private Span<byte> PlayerBytes(uint owner) => Player(owner);

    /// <summary>정렬되지 않은 원본 DWORD를 읽는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="owner">기록 번호다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>조합한 값이다.</returns>
    private uint Read(uint owner, int offset)
    {
        ReadOnlySpan<byte> raw = Player(owner);
        uint result = 0u;
        // 낮은 바이트부터 모으는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (int index = 0; index < 4; ++index)
        {
            result |= (uint)raw[offset + index] << (index * 8);
        }

        return result;
    }

    /// <summary>같은 기록의 지정 DWORD만 변경한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="owner">기록 번호다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private void Put(uint owner, int offset, uint value)
    {
        Span<byte> raw = Player(owner);
        // 낮은 바이트부터 기록하는 반복문이다. 2026-11-07 F2d Player 이식에서 추가됐다.
        for (int index = 0; index < 4; ++index)
        {
            raw[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>원본 이름 필드의 정상 문자열만 제공한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <param name="owner">기록 번호다.</param>
    /// <returns>이름 문자열이다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">Player 참여 이름의 NUL 종료 없음이다.</exception>
    private string Name(uint owner)
    {
        ReadOnlySpan<byte> raw = Player(owner);
        int length = 0;
        while (length < NameWidth && raw[NameOffset + length] != 0)
        {
            ++length;
        }

        if (length == NameWidth)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), "Player 참여 이름의 NUL 종료 없음");
        }

        return Encoding.Latin1.GetString(raw.Slice(NameOffset, length));
    }

    /// <summary>값형 사본과 섞지 않고 실제 세션의 현재 값을 읽는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <returns>로컬 소유자다.</returns>
    private uint LocalOwner() => _reads.LocalOwner is not null ? _reads.LocalOwner() : _state.LocalOwner;

    /// <summary>기술 부모와 같은 현재 known 값을 읽는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <returns>known 값이다.</returns>
    private uint PlayerKnown() => _reads.PlayerKnown is not null ? _reads.PlayerKnown() : _state.PlayerKnown;

    /// <summary>현재 전투 모드를 읽는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <returns>전투 모드다.</returns>
    private bool Battle() => _reads.Battle is not null ? _reads.Battle() : _state.Battle;

    /// <summary>각 진단 직전에 현재 metamap을 읽는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <returns>metamap 모드다.</returns>
    private bool Metamap() => _reads.Metamap is not null ? _reads.Metamap() : _state.Metamap;

    /// <summary>실제 로컬 전송 여부를 읽는다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <returns>로컬 전송 여부다.</returns>
    private bool LocalTransport() => _reads.LocalTransport is not null ? _reads.LocalTransport() : _state.LocalTransport;

    /// <summary>실제 도달한 효과만 필수로 요구한다. 2026-11-07 F2d Player 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="callback">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    /// <exception cref="InvalidOperationException">외부 효과 미연결이다.</exception>
    private static T Require<T>(T? callback)
        where T : class
    {
        if (callback is null)
        {
            throw new InvalidOperationException("Player 참여 초기화의 실제 외부 효과 미연결");
        }

        return callback;
    }
}
