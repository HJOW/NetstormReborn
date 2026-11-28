using System.Buffers.Binary;
using Netstorm.Core.Original;

// 실제 사제 목록·공간 해시·현재 CS를 장면 음악의 두 월드 조회에 빌리는 부분이다.
namespace Netstorm.Client.Audio;

/// <summary>PATCH00449220/0042cbd0의 두 조회 본체다. 실제 Surface 분배기를 빌리며 장면 감독의 생성·실행은 별도 부모가 맡는다.</summary>
public sealed class SceneMusicWorldQueries
{
    /// <summary>00422230이0041d7a0에 넘기는 실제 단정도 거의올림 상수다.</summary>
    private const float SnapBias = 0.99999F;
    /// <summary>희생 조회의 실제 탐색 반경이며 한 점 Rune/GenusAt와 다르다.</summary>
    private const int Radius = 3;
    /// <summary>현재 TYPE에서 찾는 원문 altar genus다. 고정 TYPE165를 강제하지 않는다.</summary>
    private const uint AltarGenus = 0x400000;
    /// <summary>같은 현재50BYTE 슬롯 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>같은 현재 네 단계 공간 해시다.</summary>
    private readonly RawSquidHash _hash;
    /// <summary>교체된 현재 TYPE 레코드도 조회하는 실제 장기 표 뷰다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>목록 순서와 ownerBYTE/dead비트를 읽는 기존 원문 몸체다.</summary>
    private readonly RawPriestLookup _priests;
    /// <summary>같은 대상의 실제 가상+80을 분배할 필수 장치다. Dais로 무조건 대신하지 않는다.</summary>
    private readonly Func<int, uint> _virtualCount;
    /// <summary>원문 첫 관문의 현재 Challenge 모드다.</summary>
    private readonly Func<bool> _challenge;
    /// <summary>Challenge 관문을 통과한 뒤 읽는 현재 전체 PlayerKnown DWORD다.</summary>
    private readonly Func<uint> _playersReady;
    /// <summary>두 관문을 통과한 뒤 읽는 현재 소유자다.</summary>
    private readonly Func<uint> _localPlayer;
    /// <summary>소유자 조회가 돌아온 뒤 현재 슬롯에서 다시 빌리는 실제 CS 저장소다.</summary>
    private readonly ClientChallengeGlobals _cs;

    /// <summary>실제 owner를 빌리기만 한다. TYPE적재·목록초기화·CS생성·시계/난수/음악 호출은 하지 않는다.</summary>
    public SceneMusicWorldQueries(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types,
        RawPriestLookup priests, Func<int, uint> virtualCount, Func<bool> challenge,
        Func<uint> playersReady, Func<uint> localPlayer, ClientChallengeGlobals cs)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(hash); ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(priests); ArgumentNullException.ThrowIfNull(virtualCount); ArgumentNullException.ThrowIfNull(challenge);
        ArgumentNullException.ThrowIfNull(playersReady); ArgumentNullException.ThrowIfNull(localPlayer); ArgumentNullException.ThrowIfNull(cs);
        if (pool.Edition != OriginalEdition.Patch1078 || !ReferenceEquals(pool, priests.Pool()))
            throw new ArgumentException("장면 음악 조회의 동일 PATCH 풀 계약", nameof(priests));
        _pool = pool; _hash = hash; _types = types; _priests = priests; _virtualCount = virtualCount;
        _challenge = challenge; _playersReady = playersReady; _localPlayer = localPlayer; _cs = cs;
    }

    /// <summary>실제 Game의 같은 Surface와 기존 CS를 빌리고 현재 가상표 분배기를 연결한다. 생성은 조회·파일·시계·적재를 실행하지 않는다.</summary>
    public static SceneMusicWorldQueries ForCurrentSurface(ClientSurfaceGlobals surface, ClientChallengeGlobals cs)
    {
        ArgumentNullException.ThrowIfNull(surface); ArgumentNullException.ThrowIfNull(cs);
        var counts = new SceneMusicVirtualCount(surface);
        return ForCurrentSurface(surface, cs, counts.Count);
    }

    /// <summary>같은 실제 Surface와 현재 Player를 명시 가상 경계에 연결한다. 실제 Game은 위 두 인자 경로로 같은 Surface 분배기를 선택한다.</summary>
    public static SceneMusicWorldQueries ForCurrentSurface(ClientSurfaceGlobals surface, ClientChallengeGlobals cs, Func<int, uint> virtualCount)
    {
        ArgumentNullException.ThrowIfNull(surface); ArgumentNullException.ThrowIfNull(cs); ArgumentNullException.ThrowIfNull(virtualCount);
        RawSurfaceStorage storage = surface.Storage;
        var priests = new RawPriestLookup(storage.Pool, new PriestPostPopState { SharedPriests = storage.SidLists.Priests });
        return new(storage.Pool, storage.Hash, storage.Types, priests, virtualCount, () => storage.OwnerMode.Challenge,
            () => surface.Gameplay.Player.PlayerKnown, () => surface.Gameplay.Player.BattleOwnerBox.Value, cs);
    }

    /// <summary>정상 signed좌표의 거의올림→float저장→재절삭이다. 손상/무한 좌표는 호스트 주소 계산 전에 거부한다.</summary>
    private static int Center(float value)
    {
        double first = (double)value + SnapBias;
        if (!double.IsFinite(first) || first < int.MinValue || first >= 2147483648.0)
            throw new ArgumentOutOfRangeException(nameof(value), "장면 음악 altar 좌표 절삭 범위");
        float stored = (float)(int)first;
        if (stored < int.MinValue || stored >= 2147483648.0)
            throw new ArgumentOutOfRangeException(nameof(value), "장면 음악 altar 좌표 재절삭 범위");
        return (int)stored;
    }

    /// <summary>004b2140의 반경3/flags1 일반 finder에서 현재 genus의 첫 후보를 반환한다. free/dead/owner 필터는 추가하지 않는다.</summary>
    private int AltarAt(float x, float y)
    {
        int cx = Center(x), cy = Center(y);
        var finder = new RawSquidFinder(_pool, _hash, _types);
        var area = new SquidSearchArea(unchecked(cx - Radius), unchecked(cy - Radius), unchecked(cx + Radius), unchecked(cy + Radius));
        // 기존 finder가 buried만 제외하며 다음 체인을 먼저 확보한 뒤 현재 TYPE을 읽는다.
        for (int sid = finder.Begin(area, 1); sid != 0; sid = finder.Next())
        {
            uint type = _pool.Slot(sid)[10];
            if (type >= _types.Count) throw new ArgumentOutOfRangeException(nameof(sid), "장면 음악 altar 현재 TYPE 범위");
            if ((_types[(int)type].Flags2 & AltarGenus) != 0) return sid;
        }
        return 0;
    }

    /// <summary>00449220: 첫 살아있는 소유 사제→반경3 altar→프레임 선저장→가상Count→단계 비영이다. PlayerKnown은 이 함수의 사전조건이 아니다.</summary>
    public bool Sacrificing(uint owner)
        => SacrificingBody(_pool, owner, _priests.Find, AltarAt, _virtualCount);

    /// <summary>00449220의 순수 상위다. 독립 원본 replay의 명시 하위 입력과 실제 adapter가 같은 본체를 사용한다.</summary>
    internal static bool SacrificingBody(RawSidPool pool, uint owner, Func<uint, uint> findPriest,
        Func<float, float, int> findAltar, Func<int, uint> virtualCount)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(findPriest);
        ArgumentNullException.ThrowIfNull(findAltar); ArgumentNullException.ThrowIfNull(virtualCount);
        uint priest = findPriest(owner);
        if (priest == 0) return false;
        ReadOnlySpan<byte> raw = pool.Slot(checked((int)priest));
        int altar = findAltar(BinaryPrimitives.ReadSingleLittleEndian(raw[14..]), BinaryPrimitives.ReadSingleLittleEndian(raw[18..]));
        if (altar == 0) return false;
        int frame = BinaryPrimitives.ReadInt32LittleEndian(pool.Slot(altar)[36..]);
        uint count = virtualCount(altar);
        return RawTempleRitual.StageFromCount(OriginalEdition.Patch1078, frame, count) != 0;
    }

    /// <summary>0042cbd0: Challenge→현재Known→현재소유자→그 뒤 현재 CS 참가자+a4 DWORD를 읽는다. CS0/미등록은 성공false로 숨기지 않는다.</summary>
    public bool WaitingRoom()
        => WaitingRoomBody(_challenge, _playersReady, _localPlayer, _cs);

    /// <summary>0042cbd0의 순수 상위다. 실제CS를 빌리며 원문 replay에서도 아래 경계의 순서만 입력한다.</summary>
    internal static bool WaitingRoomBody(Func<bool> challenge, Func<uint> playersReady,
        Func<uint> localPlayer, ClientChallengeGlobals cs)
    {
        ArgumentNullException.ThrowIfNull(challenge); ArgumentNullException.ThrowIfNull(playersReady);
        ArgumentNullException.ThrowIfNull(localPlayer); ArgumentNullException.ThrowIfNull(cs);
        if (!challenge() || playersReady() == 0) return false;
        uint owner = localPlayer();
        return BinaryPrimitives.ReadUInt32LittleEndian(cs.Participant(owner).Span[0xa4..]) != 0;
    }
}
