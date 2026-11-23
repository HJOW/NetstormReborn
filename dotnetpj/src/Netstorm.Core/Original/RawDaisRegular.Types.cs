// Dais 의식이 공유하는 원본 전역·요청 종류와 생략할 수 없는 하위 효과 경계를 둔다.
namespace Netstorm.Core.Original;

/// <summary>RawDaisRegular.h의 기존 의식/시간/호스트 전역을 공유하며 player 필드를 매 효과 뒤 현재 값으로 읽는다.</summary>
public sealed class DaisRegularState
{
    /// <summary>같은 boss·priestType·RuneType을 TempleRitual과 공유한다.</summary>
    public TempleRitualState Ritual { get; }
    /// <summary>같은 altarType·daisType·억제 전역을 Rune 조회와 공유한다.</summary>
    public RunePostPopState Rune { get; }
    /// <summary>공유 Regular의 현재 게임 시각이다. 소리 벽시계와 구분한다.</summary>
    public SquidProcessState Clock { get; }
    /// <summary>기존 SP 저장 전역이다. CD는 정확한 Get/Set 분기를 사용한다.</summary>
    public SquidRewardState Sp { get; }
    /// <summary>현재 로컬 플레이어 번호다. callbacks 뒤 다시 읽는다.</summary>
    public uint LocalOwner { get; set; }
    /// <summary>희생 코드표의 현재 타입 전역이다.</summary>
    public uint SacrificeType { get; set; } = 169;
    /// <summary>획득 SP의 현재 타입 전역이다.</summary>
    public uint NuggetType { get; set; } = 154;
    /// <summary>로컬 특수 분기의 현재 타입 전역이다.</summary>
    public uint NoIslandType { get; set; } = 93;
    /// <summary>원본 방향별9×9 동맹표다.</summary>
    public int[] Alliances { get; } = new int[81];
    /// <summary>patchPlayer+a0/CDPlayer+98의 마지막 개수/획득 타입이다.</summary>
    public uint[] PlayerResults { get; } = new uint[9];
    /// <summary>획득 시 지우는 patchPlayer+71/CDPlayer+69 BYTE다.</summary>
    public byte[] GrantFlags { get; } = new byte[9];
    /// <summary>로컬 획득 타입 선택이다.</summary>
    public uint SelectedType { get; set; }
    /// <summary>희생 종료에서 멈추는 전역 cue다.</summary>
    public uint VoiceCue { get; set; }
    /// <summary>패치의 사제 genus/깊이/경로 생성 assert를 확인한다. CD는 읽지 않는다.</summary>
    public RawCheckingFlag Debug { get; set; } = new RawCheckingFlag();
    /// <summary>실제9 Player 기록이며 비어 있으면 독립 원본 검사의 기존 배열을 사용한다.</summary>
    public byte[]? PlayerRecords { get; private set; }
    /// <summary>기록의0xb4/0xac 및 필드 폭을 판본대로 해석한다.</summary>
    public OriginalEdition PlayerEdition { get; private set; } = OriginalEdition.Patch1078;
    /// <summary>실제005412c0의 장기 owner이며 선택/획득 종류가 같은 번호를 읽는다.</summary>
    public UIntBox? SharedNuggetType { get; set; }
    /// <summary>의식 요청처럼 참조를 보관하는 소비자에 임시 값을 반환하지 않는다.</summary>
    public uint NuggetTypeCurrent() => SharedNuggetType?.Value ?? NuggetType;
    /// <summary>공통 WORD setter가 읽는005411a0/CD0051ca8c이며 독립 입력은 실제 초기82다.</summary>
    public uint BridgeType { get; set; } = 82;
    /// <summary>같은 Client005411a0의 장기 주소다. 독립 입력은 현재 값을 쓴다.</summary>
    public UIntBox? CurrentBridgeType { get; set; }
    /// <summary>WORD 저장 뒤 같은 현재 다리 번호를 읽는다.</summary>
    public uint BridgeTypeCurrent() => CurrentBridgeType?.Value ?? BridgeType;
    /// <summary>공유 원본 전역의 참조다. 참조 대상은 이 객체보다 오래 살아야 한다.</summary>
    public DaisRegularState(TempleRitualState ritual, RunePostPopState rune, SquidProcessState clock, SquidRewardState sp, uint localOwner)
    {
        Ritual = ritual; Rune = rune; Clock = clock; Sp = sp; LocalOwner = localOwner;
    }
    /// <summary>실제 Player 기록은 장기 주소를 빌리며 판본별 정확한9기록 크기를 요구한다.</summary>
    public void BindPlayerRecords(OriginalEdition edition, byte[] records)
    {
        ArgumentNullException.ThrowIfNull(records);
        if (records.Length != 9 * (edition == OriginalEdition.Patch1078 ? 0xb4 : 0xac)) throw new ArgumentException("다이스 실제 Player 기록 크기 오류", nameof(records));
        PlayerRecords = records; PlayerEdition = edition;
    }
    /// <summary>독립 입력과 실제 세션 기록을 같은 현재 조회 규칙으로 읽는다. 현재+71/CD+69 BYTE를 효과 뒤 다시 읽는다.</summary>
    public byte GrantFlag(uint owner)
    {
        if (owner > 8) throw new ArgumentOutOfRangeException(nameof(owner), "다이스 grant Player 범위");
        byte[]? records = PlayerRecords;
        return records is null ? GrantFlags[owner] : records[owner * (PlayerEdition == OriginalEdition.Patch1078 ? 0xb4 : 0xac) + (PlayerEdition == OriginalEdition.Patch1078 ? 0x71 : 0x69)];
    }
    /// <summary>Count가 재진입해 기록을 바꿔도 앞에서 선택한 실제 BYTE 주소를 유지한다.</summary>
    public void SetGrantFlagSlot(uint owner, byte value)
    {
        if (owner > 8) throw new ArgumentOutOfRangeException(nameof(owner), "다이스 grant 쓰기 Player 범위");
        byte[]? records = PlayerRecords;
        if (records is null) GrantFlags[owner] = value;
        else records[owner * (PlayerEdition == OriginalEdition.Patch1078 ? 0xb4 : 0xac) + (PlayerEdition == OriginalEdition.Patch1078 ? 0x71 : 0x69)] = value;
    }
    /// <summary>같은 현재 BYTE만 쓰고 다른 필드를 보존한다.</summary>
    public void SetGrantFlag(uint owner, byte value) => SetGrantFlagSlot(owner, value);
    /// <summary>PATCH 가상 Count 앞에서 Player 결과 DWORD의 실제 네 바이트 주소를 저장한다.</summary>
    public Span<byte> PlayerResultSlot(uint owner)
    {
        if (owner > 8) throw new ArgumentOutOfRangeException(nameof(owner), "다이스 결과 Player 범위");
        byte[]? records = PlayerRecords;
        if (records is null) return System.Runtime.InteropServices.MemoryMarshal.AsBytes(PlayerResults.AsSpan((int)owner, 1));
        int at = (int)owner * (PlayerEdition == OriginalEdition.Patch1078 ? 0xb4 : 0xac) + (PlayerEdition == OriginalEdition.Patch1078 ? 0xa0 : 0x98);
        return records.AsSpan(at, 4);
    }
    /// <summary>현재 기록으로 쓰는 기존 API이며 Native CD의 Count 복귀 뒤 저장소 재조회를 유지한다.</summary>
    public void SetPlayerResult(uint owner, uint value)
    {
        Span<byte> destination = PlayerResultSlot(owner);
        for (int index = 0; index < 4; index++) destination[index] = (byte)(value >> (8 * index));
    }
}

/// <summary>원본 희생 안내의 요청 종류다. 문장으로 추정하지 않는다.</summary>
public enum DaisChatKind
{
    /// <summary>실제 AltarUpgrade 요청이다.</summary>
    Upgrade = 0,
    /// <summary>실제 Sacrifice/판본별 희생 안내 요청이다.</summary>
    Stormpower = 1,
    /// <summary>로컬 사제 사망 안내다.</summary>
    LocalPriestDead = 2,
    /// <summary>다른 사제 사망 안내다.</summary>
    OtherPriestDead = 3,
}

/// <summary>RawDaisRegular.h의 원본 chat 인자다. 요청 종류/현재 owner/원래 owner/상태값으로 넘긴다.</summary>
public sealed class DaisChat
{
    /// <summary>실제 호출된 요청 종류다.</summary>
    public DaisChatKind Kind { get; set; }
    /// <summary>저장한 altar 원래owner다.</summary>
    public uint OriginalOwner { get; set; }
    /// <summary>요청 시 현재 owner다.</summary>
    public uint CurrentOwner { get; set; }
    /// <summary>상태 helper 반환값이다.</summary>
    public uint Status { get; set; }
}

/// <summary>RawDaisRegular.h의 생략할 수 없는 하위 효과다. 실제 기본 몸체는 후속에서 연결하고 UI/장치 경계는 caller가 공급한다.</summary>
public sealed class DaisRegularHooks
{
    /// <summary>같은 SquidFrame.Set의 실제 프레임/공간/표시 갱신이다.</summary>
    public Action<int, int, uint>? SetFrame { get; set; }
    /// <summary>실제 파생 가상+10이며 Rune48/포획 내용물400048을 구분한다.</summary>
    public Action<int, uint>? Destroy { get; set; }
    /// <summary>collapse/sacrifice/altar 이탈의 new0x28 성공 여부다.</summary>
    public Func<bool>? ReserveShared { get; set; }
    /// <summary>같은 Host의305/306/24 예약이다.</summary>
    public Action<int, uint, float>? AddShared { get; set; }
    /// <summary>itIsDone2/altarBurnCollapse 위치 음향이다.</summary>
    public Action<float, float, string, uint>? PlayAt { get; set; }
    /// <summary>각 지연 소리 프로세스의 독립 new0x28 성공 여부다.</summary>
    public Func<bool>? ReserveSound { get; set; }
    /// <summary>원본 이름/0/0xc의 실제 소리 조회다.</summary>
    public Func<string, uint, uint, uint>? LoadSound { get; set; }
    /// <summary>같은 SoundProcessSystem.Add이며 실패는0이다.</summary>
    public Func<int, uint, uint, uint, ulong>? AttachSound { get; set; }
    /// <summary>지연 소리에만 사용하는 실제 벽시계00460d70/CD004011b0이다.</summary>
    public Func<double>? WallSeconds { get; set; }
    /// <summary>붙인 소리 프로세스 +20의 실제 지연 시각을 쓴다.</summary>
    public Action<ulong, double>? SetSoundStart { get; set; }
    /// <summary>희생 종료의 전역 cue/0/0 정지 요청이다.</summary>
    public Action<uint, uint, uint>? StopVoice { get; set; }
    /// <summary>기존 공유 client Path mapper를 실제 현재 타입에 적용한다.</summary>
    public Func<uint, uint>? ProcessType { get; set; }
    /// <summary>마지막 점 교체 뒤 Prepare가 읽는 현재 speed/turningSpeed다.</summary>
    public Func<int, RawPathStartType>? PathMetadata { get; set; }
    /// <summary>같은 기존 DamageableHandle 전체 몸체다.</summary>
    public RegularHandler? Damageable { get; set; }
    /// <summary>패치Reward.AddSp, CD Get/Set/표시의 정확한 local5000이다.</summary>
    public Action<float>? AddLocalSp { get; set; }
    /// <summary>패치의 비로컬 너겟 획득은 현재 SP+5000을 절삭해 실제 정책 setter에 넘긴다.</summary>
    public Action<uint>? NormalizeForeignSp { get; set; }
    /// <summary>패치만 별도9×16 Core 장부의 owner 기술 비트를 켠다.</summary>
    public Action<uint, uint>? GrantForeignType { get; set; }
    /// <summary>noIsland 획득 시 로컬의 실제 특수 요청을 원본 인수0으로 호출한다.</summary>
    public Action<uint>? LocalSpecial { get; set; }
    /// <summary>나머지 로컬 타입 획득의 실제 선택 초기화를 수행한다.</summary>
    public Action<uint>? LocalType { get; set; }
    /// <summary>희생 종료에서 현재1..8 플레이어의 실제 갱신을 순서대로 호출한다.</summary>
    public Action<uint>? RefreshOwner { get; set; }
    /// <summary>현재 로컬의 Core 갱신→CoreData 기록→현재 맵 snapshot이며 단순 화면 갱신이 아니다.</summary>
    public Action? RefreshLocal { get; set; }
    /// <summary>희생 후 SP 안내가 현재 각 플레이어의 실제 사제 존재를 읽는다.</summary>
    public Func<uint, bool>? PriestExists { get; set; }
    /// <summary>원본의 희생 안내 상태 helper다. 패치/CD의 차이를 실제 caller가 연결한다.</summary>
    public Func<uint>? MultiplayerStatus { get; set; }
    /// <summary>원본 문자열 key/판본별 인자의 실제 UI 소비자다.</summary>
    public Action<DaisChat>? Chat { get; set; }
    /// <summary>현재 checking이 선택한 Dais/Carrier 원문 진단은 복귀 뒤 본문을 계속한다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
    /// <summary>진단/하위 효과 뒤 현재 Client 권한을 읽으며 독립 입력은 기존 ritual 상태를 사용한다.</summary>
    public Func<bool>? Boss { get; set; }
    /// <summary>실제 현재 동맹표의 행/열을 읽는다. 독립 입력은 기존 배열을 사용한다.</summary>
    public Func<uint, uint, int>? Alliance { get; set; }
    /// <summary>원문에서 확인된 자기 가상80 호출만 현재 Session 분배기에 위임한다.</summary>
    public Func<int, uint>? Count { get; set; }
    /// <summary>선택한 원문 위치에서 현재 canonical 사제 종류를 읽으며 독립 입력은 기존 ritual 상태를 사용한다.</summary>
    public Func<uint>? PriestType { get; set; }
    /// <summary>희생 완료의 공통 WORD 지정이 현재 다리 타입에 해당할 때만 호출한다.</summary>
    public Action<int>? NotifySurface { get; set; }
    /// <summary>효과 묶음의 얕은 사본을 만든다. 외부 재연결을 캡처된 장치와 구별한다.</summary>
    internal DaisRegularHooks Copy() => (DaisRegularHooks)MemberwiseClone();
}
