namespace Netstorm.Core.Simulation;

/// <summary>원본이 진단만 남기고 계속 진행하는 두 assertion 의 종류.</summary>
public enum PlayerParticipationAssertion
{
    /// <summary>이미 brain 이 있는 Player 에 다시 brain 을 붙이려 했다(004911a0).</summary>
    BrainAlreadyAssigned,

    /// <summary>metamap 모드에서 Player 번호를 조회했다(owner getter, Chat 방문).</summary>
    MetamapMode,
}

/// <summary>
/// 각 함수가 실제로 도달했을 때 읽는 현재 전역 상태. 연결 완료(Ready)와 사람 연결/AI 참여를 같은 값으로 합치지 않는다.
/// 효과가 끝난 뒤에도 다시 읽으므로 복사하지 않고 같은 인스턴스를 공유한다.
/// </summary>
public sealed class PlayerParticipationState
{
    /// <summary>로컬 플레이어 번호(00540c70). 동맹 Chat 표시와 AI 생성의 반대 방향 규칙이 이 값을 읽는다.</summary>
    public uint LocalOwner { get; set; } = 1;

    /// <summary>Player 번호를 알고 있는지(00595344). 0이면 동맹 설정 뒤 Chat 채널 재구성을 건너뛴다.</summary>
    public uint PlayerKnown { get; set; }

    /// <summary>전투 모드(00594fbc). 켜져 있으면 방향 추가가 서버 9번 메시지를 요청한다.</summary>
    public bool Battle { get; set; }

    /// <summary>metamap 모드(00594fc8). 켜져 있으면 Player 번호 조회마다 진단을 보고한다.</summary>
    public bool Metamap { get; set; }

    /// <summary>로컬 전송 모드(0054db50). 로그인한 연결 번호가 owner 와 같으면 연결 재배치를 생략한다.</summary>
    public bool LocalTransport { get; set; }
}

/// <summary>
/// 실제 전송·Chat UI·AI 소유자의 파괴/반납 같은 외부 효과. 실제로 도달한 효과가 비어 있으면 성공으로 바꾸지 않고 예외를 낸다.
/// 가짜 brain 생성이나 연결 성공을 만들어 주는 경계는 없다.
/// </summary>
public sealed class PlayerParticipationHooks
{
    /// <summary>원본 assert 보고 뒤 계속 진행하는 진단.</summary>
    public Action<PlayerParticipationAssertion>? Assertion { get; init; }

    /// <summary>004e27b0(대기 연결, owner) 연결 번호 재배치.</summary>
    public Action<int, int>? AssignConnection { get; init; }

    /// <summary>004a6920(대기 연결, 0) 거부 응답. 거부는 <see cref="Disconnect"/> 보다 먼저 일어난다.</summary>
    public Action<int>? RejectLogin { get; init; }

    /// <summary>004e2940(대기 연결) 연결 해제.</summary>
    public Action<int>? Disconnect { get; init; }

    /// <summary>00445080(채널) Chat 채널(0x40 동맹, 0x80 숨김) 초기화.</summary>
    public Action<uint>? ClearChat { get; init; }

    /// <summary>00444ee0(구독자, 이름 바이트, 채널) Chat 채널 등록. 이름은 NUL 앞의 원본 바이트 그대로다.</summary>
    public Action<uint, byte[], uint>? AddChat { get; init; }

    /// <summary>004e2dc0(서버 번호, Player, 마스크) 동맹 논리 패킷.</summary>
    public Action<uint, uint, uint>? SendAlliance { get; init; }

    /// <summary>00410100(brain) AI 소멸자.</summary>
    public Action<uint>? DestructBrain { get; init; }

    /// <summary>CRT free(brain) 메모리 반납. 소멸자와 같은 실제 소유자를 쓴다.</summary>
    public Action<uint>? FreeBrain { get; init; }
}

/// <summary>
/// 원본의 로그인(004a6dc0)·방향별 동맹(00490ea0/0048f6a0)·AI 부착/해제(004911a0/00413ae0/00413b20)가 Player 기록과
/// 방향별 9×9 동맹 표에 남기는 쓰기와 효과 순서를 그대로 옮긴다.
/// </summary>
/// <remarks>
/// Player 9명의 0xb4바이트 기록과 81개 DWORD 표를 호출자가 소유한 실제 저장소로 받아 같은 바이트를 읽고 쓴다
/// (사본·대칭 동맹·추정 active 를 만들지 않는다). 원본 10.78 의 실제 기계어 608개 관찰과 바이트 단위로 대조한다
/// (cpppj/tests/fixtures/player-participation-x86.tsv, X86PlayerParticipationTests).
/// 근거: docs/exe/cpp-player-participation-20261015.md. 2026-10-15 감사 D03 에서 cpppj RawPlayerParticipation 을 옮겨 추가.
/// 이 클래스는 brain 메모리나 전략을 만들지 않는다. 참여 비트(0x20)는 상위 소유자가 실제 brain 토큰을 넘길 때만 켠다.
/// </remarks>
public sealed class PlayerParticipation
{
    /// <summary>Player 기록 하나의 폭(0xb4바이트).</summary>
    public const int PlayerStride = 0xb4;

    /// <summary>Player 기록 개수(중립 0 + 플레이어 1~8).</summary>
    public const int PlayerCount = 9;

    /// <summary>Player 기록 안의 연결·참여 flags 위치(+0x50).</summary>
    public const int FlagsOffset = 0x50;

    /// <summary>Player 기록 안의 동맹 마스크 위치(+0x54).</summary>
    public const int AllyMaskOffset = 0x54;

    /// <summary>Player 기록 안의 AI brain 포인터 위치(+0xb0).</summary>
    public const int BrainOffset = 0xb0;

    /// <summary>Player 기록 안의 구독자(연결 번호) 위치(+0x2c).</summary>
    public const int SubscriberOffset = 0x2c;

    /// <summary>Player 기록 안의 이름 문자열 위치(+0xc). 구독자 앞까지 32바이트다.</summary>
    public const int NameOffset = 0xc;

    /// <summary>이름 필드의 폭. NUL 이 이 안에 없으면 원본 문자열 경계 오류다.</summary>
    private const int NameWidth = 32;

    /// <summary>사람이 연결해 로그인했음을 뜻하는 flags 비트(0x01).</summary>
    public const uint ConnectedFlag = 0x01;

    /// <summary>사람이 참여 중임을 뜻하는 flags 비트(0x02).</summary>
    public const uint HumanFlag = 0x02;

    /// <summary>AI brain 이 부착됐음을 뜻하는 flags 비트(0x20).</summary>
    public const uint BrainFlag = 0x20;

    /// <summary>승패·사제 생존 판정이 읽는 참여 조건(사람 0x02 또는 AI 0x20).</summary>
    public const uint ParticipationMask = HumanFlag | BrainFlag;

    /// <summary>로그인 때 지우는 해제 비트(0x100). 다른 flags 는 보존한다.</summary>
    private const uint ReleasedFlag = 0x100;

    /// <summary>brain 부착 때 지우는 연결 정보(IP 30바이트)의 시작 위치(+0x30).</summary>
    private const int ConnectionInfoOffset = 0x30;

    /// <summary>brain 부착 때 지우는 연결 정보의 끝 위치(+0x4e, 포함하지 않음).</summary>
    private const int ConnectionInfoEnd = 0x4e;

    /// <summary>부착 때 구독자가 없는 Player 에 주는 번호의 기준값(owner + 10000).</summary>
    private const uint SyntheticSubscriberBase = 10000;

    /// <summary>9×0xb4바이트의 실제 Player 기록. 호출자가 소유하며 이 클래스가 직접 읽고 쓴다.</summary>
    private readonly byte[] _players;

    /// <summary>방향별 9×9 DWORD 표. [출발 * 9 + 도착]이 0이 아니면 출발이 도착을 동맹으로 둔다. 한 방향만 바뀔 수 있다.</summary>
    private readonly uint[] _allies;

    /// <summary>Chat 숨김 표시 9개(0052f81e). 현재 값을 호출 시점에 읽는다.</summary>
    private readonly byte[] _hidden;

    /// <summary>현재 전역 상태. 효과 뒤에도 다시 읽는다.</summary>
    private readonly PlayerParticipationState _state;

    /// <summary>외부 효과 경계.</summary>
    private readonly PlayerParticipationHooks _hooks;

    /// <summary>
    /// 실제 저장소와 상태·효과를 묶는다. 저장소 크기가 원본과 다르면 변경 전에 거부한다.
    /// </summary>
    /// <param name="players">9×0xb4바이트 Player 기록.</param>
    /// <param name="allies">9×9 방향별 동맹 표.</param>
    /// <param name="hidden">Chat 숨김 표시 9개.</param>
    /// <param name="state">현재 전역 상태(공유).</param>
    /// <param name="hooks">외부 효과 경계.</param>
    public PlayerParticipation(byte[] players, uint[] allies, byte[] hidden, PlayerParticipationState state, PlayerParticipationHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(players);
        ArgumentNullException.ThrowIfNull(allies);
        ArgumentNullException.ThrowIfNull(hidden);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (players.Length != PlayerStride * PlayerCount || allies.Length != PlayerCount * PlayerCount || hidden.Length != PlayerCount)
            throw new ArgumentException("Player 참여 저장소의 크기가 원본 10.78 과 다릅니다.");
        _players = players;
        _allies = allies;
        _hidden = hidden;
        _state = state;
        _hooks = hooks;
    }

    /// <summary>실제로 도달한 외부 효과가 비어 있으면 성공으로 바꾸지 않고 거부한다.</summary>
    private static T Require<T>(T? effect) where T : class =>
        effect ?? throw new InvalidOperationException("Player 참여 초기화의 실제 외부 효과가 연결되지 않았습니다.");

    /// <summary>번호의 Player 기록 시작 위치를 돌려준다. 중립 0도 동맹/초기화의 입력이므로 허용한다.</summary>
    private int PlayerBase(uint owner) =>
        owner < PlayerCount ? (int)owner * PlayerStride : throw new ArgumentOutOfRangeException(nameof(owner));

    /// <summary>Player 기록의 DWORD 를 호스트 엔디언과 무관하게 낮은 바이트부터 읽는다.</summary>
    private uint Read(uint owner, int offset)
    {
        int start = PlayerBase(owner) + offset;
        return (uint)(_players[start] | _players[start + 1] << 8 | _players[start + 2] << 16 | _players[start + 3] << 24);
    }

    /// <summary>Player 기록의 DWORD 하나만 낮은 바이트부터 쓴다.</summary>
    private void Put(uint owner, int offset, uint value)
    {
        int start = PlayerBase(owner) + offset;
        // 원본 정수 폭 네 바이트만 바꾼다.
        for (int index = 0; index < 4; index++) _players[start + index] = (byte)(value >> (8 * index));
    }

    /// <summary>이름 필드(+0xc)에서 NUL 앞의 원본 바이트를 돌려준다. 32바이트 안에 NUL 이 없으면 오류다.</summary>
    private byte[] Name(uint owner)
    {
        int start = PlayerBase(owner) + NameOffset;
        int length = 0;
        // NUL 이전의 바이트만 채널 효과로 전달한다.
        while (length < NameWidth && _players[start + length] != 0) length++;
        if (length == NameWidth) throw new InvalidDataException("Player 참여 이름의 NUL 종료가 없습니다.");
        return _players.AsSpan(start, length).ToArray();
    }

    /// <summary>
    /// 지정한 출발 Player 가 도착 Player 를 동맹으로 두었는지(방향별 표 한 칸). 범위 밖 번호는 동맹이 아니다.
    /// 서로 다른 방향은 독립이므로 호출자가 필요한 방향을 직접 고른다.
    /// </summary>
    /// <param name="from">동맹 관계의 출발(관계를 선언한 쪽).</param>
    /// <param name="to">동맹 관계의 도착.</param>
    public bool Allies(int from, int to) =>
        (uint)from < PlayerCount && (uint)to < PlayerCount && _allies[from * PlayerCount + to] != 0;

    /// <summary>두 방향이 모두 동맹인지. 원본 승패 판정(004c36c0)은 이 양방향 조건을 쓴다.</summary>
    /// <param name="first">플레이어.</param>
    /// <param name="second">플레이어.</param>
    public bool MutualAllies(int first, int second) => Allies(first, second) && Allies(second, first);

    /// <summary>Player 의 연결·참여 flags 현재 값(+0x50). 범위 밖이면 0이다.</summary>
    /// <param name="owner">Player 번호.</param>
    public uint Flags(int owner) => (uint)owner < PlayerCount ? Read((uint)owner, FlagsOffset) : 0;

    /// <summary>
    /// 승패·사제 생존 판정이 읽는 참여 조건(flags &amp; 0x22)이 켜져 있는지. 사람의 연결 비트(0x02)나 AI 부착 비트(0x20)다.
    /// 0x01만 보거나 aiName 의 유무로 대신하지 않는다.
    /// </summary>
    /// <param name="owner">Player 번호.</param>
    public bool Participates(int owner) => (Flags(owner) & ParticipationMask) != 0;

    /// <summary>
    /// 현재 부착된 AI brain 토큰(+0xb0). 0이면 없다. 이 클래스는 brain 메모리를 소유하지 않으며
    /// flags 0x20 이나 연결 상태로 토큰을 추정하지 않는다.
    /// </summary>
    /// <param name="owner">Player 번호.</param>
    public uint Brain(uint owner) => Read(owner, BrainOffset);

    /// <summary>
    /// 사람 로그인(004a6dc0): 유효 owner(1~8)에게 연결 비트가 없을 때만 flags = (flags &amp; ~0x100) | 3 으로 쓰고 수락한다.
    /// 로컬 전송에서 대기 연결 번호가 owner 와 같으면 연결 재배치를 생략한다. 이미 로그인했거나 무효 owner 면
    /// 거부 응답 뒤 연결을 해제하고 실패한다. Ready(0x40)나 AI(0x20)는 이 분기와 별개다.
    /// </summary>
    /// <param name="pending">대기 중인 연결 번호.</param>
    /// <param name="owner">로그인하려는 Player 번호.</param>
    /// <returns>수락했으면 true.</returns>
    public bool CheckLogin(int pending, int owner)
    {
        if (owner > 0 && owner < PlayerCount && (Read((uint)owner, FlagsOffset) & ConnectedFlag) == 0)
        {
            uint player = (uint)owner;
            Put(player, FlagsOffset, (Read(player, FlagsOffset) & ~ReleasedFlag) | 3U);
            if (!(_state.LocalTransport && pending == owner)) Require(_hooks.AssignConnection)(pending, owner);
            return true;
        }
        Require(_hooks.RejectLogin)(pending);
        Require(_hooks.Disconnect)(pending);
        return false;
    }

    /// <summary>
    /// 한 Player 의 동맹 설정(00490ea0): 자기 비트를 더한 마스크를 +0x54 와 해당 행 9칸(각 0/1)에만 쓴다.
    /// 다른 행은 건드리지 않아 방향이 독립이다. Player 번호를 알고 있으면 Chat 채널을 지우고
    /// 현재 로컬 번호·표·숨김 값을 다시 읽으며 원본 순서로 재등록한다(양방향 동맹이면 0x40, 숨김이면 0x80).
    /// </summary>
    /// <param name="owner">행의 주인(관계를 선언하는 Player).</param>
    /// <param name="mask">동맹으로 둘 Player 들의 비트 집합.</param>
    public void SetAlliance(uint owner, uint mask)
    {
        PlayerBase(owner);
        mask |= 1U << (int)(owner & 31);
        Put(owner, AllyMaskOffset, mask);
        // 원본은 행의 9개 DWORD 를 각각 0/1로 갱신한다.
        for (uint target = 0; target < PlayerCount; target++) _allies[owner * PlayerCount + target] = (mask >> (int)target) & 1;
        if (_state.PlayerKnown == 0) return;
        Require(_hooks.ClearChat)(0x40);
        Require(_hooks.ClearChat)(0x80);
        // 채널 효과 뒤 현재 로컬 번호/표/Player 를 다시 읽어 다음 Player 를 처리한다.
        for (uint player = 0; player < PlayerCount; player++)
        {
            if (_state.LocalOwner >= PlayerCount) throw new InvalidDataException("Player 참여 Chat 의 로컬 번호가 범위를 벗어났습니다.");
            if (_allies[_state.LocalOwner * PlayerCount + player] != 0 && _allies[player * PlayerCount + _state.LocalOwner] != 0)
                Require(_hooks.AddChat)(Read(player, SubscriberOffset), Name(player), 0x40);
            if (_state.Metamap) Require(_hooks.Assertion)(PlayerParticipationAssertion.MetamapMode);
            if (_hidden[player] != 0) Require(_hooks.AddChat)(Read(player, SubscriberOffset), Name(player), 0x80);
        }
    }

    /// <summary>
    /// 한 방향 동맹 추가(0048f6a0): 지정한 출발 Player 의 마스크 한 비트와 표 한 칸만 켠다. 반대 방향은 바꾸지 않는다.
    /// 전투 모드이면 현재 마스크를 서버 9번 메시지로 요청한다.
    /// </summary>
    /// <param name="owner">관계를 선언하는 Player(출발).</param>
    /// <param name="target">동맹으로 추가할 Player(도착).</param>
    public void AddAlly(uint owner, uint target)
    {
        PlayerBase(owner);
        PlayerBase(target);
        Put(owner, AllyMaskOffset, Read(owner, AllyMaskOffset) | (1U << (int)(target & 31)));
        if (_state.Metamap) Require(_hooks.Assertion)(PlayerParticipationAssertion.MetamapMode);
        bool battle = _state.Battle;
        _allies[owner * PlayerCount + target] = 1;
        if (!battle) return;
        if (_state.Metamap) Require(_hooks.Assertion)(PlayerParticipationAssertion.MetamapMode);
        Require(_hooks.SendAlliance)(9, owner, Read(owner, AllyMaskOffset));
    }

    /// <summary>
    /// 실제 생성 소유자가 넘긴 불투명 brain 토큰을 +0xb0 에 쓰고 flags 0x20 을 켠다(004911a0). 기존 brain 이 있으면
    /// 진단만 보고하고 덮어쓴다(임의로 해제하지 않는다). 구독자가 1 미만이면 owner + 10000 을 주고 연결 정보 30바이트를 지운다.
    /// 0 입력도 원본과 같은 필드 쓰기를 한다.
    /// </summary>
    /// <param name="owner">Player 번호.</param>
    /// <param name="brain">실제 AI 소유자가 만든 불투명 토큰.</param>
    public void AttachBrain(uint owner, uint brain)
    {
        if (Read(owner, BrainOffset) != 0) Require(_hooks.Assertion)(PlayerParticipationAssertion.BrainAlreadyAssigned);
        Put(owner, BrainOffset, brain);
        Put(owner, FlagsOffset, Read(owner, FlagsOffset) | BrainFlag);
        if (unchecked((int)Read(owner, SubscriberOffset)) >= 1) return;
        if (_state.Metamap) Require(_hooks.Assertion)(PlayerParticipationAssertion.MetamapMode);
        Put(owner, SubscriberOffset, owner + SyntheticSubscriberBase);
        int start = PlayerBase(owner);
        // 원본은 DWORD 7개와 마지막 WORD 를 써 +0x30..+0x4d 의 30바이트만 지운다.
        for (int offset = ConnectionInfoOffset; offset < ConnectionInfoEnd; offset++) _players[start + offset] = 0;
    }

    /// <summary>
    /// 현재 brain 이 있으면 소멸자 → 메모리 반납 → +0xb0 = 0 순서로 해제한다(00413ae0). 두 콜백이 끝나기 전에는
    /// Player 에서 이전 토큰이 조회된다. flags 0x20 은 원본대로 지우지 않는다.
    /// </summary>
    /// <param name="owner">Player 번호.</param>
    public void RemoveBrain(uint owner)
    {
        uint brain = Brain(owner);
        if (brain == 0) return;
        Require(_hooks.DestructBrain)(brain);
        Require(_hooks.FreeBrain)(brain);
        Put(owner, BrainOffset, 0);
    }

    /// <summary>플레이어 1~8 의 현재 brain 을 각 방문 시점에 새로 읽어 모두 해제한다(00413b20). 중립 0 은 대상이 아니다.</summary>
    public void RemoveAllBrains()
    {
        // 실제 owner 1~8 의 현재 포인터를 각 방문 시점에 읽는다.
        for (uint owner = 1; owner < PlayerCount; owner++) RemoveBrain(owner);
    }
}
